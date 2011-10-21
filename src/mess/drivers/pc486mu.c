/***************************************************************************************************

    PC-486MU (c) 1994 Epson

****************************************************************************************************/

#define ADDRESS_MAP_MODERN
#include "emu.h"
#include "cpu/i386/i386.h"

class pc486mu_state : public driver_device
{
public:
	pc486mu_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
	{ }
};

static ADDRESS_MAP_START(pc486mu_map, AS_PROGRAM, 32, pc486mu_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000000, 0x0009ffff) AM_RAM //work RAM	
	AM_RANGE(0x000e8000, 0x000fffff) AM_ROM AM_REGION("ipl", 0)
	AM_RANGE(0xfffe8000, 0xffffffff) AM_ROM AM_REGION("ipl", 0)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pc486mu_io , AS_IO, 32, pc486mu_state)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( pc486mu )
INPUT_PORTS_END

static MACHINE_CONFIG_START( pc486mu, pc486mu_state )
	MCFG_CPU_ADD("maincpu", I486, 16000000)
	MCFG_CPU_PROGRAM_MAP(pc486mu_map)
	MCFG_CPU_IO_MAP(pc486mu_io)

	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(640, 480)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 200-1)
    MCFG_PALETTE_LENGTH(2)
    MCFG_PALETTE_INIT(black_and_white)
MACHINE_CONFIG_END


ROM_START( pc486mu )
	ROM_REGION( 0x18000, "ipl", ROMREGION_ERASEFF )
	ROM_LOAD( "bios.rom", 0x0000, 0x18000, CRC(57b5d701) SHA1(15029800842e93e07615b0fd91fb9f2bfe3e3c24))

	ROM_REGION( 0x4000, "sound_bios", ROMREGION_ERASEFF ) /* FM board*/
	ROM_LOAD( "sound.rom", 0x0000, 0x4000, CRC(6cdfa793) SHA1(4b8250f9b9db66548b79f961d61010558d6d6e1c))

	ROM_REGION( 0x46800, "kanji", ROMREGION_ERASEFF )
	ROM_LOAD( "font.rom", 0x0000, 0x46800, CRC(456d9fc7) SHA1(78ba9960f135372825ab7244b5e4e73a810002ff))
ROM_END

/* Genuine dumps */
COMP( 1994, pc486mu,   0,       0,     pc486mu,   pc486mu,   0, "Epson",   "PC-486MU",  GAME_NOT_WORKING | GAME_NO_SOUND)
