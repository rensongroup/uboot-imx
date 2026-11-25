.. SPDX-License-Identifier: GPL-2.0+

i.MX91 9x9 Flux DRAM selection
=============================

The Flux SPL selects ISSI or Alliance LPDDR4 timing from the memory SKU fuse
in bank 47, word 0, bit 0 (absolute word 376). A clear bit selects ISSI timing;
a set bit selects Alliance timing. Both timing profiles are included in
non-ECC builds.

For an Alliance board, ``IMX91_DRAM_ALLIANCE`` selects Alliance timing without
checking the SKU fuse. Add ``IMX91_DRAM_PROGRAM_ALLIANCE_FUSE`` to also program
and verify the fuse after DDR initialization succeeds. This programming option
is irreversible. A generic subsequent U-Boot build omits both settings and
selects Alliance timing from the programmed fuse. If programming is disabled,
keep using the Alliance override unless the fuse is already set.

.. warning::

   Fuse programming is irreversible. Boot the Alliance provisioning image only
   on boards fitted with Alliance memory. GPR2_CFG0 bit 0 must remain allocated
   exclusively to this DRAM selector. Fuse write/read protection must permit
   provisioning and subsequent reads.

SPL halts on fuse access, programming, or verification failures. The existing
inline-ECC profiles are not Alliance profiles, so an ECC image halts if the
Alliance bit is set.

Flux and Recovair products use the single ``CONFIG_TARGET_IMX91_9X9_FLUX``
target. Yocto fragments select the environment with ``CONFIG_ENV_SOURCE_FILE``
and the product device tree with ``CONFIG_DEFAULT_DEVICE_TREE`` /
``CONFIG_DEFAULT_FDT_FILE``. The Recovair DTS is supplied by Yocto. Manufacturing
sets the FIT configuration through the ``fit_conf`` environment variable or
the ``bootm_ren`` command argument. The fuse selects DRAM timing, not the device
tree.
