/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Cadence DDR Driver
 *
 * Copyright (C) 2012-2021 Cadence Design Systems, Inc.
 * Copyright (C) 2018-2021 Texas Instruments Incorporated - http://www.ti.com/
 */


#ifndef LPDDR4_RW_MASKS_H_
#define LPDDR4_RW_MASKS_H_

#include "cdn_stdint.h"

extern uint32_t g_lpddr4_ddr_controller_rw_mask[459];
extern uint32_t g_lpddr4_pi_rw_mask[300];
extern uint32_t g_lpddr4_data_slice_0_rw_mask[140];
extern uint32_t g_lpddr4_data_slice_1_rw_mask[140];
extern uint32_t g_lpddr4_data_slice_2_rw_mask[140];
extern uint32_t g_lpddr4_data_slice_3_rw_mask[140];
extern uint32_t g_lpddr4_address_slice_0_rw_mask[52];
extern uint32_t g_lpddr4_phy_core_rw_mask[143];

#endif /* LPDDR4_RW_MASKS_H_ */
