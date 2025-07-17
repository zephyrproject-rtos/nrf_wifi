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
from jinja2 import Environment, FileSystemLoader
from typing import Dict, Any, List
from collections import namedtuple

WIFI_FW_BIN_NAME: str = "nrf70.bin"

# Paths are relative to the sdk-nrfxlib repository
BlobInfo = namedtuple(
    "BlobInfo", ["name", "description", "version", "rpath", "lpath", "docpath"]
)


def parse_version_from_binary(binary_data: bytes) -> str:
    """
    Parse version from firmware binary.
    Version is stored at bytes 8-12, e.g., 02 0e 02 01 -> 1.2.14.2
    """
    if len(binary_data) < 12:
        logger.warning("Binary too short to extract version, using default")
        return "1.0.0"

    # Extract version bytes (positions 8-12)
    version_bytes = binary_data[8:12]

    # Convert to version string: 02 0e 02 01 -> 1.2.14.2
    version_parts = []
    for byte in version_bytes:
        version_parts.append(str(byte))

    version = ".".join(version_parts)
    logger.debug(f"Extracted version from binary: {version}")
    return version


def get_wifi_blob_info(name: str) -> BlobInfo:
    return BlobInfo(
        name,
        f"nRF70 Wi-Fi firmware for {name} mode",
        "1.0.0",  # This will be overridden by actual binary parsing
        f"nrf_wifi/bin/zephyr/{name}/{WIFI_FW_BIN_NAME}",
        f"wifi_fw_bins/{name}/{WIFI_FW_BIN_NAME}",
        f"https://docs.nordicsemi.com/bundle/ps_nrf7000/page/chapters/notice/doc/notice_on_sw.html",
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


def compute_sha256(url: str) -> str:
    response = requests.get(url)
    response.raise_for_status()
    sha256_hash: str = hashlib.sha256(response.content).hexdigest()
    return sha256_hash


def render_template(template_path: str, output_path: str, latest_sha: str) -> None:
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
        response = requests.get(blob_info["url"])
        response.raise_for_status()
        binary_data = response.content

        blob_info["sha256"] = hashlib.sha256(binary_data).hexdigest()
        blob_info["description"] = blob.description

        # Parse version from the actual binary
        blob_info["version"] = parse_version_from_binary(binary_data)

        blobs[blob.name] = blob_info

    logger.debug(blobs)
    # Render the template with the provided context
    rendered_content: str = template.render(blobs=blobs, latest_sha=latest_sha)

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
        required=True,
        help="The latest commit SHA for the nrfxlib repository.",
    )
    parser.add_argument(
        "-d", "--debug", action="store_true", help="Enable debug logging."
    )

    args: argparse.Namespace = parser.parse_args()

    if args.debug:
        logger.setLevel(logging.DEBUG)

    # Render the template
    render_template(args.template, args.output, args.commit)


if __name__ == "__main__":
    main()
