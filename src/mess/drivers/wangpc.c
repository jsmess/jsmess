#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/i86/i86.h"


//**************************************************************************
//  DRIVER STATE
//**************************************************************************

class wangpc_state : public driver_device
{
public:
	// constructor
	wangpc_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
	{
	}

	UINT32 screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
	{
		bitmap.fill(RGB_BLACK);
		return 0;
	}
};




//**************************************************************************
//  ADDRESS MAPS
//**************************************************************************

//-------------------------------------------------
//  ADDRESS_MAP( wangpc_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( wangpc_mem, AS_PROGRAM, 16, wangpc_state )
	AM_RANGE(0xfc000, 0xfffff) AM_ROM AM_REGION("maincpu", 0)
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( wangpc_io )
//-------------------------------------------------

static ADDRESS_MAP_START( wangpc_io, AS_IO, 16, wangpc_state )
ADDRESS_MAP_END



//**************************************************************************
//  INPUT PORTS
//**************************************************************************

//-------------------------------------------------
//  INPUT_PORTS( wangpc )
//-------------------------------------------------

static INPUT_PORTS_START( wangpc )
INPUT_PORTS_END



//**************************************************************************
//  MACHINE DRIVERS
//**************************************************************************

static MACHINE_CONFIG_START( wangpc, wangpc_state )
	MCFG_CPU_ADD("maincpu", I8086, 4000000)
	MCFG_CPU_PROGRAM_MAP(wangpc_mem)
	MCFG_CPU_IO_MAP(wangpc_io)

	// video hardware
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_UPDATE_DRIVER(wangpc_state, screen_update)
	MCFG_SCREEN_SIZE(640,480)
	MCFG_SCREEN_VISIBLE_AREA(0,639, 0,479)
	MCFG_SCREEN_REFRESH_RATE(30)
MACHINE_CONFIG_END



//**************************************************************************
//  ROM DEFINITIONS
//**************************************************************************
	
ROM_START( wangpc )
	ROM_REGION16_LE( 0x4000, "maincpu", 0)
	ROM_LOAD16_BYTE( "motherboard-l94.bin", 0x0001, 0x2000, CRC(f9f41304) SHA1(1815295809ef11573d724ede47446f9ac7aee713) )
	ROM_LOAD16_BYTE( "motherboard-l115.bin", 0x0000, 0x2000, CRC(67b37684) SHA1(70d9f68eb88cc2bc9f53f949cc77411c09a4266e) )

	ROM_REGION( 0x1000, "remotecomms", 0 )
	ROM_LOAD( "remotecomms-l28.bin", 0x0000, 0x1000, CRC(c05a1bee) SHA1(6b3f0d787d014b1fd3925812c905ddb63c5055f1) )

	ROM_REGION( 0x1000, "network", 0 )
	ROM_LOAD( "network-l22.bin", 0x0000, 0x1000, CRC(487e5f04) SHA1(81e52e70e0c6e34715119b121ec19a7758cd6772) )

	ROM_REGION( 0x1000, "keyboard", 0 )
	ROM_LOAD( "keyboard-z5.bin", 0x0000, 0x1000, CRC(82d2999f) SHA1(2bb34a1de2d94b2885d9e8fcd4964296f6276c5c) )

	ROM_REGION( 0x1000, "hdc1", 0 )
	ROM_LOAD( "hdc-l54.bin", 0x0000, 0x1000, CRC(94e9a17d) SHA1(060c576d70069ece2d0dbce86ffc448df2b169e7) )

	ROM_REGION( 0x1000, "hdc2", 0 )
	ROM_LOAD( "hdc-l19.bin", 0x0000, 0x1000, CRC(282770d2) SHA1(a0e3bad5041e0dfd6087907015b07a093b576bc0) )
ROM_END



//**************************************************************************
//  GAME DRIVERS
//**************************************************************************

COMP( 1985, wangpc, 0, 0, wangpc, wangpc, 0, "Wang Laboratories", "Wang Professional Computer", GAME_NOT_WORKING | GAME_NO_SOUND )
