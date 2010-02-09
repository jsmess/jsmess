/***************************************************************************

    VCS-80

    12/05/2009 Skeleton driver.

    http://hc-ddr.hucki.net/entwicklungssysteme.htm#VCS_80_von_Eckhard_Schiller

****************************************************************************/

#include "emu.h"
#include "includes/vcs80.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "machine/z80pio.h"
#include "devices/messram.h"
#include "vcs80.lh"

/* Read/Write Handlers */

static READ8_DEVICE_HANDLER( vcs80_z80pio_r )
{
	switch (offset)
	{
	case 0: return z80pio_c_r(device, 1);
	case 1: return z80pio_c_r(device, 0);
	case 2: return z80pio_d_r(device, 1);
	case 3: return z80pio_d_r(device, 0);
	}

	return 0;
}

static WRITE8_DEVICE_HANDLER( vcs80_z80pio_w )
{
	switch (offset)
	{
	case 0: z80pio_c_w(device, 1, data); break;
	case 1: z80pio_c_w(device, 0, data); break;
	case 2: z80pio_d_w(device, 1, data); break;
	case 3: z80pio_d_w(device, 0, data); break;
	}
}

/* Memory Maps */

static ADDRESS_MAP_START( vcs80_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x01ff) AM_ROM
	AM_RANGE(0x0400, 0x07ff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( vcs80_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x04, 0x07) AM_DEVREADWRITE(Z80PIO_TAG, vcs80_z80pio_r, vcs80_z80pio_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( vcs80 )
	PORT_START("ROW0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7')

	PORT_START("ROW1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')

	PORT_START("ROW2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("A+") PORT_CODE(KEYCODE_UP)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("A-") PORT_CODE(KEYCODE_DOWN)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("MA") PORT_CODE(KEYCODE_M)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RE") PORT_CODE(KEYCODE_R)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("GO") PORT_CODE(KEYCODE_G)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("TR") PORT_CODE(KEYCODE_T)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("ST") PORT_CODE(KEYCODE_S)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PE") PORT_CODE(KEYCODE_P)
INPUT_PORTS_END

/* Z80-PIO Interface */

static TIMER_DEVICE_CALLBACK( vcs80_keyboard_tick )
{
	/*

        bit     description

        PA0     keyboard and led latch bit 0
        PA1     keyboard and led latch bit 1
        PA2     keyboard and led latch bit 2
        PA3     GND
        PA4     keyboard row input 0
        PA5     keyboard row input 1
        PA6     keyboard row input 2
        PA7     demultiplexer clock input

    */

	vcs80_state *state = (vcs80_state *)timer->machine->driver_data;

	UINT8 data = 0;

	if (state->keyclk)
	{
		state->keylatch++;
		state->keylatch &= 0x07;
	}

	data = (state->keyclk << 7) | 0x70 | state->keylatch;

	if (!BIT(input_port_read(timer->machine, "ROW0"), state->keylatch)) data &= ~0x10;
	if (!BIT(input_port_read(timer->machine, "ROW1"), state->keylatch)) data &= ~0x20;
	if (!BIT(input_port_read(timer->machine, "ROW2"), state->keylatch)) data &= ~0x40;

	z80pio_p_w(state->z80pio, 0, data);

	state->keyclk = !state->keyclk;
}

static WRITE8_DEVICE_HANDLER( pio_port_b_w )
{
	/*

        bit     description

        PB0     VQD30 segment A
        PB1     VQD30 segment B
        PB2     VQD30 segment C
        PB3     VQD30 segment D
        PB4     VQD30 segment E
        PB5     VQD30 segment G
        PB6     VQD30 segment F
        PB7     _A0

    */

	vcs80_state *state = (vcs80_state *)device->machine->driver_data;

	UINT8 led_data = BITSWAP8(data & 0x7f, 7, 5, 6, 4, 3, 2, 1, 0);
	int digit = state->keylatch;

	/* skip middle digit */
	if (digit > 3) digit++;

	output_set_digit_value(8 - digit, led_data);
}

static const z80pio_interface pio_intf =
{
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),	/* callback when change interrupt status */
	DEVCB_NULL,						/* port A read callback */
	DEVCB_NULL,						/* port B read callback */
	DEVCB_NULL,						/* port A write callback */
	DEVCB_HANDLER(pio_port_b_w),	/* port B write callback */
	DEVCB_NULL,						/* portA ready active callback */
	DEVCB_NULL						/* portB ready active callback */
};

/* Z80 Daisy Chain */

static const z80_daisy_chain vcs80_daisy_chain[] =
{
	{ Z80PIO_TAG },
	{ NULL }
};

/* Machine Initialization */

static MACHINE_START(vcs80)
{
	vcs80_state *state = (vcs80_state *)machine->driver_data;

	/* find devices */
	state->z80pio = devtag_get_device(machine, Z80PIO_TAG);

	/* register for state saving */
	state_save_register_global(machine, state->keylatch);
	state_save_register_global(machine, state->keyclk);
}

/* Machine Driver */

static MACHINE_DRIVER_START( vcs80 )
	MDRV_DRIVER_DATA(vcs80_state)

	/* basic machine hardware */
    MDRV_CPU_ADD(Z80_TAG, Z80, XTAL_5MHz/2) /* U880D */
    MDRV_CPU_PROGRAM_MAP(vcs80_mem)
    MDRV_CPU_IO_MAP(vcs80_io)
	MDRV_CPU_CONFIG(vcs80_daisy_chain)

    MDRV_MACHINE_START(vcs80)

	/* keyboard timer */
	MDRV_TIMER_ADD_PERIODIC("keyboard", vcs80_keyboard_tick, HZ(1000))

    /* video hardware */
	MDRV_DEFAULT_LAYOUT( layout_vcs80 )

	/* devices */
	MDRV_Z80PIO_ADD(Z80PIO_TAG, pio_intf)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("1K")
MACHINE_DRIVER_END

/* ROMs */

ROM_START( vcs80 )
	ROM_REGION( 0x10000, Z80_TAG, 0 )
	ROM_LOAD( "monitor.rom", 0x0000, 0x0200, CRC(44aff4e9) SHA1(3472e5a9357eaba3ed6de65dee2b1c6b29349dd2) )
ROM_END

/* Driver Initialization */

static DIRECT_UPDATE_HANDLER( vcs80_direct_update_handler )
{
	vcs80_state *state = (vcs80_state *)space->machine->driver_data;

	/* _A0 is connected to PIO PB7 */
	z80pio_p_w(state->z80pio, 1, (!BIT(address, 0)) << 7);

	return address;
}

static DRIVER_INIT( vcs80 )
{
	memory_set_direct_update_handler(cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM), vcs80_direct_update_handler);
	memory_set_direct_update_handler(cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_IO), vcs80_direct_update_handler);
}

/* System Drivers */

/*    YEAR  NAME    PARENT  COMPAT  MACHINE INPUT   INIT    COMPANY             FULLNAME    FLAGS */
COMP( 1983, vcs80,  0,		0,		vcs80,	vcs80,	vcs80,	"Eckhard Schiller",	"VCS-80",	GAME_SUPPORTS_SAVE | GAME_NO_SOUND)
