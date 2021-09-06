# SPDX-License-Identifier: GPL-2.0+
#
# Makefile
#
# Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/

ifeq ($(CONFIG_SKIP_LOWLEVEL_INIT),)
obj-$(CONFIG_TARGET_AM335X_EVM)		:= mux.o
obj-$(CONFIG_TARGET_AM335X_EVM_MINI)	:= mux_mini.o
endif

obj-$(CONFIG_TARGET_AM335X_EVM)		+= board.o
obj-$(CONFIG_TARGET_AM335X_EVM_MINI)	+= board_mini.o
