/***************************************************************************

    commodore c65 home computer
    PeT mess@utanet.at

    documention
     www.funet.fi

***************************************************************************/

/*

2008 - Driver Updates 
---------------------

(most of the informations are taken from http://www.zimmers.net/cbmpics/ )


[CBM systems which belong to this driver]

* Commodore 65 (1989)

Also known as C64 DX at early stages of the project. It was cancelled 
around 1990-1991. Only few units survive (they were sold after Commodore 
liquidation in 1994).

CPU: CSG 4510 (3.54 MHz)
RAM: 128 kilobytes, expandable to 8 megabytes
ROM: 128 kilobytes
Video: CSG 4569 "VIC-III" (6 Video modes; Resolutions from 320x200 to 
	1280x400; 80 columns text; Palette of 4096 colors)
Sound: CSG 8580 "SID" x2 (6 voice stereo synthesizer/digital sound 
	capabilities)
Ports: CSG 4510 (2 Joystick/Mouse ports; CBM Serial port; CBM 'USER' 
	port; CBM Monitor port; Power and reset switches; C65 bus drive 
	port; RGBI video port; 2 RCA audio ports; RAM expansion port; C65 
	expansion port)
Keyboard: Full-sized 77 key QWERTY (12 programmable function keys;
	4 direction cursor-pad)
Additional Hardware: Built in 3.5" DD disk drive (1581 compatible)
Miscellaneous: Partially implemented Commodore 64 emulation

[Notes]

The datasette port was removed here. C65 supports an additional "dumb" 
drive externally. It also features, in addition to the standard CBM 
bus serial (available in all modes), a Fast and a Burst serial bus 
(both available in C65 mode only)

*/


#include "driver.h"
#include "cpu/m6502/m4510.h"

#include "sound/sid6581.h"
#include "machine/6526cia.h"

#include "machine/cbmipt.h"
#include "video/vic4567.h"
#include "video/vic6567.h"

/* devices config */
#include "includes/cbm.h"
#include "includes/cbmdrive.h"

#include "includes/c64.h"
#include "includes/c65.h"


/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( c65_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x00000, 0x07fff) AM_RAMBANK(11)
	AM_RANGE(0x08000, 0x09fff) AM_READWRITE(SMH_BANK(1), SMH_BANK(12))
	AM_RANGE(0x0a000, 0x0bfff) AM_READWRITE(SMH_BANK(2), SMH_BANK(13))
	AM_RANGE(0x0c000, 0x0cfff) AM_READWRITE(SMH_BANK(3), SMH_BANK(14))
	AM_RANGE(0x0d000, 0x0d7ff) AM_READWRITE(SMH_BANK(4), SMH_BANK(5))
	AM_RANGE(0x0d800, 0x0dbff) AM_READWRITE(SMH_BANK(6), SMH_BANK(7))
	AM_RANGE(0x0dc00, 0x0dfff) AM_READWRITE(SMH_BANK(8), SMH_BANK(9))
	AM_RANGE(0x0e000, 0x0ffff) AM_READWRITE(SMH_BANK(10), SMH_BANK(15))
	AM_RANGE(0x10000, 0x1f7ff) AM_RAM
	AM_RANGE(0x1f800, 0x1ffff) AM_RAM AM_BASE( &c64_colorram)

	AM_RANGE(0x20000, 0x23fff) AM_ROM /* &c65_dos,     maps to 0x8000    */
	AM_RANGE(0x24000, 0x28fff) AM_ROM /* reserved */
	AM_RANGE(0x29000, 0x29fff) AM_ROM AM_BASE( &c65_chargen)
	AM_RANGE(0x2a000, 0x2bfff) AM_ROM AM_BASE( &c64_basic)
	AM_RANGE(0x2c000, 0x2cfff) AM_ROM AM_BASE( &c65_interface)
	AM_RANGE(0x2d000, 0x2dfff) AM_ROM AM_BASE( &c64_chargen)
	AM_RANGE(0x2e000, 0x2ffff) AM_ROM AM_BASE( &c64_kernal)

	AM_RANGE(0x30000, 0x31fff) AM_ROM /*&c65_monitor,     monitor maps to 0x6000    */
	AM_RANGE(0x32000, 0x37fff) AM_ROM /*&c65_basic, */
	AM_RANGE(0x38000, 0x3bfff) AM_ROM /*&c65_graphics, */
	AM_RANGE(0x3c000, 0x3dfff) AM_ROM /* reserved */
	AM_RANGE(0x3e000, 0x3ffff) AM_ROM /* &c65_kernal, */

	AM_RANGE(0x40000, 0x7ffff) AM_NOP
	/* 8 megabyte full address space! */
ADDRESS_MAP_END


/*************************************
 *
 *  Input Ports
 *
 *************************************/

static INPUT_PORTS_START( c65 )
	PORT_INCLUDE( common_cbm_keyboard )		/* ROW0 -> ROW7 */

	PORT_START("FUNCT")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("ESC") PORT_CODE(KEYCODE_F1)		
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("F13 F14") PORT_CODE(KEYCODE_F11)	
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("F11 F12") PORT_CODE(KEYCODE_F10)	
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("F9 F10") PORT_CODE(KEYCODE_F9)	
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("HELP") PORT_CODE(KEYCODE_F12)		
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("ALT") PORT_CODE(KEYCODE_F2)		/* non blocking */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("TAB") PORT_CODE(KEYCODE_TAB)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("NO SCRL") PORT_CODE(KEYCODE_F4)

	PORT_INCLUDE( c65_special )				/* SPECIAL */

	PORT_INCLUDE( c64_controls )			/* CTRLSEL, JOY0, JOY1, PADDLE0 -> PADDLE3, TRACKX, TRACKY, LIGHTX, LIGHTY, OTHER */
INPUT_PORTS_END


static INPUT_PORTS_START( c65ger )
	PORT_INCLUDE( c65 )
    
	PORT_MODIFY( "ROW1" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Z  { Y }") PORT_CODE(KEYCODE_Z)					PORT_CHAR('Z')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("3  #  { 3  Paragraph }") PORT_CODE(KEYCODE_3)		PORT_CHAR('3') PORT_CHAR('#')

	PORT_MODIFY( "ROW3" )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Y  { Z }") PORT_CODE(KEYCODE_Y)					PORT_CHAR('Y')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("7 ' { 7  / }") PORT_CODE(KEYCODE_7)				PORT_CHAR('7') PORT_CHAR('\'')

	PORT_MODIFY( "ROW4" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("0  { = }") PORT_CODE(KEYCODE_0)					PORT_CHAR('0')

	PORT_MODIFY( "ROW5" )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME(",  <  { ; }") PORT_CODE(KEYCODE_COMMA)			PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Paragraph  \xE2\x86\x91  { \xc3\xbc }") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR(0x00A7) PORT_CHAR(0x2191)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME(":  [  { \xc3\xa4 }") PORT_CODE(KEYCODE_COLON)		PORT_CHAR(':') PORT_CHAR('[')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME(".  >  { : }") PORT_CODE(KEYCODE_STOP)				PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("-  { '  ` }") PORT_CODE(KEYCODE_EQUALS)			PORT_CHAR('-')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("+  { \xc3\x9f ? }") PORT_CODE(KEYCODE_MINUS)		PORT_CHAR('+')

	PORT_MODIFY( "ROW6" )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("/  ?  { -  _ }") PORT_CODE(KEYCODE_SLASH)			PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Sum Pi  { ]  \\ }") PORT_CODE(KEYCODE_DEL)		PORT_CHAR(0x03A3) PORT_CHAR(0x03C0)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("=  { #  ' }") PORT_CODE(KEYCODE_BACKSLASH)		PORT_CHAR('=')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME(";  ]  { \xc3\xb6 }") PORT_CODE(KEYCODE_QUOTE)		PORT_CHAR(';') PORT_CHAR(']')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("*  `  { +  * }") PORT_CODE(KEYCODE_CLOSEBRACE)	PORT_CHAR('*') PORT_CHAR('`')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\\  { [ \xE2\x86\x91 }") PORT_CODE(KEYCODE_BACKSLASH2) PORT_CHAR('\xa3')

	PORT_MODIFY( "ROW7" )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("_  { <  > }") PORT_CODE(KEYCODE_TILDE)			PORT_CHAR('_')

	PORT_MODIFY("SPECIAL") /* special keys */
	PORT_DIPNAME( 0x20, 0x00, "(C65) DIN ASC (switch)") PORT_CODE(KEYCODE_F3)
	PORT_DIPSETTING(	0x00, "ASC" )
	PORT_DIPSETTING(	0x20, "DIN" )
INPUT_PORTS_END



/*************************************
 *
 *  Graphics definitions
 *
 *************************************/


static PALETTE_INIT( c65 )
{
	int i;

	for ( i = 0; i < sizeof(vic3_palette) / 3; i++ ) {
		palette_set_color_rgb(machine, i, vic3_palette[i*3], vic3_palette[i*3+1], vic3_palette[i*3+2]);
	}
}


/*************************************
 *
 *  Sound definitions
 *
 *************************************/


static const sid6581_interface c65_sound_interface =
{
	c64_paddle_read
};



/*************************************
 *
 *  Machine driver
 *
 *************************************/


static MACHINE_DRIVER_START( c65 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", M4510, 3500000)  /* or VIC6567_CLOCK, */
	MDRV_CPU_PROGRAM_MAP(c65_mem)
	MDRV_CPU_VBLANK_INT("screen", c65_frame_interrupt)
	MDRV_CPU_PERIODIC_INT(vic3_raster_irq, VIC6567_HRETRACERATE)

	MDRV_MACHINE_START( c65 )

	/* video hardware */
	MDRV_IMPORT_FROM( vh_vic2 )
	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_REFRESH_RATE(VIC6567_VRETRACERATE)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_SIZE(525 * 2, 520 * 2)
	MDRV_SCREEN_VISIBLE_AREA(VIC6567_STARTVISIBLECOLUMNS ,(VIC6567_STARTVISIBLECOLUMNS + VIC6567_VISIBLECOLUMNS - 1) * 2, VIC6567_STARTVISIBLELINES, VIC6567_STARTVISIBLELINES + VIC6567_VISIBLELINES - 1)
	MDRV_PALETTE_LENGTH(sizeof(vic3_palette) / sizeof(vic3_palette[0]) / 3)
	MDRV_PALETTE_INIT( c65 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")
	MDRV_SOUND_ADD("sid_r", SID8580, 985248)
	MDRV_SOUND_CONFIG(c65_sound_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "rspeaker", 0.50)
	MDRV_SOUND_ADD("sid_l", SID8580, 985248)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "lspeaker", 0.50)

	/* quickload */
	MDRV_QUICKLOAD_ADD("quickload", cbm_c65, "p00,prg", CBM_QUICKLOAD_DELAY_SECONDS)

	/* cia */
	MDRV_CIA6526_ADD("cia_0", CIA6526R1, 3500000, c65_ntsc_cia0)
	MDRV_CIA6526_ADD("cia_1", CIA6526R1, 3500000, c65_ntsc_cia1)
	
	/* floppy from serial bus */
	MDRV_IMPORT_FROM(simulated_drive)

	MDRV_IMPORT_FROM(c64_cartslot)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( c65pal )
	MDRV_IMPORT_FROM( c65 )
	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_REFRESH_RATE(VIC6569_VRETRACERATE)
	MDRV_SCREEN_SIZE(625 * 2, 520 * 2)
	MDRV_SCREEN_VISIBLE_AREA(VIC6569_STARTVISIBLECOLUMNS, (VIC6569_STARTVISIBLECOLUMNS + VIC6569_VISIBLECOLUMNS - 1) * 2, VIC6569_STARTVISIBLELINES, VIC6569_STARTVISIBLELINES + VIC6569_VISIBLELINES - 1)

	/* sound hardware */
	MDRV_SOUND_REPLACE("sid_r", SID8580, 1022727)
	MDRV_SOUND_CONFIG(c65_sound_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "rspeaker", 0.50)
	MDRV_SOUND_REPLACE("sid_l", SID8580, 1022727)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "lspeaker", 0.50)

	/* cia */
	MDRV_DEVICE_REMOVE("cia_0")
	MDRV_DEVICE_REMOVE("cia_1")
	MDRV_CIA6526_ADD("cia_0", CIA6526R1, 3500000, c65_pal_cia0)
	MDRV_CIA6526_ADD("cia_1", CIA6526R1, 3500000, c65_pal_cia1)
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definition(s)
 *
 *************************************/


ROM_START( c65 )
	ROM_REGION( 0x400000, "maincpu", 0 )
	ROM_SYSTEM_BIOS( 0, "910111", "V0.9.910111" )
	ROMX_LOAD( "910111.bin", 0x20000, 0x20000, CRC(c5d8d32e) SHA1(71c05f098eff29d306b0170e2c1cdeadb1a5f206), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "910523", "V0.9.910523" )
	ROMX_LOAD( "910523.bin", 0x20000, 0x20000, CRC(e8235dd4) SHA1(e453a8e7e5b95de65a70952e9d48012191e1b3e7), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "910626", "V0.9.910626" )
	ROMX_LOAD( "910626.bin", 0x20000, 0x20000, CRC(12527742) SHA1(07c185b3bc58410183422f7ac13a37ddd330881b), ROM_BIOS(3) )
	ROM_SYSTEM_BIOS( 3, "910828", "V0.9.910828" )
	ROMX_LOAD( "910828.bin", 0x20000, 0x20000, CRC(3ee40b06) SHA1(b63d970727a2b8da72a0a8e234f3c30a20cbcb26), ROM_BIOS(4) )
	ROM_SYSTEM_BIOS( 4, "911001", "V0.9.911001" )
	ROMX_LOAD( "911001.bin", 0x20000, 0x20000, CRC(0888b50f) SHA1(129b9a2611edaebaa028ac3e3f444927c8b1fc5d), ROM_BIOS(5) )
ROM_END

ROM_START( c64dx )
	ROM_REGION( 0x400000, "maincpu", 0 )
	ROM_LOAD( "910429.bin", 0x20000, 0x20000, CRC(b025805c) SHA1(c3b05665684f74adbe33052a2d10170a1063ee7d) )
ROM_END


/*************************************
 *
 *  System configuration(s)
 *
 *************************************/


static SYSTEM_CONFIG_START( c65 )
	// to investigate which carts could work in the c65 expansion port!
	CONFIG_DEVICE(cbmfloppy_device_getinfo)
	CONFIG_RAM_DEFAULT(128 * 1024)
	CONFIG_RAM((128 + 512) * 1024)
	CONFIG_RAM((128 + 4096) * 1024)
SYSTEM_CONFIG_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME    PARENT  COMPAT  MACHINE INPUT   INIT    CONFIG  COMPANY                         FULLNAME                                              FLAGS */

COMP( 1991, c65,    0,      0,      c65,    c65,    c65,    c65,    "Commodore Electronics, Ltd.",  "Commodore 65 Development System (Prototype, NTSC)", GAME_NOT_WORKING )
COMP( 1991, c64dx,  c65,    0,      c65pal, c65ger, c65pal, c65,    "Commodore Electronics, Ltd.",  "Commodore 64DX Development System (Prototype, PAL, German)", GAME_NOT_WORKING )
