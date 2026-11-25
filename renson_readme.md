# Renson U-Boot Customization Overview

This document summarizes Renson-specific board enablement and customization in this `uboot-imx` tree.

## Board Support Added by Renson

- Added support for `imx91-9x9-flux-v2 `.
- Added support for `imx91-9x9-flux`.
- Added/customized support for FRDM-based platforms used by Renson (for example `imx93_frdm` and related boot environment updates).

## Flux Family Layout

Flux and Recovair use the same U-Boot target and board implementation:

- `board/freescale/imx91_flux`

Use the Flux defconfig as the shared base. Yocto configuration fragments
select the product device tree and environment. SPL selects DRAM timing from
the programmed SKU fuse; an Alliance first-boot fragment selects Alliance
timing and programs the fuse after DDR initialization:

- Flux: `configs/imx91_9x9_flux_defconfig`
- Shared target: `CONFIG_TARGET_IMX91_9X9_FLUX`
- Recovair device tree: `CONFIG_DEFAULT_DEVICE_TREE="imx91-9x9-recovair"` and `CONFIG_DEFAULT_FDT_FILE="imx91-9x9-recovair.dtb"`
- Shared environment: `board/freescale/imx91_flux/imx91_flux.env`
- FIT configuration: set `fit_conf` in the manufacturing environment or pass it to `bootm_ren`
- Alliance timing override: `CONFIG_IMX91_DRAM_ALLIANCE`
- Optional post-DDR Alliance fuse programming: `CONFIG_IMX91_DRAM_PROGRAM_ALLIANCE_FUSE`

## FRDM Customizations

Renson has also customized FRDM-oriented flows, including boot environment improvements such as explicit FDT addresses in board env files.

Examples:

- `board/freescale/imx91_flux/imx91_flux.env`
- `board/freescale/imx93_frdm/imx93_frdm.env`

## Build Quick Start

Use GNU make (`gmake`) on macOS.

- Flux:
  - `gmake imx91_9x9_flux_defconfig`
- Build:
  - `gmake -j8`

## Environment Note

Some host setups require OpenSSL development headers for U-Boot host tools (`openssl/evp.h`).
