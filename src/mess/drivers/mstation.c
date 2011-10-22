/***************************************************************************

    CIDCO MailStation

    22/10/2011 Skeleton driver.

    Hardware:
        - Z80 CPU
        - 29f080 8 Mbit flash
        - 28SF040 4 Mbit flash (for user data)
        - 128kb RAM
        - 320x128 LCD
        - RCV336ACFW 33.6kbps modem

    TODO:
    - Everything

    More info:
      http://www.fybertech.net/mailstation/info.php

****************************************************************************/

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/intelfsh.h"
#include "machine/rp5c01.h"
#include "rendlay.h"

class mstation_state : public driver_device
{
public:
	mstation_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		  m_maincpu(*this, "maincpu")
		{ }

	required_device<cpu_device> m_maincpu;

	intelfsh8_device *m_flashes[2];

	virtual void machine_start();
	virtual void machine_reset();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);
};

bool mstation_state::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)
{
	return 0;
}

static ADDRESS_MAP_START(mstation_mem, AS_PROGRAM, 8, mstation_state)
	AM_RANGE(0x0000, 0x3fff) AM_ROM AM_REGION("flash0", 0)
//  AM_RANGE(0x4000, 0x7fff) bank 1
//  AM_RANGE(0x8000, 0xbfff) bank 2
	AM_RANGE(0xc000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START(mstation_io , AS_IO, 8, mstation_state)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE( 0x10, 0x1f ) AM_DEVREADWRITE("rtc", rp5c01_device, read, write)
ADDRESS_MAP_END


/* Input ports */
static INPUT_PORTS_START( mstation )
INPUT_PORTS_END

void mstation_state::machine_start()
{
	m_flashes[0] = machine().device<intelfsh8_device>("flash0");
	m_flashes[1] = machine().device<intelfsh8_device>("flash1");
}


void mstation_state::machine_reset()
{
}

static RP5C01_INTERFACE( rtc_intf )
{
	DEVCB_NULL
};

static MACHINE_CONFIG_START( mstation, mstation_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",Z80, XTAL_4MHz)		//unknown clock
    MCFG_CPU_PROGRAM_MAP(mstation_mem)
    MCFG_CPU_IO_MAP(mstation_io)

    /* video hardware */
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(320, 128)
    MCFG_SCREEN_VISIBLE_AREA(0, 320-1, 0, 128-1)

    MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(black_and_white)
	MCFG_DEFAULT_LAYOUT(layout_lcd)

	MCFG_AMD_29F080_ADD("flash0")
	MCFG_AMD_29F080_ADD("flash1")	//SST-28SF040

	MCFG_RP5C01_ADD("rtc", XTAL_32_768kHz, rtc_intf)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( mstation )
	ROM_REGION( 0x100000, "flash0", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS( 0, "v303a", "v3.03a" )
	ROMX_LOAD( "ms303a.bin", 0x000000, 0x100000, CRC(7a5cf752) SHA1(15629ccaecd8094dd883987bed94c16eee6de7c2), ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "v253", "v2.53" )
	ROMX_LOAD( "ms253.bin",  0x000000, 0x0fc000, BAD_DUMP CRC(a27e7f8b) SHA1(ae5a0aa0f1e23f3b183c5c0bcf4d4c1ae54b1798), ROM_BIOS(2))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT COMPANY   FULLNAME       FLAGS */
COMP( 1999, mstation,  0,       0,	mstation,	mstation,  0,   "CIDCO",   "MailStation",		GAME_NOT_WORKING | GAME_NO_SOUND )
