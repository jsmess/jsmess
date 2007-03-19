/************************************************************************
 *  Mattel Intellivision + Keyboard Component Drivers
 *
 *  Frank Palazzolo
 *  Kyle Davis
 *
 *  TBD:
 *		    Map game controllers correctly (right controller + 16 way)
 *		    Add tape support (intvkbd)
 *		    Add runtime tape loading
 *		    Fix memory system workaround
 *            (memory handler stuff in CP1610, debugger, and shared mem)
 *		    STIC
 *            reenable dirty support
 *		    Cleanup
 *			  Separate stic & vidhrdw better, get rid of *2 for kbd comp
 *		    Add better runtime cart loading
 *		    Switch to tilemap system
 *
 ************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "video/stic.h"
#include "includes/intv.h"
#include "devices/cartslot.h"
#include "sound/ay8910.h"

#ifndef VERBOSE
#ifdef MAME_DEBUG
#define VERBOSE 1
#else
#define VERBOSE 0
#endif
#endif

static unsigned char intv_palette[] =
{
	0x00, 0x00, 0x00, /* BLACK */
	0x00, 0x2D, 0xFF, /* BLUE */
	0xFF, 0x3D, 0x10, /* RED */
	0xC9, 0xCF, 0xAB, /* TAN */
	0x38, 0x6B, 0x3F, /* DARK GREEN */
	0x00, 0xA7, 0x56, /* GREEN */
	0xFA, 0xEA, 0x50, /* YELLOW */
	0xFF, 0xFC, 0xFF, /* WHITE */
	0xBD, 0xAC, 0xC8, /* GRAY */
	0x24, 0xB8, 0xFF, /* CYAN */
	0xFF, 0xB4, 0x1F, /* ORANGE */
	0x54, 0x6E, 0x00, /* BROWN */
	0xFF, 0x4E, 0x57, /* PINK */
	0xA4, 0x96, 0xFF, /* LIGHT BLUE */
	0x75, 0xCC, 0x80, /* YELLOW GREEN */
	0xB5, 0x1A, 0x58  /* PURPLE */
};

static PALETTE_INIT( intv )
{
	int i,j;

	/* Two copies of the palette */
	palette_set_colors(machine, 0, intv_palette, sizeof(intv_palette) / 3);
	palette_set_colors(machine, sizeof(intv_palette) / 3, intv_palette, sizeof(intv_palette) / 3);

    /* Two copies of the color table */
    for(i=0;i<16;i++)
    {
    	for(j=0;j<16;j++)
    	{
    		*colortable++ = i;
    		*colortable++ = j;
		}
	}
    for(i=0;i<16;i++)
    {
    	for(j=0;j<16;j++)
    	{
    		*colortable++ = i+16;
    		*colortable++ = j+16;
		}
	}
}

static struct AY8910interface ay8910_interface =
{
	intv_right_control_r,
	intv_left_control_r,
	0,
	0
};

/* graphics output */

gfx_layout intv_gromlayout =
{
	16, 16,
	256,
	1,
	{ 0 },
	{ 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7},
	{ 0*16, 0*16, 1*16, 1*16, 2*16, 2*16, 3*16, 3*16,
	  4*16, 4*16, 5*16, 5*16, 6*16, 6*16, 7*16, 7*16 },
	8 * 16
};

gfx_layout intv_gramlayout =
{
	16, 16,
	64,
	1,
	{ 0 },
	{ 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7},
	{ 0*8, 0*8, 1*8, 1*8, 2*8, 2*8, 3*8, 3*8,
	  4*8, 4*8, 5*8, 5*8, 6*8, 6*8, 7*8, 7*8 },
	8 * 8
};

gfx_layout intvkbd_charlayout =
{
	8, 8,
	256,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8 * 8
};

static gfx_decode intv_gfxdecodeinfo[] =
{
	{ REGION_CPU1, 0x3000<<1, &intv_gromlayout, 0, 256},
    { 0, 0, &intv_gramlayout, 0, 256 },    /* Dynamically decoded from RAM */
	{ -1 }
};

static gfx_decode intvkbd_gfxdecodeinfo[] =
{
	{ REGION_CPU1, 0x3000<<1, &intv_gromlayout, 0, 256},
    { 0, 0, &intv_gramlayout, 0, 256 },    /* Dynamically decoded from RAM */
	{ REGION_GFX1, 0x0000, &intvkbd_charlayout, 0, 256},
	{ -1 }
};

INPUT_PORTS_START( intv )
	PORT_START /* IN0 */	/* Right Player Controller Starts Here */
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1)
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2)
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4)
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5)
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6)
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7)
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8)

	PORT_START /* IN1 */
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("DEL") PORT_CODE(KEYCODE_DEL)
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER)
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("But1") PORT_CODE(KEYCODE_LCONTROL)
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("But2") PORT_CODE(KEYCODE_Z)
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("But3") PORT_CODE(KEYCODE_X)
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("NA")

	PORT_START /* IN2 */
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("UP") PORT_CODE(KEYCODE_UP)
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("PGUP") PORT_CODE(KEYCODE_PGUP)
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RIGHT") PORT_CODE(KEYCODE_RIGHT)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("PGDN") PORT_CODE(KEYCODE_PGDN)
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("DOWN") PORT_CODE(KEYCODE_DOWN)
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("END") PORT_CODE(KEYCODE_END)
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("LEFT") PORT_CODE(KEYCODE_LEFT)
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("HOME") PORT_CODE(KEYCODE_HOME)

	PORT_START /* IN3 */	/* Left Player Controller Starts Here */

	PORT_START /* IN4 */

	PORT_START /* IN5 */

INPUT_PORTS_END

/*
        Bit 7   Bit 6   Bit 5   Bit 4   Bit 3   Bit 2   Bit 1   Bit 0

 Row 0  NC      NC      NC      NC      NC      NC      CTRL    SHIFT
 Row 1  NC      NC      NC      NC      NC      NC      RPT     LOCK
 Row 2  NC      /       ,       N       V       X       NC      SPC
 Row 3  (right) .       M       B       C       Z       NC      CLS
 Row 4  (down)  ;       K       H       F       S       NC      TAB
 Row 5  ]       P       I       Y       R       W       NC      Q
 Row 6  (up)    -       9       7       5       3       NC      1
 Row 7  =       0       8       6       4       2       NC      [
 Row 8  (return)(left)  O       U       T       E       NC      ESC
 Row 9  DEL     '       L       J       G       D       NC      A
*/

INPUT_PORTS_START( intvkbd )
	PORT_START /* IN0 */	/* Keyboard Row 0 */
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NC")
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NC")
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NC")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NC")
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NC")
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NC")
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL)
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT)

	PORT_START /* IN1 */	/* Keyboard Row 1 */
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NC")
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NC")
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NC")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NC")
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NC")
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NC")
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("REPEAT") PORT_CODE(KEYCODE_RCONTROL)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LOCK") PORT_CODE(KEYCODE_RSHIFT)

	PORT_START /* IN2 */	/* Keyboard Row 2 */
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NC")
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/") PORT_CODE(KEYCODE_SLASH)
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(",") PORT_CODE(KEYCODE_COMMA)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(?)")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE)

	PORT_START /* IN3 */	/* Keyboard Row 3 */
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT)
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(".") PORT_CODE(KEYCODE_STOP)
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(?)")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Cls") PORT_CODE(KEYCODE_LALT)

	PORT_START /* IN4 */	/* Keyboard Row 4 */
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN)
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(";") PORT_CODE(KEYCODE_COLON)
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(?)")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Tab") PORT_CODE(KEYCODE_TAB)

	PORT_START /* IN5 */	/* Keyboard Row 5 */
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("]") PORT_CODE(KEYCODE_CLOSEBRACE)
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W)
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(?)")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)

	PORT_START /* IN6 */	/* Keyboard Row 6 */
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Up") PORT_CODE(KEYCODE_UP)
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("_") PORT_CODE(KEYCODE_MINUS)
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7)
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5)
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3)
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(?)")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1)

	PORT_START /* IN7 */	/* Keyboard Row 7 */
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("=") PORT_CODE(KEYCODE_EQUALS)
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6)
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4)
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2)
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(?)")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("[") PORT_CODE(KEYCODE_OPENBRACE)

	PORT_START /* IN8 */	/* Keyboard Row 8 */
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Return") PORT_CODE(KEYCODE_ENTER)
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT)
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U)
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(?)")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Esc") PORT_CODE(KEYCODE_ESC)

	PORT_START /* IN9 */	/* Keyboard Row 9 */
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(BS)") PORT_CODE(KEYCODE_BACKSPACE)
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("'") PORT_CODE(KEYCODE_QUOTE)
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J)
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(?)")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)

	PORT_START /* IN10 */	/* For tape drive testing... */
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_0_PAD)
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_1_PAD)
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_3_PAD)
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_4_PAD)
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_5_PAD)
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_7_PAD)
INPUT_PORTS_END

static ADDRESS_MAP_START( intv_mem , ADDRESS_SPACE_PROGRAM, 16)
	AM_RANGE(0x0000, 0x003f) AM_READWRITE( stic_r, stic_w )
    AM_RANGE(0x0100, 0x01ef) AM_READWRITE( intv_ram8_r, intv_ram8_w )
    AM_RANGE(0x01f0, 0x01ff) AM_READWRITE( AY8914_directread_port_0_lsb_r, AY8914_directwrite_port_0_lsb_w )
 	AM_RANGE(0x0200, 0x035f) AM_READWRITE( intv_ram16_r, intv_ram16_w )
	AM_RANGE(0x1000, 0x1fff) AM_ROM	AM_REGION(REGION_CPU1, 0x1000<<1)	/* Exec ROM, 10-bits wide */
	AM_RANGE(0x3000, 0x37ff) AM_ROM	AM_REGION(REGION_CPU1, 0x3000<<1)	/* GROM,     8-bits wide */
	AM_RANGE(0x3800, 0x39ff) AM_READWRITE( intv_gram_r, intv_gram_w )		/* GRAM,     8-bits wide */
	AM_RANGE(0x4800, 0x7fff) AM_ROM		/* Cartridges? */
ADDRESS_MAP_END

static ADDRESS_MAP_START( intvkbd_mem , ADDRESS_SPACE_PROGRAM, 16)
	AM_RANGE(0x0000, 0x003f) AM_READWRITE( stic_r, stic_w )
    AM_RANGE(0x0100, 0x01ef) AM_READWRITE( intv_ram8_r, intv_ram8_w )
    AM_RANGE(0x01f0, 0x01ff) AM_READWRITE( AY8914_directread_port_0_lsb_r, AY8914_directwrite_port_0_lsb_w )
 	AM_RANGE(0x0200, 0x035f) AM_READWRITE( intv_ram16_r, intv_ram16_w )
	AM_RANGE(0x1000, 0x1fff) AM_ROM	AM_REGION(REGION_CPU1, 0x1000<<1)	/* Exec ROM, 10-bits wide */
	AM_RANGE(0x3000, 0x37ff) AM_ROM	AM_REGION(REGION_CPU1, 0x3000<<1)	/* GROM,     8-bits wide */
	AM_RANGE(0x3800, 0x39ff) AM_READWRITE( intv_gram_r, intv_gram_w )	/* GRAM,     8-bits wide */
	AM_RANGE(0x4800, 0x6fff) AM_ROM		/* Cartridges? */
	AM_RANGE(0x7000, 0x7fff) AM_ROM	AM_REGION(REGION_CPU1, 0x7000<<1)	/* Keyboard ROM */
	AM_RANGE(0x8000, 0xbfff) AM_READWRITE( MRA16_RAM, intvkbd_dualport16_w ) AM_BASE(&intvkbd_dualport_ram)	/* Dual-port RAM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( intv2_mem , ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(0xff) )  /* Required because of probing */
	AM_RANGE( 0x0000, 0x3fff) AM_READWRITE( intvkbd_dualport8_lsb_r, intvkbd_dualport8_lsb_w )	/* Dual-port RAM */
	AM_RANGE( 0x4000, 0x7fff) AM_READWRITE( intvkbd_dualport8_msb_r, intvkbd_dualport8_msb_w )	/* Dual-port RAM */
	AM_RANGE( 0xb7f8, 0xb7ff) AM_RAM	/* ??? */
	AM_RANGE( 0xb800, 0xbfff) AM_READWRITE( videoram_r, videoram_w ) /* Text Display */
	AM_RANGE( 0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END

static INTERRUPT_GEN( intv_interrupt2 )
{
	cpunum_set_input_line(1, 0, PULSE_LINE);
}

static MACHINE_DRIVER_START( intv )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", CP1610, 3579545/4)        /* Colorburst/4 */
	MDRV_CPU_PROGRAM_MAP(intv_mem, 0)
	MDRV_CPU_VBLANK_INT(intv_interrupt,1)
	MDRV_SCREEN_REFRESH_RATE(59.92)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_RESET( intv )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(40*8, 24*8)
	MDRV_SCREEN_VISIBLE_AREA(0, 40*8-1, 0, 24*8-1)
	MDRV_GFXDECODE( intv_gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(2 * 2 * 16 * 16)
	MDRV_PALETTE_INIT( intv )

	MDRV_VIDEO_START( intv )
	MDRV_VIDEO_UPDATE( intv )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910, 3579545/2)
	MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)	
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( intvkbd )
	MDRV_IMPORT_FROM( intv )
	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_PROGRAM_MAP(intvkbd_mem, 0)

	MDRV_CPU_ADD(M6502, 3579545/2)	/* Colorburst/2 */
	MDRV_CPU_PROGRAM_MAP(intv2_mem, 0)
	MDRV_CPU_VBLANK_INT(intv_interrupt2,1)

	MDRV_INTERLEAVE(100)

    /* video hardware */
	MDRV_GFXDECODE( intvkbd_gfxdecodeinfo )
	MDRV_VIDEO_START( intvkbd )
	MDRV_VIDEO_UPDATE( intvkbd )
MACHINE_DRIVER_END

ROM_START(intv)
	ROM_REGION(0x10000<<1,REGION_CPU1,0)
	ROM_LOAD16_WORD( "exec.bin", (0x1000<<1)+0, 0x2000, CRC(cbce86f7) SHA1(5a65b922b562cb1f57dab51b73151283f0e20c7a))
	ROM_LOAD16_BYTE( "grom.bin", (0x3000<<1)+1, 0x0800, CRC(683a4158) SHA1(f9608bb4ad1cfe3640d02844c7ad8e0bcd974917))
ROM_END

ROM_START(intvsrs)
	ROM_REGION(0x10000<<1,REGION_CPU1,0)
	ROM_LOAD16_WORD( "searsexc.bin", (0x1000<<1)+0, 0x2000, CRC(ea552a22) SHA1(834339de056d42a35571cae7fd5b04d1344001e9))
	ROM_LOAD16_BYTE( "grom.bin", (0x3000<<1)+1, 0x0800, CRC(683a4158) SHA1(f9608bb4ad1cfe3640d02844c7ad8e0bcd974917))
ROM_END

ROM_START(intvkbd)
	ROM_REGION(0x10000<<1,REGION_CPU1,0)
	ROM_LOAD16_WORD( "exec.bin", 0x1000<<1, 0x2000, CRC(cbce86f7) SHA1(5a65b922b562cb1f57dab51b73151283f0e20c7a))
	ROM_LOAD16_BYTE( "grom.bin", (0x3000<<1)+1, 0x0800, CRC(683a4158) SHA1(f9608bb4ad1cfe3640d02844c7ad8e0bcd974917))
	ROM_LOAD16_WORD( "024.u60",  0x7000<<1, 0x1000, CRC(4f7998ec) SHA1(ec006d0ae9002e9d56d83a71f5f2eddd6a456a40))
	ROM_LOAD16_BYTE( "4d72.u62", 0x7800<<1, 0x0800, CRC(aa57c594) SHA1(741860d489d90f5882ca53daa3169b6abacdf130))
	ROM_LOAD16_BYTE( "4d71.u63", (0x7800<<1)+1, 0x0800, CRC(069b2f0b) SHA1(070850bb32f8474107cc52c5183cfaa32d640f9a))

	ROM_REGION(0x10000,REGION_CPU2,0)
	ROM_LOAD( "0104.u20",  0xc000, 0x1000, CRC(5c6f1256) SHA1(271931fb354dfae6a1a5697ee888924a89a15ca8))
	ROM_RELOAD( 0xe000, 0x1000 )
	ROM_LOAD("cpu2d.u21",  0xd000, 0x1000, CRC(2c2dba33) SHA1(0db5d177fec3f8ae89abeef2e6900ad4f3460266))
	ROM_RELOAD( 0xf000, 0x1000 )

	ROM_REGION(0x00800,REGION_GFX1,0)
	ROM_LOAD( "4c52.u34",  0x0000, 0x0800, CRC(cbeb2e96) SHA1(f0e17adcd278fb376c9f90833c7fbbb60193dbe3))
ROM_END

static void intv_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;
		case DEVINFO_INT_MUST_BE_LOADED:				info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_INIT:							info->init = device_init_intv_cart; break;
		case DEVINFO_PTR_LOAD:							info->load = device_load_intv_cart; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "int,rom"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(intv)
	CONFIG_DEVICE(intv_cartslot_getinfo)
SYSTEM_CONFIG_END

static void intvkbd_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_intvkbd_cart; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "int,rom,bin"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

static void intvkbd_cassette_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:							info->i = IO_CASSETTE; break;
		case DEVINFO_INT_READABLE:						info->i = 0;	/* INVALID */ break;
		case DEVINFO_INT_WRITEABLE:						info->i = 0;	/* INVALID */ break;
		case DEVINFO_INT_CREATABLE:						info->i = 0;	/* INVALID */ break;
		case DEVINFO_INT_COUNT:							info->i = 1; break;
		case DEVINFO_INT_RESET_ON_LOAD:					info->i = 1; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "tap"); break;
	}
}

SYSTEM_CONFIG_START(intvkbd)
	CONFIG_DEVICE(intvkbd_cartslot_getinfo)
#if 0
	CONFIG_DEVICE(intvkbd_cassette_getinfo)
#endif
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME		PARENT	COMPAT	MACHINE   INPUT     INIT		CONFIG		COMPANY      FULLNAME */
CONS( 1979, intv,		0,		0,		intv,     intv, 	NULL,		intv,		"Mattel",    "Intellivision", GAME_NOT_WORKING )
CONS( 1981, intvsrs,	0,		0,		intv,     intv, 	NULL,		intv,		"Mattel",    "Intellivision (Sears)", GAME_NOT_WORKING )
COMP( 1981, intvkbd,	0,		0,		intvkbd,  intvkbd, 	NULL,		intvkbd,	"Mattel",    "Intellivision Keyboard Component (Unreleased)", GAME_NOT_WORKING)
