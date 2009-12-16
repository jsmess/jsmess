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

#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "devices/messram.h"

static ADDRESS_MAP_START( sol20_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0xc000, 0xc7ff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sol20_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
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
	return 0;
}

static MACHINE_DRIVER_START( sol20 )
	/* basic machine hardware */
    MDRV_CPU_ADD("maincpu",8080, 1780000)
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
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("8K")
	MDRV_RAM_EXTRA_OPTIONS("16K,32K")
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( sol20 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "solos.rom", 0xc000, 0x0800, BAD_DUMP CRC(4d0af383) SHA1(ac4510c3380ed4a31ccf4f538af3cb66b76701ef) )	// from solace emu
ROM_END

/* Driver */
/*    YEAR  NAME    PARENT  COMPAT  MACHINE  INPUT  INIT   CONFIG COMPANY     FULLNAME   FLAGS */
COMP( 1976, sol20,  0,      0,      sol20,   sol20, 0,     0, "Processor Technology Corporation",  "SOL-20", GAME_NOT_WORKING)
