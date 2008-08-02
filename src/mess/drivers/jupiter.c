/***************************************************************************
Jupiter Ace memory map

    CPU: Z80
        0000-1fff ROM
        2000-22ff unused
        2300-23ff RAM (cassette buffer)
        2400-26ff RAM (screen)
        2700-27ff RAM (edit buffer)
        2800-2bff unused
        2c00-2fff RAM (char set)
        3000-3bff unused
        3c00-47ff RAM (standard)
        4800-87ff RAM (16K expansion)
        8800-ffff RAM (Expansion)

Interrupts:

    IRQ:
        50Hz vertical sync

Ports:

    Out 0xfe:
        Tape and buzzer

    In 0xfe:
        Keyboard input and buzzer
***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "includes/jupiter.h"
#include "devices/cartslot.h"


/* memory w/r functions */
static ADDRESS_MAP_START( jupiter_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0x22ff) AM_NOP
	AM_RANGE(0x2300, 0x23ff) AM_RAM
	AM_RANGE(0x2400, 0x26ff) AM_RAM AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x2700, 0x27ff) AM_RAM
	AM_RANGE(0x2800, 0x2bff) AM_NOP
	AM_RANGE(0x2c00, 0x2fff) AM_READWRITE(SMH_RAM, jupiter_vh_charram_w) AM_BASE(&jupiter_charram) AM_SIZE(&jupiter_charram_size)
	AM_RANGE(0x3000, 0x3bff) AM_NOP
	AM_RANGE(0x3c00, 0x47ff) AM_RAM
	AM_RANGE(0x4800, 0x87ff) AM_RAM
	AM_RANGE(0x8800, 0xffff) AM_RAM
ADDRESS_MAP_END

/* port i/o functions */
static ADDRESS_MAP_START( jupiter_io , ADDRESS_SPACE_IO, 8)
	AM_RANGE( 0x00fe, 0xfffe) AM_WRITE( jupiter_port_fe_w )
	AM_RANGE( 0xfefe, 0xfefe) AM_READ_PORT( "KEY0" )
	AM_RANGE( 0xfdfe, 0xfdfe) AM_READ_PORT( "KEY1" )
	AM_RANGE( 0xfbfe, 0xfbfe) AM_READ_PORT( "KEY2" )
	AM_RANGE( 0xf7fe, 0xf7fe) AM_READ_PORT( "KEY3" )
	AM_RANGE( 0xeffe, 0xeffe) AM_READ_PORT( "KEY4" )
	AM_RANGE( 0xdffe, 0xdffe) AM_READ_PORT( "KEY5" )
	AM_RANGE( 0xbffe, 0xbffe) AM_READ_PORT( "KEY6" )
	AM_RANGE( 0x7ffe, 0x7ffe) AM_READ( jupiter_port_7ffe_r )
ADDRESS_MAP_END

/* graphics output */

static const gfx_layout jupiter_charlayout =
{
	8, 8,	/* 8x8 characters */
	128,	/* 128 characters */
	1,		/* 1 bits per pixel */
	{0},	/* no bitplanes; 1 bit per pixel */
	{0, 1, 2, 3, 4, 5, 6, 7},
	{0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8},
	8*8 	/* each character takes 8 consecutive bytes */
};

static PALETTE_INIT( jupiter )
{
	palette_set_color(machine,0,RGB_BLACK); /* black */
	palette_set_color(machine,1,RGB_WHITE); /* white */
	palette_set_color(machine,2,RGB_WHITE); /* white */
	palette_set_color(machine,3,RGB_BLACK); /* black */
}

static GFXDECODE_START( jupiter )
	GFXDECODE_ENTRY( "|main|", 0x2c00, jupiter_charlayout, 0, 2 )
GFXDECODE_END


/* keyboard input */

static INPUT_PORTS_START (jupiter)
	PORT_START_TAG("KEY0")	/* 0: 0xFEFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_RSHIFT)		PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Symbol Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z) 			PORT_CHAR('z') PORT_CHAR('Z') PORT_CHAR(':')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_X) 			PORT_CHAR('x') PORT_CHAR('X') PORT_CHAR('\xA3')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_C) 			PORT_CHAR('c') PORT_CHAR('C') PORT_CHAR('?')

	PORT_START_TAG("KEY1")	/* 1: 0xFDFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_A) 			PORT_CHAR('a') PORT_CHAR('A') PORT_CHAR('~')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_S) 			PORT_CHAR('s') PORT_CHAR('S') PORT_CHAR('|')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_D) 			PORT_CHAR('d') PORT_CHAR('D') PORT_CHAR('\\')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F) 			PORT_CHAR('f') PORT_CHAR('F') PORT_CHAR('{')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_G) 			PORT_CHAR('g') PORT_CHAR('G') PORT_CHAR('}')

	PORT_START_TAG("KEY2")	/* 2: 0xFBFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q) 			PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_W) 			PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_E) 			PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_R) 			PORT_CHAR('r') PORT_CHAR('R') PORT_CHAR('<')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_T) 			PORT_CHAR('t') PORT_CHAR('T') PORT_CHAR('>')
	PORT_START_TAG("KEY3")	/* 3: 0xF7FE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1) 			PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2) 			PORT_CHAR('2') PORT_CHAR('@')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3) 			PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4) 			PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5) 			PORT_CHAR('5') PORT_CHAR('%')

	PORT_START_TAG("KEY4")	/* 4: 0xEFFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0) 			PORT_CHAR('0') PORT_CHAR('_')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9) 			PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8) 			PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7) 			PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6) 			PORT_CHAR('6') PORT_CHAR('&')

	PORT_START_TAG("KEY5")	/* 5: 0xDFFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_P) 			PORT_CHAR('p') PORT_CHAR('P') PORT_CHAR('"')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_O) 			PORT_CHAR('o') PORT_CHAR('O') PORT_CHAR(';')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_I) 			PORT_CHAR('i') PORT_CHAR('I') PORT_CHAR(0x00A9)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_U) 			PORT_CHAR('u') PORT_CHAR('U') PORT_CHAR(']')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y) 			PORT_CHAR('y') PORT_CHAR('Y') PORT_CHAR('[')

	PORT_START_TAG("KEY6")	/* 6: 0xBFFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ENTER)		PORT_CHAR(13)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_L) 			PORT_CHAR('l') PORT_CHAR('L') PORT_CHAR('=')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_K) 			PORT_CHAR('k') PORT_CHAR('K') PORT_CHAR('+')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_J) 			PORT_CHAR('j') PORT_CHAR('J') PORT_CHAR('-')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("h  H  \xE2\x86\x91") PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')

	PORT_START_TAG("KEY7")	/* 7: 0x7FFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE)		PORT_CHAR(' ')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_M)			PORT_CHAR('m') PORT_CHAR('M') PORT_CHAR('.')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_N)			PORT_CHAR('n') PORT_CHAR('N') PORT_CHAR(',')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_B) 			PORT_CHAR('b') PORT_CHAR('B') PORT_CHAR('*')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_V) 			PORT_CHAR('v') PORT_CHAR('V') PORT_CHAR('/')

	PORT_START_TAG("CFG")	/* 8: machine config */
	PORT_DIPNAME( 0x03, 2, "RAM Size")
	PORT_DIPSETTING(	0, "3Kb")
	PORT_DIPSETTING(	1, "19Kb")
	PORT_DIPSETTING(	2, "49Kb")
INPUT_PORTS_END

/* Sound output */

static INTERRUPT_GEN( jupiter_interrupt )
{
	cpunum_set_input_line(machine, 0, 0, HOLD_LINE);
}

/* machine definition */
static MACHINE_DRIVER_START( jupiter )
	/* basic machine hardware */
	MDRV_CPU_ADD("main", Z80, 3250000)        /* 3.25 Mhz */
	MDRV_CPU_PROGRAM_MAP(jupiter_mem, 0)
	MDRV_CPU_IO_MAP(jupiter_io, 0)
	MDRV_CPU_VBLANK_INT("main", jupiter_interrupt)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_START( jupiter )

	/* video hardware */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(32 * 8, 24 * 8)
	MDRV_SCREEN_VISIBLE_AREA(0, 32 * 8 - 1, 0, 24 * 8 - 1)
	MDRV_GFXDECODE( jupiter )
	MDRV_PALETTE_LENGTH(4)
	MDRV_PALETTE_INIT(jupiter)

	MDRV_VIDEO_UPDATE( jupiter )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END

ROM_START (jupiter)
	ROM_REGION (0x10000, "main",0)
	ROM_LOAD ("jupiter.lo", 0x0000, 0x1000, CRC(dc8438a5) SHA1(8fa97eb71e5dd17c7d190c6587ee3840f839347c))
	ROM_LOAD ("jupiter.hi", 0x1000, 0x1000, CRC(4009f636) SHA1(98c5d4bcd74bcf014268cf4c00b2007ea5cc21f3))
ROM_END

static void jupiter_cartslot_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:							info->load = DEVICE_IMAGE_LOAD_NAME(jupiter_ace); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "ace"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

static void jupiter_cassette_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_TYPE:							info->i = IO_CASSETTE; break;
		case MESS_DEVINFO_INT_READABLE:						info->i = 1; break;
		case MESS_DEVINFO_INT_WRITEABLE:						info->i = 0; break;
		case MESS_DEVINFO_INT_CREATABLE:						info->i = 0; break;
		case MESS_DEVINFO_INT_COUNT:							info->i = 1; break;
		case MESS_DEVINFO_INT_RESET_ON_LOAD:					info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:							info->load = DEVICE_IMAGE_LOAD_NAME(jupiter_tap); break;
		case MESS_DEVINFO_PTR_UNLOAD:						info->unload = DEVICE_IMAGE_UNLOAD_NAME(jupiter_tap); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "tap"); break;
	}
}

static SYSTEM_CONFIG_START(jupiter)
	CONFIG_DEVICE(jupiter_cartslot_getinfo)
	CONFIG_DEVICE(jupiter_cassette_getinfo)
SYSTEM_CONFIG_END

/*    YEAR  NAME      PARENT    COMPAT  MACHINE   INPUT     INIT      CONFIG    COMPANY   FULLNAME */
COMP( 1981, jupiter,  0,		0,		jupiter,  jupiter,	0,		  jupiter,	"Cantab",  "Jupiter Ace" , 0)
