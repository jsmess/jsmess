/***************************************************************************

        Dream Multimedia Dreambox 7000/5620/500

        20/03/2010 Skeleton driver.


    DM7000 -    CPU STB04500 at 252 MHz
                RAM 64MB
                Flash 8MB
                1 x DVB-S
                1 x IDE interface
                1 x Common Interface (CI)
                1 x Compact flash
                2 x Smart card reader
                1 x USB
                1 x RS232
                1 x LAN 100Mbit/s
                1 x LCD display

    DM56x0 -    CPU STB04500 at 252 MHz
                RAM 64MB
                Flash 8MB
                1 x DVB-S
                2 x Common Interface (CI)
                1 x Smart card reader
                1 x LAN 100Mbit/s (just on 5620)
                1 x LCD display

    DM500 -     CPU STBx25xx at 252 MHz
                RAM 96MB
                Flash 32MB
                1 x DVB-S
                1 x Smart card reader
                1 x LAN 100Mbit/s

****************************************************************************/

#include "emu.h"
#include "cpu/powerpc/ppc.h"


class dm7000_state : public driver_device
{
public:
	dm7000_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};


/*
 Memory map for the IBM "Redwood-4" STB03xxx evaluation board.

 The  STB03xxx internal i/o addresses don't work for us 1:1,
 so we need to map them at a well know virtual address.

 4000 000x   uart1           -> 0xe000 000x
 4001 00xx   ppu
 4002 00xx   smart card
 4003 000x   iic
 4004 000x   uart0
 4005 0xxx   timer
 4006 00xx   gpio
 4007 00xx   smart card
 400b 000x   iic
 400c 000x   scp
 400d 000x   modem
*/
static ADDRESS_MAP_START( dm7000_mem, AS_PROGRAM, 32 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000000, 0x01ffffff) AM_RAM	// RAM page 0 - 32MB
	AM_RANGE(0x20000000, 0x21ffffff) AM_RAM // RAM page 1 - 32MB
	AM_RANGE(0xfffe0000, 0xffffffff) AM_ROM AM_REGION("user1",0)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( dm7000 )
INPUT_PORTS_END


static MACHINE_RESET(dm7000)
{
}

static VIDEO_START( dm7000 )
{
}

static SCREEN_UPDATE( dm7000 )
{
    return 0;
}

static MACHINE_CONFIG_START( dm7000, dm7000_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",PPC403GA, 252000000) // Should be PPC405
    MCFG_CPU_PROGRAM_MAP(dm7000_mem)

    MCFG_MACHINE_RESET(dm7000)

    /* video hardware */
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(640, 480)
    MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MCFG_SCREEN_UPDATE(dm7000)

    MCFG_PALETTE_LENGTH(2)
    MCFG_PALETTE_INIT(black_and_white)

    MCFG_VIDEO_START(dm7000)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( dm7000 )
    ROM_REGION( 0x20000, "user1", ROMREGION_32BIT | ROMREGION_BE  )
	ROMX_LOAD( "dm7000.bin", 0x0000, 0x20000, CRC(8a410f67) SHA1(9d6c9e4f5b05b28453d3558e69a207f05c766f54), ROM_GROUPWORD )
ROM_END

ROM_START( dm5620 )
    ROM_REGION( 0x20000, "user1", ROMREGION_32BIT | ROMREGION_BE  )
	ROMX_LOAD( "dm5620.bin", 0x0000, 0x20000, CRC(ccddb822) SHA1(3ecf553ced0671599438368f59d8d30df4d13ade), ROM_GROUPWORD )
ROM_END

ROM_START( dm500 )
    ROM_REGION( 0x20000, "user1", ROMREGION_32BIT | ROMREGION_BE )
	ROM_SYSTEM_BIOS( 0, "alps", "Alps" )
    ROMX_LOAD( "dm500-alps-boot.bin",   0x0000, 0x20000, CRC(daf2da34) SHA1(68f3734b4589fcb3e73372e258040bc8b83fd739), ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "phil", "Philips" )
    ROMX_LOAD( "dm500-philps-boot.bin", 0x0000, 0x20000, CRC(af3477c7) SHA1(9ac918f6984e6927f55bea68d6daaf008787136e), ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(2))
ROM_END
/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
SYST( 2003, dm7000,  0,       0,	dm7000, 	dm7000, 	 0,   "Dream Multimedia",   "Dreambox 7000",		GAME_NOT_WORKING | GAME_NO_SOUND)
SYST( 2004, dm5620,  dm7000,  0,	dm7000, 	dm7000, 	 0,   "Dream Multimedia",   "Dreambox 5620",		GAME_NOT_WORKING | GAME_NO_SOUND)
SYST( 2006, dm500,   dm7000,  0,	dm7000, 	dm7000, 	 0,   "Dream Multimedia",   "Dreambox 500",			GAME_NOT_WORKING | GAME_NO_SOUND)

