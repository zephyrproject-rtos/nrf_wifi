/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief File containing HPQM interface specific definitions for the
 * HAL Layer of the Wi-Fi driver.
 */

#include "common/hal_reg.h"
#include "common/hal_mem.h"
#include "common/hal_common.h"

enum nrf_wifi_status hal_rpu_hpq_enqueue(struct nrf_wifi_hal_dev_ctx *hal_ctx,
					 struct host_rpu_hpq *hpq,
					 unsigned int val)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	status = hal_rpu_reg_write(hal_ctx,
				   hpq->enqueue_addr,
				   val);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err("%s: Writing to enqueue address failed",
				      __func__);
		goto out;
	}

out:
	return status;
}


enum nrf_wifi_status hal_rpu_hpq_dequeue(struct nrf_wifi_hal_dev_ctx *hal_ctx,
					 struct host_rpu_hpq *hpq,
					 unsigned int *val)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	status = hal_rpu_reg_read(hal_ctx,
				  val,
				  hpq->dequeue_addr);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err("%s: Dequeue failed, val (0x%X)",
				      __func__,
				      *val);
		goto out;
	}

	/* Pop the element only if it is valid */
	if (*val) {
		status = hal_rpu_reg_write(hal_ctx,
					   hpq->dequeue_addr,
					   *val);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			nrf_wifi_osal_log_err("%s: Writing to dequeue address failed, val (0x%X)",
					      __func__,
					      *val);
			goto out;
		}
	}
out:
	return status;
}
