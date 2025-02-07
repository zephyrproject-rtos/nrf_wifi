/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief Header containing command specific declarations for the
 * system mode in the FMAC IF Layer of the Wi-Fi driver.
 */

#ifndef __FMAC_CMD_SYS_H__
#define __FMAC_CMD_SYS_H__

#include "common/fmac_cmd_common.h"

enum nrf_wifi_status umac_cmd_sys_init(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				       struct nrf_wifi_phy_rf_params *rf_params,
				       bool rf_params_valid,
				       struct nrf_wifi_data_config_params *config,
#ifdef NRF_WIFI_LOW_POWER
				       int sleep_type,
#endif /* NRF_WIFI_LOW_POWER */
				       unsigned int phy_calib,
				       enum op_band op_band,
				       bool beamforming,
				       struct nrf_wifi_tx_pwr_ctrl_params *tx_pwr_ctrl_params,
				       struct nrf_wifi_board_params *board_params,
				       unsigned char *country_code);

enum nrf_wifi_status umac_cmd_sys_prog_stats_get(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx);

enum nrf_wifi_status umac_cmd_sys_he_ltf_gi(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					    unsigned char he_ltf,
					    unsigned char he_gi,
					    unsigned char enabled);
#endif /* __FMAC_CMD_SYS_H__ */
