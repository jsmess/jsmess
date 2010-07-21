/***************************************************************************
   
        NEC TK-80BS

        21/07/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(tk80bs_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x7bff) AM_ROM
	// AM_RANGE(0x7c00, 0x7dff) memory mapped i/o
	AM_RANGE(0x7e00, 0x7fff) AM_RAM // video ram
	AM_RANGE(0x8000, 0xcfff) AM_RAM // RAM
	AM_RANGE(0xd000, 0xefff) AM_ROM // BASIC
	AM_RANGE(0xf000, 0xffff) AM_ROM // BSMON
ADDRESS_MAP_END


/* Input ports */
static INPUT_PORTS_START( tk80bs )
INPUT_PORTS_END


static MACHINE_RESET(tk80bs) 
{	
}

static VIDEO_START( tk80bs )
{
}

static VIDEO_UPDATE( tk80bs )
{
    return 0;
}

static MACHINE_DRIVER_START( tk80bs )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(tk80bs_mem)

    MDRV_MACHINE_RESET(tk80bs)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(tk80bs)
    MDRV_VIDEO_UPDATE(tk80bs)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( tk80bs )
    ROM_REGION( 0x10000, "maincpu", 0 )	
	ROM_SYSTEM_BIOS(0, "psedo", "Pseudo LEVEL 1")
	ROMX_LOAD( "tk80.dummy", 0x0000, 0x0800, CRC(553b25ca) SHA1(939350d7fa56ce567ddf393c9f4b9db6ebc18a2c), ROM_BIOS(1))
	ROMX_LOAD( "ext.l1",     0x0c00, 0x6e46, CRC(d05ed3ff) SHA1(8544aa2cb58df9edf221f5be2cdafa248dd33828), ROM_BIOS(1))
	ROMX_LOAD( "lv1basic.l1",0xe000, 0x09a2, CRC(3ff67a71) SHA1(528c9331740637e853c099e1739ecdea6dd200bc), ROM_BIOS(1))
	ROMX_LOAD( "bsmon.l1",   0xf000, 0x0db0, CRC(5daa599b) SHA1(7e6ec5bfb3eea114f7ee9ef589a89246b8533b2f), ROM_BIOS(1))
	
	ROM_SYSTEM_BIOS(1, "psedo10", "Pseudo LEVEL 2 1.0")	
	ROMX_LOAD( "tk80.dummy", 0x0000, 0x0800, CRC(553b25ca) SHA1(939350d7fa56ce567ddf393c9f4b9db6ebc18a2c), ROM_BIOS(2))
	ROMX_LOAD( "ext.10",     0x0c00, 0x3dc2, CRC(3c64d488) SHA1(919180d5b34b981ab3dd8b2885d3c0933203f355), ROM_BIOS(2))
	ROMX_LOAD( "lv2basic.10",0xd000, 0x2000, CRC(594fe70e) SHA1(5854c1be5fa78c1bfee365379495f14bc23e15e7), ROM_BIOS(2))
	ROMX_LOAD( "bsmon.10",   0xf000, 0x0daf, CRC(d0047983) SHA1(79e2b5dc47b574b55375cbafffff144744093ec1), ROM_BIOS(2))
	
	ROM_SYSTEM_BIOS(2, "psedo11", "Pseudo LEVEL 2 1.1")		
	ROMX_LOAD( "tk80.dummy", 0x0000, 0x0800, CRC(553b25ca) SHA1(939350d7fa56ce567ddf393c9f4b9db6ebc18a2c), ROM_BIOS(3))
	ROMX_LOAD( "ext.11",     0x0c00, 0x3dd4, CRC(bd5c5169) SHA1(2ad70828348372328b76bac0fa93d3f6f17ade34), ROM_BIOS(3))	
	ROMX_LOAD( "lv2basic.11",0xd000, 0x2000, CRC(3df9a3bd) SHA1(9539409c876bce27d630fe47d07a4316d2ce09cb), ROM_BIOS(3))		
	ROMX_LOAD( "bsmon.11",   0xf000, 0x0ff6, CRC(fca7a609) SHA1(7c7eb5e5e4cf1e0021383bdfc192b88262aba6f5), ROM_BIOS(3))
	
	ROM_REGION( 0x1000, "gfx", ROMREGION_ERASEFF )
	ROM_LOAD( "font.rom", 0x0000, 0x1000, CRC(94d95199) SHA1(9fe741eab866b0c520d4108bccc6277172fa190c))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1980, tk80bs,  	0,       0, 		tk80bs, 	tk80bs, 	 0,  	   "NEC",   "TK-80BS",		GAME_NOT_WORKING | GAME_NO_SOUND)

