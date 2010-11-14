/***************************************************************************

    Univac Terminals

    The terminals are models UTS20, UTS30, UTS40, UTS50 and SVT1120,
    however only the UTS20 is dumped (program roms only).

    25/05/2009 Skeleton driver [Robbbert].

    The terminal has 2 screens selectable by the operator with the Fn + 1-2
    buttons. Thus the user can have two sessions open at once, to different
    mainframes or applications.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"

static const UINT8 *FNT;
static UINT8 uts20_screen;

static WRITE8_HANDLER( uts20_43_w )
{
	uts20_screen = data & 1;
}

static READ8_HANDLER( uts20_vram_r )
{
	UINT8 *RAM = memory_region(space->machine, "maincpu");
	return RAM[offset | ((uts20_screen) ? 0xe000 : 0xc000)];
}

static WRITE8_HANDLER( uts20_vram_w )
{
	UINT8 *RAM = memory_region(space->machine, "maincpu");
	RAM[offset | ((uts20_screen) ? 0xe000 : 0xc000)] = data;
}


static ADDRESS_MAP_START(uts20_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x4fff ) AM_ROM
	AM_RANGE( 0x5000, 0x7fff ) AM_RAM AM_REGION("maincpu", 0x5000)
	AM_RANGE( 0x8000, 0x9fff ) AM_READWRITE(uts20_vram_r,uts20_vram_w)
	AM_RANGE( 0xa000, 0xffff ) AM_RAM AM_REGION("maincpu", 0xa000)
ADDRESS_MAP_END

static ADDRESS_MAP_START( uts20_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x43, 0x43 ) AM_WRITE(uts20_43_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( uts20 )
INPUT_PORTS_END


static MACHINE_RESET(uts20)
{
	uts20_screen = 0;
}

static VIDEO_START( uts20 )
{
	FNT = memory_region(machine, "chargen");
}

static VIDEO_UPDATE( uts20 )
{
	static UINT8 framecnt=0;
	UINT8 y,ra,chr,gfx;
	UINT16 sy=0,ma=0,x;
	UINT8 *videoram = memory_region(screen->machine, "maincpu")+((uts20_screen) ? 0xe000 : 0xc000);

	framecnt++;

	for (y = 0; y < 25; y++)
	{
		for (ra = 0; ra < 10; ra++)
		{
			UINT16  *p = BITMAP_ADDR16(bitmap, sy++, 0);

			for (x = ma; x < ma + 80; x++)
			{
				gfx = 0;

				if (ra < 9)
				{
					chr = videoram[x];

					/* Take care of flashing characters */
					if ((chr & 0x80) && (framecnt & 0x08))
						chr = 0x20;

					chr &= 0x7f;

					gfx = FNT[(chr<<4) | ra ];
				}

				/* Display a scanline of a character */
				*p = ( gfx & 0x80 ) ? 1 : 0; p++;
				*p = ( gfx & 0x40 ) ? 1 : 0; p++;
				*p = ( gfx & 0x20 ) ? 1 : 0; p++;
				*p = ( gfx & 0x10 ) ? 1 : 0; p++;
				*p = ( gfx & 0x08 ) ? 1 : 0; p++;
				*p = ( gfx & 0x04 ) ? 1 : 0; p++;
				*p = ( gfx & 0x02 ) ? 1 : 0; p++;
				*p = ( gfx & 0x01 ) ? 1 : 0; p++;
			}
		}
		ma+=80;
	}
	return 0;
}

static MACHINE_CONFIG_START( uts20, driver_device )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(uts20_mem)
    MDRV_CPU_IO_MAP(uts20_io)

    MDRV_MACHINE_RESET(uts20)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 250)
    MDRV_SCREEN_VISIBLE_AREA(0, 639, 0, 249)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(uts20)
    MDRV_VIDEO_UPDATE(uts20)
MACHINE_CONFIG_END


/* ROM definition */
ROM_START( uts20 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "uts20a.rom", 0x0000, 0x1000, CRC(1a7b4b4e) SHA1(c3732e25b4b7c7a80172e3fe55c77b923cf511eb) )
	ROM_LOAD( "uts20b.rom", 0x1000, 0x1000, CRC(7f8de87b) SHA1(a85f404ad9d560df831cc3e651a4b45e4ed30130) )
	ROM_LOAD( "uts20c.rom", 0x2000, 0x1000, CRC(4e334705) SHA1(ff1a730551b42f29d20af8ecc4495fd30567d35b) )
	ROM_LOAD( "uts20d.rom", 0x3000, 0x1000, CRC(76757cf7) SHA1(b0509d9a35366b21955f83ec3685163844c4dbf1) )
	ROM_LOAD( "uts20e.rom", 0x4000, 0x1000, CRC(0dfc8062) SHA1(cd681020bfb4829d4cebaf1b5bf618e67b55bda3) )

	/* character generator not dumped, using the one from 'c10' for now */
	ROM_REGION( 0x2000, "chargen", 0 )
	ROM_LOAD( "c10_char.bin", 0x0000, 0x2000, NO_DUMP CRC(cb530b6f) SHA1(95590bbb433db9c4317f535723b29516b9b9fcbf))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 198?, uts20,  0,       0, 	uts20,	uts20,	 0,		 "Sperry Univac",   "UTS-20", GAME_NOT_WORKING | GAME_NO_SOUND)
