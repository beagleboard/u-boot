// SPDX-License-Identifier: GPL-2.0+
/*
 * j722s Quality of Service (QoS) Configuration Data
 * Auto generated from K3 Resource Partitioning tool
 *
 * Copyright (C) 2024 Texas Instruments Incorporated - https://www.ti.com/
 */
#include <common.h>
#include <asm/arch/hardware.h>
#include "common.h"

struct k3_qos_data j722s_qos_data[] = {
	/* modules_qosConfig0 - 1 endpoints, 4 channels */
	{
		.reg = K3_DSS_UL_MAIN_0_VBUSM_DMA + 0x100 + 0x4 * 0,
		.val = ORDERID_15,
	},
	{
		.reg = K3_DSS_UL_MAIN_0_VBUSM_DMA + 0x100 + 0x4 * 1,
		.val = ORDERID_15,
	},
	{
		.reg = K3_DSS_UL_MAIN_0_VBUSM_DMA + 0x100 + 0x4 * 2,
		.val = ORDERID_15,
	},
	{
		.reg = K3_DSS_UL_MAIN_0_VBUSM_DMA + 0x100 + 0x4 * 3,
		.val = ORDERID_15,
	},

	/* modules_qosConfig1 - 1 endpoints, 4 channels */
	{
		.reg = K3_DSS_UL_MAIN_1_VBUSM_DMA + 0x100 + 0x4 * 0,
		.val = ORDERID_15,
	},
	{
		.reg = K3_DSS_UL_MAIN_1_VBUSM_DMA + 0x100 + 0x4 * 1,
		.val = ORDERID_15,
	},
	{
		.reg = K3_DSS_UL_MAIN_1_VBUSM_DMA + 0x100 + 0x4 * 2,
		.val = ORDERID_15,
	},
	{
		.reg = K3_DSS_UL_MAIN_1_VBUSM_DMA + 0x100 + 0x4 * 3,
		.val = ORDERID_15,
	},

	/* modules_qosConfig2 - 3 endpoints, 1 channels */
	{
		.reg = PULSAR_UL_MAIN_0_CPU0_PMST + 0x100 + 0x4 * 0,
		.val = ORDERID_15,
	},
	{
		.reg = PULSAR_UL_MAIN_0_CPU0_RMST + 0x100 + 0x4 * 0,
		.val = ORDERID_15,
	},
	{
		.reg = PULSAR_UL_MAIN_0_CPU0_WMST + 0x100 + 0x4 * 0,
		.val = ORDERID_15,
	},

	/* Following registers set 1:1 mapping for orderID MAP1/MAP2
	 * remap registers. orderID x is remapped to orderID x again
	 * This is to ensure orderID from MAP register is unchanged
	 */

	/* K3_DSS_UL_MAIN_0_VBUSM_DMA - 0 groups */

	/* K3_DSS_UL_MAIN_1_VBUSM_DMA - 0 groups */

	/* PULSAR_UL_MAIN_0_CPU0_PMST - 0 groups */

	/* PULSAR_UL_MAIN_0_CPU0_RMST - 0 groups */

	/* PULSAR_UL_MAIN_0_CPU0_WMST - 0 groups */
};

uint32_t j722s_qos_count = sizeof(j722s_qos_data) / sizeof(j722s_qos_data[0]);
