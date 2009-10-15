/*

    Telmac 2000E
    ------------
    (c) 1980 Telercas Oy, Finland

    CPU:        CDP1802A    1.75 MHz
    RAM:        8 KB
    ROM:        8 KB

    Video:      CDP1864     1.75 MHz
    Color RAM:  1 KB

    Colors:     8 fg, 4 bg
    Resolution: 64x192
    Sound:      frequency control, volume on/off
    Keyboard:   ASCII (RCA VP-601/VP-611), KB-16/KB-64

    SBASIC:     24.0


    Telmac TMC-121/111/112
    ----------------------
    (c) 198? Telercas Oy, Finland

    CPU:        CDP1802A    ? MHz

    Built from Telmac 2000 series cards. Huge metal box.

*/

#include "driver.h"
#include "includes/tmc2000e.h"
#include "cpu/cdp1802/cdp1802.h"
#include "devices/printer.h"
#include "devices/flopdrv.h"
#include "formats/basicdsk.h"
#include "devices/cassette.h"
#include "sound/cdp1864.h"
#include "machine/rescap.h"
#include "devices/messram.h"

/* Read/Write Handlers */

static READ8_HANDLER( vismac_r )
{
	return 0;
}

static WRITE8_HANDLER( vismac_w )
{
}

static READ8_HANDLER( floppy_r )
{
	return 0;
}

static WRITE8_HANDLER( floppy_w )
{
}

static READ8_HANDLER( ascii_keyboard_r )
{
	return 0;
}

static READ8_HANDLER( io_r )
{
	return 0;
}

static WRITE8_HANDLER( io_w )
{
}

static WRITE8_HANDLER( io_select_w )
{
}

static WRITE8_HANDLER( keyboard_latch_w )
{
	tmc2000e_state *state = space->machine->driver_data;

	state->keylatch = data;
}

/* Memory Maps */

static ADDRESS_MAP_START( tmc2000e_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_RAM
	AM_RANGE(0xc000, 0xdfff) AM_ROM
	AM_RANGE(0xfc00, 0xffff) AM_WRITEONLY AM_BASE_MEMBER(tmc2000e_state, colorram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( tmc2000e_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_DEVWRITE(CDP1864_TAG, cdp1864_tone_latch_w)
	AM_RANGE(0x02, 0x02) AM_DEVWRITE(CDP1864_TAG, cdp1864_step_bgcolor_w)
	AM_RANGE(0x03, 0x03) AM_READWRITE(ascii_keyboard_r, keyboard_latch_w)
	AM_RANGE(0x04, 0x04) AM_READWRITE(io_r, io_w)
	AM_RANGE(0x05, 0x05) AM_READWRITE(vismac_r, vismac_w)
	AM_RANGE(0x06, 0x06) AM_READWRITE(floppy_r, floppy_w)
	AM_RANGE(0x07, 0x07) AM_READ_PORT("DSW0") AM_WRITE(io_select_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( tmc2000e )
	PORT_START("DSW0")	// System Configuration DIPs
	PORT_DIPNAME( 0x80, 0x00, "Keyboard Type" )
	PORT_DIPSETTING(    0x00, "ASCII" )
	PORT_DIPSETTING(    0x80, "Matrix" )
	PORT_DIPNAME( 0x40, 0x00, "Operating System" )
	PORT_DIPSETTING(    0x00, "TOOL-2000-E" )
	PORT_DIPSETTING(    0x40, "Load from disk" )
	PORT_DIPNAME( 0x30, 0x00, "Display Interface" )
	PORT_DIPSETTING(    0x00, "PAL" )
	PORT_DIPSETTING(    0x10, "CDG-80" )
	PORT_DIPSETTING(    0x20, "VISMAC" )
	PORT_DIPSETTING(    0x30, "UART" )
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("RUN")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Run/Reset") PORT_CODE(KEYCODE_R) PORT_TOGGLE
INPUT_PORTS_END

/* Video */

static READ_LINE_DEVICE_HANDLER( rdata_r )
{
	tmc2000e_state *state = device->machine->driver_data;

	return BIT(state->color, 2);
}

static READ_LINE_DEVICE_HANDLER( bdata_r )
{
	tmc2000e_state *state = device->machine->driver_data;

	return BIT(state->color, 1);
}

static READ_LINE_DEVICE_HANDLER( gdata_r )
{
	tmc2000e_state *state = device->machine->driver_data;

	return BIT(state->color, 0);
}

static WRITE_LINE_DEVICE_HANDLER( tmc2000e_efx_w )
{
	tmc2000e_state *driver_state = device->machine->driver_data;

	driver_state->cdp1864_efx = state;
}

static CDP1864_INTERFACE( tmc2000e_cdp1864_intf )
{
	CDP1802_TAG,
	SCREEN_TAG,
	CDP1864_INTERLACED,
	DEVCB_LINE(rdata_r),
	DEVCB_LINE(bdata_r),
	DEVCB_LINE(gdata_r),
	DEVCB_CPU_INPUT_LINE(CDP1802_TAG, CDP1802_INPUT_LINE_INT),
	DEVCB_CPU_INPUT_LINE(CDP1802_TAG, CDP1802_INPUT_LINE_DMAOUT),
	DEVCB_LINE(tmc2000e_efx_w),
	RES_K(2.2),	// unverified
	RES_K(1),	// unverified
	RES_K(5.1),	// unverified
	RES_K(4.7)	// unverified
};

static VIDEO_UPDATE( tmc2000e )
{
	tmc2000e_state *state = screen->machine->driver_data;

	cdp1864_update(state->cdp1864, bitmap, cliprect);

	return 0;
}

/* CDP1802 Interface */

static CDP1802_MODE_READ( tmc2000e_mode_r )
{
	if (input_port_read(device->machine, "RUN") & 0x01)
	{
		return CDP1802_MODE_RUN;
	}
	else
	{
		return CDP1802_MODE_RESET;
	}
}

static CDP1802_EF_READ( tmc2000e_ef_r )
{
	tmc2000e_state *state = device->machine->driver_data;

	UINT8 flags = 0x0f;
	static const char *const keynames[] = { "IN0", "IN1", "IN2", "IN3", "IN4", "IN5", "IN6", "IN7" };

	/*
        EF1     CDP1864
        EF2     tape in/floppy
        EF3     keyboard
        EF4     I/O port
    */

	// CDP1864

	if (!state->cdp1864_efx) flags -= EF1;

	// tape in

	if (cassette_input(state->cassette) > +1.0) flags -= EF2;

	// keyboard

	if (~input_port_read(device->machine, keynames[state->keylatch / 8]) & (1 << (state->keylatch % 8))) flags -= EF3;

	return flags;
}

static WRITE_LINE_DEVICE_HANDLER( tmc2000e_q_w )
{
	tmc2000e_state *driver_state = device->machine->driver_data;

	// turn CDP1864 sound generator on/off

	cdp1864_aoe_w(driver_state->cdp1864, state);

	// set Q led status

	set_led_status(1, state);

	// tape out

	cassette_output(driver_state->cassette, state ? -1.0 : +1.0);

	// floppy control (FDC-6)
}

static WRITE8_DEVICE_HANDLER( tmc2000e_dma_w )
{
	tmc2000e_state *state = device->machine->driver_data;

	state->color = (state->colorram[offset & 0x3ff]) & 0x07; // 0x04 = R, 0x02 = B, 0x01 = G

	cdp1864_con_w(state->cdp1864, 0); // HACK
	cdp1864_dma_w(state->cdp1864, offset, data);
}

static CDP1802_INTERFACE( tmc2000e_config )
{
	tmc2000e_mode_r,
	tmc2000e_ef_r,
	NULL,
	DEVCB_LINE(tmc2000e_q_w),
	DEVCB_NULL,
	DEVCB_HANDLER(tmc2000e_dma_w)
};

/* Machine Initialization */

static MACHINE_START( tmc2000e )
{
	tmc2000e_state *state = machine->driver_data;

	/* allocate color RAM */
	state->colorram = auto_alloc_array(machine, UINT8, TMC2000E_COLORRAM_SIZE);

	/* find devices */
	state->cdp1864 = devtag_get_device(machine, CDP1864_TAG);
	state->cassette = devtag_get_device(machine, CASSETTE_TAG);

	/* register for state saving */
	state_save_register_global_pointer(machine, state->colorram, TMC2000E_COLORRAM_SIZE);
	state_save_register_global(machine, state->cdp1864_efx);
	state_save_register_global(machine, state->keylatch);
}

static MACHINE_RESET( tmc2000e )
{
	tmc2000e_state *state = machine->driver_data;

	device_reset(state->cdp1864);

	cputag_set_input_line(machine, CDP1802_TAG, INPUT_LINE_RESET, PULSE_LINE);

	// reset program counter to 0xc000
}

/* Machine Drivers */

static const cassette_config tmc2000_cassette_config =
{
	cassette_default_formats,
	NULL,
	CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_MUTED
};
static FLOPPY_OPTIONS_START(tmc2000e)
	// dsk
FLOPPY_OPTIONS_END

static const floppy_config tmc2000e_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(tmc2000e),
	DO_NOT_KEEP_GEOMETRY
};

static MACHINE_DRIVER_START( tmc2000e )
	MDRV_DRIVER_DATA(tmc2000e_state)

	// basic system hardware
	MDRV_CPU_ADD(CDP1802_TAG, CDP1802, XTAL_1_75MHz)
	MDRV_CPU_PROGRAM_MAP(tmc2000e_map)
	MDRV_CPU_IO_MAP(tmc2000e_io_map)
	MDRV_CPU_CONFIG(tmc2000e_config)

	MDRV_MACHINE_START(tmc2000e)
	MDRV_MACHINE_RESET(tmc2000e)

	// video hardware
	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_RAW_PARAMS(XTAL_1_75MHz, CDP1864_SCREEN_WIDTH, CDP1864_HBLANK_END, CDP1864_HBLANK_START, CDP1864_TOTAL_SCANLINES, CDP1864_SCANLINE_VBLANK_END, CDP1864_SCANLINE_VBLANK_START)

	MDRV_PALETTE_LENGTH(8)
	MDRV_VIDEO_UPDATE(tmc2000e)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_CDP1864_ADD(CDP1864_TAG, XTAL_1_75MHz, tmc2000e_cdp1864_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* devices */
	MDRV_PRINTER_ADD("printer")
	MDRV_CASSETTE_ADD("cassette", tmc2000_cassette_config)

	MDRV_FLOPPY_4_DRIVES_ADD(tmc2000e_floppy_config)
	
	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("8K")		
	MDRV_RAM_EXTRA_OPTIONS("40K")	
MACHINE_DRIVER_END

/* ROMs */

ROM_START( tmc2000e )
	ROM_REGION( 0x10000, CDP1802_TAG, 0 )
	ROM_LOAD( "1", 0xc000, 0x0800, NO_DUMP )
	ROM_LOAD( "2", 0xc800, 0x0800, NO_DUMP )
	ROM_LOAD( "3", 0xd000, 0x0800, NO_DUMP )
	ROM_LOAD( "4", 0xd800, 0x0800, NO_DUMP )
ROM_END

//    YEAR  NAME      PARENT   COMPAT   MACHINE   INPUT     INIT    CONFIG    COMPANY        FULLNAME
COMP( 1980, tmc2000e, 0,       0,	    tmc2000e, tmc2000e, 0,		0, "Telercas Oy", "Telmac 2000E", GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
