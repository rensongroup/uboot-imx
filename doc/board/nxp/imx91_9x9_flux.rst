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

Yocto fragments select the environment with ``CONFIG_ENV_SOURCE_FILE``
and the product device tree with ``CONFIG_DEFAULT_DEVICE_TREE`` /
``CONFIG_DEFAULT_FDT_FILE``. The Recovair DTS is supplied by Yocto. Manufacturing
sets the FIT configuration through the ``fit_conf`` environment variable or
the ``bootm_ren`` command argument.
