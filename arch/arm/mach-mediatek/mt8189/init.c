// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2026 MediaTek Inc.
 * Author: Chris-QJ Chen <chris-qj.chen@mediatek.com>
 */

#include <fdtdec.h>
#include <stdio.h>
#include <asm/global_data.h>
#include <asm/system.h>
#include <linux/kernel.h>
#include <linux/sizes.h>

DECLARE_GLOBAL_DATA_PTR;

int dram_init(void)
{
	return fdtdec_setup_mem_size_base();
}

phys_size_t get_effective_memsize(void)
{
	/*
	 * Limit gd->ram_top not exceeding SZ_4G. Because some peripherals like
	 * MMC requires DMA buffer allocated below SZ_4G.
	 */
	return min(SZ_4G - gd->ram_base, gd->ram_size);
}

void reset_cpu(ulong addr)
{
	if (!CONFIG_IS_ENABLED(SYSRESET))
		psci_system_reset();
}

int print_cpuinfo(void)
{
	printf("CPU:   MediaTek MT8189\n");

	return 0;
}
