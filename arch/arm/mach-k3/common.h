/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * K3: Architecture common definitions
 *
 * Copyright (C) 2018-2022 Texas Instruments Incorporated - https://www.ti.com/
 *	Lokesh Vutla <lokeshvutla@ti.com>
 */

#include <asm/armv7_mpu.h>
#include <asm/hardware.h>
#include <linux/soc/ti/ti_sci_protocol.h>
#include <mach/security.h>

#define J721E  0xbb64
#define J7200  0xbb6d

struct fwl_data {
	const char *name;
	u16 fwl_id;
	u16 regions;
};

void setup_k3_mpu_regions(void);
int early_console_init(void);
void disable_linefill_optimization(void);
int set_fwls(const struct ti_sci_msg_fwl_region *fwl_data, size_t fwl_data_size);
void remove_fwl_configs(struct fwl_data *fwl_data, size_t fwl_data_size);
int load_firmware(char *name_fw, char *name_loadaddr, u32 *loadaddr);
void k3_sysfw_print_ver(void);
void spl_enable_dcache(void);
void mmr_unlock(phys_addr_t base, u32 partition);
bool is_rom_loaded_sysfw(struct rom_extended_boot_data *data);
