.. SPDX-License-Identifier: GPL-2.0+ OR BSD-3-Clause
.. sectionauthor:: Lokesh Vutla <lokeshvutla@ti.com>

J721E Platforms
===============

Introduction:
-------------
The J721e family of SoCs are part of K3 Multicore SoC architecture platform
targeting automotive applications. They are designed as a low power, high
performance and highly integrated device architecture, adding significant
enhancement on processing power, graphics capability, video and imaging
processing, virtualization and coherent memory support.

The device is partitioned into three functional domains, each containing
specific processing cores and peripherals:

1. Wake-up (WKUP) domain:
        * Device Management and Security Controller (DMSC)

2. Microcontroller (MCU) domain:
        * Dual Core ARM Cortex-R5F processor

3. MAIN domain:
        * Dual core 64-bit ARM Cortex-A72
        * 2 x Dual cortex ARM Cortex-R5 subsystem
        * 2 x C66x Digital signal processor sub system
        * C71x Digital signal processor sub-system with MMA.

More info can be found in TRM: http://www.ti.com/lit/pdf/spruil1

Boot Flow:
----------
Boot flow is similar to that of AM65x SoC and extending it with remoteproc
support. Below is the pictorial representation of boot flow:

.. code-block:: text

 +------------------------------------------------------------------------+-----------------------+
 |        DMSC            |      MCU R5           |        A72            |  MAIN R5/C66x/C7x     |
 +------------------------------------------------------------------------+-----------------------+
 |    +--------+          |                       |                       |                       |
 |    |  Reset |          |                       |                       |                       |
 |    +--------+          |                       |                       |                       |
 |         :              |                       |                       |                       |
 |    +--------+          |   +-----------+       |                       |                       |
 |    | *ROM*  |----------|-->| Reset rls |       |                       |                       |
 |    +--------+          |   +-----------+       |                       |                       |
 |    |        |          |         :             |                       |                       |
 |    |  ROM   |          |         :             |                       |                       |
 |    |services|          |         :             |                       |                       |
 |    |        |          |   +-------------+     |                       |                       |
 |    |        |          |   |  *R5 ROM*   |     |                       |                       |
 |    |        |          |   +-------------+     |                       |                       |
 |    |        |<---------|---|Load and auth|     |                       |                       |
 |    |        |          |   | tiboot3.bin |     |                       |                       |
 |    |        |          |   +-------------+     |                       |                       |
 |    |        |          |         :             |                       |                       |
 |    |        |          |         :             |                       |                       |
 |    |        |          |         :             |                       |                       |
 |    |        |          |   +-------------+     |                       |                       |
 |    |        |          |   |  *R5 SPL*   |     |                       |                       |
 |    |        |          |   +-------------+     |                       |                       |
 |    |        |          |   |    Load     |     |                       |                       |
 |    |        |          |   |  sysfw.itb  |     |                       |                       |
 |    | Start  |          |   +-------------+     |                       |                       |
 |    | System |<---------|---|    Start    |     |                       |                       |
 |    |Firmware|          |   |    SYSFW    |     |                       |                       |
 |    +--------+          |   +-------------+     |                       |                       |
 |        :               |   |             |     |                       |                       |
 |    +---------+         |   |   Load      |     |                       |                       |
 |    | *SYSFW* |         |   |   system    |     |                       |                       |
 |    +---------+         |   | Config data |     |                       |                       |
 |    |         |<--------|---|             |     |                       |                       |
 |    |         |         |   +-------------+     |                       |                       |
 |    |         |         |   |    DDR      |     |                       |                       |
 |    |         |         |   |   config    |     |                       |                       |
 |    |         |         |   +-------------+     |                       |                       |
 |    |         |         |   |    Load     |     |                       |                       |
 |    |         |         |   |  tispl.bin  |     |                       |                       |
 |    |         |         |   +-------------+     |                       |                       |
 |    |         |         |   |   Load R5   |     |                       |                       |
 |    |         |         |   |   firmware  |     |                       |                       |
 |    |         |         |   +-------------+     |                       |                       |
 |    |         |<--------|---| Start A72   |     |                       |                       |
 |    |         |         |   | and jump to |     |                       |                       |
 |    |         |         |   | DM fw image |     |                       |                       |
 |    |         |         |   +-------------+     |                       |                       |
 |    |         |         |                       |     +-----------+     |                       |
 |    |         |---------|-----------------------|---->| Reset rls |     |                       |
 |    |         |         |                       |     +-----------+     |                       |
 |    |  TIFS   |         |                       |          :            |                       |
 |    |Services |         |                       |     +-----------+     |                       |
 |    |         |<--------|-----------------------|---->|*ATF/OPTEE*|     |                       |
 |    |         |         |                       |     +-----------+     |                       |
 |    |         |         |                       |          :            |                       |
 |    |         |         |                       |     +-----------+     |                       |
 |    |         |<--------|-----------------------|---->| *A72 SPL* |     |                       |
 |    |         |         |                       |     +-----------+     |                       |
 |    |         |         |                       |     |   Load    |     |                       |
 |    |         |         |                       |     | u-boot.img|     |                       |
 |    |         |         |                       |     +-----------+     |                       |
 |    |         |         |                       |          :            |                       |
 |    |         |         |                       |     +-----------+     |                       |
 |    |         |<--------|-----------------------|---->| *U-Boot*  |     |                       |
 |    |         |         |                       |     +-----------+     |                       |
 |    |         |         |                       |     |  prompt   |     |                       |
 |    |         |         |                       |     +-----------+     |                       |
 |    |         |         |                       |     |  Load R5  |     |                       |
 |    |         |         |                       |     |  Firmware |     |                       |
 |    |         |         |                       |     +-----------+     |                       |
 |    |         |<--------|-----------------------|-----|  Start R5 |     |      +-----------+    |
 |    |         |---------|-----------------------|-----+-----------+-----|----->| R5 starts |    |
 |    |         |         |                       |     |  Load C6  |     |      +-----------+    |
 |    |         |         |                       |     |  Firmware |     |                       |
 |    |         |         |                       |     +-----------+     |                       |
 |    |         |<--------|-----------------------|-----|  Start C6 |     |      +-----------+    |
 |    |         |---------|-----------------------|-----+-----------+-----|----->| C6 starts |    |
 |    |         |         |                       |     |  Load C7  |     |      +-----------+    |
 |    |         |         |                       |     |  Firmware |     |                       |
 |    |         |         |                       |     +-----------+     |                       |
 |    |         |<--------|-----------------------|-----|  Start C7 |     |      +-----------+    |
 |    |         |---------|-----------------------|-----+-----------+-----|----->| C7 starts |    |
 |    +---------+         |                       |                       |      +-----------+    |
 |                        |                       |                       |                       |
 +------------------------------------------------------------------------+-----------------------+

- Here DMSC acts as master and provides all the critical services. R5/A72
  requests DMSC to get these services done as shown in the above diagram.

Sources:
--------
1. ATF:
	Tree: https://github.com/ARM-software/arm-trusted-firmware.git
	Branch: master

2. OPTEE:
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
1. ATF:

.. code-block:: bash

 $ make CROSS_COMPILE=aarch64-linux-gnu- ARCH=aarch64 PLAT=k3 \
        TARGET_BOARD=generic SPD=opteed

2. OPTEE:

.. code-block:: bash

 $ make PLATFORM=k3-j721e CFG_ARM64_core=y

3. U-Boot:

* 4.1 R5:

.. code-block:: bash

 $ make j721e_evm_r5_defconfig
 $ make CROSS_COMPILE=arm-linux-gnueabihf- \
        BINMAN_INDIRS=<path/to/ti-linux-firmware>

* 4.2 A72:

.. code-block:: bash

 $ make j721e_evm_a72_defconfig
 $ make CROSS_COMPILE=aarch64-linux-gnu- \
        BL31=<ATF dir>/build/k3/generic/release/bl31.bin \
        TEE=<OPTEE OS dir>/out/arm-plat-k3/core/tee-pager_v2.bin \
        BINMAN_INDIRS=<path/to/ti-linux-firmware>

Target Images
--------------
Copy the below images to an SD card and boot:
 - tiboot3.bin and sysfw.itb from step 4.1
 - tispl.bin, u-boot.img from 4.2

Image formats:
--------------

- tiboot3.bin:

.. code-block:: text

                +-----------------------+
                |        X.509          |
                |      Certificate      |
                | +-------------------+ |
                | |                   | |
                | |        R5         | |
                | |   u-boot-spl.bin  | |
                | |                   | |
                | +-------------------+ |
                | |                   | |
                | |     FIT header    | |
                | | +---------------+ | |
                | | |               | | |
                | | |   DTB 1...N   | | |
                | | +---------------+ | |
                | +-------------------+ |
                +-----------------------+

- tispl.bin

.. code-block:: text

                +-----------------------+
                |                       |
                |       FIT HEADER      |
                | +-------------------+ |
                | |                   | |
                | |      A72 ATF      | |
                | +-------------------+ |
                | |                   | |
                | |     A72 OPTEE     | |
                | +-------------------+ |
                | |                   | |
                | |      R5 DM FW     | |
                | +-------------------+ |
                | |                   | |
                | |      A72 SPL      | |
                | +-------------------+ |
                | |                   | |
                | |   SPL DTB 1...N   | |
                | +-------------------+ |
                +-----------------------+

- sysfw.itb

.. code-block:: text

                +-----------------------+
                |                       |
                |       FIT HEADER      |
                | +-------------------+ |
                | |                   | |
                | |     sysfw.bin     | |
                | +-------------------+ |
                | |                   | |
                | |    board config   | |
                | +-------------------+ |
                | |                   | |
                | |     PM config     | |
                | +-------------------+ |
                | |                   | |
                | |     RM config     | |
                | +-------------------+ |
                | |                   | |
                | |    Secure config  | |
                | +-------------------+ |
                +-----------------------+

R5 Memory Map:
--------------

.. list-table::
   :widths: 16 16 16
   :header-rows: 1

   * - Region
     - Start Address
     - End Address

   * - SPL
     - 0x41c00000
     - 0x41c40000

   * - EMPTY
     - 0x41c40000
     - 0x41c81920

   * - STACK
     - 0x41c85920
     - 0x41c81920

   * - Global data
     - 0x41c859f0
     - 0x41c85920

   * - Heap
     - 0x41c859f0
     - 0x41cf59f0

   * - BSS
     - 0x41cf59f0
     - 0x41cff9f0

   * - MCU Scratchpad
     - 0x41cff9fc
     - 0x41cffbfc

   * - ROM DATA
     - 0x41cffbfc
     - 0x41cfffff

OSPI:
-----
ROM supports booting from OSPI from offset 0x0.

Flashing images to OSPI:

Below commands can be used to download tiboot3.bin, tispl.bin, u-boot.img,
and sysfw.itb over tftp and then flash those to OSPI at their respective
addresses.

Commands for J721E:

.. code-block:: text

 => sf probe
 => tftp ${loadaddr} tiboot3.bin
 => sf update $loadaddr 0x0 $filesize
 => tftp ${loadaddr} tispl.bin
 => sf update $loadaddr 0x80000 $filesize
 => tftp ${loadaddr} u-boot.img
 => sf update $loadaddr 0x280000 $filesize
 => tftp ${loadaddr} sysfw.itb
 => sf update $loadaddr 0x6C0000 $filesize

Commands for J7200:

.. code-block:: text

 => sf probe
 => tftp ${loadaddr} tiboot3.bin
 => sf update $loadaddr 0x0 $filesize
 => tftp ${loadaddr} tispl.bin
 => sf update $loadaddr 0x100000 $filesize
 => tftp ${loadaddr} u-boot.img
 => sf update $loadaddr 0x300000 $filesize

Flash layout for OSPI on J721E:

.. code-block:: text

         0x0 +----------------------------+
             |     ospi.tiboot3(512K)     |
             |                            |
     0x80000 +----------------------------+
             |     ospi.tispl(2M)         |
             |                            |
    0x280000 +----------------------------+
             |     ospi.u-boot(4M)        |
             |                            |
    0x680000 +----------------------------+
             |     ospi.env(128K)         |
             |                            |
    0x6A0000 +----------------------------+
	           |   ospi.env.backup (128K)   |
	           |                            |
    0x6C0000 +----------------------------+
             |      ospi.sysfw(1M)        |
             |                            |
    0x7C0000 +----------------------------+
	           |      padding (256k)        |
    0x800000 +----------------------------+
             |     ospi.rootfs(UBIFS)     |
             |                            |
             +----------------------------+

Flash layout for OSPI on j7200:

.. code-block:: text

        0x0 +----------------------------+
            |     ospi.tiboot3(1M)       |
            |                            |
   0x100000 +----------------------------+
            |     ospi.tispl(2M)         |
            |                            |
   0x300000 +----------------------------+
            |     ospi.u-boot(4M)        |
            |                            |
   0x700000 +----------------------------+
            |     ospi.env(128K)         |
            |                            |
   0x720000 +----------------------------+
            |   ospi.env.backup(128K)    |
            |                            |
   0x740000 +----------------------------+
            |      padding (768k)        |
   0x800000 +----------------------------+
            |     ospi.rootfs(UBIFS)     |
            |                            |
            +----------------------------+


eMMC:
-----

ROM supports booting from eMMC from boot0 partition offset 0x0

Flashing images to eMMC:

The following commands can be used to download tiboot3.bin, tispl.bin,
u-boot.img, and sysfw.itb from an SD card and write them to the eMMC boot0
partition at respective addresses.

Commands for j721e:

.. code-block:: text

 => mmc dev 0 1
 => fatload mmc 1 ${loadaddr} tiboot3.bin
 => mmc write ${loadaddr} 0x0 0x400
 => fatload mmc 1 ${loadaddr} tispl.bin
 => mmc write ${loadaddr} 0x400 0x1000
 => fatload mmc 1 ${loadaddr} u-boot.img
 => mmc write ${loadaddr} 0x1400 0x2000
 => fatload mmc 1 ${loadaddr} sysfw.itb
 => mmc write ${loadaddr} 0x3600 0x800

Commands for j7200:

.. code-block:: text

 => mmc dev 0 1
 => fatload mmc 1 ${loadaddr} tiboot3.bin
 => mmc write ${loadaddr} 0x0 0x800
 => fatload mmc 1 ${loadaddr} tispl.bin
 => mmc write ${loadaddr} 0x800 0x1000
 => fatload mmc 1 ${loadaddr} u-boot.img
 => mmc write ${loadaddr} 0x1800 0x2000

To give the ROM access to the boot partition, the following command must be
used for the first time:

.. code-block:: text

 => mmc partconf 0 1 1 1

To set bus width, reset bus width and data rate during boot, the following
command must be used for the first time:

.. code-block:: text

 => mmc bootbus 0 2 0 0

To create a software partition for the rootfs, the following command can be
used:

.. code-block:: text

 => gpt write mmc 0 ${partitions}

eMMC layout in J721e:

.. code-block:: text

             boot0 partition (8 MB)                        user partition
     0x0+----------------------------------+      0x0+------------------------+
        |     tiboot3.bin (512 KB)         |       |                         |
   0x400----------------------------------         |                         |
        |       tispl.bin (2 MB)           |       |                         |
  0x1400----------------------------------         |        rootfs           |
        |       u-boot.img (4 MB)          |       |                         |
  0x3400----------------------------------         |                         |
        |      environment (128 KB)        |       |                         |
  0x3500----------------------------------         |                         |
        |   backup environment (128 KB)    |       |                         |
  0x3600----------------------------------         |                         |
        |          sysfw (1 MB)            |       |                         |
  0x3E00+----------------------------------+       +-------------------------+

eMMC layout in J7200:

.. code-block:: text

            boot0 partition (8 MB)                        user partition
    0x0+----------------------------------     0x0+-------------------------+
       |     tiboot3.bin (1 MB)           |       |                         |
  0x800+----------------------------------        |                         |
       |       tispl.bin (2 MB)           |       |                         |
  0x1800+----------------------------------       |        rootfs           |
       |       u-boot.img (4 MB)          |       |                         |
  0x3800+----------------------------------       |                         |
       |      environment (128 KB)        |       |                         |
  0x3900+----------------------------------       |                         |
       |   backup environment (128 KB)    |       |                         |
  0x3A00+----------------------------------+      +-------------------------+

Kernel image and DT are expected to be present in the /boot folder of rootfs.
To boot kernel from eMMC, use the following commands:

.. code-block:: text

  => setenv mmcdev 0
  => setenv bootpart 0
  => boot

Firmwares:
----------

The J721e u-boot allows firmware to be loaded for the Cortex-R5 subsystem.
The CPSW5G in J7200 and CPSW9G in J721E present in MAIN domain is configured
and controlled by the ethernet firmware that executes in the MAIN Cortex R5.
The default supported environment variables support loading these firmwares
from only MMC. "dorprocboot" env variable has to be set for the U-BOOT to load
and start the remote cores in the system.

J721E common processor board can be attached to a Ethernet QSGMII card and the
PHY in the card has to be reset before it can be used for data transfer.
"do_main_cpsw0_qsgmii_phyinit" env variable has to be set for the U-BOOT to
configure this PHY.
