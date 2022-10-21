#!/bin/bash

if ! id | grep -q root; then
	echo "must be run as root"
	exit
fi

if [ -d /boot/firmware/ ] ; then
	cp -v /opt/u-boot/bb-k3-image-gen-j721e-evm/sysfw.itb /boot/firmware/
	cp -v ./tiboot3.bin /boot/firmware/
	cp -v ./tispl.bin /boot/firmware/
	cp -v ./u-boot.img /boot/firmware/
fi
