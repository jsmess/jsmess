/******************************************************************************
 *
 *  kaypro.c
 *
 *  Driver for Kaypro 2x
 *
 *  Juergen Buchmueller, July 1998
 *  Benjamin C. W. Sittler, July 1998 (new keyboard)
 *
 ******************************************************************************/

#include "driver.h"
#include "includes/kaypro.h"

#include "machine/wd17xx.h"
#include "machine/cpm_bios.h"
#include "devices/basicdsk.h"


static ADDRESS_MAP_START( kaypro_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( kaypro_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE( BIOS_CONST,  BIOS_CONST)  AM_READWRITE( kaypro_const_r, kaypro_const_w )
	AM_RANGE( BIOS_CONIN,  BIOS_CONIN)  AM_READWRITE( kaypro_conin_r, kaypro_conin_w )
	AM_RANGE( BIOS_CONOUT, BIOS_CONOUT) AM_READWRITE( kaypro_conout_r, kaypro_conout_w )
	AM_RANGE( BIOS_CMD,    BIOS_CMD)    AM_READWRITE( cpm_bios_command_r, cpm_bios_command_w )
ADDRESS_MAP_END

/*
 * The KAYPRO keyboard has roughly the following layout:
 *
 *                                                                  [up] [down] [left] [right]
 *         [ESC] [1!] [2@] [3#] [4$] [5%] [6^] [7&] [8*] [9(] [0)] [-_] [=+] [`~] [BACK SPACE]
 *           [TAB] [qQ] [wW] [eE] [rR] [tT] [yY] [uU] [iI] [oO] [pP] [[{] []}]       [DEL]
 *[CTRL] [CAPS LOCK] [aA] [sS] [dD] [fF] [gG] [hH] [jJ] [kK] [lL] [;:] ['"] [RETURN] [\|]
 *            [SHIFT] [zZ] [xX] [cC] [vV] [bB] [nN] [mM] [,<] [.>] [/?] [SHIFT] [LINE FEED]
 *                      [                 SPACE                     ]
 *
 * [7] [8] [9] [-]
 * [4] [5] [6] [,]
 * [1] [2] [3]
 * [  0  ] [.] [ENTER]
 *
 * Notes on Appearance
 * -------------------
 * The RETURN key is shaped like a backwards "L". The keypad ENTER key is actually
 * oriented vertically, not horizontally. The alpha keys are marked with the uppercase letters
 * only. Other keys with two symbols have the shifted symbol printed above the unshifted symbol.
 * The keypad is actually located to the right of the main keyboard; it is shown separately here
 * as a convenience to users of narrow listing windows. The arrow keys are actually marked with
 * arrow graphics pointing in the appropriate directions. The F and J keys are specially shaped,
 * since they are the "home keys" for touch-typing. The CAPS LOCK key has a build-in red indicator
 * which is lit when CAPS LOCK is pressed once, and extinguished when the key is pressed again.
 *
 * Technical Notes
 * ---------------
 * The keyboard interfaces to the computer using a serial protocol. Modifier keys are handled
 * inside the keyboards, as is the CAPS LOCK key. The arrow keys and the numeric keypad send
 * non-ASCII codes which are not affected by the modifier keys. The remaining keys send the
 * appropriate ASCII values.
 *
 * The keyboard has a built-in buzzer which is activated briefly by a non-modifier keypress,
 * producing a "clicking" sound for user feedback. Additionally, this buzzer can be activated
 * for a longer period by sending a 0x04 byte to the keyboard. This is used by the ROM soft
 * terminal to alert the user in case of a BEL.
*
 * 2008-05 FP:
 * Small note about natural keyboard support: currently,
 * - "Line Feed" is mapped to the 'End' key
 * - "Keypad ," is not mapped
 */

static INPUT_PORTS_START( kaypro )
	PORT_START_TAG("IN0") /* IN0 */ /* 2008-05: Is there any reason to keep this? */
	PORT_BIT(0xff, 0xff, IPT_UNUSED)

	PORT_START_TAG("ROW0")
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_ESC) 			PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_1) 			PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_2) 			PORT_CHAR('2') PORT_CHAR('@')
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_3) 			PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_4) 			PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_5) 			PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_6) 			PORT_CHAR('6') PORT_CHAR('^')
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_7) 			PORT_CHAR('7') PORT_CHAR('&')

	PORT_START_TAG("ROW1")
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_8) 			PORT_CHAR('8') PORT_CHAR('*')
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_9) 			PORT_CHAR('9') PORT_CHAR('(')
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_0) 			PORT_CHAR('0') PORT_CHAR(')')
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS) 		PORT_CHAR('-') PORT_CHAR('_')
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS) 		PORT_CHAR('=') PORT_CHAR('+')
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_TILDE) 		PORT_CHAR('`') PORT_CHAR('~')
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("BACK SPACE") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_TAB) 			PORT_CHAR('\t')

	PORT_START_TAG("ROW2")
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q) 			PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_W) 			PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_E) 			PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_R) 			PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_T) 			PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y) 			PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_U) 			PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_I) 			PORT_CHAR('i') PORT_CHAR('I')

	PORT_START_TAG("ROW3")
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_O) 			PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_P) 			PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE)		PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_CLOSEBRACE)	PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("DEL") PORT_CODE(KEYCODE_DEL) PORT_CHAR(UCHAR_MAMEKEY(DEL)) 
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_LCONTROL)		PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))

	PORT_START_TAG("ROW4")
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_A) 			PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_S) 			PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_D) 			PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_F) 			PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_G) 			PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_H) 			PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_J) 			PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_K) 			PORT_CHAR('k') PORT_CHAR('K')

	PORT_START_TAG("ROW5")
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_L) 			PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON)			PORT_CHAR(';') PORT_CHAR(':')
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE)			PORT_CHAR('\'') PORT_CHAR('"')
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH)		PORT_CHAR('\\') PORT_CHAR('|')
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_LSHIFT)		PORT_CHAR(UCHAR_MAMEKEY(LSHIFT))
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z) 			PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_X) 			PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_C) 			PORT_CHAR('c') PORT_CHAR('C')

	PORT_START_TAG("ROW6")
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_V) 			PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_B) 			PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_N) 			PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_M)				PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA)			PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP)			PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH)			PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_RSHIFT)		PORT_CHAR(UCHAR_MAMEKEY(RSHIFT))

	PORT_START_TAG("ROW7")
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("LINE FEED") PORT_CODE(KEYCODE_END) PORT_CHAR(UCHAR_MAMEKEY(END))
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE)			PORT_CHAR(' ')
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS_PAD) 	PORT_CHAR(UCHAR_MAMEKEY(MINUS_PAD))
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("Keypad ,") PORT_CODE(KEYCODE_PLUS_PAD)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_ENTER_PAD)		PORT_CHAR(UCHAR_MAMEKEY(ENTER_PAD))
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_DEL_PAD) 		PORT_CHAR(UCHAR_MAMEKEY(DEL_PAD))
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_0_PAD) 		PORT_CHAR(UCHAR_MAMEKEY(0_PAD))
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_1_PAD) 		PORT_CHAR(UCHAR_MAMEKEY(1_PAD))

	PORT_START_TAG("ROW8")
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_2_PAD) 		PORT_CHAR(UCHAR_MAMEKEY(2_PAD))
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_3_PAD) 		PORT_CHAR(UCHAR_MAMEKEY(3_PAD))
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_4_PAD) 		PORT_CHAR(UCHAR_MAMEKEY(4_PAD))
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_5_PAD) 		PORT_CHAR(UCHAR_MAMEKEY(5_PAD))
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_6_PAD) 		PORT_CHAR(UCHAR_MAMEKEY(6_PAD))
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_7_PAD) 		PORT_CHAR(UCHAR_MAMEKEY(7_PAD))
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_8_PAD) 		PORT_CHAR(UCHAR_MAMEKEY(8_PAD))
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_9_PAD)			PORT_CHAR(UCHAR_MAMEKEY(9_PAD))

	PORT_START_TAG("ROW9")
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("\xE2\x86\x91") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("\xE2\x86\x93") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("\xE2\x86\x92") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT(0xf0, 0x00, IPT_UNUSED)
INPUT_PORTS_END

#define FW  ((KAYPRO_FONT_W+7)/8)*8
#define FH  KAYPRO_FONT_H

static const gfx_layout charlayout =
{
	FW, FH,         /* 8*16 characters */
	4 * 256,        /* 4 * 256 characters */
	1,              /* 1 bits per pixel */
	{ 0 },          /* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
		8, 9,10,11,12,13,14,15,
		16,17,18,19,20,21,22,23,
		24,25,26,27,28,29,30,31 },
	/* y offsets */
	{ 0*FW, 1*FW, 2*FW, 3*FW, 4*FW, 5*FW, 6*FW, 7*FW,
		8*FW, 9*FW,10*FW,11*FW,12*FW,13*FW,14*FW,15*FW,
		16*FW,17*FW,18*FW,19*FW,20*FW,21*FW,22*FW,23*FW,
		24*FW,25*FW,26*FW,27*FW,28*FW,29*FW,30*FW,31*FW },
	FW * FH         /* every char takes 16 bytes */
};

static GFXDECODE_START( kaypro )
	GFXDECODE_ENTRY( REGION_GFX1, 0, charlayout, 0, 4 )
GFXDECODE_END

static const unsigned char kaypro_palette[] =
{
      0,  0,  0,    /* black */
      0,240,  0,    /* green */
      0,120,  0,    /* dim green */
      0,240,  0,    /* flashing green */
      0,120,  0,    /* flashing dim green */

    240,  0,  0,    /* just to keep the frontend happy */
      0,  0,240,
    120,120,120,
    240,240,240,
};

static const unsigned short kaypro_colortable[] =
{
    0,  1,      /* green on black */
    0,  2,      /* dim green on black */
    0,  3,      /* flashing green on black */
    0,  4,      /* flashing dim green on black */
};


/* Initialise the palette */
static PALETTE_INIT( kaypro )
{
	int i;

	for ( i = 0; i < sizeof(kaypro_palette) / 3; i++ ) {
		palette_set_color_rgb(machine, i, kaypro_palette[i*3], kaypro_palette[i*3+1], kaypro_palette[i*3+2]);
	}
}

static MACHINE_DRIVER_START( kaypro )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 4000000)        /* 4 Mhz */
	MDRV_CPU_PROGRAM_MAP(kaypro_mem, 0)
	MDRV_CPU_IO_MAP(kaypro_io, 0)
	MDRV_CPU_VBLANK_INT("main", kaypro_interrupt)
	MDRV_INTERLEAVE(4)

	MDRV_MACHINE_RESET( kaypro )

    /* video hardware */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(80*KAYPRO_FONT_W, 25*KAYPRO_FONT_H)
	MDRV_SCREEN_VISIBLE_AREA(0*KAYPRO_FONT_W, 80*KAYPRO_FONT_W-1, 0*KAYPRO_FONT_H, 25*KAYPRO_FONT_H-1)
	MDRV_GFXDECODE( kaypro )
	MDRV_PALETTE_LENGTH(sizeof(kaypro_palette) / 3)
	MDRV_PALETTE_INIT( kaypro )

	MDRV_VIDEO_START( kaypro )
	MDRV_VIDEO_UPDATE( kaypro )
MACHINE_DRIVER_END


ROM_START (kaypro)
    ROM_REGION(0x10000,REGION_CPU1,ROMREGION_ERASEFF) /* 64K for the Z80 */
    /* totally empty :) */

    ROM_REGION(0x04000,REGION_GFX1,0)  /* 4 * 4K font ram */
    ROM_LOAD ("kaypro2x.fnt", 0x0000, 0x1000, CRC(5f72da5b) SHA1(8a597000cce1a7e184abfb7bebcb564c6bf24fb7))

    ROM_REGION(0x01600,REGION_CPU2,0)  /* 5,5K for CCP and BDOS buffer */
    ROM_LOAD ("cpm62k.sys",   0x0000, 0x1600, CRC(d10cd036) SHA1(68c04701711fcb5ac1586eb8f060f3f0e02ba3dd))
ROM_END

static void kaypro_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 4; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:							info->load = DEVICE_IMAGE_LOAD_NAME(cpm_floppy); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "dsk"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}

static SYSTEM_CONFIG_START(kaypro)
	CONFIG_DEVICE(kaypro_floppy_getinfo)
SYSTEM_CONFIG_END

/*    YEAR  NAME      PARENT    COMPAT  MACHINE   INPUT     INIT      CONFIG    COMPANY   FULLNAME */
COMP( 1982, kaypro,   0,		0,		kaypro,   kaypro,   kaypro,   kaypro,   "Non Linear Systems",  "Kaypro 2x" , 0)
