/***************************************************************************

        Xerox 820

        12/05/2009 Skeleton driver.

	The incoming key is expected to be placed at FF30 (might be interrupt driven)

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static const UINT8 *FNT;

static READ8_HANDLER( xerox820_videoram_r )
{
	return videoram[offset];
}

static WRITE8_HANDLER( xerox820_videoram_w )
{
	videoram[offset] = data;
}

static ADDRESS_MAP_START(xerox820_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x0fff ) AM_ROM
	AM_RANGE( 0x1000, 0x2fff ) AM_RAM
	AM_RANGE( 0x3000, 0x3bff ) AM_READWRITE(xerox820_videoram_r,xerox820_videoram_w) AM_SIZE(&videoram_size) AM_BASE(&videoram)
	AM_RANGE( 0x3c00, 0xffff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( xerox820_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
/* IN 1C, OUT 7,C,14,18,1A,1B,1C,1D,1F */
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( xerox820 )
INPUT_PORTS_END


static MACHINE_RESET(xerox820)
{
}

static VIDEO_START( xerox820 )
{
	FNT = memory_region(machine, "gfx1");
}

static VIDEO_UPDATE( xerox820 )
{
	static UINT8 framecnt=0;
	UINT8 y,ra,chr,gfx;
	UINT16 sy=0,ma=0,x;

	framecnt++;

	for (y = 0; y < 24; y++)
	{
		for (ra = 0; ra < 10; ra++)
		{
			UINT16  *p = BITMAP_ADDR16(bitmap, sy++, 0);

			for (x = ma; x < ma + 80; x++)
			{
				if (ra < 8)
				{
					chr = videoram[x]^0x80;

					/* Take care of flashing characters */
					if ((chr < 0x80) && (framecnt & 0x08))
						chr |= 0x80;

					/* get pattern of pixels for that character scanline */
					gfx = FNT[(chr<<3) | ra ];
				}
				else
					gfx = 0xff;

				/* Display a scanline of a character (7 pixels) */
				*p = 0; p++;
				*p = ( gfx & 0x10 ) ? 0 : 1; p++;
				*p = ( gfx & 0x08 ) ? 0 : 1; p++;
				*p = ( gfx & 0x04 ) ? 0 : 1; p++;
				*p = ( gfx & 0x02 ) ? 0 : 1; p++;
				*p = ( gfx & 0x01 ) ? 0 : 1; p++;
				*p = 0; p++;
			}
		}
		ma+=128;
	}
	return 0;
}

static MACHINE_DRIVER_START( xerox820 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(xerox820_mem)
    MDRV_CPU_IO_MAP(xerox820_io)

    MDRV_MACHINE_RESET(xerox820)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(80*7, 24*10)
	MDRV_SCREEN_VISIBLE_AREA(0,80*7-1,0,24*10-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(xerox820)
    MDRV_VIDEO_UPDATE(xerox820)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(xerox820)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( xerox820 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "x820_u64.bin", 0x0000, 0x0800, CRC(2fc227e2) SHA1(b4ea0ae23d281a687956e8a514cb364a1372678e))
	ROM_LOAD( "x820_u63.bin", 0x0800, 0x0800, CRC(bc11f834) SHA1(4fd2b209a6e6ff9b0c41800eb5228c34a0d7f7ef))

	ROM_REGION( 0x800, "gfx1", 0 )
	ROM_LOAD( "x820_u92.bin", 0x0000, 0x0800, CRC(b823fa98) SHA1(ad0ea346aa257a53ad5701f4201896a2b3a0f928))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, xerox820,  0,       0, 	xerox820, 	xerox820, 	 0,  	  xerox820,  	 "Xerox",   "Xerox 820",		GAME_NOT_WORKING)

