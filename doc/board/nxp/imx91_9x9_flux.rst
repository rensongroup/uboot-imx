.. SPDX-License-Identifier: GPL-2.0+

i.MX91 9x9 Flux DRAM selection
=============================

Non-ECC Flux SPL images include both the standard and Alliance LPDDR4 timing
profiles. Before DDR initialization, SPL senses bit 0 of GPR2_CFG0 (bank 47,
word 0, absolute word 376). A clear bit selects the standard profile; a set bit
selects the Alliance profile.

The ``imx91_9x9_recovair_alliance_defconfig`` image is a provisioning image:
on its first boot, SPL permanently programs this bit if it is clear and senses
it again to verify the write. It programs only bit 0 and does not request a
fuse lock. Subsequent boots do not program the bit again. Other Flux targets
only sense the bit and never program it.

.. warning::

   Fuse programming is irreversible. Boot the Alliance provisioning image only
   on boards fitted with Alliance memory. GPR2_CFG0 bit 0 must remain allocated
   exclusively to this DRAM selector. Fuse write/read protection must permit
   provisioning and subsequent reads.

SPL halts on fuse access, programming, or verification failures rather than
selecting an unverified memory profile. The existing inline-ECC profiles are
not Alliance profiles: an ECC Flux image halts if the Alliance bit is set, and
an Alliance ECC build is rejected.

The Recovair DTS is supplied by Yocto. Build the production image with the same
appropriate device tree; the fuse selects DRAM timing, not the device tree.
