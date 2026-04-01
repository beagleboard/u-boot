/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2022 Marek Vasut <marex@denx.de>
 */

#ifndef __LPDDR4_TIMING_H__
#define __LPDDR4_TIMING_H__

static const u16 dh_imx8mp_dhcom_dram_size[] = {
	512, 1024, 1536, 2048, 3072, 4096, 6144, 8192
};

extern struct dram_timing_info dh_imx8mp_dhcom_dram_timing_16g_x32;
extern struct dram_timing_info dh_imx8mp_dhcom_dram_timing_32g_x32;

u8 dh_get_memcfg(void);

#define DDRC_ECCCFG0_ECC_MODE_MASK	0x7

#endif /* __LPDDR4_TIMING_H__ */
