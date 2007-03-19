/************************************************************************
Aquarius Memory map

	CPU: z80

	Memory map
		0000 1fff	BASIC
		2000 2fff	expansion?
		3000 33ff	screen ram
		3400 37ff	colour ram
		3800 3fff	RAM (standard)
		4000 7fff	RAM (expansion)
		8000 ffff	RAM (emulator only)

	Ports: Out
		fc			Buzzer, bit 0.
		fe			Printer.

	Ports: In
		fc			Tape in, bit 1.
		fe			Printer.
		ff			Keyboard, Bit set in .B selects keyboard matrix
					line. Return bit 0 - 5 low for pressed key.

************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "video/generic.h"
#include "includes/aquarius.h"

/* structures */

/* port i/o functions */

ADDRESS_MAP_START( aquarius_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) ) 
	AM_RANGE(0xfc, 0xfc) AM_WRITE( aquarius_port_fc_w)
	AM_RANGE(0xfe, 0xfe) AM_READWRITE( aquarius_port_fe_r, aquarius_port_fe_w)
	AM_RANGE(0xff, 0xff) AM_READWRITE( aquarius_port_ff_r, aquarius_port_ff_w)
ADDRESS_MAP_END

/* Memory w/r functions */

ADDRESS_MAP_START( aquarius_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0x2fff) AM_NOP
	AM_RANGE(0x3000, 0x37ff) AM_READWRITE(videoram_r, aquarius_videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x3800, 0x3fff) AM_RAM
	AM_RANGE(0x4000, 0x7fff) AM_NOP
	AM_RANGE(0x8000, 0xffff) AM_NOP
ADDRESS_MAP_END

/* graphics output */

static gfx_layout aquarius_charlayout =
{
	8, 8,
	256,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8,
	  4*8, 5*8, 6*8, 7*8, },
	8 * 8
};

static gfx_decode aquarius_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &aquarius_charlayout, 0, 256},
	{ -1 } /* end of array */
};

static unsigned char aquarius_palette[] =
{
	0x00, 0x00, 0x00,	/* Black */
	0xff, 0x00, 0x00,	/* Red */
	0x00, 0xff, 0x00,	/* Green */
	0xff, 0xff, 0x00,	/* Yellow */
	0x00, 0x00, 0xff,	/* Blue */
	0x7f, 0x00, 0x7f,	/* Violet */
	0x7f, 0xff, 0xff,	/* Light Blue-Green */
	0xff, 0xff, 0xff,	/* White */
	0xc0, 0xc0, 0xc0,	/* Light Gray */
	0x00, 0xff, 0xff,	/* Blue-Green */
	0xff, 0x00, 0xff,	/* Magenta */
	0x00, 0x00, 0x7f,	/* Dark Blue */
	0xff, 0xff, 0x7f,	/* Light Yellow */
	0x7f, 0xff, 0x7f,	/* Light Green */
	0xff, 0x7f, 0x00,	/* Orange */
	0x7f, 0x7f, 0x7f,	/* Dark Gray */
};

static unsigned short aquarius_colortable[] =
{
    0, 0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8, 0, 9, 0,10, 0,11, 0,12, 0,13, 0,14, 0,15, 0,
    0, 1, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 1, 7, 1, 8, 1, 9, 1,10, 1,11, 1,12, 1,13, 1,14, 1,15, 1,
    0, 2, 1, 2, 2, 2, 3, 2, 4, 2, 5, 2, 6, 2, 7, 2, 8, 2, 9, 2,10, 2,11, 2,12, 2,13, 2,14, 2,15, 2,
    0, 3, 1, 3, 2, 3, 3, 3, 4, 3, 5, 3, 6, 3, 7, 3, 8, 3, 9, 3,10, 3,11, 3,12, 3,13, 3,14, 3,15, 3,
    0, 4, 1, 4, 2, 4, 3, 4, 4, 4, 5, 4, 6, 4, 7, 4, 8, 4, 9, 4,10, 4,11, 4,12, 4,13, 4,14, 4,15, 4,
    0, 5, 1, 5, 2, 5, 3, 5, 4, 5, 5, 5, 6, 5, 7, 5, 8, 5, 9, 5,10, 5,11, 5,12, 5,13, 5,14, 5,15, 5,
    0, 6, 1, 6, 2, 6, 3, 6, 4, 6, 5, 6, 6, 6, 7, 6, 8, 6, 9, 6,10, 6,11, 6,12, 6,13, 6,14, 6,15, 6,
    0, 7, 1, 7, 2, 7, 3, 7, 4, 7, 5, 7, 6, 7, 7, 7, 8, 7, 9, 7,10, 7,11, 7,12, 7,13, 7,14, 7,15, 7,
    0, 8, 1, 8, 2, 8, 3, 8, 4, 8, 5, 8, 6, 8, 7, 8, 8, 8, 9, 8,10, 8,11, 8,12, 8,13, 8,14, 8,15, 8,
    0, 9, 1, 9, 2, 9, 3, 9, 4, 9, 5, 9, 6, 9, 7, 9, 8, 9, 9, 9,10, 9,11, 9,12, 9,13, 9,14, 9,15, 9,
    0,10, 1,10, 2,10, 3,10, 4,10, 5,10, 6,10, 7,10, 8,10, 9,10,10,10,11,10,12,10,13,10,14,10,15,10,
    0,11, 1,11, 2,11, 3,11, 4,11, 5,11, 6,11, 7,11, 8,11, 9,11,10,11,11,11,12,11,13,11,14,11,15,11,
    0,12, 1,12, 2,12, 3,12, 4,12, 5,12, 6,12, 7,12, 8,12, 9,12,10,12,11,12,12,12,13,12,14,12,15,12,
    0,13, 1,13, 2,13, 3,13, 4,13, 5,13, 6,13, 7,13, 8,13, 9,13,10,13,11,13,12,13,13,13,14,13,15,13,
    0,14, 1,14, 2,14, 3,14, 4,14, 5,14, 6,14, 7,14, 8,14, 9,14,10,14,11,14,12,14,13,14,14,14,15,14,
    0,15, 1,15, 2,15, 3,15, 4,15, 5,15, 6,15, 7,15, 8,15, 9,15,10,15,11,15,12,15,13,15,14,15,15,15,
};

static PALETTE_INIT( aquarius )
{
	palette_set_colors(machine, 0, aquarius_palette, sizeof(aquarius_palette) / 3);
	memcpy(colortable, aquarius_colortable, sizeof (aquarius_colortable));
}

/* Keyboard input */

INPUT_PORTS_START (aquarius)
	PORT_START	/* 0: count = 0 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("=     + NEXT") PORT_CODE(KEYCODE_EQUALS)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Del   \\") PORT_CODE(KEYCODE_BACKSPACE)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(":     * PEEK") PORT_CODE(KEYCODE_COLON)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Rtn") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(";     @ POKE") PORT_CODE(KEYCODE_QUOTE)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(".     > VAL") PORT_CODE(KEYCODE_STOP)
	PORT_BIT (0xc0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* 1: count = 1 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("-     _ FOR") PORT_CODE(KEYCODE_MINUS)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/     ^") PORT_CODE(KEYCODE_SLASH)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0     ?") PORT_CODE(KEYCODE_0)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("p     P") PORT_CODE(KEYCODE_P)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("l     L POINT") PORT_CODE(KEYCODE_L)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(",     <  STR$") PORT_CODE(KEYCODE_COMMA)
	PORT_BIT (0xc0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* 2: count = 2 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9     ) COPY") PORT_CODE(KEYCODE_9)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("o     O") PORT_CODE(KEYCODE_O)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("k     K PRESET") PORT_CODE(KEYCODE_K)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("m     M") PORT_CODE(KEYCODE_M)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("n     N RIGHT$") PORT_CODE(KEYCODE_N)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("j     J PSET") PORT_CODE(KEYCODE_J)
	PORT_BIT (0xc0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* 3: count = 3 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8     ( RETURN") PORT_CODE(KEYCODE_8)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("i     I") PORT_CODE(KEYCODE_I)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7     ' GOSUB") PORT_CODE(KEYCODE_7)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("u     U") PORT_CODE(KEYCODE_U)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("h     H") PORT_CODE(KEYCODE_H)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("b     B MID$") PORT_CODE(KEYCODE_B)
	PORT_BIT (0xc0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* 4: count = 4 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6     & ON") PORT_CODE(KEYCODE_6)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("y     Y") PORT_CODE(KEYCODE_Y)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("g     G BELL") PORT_CODE(KEYCODE_G)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("v     V LEFT$") PORT_CODE(KEYCODE_V)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("c     C STOP") PORT_CODE(KEYCODE_C)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("f     F DATA") PORT_CODE(KEYCODE_F)
	PORT_BIT (0xc0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* 5: count = 5 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5     % GOTO") PORT_CODE(KEYCODE_5)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("t     T INPUT") PORT_CODE(KEYCODE_T)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4     $ THEN") PORT_CODE(KEYCODE_4)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("r     R RETYP") PORT_CODE(KEYCODE_R)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("d     D READ") PORT_CODE(KEYCODE_D)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("x     X DELINE") PORT_CODE(KEYCODE_X)
	PORT_BIT (0xc0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* 6: count = 6 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3     # IF") PORT_CODE(KEYCODE_3)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("e     E DIM") PORT_CODE(KEYCODE_E)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("s     S STPLST") PORT_CODE(KEYCODE_S)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("z     Z CLOAD") PORT_CODE(KEYCODE_Z)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Space   CHR$") PORT_CODE(KEYCODE_SPACE)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("a     A CSAVE") PORT_CODE(KEYCODE_A)
	PORT_BIT (0xc0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* 7: count = 7 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2     \" LIST") PORT_CODE(KEYCODE_2)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("w     W REM") PORT_CODE(KEYCODE_W)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1     ! RUN") PORT_CODE(KEYCODE_1)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("q     Q") PORT_CODE(KEYCODE_Q)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Ctl") PORT_CODE(KEYCODE_LCONTROL)
	PORT_BIT (0xc0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* 8: Machine config */
	PORT_DIPNAME(0x03, 3, "RAM Size")
	PORT_DIPSETTING(0, "4 Kb")
	PORT_DIPSETTING(1, "20 Kb")
	PORT_DIPSETTING(2, "56 Kb")
INPUT_PORTS_END

/* Sound output */

static INTERRUPT_GEN( aquarius_interrupt )
{
	cpunum_set_input_line(0, 0, HOLD_LINE);
}

/* Machine definition */
static MACHINE_DRIVER_START( aquarius )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 3500000)
	MDRV_CPU_PROGRAM_MAP(aquarius_mem, 0)
	MDRV_CPU_IO_MAP(aquarius_io, 0)
	MDRV_CPU_VBLANK_INT( aquarius_interrupt, 1 )
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_RESET( aquarius )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(40 * 8, 24 * 8)
	MDRV_SCREEN_VISIBLE_AREA(0, 40 * 8 - 1, 0, 24 * 8 - 1)
	MDRV_GFXDECODE( aquarius_gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(sizeof (aquarius_palette) / 3)
	MDRV_COLORTABLE_LENGTH(sizeof (aquarius_colortable))
	MDRV_PALETTE_INIT( aquarius )

	MDRV_VIDEO_START( aquarius )
	MDRV_VIDEO_UPDATE( aquarius )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END


ROM_START(aquarius)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("aq2.rom", 0x0000, 0x2000, CRC(a2d15bcf) SHA1(ca6ef55e9ead41453efbf5062d6a60285e9661a6))
	ROM_REGION(0x0800, REGION_GFX1,0)
	ROM_LOAD("aq2.chr", 0x0000, 0x0800, BAD_DUMP CRC(0b3edeed) SHA1(d2509839386b852caddcaa89cd376be647ba1492))
ROM_END

/*		YEAR	NAME		PARENT	COMPAT	MACHINE		INPUT		INIT	CONFIG		COMPANY		FULLNAME */
COMP(	1983,	aquarius,	0,		0,		aquarius,	aquarius,	0,		NULL,		"Mattel",	"Aquarius" , 0)
