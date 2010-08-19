/*

COMX-35E Expansion Box
(c) 1984 Comx World Operations

PCB Layout
----------

F-001-EB-REV0

     |--------------------------------------|
     |  40174       4073    4049    4075    |
     |                                      |
     |  ROM         40175   4073    4075    |
     |                                      |
|----|      -       -       -       -       |
|           |       |       |       | 7805  |
|           |       |       |       |       |
|           |       |       |       |       |
|C          C       C       C       C       |
|N          N       N       N       N       |
|5          1       2       3       4       |
|           |       |       |       |       |
|           |       |       |       |       |
|           |       |       |       |       |
|           -       -       - LD1   -       |
|-------------------------------------------|

Notes:
    All IC's shown.

    ROM     - NEC D2732D-4 4Kx8 EPROM, unlabeled
    CN1     - COMX-35 bus connector slot 1
    CN2     - COMX-35 bus connector slot 2
    CN3     - COMX-35 bus connector slot 3
    CN4     - COMX-35 bus connector slot 4
    CN5     - COMX-35 bus PCB edge connector
    LD1     - LED

*/

/*

COMX-35 Disk Controller Card
(c) 1984 Comx World Operations

PCB Layout
----------

F-001-FD-REV0

    |---------------|
    |      CN1      |
|---|               |---------------------------|
|                                               |
|   40174               4068    4072           -|
|           ROM                                ||
|   LS04                4072    4050    7438   C|
|8MHz                                          N|
|                       4049    4075    LS08   2|
|LD1        WD1770                             ||
|   40174               4503    4075    7438   -|
|LD2                                            |
|-----------------------------------------------|

Notes:
    All IC's shown.

    ROM     - "D.O.S. V1.2"
    WD1770  - Western Digital WD1770-xx Floppy Disc Controller @ 8MHz
    CN1     - COMX-35 bus PCB edge connector
    CN2     - 34 pin floppy connector
    LD1     - card selected LED
    LD2     - floppy motor on LED

*/

/*

COMX-35 80-Column Card
(c) 1985 Comx World Operations

PCB Layout
----------

F-003-CLM-REV 1

    |---------------|
    |      CN1      |
|---|               |---------------------------|
|                                               |
|  MC14174  LS86    LS175   LS10    LS161       |
|                                   14.31818MHz |
|                           LS245   LS04        |
|    ROM1       6845                        CN2 |
|                           LS374   LS165       |
|LD2 LS138  LS157   LS157                       |
|LD1                        6116    ROM2    SW1 |
|    LS126  LS32    LS157                       |
|-----------------------------------------------|

Notes:
    All IC's shown.

    6845    - Motorola MC6845P CRT Controller
    6116    - Motorola MCM6116P15 2Kx8 Asynchronous CMOS Static RAM
    ROM1    - Mitsubishi 2Kx8 EPROM "C"
    ROM2    - Mitsubishi 2Kx8 EPROM "P"
    CN1     - COMX-35 bus PCB edge connector
    CN2     - RCA video output connector
    LD1     - LED
    LD2     - LED
    SW1     - switch

*/

#include "emu.h"
#include "cpu/cdp1802/cdp1802.h"
#include "machine/wd17xx.h"
#include "devices/printer.h"
#include "devices/snapquik.h"
#include "sound/cdp1869.h"
#include "video/mc6845.h"
#include "includes/comx35.h"
#include "machine/rescap.h"
#include "devices/flopdrv.h"
#include "devices/messram.h"

enum
{
	PRINTER_PARALLEL = 0,
	PRINTER_SERIAL,
	PRINTER_THERMAL,
	PRINTER_PLOTTER
};

static UINT8 read_expansion(running_machine *machine)
{
	UINT8 result;
	switch(machine->phase())
	{
		case MACHINE_PHASE_RESET:
		case MACHINE_PHASE_RUNNING:
			result = input_port_read(machine, "EXPANSION");
			break;

		default:
			result = 0x00;
			break;
	}
	return result;
}

static running_device *get_printer_device(running_machine *machine)
{
	return machine->device("printer");
}

static int expansion_box_installed(running_machine *machine)
{
	return (memory_region(machine, CDP1802_TAG)[0xe800] != 0x00);
}

static int dos_card_active(running_machine *machine)
{
	comx35_state *state = machine->driver_data<comx35_state>();

	if (expansion_box_installed(machine))
	{
		return (state->bank == BANK_FLOPPY);
	}
	else
	{
		return read_expansion(machine) == BANK_FLOPPY;
	}
}

/* Floppy Disc Controller */
static WRITE_LINE_DEVICE_HANDLER( comx35_fdc_intrq_w )
{
	comx35_state *driver_state = device->machine->driver_data<comx35_state>();
	driver_state->fdc_irq = state;
}

static WRITE_LINE_DEVICE_HANDLER( comx35_fdc_drq_w )
{
	comx35_state *driver_state = device->machine->driver_data<comx35_state>();

	if (driver_state->fdc_drq_enable)
		driver_state->cdp1802_ef4 = state;
}

const wd17xx_interface comx35_wd17xx_interface =
{
	DEVCB_NULL,
	DEVCB_LINE(comx35_fdc_intrq_w),
	DEVCB_LINE(comx35_fdc_drq_w),
	{FLOPPY_0, FLOPPY_1, FLOPPY_2, FLOPPY_3}
};

static UINT8 fdc_r(address_space *space)
{
	comx35_state *state = space->machine->driver_data<comx35_state>();
	running_device *fdc = space->machine->device(WD1770_TAG);

	UINT8 data;

	if (state->cdp1802_q)
	{
		data = state->fdc_irq;
	}
	else
	{
		data = wd17xx_r(fdc, state->fdc_addr);
	}

	return data;
}

static void fdc_w(address_space *space, UINT8 data)
{
	running_device *fdc = space->machine->device(WD1770_TAG);
	/*

        bit     description

        0       A0
        1       A1
        2       DRIVE0
        3       DRIVE1
        4       F9 DISB
        5       SIDE SELECT

    */

	comx35_state *state = space->machine->driver_data<comx35_state>();

	if (state->cdp1802_q)
	{
		// latch data to F3

		state->fdc_addr = data & 0x03;

		if (BIT(data, 2))
		{
			wd17xx_set_drive(fdc,0);
		}
		else if (BIT(data, 3))
		{
			wd17xx_set_drive(fdc,1);
		}

		state->fdc_drq_enable = BIT(data, 4);

		if (!state->fdc_drq_enable)
		{
			state->cdp1802_ef4 = 1;
		}

		wd17xx_set_side(fdc,BIT(data, 5));
	}
	else
	{
		// write data to WD1770

		wd17xx_w(fdc, state->fdc_addr, data);
	}
}

/* Printer */

static UINT8 printer_r(running_machine *machine)
{
//  comx35_state *state = machine->driver_data<comx35_state>();

	int printer = input_port_read(machine, "PRINTER") & 0x07;
	UINT8 data = 0;

	switch (printer)
	{
	case PRINTER_PARALLEL:
	case PRINTER_PLOTTER:
		/*
            INP 2 for the printer status, where:
            b0=1: Acknowledge Fault
            b1=0: Device Busy
            b2=0: Paper Empty
            b3=1: Device Not Selected
        */

		data = 0x06;
		break;

	case PRINTER_SERIAL:
		/*
            INP 2 for the printer status and to start a new range of bits for the next byte.
        */
		break;

	case PRINTER_THERMAL:
		/*
            INP 2 is used for the printer status, where:
            b0=1: Printer Not Ready
            b1=1: Energizing Head
            b2=1: Head At Position 0
        */
		break;
	}

	return data;
}

static void printer_w(running_machine *machine, UINT8 data)
{
//  comx35_state *state = machine->driver_data<comx35_state>();
	int printer = input_port_read(machine, "PRINTER") & 0x07;

	switch (printer)
	{
	case PRINTER_PARALLEL:
	case PRINTER_PLOTTER:
		/*
            OUT 2 is used to send a byte to the printer
        */

		printer_output(get_printer_device(machine), data);
		break;

	case PRINTER_SERIAL:
		/*
            OUT 2 is used to send a bit to the printer
        */
		break;

	case PRINTER_THERMAL:
		/*
            OUT 2 is used to control the thermal printer where:
            Q = 0, b0-7: Pixel 1 to 8
            Q = 1, b7: Pixel 9 (if b0-6=#21)
            Q = 1, b3=1: Move head right
            Q = 1, b0-7=#12: Move head left
        */
		break;
	}
}

/* Read/Write Handlers */

static void get_active_bank(running_machine *machine, UINT8 data)
{
	comx35_state *state = machine->driver_data<comx35_state>();
	static const char *const slotnames[] = { "", "SLOT1", "SLOT2", "SLOT3", "SLOT4" };

	if (expansion_box_installed(machine))
	{
		// expansion box

		int i;

		for (i = 1; i < 5; i++)
		{
			if (BIT(data, i))
			{
				state->slot = i;
				break;
			}
		}

		if (state->slot > 0)
			state->bank = input_port_read(machine, slotnames[state->slot]);
	}
	else
	{
		// no expansion box

		state->bank = read_expansion(machine);
	}

	// RAM expansion bank
	state->rambank = (data >> 5) & 0x03;
}

static void set_active_bank(running_machine *machine)
{
	comx35_state *state = machine->driver_data<comx35_state>();
	address_space *program = cputag_get_address_space(machine, CDP1802_TAG, ADDRESS_SPACE_PROGRAM);
	int bank = state->bank;

	switch (state->bank)
	{
	case BANK_NONE:
		memory_unmap_readwrite(program, 0xc000, 0xdfff, 0, 0);
		break;

	case BANK_FLOPPY:
		memory_install_read_bank(program, 0xc000, 0xdfff, 0, 0, "bank1");
		memory_unmap_write(program, 0xc000, 0xdfff, 0, 0);
		break;

	case BANK_PRINTER_PARALLEL:
		memory_install_read_bank(program, 0xc000, 0xc7ff, 0, 0, "bank1");
		memory_unmap_write(program, 0xc000, 0xc7ff, 0, 0);
		memory_unmap_readwrite(program, 0xc800, 0xdfff, 0, 0);
		break;

	case BANK_PRINTER_PARALLEL_FM:
		memory_install_read_bank(program, 0xc000, 0xcfff, 0, 0, "bank1");
		memory_unmap_write(program, 0xc000, 0xcfff, 0, 0);
		memory_unmap_readwrite(program, 0xd000, 0xdfff, 0, 0);
		break;

	case BANK_PRINTER_SERIAL:
		memory_install_read_bank(program, 0xc000, 0xc7ff, 0, 0, "bank1");
		memory_unmap_write(program, 0xc000, 0xc7ff, 0, 0);
		memory_unmap_readwrite(program, 0xc800, 0xdfff, 0, 0);
		break;

	case BANK_PRINTER_THERMAL:
		memory_install_read_bank(program, 0xc000, 0xcfff, 0, 0, "bank1");
		memory_unmap_write(program, 0xc000, 0xcfff, 0, 0);
		memory_unmap_readwrite(program, 0xd000, 0xdfff, 0, 0);
		break;

	case BANK_JOYCARD:
		memory_unmap_readwrite(program, 0xc000, 0xdfff, 0, 0);
		break;

	case BANK_80_COLUMNS:
		{
			running_device *mc6845 = machine->device(MC6845_TAG);

			memory_install_read_bank(program, 0xc000, 0xc7ff, 0, 0, "bank1"); // ROM
			memory_unmap_write(program, 0xc000, 0xc7ff, 0, 0); // ROM
			memory_unmap_readwrite(program, 0xc800, 0xcfff, 0, 0);
			memory_install_readwrite8_handler(program, 0xd000, 0xd7ff, 0, 0, comx35_videoram_r, comx35_videoram_w);
			memory_unmap_read(program, 0xd800, 0xd800, 0, 0);
			memory_install_write8_device_handler(program, mc6845, 0xd800, 0xd800, 0, 0, mc6845_address_w);
			memory_install_readwrite8_device_handler(program, mc6845, 0xd801, 0xd801, 0, 0, mc6845_register_r, mc6845_register_w);
			memory_unmap_readwrite(program, 0xd802, 0xdfff, 0, 0);
		}
		break;

	case BANK_RAMCARD:
		{
			bank = BANK_RAMCARD + state->rambank;

			memory_install_readwrite_bank(program, 0xc000, 0xdfff, 0, 0, "bank1");
		}
		break;
	}

	memory_set_bank(machine, "bank1", bank);
}

WRITE8_HANDLER( comx35_bank_select_w )
{
	get_active_bank(space->machine, data);
	set_active_bank(space->machine);
}

READ8_HANDLER( comx35_io_r )
{
	comx35_state *state = space->machine->driver_data<comx35_state>();

	UINT8 data = 0xff;

	switch (state->bank)
	{
	case BANK_NONE:
		break;

	case BANK_FLOPPY:
		data = fdc_r(space);
		break;

	case BANK_PRINTER_PARALLEL:
	case BANK_PRINTER_PARALLEL_FM:
	case BANK_PRINTER_SERIAL:
	case BANK_PRINTER_THERMAL:
		data = printer_r(space->machine);
		break;

	case BANK_JOYCARD:
		data = input_port_read(space->machine, "JOY1");
		break;

	case BANK_80_COLUMNS:
		break;

	case BANK_RAMCARD:
		break;
	}

	return data;
}

READ8_HANDLER( comx35_io2_r )
{
	comx35_state *state = space->machine->driver_data<comx35_state>();

	UINT8 data = 0xff;

	switch (state->bank)
	{
	case BANK_NONE:
		break;

	case BANK_FLOPPY:
		break;

	case BANK_PRINTER_PARALLEL:
	case BANK_PRINTER_PARALLEL_FM:
	case BANK_PRINTER_SERIAL:
	case BANK_PRINTER_THERMAL:
		break;

	case BANK_JOYCARD:
		data = input_port_read(space->machine, "JOY2");
		break;

	case BANK_80_COLUMNS:
		break;

	case BANK_RAMCARD:
		break;
	}

	return data;
}

WRITE8_HANDLER( comx35_io_w )
{
	comx35_state *state = space->machine->driver_data<comx35_state>();

	switch (state->bank)
	{
	case BANK_NONE:
		break;

	case BANK_FLOPPY:
		fdc_w(space, data);
		break;

	case BANK_PRINTER_PARALLEL:
	case BANK_PRINTER_PARALLEL_FM:
	case BANK_PRINTER_SERIAL:
	case BANK_PRINTER_THERMAL:
		printer_w(space->machine, data);
		break;

	case BANK_JOYCARD:
		break;

	case BANK_80_COLUMNS:
		break;

	case BANK_RAMCARD:
		break;
	}
}

/* Machine Initialization */

static TIMER_CALLBACK( reset_tick )
{
	comx35_state *state = machine->driver_data<comx35_state>();

	state->cdp1802_mode = CDP1802_MODE_RUN;
}

DIRECT_UPDATE_HANDLER( comx35_opbase_handler )
{
	if (address >= 0x0dd0 && address <= 0x0ddf)
	{
		if (dos_card_active(machine))
		{
			// read opcode from DOS ROM
			direct.explicit_configure(0x0dd0, 0x0ddf, 0x000f, memory_region(machine, "fdc"));
			return ~0;
		}
	}

	return address;
}

static STATE_POSTLOAD( comx35_state_save_postload )
{
	set_active_bank(machine);
}

MACHINE_START( comx35p )
{
	comx35_state *state = machine->driver_data<comx35_state>();
	address_space *program = cputag_get_address_space(machine, CDP1802_TAG, ADDRESS_SPACE_PROGRAM);

	/* opbase handling for DOS Card */

	program->set_direct_update_handler(direct_update_delegate_create_static(comx35_opbase_handler, *machine));

	/* BASIC ROM banking */

	memory_install_read_bank(program, 0x1000, 0x17ff, 0, 0, "bank2");
	memory_unmap_write(program, 0x1000, 0x17ff, 0, 0);
	memory_configure_bank(machine, "bank2", 0, 1, memory_region(machine, CDP1802_TAG) + 0x1000, 0); // normal ROM
	memory_configure_bank(machine, "bank2", 1, 1, memory_region(machine, CDP1802_TAG) + 0xe000, 0); // expansion box ROM

	memory_configure_bank(machine, "bank3", 0, 1, memory_region(machine, CDP1802_TAG) + 0xe000, 0);
	memory_set_bank(machine, "bank3", 0);

	if (expansion_box_installed(machine))
	{
		memory_install_read_bank(program, 0xe000, 0xefff, 0, 0, "bank3");
		memory_unmap_write(program, 0xe000, 0xefff, 0, 0);
		memory_set_bank(machine, "bank2", 1);
	}
	else
	{
		memory_unmap_readwrite(program, 0xe000, 0xefff, 0, 0);
		memory_set_bank(machine, "bank2", 0);
	}

	/* card slot banking */

	memory_configure_bank(machine, "bank1", 0, 1, memory_region(machine, CDP1802_TAG) + 0xc000, 0);
	memory_configure_bank(machine, "bank1", BANK_FLOPPY, 1, memory_region(machine, "fdc"), 0);
	memory_configure_bank(machine, "bank1", BANK_PRINTER_PARALLEL, 1, memory_region(machine, "printer"), 0);
	memory_configure_bank(machine, "bank1", BANK_PRINTER_PARALLEL_FM, 1, memory_region(machine, "printer_fm"), 0);
	memory_configure_bank(machine, "bank1", BANK_PRINTER_SERIAL, 1, memory_region(machine, "rs232"), 0);
	memory_configure_bank(machine, "bank1", BANK_PRINTER_THERMAL, 1, memory_region(machine, "thermal"), 0);
	memory_configure_bank(machine, "bank1", BANK_JOYCARD, 1, memory_region(machine, CDP1802_TAG), 0);
	memory_configure_bank(machine, "bank1", BANK_80_COLUMNS, 1, memory_region(machine, "80column"), 0);
	memory_configure_bank(machine, "bank1", BANK_RAMCARD, 4, messram_get_ptr(machine->device("messram")), 0x2000);

	memory_set_bank(machine, "bank1", 0);

	if (!expansion_box_installed(machine))
	{
		state->bank = read_expansion(machine);
	}

	/* allocate reset timer */

	state->reset_timer = timer_alloc(machine, reset_tick, NULL);

	/* screen format */

	state->pal_ntsc = 1;

	/* register for state saving */

	state_save_register_postload(machine, comx35_state_save_postload, NULL);

	state_save_register_global(machine, state->cdp1802_mode);
	state_save_register_global(machine, state->cdp1802_q);
	state_save_register_global(machine, state->cdp1802_ef4);
	state_save_register_global(machine, state->iden);
	state_save_register_global(machine, state->slot);
	state_save_register_global(machine, state->bank);
	state_save_register_global(machine, state->rambank);
	state_save_register_global(machine, state->dma);

	state_save_register_global(machine, state->cdp1871_efxa);
	state_save_register_global(machine, state->cdp1871_efxb);

	state_save_register_global(machine, state->fdc_addr);
	state_save_register_global(machine, state->fdc_irq);
	state_save_register_global(machine, state->fdc_drq_enable);
}

MACHINE_START( comx35n )
{
	comx35_state *state = machine->driver_data<comx35_state>();

	MACHINE_START_CALL(comx35p);

	// screen format

	state->pal_ntsc = 0;
}

MACHINE_RESET( comx35 )
{
	comx35_state *state = machine->driver_data<comx35_state>();
	int t = RES_K(27) * CAP_U(1) * 1000; // t = R1 * C1

	state->cdp1802_mode = CDP1802_MODE_RESET;
	state->iden = 1;
	state->cdp1802_ef4 = 1;

	timer_adjust_oneshot(state->reset_timer, ATTOTIME_IN_MSEC(t), 0);
}

INPUT_CHANGED( comx35_reset )
{
	if (BIT(input_port_read(field->port->machine, "RESET"), 0) && BIT(input_port_read(field->port->machine, "D6"), 7))
	{
		running_machine *machine = field->port->machine;
		MACHINE_RESET_CALL(comx35);
	}
}
