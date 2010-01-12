#include "driver.h"
#include "includes/eti660.h"
#include "cpu/cdp1802/cdp1802.h"
#include "devices/cassette.h"
#include "devices/messram.h"
#include "machine/6821pia.h"
#include "machine/rescap.h"
#include "sound/cdp1864.h"

/* Read/Write Handlers */

static WRITE8_HANDLER( eti660_colorram_w )
{
}

/* Memory Maps */

static ADDRESS_MAP_START( eti660_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_ROM
	AM_RANGE(0x0400, 0x0fff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( eti660_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_DEVREADWRITE(CDP1864_TAG, cdp1864_dispon_r, cdp1864_step_bgcolor_w)
	AM_RANGE(0x03, 0x03) AM_WRITE(eti660_colorram_w)
	AM_RANGE(0x04, 0x04) AM_DEVREADWRITE(CDP1864_TAG, cdp1864_dispoff_r, cdp1864_tone_latch_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( eti660 )
INPUT_PORTS_END

/* Video */

static WRITE_LINE_DEVICE_HANDLER( eti660_efx_w )
{
	eti660_state *driver_state = device->machine->driver_data;

	driver_state->cdp1864_efx = state;
}

static VIDEO_UPDATE( eti660 )
{
	eti660_state *state = screen->machine->driver_data;

	cdp1864_update(state->cdp1864, bitmap, cliprect);

	return 0;
}

static CDP1864_INTERFACE( eti660_cdp1864_intf )
{
	CDP1802_TAG,
	SCREEN_TAG,
	CDP1864_INTERLACED,
	DEVCB_LINE_VCC,
	DEVCB_LINE_VCC,
	DEVCB_LINE_VCC,
	DEVCB_CPU_INPUT_LINE(CDP1802_TAG, CDP1802_INPUT_LINE_INT),
	DEVCB_CPU_INPUT_LINE(CDP1802_TAG, CDP1802_INPUT_LINE_DMAOUT),
	DEVCB_LINE(eti660_efx_w),
	RES_K(2.2), /* R7 */
	RES_K(1),	/* R5 */
	RES_K(4.7), /* R6 */
	RES_K(4.7)	/* R4 */
};

/* CDP1802 Interface */

static CDP1802_MODE_READ( eti660_mode_r )
{
	eti660_state *state = device->machine->driver_data;

	return state->cdp1802_mode;
}

static CDP1802_EF_READ( eti660_ef_r )
{
	eti660_state *state = device->machine->driver_data;

	int ef = 0x0f;

	/* CDP1864 EFx */
	if (state->cdp1864_efx) ef -= EF1;

	/* tape input */
	if (cassette_input(state->cassette) < 0) ef -= EF2;

	return ef;
}

static WRITE_LINE_DEVICE_HANDLER( eti660_q_w )
{
	eti660_state *driver_state = device->machine->driver_data;

	/* CDP1864 audio output enable */
	cdp1864_aoe_w(driver_state->cdp1864, state);

	/* PULSE led */
	set_led_status(device->machine, LED_PULSE, state);

	/* tape output */
	cassette_output(driver_state->cassette, state ? 1.0 : -1.0);
}

static WRITE8_DEVICE_HANDLER( eti660_dma_w )
{
	eti660_state *state = device->machine->driver_data;

	state->color = state->color_ram[offset / 4];

	cdp1864_con_w(state->cdp1864, 0); // HACK
	cdp1864_dma_w(state->cdp1864, offset, data);
}

static CDP1802_INTERFACE( eti660_config )
{
	eti660_mode_r,
	eti660_ef_r,
	NULL,
	DEVCB_LINE(eti660_q_w),
	DEVCB_NULL,
	DEVCB_HANDLER(eti660_dma_w)
};

/* PIA6821 Interface */

static READ8_DEVICE_HANDLER( eti660_pia_a_r )
{
	/*

        bit     description

        PA0     keyboard row 0
        PA1     keyboard row 1
        PA2     keyboard row 2
        PA3     keyboard row 3
        PA4     keyboard column 0
        PA5     keyboard column 1
        PA6     keyboard column 2
        PA7     keyboard column 3

    */

//	eti660_state *state = device->machine->driver_data;

	return 0xf0;
}

static WRITE8_DEVICE_HANDLER( eti660_pia_a_w )
{
	/*

        bit     description

        PA0     keyboard row 0
        PA1     keyboard row 1
        PA2     keyboard row 2
        PA3     keyboard row 3
        PA4     keyboard column 0
        PA5     keyboard column 1
        PA6     keyboard column 2
        PA7     keyboard column 3

    */

	eti660_state *state = device->machine->driver_data;

	state->keylatch = data & 0x0f;
}

static const pia6821_interface bw12_pia_config =
{
	DEVCB_HANDLER(eti660_pia_a_r),								/* port A input */
	DEVCB_NULL,													/* port B input */
	DEVCB_NULL,													/* CA1 input */
	DEVCB_NULL,													/* CB1 input */
	DEVCB_NULL,													/* CA2 input */
	DEVCB_NULL,													/* CB2 input */
	DEVCB_HANDLER(eti660_pia_a_w),								/* port A output */
	DEVCB_NULL,													/* port B output */
	DEVCB_NULL,													/* CA2 output */
	DEVCB_NULL,													/* CB2 output */
	DEVCB_NULL,DEVCB_NULL
//	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),				/* IRQA output */
//	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0)				/* IRQB output */
};

/* Machine Initialization */

static MACHINE_START( eti660 )
{
	eti660_state *state = machine->driver_data;

	/* find devices */
	state->cdp1864 = devtag_get_device(machine, CDP1864_TAG);
	state->cassette = devtag_get_device(machine, CASSETTE_TAG);

	/* register for state saving */
	state_save_register_global(machine, state->cdp1802_mode);
	state_save_register_global(machine, state->cdp1864_efx);
}

/* Machine Drivers */

static const cassette_config eti660_cassette_config =
{
	cassette_default_formats,
	NULL,
	CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_MUTED
};

static MACHINE_DRIVER_START( eti660 )
	MDRV_DRIVER_DATA(eti660_state)

	/* basic machine hardware */
	MDRV_CPU_ADD(CDP1802_TAG, CDP1802, XTAL_8_867238MHz/5)
	MDRV_CPU_PROGRAM_MAP(eti660_map)
	MDRV_CPU_IO_MAP(eti660_io_map)
	MDRV_CPU_CONFIG(eti660_config)

	MDRV_MACHINE_START(eti660)

    /* video hardware */
	MDRV_CDP1864_SCREEN_ADD(SCREEN_TAG, XTAL_8_867238MHz/5)

	MDRV_PALETTE_LENGTH(8+8)
	MDRV_VIDEO_UPDATE(eti660)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_CDP1864_ADD(CDP1864_TAG, XTAL_8_867238MHz/5, eti660_cdp1864_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* devices */
//	MDRV_PIA6821_ADD(MC6821_TAG, eti660_mc6821_intf)
	MDRV_CASSETTE_ADD("cassette", eti660_cassette_config)
	
	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("1K")
	MDRV_RAM_EXTRA_OPTIONS("3K")
MACHINE_DRIVER_END

/* ROMs */

ROM_START( eti660 )
	ROM_REGION( 0x10000, CDP1802_TAG, 0 )
	ROM_LOAD( "eti660.bin", 0x0000, 0x0400, CRC(811dfa62) SHA1(c0c4951e02f873f15560bdc3f35cdf3f99653922) )
ROM_END

/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT       INIT        COMPANY								FULLNAME				FLAGS */
COMP( 1978, eti660,		0,		0,		eti660,		eti660,		0,			"Electronics Today International",	"ETI-660 (Australia)",	GAME_NOT_WORKING )
