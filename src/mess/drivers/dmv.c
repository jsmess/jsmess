/***************************************************************************
   
        NCR Decision Mate V 

        04/01/2012 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"

class dmv_state : public driver_device
{
public:
	dmv_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }
};

static ADDRESS_MAP_START(dmv_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x1fff ) AM_ROM
	AM_RANGE( 0x2000, 0xffff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( dmv_io , AS_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( dmv )
INPUT_PORTS_END


static MACHINE_RESET(dmv) 
{	
}

static VIDEO_START( dmv )
{
}

static SCREEN_UPDATE( dmv )
{
    return 0;
}

/* F4 Character Displayer */
static const gfx_layout dmv_charlayout =
{
	8, 16,					/* 8 x 16 characters */
	128,					/* 128 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	/* y offsets */
	{ STEP16(0,8) },
	8*16					/* every char takes 16 bytes */
};

static GFXDECODE_START( dmv )
	GFXDECODE_ENTRY("maincpu", 0x1000, dmv_charlayout, 0, 1)
GFXDECODE_END

static MACHINE_CONFIG_START( dmv, dmv_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MCFG_CPU_PROGRAM_MAP(dmv_mem)
    MCFG_CPU_IO_MAP(dmv_io)	

    MCFG_MACHINE_RESET(dmv)
	
    /* video hardware */
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(640, 480)
    MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MCFG_SCREEN_UPDATE(dmv)

	MCFG_GFXDECODE(dmv)
    MCFG_PALETTE_LENGTH(2)
    MCFG_PALETTE_INIT(black_and_white)

    MCFG_VIDEO_START(dmv)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( dmv )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "dmv_norm.bin", 0x0000, 0x2000, CRC(bf25f3f0) SHA1(0c7dd37704db4799e340cc836f887cd543e5c964))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       		FLAGS */
COMP( 1984, dmv,  	0,       0, 		dmv, 	dmv, 	 0,  	 "NCR",   "Decision Mate V",	GAME_NOT_WORKING | GAME_NO_SOUND)

