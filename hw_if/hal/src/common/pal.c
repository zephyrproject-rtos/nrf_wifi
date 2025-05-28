/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief File containing SoC specific definitions for the
 * HAL Layer of the Wi-Fi driver.
 */

#include "common/pal.h"

bool pal_check_rpu_mcu_regions(enum RPU_PROC_TYPE proc, unsigned int addr_val)
{
	const struct rpu_addr_map *map = &RPU_ADDR_MAP_MCU[proc];
	enum RPU_MCU_ADDR_REGIONS region_type;

	if (proc >= RPU_PROC_TYPE_MAX) {
		return false;
	}

	for (region_type = 0; region_type < RPU_MCU_ADDR_REGION_MAX; region_type++) {
		const struct rpu_addr_region *region = &map->regions[region_type];

		if ((addr_val >= region->start) && (addr_val <= region->end)) {
			return true;
		}
	}

	return false;
}

enum nrf_wifi_status pal_rpu_addr_offset_get(unsigned int rpu_addr,
					     unsigned long *addr,
					     enum RPU_PROC_TYPE proc)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	unsigned int addr_base = (rpu_addr & RPU_ADDR_MASK_BASE);
	unsigned long region_offset = 0;

#ifdef WIFI_NRF71
	unsigned int bellboard_grtc_addr_base = 0;

	if (addr_base == RPU_ADDR_WIFI_MCU_REGS_START) {
		region_offset = SOC_MMAP_ADDR_OFFSET_WIFI_MCU_REGS;
	} else if ((rpu_addr >= RPU_ADDR_RAM0_START) &&
		   (rpu_addr <= RPU_ADDR_RAM0_END)) {
		region_offset = SOC_MMAP_ADDR_OFFSET_RAM0_PKD;
		*addr = region_offset + ((rpu_addr -
			RPU_ADDR_RAM0_START) & RPU_ADDR_RAM_ROM_MASK_OFFSET);
		status = NRF_WIFI_STATUS_SUCCESS;
		goto out;
	} else if ((rpu_addr >= RPU_ADDR_RAM1_START) &&
		   (rpu_addr <= RPU_ADDR_RAM1_END)) {
		region_offset = SOC_MMAP_ADDR_OFFSET_RAM1_PKD;
		*addr = region_offset + ((rpu_addr -
			RPU_ADDR_RAM1_START) & RPU_ADDR_RAM_ROM_MASK_OFFSET);
		status = NRF_WIFI_STATUS_SUCCESS;
		goto out;
	} else if ((rpu_addr >= RPU_ADDR_ROM0_START) &&
		   (rpu_addr <= RPU_ADDR_ROM0_END)) {
		region_offset = SOC_MMAP_ADDR_OFFSET_ROM0_PKD;
		*addr = region_offset + ((rpu_addr -
			RPU_ADDR_ROM0_START) & RPU_ADDR_RAM_ROM_MASK_OFFSET);
		status = NRF_WIFI_STATUS_SUCCESS;
		goto out;
	} else if ((rpu_addr >= RPU_ADDR_ROM1_START) &&
		   (rpu_addr <= RPU_ADDR_ROM1_END)) {
		region_offset = SOC_MMAP_ADDR_OFFSET_ROM1_PKD;
		*addr = region_offset + ((rpu_addr -
			RPU_ADDR_ROM1_START) & RPU_ADDR_RAM_ROM_MASK_OFFSET);
		status = NRF_WIFI_STATUS_SUCCESS;
		goto out;
	} else if ((rpu_addr >= RPU_ADDR_DATA_RAM_START) &&
		   (rpu_addr <= RPU_ADDR_DATA_RAM_END)) {
		region_offset = SOC_MMAP_ADDR_OFFSET_DATA_RAM_PKD;
	} else if ((rpu_addr >= RPU_ADDR_ACTUAL_DATA_RAM_START) &&
		   (rpu_addr <= RPU_ADDR_ACTUAL_DATA_RAM_END)) {
		region_offset = SOC_MMAP_ADDR_OFFSET_DATA_RAM_PKD;
	} else if ((rpu_addr >= RPU_ADDR_CODE_RAM_START) &&
		   (rpu_addr <= RPU_ADDR_CODE_RAM_END)) {
		region_offset = SOC_MMAP_ADDR_OFFSET_CODE_RAM_PKD;
	} else if (addr_base == RPU_ADDR_BELLBOARD_GRTC_REGION) {
		bellboard_grtc_addr_base =
			(rpu_addr & RPU_ADDR_BELLBOARD_GRTC_MASK_BASE);
		if (bellboard_grtc_addr_base ==
			RPU_ADDR_BELLBOARD_APP_REGION) {
			region_offset = SOC_MMAP_ADDR_OFFSET_BELLBOARD_APP;
		} else if (bellboard_grtc_addr_base ==
			RPU_ADDR_BELLBOARD_WIFI_REGION) {
			region_offset = SOC_MMAP_ADDR_OFFSET_BELLBOARD_WIFI;
		} else if (bellboard_grtc_addr_base ==
			RPU_ADDR_GRTC_REGION) {
			region_offset = SOC_MMAP_ADDR_OFFSET_GRTC;
		}
	} else if (addr_base == RPU_ADDR_FPGA_REGS_REGION) {
		region_offset = SOC_MMAP_ADDR_OFFSET_FPGA_REGS;
	} else if (addr_base == RPU_ADDR_WICR_REGS_REGION) {
		region_offset = SOC_MMAP_ADDR_OFFSET_WICR_REGS;
	} else if (addr_base == RPU_ADDR_SECURERAM_REGION) {
		region_offset = SOC_MMAP_ADDR_OFFSET_SECURERAM;
#else /* WIFI_NRF71 */
	if (addr_base == RPU_ADDR_SBUS_START) {
		region_offset = SOC_MMAP_ADDR_OFFSET_SYSBUS;
	} else if ((rpu_addr >= RPU_ADDR_GRAM_START) &&
		   (rpu_addr <= RPU_ADDR_GRAM_END)) {
		region_offset = SOC_MMAP_ADDR_OFFSET_GRAM_PKD;
	} else if (addr_base == RPU_ADDR_PBUS_START) {
		region_offset = SOC_MMAP_ADDR_OFFSET_PBUS;
	} else if (addr_base == RPU_ADDR_PKTRAM_START) {
		region_offset = SOC_MMAP_ADDR_OFFSET_PKTRAM_HOST_VIEW;
	} else if (pal_check_rpu_mcu_regions(proc, rpu_addr)) {
		region_offset = SOC_MMAP_ADDR_OFFSETS_MCU[proc];
#endif /* !WIFI_NRF71 */
	} else {
		nrf_wifi_osal_log_err("%s: Invalid rpu_addr 0x%X",
				      __func__,
				      rpu_addr);
		goto out;
	}

	*addr = region_offset + (rpu_addr & RPU_ADDR_MASK_OFFSET);
#ifdef WIFI_NRF71
	if (addr_base == RPU_ADDR_BELLBOARD_GRTC_REGION) {
		*addr = region_offset +
			(rpu_addr & RPU_BELLBOARD_GRTC_ADDR_MASK_OFFSET);
	} else if (addr_base == RPU_ADDR_CODE_RAM_REGION) {
		*addr = region_offset +	((rpu_addr -
			RPU_ADDR_CODE_RAM_START) & RPU_ADDR_MASK_OFFSET);
	} else if (addr_base == RPU_ADDR_WICR_REGS_REGION) {
		*addr = region_offset + (rpu_addr & RPU_WICR_ADDR_MASK_OFFSET);
	}
#endif /* WIFI_NRF71 */

	status = NRF_WIFI_STATUS_SUCCESS;
out:
	return status;
}

#ifdef WIFI_NRF71
unsigned long pal_rpu_rom_access_reg_addr_get(void)
{
	return SOC_MMAP_ADDR_OFFSET_ROM_ACCESS_FPGA_REG;
}

#ifdef RPU_HARD_RESET_SUPPORT
unsigned long pal_rpu_hard_rst_reg_offset_get(void)
{
	return SOC_MMAP_ADDR_OFFSET_HARDRESET;
}
#endif /* RPU_HARD_RESET_SUPPORT */
#endif /* WIFI_NRF71 */

#ifdef NRF_WIFI_LOW_POWER
unsigned long pal_rpu_ps_ctrl_reg_addr_get(void)
{
	return SOC_MMAP_ADDR_RPU_PS_CTRL;
}
#endif /* NRF_WIFI_LOW_POWER */

char *pal_ops_get_fw_loc(enum nrf_wifi_fw_type fw_type,
			 enum nrf_wifi_fw_subtype fw_subtype)
{
	char *fw_loc = NULL;

	switch (fw_type) {
	case NRF_WIFI_FW_TYPE_LMAC_PATCH:
		if (fw_subtype == NRF_WIFI_FW_SUBTYPE_PRI) {
			fw_loc = NRF_WIFI_FW_LMAC_PATCH_LOC_PRI;
		} else if (fw_subtype == NRF_WIFI_FW_SUBTYPE_SEC) {
			fw_loc = NRF_WIFI_FW_LMAC_PATCH_LOC_SEC;
		} else {
			nrf_wifi_osal_log_err("%s: Invalid LMAC FW sub-type = %d",
					      __func__,
					      fw_subtype);
			goto out;
		}
		break;
	case NRF_WIFI_FW_TYPE_UMAC_PATCH:
		if (fw_subtype == NRF_WIFI_FW_SUBTYPE_PRI) {
			fw_loc = NRF_WIFI_FW_UMAC_PATCH_LOC_PRI;
		} else if (fw_subtype == NRF_WIFI_FW_SUBTYPE_SEC) {
			fw_loc = NRF_WIFI_FW_UMAC_PATCH_LOC_SEC;
		} else {
			nrf_wifi_osal_log_err("%s: Invalid UMAC FW sub-type = %d",
					      __func__,
					      fw_subtype);
			goto out;
		}
		break;
	default:
		nrf_wifi_osal_log_err("%s: Invalid FW type = %d",
				      __func__,
				      fw_type);
		goto out;
	}

out:
	return fw_loc;
}
