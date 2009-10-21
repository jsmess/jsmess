/*

	COSMICOS

	http://retro.hansotten.nl/index.php?page=1802-cosmicos

*/

/*

	TODO:

	- cdp1802 mode
	- quickload
	- ascii monitor
	- cassette led
	- switches
	- PPI 8255
	- Floppy WD1793
	- COM8017 UART to printer
	- save state

*/

#include "driver.h"
#include "includes/cosmicos.h"
#include "cpu/cdp1802/cdp1802.h"
#include "devices/cassette.h"
#include "devices/messram.h"
#include "machine/rescap.h"
#include "sound/cdp1864.h"
#include "video/dm9368.h"
#include "cosmicos.lh"

/* Read/Write Handlers */

static READ8_DEVICE_HANDLER( video_off_r )
{
	cosmicos_state *state = device->machine->driver_data;
	UINT8 data = 0;

	if (!state->q)
	{
		data = cdp1864_dispoff_r(device, 0);
	}

	return data;
}

static READ8_DEVICE_HANDLER( video_on_r )
{
	cosmicos_state *state = device->machine->driver_data;
	UINT8 data = 0;

	if (!state->q)
	{
		data = cdp1864_dispon_r(device, 0);
	}

	return data;
}

static WRITE8_DEVICE_HANDLER( audio_latch_w )
{
	cosmicos_state *state = device->machine->driver_data;

	if (state->q)
	{
		cdp1864_tone_latch_w(device, 0, data);
	}
}

static READ8_HANDLER( hex_keyboard_r )
{
	cosmicos_state *state = space->machine->driver_data;
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
	cosmicos_state *state = space->machine->driver_data;

	state->keylatch = data & 0x0f;
}

static READ8_HANDLER( reset_counter_r )
{
	cosmicos_state *state = space->machine->driver_data;

	state->counter = 0;

	return 0;
}

static WRITE8_HANDLER( segment_w )
{
	cosmicos_state *state = space->machine->driver_data;

	state->counter++;

	if (state->counter == 10)
	{
		state->counter = 0;
	}

	if ((state->counter > 0) && (state->counter < 9))
	{
		output_set_digit_value(state->counter + 2, data);
	}
}

/* Memory Maps */

static ADDRESS_MAP_START( cosmicos_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_RAMBANK(1)
	AM_RANGE(0xc000, 0xcfff) AM_ROM
	AM_RANGE(0xff00, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( cosmicos_io, ADDRESS_SPACE_IO, 8 )
//	AM_RANGE(0x00, 0x00)
	AM_RANGE(0x01, 0x01) AM_DEVREAD(CDP1864_TAG, video_on_r)
	AM_RANGE(0x02, 0x02) AM_DEVREADWRITE(CDP1864_TAG, video_off_r, audio_latch_w)
//	AM_RANGE(0x03, 0x03)
//	AM_RANGE(0x04, 0x04)
	AM_RANGE(0x05, 0x05) AM_READWRITE(hex_keyboard_r, hex_keylatch_w)
	AM_RANGE(0x06, 0x06) AM_READWRITE(reset_counter_r, segment_w)
//	AM_RANGE(0x07, 0x07) AM_READWRITE(data_r, display_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( cosmicos )
	PORT_START("DATA")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7')

	PORT_START("BUTTONS")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("ENTER")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SINGLE STEP")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RUN")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("LOAD")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PAUSE")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RESET")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("MEMORY PROTECT") PORT_TOGGLE

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
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("RET") PORT_CODE(KEYCODE_F1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("DEC") PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("REQ") PORT_CODE(KEYCODE_F3)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("SEQ") PORT_CODE(KEYCODE_F4)
INPUT_PORTS_END

/* Video */

static WRITE_LINE_DEVICE_HANDLER( cosmicos_dmaout_w )
{
	cosmicos_state *driver_state = device->machine->driver_data;

	driver_state->dmaout = state;
}

static WRITE_LINE_DEVICE_HANDLER( cosmicos_efx_w )
{
	cosmicos_state *driver_state = device->machine->driver_data;

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
	cosmicos_state *state = screen->machine->driver_data;

	cdp1864_update(state->cdp1864, bitmap, cliprect);

	return 0;
}

/* CDP1802 Configuration */

static CDP1802_MODE_READ( cosmicos_mode_r )
{
	cdp1802_control_mode mode = CDP1802_MODE_RUN;

	return mode;
}

static CDP1802_EF_READ( cosmicos_ef_r )
{
	/*
        EF1     
        EF2		cassette input
        EF3
        EF4     
    */

	cosmicos_state *state = device->machine->driver_data;

	UINT8 flags = 0x0f;

	/* cassette input */
	if (cassette_input(state->cassette) < 0.0) flags -= EF2;

	return flags;
}

static CDP1802_SC_WRITE( cosmicos_sc_w )
{
}

static WRITE_LINE_DEVICE_HANDLER( cosmicos_q_w )
{
	cosmicos_state *driver_state = device->machine->driver_data;

	/* cassette */
	cassette_output(driver_state->cassette, state ? +1.0 : -1.0);

	/* boot */
	if (state) driver_state->boot = 0;

	/* CDP1864 audio enable */
	cdp1864_aoe_w(driver_state->cdp1864, state);

	driver_state->q = state;
}

static CDP1802_INTERFACE( cosmicos_config )
{
	cosmicos_mode_r,
	cosmicos_ef_r,
	cosmicos_sc_w,
	DEVCB_LINE(cosmicos_q_w),
	DEVCB_NULL,
	DEVCB_NULL
};

/* Machine Initialization */

static MACHINE_START( cosmicos )
{
	cosmicos_state *state = machine->driver_data;
	const address_space *program = cputag_get_address_space(machine, CDP1802_TAG, ADDRESS_SPACE_PROGRAM);

	/* find devices */
	state->dm9368 = devtag_get_device(machine, DM9368_TAG);
	state->cdp1864 = devtag_get_device(machine, CDP1864_TAG);
	state->cassette = devtag_get_device(machine, CASSETTE_TAG);

	/* set initial values */
	state->boot = 1;

	/* initialize LED display */
	dm9368_rbi_w(state->dm9368, 1);

	/* setup memory banking */
	memory_configure_bank(machine, 1, 0, 1, memory_region(machine, CDP1802_TAG), 0);
	memory_set_bank(machine, 1, 0);

	switch (messram_get_size(devtag_get_device(machine, "messram")))
	{
	case 256:
		//memory_install_readwrite8_handler(program, 0x0000, 0x00ff, 0, 0, SMH_BANK(1), SMH_BANK(1));
		//memory_install_readwrite8_handler(program, 0x0100, 0xbfff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		break;

	case 4*1024:
		memory_install_readwrite8_handler(program, 0x0000, 0x0fff, 0, 0, SMH_BANK(1), SMH_BANK(1));
		memory_install_readwrite8_handler(program, 0x1000, 0xbfff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		break;

	case 48*1024:
		memory_install_readwrite8_handler(program, 0x0000, 0xbfff, 0, 0, SMH_BANK(1), SMH_BANK(1));
		break;
	}

	/* register for state saving */
//	state_save_register_global(machine, state->);
}

/* Machine Driver */

static const cassette_config cosmicos_cassette_config =
{
	cassette_default_formats,
	NULL,
	CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_MUTED
};

static MACHINE_DRIVER_START( cosmicos )
	MDRV_DRIVER_DATA(cosmicos_state)

	/* basic machine hardware */
    MDRV_CPU_ADD(CDP1802_TAG, CDP1802, XTAL_1_75MHz)
    MDRV_CPU_PROGRAM_MAP(cosmicos_mem)
    MDRV_CPU_IO_MAP(cosmicos_io)
	MDRV_CPU_CONFIG(cosmicos_config)

    MDRV_MACHINE_START(cosmicos)

    /* video hardware */
	MDRV_DEFAULT_LAYOUT( layout_cosmicos )
	MDRV_DM9368_ADD(DM9368_TAG, 0, NULL)

	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_RAW_PARAMS(XTAL_1_75MHz, CDP1864_SCREEN_WIDTH, CDP1864_HBLANK_END, CDP1864_HBLANK_START, CDP1864_TOTAL_SCANLINES, CDP1864_SCANLINE_VBLANK_END, CDP1864_SCANLINE_VBLANK_START)

	MDRV_PALETTE_LENGTH(8+8)
	MDRV_VIDEO_UPDATE(cosmicos)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_CDP1864_ADD(CDP1864_TAG, XTAL_1_75MHz, cosmicos_cdp1864_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* devices */
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
	cosmicos_state *state = space->machine->driver_data;

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

/*    YEAR  NAME		PARENT  COMPAT  MACHINE		INPUT		INIT		CONFIG		COMPANY				FULLNAME    FLAGS */
COMP( 1979, cosmicos,	0,		0,		cosmicos,	cosmicos,	cosmicos,	0,			"Radio Bulletin",	"Cosmicos",	GAME_NOT_WORKING )
