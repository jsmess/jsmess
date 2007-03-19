/************************************************************************
Nascom Memory map

	CPU: z80
		0000-03ff	ROM	(Nascom1 Monitor)
		0400-07ff	ROM	(Nascom2 Monitor extension)
		0800-0bff	RAM (Screen)
		0c00-0c7f	RAM (OS workspace)
		0c80-0cff	RAM (extended OS workspace)
		0d00-0f7f	RAM (Firmware workspace)
		0f80-0fff	RAM (Stack space)
		1000-8fff	RAM (User space)
		9000-97ff	RAM (Programmable graphics RAM/User space)
		9800-afff	RAM (Colour graphics RAM/User space)
		b000-b7ff	ROM (OS extensions)
		b800-bfff	ROM (WP/Naspen software)
		c000-cfff	ROM (Disassembler/colour graphics software)
		d000-dfff	ROM (Assembler/Basic extensions)
		e000-ffff	ROM (Nascom2 Basic)

	Interrupts:

	Ports:
		OUT (00)	0:	Increment keyboard scan
					1:	Reset keyboard scan
					2:
					3:	Read from cassette
					4:
					5:
					6:
					7:
		IN  (00)	Read keyboard
		OUT (01)	Write to cassette/serial
		IN  (01)	Read from cassette/serial
		OUT (02)	Unused
		IN  (02)	?

	Monitors:
		Nasbug1		1K	Original Nascom1
		Nasbug2     	1K
		Nasbug3     Probably non existing
		Nasbug4		2K
		Nassys1		2K	Original Nascom2
		Nassys2     Probably non existing
		Nassys3		2K
		Nassys4		2K
		T4			2K

************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "video/generic.h"
#include "includes/nascom1.h"
#include "devices/cartslot.h"

/* Memory w/r functions */
ADDRESS_MAP_START( nascom1_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x07ff) AM_ROM
	AM_RANGE(0x0800, 0x0bff) AM_READWRITE( videoram_r, videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x0c00, 0x0fff) AM_RAM
	AM_RANGE(0x1000, 0x13ff) AM_RAM	/* 1Kb */
	AM_RANGE(0x1400, 0x4fff) AM_RAM	/* 16Kb */
	AM_RANGE(0x5000, 0x8fff) AM_RAM	/* 32Kb */
	AM_RANGE(0x9000, 0xafff) AM_RAM	/* 40Kb */
	AM_RANGE(0xb000, 0xffff) AM_ROM
ADDRESS_MAP_END

/* port i/o functions */
ADDRESS_MAP_START( nascom1_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) ) 
	AM_RANGE(0x00, 0x00) AM_READWRITE( nascom1_port_00_r, nascom1_port_00_w )
	AM_RANGE(0x01, 0x01) AM_READWRITE( nascom1_port_01_r, nascom1_port_01_w )
	AM_RANGE(0x02, 0x02) AM_READ( nascom1_port_02_r )
ADDRESS_MAP_END

/* graphics output */

static gfx_layout nascom1_charlayout =
{
	8, 16,
	128,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8,10*8,11*8,12*8,13*8,14*8,15*8 },
	8 * 16
};

static gfx_decode nascom1_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &nascom1_charlayout, 0, 1},
	{-1}
};

static gfx_layout nascom2_charlayout =
{
	8, 14,
	256,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8,  3*8,  4*8,  5*8,  6*8,
	  7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8 },
	8 * 16
};

static gfx_decode nascom2_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &nascom2_charlayout, 0, 1},
	{-1}
};

static	unsigned	char	nascom1_palette[] =
{
	0x00, 0x00, 0x00,	/* Black */
	0xff, 0xff, 0xff	/* White */
};

static	unsigned	short	nascom1_colortable[] =
{
	0, 1
};

static PALETTE_INIT( nascom1 )
{
	palette_set_colors(machine, 0, nascom1_palette, sizeof(nascom1_palette) / 3);
	memcpy(colortable, nascom1_colortable, sizeof (nascom1_colortable));
}

/* Keyboard input */

INPUT_PORTS_START (nascom1)
	PORT_START	/* 0: count = 0 */
	PORT_BIT(0x6f, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_RSHIFT)

	PORT_START	/* 1: count = 1 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("up") PORT_CODE(KEYCODE_UP)

	PORT_START	/* 2: count = 2 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("left") PORT_CODE(KEYCODE_LEFT)

	PORT_START	/* 3: count = 3 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("down") PORT_CODE(KEYCODE_DOWN)

	PORT_START	/* 4: count = 4 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(",") PORT_CODE(KEYCODE_COMMA)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("right") PORT_CODE(KEYCODE_RIGHT)

	PORT_START	/* 5: count = 5 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(";") PORT_CODE(KEYCODE_COLON)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(".") PORT_CODE(KEYCODE_STOP)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* 6: count = 6 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(":") PORT_CODE(KEYCODE_TILDE)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/") PORT_CODE(KEYCODE_SLASH)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* 7: count = 7 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* 8: count = 8 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("BACKSPACE") PORT_CODE(KEYCODE_BACKSPACE)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS)
	PORT_BIT(0x58, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("@") PORT_CODE(KEYCODE_QUOTE)
	PORT_START	/* 9: Machine config */
	PORT_DIPNAME(0x03, 3, "RAM Size")
	PORT_DIPSETTING(0, "1Kb")
	PORT_DIPSETTING(1, "16Kb")
	PORT_DIPSETTING(2, "32Kb")
	PORT_DIPSETTING(3, "40Kb")
INPUT_PORTS_END

/* Sound output */

static INTERRUPT_GEN( nascom_interrupt )
{
	cpunum_set_input_line(0, 0, HOLD_LINE);
}

/* Machine definition */
static MACHINE_DRIVER_START( nascom1 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 1000000)
	MDRV_CPU_PROGRAM_MAP(nascom1_mem, 0)
	MDRV_CPU_IO_MAP(nascom1_io, 0)
	MDRV_CPU_VBLANK_INT(nascom_interrupt, 1)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(TIME_IN_USEC(2500))
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_RESET( nascom1 )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(48 * 8, 16 * 16)
	MDRV_SCREEN_VISIBLE_AREA(0, 48 * 8 - 1, 0, 16 * 16 - 1)
	MDRV_GFXDECODE( nascom1_gfxdecodeinfo )
	MDRV_PALETTE_LENGTH( sizeof (nascom1_palette) / 3 )
	MDRV_COLORTABLE_LENGTH( sizeof (nascom1_colortable) )
	MDRV_PALETTE_INIT( nascom1 )

	MDRV_VIDEO_START( generic )
	MDRV_VIDEO_UPDATE( nascom1 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( nascom2 )
	MDRV_IMPORT_FROM( nascom1 )
	MDRV_CPU_REPLACE( "main", Z80, 2000000 )
	MDRV_SCREEN_SIZE(48 * 8, 16 * 14)
	MDRV_SCREEN_VISIBLE_AREA(0, 48 * 8 - 1, 0, 16 * 14 - 1)
	MDRV_GFXDECODE( nascom2_gfxdecodeinfo )
	MDRV_VIDEO_UPDATE( nascom2 )
MACHINE_DRIVER_END

ROM_START(nascom1)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("nasbugt1.rom", 0x0000, 0x0400, CRC(8ea07054) SHA1(3f9a8632826003d6ea59d2418674d0fb09b83a4c))
	ROM_REGION(0x0800, REGION_GFX1,0)
	ROM_LOAD("nascom1.chr", 0x0000, 0x0800, CRC(33e92a04) SHA1(be6e1cc80e7f95a032759f7df19a43c27ff93a52))
ROM_END

ROM_START(nascom1a)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("nasbugt2.rom", 0x0000, 0x0400, CRC(e371b58a) SHA1(485b20a560b587cf9bb4208ba203b12b3841689b))
	ROM_REGION(0x0800, REGION_GFX1,0)
	ROM_LOAD("nascom1.chr", 0x0000, 0x0800, CRC(33e92a04) SHA1(be6e1cc80e7f95a032759f7df19a43c27ff93a52))
ROM_END

ROM_START(nascom1b)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("nasbugt4.rom", 0x0000, 0x0800, CRC(f391df68) SHA1(00218652927afc6360c57e77d6a4fd32d4e34566))
	ROM_REGION(0x0800, REGION_GFX1,0)
	ROM_LOAD("nascom1.chr", 0x0000, 0x0800, CRC(33e92a04) SHA1(be6e1cc80e7f95a032759f7df19a43c27ff93a52))
ROM_END

ROM_START(nascom2)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("nassys1.rom", 0x0000, 0x0800, CRC(b6300716) SHA1(29da7d462ba3f569f70ed3ecd93b981f81c7adfa))
	ROM_LOAD("basic.rom", 0xe000, 0x2000, CRC(5cb5197b) SHA1(c41669c2b6d6dea808741a2738426d97bccc9b07))
	ROM_REGION(0x1000, REGION_GFX1,0)
	ROM_LOAD("nascom1.chr", 0x0000, 0x0800, CRC(33e92a04) SHA1(be6e1cc80e7f95a032759f7df19a43c27ff93a52))
	ROM_LOAD("nasgra.chr", 0x0800, 0x0800, CRC(2bc09d32) SHA1(d384297e9b02cbcb283c020da51b3032ff62b1ae))
ROM_END

ROM_START(nascom2a)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("nassys3.rom", 0x0000, 0x0800, CRC(3da17373) SHA1(5fbda15765f04e4cd08cf95c8d82ce217889f240))
	ROM_LOAD("basic.rom", 0xe000, 0x2000, CRC(5cb5197b) SHA1(c41669c2b6d6dea808741a2738426d97bccc9b07))
	ROM_REGION(0x1000, REGION_GFX1,0)
	ROM_LOAD("nascom1.chr", 0x0000, 0x0800, CRC(33e92a04) SHA1(be6e1cc80e7f95a032759f7df19a43c27ff93a52))
	ROM_LOAD("nasgra.chr", 0x0800, 0x0800, CRC(2bc09d32) SHA1(d384297e9b02cbcb283c020da51b3032ff62b1ae))
ROM_END

SYSTEM_CONFIG_START(nascom)
#ifdef CART
	CONFIG_DEVICE_CARTSLOT_REQ(1, "nas\0bin\0", nascom1_init_cartridge, NULL, NULL)
#endif
SYSTEM_CONFIG_END

static void nascom1_cassette_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:							info->i = IO_CASSETTE; break;
		case DEVINFO_INT_READABLE:						info->i = 1; break;
		case DEVINFO_INT_WRITEABLE:						info->i = 0; break;
		case DEVINFO_INT_CREATABLE:						info->i = 0; break;
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_nascom1_cassette; break;
		case DEVINFO_PTR_UNLOAD:						info->unload = device_unload_nascom1_cassette; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "nas,bin"); break;
	}
}

SYSTEM_CONFIG_START(nascom1)
	CONFIG_IMPORT_FROM(nascom)
	CONFIG_DEVICE(nascom1_cassette_getinfo)
SYSTEM_CONFIG_END

static void nascom2_cassette_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:							info->i = IO_CASSETTE; break;
		case DEVINFO_INT_READABLE:						info->i = 1; break;
		case DEVINFO_INT_WRITEABLE:						info->i = 0; break;
		case DEVINFO_INT_CREATABLE:						info->i = 0; break;
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_nascom1_cassette; break;
		case DEVINFO_PTR_UNLOAD:						info->unload = device_unload_nascom1_cassette; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "cas,nas,bin"); break;
	}
}

SYSTEM_CONFIG_START(nascom2)
	CONFIG_IMPORT_FROM(nascom)
	CONFIG_DEVICE(nascom2_cassette_getinfo)
SYSTEM_CONFIG_END

/*	  YEAR	NAME		PARENT		COMPAT	MACHINE		INPUT		INIT	CONFIG		COMPANY		FULLNAME */
COMP( 1978,	nascom1,	0,			0,		nascom1,	nascom1,	0,		nascom1,	"Nascom Microcomputers",	"Nascom 1 (NasBug T1)" , 0)
COMP( 1978,	nascom1a,	nascom1,	0,		nascom1,	nascom1,	0,		nascom1,	"Nascom Microcomputers",	"Nascom 1 (NasBug T2)" , 0)
COMP( 1978,	nascom1b,	nascom1,	0,		nascom1,	nascom1,	0,		nascom1,	"Nascom Microcomputers",	"Nascom 1 (NasBug T4)" , 0)
COMP( 1979,	nascom2,	nascom1,	0,		nascom2,	nascom1,	0,		nascom2,	"Nascom Microcomputers",	"Nascom 2 (NasSys 1)" , 0)
COMP( 1979,	nascom2a,	nascom1,	0,		nascom2,	nascom1,	0,		nascom2,	"Nascom Microcomputers",	"Nascom 2 (NasSys 3)" , 0)
