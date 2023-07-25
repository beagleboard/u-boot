.. SPDX-License-Identifier: GPL-2.0+ OR BSD-3-Clause
.. sectionauthor:: Nishanth Menon <nm@ti.com>

AM62x Beagleboard.org Beagleplay
================================

Introduction:
-------------

BeagleBoard.org BeaglePlay is an easy to use, affordable open source
hardware single board computer based on the Texas Instruments AM625
SoC that allows you to create connected devices that work even at long
distances using IEEE 802.15.4g LR-WPAN and IEEE 802.3cg 10Base-T1L.
Expansion is provided over open standards based mikroBUS, Grove and
QWIIC headers among other interfaces.

Further information can be found at:

* Product Page: https://beagleplay.org/
* Hardware documentation: https://git.beagleboard.org/beagleplay/beagleplay

Boot Flow:
----------
Below is the pictorial representation of boot flow:

.. image:: img/boot_diagram_k3_current.svg

- On this platform, 'TI Foundational Security' (TIFS) functions as the
  security enclave master while 'Device Manager' (DM), also known as the
  'TISCI server' in "TI terminology", offers all the essential services.
  The A53/M4F (Aux core) sends requests to TIFS/DM to accomplish these
  services, as illustrated in the diagram above.

Sources:
--------
.. include::  k3.rst
    :start-after: .. k3_rst_include_start_boot_sources
    :end-before: .. k3_rst_include_end_boot_sources

Build procedure:
----------------
0. Setup the environment variables:

.. include::  k3.rst
    :start-after: .. k3_rst_include_start_common_env_vars_desc
    :end-before: .. k3_rst_include_end_common_env_vars_desc

.. include::  k3.rst
    :start-after: .. k3_rst_include_start_board_env_vars_desc
    :end-before: .. k3_rst_include_end_board_env_vars_desc

Set the variables corresponding to this platform:

.. include::  k3.rst
    :start-after: .. k3_rst_include_start_common_env_vars_defn
    :end-before: .. k3_rst_include_end_common_env_vars_defn
.. code-block:: bash

 $ export UBOOT_CFG_CORTEXR="am62x_evm_r5_defconfig beagleplay_r5.config"
 $ export UBOOT_CFG_CORTEXA="am62x_evm_a53_defconfig beagleplay_a53.config"
 $ export TFA_BOARD=lite
 $ # we dont use any extra TFA parameters
 $ unset TFA_EXTRA_ARGS
 $ export OPTEE_PLATFORM=k3-am62x
 $ export OPTEE_EXTRA_ARGS="CFG_WITH_SOFTWARE_PRNG=y"

.. include::  am62x_sk.rst
    :start-after: .. am62x_evm_rst_include_start_build_steps
    :end-before: .. am62x_evm_rst_include_end_build_steps

Target Images
--------------
Copy the below images to an SD card and boot:

* tiboot3-am62x-gp-evm.bin from R5 build as tiboot3.bin
* tispl.bin_unsigned from Cortex-A build as tispl.bin
* u-boot.img_unsigned from Cortex-A build as uboot.img

Image formats:
--------------

- tiboot3.bin

.. image:: img/multi_cert_tiboot3.bin.svg

- tispl.bin

.. image:: img/dm_tispl.bin.svg

A53 SPL DDR Memory Layout
-------------------------

.. include::  am62x_sk.rst
    :start-after: .. am62x_evm_rst_include_start_ddr_mem_layout
    :end-before: .. am62x_evm_rst_include_end_ddr_mem_layout

Switch Setting for Boot Mode
----------------------------

The boot time option is configured via "USR" button on the board.
See `Beagleplay Schematics <https://git.beagleboard.org/beagleplay/beagleplay/-/blob/main/BeaglePlay_sch.pdf>`_
for details.

.. list-table:: Boot Modes
   :widths: 16 16 16
   :header-rows: 1

   * - USR Switch Position
     - Primary Boot
     - Secondary Boot

   * - Not Pressed
     - eMMC
     - UART

   * - Pressed
     - SD/MMC File System (FS) mode
     - USB Device Firmware Upgrade (DFU) mode

To switch to SD card boot mode, hold the USR button while powering on
with Type-C power supply, then release when power LED lights up.
