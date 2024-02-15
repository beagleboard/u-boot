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

	/* Setup debounce conf registers - arbitrary values.
	 * Times are approx
	 */
	/* 1.9ms debounce @ 32k */
	writel(WKUP_CTRLMMR_DBOUNCE_CFG1, 0x1);
	/* 5ms debounce @ 32k */
	writel(WKUP_CTRLMMR_DBOUNCE_CFG2, 0x5);
	/* 20ms debounce @ 32k */
	writel(WKUP_CTRLMMR_DBOUNCE_CFG3, 0x14);
	/* 46ms debounce @ 32k */
	writel(WKUP_CTRLMMR_DBOUNCE_CFG4, 0x18);
	/* 100ms debounce @ 32k */
	writel(WKUP_CTRLMMR_DBOUNCE_CFG5, 0x1c);
	/* 156ms debounce @ 32k */
	writel(WKUP_CTRLMMR_DBOUNCE_CFG6, 0x1f);
}
