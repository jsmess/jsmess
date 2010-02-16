/***************************************************************************

        PC/M by Miodrag Milanovic

        12/05/2009 Preliminary driver.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"

static UINT8 *pcm_video_ram;

static ADDRESS_MAP_START(pcm_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
    AM_RANGE( 0x0000, 0x1fff ) AM_ROM  // ROM
    AM_RANGE( 0x2000, 0xf7ff ) AM_RAM  // RAM
    AM_RANGE( 0xf800, 0xffff ) AM_RAM AM_BASE(&pcm_video_ram) // Video RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( pcm_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_EXTERN( k7659 );

static MACHINE_RESET(pcm)
{
}

static VIDEO_START( pcm )
{
}

static VIDEO_UPDATE( pcm )
{
	UINT8 code;
	UINT8 line;
	int y, x, j, b;

	UINT8 *gfx = memory_region(screen->machine, "gfx1");

	for (y = 0; y < 32; y++)
	{
		for (x = 0; x < 64; x++)
		{
			code = pcm_video_ram[(0x400 + y*64 + x) & 0x7ff];
			for(j = 0; j < 8; j++ )
			{
				line = gfx[code*8 + j];
				for (b = 0; b < 8; b++)
				{
					*BITMAP_ADDR16(bitmap, y*8+j, x * 8 + (7 - b)) =  ((line >> b) & 0x01);
				}
			}
		}
	}
	return 0;
}


static MACHINE_DRIVER_START( pcm )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_10MHz /4)
    MDRV_CPU_PROGRAM_MAP(pcm_mem)
    MDRV_CPU_IO_MAP(pcm_io)

    MDRV_MACHINE_RESET(pcm)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(64*8, 32*8)
    MDRV_SCREEN_VISIBLE_AREA(0, 64*8-1, 0, 32*8-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(pcm)
    MDRV_VIDEO_UPDATE(pcm)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( pcm )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
    ROM_SYSTEM_BIOS( 0, "v202", "Version 2.02" )
	ROMX_LOAD( "bios_v202.d14", 0x0000, 0x0800, CRC(27c24892) SHA1(a97bf9ef075de91330dc0c7cfd3bb6c7a88bb585), ROM_BIOS(1))
	ROMX_LOAD( "bios_v202.d15", 0x0800, 0x0800, CRC(e9cedc70) SHA1(913c526283d9289d0cb2157985bb48193df7aa16), ROM_BIOS(1))
	ROMX_LOAD( "bios_v202.d16", 0x1000, 0x0800, CRC(abe12001) SHA1(d8f0db6b141736d7715d588384fa49ab386bcc55), ROM_BIOS(1))
	ROMX_LOAD( "bios_v202.d17", 0x1800, 0x0800, CRC(2d48d1cc) SHA1(36a825140124dbe10d267fdf28b3eacec6f6d556), ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "v210", "Version 2.10" )
	ROMX_LOAD( "bios_v210.d14", 0x0000, 0x0800, CRC(45923112) SHA1(dde922533ebd0f6ac06d25b9786830ee3c7178b9), ROM_BIOS(2))
	ROMX_LOAD( "bios_v210.d15", 0x0800, 0x0800, CRC(e9cedc70) SHA1(913c526283d9289d0cb2157985bb48193df7aa16), ROM_BIOS(2))
	ROMX_LOAD( "bios_v210.d16", 0x1000, 0x0800, CRC(ee9ed77b) SHA1(12ea18e3e280f2a0657ff11c7bcdd658d325235c), ROM_BIOS(2))
	ROMX_LOAD( "bios_v210.d17", 0x1800, 0x0800, CRC(2d48d1cc) SHA1(36a825140124dbe10d267fdf28b3eacec6f6d556), ROM_BIOS(2))
	ROM_SYSTEM_BIOS( 2, "v330", "Version 3.30" )
	ROMX_LOAD( "bios_v330.d14", 0x0000, 0x0800, CRC(9bbfee10) SHA1(895002f2f4c711278f1e2d0e2a987e2d31472e4f), ROM_BIOS(3))
	ROMX_LOAD( "bios_v330.d15", 0x0800, 0x0800, CRC(4f8d5b40) SHA1(440b0be4cf45a5d450f9eb7684ceb809450585dc), ROM_BIOS(3))
	ROMX_LOAD( "bios_v330.d16", 0x1000, 0x0800, CRC(93fd0d91) SHA1(c8f1bbb63eca3c93560622581ecbb588716aeb91), ROM_BIOS(3))
	ROMX_LOAD( "bios_v330.d17", 0x1800, 0x0800, CRC(d8c7ce33) SHA1(9030d9a73ef1c12a31ac2cb9a593fb2a5097f24d), ROM_BIOS(3))
	ROM_REGION(0x0800, "gfx1",0)
	ROM_LOAD( "charrom.d113", 0x0000, 0x0800, CRC(5684b3c3) SHA1(418054aa70a0fd120611e32059eb2051d3b82b5a))
	ROM_REGION(0x0800, "k7659",0)
	ROM_LOAD ("k7659n.d8", 0x0000, 0x0800, CRC(7454bf0a) SHA1(b97e7df93778fa371b96b6f4fb1a5b1c8b89d7ba) )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1988, pcm,	 0,       0,	 pcm,		k7659,	 0, 	 "Mugler/Mathes",   "PC/M",		GAME_NOT_WORKING | GAME_NO_SOUND)

