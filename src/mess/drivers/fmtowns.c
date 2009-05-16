/*

	Fujitsu FM-Towns
	
	Japanese computer system released in 1989.
	
	CPU:  AMD 80386SX (80387 available as add-on)
	Sound:  Yamaha YM2612 (or YM3438?)
	        Ricoh RF5c68
	Video:  Custom(?)
	        320x200 - 640x480
	        16 - 32768 colours from a possible palette of between 4096 and
	          16.7m colours (depending on video mode)
	        1024 sprites (16x16)
	        
	        
	Fujitsu FM-Towns Marty
	
	Japanese console, based on the FM-Towns computer, released in 1993 


	16/5/09:  Skeleton driver.

*/

#include "driver.h"
#include "cpu/i386/i386.h"
#include "sound/2612intf.h"
#include "sound/rf5c68.h"

static ADDRESS_MAP_START(marty_mem, ADDRESS_SPACE_PROGRAM, 32)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

static ADDRESS_MAP_START( marty_io , ADDRESS_SPACE_IO, 32)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( marty )
INPUT_PORTS_END


static MACHINE_RESET(marty)
{
}

static VIDEO_START( marty )
{
}

static VIDEO_UPDATE( marty )
{
    return 0;
}

static MACHINE_DRIVER_START( marty )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",I386, 16000000)
    MDRV_CPU_PROGRAM_MAP(marty_mem)
    MDRV_CPU_IO_MAP(marty_io)

    MDRV_MACHINE_RESET(marty)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(60)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    
    /* sound hardware */
    MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("fm", YM2612, 4000000) // actual clock speed unknown
//	MDRV_SOUND_CONFIG(ym2612_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SOUND_ADD("pcm", RF5C68, 4000000)  // actual clock speed unknown
//	MDRV_SOUND_CONFIG(rf5c68_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
    

    MDRV_VIDEO_START(marty)
    MDRV_VIDEO_UPDATE(marty)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( fmtmarty )
  ROM_REGION( 0x400000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD("mrom.m36",  0x000000, 0x200000, CRC(9c0c060c) SHA1(5721c5f9657c570638352fa9acac57fa8d0b94bd) )
	ROM_LOAD("mrom.m37",  0x200000, 0x200000, CRC(fb66bb56) SHA1(e273b5fa618373bdf7536495cd53c8aac1cce9a5) )

ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT  MACHINE     INPUT    INIT    CONFIG COMPANY      FULLNAME            FLAGS */
CONS( 1993, fmtmarty, 0,    0, 		marty, 		marty, 	 0,  	 0,  	"Fujitsu",   "FM-Towns Marty",	 GAME_NOT_WORKING)

