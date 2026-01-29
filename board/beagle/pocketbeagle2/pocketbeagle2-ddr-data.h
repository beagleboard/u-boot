/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2024 PHYTEC Messtechnik GmbH
 * Author: Wadim Egorov <w.egorov@phytec.de>
 */

#ifndef POCKETBEAGLE2_DDR_DATA
#define POCKETBEAGLE2_DDR_DATA

#include "../../phytec/common/k3/k3_ddrss_patch.h"

/* 1 GB variant delta */
struct ddr_reg ddr_1gb_ctl_regs[] = {
	{ 317, 0x00000101 },
	{ 318, 0x1FFF0000 }
};

struct ddr_reg ddr_1gb_pi_regs[] = {
	{ 77, 0x04010100 }
};

struct ddr_reg ddr_1gb_phy_regs[] = {
	// NOT really a delta...
	{ 1, 0x00000000 }
};

enum {
	POCKETBEAGLE2_1GB,
};

struct ddrss phycore_ddrss_data[] = {
	[POCKETBEAGLE2_1GB] = {
		.ctl_regs = &ddr_1gb_ctl_regs[0],
		.ctl_regs_num = ARRAY_SIZE(ddr_1gb_ctl_regs),
		.pi_regs = &ddr_1gb_pi_regs[0],
		.pi_regs_num = ARRAY_SIZE(ddr_1gb_pi_regs),
		.phy_regs = &ddr_1gb_phy_regs[0],
		.phy_regs_num = ARRAY_SIZE(ddr_1gb_phy_regs),
	},
};

#endif /* POCKETBEAGLE2_DDR_DATA */
