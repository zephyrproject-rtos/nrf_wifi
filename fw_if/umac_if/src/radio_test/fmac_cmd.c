/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief Header containing command specific declarations for the
 * radio test mode in the FMAC IF Layer of the Wi-Fi driver.
 */

#include "radio_test/fmac_cmd.h"
#include "common/hal_api_common.h"

enum nrf_wifi_status umac_cmd_rt_init(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				      struct nrf_wifi_phy_rf_params *rf_params,
				      bool rf_params_valid,
#ifdef NRF_WIFI_LOW_POWER
				      int sleep_type,
#endif /* NRF_WIFI_LOW_POWER */
				      unsigned int phy_calib,
				      enum op_band op_band,
				      bool beamforming,
				      struct nrf_wifi_tx_pwr_ctrl_params *tx_pwr_ctrl_params,
				      struct nrf_wifi_board_params *board_params,
				      unsigned char *country_code)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct host_rpu_msg *umac_cmd = NULL;
	struct nrf_wifi_cmd_sys_init *umac_cmd_data = NULL;
	unsigned int len = 0;

	len = sizeof(*umac_cmd_data);

	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
				  len);

	if (!umac_cmd) {
		nrf_wifi_osal_log_err("%s: umac_cmd_alloc failed",
				      __func__);
		goto out;
	}

	umac_cmd_data = (struct nrf_wifi_cmd_sys_init *)(umac_cmd->msg);

	umac_cmd_data->sys_head.cmd_event = NRF_WIFI_CMD_INIT;
	umac_cmd_data->sys_head.len = len;


	umac_cmd_data->sys_params.rf_params_valid = rf_params_valid;

	if (rf_params_valid) {
		nrf_wifi_osal_mem_cpy(umac_cmd_data->sys_params.rf_params,
				      rf_params,
				      NRF_WIFI_RF_PARAMS_SIZE);
	}


	umac_cmd_data->sys_params.phy_calib = phy_calib;
	umac_cmd_data->sys_params.hw_bringup_time = HW_DELAY;
	umac_cmd_data->sys_params.sw_bringup_time = SW_DELAY;
	umac_cmd_data->sys_params.bcn_time_out = BCN_TIMEOUT;
	umac_cmd_data->sys_params.calib_sleep_clk = CALIB_SLEEP_CLOCK_ENABLE;
#ifdef NRF_WIFI_LOW_POWER
	umac_cmd_data->sys_params.sleep_enable = sleep_type;
#endif /* NRF_WIFI_LOW_POWER */
#ifdef NRF70_TCP_IP_CHECKSUM_OFFLOAD
	umac_cmd_data->tcp_ip_checksum_offload = 1;
#endif /* NRF70_TCP_IP_CHECKSUM_OFFLOAD */
	umac_cmd_data->discon_timeout = NRF_WIFI_AP_DEAD_DETECT_TIMEOUT;
#ifdef NRF_WIFI_RPU_RECOVERY
	umac_cmd_data->watchdog_timer_val =
		(NRF_WIFI_RPU_RECOVERY_PS_ACTIVE_TIMEOUT_MS) / 1000;
#else
	/* Disable watchdog */
	umac_cmd_data->watchdog_timer_val = 0xFFFFFF;
#endif /* NRF_WIFI_RPU_RECOVERY */

	nrf_wifi_osal_log_dbg("RPU LPM type: %s",
		umac_cmd_data->sys_params.sleep_enable == 2 ? "HW" :
		umac_cmd_data->sys_params.sleep_enable == 1 ? "SW" : "DISABLED");

#ifdef NRF_WIFI_MGMT_BUFF_OFFLOAD
	umac_cmd_data->mgmt_buff_offload =  1;
	nrf_wifi_osal_log_info("Management buffer offload enabled\n");
#endif /* NRF_WIFI_MGMT_BUFF_OFFLOAD */
#ifdef NRF_WIFI_FEAT_KEEPALIVE
	umac_cmd_data->keep_alive_enable = KEEP_ALIVE_ENABLED;
	umac_cmd_data->keep_alive_period = NRF_WIFI_KEEPALIVE_PERIOD_S;
	nrf_wifi_osal_log_dbg("Keepalive enabled with period %d\n",
				   umac_cmd_data->keepalive_period);
#endif /* NRF_WIFI_FEAT_KEEPALIVE */

	umac_cmd_data->op_band = op_band;

	nrf_wifi_osal_mem_cpy(&umac_cmd_data->sys_params.rf_params[PCB_LOSS_BYTE_2G_OFST],
			      &board_params->pcb_loss_2g,
			      NUM_PCB_LOSS_OFFSET);

	nrf_wifi_osal_mem_cpy(&umac_cmd_data->sys_params.rf_params[ANT_GAIN_2G_OFST],
			      &tx_pwr_ctrl_params->ant_gain_2g,
			      NUM_ANT_GAIN);

	nrf_wifi_osal_mem_cpy(&umac_cmd_data->sys_params.rf_params[BAND_2G_LW_ED_BKF_DSSS_OFST],
			      &tx_pwr_ctrl_params->band_edge_2g_lo_dss,
			      NUM_EDGE_BACKOFF);

	nrf_wifi_osal_mem_cpy(umac_cmd_data->country_code,
			      country_code,
			      NRF_WIFI_COUNTRY_CODE_LEN);

#ifdef NRF70_RPU_EXTEND_TWT_SP
	 umac_cmd_data->feature_flags |= TWT_EXTEND_SP_EDCA;
#endif
#ifdef CONFIG_WIFI_NRF70_SCAN_DISABLE_DFS_CHANNELS
	umac_cmd_data->feature_flags |= DISABLE_DFS_CHANNELS;
#endif /* NRF70_SCAN_DISABLE_DFS_CHANNELS */

	if (!beamforming) {
		umac_cmd_data->disable_beamforming = 1;
	}

#if defined(NRF_WIFI_PS_INT_PS)
	umac_cmd_data->ps_exit_strategy = INT_PS;
#else
	umac_cmd_data->ps_exit_strategy = EVERY_TIM;
#endif  /* NRF_WIFI_PS_INT_PS */

	umac_cmd_data->display_scan_bss_limit = NRF_WIFI_DISPLAY_SCAN_BSS_LIMIT;

#ifdef NRF_WIFI_COEX_DISABLE_PRIORITY_WINDOW_FOR_SCAN
	umac_cmd_data->coex_disable_ptiwin_for_wifi_scan = 1;
#else
	umac_cmd_data->coex_disable_ptiwin_for_wifi_scan = 0;
#endif /* NRF_WIFI_COEX_DISABLE_PRIORITY_WINDOW_FOR_SCAN */

	status = nrf_wifi_hal_ctrl_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    umac_cmd,
					    (sizeof(*umac_cmd) + len));

out:
	return status;
}

enum nrf_wifi_status umac_cmd_rt_prog_stats_get(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
						int op_mode)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct host_rpu_msg *umac_cmd = NULL;
	struct nrf_wifi_cmd_get_stats *umac_cmd_data = NULL;
	int len = 0;

	len = sizeof(*umac_cmd_data);

	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
				  len);

	if (!umac_cmd) {
		nrf_wifi_osal_log_err("%s: umac_cmd_alloc failed",
				      __func__);
		goto out;
	}

	umac_cmd_data = (struct nrf_wifi_cmd_get_stats *)(umac_cmd->msg);

	umac_cmd_data->sys_head.cmd_event = NRF_WIFI_CMD_GET_STATS;
	umac_cmd_data->sys_head.len = len;
	umac_cmd_data->stats_type = RPU_STATS_TYPE_PHY;
	umac_cmd_data->op_mode = op_mode;

	status = nrf_wifi_hal_ctrl_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    umac_cmd,
					    (sizeof(*umac_cmd) + len));

out:
	return status;
}


enum nrf_wifi_status umac_cmd_rt_prog_init(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					   struct nrf_wifi_radio_test_init_info *init_params)
{
	struct host_rpu_msg *umac_cmd = NULL;
	struct nrf_wifi_cmd_radio_test_init *umac_cmd_data = NULL;
	int len = 0;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	len = sizeof(*umac_cmd_data);

	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
				  len);

	if (!umac_cmd) {
		nrf_wifi_osal_log_err("%s: umac_cmd_alloc failed",
				      __func__);
		goto out;
	}

	umac_cmd_data = (struct nrf_wifi_cmd_radio_test_init *)(umac_cmd->msg);

	umac_cmd_data->sys_head.cmd_event = NRF_WIFI_CMD_RADIO_TEST_INIT;
	umac_cmd_data->sys_head.len = len;

	nrf_wifi_osal_mem_cpy(&umac_cmd_data->conf,
			      init_params,
			      sizeof(umac_cmd_data->conf));

	status = nrf_wifi_hal_ctrl_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    umac_cmd,
					    (sizeof(*umac_cmd) + len));
out:
	return status;
}


enum nrf_wifi_status umac_cmd_rt_prog_tx(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					 struct rpu_conf_params *params)
{
	struct host_rpu_msg *umac_cmd = NULL;
	struct nrf_wifi_cmd_mode_params *umac_cmd_data = NULL;
	int len = 0;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	len = sizeof(*umac_cmd_data);

	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
				  len);

	if (!umac_cmd) {
		nrf_wifi_osal_log_err("%s: umac_cmd_alloc failed",
				      __func__);
		goto out;
	}

	umac_cmd_data = (struct nrf_wifi_cmd_mode_params *)(umac_cmd->msg);

	umac_cmd_data->sys_head.cmd_event = NRF_WIFI_CMD_TX;
	umac_cmd_data->sys_head.len = len;

	nrf_wifi_osal_mem_cpy(&umac_cmd_data->conf,
			      params,
			      sizeof(umac_cmd_data->conf));

	status = nrf_wifi_hal_ctrl_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    umac_cmd,
					    (sizeof(*umac_cmd) + len));

out:
	return status;
}


enum nrf_wifi_status umac_cmd_rt_prog_rx(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					 struct rpu_conf_rx_radio_test_params *rx_params)
{
	struct host_rpu_msg *umac_cmd = NULL;
	struct nrf_wifi_cmd_rx *umac_cmd_data = NULL;
	int len = 0;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	len = sizeof(*umac_cmd_data);

	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
				  len);

	if (!umac_cmd) {
		nrf_wifi_osal_log_err("%s: umac_cmd_alloc failed",
				      __func__);
		goto out;
	}

	umac_cmd_data = (struct nrf_wifi_cmd_rx *)(umac_cmd->msg);

	umac_cmd_data->sys_head.cmd_event = NRF_WIFI_CMD_RX;
	umac_cmd_data->sys_head.len = len;

	nrf_wifi_osal_mem_cpy(&umac_cmd_data->conf,
			      rx_params,
			      sizeof(umac_cmd_data->conf));

	status = nrf_wifi_hal_ctrl_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    umac_cmd,
					    (sizeof(*umac_cmd) + len));

out:
	return status;
}


enum nrf_wifi_status umac_cmd_rt_prog_rf_test(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					      void *rf_test_params,
					      unsigned int rf_test_params_sz)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct host_rpu_msg *umac_cmd = NULL;
	struct nrf_wifi_cmd_rftest *umac_cmd_data = NULL;
	int len = 0;

	len = (sizeof(*umac_cmd_data) + rf_test_params_sz);

	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
				  len);

	if (!umac_cmd) {
		nrf_wifi_osal_log_err("%s: umac_cmd_alloc failed",
				      __func__);
		goto out;
	}

	umac_cmd_data = (struct nrf_wifi_cmd_rftest *)(umac_cmd->msg);

	umac_cmd_data->sys_head.cmd_event = NRF_WIFI_CMD_RF_TEST;
	umac_cmd_data->sys_head.len = len;

	nrf_wifi_osal_mem_cpy((void *)umac_cmd_data->rf_test_info.rfcmd,
			      rf_test_params,
			      rf_test_params_sz);

	umac_cmd_data->rf_test_info.len = rf_test_params_sz;

	status = nrf_wifi_hal_ctrl_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    umac_cmd,
					    (sizeof(*umac_cmd) + len));

out:
	return status;
}