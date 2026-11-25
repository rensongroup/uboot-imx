# Renson U-Boot Customization Overview

This document summarizes Renson-specific board enablement and customization in this `uboot-imx` tree.

## Board Support Added by Renson

- Added support for `imx91-9x9-flux-v2 `.
- Added support for `imx91-9x9-flux`.
- Added/customized support for FRDM-based platforms used by Renson (for example `imx93_frdm` and related boot environment updates).

## Flux Family Layout

Flux and Recovair use the same U-Boot target and board implementation:

- `board/freescale/imx91_flux`

The three defconfigs share the same board target and vary by DRAM mode. The
Flux defconfig selects timing from the SKU fuse; supplier defconfigs force the
configured vendor. Yocto configuration fragments select the product device
tree and environment. Supplier Alliance also programs the fuse after DDR
initialization:

- Flux, fuse-selected DRAM: `configs/imx91_9x9_flux_defconfig`
- Supplier ISSI: `configs/imx91_9x9_supplier_issi_defconfig`
- Supplier Alliance: `configs/imx91_9x9_supplier_alliance_defconfig`
- Shared target: `CONFIG_TARGET_IMX91_9X9_FLUX`
- Recovair device tree: `CONFIG_DEFAULT_DEVICE_TREE="imx91-9x9-recovair"` and `CONFIG_DEFAULT_FDT_FILE="imx91-9x9-recovair.dtb"`
- Shared environment: `board/freescale/imx91_flux/imx91_flux.env`
- FIT configuration: set `fit_conf` in the manufacturing environment or pass it to `bootm_ren`
- Supplier ISSI forces timing with `CONFIG_IMX91_DRAM_ISSI`.
- Supplier Alliance forces timing with `CONFIG_IMX91_DRAM_ALLIANCE`.
- Only Supplier Alliance programs the fuse with `CONFIG_IMX91_DRAM_PROGRAM_ALLIANCE_FUSE`.

## FRDM Customizations

Renson has also customized FRDM-oriented flows, including boot environment improvements such as explicit FDT addresses in board env files.

Examples:

- `board/freescale/imx91_flux/imx91_flux.env`
- `board/freescale/imx93_frdm/imx93_frdm.env`

## Build Quick Start

Use GNU make (`gmake`) on macOS.

- Flux:
  - `gmake imx91_9x9_flux_defconfig`
- Supplier:
  - `gmake imx91_9x9_supplier_issi_defconfig` or `gmake imx91_9x9_supplier_alliance_defconfig`
- Build:
  - `gmake -j8`

## Environment Note

Some host setups require OpenSSL development headers for U-Boot host tools (`openssl/evp.h`).
