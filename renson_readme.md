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
may still supply product-specific Linux device trees and FIT configurations.

- Shared target: `CONFIG_TARGET_IMX91_9X9_FLUX`
- Shared U-Boot device tree: `arch/arm/dts/imx91-9x9-flux.dts`
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

## Factory network boot (Flux)

`board/freescale/imx91_flux/imx91_flux.env` contains two boot paths, selected
by `boot_mode` in the stored environment (both `boot_mode` and `device_type` are
on the `CFG_ENV_FLAGS_LIST_STATIC` writeable list in `include/configs/imx91_flux.h`):

| `boot_mode` | Behaviour |
|---|---|
| `nfs` | `nfs_bootcmd`: allowlist `device_type` (`flux`), `dhcp`, use `${serverip}` (DHCP next-server/siaddr, as on Sitara, RAM only): `tftpboot ${cntr_addr} ${serverip}:imx91/os_cntr_signed.bin` (one i.MX91 FIT), authenticate and boot it with `bootm_ren` and `mfg.nfsroot=${serverip}:/imx/manufacturing mfg.device_type=<device_type> rauc.slot=A` (shared manufacturing root). Any failure: wait 10 s and reset; never boot unverified, never fall back to eMMC. |
| `emmc`, unset, invalid/missing env | normal local A/B boot (`fit_bootcmd` -> `boot.scr`). No network access. |

For testing on a network whose DHCP server does not point next-server at the TFTP/NFS server, set
`mfg_server_ip=<ip>` in the stored env (e.g. `fw_setenv mfg_server_ip 192.168.1.10`
or `setenv mfg_server_ip ...; saveenv` in U-Boot); it replaces `serverip`.
Remove it (`fw_setenv mfg_server_ip`) for production.

The supplier writes `boot_mode=nfs`; Renson manufacturing runs
`fw_setenv boot_mode emmc` after provisioning. Networking (DHCP/TFTP, EQOS,
TI DP83825 PHY via `CONFIG_PHY_TI_GENERIC`) is enabled in the defconfig and in
the Yocto `production_boot.cfg` fragment.

### Environment loading

The env is stored once in eMMC at `0x700000`, size `0x4000` (CRC32 + data).
With `CONFIG_ENV_WRITEABLE_LIST=y`, `env_load()` first loads the full built-in
default env and then imports the stored env; variables not on
`CFG_ENV_FLAGS_LIST_STATIC` are ignored. Boot scripts always come from the
binary; the stored env only provides listed variables (`boot_mode`,
`device_type`, `mfg_server_ip`, RAUC `BOOT_ORDER`/`BOOT_x_LEFT`, ...). A bad CRC
(blank or erased env, e.g. during the supplier USB boot) means defaults only,
i.e. the eMMC boot path. The supplier writes the factory env
(`boot_mode=nfs`, `device_type=flux`) as its last step; manufacturing changes
only `boot_mode` with `fw_setenv`, so RAUC's state is kept.
