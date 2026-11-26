.. SPDX-License-Identifier: GPL-2.0+

i.MX91 9x9 Flux DRAM selection
=============================

Flux and Recovair use ``imx91_9x9_renson_defconfig`` and share the
``CONFIG_TARGET_IMX91_9X9_FLUX`` target. SPL tries ISSI LPDDR4 timing first and
checks the manufacturer ID (MR5) after initialization. If initialization
returns an error or MR5 identifies Alliance memory, SPL resets DDRMIX and
tries the Alliance timing. SPL halts if no supported profile matches.
Inline-ECC builds only include the ISSI profile. SPL does not read or
program the DRAM SKU fuse.

DDR retry test
--------------

SPL currently tries a test timing profile first. It copies the ISSI profile
but uses a zero training-frequency field in the first PHY training message.
If the firmware reports a training failure, SPL power-cycles DDRMIX and tries
the normal profiles. Check the serial log and confirm the board subsequently
boots with the installed DRAM.

This invalid configuration may hang during hardware polling rather than
return an error, and its failure is not guaranteed. Use only a recoverable
test board and remove the test profile before production.

Both products use the U-Boot control device tree ``imx91-9x9-flux.dts`` and
the shared ``imx91_flux.env`` environment. Yocto may supply product-specific
Linux device trees. Manufacturing sets the FIT configuration through the
``fit_conf`` environment variable or the ``bootm_ren`` command argument.
