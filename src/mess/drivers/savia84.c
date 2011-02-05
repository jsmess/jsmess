/***************************************************************************
   
        Savia 84

		More data at : 
				http://www.nostalcomp.cz/pdfka/savia84.pdf

        05/02/2011 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/i8255a.h"

class savia84_state : public driver_device
{
public:
	savia84_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};

static ADDRESS_MAP_START(savia84_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x03ff ) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( savia84_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0x03)
	AM_RANGE(0x00, 0x03) AM_DEVREADWRITE("ppi8255", i8255a_r, i8255a_w)
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( savia84 )
INPUT_PORTS_END


static MACHINE_RESET(savia84) 
{	
}

static VIDEO_START( savia84 )
{
}

static VIDEO_UPDATE( savia84 )
{
    return 0;
}

static WRITE8_DEVICE_HANDLER (savia84_8255_porta_w )
{
}

static WRITE8_DEVICE_HANDLER (savia84_8255_portb_w )
{
}

static WRITE8_DEVICE_HANDLER (savia84_8255_portc_w )
{
}

static READ8_DEVICE_HANDLER (savia84_8255_portc_r )
{
	return 0xff;
}


I8255A_INTERFACE( savia84_ppi8255_interface )
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(savia84_8255_portc_r),
	DEVCB_HANDLER(savia84_8255_porta_w),
	DEVCB_HANDLER(savia84_8255_portb_w),
	DEVCB_HANDLER(savia84_8255_portc_w)
};

static MACHINE_CONFIG_START( savia84, savia84_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",Z80, XTAL_4MHz / 2)
    MCFG_CPU_PROGRAM_MAP(savia84_mem)
    MCFG_CPU_IO_MAP(savia84_io)	

    MCFG_MACHINE_RESET(savia84)
	
    /* video hardware */
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(640, 480)
    MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MCFG_PALETTE_LENGTH(2)
    MCFG_PALETTE_INIT(black_and_white)

    MCFG_VIDEO_START(savia84)
    MCFG_VIDEO_UPDATE(savia84)
	
	MCFG_I8255A_ADD( "ppi8255", savia84_ppi8255_interface )
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( savia84 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD("savia84_1kb.bin", 0x0000, 0x0400, CRC(23a5c15e) SHA1(7e769ed8960d8c591a25cfe4effffcca3077c94b)) // 2758 ROM - 1KB
	ROM_COPY("maincpu", 0x0000, 0x2000, 0x0400)
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1984, savia84,  0,       0, 	savia84, 	savia84, 	 0,   "<unknown>",   "Savia 84",		GAME_NOT_WORKING | GAME_NO_SOUND)

