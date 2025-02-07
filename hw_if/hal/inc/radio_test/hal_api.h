/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file hal_api.h
 *
 * @brief Header containing API declarations for the HAL Layer of the Wi-Fi driver
 * in the radio test mode of operation.
 */

#ifndef __HAL_API_RT_H__
#define __HAL_API_RT__H__

#include "osal_api.h"
#include "common/rpu_if.h"
#include "bal_api.h"
#include "common/hal_structs_common.h"
#include "common/hal_mem.h"
#include "common/hal_reg.h"
#include "common/hal_fw_patch_loader.h"
#include "common/hal_api_common.h"

struct nrf_wifi_hal_dev_ctx *nrf_wifi_rt_hal_dev_add(struct nrf_wifi_hal_priv *hpriv,
						     void *mac_dev_ctx);
#endif /* __HAL_API_RT_H__ */
