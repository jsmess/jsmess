/***************************************************************************

        Hector 2HR+

        12/05/2009 Skeleton driver - Micko
	31/06/2009 Video - Robbbert

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static READ8_HANDLER( hector_videoram_r )
{
	return videoram[offset];
}

static WRITE8_HANDLER( hector_videoram_w )
{
	videoram[offset] = data;
}

static ADDRESS_MAP_START(hec2hrp_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000,0x3fff) AM_ROM
	AM_RANGE(0x4000,0xffff) AM_RAM
	AM_RANGE(0xc000,0xf7ff) AM_READWRITE(hector_videoram_r,hector_videoram_w) AM_BASE(&videoram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( hec2hrp_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( hec2hrp )
INPUT_PORTS_END


static MACHINE_RESET(hec2hrp)
{
}

static VIDEO_START( hec2hrp )
{
}

static VIDEO_UPDATE( hec2hrp )
{
	UINT8 gfx,y;
	UINT16 sy=0,ma=0,x;

	for (y = 0; y < 224; y++)
	{
		UINT16  *p = BITMAP_ADDR16(bitmap, sy++, 0);

		for (x = ma; x < ma + 64; x++)
		{
			gfx = videoram[x];

			/* Display a scanline of a character (8 pixels) */
			*p = ( gfx & 0x01 ) ? 1 : 0; p++;
			*p = ( gfx & 0x02 ) ? 1 : 0; p++;
			*p = ( gfx & 0x04 ) ? 1 : 0; p++;
			*p = ( gfx & 0x08 ) ? 1 : 0; p++;
			*p = ( gfx & 0x10 ) ? 1 : 0; p++;
			*p = ( gfx & 0x20 ) ? 1 : 0; p++;
			*p = ( gfx & 0x40 ) ? 1 : 0; p++;
			*p = ( gfx & 0x80 ) ? 1 : 0; p++;
		}
		ma+=64;
	}
	return 0;
}

static MACHINE_DRIVER_START( hec2hrp )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
	MDRV_CPU_PROGRAM_MAP(hec2hrp_mem)
	MDRV_CPU_IO_MAP(hec2hrp_io)

	MDRV_MACHINE_RESET(hec2hrp)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(512, 224)
	MDRV_SCREEN_VISIBLE_AREA(0, 511, 0, 223)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(hec2hrp)
	MDRV_VIDEO_UPDATE(hec2hrp)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( hec2hrp )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "hector2hrp.rom", 0x0000, 0x4000, CRC(983f52e4) SHA1(71695941d689827356042ee52ffe55ce7e6b8ecd))
ROM_END

ROM_START( hec2hrx )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "hector2hrx.rom", 0x0000, 0x4000, CRC(f047c521) SHA1(744336b2acc76acd7c245b562bdc96dca155b066))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP(1983, hec2hrp,  0,       0,     hec2hrp, 	hec2hrp, 0,  	  0,  	 "Micronique",   "Hector 2HR+",	GAME_NOT_WORKING)
COMP(????, hec2hrx,  hec2hrp, 0,     hec2hrp, 	hec2hrp, 0,  	  0,  	 "Micronique",   "Hector 2HRX",	GAME_NOT_WORKING)

