/***************************************************************************

    Processor Technology Corp. SOL-20

    07/2009 Skeleton driver.

    Info from: http://www.sol20.org/

    Note that the SOLOS dump comes from the Solace emu. Not being sure if
    it has been verified with a real SOL-20 (even if I think it has been), I
    marked it as a BAD_DUMP

    Other OS ROMs to be dumped:
      - CONSOL
      - CUTER
      - BOOTLOAD
      - DPMON (3rd party)

    Hardware variants (ever shipped?):
      - SOL-10, i.e. a stripped down version without expansion backplane and
        numeric keypad
      - SOL-PC, i.e. the single board version, basically it consisted of the
        SOL-20 board only

****************************************************************************/

#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "devices/messram.h"

static ADDRESS_MAP_START( sol20_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0X0000, 0Xbfff) AM_RAM	// optional s100 ram
	AM_RANGE(0xc000, 0xc7ff) AM_ROM
	AM_RANGE(0Xc800, 0Xcbff) AM_RAM	// system ram
	AM_RANGE(0Xcc00, 0Xcfff) AM_RAM	// video ram
	AM_RANGE(0Xd000, 0Xffff) AM_RAM	// optional s100 ram
ADDRESS_MAP_END

static ADDRESS_MAP_START( sol20_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
/*	AM_RANGE(0xf8, 0xf8) serial status in (bit 6=data av, bit 7=tmbe)
	AM_RANGE(0xf9, 0xf9) serial data in, out
	AM_RANGE(0xfa, 0xfa) general status in (bit 0=keyb data av, bit 1=parin data av, bit 2=parout ready)
	AM_RANGE(0xfb, 0xfb) tape
	AM_RANGE(0xfc, 0xfc) keyboard data in
	AM_RANGE(0xfd, 0xfd) parallel data in, out
	AM_RANGE(0xfe, 0xfe) scroll register
	AM_RANGE(0xff, 0xff) sense switches */
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( sol20 )
INPUT_PORTS_END


static MACHINE_RESET( sol20 )
{
}

static VIDEO_START( sol20 )
{
}

static VIDEO_UPDATE( sol20 )
{
/* visible screen is 64 x 16, with start position controlled by scroll register */
	return 0;
}

static MACHINE_DRIVER_START( sol20 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu",8080, XTAL_14_31818MHz/7)
	MDRV_CPU_PROGRAM_MAP(sol20_mem)
	MDRV_CPU_IO_MAP(sol20_io)

	MDRV_MACHINE_RESET(sol20)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(sol20)
	MDRV_VIDEO_UPDATE(sol20)

	/* internal ram */
//	MDRV_RAM_ADD("messram")
//	MDRV_RAM_DEFAULT_SIZE("8K")
//	MDRV_RAM_EXTRA_OPTIONS("16K,32K")
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( sol20 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "solos.rom", 0xc000, 0x0800, BAD_DUMP CRC(4d0af383) SHA1(ac4510c3380ed4a31ccf4f538af3cb66b76701ef) )	// from solace emu
	/* Character generator rom is missing */
ROM_END

/* Driver */
/*    YEAR  NAME    PARENT  COMPAT  MACHINE  INPUT  INIT   COMPANY     FULLNAME   FLAGS */
COMP( 1976, sol20,  0,      0,      sol20,   sol20, 0,     "Processor Technology Corporation",  "SOL-20", GAME_NOT_WORKING)
