#!/usr/bin/env python3
#
# Copyright (c) 2024, Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

"""
This script generates a module.yml file for the Zephyr project. The module.yml file contains
information about the blobs. The script computes the SHA-256 hash for each blob and renders
the Jinja2 template with the blob information.
"""

import argparse
import hashlib
import requests
import logging
import os
from jinja2 import Environment, FileSystemLoader
from typing import Dict, Any, List, Optional
from collections import namedtuple

WIFI_FW_BIN_NAME: str = "nrf70.bin"

# Paths are relative to the sdk-nrfxlib repository
BlobInfo = namedtuple(
    "BlobInfo", ["name", "description", "version", "rpath", "lpath", "docpath"]
)


def parse_version_from_binary(binary_data: bytes) -> str:
    """
    Parse version from firmware binary.
    Version format: FAMILY.MAJOR.MINOR.PATCH
    Based on patch_info.h: RPU_FAMILY, RPU_MAJOR_VERSION, RPU_MINOR_VERSION, RPU_PATCH_VERSION
    """
    if len(binary_data) < 12:
        logger.warning("Binary too short to extract version, using default")
        return "1.0.0"

    # Extract version bytes (positions 8-11)
    # Display format: FAMILY.MAJOR.MINOR.PATCH
    # Bytes are stored in reverse order due to endianness: patch.min.maj.fam
    # Example: 33 0d 02 01 -> 1.1.2.51 (FAMILY.MAJOR.MINOR.PATCH)
    patch = binary_data[8]    # byte 8 = patch version (RPU_PATCH_VERSION) - 0x33 = 51
    minor = binary_data[9]    # byte 9 = minor version (RPU_MINOR_VERSION) - 0x0d = 13
    major = binary_data[10]   # byte 10 = major version (RPU_MAJOR_VERSION) - 0x02 = 2
    family = binary_data[11]  # byte 11 = family (RPU_FAMILY) - 0x01 = 1

    # Display as FAMILY.MAJOR.MINOR.PATCH
    version = f"{family}.{major}.{minor}.{patch}"
    logger.debug(f"Extracted version from binary: {version}")
    return version


def get_wifi_blob_info(name: str) -> BlobInfo:
    return BlobInfo(
        name,
        f"nRF70 Wi-Fi firmware for {name} mode",
        "1.0.0",  # This will be overridden by actual binary parsing
        f"nrf_wifi/bin/zephyr/{name}/{WIFI_FW_BIN_NAME}",
        f"wifi_fw_bins/{name}/{WIFI_FW_BIN_NAME}",
        "https://docs.nordicsemi.com/bundle/ps_nrf7000/page/chapters/notice/doc/notice_on_sw.html",
    )


nordic_blobs: List[BlobInfo] = [
    get_wifi_blob_info("default"),
    get_wifi_blob_info("scan_only"),
    get_wifi_blob_info("radio_test"),
    get_wifi_blob_info("system_with_raw"),
    get_wifi_blob_info("offloaded_raw_tx"),
]

logger: logging.Logger = logging.getLogger(__name__)
logging.basicConfig(level=logging.INFO)


def get_pr_head_commit(pr_number: str) -> str:
    """Get the head commit SHA for a PR from GitHub API"""
    github_token = os.environ.get('GITHUB_TOKEN')
    headers = {}
    if github_token:
        headers['Authorization'] = f'token {github_token}'

    url = f"https://api.github.com/repos/nrfconnect/sdk-nrfxlib/pulls/{pr_number}"

    try:
        response = requests.get(url, headers=headers, timeout=30)
        response.raise_for_status()

        pr_data = response.json()

        # Check if PR exists and is not closed/merged
        if pr_data.get('state') == 'closed':
            logger.warning(f"PR #{pr_number} is closed")

        if pr_data.get('merged'):
            logger.warning(f"PR #{pr_number} is already merged")

        # Get head commit
        head_commit = pr_data['head']['sha']
        if not head_commit:
            raise ValueError(f"PR #{pr_number} has no head commit")

        logger.debug(f"PR #{pr_number} head commit: {head_commit}")
        return head_commit

    except requests.exceptions.RequestException as e:
        if response.status_code == 404:
            raise ValueError(f"PR #{pr_number} not found in nrfconnect/sdk-nrfxlib repository")
        elif response.status_code == 403:
            raise ValueError(f"Access denied to PR #{pr_number}. Check GITHUB_TOKEN permissions")
        else:
            raise ValueError(f"Failed to fetch PR #{pr_number}: {e}")
    except KeyError as e:
        raise ValueError(f"Invalid response format for PR #{pr_number}: missing {e}")
    except Exception as e:
        raise ValueError(f"Unexpected error fetching PR #{pr_number}: {e}")

def compute_sha256(url: str) -> str:
    response = requests.get(url)
    response.raise_for_status()
    sha256_hash: str = hashlib.sha256(response.content).hexdigest()
    return sha256_hash


def render_template(template_path: str, output_path: str, latest_sha: str, is_pr: bool = False, pr_number: Optional[str] = None) -> None:
    # Load the Jinja2 template
    env: Environment = Environment(loader=FileSystemLoader("."))
    template = env.get_template(template_path)

    # list of dictionaries containing blob information
    blobs: Dict[str, Dict[str, Any]] = {}
    # Compute SHA-256 for each blob based on the URL
    for blob in nordic_blobs:
        logger.debug(f"Processing blob: {blob.name}")
        nrfxlib_url = f"https://github.com/nrfconnect/sdk-nrfxlib/raw/{latest_sha}"
        blob_info: Dict[str, Any] = {}
        blob_info["path"] = blob.lpath
        blob_info["rpath"] = blob.rpath
        blob_info["version"] = blob.version
        blob_info["url"] = f"{nrfxlib_url}/{blob.rpath}"
        blob_info["doc_url"] = f"{blob.docpath}"

        # Download the binary to compute SHA-256 and extract version
        try:
            response = requests.get(blob_info["url"], timeout=60)
            response.raise_for_status()
            binary_data = response.content

            blob_info["sha256"] = hashlib.sha256(binary_data).hexdigest()
            blob_info["description"] = blob.description

            # Parse version from the actual binary
            blob_info["version"] = parse_version_from_binary(binary_data)

        except requests.exceptions.RequestException as e:
            logger.error(f"Failed to download blob from {blob_info['url']}: {e}")
            raise ValueError(f"Failed to download blob for {blob.name}: {e}")
        except Exception as e:
             logger.error(f"Unexpected error processing blob {blob.name}: {e}")
             raise ValueError(f"Error processing blob {blob.name}: {e}")

        blobs[blob.name] = blob_info

    logger.debug(blobs)

    # Prepare metadata comment
    metadata_comment = None
    if is_pr and pr_number:
        metadata_comment = f"# Generated from PR #{pr_number} (commit: {latest_sha})"
    else:
        metadata_comment = f"# Generated from commit: {latest_sha}"

    # Render the template with the provided context
    rendered_content: str = template.render(
        blobs=blobs,
        latest_sha=latest_sha,
        metadata_comment=metadata_comment
    )

    # Write the rendered content to the output file
    with open(output_path, "w") as output_file:
        output_file.write(rendered_content)


def main() -> None:
    parser: argparse.ArgumentParser = argparse.ArgumentParser(
        description="Generate a module.yml file for the Zephyr project."
    )
    parser.add_argument(
        "-t",
        "--template",
        default="scripts/module.yml.j2",
        help="Path to the Jinja2 template file.",
    )
    parser.add_argument(
        "-o",
        "--output",
        default="zephyr/module.yml",
        help="Path to the output YAML file.",
    )
    parser.add_argument(
        "-c",
        "--commit",
        help="The commit SHA for the nrfxlib repository (for merged commits).",
    )
    parser.add_argument(
        "-p",
        "--pr",
        help="The PR number for the nrfxlib repository (for unmerged PRs).",
    )
    parser.add_argument(
        "-d", "--debug", action="store_true", help="Enable debug logging."
    )

    args: argparse.Namespace = parser.parse_args()

    if args.debug:
        logger.setLevel(logging.DEBUG)

    # Validate arguments
    if not args.commit and not args.pr:
        parser.error("Either --commit or --pr must be specified")
    if args.commit and args.pr:
        parser.error("Only one of --commit or --pr can be specified")

    # Validate commit format if provided
    if args.commit:
        import re
        if not re.match(r'^[a-fA-F0-9]{7,40}$', args.commit):
            parser.error(f"Invalid commit hash format: {args.commit}. Expected 7-40 hex characters.")

    # Validate PR number if provided
    if args.pr:
        if not args.pr.isdigit() or int(args.pr) <= 0:
            parser.error(f"Invalid PR number: {args.pr}. Expected a positive integer.")

    # Determine the reference to use
    try:
        if args.pr:
            # For PRs, get the head commit from GitHub API
            logger.debug(f"Processing PR #{args.pr}")
            reference = get_pr_head_commit(args.pr)
            is_pr = True
            pr_number = args.pr
        else:
            # For merged commits, use the commit hash directly
            logger.debug(f"Processing commit {args.commit}")
            reference = args.commit
            is_pr = False
            pr_number = None

        # Render the template
        render_template(args.template, args.output, reference, is_pr, pr_number)

    except ValueError as e:
        logger.error(f"Error: {e}")
        exit(1)
    except Exception as e:
        logger.error(f"Unexpected error: {e}")
        exit(1)


if __name__ == "__main__":
    main()
