/***************************************************************************

        Commodore LCD prototype

        17/12/2009 Skeleton driver.

****************************************************************************/

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "rendlay.h"

class clcd_state : public driver_device
{
public:
	clcd_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	virtual void video_start();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);
};


static ADDRESS_MAP_START( clcd_mem, AS_PROGRAM, 8, clcd_state )
	AM_RANGE(0x0000, 0x7fff) AM_RAM
	AM_RANGE(0x8000, 0xffff) AM_ROM AM_REGION("maincpu", 0)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( clcd )
INPUT_PORTS_END


static PALETTE_INIT( clcd )
{
	palette_set_color(machine, 0, MAKE_RGB(138, 146, 148));
	palette_set_color(machine, 1, MAKE_RGB(92, 83, 88));
}

void clcd_state::video_start()
{
}

bool clcd_state::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)
{
    return 0;
}

static MACHINE_CONFIG_START( clcd, clcd_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",M6502, 2000000) // really M65C102
    MCFG_CPU_PROGRAM_MAP(clcd_mem)

    /* video hardware */
    MCFG_SCREEN_ADD("screen", LCD)
	MCFG_SCREEN_REFRESH_RATE(80)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(480, 128)
	MCFG_SCREEN_VISIBLE_AREA(0, 480-1, 0, 128-1)

	MCFG_DEFAULT_LAYOUT(layout_lcd)

	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(clcd)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( clcd )
    ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "kizapr.u102",		0x00000, 0x8000, CRC(59103d52) SHA1(e49c20b237a78b54c2cb26b133d5903bb60bd8ef))
	ROM_LOAD( "sizapr.u103",		0x08000, 0x8000, CRC(0aa91d9f) SHA1(f0842f370607f95d0a0ec6afafb81bc063c32745))
	ROM_LOAD( "sept-m-13apr.u104",	0x10000, 0x8000, CRC(41028c3c) SHA1(fcab6f0bbeef178eb8e5ecf82d9c348d8f318a8f))
	ROM_LOAD( "ss-calc-13apr.u105", 0x18000, 0x8000, CRC(88a587a7) SHA1(b08f3169b7cd696bb6a9b6e6e87a077345377ac4))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1985, clcd,  0,       0,	clcd,	clcd,	 0,			 "Commodore Business Machines",   "LCD (Prototype)",		GAME_NOT_WORKING | GAME_NO_SOUND )
