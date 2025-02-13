/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Common structures and definitions.
 */

#ifndef __RPU_UMAC_STATS_H__
#define __RPU_UMAC_STATS_H__

/**
 * @brief Common debug variables structure.
 */
struct stat {	
	char name[32];
	unsigned int addr;
};

/**
* @brief MAC debug variables.
*/
struct stat rpu_umac_stats[] = {
	{"tx_cmd", 0x8008bd9c },
	{"tx_cmds_currently_in_use", 0x8008bdb0 },
	{"tx_done_events_send_to_host", 0x8008bdb4 },
	{"tx_cmd_to_lmactx_dones_from_lmac", 0x8008bdec },
	{"tx_dones_from_lmac", 0x8008bdf0 },
	{"total_cmds_to_lmac", 0x8008bdf4 },
	{"lmac_events", 0x80080388 },
	{"rx_events", 0x8008038c },
	{"current_refill_gap", 0x8008039c },
	{"umac_consumed_pkts", 0x800803a8 },
	{"host_consumed_pkts", 0x800803ac },
	{"rx_mbox_post", 0x800803b0 },
	{"rx_mbox_receive", 0x800803b4 },
	{"null_skb_pointer_from_lmac", 0x80080415 },
	{"cmd_init", 0x8008bec4 },
	{"event_init_done", 0x8008bec5 },
	{"cmd_trigger_scan", 0x8008bedc },
	{"event_scan_done", 0x8008bee0 },
	{"umac_scan_req", 0x8008bee8 },
	{"umac_scan_complete", 0x8008beec },
	{"cmd_set_ifflags", 0x8008bf3c },
	{"cmd_set_ifflags_done", 0x8008bf40 },
	{"command_cnt", 0x8008be24 },
	{"send_rx_buffs_to_host", 0x8008be2c },
	{"event_node_alloc_fail", 0x8008be3c },
	{"hpqm_event_pop_fail", 0x8008be40 },
	{"total_events_to_host", 0x8008be48 },
};
 
#endif /* __RPU_UMAC_STATS_H__ */
