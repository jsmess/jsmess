/***************************************************************************

        Heathkit H19

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "video/mc6845.h"

static ADDRESS_MAP_START(h19_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( h19_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( h19 )
INPUT_PORTS_END


static MACHINE_RESET(h19)
{
}

VIDEO_START( h19 )
{
}

VIDEO_UPDATE( h19 )
{
	const device_config *mc6845 = devtag_get_device(screen->machine, "crtc");
	mc6845_update(mc6845, bitmap, cliprect);
	return 0;
}

static const mc6845_interface h19_crtc6845_interface =
{
	"screen",
	8 /*?*/,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static MACHINE_DRIVER_START( h19 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_12_288MHz / 6) // From schematics
    MDRV_CPU_PROGRAM_MAP(h19_mem)
    MDRV_CPU_IO_MAP(h19_io)

    MDRV_MACHINE_RESET(h19)

    /* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 200)
	MDRV_SCREEN_VISIBLE_AREA(0, 640 - 1, 0, 200 - 1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_MC6845_ADD("crtc", MC6845, XTAL_12_288MHz / 8, h19_crtc6845_interface) // clk taken from schematics

	MDRV_VIDEO_START( h19 )
	MDRV_VIDEO_UPDATE( h19 )
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(h19)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( h19 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
    // Super H19 ROM (
    ROM_LOAD( "2732_super19_h447.bin", 0x0000, 0x1000, CRC(68FBFF54) SHA1(c0aa7199900709d717b07e43305dfdf36824da9b))
    // Watzman ROM
    ROM_LOAD( "watzman.bin", 0x0000, 0x1000, CRC(8168b6dc) SHA1(bfaebb9d766edbe545d24bc2b6630be4f3aa0ce9))
	ROM_REGION( 0x0800, "gfx1", 0 )
	// Original font dump
  	ROM_LOAD( "2716_444-29_h19font.bin", 0x0000, 0x0800, CRC(d595ac1d) SHA1(130fb4ea8754106340c318592eec2d8a0deaf3d0))
  	ROM_REGION( 0x0800, "keyboard", 0 )
  	// Original dump
  	ROM_LOAD( "2716_444-37_h19keyb.bin", 0x0000, 0x0800, CRC(5c3e6972) SHA1(df49ce64ae48652346a91648c58178a34fb37d3c))
  	// Watzman keyboard
  	ROM_LOAD( "keybd.bin", 0x0000, 0x0800, CRC(58dc8217) SHA1(1b23705290bdf9fc6342065c6a528c04bff67b13))

ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, h19,  0,       0, 	h19, 	h19, 	 0,  	  h19,  	 "Heath, Inc.",   "Heathkit H19",		GAME_NOT_WORKING)

