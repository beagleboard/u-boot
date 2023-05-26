// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2017 exceet electronics GmbH
 *
 * Authors:
 *	Frieder Schrempf <frieder.schrempf@exceet.de>
 *	Boris Brezillon <boris.brezillon@bootlin.com>
 */

#ifndef __UBOOT__
#include <malloc.h>
#include <linux/device.h>
#include <linux/kernel.h>
#endif
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/mtd/spinand.h>

#define SPINAND_MFR_WINBOND		0xEF
#define WINBOND_ID_LEN			3

#define WINBOND_CFG_BUF_READ		BIT(3)

/* Octal DTR SPI mode (8D-8D-8D) with Data Strobe output*/
#define WINBOND_VCR_IO_MODE_OCTAL_DTR	0xE7
#define WINBOND_VCR_IO_MODE_SINGLE_STR	0xFF
#define WINBOND_VCR_IO_MODE_ADDR	0x00

/* Use 12 dummy clk cycles for using Octal DTR SPI at max 120MHZ */
#define WINBOND_VCR_DUMMY_CLK_COUNT	12
#define WINBOND_VCR_DUMMY_CLK_DEFAULT	0xFF
#define WINBOND_VCR_DUMMY_CLK_ADDR	0x01

#define WINBOND_POR_DELAY_US		1000

static SPINAND_OP_VARIANTS(read_cache_variants_w25m02gv,
		SPINAND_PAGE_READ_FROM_CACHE_QUADIO_OP(0, 2, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_X4_OP(0, 1, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_DUALIO_OP(0, 1, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_X2_OP(0, 1, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_OP(true, 0, 1, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_OP(false, 0, 1, NULL, 0));

static SPINAND_OP_VARIANTS(write_cache_variants_w25m02gv,
		SPINAND_PROG_LOAD_X4(true, 0, NULL, 0),
		SPINAND_PROG_LOAD(true, 0, NULL, 0));

static SPINAND_OP_VARIANTS(update_cache_variants_w25m02gv,
		SPINAND_PROG_LOAD_X4(false, 0, NULL, 0),
		SPINAND_PROG_LOAD(false, 0, NULL, 0));

static SPINAND_OP_VARIANTS(read_cache_variants_w35n01jw,
		SPINAND_PAGE_READ_FROM_CACHE_OCTALIO_DTR_OP(0, 24, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_OP(true, 0, 1, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_OP(false, 0, 1, NULL, 0));

static SPINAND_OP_VARIANTS(write_cache_variants_w35n01jw,
		SPINAND_PROG_LOAD_OCTALIO_DTR(true, 0, NULL, 0),
		SPINAND_PROG_LOAD(true, 0, NULL, 0));

static SPINAND_OP_VARIANTS(update_cache_variants_w35n01jw,
		SPINAND_PROG_LOAD_OCTALIO_DTR(false, 0, NULL, 0),
		SPINAND_PROG_LOAD(false, 0, NULL, 0));

static SPINAND_CTRL_OPS_VARIANTS(ctrl_ops_variants_w35n01jw,
		SPINAND_CTRL_OPS(SPINAND_8D,
				 SPINAND_RESET_OP_OCTAL_DTR,
				 SPINAND_GET_FEATURE_OP_OCTAL_DTR(0, NULL),
				 SPINAND_SET_FEATURE_OP_OCTAL_DTR(0, NULL),
				 SPINAND_WR_EN_DIS_OP_OCTAL_DTR(true),
				 SPINAND_BLK_ERASE_OP_OCTAL_DTR(0),
				 SPINAND_PAGE_READ_OP_OCTAL_DTR(0),
				 SPINAND_PROG_EXEC_OP_OCTAL_DTR(0)),
		SPINAND_CTRL_OPS(SPINAND_1S,
				 SPINAND_RESET_OP,
				 SPINAND_GET_FEATURE_OP(0, NULL),
				 SPINAND_SET_FEATURE_OP(0, NULL),
				 SPINAND_WR_EN_DIS_OP(true),
				 SPINAND_BLK_ERASE_OP(0),
				 SPINAND_PAGE_READ_OP(0),
				 SPINAND_PROG_EXEC_OP(0)));

static int w25m02gv_ooblayout_ecc(struct mtd_info *mtd, int section,
				  struct mtd_oob_region *region)
{
	if (section > 3)
		return -ERANGE;

	region->offset = (16 * section) + 8;
	region->length = 8;

	return 0;
}

static int w25m02gv_ooblayout_free(struct mtd_info *mtd, int section,
				   struct mtd_oob_region *region)
{
	if (section > 3)
		return -ERANGE;

	region->offset = (16 * section) + 2;
	region->length = 6;

	return 0;
}

static int w35n01jw_ooblayout_ecc(struct mtd_info *mtd, int section,
				  struct mtd_oob_region *region)
{
	if (section > 7)
		return -ERANGE;

	region->offset = (16 * section) + 12;
	region->length = 4;

	return 0;
}

static int w35n01jw_ooblayout_free(struct mtd_info *mtd, int section,
				   struct mtd_oob_region *region)
{
	if (section > 7)
		return -ERANGE;

	region->offset = (16 * section) + 2;
	region->length = 10;

	return 0;
}

static const struct mtd_ooblayout_ops w25m02gv_ooblayout = {
	.ecc = w25m02gv_ooblayout_ecc,
	.rfree = w25m02gv_ooblayout_free,
};

static const struct mtd_ooblayout_ops w35n01jw_ooblayout = {
	.ecc = w35n01jw_ooblayout_ecc,
	.rfree = w35n01jw_ooblayout_free,
};

static int w25m02gv_select_target(struct spinand_device *spinand,
				  unsigned int target)
{
	struct spi_mem_op op = SPI_MEM_OP(SPI_MEM_OP_CMD(0xc2, 1),
					  SPI_MEM_OP_NO_ADDR,
					  SPI_MEM_OP_NO_DUMMY,
					  SPI_MEM_OP_DATA_OUT(1,
							spinand->scratchbuf,
							1));

	*spinand->scratchbuf = target;
	return spi_mem_exec_op(spinand->slave, &op);
}

static const struct spinand_info winbond_spinand_table[] = {
	SPINAND_INFO("W25M02GV", 0xAB,
		     NAND_MEMORG(1, 2048, 64, 64, 1024, 1, 1, 2),
		     NAND_ECCREQ(1, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants_w25m02gv,
					      &write_cache_variants_w25m02gv,
					      &update_cache_variants_w25m02gv),
		     0,
		     SPINAND_ECCINFO(&w25m02gv_ooblayout, NULL),
		     SPINAND_SELECT_TARGET(w25m02gv_select_target)),
	SPINAND_INFO("W25N01GV", 0xAA,
		     NAND_MEMORG(1, 2048, 64, 64, 1024, 1, 1, 1),
		     NAND_ECCREQ(1, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants_w25m02gv,
					      &write_cache_variants_w25m02gv,
					      &update_cache_variants_w25m02gv),
		     0,
		     SPINAND_ECCINFO(&w25m02gv_ooblayout, NULL)),
	SPINAND_INFO("W35N01JW", 0xDC,
		     NAND_MEMORG(1, 4096, 128, 64, 512, 1, 1, 1),
		     NAND_ECCREQ(1, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants_w35n01jw,
					      &write_cache_variants_w35n01jw,
					      &update_cache_variants_w35n01jw),
		     SPINAND_HAS_OCTAL_DTR_BIT | SPINAND_HAS_CR_FEAT_BIT,
		     SPINAND_ECCINFO(&w35n01jw_ooblayout, NULL),
		     SPINAND_INFO_CTRL_OPS_VARIANTS(&ctrl_ops_variants_w35n01jw)),
};

/**
 * winbond_spinand_detect - initialize device related part in spinand_device
 * struct if it is a Winbond device.
 * @spinand: SPI NAND device structure
 */
static int winbond_spinand_detect(struct spinand_device *spinand)
{
	u8 *id = spinand->id.data;
	int ret;

	/*
	 * Winbond SPI NAND read ID need a dummy byte,
	 * so the first byte in raw_id is dummy.
	 */
	if (id[1] != SPINAND_MFR_WINBOND)
		return 0;

	ret = spinand_match_and_init(spinand, winbond_spinand_table,
				     ARRAY_SIZE(winbond_spinand_table), id[2]);
	if (ret)
		return ret;

	return 1;
}

static int winbond_spinand_init(struct spinand_device *spinand)
{
	struct nand_device *nand = spinand_to_nand(spinand);
	unsigned int i;

	/*
	 * Make sure all dies are in buffer read mode and not continuous read
	 * mode.
	 */
	for (i = 0; i < nand->memorg.ntargets; i++) {
		spinand_select_target(spinand, i);
		spinand_upd_cfg(spinand, WINBOND_CFG_BUF_READ,
				WINBOND_CFG_BUF_READ);
	}

	return 0;
}

/**
 * winbond_write_vcr_op() - write values onto the volatile configuration
 *			    registers (VCR)
 * @spinand: the spinand device
 * @reg: the address of the particular reg in the VCR to be written on
 * @val: the value to be written on the reg in the VCR
 *
 * Volatile configuration registers are a separate set of configuration
 * registers, i.e. they differ from the status registers SR-1/2/3. A different
 * SPI instruction is required to write to these registers. Any changes
 * to the Volatile Configuration Register get transferred directly to
 * the Internal Configuration Register and instantly reflect on the
 * device operation.
 */
static int winbond_write_vcr_op(struct spinand_device *spinand, u8 reg, u8 val)
{
	int ret;
	struct spi_mem_op op =
		SPI_MEM_OP(SPI_MEM_OP_CMD(0x81, 1),
			   SPI_MEM_OP_ADDR(3, reg, 1),
			   SPI_MEM_OP_NO_DUMMY,
			   SPI_MEM_OP_DATA_OUT(1, spinand->scratchbuf, 1));

	*spinand->scratchbuf = val;

	ret = spinand_write_enable_op(spinand);
	if (ret)
		return ret;

	ret = spi_mem_exec_op(spinand->slave, &op);
	if (ret)
		return ret;

	/*
	 * Write VCR operation doesn't set the busy bit in SR, so can't perform
	 * a status poll. Minimum time of 50ns is needed to complete the write.
	 * So, give thrice the minimum required delay.
	 */
	ndelay(150);
	return 0;
}

static int winbond_spinand_octal_dtr_enable(struct spinand_device *spinand)
{
	int ret;
	struct spi_mem_op op;

	ret = winbond_write_vcr_op(spinand, WINBOND_VCR_DUMMY_CLK_ADDR,
				   WINBOND_VCR_DUMMY_CLK_COUNT);
	if (ret)
		return ret;

	ret = winbond_write_vcr_op(spinand, WINBOND_VCR_IO_MODE_ADDR,
				   WINBOND_VCR_IO_MODE_OCTAL_DTR);
	if (ret)
		return ret;

	/* Read flash ID to make sure the switch was successful. */
	op = (struct spi_mem_op)
		SPI_MEM_OP(SPI_MEM_OP_EXT_CMD(2, 0x9f9f, 8, SPI_MEM_OP_DTR),
			   SPI_MEM_OP_NO_ADDR,
			   SPI_MEM_OP_DUMMY(16, 8, SPI_MEM_OP_DTR),
			   SPI_MEM_OP_DATA_IN(SPINAND_MAX_ID_LEN,
					      spinand->scratchbuf, 8,
					      SPI_MEM_OP_DTR));

	ret = spi_mem_exec_op(spinand->slave, &op);
	if (ret)
		return ret;

	if (memcmp(spinand->scratchbuf, spinand->id.data + 1, WINBOND_ID_LEN))
		return -EINVAL;

	return 0;
}

static int winbond_spinand_octal_dtr_disable(struct spinand_device *spinand)
{
	int ret;
	struct spi_mem_op op =
		SPI_MEM_OP(SPI_MEM_OP_EXT_CMD(2, 0x8181, 8, SPI_MEM_OP_DTR),
			   SPI_MEM_OP_ADDR(4, WINBOND_VCR_IO_MODE_ADDR, 8,
					   SPI_MEM_OP_DTR),
			   SPI_MEM_OP_NO_DUMMY,
			   SPI_MEM_OP_DATA_OUT(2, spinand->scratchbuf, 8,
					       SPI_MEM_OP_DTR));

	*spinand->scratchbuf = WINBOND_VCR_IO_MODE_SINGLE_STR;

	ret = spinand_write_enable_op(spinand);
	if (ret)
		return ret;

	ret = spi_mem_exec_op(spinand->slave, &op);
	if (ret)
		return ret;

	ret = winbond_write_vcr_op(spinand, WINBOND_VCR_DUMMY_CLK_ADDR,
				   WINBOND_VCR_DUMMY_CLK_DEFAULT);
	if (ret)
		return ret;

	/* Read flash ID to make sure the switch was successful. */
	op = (struct spi_mem_op)
		SPI_MEM_OP(SPI_MEM_OP_CMD(0x9f, 1),
			   SPI_MEM_OP_NO_ADDR,
			   SPI_MEM_OP_DUMMY(1, 1),
			   SPI_MEM_OP_DATA_IN(SPINAND_MAX_ID_LEN,
					      spinand->scratchbuf, 1));

	ret = spi_mem_exec_op(spinand->slave, &op);
	if (ret)
		return ret;

	if (memcmp(spinand->scratchbuf, spinand->id.data + 1, WINBOND_ID_LEN))
		return -EINVAL;

	return 0;
}

static int winbond_change_spi_protocol(struct spinand_device *spinand,
				       const enum spinand_protocol protocol)
{
	if (spinand->protocol == protocol)
		return 0;

	switch (spinand->protocol) {
	case SPINAND_1S:
		if (protocol == SPINAND_8D)
			return winbond_spinand_octal_dtr_enable(spinand);
		break;

	case SPINAND_8D:
		if (protocol == SPINAND_1S)
			return winbond_spinand_octal_dtr_disable(spinand);
		break;

	default:
		return -EOPNOTSUPP;
	}

	return -EOPNOTSUPP;
}

static void winbond_spinand_cleanup(struct spinand_device *spinand)
{
	struct spi_mem_op op;

	/* Perform Power-On-Reset if the device is in Octal-DTR mode */
	if (spinand->protocol == SPINAND_8D) {
		op = (struct spi_mem_op)
			SPI_MEM_OP(SPI_MEM_OP_EXT_CMD(2, 0x6666, 8, SPI_MEM_OP_DTR),
				   SPI_MEM_OP_NO_ADDR,
				   SPI_MEM_OP_NO_DUMMY,
				   SPI_MEM_OP_NO_DATA);
		spi_mem_exec_op(spinand->slave, &op);

		op = (struct spi_mem_op)
			SPI_MEM_OP(SPI_MEM_OP_EXT_CMD(2, 0x9999, 8, SPI_MEM_OP_DTR),
				   SPI_MEM_OP_NO_ADDR,
				   SPI_MEM_OP_NO_DUMMY,
				   SPI_MEM_OP_NO_DATA);
		spi_mem_exec_op(spinand->slave, &op);

		/* PoR can take max 500 us to complete, so sleep for 1000 us*/
		udelay(WINBOND_POR_DELAY_US);
	}
}

static const struct spinand_manufacturer_ops winbond_spinand_manuf_ops = {
	.detect = winbond_spinand_detect,
	.init = winbond_spinand_init,
	.change_protocol = winbond_change_spi_protocol,
	.cleanup = winbond_spinand_cleanup,
};

const struct spinand_manufacturer winbond_spinand_manufacturer = {
	.id = SPINAND_MFR_WINBOND,
	.name = "Winbond",
	.ops = &winbond_spinand_manuf_ops,
};
