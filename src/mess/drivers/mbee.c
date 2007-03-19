/***************************************************************************
    microbee.c

    system driver
    Juergen Buchmueller <pullmoll@t-online.de>, Jan 2000

    Brett Selwood, Andrew Davies (technical assistance)

    Microbee memory map (preliminary)

        0000-7FFF RAM
        8000-BFFF SYSTEM roms (bas522a.rom, bas522b.rom)
        C000-DFFF Edasm or WBee (edasm.rom or wbeee12.rom, optional)
        E000-EFFF Telcom (tecl321.rom; optional)
        F000-F7FF Video RAM
        F800-FFFF PCG RAM (graphics), Colour RAM (banked)

    Microbee 56KB ROM memory map (preliminary)

        0000-DFFF RAM
        E000-EFFF ROM 56kb.rom CP/M bootstrap loader
        F000-F7FF Video RAM
        F800-FFFF PCG RAM (graphics), Colour RAM (banked)

    Microbee 32 came in three versions:
    	IE: features a terminal emulator mapped at $E000
            (maybe there is a keyword to activate it?)

        PC: features an editor/assembler - type EDASM to run

        PC85: features the WordBee wordprocessor - type EDASM to run
              (maybe the ROM was patched to use another keyword?)

***************************************************************************/

#include <math.h>
#include "driver.h"
#include "machine/z80ctc.h"
#include "machine/z80pio.h"
#include "machine/z80sio.h"
#include "video/generic.h"
#include "machine/wd17xx.h"
#include "includes/mbee.h"
#include "devices/basicdsk.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "devices/z80bin.h"
#include "inputx.h"
#include "cpu/z80/z80daisy.h"

#define VERBOSE 1

#if VERBOSE
#define LOG(x)  logerror x
#else
#define LOG(x)  /* x */
#endif

static ADDRESS_MAP_START(mbee_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x7fff) AM_RAM AM_READ( mbee_lowram_r ) AM_BASE( &mbee_workram )
	AM_RANGE(0x8000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xdfff) AM_ROM
	AM_RANGE(0xe000, 0xefff) AM_ROM
	AM_RANGE(0xf000, 0xf7ff) AM_READWRITE(mbee_videoram_r, mbee_videoram_w) AM_BASE(&pcgram) AM_SIZE(&videoram_size)
	AM_RANGE(0xf800, 0xffff) AM_READWRITE(mbee_pcg_color_r, mbee_pcg_color_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START(mbee56_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0xdfff) AM_RAM
	AM_RANGE(0xe000, 0xefff) AM_ROM
	AM_RANGE(0xf000, 0xf7ff) AM_READWRITE(mbee_videoram_r, mbee_videoram_w) AM_BASE(&pcgram) AM_SIZE(&videoram_size)
	AM_RANGE(0xf800, 0xffff) AM_READWRITE(mbee_pcg_color_r, mbee_pcg_color_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START(mbee_ports, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) ) 
	AM_RANGE(0x00, 0x03) AM_READWRITE(mbee_pio_r, mbee_pio_w)
	AM_RANGE(0x08, 0x08) AM_READWRITE(mbee_pcg_color_latch_r, mbee_pcg_color_latch_w)
	AM_RANGE(0x0a, 0x0a) AM_READWRITE(mbee_color_bank_r, mbee_color_bank_w)
	AM_RANGE(0x0b, 0x0b) AM_READWRITE(mbee_video_bank_r, mbee_video_bank_w)
	AM_RANGE(0x0c, 0x0c) AM_READWRITE(m6545_status_r, m6545_index_w)
	AM_RANGE(0x0d, 0x0d) AM_READWRITE(m6545_data_r, m6545_data_w)
	AM_RANGE(0x44, 0x44) AM_READWRITE(wd179x_status_r, wd179x_command_w)
	AM_RANGE(0x45, 0x45) AM_READWRITE(wd179x_track_r, wd179x_track_w)
	AM_RANGE(0x46, 0x46) AM_READWRITE(wd179x_sector_r, wd179x_sector_w)
	AM_RANGE(0x47, 0x47) AM_READWRITE(wd179x_data_r, wd179x_data_w)
	AM_RANGE(0x48, 0x48) AM_READWRITE(mbee_fdc_status_r, mbee_fdc_motor_w)
ADDRESS_MAP_END


INPUT_PORTS_START( mbee )
    PORT_START /* IN0 KEY ROW 0 [000] */
    PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("@") PORT_CODE(KEYCODE_ASTERISK) PORT_CHAR('@')
    PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A) PORT_CHAR('A')
    PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B) PORT_CHAR('B')
    PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C) PORT_CHAR('C')
    PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D) PORT_CHAR('D')
    PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E) PORT_CHAR('E')
    PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F) PORT_CHAR('F')
    PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G) PORT_CHAR('G')
    PORT_START /* IN1 KEY ROW 1 [080] */
    PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H) PORT_CHAR('H')
    PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I) PORT_CHAR('I')
    PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J) PORT_CHAR('J')
    PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K) PORT_CHAR('K')
    PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L) PORT_CHAR('L')
    PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M) PORT_CHAR('M')
    PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N) PORT_CHAR('N')
    PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O) PORT_CHAR('O')
    PORT_START /* IN2 KEY ROW 2 [100] */
    PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P) PORT_CHAR('P')
    PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
    PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R) PORT_CHAR('R')
    PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S) PORT_CHAR('S')
    PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T) PORT_CHAR('T')
    PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U) PORT_CHAR('U')
    PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V) PORT_CHAR('V')
    PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W) PORT_CHAR('W')
    PORT_START /* IN3 KEY ROW 3 [180] */
    PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X) PORT_CHAR('X')
    PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
    PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
    PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("[") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[')
    PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("\\") PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\')
    PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("]") PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']')
    PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("^") PORT_CODE(KEYCODE_TILDE) PORT_CHAR('^')
    PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Delete") PORT_CODE(KEYCODE_DEL) PORT_CHAR(8)
    PORT_START /* IN4 KEY ROW 4 [200] */
    PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0) PORT_CHAR('0')
    PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("1 !") PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
    PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2 \"") PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('\"')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3 #") PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
    PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("4 $") PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
    PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("5 %") PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
    PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("6 &") PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
    PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("7 '") PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
    PORT_START /* IN5 KEY ROW 5 [280] */
    PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("8 (") PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
    PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9 )") PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
    PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("; +") PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(": *") PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(':') PORT_CHAR('*')
    PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(", <") PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
    PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("- =") PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('=')
    PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(". >") PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
    PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("/ ?") PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
    PORT_START /* IN6 KEY ROW 6 [300] */
    PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Escape") PORT_CODE(KEYCODE_ESC) PORT_CHAR(27)
    PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Backspace") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
    PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Tab") PORT_CODE(KEYCODE_TAB) PORT_CHAR(9)
    PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Linefeed") PORT_CODE(KEYCODE_HOME) PORT_CHAR(10)
    PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Enter") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
    PORT_BIT (0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Break") PORT_CODE(KEYCODE_END) PORT_CHAR(3)
    PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
    PORT_START /* IN7 KEY ROW 7 [380] */
    PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Ctrl") PORT_CODE(KEYCODE_LCONTROL)
    PORT_BIT (0x04, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT (0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT (0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT (0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("L-Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
    PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("R-Shift") PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
    PORT_START /* IN8 extra keys */
    PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(Up)") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
    PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(Down)") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
    PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(Left)") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
    PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(Right)") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
    PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(Insert)") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(INSERT))
    PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

const gfx_layout mbee_charlayout =
{
    8,16,                   /* 8 x 16 characters */
    256,                    /* 256 characters */
    1,                      /* 1 bits per pixel */
    { 0 },                  /* no bitplanes; 1 bit per pixel */
    /* x offsets */
    { 0, 1, 2, 3, 4, 5, 6, 7 },
    /* y offsets triple height: use each line three times */
    {  0*8,  1*8,  2*8,  3*8,  4*8,  5*8,  6*8,  7*8,
       8*8,  9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
    8*16                    /* every char takes 16 bytes */
};

static gfx_decode mbee_gfxdecodeinfo[] =
{
    { REGION_CPU1, 0xf000, &mbee_charlayout, 0, 256},
	{ -1 }   /* end of array */
};

static const UINT8 palette[] =
{
    0x00,0x00,0x00, /* black    */
    0xf0,0x00,0x00, /* red      */
    0x00,0xf0,0x00, /* green    */
    0xf0,0xf0,0x00, /* yellow   */
    0x00,0x00,0xf0, /* blue     */
    0xf0,0x00,0xf0, /* magenta  */
    0x00,0xf0,0xf0, /* cyan     */
    0xf0,0xf0,0xf0, /* white    */
    0x08,0x08,0x08, /* black    */
    0xe0,0x08,0x08, /* red      */
    0x08,0xe0,0x08, /* green    */
    0xe0,0xe0,0x08, /* yellow   */
    0x08,0x08,0xe0, /* blue     */
    0xe0,0x08,0xe0, /* magenta  */
    0x08,0xe0,0xe0, /* cyan     */
    0xe0,0xe0,0xe0, /* white    */
    0x10,0x10,0x10, /* black    */
    0xd0,0x10,0x10, /* red      */
    0x10,0xd0,0x10, /* green    */
    0xd0,0xd0,0x10, /* yellow   */
    0x10,0x10,0xd0, /* blue     */
    0xd0,0x10,0xd0, /* magenta  */
    0x10,0xd0,0xd0, /* cyan     */
    0xd0,0xd0,0xd0, /* white    */
    0x18,0x18,0x18, /* black    */
    0xe0,0x18,0x18, /* red      */
    0x18,0xe0,0x18, /* green    */
    0xe0,0xe0,0x18, /* yellow   */
    0x18,0x18,0xe0, /* blue     */
    0xe0,0x18,0xe0, /* magenta  */
    0x18,0xe0,0xe0, /* cyan     */
    0xe0,0xe0,0xe0  /* white    */
};

static PALETTE_INIT( mbee )
{
	int i;
	palette_set_colors(machine, 0, palette, sizeof(palette) / 3);
	for( i = 0; i < 256; i++ )
	{
		colortable[2*i+0] = i / 32;
		colortable[2*i+1] = i & 31;
	}
}

static const struct z80_irq_daisy_chain mbee_daisy_chain[] =
{
	{ z80pio_reset, z80pio_irq_state, z80pio_irq_ack, z80pio_irq_reti, 0 }, /* PIO number 0 */
	{ 0, 0, 0, 0, -1}      /* end mark */
};



static MACHINE_DRIVER_START( mbee )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 3375000)         /* 3.37500 Mhz */
	MDRV_CPU_PROGRAM_MAP(mbee_mem, 0)
	MDRV_CPU_IO_MAP(mbee_ports, 0)
	/* MDRV_CPU_CONFIG(mbee_daisy_chain) */
	MDRV_CPU_VBLANK_INT(mbee_interrupt,1)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(TIME_IN_USEC(2500))
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_RESET( mbee )
	MDRV_MACHINE_START( mbee )

	MDRV_GFXDECODE(mbee_gfxdecodeinfo)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(70*8, 310)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 70*8-1, 0, 19*16-1)
	MDRV_PALETTE_LENGTH(sizeof(palette)/sizeof(palette[0])/3)
	MDRV_COLORTABLE_LENGTH(256 * 2)
	MDRV_PALETTE_INIT(mbee)

	MDRV_VIDEO_START(mbee)
	MDRV_VIDEO_UPDATE(mbee)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(WAVE, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD(SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.75)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( mbee56 )
	MDRV_IMPORT_FROM( mbee )
	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_PROGRAM_MAP(mbee56_mem, 0)
	MDRV_MACHINE_START( mbee56 )
MACHINE_DRIVER_END


ROM_START( mbee )
    ROM_REGION(0x10000,REGION_CPU1,0)
    ROM_LOAD("bas522a.rom",  0x8000, 0x2000, CRC(7896a696) SHA1(a158f7803296766160e1f258dfc46134735a9477))
    ROM_LOAD("bas522b.rom",  0xa000, 0x2000, CRC(b21d9679) SHA1(332844433763331e9483409cd7da3f90ac58259d))
    ROM_LOAD("edasm.rom",    0xc000, 0x2000, CRC(1af1b3a9) SHA1(d035a997c2dbbb3918b3395a3a5a1076aa203ee5))
    ROM_LOAD("charrom.bin",  0xf000, 0x1000, CRC(1f9fcee4) SHA1(e57ac94e03638075dde68a0a8c834a4f84ba47b0))
ROM_END

ROM_START( mbeepc85 )
    ROM_REGION(0x10000,REGION_CPU1,0)
    ROM_LOAD("bas522a.rom",  0x8000, 0x2000, CRC(7896a696) SHA1(a158f7803296766160e1f258dfc46134735a9477))
    ROM_LOAD("bas522b.rom",  0xa000, 0x2000, CRC(b21d9679) SHA1(332844433763331e9483409cd7da3f90ac58259d))
    ROM_LOAD("wbee12.rom",   0xc000, 0x2000, CRC(0fc21cb5) SHA1(33b3995988fc51ddef1568e160dfe699867adbd5))
    ROM_LOAD("charrom.bin",  0xf000, 0x1000, CRC(1f9fcee4) SHA1(e57ac94e03638075dde68a0a8c834a4f84ba47b0))
ROM_END

ROM_START( mbeepc )
    ROM_REGION(0x10000,REGION_CPU1,0)
    ROM_LOAD("bas522a.rom",  0x8000, 0x2000, CRC(7896a696) SHA1(a158f7803296766160e1f258dfc46134735a9477))
    ROM_LOAD("bas522b.rom",  0xa000, 0x2000, CRC(b21d9679) SHA1(332844433763331e9483409cd7da3f90ac58259d))
    ROM_LOAD("telc321.rom",  0xe000, 0x2000, CRC(15b9d2df) SHA1(6e7606099d036f87230b3595eb873be60c190f11))
    ROM_LOAD("charrom.bin",  0xf000, 0x1000, CRC(1f9fcee4) SHA1(e57ac94e03638075dde68a0a8c834a4f84ba47b0))
ROM_END

ROM_START( mbee56 )
    ROM_REGION(0x10000,REGION_CPU1,0)
    ROM_LOAD("56kb.rom",     0xe000, 0x1000, CRC(28211224) SHA1(b6056339402a6b2677b0e6c57bd9b78a62d20e4f))
    ROM_LOAD("charrom.bin",  0xf000, 0x1000, CRC(1f9fcee4) SHA1(e57ac94e03638075dde68a0a8c834a4f84ba47b0))
ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

static void mbee_cassette_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}

static void mbee_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_mbee_cart; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "rom"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

static void mbee_floppy_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 4; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_basicdsk_floppy; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "dsk"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}



SYSTEM_CONFIG_START(mbee)
	CONFIG_DEVICE(mbee_cassette_getinfo)
	CONFIG_DEVICE(mbee_cartslot_getinfo)
	CONFIG_DEVICE(mbee_floppy_getinfo)
	CONFIG_DEVICE(z80bin_quickload_getinfo)
SYSTEM_CONFIG_END

/*    YEAR  NAME      PARENT	COMPAT	MACHINE   INPUT     INIT      CONFIG	COMPANY   FULLNAME */
COMP( 1982, mbee,     0,	0,	mbee,     mbee,     0,        mbee,		"Applied Technology",  "Microbee 32 IC" , 0)
COMP( 1982, mbeepc,   mbee,	0,	mbee,     mbee,     0,        mbee,		"Applied Technology",  "Microbee 32 PC" , 0)
COMP( 1985?,mbeepc85, mbee,	0,	mbee,     mbee,     0,        mbee,		"Applied Technology",  "Microbee 32 PC85" , 0)
COMP( 1983, mbee56,   mbee,	0,	mbee56,   mbee,     0,        mbee,		"Applied Technology",  "Microbee 56" , 0)

