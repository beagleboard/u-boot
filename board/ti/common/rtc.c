// SPDX-License-Identifier: GPL-2.0+
/*
 * RTC setup for TI Platforms
 *
 * Copyright (C) 2023 Texas Instruments Incorporated - https://www.ti.com/
 */
#include <asm/arch/hardware.h>
#include <asm/io.h>
#include <log.h>

#include "rtc.h"

void board_rtc_init(void)
{
	u32 val;

	/* We have 32k crystal, so lets enable it */
	val = readl(MCU_CTRL_LFXOSC_CTRL);
	val &= ~(MCU_CTRL_LFXOSC_32K_DISABLE_VAL);
	writel(val, MCU_CTRL_LFXOSC_CTRL);

	/* Add any TRIM needed for the crystal here.. */
	/* Make sure to mux up to take the SoC 32k from the crystal */
	writel(MCU_CTRL_DEVICE_CLKOUT_LFOSC_SELECT_VAL,
	       MCU_CTRL_DEVICE_CLKOUT_32K_CTRL);
}
