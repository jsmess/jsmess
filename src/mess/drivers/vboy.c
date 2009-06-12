/***************************************************************************
   
        Nintendo Virtual Boy

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/v810/v810.h"
#include "devices/cartslot.h"

static ADDRESS_MAP_START( vboy_mem, ADDRESS_SPACE_PROGRAM, 32 )
	ADDRESS_MAP_GLOBAL_MASK(0x07FFFFFF)
	AM_RANGE( 0x01000000, 0x010005FF ) AM_RAM // Sound RAM 
	AM_RANGE( 0x05000000, 0x0500FFFF ) AM_RAM // Main RAM - 64K
	AM_RANGE( 0x06000000, 0x06003FFF ) AM_RAM // Cart RAM - 8K
	AM_RANGE( 0x07000000, 0x071FFFFF ) AM_MIRROR(0x8E00000) AM_ROMBANK(1)	/* ROM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( vboy_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( vboy )
INPUT_PORTS_END


static MACHINE_RESET(vboy) 
{	
	memory_set_bankptr( machine, 1, memory_region(machine, "user1") );
}

static VIDEO_START( vboy )
{
}

static VIDEO_UPDATE( vboy )
{
    return 0;
}

static MACHINE_DRIVER_START( vboy )
    /* basic machine hardware */
    MDRV_CPU_ADD( "maincpu", V810, 21477270 )
    MDRV_CPU_PROGRAM_MAP(vboy_mem)
    MDRV_CPU_IO_MAP(vboy_io)	

    MDRV_MACHINE_RESET(vboy)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(384, 224)
    MDRV_SCREEN_VISIBLE_AREA(0, 384-1, 0, 224-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(vboy)
    MDRV_VIDEO_UPDATE(vboy)
	
    /* cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("vb")
	MDRV_CARTSLOT_MANDATORY
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(vboy)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( vboy )
    ROM_REGION( 0x200000, "user1", 0 )
    ROM_CART_LOAD("cart", 0x0000, 0x200000, ROM_MIRROR)
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   	FULLNAME       FLAGS */
CONS( 1995, vboy,   0,      0, 		 vboy, 		vboy, 	 0,  	 vboy, 	"Nintendo", "Virtual Boy", GAME_NOT_WORKING)

