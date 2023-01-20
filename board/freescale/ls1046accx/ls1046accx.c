// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Updates Copyright 2020 CCX Technologies, Inc.
 */

#include <common.h>
#include <i2c.h>
#include <fdt_support.h>
#include <fsl_ddr_sdram.h>
#include <init.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/fsl_serdes.h>
#include <asm/arch/ppa.h>
#include <asm/arch/fdt.h>
#include <asm/arch/mmu.h>
#include <asm/arch/cpu.h>
#include <asm/arch/soc.h>
#include <asm/arch-fsl-layerscape/fsl_icid.h>
#include <ahci.h>
#include <hwconfig.h>
#include <mmc.h>
#include <scsi.h>
#include <fm_eth.h>
#include <fsl_csu.h>
#include <fsl_esdhc.h>
#include <fsl_ifc.h>
#include <spl.h>
#include <linux/delay.h>

DECLARE_GLOBAL_DATA_PTR;

int board_early_init_f(void)
{
	fsl_lsch2_early_init_f();

	return 0;
}

#ifndef CONFIG_SPL_BUILD
int checkboard(void)
{
	return 0;
}

/* configure GPIO1 19 and 21 as outputs, NOTE bit endianess flip  */
#define MASK_ETH_PHY_RST 0x00001400

void reset_plugin_phys(void)
{
	u32 val;
	struct ccsr_gpio *pgpio = (void *)(GPIO1_BASE_ADDR);

	val = in_be32(&pgpio->gpdir);
	val |=  MASK_ETH_PHY_RST;
	out_be32(&pgpio->gpdir, val);

	val = in_be32(&pgpio->gpdat);

	setbits_be32(&pgpio->gpdat, val & ~MASK_ETH_PHY_RST);
    mdelay(50);
	setbits_be32(&pgpio->gpdat, val | MASK_ETH_PHY_RST);

	printf("Plugin PHY Reset: complete\n");
}

void config_floating_gpio_as_outputs(void)
{
	u32 val;
	struct ccsr_gpio *pgpio = (void *)(GPIO2_BASE_ADDR);

	val = in_be32(&pgpio->gpdir);
	/* configure GPIO2 12-15 as outputs, NOTE bit endianess flip  */
	val |=  0x00070000;
	out_be32(&pgpio->gpdir, val);

	printf("GPIO Config: complete\n");
}

int pld_enable_reset_req(void)
{
	int err;
	u32 rstrqsr1, rstrqmr1;
	u8 banka_value;

	struct ccsr_gur *gur = (void *)(CONFIG_SYS_FSL_GUTS_ADDR);
	rstrqmr1 = in_be32(&gur->rstrqmr1);
	rstrqsr1 = in_be32(&gur->rstrqsr1);

	if (rstrqsr1 & (~rstrqmr1)) {
		printf("WARNING: RESET_REQ_B is active, the processor will"
		       " be reset as soon as the PLD is configured.\n");
		printf("RSTRQSR1 & RSTRQSM1: 0x%08x\n", rstrqsr1 & (~rstrqmr1));
	}


	/* enable the reset_req driven reset by programming
	 * the GPIO Expander / PLD on IIC3 (Semtech SX1503) */

	err = i2c_set_bus_num(2);
	if (err < 0) {
		printf("Failed to set I2C Bus to IIC3.\n");
		return err;
	}

	i2c_reg_write(0x20, 0x23, 0x0a);
	if (i2c_reg_read(0x20, 0x23) != 0x0a) {
		printf("Failed to set PLD mode on bank a.\n");
		return err;
	}

	i2c_reg_write(0x20, 0x21, 0x01);
	if (i2c_reg_read(0x20, 0x21) != 0x01) {
		printf("Failed to enable PLD on bank a.\n");
		return err;
	}

	i2c_reg_write(0x20, 0x03, 0x00);
	if (i2c_reg_read(0x20, 0x03) != 0x00) {
		printf("Failed to set direction of bank a.\n");
		return err;
	}

	i2c_reg_write(0x20, 0x01, 0xf0);
	banka_value = (i2c_reg_read(0x20, 0x01) & 0xf0);
	if (banka_value == 0xe0) {
		/* First revision, no resets */
		printf("Mainboard: Revision < D\n");
		i2c_reg_write(0x20, 0x03, 0xf0);
		if (i2c_reg_read(0x20, 0x03) != 0xf0) {
			printf("Failed to re-set direction of bank a.\n");
			return err;
		}

		i2c_reg_write(0x20, 0x02, 0x78);
		if (i2c_reg_read(0x20, 0x02) != 0x78) {
			printf("Failed to set direction on bank b.\n");
			return err;
		}

		i2c_reg_write(0x20, 0x00, 0x00);
		if ((i2c_reg_read(0x20, 0x00) & 0x87) != 0x00) {
			printf("Failed to set resets on bank b.\n");
			return err;
		}

		mdelay(50);

		i2c_reg_write(0x20, 0x00, 0x87);
		if ((i2c_reg_read(0x20, 0x00) & 0x87) != 0x87) {
			printf("Failed to release resets on bank b.\n");
			return err;
		}

	} else if (banka_value == 0xf0) {
		printf("Mainboard: Revision >= D\n");

		i2c_reg_write(0x20, 0x02, 0x30);
		if (i2c_reg_read(0x20, 0x02) != 0x30) {
			printf("Failed to set direction on bank b.\n");
			return err;
		}

		i2c_reg_write(0x20, 0x00, 0x08);
		if ((i2c_reg_read(0x20, 0x00) & 0xcf) != 0x08) {
			printf("Failed to set resets on bank b.\n");
			return err;
		}

		mdelay(50);

		i2c_reg_write(0x20, 0x00, 0x87);
		if ((i2c_reg_read(0x20, 0x00) & 0xcf) != 0x87) {
			printf("Failed to release resets on bank b.\n");
			return err;
		}

	} else {
		printf("Failed to read value of bank a.\n");
		return err;
	}

	err = i2c_set_bus_num(0);
	if (err < 0) {
		printf("Failed to set I2C Bus to IIC1.\n");
		return err;
	}

	printf("Reset PLD: enabled\n");

	return 0;
}

int board_init(void)
{
	struct ccsr_scfg *scfg = (struct ccsr_scfg *)CONFIG_SYS_FSL_SCFG_ADDR;

#ifdef CONFIG_NXP_ESBC
	/*
	 * In case of Secure Boot, the IBR configures the SMMU
	 * to allow only Secure transactions.
	 * SMMU must be reset in bypass mode.
	 * Set the ClientPD bit and Clear the USFCFG Bit
	 */
	u32 val;
	val = (in_le32(SMMU_SCR0) | SCR0_CLIENTPD_MASK) & ~(SCR0_USFCFG_MASK);
	out_le32(SMMU_SCR0, val);
	val = (in_le32(SMMU_NSCR0) | SCR0_CLIENTPD_MASK) & ~(SCR0_USFCFG_MASK);
	out_le32(SMMU_NSCR0, val);
#endif

#ifdef CONFIG_FSL_LS_PPA
	ppa_init();
#endif

	/* The GIC driver only supports HIGH level and RISING edge
	 * IRQs, so any LOW level of FALLING IRQs need to be
	 * inverted.
	 *
	 * - SCFG_INTPCR resgister is little endian bit order
	 * - the only non-inverted IRQ is IRQ5 (the acceleromter)
	 *
	 * */

	out_be32(&scfg->intpcr, 0xfbf00000);

	return 0;
}

int board_setup_core_volt(u32 vdd)
{
	return 0;
}

int power_init_board(void)
{
	return 0;
}

void config_board_mux(void)
{
	struct ccsr_scfg *scfg = (struct ccsr_scfg *)CONFIG_SYS_FSL_SCFG_ADDR;

	/* enable IIC3 */
	out_be32(&scfg->rcwpmuxcr0, 0x0000);
}

#ifdef CONFIG_MISC_INIT_R
int misc_init_r(void)
{
	config_board_mux();
	config_floating_gpio_as_outputs();
	reset_plugin_phys();
	return pld_enable_reset_req();
}
#endif


int ft_board_setup(void *blob, struct bd_info *bd)
{
	int i;
	u64 base[CONFIG_NR_DRAM_BANKS];
	u64 size[CONFIG_NR_DRAM_BANKS];

	/* fixup DT for the two DDR banks */
	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++) {
		base[i] = gd->bd->bi_dram[i].start;
		size[i] = gd->bd->bi_dram[i].size;
	}

	fdt_fixup_memory_banks(blob, base, size, CONFIG_NR_DRAM_BANKS);
	ft_cpu_setup(blob, bd);

#ifdef CONFIG_SYS_DPAA_FMAN
#ifndef CONFIG_DM_ETH
	fdt_fixup_fman_ethernet(blob);
#endif
#endif

	fdt_fixup_icid(blob);

	return 0;
}
#endif
