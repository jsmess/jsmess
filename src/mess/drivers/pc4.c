/*

	Laser pc4

	http://www.8bit-micro.com/laser.htm
	http://www.euskalnet.net/ingepal/images/Vtech_Laser_PC4_1988.jpg

*/

#include "emu.h"
#include "cpu/z80/z80.h"

class pc4_state : public driver_device
{
public:
	pc4_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};

static ADDRESS_MAP_START(pc4_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START(pc4_io, ADDRESS_SPACE_IO, 8)
ADDRESS_MAP_END

static INPUT_PORTS_START( pc4 )
INPUT_PORTS_END

static PALETTE_INIT( pc4 )
{
	palette_set_color(machine, 0, MAKE_RGB(138, 146, 148));
	palette_set_color(machine, 1, MAKE_RGB(92, 83, 88));
}

static VIDEO_UPDATE( pc4 )
{
	return 0;
}

static MACHINE_START(pc4)
{
}

static MACHINE_CONFIG_START( pc4, pc4_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu", Z80, XTAL_4MHz)
    MCFG_CPU_PROGRAM_MAP(pc4_mem)
    MCFG_CPU_IO_MAP(pc4_io)
	
	MCFG_MACHINE_START(pc4)

    /* video hardware */
	MCFG_SCREEN_ADD("screen", LCD)
	MCFG_SCREEN_REFRESH_RATE(72)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(312, 32)
	MCFG_SCREEN_VISIBLE_AREA(0, 311, 0, 31)

	MCFG_DEFAULT_LAYOUT(layout_lcd)

	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(pc4)

	MCFG_VIDEO_UPDATE(pc4)
MACHINE_CONFIG_END

ROM_START( pc4 )
    ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "laser pc4 v4.14a.u2", 0x00000, 0x20000, CRC(f8dabf5d) SHA1(6988517b3ccb42df2b8d6e1517ff04b24458d146) )
ROM_END

/*    YEAR  NAME        PARENT      COMPAT  MACHINE     INPUT   INIT      COMPANY                     FULLNAME                FLAGS */
COMP( 1988, pc4,       0,          0,      pc4,  pc4, 0,      "Laser Computer",   "Laser PC4",						GAME_NOT_WORKING | GAME_NO_SOUND )
