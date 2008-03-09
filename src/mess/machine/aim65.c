/******************************************************************************

 AIM65

******************************************************************************/


#include "driver.h"
#include "deprecat.h"
#include "includes/aim65.h"

/* Peripheral chips */
#include "machine/6821pia.h"
#include "machine/6522via.h"
#include "machine/6532riot.h"

/* DL1416A display chip */
#include "video/dl1416.h"

/* M6502 main CPU */
#include "cpu/m6502/m6502.h"



/******************************************************************************
 Global variables
******************************************************************************/


static UINT8 pia_a, pia_b;
static UINT8 riot_port_a;



/******************************************************************************
 Interrupt handling
******************************************************************************/


static void aim65_via_irq_func(int state)
{
	cpunum_set_input_line(Machine, 0, M6502_IRQ_LINE, state ? HOLD_LINE : CLEAR_LINE);
}

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

static void dl1416_update(dl1416_t *chip, int index)
{
	dl1416_set_input_ce(chip, pia_a & (0x04 << index));
	dl1416_set_input_w(chip, pia_a & 0x80);
	dl1416_set_input_cu(chip, pia_b & 0x80);
	dl1416_write(chip, pia_a & 0x03, pia_b & 0x7f);
}

static void aim65_pia(void)
{
	dl1416_update(devtag_get_token(Machine, DL1416, "ds1"), 0);
	dl1416_update(devtag_get_token(Machine, DL1416, "ds2"), 1);
	dl1416_update(devtag_get_token(Machine, DL1416, "ds3"), 2);
	dl1416_update(devtag_get_token(Machine, DL1416, "ds4"), 3);
	dl1416_update(devtag_get_token(Machine, DL1416, "ds5"), 4);
}


static WRITE8_HANDLER( aim65_pia_a_w )
{
	pia_a = data;
	aim65_pia();
}


static WRITE8_HANDLER( aim65_pia_b_w )
{
	pia_b = data;
	aim65_pia();
}


static const pia6821_interface pia =
{
	NULL, // read8_machine_func in_a_func,
	NULL, // read8_machine_func in_b_func,
	NULL, // read8_machine_func in_ca1_func,
	NULL, // read8_machine_func in_cb1_func,
	NULL, // read8_machine_func in_ca2_func,
	NULL, // read8_machine_func in_cb2_func,
	aim65_pia_a_w,
	aim65_pia_b_w,
	NULL, // write8_machine_func out_ca2_func,
	NULL, // write8_machine_func out_cb2_func,
	NULL, // void (*irq_a_func)(int state),
	NULL, // void (*irq_b_func)(int state),
};


void aim65_update_ds1(int digit, int data) { output_set_digit_value( 0 + (digit ^ 3), data); }
void aim65_update_ds2(int digit, int data) { output_set_digit_value( 4 + (digit ^ 3), data); }
void aim65_update_ds3(int digit, int data) { output_set_digit_value( 8 + (digit ^ 3), data); }
void aim65_update_ds4(int digit, int data) { output_set_digit_value(12 + (digit ^ 3), data); }
void aim65_update_ds5(int digit, int data) { output_set_digit_value(16 + (digit ^ 3), data); }



/******************************************************************************
 6532 RIOT
******************************************************************************/


static READ8_HANDLER( aim65_riot_b_r )
{
	char portname[11];
	int row, data = 0xff;

	/* scan keyboard rows */
	for (row = 0; row < 8; row++) {
		sprintf(portname, "keyboard_%d", row);
		if (!(riot_port_a & (1 << row)))
			data &= readinputportbytag(portname);
	}

	return data;
}


static WRITE8_HANDLER(aim65_riot_a_w)
{
	riot_port_a = data;
}


static void aim65_riot_irq(int state)
{
	cpunum_set_input_line(Machine, 0, M6502_IRQ_LINE, state ? HOLD_LINE : CLEAR_LINE);
}


static const struct riot6532_interface r6532_interface =
{
	NULL,
	aim65_riot_b_r,
	aim65_riot_a_w,
	NULL,
	aim65_riot_irq
};



/******************************************************************************
 6522 VIA
******************************************************************************/


static WRITE8_HANDLER( aim65_via0_a_w )
{
	 aim65_printer_data_a(data);
}


static WRITE8_HANDLER( aim65_via0_b_w )
{
	aim65_printer_data_b(data);
}


static READ8_HANDLER( aim65_via0_b_r )
{
	return readinputportbytag("switches");
}


static const struct via6522_interface via0 =
{
	0, // read8_machine_func in_a_func;
	aim65_via0_b_r, // read8_machine_func in_b_func;
	0, // read8_machine_func in_ca1_func;
	0, // read8_machine_func in_cb1_func;
	0, // read8_machine_func in_ca2_func;
	0, // read8_machine_func in_cb2_func;
	aim65_via0_a_w,	// write8_machine_func out_a_func;
	aim65_via0_b_w, // write8_machine_func out_b_func;
	0, // write8_machine_func out_ca1_func;
	0, // write8_machine_func out_cb1_func;
	0, // write8_machine_func out_ca2_func;
	aim65_printer_on, // write8_machine_func out_cb2_func;
	aim65_via_irq_func // void (*irq_func)(int state);
};

static const struct via6522_interface user_via =
{
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};



/******************************************************************************
 Driver init
******************************************************************************/


DRIVER_INIT( aim65 )
{
	/* Init RAM */
	memory_install_readwrite8_handler (0, ADDRESS_SPACE_PROGRAM,
		0, mess_ram_size - 1, 0, 0, MRA8_BANK1, MWA8_BANK1);
	memory_set_bankptr(1, mess_ram);

	if (mess_ram_size < 4 * 1024)
		memory_install_readwrite8_handler(0, ADDRESS_SPACE_PROGRAM,
			mess_ram_size, 0x0fff, 0, 0, MRA8_NOP, MWA8_NOP);

	pia_config(0, &pia);

	r6532_config(0, &r6532_interface);
	r6532_set_clock(0, AIM65_CLOCK);
	r6532_reset(0);

	via_config(0, &via0);
	via_0_cb1_w(machine, 1, 1);
	via_0_ca1_w(machine, 1, 0);

	via_config(1, &user_via);
	via_reset();
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
