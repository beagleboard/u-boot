// SPDX-License-Identifier: GPL-2.0+
/*
 * Board specific initialization for AM62Ax platforms
 *
 * Copyright (C) 2022 Texas Instruments Incorporated - https://www.ti.com/
 *
 */

#include <asm/arch/hardware.h>
#include <asm/arch/sys_proto.h>
#include <asm/io.h>
#include <common.h>
#include <dm/uclass.h>
#include <env.h>
#include <fdt_support.h>
#include <spl.h>

#include "../common/rtc.c"

#define CTRLMMR_USB0_PHY_CTRL  0x43004008
#define CTRLMMR_USB1_PHY_CTRL  0x43004018
#define CORE_VOLTAGE           0x80000000

int board_init(void)
{
	if (IS_ENABLED(CONFIG_BOARD_HAS_32K_RTC_CRYSTAL))
		board_rtc_init();

	return 0;
}

int dram_init(void)
{
	return fdtdec_setup_mem_size_base();
}

int dram_init_banksize(void)
{
	return fdtdec_setup_memory_banksize();
}

#if defined(CONFIG_SPL_BOARD_INIT)
void spl_board_init(void)
{
	u32 val;

	/* Set USB0 PHY core voltage to 0.85V */
	val = readl(CTRLMMR_USB0_PHY_CTRL);
	val &= ~(CORE_VOLTAGE);
	writel(val, CTRLMMR_USB0_PHY_CTRL);

	/* Set USB1 PHY core voltage to 0.85V */
	val = readl(CTRLMMR_USB1_PHY_CTRL);
	val &= ~(CORE_VOLTAGE);
	writel(val, CTRLMMR_USB1_PHY_CTRL);
}
#endif
