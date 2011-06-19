/******************************************************************************

 AIM65

******************************************************************************/


#include "emu.h"
#include "includes/aim65.h"

/* Peripheral chips */
#include "machine/6821pia.h"
#include "machine/6522via.h"

/* DL1416A display chip */
#include "video/dl1416.h"

/* M6502 main CPU */
#include "cpu/m6502/m6502.h"

#include "machine/ram.h"


/******************************************************************************
 Global variables
******************************************************************************/





/******************************************************************************
 Interrupt handling
******************************************************************************/

/* STEP/RUN
 *
 * Switch S2 (STEP/RUN) causes AIM 65 to operate either in the RUN mode or the
 * single STEP mode. In the STEP mode, the NMI interrupt line is driven low
 * when SYNC and O2 go high during instruction execution if the address lines
 * are outside the A000-FFFF range. The NMI interrupt occurs on the high to
 * low transition of the NMI line. The Monitor software will trace instructions
 * and register, outside the Monitor instruction address range if the trace
 * modes are selected and the NMI Interrupt Routine is not bypassed.
 */



/******************************************************************************
 6821 PIA
******************************************************************************/

/* PA0: A0 (Address)
 * PA1: A1 (Address)
 * PA2: CE1 (Chip enable)
 * PA3: CE2 (Chip enable)
 * PA4: CE3 (Chip enable)
 * PA5: CE4 (Chip enable)
 * PA6: CE5 (Chip enable)
 * PA7: W (Write enable)
 * PB0-6: D0-D6 (Data)
 * PB7: CU (Cursor)
 */

static void dl1416_update(device_t *device, int index)
{
	aim65_state *state = device->machine().driver_data<aim65_state>();
	dl1416_ce_w(device, state->m_pia_a & (0x04 << index));
	dl1416_wr_w(device, BIT(state->m_pia_a, 7));
	dl1416_cu_w(device, BIT(state->m_pia_b, 7));
	dl1416_data_w(device, state->m_pia_a & 0x03, state->m_pia_b & 0x7f);
}

static void aim65_pia(running_machine &machine)
{
	dl1416_update(machine.device("ds1"), 0);
	dl1416_update(machine.device("ds2"), 1);
	dl1416_update(machine.device("ds3"), 2);
	dl1416_update(machine.device("ds4"), 3);
	dl1416_update(machine.device("ds5"), 4);
}


WRITE8_DEVICE_HANDLER(aim65_pia_a_w)
{
	aim65_state *state = device->machine().driver_data<aim65_state>();
	state->m_pia_a = data;
	aim65_pia(device->machine());
}


WRITE8_DEVICE_HANDLER(aim65_pia_b_w)
{
	aim65_state *state = device->machine().driver_data<aim65_state>();
	state->m_pia_b = data;
	aim65_pia(device->machine());
}


void aim65_update_ds1(device_t *device, int digit, int data)
{
	output_set_digit_value(0 + (digit ^ 3), data);
}

void aim65_update_ds2(device_t *device, int digit, int data)
{
	output_set_digit_value(4 + (digit ^ 3), data);
}

void aim65_update_ds3(device_t *device, int digit, int data)
{
	output_set_digit_value(8 + (digit ^ 3), data);
}

void aim65_update_ds4(device_t *device, int digit, int data)
{
	output_set_digit_value(12 + (digit ^ 3), data);
}

void aim65_update_ds5(device_t *device, int digit, int data)
{
	output_set_digit_value(16 + (digit ^ 3), data);
}



/******************************************************************************
 6532 RIOT
******************************************************************************/


READ8_DEVICE_HANDLER(aim65_riot_b_r)
{
	aim65_state *state = device->machine().driver_data<aim65_state>();
	static const char *const keynames[] =
	{
		"keyboard_0", "keyboard_1", "keyboard_2", "keyboard_3",
		"keyboard_4", "keyboard_5", "keyboard_6", "keyboard_7"
	};

	int row, data = 0xff;

	/* scan keyboard rows */
	for (row = 0; row < 8; row++)
	{
		if (!(state->m_riot_port_a & (1 << row)))
			data &= input_port_read(device->machine(), keynames[row]);
	}

	return data;
}


WRITE8_DEVICE_HANDLER(aim65_riot_a_w)
{
	aim65_state *state = device->machine().driver_data<aim65_state>();
	state->m_riot_port_a = data;
}


WRITE_LINE_DEVICE_HANDLER(aim65_riot_irq)
{
	cputag_set_input_line(device->machine(), "maincpu", M6502_IRQ_LINE, state ? HOLD_LINE : CLEAR_LINE);
}



/***************************************************************************
    DRIVER INIT
***************************************************************************/

MACHINE_START( aim65 )
{
	device_t *ram = machine.device(RAM_TAG);
	address_space *space = machine.device("maincpu")->memory().space(AS_PROGRAM);

	/* Init RAM */
	space->install_ram(0x0000, ram_get_size(ram) - 1, ram_get_ptr(ram));
}


/* RESET
 *
 * Pushbutton switch S1 initiates RESET of the AIM65 hardware and software.
 * Timer Z4 holds the RES low for at least 15 ms from the time the pushbutton
 * is released. RES is routed to the R6502 CPU, the Monitor R6522 (Z32), the
 * Monitor R6532 RIOT (Z33), the user R6522 VIA (Z1), and the display R6520 PIA
 * (U1). To initiate the device RESET function is also routed to the expansion
 * connector for off-board RESET functions. The Monitor performs a software
 * reset when the RES line goes high.
 */
