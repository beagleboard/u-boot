/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Configuration header file for PHYTEC phyCORE-AM62x
 *
 * Copyright (C) 2023 PHYTEC Messtechnik GmbH
 * Author: Wadim Egorov <w.egorov@phytec.de>
 */

#ifndef __PHYCORE_AM62X_H
#define __PHYCORE_AM62X_H

/* DDR Configuration */
#define CFG_SYS_SDRAM_BASE		0x80000000

#define PHYCORE_AM6XX_FW_NAME_TIBOOT3	u"PHYCORE_AM62X_TIBOOT3"
#define PHYCORE_AM6XX_FW_NAME_SPL	u"PHYCORE_AM62X_SPL"
#define PHYCORE_AM6XX_FW_NAME_UBOOT	u"PHYCORE_AM62X_UBOOT"

#endif /* __PHYCORE_AM62X_H */
