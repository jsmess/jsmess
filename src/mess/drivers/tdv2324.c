/*

    Tandberg TDV2324

    Skeleton driver

*/

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "machine/s1410.h"


class tdv2324_state : public driver_device
{
public:
	tdv2324_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

	virtual void video_start();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);
};

/* Memory Maps */

static ADDRESS_MAP_START( tdv2324_mem, AS_PROGRAM, 8, tdv2324_state )
	AM_RANGE(0x0000, 0x07ff) AM_ROM AM_REGION("maincpu", 0)
ADDRESS_MAP_END

static ADDRESS_MAP_START( tdv2324_sub_mem, AS_PROGRAM, 8, tdv2324_state )
	AM_RANGE(0x0000, 0x3fff) AM_ROM AM_REGION("subcpu", 0)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( tdv2324 )
INPUT_PORTS_END

/* Video */

void tdv2324_state::video_start()
{
}

bool tdv2324_state::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)
{
	return 0;
}

static I8085_CONFIG( i8085_intf )
{
	DEVCB_NULL,			/* STATUS changed callback */
	DEVCB_NULL,			/* INTE changed callback */
	DEVCB_NULL,	/* SID changed callback (I8085A only) */
	DEVCB_NULL	/* SOD changed callback (I8085A only) */
};

static I8085_CONFIG( i8085_sub_intf )
{
	DEVCB_NULL,			/* STATUS changed callback */
	DEVCB_NULL,			/* INTE changed callback */
	DEVCB_NULL,	/* SID changed callback (I8085A only) */
	DEVCB_NULL	/* SOD changed callback (I8085A only) */
};

/* Machine Drivers */

static MACHINE_CONFIG_START( tdv2324, tdv2324_state )
	/* basic system hardware */
	MCFG_CPU_ADD("maincpu", I8085A, 4000000)
	MCFG_CPU_PROGRAM_MAP(tdv2324_mem)
	MCFG_CPU_CONFIG(i8085_intf)

	MCFG_CPU_ADD("subcpu", I8085A, 4000000)
	MCFG_CPU_PROGRAM_MAP(tdv2324_sub_mem)
	MCFG_CPU_CONFIG(i8085_sub_intf)

	/* video hardware */
	MCFG_SCREEN_ADD( "screen", RASTER )
	MCFG_SCREEN_REFRESH_RATE( 50 )
	MCFG_SCREEN_FORMAT( BITMAP_FORMAT_INDEXED16 )
	MCFG_SCREEN_SIZE( 800, 400 )
	MCFG_SCREEN_VISIBLE_AREA( 0, 800-1, 0, 400-1 )

	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(monochrome_green)

	// devices
	MCFG_S1410_ADD() // 104521-F ROM
MACHINE_CONFIG_END

/* ROMs */

ROM_START( tdv2324 )
	ROM_REGION( 0x800, "maincpu", 0 )
	ROM_LOAD( "962058-1.bin", 0x000, 0x800, CRC(3771aece) SHA1(36d3f03235f327d6c8682e5c167aed6dddfaa6ec) )

	ROM_REGION( 0x4000, "subcpu", 0 )
	ROM_LOAD( "962107-1.bin", 0x0000, 0x4000, CRC(29c1a139) SHA1(f55fa9075fdbfa6a3e94e5120270179f754d0ea5) )

	ROM_REGION( 0x2000, "unknown", 0 )
	ROM_LOAD( "962014-1.bin", 0x0000, 0x2000, CRC(d01c32cd) SHA1(1f00f5f5ff0c035eec6af820b5acb6d0c207b6db) )

	ROM_REGION( 0x4000, "chargen", 0 )
	ROM_LOAD( "965268-1.bin", 0x0000, 0x4000, CRC(7222a85f) SHA1(e94074b68d90698734ab1fc38d156407846df47c) )
ROM_END

#define rom_tdv2324m7 rom_tdv2324

/* System Drivers */

//    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT       INIT    COMPANY           FULLNAME                FLAGS
COMP( 1983, tdv2324,		0,		0,		tdv2324,		tdv2324,		0,		"Tandberg",		"TDV 2324",		GAME_NOT_WORKING|GAME_NO_SOUND)
