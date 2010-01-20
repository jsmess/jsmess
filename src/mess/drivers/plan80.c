/***************************************************************************

        Plan-80

        06/12/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/i8085/i8085.h"

static UINT8* plan80_video_ram;
static ADDRESS_MAP_START(plan80_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xefff) AM_RAM
	AM_RANGE(0xf000, 0xf7ff) AM_RAM AM_BASE(&plan80_video_ram)
	AM_RANGE(0xf800, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( plan80_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( plan80 )
INPUT_PORTS_END


static MACHINE_RESET(plan80)
{
}

static VIDEO_START( plan80 )
{
}

static VIDEO_UPDATE( plan80 )
{
	UINT8 *gfx = memory_region(screen->machine, "gfx");
 	int x,y,j,b;
	UINT16 addr;

	for(y = 0; y < 32; y++ )
	{
		addr = y*64;
		for(x = 0; x < 48; x++ )
		{
			UINT8 code = plan80_video_ram[addr + x];
			for(j = 0; j < 8; j++ )
			{
				UINT8 val = gfx[code*8 + j];
				if (BIT(code,7))  val ^= 0xff;
				for(b = 0; b < 6; b++ )
				{
					*BITMAP_ADDR16(bitmap, y*8+j, x*6+b ) = (val >> (5-b)) & 1;
				}
			}
		}
	}
	return 0;
}

/* F4 Character Displayer */
static const gfx_layout plan80_charlayout =
{
	8, 8,					/* 8 x 8 characters */
	256,					/* 256 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8					/* every char takes 8 bytes */
};

static GFXDECODE_START( plan80 )
	GFXDECODE_ENTRY( "gfx", 0x0000, plan80_charlayout, 0, 1 )
GFXDECODE_END


static MACHINE_DRIVER_START( plan80 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",8080, 2048000)
    MDRV_CPU_PROGRAM_MAP(plan80_mem)
    MDRV_CPU_IO_MAP(plan80_io)

    MDRV_MACHINE_RESET(plan80)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(48*6, 32*8)
    MDRV_SCREEN_VISIBLE_AREA(0, 48*6-1, 0, 32*8-1)
	MDRV_GFXDECODE(plan80)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(plan80)
    MDRV_VIDEO_UPDATE(plan80)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( plan80 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "pl80mod.bin", 0x0000, 0x0800, CRC(6bdd7136) SHA1(721eab193c33c9330e0817616d3d2b601285fe50))
	ROM_LOAD( "pl80mon.bin", 0xf800, 0x0800, CRC(433fb685) SHA1(43d53c35544d3a197ab71b6089328d104535cfa5))
	ROM_REGION( 0x0800, "gfx", ROMREGION_ERASEFF )
	ROM_LOAD( "pl80gzn.bin", 0x0000, 0x0800, CRC(b4ddbdb6) SHA1(31bf9cf0f2ed53f48dda29ea830f74cea7b9b9b2))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY          FULLNAME       FLAGS */
COMP( 1988, plan80,  0,       0, 	plan80, 	plan80, 	 0,   "Tesla Eltos",   "Plan-80",		GAME_NOT_WORKING)

