/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Configuration header file for K3 J784S4 EVM
 *
 * Copyright (C) 2022 Texas Instruments Incorporated - https://www.ti.com/
 *	Hari Nagalla <hnagalla@ti.com>
 */

#ifndef __CONFIG_J784S4_EVM_H
#define __CONFIG_J784S4_EVM_H

#include <linux/sizes.h>
#include <config_distro_bootcmd.h>
#include <environment/ti/mmc.h>

/* DDR Configuration */
#define CFG_SYS_SDRAM_BASE1		0x880000000

/* Now for the remaining common defines */
#include <configs/ti_armv7_common.h>

#endif /* __CONFIG_J784S4_EVM_H */
