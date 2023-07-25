.. SPDX-License-Identifier: GPL-2.0+ OR BSD-3-Clause
.. sectionauthor:: Nishanth Menon <nm@ti.com>

Beagleboard.org Beagleplay
==========================

Introduction:
-------------
BeagleBoard.org BeaglePlay is an easy to use, affordable open source
hardware single board computer based on the Texas Instruments AM625
SoC that allows you to create connected devices that work even at long
distances using IEEE 802.15.4g LR-WPAN and IEEE 802.3cg 10Base-T1L.
Expansion is provided over open standards based mikroBUS, Grove and
QWIIC headers among other interfaces.

Further information can be found at:
https://beagleplay.org/
https://git.beagleboard.org/beagleplay/beagleplay

The boot flow follows the standard AM62X bootflow [1]_.

Sources:
--------
1. Trusted Firmware-A:
	Tree: https://git.trustedfirmware.org/TF-A/trusted-firmware-a.git/
	Branch: master

2. OP-TEE:
	Tree: https://github.com/OP-TEE/optee_os.git
	Branch: master

3. U-Boot:
	Tree: https://source.denx.de/u-boot/u-boot
	Branch: master

4. TI Linux Firmware:
	Tree: git://git.ti.com/processor-firmware/ti-linux-firmware.git
	Branch: ti-linux-firmware

Build procedure:
----------------
1. Trusted Firmware-A:

.. code-block:: bash

 $ make CROSS_COMPILE=aarch64-none-linux-gnu- ARCH=aarch64 PLAT=k3 \
        TARGET_BOARD=lite SPD=opteed

2. OP-TEE:

.. code-block:: bash

 $ make PLATFORM=k3 CFG_ARM64_core=y CROSS_COMPILE=arm-none-linux-gnueabihf- \
        CROSS_COMPILE64=aarch64-none-linux-gnu-

3. U-Boot:

* 3.1 R5:

.. code-block:: bash

 $ make ARCH=arm am62x_beagleplay_r5_defconfig
 $ make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabihf- \
        BINMAN_INDIRS=<path/to/ti-linux-firmware>

* 3.2 A53:

.. code-block:: bash

 $ make ARCH=arm am62x_beagleplay_a53_defconfig
 $ make ARCH=arm CROSS_COMPILE=aarch64-none-linux-gnu- \
        BL31=<path/to/trusted-firmware-a/dir>/build/k3/lite/release/bl31.bin \
        TEE=<path/to/optee_os/dir>/out/arm-plat-k3/core/tee-raw.bin \
        BINMAN_INDIRS=<path/to/ti-linux-firmware>

Target Images
--------------
Copy the below images to an SD card and boot:

* tiboot3-am62x-gp-evm.bin from step 3.1 as tiboot3.bin
* tispl.bin_unsigned from step 3.2 as tispl.bin
* u-boot.img_unsigned from step 3.2 as uboot.img

Image formats:
--------------

The Image format follows the standard AM62X image format [1]_.

A53 SPL DDR Memory Layout
-------------------------

This provides an overview memory usage in A53 SPL stage.

.. list-table::
   :widths: 16 16 16
   :header-rows: 1

   * - Region
     - Start Address
     - End Address

   * - EMPTY
     - 0x80000000
     - 0x80080000

   * - TEXT BASE
     - 0x80080000
     - 0x800d8000

   * - EMPTY
     - 0x800d8000
     - 0x80200000

   * - BMP IMAGE
     - 0x80200000
     - 0x80b77660

   * - STACK
     - 0x80b77660
     - 0x80b77e60

   * - GD
     - 0x80b77e60
     - 0x80b78000

   * - MALLOC
     - 0x80b78000
     - 0x80b80000

   * - EMPTY
     - 0x80b80000
     - 0x80c80000

   * - BSS
     - 0x80c80000
     - 0x80d00000

   * - BLOBS
     - 0x80d00000
     - 0x80d00400

   * - EMPTY
     - 0x80d00400
     - 0x81000000

Switch Setting for Boot Mode
----------------------------

The Boot time switch option is configured via "USR" button on the board.
See https://git.beagleboard.org/beagleplay/beagleplay/-/blob/main/BeaglePlay_sch.pdf
for details.

*Boot Modes*

=========== ============ ==============
Switch Posn Primary Boot Secondary Boot
=========== ============ ==============
Not Pressed  eMMC          UART
Pressed      SD FS mode    USB DFU
=========== ============ ==============

The procedure for using the USR button for switching to SD card boot mode
is to keep the USR button pressed while providing power over the Type-C power
supply and releasing the USR button once the power LED glows.

References
----------

.. [1] :doc:`am62x_sk`
