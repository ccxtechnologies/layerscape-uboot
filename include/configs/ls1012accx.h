/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2020 NXP
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2022 CCX Technologies
 *
 */

#ifndef __LS1012ACCX_H__
#define __LS1012ACCX_H__

#include "ls1012a_common.h"

/* This should be incremented to force a firmware update, it isn't a
 * version inidicator but a compatibility indicator, this shouldn't
 * be updated on changes that don't break backwards compatibility */
#define CCX_FIRMWARE_API_VERSION	"10"

/* DDR */
#define CONFIG_DIMM_SLOTS_PER_CTLR		1
#define CONFIG_CHIP_SELECTS_PER_CTRL	1
#define CONFIG_SYS_SDRAM_SIZE			0x40000000

#define CONFIG_PCIE1		/* PCIE controller 1 */

#define CONFIG_PCI_SCAN_SHOW

#if defined(CONFIG_SYSTEMX_LOADER_FUSES)
#undef QSPI_NOR_BOOTCOMMAND
#define QSPI_NOR_BOOTCOMMAND "run system_fuse"
#elif defined(CONFIG_SYSTEMX_LOADER_LOAD)
#undef QSPI_NOR_BOOTCOMMAND
#define QSPI_NOR_BOOTCOMMAND "run system_load"
#elif defined(CONFIG_SYSTEMX_LOADER_NONE)
#undef QSPI_NOR_BOOTCOMMAND
#define QSPI_NOR_BOOTCOMMAND "echo Hardware Development Mode"
#else
#undef QSPI_NOR_BOOTCOMMAND
#define QSPI_NOR_BOOTCOMMAND "run system_boot"
#endif

#undef CONFIG_EXTRA_ENV_SETTINGS
#define CONFIG_EXTRA_ENV_SETTINGS \
	"crypto_set_key=" \
		"setenv _key_addr ${loadaddr_ram_key} && " \
		"mw.l ${_key_addr} 0x00000000 && " \
		"setexpr _key_addr ${_key_addr} + 0x4 && " \
		"mw.l ${_key_addr} 0x00000000 && " \
		"setexpr _key_addr ${_key_addr} + 0x4 && " \
		"mw.l ${_key_addr} 0x00000000 && " \
		"setexpr _key_addr ${_key_addr} + 0x4 && " \
		"mw.l ${_key_addr} 0x00000000\0" \
	"crypto_encrypt=" \
		"run crypto_set_key && " \
		"blob enc ${loadaddr_ram_dec} ${loadaddr_ram_enc} ${filesize} ${loadaddr_ram_key} && " \
		"setexpr filesize ${filesize} + 0x30\0" \
	"crypto_decrypt=" \
		"run crypto_set_key && " \
		"setexpr filesize ${filesize} - 0x30 && " \
		"blob dec ${loadaddr_ram_enc} ${loadaddr_ram_dec} ${filesize} ${loadaddr_ram_key}\0" \
	"crypto_verify_kernel=esbc_validate ${loadaddr_ram_kernel_header} ${crypto_key_kernel}\0" \
	"crypto_verify_dtb=esbc_validate ${loadaddr_ram_dtb_header} ${crypto_key_dtb}\0" \
	"crypto_key_kernel=\0" \
	"crypto_key_dtb=\0" \
	"hwconfig=fsl_ddr:bank_intlv=auto\0" \
	"bootargs=ccx.firmware=" CCX_FIRMWARE_API_VERSION "\0"\
	"bootargs_enable_loader=setenv bootargs ${bootargs} loader\0" \
	"bootargs_set_rootfs=setenv bootargs ${bootargs} root=/dev/sda${bootarg_rootpart} ro\0" \
	"bootargs_set_console=setenv bootargs ${bootargs} console=ttyS0,${baudrate} earlycon=uart8250,mmio,0x21c0500\0" \
	"bootargs_set_ccx=setenv bootargs ${bootargs} ccx.serialnum=\"${serialnum}\"\0" \
	"bootarg_rootpart=2\0" \
	"loadaddr_ram=0xa0000000\0" \
	"loadaddr_ram_dec=0xa0400000\0" \
	"loadaddr_ram_enc=0xa0800000\0" \
	"loadaddr_flash=0x0\0" \
	"loadaddr_flash_bl2=0x000000\0" \
	"loadaddr_flash_bl2_sec=0x80000\0" \
	"loadaddr_flash_fip=0x100000\0" \
	"loadaddr_flash_fip_sec=0x400000\0" \
	"loadaddr_flash_ids=0x500000\0" \
	"loadaddr_ram_kernel=0xa0800000\0" \
	"loadaddr_ram_dtb=0xa0000000\0" \
	"loadaddr_ram_key=0x87000000\0" \
	"loadaddr_ram_kernel_header=0x80200000\0" \
	"loadaddr_ram_dtb_header=0x80100000\0" \
	"filename_onetimeenv=uEnv.onetime.txt\0" \
	"ram_to_flash=" \
		"sf erase ${loadaddr_flash} +${filesize} && " \
		"sf write ${loadaddr_ram} ${loadaddr_flash} ${filesize}\0" \
	"ram_to_sata=" \
		"scsi rescan && " \
		"ext4write scsi 0:1 ${loadaddr_ram} /${filename} ${filesize}\0" \
	"sata_to_ram=ext4load scsi 0:1 ${loadaddr_ram} /${filename}\0" \
	"usb_to_ram=ext4load usb 0 ${loadaddr_ram} /${filename}\0" \
	"flash_to_ram=sf read ${loadaddr_ram} ${loadaddr_flash} ${filesize}\0" \
	"usb_to_flash=" \
		"run usb_to_ram && " \
		"run ram_to_flash\0" \
	"usb_to_flash_pbl=" \
		"setenv filename bl2.pbl && " \
		"setenv loadaddr_flash ${loadaddr_flash_bl2} && " \
		"run usb_to_flash\0" \
	"usb_to_flash_fib=" \
		"setenv filename fip.bin && " \
		"setenv loadaddr_flash ${loadaddr_flash_fip} && " \
		"run usb_to_flash\0" \
	"usb_to_ram_dtb=" \
		"setenv filename linux.dtb && " \
		"setenv loadaddr_ram ${loadaddr_ram_dtb} && " \
		"run usb_to_ram\0" \
	"usb_to_ram_kernel=" \
		"setenv filename Image && " \
		"setenv loadaddr_ram ${loadaddr_ram_kernel} && " \
		"run usb_to_ram\0" \
	"usb_to_ram_dtb_sig=" \
		"setenv filename linux.dtb.sig && " \
		"setenv loadaddr_ram ${loadaddr_ram_dtb_header} && " \
		"run usb_to_ram\0" \
	"usb_to_ram_kernel_sig=" \
		"setenv filename Image.sig && " \
		"setenv loadaddr_ram ${loadaddr_ram_kernel_header} && " \
		"run usb_to_ram\0" \
	"sata_to_ram_dtb=" \
		"setenv filename boot-${bootarg_rootpart}/linux.dtb && " \
		"setenv loadaddr_ram ${loadaddr_ram_dtb} && " \
		"run sata_to_ram\0" \
	"sata_to_ram_kernel=" \
		"setenv filename boot-${bootarg_rootpart}/Image && " \
		"setenv loadaddr_ram ${loadaddr_ram_kernel} && " \
		"run sata_to_ram\0" \
	"sata_to_ram_dtb_sig=" \
		"setenv filename boot-${bootarg_rootpart}/linux.dtb.sig && " \
		"setenv loadaddr_ram ${loadaddr_ram_dtb_header} && " \
		"run sata_to_ram\0" \
	"sata_to_ram_kernel_sig=" \
		"setenv filename boot-${bootarg_rootpart}/Image.sig && " \
		"setenv loadaddr_ram ${loadaddr_ram_kernel_header} && " \
		"run sata_to_ram\0" \
	"load_sec_bootloader=" \
		"sf probe && " \
		"setenv loadaddr_flash ${loadaddr_flash_bl2_sec} && " \
		"setenv filesize 0x50000 && " \
		"run flash_to_ram && " \
		"setenv loadaddr_flash ${loadaddr_flash_bl2} && " \
		"echo Overwriting Stage 1 Bootloader, not not turn off power! && " \
		"run ram_to_flash && " \
		"setenv loadaddr_flash ${loadaddr_flash_fip_sec} && " \
		"setenv filesize 0x200000 && " \
		"run flash_to_ram && " \
		"setenv loadaddr_flash ${loadaddr_flash_fip} && " \
		"echo Overwriting Stage 2 Bootloader, not not turn off power! && " \
		"run ram_to_flash && " \
		"echo Bootloader Overwritten, Rebooting System&& " \
		"reset\0" \
	"set_rootpart_from_defaultrootpart=" \
		"if test \"${defaultrootpart}\" = \"2\" ; then " \
			"setenv bootarg_rootpart 2; " \
		"fi; " \
		"if test \"${defaultrootpart}\" = \"3\" ; then " \
			"setenv bootarg_rootpart 3; " \
		"fi\0" \
	"set_rootpart_from_onetimerootpart=" \
		"if test \"${onetimerootpart}\" = \"2\" ; then " \
			"setenv bootarg_rootpart 2; " \
		"fi; " \
		"if test \"${onetimerootpart}\" = \"3\" ; then " \
			"setenv bootarg_rootpart 3; " \
		"fi\0" \
	"sata_to_env_rootpart=" \
		"setenv filename ${filename_onetimeenv} && " \
		"if run sata_to_ram; then " \
			"env import -t ${loadaddr_ram} ${filesize} lastboot defaultrootpart onetimerootpart; " \
		"fi; " \
		"if env exists defaultrootpart; then "\
			"run set_rootpart_from_defaultrootpart; " \
		"else " \
			"setenv defaultrootpart ${bootarg_rootpart}; " \
		"fi; " \
		"if env exists onetimerootpart; then " \
			"run set_rootpart_from_onetimerootpart; " \
		"else " \
			"true; " \
		"fi\0" \
	"env_to_sata_rootpart=" \
		"if env exists onetimerootpart || test \"${lastboot}\" != \"${bootarg_rootpart}\"; then " \
			"setenv lastboot ${bootarg_rootpart} && " \
			"env export -t ${loadaddr_ram} lastboot defaultrootpart && " \
			"setenv filename ${filename_onetimeenv} && " \
			"run ram_to_sata; " \
		"else " \
			"true; " \
		"fi\0" \
	"boot_kernel_loader=" \
		"run bootargs_enable_loader && " \
		"run boot_kernel_usb\0" \
	"boot_kernel_usb=" \
		"pfe stop && " \
		"run bootargs_set_console && " \
		"run bootargs_set_ccx && " \
		"run usb_to_ram_dtb && " \
		"run usb_to_ram_dtb_sig && " \
		"run crypto_verify_dtb && " \
		"run usb_to_ram_kernel && " \
		"run usb_to_ram_kernel_sig && " \
		"run crypto_verify_kernel && " \
		"booti ${loadaddr_ram_kernel} - ${loadaddr_ram_dtb}\0" \
	"boot_kernel_sata=" \
		"pfe stop && " \
		"run bootargs_set_rootfs && " \
		"run bootargs_set_console && " \
		"run bootargs_set_ccx && " \
		"run sata_to_ram_dtb && " \
		"run sata_to_ram_dtb_sig && " \
		"run crypto_verify_dtb && " \
		"run sata_to_ram_kernel && " \
		"run sata_to_ram_kernel_sig && " \
		"run crypto_verify_kernel && " \
		"booti ${loadaddr_ram_kernel} - ${loadaddr_ram_dtb}\0" \
	"system_set_ids=" \
		"askenv serialnum \"Enter Serial Number [nnnn], ie. 1062 => \" 6 && " \
		"askenv ethaddr \"Enter MAC Address 1 [xx:xx:xx:xx:xx:xx], ie. 84:8B:CD:20:00:C8 => \" 17 && " \
		"askenv eth1addr \"Enter MAC Address 2 [xx:xx:xx:xx:xx:xx], ie. 84:8B:CD:20:00:C9 => \" 17 && " \
		"env export -t ${loadaddr_ram_dec} serialnum ethaddr eth1addr && " \
		"setenv filesize 0x3d0 && " \
		"run crypto_encrypt && " \
		"setenv loadaddr_ram ${loadaddr_ram_enc} && " \
		"setenv loadaddr_flash ${loadaddr_flash_ids} && " \
		"run ram_to_flash\0" \
	"system_get_ids=" \
		"setenv loadaddr_ram ${loadaddr_ram_enc} && " \
		"setenv loadaddr_flash ${loadaddr_flash_ids} && " \
		"setenv filesize 0x400 && " \
		"run flash_to_ram && " \
		"run crypto_decrypt && " \
		"env import -t ${loadaddr_ram_dec} ${filesize} serialnum ethaddr eth1addr\0" \
	"system_load=" \
		"sf probe && " \
		"run system_set_ids && " \
		"usb reset && " \
		"if run usb_to_flash_pbl && run usb_to_flash_fib; then " \
			"run boot_kernel_loader; esbc_halt; " \
		"else " \
			"echo Failed to find file firmware ${filename} && esbc_halt; " \
		"fi\0" \
	"system_boot=" \
		"sf probe && " \
		"run system_get_ids; " \
		"scsi rescan; " \
		"run sata_to_env_rootpart; " \
		"run env_to_sata_rootpart; " \
		"run boot_kernel_sata; " \
		"esbc_halt;\0" \
	"fuse_burn=i2c mw 08 6c 10 && reset\0" \
	"system_fuse=" \
		"if test \"${povdd}\" = \"enabled\"; " \
		"then echo !!! FUSES BURNED, LOADING SIGNED BOOTLOADER !!!; " \
		"run load_sec_bootloader;" \
		"fi; " \
		"if test \"${povdd}\" = \"disabled\"; " \
		"then echo !!! POVDD LDO is disabled, run fuse_burn to enable and burn fuses !!!; " \
		"fi\0"

#include <asm/fsl_secure_boot.h>

#endif /* __LS1012ACCX_H__ */
