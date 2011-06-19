/***************************************************************************

        Robotron Z9001 (KC85/1)

        12/05/2009 Skeleton driver.

        RAM E800-EBFF is colorram. When this is fixed, the cursor
        will show.

        The keyboard is interrupt-driven, so the devices need to be
        identified and added.

****************************************************************************/
#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"

#define MACHINE_RESET_MEMBER(name) void name::machine_reset()
//#define MACHINE_START_MEMBER(name) void name::machine_start()
#define VIDEO_START_MEMBER(name) void name::video_start()

class z9001_state : public driver_device
{
public:
	z9001_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
	m_maincpu(*this, "maincpu")
	{ }

	required_device<cpu_device> m_maincpu;
	const UINT8 *m_p_videoram;
	const UINT8 *m_p_chargen;
	virtual void machine_reset();
	//virtual void machine_start();
	virtual void video_start();
};

static ADDRESS_MAP_START(z9001_mem, AS_PROGRAM, 8, z9001_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0xebff ) AM_RAM
	AM_RANGE( 0xec00, 0xefff ) AM_RAM AM_BASE(m_p_videoram)
	AM_RANGE( 0xf000, 0xffff ) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( z9001_io, AS_IO, 8, z9001_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( z9001 )
INPUT_PORTS_END


MACHINE_RESET_MEMBER( z9001_state )
{
	cpu_set_reg(machine().device("maincpu"), Z80_PC, 0xf000);
}

VIDEO_START_MEMBER( z9001_state )
{
	m_p_chargen = machine().region("chargen")->base();
}

static SCREEN_UPDATE( z9001 )
{
	z9001_state *state = screen->machine().driver_data<z9001_state>();
	UINT8 y,ra,chr,gfx;
	UINT16 sy=0,ma=0,x;

	for(y = 0; y < 24; y++ )
	{
		for (ra = 0; ra < 8; ra++)
		{
			UINT16 *p = BITMAP_ADDR16(bitmap, sy++, 0);

			for (x = ma; x < ma + 40; x++)
			{
				chr = state->m_p_videoram[x]; // get char in videoram
				gfx = state->m_p_chargen[(chr<<3) | ra]; // get dot pattern in chargen

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
		ma+=40;
	}
	return 0;
}

/* F4 Character Displayer */
static const gfx_layout z9001_charlayout =
{
	8, 8,					/* 8 x 8 characters */
	1024,					/* 4 x 256 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8					/* every char takes 8 bytes */
};

static GFXDECODE_START( z9001 )
	GFXDECODE_ENTRY( "chargen", 0x0000, z9001_charlayout, 0, 1 )
GFXDECODE_END


static MACHINE_CONFIG_START( z9001, z9001_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",Z80, XTAL_9_8304MHz / 4)
	MCFG_CPU_PROGRAM_MAP(z9001_mem)
	MCFG_CPU_IO_MAP(z9001_io)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(40*8, 24*8)
	MCFG_SCREEN_VISIBLE_AREA(0, 40*8-1, 0, 24*8-1)
	MCFG_SCREEN_UPDATE(z9001)
	MCFG_GFXDECODE(z9001)
	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(monochrome_green)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( z9001 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS( 0, "orig", "Original" )
	ROMX_LOAD( "os____f0.851", 0xf000, 0x1000, CRC(9fe60a92) SHA1(553609631f5eaa7d6758a73f56c613e280a5b310), ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "rb20", "ROM-Bank System without menu" )
	ROMX_LOAD( "os_rb20.rom",  0xf000, 0x1000, CRC(c783124d) SHA1(c2893ce5bb23b280ba4e982e860586d21de2469b), ROM_BIOS(2))
	ROM_SYSTEM_BIOS( 2, "rb21", "ROM-Bank System with menu" )
	ROMX_LOAD( "os_rb21.rom",  0xf000, 0x1000, CRC(11eec2dd) SHA1(5dbb661bdf4daf92d6c4ffbbdec674e57917e9eb), ROM_BIOS(3))

	ROM_REGION( 0x2000, "chargen", 0 )
	ROM_LOAD( "chargen.851", 0x0000, 0x0800, CRC(dd9c0f4e) SHA1(2e4928ba7161f5cce7173b7d2ded3d6596ae2aa2))
	ROM_LOAD( "zg_cga.rom",  0x0800, 0x0800, CRC(697cefb1) SHA1(f57a78a928fe1151b2fedb7f1a93a141195422ff))
	ROM_LOAD( "zg_cgai.rom", 0x1000, 0x0800, CRC(ecadf355) SHA1(4d36fefd335903680c45a5e3f38b969d2e9bb621))
	ROM_LOAD( "zg_de.rom",   0x1800, 0x0800, CRC(71854b0a) SHA1(912bb7d1f8b4582894125e82da080bd9c3b88f34))
ROM_END

#define rom_kc85_111 rom_z9001

ROM_START( kc87_10 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS( 0, "orig", "Original" )
	ROMX_LOAD( "os____f0.851", 0xf000, 0x1000, CRC(9fe60a92) SHA1(553609631f5eaa7d6758a73f56c613e280a5b310), ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "rb20", "ROM-Bank System without menu" )
	ROMX_LOAD( "os_rb20.rom",  0xf000, 0x1000, CRC(c783124d) SHA1(c2893ce5bb23b280ba4e982e860586d21de2469b), ROM_BIOS(2))
	ROM_SYSTEM_BIOS( 2, "rb21", "ROM-Bank System with menu" )
	ROMX_LOAD( "os_rb21.rom",  0xf000, 0x1000, CRC(11eec2dd) SHA1(5dbb661bdf4daf92d6c4ffbbdec674e57917e9eb), ROM_BIOS(3))

	ROM_LOAD( "basic_c0.87a", 0xc000, 0x2800, CRC(c508d45e) SHA1(ea85b53e21429c4cb85cdb81b92f278a8f4eb574))

	ROM_REGION( 0x2000, "chargen", 0 )
	ROM_LOAD( "chargen.851", 0x0000, 0x0800, CRC(dd9c0f4e) SHA1(2e4928ba7161f5cce7173b7d2ded3d6596ae2aa2))
	ROM_LOAD( "zg_cga.rom",  0x0800, 0x0800, CRC(697cefb1) SHA1(f57a78a928fe1151b2fedb7f1a93a141195422ff))
	ROM_LOAD( "zg_cgai.rom", 0x1000, 0x0800, CRC(ecadf355) SHA1(4d36fefd335903680c45a5e3f38b969d2e9bb621))
	ROM_LOAD( "zg_de.rom",   0x1800, 0x0800, CRC(71854b0a) SHA1(912bb7d1f8b4582894125e82da080bd9c3b88f34))
ROM_END

#define rom_kc87_11 rom_kc87_10

ROM_START( kc87_20 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS( 0, "orig", "Original" )
	ROMX_LOAD( "os____f0.87b", 0xf000, 0x1000, CRC(a357d093) SHA1(b1df6b499517c8366a0795030ee800e8a258e938), ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "rb20", "ROM-Bank System without menu" )
	ROMX_LOAD( "os_rb20.rom",  0xf000, 0x1000, CRC(c783124d) SHA1(c2893ce5bb23b280ba4e982e860586d21de2469b), ROM_BIOS(2))
	ROM_SYSTEM_BIOS( 2, "rb21", "ROM-Bank System with menu" )
	ROMX_LOAD( "os_rb21.rom",  0xf000, 0x1000, CRC(11eec2dd) SHA1(5dbb661bdf4daf92d6c4ffbbdec674e57917e9eb), ROM_BIOS(3))

	ROM_LOAD( "basic_c0.87b", 0xc000, 0x2800, CRC(9e8f6380) SHA1(8ffecc64ba35c953c93738f8568c83dc6af1ae72))

	ROM_REGION( 0x2000, "chargen", 0 )
	ROM_LOAD( "chargen.851", 0x0000, 0x0800, CRC(dd9c0f4e) SHA1(2e4928ba7161f5cce7173b7d2ded3d6596ae2aa2))
	ROM_LOAD( "zg_cga.rom",  0x0800, 0x0800, CRC(697cefb1) SHA1(f57a78a928fe1151b2fedb7f1a93a141195422ff))
	ROM_LOAD( "zg_cgai.rom", 0x1000, 0x0800, CRC(ecadf355) SHA1(4d36fefd335903680c45a5e3f38b969d2e9bb621))
	ROM_LOAD( "zg_de.rom",   0x1800, 0x0800, CRC(71854b0a) SHA1(912bb7d1f8b4582894125e82da080bd9c3b88f34))
ROM_END

#define rom_kc87_21 rom_kc87_20

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT COMPANY   FULLNAME       FLAGS */
COMP( 198?, z9001,   0,       0,	z9001,	z9001,	 0, 	  "Robotron",   "Z9001 (KC 85/1.10)", GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 198?, kc85_111,z9001,   0,	z9001,	z9001,	 0, 	  "Robotron",   "KC 85/1.11", GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 198?, kc87_10, z9001,   0,	z9001,	z9001,	 0, 	  "Robotron",   "KC 87.10", GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 198?, kc87_11, z9001,   0,	z9001,	z9001,	 0, 	  "Robotron",   "KC 87.11", GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 198?, kc87_20, z9001,   0,	z9001,	z9001,	 0, 	  "Robotron",   "KC 87.20", GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 198?, kc87_21, z9001,   0,	z9001,	z9001,	 0, 	  "Robotron",   "KC 87.21", GAME_NOT_WORKING | GAME_NO_SOUND)
