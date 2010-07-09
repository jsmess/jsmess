/***************************************************************************

        Morrow Designs Micro Decision

        10/12/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/upd765.h"
#include "devices/flopdrv.h"
#include "machine/terminal.h"

static UINT8 received_char = 0;

static WRITE8_HANDLER(microdec_terminal_w)
{
	running_device *devconf = space->machine->device("terminal");
	terminal_write(devconf,0,data);
}

static READ8_HANDLER(microdec_terminal_status_r)
{
	if (received_char!=0) return 3; // char received
	return 1; // ready
}

static READ8_HANDLER(microdec_terminal_r)
{
	UINT8 retVal = received_char;
	received_char = 0;
	return retVal;
}

static ADDRESS_MAP_START(microdec_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x0fff ) AM_ROM
	AM_RANGE( 0x1000, 0xffff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( microdec_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0xfc, 0xfc) AM_READWRITE(microdec_terminal_r,microdec_terminal_w)
	AM_RANGE(0xfd, 0xfd) AM_READ(microdec_terminal_status_r)
	AM_RANGE(0xfa, 0xfa) AM_DEVREAD("upd765", upd765_status_r)
	AM_RANGE(0xfb, 0xfb) AM_DEVREADWRITE("upd765", upd765_data_r, upd765_data_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( microdec )
	PORT_INCLUDE(generic_terminal)
INPUT_PORTS_END


static MACHINE_RESET(microdec)
{
	received_char = 0;
}

static WRITE8_DEVICE_HANDLER( microdec_kbd_put )
{
	received_char = data;
}

static GENERIC_TERMINAL_INTERFACE( microdec_terminal_intf )
{
	DEVCB_HANDLER(microdec_kbd_put)
};

static WRITE_LINE_DEVICE_HANDLER( microdec_irq_w )
{
}

static const struct upd765_interface microdec_upd765_interface =
{
	DEVCB_LINE(microdec_irq_w), /* interrupt */
	NULL,						/* DMA request */
	NULL,	/* image lookup */
	UPD765_RDY_PIN_CONNECTED,	/* ready pin */
	{FLOPPY_0,FLOPPY_1, NULL, NULL}
};

static const floppy_config microdec_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(default),
	DO_NOT_KEEP_GEOMETRY,
	NULL
};

static MACHINE_DRIVER_START( microdec )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(microdec_mem)
    MDRV_CPU_IO_MAP(microdec_io)

    MDRV_MACHINE_RESET(microdec)

    /* video hardware */
    MDRV_IMPORT_FROM( generic_terminal )
	MDRV_GENERIC_TERMINAL_ADD(TERMINAL_TAG,microdec_terminal_intf)

	MDRV_UPD765A_ADD("upd765", microdec_upd765_interface)
	MDRV_FLOPPY_2_DRIVES_ADD(microdec_floppy_config)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( md2 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS( 0, "v13", "v1.3" )
	ROMX_LOAD( "md2-13.bin",  0x0000, 0x0800, CRC(43f4c9ab) SHA1(48a35cbee4f341310e9cba5178c3fd6e74ef9748), ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "v13a", "v1.3a" )
	ROMX_LOAD( "md2-13a.bin", 0x0000, 0x0800, CRC(d7fcddfd) SHA1(cae29232b737ebb36a27b8ad17bc69e9968f1309), ROM_BIOS(2))
	ROM_SYSTEM_BIOS( 2, "v13b", "v1.3b" )
	ROMX_LOAD( "md2-13b.bin", 0x0000, 0x1000, CRC(a8b96835) SHA1(c6b111939aa7e725da507da1915604656540b24e), ROM_BIOS(3))
	ROM_SYSTEM_BIOS( 3, "v20", "v2.0" )
	ROMX_LOAD( "md2-20.bin",  0x0000, 0x1000, CRC(a604735c) SHA1(db6e6e82a803f5cbf4f628f5778a93ae3e211fe1), ROM_BIOS(4))
	ROM_SYSTEM_BIOS( 4, "v23", "v2.3" )
	ROMX_LOAD( "md2-23.bin",  0x0000, 0x1000, CRC(49bae273) SHA1(00381a226fe250aa3636b0b740df0af63efb0d18), ROM_BIOS(5))
ROM_END

ROM_START( md3 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS( 0, "v23a", "v2.3a" )
	ROMX_LOAD( "md3-23a.bin", 0x0000, 0x1000, CRC(95d59980) SHA1(ae65a8e8e2823cf4cf6b1d74c0996248e290e9f1), ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "v25", "v2.5" )
	ROMX_LOAD( "md3-25.bin",  0x0000, 0x1000, CRC(14f86bc5) SHA1(82fe022c85f678744bb0340ca3f88b18901fdfcb), ROM_BIOS(2))
	ROM_SYSTEM_BIOS( 2, "v31", "v3.1" )
	ROMX_LOAD( "md3-31.bin",  0x0000, 0x1000, CRC(bd4014f6) SHA1(5b33220af34c64676756177db4915f97840b2996), ROM_BIOS(3))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1982, md2,	0,       0, 	microdec,	microdec,	 0,   "Morrow Designs",   "Micro Decision MD-2",		GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 1982, md3,	md2,       0,	microdec,	microdec,	 0,   "Morrow Designs",   "Micro Decision MD-3",		GAME_NOT_WORKING | GAME_NO_SOUND)

