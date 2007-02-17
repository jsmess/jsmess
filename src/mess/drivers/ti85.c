/***************************************************************************
TI-85 and TI-86 drivers by Krzysztof Strzecha

Notes:
1. After start TI-85 waits for ON key interrupt, so press ON key to start
   calculator.
2. Only difference beetwen all TI-85 drivers is ROM version.
3. TI-86 is TI-85 with more RAM and ROM.
4. Only difference beetwen all TI-86 drivers is ROM version.
5. Video engine (with grayscale support) based on the idea found in VTI source
   emulator written by Rusty Wagner.
6. NVRAM is saved properly only when calculator is turned off before exiting MESS.
7. To receive data from TI press "R" immediately after TI starts to send data.
8. To request screen dump from calculator press "S".
9. TI-81 have not serial link.

Needed:
1. Info about ports 3 (bit 2 seems to be allways 0) and 4.
2. Any info on TI-81 hardware.
3. ROM dumps of unemulated models.
4. Artworks.

New:
05/10/2002 TI-85 serial link works again.
17/09/2002 TI-85 snapshots loading fixed. Few code cleanups.
	   TI-86 SNAPSHOT LOADING DOESNT WORK.
	   TI-85, TI-86 SERIAL LINK DOESNT WORK.
08/09/2001 TI-81, TI-85, TI-86 modified to new core.
	   TI-81, TI-85, TI-86 reset corrected.
21/08/2001 TI-81, TI-85, TI-86 NVRAM corrected.
20/08/2001 TI-81 ON/OFF fixed.
	   TI-81 ROM bank switching added (port 5).
	   TI-81 NVRAM support added.
15/08/2001 TI-81 kayboard is now mapped as it should be.
14/08/2001 TI-81 preliminary driver added.
05/07/2001 Serial communication corrected (transmission works now after reset).
02/07/2001 Many source cleanups.
	   PCR added.
01/07/2001 Possibility to request screen dump from TI (received dumps are saved
	   as t85i file).
29/06/2001 Received variables can be saved now.
19/06/2001 Possibility to receive variables from calculator (they are nor saved
	   yet).
17/06/2001 TI-86 reset fixed.
15/06/2001 Possibility to receive memory backups from calculator.
07/06/2001 TI-85 reset fixed.
	   Work on receiving data from calculator started.
04/06/2001 TI-85 is able to receive variables and memory backups.
14/05/2001 Many source cleanups.
11/05/2001 Release years corrected. Work on serial link started.
26/04/2001 NVRAM support added.
25/04/2001 Video engine totaly rewriten so grayscale works now.
17/04/2001 TI-86 snapshots loading added.
	   ti86grom driver added.
16/04/2001 Sound added.
	   Five TI-86 drivers added (all features of TI-85 drivers without
	   snapshot loading).
13/04/2001 Snapshot loading (VTI 2.0 save state files).
18/02/2001 Palette (not perfect).
	   Contrast control (port 2) implemented.
	   LCD ON/OFF implemented (port 3).
	   Interrupts corrected (port 3) - ON/OFF and APD works now.
	   Artwork added.
09/02/2001 Keypad added.
	   200Hz timer interrupts implemented.
	   ON key and its interrupts implemented.
	   Calculator is now fully usable.
02/02/2001 Preliminary driver

To do:
- port 7 (TI-86)
- port 4 (all models)
- artwork (all models)
- add TI-82, TI-83 and TI-83+ drivers


TI-81 memory map

	CPU: Z80 2MHz
		0000-7fff ROM
		8000-ffff RAM (?)

TI-85 memory map

	CPU: Z80 6MHz
		0000-3fff ROM 0
		4000-7fff ROM 1-7 (switched)
		8000-ffff RAM

TI-86 memory map

	CPU: Z80 6MHz
		0000-3fff ROM 0
		4000-7fff ROM 0-15 or RAM 0-7 (switched)
		7000-bfff ROM 0-15 or RAM 0-7 (switched)
		c000-ffff RAM 0
		
Interrupts:

	IRQ: 200Hz timer
	     ON key	

TI-81 ports:
	0: Video buffer offset (write only)
	1: Keypad
	2: Contrast (write only)
	3: ON status, LCD power
	4: Video buffer width, interrupt control (write only)
	5: ?
	6: 
	7: ?

TI-85 ports:
	0: Video buffer offset (write only)
	1: Keypad
	2: Contrast (write only)
	3: ON status, LCD power
	4: Video buffer width, interrupt control (write only)
	5: Memory page
	6: Power mode
	7: Link

TI-86 ports:
	0: Video buffer offset (write only)
	1: Keypad
	2: Contrast (write only)
	3: ON status, LCD power
	4: Power mode
	5: Memory page
	6: Memory page
	7: Link

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "video/generic.h"
#include "includes/ti85.h"
#include "devices/snapquik.h"

/* port i/o functions */

ADDRESS_MAP_START( ti81_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x0000, 0x0000) AM_READWRITE( ti85_port_0000_r, ti85_port_0000_w )
	AM_RANGE(0x0001, 0x0001) AM_READWRITE( ti85_port_0001_r, ti85_port_0001_w )
	AM_RANGE(0x0002, 0x0002) AM_READWRITE( ti85_port_0002_r, ti85_port_0002_w )
	AM_RANGE(0x0003, 0x0003) AM_READWRITE( ti85_port_0003_r, ti85_port_0003_w )
	AM_RANGE(0x0004, 0x0004) AM_READWRITE( ti85_port_0004_r, ti85_port_0004_w )
	AM_RANGE(0x0005, 0x0005) AM_READWRITE( ti85_port_0005_r, ti85_port_0005_w )
	AM_RANGE(0x0007, 0x0007) AM_WRITE( ti81_port_0007_w)
ADDRESS_MAP_END

ADDRESS_MAP_START( ti85_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x0000, 0x0000) AM_READWRITE( ti85_port_0000_r, ti85_port_0000_w )
	AM_RANGE(0x0001, 0x0001) AM_READWRITE( ti85_port_0001_r, ti85_port_0001_w )
	AM_RANGE(0x0002, 0x0002) AM_READWRITE( ti85_port_0002_r, ti85_port_0002_w )
	AM_RANGE(0x0003, 0x0003) AM_READWRITE( ti85_port_0003_r, ti85_port_0003_w )
	AM_RANGE(0x0004, 0x0004) AM_READWRITE( ti85_port_0004_r, ti85_port_0004_w )
	AM_RANGE(0x0005, 0x0005) AM_READWRITE( ti85_port_0005_r, ti85_port_0005_w )
	AM_RANGE(0x0006, 0x0006) AM_READWRITE( ti85_port_0006_r, ti85_port_0006_w )
	AM_RANGE(0x0007, 0x0007) AM_READWRITE( ti85_port_0007_r, ti85_port_0007_w )
ADDRESS_MAP_END

ADDRESS_MAP_START( ti86_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x0000, 0x0000) AM_READWRITE( ti85_port_0000_r, ti85_port_0000_w )
	AM_RANGE(0x0001, 0x0001) AM_READWRITE( ti85_port_0001_r, ti85_port_0001_w )
	AM_RANGE(0x0002, 0x0002) AM_READWRITE( ti85_port_0002_r, ti85_port_0002_w )
	AM_RANGE(0x0003, 0x0003) AM_READWRITE( ti85_port_0003_r, ti85_port_0003_w )
	AM_RANGE(0x0004, 0x0004) AM_READWRITE( ti85_port_0006_r, ti85_port_0006_w )
	AM_RANGE(0x0005, 0x0005) AM_READWRITE( ti86_port_0005_r, ti86_port_0005_w )
	AM_RANGE(0x0006, 0x0006) AM_READWRITE( ti86_port_0006_r, ti86_port_0006_w )
	AM_RANGE(0x0007, 0x0007) AM_READWRITE( ti85_port_0007_r, ti85_port_0007_w )
ADDRESS_MAP_END

/* memory w/r functions */

ADDRESS_MAP_START( ti81_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_READWRITE( MRA8_BANK1, MWA8_BANK3 )
	AM_RANGE(0x4000, 0x7fff) AM_READWRITE( MRA8_BANK2, MWA8_BANK4 )
	AM_RANGE(0x8000, 0xffff) AM_RAM
ADDRESS_MAP_END

ADDRESS_MAP_START( ti85_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_READWRITE( MRA8_BANK1, MWA8_BANK3 )
	AM_RANGE(0x4000, 0x7fff) AM_READWRITE( MRA8_BANK2, MWA8_BANK4 )
	AM_RANGE(0x8000, 0xffff) AM_RAM
ADDRESS_MAP_END

ADDRESS_MAP_START( ti86_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_READWRITE( MRA8_BANK1, MWA8_BANK5 )
	AM_RANGE(0x4000, 0x7fff) AM_READWRITE( MRA8_BANK2, MWA8_BANK6 )
	AM_RANGE(0x8000, 0xbfff) AM_READWRITE( MRA8_BANK3, MWA8_BANK7 )
	AM_RANGE(0xc000, 0xffff) AM_READWRITE( MRA8_BANK4, MWA8_BANK8 )
ADDRESS_MAP_END
        
/* keyboard input */
INPUT_PORTS_START (ti81)
	PORT_START   /* bit 0 */
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(-)") PORT_CODE(KEYCODE_M)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(".") PORT_CODE(KEYCODE_STOP)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("GRAPH") PORT_CODE(KEYCODE_F5)
	PORT_START   /* bit 1 */
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("+") PORT_CODE(KEYCODE_EQUALS)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("STORE") PORT_CODE(KEYCODE_TAB)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("TRACE") PORT_CODE(KEYCODE_F4)
	PORT_START   /* bit 2 */
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("LN") PORT_CODE(KEYCODE_BACKSLASH)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ZOOM") PORT_CODE(KEYCODE_F3)
	PORT_START   /* bit 3 */
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Up") PORT_CODE(KEYCODE_UP)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("*") PORT_CODE(KEYCODE_L)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("LOG") PORT_CODE(KEYCODE_QUOTE)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RANGE") PORT_CODE(KEYCODE_F2)
	PORT_START   /* bit 4 */
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("/") PORT_CODE(KEYCODE_SLASH)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(")") PORT_CODE(KEYCODE_CLOSEBRACE)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(") PORT_CODE(KEYCODE_OPENBRACE)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("EE") PORT_CODE(KEYCODE_END)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("x^2") PORT_CODE(KEYCODE_COLON)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Y=") PORT_CODE(KEYCODE_F1)
	PORT_START   /* bit 5 */                                                        
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("^") PORT_CODE(KEYCODE_P)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("TAN") PORT_CODE(KEYCODE_PGUP)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("COS") PORT_CODE(KEYCODE_HOME)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("SIN") PORT_CODE(KEYCODE_INSERT)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("x^-1") PORT_CODE(KEYCODE_COMMA)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2nd") PORT_CODE(KEYCODE_LALT)
	PORT_START   /* bit 6 */
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CLEAR") PORT_CODE(KEYCODE_PGDN)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("VARS") PORT_CODE(KEYCODE_F9)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("PRGM") PORT_CODE(KEYCODE_F8)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("MATRX") PORT_CODE(KEYCODE_F7)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("MATH") PORT_CODE(KEYCODE_F6)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("INS") PORT_CODE(KEYCODE_TILDE)
	PORT_START   /* bit 7 */
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("MODE") PORT_CODE(KEYCODE_ESC)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("X|T") PORT_CODE(KEYCODE_X)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ALPHA") PORT_CODE(KEYCODE_CAPSLOCK)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("DEL") PORT_CODE(KEYCODE_DEL)
	PORT_START   /* ON */
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ON/OFF") PORT_CODE(KEYCODE_Q)
INPUT_PORTS_END

INPUT_PORTS_START (ti85)
	PORT_START   /* bit 0 */
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(-)") PORT_CODE(KEYCODE_M)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(".") PORT_CODE(KEYCODE_STOP)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F5") PORT_CODE(KEYCODE_F5)
	PORT_START   /* bit 1 */
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("+") PORT_CODE(KEYCODE_EQUALS)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("STORE") PORT_CODE(KEYCODE_TAB)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F4") PORT_CODE(KEYCODE_F4)
	PORT_START   /* bit 2 */
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(",") PORT_CODE(KEYCODE_COMMA)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F3") PORT_CODE(KEYCODE_F3)
	PORT_START   /* bit 3 */
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Up") PORT_CODE(KEYCODE_UP)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("*") PORT_CODE(KEYCODE_L)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("x^2") PORT_CODE(KEYCODE_COLON)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F2") PORT_CODE(KEYCODE_F2)
	PORT_START   /* bit 4 */
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("/") PORT_CODE(KEYCODE_SLASH)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(")") PORT_CODE(KEYCODE_CLOSEBRACE)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(") PORT_CODE(KEYCODE_OPENBRACE)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("EE") PORT_CODE(KEYCODE_END)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("LN") PORT_CODE(KEYCODE_BACKSLASH)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F1") PORT_CODE(KEYCODE_F1)
	PORT_START   /* bit 5 */                                                        
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("^") PORT_CODE(KEYCODE_P)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("TAN") PORT_CODE(KEYCODE_PGUP)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("COS") PORT_CODE(KEYCODE_HOME)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("SIN") PORT_CODE(KEYCODE_INSERT)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("LOG") PORT_CODE(KEYCODE_QUOTE)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2nd") PORT_CODE(KEYCODE_LALT)
	PORT_START   /* bit 6 */
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CLEAR") PORT_CODE(KEYCODE_PGDN)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CUSTOM") PORT_CODE(KEYCODE_F9)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("PRGM") PORT_CODE(KEYCODE_F8)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("STAT") PORT_CODE(KEYCODE_F7)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("GRAPH") PORT_CODE(KEYCODE_F6)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("EXIT") PORT_CODE(KEYCODE_ESC)
	PORT_START   /* bit 7 */
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("DEL") PORT_CODE(KEYCODE_DEL)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("x-VAR") PORT_CODE(KEYCODE_LCONTROL)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ALPHA") PORT_CODE(KEYCODE_CAPSLOCK)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("MORE") PORT_CODE(KEYCODE_TILDE)
	PORT_START   /* ON */
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ON/OFF") PORT_CODE(KEYCODE_Q)
	PORT_START   /* receive data from calculator */
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Receive data") PORT_CODE(KEYCODE_R)
	PORT_START   /* screen dump requesting */
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Screen dump request") PORT_CODE(KEYCODE_S)
INPUT_PORTS_END



/* machine definition */
static MACHINE_DRIVER_START( ti81 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 2000000)        /* 2 MHz */
	MDRV_CPU_PROGRAM_MAP(ti81_mem, 0)
	MDRV_CPU_IO_MAP(ti81_io, 0)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(TIME_IN_USEC(0))
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_START( ti81 )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(96, 64)
	MDRV_SCREEN_VISIBLE_AREA(0, 96-1, 0, 64-1)
	MDRV_PALETTE_LENGTH(32*7 + 32768)
	MDRV_COLORTABLE_LENGTH(32*7 + 32768)
	MDRV_PALETTE_INIT( ti85 )

	MDRV_VIDEO_START( ti85 )
	MDRV_VIDEO_UPDATE( ti85 )

	MDRV_NVRAM_HANDLER( ti81 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( ti85 )
	MDRV_IMPORT_FROM( ti81 )
	MDRV_CPU_REPLACE("main", Z80, 6000000)		/* 6 MHz */
	MDRV_CPU_PROGRAM_MAP(ti85_mem, 0)
	MDRV_CPU_IO_MAP(ti85_io, 0)

	MDRV_MACHINE_START( ti85 )

	MDRV_SCREEN_SIZE(128, 64)
	MDRV_SCREEN_VISIBLE_AREA(0, 128-1, 0, 64-1)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_NVRAM_HANDLER( ti85 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( ti86 )
	MDRV_IMPORT_FROM( ti85 )
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(ti86_mem, 0)
	MDRV_CPU_IO_MAP(ti86_io, 0)

	MDRV_MACHINE_START( ti86 )

	MDRV_NVRAM_HANDLER( ti86 )
MACHINE_DRIVER_END


ROM_START (ti81)
	ROM_REGION (0x18000, REGION_CPU1,0)
	ROM_LOAD ("ti81.bin", 0x10000, 0x8000, CRC(94ac58e2) SHA1(ba915cfe2fe50a452ef8287db8f2244e29056d54))
ROM_END

ROM_START (ti85)
	ROM_REGION (0x30000, REGION_CPU1,0)
	ROM_LOAD ("ti85v30a.bin", 0x10000, 0x20000, CRC(de4c0b1a) SHA1(f4cf4b8309372dbe26187bb279545f5d4bd48fc1))
ROM_END

ROM_START (ti85v40)
	ROM_REGION (0x30000, REGION_CPU1,0)
	ROM_LOAD ("ti85v40.bin", 0x10000, 0x20000, CRC(a1723a17) SHA1(ff5866636bb3f206a6bf39cc9c9dc8308332aaf0))
ROM_END

ROM_START (ti85v50)
	ROM_REGION (0x30000, REGION_CPU1,0)
	ROM_LOAD ("ti85v50.bin", 0x10000, 0x20000, CRC(781fa403) SHA1(bf20d520d8efd7e5ae269789ca4b3c71848ac32a))
ROM_END

ROM_START (ti85v60)
	ROM_REGION (0x30000, REGION_CPU1,0)
	ROM_LOAD ("ti85v60.bin", 0x10000, 0x20000, CRC(b694a117) SHA1(36d58e2723e5ae4ffe0f8da691fa9a83bfe9e06b))
ROM_END

ROM_START (ti85v80)
	ROM_REGION (0x30000, REGION_CPU1,0)
	ROM_LOAD ("ti85v80.bin", 0x10000, 0x20000, CRC(7f296338) SHA1(765d5c612b6ffc0d1ded8f79bcbe880b1b562a98))
ROM_END

ROM_START (ti85v90)
	ROM_REGION (0x30000, REGION_CPU1,0)
	ROM_LOAD ("ti85v90.bin", 0x10000, 0x20000, CRC(6a0a94d0) SHA1(7742bf8a6929a21d06f306b494fc03b1fbdfe3e4))
ROM_END

ROM_START (ti85v100)
	ROM_REGION (0x30000, REGION_CPU1,0)
	ROM_LOAD ("ti85v100.bin", 0x10000, 0x20000, CRC(053325b0) SHA1(36da1080c34e7b53cbe8463be5804e30e4a50dc8))
ROM_END

ROM_START (ti86)
	ROM_REGION (0x50000, REGION_CPU1,0)
	ROM_LOAD ("ti86v12.bin", 0x10000, 0x40000, CRC(bdf16105) SHA1(e40b22421c31bf0af104518b748ae79cd21d9c57))
ROM_END

ROM_START (ti86v13)
	ROM_REGION (0x50000, REGION_CPU1,0)
	ROM_LOAD ("ti86v13.bin", 0x10000, 0x40000, CRC(073ef70f) SHA1(5702d4bb835bdcbfa8075ffd620fca0eaf3a1592))
ROM_END

ROM_START (ti86v14)
	ROM_REGION (0x50000, REGION_CPU1,0)
	ROM_LOAD ("ti86v14.bin", 0x10000, 0x40000, CRC(fe6e2986) SHA1(23e0fb9a1763d5b9a7b0e593f09c2ff30c760866))
ROM_END

ROM_START (ti86v15)
	ROM_REGION (0x50000, REGION_CPU1,0)
	ROM_LOAD ("ti86v15.bin", 0x10000, 0x40000, BAD_DUMP CRC(e6e10546))
ROM_END

ROM_START (ti86v16)
	ROM_REGION (0x50000, REGION_CPU1,0)
	ROM_LOAD ("ti86v16.bin", 0x10000, 0x40000, CRC(37e02acc) SHA1(b5ad204885e5dde23a22f18f8d5eaffca69d638d))
ROM_END

ROM_START (ti86grom)
	ROM_REGION (0x50000, REGION_CPU1,0)
	ROM_LOAD ("ti86grom.bin", 0x10000, 0x40000, CRC(d2c67280) SHA1(d5c41f6584fe209ed3c615bda58c35469782f3cd))
ROM_END

static void ti85_snapshot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* snapshot */
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "sav"); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_SNAPSHOT_LOAD:					info->f = (genf *) snapshot_load_ti8x; break;

		default:										snapshot_device_getinfo(devclass, state, info); break;
	}
}

static void ti85_serial_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* serial */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:							info->i = IO_SERIAL; break;
		case DEVINFO_INT_READABLE:						info->i = 1; break;
		case DEVINFO_INT_WRITEABLE:						info->i = 0; break;
		case DEVINFO_INT_CREATABLE:						info->i = 0; break;
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_INIT:							info->init = device_init_ti85_serial; break;
		case DEVINFO_PTR_LOAD:							info->load = device_load_ti85_serial; break;
		case DEVINFO_PTR_UNLOAD:						info->unload = device_unload_ti85_serial; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "85p,85s,85i,85n,85c,85l,85k,85m,85v,85d,85e,85r,85g,85b"); break;
	}
}

SYSTEM_CONFIG_START(ti85)
	CONFIG_DEVICE(ti85_serial_getinfo)
	CONFIG_DEVICE(ti85_snapshot_getinfo)
SYSTEM_CONFIG_END

static void ti86_snapshot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* snapshot */
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "sav"); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_SNAPSHOT_LOAD:					info->f = (genf *) snapshot_load_ti8x; break;

		default:										snapshot_device_getinfo(devclass, state, info); break;
	}
}

static void ti86_serial_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* serial */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:							info->i = IO_SERIAL; break;
		case DEVINFO_INT_READABLE:						info->i = 1; break;
		case DEVINFO_INT_WRITEABLE:						info->i = 0; break;
		case DEVINFO_INT_CREATABLE:						info->i = 0; break;
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_INIT:							info->init = device_init_ti85_serial; break;
		case DEVINFO_PTR_LOAD:							info->load = device_load_ti85_serial; break;
		case DEVINFO_PTR_UNLOAD:						info->unload = device_unload_ti85_serial; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "86p,86s,86i,86n,86c,86l,86k,86m,86v,86d,86e,86r,86g"); break;
	}
}

SYSTEM_CONFIG_START(ti86)
	CONFIG_DEVICE(ti86_serial_getinfo)
	CONFIG_DEVICE(ti86_snapshot_getinfo)
SYSTEM_CONFIG_END
                            
/*    YEAR  NAME		PARENT	COMPAT	MACHINE INPUT	INIT	CONFIG	COMPANY        FULLNAME */
COMP( 1990, ti81,          0,   0,		ti81,	ti81,	0,		NULL,	"Texas Instruments", "TI-81 Ver. 1.8" , 0)

COMP( 1992, ti85,          0,   0,		ti85,	ti85,	0,		ti85,	"Texas Instruments", "TI-85 ver. 3.0a" , 0)
COMP( 1992, ti85v40,    ti85,   0,		ti85,	ti85,	0,		ti85,	"Texas Instruments", "TI-85 ver. 4.0" , 0)
COMP( 1992, ti85v50,    ti85,   0,		ti85,	ti85,	0,		ti85,	"Texas Instruments", "TI-85 ver. 5.0" , 0)
COMP( 1992, ti85v60,    ti85,   0,		ti85,	ti85,	0,		ti85,	"Texas Instruments", "TI-85 ver. 6.0" , 0)
COMP( 1992, ti85v80,    ti85,   0,		ti85,	ti85,	0,		ti85,	"Texas Instruments", "TI-85 ver. 8.0" , 0)
COMP( 1992, ti85v90,    ti85,   0,		ti85,	ti85,	0,		ti85,	"Texas Instruments", "TI-85 ver. 9.0" , 0)
COMP( 1992, ti85v100,   ti85,   0,		ti85,	ti85,	0,		ti85,	"Texas Instruments", "TI-85 ver. 10.0" , 0)

COMP( 1997, ti86,   	   0,   0,		ti86,	ti85,	0,		ti86,	"Texas Instruments", "TI-86 ver. 1.2" , 0)
COMP( 1997, ti86v13,   	ti86,   0,		ti86,	ti85,	0,		ti86,	"Texas Instruments", "TI-86 ver. 1.3" , 0)
COMP( 1997, ti86v14,   	ti86,   0,		ti86,	ti85,	0,		ti86,	"Texas Instruments", "TI-86 ver. 1.4" , 0)
COMP( 1997, ti86v15,   	ti86,   0,		ti86,	ti85,	0,		ti86,	"Texas Instruments", "TI-86 ver. 1.5" , 0)
COMP( 1997, ti86v16,   	ti86,   0,		ti86,	ti85,	0,		ti86,	"Texas Instruments", "TI-86 ver. 1.6" , 0)
COMP( 1997, ti86grom,   ti86,   0,		ti86,	ti85,	0,		ti86,	"Texas Instruments", "TI-86 homebrew rom by Daniel Foesch" , 0)
