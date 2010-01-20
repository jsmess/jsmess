/*

    COSMICOS

    http://retro.hansotten.nl/index.php?page=1802-cosmicos


    HEX-monitor

    0 - start user program
    1 - inspect and/or change memory
    2 - write memory block to cassette
    3 - read memory block from cassette
    4 - move memory block
    5 - write memory block to EPROM
    C - start user program from address 0000

*/

/*

    TODO:

    - display interface INH
    - 2 segment display
    - single step
    - ascii monitor
    - PPI 8255
    - Floppy WD1793
    - COM8017 UART to printer

*/

#include "emu.h"
#include "includes/cosmicos.h"
#include "cpu/cdp1802/cdp1802.h"
#include "devices/cassette.h"
#include "devices/messram.h"
#include "devices/snapquik.h"
#include "machine/rescap.h"
#include "sound/cdp1864.h"
#include "sound/speaker.h"
#include "video/dm9368.h"
#include "cosmicos.lh"

/* Read/Write Handlers */

static READ8_DEVICE_HANDLER( video_off_r )
{
	cosmicos_state *state = (cosmicos_state *)device->machine->driver_data;
	UINT8 data = 0;

	if (!state->q)
	{
		data = cdp1864_dispoff_r(device, 0);
	}

	return data;
}

static READ8_DEVICE_HANDLER( video_on_r )
{
	cosmicos_state *state = (cosmicos_state *)device->machine->driver_data;
	UINT8 data = 0;

	if (!state->q)
	{
		data = cdp1864_dispon_r(device, 0);
	}

	return data;
}

static WRITE8_DEVICE_HANDLER( audio_latch_w )
{
	cosmicos_state *state = (cosmicos_state *)device->machine->driver_data;

	if (state->q)
	{
		cdp1864_tone_latch_w(device, 0, data);
	}
}

static READ8_HANDLER( hex_keyboard_r )
{
	cosmicos_state *state = (cosmicos_state *)space->machine->driver_data;
	static const char *const keynames[] = { "ROW1", "ROW2", "ROW3", "ROW4" };
	UINT8 data = 0;
	int i;

	for (i = 0; i < 4; i++)
	{
		if (BIT(state->keylatch, i))
		{
			UINT8 keydata = input_port_read(space->machine, keynames[i]);

			if (BIT(keydata, 0)) data |= 0x01;
			if (BIT(keydata, 1)) data |= 0x02;
			if (BIT(keydata, 2)) data |= 0x04;
			if (BIT(keydata, 3)) data |= 0x06;
		}
	}

	return data;
}

static WRITE8_HANDLER( hex_keylatch_w )
{
	cosmicos_state *state = (cosmicos_state *)space->machine->driver_data;

	state->keylatch = data & 0x0f;
}

static READ8_HANDLER( reset_counter_r )
{
	cosmicos_state *state = (cosmicos_state *)space->machine->driver_data;

	state->counter = 0;

	return 0;
}

static WRITE8_HANDLER( segment_w )
{
	cosmicos_state *state = (cosmicos_state *)space->machine->driver_data;

	state->counter++;

	if (state->counter == 10)
	{
		state->counter = 0;
	}

	if ((state->counter > 0) && (state->counter < 9))
	{
		output_set_digit_value(10 - state->counter, data);
	}
}

static READ8_HANDLER( data_r )
{
	cosmicos_state *state = (cosmicos_state *)space->machine->driver_data;

	return state->data;
}

static WRITE8_HANDLER( display_w )
{
	cosmicos_state *state = (cosmicos_state *)space->machine->driver_data;

	state->segment = data;
}

/* Memory Maps */

static ADDRESS_MAP_START( cosmicos_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_RAMBANK("bank1")
	AM_RANGE(0xc000, 0xcfff) AM_ROM
	AM_RANGE(0xff00, 0xffff) AM_RAMBANK("bank2")
ADDRESS_MAP_END

static ADDRESS_MAP_START( cosmicos_io, ADDRESS_SPACE_IO, 8 )
//  AM_RANGE(0x00, 0x00)
	AM_RANGE(0x01, 0x01) AM_DEVREAD(CDP1864_TAG, video_on_r)
	AM_RANGE(0x02, 0x02) AM_DEVREADWRITE(CDP1864_TAG, video_off_r, audio_latch_w)
//  AM_RANGE(0x03, 0x03)
//  AM_RANGE(0x04, 0x04)
	AM_RANGE(0x05, 0x05) AM_READWRITE(hex_keyboard_r, hex_keylatch_w)
	AM_RANGE(0x06, 0x06) AM_READWRITE(reset_counter_r, segment_w)
	AM_RANGE(0x07, 0x07) AM_READWRITE(data_r, display_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_CHANGED( data )
{
	cosmicos_state *state = (cosmicos_state *)field->port->machine->driver_data;
	UINT8 data = input_port_read(field->port->machine, "DATA");
	int i;

	for (i = 0; i < 8; i++)
	{
		if (!BIT(data, i))
		{
			state->data |= (1 << i);
			output_set_led_value(LED_D0 - i, 1);
		}
	}
}

static INPUT_CHANGED( enter )
{
	cosmicos_state *state = (cosmicos_state *)field->port->machine->driver_data;

	if (!newval && (state->cdp1802_mode == CDP1802_MODE_LOAD))
	{
		cputag_set_input_line(field->port->machine, CDP1802_TAG, CDP1802_INPUT_LINE_DMAIN, ASSERT_LINE);
	}
}

static INPUT_CHANGED( single_step )
{
	// if in PAUSE mode, set RUN mode until TPB=active
}

static void set_cdp1802_mode(running_machine *machine, cdp1802_control_mode mode)
{
	cosmicos_state *state = (cosmicos_state *)machine->driver_data;

	state->cdp1802_mode = mode;

	output_set_led_value(LED_RUN, 0);
	output_set_led_value(LED_LOAD, 0);
	output_set_led_value(LED_PAUSE, 0);
	output_set_led_value(LED_RESET, 0);

	switch (mode)
	{
	case CDP1802_MODE_RUN:
		output_set_led_value(LED_RUN, 1);
		break;

	case CDP1802_MODE_LOAD:
		output_set_led_value(LED_LOAD, 1);
		break;

	case CDP1802_MODE_PAUSE:
		output_set_led_value(LED_PAUSE, 1);
		break;

	case CDP1802_MODE_RESET:
		cputag_set_input_line(machine, CDP1802_TAG, CDP1802_INPUT_LINE_INT, CLEAR_LINE);
		cputag_set_input_line(machine, CDP1802_TAG, CDP1802_INPUT_LINE_DMAIN, CLEAR_LINE);

		state->boot = 1;

		output_set_led_value(LED_RESET, 1);
		break;
	}
}

static INPUT_CHANGED( run )   { if (!newval) set_cdp1802_mode(field->port->machine, CDP1802_MODE_RUN); }
static INPUT_CHANGED( load )  { if (!newval) set_cdp1802_mode(field->port->machine, CDP1802_MODE_LOAD); }
static INPUT_CHANGED( cosmicos_pause ) {	if (!newval) set_cdp1802_mode(field->port->machine, CDP1802_MODE_PAUSE); }
static INPUT_CHANGED( reset ) {	if (!newval) set_cdp1802_mode(field->port->machine, CDP1802_MODE_RESET); }

static void clear_input_data(running_machine *machine)
{
	cosmicos_state *state = (cosmicos_state *)machine->driver_data;
	int i;

	state->data = 0;

	for (i = 0; i < 8; i++)
	{
		output_set_led_value(LED_D0 - i, 0);
	}
}

static INPUT_CHANGED( clear_data )
{
	clear_input_data(field->port->machine);
}

static void set_ram_mode(running_machine *machine)
{
	cosmicos_state *state = (cosmicos_state *)machine->driver_data;
	const address_space *program = cputag_get_address_space(machine, CDP1802_TAG, ADDRESS_SPACE_PROGRAM);

	if (state->ram_disable)
	{
		memory_unmap_read(program, 0xff00, 0xffff, 0, 0);
		memory_unmap_write(program, 0xff00, 0xffff, 0, 0);
	}
	else
	{
		if (state->ram_protect)
		{
			memory_install_read_bank(program, 0xff00, 0xffff, 0, 0, "bank2");
			memory_unmap_write(program, 0xff00, 0xffff, 0, 0);
		}
		else
		{
			memory_install_read_bank(program, 0xff00, 0xffff, 0, 0, "bank2");
			memory_install_write_bank(program, 0xff00, 0xffff, 0, 0, "bank2");
		}
	}
}

static INPUT_CHANGED( memory_protect )
{
	cosmicos_state *state = (cosmicos_state *)field->port->machine->driver_data;

	state->ram_protect = newval;

	set_ram_mode(field->port->machine);
}

static INPUT_CHANGED( memory_disable )
{
	cosmicos_state *state = (cosmicos_state *)field->port->machine->driver_data;

	state->ram_disable = newval;

	set_ram_mode(field->port->machine);
}

static INPUT_PORTS_START( cosmicos )
	PORT_START("DATA")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("D0") PORT_CODE(KEYCODE_0_PAD) PORT_CHAR('0') PORT_CHANGED(data, 0)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("D1") PORT_CODE(KEYCODE_1_PAD) PORT_CHAR('1') PORT_CHANGED(data, 0)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("D2") PORT_CODE(KEYCODE_2_PAD) PORT_CHAR('2') PORT_CHANGED(data, 0)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("D3") PORT_CODE(KEYCODE_3_PAD) PORT_CHAR('3') PORT_CHANGED(data, 0)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("D4") PORT_CODE(KEYCODE_4_PAD) PORT_CHAR('4') PORT_CHANGED(data, 0)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("D5") PORT_CODE(KEYCODE_5_PAD) PORT_CHAR('5') PORT_CHANGED(data, 0)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("D6") PORT_CODE(KEYCODE_6_PAD) PORT_CHAR('6') PORT_CHANGED(data, 0)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("D7") PORT_CODE(KEYCODE_7_PAD) PORT_CHAR('7') PORT_CHANGED(data, 0)

	PORT_START("BUTTONS")
	PORT_BIT( 0x001, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_ENTER_PAD) PORT_NAME("Enter") PORT_CHANGED(enter, 0)
	PORT_BIT( 0x002, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SPACE) PORT_NAME("Single Step") PORT_CHANGED(single_step, 0)
	PORT_BIT( 0x004, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_NAME("Run") PORT_CHANGED(run, 0)
	PORT_BIT( 0x008, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_NAME("Load") PORT_CHANGED(load, 0)
	PORT_BIT( 0x010, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_NAME(DEF_STR( Pause )) PORT_CHANGED(cosmicos_pause, 0)
	PORT_BIT( 0x020, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_NAME("Reset") PORT_CHANGED(reset, 0)
	PORT_BIT( 0x040, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_DEL_PAD) PORT_NAME("Clear Data") PORT_CHANGED(clear_data, 0)
	PORT_BIT( 0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_NAME("Memory Protect") PORT_CHANGED(memory_protect, 0) PORT_TOGGLE
	PORT_BIT( 0x100, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_NAME("Memory Disable") PORT_CHANGED(memory_disable, 0) PORT_TOGGLE

	PORT_START("ROW1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3')

	PORT_START("ROW2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7')

	PORT_START("ROW3")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')

	PORT_START("ROW4")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')

	PORT_START("SPECIAL")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RET") PORT_CODE(KEYCODE_F1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("DEC") PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("REQ") PORT_CODE(KEYCODE_F3)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SEQ") PORT_CODE(KEYCODE_F4)
INPUT_PORTS_END

/* Video */

static TIMER_DEVICE_CALLBACK( digit_tick )
{
	cosmicos_state *state = (cosmicos_state *)timer->machine->driver_data;

	state->digit = !state->digit;

	output_set_digit_value(state->digit, state->segment);
}

static TIMER_DEVICE_CALLBACK( int_tick )
{
	cputag_set_input_line(timer->machine, CDP1802_TAG, CDP1802_INPUT_LINE_INT, ASSERT_LINE);
}

static WRITE_LINE_DEVICE_HANDLER( cosmicos_dmaout_w )
{
	cosmicos_state *driver_state = (cosmicos_state *)device->machine->driver_data;

	driver_state->dmaout = state;
}

static WRITE_LINE_DEVICE_HANDLER( cosmicos_efx_w )
{
	cosmicos_state *driver_state = (cosmicos_state *)device->machine->driver_data;

	driver_state->efx = state;
}

static CDP1864_INTERFACE( cosmicos_cdp1864_intf )
{
	CDP1802_TAG,
	SCREEN_TAG,
	CDP1864_INTERLACED,
	DEVCB_LINE_VCC,
	DEVCB_LINE_VCC,
	DEVCB_LINE_VCC,
	DEVCB_CPU_INPUT_LINE(CDP1802_TAG, CDP1802_INPUT_LINE_INT),
	DEVCB_LINE(cosmicos_dmaout_w),
	DEVCB_LINE(cosmicos_efx_w),
	RES_K(2), // R2
	0, // not connected
	0, // not connected
	0  // not connected
};

static VIDEO_UPDATE( cosmicos )
{
	cosmicos_state *state = (cosmicos_state *)screen->machine->driver_data;

	cdp1864_update(state->cdp1864, bitmap, cliprect);

	return 0;
}

/* CDP1802 Configuration */

static CDP1802_MODE_READ( cosmicos_mode_r )
{
	cosmicos_state *state = (cosmicos_state *)device->machine->driver_data;

	return state->cdp1802_mode;
}

static CDP1802_EF_READ( cosmicos_ef_r )
{
	/*
        EF1
        EF2     cassette input
        EF3
        EF4     ENTER
    */

	cosmicos_state *state = (cosmicos_state *)device->machine->driver_data;
	UINT8 special = input_port_read(device->machine, "SPECIAL");
	UINT8 flags = 0x0f;

	/* cassette input */
	if (cassette_input(state->cassette) < 0.0)
	{
		output_set_led_value(LED_CASSETTE, 1);
		flags &= ~EF2;
	}
	else
	{
		output_set_led_value(LED_CASSETTE, 0);
	}

	/* ENTER */
	if (!BIT(input_port_read(device->machine, "BUTTONS"), 0)) flags &= ~EF4;

	/* hexadecimal keypad */
	if (!BIT(special, 0)) flags &= ~EF1;
	if (!BIT(special, 1)) flags &= ~EF2;
	if (!BIT(special, 2)) flags &= ~EF3;
	if (!BIT(special, 3)) flags &= ~(EF2 | EF3);

	return flags;
}

static CDP1802_SC_WRITE( cosmicos_sc_w )
{
	cosmicos_state *driver_state = (cosmicos_state *)device->machine->driver_data;

	if (driver_state->sc1 && !sc1)
	{
		clear_input_data(device->machine);
	}

	if (sc1)
	{
		cpu_set_input_line(device, CDP1802_INPUT_LINE_INT, CLEAR_LINE);
		cpu_set_input_line(device, CDP1802_INPUT_LINE_DMAIN, CLEAR_LINE);
	}

	driver_state->sc1 = sc1;
}

static WRITE_LINE_DEVICE_HANDLER( cosmicos_q_w )
{
	cosmicos_state *driver_state = (cosmicos_state *)device->machine->driver_data;

	/* cassette */
	cassette_output(driver_state->cassette, state ? +1.0 : -1.0);

	/* boot */
	if (state) driver_state->boot = 0;

	/* CDP1864 audio enable */
	cdp1864_aoe_w(driver_state->cdp1864, state);

	driver_state->q = state;
}

static READ8_DEVICE_HANDLER( cosmicos_dma_r )
{
	cosmicos_state *state = (cosmicos_state *)device->machine->driver_data;

	return state->data;
}

static CDP1802_INTERFACE( cosmicos_config )
{
	cosmicos_mode_r,
	cosmicos_ef_r,
	cosmicos_sc_w,
	DEVCB_LINE(cosmicos_q_w),
	DEVCB_HANDLER(cosmicos_dma_r),
	DEVCB_NULL
};

/* Machine Initialization */

static MACHINE_START( cosmicos )
{
	cosmicos_state *state = (cosmicos_state *)machine->driver_data;
	const address_space *program = cputag_get_address_space(machine, CDP1802_TAG, ADDRESS_SPACE_PROGRAM);

	/* find devices */
	state->dm9368 = devtag_get_device(machine, DM9368_TAG);
	state->cdp1864 = devtag_get_device(machine, CDP1864_TAG);
	state->cassette = devtag_get_device(machine, CASSETTE_TAG);
	state->speaker = devtag_get_device(machine, SPEAKER_TAG);

	/* initialize LED display */
	dm9368_rbi_w(state->dm9368, 1);

	/* setup memory banking */
	memory_configure_bank(machine, "bank1", 0, 1, memory_region(machine, CDP1802_TAG), 0);
	memory_set_bank(machine, "bank1", 0);

	memory_configure_bank(machine, "bank2", 0, 1, memory_region(machine, CDP1802_TAG) + 0xff00, 0);
	memory_set_bank(machine, "bank2", 0);

	switch (messram_get_size(devtag_get_device(machine, "messram")))
	{
	case 256:
		memory_unmap_readwrite(program, 0x0000, 0xbfff, 0, 0);
		break;

	case 4*1024:
		memory_install_readwrite_bank(program, 0x0000, 0x0fff, 0, 0, "bank1");
		memory_unmap_readwrite(program, 0x1000, 0xbfff, 0, 0);
		break;

	case 48*1024:
		memory_install_readwrite_bank(program, 0x0000, 0xbfff, 0, 0, "bank1");
		break;
	}

	memory_install_readwrite_bank(program, 0xff00, 0xffff, 0, 0, "bank2");

	/* register for state saving */
	state_save_register_global(machine, state->cdp1802_mode);
	state_save_register_global(machine, state->sc1);
	state_save_register_global(machine, state->data);
	state_save_register_global(machine, state->boot);
	state_save_register_global(machine, state->ram_protect);
	state_save_register_global(machine, state->ram_disable);
	state_save_register_global(machine, state->keylatch);
	state_save_register_global(machine, state->segment);
	state_save_register_global(machine, state->digit);
	state_save_register_global(machine, state->counter);
	state_save_register_global(machine, state->q);
	state_save_register_global(machine, state->dmaout);
	state_save_register_global(machine, state->efx);
	state_save_register_global(machine, state->video_on);
}

static MACHINE_RESET( cosmicos )
{
	set_cdp1802_mode(machine, CDP1802_MODE_RESET);
}

/* Quickload */

static QUICKLOAD_LOAD( cosmicos )
{
	UINT8 *ptr = memory_region(image->machine, CDP1802_TAG);
	int size = image_length(image);

	/* load image to RAM */
	image_fread(image, ptr, size);

	return INIT_PASS;
}

/* Machine Driver */

static const cassette_config cosmicos_cassette_config =
{
	cassette_default_formats,
	NULL,
	(cassette_state)(CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_MUTED)
};

static MACHINE_DRIVER_START( cosmicos )
	MDRV_DRIVER_DATA(cosmicos_state)

	/* basic machine hardware */
    MDRV_CPU_ADD(CDP1802_TAG, CDP1802, XTAL_1_75MHz)
    MDRV_CPU_PROGRAM_MAP(cosmicos_mem)
    MDRV_CPU_IO_MAP(cosmicos_io)
	MDRV_CPU_CONFIG(cosmicos_config)

    MDRV_MACHINE_START(cosmicos)
    MDRV_MACHINE_RESET(cosmicos)

    /* video hardware */
	MDRV_DEFAULT_LAYOUT( layout_cosmicos )
	MDRV_DM9368_ADD(DM9368_TAG, 0, NULL)
	MDRV_TIMER_ADD_PERIODIC("digit", digit_tick, HZ(100))
	MDRV_TIMER_ADD_PERIODIC("interrupt", int_tick, HZ(1000))

	MDRV_CDP1864_SCREEN_ADD(SCREEN_TAG, XTAL_1_75MHz)

	MDRV_PALETTE_LENGTH(8+8)
	MDRV_VIDEO_UPDATE(cosmicos)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SPEAKER_TAG, SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_CDP1864_ADD(CDP1864_TAG, XTAL_1_75MHz, cosmicos_cdp1864_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* devices */
	MDRV_QUICKLOAD_ADD("quickload", cosmicos, "bin", 0)
	MDRV_CASSETTE_ADD(CASSETTE_TAG, cosmicos_cassette_config)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("256")
	MDRV_RAM_EXTRA_OPTIONS("4K,48K")
MACHINE_DRIVER_END

/* ROMs */

ROM_START( cosmicos )
	ROM_REGION( 0x10000, CDP1802_TAG, 0 )
	ROM_SYSTEM_BIOS( 0, "hex", "Hex Monitor" )
	ROMX_LOAD( "hex.ic6",	0xc000, 0x0400, BAD_DUMP CRC(d25124bf) SHA1(121215ba3a979e1962327ebe73cbadf784c568d9), ROM_BIOS(1) ) /* typed in from manual */
	ROMX_LOAD( "hex.ic7",	0xc400, 0x0400, BAD_DUMP CRC(364ac81b) SHA1(83936ee6a7ed44632eb290889b98fb9a500f15d4), ROM_BIOS(1) ) /* typed in from manual */
	ROM_SYSTEM_BIOS( 1, "ascii", "ASCII Monitor" )
	ROMX_LOAD( "ascii.ic6", 0xc000, 0x0400, NO_DUMP, ROM_BIOS(2) )
	ROMX_LOAD( "ascii.ic7", 0xc400, 0x0400, NO_DUMP, ROM_BIOS(2) )
ROM_END

/* System Drivers */

static DIRECT_UPDATE_HANDLER( cosmicos_direct_update_handler )
{
	cosmicos_state *state = (cosmicos_state *)space->machine->driver_data;

	if (state->boot)
	{
		/* force A6 and A7 high */
		return address | 0xc0c0;
	}

	return address;
}

static DRIVER_INIT( cosmicos )
{
	const address_space *program = cputag_get_address_space(machine, CDP1802_TAG, ADDRESS_SPACE_PROGRAM);

	memory_set_direct_update_handler(program, cosmicos_direct_update_handler);
}

/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT       INIT        COMPANY             FULLNAME    FLAGS */
COMP( 1979, cosmicos,	0,		0,		cosmicos,	cosmicos,	cosmicos,	"Radio Bulletin",	"Cosmicos",	GAME_SUPPORTS_SAVE | GAME_IMPERFECT_GRAPHICS )
