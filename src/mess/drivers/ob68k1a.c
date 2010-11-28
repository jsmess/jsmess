/***************************************************************************
   
        Omnibyte OB68K1A

        09/11/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/m68000/m68000.h"


class ob68k1a_state : public driver_device
{
public:
	ob68k1a_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT16* ram;
};



static ADDRESS_MAP_START(ob68k1a_mem, ADDRESS_SPACE_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000000, 0x00007fff) AM_RAM AM_BASE_MEMBER(ob68k1a_state, ram) // 32 KB RAM up to 128K
	AM_RANGE(0x00fe0000, 0x00feffff) AM_ROM AM_REGION("user1",0)	
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( ob68k1a )
INPUT_PORTS_END


static MACHINE_RESET(ob68k1a) 
{
	ob68k1a_state *state = machine->driver_data<ob68k1a_state>();	
	UINT8* user1 = memory_region(machine, "user1");

	memcpy((UINT8*)state->ram,user1,0x8000);

	machine->device("maincpu")->reset();
}

static VIDEO_START( ob68k1a )
{
}

static VIDEO_UPDATE( ob68k1a )
{
    return 0;
}

static MACHINE_CONFIG_START( ob68k1a, ob68k1a_state )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",M68000, XTAL_10MHz)
    MDRV_CPU_PROGRAM_MAP(ob68k1a_mem)
	
    MDRV_MACHINE_RESET(ob68k1a)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(ob68k1a)
    MDRV_VIDEO_UPDATE(ob68k1a)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( ob68k1a )
    ROM_REGION( 0x10000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "macsbug.u60",    0x0001, 0x2000, CRC(7c8905ff) SHA1(eba6c70f6b5b40d60e2885c2bd33dd93ec2aae48))
	ROM_LOAD16_BYTE( "macsbug.u61",    0x0000, 0x2000, CRC(b5069252) SHA1(b310465d8ece944bd694cc9726d03fed0f4b2c0f))
	ROM_LOAD16_BYTE( "idris_boot.u62", 0x4001, 0x2000, CRC(091e900e) SHA1(ea0c9f3ad5179eab2e743459c8afb707c059f0e2))
	ROM_LOAD16_BYTE( "idris_boot.u63", 0x4000, 0x2000, CRC(a132259f) SHA1(34216bf1d22ff0f0af29699a1e4e0e57631f775d))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1982, ob68k1a,  0,       0, 	ob68k1a, 	ob68k1a, 	 0,  "Omnibyte",   "OB68K1A",		GAME_NOT_WORKING | GAME_NO_SOUND_HW)

