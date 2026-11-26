# Renson U-Boot Customization Overview

This document summarizes Renson-specific board enablement and customization in this `uboot-imx` tree.

## Board Support Added by Renson

- Added support for `imx91-9x9-flux-v2 `.
- Added support for `imx91-9x9-flux`.
- Added/customized support for FRDM-based platforms used by Renson (for example `imx93_frdm` and related boot environment updates).

## Flux Family Layout

Flux and Recovair use the same U-Boot target and board implementation:

- `board/freescale/imx91_flux`

Both products use `configs/imx91_9x9_renson_defconfig`. SPL tries ISSI timing
first and checks the LPDDR4 manufacturer ID (MR5). If initialization returns
an error or MR5 identifies Alliance memory, SPL resets DDRMIX and tries Alliance
timing. The DRAM SKU fuse is no longer used or programmed. Yocto configuration
fragments still select the product device tree and environment.

- Shared target: `CONFIG_TARGET_IMX91_9X9_FLUX`
- Recovair device tree: `CONFIG_DEFAULT_DEVICE_TREE="imx91-9x9-recovair"` and `CONFIG_DEFAULT_FDT_FILE="imx91-9x9-recovair.dtb"`
- Shared environment: `board/freescale/imx91_flux/imx91_flux.env`
- FIT configuration: set `fit_conf` in the manufacturing environment or pass it to `bootm_ren`

## FRDM Customizations

Renson has also customized FRDM-oriented flows, including boot environment improvements such as explicit FDT addresses in board env files.

Examples:

- `board/freescale/imx91_flux/imx91_flux.env`
- `board/freescale/imx93_frdm/imx93_frdm.env`

## Build Quick Start

Use GNU make (`gmake`) on macOS.

- Flux / Recovair:
  - `gmake imx91_9x9_renson_defconfig`
- Build:
  - `gmake -j8`

## Environment Note

Some host setups require OpenSSL development headers for U-Boot host tools (`openssl/evp.h`).
