// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2024 NXP
 */

#include <command.h>
#include <cpu_func.h>
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

#define SRC_DDRC_SW_CTRL		(SRC_BASE_ADDR + 0x1020)

#ifndef CONFIG_IMX9_DRAM_INLINE_ECC
extern struct dram_timing_info dram_timing_alliance;
#endif
extern struct dram_timing_info dram_timing_test;
u32 lpddr4_mr_read(u32 mr_rank, u32 mr_addr);

static const struct {
	struct dram_timing_info *timing;
	u32 mr5;
	const char *name;
} dram_profiles[] = {
	{ &dram_timing_test, 0, "invalid test" },
	{ &dram_timing, 0x13, "ISSI" },
#ifndef CONFIG_IMX9_DRAM_INLINE_ECC
	{ &dram_timing_alliance, 0x1a, "Alliance" },
#endif
};

static void ddrmix_power_cycle(void)
{
	setbits_32(REG_SRC_DPHY_SINGLE_RESET_SW_CTRL, BIT(0));
	setbits_32(SRC_DDRC_SW_CTRL, BIT(31));
	udelay(50);
	clrbits_32(SRC_DDRC_SW_CTRL, BIT(31));
	setbits_32(SRC_DDRC_SW_CTRL, BIT(0));
	udelay(10);
	clrbits_32(SRC_DDRC_SW_CTRL, BIT(0));
	udelay(10);
}

void spl_dram_init(void)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(dram_profiles); i++) {
		struct dram_timing_info *timing = dram_profiles[i].timing;
		u32 mr5;
		int ret;

		if (i)
			ddrmix_power_cycle();
		printf("DDR: trying %s profile at %uMTS\n",
		       dram_profiles[i].name, timing->fsp_msg[0].drate);
		ret = ddr_init(timing);
		if (ret) {
			printf("DDR: initialization failed: %d\n", ret);
			continue;
		}

		mr5 = lpddr4_mr_read(1, 5) & 0xff;
		if (mr5 == dram_profiles[i].mr5)
			break;

		printf("DDR: MR5 0x%02x does not match the %s profile\n",
		       mr5, dram_profiles[i].name);
		if (i + 1 == ARRAY_SIZE(dram_profiles) ||
		    mr5 != dram_profiles[i + 1].mr5)
			hang();
	}

	if (i == ARRAY_SIZE(dram_profiles)) {
		puts("DDR: no matching memory profile initialized\n");
		hang();
	}

	// ISSI 	--> DDR: LPDDR4 MR5 manufacturer ID: 0x13, MR6/7 revision: 0x00/0x02, MR8 organization: 0x10
	// ALLIANCE --> DDR: LPDDR4 MR5 manufacturer ID: 0x1a, MR6/7 revision: 0x00/0x00, MR8 organization: 0x08
	printf("DDR: LPDDR4 MR5 manufacturer ID: 0x%02x, "
	       "MR6/7 revision: 0x%02x/0x%02x, MR8 organization: 0x%02x\n",
	       dram_profiles[i].mr5,
	       lpddr4_mr_read(1, 6) & 0xff,
	       lpddr4_mr_read(1, 7) & 0xff,
	       lpddr4_mr_read(1, 8) & 0xff);
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
