# SPDX-License-Identifier: GPL-2.0+

ifeq ($(CONFIG_SPL_BUILD)$(CONFIG_TPL_BUILD),)
nandcore-objs := core.o bbt.o
obj-$(CONFIG_MTD_NAND_CORE) += nandcore.o
obj-$(CONFIG_MTD_RAW_NAND) += raw/
obj-$(CONFIG_MTD_SPI_NAND) += spi/
else
nandcore-objs := core.o bbt.o
obj-$(CONFIG_$(SPL_TPL_)NAND_SUPPORT) += raw/
obj-$(CONFIG_$(SPL_TPL_)NAND_SPI_SUPPORT) += spi/ nandcore.o
endif
