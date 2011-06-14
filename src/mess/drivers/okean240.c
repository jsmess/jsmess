/***************************************************************************

        Okeah-240 (Ocean-240)

        28/12/2011 Skeleton driver.

        Info from EMU80:

intctl : K580wn59
  irq[0]=kbd.irq2
  irq[1]=kbd.irq
  irq[4]=tim.out[0]

ppa40 : K580ww55
  portA=kbd.pressed.mask
  portB[2]=cas.playback
  portB[5]=kbd.key[58]
  portB[6]=kbd.key[59]
  portC[0-3]=kbd.pressed.row
  portC[4]=kbd.ack

ppaC0 : K580ww55
  portA=vid.scroll.y
  portB[0-3]=mem.frame[0].page
  portB[1-3]=mem.frame[1].page
  portB[4-5]=mm.page
  portC=vid.scroll.x


NOTES ABOUT THE TEST ROM:
The test rom tests various things including video memory. The test results
are sent to the comport (for us, the generic terminal). Therefore it would
be useful to have the 2 screens side-by-side. Unfortunately they will not
co-exist, instead causing a crash at start ("Mandatory artwork is missing").
So, if you want to see the test rom results, you'll have to compile a modified
driver (change the below line to 1).

****************************************************************************/
#define OKEAN240_USING_TESTROM 0

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "machine/terminal.h"

#define MACHINE_RESET_MEMBER(name) void name::machine_reset()
#define VIDEO_START_MEMBER(name) void name::video_start()
#define SCREEN_UPDATE_MEMBER(name) bool name::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)

class okean240_state : public driver_device
{
public:
	okean240_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

	DECLARE_READ8_MEMBER(okean240_rand_r);
	UINT8 *m_p_videoram;
	UINT8 m_term_data;
	virtual void machine_reset();
	virtual void video_start();
	//virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);
};


READ8_MEMBER( okean240_state::okean240_rand_r )
{
	if ((offset == 0xa1) || (offset == 0x41))
		return 1 | (machine().rand() & 2);
	else
	if (offset == 0x80)
		return machine().rand() & 0x10; //12
	else
		return 0;
}

static ADDRESS_MAP_START(okean240_mem, AS_PROGRAM, 8, okean240_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x07ff) AM_RAMBANK("boot")
	AM_RANGE(0x0800, 0x3fff) AM_RAM
	AM_RANGE(0x4000, 0x7fff) AM_RAM AM_BASE(m_p_videoram)
	AM_RANGE(0x8000, 0xbfff) AM_RAM
	AM_RANGE(0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START(okean240_io, AS_IO, 8, okean240_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0xff) AM_READ(okean240_rand_r)
#if OKEAN240_USING_TESTROM
	AM_RANGE(0xa0, 0xa0) AM_DEVWRITE_LEGACY(TERMINAL_TAG, terminal_write)
#endif
	// AM_RANGE(0x00, 0x1f)=ppa00.data
	// AM_RANGE(0x20, 0x23)=dsk.data
	// AM_RANGE(0x24, 0x24)=dsk.wait
	// AM_RANGE(0x25, 0x25)=dskctl.data
	// AM_RANGE(0x40, 0x5f)=ppa40.data
	// AM_RANGE(0x60, 0x7f)=tim.data
	// AM_RANGE(0x80, 0x81)=intctl.data
	// AM_RANGE(0xa0, 0xa1)=comport.data
	// AM_RANGE(0xc0, 0xdf)=ppaC0.data
	// AM_RANGE(0xe0, 0xff)=ppaE0.data
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( okean240 )
INPUT_PORTS_END


/* after the first 6 bytes have been read from ROM, switch the ram back in */
static TIMER_CALLBACK( okean240_boot )
{
	memory_set_bank(machine, "boot", 0);
}

MACHINE_RESET_MEMBER( okean240_state )
{
	machine().scheduler().timer_set(attotime::from_usec(10), FUNC(okean240_boot));
	memory_set_bank(machine(), "boot", 1);
}

static GENERIC_TERMINAL_INTERFACE( terminal_intf )
{
	DEVCB_NULL // testrom doesn't require input
};

DRIVER_INIT( okean240 )
{
	UINT8 *RAM = machine.region("maincpu")->base();
	memory_configure_bank(machine, "boot", 0, 2, &RAM[0x0000], 0xe000);
}

VIDEO_START_MEMBER( okean240_state )
{
}

// need to add colour
SCREEN_UPDATE( okean240 )
{
	okean240_state *state = screen->machine().driver_data<okean240_state>();
	UINT8 gfx,a;
	UINT16 x,y;

	for (y = 0; y < 256; y++)
	{
		UINT16 *p = BITMAP_ADDR16(bitmap, y, 0);

		for (x = 0; x < 0x4000; x+=0x200)
		{
			gfx = state->m_p_videoram[x|y];
			a = state->m_p_videoram[x|0x100|y];
			gfx |= a;

			/* Display a scanline of a character */
			*p++ = BIT(gfx, 0);
			*p++ = BIT(gfx, 1);
			*p++ = BIT(gfx, 2);
			*p++ = BIT(gfx, 3);
			*p++ = BIT(gfx, 4);
			*p++ = BIT(gfx, 5);
			*p++ = BIT(gfx, 6);
			*p++ = BIT(gfx, 7);
		}
	}
	return 0;
}

/* F4 Character Displayer */
static const gfx_layout okean240_charlayout =
{
	8, 7,					/* 8 x 7 characters */
	160,					/* 160 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8 },
	8*7					/* every char takes 7 bytes */
};

static GFXDECODE_START( okean240 )
	GFXDECODE_ENTRY( "maincpu", 0xef63, okean240_charlayout, 0, 1 )
GFXDECODE_END

static MACHINE_CONFIG_START( okean240, okean240_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",I8080, XTAL_12MHz / 6)
	MCFG_CPU_PROGRAM_MAP(okean240_mem)
	MCFG_CPU_IO_MAP(okean240_io)

	/* video hardware */
#if OKEAN240_USING_TESTROM
	MCFG_FRAGMENT_ADD( generic_terminal )
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, terminal_intf)
#else
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(256, 256)
	MCFG_SCREEN_VISIBLE_AREA(0, 255, 0, 255)
	MCFG_GFXDECODE(okean240)
	MCFG_PALETTE_LENGTH(8)
	//MCFG_PALETTE_INIT(black_and_white)
	MCFG_SCREEN_UPDATE(okean240)
#endif

MACHINE_CONFIG_END

/* ROM definition */
ROM_START( okean240 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS(0, "Normal", "Normal")
	ROMX_LOAD( "monitor.bin", 0xe000, 0x2000, CRC(587799bc) SHA1(1f677afa96722ca4ed2643eaca243548845fc854), ROM_BIOS(1))
	ROMX_LOAD( "cpm80.bin",   0xc000, 0x2000, CRC(7378e4f9) SHA1(c3c06c6f2e953546452ca6f82140a79d0e4953b4), ROM_BIOS(1))

	ROM_SYSTEM_BIOS(1, "FDDNormal", "FDDNormal")
	ROMX_LOAD( "monitor.bin", 0xe000, 0x2000, CRC(bcac5ca0) SHA1(602ab824704d3d5d07b3787f6227ff903c33c9d5), ROM_BIOS(2))
	ROMX_LOAD( "cpm80.bin",   0xc000, 0x2000, CRC(b89a7e16) SHA1(b8f937c04f430be18e48f296ed3ef37194171204), ROM_BIOS(2))

	ROM_SYSTEM_BIOS(2, "Test", "Test")
	ROMX_LOAD( "test.bin",    0xe000, 0x0800, CRC(e9e2b7b9) SHA1(e4e0b6984a2514b6ba3e97500d487ea1a68b7577), ROM_BIOS(3))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT     INIT         COMPANY     FULLNAME       FLAGS */
COMP( ????, okean240,  0,       0,   okean240,  okean240, okean240,  "<unknown>", "Okean-240", GAME_NOT_WORKING | GAME_NO_SOUND)

