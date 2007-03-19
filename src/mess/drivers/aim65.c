/******************************************************************************
 PeT mess@utanet.at Nov 2000
Updated by Dan Boris, 3/4/2007

ToDo:
	- Printer. Tried to implement this but it was not working, currently disabled.
	- Artwork.
	- Tape interface
	- User VIA (not really necessary since it only interfaces to external devices)

******************************************************************************/

#include "driver.h"

#include "machine/6821pia.h"
#include "machine/6522via.h"
#include "includes/riot6532.h"
#include "includes/aim65.h"

static ADDRESS_MAP_START( aim65_mem , ADDRESS_SPACE_PROGRAM, 8)
	//     -03ff 1k version
	//     -0fff 4k version
	AM_RANGE( 0x0000, 0x0fff) AM_RAM
//	{ 0xa000, 0xa00f, via_1_r }, // user via
	AM_RANGE( 0xa400, 0xa47f) AM_RAM // riot6532 ram
	AM_RANGE( 0xa480, 0xa49f) AM_READWRITE( riot_0_r, riot_0_w )
	AM_RANGE( 0xa800, 0xa80f) AM_READWRITE( via_0_r, via_0_w )
	AM_RANGE( 0xac00, 0xac03) AM_READWRITE( pia_0_r, pia_0_w )
	AM_RANGE( 0xac04, 0xac43) AM_RAM
	AM_RANGE( 0xb000, 0xffff) AM_ROM
ADDRESS_MAP_END

#define DIPS_HELPER(bit, name, keycode, r) \
   PORT_BIT(bit, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(name) PORT_CODE(keycode) PORT_CODE(r)

INPUT_PORTS_START( aim65 )
	PORT_START
	DIPS_HELPER( 0x0001, "1     !", KEYCODE_1, CODE_NONE)
	DIPS_HELPER( 0x0002, "2     \"", KEYCODE_2, CODE_NONE)
	DIPS_HELPER( 0x0004, "3     #", KEYCODE_3, CODE_NONE)
	DIPS_HELPER( 0x0008, "4     $", KEYCODE_4, CODE_NONE)
	DIPS_HELPER( 0x0010, "5     %", KEYCODE_5, CODE_NONE)
	DIPS_HELPER( 0x0020, "6     &", KEYCODE_6, CODE_NONE)
	DIPS_HELPER( 0x0040, "7     '", KEYCODE_7, CODE_NONE)
	DIPS_HELPER( 0x0080, "8     (", KEYCODE_8, CODE_NONE)
	DIPS_HELPER( 0x0100, "9     )", KEYCODE_9, CODE_NONE)
	DIPS_HELPER( 0x0200, "0"      , KEYCODE_0, CODE_NONE)
	DIPS_HELPER( 0x0400, ":     *", KEYCODE_MINUS, CODE_NONE)
	DIPS_HELPER( 0x0800, "F3"     , KEYCODE_EQUALS, KEYCODE_F3)
	DIPS_HELPER( 0x1000, "PRINT"  , KEYCODE_BACKSPACE, KEYCODE_PRTSCR)
	PORT_START
	DIPS_HELPER( 0x0001, "ESC", KEYCODE_TAB, KEYCODE_ESC)
	DIPS_HELPER( 0x0002, "Q", KEYCODE_Q, CODE_NONE)
	DIPS_HELPER( 0x0004, "W", KEYCODE_W, CODE_NONE)
	DIPS_HELPER( 0x0008, "E", KEYCODE_E, CODE_NONE)
	DIPS_HELPER( 0x0010, "R", KEYCODE_R, CODE_NONE)
	DIPS_HELPER( 0x0020, "T", KEYCODE_T, CODE_NONE)
	DIPS_HELPER( 0x0040, "Y", KEYCODE_Y, CODE_NONE)
	DIPS_HELPER( 0x0080, "U", KEYCODE_U, CODE_NONE)
	DIPS_HELPER( 0x0100, "I", KEYCODE_I, CODE_NONE)
	DIPS_HELPER( 0x0200, "O", KEYCODE_O, CODE_NONE)
	DIPS_HELPER( 0x0400, "P", KEYCODE_P, CODE_NONE)
	DIPS_HELPER( 0x0800, "-     =", KEYCODE_OPENBRACE, CODE_NONE)
	DIPS_HELPER( 0x1000, "F2    ]", KEYCODE_CLOSEBRACE, KEYCODE_F2) //? maybe f1
	DIPS_HELPER( 0x2000, "Return", KEYCODE_ENTER, CODE_NONE)
	PORT_START
    DIPS_HELPER( 0x0001, "CRTL", KEYCODE_LCONTROL, KEYCODE_RCONTROL)
	DIPS_HELPER( 0x0002, "A", KEYCODE_A, CODE_NONE)
	DIPS_HELPER( 0x0004, "S", KEYCODE_S, CODE_NONE)
	DIPS_HELPER( 0x0008, "D", KEYCODE_D, CODE_NONE)
	DIPS_HELPER( 0x0010, "F", KEYCODE_F, CODE_NONE)
	DIPS_HELPER( 0x0020, "G", KEYCODE_G, CODE_NONE)
	DIPS_HELPER( 0x0040, "H", KEYCODE_H, CODE_NONE)
	DIPS_HELPER( 0x0080, "J", KEYCODE_J, CODE_NONE)
	DIPS_HELPER( 0x0100, "K", KEYCODE_K, CODE_NONE)
	DIPS_HELPER( 0x0200, "L", KEYCODE_L, CODE_NONE)
	DIPS_HELPER( 0x0400, ";     +", KEYCODE_COLON, CODE_NONE)
	DIPS_HELPER( 0x0800, "LF    @", KEYCODE_QUOTE, CODE_NONE)
	DIPS_HELPER( 0x1000, "F1    [", KEYCODE_BACKSLASH, KEYCODE_F1)
	DIPS_HELPER( 0x2000, "DEL", KEYCODE_BACKSPACE, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x0001, "Left Shift", KEYCODE_LSHIFT, CODE_NONE)
	DIPS_HELPER( 0x0002, "Z", KEYCODE_Z, CODE_NONE)
	DIPS_HELPER( 0x0004, "X", KEYCODE_X, CODE_NONE)
	DIPS_HELPER( 0x0008, "C", KEYCODE_C, CODE_NONE)
	DIPS_HELPER( 0x0010, "V", KEYCODE_V, CODE_NONE)
	DIPS_HELPER( 0x0020, "B", KEYCODE_B, CODE_NONE)
	DIPS_HELPER( 0x0040, "N", KEYCODE_N, CODE_NONE)
	DIPS_HELPER( 0x0080, "M", KEYCODE_M, CODE_NONE)
	DIPS_HELPER( 0x0100, ",    <", KEYCODE_COMMA, CODE_NONE)
	DIPS_HELPER( 0x0200, ".    >", KEYCODE_STOP, CODE_NONE)
	DIPS_HELPER( 0x0400, "/    ?", KEYCODE_SLASH, CODE_NONE)
	DIPS_HELPER( 0x0800, "Right Shift", KEYCODE_RSHIFT, CODE_NONE)
	DIPS_HELPER( 0x1000, "Space", KEYCODE_SPACE, CODE_NONE)
	PORT_START
    PORT_DIPNAME(0x08, 0x08, "Terminal")
	PORT_DIPSETTING( 0x00, "TTY")
	PORT_DIPSETTING( 0x08, "Keyboard")
INPUT_PORTS_END

static gfx_layout aim65_charlayout =
{
        32,2,
        256,                                    /* 256 characters */
        1,                      /* 1 bits per pixel */
        { 0,0 },                  /* no bitplanes; 1 bit per pixel */
        /* x offsets */
        {
			7, 7, 7, 7,
			6, 6, 6, 6,
			5, 5, 5, 5,
			4, 4, 4, 4,
			3, 3, 3, 3,
			2, 2, 2, 2,
			1, 1, 1, 1,
			0, 0, 0, 0
        },
        /* y offsets */
        { 0, 0 },
        1*8
};

static gfx_decode aim65_gfxdecodeinfo[] = {
	{ REGION_GFX1, 0x0000, &aim65_charlayout,                     0, 2 },
    { -1 } /* end of array */
};

static unsigned short aim65_colortable[1][2] = {
	{ 0, 1 },
};


static MACHINE_DRIVER_START( aim65 )
	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, 1000000)
	MDRV_CPU_PROGRAM_MAP(aim65_mem, 0)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(800, 600)
	MDRV_SCREEN_VISIBLE_AREA(0, 800-1, 0, 600-1)
	MDRV_GFXDECODE( aim65_gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(242 + 32768)
	MDRV_COLORTABLE_LENGTH(sizeof (aim65_colortable) / sizeof(aim65_colortable[0][0]))
	MDRV_PALETTE_INIT( aim65 )

	MDRV_VIDEO_START( aim65 )
	MDRV_VIDEO_UPDATE( aim65 )
MACHINE_DRIVER_END


ROM_START(aim65)
	ROM_REGION(0x10000,REGION_CPU1, 0)
	ROM_LOAD_OPTIONAL ("aim65bas.z26", 0xb000, 0x1000, CRC(36a61f39))
	ROM_LOAD_OPTIONAL ("aim65bas.z25", 0xc000, 0x1000, CRC(d7b42d2a))
	ROM_LOAD_OPTIONAL ("aim65ass.z24", 0xd000, 0x1000, CRC(0878b399))
	ROM_LOAD ("aim65mon.z23", 0xe000, 0x1000, CRC(90e44afe) SHA1(78e38601edf6bfc787b58750555a636b0cf74c5c))
	ROM_LOAD ("aim65mon.z22", 0xf000, 0x1000, CRC(d01914b0) SHA1(e5b5ddd4cd43cce073a718ee4ba5221f2bc84eaf))
	ROM_REGION(0x100,REGION_GFX1, 0)
ROM_END

SYSTEM_CONFIG_START(aim65)
SYSTEM_CONFIG_END

/*    YEAR	NAME	PARENT	COMPAT	MACHINE	INPUT	INIT	CONFIG	COMPANY		FULLNAME */
COMP(197?,	aim65,	0,		0,		aim65,	aim65,	aim65,	aim65,	"Rockwell",	"AIM 65",0)
