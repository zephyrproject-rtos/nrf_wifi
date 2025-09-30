#!/usr/bin/env python3
# Copyright (c) 2025, Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0
"""
Parse nRF70 rpu_sys_fw_stats using header file definitions
"""

import struct
import sys
import re
import os
import argparse
import logging

class StructParser:
    def __init__(self, header_file: str, debug: bool = False):
        self.header_file = header_file
        self.structs = {}
        self.debug = debug
        self.parse_header()
    
    def parse_header(self):
        """Parse the header file to extract struct definitions"""
        with open(self.header_file, 'r') as f:
            content = f.read()
        
        # Find all struct definitions
        struct_patterns = [
            'rpu_phy_stats',
            'rpu_lmac_stats', 
            'rpu_umac_stats',
            'rpu_sys_fw_stats',
            'umac_tx_dbg_params',
            'umac_rx_dbg_params',
            'umac_cmd_evnt_dbg_params',
            'nrf_wifi_interface_stats'
        ]
        
        for struct_name in struct_patterns:
            pattern = rf'struct {struct_name}\s*\{{([^}}]+)\}}'
            match = re.search(pattern, content, re.DOTALL)
            if match:
                fields = self.parse_struct_fields(match.group(1))
                self.structs[struct_name] = fields
                logging.debug(f"Found struct {struct_name}: {len(fields)} fields")
    
    def parse_struct_fields(self, struct_body: str):
        """Parse struct body to extract field names and types"""
        fields = []
        lines = struct_body.strip().split('\n')
        
        for line in lines:
            line = line.strip()
            if not line or line.startswith('//') or line.startswith('/*'):
                continue
            
            # Remove comments
            if '//' in line:
                line = line[:line.index('//')]
            if '/*' in line:
                line = line[:line.index('/*')]
            
            # Extract field name (last word before semicolon)
            if ';' in line:
                field_part = line[:line.index(';')].strip()
                parts = field_part.split()
                if len(parts) >= 2:
                    field_name = parts[-1]
                    field_type = ' '.join(parts[:-1])
                    fields.append((field_type, field_name))
        
        return fields
    
    def get_type_format(self, field_type: str):
        """Convert C type to struct format character"""
        type_mapping = {
            'signed char': 'b',
            'unsigned char': 'B', 
            'char': 'b',
            'short': 'h',
            'unsigned short': 'H',
            'int': 'i',
            'unsigned int': 'I',
            'long': 'l',
            'unsigned long': 'L',
            'long long': 'q',
            'unsigned long long': 'Q',
            'float': 'f',
            'double': 'd'
        }
        
        # Handle struct types
        if 'struct' in field_type:
            return None  # Will be handled separately
        
        return type_mapping.get(field_type, 'I')  # Default to unsigned int
    
    def parse_rpu_sys_fw_stats(self, blob_data: bytes, endianness: str = '<'):
        """Parse rpu_sys_fw_stats struct from blob data"""
        logging.debug(f"=== Parsing rpu_sys_fw_stats ===")
        logging.debug(f"Blob size: {len(blob_data)} bytes")
        logging.debug(f"Endianness: {endianness}")
        logging.debug("")
        
        offset = 0
        
        # Parse PHY stats
        if 'rpu_phy_stats' in self.structs:
            phy_fields = self.structs['rpu_phy_stats']
            phy_format = endianness
            
            for field_type, field_name in phy_fields:
                fmt_char = self.get_type_format(field_type)
                if fmt_char:
                    phy_format += fmt_char
                else:
                    phy_format += 'I'  # Default to unsigned int for nested structs
            
            if offset + struct.calcsize(phy_format) <= len(blob_data):
                phy_data = struct.unpack(phy_format, blob_data[offset:offset+struct.calcsize(phy_format)])
                print("PHY stats")
                print("======================")
                for i, (field_type, field_name) in enumerate(phy_fields):
                    if i < len(phy_data):
                        print(f"{field_name}: {phy_data[i]}")
                print()
                offset += struct.calcsize(phy_format)
        
        # Parse LMAC stats
        if 'rpu_lmac_stats' in self.structs:
            lmac_fields = self.structs['rpu_lmac_stats']
            lmac_format = endianness
            
            for field_type, field_name in lmac_fields:
                fmt_char = self.get_type_format(field_type)
                if fmt_char:
                    lmac_format += fmt_char
                else:
                    lmac_format += 'I'  # Default to unsigned int
            
            if offset + struct.calcsize(lmac_format) <= len(blob_data):
                lmac_data = struct.unpack(lmac_format, blob_data[offset:offset+struct.calcsize(lmac_format)])
                print("LMAC stats")
                print("======================")
                for i, (field_type, field_name) in enumerate(lmac_fields):
                    if i < len(lmac_data):
                        print(f"{field_name}: {lmac_data[i]}")
                print()
                offset += struct.calcsize(lmac_format)
        
        # Parse UMAC stats (nested structs within rpu_umac_stats)
        # rpu_umac_stats contains: tx_dbg_params, rx_dbg_params, cmd_evnt_dbg_params, interface_data_stats
        
        # Parse UMAC TX debug params
        if 'umac_tx_dbg_params' in self.structs:
            tx_fields = self.structs['umac_tx_dbg_params']
            tx_format = endianness + 'I' * len(tx_fields)  # All unsigned int
            
            if offset + struct.calcsize(tx_format) <= len(blob_data):
                tx_data = struct.unpack(tx_format, blob_data[offset:offset+struct.calcsize(tx_format)])
                print("UMAC TX debug stats")
                print("======================")
                for i, (field_type, field_name) in enumerate(tx_fields):
                    if i < len(tx_data):
                        print(f"{field_name}: {tx_data[i]}")
                print()
                offset += struct.calcsize(tx_format)
        
        # Parse UMAC RX debug params
        if 'umac_rx_dbg_params' in self.structs:
            rx_fields = self.structs['umac_rx_dbg_params']
            rx_format = endianness + 'I' * len(rx_fields)  # All unsigned int
            
            if offset + struct.calcsize(rx_format) <= len(blob_data):
                rx_data = struct.unpack(rx_format, blob_data[offset:offset+struct.calcsize(rx_format)])
                print("UMAC RX debug stats")
                print("======================")
                for i, (field_type, field_name) in enumerate(rx_fields):
                    if i < len(rx_data):
                        print(f"{field_name}: {rx_data[i]}")
                print()
                offset += struct.calcsize(rx_format)
        
        # Parse UMAC control path stats
        if 'umac_cmd_evnt_dbg_params' in self.structs:
            cmd_fields = self.structs['umac_cmd_evnt_dbg_params']
            cmd_format = endianness + 'I' * len(cmd_fields)  # All unsigned int
            
            if offset + struct.calcsize(cmd_format) <= len(blob_data):
                cmd_data = struct.unpack(cmd_format, blob_data[offset:offset+struct.calcsize(cmd_format)])
                print("UMAC control path stats")
                print("======================")
                for i, (field_type, field_name) in enumerate(cmd_fields):
                    if i < len(cmd_data):
                        print(f"{field_name}: {cmd_data[i]}")
                print()
                offset += struct.calcsize(cmd_format)
        
        # Parse interface stats
        if 'nrf_wifi_interface_stats' in self.structs:
            iface_fields = self.structs['nrf_wifi_interface_stats']
            iface_format = endianness + 'I' * len(iface_fields)  # All unsigned int
            
            if offset + struct.calcsize(iface_format) <= len(blob_data):
                iface_data = struct.unpack(iface_format, blob_data[offset:offset+struct.calcsize(iface_format)])
                print("UMAC interface stats")
                print("======================")
                for i, (field_type, field_name) in enumerate(iface_fields):
                    if i < len(iface_data):
                        print(f"{field_name}: {iface_data[i]}")
                print()
                offset += struct.calcsize(iface_format)
        
            # Show remaining bytes
            remaining = len(blob_data) - offset
            if remaining > 0:
                logging.debug(f"Remaining data: {remaining} bytes")
                logging.debug(f"Data: {blob_data[offset:].hex()[:100]}...")
                logging.debug("")
                logging.debug("=== Debug: Byte count analysis ===")
                logging.debug(f"Total blob size: {len(blob_data)} bytes")
                logging.debug(f"Parsed so far: {offset} bytes")
                logging.debug(f"Remaining: {remaining} bytes")
                logging.debug("")
                logging.debug("Expected struct sizes (packed):")
                if 'rpu_phy_stats' in self.structs:
                    phy_size = struct.calcsize(endianness + 'bBIIII')  # 18 bytes
                    logging.debug(f"  rpu_phy_stats: {phy_size} bytes")
                if 'rpu_lmac_stats' in self.structs:
                    lmac_size = struct.calcsize(endianness + 'I' * 37)  # 148 bytes
                    logging.debug(f"  rpu_lmac_stats: {lmac_size} bytes")
                if 'umac_tx_dbg_params' in self.structs:
                    tx_size = struct.calcsize(endianness + 'I' * 34)  # 136 bytes
                    logging.debug(f"  umac_tx_dbg_params: {tx_size} bytes")
                if 'umac_rx_dbg_params' in self.structs:
                    rx_size = struct.calcsize(endianness + 'I' * 38)  # 152 bytes
                    logging.debug(f"  umac_rx_dbg_params: {rx_size} bytes")
                if 'umac_cmd_evnt_dbg_params' in self.structs:
                    cmd_size = struct.calcsize(endianness + 'I' * 40)  # 160 bytes
                    logging.debug(f"  umac_cmd_evnt_dbg_params: {cmd_size} bytes")
                if 'nrf_wifi_interface_stats' in self.structs:
                    # Interface stats is only 30 bytes (7.5 uint32 values) in the actual blob
                    iface_size = remaining  # Use actual remaining bytes
                    logging.debug(f"  nrf_wifi_interface_stats: {iface_size} bytes (actual)")
                
                total_expected = 18 + 148 + 136 + 152 + 160 + remaining  # 644 bytes
                logging.debug(f"  Total expected: {total_expected} bytes")
                logging.debug(f"  Actual blob: {len(blob_data)} bytes")
                logging.debug(f"  Match: {'✓' if total_expected == len(blob_data) else '✗'}")

def main():
    parser = argparse.ArgumentParser(description='Parse rpu_sys_fw_stats from hex blob using header file')
    parser.add_argument('header_file', help='Path to header file containing struct definitions')
    parser.add_argument('hex_blob', help='Hex blob data to parse')
    parser.add_argument('-d', '--debug', action='store_true', help='Enable debug output')
    
    args = parser.parse_args()
    
    # Configure logging
    if args.debug:
        logging.basicConfig(level=logging.DEBUG, format='%(levelname)s: %(message)s')
    else:
        logging.basicConfig(level=logging.WARNING, format='%(levelname)s: %(message)s')
    
    if not os.path.exists(args.header_file):
        print(f"Error: Header file '{args.header_file}' not found")
        sys.exit(1)
    
    # Convert hex string to binary
    blob_data = bytes.fromhex(args.hex_blob.replace(' ', ''))
    
    # Parse using header file
    struct_parser = StructParser(args.header_file, debug=args.debug)
    struct_parser.parse_rpu_sys_fw_stats(blob_data, '<')  # Hardcoded to little-endian

if __name__ == "__main__":
    main()
