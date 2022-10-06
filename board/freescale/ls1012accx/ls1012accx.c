// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2016 Freescale Semiconductor, Inc., 2022 CCX Technologies
 */

#include <common.h>
#include <command.h>
#include <fdt_support.h>
#include <hang.h>
#include <i2c.h>
#include <asm/cache.h>
#include <init.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/fsl_serdes.h>
#ifdef CONFIG_FSL_LS_PPA
#include <asm/arch/ppa.h>
#endif
#include <asm/arch/mmu.h>
#include <asm/arch/soc.h>
#include <hwconfig.h>
#include <ahci.h>
#include <mmc.h>
#include <scsi.h>
#include <fsl_esdhc.h>
#include <env_internal.h>
#include <fsl_mmdc.h>
#include <netdev.h>

DECLARE_GLOBAL_DATA_PTR;

int checkboard(void)
{
	puts("Board: CCX Technologies DataPHY-NG\n");
	return 0;
}

int dram_init(void)
{
	gd->ram_size = tfa_get_dram_size();
	if (!gd->ram_size)
		gd->ram_size = CONFIG_SYS_SDRAM_SIZE;

	return 0;
}

int set_vbus_voltage(void)
{
	int err;

	err = i2c_set_bus_num(0);
	if (err < 0) {
		printf("Failed to set I2C Bus to IIC1.\n");
		return err;
	}

	i2c_reg_write(0x08, 0x66, 0x24);
	if (i2c_reg_read(0x08, 0x66) != 0x24) {
		printf("Failed to set USB VBUS Voltage to 6V.\n");
		return err;
	}

	printf("USB VBUS: 5V\n");

	return 0;
}


int pld_enable_reset_req(void)
{
	int err;
	u32 rstrqsr1, rstrqmr1;

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

	err = i2c_set_bus_num(0);
	if (err < 0) {
		printf("Failed to set I2C Bus to IIC1.\n");
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

	i2c_reg_write(0x20, 0x02, 0x93);
	if (i2c_reg_read(0x20, 0x02) != 0x83) {
		printf("Failed to set direction on bank b.\n");
		return err;
	}

	i2c_reg_write(0x20, 0x00, 0x48);
	if ((i2c_reg_read(0x20, 0x00) & 0x48) != 0x48) {
		printf("Failed to release resets on bank b.\n");
		return err;
	}

	printf("Reset PLD: enabled\n");

	return 0;
}

int read_povdd_state(void)
{
	int ret;

	ret = i2c_set_bus_num(0);
	if (ret < 0) {
		printf("Failed to set I2C Bus to IIC1.\n");
		return ret;
	}

	if (i2c_reg_read(0x08, 0x6c) == 0x10) {
		printf("Fuse POVDD: enabled\n");

		ret = env_set("povdd", "enabled");
		if (ret) {
			printf("Failed to set povdd to enabled: ret = %d\n", ret);
			return ret;
		}
	} else {
		printf("Fuse POVDD: disabled\n");

		ret = env_set("povdd", "disabled");
		if (ret) {
			printf("Failed to set povdd to disabled: ret = %d\n", ret);
			return ret;
		}
	}

	return 0;
}


int board_early_init_f(void)
{
	fsl_lsch2_early_init_f();

	return 0;
}

#ifdef CONFIG_MISC_INIT_R
int misc_init_r(void)
{
	if (pld_enable_reset_req()) {
		printf("Failed to enable reset PLD\n");
		return -1;
	}

	if (set_vbus_voltage()) {
		printf("Failed to set vbus voltage\n");
		return -1;
	}

	if (read_povdd_state()) {
		printf("Failed to get povdd state\n");
		return -1;
	}

	return 0;
}
#endif

int board_init(void)
{
	struct ccsr_cci400 *cci = (struct ccsr_cci400 *)(CONFIG_SYS_IMMR +
					CONFIG_SYS_CCI400_OFFSET);
	/*
	 * Set CCI-400 control override register to enable barrier
	 * transaction
	 */
	if (current_el() == 3)
		out_le32(&cci->ctrl_ord, CCI400_CTRLORD_EN_BARRIER);

#ifdef CONFIG_SYS_FSL_ERRATUM_A010315
	erratum_a010315();
#endif

#ifdef CONFIG_ENV_IS_NOWHERE
	gd->env_addr = (ulong)&default_environment[0];
#endif

#ifdef CONFIG_FSL_LS_PPA
	ppa_init();
#endif
	return 0;
}

int ft_board_setup(void *blob, struct bd_info *bd)
{
	arch_fixup_fdt(blob);

	ft_cpu_setup(blob, bd);

	return 0;
}
