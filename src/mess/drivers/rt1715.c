/***************************************************************************

        Robotron 1715 video driver by Miodrag Milanovic

        10/06/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "cpu/z80/z80.h"
#include "cpu/i8085/i8085.h"
#include "machine/i8255a.h"
#include "machine/8257dma.h"
#include "video/i8275.h"
#include "devices/cassette.h"
#include "formats/rk_cas.h"
#include "includes/rt1715.h"
#include "devices/messram.h"

static WRITE8_HANDLER (rt1717_set_bank )
{
	memory_set_bankptr(space->machine, 1, messram_get_ptr(devtag_get_device(space->machine, "messram")));
	memory_set_bankptr(space->machine, 3, messram_get_ptr(devtag_get_device(space->machine, "messram")));
}

/* Address maps */
static ADDRESS_MAP_START(rt1715_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x07ff ) AM_READWRITE(SMH_BANK(1), SMH_BANK(3))
  AM_RANGE( 0x0800, 0xffff ) AM_READWRITE(SMH_BANK(2), SMH_BANK(2))
ADDRESS_MAP_END

static ADDRESS_MAP_START( rt1715_io , ADDRESS_SPACE_IO, 8)
	//ADDRESS_MAP_GLOBAL_MASK(0xFF)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x28, 0x28 ) AM_WRITE( rt1717_set_bank )
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( rt1715 )
INPUT_PORTS_END

/* Machine driver */
static MACHINE_DRIVER_START( rt1715 )
  /* basic machine hardware */
  MDRV_CPU_ADD("maincpu", Z80, XTAL_16MHz / 4)
  MDRV_CPU_PROGRAM_MAP(rt1715_mem)
  MDRV_CPU_IO_MAP(rt1715_io)
  MDRV_MACHINE_RESET( rt1715 )

    /* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(78*6, 30*10)
	MDRV_SCREEN_VISIBLE_AREA(0, 78*6-1, 0, 30*10-1)
	MDRV_PALETTE_LENGTH(3)
	MDRV_PALETTE_INIT(rt1715)

	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(rt1715)
	
	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("64K")
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( rt1715 )
	ROM_REGION( 0x10800, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "s502.bin", 0x10000, 0x0800, CRC(7b6302e1) SHA1(e8f61763ff8841078a1939aa5e85a17f2af42163))
	ROM_REGION(0x0800, "gfx",0)
	ROM_LOAD ("s619.bin", 0x0000, 0x0800, CRC(98647763) SHA1(93fba51ed26392ec3eff1037886576fa12443fe5))
ROM_END

ROM_START( rt1715w )
	ROM_REGION( 0x10800, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "s550.bin", 0x10000, 0x0800, CRC(0a96c754) SHA1(4d9ad5b877353d91ba355044d2847e1d621e2b01))
	ROM_REGION(0x0800, "gfx",0)
	ROM_LOAD ("s619.bin", 0x0000, 0x0800, CRC(98647763) SHA1(93fba51ed26392ec3eff1037886576fa12443fe5))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT   INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( 1986, rt1715, 0,      0, 		rt1715, 	rt1715,rt1715, 0,  "Robotron", 	"Robotron 1715",	GAME_NOT_WORKING)
COMP( 1986, rt1715w,rt1715, 0, 		rt1715, 	rt1715,rt1715, 0,  "Robotron", 	"Robotron 1715W",	GAME_NOT_WORKING)
