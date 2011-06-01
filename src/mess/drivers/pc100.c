/***************************************************************************

    NEC PC-100

****************************************************************************/

#include "emu.h"
#include "cpu/i86/i86.h"


class pc100_state : public driver_device
{
public:
	pc100_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

};

static VIDEO_START( pc100 )
{
}

static SCREEN_UPDATE( pc100 )
{
	return 0;
}


static ADDRESS_MAP_START(pc100_map, AS_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000,0xbffff) AM_RAM // work ram
	AM_RANGE(0xc0000,0xdffff) AM_RAM // vram, blitter based!
	AM_RANGE(0xf8000,0xfffff) AM_ROM AM_REGION("ipl", 0)
ADDRESS_MAP_END

/* everything is 8-bit bus wide */
static ADDRESS_MAP_START(pc100_io, AS_IO, 16)
//	AM_RANGE(0x00, 0x03) i8259
//	AM_RANGE(0x04, 0x07) i8237?
//	AM_RANGE(0x08, 0x0b) upd765
//	AM_RANGE(0x10, 0x17) i8255 #0
//	AM_RANGE(0x18, 0x1f) i8255 #1
//	AM_RANGE(0x20, 0x23) misc ports
//	AM_RANGE(0x28, 0x2b) i8251
//	AM_RANGE(0x30, 0x31) crtc shift
//	AM_RANGE(0x38, 0x39) crtc address reg
//	AM_RANGE(0x3a, 0x3b) crtc data reg
//	AM_RANGE(0x3c, 0x3f) crtc vertical start position
//	AM_RANGE(0x40, 0x5f) palette
//	AM_RANGE(0x60, 0x61) crtc command (16-bit wide)
//	AM_RANGE(0x80, 0x83) kanji
//	AM_RANGE(0x84, 0x87) kanji "strobe" signal 0/1
ADDRESS_MAP_END


/* Input ports */
static INPUT_PORTS_START( pc100 )
INPUT_PORTS_END

static const gfx_layout kanji_layout =
{
	16, 16,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8 },
	{ STEP16(0,16) },
	16*16
};

static GFXDECODE_START( pc100 )
	GFXDECODE_ENTRY( "kanji", 0x0000, kanji_layout, 0, 1 )
GFXDECODE_END

static MACHINE_RESET(pc100)
{
}

static MACHINE_CONFIG_START( pc100, pc100_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", I8086, 4000000)
	MCFG_CPU_PROGRAM_MAP(pc100_map)
	MCFG_CPU_IO_MAP(pc100_io)

	MCFG_MACHINE_RESET(pc100)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(640, 480)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MCFG_SCREEN_UPDATE(pc100)

	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(black_and_white)

	MCFG_GFXDECODE(pc100)

	MCFG_VIDEO_START(pc100)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( pc100 )
	ROM_REGION( 0x8000, "ipl", ROMREGION_ERASEFF )
	ROM_LOAD( "ipl.rom", 0x0000, 0x8000, CRC(fd54a80e) SHA1(605a1b598e623ba2908a14a82454b9d32ea3c331))

	ROM_REGION( 0x20000, "kanji", ROMREGION_ERASEFF )
	ROM_LOAD( "kanji.rom", 0x0000, 0x20000, BAD_DUMP CRC(cb63802a) SHA1(0f9c2cd11b41b46a5c51426a04376c232ec90448))

ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY           FULLNAME       FLAGS */
COMP( 1986, pc100,  0,      0,       pc100,     pc100,    0,     "NEC",   "PC-100", GAME_NOT_WORKING | GAME_NO_SOUND)
