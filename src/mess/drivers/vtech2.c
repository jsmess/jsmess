/***************************************************************************
	vtech2.c

    system driver
	Juergen Buchmueller <pullmoll@t-online.de> MESS driver, Jan 2000
	Davide Moretti <dave@rimini.com> ROM dump and hardware description

	LASER 350 (it has only 16K of RAM)
	FFFF|-------|
		| Empty |
		|	5	|
	C000|-------|
		|  RAM	|
		|	3	|
	8000|-------|-------|-------|
		|  ROM	|Display|  I/O	|
		|	1	|	3	|	2	|
	4000|-------|-------|-------|
		|  ROM	|
		|	0	|
	0000|-------|


	Laser 500/700 with 64K of RAM and
	Laser 350 with 64K RAM expansion module
	FFFF|-------|
		|  RAM	|
		|	5	|
	C000|-------|
		|  RAM	|
		|	4	|
	8000|-------|-------|-------|
		|  ROM	|Display|  I/O	|
		|	1	|	7	|	2	|
	4000|-------|-------|-------|
		|  ROM	|
		|	0	|
	0000|-------|


	Bank REGION_CPU1	   Contents
	0	 0x00000 - 0x03fff ROM 1st half
	1	 0x04000 - 0x07fff ROM 2nd half
	2			n/a 	   I/O 2KB area (mirrored 8 times?)
	3	 0x0c000 - 0x0ffff Display RAM (16KB) present in Laser 350 only!
	4	 0x10000 - 0x13fff RAM #4
	5	 0x14000 - 0x17fff RAM #5
	6	 0x18000 - 0x1bfff RAM #6
	7	 0x1c000 - 0x1ffff RAM #7 (Display RAM with 64KB)
	8	 0x20000 - 0x23fff RAM #8 (Laser 700 or 128KB extension)
	9	 0x24000 - 0x27fff RAM #9
	A	 0x28000 - 0x2bfff RAM #A
	B	 0x2c000 - 0x2ffff RAM #B
	C	 0x30000 - 0x33fff ROM expansion
	D	 0x34000 - 0x34fff ROM expansion
	E	 0x38000 - 0x38fff ROM expansion
	F	 0x3c000 - 0x3ffff ROM expansion

    TODO:
		Add ROMs and drivers for the Laser100, 110,
		210 and 310 machines and the Texet 8000.
		They should probably go to the vtech1.c files, though.

***************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "includes/vtech2.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "formats/vt_cas.h"

#define VERBOSE 0

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)	/* x */
#endif

static ADDRESS_MAP_START(vtech2_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READWRITE(MRA8_BANK1, MWA8_BANK1)
	AM_RANGE(0x4000, 0x7fff) AM_READWRITE(MRA8_BANK2, MWA8_BANK2)
	AM_RANGE(0x8000, 0xbfff) AM_READWRITE(MRA8_BANK3, MWA8_BANK3)
	AM_RANGE(0xc000, 0xffff) AM_READWRITE(MRA8_BANK4, MWA8_BANK4)
ADDRESS_MAP_END

static ADDRESS_MAP_START(vtech2_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x10, 0x1f) AM_READWRITE(laser_fdc_r, laser_fdc_w)
	AM_RANGE(0x40, 0x43) AM_WRITE(laser_bank_select_w)
	AM_RANGE(0x44, 0x44) AM_WRITE(laser_bg_mode_w)
	AM_RANGE(0x45, 0x45) AM_WRITE(laser_two_color_w)
ADDRESS_MAP_END

INPUT_PORTS_START( laser350 )
	PORT_START /* IN0 KEY ROW 0 */
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)

    PORT_START /* IN1 KEY ROW 1 */
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)

    PORT_START /* IN2 KEY ROW 2 */
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED ) /* TAB not on the Laser350 */
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)

    PORT_START /* IN3 KEY ROW 3 */
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED ) /* ESC not on the Laser350 */
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1 !") PORT_CODE(KEYCODE_1)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2 @") PORT_CODE(KEYCODE_2)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3 #") PORT_CODE(KEYCODE_3)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4 $") PORT_CODE(KEYCODE_4)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5 %") PORT_CODE(KEYCODE_5)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6 ^") PORT_CODE(KEYCODE_6)

	PORT_START /* IN4 KEY ROW 4 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("= +") PORT_CODE(KEYCODE_EQUALS)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("- _") PORT_CODE(KEYCODE_MINUS)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0 )") PORT_CODE(KEYCODE_0)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9 (") PORT_CODE(KEYCODE_9)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8 *") PORT_CODE(KEYCODE_8)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7 &") PORT_CODE(KEYCODE_7)

	PORT_START /* IN5 KEY ROW 5 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED ) /* BS not on the Laser350 */
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U)

	PORT_START /* IN6 KEY ROW 6 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("' \"") PORT_CODE(KEYCODE_QUOTE)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("; :") PORT_CODE(KEYCODE_COLON)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J)

	PORT_START /* IN7 KEY ROW 7 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED ) /* GRAPH not on the Laser350 */
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("` ~") PORT_CODE(KEYCODE_TILDE)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/ ?") PORT_CODE(KEYCODE_SLASH)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(". >") PORT_CODE(KEYCODE_STOP)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(", <") PORT_CODE(KEYCODE_COMMA)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)

    PORT_START /* IN8 KEY ROW A */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED ) /* not on the Laser350 */

	PORT_START /* IN9 KEY ROW B */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED ) /* not on the Laser350 */

	PORT_START /* IN10 KEY ROW C */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED ) /* not on the Laser350 */

	PORT_START /* IN11 KEY ROW D */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED ) /* not on the Laser350 */

	PORT_START /* IN12 Tape control */
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Tape start") PORT_CODE(KEYCODE_SLASH_PAD)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Tape stop") PORT_CODE(KEYCODE_ASTERISK)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Tape rewind") PORT_CODE(KEYCODE_MINUS_PAD)
	PORT_BIT (0x1f, IP_ACTIVE_HIGH, IPT_UNUSED )

INPUT_PORTS_END

INPUT_PORTS_START( laser500 )
	PORT_START /* IN0 KEY ROW 0 */
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)

    PORT_START /* IN1 KEY ROW 1 */
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)

    PORT_START /* IN2 KEY ROW 2 */
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("TAB") PORT_CODE(KEYCODE_TAB)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)

    PORT_START /* IN3 KEY ROW 3 */
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1 !") PORT_CODE(KEYCODE_1)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2 @") PORT_CODE(KEYCODE_2)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3 #") PORT_CODE(KEYCODE_3)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4 $") PORT_CODE(KEYCODE_4)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5 %") PORT_CODE(KEYCODE_5)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6 ^") PORT_CODE(KEYCODE_6)

	PORT_START /* IN4 KEY ROW 4 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("= +") PORT_CODE(KEYCODE_EQUALS)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("- _") PORT_CODE(KEYCODE_MINUS)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0 )") PORT_CODE(KEYCODE_0)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9 (") PORT_CODE(KEYCODE_9)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8 *") PORT_CODE(KEYCODE_8)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7 &") PORT_CODE(KEYCODE_7)

	PORT_START /* IN5 KEY ROW 5 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("BS") PORT_CODE(KEYCODE_BACKSPACE)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U)

	PORT_START /* IN6 KEY ROW 6 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("' \"") PORT_CODE(KEYCODE_QUOTE)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("; :") PORT_CODE(KEYCODE_COLON)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J)

	PORT_START /* IN7 KEY ROW 7 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("GRAPH") PORT_CODE(KEYCODE_LALT)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("` ~") PORT_CODE(KEYCODE_TILDE)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/ ?") PORT_CODE(KEYCODE_SLASH)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(". >") PORT_CODE(KEYCODE_STOP)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(", <") PORT_CODE(KEYCODE_COMMA)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)

    PORT_START /* IN8 KEY ROW A */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F1") PORT_CODE(KEYCODE_F1)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F2") PORT_CODE(KEYCODE_F2)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F3") PORT_CODE(KEYCODE_F3)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F4") PORT_CODE(KEYCODE_F4)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* IN9 KEY ROW B */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F10") PORT_CODE(KEYCODE_F10)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F9") PORT_CODE(KEYCODE_F9)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F8") PORT_CODE(KEYCODE_F8)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F7") PORT_CODE(KEYCODE_F7)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F6") PORT_CODE(KEYCODE_F6)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F5") PORT_CODE(KEYCODE_F5)

	PORT_START /* IN10 KEY ROW C */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CAPS") PORT_CODE(KEYCODE_CAPSLOCK)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DLINE") PORT_CODE(KEYCODE_PGUP)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("HOME") PORT_CODE(KEYCODE_HOME)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("UP") PORT_CODE(KEYCODE_UP)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LEFT") PORT_CODE(KEYCODE_LEFT)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RIGHT") PORT_CODE(KEYCODE_RIGHT)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DOWN") PORT_CODE(KEYCODE_DOWN)

	PORT_START /* IN11 KEY ROW D */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\\ |") PORT_CODE(KEYCODE_BACKSLASH)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("] }") PORT_CODE(KEYCODE_CLOSEBRACE)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("[ {") PORT_CODE(KEYCODE_OPENBRACE)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xC3\xA6") PORT_CODE(KEYCODE_ASTERISK)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DEL") PORT_CODE(KEYCODE_DEL)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("INS") PORT_CODE(KEYCODE_INSERT)

INPUT_PORTS_END

static gfx_layout charlayout_80 =
{
	8,8,					/* 8 x 8 characters */
	256,					/* 256 characters */
	1,                      /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },
	8*8 					/* every char takes 8 bytes */
};

static gfx_layout charlayout_40 =
{
	8*2,8,					/* 8*2 x 8 characters */
	256,					/* 256 characters */
	1,                      /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0,0, 1,1, 2,2, 3,3, 4,4, 5,5, 6,6, 7,7 },
	/* y offsets */
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },
	8*8 					/* every char takes 8 bytes */
};

static gfx_layout gfxlayout_1bpp =
{
	8,1,					/* 8x1 pixels */
	256,					/* 256 codes */
	1,						/* 1 bit per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 7,6,5,4,3,2,1,0 },
	/* y offsets */
	{ 0 },
	8						/* one byte per code */
};

static gfx_layout gfxlayout_1bpp_dw =
{
	8*2,1,					/* 8 times 2x1 pixels */
	256,					/* 256 codes */
	1,						/* 1 bit per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 7,7,6,6,5,5,4,4,3,3,2,2,1,1,0,0 },
	/* y offsets */
	{ 0 },
	8						/* one byte per code */
};

static gfx_layout gfxlayout_1bpp_qw =
{
	8*4,1,					/* 8 times 4x1 pixels */
	256,					/* 256 codes */
	1,						/* 1 bit per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 7,7,7,7,6,6,6,6,5,5,5,5,4,4,4,4,3,3,3,3,2,2,2,2,1,1,1,1,0,0,0,0 },
	/* y offsets */
	{ 0 },
	8						/* one byte per code */
};

static gfx_layout gfxlayout_4bpp =
{
	2*4,1,					/* 2 times 4x1 pixels */
	256,					/* 256 codes */
	4,						/* 4 bit per pixel */
	{ 0,1,2,3 },			/* four bitplanes */
	/* x offsets */
	{ 4,4,4,4, 0,0,0,0 },
	/* y offsets */
	{ 0 },
	2*4 					/* one byte per code */
};

static gfx_layout gfxlayout_4bpp_dh =
{
	2*4,2,					/* 2 times 4x2 pixels */
	256,					/* 256 codes */
	4,						/* 4 bit per pixel */
	{ 0,1,2,3 },			/* four bitplanes */
	/* x offsets */
	{ 4,4,4,4, 0,0,0,0 },
	/* y offsets */
	{ 0,0 },
	2*4 					/* one byte per code */
};

static gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout_80,		  0, 256 },
	{ REGION_GFX1, 0, &charlayout_40,		  0, 256 },
	{ REGION_GFX2, 0, &gfxlayout_1bpp,		  0, 256 },
	{ REGION_GFX2, 0, &gfxlayout_1bpp_dw,	  0, 256 },
	{ REGION_GFX2, 0, &gfxlayout_1bpp_qw,	  0, 256 },
	{ REGION_GFX2, 0, &gfxlayout_4bpp,	  2*256,   1 },
	{ REGION_GFX2, 0, &gfxlayout_4bpp_dh, 2*256,   1 },
	{ -1 } /* end of array */
};


static unsigned char vt_palette[] =
{
      0,  0,  0,    /* black */
      0,  0,127,    /* blue */
      0,127,  0,    /* green */
      0,127,127,    /* cyan */
    127,  0,  0,    /* red */
    127,  0,127,    /* magenta */
    127,127,  0,    /* yellow */
	160,160,160,	/* bright grey */
    127,127,127,    /* dark grey */
      0,  0,255,    /* bright blue */
      0,255,  0,    /* bright green */
      0,255,255,    /* bright cyan */
    255,  0,  0,    /* bright red */
    255,  0,255,    /* bright magenta */
    255,255,  0,    /* bright yellow */
    255,255,255,    /* bright white */
};

/* Initialise the palette */
static PALETTE_INIT( vtech2 )
{
	int i;

	palette_set_colors(machine, 0, vt_palette, sizeof(vt_palette) / 3);

	for (i = 0; i < 256; i++)
	{
		colortable[2*i] = i%16;
		colortable[2*i+1] = i/16;
	}
	for (i = 0; i < 16; i++)
		colortable[2*256+i] = i;
}

static INTERRUPT_GEN( vtech2_interrupt )
{
	cpunum_set_input_line(0, 0, HOLD_LINE);
}

static MACHINE_DRIVER_START( laser350 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 3694700)        /* 3.694700 Mhz */
	MDRV_CPU_PROGRAM_MAP(vtech2_mem, 0)
	MDRV_CPU_IO_MAP(vtech2_io, 0)
	MDRV_CPU_VBLANK_INT(vtech2_interrupt, 1)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(TIME_IN_USEC(0))
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_RESET( laser350 )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(88*8, 24*8+32)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 88*8-1, 0*8, 24*8+32-1)
	MDRV_GFXDECODE( gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(sizeof(vt_palette)/sizeof(vt_palette[0])/3)
	MDRV_COLORTABLE_LENGTH(256*2+16)
	MDRV_PALETTE_INIT(vtech2)

	MDRV_VIDEO_START(laser)
	MDRV_VIDEO_UPDATE(laser)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(WAVE, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SOUND_ADD(SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.75)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( laser500 )
	MDRV_IMPORT_FROM( laser350 )
	MDRV_MACHINE_RESET( laser500 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( laser700 )
	MDRV_IMPORT_FROM( laser350 )
	MDRV_MACHINE_RESET( laser700 )
MACHINE_DRIVER_END


ROM_START(laser350)
	ROM_REGION(0x40000,REGION_CPU1,0)
	ROM_LOAD("laserv3.rom", 0x00000, 0x08000, CRC(9bed01f7) SHA1(3210fddfab2f4c7855fa902fb8e2fc18d10d48f1))
	ROM_REGION(0x00800,REGION_GFX1,0)
	ROM_LOAD("laser.fnt",   0x00000, 0x00800, CRC(ed6bfb2a) SHA1(95e247021a10167b9de1d6ffc91ec4ba83b0ec87))
	ROM_REGION(0x00100,REGION_GFX2,0)
    /* initialized in init_laser */
ROM_END


ROM_START(laser500)
	ROM_REGION(0x40000,REGION_CPU1,0)
	ROM_LOAD("laserv3.rom", 0x00000, 0x08000, CRC(9bed01f7) SHA1(3210fddfab2f4c7855fa902fb8e2fc18d10d48f1))
	ROM_REGION(0x00800,REGION_GFX1,0)
	ROM_LOAD("laser.fnt",   0x00000, 0x00800, CRC(ed6bfb2a) SHA1(95e247021a10167b9de1d6ffc91ec4ba83b0ec87))
	ROM_REGION(0x00100,REGION_GFX2,0)
	/* initialized in init_laser */
ROM_END

ROM_START(laser700)
	ROM_REGION(0x40000,REGION_CPU1,0)
	ROM_LOAD("laserv3.rom", 0x00000, 0x08000, CRC(9bed01f7) SHA1(3210fddfab2f4c7855fa902fb8e2fc18d10d48f1))
	ROM_REGION(0x00800,REGION_GFX1,0)
	ROM_LOAD("laser.fnt",   0x00000, 0x00800, CRC(ed6bfb2a) SHA1(95e247021a10167b9de1d6ffc91ec4ba83b0ec87))
	ROM_REGION(0x00100,REGION_GFX2,0)
	/* initialized in init_laser */
ROM_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

static void laser_cassette_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_CASSETTE_FORMATS:				info->p = (void *) vtech2_cassette_formats; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}

static void laser_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_laser_cart; break;
		case DEVINFO_PTR_UNLOAD:						info->unload = device_unload_laser_cart; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "rom"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

static void laser_floppy_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:							info->i = IO_FLOPPY; break;
		case DEVINFO_INT_READABLE:						info->i = 1; break;
		case DEVINFO_INT_WRITEABLE:						info->i = 0; break;
		case DEVINFO_INT_CREATABLE:						info->i = 0; break;
		case DEVINFO_INT_COUNT:							info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_laser_floppy; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "dsk"); break;
	}
}

SYSTEM_CONFIG_START(laser)
	CONFIG_DEVICE(laser_cassette_getinfo)
	CONFIG_DEVICE(laser_cartslot_getinfo)
	CONFIG_DEVICE(laser_floppy_getinfo)
SYSTEM_CONFIG_END

/*	  YEAR	 NAME      PARENT    COMPAT MACHINE   INPUT	    INIT      CONFIG    COMPANY	             FULLNAME */
COMP( 1984?, laser350, 0,		 0,		laser350, laser350, laser,    laser,	"Video Technology",  "Laser 350" , 0)
COMP( 1984?, laser500, laser350, 0,		laser500, laser500, laser,    laser,	"Video Technology",  "Laser 500" , 0)
COMP( 1984?, laser700, laser350, 0,		laser700, laser500, laser,    laser,	"Video Technology",  "Laser 700" , 0)
