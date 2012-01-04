/***************************************************************************
   
        DEC Rainbow 100

        04/01/2012 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/i86/i86.h"
#include "cpu/z80/z80.h"

class rainbow_state : public driver_device
{
public:
	rainbow_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }
};


static ADDRESS_MAP_START( rainbow8088_map, AS_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000, 0x1ffff) AM_RAM
	AM_RANGE(0xec000, 0xeffff) AM_RAM 
	AM_RANGE(0xf0000, 0xfffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( rainbow8088_io , AS_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

static ADDRESS_MAP_START(rainbowz80_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0xffff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( rainbowz80_io , AS_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( rainbow )
INPUT_PORTS_END


static MACHINE_RESET(rainbow) 
{	
}

static VIDEO_START( rainbow )
{
}

static SCREEN_UPDATE( rainbow )
{
    return 0;
}

static MACHINE_CONFIG_START( rainbow, rainbow_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",I8088, 4772720)
    MCFG_CPU_PROGRAM_MAP(rainbow8088_map)
    MCFG_CPU_IO_MAP(rainbow8088_io)	

    MCFG_CPU_ADD("subcpu",Z80, XTAL_4MHz)
    MCFG_CPU_PROGRAM_MAP(rainbowz80_mem)
    MCFG_CPU_IO_MAP(rainbowz80_io)
	MCFG_DEVICE_DISABLE()	

    MCFG_MACHINE_RESET(rainbow)
	
    /* video hardware */
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(640, 480)
    MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MCFG_SCREEN_UPDATE(rainbow)
    MCFG_PALETTE_LENGTH(2)
    MCFG_PALETTE_INIT(black_and_white)

    MCFG_VIDEO_START(rainbow)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( rainbow )
    ROM_REGION(0x100000,"maincpu", 0)
	ROM_LOAD( "rblow.16k",  0xf0000, 0x4000, CRC(9d1332b4) SHA1(736306d2a36bd44f95a39b36ebbab211cc8fea6e))
	ROM_RELOAD(0xf4000,0x4000)	
	ROM_LOAD( "rbhigh.16k", 0xf8000, 0x4000, CRC(8638712f) SHA1(8269b0d95dc6efbe67d500dac3999df4838625d8))
	ROM_RELOAD(0xfc000,0x4000)	
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( ????, rainbow,  0,       0, 	rainbow, 	rainbow, 	 0,  "Digital Equipment Corporation",   "Rainbow 100B",		GAME_NOT_WORKING | GAME_NO_SOUND)

