// SPDX-License-Identifier: GPL-2.0+
/*
 * Board specific initialization for AM62Px platforms
 *
 * Copyright (C) 2023 Texas Instruments Incorporated - https://www.ti.com/
 *
 */

#include <asm/arch/hardware.h>
#include <asm/io.h>
#include <common.h>
#include <dm/uclass.h>
#include <env.h>
#include <fdt_support.h>
#include <spl.h>
#include <video.h>
#include <splash.h>

#include "../common/rtc.h"
#include "../common/k3-ddr-init.h"

#if CONFIG_IS_ENABLED(SPLASH_SCREEN)
static struct splash_location default_splash_locations[] = {
	{
		.name = "sf",
		.storage = SPLASH_STORAGE_SF,
		.flags = SPLASH_STORAGE_RAW,
		.offset = 0x700000,
	},
	{
		.name		= "mmc",
		.storage	= SPLASH_STORAGE_MMC,
		.flags		= SPLASH_STORAGE_FS,
		.devpart	= "1:1",
	},
};

int splash_screen_prepare(void)
{
	return splash_source_load(default_splash_locations,
				ARRAY_SIZE(default_splash_locations));
}
#endif

int board_init(void)
{
	if (IS_ENABLED(CONFIG_BOARD_HAS_32K_RTC_CRYSTAL))
		board_rtc_init();

	return 0;
}

#if defined(CONFIG_SPL_BUILD)
void spl_perform_fixups(struct spl_image_info *spl_image)
{
	if (IS_ENABLED(CONFIG_K3_INLINE_ECC))
		fixup_ddr_driver_for_ecc(spl_image);
	else
		fixup_memory_node(spl_image);
}

static int video_setup(void)
{
	if (CONFIG_IS_ENABLED(VIDEO)) {
		ulong addr;
		int ret;

		addr = gd->relocaddr;
		ret = video_reserve(&addr);
		if (ret)
			return ret;
		debug("Reserving %luk for video at: %08lx\n",
		      ((unsigned long)gd->relocaddr - addr) >> 10, addr);
		gd->relocaddr = addr;
	}

	return 0;
}

void spl_board_init(void)
{
	video_setup();
	enable_caches();
	if (IS_ENABLED(CONFIG_SPL_SPLASH_SCREEN) && IS_ENABLED(CONFIG_SPL_BMP))
		splash_display();
}
#endif
