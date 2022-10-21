# SPDX-License-Identifier: GPL-2.0+
#
# Makefile
#
# Copyright (C) 2013 Texas Instruments Incorporated - http://www.ti.com/

ifeq ($(CONFIG_SKIP_LOWLEVEL_INIT),)
obj-$(CONFIG_TARGET_AM43XX_EVM)		:= mux.o
obj-$(CONFIG_TARGET_AM43XX_EVM_MINI)	:= mux_mini.o
endif

obj-$(CONFIG_TARGET_AM43XX_EVM)		+= board.o
obj-$(CONFIG_TARGET_AM43XX_EVM_MINI)	+= board_mini.o
