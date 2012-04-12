/***************************************************************************

        DEC Rainbow 100

        04/01/2012 Skeleton driver.

****************************************************************************/
#include "emu.h"
#include "cpu/i86/i86.h"
#include "cpu/z80/z80.h"
#include "video/vtvideo.h"

class rainbow_state : public driver_device
{
public:
	rainbow_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
	m_crtc(*this, "vt100_video")
	,
		m_p_ram(*this, "p_ram"){ }

	required_device<device_t> m_crtc;
	required_shared_ptr<UINT8> m_p_ram;
	UINT8 m_diagnostic;

	DECLARE_READ8_MEMBER(read_video_ram_r);
	DECLARE_WRITE8_MEMBER(clear_video_interrupt);

	DECLARE_READ8_MEMBER(diagnostic_r);
	DECLARE_WRITE8_MEMBER(diagnostic_w);
};


static ADDRESS_MAP_START( rainbow8088_map, AS_PROGRAM, 8, rainbow_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000, 0x1ffff) AM_RAM
	AM_RANGE(0xec000, 0xedfff) AM_RAM
	AM_RANGE(0xee000, 0xeffff) AM_RAM AM_SHARE("p_ram")
	AM_RANGE(0xf0000, 0xfffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( rainbow8088_io , AS_IO, 8, rainbow_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	// 0x04 Video processor DC011
	AM_RANGE (0x04, 0x04) AM_DEVWRITE_LEGACY("vt100_video", vt_video_dc011_w)

	AM_RANGE (0x0a, 0x0a) AM_READWRITE(diagnostic_r, diagnostic_w)
	// 0x0C Video processor DC012
	AM_RANGE (0x0c, 0x0c) AM_DEVWRITE_LEGACY("vt100_video", vt_video_dc012_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(rainbowz80_mem, AS_PROGRAM, 8, rainbow_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0xffff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( rainbowz80_io, AS_IO, 8, rainbow_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( rainbow )
INPUT_PORTS_END


static MACHINE_RESET( rainbow )
{
}

static SCREEN_UPDATE_IND16( rainbow )
{
	device_t *devconf = screen.machine().device("vt100_video");
	rainbow_video_update( devconf, bitmap, cliprect);
	return 0;
}

READ8_MEMBER( rainbow_state::read_video_ram_r )
{
	return m_p_ram[offset];
}

WRITE8_MEMBER( rainbow_state::clear_video_interrupt )
{
}

READ8_MEMBER( rainbow_state::diagnostic_r )
{
	return m_diagnostic | 0x0e;
}

WRITE8_MEMBER( rainbow_state::diagnostic_w )
{
	m_diagnostic = data;
}

static const vt_video_interface video_interface =
{
	"screen",
	"chargen",
	DEVCB_DRIVER_MEMBER(rainbow_state, read_video_ram_r),
	DEVCB_DRIVER_MEMBER(rainbow_state, clear_video_interrupt)
};

/* F4 Character Displayer */
static const gfx_layout rainbow_charlayout =
{
	8, 10,					/* 8 x 16 characters */
	256,					/* 256 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 15*8, 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8 },
	8*16					/* every char takes 16 bytes */
};

static GFXDECODE_START( rainbow )
	GFXDECODE_ENTRY( "chargen", 0x0000, rainbow_charlayout, 0, 1 )
GFXDECODE_END

static MACHINE_CONFIG_START( rainbow, rainbow_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",I8088, XTAL_24_0734MHz / 5)
	MCFG_CPU_PROGRAM_MAP(rainbow8088_map)
	MCFG_CPU_IO_MAP(rainbow8088_io)

	MCFG_CPU_ADD("subcpu",Z80, XTAL_24_0734MHz / 6)
	MCFG_CPU_PROGRAM_MAP(rainbowz80_mem)
	MCFG_CPU_IO_MAP(rainbowz80_io)
	MCFG_DEVICE_DISABLE()

	MCFG_MACHINE_RESET(rainbow)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_SIZE(80*10, 25*10)
	MCFG_SCREEN_VISIBLE_AREA(0, 80*10-1, 0, 25*10-1)
	MCFG_SCREEN_UPDATE_STATIC(rainbow)
	MCFG_GFXDECODE(rainbow)
	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(monochrome_green)
	MCFG_VT100_VIDEO_ADD("vt100_video", video_interface)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( rainbow )
	ROM_REGION(0x100000,"maincpu", 0)
	ROM_LOAD( "23-022e5-00.bin",  0xf0000, 0x4000, CRC(9d1332b4) SHA1(736306d2a36bd44f95a39b36ebbab211cc8fea6e))
	ROM_RELOAD(0xf4000,0x4000)
	ROM_LOAD( "23-020e5-00.bin", 0xf8000, 0x4000, CRC(8638712f) SHA1(8269b0d95dc6efbe67d500dac3999df4838625d8)) // German, French, English
	//ROM_LOAD( "23-015e5-00.bin", 0xf8000, 0x4000, NO_DUMP) // Dutch, French, English
	//ROM_LOAD( "23-016e5-00.bin", 0xf8000, 0x4000, NO_DUMP) // Finish, Swedish, English
	//ROM_LOAD( "23-017e5-00.bin", 0xf8000, 0x4000, NO_DUMP) // Danish, Norwegian, English
	//ROM_LOAD( "23-018e5-00.bin", 0xf8000, 0x4000, NO_DUMP) // Spanish, Italian, English
	ROM_RELOAD(0xfc000,0x4000)
	ROM_REGION(0x1000, "chargen", 0)
	ROM_LOAD( "chargen.bin", 0x0000, 0x1000, CRC(1685e452) SHA1(bc299ff1cb74afcededf1a7beb9001188fdcf02f))
ROM_END

/* Driver */

/*    YEAR  NAME     PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY                         FULLNAME       FLAGS */
COMP( 1982, rainbow, 0,      0,       rainbow,   rainbow, 0,  "Digital Equipment Corporation", "Rainbow 100B", GAME_NOT_WORKING | GAME_NO_SOUND)
