/***************************************************************************

    SITCOM

    25/09/2011 Skeleton driver.

    http://www.izabella.me.uk/html/sitcom_.html
    http://www.sbprojects.com/sitcom/sitcom.htm

    The display consists of a red LED connected to SOD,
    and a pair of DL1414 intelligent alphanumeric displays (not emulated).

    The DL1414 has its own character generator rom, and control logic
    to display 4 characters (choice of 64) on a tiny LED readout.

    The DL1414 is a slightly simplified version of the DL2416.

    To replace the displays, we use a small video screen of 1x8 chars.

****************************************************************************/
#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "sitcom.lh"

#define VIDEO_START_MEMBER(name) void name::video_start()
#define SCREEN_UPDATE_MEMBER(name) bool name::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)


class sitcom_state : public driver_device
{
public:
	sitcom_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
	m_maincpu(*this, "maincpu")
	{ }

	required_device<cpu_device> m_maincpu;
	DECLARE_WRITE_LINE_MEMBER(sod_led);
	DECLARE_READ_LINE_MEMBER(sid_line);
	UINT8 *m_p_chargen;
	UINT8 *m_p_videoram;
	virtual void video_start();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);
};

static ADDRESS_MAP_START( sitcom_mem, AS_PROGRAM, 8, sitcom_state )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x07ff) AM_ROM
	AM_RANGE(0x8000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sitcom_io, AS_IO, 8, sitcom_state )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	// AM_RANGE(0x00, 0x1f) 8255 for expansion only
	AM_RANGE(0xc0, 0xc3) AM_RAM AM_REGION("maincpu", 0x7004) //left display
	AM_RANGE(0xe0, 0xe3) AM_RAM AM_REGION("maincpu", 0x7000) //right display
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( sitcom )
INPUT_PORTS_END

VIDEO_START_MEMBER( sitcom_state )
{
	m_p_chargen = machine().region("chargen")->base();
	m_p_videoram = machine().region("maincpu")->base()+0x7000;
}

SCREEN_UPDATE_MEMBER( sitcom_state )
{
	UINT8 ra,chr,gfx,x;

	for (ra = 0; ra < 9; ra++)
	{
		UINT16 *p = BITMAP_ADDR16(&bitmap, ra, 0);

		for (x = 0; x < 8; x++)
		{
			chr = m_p_videoram[7-x];

			if ((chr < 32) || (chr > 95))
				gfx = 0;
			else
				gfx = m_p_chargen[(chr<<4) | ra ];

			*p++ = BIT(gfx, 7);
			*p++ = BIT(gfx, 6);
			*p++ = BIT(gfx, 5);
			*p++ = BIT(gfx, 4);
			*p++ = BIT(gfx, 3);
			*p++ = BIT(gfx, 2);
			*p++ = BIT(gfx, 1);
			*p++ = BIT(gfx, 0);
		}
	}
	return 0;
}

// SID line used as serial input from a pc
READ_LINE_MEMBER( sitcom_state::sid_line )
{
	return 1; //idle - changing to 0 gives a FR ERROR
}

WRITE_LINE_MEMBER( sitcom_state::sod_led )
{
	output_set_value("sod_led", state);
}

static I8085_CONFIG( sitcom_cpu_config )
{
	DEVCB_NULL,		/* Status changed callback */
	DEVCB_NULL,			/* INTE changed callback */
	DEVCB_DRIVER_LINE_MEMBER(sitcom_state, sid_line), /* SID changed callback (I8085A only) */
	DEVCB_DRIVER_LINE_MEMBER(sitcom_state, sod_led) /* SOD changed callback (I8085A only) */
};

static MACHINE_CONFIG_START( sitcom, sitcom_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", I8085A, XTAL_6_144MHz) // 3.072MHz can be used for an old slow 8085
	MCFG_CPU_PROGRAM_MAP(sitcom_mem)
	MCFG_CPU_IO_MAP(sitcom_io)
	MCFG_CPU_CONFIG(sitcom_cpu_config)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(64, 9)
	MCFG_SCREEN_VISIBLE_AREA(0, 63, 0, 8)
	MCFG_PALETTE_LENGTH(2)
	MCFG_DEFAULT_LAYOUT(layout_sitcom)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( sitcom )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "boot8085.bin", 0x0000, 0x06b8, CRC(1b5e3310) SHA1(3323b65f0c10b7ab6bb75ec824e6d5fb643693a8))

	// dummy chargen to replace DL1414 displays
	ROM_REGION( 0x2000, "chargen", 0 )
	ROM_LOAD( "c10_char.bin", 0x0000, 0x2000, CRC(cb530b6f) SHA1(95590bbb433db9c4317f535723b29516b9b9fcbf))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT               COMPANY              FULLNAME       FLAGS */
COMP( 2002, sitcom, 0,      0,       sitcom,    sitcom,  0,   "San Bergmans & Izabella Malcolm", "Sitcom", GAME_NO_SOUND_HW)
