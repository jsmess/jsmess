/***************************************************************************
    commodore c64 home computer

    PeT mess@utanet.at

    documentation
     www.funet.fi
***************************************************************************/

/*

2008 - Driver Updates 
---------------------

(most of the informations are taken from http://www.zimmers.net/cbmpics/ )


[CBM systems which belong to this driver]

* Commodore Max Machine (1982, Japan)

  The Max Machine was produced by Commodore Japan and was released along 
with the C64 in Japan. Also known as "UltiMax" (in North America ads) 
or "VC-10 / VIC-10" (in some European advertising), it was designed to be a 
game console. It has no internal kernel and no BASIC, everything must be 
supplied by the cart. A Mini BASIC and a BASIC carts were produced (the 
latter with support for LOAD/SAVE commands as well). The character rom 
is not present either and its content was, again, in the cart. Max Machine
carts can be run by both C64 and C128.

CPU: MOS Technology 6510 (1 MHz)
RAM: 4 kilobytes (2 kilobytes?)
ROM: none
Video: MOS 6566 "VIC-II" (320 x 200 Hi-Resolution, 40 columns text, 16 
	colors)
Sound: MOS 6581 "SID" (3 voice stereo synthesizer/digital sound 
	capabilities) 
Ports: 6526 CIA (Power switch; 2 Commodore "Joystick" ports; 'EXPANSION' 
	port; "TV" port; CBM Datasette port)
Keyboard: Full-sized QWERTY "Contact-Pad"


* Commodore 64 (1982)

  Released in August 1982 in the US and marketed as a lower end home 
computer, the C64 dominated the home computer market for a few years,
outselling competitors like Apple computers, IBM PCs and TRS-80s.
During the early stages of the project it was called VIC-40, but the 
named was changed in the end probably to fit the name scheme used at
time for business product (letter + memory size: P128 & B256).
It was also advertised as VC 64 in Germany, and as VIC 64S in Sweden
(the latter came with a modified character set)

CPU: CSG 6510 (1 MHz)
RAM: 64 kilobytes
ROM: 20 kilobytes
Video: MOS 6569 "VIC-II" (320 x 200 Hi-Resolution, 40 columns text, 16 
	colors)
Sound: MOS 6581 "SID" (3 voice stereo synthesizer/digital sound 
	capabilities) 
Ports: MOS 6526 CIA x2 (Power switch; 2 Commodore Joystick/Mouse ports; 
	CBM Serial port; CBM Datasette port; parallel programmable "User" 
	port; CBM Monitor port; C64 expansion port)
Keyboard: Full-sized 62 key QWERTY (8 programmable function keys; 2
	cursor keys, 2 directions each)

BIOS:
	kernal.901227-02.bin
	basic.901226-01.bin
	characters.901225-01.bin


* Commodore 64 (1982, Japan)

  Released in Japan at the same time as Max Machine, in the hope to repeat 
the success of the VIC-1001, this system was a flop. Technically it is the 
same as the C64, but it has native support for the Katakana character set.

Video: MOS 6569 "VIC-II" (320 x 200 Hi-Resolution, 40 columns text, 16 
	colors)
Keyboard: Full-sized 62 key QWERTY (8 programmable function keys; 2
	cursor keys, 2 directions each; Katakana characters accessible through
	C= key)

BIOS
	kernal.906145-02.bin
	basic.901226-01.bin
	characters.906143-02.bin


* PET 64 (1983)

  Also known as CBM 4064, it was a C64 fitted in a PET case and with a PET 
monochrome screen. Exactly like the C64, it features a VIC-II video chip,
a SID sound chip, 2 x CIA 6526, 64K of memory and BASIC 2.0. Being forced 
to display on a monochrome screen, color codes were removed from the front 
of number keys. The kernel itself was modified to output only white 
characters on a black background. A plate with BASIC commands and other hints 
was put above the keyboard.

Video: MOS 6569 "VIC-II" (320 x 200 Hi-Resolution, 40 columns text, 16 
	colors, monochrome 14" monitor support)
Sound: MOS 6581 "SID" (3 voice stereo synthesizer/digital sound 
	capabilities; Internal speaker) 
Keyboard: Full-sized 62 key QWERTY (8 programmable function keys; 2
	cursor keys, 2 directions each; complete BASIC reference on keyboard 
	panel )

BIOS
	kernal.4064.901246-01.bin
	basic.901226-01.bin
	characters.901225-01.bin


* Educator 64 (1983)

 Basically the same as the PET 64, but it uses a standard C64 kernel, 
allowing  to 'explore' all shades of green the monitor was capable of. Also 
the BASIC plate is slightly different. There could have been an earlier 
version, already called Educator 64, which came with no monitor and in a 
C64 case.

Keyboard: Full-sized 62 key QWERTY (8 programmable function keys; 2
	cursor keys, 2 directions each; complete BASIC reference on keyboard 
	panel )

BIOS
	kernal.901227-02.bin
	basic.901226-01.bin
	characters.901225-01.bin


* SX-64 Executive Computer (1984)

  Portable color computer based on the C64 hardware. The unit is heavy, with 
its metal case, features a built-in disk driver (mostly 1541 compatible) and 
has a large handle to carry the computer around. The detachable keyboard can 
be used as protective front plate. The SX-64 had a 4" full color screen and a
built-in speaker. The only differences between the C64 and the SX-64 are in 
the start up colors and in the SX-64 better support for the internal floppy
drive. The tape drive support was removed in the portable system.

CPU: MOS 6510 (1 MHz)
RAM: 64 kilobytes (68 with the 1541)
ROM: 20 kilobytes (36 with the 1541)
Video: MOS 6569 "VIC-II" (320 x 200 Hi-Resolution, 40 columns text, 16 
	colors, 4" Full-color screen)
Sound: MOS 6581 "SID" (3 voice stereo synthesizer/digital sound 
	capabilities; Internal MONO speaker) 
Ports: MOS 6526 CIA x2 (Power switch; 2 Commodore Joystick/Mouse ports; 
	CBM Serial port; CBM Datasette port; parallel programmable "User" 
	port; CBM Monitor port; C64 expansion port)
Keyboard: Full-sized 62 key QWERTY (8 programmable function keys; 2
	cursor keys, 2 directions each; detachable)
Additional hardware: Commodore 1541 Disk Drive (5.25" 170K SS SD Floppy)

BIOS:
	kernal.sx.251104-04.bin
	basic.901226-01.bin
	characters.901225-01.bin


* DX-64 (198?)

  Rumored version of a SX-64 variant featuring two floppy drives. Not sure
if it ever reached the prototype stage.


* Commodore 64C (1986)

  Redesigned version of the C64, released with slightly upgraded versions
of chips and peripherals. It came often bundled with GEOS, GUI based OS 
by Berkeley Softworks (it was stored on floppy disks). It's also known as
C64-II but the name doesn't seems to have ever been official (only used
by magazines at early stage of ad & review). Another redesign, in a slightly 
larger case (closer to the original C64) is known as Commodore 64G, because 
mainly sold in Germany. Other repackaged versions followed, built around the 
same kernel and chips.

CPU: CSG 8500 (1 MHz; 6510 compatible)
RAM: 64 kilobytes
ROM: 20 kilobytes
Video: MOS 8565 "VIC-II" (320 x 200 Hi-Resolution, 40 columns text, 16 
	colors)
Sound: CSG 8580 "SID" (3 voice stereo synthesizer/digital sound 
	capabilities) 
Ports: MOS 6526 CIA x2 (Power switch; 2 Commodore Joystick/Mouse ports; 
	CBM Serial port; CBM Datasette port; parallel programmable "User" 
	port; CBM Monitor port; C64 expansion port)
Keyboard: Full-sized 62 key QWERTY (8 programmable function keys; 2
	cursor keys, 2 directions each)

BIOS:
	64c.251913-01.bin
	characters.901225-01.bin


* Commodore 64 Games System (1990)

  Repackaged C64 with neither keyboard, nor ports. Basically, the
system came far too late to enter the console market of the 90s.

CPU: CSG 6510 (1 MHz)
RAM: 64 kilobytes
ROM: 16 kilobytes
Video: MOS 8565 "VIC-II" (320 x 200 Hi-Resolution, 40 columns text, 16 
	colors)
Sound: CSG 8580 "SID" (3 voice stereo synthesizer/digital sound 
	capabilities) 
Ports: MOS 6526 CIA (Power switch; 2 Commodore Joystick/Mouse ports; 
	CBM Monitor port; C64 expansion port)

BIOS:
	64gs.390852-01.bin
	characters.901225-01.bin

* Commodore 64 "Gold" (1984, US - 1986, Germany)

  Commemorative version released to celebrate the one millionth Commodore 64 
sold in the country. The whole case is golden colored. It is also reasonable 
to assume that the US version is based on the original C64, while the German 
version is based on the C64C.


[Rumored / Unconfirmed]

VC-10 - Ultimax with built-in (even if stripped down) BASIC 2.0
SX-100 - Rumored SX-64 prototype with built-in b&w screen
C64CGS - C64GS in a C64C-like case, sold in Ireland (?), with a keyboard (?)


[Known Fake / Unofficial BIOS]

Max Machine - These are BASIC V2.0 and kernel contained in the MAX BASIC 
cart, not a real Max BIOS.  
	basic.901230-01.bin
	kernal.901231-01.bin
	
Character roms
	c64-german.bin - amateur hack
	c64-hungarian.bin - amateur hack
	kauno.bin - calligraphic font for the C64, data saved from a 1985 tape

[Peripherals]

- CBM 1530 Datasette Recorder
- VIC 1541 / 1541-II / 1571 / 1581 disk drive (up to 5 at a time)
- Commodore Modem Cartridge 1650 / 1660 / 1670
- IEEE Interface Expansion Card
- Commodore CBM 8050 / 4040 Dual Floppy Disk Drives (through IEEE Interface)
- Commodore 6400 Letter Quality Printer (through IEEE Interface)
- Commodore 8023 Dot Matrix Printer (through IEEE Interface)
- Commodore 1520 Printer / Plotter
- Commodore 1701 / 1702 Monitor

- Lt. Kernal Hard Drive
- CMD HD-Series

- Commodore Joysticks
- Commodore Paddles
- Commodore 1350 / 1351 Mouse
- CMD SmartMouse 
- Inkwell Light Pen
- Koala Pad
- Commodore Music Maker overlay
- External music keyboard (to be plugged into the Sound Expander)

- Commodore REU: REU stands for RAM Expansion Unit for C64 & C128. Three
models 1700 (128 KB) and 1750 (512 KB), and later the 1764 (256 KB, for the 
C64)
- Commodore Sound Expander Cart
- Commodore Sound Sampler Cart
- Berkeley Softworks GeoRAM
- Schnedler Systems Turbo Master CPU
- Creative Micro Designs (CMD) RAMDrive, RAMLink and Super CPU Accelerator
- CMD SID symphony cartridge
- Commodore Universal Btx Decoder: Same design of the REU, it's a decoder 
for the Bildschirmtext, a sort of videotext online service similar to the
French Minitel. It allowed the C64 + Decoder to replace the very expensive
standalone decoder.

Freezer Carts: Datel "Action Replay", Freeze Frame MK III B, Trilogic 
"Expert", "The Final Cartridge III", "Retro Replay"
Kernal Replacement: SpeedDOS, DolphinDOS, JiffyDOS etc.

[TO DO]

* Floppy drives:

- Drives 8 & 9 supported, but limited compatibility. Real 1541 emulation is
needed.

* Cartridges:

- Currently only Type 0 hardware is supported (check cart code in 
machine/c64.c for more notes). Other Types may load but not work.

* Datasette:

- Currently, .t64 images are supported as Quickload device. This only works 
if the file contains a single entry with a start address of $0801. This 
covers most of the existing files, but the format is desinged to be much 
more flexible and it could be implemented as a real tape format.

* Other Peripherals:

- Lightpen support is unfinished
- Missing support for (it might or might not be added eventually):
printers; IEEE488; CPM cartridge, Speech cartridge, FM Sound cartridge
and other expansion modules; userport; rs232/v.24 interface; Super CPU;
dual SID configuration

* Informations / BIOS / Supported Sets:

- Find out if C64 was sold both as VIC 64S and C64 in Sweden, and which 
were inner/outer differences. For sure advertising was using the VIC 64S
name.
- Was it really sold as VC 64 in Germany, or was it just a nickname used
at launch to indicate standard C64 units?
- What's the difference between Educator 64-1 and Educator 64-2? My guess 
is that Edu64-1 was a standard PET64/CBMB4064 with a different name, while 
the Edu64-1 used the full C64 BIOS. Confirmations are needed, anyway.
 */

#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "sound/sid6581.h"
#include "sound/dac.h"
#include "machine/6526cia.h"

#include "machine/cbmipt.h"
#include "video/vic6567.h"

/* devices config */
#include "includes/cbm.h"
#include "includes/cbmserb.h"	// needed for MDRV_DEVICE_REMOVE
#include "includes/cbmdrive.h"
#include "includes/vc1541.h"

#include "includes/c64.h"


/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START(ultimax_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x0fff) AM_RAM AM_BASE(&c64_memory)
	AM_RANGE(0x8000, 0x9fff) AM_ROM AM_BASE(&c64_roml)
	AM_RANGE(0xd000, 0xd3ff) AM_READWRITE(vic2_port_r, vic2_port_w)
	AM_RANGE(0xd400, 0xd7ff) AM_DEVREADWRITE("sid6581", sid6581_r, sid6581_w)
	AM_RANGE(0xd800, 0xdbff) AM_READWRITE(SMH_RAM, c64_colorram_write) AM_BASE(&c64_colorram) /* colorram  */
	AM_RANGE(0xdc00, 0xdcff) AM_DEVREADWRITE("cia_0", cia_r, cia_w)
	AM_RANGE(0xe000, 0xffff) AM_ROM AM_BASE(&c64_romh)				/* ram or kernel rom */
ADDRESS_MAP_END

static ADDRESS_MAP_START(c64_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x7fff) AM_RAM AM_BASE(&c64_memory)
	AM_RANGE(0x8000, 0x9fff) AM_READWRITE(SMH_BANK(1), SMH_BANK(2))		/* ram or external roml */
	AM_RANGE(0xa000, 0xbfff) AM_ROMBANK(3) AM_WRITEONLY				/* ram or basic rom or external romh */
	AM_RANGE(0xc000, 0xcfff) AM_RAM
	AM_RANGE(0xd000, 0xdfff) AM_READWRITE(c64_ioarea_r, c64_ioarea_w)
	AM_RANGE(0xe000, 0xffff) AM_READWRITE(SMH_BANK(4), SMH_BANK(5))	   /* ram or kernel rom or external romh */
ADDRESS_MAP_END


/*************************************
 *
 *  Input Ports
 *
 *************************************/

static INPUT_PORTS_START( c64 )
	PORT_INCLUDE( common_cbm_keyboard )		/* ROW0 -> ROW7 */

	PORT_INCLUDE( c64_special )				/* SPECIAL */

	PORT_INCLUDE( c64_controls )			/* CTRLSEL, JOY0, JOY1, PADDLE0 -> PADDLE3, TRACKX, TRACKY, LIGHTX, LIGHTY, OTHER */
INPUT_PORTS_END

static INPUT_PORTS_START (c64gs)
	PORT_INCLUDE( c64 )
	
	/* 2008 FP: This has to be cleaned up later */
	/* C64gs should simply not scan these inputs */
	/* as a temporary solution, we keep PeT IPT_UNUSED shortcut */

	PORT_MODIFY( "ROW0" ) /* no keyboard */
	PORT_BIT (0xff, 0x00, IPT_UNUSED )
	PORT_MODIFY( "ROW1" ) /* no keyboard */
	PORT_BIT (0xff, 0x00, IPT_UNUSED )
	PORT_MODIFY( "ROW2" ) /* no keyboard */
	PORT_BIT (0xff, 0x00, IPT_UNUSED )
	PORT_MODIFY( "ROW3" ) /* no keyboard */
	PORT_BIT (0xff, 0x00, IPT_UNUSED )
	PORT_MODIFY( "ROW4" ) /* no keyboard */
	PORT_BIT (0xff, 0x00, IPT_UNUSED )
	PORT_MODIFY( "ROW5" ) /* no keyboard */
	PORT_BIT (0xff, 0x00, IPT_UNUSED )
	PORT_MODIFY( "ROW6" ) /* no keyboard */
	PORT_BIT (0xff, 0x00, IPT_UNUSED )
	PORT_MODIFY( "ROW7" ) /* no keyboard */
	PORT_BIT (0xff, 0x00, IPT_UNUSED )
	PORT_MODIFY( "SPECIAL" ) /* no keyboard */
	PORT_BIT (0xff, 0x00, IPT_UNUSED )
INPUT_PORTS_END


static INPUT_PORTS_START (vic64s)
	PORT_INCLUDE( c64 )

	PORT_MODIFY( "ROW5" )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xc3\xa5") PORT_CODE(KEYCODE_OPENBRACE)	PORT_CHAR('\xA5')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON)								PORT_CHAR(';') PORT_CHAR(']')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS)							PORT_CHAR('=')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS)								PORT_CHAR('-')

	PORT_MODIFY( "ROW6" )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xc3\xa4") PORT_CODE(KEYCODE_BACKSLASH)	PORT_CHAR('\xA4')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xc3\xb6") PORT_CODE(KEYCODE_QUOTE)		PORT_CHAR('\xB6')
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE)						PORT_CHAR('@')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH2)						PORT_CHAR(':') PORT_CHAR('*')
INPUT_PORTS_END

static INPUT_PORTS_START (vip64)
	PORT_INCLUDE( c64 )

	PORT_MODIFY( "ROW5" )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xc3\xa5") PORT_CODE(KEYCODE_OPENBRACE)	PORT_CHAR('\xA5')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON)								PORT_CHAR(';') PORT_CHAR(']')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS)							PORT_CHAR('=')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS)								PORT_CHAR('-')

	PORT_MODIFY( "ROW6" )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xc3\xa4") PORT_CODE(KEYCODE_BACKSLASH)	PORT_CHAR('\xA4')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xc3\xb6") PORT_CODE(KEYCODE_QUOTE)		PORT_CHAR('\xB6')
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE)						PORT_CHAR('@')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH2)						PORT_CHAR(':') PORT_CHAR('*')
INPUT_PORTS_END



/*************************************
 *
 *  Graphics definitions
 *
 *************************************/


static PALETTE_INIT( pet64 )
{
	int i;
	for (i = 0; i < 16; i++)
		palette_set_color_rgb(machine, i, 0, vic2_palette[i * 3 + 1], 0);
}


/*************************************
 *
 *  Sound definitions
 *
 *************************************/


static const sid6581_interface c64_sound_interface =
{
	c64_paddle_read
};


static const m6502_interface c64_m6510_interface =
{
	NULL,
	NULL,
	c64_m6510_port_read,
	c64_m6510_port_write
};


/*************************************
 *
 *  Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( c64 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", M6510, VIC6567_CLOCK)
	MDRV_CPU_PROGRAM_MAP(c64_mem)
	MDRV_CPU_CONFIG( c64_m6510_interface )
	MDRV_CPU_VBLANK_INT("screen", c64_frame_interrupt)
	//MDRV_CPU_PERIODIC_INT(vic2_raster_irq, VIC6567_HRETRACERATE)
	MDRV_QUANTUM_TIME(HZ(60))

	MDRV_MACHINE_START( c64 )
	MDRV_MACHINE_RESET( c64 )

	/* video hardware */
	MDRV_IMPORT_FROM( vh_vic2 )
	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_REFRESH_RATE(VIC6567_VRETRACERATE)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("sid6581", SID6581, VIC6567_CLOCK)
	MDRV_SOUND_CONFIG(c64_sound_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
	MDRV_SOUND_ADD("dac", DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* quickload */
	MDRV_QUICKLOAD_ADD("quickload", cbm_c64, "p00,prg,t64", CBM_QUICKLOAD_DELAY_SECONDS)

	/* cassette */
	MDRV_CASSETTE_ADD( "cassette", cbm_cassette_config )

	/* cia */
	MDRV_CIA6526_ADD("cia_0", CIA6526R1, VIC6567_CLOCK, c64_ntsc_cia0)
	MDRV_CIA6526_ADD("cia_1", CIA6526R1, VIC6567_CLOCK, c64_ntsc_cia1)

	/* floppy from serial bus */
	MDRV_IMPORT_FROM(simulated_drive)

	MDRV_IMPORT_FROM(c64_cartslot)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( c64pal )
	MDRV_CPU_ADD( "maincpu", M6510, VIC6569_CLOCK)
	MDRV_CPU_PROGRAM_MAP(c64_mem)
	MDRV_CPU_CONFIG( c64_m6510_interface )
	MDRV_CPU_VBLANK_INT("screen", c64_frame_interrupt)
	// MDRV_CPU_PERIODIC_INT(vic2_raster_irq, VIC6569_HRETRACERATE)
	MDRV_QUANTUM_TIME(HZ(50))

	MDRV_MACHINE_START( c64 )
	MDRV_MACHINE_RESET( c64 )

	/* video hardware */
	MDRV_IMPORT_FROM( vh_vic2_pal )
	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_REFRESH_RATE(VIC6569_VRETRACERATE)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0)) /* 2500 not accurate */

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("sid6581", SID6581, VIC6569_CLOCK)
	MDRV_SOUND_CONFIG(c64_sound_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
	MDRV_SOUND_ADD("dac", DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* quickload */
	MDRV_QUICKLOAD_ADD("quickload", cbm_c64, "p00,prg,t64", CBM_QUICKLOAD_DELAY_SECONDS)

	/* cassette */
	MDRV_CASSETTE_ADD( "cassette", cbm_cassette_config )

	/* cia */
	MDRV_CIA6526_ADD("cia_0", CIA6526R1, VIC6569_CLOCK, c64_pal_cia0)
	MDRV_CIA6526_ADD("cia_1", CIA6526R1, VIC6569_CLOCK, c64_pal_cia1)
	
	/* floppy from serial bus */
	MDRV_IMPORT_FROM(simulated_drive)

	MDRV_IMPORT_FROM(c64_cartslot)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ultimax )
	MDRV_IMPORT_FROM( c64 )
	MDRV_CPU_REPLACE( "maincpu", M6510, VIC6567_CLOCK)
	MDRV_CPU_PROGRAM_MAP( ultimax_mem)
	MDRV_CPU_CONFIG( c64_m6510_interface )

	MDRV_SOUND_REPLACE("sid6581", SID6581, VIC6567_CLOCK)
	MDRV_SOUND_CONFIG(c64_sound_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
	
	MDRV_DEVICE_REMOVE("serial_bus")	// in the current code, serial bus device is tied to the floppy drive
	MDRV_DEVICE_REMOVE("cart1")
	MDRV_DEVICE_REMOVE("cart2")

	MDRV_IMPORT_FROM(ultimax_cartslot)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pet64 )
	MDRV_IMPORT_FROM( c64 )
	MDRV_PALETTE_INIT( pet64 )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( c64gs )
	MDRV_IMPORT_FROM( c64pal )
	MDRV_DEVICE_REMOVE( "dac" )
	MDRV_DEVICE_REMOVE( "cassette" )
	MDRV_DEVICE_REMOVE( "quickload" )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( sx64 )
	MDRV_IMPORT_FROM( c64pal )

	MDRV_DEVICE_REMOVE("serial_bus")	// in the current code, serial bus device is tied to the floppy drive
	MDRV_IMPORT_FROM( cpu_vc1541 )			// so we need to remove the one from MDRV_IMPORT_FROM(simulated_drive)!

	MDRV_DEVICE_REMOVE( "dac" )
	MDRV_DEVICE_REMOVE( "cassette" )
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


ROM_START( max )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_REGION( 0x80000, "cart", ROMREGION_ERASE00 )
ROM_END

ROM_START( c64 )
	ROM_REGION( 0x19400, "maincpu", 0 )
	ROM_LOAD( "901226-01.bin", 0x10000, 0x2000, CRC(f833d117) SHA1(79015323128650c742a3694c9429aa91f355905e) )	// BASIC
	ROM_SYSTEM_BIOS(0, "default", "Kernal rev. 3" )
	ROMX_LOAD( "901227-03.bin", 0x12000, 0x2000, CRC(dbe3e7c7) SHA1(1d503e56df85a62fee696e7618dc5b4e781df1bb), ROM_BIOS(1) )	// Kernal
	ROM_SYSTEM_BIOS(1, "rev1", "Kernal rev. 1" )
	ROMX_LOAD( "901227-01.bin", 0x12000, 0x2000, CRC(dce782fa) SHA1(87cc04d61fc748b82df09856847bb5c2754a2033), ROM_BIOS(2) )	// Kernal
	ROM_SYSTEM_BIOS(2, "rev2", "Kernal rev. 2" )
	ROMX_LOAD( "901227-02.bin", 0x12000, 0x2000, CRC(a5c687b3) SHA1(0e2e4ee3f2d41f00bed72f9ab588b83e306fdb13), ROM_BIOS(3) )	// Kernal
	ROM_LOAD( "901225-01.bin", 0x14000, 0x1000, CRC(ec4272ee) SHA1(adc7c31e18c7c7413d54802ef2f4193da14711aa) )	// Character

	ROM_REGION( 0x80000, "cart", ROMREGION_ERASE00 )
ROM_END

#define rom_c64pal	rom_c64

ROM_START( c64jpn )
	ROM_REGION( 0x19400, "maincpu", 0 )
	ROM_LOAD( "901226-01.bin", 0x10000, 0x2000, CRC(f833d117) SHA1(79015323128650c742a3694c9429aa91f355905e) )
	ROM_LOAD( "906145-02.bin", 0x12000, 0x2000, CRC(3a9ef6f1) SHA1(4ff0f11e80f4b57430d8f0c3799ed0f0e0f4565d) )
	ROM_LOAD( "906143-02.bin", 0x14000, 0x1000, CRC(1604f6c1) SHA1(0fad19dbcdb12461c99657b2979dbb5c2e47b527) )

	ROM_REGION( 0x80000, "cart", ROMREGION_ERASE00 )
ROM_END


ROM_START( vic64s )
	ROM_REGION( 0x19400, "maincpu", 0 )
	ROM_LOAD( "901226-01.bin",	0x10000, 0x2000, CRC(f833d117) SHA1(79015323128650c742a3694c9429aa91f355905e) )
	ROM_LOAD( "kernel.swe",	0x12000, 0x2000, CRC(f10c2c25) SHA1(e4f52d9b36c030eb94524eb49f6f0774c1d02e5e) )
	ROM_SYSTEM_BIOS(0, "default", "Swedish Characters" )
	ROMX_LOAD( "charswe.bin",0x14000, 0x1000, CRC(bee9b3fd) SHA1(446ae58f7110d74d434301491209299f66798d8a), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS(1, "alt", "Swedish Characters (Alt)" )
	ROMX_LOAD( "charswe2.bin",0x14000, 0x1000, CRC(377a382b) SHA1(20df25e0ba1c88f31689c1521397c96968967fac), ROM_BIOS(2) )

	ROM_REGION( 0x80000, "cart", ROMREGION_ERASE00 )
ROM_END

#define rom_c64swe	rom_vic64s

ROM_START( pet64 )
	ROM_REGION( 0x19400, "maincpu", 0 )
	ROM_LOAD( "901226-01.bin", 0x10000, 0x2000, CRC(f833d117) SHA1(79015323128650c742a3694c9429aa91f355905e) )
	ROM_LOAD( "901246-01.bin", 0x12000, 0x2000, CRC(789c8cc5) SHA1(6c4fa9465f6091b174df27dfe679499df447503c) )
	ROM_LOAD( "901225-01.bin", 0x14000, 0x1000, CRC(ec4272ee) SHA1(adc7c31e18c7c7413d54802ef2f4193da14711aa) )

	ROM_REGION( 0x80000, "cart", ROMREGION_ERASE00 )
ROM_END

#define rom_cbm4064 rom_pet64
#define rom_edu64	rom_c64

ROM_START( sx64 )
	ROM_REGION( 0x19400, "maincpu", 0 )
	ROM_LOAD( "901226-01.bin", 0x10000, 0x2000, CRC(f833d117) SHA1(79015323128650c742a3694c9429aa91f355905e) )
	ROM_LOAD( "251104-04.bin", 0x12000, 0x2000, CRC(2c5965d4) SHA1(aa136e91ecf3c5ac64f696b3dbcbfc5ba0871c98) )
	ROM_LOAD( "901225-01.bin", 0x14000, 0x1000, CRC(ec4272ee) SHA1(adc7c31e18c7c7413d54802ef2f4193da14711aa) )

	ROM_REGION( 0x80000, "cart", ROMREGION_ERASE00 )

	VC1541_ROM("cpu_vc1540")
ROM_END

ROM_START( dx64 )
	ROM_REGION( 0x19400, "maincpu", 0 )
    ROM_LOAD( "901226-01.bin", 0x10000, 0x2000, CRC(f833d117) SHA1(79015323128650c742a3694c9429aa91f355905e) )
    ROM_LOAD( "dx64kern.bin",  0x12000, 0x2000, CRC(58065128) )

	ROM_REGION( 0x80000, "cart", ROMREGION_ERASE00 )

    // correct vc1541 roms were not included in submission
    VC1541_ROM("cpu_vc1540")
ROM_END

ROM_START( vip64 )
	ROM_REGION( 0x19400, "maincpu", 0 )
	ROM_LOAD( "901226-01.bin", 0x10000, 0x2000, CRC(f833d117) SHA1(79015323128650c742a3694c9429aa91f355905e) )
	ROM_LOAD( "kernelsx.swe",   0x12000, 0x2000, CRC(7858d3d7) SHA1(097cda60469492a8916c2677b7cce4e12a944bc0) )
	ROM_LOAD( "charswe.bin", 0x14000, 0x1000, CRC(bee9b3fd) SHA1(446ae58f7110d74d434301491209299f66798d8a) )

	ROM_REGION( 0x80000, "cart", ROMREGION_ERASE00 )

	VC1541_ROM("cpu_vc1540")
ROM_END


ROM_START( c64c )
	ROM_REGION( 0x19400, "maincpu", 0 )
	/* standard basic, modified kernel */
	ROM_LOAD( "251913-01.bin", 0x10000, 0x4000, CRC(0010ec31) SHA1(765372a0e16cbb0adf23a07b80f6b682b39fbf88) )
	ROM_LOAD( "901225-01.bin", 0x14000, 0x1000, CRC(ec4272ee) SHA1(adc7c31e18c7c7413d54802ef2f4193da14711aa) )

	ROM_REGION( 0x80000, "cart", ROMREGION_ERASE00 )
ROM_END

#define rom_c64cpal		rom_c64c
#define rom_c64g		rom_c64c

ROM_START( c64gs )
	ROM_REGION( 0x19400, "maincpu", 0 )
	/* standard basic, modified kernel */
	ROM_LOAD( "390852-01.bin", 0x10000, 0x4000, CRC(b0a9c2da) SHA1(21940ef5f1bfe67d7537164f7ca130a1095b067a) )
	ROM_LOAD( "901225-01.bin", 0x14000, 0x1000, CRC(ec4272ee) SHA1(adc7c31e18c7c7413d54802ef2f4193da14711aa) )

	ROM_REGION( 0x80000, "cart", ROMREGION_ERASE00 )
ROM_END



/*************************************
 *
 *  System configuration(s)
 *
 *************************************/


static SYSTEM_CONFIG_START(c64)
	CONFIG_DEVICE(cbmfloppy_device_getinfo)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(sx64)
	CONFIG_DEVICE(vc1541_device_getinfo)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*   YEAR  NAME   PARENT COMPAT MACHINE  INPUT    INIT    CONFIG    COMPANY                            FULLNAME */

COMP(1982, max,	    0,    0,    ultimax, c64,     ultimax, 0, "Commodore Business Machines Co.", "Commodore Max Machine", 0)

COMP(1982, c64,     0,    0,    c64,     c64,     c64,     c64,     "Commodore Business Machines Co.", "Commodore 64 (NTSC)", 0)
COMP(1982, c64pal,  c64,  0,    c64pal,  c64,     c64pal,  c64,     "Commodore Business Machines Co.", "Commodore 64 (PAL)", 0)
COMP(1982, c64jpn,  c64,  0,    c64,     c64,     c64,     c64,     "Commodore Business Machines Co.", "Commodore 64 (Japan)", 0)
COMP(1982, vic64s,  c64,  0,    c64pal,  vic64s,  c64pal,  c64,     "Commodore Business Machines Co.", "VIC 64S", 0)
COMP(1982, c64swe,  c64,  0,    c64pal,  vic64s,  c64pal,  c64,     "Commodore Business Machines Co.", "Commodore 64 (Sweden)", 0)

COMP(1983, pet64,	c64,  0,    pet64,   c64,     c64,     c64,     "Commodore Business Machines Co.", "PET 64 (NTSC)", 0)
COMP(1983, cbm4064, c64,  0,    pet64,   c64,     c64,     c64,     "Commodore Business Machines Co.", "CBM 4064 (NTSC)", 0)
COMP(1983, edu64,   c64,  0,    pet64,   c64,     c64,     c64,     "Commodore Business Machines Co.", "Educator 64 (NTSC)", 0) // maybe different palette?

// missing floppy emulation, among other things
COMP(1984, sx64,    c64,  0,    sx64,    c64,     sx64,    sx64,    "Commodore Business Machines Co.", "SX-64 Executive Computer (PAL)", GAME_NOT_WORKING)
COMP(1984, vip64,   c64,  0,    sx64,    vip64,   sx64,    sx64,    "Commodore Business Machines Co.", "VIP64 (SX64 PAL), Swedish Expansion Kit", GAME_NOT_WORKING)
COMP(198?, dx64,    c64,  0,    sx64,    c64,     sx64,    sx64,    "Commodore Business Machines Co.", "DX-64 (Prototype, PAL)", GAME_NOT_WORKING)

COMP(1986, c64c,    c64,  0,    c64,     c64,     c64,     c64,     "Commodore Business Machines Co.", "Commodore 64C (NTSC)", 0)
COMP(1986, c64cpal, c64,  0,    c64pal,  c64,     c64pal,  c64,     "Commodore Business Machines Co.", "Commodore 64C (PAL)", 0)
COMP(1986, c64g,    c64,  0,    c64pal,  c64,     c64pal,  c64,     "Commodore Business Machines Co.", "Commodore 64G (PAL)", 0)

CONS(1990, c64gs,   c64,  0,    c64gs,   c64gs,   c64gs,   0,   "Commodore Business Machines Co.", "Commodore 64 Games System (PAL)", 0)
