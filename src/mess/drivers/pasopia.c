/***************************************************************************

        Toshiba PASOPIA / PASOPIA7 emulation

        Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"

// sketchy port 0x3c implementation to see what the CPU does...
// however, it writes 0x11 - in theory setting BASIC+BIOS in the lower banks
// and then it writes at 0x0000... maybe bank1 should be RAM? or 
// should we have writes to RAM and only reads to BIOS/BASIC?
static WRITE8_HANDLER( paso7_bankswitch )
{
	UINT8 *cpu = memory_region(space->machine, "maincpu");
	UINT8 *basic = memory_region(space->machine, "basic");

	if (BIT(data, 0))
	{
		memory_unmap_write(space, 0x0000, 0x7fff, 0, 0);
		memory_set_bankptr(space->machine, "bank1", basic);
		memory_set_bankptr(space->machine, "bank2", cpu + 0x10000);
	}
	else if (BIT(data, 1))
	{
		memory_install_write_bank(space, 0x0000, 0x3fff, 0, 0, "bank1");
		memory_install_write_bank(space, 0x4000, 0x7fff, 0, 0, "bank2");
		memory_set_bankptr(space->machine, "bank1", cpu);
		memory_set_bankptr(space->machine, "bank2", cpu + 0x4000);
	}
	else 
	{
		memory_unmap_write(space, 0x0000, 0x7fff, 0, 0);
		memory_set_bankptr(space->machine, "bank1", cpu + 0x10000);
		memory_set_bankptr(space->machine, "bank2", cpu + 0x10000);
	}

	if (BIT(data, 2))
		memory_unmap_readwrite(space, 0x8000, 0xbfff, 0, 0);
	else
	{
		memory_install_write_bank(space, 0x8000, 0xbfff, 0, 0, "bank3");
		memory_set_bankptr(space->machine, "bank3", cpu + 0x8000);
	}
	// bit 2 also selects vram

	// bit 3? PIO2 port C

	// bank4 is always RAM
}

static ADDRESS_MAP_START(paso7_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x3fff ) AM_RAMBANK("bank1")
	AM_RANGE( 0x4000, 0x7fff ) AM_RAMBANK("bank2")
	AM_RANGE( 0x8000, 0xbfff ) AM_RAMBANK("bank3")
	AM_RANGE( 0xc000, 0xffff ) AM_RAMBANK("bank4")
ADDRESS_MAP_END

static ADDRESS_MAP_START( paso7_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE( 0x3c, 0x3c ) AM_WRITE(paso7_bankswitch)
#if 0
	AM_RANGE( 0x08, 0x0b )  // PIO0
	AM_RANGE( 0x0c, 0x0f )  // PIO1
	AM_RANGE( 0x10, 0x11 )  // CRTC
	AM_RANGE( 0x20, 0x23 )  // PIO2
	AM_RANGE( 0x28, 0x2b )  // CTC
	AM_RANGE( 0x3a, 0x3a )  // PSG0
	AM_RANGE( 0x3b, 0x3b )  // PSG1
	AM_RANGE( 0xe0, 0xe6 )  // FLOPPY
#endif	
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( paso7 )
INPUT_PORTS_END

static MACHINE_RESET( paso7 )
{
}

static VIDEO_START( paso7 )
{
}

static VIDEO_UPDATE( paso7 )
{
    return 0;
}

static MACHINE_DRIVER_START( paso7 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(paso7_mem)
    MDRV_CPU_IO_MAP(paso7_io)

    MDRV_MACHINE_RESET(paso7)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(paso7)
    MDRV_VIDEO_UPDATE(paso7)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( pasopia7 )
	ROM_REGION( 0x14000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "bios.rom", 0x10000, 0x4000, CRC(b8111407) SHA1(ac93ae62db4c67de815f45de98c79cfa1313857d))

	ROM_REGION( 0x8000, "basic", ROMREGION_ERASEFF )
	ROM_LOAD( "basic.rom", 0x0000, 0x8000, CRC(8a58fab6) SHA1(5e1a91dfb293bca5cf145b0a0c63217f04003ed1))

	ROM_REGION( 0x800, "font", ROMREGION_ERASEFF )
	ROM_LOAD( "font.rom", 0x0000, 0x0800, CRC(a91c45a9) SHA1(a472adf791b9bac3dfa6437662e1a9e94a88b412))

	ROM_REGION( 0x20000, "kanji", ROMREGION_ERASEFF )
	ROM_LOAD( "kanji.rom", 0x0000, 0x20000, CRC(6109e308) SHA1(5c21cf1f241ef1fa0b41009ea41e81771729785f))
ROM_END

static DRIVER_INIT( paso7 )
{
	UINT8 *bios = memory_region(machine, "maincpu");
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	memory_unmap_write(space, 0x0000, 0xbfff, 0, 0);
	memory_set_bankptr(machine, "bank1", bios + 0x10000);
	memory_set_bankptr(machine, "bank2", bios + 0x10000);
	memory_set_bankptr(machine, "bank3", bios + 0x10000);
//	memory_set_bankptr(machine, "bank4", bios + 0x10000);
}

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 19??, pasopia7,  0,       0, 	 paso7,	paso7,   paso7,   "Toshiba",   "PASOPIA 7", GAME_NOT_WORKING | GAME_NO_SOUND )
