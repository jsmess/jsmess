/***************************************************************************

        Hitachi Basic Master Jr (MB-6885)

        13/07/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/m6800/m6800.h"

static UINT8 *wram;

static VIDEO_START( bmjr )
{
}

static VIDEO_UPDATE( bmjr )
{
	int x,y,xi,yi,count;
	static UINT8 *gfx_rom = memory_region(screen->machine, "char");

	count = 0x0100;

	for(y=0;y<24;y++)
	{
		for(x=0;x<32;x++)
		{
			int tile = wram[count];
			int color = 4;

			for(yi=0;yi<8;yi++)
			{
				for(xi=0;xi<8;xi++)
				{
					int pen;

					pen = (gfx_rom[tile*8+yi] >> (7-xi) & 1) ? color : 0;

					*BITMAP_ADDR16(bitmap, y*8+yi, x*8+xi) = screen->machine->pens[pen];
				}
			}

			count++;
		}

	}

    return 0;
}

static ADDRESS_MAP_START(bmjr_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	//0x0100, 0x03ff basic vram
	//0x0900, 0x20ff vram, modes 0x40 / 0xc0
	//0x2100, 0x38ff vram, modes 0x44 / 0xcc
	AM_RANGE(0x0000, 0xafff) AM_RAM AM_BASE(&wram)
	AM_RANGE(0xb000, 0xe7ff) AM_ROM
//	AM_RANGE(0xe890, 0xe890) W MP-1710 tile color
//	AM_RANGE(0xe891, 0xe891) W MP-1710 background color
//	AM_RANGE(0xe892, 0xe892) W MP-1710 monochrome / color setting
//	AM_RANGE(0xee00, 0xee00) R stop tape
//	AM_RANGE(0xee20, 0xee20) R start tape
//	AM_RANGE(0xee40, 0xee40) W Picture reverse (?)
//	AM_RANGE(0xee80, 0xee80) RW tape input / output
//	AM_RANGE(0xeec0, 0xeec0) RW keyboard
//	AM_RANGE(0xef00, 0xef00) R timer
//	AM_RANGE(0xef40, 0xef40) R unknown
//	AM_RANGE(0xef80, 0xef80) R unknown
//	AM_RANGE(0xefe0, 0xefe0) W screen mode
	AM_RANGE(0xf000, 0xffff) AM_ROM
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( bmjr )
INPUT_PORTS_END


static MACHINE_RESET(bmjr)
{
}

static const gfx_layout bmjr_charlayout =
{
	8, 8,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static GFXDECODE_START( bmjr )
	GFXDECODE_ENTRY( "char", 0x0000, bmjr_charlayout, 0, 8 )
GFXDECODE_END

static PALETTE_INIT( bmjr )
{
	int i;

	for(i=0;i<8;i++)
		palette_set_color_rgb(machine, i, pal1bit(i >> 1),pal1bit(i >> 2),pal1bit(i >> 0));
}

static MACHINE_DRIVER_START( bmjr )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",M6800, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(bmjr_mem)

    MDRV_MACHINE_RESET(bmjr)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(256, 192)
    MDRV_SCREEN_VISIBLE_AREA(0, 256-1, 0, 192-1)
    MDRV_PALETTE_LENGTH(8)
    MDRV_PALETTE_INIT(bmjr)

    MDRV_GFXDECODE(bmjr)

    MDRV_VIDEO_START(bmjr)
    MDRV_VIDEO_UPDATE(bmjr)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( bmjr )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "bas.rom", 0xb000, 0x3000, BAD_DUMP CRC(2318e04e) SHA1(cdb3535663090f5bcaba20b1dbf1f34724ef6a5f)) //12k ROMs doesn't exist ...
	ROM_LOAD( "mon.rom", 0xf000, 0x1000, CRC(776cfa3a) SHA1(be747bc40fdca66b040e0f792b05fcd43a1565ce))
	ROM_LOAD( "prt.rom", 0xe000, 0x0800, CRC(b9aea867) SHA1(b8dd5348790d76961b6bdef41cfea371fdbcd93d))

	ROM_REGION( 0x800, "char", 0 )
	ROM_LOAD( "font.rom", 0x0000, 0x0800, BAD_DUMP CRC(258c6fd7) SHA1(d7c7dd57d6fc3b3d44f14c32182717a48e24587f)) //taken from a JP emulator
ROM_END

/* Driver */
static DRIVER_INIT( bmjr )
{

}

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1982, bmjr,  	0,       0, 	bmjr, 		bmjr, 	 bmjr,  	 "Hitachi",   "Basic Master Jr",	GAME_NOT_WORKING | GAME_NO_SOUND)

