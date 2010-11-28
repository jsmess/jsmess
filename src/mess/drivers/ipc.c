/***************************************************************************

        Intel iPC

        17/12/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/i8085/i8085.h"


class ipc_state : public driver_device
{
public:
	ipc_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};


static ADDRESS_MAP_START(ipc_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xdfff) AM_RAM
	AM_RANGE(0xe800, 0xf7ff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( ipc_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( ipc )
INPUT_PORTS_END


static MACHINE_RESET(ipc)
{
	cpu_set_reg(machine->device("maincpu"), I8085_PC, 0xE800);
}

static VIDEO_START( ipc )
{
}

static VIDEO_UPDATE( ipc )
{
    return 0;
}

static MACHINE_CONFIG_START( ipc, ipc_state )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",I8085A, XTAL_19_6608MHz / 4)
    MDRV_CPU_PROGRAM_MAP(ipc_mem)
    MDRV_CPU_IO_MAP(ipc_io)

    MDRV_MACHINE_RESET(ipc)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(ipc)
    MDRV_VIDEO_UPDATE(ipc)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( ipc )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "ipc_u82_v1.3_104584-001.bin", 0xe800, 0x1000, CRC(0889394f) SHA1(b7525baf1884a7d67402dea4b5566016a9861ef2))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 19??, ipc,  0,       0,			ipc,	ipc,	 0, 	 "Intel",   "iPC",		GAME_NOT_WORKING | GAME_NO_SOUND)

