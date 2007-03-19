/***************************************************************************

	systems/bebox.c

	BeBox

***************************************************************************/

#include "includes/bebox.h"
#include "video/pc_vga.h"
#include "cpu/powerpc/ppc.h"
#include "machine/uart8250.h"
#include "machine/pic8259.h"
#include "machine/mc146818.h"
#include "machine/pc_fdc.h"
#include "machine/pci.h"
#include "machine/8237dma.h"
#include "machine/pckeybrd.h"
#include "machine/8042kbdc.h"
#include "machine/pit8253.h"
#include "devices/mflopimg.h"
#include "devices/chd_cd.h"
#include "devices/harddriv.h"
#include "formats/pc_dsk.h"
#include "video/cirrus.h"
#include "memconv.h"


static READ8_HANDLER(at_dma8237_1_r)  { return dma8237_1_r(offset / 2); }
static WRITE8_HANDLER(at_dma8237_1_w) { dma8237_1_w(offset / 2, data); }

static READ64_HANDLER( bebox_dma8237_1_r )
{
	return read64be_with_read8_handler(at_dma8237_1_r, offset, mem_mask);
}

static WRITE64_HANDLER( bebox_dma8237_1_w )
{
	write64be_with_write8_handler(at_dma8237_1_w, offset, data, mem_mask);
}

static ADDRESS_MAP_START( bebox_mem, ADDRESS_SPACE_PROGRAM, 64 )
	AM_RANGE(0x7FFFF0F0, 0x7FFFF0F7) AM_READWRITE( bebox_cpu0_imask_r, bebox_cpu0_imask_w )
	AM_RANGE(0x7FFFF1F0, 0x7FFFF1F7) AM_READWRITE( bebox_cpu1_imask_r, bebox_cpu1_imask_w )
	AM_RANGE(0x7FFFF2F0, 0x7FFFF2F7) AM_READ( bebox_interrupt_sources_r )
	AM_RANGE(0x7FFFF3F0, 0x7FFFF3F7) AM_READWRITE( bebox_crossproc_interrupts_r, bebox_crossproc_interrupts_w )
	AM_RANGE(0x7FFFF4F0, 0x7FFFF4F7) AM_WRITE( bebox_processor_resets_w )

	AM_RANGE(0x80000000, 0x8000001F) AM_READWRITE( dma8237_64be_0_r, dma8237_64be_0_w )
	AM_RANGE(0x80000020, 0x8000003F) AM_READWRITE( pic8259_64be_0_r, pic8259_64be_0_w )
	AM_RANGE(0x80000040, 0x8000005f) AM_READWRITE( pit8253_64be_0_r, pit8253_64be_0_w )
	AM_RANGE(0x80000060, 0x8000006F) AM_READWRITE( kbdc8042_64be_r, kbdc8042_64be_w )
	AM_RANGE(0x80000070, 0x8000007F) AM_READWRITE( mc146818_port64be_r, mc146818_port64be_w )
	AM_RANGE(0x80000080, 0x8000009F) AM_READWRITE( bebox_page_r, bebox_page_w)
	AM_RANGE(0x800000A0, 0x800000BF) AM_READWRITE( pic8259_64be_1_r, pic8259_64be_1_w )
	AM_RANGE(0x800000C0, 0x800000DF) AM_READWRITE( bebox_dma8237_1_r, bebox_dma8237_1_w)
	AM_RANGE(0x800001F0, 0x800001F7) AM_READWRITE( bebox_800001F0_r, bebox_800001F0_w )
	AM_RANGE(0x800002F8, 0x800002FF) AM_READWRITE( uart8250_64be_1_r, uart8250_64be_1_w )
	AM_RANGE(0x80000380, 0x80000387) AM_READWRITE( uart8250_64be_2_r, uart8250_64be_2_w )
	AM_RANGE(0x80000388, 0x8000038F) AM_READWRITE( uart8250_64be_3_r, uart8250_64be_3_w )
	AM_RANGE(0x800003F0, 0x800003F7) AM_READWRITE( bebox_800003F0_r, bebox_800003F0_w )
	AM_RANGE(0x800003F8, 0x800003FF) AM_READWRITE( uart8250_64be_0_r, uart8250_64be_0_w )
	AM_RANGE(0x80000480, 0x8000048F) AM_READWRITE( bebox_80000480_r, bebox_80000480_w )
	AM_RANGE(0x80000CF8, 0x80000CFF) AM_READWRITE( pci_64be_r, pci_64be_w )
	AM_RANGE(0x800042E8, 0x800042EF) AM_WRITE( cirrus_64be_42E8_w )

	AM_RANGE(0xBFFFFFF0, 0xBFFFFFFF) AM_READ( bebox_interrupt_ack_r )

	AM_RANGE(0xFFF00000, 0xFFF03FFF) AM_ROMBANK(2)
	AM_RANGE(0xFFF04000, 0xFFFFFFFF) AM_READWRITE( bebox_flash_r, bebox_flash_w )
ADDRESS_MAP_END


static ppc_config bebox_ppc_config =
{
	PPC_MODEL_603,	/* 603 "Wart"					*/
	0x10,		/* Multiplier 1.0, Bus = 66MHz, Core = 66MHz	*/
	BUS_FREQUENCY_66MHZ
};

static ppc_config bebox2_ppc_config =
{
	PPC_MODEL_603E,	/* 603E "Stretch", version 1.3			*/
	0x19,		/* Multiplier 2.0, Bus = 66MHz, Core = 133MHz	*/
	BUS_FREQUENCY_66MHZ
};


static MACHINE_DRIVER_START( bebox )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("ppc1", PPC603, 66000000)	/* 66 MHz */
	MDRV_CPU_CONFIG(bebox_ppc_config)
	MDRV_CPU_PROGRAM_MAP(bebox_mem, 0)

	MDRV_CPU_ADD_TAG("ppc2", PPC603, 66000000)	/* 66 MHz */
	MDRV_CPU_CONFIG(bebox_ppc_config)
	MDRV_CPU_PROGRAM_MAP(bebox_mem, 0)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_IMPORT_FROM( pcvideo_vga )

	MDRV_MACHINE_RESET( bebox )

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(YM3812, 3579545)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MDRV_NVRAM_HANDLER( bebox )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( bebox2 )
	MDRV_IMPORT_FROM( bebox )
	MDRV_CPU_REPLACE("ppc1", PPC603, 133000000)	/* 133 MHz */
	MDRV_CPU_CONFIG(bebox2_ppc_config)

	MDRV_CPU_REPLACE("ppc2", PPC603, 133000000)	/* 133 MHz */
	MDRV_CPU_CONFIG(bebox2_ppc_config)
MACHINE_DRIVER_END

static INPUT_PORTS_START( bebox )
	PORT_INCLUDE( at_keyboard )
INPUT_PORTS_END

ROM_START(bebox)
    ROM_REGION(0x00200000, REGION_USER1, ROMREGION_64BIT | ROMREGION_BE)
	ROM_LOAD( "bootmain.rom", 0x000000, 0x20000, CRC(df2d19e0) SHA1(da86a7d23998dc953dd96a2ac5684faaa315c701) )
    ROM_REGION(0x4000, REGION_USER2, ROMREGION_64BIT | ROMREGION_BE)
	ROM_LOAD( "bootnub.rom", 0x000000, 0x4000, CRC(5348d09a) SHA1(1b637a3d7a2b072aa128dd5c037bbb440d525c1a) )
ROM_END

ROM_START(bebox2)
    ROM_REGION(0x00200000, REGION_USER1, ROMREGION_64BIT | ROMREGION_BE)
	ROM_LOAD( "bootmain.rom", 0x000000, 0x20000, CRC(df2d19e0) SHA1(da86a7d23998dc953dd96a2ac5684faaa315c701) )
    ROM_REGION(0x4000, REGION_USER2, ROMREGION_64BIT | ROMREGION_BE)
	ROM_LOAD( "bootnub.rom", 0x000000, 0x4000, CRC(5348d09a) SHA1(1b637a3d7a2b072aa128dd5c037bbb440d525c1a) )
ROM_END

static void bebox_floppy_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_FLOPPY_OPTIONS:				info->p = (void *) floppyoptions_pc; break;

		default:										floppy_device_getinfo(devclass, state, info); break;
	}
}

static void bebox_cdrom_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cdrom */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										cdrom_device_getinfo(devclass, state, info); break;
	}
}

static void bebox_harddisk_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* harddisk */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										harddisk_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(bebox)
	CONFIG_RAM(8 * 1024 * 1024)
	CONFIG_RAM(16 * 1024 * 1024)
	CONFIG_RAM_DEFAULT(32 * 1024 * 1024)
	CONFIG_DEVICE(bebox_floppy_getinfo)
	CONFIG_DEVICE(bebox_cdrom_getinfo)
	CONFIG_DEVICE(bebox_harddisk_getinfo)
SYSTEM_CONFIG_END


/*     YEAR   NAME      PARENT  COMPAT  MACHINE   INPUT     INIT    CONFIG  COMPANY             FULLNAME */
COMP( 1995,  bebox,    0,      0,      bebox,    bebox,    bebox,  bebox,  "Be Incorporated",  "BeBox Dual603-66", GAME_NOT_WORKING )
COMP( 1996,  bebox2,   bebox,  0,      bebox2,   bebox,    bebox,  bebox,  "Be Incorporated",  "BeBox Dual603-133", GAME_NOT_WORKING )
