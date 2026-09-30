// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2024 NXP
 */

#include <command.h>
#include <cpu_func.h>
#include <fuse.h>
#include <hang.h>
#include <image.h>
#include <init.h>
#include <log.h>
#include <spl.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <asm/arch/mu.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <asm/arch-mx7ulp/gpio.h>
#include <asm/mach-imx/ele_api.h>
#include <asm/mach-imx/syscounter.h>
#include <asm/sections.h>
#include <dm/uclass.h>
#include <dm/device.h>
#include <dm/uclass-internal.h>
#include <dm/device-internal.h>
#include <linux/delay.h>
#include <asm/arch/clock.h>
#include <asm/arch/ccm_regs.h>
#include <asm/arch/ddr.h>
#include <power/pmic.h>
#include <power/pf9453.h>
#include <asm/arch/trdc.h>

DECLARE_GLOBAL_DATA_PTR;

int spl_board_boot_device(enum boot_device boot_dev_spl)
{
	return BOOT_DEVICE_BOOTROM;
}

void spl_board_init(void)
{
	int ret;

	ret = ele_start_rng();
	if (ret)
		printf("Fail to start RNG: %d\n", ret);

	puts("Normal Boot\n");
}

#define DRAM_SKU_FUSE_BANK	47
#define DRAM_SKU_FUSE_WORD	0
#define DRAM_SKU_ALLIANCE		BIT(0)

#ifndef CONFIG_IMX9_DRAM_INLINE_ECC
extern struct dram_timing_info dram_timing_alliance;
#endif

void spl_dram_init(void)
{
	struct dram_timing_info *ptiming = &dram_timing;
	u32 sku;
	int ret;

	ret = fuse_sense(DRAM_SKU_FUSE_BANK, DRAM_SKU_FUSE_WORD, &sku);
	if (ret) {
		printf("DDR: failed to sense SKU fuse: %d\n", ret);
		hang();
	}

#ifdef CONFIG_TARGET_IMX91_9X9_RECOVAIR_ALLIANCE
	if (!(sku & DRAM_SKU_ALLIANCE)) {
		puts("DDR: permanently programming Alliance SKU fuse\n");
		ret = fuse_prog(DRAM_SKU_FUSE_BANK, DRAM_SKU_FUSE_WORD,
				DRAM_SKU_ALLIANCE);
		if (ret) {
			printf("DDR: failed to program SKU fuse: %d\n", ret);
			hang();
		}

		ret = fuse_sense(DRAM_SKU_FUSE_BANK, DRAM_SKU_FUSE_WORD, &sku);
		if (ret) {
			printf("DDR: failed to verify SKU fuse: %d\n", ret);
			hang();
		}
		if (!(sku & DRAM_SKU_ALLIANCE)) {
			puts("DDR: Alliance SKU fuse verification failed\n");
			hang();
		}
	}
#endif

	if (sku & DRAM_SKU_ALLIANCE) {
#ifdef CONFIG_IMX9_DRAM_INLINE_ECC
		puts("DDR: Alliance inline ECC timing is not supported\n");
		hang();
#else
		ptiming = &dram_timing_alliance;
#endif
	}

	printf("DDR: memory profile: %s\n",
	       (sku & DRAM_SKU_ALLIANCE) ? "Alliance" : "standard");
	printf("DDR: %uMTS\n", ptiming->fsp_msg[0].drate);
	ddr_init(ptiming);
}

#if CONFIG_IS_ENABLED(DM_PMIC_PF9453)
int power_init_board(void)
{
	struct udevice *dev;
	int ret;
	unsigned int buck_val;

	ret = pmic_get("pmic@32", &dev);
	if (ret == -ENODEV) {
		puts("No pf9453@32\n");
		return 0;
	}
	if (ret != 0)
		return ret;

	if (is_voltage_mode(VOLT_LOW_DRIVE)) {
		buck_val = 0x10; /* 0.8v for Low drive mode */
		printf("PMIC: Low Drive Voltage Mode\n");
	} else if (is_voltage_mode(VOLT_NOMINAL_DRIVE)) {
		buck_val = 0x14; /* 0.85v for Nominal drive mode */
		printf("PMIC: Nominal Voltage Mode\n");
	} else {
		buck_val = 0x18; /* 0.9v for Over drive mode */
		printf("PMIC: Over Drive Voltage Mode\n");
	}

	pmic_reg_write(dev, PF9453_BUCK2OUT, buck_val);

	/* set standby voltage to 0.65v */
	pmic_reg_write(dev, PF9453_BUCK2OUT_STBY, 0x4);

	/* 1.1v for LPDDR4 */
	pmic_reg_write(dev, PF9453_BUCK1OUT, 0x14);

	ret = pmic_reg_read(dev, PF9453_CONFIG1);
	if (ret < 0)
		return ret;
	/*The timer default is 8sec and too long, so change to 100ms.*/
	pmic_reg_write(dev, PF9453_CONFIG1, ~PF9453_RESETKEY_TIMER_MASK & ret);

	/* set WDOG_B_CFG to cold reset */
	pmic_reg_write(dev, PF9453_RESET_CTRL, 0xA0);
	return 0;
}
#endif

void board_init_f(ulong dummy)
{
	int ret;

	/* Set GPIO2_19, 20, 21 to 0 (active low - all LEDs ON) */
	struct gpio_regs *gpio2 = (struct gpio_regs *)(GPIO2_BASE_ADDR + 0x40);
	setbits_le32(&gpio2->gpio_pddr, BIT(19) | BIT(21)); /* Set as output */
	setbits_le32(&gpio2->gpio_psor, BIT(19) | BIT(21)); /* Set low (all LEDs ON) */

	/* Set 4 5 and 20 to 0 so that fan is off and power meter is off */
	setbits_le32(&gpio2->gpio_pcor, BIT(4) | BIT(5) | BIT(20));

	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

	timer_init();

	arch_cpu_init();

	board_early_init_f();

	spl_early_init();

	preloader_console_init();

	ret = imx9_probe_mu();
	if (ret) {
		printf("Fail to init ELE API\n");
	} else {
		debug("SOC: 0x%x\n", gd->arch.soc_rev);
		debug("LC: 0x%x\n", gd->arch.lifecycle);
	}

	clock_init_late();

	power_init_board();

	if (!is_voltage_mode(VOLT_LOW_DRIVE))
		set_arm_core_max_clk();

	/* Init power of mix */
	soc_power_init();

	/* Setup TRDC for DDR access */
	trdc_init();

	/* DDR initialization */
	spl_dram_init();

	board_init_r(NULL, 0);
}
