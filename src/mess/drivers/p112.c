/***************************************************************************

        P112 Single Board Computer

        30/08/2010 Skeleton driver

****************************************************************************/

#include "emu.h"
#include "cpu/z180/z180.h"


class p112_state : public driver_device
{
public:
	p112_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};


static ADDRESS_MAP_START(p112_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000, 0x07fff) AM_ROM
	AM_RANGE(0x08000, 0xfffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( p112_io , AS_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( p112 )
INPUT_PORTS_END


static MACHINE_RESET(p112)
{
}

static VIDEO_START( p112 )
{
}

static SCREEN_UPDATE( p112 )
{
    return 0;
}

static MACHINE_CONFIG_START( p112, p112_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",Z180, XTAL_16MHz)
    MCFG_CPU_PROGRAM_MAP(p112_mem)
    MCFG_CPU_IO_MAP(p112_io)

    MCFG_MACHINE_RESET(p112)

    /* video hardware */
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(240, 320)
    MCFG_SCREEN_VISIBLE_AREA(0, 240-1, 0, 320-1)
    MCFG_SCREEN_UPDATE(p112)

    MCFG_PALETTE_LENGTH(2)
    MCFG_PALETTE_INIT(black_and_white)

    MCFG_VIDEO_START(p112)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( p112 )
    ROM_REGION( 0x20000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS( 0, "960513", "ver 13-05-1996" )
	ROMX_LOAD( "960513.bin",  0x00000, 0x8000, CRC(077c1c40) SHA1(c1e6b4b0de50bba90f0d4667f9344815bb578b9b), ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "970308", "ver 08-03-1997" )
	ROMX_LOAD( "970308.bin",  0x00000, 0x8000, CRC(15e61f0d) SHA1(66ba1af7da0450b69650086ab20230390ba23e17), ROM_BIOS(2))
	ROM_SYSTEM_BIOS( 2, "4b2", "ver 4b2" )
	ROMX_LOAD( "romv4b1.bin", 0x00000, 0x8000, CRC(15d79beb) SHA1(f971f75a717e3f6d59b257eb3af369d4d2e0f301), ROM_BIOS(3))
	ROM_SYSTEM_BIOS( 3, "4b2", "ver 4b2" )
	ROMX_LOAD( "romv4b2.bin", 0x00000, 0x8000, CRC(9b9a8a5e) SHA1(c40151ee654008b9f803d6a05d692a5a3619a726), ROM_BIOS(4))
	ROM_SYSTEM_BIOS( 4, "4b3", "ver 4b3" )
	ROMX_LOAD( "romv4b3.bin", 0x00000, 0x8000, CRC(734031ea) SHA1(2e5e5ac6bd17d15cab24a36bc3da42ca49cbde92), ROM_BIOS(5))
	ROM_SYSTEM_BIOS( 5, "4b4", "ver 4b4" )
	ROMX_LOAD( "romv4b4.bin", 0x00000, 0x8000, CRC(cd419c40) SHA1(6002130d874387c9ec23b4363fe9f0ca78208878), ROM_BIOS(6))
	ROM_SYSTEM_BIOS( 6, "5", "ver 5" )
	ROMX_LOAD( "051103.bin",  0x00000, 0x8000, CRC(6c47ec13) SHA1(24f5bf1524425186fe10e1d29d05f6efbd3366d9), ROM_BIOS(7))
	ROM_SYSTEM_BIOS( 7, "5b1", "ver 5b1" )
	ROMX_LOAD( "romv5b1.bin", 0x00000, 0x8000, CRC(047296f7) SHA1(380f8e4237525636c605b7e37d989ace8437beb4), ROM_BIOS(8))
ROM_END


/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY    FULLNAME       FLAGS */
COMP( 1996, p112,  0,       0,	p112,	p112,  0,	 "Dave Brooks",   "P112",		GAME_NOT_WORKING | GAME_NO_SOUND)
