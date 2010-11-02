/***************************************************************************

    Tektronix 4705

    Skeleton driver.

****************************************************************************/

/*

    TODO:

	- everything

*/

#include "emu.h"
#include "cpu/i86/i86.h"

#define I80188_TAG "i80188"
#define SCREEN_TAG "screen"

class tek4705_state : public driver_device
{
public:
	tek4705_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }
};

/* Memory Maps */

static ADDRESS_MAP_START( tek4705_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0xc0000, 0xfffff) AM_ROM AM_REGION(I80188_TAG, 0)
ADDRESS_MAP_END

static ADDRESS_MAP_START( tek4705_io, ADDRESS_SPACE_IO, 8 )
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( tek4705 )
INPUT_PORTS_END

/* Video */

static VIDEO_START( tek4705 )
{
}

static VIDEO_UPDATE( tek4705 )
{
	return 0;
}

/* Machine Initialization */

static MACHINE_START( tek4705 )
{
}

static MACHINE_RESET( tek4705 )
{
}

/* Machine Driver */

static MACHINE_CONFIG_START( tek4705, tek4705_state )
    /* basic machine hardware */
	MDRV_CPU_ADD(I80188_TAG, I80188, 21000000)
    MDRV_CPU_PROGRAM_MAP(tek4705_mem)
    MDRV_CPU_IO_MAP(tek4705_io)

    MDRV_MACHINE_START(tek4705)
    MDRV_MACHINE_RESET(tek4705)

    /* video hardware */
    MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)

	MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_START(tek4705)
    MDRV_VIDEO_UPDATE(tek4705)
MACHINE_CONFIG_END

/* ROMs */

ROM_START( tek4705 )
	ROM_REGION( 0x40000, I80188_TAG, 0 )
	ROM_LOAD16_BYTE( "160-3283-02 v8.2.u60",  0x00000, 0x8000, CRC(2a821db6) SHA1(b4d8b74bd9fe43885dcdc4efbdd1eebb96e32060) )
	ROM_LOAD16_BYTE( "160-3284-02 v8.2.u160", 0x00001, 0x8000, CRC(ee567b01) SHA1(67b1b0648cfaa28d57473bcc45358ff2bf986acf) )
	ROM_LOAD16_BYTE( "160-3281-02 v8.2.u70",  0x10000, 0x8000, CRC(e2713328) SHA1(b0bb3471539ef24d79b18d0e33bc148ed27d0ec4) )
	ROM_LOAD16_BYTE( "160-3282-02 v8.2.u170", 0x10001, 0x8000, CRC(c109a4f7) SHA1(762019105c1f82200a9c99ccfcfd8ee81d2ac4fe) )
	ROM_LOAD16_BYTE( "160-3279-02 v8.2.u80",  0x20000, 0x8000, CRC(00822078) SHA1(a82e61dafccbaea44e67efaa5940e52ec6d07d7d) )
	ROM_LOAD16_BYTE( "160-3280-02 v8.2.u180", 0x20001, 0x8000, CRC(eec9f70f) SHA1(7b65336219f5fa0d11f8be2b37040b564a53c52f) )
	ROM_LOAD16_BYTE( "160-3277-02 v8.2.u90",  0x30000, 0x8000, CRC(cf6ebc97) SHA1(298db473874c57bf4eec788818179748030a9ad8) )
	ROM_LOAD16_BYTE( "160-3278-02 v8.2.u190", 0x30001, 0x8000, CRC(d6124cd1) SHA1(f826aee5ec07cf5ac369697d93def0259ad225bb) )

	ROM_REGION( 0x1000, "chargen", 0 )
	ROM_LOAD( "160-3087 v1.0.u855", 0x0000, 0x1000, CRC(97479528) SHA1(e9e15f1f64b3b6bd139accd51950bae71fdc2193) )
ROM_END

/* System Drivers */

/*    YEAR  NAME        PARENT      COMPAT  MACHINE     INPUT       INIT    COMPANY         FULLNAME            FLAGS */
COMP( 1985, tek4705,	0,			0,		tek4705,	tek4705,	0,		"Tektronix",	"Tektronix 4705",	GAME_NOT_WORKING | GAME_NO_SOUND )
