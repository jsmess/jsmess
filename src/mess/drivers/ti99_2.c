/*
  Experimental ti99/2 driver

TODO :
  * find a TI99/2 ROM dump (some TI99/2 ARE in private hands)
  * test the driver !
  * understand the "viden" pin
  * implement cassette
  * implement Hex-Bus

  Raphael Nabet (who really has too much time to waste), december 1999, 2000
*/

/*
  TI99/2 facts :

References :
* TI99/2 main logic board schematics, 83/03/28-30 (on ftp.whtech.com, or just ask me)
  (Thanks to Charles Good for posting this)

general :
* prototypes in 1983
* uses a 10.7MHz TMS9995 CPU, with the following features :
  - 8-bit data bus
  - 256 bytes 16-bit RAM (0xff00-0xff0b & 0xfffc-0xffff)
  - only available int lines are INT4 (used by vdp), INT1*, and NMI* (both free for extension)
  - on-chip decrementer (0xfffa-0xfffb)
  - Unlike tms9900, CRU address range is full 0x0000-0xFFFE (A0 is not used as address).
    This is possible because tms9995 uses d0-d2 instead of the address MSBits to support external
    opcodes.
  - quite more efficient than tms9900, and a few additionnal instructions and features
* 24 or 32kb ROM (16kb plain (1kb of which used by vdp), 16kb split into 2 8kb pages)
* 4kb 8-bit RAM, 256 bytes 16-bit RAM
* custom vdp shares CPU RAM/ROM.  The display is quite alike to tms9928 graphics mode, except
  that colors are a static B&W, and no sprite is displayed. The config (particularily the
  table base addresses) cannot be changed.  Since TI located the pattern generator table in
  ROM, we cannot even redefine the char patterns (unless you insert a custom cartidge which
  overrides the system ROMs).  VBL int triggers int4 on tms9995.
* CRU is handled by one single custom chip, so the schematics don't show many details :-( .
* I/O :
  - 48-key keyboard.  Same as TI99/4a, without alpha lock, and with an additional break key.
    Note that the hardware can make the difference between the two shift keys.
  - cassette I/O (one unit)
  - ALC bus (must be another name for Hex-Bus)
* 60-pin expansion/cartidge port on the back

memory map :
* 0x0000-0x4000 : system ROM (0x1C00-0x2000 (?) : char ROM used by vdp)
* 0x4000-0x6000 : system ROM, mapped to either of two distinct 8kb pages according to the S0
  bit from the keyboard interface (!), which is used to select the first key row.
  [only on second-generation TI99/2 protos, first generation protos only had 24kb of ROM]
* 0x6000-0xE000 : free for expansion
* 0xE000-0xF000 : 8-bit "system" RAM (0xEC00-0xEF00 used by vdp)
* 0xF000-0xF0FB : 16-bit processor RAM (on tms9995)
* 0xF0FC-0xFFF9 : free for expansion
* 0xFFFA-0xFFFB : tms9995 internal decrementer
* 0xFFFC-0xFFFF : 16-bit processor RAM (provides the NMI vector)

CRU map :
* 0x0000-0x1EFC : reserved
* 0x1EE0-0x1EFE : tms9995 flag register
* 0x1F00-0x1FD8 : reserved
* 0x1FDA : tms9995 MID flag
* 0x1FDC-0x1FFF : reserved
* 0x2000-0xE000 : unaffected
* 0xE400-0xE40E : keyboard I/O (8 bits input, either 3 or 6 bit output)
* 0xE80C : cassette I/O
* 0xE80A : ALC BAV
* 0xE808 : ALC HSK
* 0xE800-0xE808 : ALC data (I/O)
* 0xE80E : video enable (probably input - seems to come from the VDP, and is present on the
  expansion connector)
* 0xF000-0xFFFE : reserved
Note that only A15-A11 and A3-A1 (A15 = MSB, A0 = LSB) are decoded in the console, so the keyboard
is actually mapped to 0xE000-0xE7FE, and other I/O bits to 0xE800-0xEFFE.
Also, ti99/2 does not support external instructions better than ti99/4(a).  This is crazy, it
would just have taken three extra tracks on the main board and a OR gate in an ASIC.
*/

#include "driver.h"
#include "video/generic.h"
#include "machine/tms9901.h"
#include "cpu/tms9900/tms9900.h"

static int ROM_paged;

static DRIVER_INIT( ti99_2_24 )
{
	/* no ROM paging */
	ROM_paged = 0;
}

static DRIVER_INIT( ti99_2_32 )
{
	/* ROM paging enabled */
	ROM_paged = 1;
}

#define TI99_2_32_ROMPAGE0 (memory_region(REGION_CPU1)+0x4000)
#define TI99_2_32_ROMPAGE1 (memory_region(REGION_CPU1)+0x10000)

static MACHINE_RESET( ti99_2 )
{
	if (! ROM_paged)
		memory_set_bankptr(1, memory_region(REGION_CPU1)+0x4000);
	else
		memory_set_bankptr(1, TI99_2_32_ROMPAGE0);
}

static void ti99_2_vblank_interrupt(void)
{
	/* We trigger a level-4 interrupt.  The PULSE_LINE is a mere guess. */
	cpunum_set_input_line(0, 1, PULSE_LINE);
}


/*
  TI99/2 vdp emulation.

  Things could not be simpler.
  We display 24 rows and 32 columns of characters.  Each 8*8 pixel character pattern is defined
  in a 128-entry table located in ROM.  Character code for each screen position are stored
  sequentially in RAM.  Colors are a fixed Black on White.

	There is an EOL character that blanks the end of the current line, so that
	the CPU can get more bus time.
*/

static unsigned char ti99_2_palette[] =
{
	255, 255, 255,
	0, 0, 0
};

static unsigned short ti99_2_colortable[] =
{
	0, 1
};

#define TI99_2_PALETTE_SIZE sizeof(ti99_2_palette)/3
#define TI99_2_COLORTABLE_SIZE sizeof(ti99_2_colortable)/2

static PALETTE_INIT(ti99_2)
{
	palette_set_colors(machine, 0, ti99_2_palette, TI99_2_PALETTE_SIZE);
	memcpy(colortable, & ti99_2_colortable, sizeof(ti99_2_colortable));
}

static VIDEO_START(ti99_2)
{
	videoram_size = 768;

	return video_start_generic(machine);
}

#define ti99_2_video_w videoram_w

static VIDEO_UPDATE(ti99_2)
{
	int i, sx, sy;


	sx = sy = 0;

	for (i = 0; i < 768; i++)
	{
		if (dirtybuffer[i])
		{
			dirtybuffer[i] = 0;

			/* Is the char code masked or not ??? */
			drawgfx(tmpbitmap, Machine->gfx[0], videoram[i] & 0x7F, 0,
			          0, 0, sx, sy, &Machine->screen[0].visarea, TRANSPARENCY_NONE, 0);
		}

		sx += 8;
		if (sx == 256)
		{
			sx = 0;
			sy += 8;
		}
	}

	copybitmap(bitmap, tmpbitmap, 0, 0, 0, 0, &Machine->screen[0].visarea, TRANSPARENCY_NONE, 0);
	return 0;
}

static gfx_layout ti99_2_charlayout =
{
	8,8,        /* 8 x 8 characters */
	128,        /* 128 characters */
	1,          /* 1 bits per pixel */
	{ 0 },      /* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0,1,2,3,4,5,6,7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, },
	8*8         /* every char takes 8 bytes */
};

static gfx_decode gfxdecodeinfo[] =
{
	{ REGION_CPU1, 0x1c00, & ti99_2_charlayout, 0, 0 },
	{ -1 }    /* end of array */
};


/*
  Memory map - see description above
*/

static ADDRESS_MAP_START(ti99_2_memmap, ADDRESS_SPACE_PROGRAM, 8)

	AM_RANGE(0x0000, 0x3fff) AM_READWRITE(MRA8_ROM, MWA8_ROM)		/*system ROM*/
	AM_RANGE(0x4000, 0x5fff) AM_READWRITE(MRA8_BANK1, MWA8_BANK1)	/*system ROM, banked on 32kb ROMs protos*/
	AM_RANGE(0x6000, 0xdfff) AM_READWRITE(MRA8_NOP, MWA8_NOP)		/*free for expansion*/
	AM_RANGE(0xe000, 0xebff) AM_READWRITE(MRA8_RAM, MWA8_RAM)		/*system RAM*/
	AM_RANGE(0xec00, 0xeeff) AM_READWRITE(MRA8_RAM, ti99_2_video_w) AM_BASE(& videoram)	/*system RAM: used for video*/
	AM_RANGE(0xef00, 0xefff) AM_READWRITE(MRA8_RAM, MWA8_RAM)		/*system RAM*/
	AM_RANGE(0xf000, 0xffff) AM_READWRITE(MRA8_NOP, MWA8_NOP)		/*free for expansion (and internal processor RAM)*/

ADDRESS_MAP_END


/*
  CRU map - see description above
*/

/* current keyboard row */
static int KeyRow = 0;

/* write the current keyboard row */
static WRITE8_HANDLER ( ti99_2_write_kbd )
{
	offset &= 0x7;  /* other address lines are not decoded */

	if (offset <= 2)
	{
		/* this implementation is just a guess */
		if (data)
			KeyRow |= 1 << offset;
		else
			KeyRow &= ~ (1 << offset);
	}
	/* now, we handle ROM paging */
	if (ROM_paged)
	{	/* if we have paged ROMs, page according to S0 keyboard interface line */
		memory_set_bankptr(1, (KeyRow == 0) ? TI99_2_32_ROMPAGE1 : TI99_2_32_ROMPAGE0);
	}
}

static WRITE8_HANDLER ( ti99_2_write_misc_cru )
{
	offset &= 0x7;  /* other address lines are not decoded */

	switch (offset)
	{
	case 0:
	case 1:
	case 2:
	case 3:
		/* ALC I/O */
		break;
	case 4:
		/* ALC HSK */
		break;
	case 5:
		/* ALC BAV */
		break;
	case 6:
		/* cassette output */
		break;
	case 7:
		/* video enable */
		break;
	}
}

static ADDRESS_MAP_START(ti99_2_writecru, ADDRESS_SPACE_IO, 8)

	AM_RANGE(0x7000, 0x73ff) AM_WRITE(ti99_2_write_kbd)
	AM_RANGE(0x7400, 0x77ff) AM_WRITE(ti99_2_write_misc_cru)

ADDRESS_MAP_END

/* read keys in the current row */
static  READ8_HANDLER ( ti99_2_read_kbd )
{
	return readinputport(KeyRow);
}

static  READ8_HANDLER ( ti99_2_read_misc_cru )
{
	return 0;
}

static ADDRESS_MAP_START(ti99_2_readcru, ADDRESS_SPACE_IO, 8)

	AM_RANGE(0x0E00, 0x0E7f) AM_READ(ti99_2_read_kbd)
	AM_RANGE(0x0E80, 0x0Eff) AM_READ(ti99_2_read_misc_cru)

ADDRESS_MAP_END


/* ti99/2 : 54-key keyboard */
INPUT_PORTS_START(ti99_2)

	PORT_START    /* col 0 */
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1 ! DEL") PORT_CODE(KEYCODE_1)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2 @ INS") PORT_CODE(KEYCODE_2)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3 #") PORT_CODE(KEYCODE_3)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4 $ CLEAR") PORT_CODE(KEYCODE_4)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5 % BEGIN") PORT_CODE(KEYCODE_5)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6 ^ PROC'D") PORT_CODE(KEYCODE_6)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7 & AID") PORT_CODE(KEYCODE_7)
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8 * REDO") PORT_CODE(KEYCODE_8)

	PORT_START    /* col 1 */
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("q Q") PORT_CODE(KEYCODE_Q)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("w W ~") PORT_CODE(KEYCODE_W)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("e E (UP)") PORT_CODE(KEYCODE_E)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("r R [") PORT_CODE(KEYCODE_R)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("t T ]") PORT_CODE(KEYCODE_T)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("y Y") PORT_CODE(KEYCODE_Y)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("i I ?") PORT_CODE(KEYCODE_I)
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9 ( BACK") PORT_CODE(KEYCODE_9)

	PORT_START    /* col 2 */
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("a A") PORT_CODE(KEYCODE_A)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("s S (LEFT)") PORT_CODE(KEYCODE_S)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("d D (RIGHT)") PORT_CODE(KEYCODE_D)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("f F {") PORT_CODE(KEYCODE_F)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("h H") PORT_CODE(KEYCODE_H)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("u U _") PORT_CODE(KEYCODE_U)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("o O '") PORT_CODE(KEYCODE_O)
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0 )") PORT_CODE(KEYCODE_0)

	PORT_START    /* col 3 */
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("z Z \\") PORT_CODE(KEYCODE_Z)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("x X (DOWN)") PORT_CODE(KEYCODE_X)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("c C `") PORT_CODE(KEYCODE_C)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("g G }") PORT_CODE(KEYCODE_G)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("j J") PORT_CODE(KEYCODE_J)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("k K") PORT_CODE(KEYCODE_K)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("p P \"") PORT_CODE(KEYCODE_P)
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("= + QUIT") PORT_CODE(KEYCODE_EQUALS)

	PORT_START    /* col 4 */
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT/*KEYCODE_CAPSLOCK*/)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("v V") PORT_CODE(KEYCODE_V)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("n N") PORT_CODE(KEYCODE_N)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(", <") PORT_CODE(KEYCODE_COMMA)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("l L") PORT_CODE(KEYCODE_L)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("; :") PORT_CODE(KEYCODE_COLON)
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/ -") PORT_CODE(KEYCODE_SLASH)

	PORT_START    /* col 5 */
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("BREAK") PORT_CODE(KEYCODE_ESC)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(SPACE)") PORT_CODE(KEYCODE_SPACE)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("b B") PORT_CODE(KEYCODE_B)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("m M") PORT_CODE(KEYCODE_M)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(". >") PORT_CODE(KEYCODE_STOP)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("FCTN") PORT_CODE(KEYCODE_LALT)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_RSHIFT)
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER)

	PORT_START    /* col 6 */
		PORT_BIT(0xFF, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START    /* col 7 */
		PORT_BIT(0xFF, IP_ACTIVE_LOW, IPT_UNUSED)

INPUT_PORTS_END


static struct tms9995reset_param ti99_2_processor_config =
{
#if 0
	REGION_CPU1,/* region for processor RAM */
	0xf000,     /* offset : this area is unused in our region, and matches the processor address */
	0xf0fc,		/* offset for the LOAD vector */
	NULL,       /* no IDLE callback */
	1,          /* use fast IDLE */
#endif
	1           /* enable automatic wait state generation */
};

static MACHINE_DRIVER_START(ti99_2)

	/* basic machine hardware */
	/* TMS9995 CPU @ 10.7 MHz */
	MDRV_CPU_ADD(TMS9995, 10700000)
	MDRV_CPU_CONFIG(ti99_2_processor_config)
	MDRV_CPU_PROGRAM_MAP(ti99_2_memmap, 0)
	MDRV_CPU_IO_MAP(ti99_2_readcru, ti99_2_writecru)
	MDRV_CPU_VBLANK_INT(ti99_2_vblank_interrupt, 1)
	/*MDRV_CPU_PERIODIC_INT(func, rate)*/

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	/*MDRV_INTERLEAVE(interleave)*/

	MDRV_MACHINE_RESET( ti99_2 )

	/* video hardware */
	/*MDRV_TMS9928A( &tms9918_interface )*/
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256, 192)
	MDRV_SCREEN_VISIBLE_AREA(0, 256-1, 0, 192-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(TI99_2_PALETTE_SIZE)
	MDRV_COLORTABLE_LENGTH(TI99_2_COLORTABLE_SIZE)
	MDRV_PALETTE_INIT(ti99_2)

	MDRV_VIDEO_START(ti99_2)
	MDRV_VIDEO_UPDATE(ti99_2)

	/* no sound! */

MACHINE_DRIVER_END



/*
  ROM loading
*/
ROM_START(ti99_224)
	/*CPU memory space*/
	ROM_REGION(0x10000,REGION_CPU1,0)
	ROM_LOAD("992rom.bin", 0x0000, 0x6000, NO_DUMP)      /* system ROMs */
ROM_END

ROM_START(ti99_232)
	/*64kb CPU memory space + 8kb to read the extra ROM page*/
	ROM_REGION(0x12000,REGION_CPU1,0)
	ROM_LOAD("992rom32.bin", 0x0000, 0x6000, NO_DUMP)    /* system ROM - 32kb */
	ROM_CONTINUE(0x10000,0x2000)
ROM_END

SYSTEM_CONFIG_START(ti99_2)
	/* one expansion/cartridge port on the back */
	/* one cassette unit port */
	/* Hex-bus disk controller: supports up to 4 floppy disk drives */
	/* None of these is supported (tape should be easy to emulate) */
SYSTEM_CONFIG_END

/*		YEAR	NAME		PARENT		COMPAT	MACHINE		INPUT	INIT		CONFIG		COMPANY					FULLNAME */
COMP(	1983,	ti99_224,	0,			0,		ti99_2,		ti99_2,	ti99_2_24,	ti99_2,		"Texas Instruments",	"TI-99/2 BASIC Computer (24kb ROMs)" , 0)
COMP(	1983,	ti99_232,	ti99_224,	0,		ti99_2,		ti99_2,	ti99_2_32,	ti99_2,		"Texas Instruments",	"TI-99/2 BASIC Computer (32kb ROMs)" , 0)
