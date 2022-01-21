#!/bin/bash

sys=$(uname -m)

#add: bc bison flex libssl-dev u-boot-tools python3-pycryptodome python3-pyelftools
#binutils-arm-linux-gnueabihf binutils-aarch64-linux-gnu
#gcc-arm-linux-gnueabihf gcc-aarch64-linux-gnu

DIR="$PWD"

make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- distclean

if [ "x${sys}" = "xaarch64" ] ; then
	make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- j721e_evm_r5_defconfig O=../r5
	make ARCH=arm -j2 CROSS_COMPILE=arm-linux-gnueabihf- O=../r5

	sudo cp -v ../r5/tiboot3.bin /opt/u-boot/bb-u-boot-beagleboneai64
else
	make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- j721e_evm_r5_defconfig
	make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- menuconfig
	make ARCH=arm -j4 CROSS_COMPILE=arm-linux-gnueabihf-

	make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- savedefconfig
	cp -v defconfig ./configs/j721e_evm_r5_defconfig

	ls -lh ./tiboot3.bin
fi

make ARCH=arm CROSS_COMPILE=aarch64-linux-gnu- distclean

if [ "x${sys}" = "xaarch64" ] ; then
	make ARCH=arm CROSS_COMPILE=aarch64-linux-gnu- j721e_evm_a72_defconfig O=../a72
	make -j2 CROSS_COMPILE=aarch64-linux-gnu- ATF=/opt/u-boot/bb-arm-trusted-firmware-k3-generic-opteed/bl31.bin TEE=/opt/u-boot/bb-optee-os-k3-j721e/tee-pager_v2.bin DM=/opt/u-boot/bb-u-boot-beagleboneai64/ipc_echo_testb_mcu1_0_release_strip.xer5f O=../a72

	sudo cp -v ../a72/tispl.bin /opt/u-boot/bb-u-boot-beagleboneai64
	sudo cp -v ../a72/u-boot.img /opt/u-boot/bb-u-boot-beagleboneai64
else
	make ARCH=arm CROSS_COMPILE=aarch64-linux-gnu- j721e_evm_a72_defconfig
	make ARCH=arm CROSS_COMPILE=aarch64-linux-gnu- menuconfig
	make ARCH=arm -j4 CROSS_COMPILE=aarch64-linux-gnu- ATF=${DIR}/ti-blobs/bl31.bin TEE=${DIR}/ti-blobs/tee-pager_v2.bin DM=${DIR}/ti-blobs/ipc_echo_testb_mcu1_0_release_strip.xer5f

	make ARCH=arm CROSS_COMPILE=aarch64-linux-gnu- savedefconfig
	cp -v defconfig ./configs/j721e_evm_a72_defconfig

	ls -lh ./tispl.bin
	ls -lh ./u-boot.bin
fi
