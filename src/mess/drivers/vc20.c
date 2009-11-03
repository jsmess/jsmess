/***************************************************************************

    commodore vic20 home computer
    PeT mess@utanet.at

    documentation
     Marko.Makela@HUT.FI (vic6560)
     www.funet.fi

***************************************************************************/

/*

2008 - Driver Updates
---------------------

(most of the informations are taken from http://www.zimmers.net/cbmpics/ )


[CBM systems which belong to this driver]


* VIC-1001 (1981, Japan)

  The first model released was the Japanese one. It featured support for the
Japanese katakana character.

CPU: MOS Technology 6502 (1.01 MHz)
RAM: 5 kilobytes (Expanded to 21k though an external 16k unit)
ROM: 20 kilobytes
Video: MOS Technology 6560 "VIC"(Text: 22 columns, 23 rows; Hires: 176x184
pixels bitmapped; 8 text colors, 16 background colors)
Sound: MOS Technology 6560 "VIC" (3 voices -square wave-, noise and volume)
Ports: 6522 VIA x2 (1 Joystick/Mouse port; CBM Serial port; 'Cartridge /
    Game / Expansion' port; CBM Monitor port; CBM 'USER' port; Power and
    reset switches; Power connector)
Keyboard: Full-sized QWERTY 66 key (8 programmable function keys; 2 sets of
    Keyboardable graphic characters; 2 key direction cursor-pad)


* VIC 20 (1981)

  This system was the first computer to sell more than one million units
worldwide. It was sold both in Europe and in the US. In Germany the
computer was renamed as VC 20 (apparently, it stands for 'VolksComputer'

CPU: MOS Technology 6502A (1.01 MHz)
RAM: 5 kilobytes (Expanded to 32k)
ROM: 20 kilobytes
Video: MOS Technology 6560 "VIC"(Text: 22 columns, 23 rows; Hires: 176x184
pixels bitmapped; 8 text colors, 16 background colors)
Sound: MOS Technology 6560 "VIC" (3 voices -square wave-, noise and volume)
Ports: 6522 VIA x2 (1 Joystick/Mouse port; CBM Serial port; 'Cartridge /
    Game / Expansion' port; CBM Monitor port; CBM 'USER' port; Power and
    reset switches; Power connector)
Keyboard: Full-sized QWERTY 66 key (8 programmable function keys; 2 sets of
    Keyboardable graphic characters; 2 key direction cursor-pad)


* VIC 21 (1983)

  It consists of a VIC 20 with built-in RAM expansion, to reach a RAM
  capability of 21 kilobytes.


* VIC 20CR

  CR stands for Cost Reduced, as it consisted of a board with only 2 (larger)
block of RAM instead of 8.


[TO DO]


* Imperfect sound

* Timer system only 98% accurate

* No support for printer or other devices; no userport; no rs232/v.24
interface; no special expansion modules like ieee488 interface

*/


#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "sound/dac.h"

#include "machine/6522via.h"
#include "machine/cbmipt.h"
#include "video/vic6560.h"

/* devices config */
#include "includes/cbm.h"
#include "includes/cbmserb.h"	// needed for MDRV_DEVICE_REMOVE
#include "includes/cbmdrive.h"
#include "includes/vc1541.h"
#include "includes/cbmieeeb.h"
#include "devices/messram.h"
#include "includes/vc20.h"


/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/


static ADDRESS_MAP_START( vc20_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x03ff) AM_RAM
	AM_RANGE(0x0400, 0x0fff) AM_ROMBANK(1) AM_WRITE( vc20_0400_w )	/* ram, rom or nothing */
	AM_RANGE(0x1000, 0x1fff) AM_RAM
	AM_RANGE(0x2000, 0x3fff) AM_ROMBANK(2) AM_WRITE( vc20_2000_w )	/* ram, rom or nothing */
	AM_RANGE(0x4000, 0x5fff) AM_ROMBANK(3) AM_WRITE( vc20_4000_w )	/* ram, rom or nothing */
	AM_RANGE(0x6000, 0x7fff) AM_ROMBANK(4) AM_WRITE( vc20_6000_w )	/* ram, rom or nothing */
	AM_RANGE(0x8000, 0x8fff) AM_ROM
	AM_RANGE(0x9000, 0x900f) AM_READWRITE( vic6560_port_r, vic6560_port_w )
	AM_RANGE(0x9010, 0x910f) AM_NOP
	AM_RANGE(0x9110, 0x911f) AM_DEVREADWRITE("via6522_0", via_r, via_w)
	AM_RANGE(0x9120, 0x912f) AM_DEVREADWRITE("via6522_1", via_r, via_w)
	AM_RANGE(0x9130, 0x93ff) AM_NOP
	AM_RANGE(0x9400, 0x97ff) AM_READWRITE(SMH_RAM, vc20_write_9400) AM_BASE(&vc20_memory_9400)	/*color ram 4 bit */
	AM_RANGE(0x9800, 0x9fff) AM_RAM
	AM_RANGE(0xa000, 0xbfff) AM_ROMBANK(5)
	AM_RANGE(0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( vc20i_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x03ff) AM_RAM
	AM_RANGE(0x0400, 0x0fff) AM_ROMBANK(1) AM_WRITE( vc20_0400_w )	/* ram, rom or nothing */
	AM_RANGE(0x1000, 0x1fff) AM_RAM
	AM_RANGE(0x2000, 0x3fff) AM_ROMBANK(2) AM_WRITE( vc20_2000_w )	/* ram, rom or nothing */
	AM_RANGE(0x4000, 0x5fff) AM_ROMBANK(3) AM_WRITE( vc20_4000_w )	/* ram, rom or nothing */
	AM_RANGE(0x6000, 0x7fff) AM_ROMBANK(4) AM_WRITE( vc20_6000_w )	/* ram, rom or nothing */
	AM_RANGE(0x8000, 0x8fff) AM_ROM
	AM_RANGE(0x9000, 0x900f) AM_READWRITE( vic6560_port_r, vic6560_port_w )
	AM_RANGE(0x9010, 0x910f) AM_NOP
	AM_RANGE(0x9110, 0x911f) AM_DEVREADWRITE("via6522_0", via_r, via_w)
	AM_RANGE(0x9120, 0x912f) AM_DEVREADWRITE("via6522_1", via_r, via_w)
	AM_RANGE(0x9400, 0x97ff) AM_READWRITE( SMH_RAM, vc20_write_9400) AM_BASE(&vc20_memory_9400)	/* color ram 4 bit */
	AM_RANGE(0x9800, 0x980f) AM_DEVREADWRITE("via6522_2", via_r, via_w)
	AM_RANGE(0x9810, 0x981f) AM_DEVREADWRITE("via6522_3", via_r, via_w)
	AM_RANGE(0xa000, 0xbfff) AM_ROMBANK(5)
	AM_RANGE(0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END


/*************************************
 *
 *  Input Ports
 *
 *************************************/

/* 2008-05 FP: These lightpen input ports, would better
fit machine/cbmipt.c but PORT_MINMAX requires vic6560.c
constants. Therefore, they will stay here, atm    */

static INPUT_PORTS_START( vic_lightpen_6560 )
	PORT_START( "LIGHTX" )
	PORT_BIT( 0xff, 0, IPT_PADDLE ) PORT_NAME("Lightpen X Axis") PORT_CROSSHAIR(X, 1.0, 0.0, 0) PORT_SENSITIVITY(30) PORT_KEYDELTA(2) PORT_MINMAX(0,(VIC6560_MAME_XSIZE - 1)) PORT_CATEGORY(12) PORT_CODE_DEC(KEYCODE_LEFT) PORT_CODE_INC(KEYCODE_RIGHT) PORT_CODE_DEC(JOYCODE_X_LEFT_SWITCH) PORT_CODE_INC(JOYCODE_X_RIGHT_SWITCH)

	PORT_START( "LIGHTY" )
	PORT_BIT( 0xff, 0, IPT_PADDLE ) PORT_NAME("Lightpen Y Axis") PORT_CROSSHAIR(Y, 1.0, 0.0, 0) PORT_SENSITIVITY(30) PORT_KEYDELTA(2) PORT_MINMAX(0,(VIC6560_MAME_YSIZE - 1)) PORT_CATEGORY(12) PORT_CODE_DEC(KEYCODE_UP) PORT_CODE_INC(KEYCODE_DOWN) PORT_CODE_DEC(JOYCODE_Y_UP_SWITCH) PORT_CODE_INC(JOYCODE_Y_DOWN_SWITCH)
INPUT_PORTS_END


static INPUT_PORTS_START( vic_lightpen_6561 )
	PORT_START( "LIGHTX" )
	PORT_BIT( 0xff, 0, IPT_PADDLE ) PORT_NAME("Lightpen X Axis") PORT_CROSSHAIR(X, 1.0, 0.0, 0) PORT_SENSITIVITY(30) PORT_KEYDELTA(2) PORT_MINMAX(0,(VIC6560_MAME_XSIZE - 1)) PORT_CATEGORY(12) PORT_CODE_DEC(KEYCODE_LEFT) PORT_CODE_INC(KEYCODE_RIGHT) PORT_CODE_DEC(JOYCODE_X_LEFT_SWITCH) PORT_CODE_INC(JOYCODE_X_RIGHT_SWITCH)

	PORT_START( "LIGHTY" )
	PORT_BIT( 0x1ff, 0, IPT_PADDLE ) PORT_NAME("Lightpen Y Axis") PORT_CROSSHAIR(Y, 1.0, 0.0, 0) PORT_SENSITIVITY(30) PORT_KEYDELTA(2) PORT_MINMAX(0,(VIC6561_MAME_YSIZE - 1)) PORT_CATEGORY(12) PORT_CODE_DEC(KEYCODE_UP) PORT_CODE_INC(KEYCODE_DOWN) PORT_CODE_DEC(JOYCODE_Y_UP_SWITCH) PORT_CODE_INC(JOYCODE_Y_DOWN_SWITCH)
INPUT_PORTS_END


static INPUT_PORTS_START( vic20 )
	PORT_INCLUDE( vic_keyboard )		/* ROW0 -> ROW7 */

	PORT_INCLUDE( vic_special )			/* SPECIAL */

	PORT_INCLUDE( vic_controls )		/* JOY, PADDLE0, PADDLE1 */

	/* Lightpen 6560 */
	PORT_INCLUDE( vic_lightpen_6560 )	/* LIGHTX, LIGHTY */

	PORT_INCLUDE( vic_expansion )		/* EXP */
INPUT_PORTS_END


static INPUT_PORTS_START( vic1001 )
	PORT_INCLUDE( vic20 )

	PORT_MODIFY( "ROW0" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('\xA5')
INPUT_PORTS_END


static INPUT_PORTS_START( vc20 )
	PORT_INCLUDE( vic_keyboard )		/* ROW0 -> ROW7 */

	PORT_INCLUDE( vic_special )			/* SPECIAL */

	PORT_INCLUDE( vic_controls )		/* JOY, PADDLE0, PADDLE1 */

	/* Lightpen 6561 */
	PORT_INCLUDE( vic_lightpen_6561 )	/* LIGHTX, LIGHTY */

	PORT_INCLUDE( vic_expansion )		/* EXP */
INPUT_PORTS_END


static INPUT_PORTS_START( vic20swe )
	PORT_INCLUDE( vc20 )

	PORT_MODIFY( "ROW0" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH2)		PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS)			PORT_CHAR('-')
	PORT_MODIFY( "ROW1" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE)		PORT_CHAR('@')
	PORT_MODIFY( "ROW2" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE)			PORT_CHAR(0x00C4)
	PORT_MODIFY( "ROW5" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH)		PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON)			PORT_CHAR(0x00D6)
	PORT_MODIFY( "ROW6" )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE)		PORT_CHAR(0x00C5)
	PORT_MODIFY( "ROW7" )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS)			PORT_CHAR('=')
INPUT_PORTS_END


/*************************************
 *
 *  Graphics definitions
 *
 *************************************/


static PALETTE_INIT( vc20 )
{
	int i;

	for ( i = 0; i < sizeof(vic6560_palette) / 3; i++ ) {
		palette_set_color_rgb(machine, i, vic6560_palette[i*3], vic6560_palette[i*3+1], vic6560_palette[i*3+2]);
	}
}



/*************************************
 *
 *  Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( vic20 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", M6502, VIC6560_CLOCK)        /* 7.8336 MHz */
	MDRV_CPU_PROGRAM_MAP(vc20_mem)
	MDRV_CPU_VBLANK_INT("screen", vic20_frame_interrupt)
	MDRV_CPU_PERIODIC_INT(vic656x_raster_interrupt, VIC656X_HRETRACERATE)

	MDRV_MACHINE_RESET( vic20 )

    /* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(VIC6560_VRETRACERATE)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE((VIC6560_XSIZE + 7) & ~7, VIC6560_YSIZE)
	MDRV_SCREEN_VISIBLE_AREA(VIC6560_MAME_XPOS, VIC6560_MAME_XPOS + VIC6560_MAME_XSIZE - 1, VIC6560_MAME_YPOS, VIC6560_MAME_YPOS + VIC6560_MAME_YSIZE - 1)
	MDRV_PALETTE_LENGTH(ARRAY_LENGTH(vic6560_palette) / 3)
	MDRV_PALETTE_INIT( vc20 )

	MDRV_VIDEO_START( vic6560 )
	MDRV_VIDEO_UPDATE( vic6560 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("custom", VIC6560, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD("dac", DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* devices */
	MDRV_QUICKLOAD_ADD("quickload", cbm_vc20, "p00,prg", CBM_QUICKLOAD_DELAY_SECONDS)

	/* cassette */
	MDRV_CASSETTE_ADD( "cassette", cbm_cassette_config )

	/* floppy from serial bus */
	MDRV_IMPORT_FROM(simulated_drive)

	/* IEEE bus */
	MDRV_CBM_IEEEBUS_ADD("ieee_bus")

	/* via */
	MDRV_VIA6522_ADD("via6522_0", 0, vc20_via0)
	MDRV_VIA6522_ADD("via6522_1", 0, vc20_via1)
	MDRV_VIA6522_ADD("via6522_2", 0, vc20_via2)
	MDRV_VIA6522_ADD("via6522_3", 0, vc20_via3)

	MDRV_IMPORT_FROM(vic20_cartslot)
	
	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("5K")
	MDRV_RAM_EXTRA_OPTIONS("8K,16K,24K,32K")	
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( vic20v )
	MDRV_IMPORT_FROM( vic20 )

	MDRV_DEVICE_REMOVE("serial_bus")	// in the current code, serial bus device is tied to the floppy drive
	MDRV_IMPORT_FROM( cpu_vc1540 )
#ifdef CPU_SYNC
	MDRV_QUANTUM_TIME(HZ(60))
#else
	MDRV_QUANTUM_TIME(HZ(180000))
#endif
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( vic20i )
	MDRV_IMPORT_FROM( vic20 )
	/* MDRV_IMPORT_FROM( cpu_c2031 ) */ // not emulated yet, we only include the firmware below
#if 1 || CPU_SYNC
	MDRV_QUANTUM_TIME(HZ(60))
#else
	MDRV_QUANTUM_TIME(HZ(180000))
#endif

	MDRV_CPU_MODIFY( "maincpu" )
	MDRV_CPU_PROGRAM_MAP(vc20i_mem)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( vc20 )
	MDRV_IMPORT_FROM( vic20 )

	MDRV_CPU_REPLACE( "maincpu", M6502, VIC6561_CLOCK )
	MDRV_CPU_PROGRAM_MAP(vc20i_mem)

	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_REFRESH_RATE(VIC6561_VRETRACERATE)
	MDRV_SCREEN_SIZE((VIC6561_XSIZE + 7) & ~7, VIC6561_YSIZE)
	MDRV_SCREEN_VISIBLE_AREA(VIC6561_MAME_XPOS, VIC6561_MAME_XPOS + VIC6561_MAME_XSIZE - 1, VIC6561_MAME_YPOS, VIC6561_MAME_YPOS + VIC6561_MAME_YSIZE - 1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( vc20v )
	MDRV_IMPORT_FROM( vc20 )

	MDRV_DEVICE_REMOVE("serial_bus")	// in the current code, serial bus device is tied to the floppy drive
	MDRV_IMPORT_FROM( cpu_vc1540 )
#ifdef CPU_SYNC
	MDRV_QUANTUM_TIME(HZ(60))
#else
	MDRV_QUANTUM_TIME(HZ(180000))
#endif
MACHINE_DRIVER_END


/*************************************
 *
 *  ROM definition(s)
 *
 *************************************/


ROM_START( vic1001 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_FILL( 0x0000, 0x8000, 0xff )
	ROM_LOAD( "901460.02", 0x8000, 0x1000, CRC(fcfd8a4b) SHA1(dae61ac03065aa2904af5c123ce821855898c555) )
	ROM_FILL( 0x9000, 0x3000, 0xff )
	ROM_LOAD( "901486.01", 0xc000, 0x2000, CRC(db4c43c1) SHA1(587d1e90950675ab6b12d91248a3f0d640d02e8d) )
	ROM_LOAD( "901486.02", 0xe000, 0x2000, CRC(336900d7) SHA1(c9ead45e6674d1042ca6199160e8583c23aeac22) )
ROM_END


ROM_START( vic20 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_FILL( 0x0000, 0x8000, 0xff )
	ROM_LOAD( "901460.03", 0x8000, 0x1000, CRC(83e032a6) SHA1(4fd85ab6647ee2ac7ba40f729323f2472d35b9b4) )
	ROM_FILL( 0x9000, 0x3000, 0xff )
	ROM_LOAD( "901486.01", 0xc000, 0x2000, CRC(db4c43c1) SHA1(587d1e90950675ab6b12d91248a3f0d640d02e8d) )
	ROM_LOAD( "901486.06", 0xe000, 0x2000, CRC(e5e7c174) SHA1(06de7ec017a5e78bd6746d89c2ecebb646efeb19) )
ROM_END

ROM_START( vic20pal )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_FILL( 0x0000, 0x8000, 0xff )
	ROM_LOAD( "901460.03", 0x8000, 0x1000, CRC(83e032a6) SHA1(4fd85ab6647ee2ac7ba40f729323f2472d35b9b4) )
	ROM_FILL( 0x9000, 0x3000, 0xff )
	ROM_LOAD( "901486.01", 0xc000, 0x2000, CRC(db4c43c1) SHA1(587d1e90950675ab6b12d91248a3f0d640d02e8d) )
	ROM_LOAD( "901486.07", 0xe000, 0x2000, CRC(4be07cb4) SHA1(ce0137ed69f003a299f43538fa9eee27898e621e) )
ROM_END

#define rom_vic20cr		rom_vic20
#define rom_vic20crp	rom_vic20pal

#define rom_vc20		rom_vic20pal

ROM_START( vic20swe )
	/* patched pal system for swedish/finish keyboard and chars */
	/* but in rom? (maybe patched means in this case nec version) */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_FILL( 0x0000, 0x8000, 0xff )
	ROM_LOAD( "nec22101.207", 0x8000, 0x1000, CRC(d808551d) SHA1(f403f0b0ce5922bd61bbd768bdd6f0b38e648c9f) )
	ROM_FILL( 0x9000, 0x3000, 0xff )
	ROM_LOAD( "901486.01", 0xc000, 0x2000, CRC(db4c43c1) SHA1(587d1e90950675ab6b12d91248a3f0d640d02e8d) )
	ROM_LOAD( "nec22081.206", 0xe000, 0x2000, CRC(b2a60662) SHA1(cb3e2f6e661ea7f567977751846ce9ad524651a3) )
ROM_END


ROM_START( vic20i )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_FILL( 0x0000, 0x8000, 0xff )
	ROM_LOAD( "901460.03", 0x8000, 0x1000, CRC(83e032a6) SHA1(4fd85ab6647ee2ac7ba40f729323f2472d35b9b4) )
	ROM_FILL( 0x9000, 0x2000, 0xff )
	ROM_LOAD( "325329.04", 0xb000, 0x800, CRC(d37b6335) SHA1(828c965829d21c60e8c2d083caee045c639a270f) )
	ROM_LOAD( "901486.01", 0xc000, 0x2000, CRC(db4c43c1) SHA1(587d1e90950675ab6b12d91248a3f0d640d02e8d) )
	ROM_LOAD( "901486.06", 0xe000, 0x2000, CRC(e5e7c174) SHA1(06de7ec017a5e78bd6746d89c2ecebb646efeb19) )

	C2031_ROM("cpu_vc1540")	// this is not currently emulated, but we document here the firmware
ROM_END

ROM_START( vic20v )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_FILL( 0x0000, 0x8000, 0xff )
	ROM_LOAD( "901460.03", 0x8000, 0x1000, CRC(83e032a6) SHA1(4fd85ab6647ee2ac7ba40f729323f2472d35b9b4) )
	ROM_FILL( 0x9000, 0x3000, 0xff )
	ROM_LOAD( "901486.01", 0xc000, 0x2000, CRC(db4c43c1) SHA1(587d1e90950675ab6b12d91248a3f0d640d02e8d) )
	ROM_LOAD( "901486.06", 0xe000, 0x2000, CRC(e5e7c174) SHA1(06de7ec017a5e78bd6746d89c2ecebb646efeb19) )

	VC1540_ROM("cpu_vc1540")
ROM_END

ROM_START( vic20plv )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_FILL( 0x0000, 0x8000, 0xff )
	ROM_LOAD( "901460.03", 0x8000, 0x1000, CRC(83e032a6) SHA1(4fd85ab6647ee2ac7ba40f729323f2472d35b9b4) )
	ROM_FILL( 0x9000, 0x3000, 0xff )
	ROM_LOAD( "901486.01", 0xc000, 0x2000, CRC(db4c43c1) SHA1(587d1e90950675ab6b12d91248a3f0d640d02e8d) )
	ROM_LOAD( "901486.07", 0xe000, 0x2000, CRC(4be07cb4) SHA1(ce0137ed69f003a299f43538fa9eee27898e621e) )

	VC1540_ROM("cpu_vc1540")
ROM_END

#define rom_vc20v		rom_vic20plv


/*************************************
 *
 *  System configuration(s)
 *
 *************************************/


static SYSTEM_CONFIG_START(vic20)
	CONFIG_DEVICE(cbmfloppy_device_getinfo)

SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(vic20v)
	CONFIG_DEVICE(vc1541_device_getinfo)
SYSTEM_CONFIG_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME      PARENT COMPAT MACHINE INPUT    INIT     CONFIG     COMPANY                             FULLNAME            FLAGS */

COMP( 1981, vic1001,   vic20,  0,  vic20,   vic1001,  vic20,  vic20,     "Commodore Business Machines Co.",  "VIC-1001 (NTSC, Japan)", GAME_IMPERFECT_SOUND)

COMP( 1981, vic20,     0,      0,   vic20,  vic20,    vic20,  vic20,     "Commodore Business Machines Co.",  "VIC 20 (NTSC)", GAME_IMPERFECT_SOUND)
COMP( 1981, vic20cr,   vic20,  0,   vic20,  vic20,    vic20,  vic20,     "Commodore Business Machines Co.",  "VIC 20CR (NTSC)", GAME_IMPERFECT_SOUND)
COMP( 1981, vic20i,    vic20,  0,   vic20i, vic20,    vic20i, vic20,     "Commodore Business Machines Co.",  "VIC 20 (NTSC, IEEE488 Interface - SYS45065)", GAME_IMPERFECT_SOUND)
COMP( 1981, vic20v,    vic20,  0,   vic20v, vic20,    vic20v, vic20v,    "Commodore Business Machines Co.",  "VIC 20 (NTSC, VC1540)", GAME_IMPERFECT_SOUND | GAME_NOT_WORKING )

COMP( 1981, vic20pal,  vic20,  0,   vc20,   vc20,     vc20,   vic20,     "Commodore Business Machines Co.",  "VC 20 (PAL)", GAME_IMPERFECT_SOUND)
COMP( 1981, vic20crp,  vic20,  0,   vc20,   vc20,     vc20,   vic20,     "Commodore Business Machines Co.",  "VC 20CR (PAL)", GAME_IMPERFECT_SOUND)
COMP( 1981, vic20plv,  vic20,  0,   vc20v,  vic20,    vc20v,  vic20v,    "Commodore Business Machines Co.",  "VC 20 (PAL, VC1540)", GAME_IMPERFECT_SOUND | GAME_NOT_WORKING )
COMP( 1981, vc20,      vic20,  0,   vc20,   vc20,     vc20,   vic20,     "Commodore Business Machines Co.",  "VIC 20 (PAL, Germany)", GAME_IMPERFECT_SOUND)
COMP( 1981, vc20v,     vic20,  0,   vc20v,  vic20,    vc20v,  vic20v,    "Commodore Business Machines Co.",  "VIC 20 (PAL, Germany, VC1540)", GAME_IMPERFECT_SOUND | GAME_NOT_WORKING )

COMP( 1981, vic20swe,  vic20,  0,   vc20,   vic20swe, vc20,   vic20,     "Commodore Business Machines Co.",  "VIC 20 (PAL, Swedish Expansion Kit)", GAME_IMPERFECT_SOUND)
