/***************************************************************************

  snes.c

  Driver file to handle emulation of the Nintendo Super NES.

  R. Belmont
  Anthony Kruize
  Angelo Salese
  Fabio Priuli
  Based on the original MESS driver by Lee Hammerton (aka Savoury Snax)

  Driver is preliminary right now.

  The memory map included below is setup in a way to make it easier to handle
  Mode 20 and Mode 21 ROMs.

  Todo (in no particular order):
    - Fix additional sound bugs
    - Emulate extra chips - superfx, dsp2, sa-1 etc.
    - Add horizontal mosaic, hi-res. interlaced etc to video emulation.
    - Fix support for Mode 7. (In Progress)
    - Handle interleaved roms (maybe even multi-part roms, but how?)
    - Add support for running at 3.58 MHz at the appropriate time.
    - I'm sure there's lots more ...

***************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "cpu/spc700/spc700.h"
#include "cpu/superfx/superfx.h"
#include "cpu/g65816/g65816.h"
#include "includes/snes.h"
#include "machine/snescart.h"


/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( snes_map, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x000000, 0x2fffff) AM_READWRITE(snes_r_bank1, snes_w_bank1)	/* I/O and ROM (repeats for each bank) */
	AM_RANGE(0x300000, 0x3fffff) AM_READWRITE(snes_r_bank2, snes_w_bank2)	/* I/O and ROM (repeats for each bank) */
	AM_RANGE(0x400000, 0x5fffff) AM_READWRITE(snes_r_bank3, SMH_ROM)		/* ROM (and reserved in Mode 20) */
	AM_RANGE(0x600000, 0x6fffff) AM_READWRITE(snes_r_bank4, snes_w_bank4)	/* used by Mode 20 DSP-1 */
	AM_RANGE(0x700000, 0x7dffff) AM_READWRITE(snes_r_bank5, snes_w_bank5)
	AM_RANGE(0x7e0000, 0x7fffff) AM_RAM					/* 8KB Low RAM, 24KB High RAM, 96KB Expanded RAM */
	AM_RANGE(0x800000, 0xbfffff) AM_READWRITE(snes_r_bank6, snes_w_bank6)	/* Mirror and ROM */
	AM_RANGE(0xc00000, 0xffffff) AM_READWRITE(snes_r_bank7, snes_w_bank7)	/* Mirror and ROM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( superfx_map, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x000000, 0x3fffff) AM_READWRITE(superfx_r_bank1, superfx_w_bank1)
	AM_RANGE(0x400000, 0x5fffff) AM_READWRITE(superfx_r_bank2, superfx_w_bank2)
	AM_RANGE(0x600000, 0x7fffff) AM_READWRITE(superfx_r_bank3, superfx_w_bank3)
ADDRESS_MAP_END

static READ8_HANDLER( spc_ram_100_r )
{
	return spc_ram_r(space, offset + 0x100);
}

static WRITE8_HANDLER( spc_ram_100_w )
{
	spc_ram_w(space, offset + 0x100, data);
}

static ADDRESS_MAP_START( spc_map, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x00ef) AM_READWRITE(spc_ram_r, spc_ram_w) AM_BASE(&spc_ram)   	/* lower 32k ram */
	AM_RANGE(0x00f0, 0x00ff) AM_READWRITE(spc_io_r, spc_io_w)   	/* spc io */
	AM_RANGE(0x0100, 0xffff) AM_WRITE(spc_ram_100_w)
	AM_RANGE(0x0100, 0xffbf) AM_READ(spc_ram_100_r)
	AM_RANGE(0xffc0, 0xffff) AM_READ(spc_ipl_r)
ADDRESS_MAP_END



/***************************************************************************

  Input ports

***************************************************************************/

static INPUT_PORTS_START( snes )
	PORT_START("PAD1L")		/* Joypad 1 - L */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_NAME("P1 Button A")  PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_NAME("P1 Button X")  PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_NAME("P1 Button L")  PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON6 ) PORT_NAME("P1 Button R")  PORT_PLAYER(1)

	PORT_START("PAD1H")		/* Joypad 1 - H */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("P1 Button B")  PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("P1 Button Y")  PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SELECT ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)

	PORT_START("PAD2L")		/* Joypad 2 - L */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_NAME("P2 Button A")  PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_NAME("P2 Button X")  PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_NAME("P2 Button L")  PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON6 ) PORT_NAME("P2 Button R")  PORT_PLAYER(2)

	PORT_START("PAD2H")		/* Joypad 2 - H */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("P2 Button B")  PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("P2 Button Y")  PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SELECT ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)

	PORT_START("PAD3L")		/* Joypad 3 - L */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_NAME("P3 Button A")  PORT_PLAYER(3)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_NAME("P3 Button X")  PORT_PLAYER(3)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_NAME("P3 Button L")  PORT_PLAYER(3)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON6 ) PORT_NAME("P3 Button R")  PORT_PLAYER(3)

	PORT_START("PAD3H")		/* Joypad 3 - H */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("P3 Button B")  PORT_PLAYER(3)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("P3 Button Y")  PORT_PLAYER(3)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SELECT ) PORT_PLAYER(3)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START ) PORT_PLAYER(3)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(3)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(3)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(3)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(3)

	PORT_START("PAD4L")		/* Joypad 4 - L */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_NAME("P4 Button A")  PORT_PLAYER(4)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_NAME("P4 Button X")  PORT_PLAYER(4)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_NAME("P4 Button L")  PORT_PLAYER(4)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON6 ) PORT_NAME("P4 Button R")  PORT_PLAYER(4)

	PORT_START("PAD4H")		/* Joypad 4 - H */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("P4 Button B")  PORT_PLAYER(4)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("P4 Button Y")  PORT_PLAYER(4)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SELECT ) PORT_PLAYER(4)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START ) PORT_PLAYER(4)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(4)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(4)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(4)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(4)

	PORT_START("INTERNAL")	/* Configuration */
	PORT_CONFNAME( 0x1, 0x1, "Enforce 32 sprites/line" )
	PORT_CONFSETTING(   0x0, DEF_STR( No ) )
	PORT_CONFSETTING(   0x1, DEF_STR( Yes ) )

#ifdef MAME_DEBUG
	PORT_START("DEBUG1")	/* debug switches */
	PORT_DIPNAME( 0x3, 0x0, "Browse tiles" )
	PORT_DIPSETTING(   0x0, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x1, "2bpl" )
	PORT_DIPSETTING(   0x2, "4bpl" )
	PORT_DIPSETTING(   0x3, "8bpl" )
	PORT_DIPNAME( 0xc, 0x0, "Browse maps" )
	PORT_DIPSETTING(   0x0, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x4, "2bpl" )
	PORT_DIPSETTING(   0x8, "4bpl" )
	PORT_DIPSETTING(   0xc, "8bpl" )

	PORT_START("DEBUG2")	/* debug switches */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Toggle BG 1") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Toggle BG 2") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Toggle BG 3") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Toggle BG 4") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Toggle Objects") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Toggle Main/Sub") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Toggle Back col") PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Toggle Windows") PORT_CODE(KEYCODE_8_PAD)

	PORT_START("DEBUG3")	/* IN 11 : debug input */
	PORT_BIT( 0x1, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Pal prev") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x2, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Pal next") PORT_CODE(KEYCODE_N)
	PORT_BIT( 0x4, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Toggle Transparency") PORT_CODE(KEYCODE_9_PAD)

	PORT_START("DEBUG4")	/* debug switches */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Toggle Mode 0 draw")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Toggle Mode 1 draw")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Toggle Mode 2 draw")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Toggle Mode 3 draw")
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Toggle Mode 4 draw")
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Toggle Mode 5 draw")
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Toggle Mode 6 draw")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Toggle Mode 7 draw")
#endif
INPUT_PORTS_END


/***************************************************************************

  Machine driver

***************************************************************************/


static MACHINE_DRIVER_START( snes )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", G65816, MCLK_NTSC/6)	/* 2.68 MHz, also 3.58 MHz */
	MDRV_CPU_PROGRAM_MAP(snes_map)

	MDRV_CPU_ADD("soundcpu", SPC700, 1024000)	/* 1.024 MHz */
	MDRV_CPU_PROGRAM_MAP(spc_map)

	//MDRV_QUANTUM_TIME(HZ(48000))
	MDRV_QUANTUM_PERFECT_CPU("maincpu")

	MDRV_MACHINE_START( snes_mess )
	MDRV_MACHINE_RESET( snes )

	/* video hardware */
	MDRV_VIDEO_START( generic_bitmapped )
	MDRV_VIDEO_UPDATE( snes )

	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)

	MDRV_SCREEN_RAW_PARAMS(DOTCLK_NTSC, SNES_HTOTAL, 0, SNES_SCR_WIDTH, SNES_VTOTAL_NTSC, 0, SNES_SCR_HEIGHT_NTSC)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")
	MDRV_SOUND_ADD("custom", SNES, 0)
	MDRV_SOUND_ROUTE(0, "lspeaker", 1.00)
	MDRV_SOUND_ROUTE(1, "rspeaker", 1.00)

	MDRV_IMPORT_FROM(snes_cartslot)
MACHINE_DRIVER_END

static SUPERFX_CONFIG( snes_superfx_config )
{
	DEVCB_LINE(snes_extern_irq_w)	/* IRQ line from cart */
};

static MACHINE_DRIVER_START( snessfx )
	MDRV_IMPORT_FROM(snes)

	MDRV_CPU_ADD("superfx", SUPERFX, 21480000)	/* 21.48MHz */
	MDRV_CPU_PROGRAM_MAP(superfx_map)
	MDRV_CPU_CONFIG(snes_superfx_config)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( snespal )
	MDRV_IMPORT_FROM(snes)
	MDRV_CPU_REPLACE("maincpu", G65816, MCLK_PAL/6)

	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_RAW_PARAMS(DOTCLK_PAL, SNES_HTOTAL, 0, SNES_SCR_WIDTH, SNES_VTOTAL_PAL, 0, SNES_SCR_HEIGHT_PAL)
MACHINE_DRIVER_END


/***************************************************************************

  ROM definition(s)

***************************************************************************/

ROM_START(snes)
	ROM_REGION( 0x100, "user5", 0 )		/* IPL ROM */
	ROM_LOAD( "spc700.rom", 0, 0x40, CRC(44bb3a40) SHA1(97e352553e94242ae823547cd853eecda55c20f0) )	/* boot rom */
	ROM_REGION( 0x800, "user6", 0 )		/* add-on chip ROMs (DSP, SFX, etc) */
	ROM_LOAD( "dsp1data.bin", 0x000000, 0x000800, CRC(4b02d66d) SHA1(1534f4403d2a0f68ba6e35186fe7595d33de34b1) )
ROM_END

ROM_START(snessfx)
	ROM_REGION( 0x100, "user5", 0 )		/* IPL ROM */
	ROM_LOAD( "spc700.rom", 0, 0x40, CRC(44bb3a40) SHA1(97e352553e94242ae823547cd853eecda55c20f0) )	/* boot rom */
	ROM_REGION( 0x800, "user6", 0 )		/* add-on chip ROMs (DSP, SFX, etc) */
	ROM_LOAD( "dsp1data.bin", 0x000000, 0x000800, CRC(4b02d66d) SHA1(1534f4403d2a0f68ba6e35186fe7595d33de34b1) )
ROM_END

ROM_START(snespal)
	ROM_REGION( 0x100, "user5", 0 )		/* IPL ROM */
	ROM_LOAD( "spc700.rom", 0, 0x40, CRC(44bb3a40) SHA1(97e352553e94242ae823547cd853eecda55c20f0) )	/* boot rom */
	ROM_REGION( 0x800, "user6", 0 )		/* add-on chip ROMs (DSP, SFX, etc) */
	ROM_LOAD( "dsp1data.bin", 0x000000, 0x000800, CRC(4b02d66d) SHA1(1534f4403d2a0f68ba6e35186fe7595d33de34b1) )
ROM_END

ROM_START(sfcbox)
	/* atm, we only load these here, but we don't really emulate anything */
	ROM_REGION( 0x10000, "grom", 0 )
	ROM_LOAD( "grom1-1.bin", 0x0000, 0x8000, CRC(333bf9a7) SHA1(5d0cd9ca29e5580c3eebe9f136839987c879f979) )
	ROM_LOAD( "grom1-3.bin", 0x8000, 0x8000, CRC(ebec4c1c) SHA1(d638ef1486b4c0b3d4d5b666929ca7947e16efad) )

	ROM_REGION( 0x80000, "atrom", 0 )
	ROM_LOAD( "atrom-4s-0.smc", 0x00000, 0x80000, CRC(ad3ec05c) SHA1(a3d336db585fe02a37c323422d9db6a33fd489a6) )

	ROM_REGION( 0x100, "user5", 0 )		/* IPL ROM */
	ROM_LOAD( "spc700.rom", 0x00, 0x40, CRC(44bb3a40) SHA1(97e352553e94242ae823547cd853eecda55c20f0) )	/* boot rom */

	ROM_REGION( 0x800, "user6", ROMREGION_ERASE00 )		/* add-on chip ROMs (DSP, SFX, etc) */
ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME     PARENT  COMPAT  MACHINE  INPUT  INIT  CONFIG  COMPANY     FULLNAME                                      FLAGS */
CONS( 1989, snes,    0,      0,      snes,    snes,  0,    0,   "Nintendo", "Super Nintendo Entertainment System / Super Famicom (NTSC)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
CONS( 1989, snessfx, snes,   0,      snessfx, snes,  0,    0,   "Nintendo", "Super Nintendo Entertainment System / Super Famicom (NTSC), SuperFX", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
CONS( 1991, snespal, snes,   0,      snespal, snes,  0,    0,   "Nintendo", "Super Nintendo Entertainment System (PAL)",  GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
CONS( 199?, sfcbox,  snes,   0,      snes,    snes,  0,    0,   "Nintendo", "Super Famicom Box (NTSC)", GAME_NOT_WORKING )
