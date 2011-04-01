/***************************************************************************

  machine/dai.c

  Functions to emulate general aspects of DAI (RAM, ROM, interrupts, I/O ports)

  Krzysztof Strzecha
  Nathan Woods

***************************************************************************/

#include <stdarg.h>
#include "emu.h"
#include "imagedev/cassette.h"
#include "cpu/i8085/i8085.h"
#include "machine/i8255a.h"
#include "includes/dai.h"
#include "machine/pit8253.h"
#include "machine/tms5501.h"
#include "machine/ram.h"

#define DEBUG_DAI_PORTS	0

#define LOG_DAI_PORT_R(_port, _data, _comment) do { if (DEBUG_DAI_PORTS) logerror ("DAI port read : %04x, Data: %02x (%s)\n", _port, _data, _comment); } while (0)
#define LOG_DAI_PORT_W(_port, _data, _comment) do { if (DEBUG_DAI_PORTS) logerror ("DAI port write: %04x, Data: %02x (%s)\n", _port, _data, _comment); } while (0)

/* Discrete I/O devices */


/* Memory */

WRITE8_HANDLER( dai_stack_interrupt_circuit_w )
{
	dai_state *state = space->machine().driver_data<dai_state>();
	tms5501_sensor (state->m_tms5501, 1);
	tms5501_sensor (state->m_tms5501, 0);
}

static void dai_update_memory(running_machine &machine, int dai_rom_bank)
{
	memory_set_bank(machine, "bank2", dai_rom_bank);
}

static TIMER_CALLBACK(dai_bootstrap_callback)
{
	cpu_set_reg(machine.device("maincpu"), STATE_GENPC, 0xc000);
}


static UINT8 dai_keyboard_read (device_t *device)
{
	dai_state *state = device->machine().driver_data<dai_state>();
	UINT8 data = 0x00;
	int i;
	static const char *const keynames[] = { "IN0", "IN1", "IN2", "IN3", "IN4", "IN5", "IN6", "IN7" };

	for (i = 0; i < 8; i++)
	{
		if (state->m_keyboard_scan_mask & (1 << i))
			data |= input_port_read(device->machine(), keynames[i]);
	}
	return data;
}

static void dai_keyboard_write (device_t *device, UINT8 data)
{
	dai_state *state = device->machine().driver_data<dai_state>();
	state->m_keyboard_scan_mask = data;
}

static void dai_interrupt_callback(device_t *device, int intreq, UINT8 vector)
{
	if (intreq)
		cputag_set_input_line_and_vector(device->machine(), "maincpu", 0, HOLD_LINE, vector);
	else
		cputag_set_input_line(device->machine(), "maincpu", 0, CLEAR_LINE);
}

const tms5501_interface dai_tms5501_interface =
{
	dai_keyboard_read,
	dai_keyboard_write,
	dai_interrupt_callback,
	2000000.
};

I8255A_INTERFACE( dai_ppi82555_intf )
{
	DEVCB_NULL,	/* Port A read */
	DEVCB_NULL,	/* Port B read */
	DEVCB_NULL,	/* Port C read */
	DEVCB_NULL,	/* Port A write */
	DEVCB_NULL,	/* Port B write */
	DEVCB_NULL	/* Port C write */
};

static WRITE_LINE_DEVICE_HANDLER( dai_pit_out0 )
{
	dai_state *drvstate = device->machine().driver_data<dai_state>();
	dai_set_input(drvstate->m_sound, 0, state);
}


static WRITE_LINE_DEVICE_HANDLER( dai_pit_out1 )
{
	dai_state *drvstate = device->machine().driver_data<dai_state>();
	dai_set_input(drvstate->m_sound, 1, state);
}


static WRITE_LINE_DEVICE_HANDLER( dai_pit_out2 )
{
	dai_state *drvstate = device->machine().driver_data<dai_state>();
	dai_set_input(drvstate->m_sound, 2, state);
}


const struct pit8253_config dai_pit8253_intf =
{
	{
		{
			2000000,
			DEVCB_NULL,
			DEVCB_LINE(dai_pit_out0)
		},
		{
			2000000,
			DEVCB_NULL,
			DEVCB_LINE(dai_pit_out1)
		},
		{
			2000000,
			DEVCB_NULL,
			DEVCB_LINE(dai_pit_out2)
		}
	}
};

static TIMER_CALLBACK( dai_timer )
{
	dai_state *state = machine.driver_data<dai_state>();
	tms5501_set_pio_bit_7 (state->m_tms5501, (input_port_read(machine, "IN8") & 0x04) ? 1:0);
}

MACHINE_START( dai )
{
	dai_state *state = machine.driver_data<dai_state>();
	state->m_sound = machine.device("custom");
	state->m_tms5501 = machine.device("tms5501");

	memory_configure_bank(machine, "bank2", 0, 4, machine.region("maincpu")->base() + 0x010000, 0x1000);
	machine.scheduler().timer_set(attotime::zero, FUNC(dai_bootstrap_callback));
	machine.scheduler().timer_pulse(attotime::from_hz(100), FUNC(dai_timer));	/* timer for tms5501 */
}

MACHINE_RESET( dai )
{
	memory_set_bankptr(machine, "bank1", ram_get_ptr(machine.device(RAM_TAG)));
}

/***************************************************************************

    Discrete Devices IO

    FD00    POR1:   IN  bit 0   -
                bit 1   -
                bit 2   PIPGE: Page signal
                bit 3   PIDTR: Serial output ready
                bit 4   PIBU1: Button on paddle 1 (1 = closed)
                bit 5   PIBU1: Button on paddle 2 (1 = closed)
                bit 6   PIRPI: Random data
                bit 7   PICAI: Cassette input data

    FD01    PDLST:  IN  Single pulse used to trigger paddle timer circuit

    FD04    POR1:   OUT bit 0-3 Volume oscillator channel 0
                bit 4-7 Volume oscillator channel 1

    FD05    POR1:   OUT bit 0-3 Volume oscillator channel 2
                bit 4-7 Volume random noise generator

    FD06    POR0:   OUT bit 0   POCAS: Cassette data output
                bit 1-2 PDLMSK: Paddle select
                bit 3   PDPNA:  Paddle enable
                bit 4   POCM1:  Cassette 1 motor control (0 = run)
                bit 5   POCM2:  Cassette 2 motor control (0 = run)
                bit 6-7         ROM bank switching
***************************************************************************/

READ8_HANDLER( dai_io_discrete_devices_r )
{
	UINT8 data = 0x00;

	switch(offset & 0x000f) {
	case 0x00:
		data = input_port_read(space->machine(), "IN8");
		data |= 0x08;			// serial ready
		if (space->machine().rand()&0x01)
			data |= 0x40;		// random number generator
		if (cassette_input(space->machine().device("cassette")) > 0.01)
			data |= 0x80;		// tape input
		break;

	default:
		data = 0xff;
		LOG_DAI_PORT_R (offset, data, "discrete devices - unmapped");

		break;
	}
	return data;
}

WRITE8_HANDLER( dai_io_discrete_devices_w )
{
	dai_state *state = space->machine().driver_data<dai_state>();
	switch(offset & 0x000f) {
	case 0x04:
		dai_set_volume(state->m_sound, offset, data);
		LOG_DAI_PORT_W (offset, data&0x0f, "discrete devices - osc. 0 volume");
		LOG_DAI_PORT_W (offset, (data&0xf0)>>4, "discrete devices - osc. 1 volume");
		break;

	case 0x05:
		dai_set_volume(state->m_sound, offset, data);
		LOG_DAI_PORT_W (offset, data&0x0f, "discrete devices - osc. 2 volume");
		LOG_DAI_PORT_W (offset, (data&0xf0)>>4, "discrete devices - noise volume");
		break;

	case 0x06:
		state->m_paddle_select = (data&0x06)>>2;
		state->m_paddle_enable = (data&0x08)>>3;
		state->m_cassette_motor[0] = (data&0x10)>>4;
		state->m_cassette_motor[1] = (data&0x20)>>5;
		cassette_change_state(space->machine().device("cassette"), state->m_cassette_motor[0]?CASSETTE_MOTOR_DISABLED:CASSETTE_MOTOR_ENABLED, CASSETTE_MASK_MOTOR);
		cassette_output(space->machine().device("cassette"), (data & 0x01) ? -1.0 : 1.0);
		dai_update_memory (space->machine(), (data&0xc0)>>6);
		LOG_DAI_PORT_W (offset, (data&0x06)>>2, "discrete devices - paddle select");
		LOG_DAI_PORT_W (offset, (data&0x08)>>3, "discrete devices - paddle enable");
		LOG_DAI_PORT_W (offset, (data&0x10)>>4, "discrete devices - cassette motor 1");
		LOG_DAI_PORT_W (offset, (data&0x20)>>5, "discrete devices - cassette motor 2");
		LOG_DAI_PORT_W (offset, (data&0xc0)>>6, "discrete devices - ROM bank");
		break;

	default:
		LOG_DAI_PORT_W (offset, data, "discrete devices - unmapped");
		break;
	}
}

/***************************************************************************

    AMD 9911 mathematical coprocesor

***************************************************************************/

READ8_HANDLER( dai_amd9511_r )
{
	/* optional and no present at this moment */
	return 0xff;
}

WRITE8_HANDLER( dai_amd9511_w )
{
	logerror ("Writing to AMD9511 math chip, %04x, %02x\n", offset, data);
}

