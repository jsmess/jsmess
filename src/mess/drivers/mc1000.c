/***************************************************************************

    MC 1000

    12/05/2009 Skeleton driver.

	http://ensjo.wikispaces.com/MC-1000+on+JEMU
	http://ensjo.blogspot.com/2006/11/color-artifacting-no-mc-1000.html

****************************************************************************/

/*

	TODO:

	- cassette
	- interrupt from NE555
	- xtal frequency?
	- Z80 wait at 0x0000-0x1fff when !hsync & !vsync
	- MC6847 color artifacting is broken
	- 80-column card (MC6845)
	- Charlemagne / GEM-1000 / Junior Computer

*/

#include "driver.h"
#include "includes/mc1000.h"
#include "cpu/z80/z80.h"
#include "devices/cassette.h"
#include "video/m6847.h"
#include "sound/ay8910.h"
#include "machine/ctronics.h"
#include "machine/rescap.h"

/* Memory Banking */

static void mc1000_bankswitch(running_machine *machine)
{
	mc1000_state *state = machine->driver_data;
	const address_space *program = cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM);

	/* MC6845 video RAM */
	memory_set_bank(machine, 2, state->mc6845_bank);

	/* extended RAM */
	if (mess_ram_size > 16*1024)
	{
		memory_install_readwrite8_handler(program, 0x4000, 0x7fff, 0, 0, SMH_BANK(3), SMH_BANK(3));
	}
	else
	{
		memory_install_readwrite8_handler(program, 0x4000, 0x7fff, 0, 0, SMH_UNMAP, SMH_UNMAP);
	}

	/* MC6847 video RAM */
	if (state->mc6847_bank)
	{
		if (mess_ram_size > 16*1024)
		{
			memory_install_readwrite8_handler(program, 0x8000, 0x97ff, 0, 0, SMH_BANK(4), SMH_BANK(4));
		}
		else
		{
			memory_install_readwrite8_handler(program, 0x8000, 0x97ff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		}
	}
	else
	{
		memory_install_readwrite8_handler(program, 0x8000, 0x97ff, 0, 0, SMH_BANK(4), SMH_BANK(4));
	}

	memory_set_bank(machine, 4, state->mc6847_bank);

	/* extended RAM */
	if (mess_ram_size > 16*1024)
	{
		memory_install_readwrite8_handler(program, 0x9800, 0xbfff, 0, 0, SMH_BANK(5), SMH_BANK(5));
	}
	else
	{
		memory_install_readwrite8_handler(program, 0x9800, 0xbfff, 0, 0, SMH_UNMAP, SMH_UNMAP);
	}
}

/* Read/Write Handlers */

static READ8_HANDLER( printer_r )
{
	mc1000_state *state = space->machine->driver_data;
	
	return centronics_busy_r(state->centronics);
}

static WRITE8_HANDLER( printer_w )
{
	mc1000_state *state = space->machine->driver_data;

	centronics_strobe_w(state->centronics, BIT(data, 0));
}

static WRITE8_HANDLER( mc6845_ctrl_w )
{
	mc1000_state *state = space->machine->driver_data;

	state->mc6845_bank = BIT(data, 0);

	mc1000_bankswitch(space->machine);
}

static WRITE8_HANDLER( mc6847_attr_w )
{
	/*

		bit		description

		0		enable CPU video RAM access
		1		CSS
		2		GM0
		3		GM1
		4		GM2
		5		_INT/EXT
		6		_A/S
		7		_A/G

	*/

	mc1000_state *state = space->machine->driver_data;

	state->mc6847_bank = BIT(data, 0);

	mc1000_bankswitch(space->machine);

	state->mc6847_attr = data;
}

/* Memory Maps */

static ADDRESS_MAP_START( mc1000_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_RAMBANK(1)
	AM_RANGE(0x2000, 0x27ff) AM_RAMBANK(2) AM_BASE_MEMBER(mc1000_state, mc6845_video_ram)
	AM_RANGE(0x2800, 0x3fff) AM_RAM
	AM_RANGE(0x4000, 0x7fff) AM_RAMBANK(3)
	AM_RANGE(0x8000, 0x97ff) AM_RAMBANK(4) AM_BASE_MEMBER(mc1000_state, mc6847_video_ram)
	AM_RANGE(0x9800, 0xbfff) AM_RAMBANK(5)
	AM_RANGE(0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( mc1000_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x04, 0x04) AM_READWRITE(printer_r, printer_w)
	AM_RANGE(0x05, 0x05) AM_DEVWRITE(CENTRONICS_TAG, centronics_data_w)
//	AM_RANGE(0x10, 0x10) AM_DEVWRITE(MC6845_TAG, mc6845_address_w)
//	AM_RANGE(0x11, 0x11) AM_DEVREADWRITE(MC6845_TAG, mc6845_register_r, mc6845_register_w)
	AM_RANGE(0x12, 0x12) AM_WRITE(mc6845_ctrl_w)	
	AM_RANGE(0x20, 0x20) AM_DEVWRITE(AY8910_TAG, ay8910_address_w)
	AM_RANGE(0x40, 0x40) AM_DEVREAD(AY8910_TAG, ay8910_r)
	AM_RANGE(0x60, 0x60) AM_DEVWRITE(AY8910_TAG, ay8910_data_w)
	AM_RANGE(0x80, 0x80) AM_WRITE(mc6847_attr_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( mc1000 )
	PORT_START("ROW0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('@')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2) PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2) PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("ROW1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("ROW2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("ROW3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("ROW4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("ROW5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RUBOUT") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("ROW6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('^')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("ROW7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RESET")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("MODIFIERS")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CTRL") PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_MAMEKEY(RCONTROL))

	PORT_INCLUDE( m6847_artifacting )
INPUT_PORTS_END

/* Video */

static void mc1000_hsync(running_machine *machine, int data)
{
	mc1000_state *state = machine->driver_data;

	state->hsync = data;
}

static void mc1000_vsync(running_machine *machine, int data)
{
	mc1000_state *state = machine->driver_data;

	state->vsync = data;
}

static ATTR_CONST UINT8 mc1000_get_attributes(running_machine *machine, UINT8 c, int scanline, int pos)
{
	mc1000_state *state = machine->driver_data;

	UINT8 data = 0;

	data |= BIT(c, 7) ? M6847_INV : 0;
	data |= BIT(state->mc6847_attr, 1) ? M6847_CSS : 0;
	data |= BIT(state->mc6847_attr, 2) ? M6847_GM0 : 0;
	data |= BIT(state->mc6847_attr, 3) ? M6847_GM1 : 0;
	data |= BIT(state->mc6847_attr, 4) ? M6847_GM2 : 0;
	data |= BIT(state->mc6847_attr, 5) ? M6847_INTEXT : 0;
	data |= BIT(state->mc6847_attr, 6) ? M6847_AS : 0;
	data |= BIT(state->mc6847_attr, 7) ? M6847_AG : 0;

	return data;
}

static const UINT8 *mc1000_get_video_ram(running_machine *machine, int scanline)
{
	mc1000_state *state = machine->driver_data;

	return state->mc6847_video_ram + (scanline / 12) * 0x20;
}

static VIDEO_START( mc1000 )
{
	m6847_config cfg;

	memset(&cfg, 0, sizeof(cfg));

	cfg.type = M6847_VERSION_ORIGINAL_NTSC;
	cfg.horizontal_sync_callback = mc1000_hsync;
	cfg.field_sync_callback = mc1000_vsync;
	cfg.get_attributes = mc1000_get_attributes;
	cfg.get_video_ram = mc1000_get_video_ram;

	m6847_init(machine, &cfg);
}

/* AY-3-8910 Interface */

static WRITE8_DEVICE_HANDLER( keylatch_w )
{
	mc1000_state *state = device->machine->driver_data;

	state->keylatch = data;

	cassette_output(state->cassette, BIT(data, 7) ? +1.0 : -1.0);
}

static READ8_DEVICE_HANDLER( keydata_r )
{
	mc1000_state *state = device->machine->driver_data;

	UINT8 data = 0xff;

	if (!BIT(state->keylatch, 0)) data &= input_port_read(device->machine, "ROW0");
	if (!BIT(state->keylatch, 1)) data &= input_port_read(device->machine, "ROW1");
	if (!BIT(state->keylatch, 2)) data &= input_port_read(device->machine, "ROW2");
	if (!BIT(state->keylatch, 3)) data &= input_port_read(device->machine, "ROW3");
	if (!BIT(state->keylatch, 4)) data &= input_port_read(device->machine, "ROW4");
	if (!BIT(state->keylatch, 5)) data &= input_port_read(device->machine, "ROW5");
	if (!BIT(state->keylatch, 6)) data &= input_port_read(device->machine, "ROW6");
	if (!BIT(state->keylatch, 7)) data &= input_port_read(device->machine, "ROW7");

	data = (input_port_read(device->machine, "MODIFIERS") & 0xc0) | (data & 0x3f);

	if (cassette_input(state->cassette) < +0.0)	data &= 0x7f;

	return data;
}

static const ay8910_interface ay8910_intf =
{
	AY8910_SINGLE_OUTPUT,
	{ RES_K(2.2), 0, 0 },
	DEVCB_NULL,
	DEVCB_HANDLER(keydata_r),
	DEVCB_HANDLER(keylatch_w),
	DEVCB_NULL
};

/* Machine Initialization */

static MACHINE_START( mc1000 )
{
	mc1000_state *state = machine->driver_data;
	const address_space *program = cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM);

	/* find devices */
	state->mc6845 = devtag_get_device(machine, MC6845_TAG);
	state->centronics = devtag_get_device(machine, CENTRONICS_TAG);
	state->cassette = devtag_get_device(machine, CASSETTE_TAG);

	/* setup memory banking */
	memory_install_readwrite8_handler(program, 0x0000, 0x1fff, 0, 0, SMH_BANK(1), SMH_BANK(1));
	memory_configure_bank(machine, 1, 0, 1, memory_region(machine, Z80_TAG), 0);
	memory_configure_bank(machine, 1, 1, 1, memory_region(machine, Z80_TAG) + 0xc000, 0);
	memory_set_bank(machine, 1, 1);

	state->rom0000 = 1;

	memory_install_readwrite8_handler(program, 0x2000, 0x27ff, 0, 0, SMH_BANK(2), SMH_BANK(2));
	memory_configure_bank(machine, 2, 0, 1, memory_region(machine, Z80_TAG) + 0x2000, 0);
	memory_configure_bank(machine, 2, 1, 1, state->mc6845_video_ram, 0);
	memory_set_bank(machine, 2, 0);

	memory_configure_bank(machine, 3, 0, 1, memory_region(machine, Z80_TAG) + 0x4000, 0);
	memory_set_bank(machine, 3, 0);

	memory_configure_bank(machine, 4, 0, 1, state->mc6847_video_ram, 0);
	memory_configure_bank(machine, 4, 1, 1, memory_region(machine, Z80_TAG) + 0x8000, 0);
	memory_set_bank(machine, 4, 0);

	memory_configure_bank(machine, 5, 0, 1, memory_region(machine, Z80_TAG) + 0x9800, 0);
	memory_set_bank(machine, 5, 0);

	mc1000_bankswitch(machine);

	/* register for state saving */
	state_save_register_global(machine, state->rom0000);
	state_save_register_global(machine, state->mc6845_bank);
	state_save_register_global(machine, state->mc6847_bank);
	state_save_register_global(machine, state->keylatch);
	state_save_register_global(machine, state->hsync);
	state_save_register_global(machine, state->vsync);
	state_save_register_global(machine, state->mc6847_attr);
}

static MACHINE_RESET( mc1000 )
{
	mc1000_state *state = machine->driver_data;

	memory_set_bank(machine, 1, 1);

	state->rom0000 = 1;
}

/* Machine Driver */

static const cassette_config mc1000_cassette_config =
{
	cassette_default_formats,
	NULL,
	CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_ENABLED
};

static MACHINE_DRIVER_START( mc1000 )
	MDRV_DRIVER_DATA(mc1000_state)

	/* basic machine hardware */
    MDRV_CPU_ADD(Z80_TAG, Z80, 3579545)
    MDRV_CPU_PROGRAM_MAP(mc1000_mem)
    MDRV_CPU_IO_MAP(mc1000_io)

    MDRV_MACHINE_START(mc1000)
    MDRV_MACHINE_RESET(mc1000)

    /* video hardware */
    MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
    MDRV_SCREEN_REFRESH_RATE(M6847_NTSC_FRAMES_PER_SECOND)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_SIZE(320, 25+192+26)
	MDRV_SCREEN_VISIBLE_AREA(0, 319, 1, 239)
    MDRV_PALETTE_LENGTH(16)

    MDRV_VIDEO_START(mc1000)
    MDRV_VIDEO_UPDATE(m6847)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910_TAG, AY8910, 3579545/2)
	MDRV_SOUND_CONFIG(ay8910_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* devices */
	MDRV_CASSETTE_ADD(CASSETTE_TAG, mc1000_cassette_config)
	MDRV_CENTRONICS_ADD(CENTRONICS_TAG, standard_centronics)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( mc1000 )
	ROM_REGION( 0x10000, Z80_TAG, 0 )
	ROM_LOAD( "mc1000.ic17", 0xc000, 0x2000, CRC(8e78d80d) SHA1(9480270e67a5db2e7de8bc5c8b9e0bb210d4142b) )
	ROM_LOAD( "mc1000.ic12", 0xe000, 0x2000, CRC(750c95f0) SHA1(fd766f5ea4481ef7fd4df92cf7d8397cc2b5a6c4) )
ROM_END

/* System Configuration */

static SYSTEM_CONFIG_START( mc1000 )
	CONFIG_RAM_DEFAULT( 16 * 1024 )
	CONFIG_RAM		  ( 48 * 1024 )
SYSTEM_CONFIG_END

/* Driver Initialization */

static DIRECT_UPDATE_HANDLER( mc1000_direct_update_handler )
{
	mc1000_state *state = space->machine->driver_data;

	if (state->rom0000)
	{
		if (address >= 0xc000)
		{
			memory_set_bank(space->machine, 1, 0);
			state->rom0000 = 0;
		}
	}

	return address;
}

static DRIVER_INIT( mc1000 )
{
	memory_set_direct_update_handler(cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM), mc1000_direct_update_handler);
}

/* System Drivers */

/*    YEAR	NAME		PARENT		COMPAT	MACHINE		INPUT		INIT		CONFIG		COMPANY				FULLNAME		FLAGS */
COMP( 1985,	mc1000,		0,			0,		mc1000,		mc1000,		mc1000,		mc1000,		"CCE",				"MC-1000",		GAME_IMPERFECT_GRAPHICS | GAME_SUPPORTS_SAVE )
