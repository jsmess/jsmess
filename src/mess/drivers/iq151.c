/***************************************************************************

        IQ-151

        12/05/2009 Skeleton driver.

****************************************************************************/
#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"

#define MACHINE_RESET_MEMBER(name) void name::machine_reset()
#define VIDEO_START_MEMBER(name) void name::video_start()
#define SCREEN_UPDATE_MEMBER(name) bool name::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)


class iq151_state : public driver_device
{
public:
	iq151_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

	UINT8 *m_p_ram;
	UINT8 *m_p_videoram;
	const UINT8 *m_p_chargen;
	UINT8 m_framecnt;
	virtual void machine_reset();
	virtual void video_start();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);
};


static ADDRESS_MAP_START(iq151_mem, AS_PROGRAM, 8, iq151_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x07ff ) AM_RAMBANK("boot")
	AM_RANGE( 0x0000, 0xc7ff ) AM_RAM
	AM_RANGE( 0xc800, 0xcfff ) AM_ROM // c800 has to be FF before we can boot
	AM_RANGE( 0xd000, 0xebff ) AM_RAM
	AM_RANGE( 0xec00, 0xefff ) AM_RAM AM_BASE(m_p_videoram)
	AM_RANGE( 0xf000, 0xffff ) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START(iq151_io, AS_IO, 8, iq151_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( iq151 )
INPUT_PORTS_END


/* after the first 4 bytes have been read from ROM, switch the ram back in */
static TIMER_CALLBACK( iq151_boot )
{
	memory_set_bank(machine, "boot", 0);
}

static TIMER_DEVICE_CALLBACK( iq151a )
{
	iq151_state *state = timer.machine().driver_data<iq151_state>();
	state->m_p_ram[6]=0; // fixme
}

DRIVER_INIT( iq151 )
{
	UINT8 *RAM = machine.region("maincpu")->base();
	memory_configure_bank(machine, "boot", 0, 2, &RAM[0x0000], 0xf800);
}

MACHINE_RESET_MEMBER( iq151_state )
{
	memory_set_bank(machine(), "boot", 1);
	machine().scheduler().timer_set(attotime::from_usec(5), FUNC(iq151_boot));
}

VIDEO_START_MEMBER( iq151_state )
{
	m_p_chargen = machine().region("chargen")->base();
	m_p_ram = machine().region("maincpu")->base();
}

SCREEN_UPDATE_MEMBER( iq151_state )
{
	UINT8 y,ra,chr,gfx;
	UINT16 sy=0,ma=0,x;
	UINT16 cursor = m_p_ram[12] | ((m_p_ram[13] << 8) & 3); // cursor address masked

	m_framecnt++;

	for (y = 0; y < 16; y++)
	{
		for (ra = 0; ra < 8; ra++)
		{
			UINT16 *p = BITMAP_ADDR16(&bitmap, sy++, 0);

			for (x = ma; x < ma+64; x++)
			{
				chr = m_p_videoram[x];

				gfx = m_p_chargen[(chr<<3) | ra ];

				//display cursor if at cursor position and flash on
				if ( (x==cursor) && (m_framecnt & 0x08) )
					gfx = 0xff;

				/* Display a scanline of a character */
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
		ma+=64;
	}
	return 0;
}

/* F4 Character Displayer */
static const gfx_layout iq151_32_charlayout =
{
	8, 8,					/* 8 x 8 characters */
	128,					/* 128 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8					/* every char takes 8 bytes */
};

static const gfx_layout iq151_64_charlayout =
{
	6, 8,					/* 6 x 8 characters */
	256,					/* 256 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8					/* every char takes 8 bytes */
};

static GFXDECODE_START( iq151 )
	GFXDECODE_ENTRY( "chargen", 0x0000, iq151_32_charlayout, 0, 1 )
	GFXDECODE_ENTRY( "chargen", 0x0400, iq151_64_charlayout, 0, 1 )
GFXDECODE_END

static MACHINE_CONFIG_START( iq151, iq151_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",Z80, XTAL_4MHz)
	MCFG_CPU_PROGRAM_MAP(iq151_mem)
	MCFG_CPU_IO_MAP(iq151_io)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(64*8, 16*8)
	MCFG_SCREEN_VISIBLE_AREA(0, 64*8-1, 0, 16*8-1)
	MCFG_GFXDECODE(iq151)
	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(monochrome_green)

	MCFG_TIMER_ADD_PERIODIC("iq151a", iq151a, attotime::from_hz(50) )
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( iq151 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	/* A number of bios versions here. The load address is shown for each */
	ROM_LOAD( "iq151_disc2_12_5_1987_v4_0.rom", 0xe000, 0x0800, CRC(b189b170) SHA1(3e2ca80934177e7a32d0905f5a0ad14072f9dabf))
	ROM_LOAD( "iq151_monitor_cpm.rom", 0xf000, 0x1000, CRC(26f57013) SHA1(4df396edc375dd2dd3c82c4d2affb4f5451066f1))
	ROM_LOAD( "iq151_monitor_cpm_old.rom", 0xf000, 0x1000, CRC(6743e1b7) SHA1(ae4f3b1ba2511a1f91c4e8afdfc0e5aeb0fb3a42))
	ROM_LOAD( "iq151_monitor_disasm.rom", 0xf000, 0x1000, CRC(45c2174e) SHA1(703e3271a124c3ef9330ae399308afd903316ab9))
	ROM_LOAD( "iq151_monitor_orig.rom", 0xf000, 0x1000, CRC(acd10268) SHA1(4d75c73f155ed4dc2ac51a9c22232f869cca95e2))

	ROM_REGION( 0x0c00, "chargen", ROMREGION_INVERT )
	ROM_LOAD( "iq151_video32font.rom", 0x0000, 0x0400, CRC(395567a7) SHA1(18800543daf4daed3f048193c6ae923b4b0e87db))
	ROM_LOAD( "iq151_video64font.rom", 0x0400, 0x0800, CRC(cb6f43c0) SHA1(4b2c1d41838d569228f61568c1a16a8d68b3dadf))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT   COMPANY   FULLNAME       FLAGS */
COMP( 198?, iq151,  0,       0,      iq151,     iq151,   iq151, "ZPA Novy Bor", "IQ-151", GAME_NOT_WORKING | GAME_NO_SOUND)
