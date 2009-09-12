/***************************************************************************

    drivers/bebox.c

    BeBox

***************************************************************************/

/* Core includes */
#include "driver.h"
#include "memconv.h"
#include "devconv.h"
#include "includes/bebox.h"

/* Components */
#include "video/pc_vga.h"
#include "video/cirrus.h"
#include "cpu/powerpc/ppc.h"
#include "sound/3812intf.h"
#include "machine/ins8250.h"
#include "machine/pic8259.h"
#include "machine/mc146818.h"
#include "machine/pc_fdc.h"
#include "machine/pci.h"
#include "machine/8237dma.h"
#include "machine/pckeybrd.h"
#include "machine/8042kbdc.h"
#include "machine/pit8253.h"
#include "machine/idectrl.h"
#include "machine/mpc105.h"

/* Devices */
#include "devices/flopdrv.h"
#include "devices/chd_cd.h"
#include "devices/harddriv.h"
#include "formats/pc_dsk.h"


static READ8_HANDLER(at_dma8237_1_r)  { return dma8237_r(devtag_get_device(space->machine, "dma8237_2"), offset / 2); }
static WRITE8_HANDLER(at_dma8237_1_w) { dma8237_w(devtag_get_device(space->machine, "dma8237_2"), offset / 2, data); }

static READ64_HANDLER( bebox_dma8237_1_r )
{
	return read64be_with_read8_handler(at_dma8237_1_r, space, offset, mem_mask);
}

static WRITE64_HANDLER( bebox_dma8237_1_w )
{
	write64be_with_write8_handler(at_dma8237_1_w, space, offset, data, mem_mask);
}


static ADDRESS_MAP_START( bebox_mem, ADDRESS_SPACE_PROGRAM, 64 )
	AM_RANGE(0x7FFFF0F0, 0x7FFFF0F7) AM_READWRITE( bebox_cpu0_imask_r, bebox_cpu0_imask_w )
	AM_RANGE(0x7FFFF1F0, 0x7FFFF1F7) AM_READWRITE( bebox_cpu1_imask_r, bebox_cpu1_imask_w )
	AM_RANGE(0x7FFFF2F0, 0x7FFFF2F7) AM_READ( bebox_interrupt_sources_r )
	AM_RANGE(0x7FFFF3F0, 0x7FFFF3F7) AM_READWRITE( bebox_crossproc_interrupts_r, bebox_crossproc_interrupts_w )
	AM_RANGE(0x7FFFF4F0, 0x7FFFF4F7) AM_WRITE( bebox_processor_resets_w )

	AM_RANGE(0x80000000, 0x8000001F) AM_DEVREADWRITE8( "dma8237_1", dma8237_r, dma8237_w, U64(0xffffffffffffffff) )
	AM_RANGE(0x80000020, 0x8000003F) AM_DEVREADWRITE8( "pic8259_master", pic8259_r, pic8259_w, U64(0xffffffffffffffff) )
	AM_RANGE(0x80000040, 0x8000005f) AM_DEVREADWRITE8( "pit8254", pit8253_r, pit8253_w, U64(0xffffffffffffffff) )
	AM_RANGE(0x80000060, 0x8000006F) AM_READWRITE( kbdc8042_64be_r, kbdc8042_64be_w )
	AM_RANGE(0x80000070, 0x8000007F) AM_READWRITE( mc146818_port64be_r, mc146818_port64be_w )
	AM_RANGE(0x80000080, 0x8000009F) AM_READWRITE( bebox_page_r, bebox_page_w)
	AM_RANGE(0x800000A0, 0x800000BF) AM_DEVREADWRITE8( "pic8259_slave", pic8259_r, pic8259_w, U64(0xffffffffffffffff) )
	AM_RANGE(0x800000C0, 0x800000DF) AM_READWRITE( bebox_dma8237_1_r, bebox_dma8237_1_w)
	AM_RANGE(0x800001F0, 0x800001F7) AM_READWRITE( bebox_800001F0_r, bebox_800001F0_w )
	AM_RANGE(0x800002F8, 0x800002FF) AM_DEVREADWRITE8( "ns16550_1", ins8250_r, ins8250_w, U64(0xffffffffffffffff) )
	AM_RANGE(0x80000380, 0x80000387) AM_DEVREADWRITE8( "ns16550_2", ins8250_r, ins8250_w, U64(0xffffffffffffffff) )
	AM_RANGE(0x80000388, 0x8000038F) AM_DEVREADWRITE8( "ns16550_3", ins8250_r, ins8250_w, U64(0xffffffffffffffff) )
	AM_RANGE(0x800003F0, 0x800003F7) AM_READWRITE( bebox_800003F0_r, bebox_800003F0_w )
	AM_RANGE(0x800003F8, 0x800003FF) AM_DEVREADWRITE8( "ns16550_0", ins8250_r, ins8250_w, U64(0xffffffffffffffff) )
	AM_RANGE(0x80000480, 0x8000048F) AM_READWRITE( bebox_80000480_r, bebox_80000480_w )
	AM_RANGE(0x80000CF8, 0x80000CFF) AM_DEVREADWRITE("pcibus", pci_64be_r, pci_64be_w )
	AM_RANGE(0x800042E8, 0x800042EF) AM_WRITE( cirrus_64be_42E8_w )

	AM_RANGE(0xBFFFFFF0, 0xBFFFFFFF) AM_READ( bebox_interrupt_ack_r )

	AM_RANGE(0xFFF00000, 0xFFF03FFF) AM_ROMBANK(2)
	AM_RANGE(0xFFF04000, 0xFFFFFFFF) AM_READWRITE( bebox_flash_r, bebox_flash_w )
ADDRESS_MAP_END

// The following is a gross hack to let the BeBox boot ROM identify the processors correctly.
// This needs to be done in a better way if someone comes up with one.

static READ64_HANDLER(bb_slave_64be_r)
{
	const device_config *device = devtag_get_device(space->machine, "pcibus");

	// 2e94 is the real address, 2e84 is where the PC appears to be under full DRC
	if ((cpu_get_pc(space->cpu) == 0xfff02e94) || (cpu_get_pc(space->cpu) == 0xfff02e84))
	{
		return 0x108000ff;	// indicate slave CPU
	}

	return pci_64be_r(device, offset, mem_mask);
}

static ADDRESS_MAP_START( bebox_slave_mem, ADDRESS_SPACE_PROGRAM, 64 )
	AM_RANGE(0x80000cf8, 0x80000cff) AM_READ(bb_slave_64be_r)
	AM_RANGE(0x80000cf8, 0x80000cff) AM_DEVWRITE("pcibus", pci_64be_w )
	AM_IMPORT_FROM(bebox_mem)
ADDRESS_MAP_END


static MACHINE_DRIVER_START( bebox )
	/* basic machine hardware */
	MDRV_CPU_ADD("ppc1", PPC603, 66000000)	/* 66 MHz */
	MDRV_CPU_PROGRAM_MAP(bebox_mem)

	MDRV_CPU_ADD("ppc2", PPC603, 66000000)	/* 66 MHz */
	MDRV_CPU_PROGRAM_MAP(bebox_slave_mem)

	MDRV_QUANTUM_TIME(HZ(60))

	MDRV_PIT8254_ADD( "pit8254", bebox_pit8254_config )

	MDRV_DMA8237_ADD( "dma8237_1", bebox_dma8237_1_config )

	MDRV_DMA8237_ADD( "dma8237_2", bebox_dma8237_2_config )

	MDRV_PIC8259_ADD( "pic8259_master", bebox_pic8259_master_config )

	MDRV_PIC8259_ADD( "pic8259_slave", bebox_pic8259_slave_config )

	MDRV_NS16550_ADD( "ns16550_0", bebox_uart_inteface_0 )			/* TODO: Verify model */
	MDRV_NS16550_ADD( "ns16550_1", bebox_uart_inteface_1 )			/* TODO: Verify model */
	MDRV_NS16550_ADD( "ns16550_2", bebox_uart_inteface_2 )			/* TODO: Verify model */
	MDRV_NS16550_ADD( "ns16550_3", bebox_uart_inteface_3 )			/* TODO: Verify model */

	MDRV_IDE_CONTROLLER_ADD( "ide", bebox_ide_interrupt )	/* FIXME */

	/* video hardware */
	MDRV_IMPORT_FROM( pcvideo_vga )
	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */

	MDRV_MACHINE_START( bebox )
	MDRV_MACHINE_RESET( bebox )

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("ym3812", YM3812, 3579545)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MDRV_NVRAM_HANDLER( bebox )

	MDRV_CDROM_ADD( "cdrom" )
	MDRV_HARDDISK_ADD( "harddisk1" )

	/* pci */
	MDRV_PCI_BUS_ADD("pcibus", 0)
	MDRV_PCI_BUS_DEVICE(0, NULL, mpc105_pci_read, mpc105_pci_write)
	MDRV_PCI_BUS_DEVICE(1, NULL, cirrus5430_pci_read, cirrus5430_pci_write)
	/*MDRV_PCI_BUS_DEVICE(12, NULL, scsi53c810_pci_read, scsi53c810_pci_write)*/

	MDRV_SMC37C78_ADD("smc37c78", pc_fdc_nec765_connected_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( bebox2 )
	MDRV_IMPORT_FROM( bebox )
	MDRV_CPU_REPLACE("ppc1", PPC603E, 133000000)	/* 133 MHz */
	MDRV_CPU_REPLACE("ppc2", PPC603E, 133000000)	/* 133 MHz */
MACHINE_DRIVER_END

static INPUT_PORTS_START( bebox )
	PORT_INCLUDE( at_keyboard )
INPUT_PORTS_END

ROM_START(bebox)
    ROM_REGION(0x00200000, "user1", ROMREGION_64BIT | ROMREGION_BE)
	ROM_LOAD( "bootmain.rom", 0x000000, 0x20000, CRC(df2d19e0) SHA1(da86a7d23998dc953dd96a2ac5684faaa315c701) )
    ROM_REGION(0x4000, "user2", ROMREGION_64BIT | ROMREGION_BE)
	ROM_LOAD( "bootnub.rom", 0x000000, 0x4000, CRC(5348d09a) SHA1(1b637a3d7a2b072aa128dd5c037bbb440d525c1a) )
ROM_END

ROM_START(bebox2)
    ROM_REGION(0x00200000, "user1", ROMREGION_64BIT | ROMREGION_BE)
	ROM_LOAD( "bootmain.rom", 0x000000, 0x20000, CRC(df2d19e0) SHA1(da86a7d23998dc953dd96a2ac5684faaa315c701) )
    ROM_REGION(0x4000, "user2", ROMREGION_64BIT | ROMREGION_BE)
	ROM_LOAD( "bootnub.rom", 0x000000, 0x4000, CRC(5348d09a) SHA1(1b637a3d7a2b072aa128dd5c037bbb440d525c1a) )
ROM_END

static void bebox_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_FLOPPY_OPTIONS:				info->p = (void *) floppyoptions_pc; break;

		default:			floppy_device_getinfo(devclass, state, info); break;
	}
}


static SYSTEM_CONFIG_START(bebox)
	CONFIG_RAM(8 * 1024 * 1024)
	CONFIG_RAM(16 * 1024 * 1024)
	CONFIG_RAM_DEFAULT(32 * 1024 * 1024)
	CONFIG_DEVICE(bebox_floppy_getinfo)
SYSTEM_CONFIG_END


/*     YEAR   NAME      PARENT  COMPAT  MACHINE   INPUT     INIT    CONFIG  COMPANY             FULLNAME */
COMP( 1995,  bebox,    0,      0,      bebox,    bebox,    bebox,  bebox,  "Be Incorporated",  "BeBox Dual603-66", GAME_NOT_WORKING )
COMP( 1996,  bebox2,   bebox,  0,      bebox2,   bebox,    bebox,  bebox,  "Be Incorporated",  "BeBox Dual603-133", GAME_NOT_WORKING )
