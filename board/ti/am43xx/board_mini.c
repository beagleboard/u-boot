// SPDX-License-Identifier: GPL-2.0+
/*
 * Mini board functions for TI AM43XX based boards
 *
 * Copyright (C) 2020, Texas Instruments, Incorporated - http://www.ti.com/
 */

#include <common.h>
#include <eeprom.h>
#include <env.h>
#include <i2c.h>
#include <init.h>
#include <linux/errno.h>
#include <spl.h>
#include <usb.h>
#include <fdt_support.h>
#include <image.h>
#include <asm/omap_sec_common.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/mux.h>
#include <asm/arch/ddr_defs.h>
#include <asm/arch/gpio.h>
#include <asm/emif.h>
#include <asm/omap_common.h>
#include <power/pmic.h>
#include <power/tps65218.h>
#include "board_mini.h"
#include "board_hs_mini.h"

DECLARE_GLOBAL_DATA_PTR;

static struct ctrl_dev *cdev = (struct ctrl_dev *)CTRL_DEVICE_BASE;

const struct dpll_params dpll_mpu[NUM_CRYSTAL_FREQ][NUM_OPPS] = {
	{	/* 19.2 MHz */
		{125, 3, 2, -1, -1, -1, -1},	/* OPP 50 */
		{-1, -1, -1, -1, -1, -1, -1},	/* OPP RESERVED	*/
		{125, 3, 1, -1, -1, -1, -1},	/* OPP 100 */
		{150, 3, 1, -1, -1, -1, -1},	/* OPP 120 */
		{125, 2, 1, -1, -1, -1, -1},	/* OPP TB */
		{625, 11, 1, -1, -1, -1, -1}	/* OPP NT */
	},
	{	/* 24 MHz */
		{300, 23, 1, -1, -1, -1, -1},	/* OPP 50 */
		{-1, -1, -1, -1, -1, -1, -1},	/* OPP RESERVED	*/
		{600, 23, 1, -1, -1, -1, -1},	/* OPP 100 */
		{720, 23, 1, -1, -1, -1, -1},	/* OPP 120 */
		{800, 23, 1, -1, -1, -1, -1},	/* OPP TB */
		{1000, 23, 1, -1, -1, -1, -1}	/* OPP NT */
	},
	{	/* 25 MHz */
		{300, 24, 1, -1, -1, -1, -1},	/* OPP 50 */
		{-1, -1, -1, -1, -1, -1, -1},	/* OPP RESERVED	*/
		{600, 24, 1, -1, -1, -1, -1},	/* OPP 100 */
		{720, 24, 1, -1, -1, -1, -1},	/* OPP 120 */
		{800, 24, 1, -1, -1, -1, -1},	/* OPP TB */
		{1000, 24, 1, -1, -1, -1, -1}	/* OPP NT */
	},
	{	/* 26 MHz */
		{300, 25, 1, -1, -1, -1, -1},	/* OPP 50 */
		{-1, -1, -1, -1, -1, -1, -1},	/* OPP RESERVED	*/
		{600, 25, 1, -1, -1, -1, -1},	/* OPP 100 */
		{720, 25, 1, -1, -1, -1, -1},	/* OPP 120 */
		{800, 25, 1, -1, -1, -1, -1},	/* OPP TB */
		{1000, 25, 1, -1, -1, -1, -1}	/* OPP NT */
	},
};

const struct dpll_params dpll_core[NUM_CRYSTAL_FREQ] = {
		{625, 11, -1, -1, 10, 8, 4},	/* 19.2 MHz */
		{1000, 23, -1, -1, 10, 8, 4},	/* 24 MHz */
		{1000, 24, -1, -1, 10, 8, 4},	/* 25 MHz */
		{1000, 25, -1, -1, 10, 8, 4}	/* 26 MHz */
};

const struct dpll_params dpll_per[NUM_CRYSTAL_FREQ] = {
		{400, 7, 5, -1, -1, -1, -1},	/* 19.2 MHz */
		{400, 9, 5, -1, -1, -1, -1},	/* 24 MHz */
		{384, 9, 5, -1, -1, -1, -1},	/* 25 MHz */
		{480, 12, 5, -1, -1, -1, -1}	/* 26 MHz */
};

const struct dpll_params epos_evm_dpll_ddr[NUM_CRYSTAL_FREQ] = {
		{665, 47, 1, -1, 4, -1, -1}, /*19.2*/
		{133, 11, 1, -1, 4, -1, -1}, /* 24 MHz */
		{266, 24, 1, -1, 4, -1, -1}, /* 25 MHz */
		{133, 12, 1, -1, 4, -1, -1}  /* 26 MHz */
};

const struct dpll_params gp_evm_dpll_ddr = {
		50, 2, 1, -1, 2, -1, -1};

const struct ctrl_ioregs ioregs_ddr3 = {
	.cm0ioctl		= DDR3_ADDRCTRL_IOCTRL_VALUE,
	.cm1ioctl		= DDR3_ADDRCTRL_WD0_IOCTRL_VALUE,
	.cm2ioctl		= DDR3_ADDRCTRL_WD1_IOCTRL_VALUE,
	.dt0ioctl		= DDR3_DATA0_IOCTRL_VALUE,
	.dt1ioctl		= DDR3_DATA0_IOCTRL_VALUE,
	.dt2ioctrl		= DDR3_DATA0_IOCTRL_VALUE,
	.dt3ioctrl		= DDR3_DATA0_IOCTRL_VALUE,
	.emif_sdram_config_ext	= 0xc163,
};

/* EMIF DDR3 Configurations are different for production AM43X GP EVMs */
const struct emif_regs ddr3_emif_regs_400mhz_production = {
	.sdram_config			= 0x638413B2,
	.ref_ctrl			= 0x00000C30,
	.sdram_tim1			= 0xEAAAD4DB,
	.sdram_tim2			= 0x266B7FDA,
	.sdram_tim3			= 0x107F8678,
	.read_idle_ctrl			= 0x00050000,
	.zq_config			= 0x50074BE4,
	.temp_alert_config		= 0x0,
	.emif_ddr_phy_ctlr_1		= 0x00048008,
	.emif_ddr_ext_phy_ctrl_1	= 0x08020080,
	.emif_ddr_ext_phy_ctrl_2	= 0x00000066,
	.emif_ddr_ext_phy_ctrl_3	= 0x00000091,
	.emif_ddr_ext_phy_ctrl_4	= 0x000000B9,
	.emif_ddr_ext_phy_ctrl_5	= 0x000000E6,
	.emif_rd_wr_exec_thresh		= 0x80000405,
	.emif_prio_class_serv_map	= 0x80000001,
	.emif_connect_id_serv_1_map	= 0x80000094,
	.emif_connect_id_serv_2_map	= 0x00000000,
	.emif_cos_config		= 0x000FFFFF
};

void emif_get_ext_phy_ctrl_const_regs(const u32 **regs, u32 *size)
{
}

const struct dpll_params *get_dpll_ddr_params(void)
{
	if (!IS_ENABLED(CONFIG_SKIP_LOWLEVEL_INIT))
		return &gp_evm_dpll_ddr;
	return NULL;
}

/*
 * get_opp_offset:
 * Returns the index for safest OPP of the device to boot.
 * max_off:	Index of the MAX OPP in DEV ATTRIBUTE register.
 * min_off:	Index of the MIN OPP in DEV ATTRIBUTE register.
 * This data is read from dev_attribute register which is e-fused.
 * A'1' in bit indicates OPP disabled and not available, a '0' indicates
 * OPP available. Lowest OPP starts with min_off. So returning the
 * bit with rightmost '0'.
 */
static int get_opp_offset(int max_off, int min_off)
{
	struct ctrl_stat *ctrl = (struct ctrl_stat *)CTRL_BASE;
	int opp, offset, i;

	if (IS_ENABLED(CONFIG_SKIP_LOWLEVEL_INIT))
		return 0;

	/* Bits 0:11 are defined to be the MPU_MAX_FREQ */
	opp = readl(&ctrl->dev_attr) & ~0xFFFFF000;

	for (i = max_off; i >= min_off; i--) {
		offset = opp & (1 << i);
		if (!offset)
			return i;
	}

	return min_off;
}

const struct dpll_params *get_dpll_mpu_params(void)
{
	int opp;
	u32 ind;

	if (IS_ENABLED(CONFIG_SKIP_LOWLEVEL_INIT))
		return NULL;

	opp = get_opp_offset(DEV_ATTR_MAX_OFFSET, DEV_ATTR_MIN_OFFSET);
	ind = get_sys_clk_index();

	return &dpll_mpu[ind][opp];
}

const struct dpll_params *get_dpll_core_params(void)
{
	int ind;

	if (IS_ENABLED(CONFIG_SKIP_LOWLEVEL_INIT))
		return NULL;

	ind = get_sys_clk_index();

	return &dpll_core[ind];
}

const struct dpll_params *get_dpll_per_params(void)
{
	int ind;

	if (IS_ENABLED(CONFIG_SKIP_LOWLEVEL_INIT))
		return NULL;

	ind = get_sys_clk_index();

	return &dpll_per[ind];
}

void scale_vcores_generic(u32 m)
{
	int mpu_vdd, ddr_volt;

	if (IS_ENABLED(CONFIG_SKIP_LOWLEVEL_INIT))
		return;

	if (IS_ENABLED(CONFIG_DM_I2C)) {
		if (power_tps65218_init(0))
			return;
	} else {
		if (i2c_probe(TPS65218_CHIP_PM))
			return;
	}

	switch (m) {
	case 1000:
		mpu_vdd = TPS65218_DCDC_VOLT_SEL_1330MV;
		break;
	case 800:
		mpu_vdd = TPS65218_DCDC_VOLT_SEL_1260MV;
		break;
	case 720:
		mpu_vdd = TPS65218_DCDC_VOLT_SEL_1200MV;
		break;
	case 600:
		mpu_vdd = TPS65218_DCDC_VOLT_SEL_1100MV;
		break;
	case 300:
		mpu_vdd = TPS65218_DCDC_VOLT_SEL_0950MV;
		break;
	default:
		puts("Unknown MPU clock, not scaling\n");
		return;
	}

	/* Set DCDC1 (CORE) voltage to 1.1V */
	if (tps65218_voltage_update(TPS65218_DCDC1,
				    TPS65218_DCDC_VOLT_SEL_1100MV)) {
		printf("%s failure\n", __func__);
		return;
	}

	/* Set DCDC2 (MPU) voltage */
	if (tps65218_voltage_update(TPS65218_DCDC2, mpu_vdd)) {
		printf("%s failure\n", __func__);
		return;
	}

	ddr_volt = TPS65218_DCDC3_VOLT_SEL_1350MV;

	/* Set DCDC3 (DDR) voltage */
	if (tps65218_voltage_update(TPS65218_DCDC3, ddr_volt)) {
		printf("%s failure\n", __func__);
		return;
	}
}

void gpi2c_init(void)
{
	/* When needed to be invoked prior to BSS initialization */
	static bool first_time = true;

	if (IS_ENABLED(CONFIG_SKIP_LOWLEVEL_INIT))
		return;

	if (first_time) {
		enable_i2c0_pin_mux();

		if (!IS_ENABLED(CONFIG_DM_I2C))
			i2c_init(CONFIG_SYS_OMAP24_I2C_SPEED,
				 CONFIG_SYS_OMAP24_I2C_SLAVE);

		first_time = false;
	}
}

void scale_vcores(void)
{
	const struct dpll_params *mpu_params;

	if (IS_ENABLED(CONFIG_SKIP_LOWLEVEL_INIT))
		return;

	/* Ensure I2C is initialized for PMIC configuration */
	gpi2c_init();

	/* Get the frequency */
	mpu_params = get_dpll_mpu_params();

	scale_vcores_generic(mpu_params->m);
}

void set_uart_mux_conf(void)
{
	if (IS_ENABLED(CONFIG_SKIP_LOWLEVEL_INIT))
		return;

	enable_uart0_pin_mux();
}

void set_mux_conf_regs(void)
{
	if (IS_ENABLED(CONFIG_SKIP_LOWLEVEL_INIT))
		return;

	enable_board_pin_mux();
}

static void enable_vtt_regulator(void)
{
	u32 temp;

	if (IS_ENABLED(CONFIG_SKIP_LOWLEVEL_INIT))
		return;

	/* enable module */
	writel(GPIO_CTRL_ENABLEMODULE, AM33XX_GPIO5_BASE + OMAP_GPIO_CTRL);

	/* enable output for GPIO5_7 */
	writel(GPIO_SETDATAOUT(7),
	       AM33XX_GPIO5_BASE + OMAP_GPIO_SETDATAOUT);
	temp = readl(AM33XX_GPIO5_BASE + OMAP_GPIO_OE);
	temp = temp & ~(GPIO_OE_ENABLE(7));
	writel(temp, AM33XX_GPIO5_BASE + OMAP_GPIO_OE);
}

void sdram_init(void)
{
	if (IS_ENABLED(CONFIG_SKIP_LOWLEVEL_INIT))
		return;

	/* GP EVM has 1GB DDR3 connected to EMIF along with VTT regulator */
	enable_vtt_regulator();
	config_ddr(0, &ioregs_ddr3, NULL, NULL,
		   &ddr3_emif_regs_400mhz_production, 0);
}

/* setup board specific PMIC */
int power_init_board(void)
{
	int rc;
	struct pmic *p = NULL;

	rc = power_tps65218_init(0);
	if (rc)
		goto done;

	if (!IS_ENABLED(CONFIG_DM_I2C)) {
		p = pmic_get("TPS65218_PMIC");
		if (!p || pmic_probe(p))
			goto done;
	}

	puts("PMIC:  TPS65218\n");

done:
	return 0;
}

int board_init(void)
{
	struct l3f_cfg_bwlimiter *bwlimiter = (struct l3f_cfg_bwlimiter *)L3F_CFG_BWLIMITER;
	u32 mreqprio_0, mreqprio_1, modena_init0_bw_fractional,
	    modena_init0_bw_integer, modena_init0_watermark_0;

	gd->bd->bi_boot_params = CONFIG_SYS_SDRAM_BASE + 0x100;
	gpmc_init();

	/*
	 * Call this to initialize *ctrl again
	 */
	hw_data_init();

	/* Clear all important bits for DSS errata that may need to be tweaked*/
	mreqprio_0 = readl(&cdev->mreqprio_0) & MREQPRIO_0_SAB_INIT1_MASK &
		     MREQPRIO_0_SAB_INIT0_MASK;

	mreqprio_1 = readl(&cdev->mreqprio_1) & MREQPRIO_1_DSS_MASK;

	modena_init0_bw_fractional = readl(&bwlimiter->modena_init0_bw_fractional) &
				     BW_LIMITER_BW_FRAC_MASK;

	modena_init0_bw_integer = readl(&bwlimiter->modena_init0_bw_integer) &
				  BW_LIMITER_BW_INT_MASK;

	modena_init0_watermark_0 = readl(&bwlimiter->modena_init0_watermark_0) &
				   BW_LIMITER_BW_WATERMARK_MASK;

	/* Setting MReq Priority of the DSS*/
	mreqprio_0 |= 0x77;

	/*
	 * Set L3 Fast Configuration Register
	 * Limiting bandwidth for ARM core to 700 MBPS
	 */
	modena_init0_bw_fractional |= 0x10;
	modena_init0_bw_integer |= 0x3;

	writel(mreqprio_0, &cdev->mreqprio_0);
	writel(mreqprio_1, &cdev->mreqprio_1);

	writel(modena_init0_bw_fractional, &bwlimiter->modena_init0_bw_fractional);
	writel(modena_init0_bw_integer, &bwlimiter->modena_init0_bw_integer);
	writel(modena_init0_watermark_0, &bwlimiter->modena_init0_watermark_0);

	return 0;
}

int board_late_init(void)
{
	if (IS_ENABLED(CONFIG_BOARD_LATE_INIT) &&
	    IS_ENABLED(CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG)) {
		env_set("board_name", CONFIG_SYS_BOARD);

		/*
		 * Default FIT boot on HS devices. Non FIT images are not allowed
		 * on HS devices.
		 */
		if (get_device_type() == HS_DEVICE)
			env_set("boot_fit", "1");
	}

	return 0;
}

int ft_board_setup(void *blob, struct bd_info *bd)
{
	if (IS_ENABLED(CONFIG_OF_LIBFDT) && IS_ENABLED(CONFIG_OF_BOARD_SETUP))
		ft_cpu_setup(blob, bd);

	return 0;
}

int board_fit_config_name_match(const char *name)
{
	return 0;
}

int embedded_dtb_select(void)
{
	if (IS_ENABLED(CONFIG_DTB_RESELECT))
		do_board_detect();

	return 0;
}
