/***************************************************************************
   
        Sanyo MBC-200
		
		Machince MBC-1200 is identicaly but sold outside of Japan

        31/10/2011 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"


class mbc200_state : public driver_device
{
public:
	mbc200_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

};

static ADDRESS_MAP_START(mbc200_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x2fff ) AM_ROM
	AM_RANGE( 0x3000, 0xffff ) AM_RAM 
ADDRESS_MAP_END

static ADDRESS_MAP_START( mbc200_io , AS_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
ADDRESS_MAP_END
/*
static ADDRESS_MAP_START(mbc200_sub_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x1fff ) AM_ROM
	AM_RANGE( 0x2000, 0xffff ) AM_RAM 
ADDRESS_MAP_END

static ADDRESS_MAP_START( mbc200_sub_io , AS_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END
*/
/* Input ports */
INPUT_PORTS_START( mbc200 )
INPUT_PORTS_END


static MACHINE_RESET(mbc200) 
{	
}

static VIDEO_START( mbc200 )
{
}

static SCREEN_UPDATE( mbc200 )
{
    return 0;
}

static MACHINE_CONFIG_START( mbc200, mbc200_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MCFG_CPU_PROGRAM_MAP(mbc200_mem)
    MCFG_CPU_IO_MAP(mbc200_io)	

    /*MCFG_CPU_ADD("subcpu",Z80, XTAL_4MHz)
    MCFG_CPU_PROGRAM_MAP(mbc200_sub_mem)
    MCFG_CPU_IO_MAP(mbc200_sub_io)	
*/
    MCFG_MACHINE_RESET(mbc200)
	
    /* video hardware */
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(640, 400)
    MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 400-1)
    MCFG_SCREEN_UPDATE(mbc200)

    MCFG_PALETTE_LENGTH(2)
    MCFG_PALETTE_INIT(black_and_white)

    MCFG_VIDEO_START(mbc200)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( mbc200 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "m5l2764.bin", 0x0000, 0x2000, CRC(377300a2) SHA1(8563172f9e7f84330378a8d179f4138be5fda099))
	ROM_LOAD( "d2732a.bin",  0x2000, 0x1000, CRC(bf364ce8) SHA1(baa3a20a5b01745a390ef16628dc18f8d682d63b))
	ROM_REGION( 0x10000, "subcpu", ROMREGION_ERASEFF )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1982, mbc200,  0,       0, 	mbc200, 	mbc200, 	 0,	 "Sanyo",   "MBC-200",		GAME_NOT_WORKING | GAME_NO_SOUND)

