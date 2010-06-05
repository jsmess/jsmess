/***************************************************************************

    Philips VG-5000mu

    05/2010 (Sandro Ronco)
     - EF9345 video controller
     - keyboard input ports
    05/2009 Skeleton driver.

    Informations ( see the very informative http://vg5k.free.fr/ ):
     - Variants: Radiola VG5000 and Schneider VG5000
     - CPU: Zilog Z80 running at 4MHz
     - ROM: 18KB (16 KB BASIC + 2 KB charset )
     - RAM: 24 KB
     - Video: SGS Thomson EF9345 processor
            - Text mode: 25 rows x 40 columns
            - Character matrix: 8 x 10
            - ASCII characters set, 128 graphics mode characters, 192 user characters.
            - Graphics mode: not available within basic, only semi graphic is available.
            - Colors: 8
     - Sound: Synthesizer, 4 Octaves
     - Keyboard: 63 keys AZERTY, Caps Lock, CTRL key to access 33 BASIC instructions
     - I/O: Tape recorder connector (1200/2400 bauds), Scart connector to TV (RGB),
       External PSU (VU0022) connector, Bus connector (2x25 pins)
     - There are 2 versions of the VG5000 ROM, one with Basic v1.0,
       contained in two 8 KB ROMs, and one with Basic 1.1, contained in
       a single 16 KB ROM.
     - RAM: 24 KB (3 x 8 KB) type SRAM D4168C, more precisely:
         2 x 8 KB, used by the system
         1 x 8 KB, used by the video processor
     - Memory Map:
         $0000 - $3fff  BASIC + monitor
         $4000 - $47cf  Screen
         $47d0 - $7fff  reserved area for BASIC, variables, etc
         $8000 - $bfff  Memory Expansion 16K or ROM cart
         $c000 - $ffff  Memory Expansion 32K or ROM cart
     - This computer was NOT MSX-compatible!


****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "devices/messram.h"
#include "devices/printer.h"
#include "video/ef9345.h"
#include "sound/dac.h"
#include "devices/cassette.h"
#include "sound/wave.h"


class vg5k_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, vg5k_state(machine)); }

	vg5k_state(running_machine &machine) { }

	running_device *ef9345;
	running_device *dac;
	running_device *printer;
	running_device *cassette;

	UINT8 ef9345_offset;
};


READ8_HANDLER( printer_r )
{
	vg5k_state *vg5k = (vg5k_state *)space->machine->driver_data;

	return (printer_is_ready(vg5k->printer) ? 0x00 : 0xff);
}


WRITE8_HANDLER( printer_w )
{
	vg5k_state *vg5k = (vg5k_state *)space->machine->driver_data;

	printer_output(vg5k->printer, data);
}


WRITE8_HANDLER ( ef9345_offset_w )
{
	vg5k_state *vg5k = (vg5k_state *)space->machine->driver_data;

	vg5k->ef9345_offset = data;
}


READ8_HANDLER ( ef9345_io_r )
{
	vg5k_state *vg5k = (vg5k_state *)space->machine->driver_data;

	return ef9345_r(vg5k->ef9345, vg5k->ef9345_offset);
}


WRITE8_HANDLER ( ef9345_io_w )
{
	vg5k_state *vg5k = (vg5k_state *)space->machine->driver_data;

	ef9345_w(vg5k->ef9345, vg5k->ef9345_offset, data);
}


READ8_HANDLER ( cassette_r )
{
	vg5k_state *vg5k = (vg5k_state *)space->machine->driver_data;
	double level;

	level = cassette_input(vg5k->cassette);

	return (level > 0.03) ? 0xff : 0x00;
}


WRITE8_HANDLER ( cassette_w )
{
	vg5k_state *vg5k = (vg5k_state *)space->machine->driver_data;

	dac_signed_data_w(vg5k->dac, data <<2);
}


static ADDRESS_MAP_START( vg5k_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x3fff ) AM_ROM
	AM_RANGE( 0x4000, 0x7fff ) AM_RAM
	AM_RANGE( 0x8000, 0xffff ) AM_NOP /* messram expansion memory */
ADDRESS_MAP_END

static ADDRESS_MAP_START( vg5k_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK (0xff)

	/* joystick */
	AM_RANGE( 0x07, 0x07 ) AM_READ_PORT("JOY0")
	AM_RANGE( 0x08, 0x08 ) AM_READ_PORT("JOY1")

	/* printer */
	AM_RANGE( 0x10, 0x10 ) AM_READ(printer_r)
	AM_RANGE( 0x11, 0x11 ) AM_WRITE(printer_w)

	/* keyboard */
	AM_RANGE( 0x80, 0x80 ) AM_READ_PORT("ROW1")
	AM_RANGE( 0x81, 0x81 ) AM_READ_PORT("ROW2")
	AM_RANGE( 0x82, 0x82 ) AM_READ_PORT("ROW3")
	AM_RANGE( 0x83, 0x83 ) AM_READ_PORT("ROW4")
	AM_RANGE( 0x84, 0x84 ) AM_READ_PORT("ROW5")
	AM_RANGE( 0x85, 0x85 ) AM_READ_PORT("ROW6")
	AM_RANGE( 0x86, 0x86 ) AM_READ_PORT("ROW7")
	AM_RANGE( 0x87, 0x87 ) AM_READ_PORT("ROW8")

	/* EF9345 */
	AM_RANGE( 0x8f, 0x8f ) AM_WRITE(ef9345_offset_w)
	AM_RANGE( 0xcf, 0xcf ) AM_READWRITE(ef9345_io_r, ef9345_io_w)

	/* cassette */
	AM_RANGE(0xaf,0xaf) AM_READWRITE( cassette_r, cassette_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( vg5k )

	PORT_START("ROW1")
		PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_HOME) PORT_NAME("HOME")
		PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED ) PORT_UNUSED
		PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_LSHIFT) PORT_NAME("SHIFT")
		PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_LEFT) PORT_NAME("LEFT")
		PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_RIGHT) PORT_NAME("RIGHT")
		PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_DOWN) PORT_NAME("DOWN")
		PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_LCONTROL)  PORT_NAME("CTRL")
		PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_INSERT) PORT_NAME("INS")

	PORT_START("ROW2")
		PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F2) PORT_NAME("RUN")
		PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
		PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
		PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CAPSLOCK) PORT_NAME("CAPSLOCK")
		PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED ) PORT_UNUSED
		PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_ENTER) PORT_NAME("ENTER")
		PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_UP) PORT_NAME("UP")
		PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')

	PORT_START("ROW3")
		PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('W')
		PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('X')
		PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
		PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('V')
		PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
		PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1')
		PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR(':')
		PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')

	PORT_START("ROW4")
		PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
		PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
		PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3')
		PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4')
		PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5')
		PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6')
		PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2')
		PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';')

	PORT_START("ROW5")
		PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/')
		PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_ASTERISK) PORT_CHAR('*')
		PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
		PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('G')
		PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
		PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
		PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('O')
		PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('P')

	PORT_START("ROW6")
		PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED ) PORT_UNUSED
		PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0')
		PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']')
		PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F1) PORT_NAME("..")
		PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',')
		PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7')
		PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8')
		PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9')

	PORT_START("ROW7")
		PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-')
		PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_PLUS_PAD) PORT_CHAR('+')
		PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
		PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
		PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
		PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_PGUP) PORT_CHAR('<')
		PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_PGDN) PORT_NAME("PRINT")
		PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')

	PORT_START("ROW8")
		PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('=')
		PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSPACE) PORT_NAME("<--")
		PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('N')
		PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
		PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
		PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
		PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
		PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M')

	PORT_START("JOY0")
		PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_PLAYER(1)
		PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_PLAYER(1)
		PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_PLAYER(1)
		PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_PLAYER(1)
		PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_PLAYER(1)
		PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED ) PORT_UNUSED
		PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED ) PORT_UNUSED
		PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED ) PORT_UNUSED

	PORT_START("JOY1")
		PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_PLAYER(2)
		PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_PLAYER(2)
		PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_PLAYER(2)
		PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_PLAYER(2)
		PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_PLAYER(2)
		PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED ) PORT_UNUSED
		PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED ) PORT_UNUSED
		PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED ) PORT_UNUSED

INPUT_PORTS_END


static TIMER_CALLBACK( z80_irq )
{
	cputag_set_input_line(machine, "maincpu", 0, HOLD_LINE);
}

static TIMER_DEVICE_CALLBACK( vg5k_scanline )
{
	vg5k_state *vg5k = (vg5k_state *)timer->machine->driver_data;

	ef9345_scanline(vg5k->ef9345, (UINT16)param);
}


static MACHINE_START( vg5k )
{
	vg5k_state *vg5k = (vg5k_state *)machine->driver_data;

	vg5k->ef9345 = devtag_get_device(machine, "ef9345");
	vg5k->dac = devtag_get_device(machine, "dac");
	vg5k->printer = devtag_get_device(machine, "printer");
	vg5k->cassette = devtag_get_device(machine, "cassette");

	timer_pulse(machine, ATTOTIME_IN_MSEC(20), NULL, 0, z80_irq);

	state_save_register_global(machine, vg5k->ef9345_offset);
}

static MACHINE_RESET( vg5k )
{
	vg5k_state *vg5k = (vg5k_state *)machine->driver_data;

	vg5k->ef9345_offset = 0;
}

static VIDEO_START( vg5k )
{
}

static VIDEO_UPDATE( vg5k )
{
	vg5k_state *vg5k = (vg5k_state *)screen->machine->driver_data;

	video_update_ef9345(vg5k->ef9345, bitmap, cliprect);

	return 0;
}

/* F4 Character Displayer */
static const gfx_layout vg5k_charlayout =
{
	8, 16,					/* 8 x 16 characters */
	256,					/* 256 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	/* y offsets */
	{ 0, 8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	8*16					/* every char takes 16 bytes */
};

static GFXDECODE_START( vg5k )
	GFXDECODE_ENTRY( "gfx1", 0x2000, vg5k_charlayout, 0, 4 )
GFXDECODE_END

static DRIVER_INIT( vg5k )
{
	UINT8 *FNT = memory_region(machine, "gfx1");
	UINT16 a,b,c,d,dest=0x2000;

	/* Unscramble the chargen rom as the format is too complex for gfxdecode to handle unaided */
	for (a = 0; a < 8192; a+=4096)
		for (b = 0; b < 2048; b+=64)
			for (c = 0; c < 4; c++)
				for (d = 0; d < 64; d+=4)
					FNT[dest++]=FNT[a|b|c|d];


	/* install expansion memory*/
	const address_space *program = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT8 *ram = messram_get_ptr(devtag_get_device(machine, "messram"));
	UINT16 ram_size = messram_get_size(devtag_get_device(machine, "messram"));

	if (ram_size > 0x4000)
		memory_install_ram(program, 0x8000, 0x3fff + ram_size, 0, 0, ram);
}


static const ef9345_config vg5k_ef9345_config =
{
	"gfx1",				/* charset */
	336,				/* screen width */
	300					/* screen height */
};

static const struct CassetteOptions vg5kr_cassette_options =
{
	1,		/* channels */
	16,		/* bits per sample */
	44100	/* sample frequency */
};

static const cassette_config vg5k_cassette_config =
{
	cassette_default_formats,
	&vg5kr_cassette_options,
	(cassette_state)(CASSETTE_STOPPED | CASSETTE_MASK_SPEAKER)
};


static MACHINE_DRIVER_START( vg5k )
	MDRV_DRIVER_DATA(vg5k_state)

	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
	MDRV_CPU_PROGRAM_MAP(vg5k_mem)
	MDRV_CPU_IO_MAP(vg5k_io)

	MDRV_TIMER_ADD_SCANLINE("vg5k_scanline", vg5k_scanline, "screen", 0, 10)

	MDRV_EF9345_ADD("ef9345", vg5k_ef9345_config)

	MDRV_MACHINE_START(vg5k)
	MDRV_MACHINE_RESET(vg5k)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(336, 300)
	MDRV_SCREEN_VISIBLE_AREA(00, 336-1, 00, 270-1)

	MDRV_GFXDECODE(vg5k)
	MDRV_PALETTE_LENGTH(8)

	MDRV_VIDEO_START(vg5k)
	MDRV_VIDEO_UPDATE(vg5k)


	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("dac", DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	/* cassette */
	MDRV_SOUND_WAVE_ADD("wave", "cassette")
	MDRV_SOUND_ROUTE(0, "mono", 0.1)

	MDRV_CASSETTE_ADD( "cassette", vg5k_cassette_config )

	/* printer */
	MDRV_PRINTER_ADD("printer")

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("16K")
	MDRV_RAM_EXTRA_OPTIONS("32K,48k")
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( vg5k )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS(0, "default", "VG 5000")
	ROMX_LOAD( "vg5k.bin",  0x0000, 0x4000, CRC(a6998ff8) SHA1(881ba594be0a721a999378312aea0c3c1c7b2b58), ROM_BIOS(1) )			// dumped from a Radiola VG-5000
	ROM_SYSTEM_BIOS(1, "alt", "VG 5000 (alt)")
	ROMX_LOAD( "vg5k.rom", 0x0000, 0x4000, BAD_DUMP CRC(a6f4a0ea) SHA1(58eccce33cc21fc17bc83921018f531b8001eda3), ROM_BIOS(2) )	// from dcvg5k

	ROM_REGION( 0x4000, "gfx1", 0 )
	ROM_LOAD( "charset.rom", 0x0000, 0x2000, BAD_DUMP CRC(b2f49eb3) SHA1(d0ef530be33bfc296314e7152302d95fdf9520fc) )			// from dcvg5k
ROM_END

/* Driver */
/*    YEAR  NAME    PARENT  COMPAT  MACHINE  INPUT  INIT   COMPANY     FULLNAME   FLAGS */
COMP( 1984, vg5k,   0,      0,      vg5k,    vg5k,  vg5k, "Philips",  "VG-5000", GAME_NOT_WORKING | GAME_NO_SOUND)
