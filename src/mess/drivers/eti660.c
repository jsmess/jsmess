/*

    TODO:

    - does not boot!
    - allocate color ram
    - quickload
    - color on

*/

#include "emu.h"
#include "includes/eti660.h"
#include "cpu/cosmac/cosmac.h"
#include "devices/cassette.h"
#include "devices/messram.h"
#include "machine/6821pia.h"
#include "machine/rescap.h"
#include "sound/cdp1864.h"

static MACHINE_RESET( eti660 );

#define RX(_machine) \
	cpu_get_reg(_machine->firstcpu, COSMAC_R0 + cpu_get_reg(_machine->firstcpu, COSMAC_X))

/* Read/Write Handlers */

static READ8_DEVICE_HANDLER( pia_r )
{
	int pia_offset = RX(device->machine) & 0x03;

	return pia6821_r(device, pia_offset);
}

static WRITE8_DEVICE_HANDLER( pia_w )
{
	int pia_offset = RX(device->machine) & 0x03;

	pia6821_w(device, pia_offset, data);
}

static WRITE8_HANDLER( eti660_colorram_w )
{
	eti660_state *state = space->machine->driver_data<eti660_state>();
	int colorram_offset = RX(space->machine) & 0xff;

	colorram_offset = ((colorram_offset & 0xf8) >> 1) || (colorram_offset & 0x03);

	state->color_ram[colorram_offset] = data;
}

/* Memory Maps */

static ADDRESS_MAP_START( eti660_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_ROM
	AM_RANGE(0x0400, 0x0fff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( eti660_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_DEVREADWRITE(CDP1864_TAG, cdp1864_dispon_r, cdp1864_step_bgcolor_w)
	AM_RANGE(0x02, 0x02) AM_DEVREADWRITE(MC6821_TAG, pia_r, pia_w)
	AM_RANGE(0x03, 0x03) AM_WRITE(eti660_colorram_w)
	AM_RANGE(0x04, 0x04) AM_DEVREADWRITE(CDP1864_TAG, cdp1864_dispoff_r, cdp1864_tone_latch_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_CHANGED( reset_pressed )
{
	if (oldval && !newval)
	{
		running_machine *machine = field->port->machine;
		MACHINE_RESET_CALL(eti660);
	}
}

static INPUT_PORTS_START( eti660 )
	PORT_START("PA0")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7')

	PORT_START("PA1")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6')

	PORT_START("PA2")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5')

	PORT_START("PA3")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4')

	PORT_START("SPECIAL")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("RESET") PORT_CODE(KEYCODE_R) PORT_CHANGED(reset_pressed, 0)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("STEP") PORT_CODE(KEYCODE_S)
INPUT_PORTS_END

/* Video */

static READ_LINE_DEVICE_HANDLER( rdata_r )
{
	eti660_state *state = device->machine->driver_data<eti660_state>();

	return BIT(state->color, 0);
}

static READ_LINE_DEVICE_HANDLER( bdata_r )
{
	eti660_state *state = device->machine->driver_data<eti660_state>();

	return BIT(state->color, 1);
}

static READ_LINE_DEVICE_HANDLER( gdata_r )
{
	eti660_state *state = device->machine->driver_data<eti660_state>();

	return BIT(state->color, 2);
}

static CDP1864_INTERFACE( eti660_cdp1864_intf )
{
	CDP1802_TAG,
	SCREEN_TAG,
	CDP1864_INTERLACED,
	DEVCB_LINE(rdata_r),
	DEVCB_LINE(bdata_r),
	DEVCB_LINE(gdata_r),
	DEVCB_CPU_INPUT_LINE(CDP1802_TAG, COSMAC_INPUT_LINE_INT),
	DEVCB_CPU_INPUT_LINE(CDP1802_TAG, COSMAC_INPUT_LINE_DMAOUT),
	DEVCB_CPU_INPUT_LINE(CDP1802_TAG, COSMAC_INPUT_LINE_EF1),
	RES_K(2.2), /* R7 */
	RES_K(1),	/* R5 */
	RES_K(4.7), /* R6 */
	RES_K(4.7)	/* R4 */
};

static VIDEO_UPDATE( eti660 )
{
	eti660_state *state = screen->machine->driver_data<eti660_state>();

	cdp1864_update(state->cdp1864, bitmap, cliprect);

	return 0;
}

/* CDP1802 Interface */

static READ_LINE_DEVICE_HANDLER( clear_r )
{
	return BIT(input_port_read(device->machine, "SPECIAL"), 0);
}

static READ_LINE_DEVICE_HANDLER( ef2_r )
{
	return cassette_input(device) < 0;
}

static READ_LINE_DEVICE_HANDLER( ef4_r )
{
	return BIT(input_port_read(device->machine, "SPECIAL"), 1);
}

static WRITE_LINE_DEVICE_HANDLER( eti660_q_w )
{
	eti660_state *driver_state = device->machine->driver_data<eti660_state>();

	/* CDP1864 audio output enable */
	cdp1864_aoe_w(driver_state->cdp1864, state);

	/* PULSE led */
	set_led_status(device->machine, LED_PULSE, state);

	/* tape output */
	cassette_output(driver_state->cassette, state ? 1.0 : -1.0);
}

static WRITE8_DEVICE_HANDLER( eti660_dma_w )
{
	eti660_state *state = device->machine->driver_data<eti660_state>();
	UINT8 colorram_offset = ((offset & 0xf8) >> 1) || (offset & 0x03);

	state->color = state->color_ram[colorram_offset];

	cdp1864_con_w(state->cdp1864, 0); // HACK
	cdp1864_dma_w(state->cdp1864, offset, data);
}

static COSMAC_INTERFACE( eti660_config )
{
	DEVCB_LINE_VCC,
	DEVCB_LINE(clear_r),
	DEVCB_NULL,
	DEVCB_DEVICE_LINE(CASSETTE_TAG, ef2_r),
	DEVCB_NULL,
	DEVCB_LINE(ef4_r),
	DEVCB_LINE(eti660_q_w),
	DEVCB_NULL,
	DEVCB_HANDLER(eti660_dma_w),
	NULL,
	DEVCB_NULL,
	DEVCB_NULL
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

	eti660_state *state = device->machine->driver_data<eti660_state>();
	UINT8 data = 0xf0;

	if (!BIT(state->keylatch, 0)) data &= input_port_read(device->machine, "PA0");
	if (!BIT(state->keylatch, 1)) data &= input_port_read(device->machine, "PA1");
	if (!BIT(state->keylatch, 2)) data &= input_port_read(device->machine, "PA2");
	if (!BIT(state->keylatch, 3)) data &= input_port_read(device->machine, "PA3");

	return data;
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

	eti660_state *state = device->machine->driver_data<eti660_state>();

	state->keylatch = data & 0x0f;
}

static const pia6821_interface eti660_mc6821_intf =
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
	DEVCB_NULL,													/* IRQA output */
	DEVCB_NULL													/* IRQB output */
};

/* Machine Initialization */

static MACHINE_START( eti660 )
{
	eti660_state *state = machine->driver_data<eti660_state>();

	/* find devices */
	state->cdp1864 = machine->device(CDP1864_TAG);
	state->cassette = machine->device(CASSETTE_TAG);
}

static MACHINE_RESET( eti660 )
{
	eti660_state *state = machine->driver_data<eti660_state>();

	/* reset CDP1864 */
	state->cdp1864->reset();
}

/* Machine Drivers */

static const cassette_config eti660_cassette_config =
{
	cassette_default_formats,
	NULL,
	(cassette_state)(CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_MUTED),
	NULL
};

static MACHINE_CONFIG_START( eti660, eti660_state )
	/* basic machine hardware */
	MDRV_CPU_ADD(CDP1802_TAG, COSMAC, XTAL_8_867238MHz/5)
	MDRV_CPU_PROGRAM_MAP(eti660_map)
	MDRV_CPU_IO_MAP(eti660_io_map)
	MDRV_CPU_CONFIG(eti660_config)

	MDRV_MACHINE_START(eti660)
	MDRV_MACHINE_RESET(eti660)

    /* video hardware */
	MDRV_CDP1864_SCREEN_ADD(SCREEN_TAG, XTAL_8_867238MHz/5)

	MDRV_PALETTE_LENGTH(8+8)
	MDRV_VIDEO_UPDATE(eti660)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_CDP1864_ADD(CDP1864_TAG, XTAL_8_867238MHz/5, eti660_cdp1864_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* devices */
	MDRV_PIA6821_ADD(MC6821_TAG, eti660_mc6821_intf)
	MDRV_CASSETTE_ADD(CASSETTE_TAG, eti660_cassette_config)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("3K")
MACHINE_CONFIG_END

/* ROMs */

ROM_START( eti660 )
	ROM_REGION( 0x10000, CDP1802_TAG, 0 )
	ROM_LOAD( "eti660.bin", 0x0000, 0x0400, CRC(811dfa62) SHA1(c0c4951e02f873f15560bdc3f35cdf3f99653922) )
ROM_END

/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT       INIT        COMPANY                             FULLNAME                FLAGS */
COMP( 1981, eti660,		0,		0,		eti660,		eti660,		0,			"Electronics Today International",	"ETI-660 (Australia)",	GAME_NOT_WORKING )
