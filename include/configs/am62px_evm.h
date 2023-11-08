/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Configuration header file for K3 AM62Px SoC family
 *
 * Copyright (C) 2023 Texas Instruments Incorporated - https://www.ti.com/
 */

#ifndef __CONFIG_AM62PX_EVM_H
#define __CONFIG_AM62PX_EVM_H

#include <config_distro_bootcmd.h>
#include <environment/ti/mmc.h>

/* Now for the remaining common defines */
#include <configs/ti_armv7_common.h>

/* include Android related settings */
#if CONFIG_CMD_ABOOTIMG
#include <configs/am62x_evm_android.h>
#endif

#endif /* __CONFIG_AM62PX_EVM_H */
