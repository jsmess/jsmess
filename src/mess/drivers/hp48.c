/******************************************************************************
 hp48
 Peter.Trauner@jk.uni-linz.ac.at May 2000
******************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "cpu/saturn/saturn.h"

#include "includes/hp48.h"

/* hp28s 
 0-0x3ffff rom
 0xc0000- 0xcffff ram also mapped at 0xd0000-0xdffff */

static ADDRESS_MAP_START(hp48_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x00000, 0xfffff) AM_ROM // configured at runtime, complexe mmu
ADDRESS_MAP_END

#define DIPS_HELPER(bit, name, keycode, r) \
   PORT_BIT(bit, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(name) PORT_CODE(keycode) PORT_CODE(r)


INPUT_PORTS_START( hp48s )
#if 1
	PORT_START
	DIPS_HELPER( 0x80, "        A", KEYCODE_A, CODE_NONE)
	DIPS_HELPER( 0x40, "        B", KEYCODE_B, CODE_NONE)
	DIPS_HELPER( 0x20, "        C", KEYCODE_C, CODE_NONE)
	DIPS_HELPER( 0x10, "        D", KEYCODE_D, CODE_NONE)
	DIPS_HELPER( 0x08, "        E", KEYCODE_E, CODE_NONE)
	DIPS_HELPER( 0x04, "        F", KEYCODE_F, CODE_NONE)
	DIPS_HELPER( 0x02, "MTH     G       PRINT", KEYCODE_G, CODE_NONE)
	DIPS_HELPER( 0x01, "PRG     H       I/0", KEYCODE_H, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x80, "CST     I       MODES", KEYCODE_I, CODE_NONE)
	DIPS_HELPER( 0x40, "VAR     J       MEMORY", KEYCODE_J, CODE_NONE)
	DIPS_HELPER( 0x20, "up      K       LIBRARY", KEYCODE_K, KEYCODE_UP)
	DIPS_HELPER( 0x10, "NXT     L       PREV", KEYCODE_L, CODE_NONE)
	DIPS_HELPER( 0x08, "'       M       UP      HOME", KEYCODE_M, CODE_NONE)
	DIPS_HELPER( 0x04, "STO     N       DEF     RCL", KEYCODE_N, CODE_NONE)
	DIPS_HELPER( 0x02, "EVAL    O       ->Q     ->NUM", KEYCODE_P, CODE_NONE)
	DIPS_HELPER( 0x01, "left    P       GRAPH", KEYCODE_P, KEYCODE_LEFT)
	PORT_START
	DIPS_HELPER( 0x80, "down    Q       REVIEW", KEYCODE_Q, KEYCODE_DOWN)
	DIPS_HELPER( 0x40, "right   R       SWAP", KEYCODE_R, KEYCODE_RIGHT)
	DIPS_HELPER( 0x20, "SIN     S       ASIN    a", KEYCODE_S, CODE_NONE)
	DIPS_HELPER( 0x10, "COS     T       ACOS    Integral", KEYCODE_T, CODE_NONE)
	DIPS_HELPER( 0x08, "TAN     U       ATAN    sum", KEYCODE_U, CODE_NONE)
	DIPS_HELPER( 0x04, "sqrt    V       square  root", KEYCODE_V, CODE_NONE)
	DIPS_HELPER( 0x02, "power   W       10^x    LOG", KEYCODE_W, CODE_NONE)
	DIPS_HELPER( 0x01, "1/x     X       e^x     LN", KEYCODE_X, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x80, "ENTER           EQUATION MATRIX", KEYCODE_ENTER, KEYCODE_ENTER_PAD)
	DIPS_HELPER( 0x40, "+/-     Y       EDIT    VISIT", KEYCODE_Y, CODE_NONE)
	DIPS_HELPER( 0x20, "EEX     Z       2D      3D", KEYCODE_Z, CODE_NONE)
	DIPS_HELPER( 0x10, "DEL             PURGE", KEYCODE_DEL, CODE_NONE)
	DIPS_HELPER( 0x08, "<--             DROP    CLR", KEYCODE_BACKSPACE, CODE_NONE)
	DIPS_HELPER( 0x04, "alpha           USR     ENTRY", KEYCODE_LSHIFT, KEYCODE_RSHIFT)
	DIPS_HELPER( 0x02, "7               SOLVE", KEYCODE_7, KEYCODE_7_PAD)
	DIPS_HELPER( 0x01, "8               PLOT", KEYCODE_8, KEYCODE_8_PAD)
	PORT_START
	DIPS_HELPER( 0x80, "9               ALGEBRA", KEYCODE_9, KEYCODE_9_PAD)
	DIPS_HELPER( 0x40, "divide          ()      #", KEYCODE_SLASH, KEYCODE_SLASH_PAD)
	DIPS_HELPER( 0x20, "orange", KEYCODE_LCONTROL, KEYCODE_RCONTROL)
	DIPS_HELPER( 0x10, "4               TIME", KEYCODE_4, KEYCODE_4_PAD)
	DIPS_HELPER( 0x08, "5               STAT", KEYCODE_5, KEYCODE_5_PAD)
	DIPS_HELPER( 0x04, "6               UNITS", KEYCODE_6, KEYCODE_6_PAD)
	DIPS_HELPER( 0x02, "*               []      _", KEYCODE_BACKSPACE, KEYCODE_ASTERISK)
	DIPS_HELPER( 0x01, "blue", KEYCODE_LALT, KEYCODE_RALT)
	PORT_START
	DIPS_HELPER( 0x80, "1               RAD     POLAR", KEYCODE_1, KEYCODE_1_PAD)
	DIPS_HELPER( 0x40, "2               STACK   ARG", KEYCODE_2, KEYCODE_2_PAD)
	DIPS_HELPER( 0x20, "3               CMD     MENU", KEYCODE_3, KEYCODE_3_PAD)
	DIPS_HELPER( 0x10, "-               <<>>    \"\"", KEYCODE_MINUS, KEYCODE_MINUS_PAD)
	DIPS_HELPER( 0x08, "ON              CONT    OFF", KEYCODE_F1, CODE_NONE)
	DIPS_HELPER( 0x04, "0               =       ->", KEYCODE_0, KEYCODE_0_PAD)
	DIPS_HELPER( 0x02, ".               ,       enter", KEYCODE_STOP, KEYCODE_DEL_PAD)
	DIPS_HELPER( 0x01, "SPC             Pi      angle", KEYCODE_SPACE, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x80, "+               {}      ::", KEYCODE_EQUALS, KEYCODE_PLUS_PAD)

#else
	PORT_START
	PORT_BIT ( 0x20, 0,     IPT_UNUSED )
	DIPS_HELPER( 0x10, "B", KEYCODE_F2)
	DIPS_HELPER( 0x08, "C", KEYCODE_F3)
	DIPS_HELPER( 0x04, "D", KEYCODE_F4)
	DIPS_HELPER( 0x02, "E", KEYCODE_F5)
	DIPS_HELPER( 0x01, "F", KEYCODE_F6)
	PORT_START
	PORT_BIT ( 0x20, 0,     IPT_UNUSED )
	DIPS_HELPER( 0x10, "PRG", KEYCODE_A)
	DIPS_HELPER( 0x08, "CST", KEYCODE_B)
	DIPS_HELPER( 0x04, "VAR", KEYCODE_C)
	DIPS_HELPER( 0x02, "up", KEYCODE_UP)
	DIPS_HELPER( 0x01, "NXT", KEYCODE_D)
	PORT_START
	PORT_BIT ( 0x20, 0,     IPT_UNUSED )
	DIPS_HELPER( 0x10, "STO", KEYCODE_E)
	DIPS_HELPER( 0x08, "EVL", KEYCODE_F)
	DIPS_HELPER( 0x04, "left", KEYCODE_LEFT)
	DIPS_HELPER( 0x02, "down", KEYCODE_DOWN)
	DIPS_HELPER( 0x01, "right", KEYCODE_RIGHT)
	PORT_START
	PORT_BIT ( 0x20, 0,     IPT_UNUSED )
	DIPS_HELPER( 0x10, "COS", CODE_DEFAULT)
	DIPS_HELPER( 0x08, "TAN", KEYCODE_H)
	DIPS_HELPER( 0x04, "sqt", KEYCODE_LEFT)
	DIPS_HELPER( 0x02, "pwr", KEYCODE_DOWN)
	DIPS_HELPER( 0x01, "inv", KEYCODE_RIGHT)
	PORT_START
	DIPS_HELPER( 0x20, DEF_STR( On) , KEYCODE_NUMLOCK)
	DIPS_HELPER( 0x10, "ENT", KEYCODE_I)
	DIPS_HELPER( 0x08, "+/-", KEYCODE_MINUS)
	DIPS_HELPER( 0x04, "EEX", KEYCODE_LEFT)
	DIPS_HELPER( 0x02, "DEL", KEYCODE_BACKSPACE)
	DIPS_HELPER( 0x01, "<==", KEYCODE_ENTER_PAD)	
	PORT_START
	DIPS_HELPER( 0x20, "alp", KEYCODE_LALT)
	DIPS_HELPER( 0x10, "SIN", KEYCODE_J)
	DIPS_HELPER( 0x08, "7", KEYCODE_7_PAD)
	DIPS_HELPER( 0x04, "8", KEYCODE_8_PAD)
	DIPS_HELPER( 0x02, "9", KEYCODE_9_PAD)
	DIPS_HELPER( 0x01, "/", KEYCODE_SLASH_PAD)
	PORT_START
	DIPS_HELPER( 0x20, "yel", KEYCODE_LSHIFT)
	DIPS_HELPER( 0x10, "MTH", KEYCODE_K)
	DIPS_HELPER( 0x08, "4", KEYCODE_4_PAD)
	DIPS_HELPER( 0x04, "5", KEYCODE_5_PAD)
	DIPS_HELPER( 0x02, "6", KEYCODE_6_PAD)
	DIPS_HELPER( 0x01, "*", KEYCODE_ASTERISK)
	PORT_START
	DIPS_HELPER( 0x20, "blu", KEYCODE_LSHIFT)
	DIPS_HELPER( 0x10, "A", KEYCODE_F1)
	DIPS_HELPER( 0x08, "1", KEYCODE_1_PAD)
	DIPS_HELPER( 0x04, "2", KEYCODE_2_PAD)
	DIPS_HELPER( 0x02, "3", KEYCODE_3_PAD)
	DIPS_HELPER( 0x01, "-", KEYCODE_MINUS_PAD)
	PORT_START
	PORT_BIT ( 0x20, 0,     IPT_UNUSED )
	DIPS_HELPER( 0x10, "'", KEYCODE_L)
	DIPS_HELPER( 0x08, "0", KEYCODE_0_PAD)
	DIPS_HELPER( 0x04, ".", KEYCODE_DEL_PAD)
	DIPS_HELPER( 0x02, "SPC", KEYCODE_SPACE)
	DIPS_HELPER( 0x01, "+", KEYCODE_PLUS_PAD)
#endif
INPUT_PORTS_END


static gfx_layout hp48_charlayout =
{
        2,16,
        256,                                    /* 256 characters */
        1,                      /* 1 bits per pixel */
        { 0 },                  /* no bitplanes; 1 bit per pixel */
        /* x offsets */
        { 0,0 },
        /* y offsets */
        {
			7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1, 0, 0
        },
        1*8
};

static gfx_decode hp48_gfxdecodeinfo[] = {
	{ REGION_GFX1, 0x0000, &hp48_charlayout,                     0, 8 },
    { -1 } /* end of array */
};

static SATURN_CONFIG config={ 
	hp48_out, hp48_in, 
	hp48_mem_reset, hp48_mem_config, hp48_mem_unconfig, hp48_mem_id,
	hp48_crc
};


static MACHINE_DRIVER_START( hp48s )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", SATURN, 4000000)        /* 2 MHz */
	MDRV_CPU_PROGRAM_MAP(hp48_mem, 0)
	MDRV_CPU_CONFIG(config)
	MDRV_SCREEN_REFRESH_RATE(64)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_RESET( hp48 )

	/* video hardware (well, actually there was no video ;) */
	/* scanned with 300 dpi, scaled x 55%, y 55% for perfect display 2x2 pixels */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(339, 775)
	MDRV_SCREEN_VISIBLE_AREA(0, 339-1, 0, 775-1)
	MDRV_GFXDECODE( hp48_gfxdecodeinfo )
	MDRV_PALETTE_LENGTH( 248 )
	MDRV_COLORTABLE_LENGTH( sizeof (hp48_colortable) / sizeof(hp48_colortable[0][0]) )
	MDRV_PALETTE_INIT( hp48 )

	MDRV_VIDEO_START( hp48 )
	MDRV_VIDEO_UPDATE( hp48 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( hp48g )
	MDRV_IMPORT_FROM( hp48s )
	MDRV_CPU_REPLACE( "main", SATURN, 8000000 )		/* 4 MHz */
	MDRV_CPU_PROGRAM_MAP(hp48_mem, 0)
	MDRV_CPU_CONFIG(config)
MACHINE_DRIVER_END

ROM_START(hp48s)
	ROM_REGION(0x1c0000,REGION_CPU1, 0)
	/* version at 0x7fff0 little endian 6 characters */
	/* 0x3fff8 in byte wide rom */
//	ROM_LOAD("sxrom-a", 0x00000, 0x40000, CRC(a87696c7) SHA1(3271b103ad99254d069e20171beb418ace72cc90))
//	ROM_LOAD("sxrom-b", 0x00000, 0x40000, CRC(034f6ce4) SHA1(acd256f2efee868ce402008f4131d94b312e60bc))
//	ROM_LOAD("sxrom-c", 0x00000, 0x40000, CRC(a9a0279d) SHA1(fee852d43ae6941d07a9d0d31f37e68e4f9051b1))
//	ROM_LOAD("sxrom-d", 0x00000, 0x40000, CRC(6e71244e) SHA1(893f29abd17434db7da986b27dc10c48f458d3aa))
//	ROM_LOAD("sxrom-e", 0x00000, 0x40000, CRC(704ffa08) SHA1(0d498d135bf729c1d775cce522528837729e2e94))
	ROM_LOAD("sxrom-e", 0x00000, 0x40000, CRC(d4f1390b) SHA1(f6be01a05aedefa8a285d480e33458447cf2bdaa)) // differences only in the hardware window
//	ROM_LOAD("rom.sx", 0x00000, 0x40000, CRC(5619ccaf)) //revision E bad dump
//	ROM_LOAD("sxrom-j", 0x00000, 0x40000, CRC(1a6378ef) SHA1(5235f5379f1fd7edfe9bb6bf466b60d279163e73))
	ROM_REGION(0x100,REGION_GFX1,0)
ROM_END

ROM_START(hp48g)
	ROM_REGION(0x580000,REGION_CPU1, 0)
	/* version at 0x7ffbf little endian 6 characters */
//	ROM_LOAD("gxrom-k", 0x00000, 0x80000, CRC(bdd5d2ee) SHA1(afa1498238e991b1e3d07fb8b4c227b115f7bcc1))
//	ROM_LOAD("gxrom-l", 0x00000, 0x80000, CRC(70958e6b) SHA1(8eebac69ff804086247b989bf320e57a2d8a59a7))
//	ROM_LOAD("gxrom-m", 0x00000, 0x80000, CRC(e21a09e4) SHA1(09932d543594e459eeb94a79654168cd15e79a87))
//	ROM_LOAD("gxrom-p", 0x00000, 0x80000, CRC(27f90428) SHA1(b98312716ff81b9b439002c2ec7a07b9808440fb))
	ROM_LOAD("gxrom-r", 0x00000, 0x80000, CRC(00ee1a62))
//	ROM_LOAD("rom.gx", 0x00000, 0x80000, CRC(d6bb68c5)) //revision R bad dump
	ROM_REGION(0x100,REGION_GFX1,0)
ROM_END

/*   YEAR	NAME	PARENT	COMPAT	MACHINE	INPUT	INIT	CONFIG	MONITOR	COMPANY   FULLNAME */
COMP(1989,	hp48s,	0,		0,		hp48s,	hp48s,	hp48s,	NULL,	"Hewlett Packard",  "HP48S/SX", 0)
COMP(1993,	hp48g,	0,		0,		hp48g,	hp48s,	hp48g,	NULL,	"Hewlett Packard",  "HP48G/GX", 0)
