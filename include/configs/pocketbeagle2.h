/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Configuration header file for PocketBeagle 2
 *
 * Copyright (C) 2025 Texas Instruments Incorporated - https://www.ti.com/
 */

#ifndef __CONFIG_POCKETBEAGLE2_H
#define __CONFIG_POCKETBEAGLE2_H

/**
 * define POCKETBEAGLE2_TIBOOT3_IMAGE_GUID - firmware GUID for PocketBeagle 2
 *                                        tiboot3.bin
 * define POCKETBEAGLE2_SPL_IMAGE_GUID     - firmware GUID for PocketBeagle 2 SPL
 * define POCKETBEAGLE2_UBOOT_IMAGE_GUID   - firmware GUID for PocketBeagle 2 UBOOT
 *
 * These GUIDs are used in capsules updates to identify the corresponding
 * firmware object.
 *
 * Board developers using this as a starting reference should
 * define their own GUIDs to ensure that firmware repositories (like
 * LVFS) do not confuse them.
 */
#define POCKETBEAGLE2_TIBOOT3_IMAGE_GUID \
	EFI_GUID(0x411c0fbc, 0xb090, 0x45bd, 0x8c, 0x48, \
		0x44, 0xb8, 0xf7, 0xb7, 0xf8, 0xcc)

#define POCKETBEAGLE2_SPL_IMAGE_GUID \
	EFI_GUID(0x2cca8d5d, 0x3688, 0x4d52, 0x9b, 0x1d, \
		0x73, 0x52, 0xaa, 0x47, 0xd1, 0x56)

#define POCKETBEAGLE2_UBOOT_IMAGE_GUID \
	EFI_GUID(0xf0bee454, 0x06e0, 0x4065, 0xbf, 0xa6, \
		0xec, 0x82, 0xe1, 0xdc, 0xf3, 0x5d)

/* Now for the remaining common defines */
#include <configs/ti_armv7_common.h>

#endif /* __CONFIG_POCKETBEAGLE2_H */
