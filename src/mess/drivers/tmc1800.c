/*

Telmac 2000

PCB Layout
----------

|-----------------------------------------------------------|
|                                                           |
|    |                  4051    4042        2114    2114    |
|   CN1                                                     |
|    |                  4051    4042        2114    2114    |
|                                                           |
|   4011                4013    |--------|  2114    2114    |
|                               |  4515  |                  |
|                       4013    |--------|  2114    2114    |
|                               4042                        |
|                       4011                2114    2114    |
|                               4011                        |
|                       4011                2114    2114    |
|SW1                                                        |
|                       4011    |--------|  2114    2114    |
|                               |  PROM  |                  |
|                       40107   |--------|  2114    2114    |
|   4050    1.75MHz                                         |
|                       |-------------|     2114    2114    |
|                       |   CDP1802   |                     |
|                       |-------------|     2114    2114    |
|           4049                                            |
|                       |-------------|     2114    2114    |
|       4.43MHz         |   CDP1864   |                     |
|                       |-------------|     2114    2114    |
|                                                           |
|               741             4502        2114    2114    |
|  -                                                        |
|  |                            2114        2114    2114    |
| CN2                                                       |
|  |            4001            4051        2114    2114    |
|  -                                                        |
|                                           2114    2114    |
|-----------------------------------------------------------|

Notes:
    All IC's shown.

    PROM    - MMI6341
    2114    - 2114 4096 Bit (1024x4) NMOS Static RAM
    CDP1802 - RCA CDP1802 CMOS 8-Bit Microprocessor @ 1.75 MHz
    CDP1864 - RCA CDP1864CE COS/MOS PAL Compatible Color TV Interface @ 1.75 MHz
    CN1     - keyboard connector
    CN2     - ASTEC RF modulator connector
    SW1     - Run/Reset switch

*/

/*

OSCOM Nano

PCB Layout
----------

OK 30379

|-------------------------------------------------|
|   CN1     CN2     CN3                 7805      |
|                       1.75MHz                   |
|                               741               |
|                       |-------------|         - |
|   741     741   4011  |   CDP1864   |         | |
|                       |-------------|         | |
|                 4013  |-------------|         | |
|                       |   CDP1802   |         | |
|                 4093  |-------------|         | |
|                                               C |
|            4051   4042    4017        4042    N |
|                           |-------|           4 |
|                           |  ROM  |   14556   | |
|                           |-------|           | |
|                           2114    2114        | |
|                                               | |
|                           2114    2114        | |
|                                               - |
|                           2114    2114          |
|                                                 |
|                           2114    2114          |
|-------------------------------------------------|

Notes:
    All IC's shown.

    ROM     - Intersil 5504?
    2114    - 2114UCB 4096 Bit (1024x4) NMOS Static RAM
    CDP1802 - RCA CDP1802E CMOS 8-Bit Microprocessor @ 1.75 MHz
    CDP1864 - RCA CDP1864CE COS/MOS PAL Compatible Color TV Interface @ 1.75 MHz
    CN1     - tape connector
    CN2     - video connector
    CN3     - power connector
    CN4     - expansion connector

*/

/*

    TODO:

    - tape input/output
    - tmc2000: add missing keys
    - tmc2000: TOOL-2000 rom banking
    - nano: correct time constant for EF4 RC circuit

*/

#include "emu.h"
#include "includes/tmc1800.h"
#include "cpu/cosmac/cosmac.h"
#include "video/cdp1861.h"
#include "sound/cdp1864.h"
#include "devices/cassette.h"
#include "devices/snapquik.h"
#include "machine/rescap.h"
#include "sound/beep.h"
#include "devices/messram.h"

#define TMC2000_BANK_RAM		0
#define TMC2000_BANK_ROM		1
#define TMC2000_BANK_MONITOR	0
#define TMC2000_BANK_COLORRAM	1
#define TMC2000_COLORRAM_SIZE	0x200

#define OSCNANO_BANK_RAM		0
#define OSCNANO_BANK_ROM		1

static QUICKLOAD_LOAD( tmc1800 );

static MACHINE_RESET( tmc1800 );
static MACHINE_RESET( tmc2000 );
static MACHINE_RESET( nano );

/* Read/Write Handlers */

static WRITE8_HANDLER( tmc1800_keylatch_w )
{
	tmc1800_state *state = space->machine->driver_data<tmc1800_state>();

	state->keylatch = data;
}

static WRITE8_HANDLER( tmc2000_keylatch_w )
{
	/*

        bit     description

        0       X0
        1       X1
        2       X2
        3       Y0
        4       Y1
        5       Y2
        6       EXP1
        7       EXP2

    */

	tmc2000_state *state = space->machine->driver_data<tmc2000_state>();

	state->keylatch = data & 0x3f;
}

static WRITE8_HANDLER( nano_keylatch_w )
{
	/*

        bit     description

        0       X0
        1       X1
        2       X2
        3       Y0
        4       not connected
        5       not connected
        6       not connected
        7       not connected

    */

	nano_state *state = space->machine->driver_data<nano_state>();

	state->keylatch = data & 0x0f;
}

static WRITE8_DEVICE_HANDLER( tmc2000_bankswitch_w )
{
	int bank = data & 0x01;

	/* enable RAM */

	memory_set_bank(device->machine, "bank1", TMC2000_BANK_RAM);

	switch (messram_get_size(device->machine->device("messram")))
	{
	case 4 * 1024:
		memory_install_readwrite_bank(cputag_get_address_space(device->machine, CDP1802_TAG, ADDRESS_SPACE_PROGRAM), 0x0000, 0x0fff, 0, 0x7000, "bank1");
		break;

	case 16 * 1024:
		memory_install_readwrite_bank(cputag_get_address_space(device->machine, CDP1802_TAG, ADDRESS_SPACE_PROGRAM), 0x0000, 0x3fff, 0, 0x4000, "bank1");
		break;

	case 32 * 1024:
		memory_install_readwrite_bank(cputag_get_address_space(device->machine, CDP1802_TAG, ADDRESS_SPACE_PROGRAM), 0x0000, 0x7fff, 0, 0, "bank1");
		break;
	}

	/* enable monitor or color RAM */

	memory_set_bank(device->machine, "bank2", bank);

	switch (bank)
	{
	case TMC2000_BANK_MONITOR:
		memory_install_read_bank(cputag_get_address_space(device->machine, CDP1802_TAG, ADDRESS_SPACE_PROGRAM), 0x8000, 0x81ff, 0, 0x7e00, "bank2");
		memory_unmap_write(cputag_get_address_space(device->machine, CDP1802_TAG, ADDRESS_SPACE_PROGRAM), 0x8000, 0x81ff, 0, 0x7e00 );
		break;

	case TMC2000_BANK_COLORRAM: // write-only
		memory_install_write_bank(cputag_get_address_space(device->machine, CDP1802_TAG, ADDRESS_SPACE_PROGRAM), 0x8000, 0x81ff, 0, 0x7e00, "bank2");
		memory_unmap_read(cputag_get_address_space(device->machine, CDP1802_TAG, ADDRESS_SPACE_PROGRAM), 0x8000, 0x81ff, 0, 0x7e00);
		break;
	}

	/* write to CDP1864 tone latch */

	cdp1864_tone_latch_w(device, 0, data);
}

static WRITE8_DEVICE_HANDLER( nano_bankswitch_w )
{
	/* enable RAM */

	memory_set_bank(device->machine, "bank1", OSCNANO_BANK_RAM);

	memory_install_readwrite_bank(cputag_get_address_space(device->machine, CDP1802_TAG, ADDRESS_SPACE_PROGRAM), 0x0000, 0x0fff, 0, 0x7000, "bank1");

	/* write to CDP1864 tone latch */

	cdp1864_tone_latch_w(device, 0, data);
}

static READ8_DEVICE_HANDLER( tmc1800_cdp1861_dispon_r )
{
	cdp1861_dispon_w(device, 1);
	cdp1861_dispon_w(device, 0);

	return 0xff;
}

static WRITE8_DEVICE_HANDLER( tmc1800_cdp1861_dispoff_w )
{
	cdp1861_dispoff_w(device, 1);
	cdp1861_dispoff_w(device, 0);
}

/* Memory Maps */

// Telmac 1800

static ADDRESS_MAP_START( tmc1800_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM
	AM_RANGE(0x8000, 0x81ff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( tmc1800_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_DEVREADWRITE(CDP1861_TAG, tmc1800_cdp1861_dispon_r, tmc1800_cdp1861_dispoff_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(tmc1800_keylatch_w)
ADDRESS_MAP_END

// OSCOM 1000B

static ADDRESS_MAP_START( osc1000b_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM
	AM_RANGE(0x8000, 0x81ff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( osc1000b_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x02, 0x02) AM_WRITE(tmc1800_keylatch_w)
ADDRESS_MAP_END

// Telmac 2000

static ADDRESS_MAP_START( tmc2000_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x7fff) AM_RAMBANK("bank1")
	AM_RANGE(0x8000, 0xffff) AM_RAMBANK("bank2")
ADDRESS_MAP_END

static ADDRESS_MAP_START( tmc2000_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x01, 0x01) AM_DEVREADWRITE(CDP1864_TAG, cdp1864_dispon_r, cdp1864_step_bgcolor_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(tmc2000_keylatch_w)
	AM_RANGE(0x04, 0x04) AM_DEVREADWRITE(CDP1864_TAG, cdp1864_dispoff_r, tmc2000_bankswitch_w)
ADDRESS_MAP_END

// OSCOM Nano

static ADDRESS_MAP_START( nano_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_RAMBANK("bank1")
	AM_RANGE(0x8000, 0x83ff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( nano_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_DEVREADWRITE(CDP1864_TAG, cdp1864_dispon_r, cdp1864_step_bgcolor_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(nano_keylatch_w)
	AM_RANGE(0x04, 0x04) AM_DEVREADWRITE(CDP1864_TAG, cdp1864_dispoff_r, nano_bankswitch_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( tmc1800 )
	PORT_START("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_0) PORT_CODE(KEYCODE_0_PAD) PORT_CHAR('0')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_1) PORT_CODE(KEYCODE_1_PAD) PORT_CHAR('1')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_2) PORT_CODE(KEYCODE_2_PAD) PORT_CHAR('2')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_3) PORT_CODE(KEYCODE_3_PAD) PORT_CHAR('3')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_4) PORT_CODE(KEYCODE_4_PAD) PORT_CHAR('4')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_5) PORT_CODE(KEYCODE_5_PAD) PORT_CHAR('5')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_6) PORT_CODE(KEYCODE_6_PAD) PORT_CHAR('6')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_7) PORT_CODE(KEYCODE_7_PAD) PORT_CHAR('7')

	PORT_START("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_8) PORT_CODE(KEYCODE_8_PAD) PORT_CHAR('8')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_9) PORT_CODE(KEYCODE_9_PAD) PORT_CHAR('9')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')

	PORT_START("RUN")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("Run/Reset") PORT_CODE(KEYCODE_R) PORT_TOGGLE
INPUT_PORTS_END

static INPUT_PORTS_START( tmc2000 )
	PORT_INCLUDE(tmc1800)

	PORT_START("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('N')

	PORT_START("IN3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('V')

	PORT_START("IN4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START("IN5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START("IN6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START("IN7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )
INPUT_PORTS_END

static INPUT_CHANGED( run_pressed )
{
	if (oldval && !newval)
	{
		running_machine *machine = field->port->machine;
		MACHINE_RESET_CALL(nano);
	}
}

static INPUT_CHANGED( monitor_pressed )
{
	running_machine *machine = field->port->machine;
	nano_state *state = machine->driver_data<nano_state>();

	if (oldval && !newval)
	{
		MACHINE_RESET_CALL(nano);

		cputag_set_input_line(machine, CDP1802_TAG, COSMAC_INPUT_LINE_EF4, CLEAR_LINE);
	}
	else if (!oldval && newval)
	{
		// TODO: what are the correct values?
		int t = RES_K(27) * CAP_U(1) * 1000; // t = R26 * C1
		timer_adjust_oneshot(state->ef4_timer, ATTOTIME_IN_MSEC(t), 0);
	}
}

static INPUT_PORTS_START( nano )
	PORT_START("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_0) PORT_CODE(KEYCODE_0_PAD) PORT_CHAR('0')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_1) PORT_CODE(KEYCODE_1_PAD) PORT_CHAR('1')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_2) PORT_CODE(KEYCODE_2_PAD) PORT_CHAR('2')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_3) PORT_CODE(KEYCODE_3_PAD) PORT_CHAR('3')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_4) PORT_CODE(KEYCODE_4_PAD) PORT_CHAR('4')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_5) PORT_CODE(KEYCODE_5_PAD) PORT_CHAR('5')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_6) PORT_CODE(KEYCODE_6_PAD) PORT_CHAR('6')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_7) PORT_CODE(KEYCODE_7_PAD) PORT_CHAR('7')

	PORT_START("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_8) PORT_CODE(KEYCODE_8_PAD) PORT_CHAR('8')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_9) PORT_CODE(KEYCODE_9_PAD) PORT_CHAR('9')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')

	PORT_START("RUN")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("RUN") PORT_CODE(KEYCODE_R) PORT_CHANGED(run_pressed, 0)

	PORT_START("MONITOR")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("MONITOR") PORT_CODE(KEYCODE_M) PORT_CHANGED(monitor_pressed, 0)
INPUT_PORTS_END

/* CDP1802 Interfaces */

// Telmac 1800

static READ_LINE_DEVICE_HANDLER( clear_r )
{
	return BIT(input_port_read(device->machine, "RUN"), 0);
}

static READ_LINE_DEVICE_HANDLER( ef2_r )
{
	return cassette_input(device) < 0;
}

static READ_LINE_DEVICE_HANDLER( ef3_r )
{
	tmc1800_state *state = device->machine->driver_data<tmc1800_state>();
	static const char *const keynames[] = { "IN0", "IN1", "IN2", "IN3", "IN4", "IN5", "IN6", "IN7" };
	UINT8 data = ~input_port_read(device->machine, keynames[state->keylatch / 8]);

	return BIT(data, state->keylatch % 8);
}

static WRITE_LINE_DEVICE_HANDLER( tmc1800_q_w )
{
	tmc1800_state *driver_state = device->machine->driver_data<tmc1800_state>();

	/* tape output */
	cassette_output(driver_state->cassette, state ? 1.0 : -1.0);
}

static COSMAC_INTERFACE( tmc1800_config )
{
	DEVCB_LINE_VCC,
	DEVCB_LINE(clear_r),
	DEVCB_NULL,
	DEVCB_DEVICE_LINE(CASSETTE_TAG, ef2_r),
	DEVCB_LINE(ef3_r),
	DEVCB_NULL,
	DEVCB_LINE(tmc1800_q_w),
	DEVCB_NULL,
	DEVCB_DEVICE_HANDLER(CDP1861_TAG, cdp1861_dma_w),
	NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

static COSMAC_INTERFACE( osc1000b_config )
{
	DEVCB_LINE_VCC,
	DEVCB_LINE(clear_r),
	DEVCB_NULL,
	DEVCB_DEVICE_LINE(CASSETTE_TAG, ef2_r),
	DEVCB_LINE(ef3_r),
	DEVCB_NULL,
	DEVCB_LINE(tmc1800_q_w),
	DEVCB_NULL,
	DEVCB_NULL,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

// Telmac 2000

static WRITE_LINE_DEVICE_HANDLER( tmc2000_q_w )
{
	tmc2000_state *driver_state = device->machine->driver_data<tmc2000_state>();

	/* CDP1864 audio output enable */
	cdp1864_aoe_w(driver_state->cdp1864, state);

	/* set Q led status */
	set_led_status(device->machine, 1, state);

	/* tape output */
	cassette_output(driver_state->cassette, state ? 1.0 : -1.0);
}

static WRITE8_DEVICE_HANDLER( tmc2000_dma_w )
{
	tmc2000_state *state = device->machine->driver_data<tmc2000_state>();

	state->color = ~(state->colorram[offset & 0x1ff]) & 0x07;

	cdp1864_con_w(state->cdp1864, 0); // HACK
	cdp1864_dma_w(state->cdp1864, offset, data);
}

static COSMAC_INTERFACE( tmc2000_config )
{
	DEVCB_LINE_VCC,
	DEVCB_LINE(clear_r),
	DEVCB_NULL,
	DEVCB_DEVICE_LINE(CASSETTE_TAG, ef2_r),
	DEVCB_LINE(ef3_r),
	DEVCB_NULL,
	DEVCB_LINE(tmc2000_q_w),
	DEVCB_NULL,
	DEVCB_HANDLER(tmc2000_dma_w),
	NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

// OSCOM Nano

static TIMER_CALLBACK( nano_ef4_tick )
{
	cputag_set_input_line(machine, CDP1802_TAG, COSMAC_INPUT_LINE_EF4, ASSERT_LINE);
}

static READ_LINE_DEVICE_HANDLER( nano_clear_r )
{
	int run = BIT(input_port_read(device->machine, "RUN"), 0);
	int monitor = BIT(input_port_read(device->machine, "MONITOR"), 0);

	return run & monitor;
}

static READ_LINE_DEVICE_HANDLER( nano_ef2_r )
{
	return cassette_input(device) < 0;
}

static READ_LINE_DEVICE_HANDLER( nano_ef3_r )
{
	nano_state *state = device->machine->driver_data<nano_state>();
	static const char *const keynames[] = { "IN0", "IN1", "IN2", "IN3", "IN4", "IN5", "IN6", "IN7" };
	UINT8 data = ~input_port_read(device->machine, keynames[state->keylatch / 8]);

	return BIT(data, state->keylatch % 8);
}

static WRITE_LINE_DEVICE_HANDLER( nano_q_w )
{
	nano_state *driver_state = device->machine->driver_data<nano_state>();

	/* CDP1864 audio output enable */
	cdp1864_aoe_w(driver_state->cdp1864, state);

	/* set Q led status */
	set_led_status(device->machine, 1, state);

	/* tape output */
	cassette_output(driver_state->cassette, state ? 1.0 : -1.0);
}

static COSMAC_INTERFACE( nano_config )
{
	DEVCB_LINE_VCC,
	DEVCB_LINE(nano_clear_r),
	DEVCB_NULL,
	DEVCB_DEVICE_LINE(CASSETTE_TAG, nano_ef2_r),
	DEVCB_LINE(nano_ef3_r),
	DEVCB_NULL,
	DEVCB_LINE(nano_q_w),
	DEVCB_NULL,
	DEVCB_DEVICE_HANDLER(CDP1864_TAG, cdp1864_dma_w),
	NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

/* Machine Initialization */

// Telmac 1800

static MACHINE_START( tmc1800 )
{
	tmc1800_state *state = machine->driver_data<tmc1800_state>();

	/* find devices */
	state->cdp1861 = machine->device(CDP1861_TAG);
	state->cassette = machine->device(CASSETTE_TAG);

	/* register for state saving */
	state_save_register_global(machine, state->keylatch);
}

static MACHINE_RESET( tmc1800 )
{
	tmc1800_state *state = machine->driver_data<tmc1800_state>();

	/* reset CDP1861 */
	state->cdp1861->reset();
}

// OSCOM 1000B

static MACHINE_START( osc1000b )
{
	osc1000b_state *state = machine->driver_data<osc1000b_state>();

	/* find devices */
	state->cassette = machine->device(CASSETTE_TAG);

	/* register for state saving */
	state_save_register_global(machine, state->keylatch);
}

static MACHINE_RESET( osc1000b )
{
}

// Telmac 2000

static MACHINE_START( tmc2000 )
{
	tmc2000_state *state = machine->driver_data<tmc2000_state>();

	UINT16 addr;

	/* RAM banking */
	memory_configure_bank(machine, "bank1", 0, 2, machine->region(CDP1802_TAG)->base(), 0x8000);

	/* ROM/colorram banking */
	state->colorram = auto_alloc_array(machine, UINT8, TMC2000_COLORRAM_SIZE);

	memory_configure_bank(machine, "bank2", TMC2000_BANK_MONITOR, 1, machine->region(CDP1802_TAG)->base() + 0x8000, 0);
	memory_configure_bank(machine, "bank2", TMC2000_BANK_COLORRAM, 1, state->colorram, 0);

	/* randomize color RAM contents */
	for (addr = 0; addr < TMC2000_COLORRAM_SIZE; addr++)
	{
		state->colorram[addr] = machine->rand() & 0xff;
	}

	/* find devices */
	state->cdp1864 = machine->device(CDP1864_TAG);
	state->cassette = machine->device(CASSETTE_TAG);

	/* register for state saving */
	state_save_register_global_pointer(machine, state->colorram, TMC2000_COLORRAM_SIZE);
	state_save_register_global(machine, state->keylatch);
}

static MACHINE_RESET( tmc2000 )
{
	tmc2000_state *state = machine->driver_data<tmc2000_state>();

	/* reset CDP1864 */
	state->cdp1864->reset();

	/* enable monitor mirror at 0x0000 */
	memory_set_bank(machine, "bank1", TMC2000_BANK_ROM);
	memory_install_readwrite_bank(cputag_get_address_space(machine, CDP1802_TAG, ADDRESS_SPACE_PROGRAM), 0x0000, 0x01ff, 0, 0x7e00, "bank1");
	memory_unmap_write(cputag_get_address_space(machine, CDP1802_TAG, ADDRESS_SPACE_PROGRAM), 0x0000, 0x01ff, 0, 0x7e00);

	/* enable monitor */
	memory_set_bank(machine, "bank2", TMC2000_BANK_MONITOR);
	memory_install_readwrite_bank(cputag_get_address_space(machine, CDP1802_TAG, ADDRESS_SPACE_PROGRAM), 0x8000, 0x81ff, 0, 0x7e00, "bank2");
	memory_unmap_write(cputag_get_address_space(machine, CDP1802_TAG, ADDRESS_SPACE_PROGRAM), 0x8000, 0x81ff, 0, 0x7e00);
}

// OSCOM Nano

static MACHINE_START( nano )
{
	nano_state *state = machine->driver_data<nano_state>();

	/* RAM/ROM banking */
	memory_configure_bank(machine, "bank1", 0, 2, machine->region(CDP1802_TAG)->base(), 0x8000);

	/* allocate monitor timer */
	state->ef4_timer = timer_alloc(machine, nano_ef4_tick, NULL);

	/* find devices */
	state->cdp1864 = machine->device(CDP1864_TAG);
	state->cassette = machine->device(CASSETTE_TAG);

	/* register for state saving */
	state_save_register_global(machine, state->keylatch);
}

static MACHINE_RESET( nano )
{
	nano_state *state = machine->driver_data<nano_state>();

	/* assert EF4 */
	cputag_set_input_line(machine, CDP1802_TAG, COSMAC_INPUT_LINE_EF4, ASSERT_LINE);

	/* reset CDP1864 */
	state->cdp1864->reset();

	/* enable ROM */
	memory_set_bank(machine, "bank1", OSCNANO_BANK_ROM);
	memory_install_readwrite_bank(cputag_get_address_space(machine, CDP1802_TAG, ADDRESS_SPACE_PROGRAM), 0x0000, 0x01ff, 0, 0x7e00, "bank1");
}

/* Machine Drivers */

static const cassette_config tmc1800_cassette_config =
{
	cassette_default_formats,
	NULL,
	(cassette_state)(CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_MUTED),
	NULL
};


static MACHINE_CONFIG_START( tmc1800, tmc1800_state )
	// basic system hardware
	MCFG_CPU_ADD(CDP1802_TAG, COSMAC, XTAL_1_75MHz)
	MCFG_CPU_PROGRAM_MAP(tmc1800_map)
	MCFG_CPU_IO_MAP(tmc1800_io_map)
	MCFG_CPU_CONFIG(tmc1800_config)

	MCFG_MACHINE_START(tmc1800)
	MCFG_MACHINE_RESET(tmc1800)

	// video hardware
	MCFG_FRAGMENT_ADD(tmc1800_video)

	// sound hardware
	MCFG_SPEAKER_STANDARD_MONO("mono")

	MCFG_SOUND_ADD("beep", BEEP, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	// devices
	MCFG_QUICKLOAD_ADD("quickload", tmc1800, "bin", 0)
	MCFG_CASSETTE_ADD( "cassette", tmc1800_cassette_config )

	// internal ram
	MCFG_RAM_ADD("messram")
	MCFG_RAM_DEFAULT_SIZE("2K")
	MCFG_RAM_EXTRA_OPTIONS("4K")
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( osc1000b, osc1000b_state )
	// basic system hardware
	MCFG_CPU_ADD(CDP1802_TAG, COSMAC, XTAL_1_75MHz)
	MCFG_CPU_PROGRAM_MAP(osc1000b_map)
	MCFG_CPU_IO_MAP(osc1000b_io_map)
	MCFG_CPU_CONFIG(osc1000b_config)

	MCFG_MACHINE_START(osc1000b)
	MCFG_MACHINE_RESET(osc1000b)

	// video hardware
	MCFG_FRAGMENT_ADD(osc1000b_video)

	// sound hardware
	MCFG_SPEAKER_STANDARD_MONO("mono")

	MCFG_SOUND_ADD("beep", BEEP, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	// devices
	MCFG_QUICKLOAD_ADD("quickload", tmc1800, "bin", 0)
	MCFG_CASSETTE_ADD( "cassette", tmc1800_cassette_config )

	/* internal ram */
	MCFG_RAM_ADD("messram")
	MCFG_RAM_DEFAULT_SIZE("2K")
	MCFG_RAM_EXTRA_OPTIONS("4K")
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( tmc2000, tmc2000_state )
	// basic system hardware
	MCFG_CPU_ADD(CDP1802_TAG, COSMAC, XTAL_1_75MHz)
	MCFG_CPU_PROGRAM_MAP(tmc2000_map)
	MCFG_CPU_IO_MAP(tmc2000_io_map)
	MCFG_CPU_CONFIG(tmc2000_config)

	MCFG_MACHINE_START(tmc2000)
	MCFG_MACHINE_RESET(tmc2000)

	// video hardware
	MCFG_FRAGMENT_ADD(tmc2000_video)

	// devices
	MCFG_QUICKLOAD_ADD("quickload", tmc1800, "bin", 0)
	MCFG_CASSETTE_ADD( "cassette", tmc1800_cassette_config )

	/* internal ram */
	MCFG_RAM_ADD("messram")
	MCFG_RAM_DEFAULT_SIZE("4K")
	MCFG_RAM_EXTRA_OPTIONS("16K,32K")
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( nano, nano_state )
	// basic system hardware
	MCFG_CPU_ADD(CDP1802_TAG, COSMAC, XTAL_1_75MHz)
	MCFG_CPU_PROGRAM_MAP(nano_map)
	MCFG_CPU_IO_MAP(nano_io_map)
	MCFG_CPU_CONFIG(nano_config)

	MCFG_MACHINE_START(nano)
	MCFG_MACHINE_RESET(nano)

	// video hardware
	MCFG_FRAGMENT_ADD(nano_video)

	// devices
	MCFG_QUICKLOAD_ADD("quickload", tmc1800, "bin", 0)
	MCFG_CASSETTE_ADD( "cassette", tmc1800_cassette_config )

	/* internal ram */
	MCFG_RAM_ADD("messram")
	MCFG_RAM_DEFAULT_SIZE("4K")
MACHINE_CONFIG_END

/* ROMs */

ROM_START( tmc1800 )
	ROM_REGION( 0x10000, CDP1802_TAG, 0 )
	ROM_LOAD( "mmi6341-1.ic2", 0x8000, 0x0200, NO_DUMP ) // equivalent to 82S141
ROM_END

ROM_START( osc1000b )
	ROM_REGION( 0x10000, CDP1802_TAG, 0 )
	ROM_LOAD( "mmi6341-1.ic2", 0x8000, 0x0200, NO_DUMP ) // equivalent to 82S141

	ROM_REGION( 0x400, "gfx1", 0 )
	ROM_LOAD( "mmi6349.5d",	0x0000, 0x0200, NO_DUMP ) // equivalent to 82S147
	ROM_LOAD( "mmi6349.5c",	0x0200, 0x0200, NO_DUMP ) // equivalent to 82S147
ROM_END

ROM_START( tmc2000 )
	ROM_REGION( 0x10000, CDP1802_TAG, 0 )
	ROM_SYSTEM_BIOS( 0, "prom200", "PROM N:o 200" )
	ROMX_LOAD( "200.m5",    0x8000, 0x0200, BAD_DUMP CRC(79da3221) SHA1(008da3ef4f69ab1a493362dfca856375b19c94bd), ROM_BIOS(1) ) // typed in from the manual
	ROM_SYSTEM_BIOS( 1, "prom202", "PROM N:o 202" )
	ROMX_LOAD( "202.m5",    0x8000, 0x0200, NO_DUMP, ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "tool2000", "TOOL-2000" )
	ROMX_LOAD( "tool2000",	0x8000, 0x0800, NO_DUMP, ROM_BIOS(3) )
ROM_END

ROM_START( nano )
	ROM_REGION( 0x10000, CDP1802_TAG, 0 )
	ROM_LOAD( "mmi6349.ic", 0x8000, 0x0200, BAD_DUMP CRC(1ec1b432) SHA1(ac41f5e38bcd4b80bd7a5b277a2c600899fd5fb8) ) // equivalent to 82S141
ROM_END

/* System Configuration */

static QUICKLOAD_LOAD( tmc1800 )
{
	UINT8 *ptr = image.device().machine->region(CDP1802_TAG)->base();
	int size = image.length();

	if (size > messram_get_size(image.device().machine->device("messram")))
	{
		return IMAGE_INIT_FAIL;
	}

	image.fread( ptr, size);

	return IMAGE_INIT_PASS;
}

/* Driver Initialization */

static TIMER_CALLBACK(setup_beep)
{
	device_t *speaker = machine->device("beep");
	beep_set_state(speaker, 0);
	beep_set_frequency( speaker, 0 );
}

static DRIVER_INIT( tmc1800 )
{
	timer_set(machine, attotime_zero, NULL, 0, setup_beep);
}

/* System Drivers */

/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT       INIT        COMPANY         FULLNAME        FLAGS */
COMP( 1977, tmc1800,    0,      0,      tmc1800,    tmc1800,    tmc1800,	"Telercas Oy",	"Telmac 1800",  GAME_NOT_WORKING )
COMP( 1977, osc1000b,   tmc1800,0,      osc1000b,   tmc1800,    tmc1800,    "OSCOM Oy",		"OSCOM 1000B",  GAME_NOT_WORKING )
COMP( 1980, tmc2000,    0,      0,      tmc2000,    tmc2000,    0,		    "Telercas Oy",  "Telmac 2000",  GAME_SUPPORTS_SAVE )
COMP( 1980, nano,		tmc2000,0,		nano,		nano,	0,			"OSCOM Oy",		"OSCOM Nano",	GAME_SUPPORTS_SAVE )
