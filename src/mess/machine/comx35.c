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
#include "cpu/cosmac/cosmac.h"
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

UINT8 comx35_state::read_expansion()
{
	UINT8 result;
	switch(m_machine.phase())
	{
		case MACHINE_PHASE_RESET:
		case MACHINE_PHASE_RUNNING:
			result = input_port_read(&m_machine, "EXPANSION");
			break;

		default:
			result = 0x00;
			break;
	}
	return result;
}

bool comx35_state::is_expansion_box_installed()
{
	return (memory_region(&m_machine, CDP1802_TAG)[0xe800] != 0x00);
}

bool comx35_state::is_dos_card_active()
{
	if (is_expansion_box_installed())
	{
		return (m_bank == BANK_FLOPPY);
	}
	else
	{
		return read_expansion() == BANK_FLOPPY;
	}
}

/* Floppy Disc Controller */
static WRITE_LINE_DEVICE_HANDLER( comx35_fdc_intrq_w )
{
	comx35_state *driver_state = device->machine->driver_data<comx35_state>();
	driver_state->m_fdc_irq = state;
}

static WRITE_LINE_DEVICE_HANDLER( comx35_fdc_drq_w )
{
	comx35_state *driver_state = device->machine->driver_data<comx35_state>();

	if (driver_state->m_fdc_drq_enable)
		driver_state->m_cdp1802_ef4 = state;
}

const wd17xx_interface comx35_wd17xx_interface =
{
	DEVCB_NULL,
	DEVCB_LINE(comx35_fdc_intrq_w),
	DEVCB_LINE(comx35_fdc_drq_w),
	{ FLOPPY_0, FLOPPY_1, NULL, NULL }
};

UINT8 comx35_state::fdc_r()
{
	UINT8 data;

	if (m_cdp1802_q)
	{
		data = m_fdc_irq;
	}
	else
	{
		data = wd17xx_r(m_fdc, m_fdc_addr);
	}

	return data;
}

void comx35_state::fdc_w(UINT8 data)
{
	/*

        bit     description

        0       A0
        1       A1
        2       DRIVE0
        3       DRIVE1
        4       F9 DISB
        5       SIDE SELECT

    */

	if (m_cdp1802_q)
	{
		// latch data to F3
		m_fdc_addr = data & 0x03;

		if (BIT(data, 2))
		{
			wd17xx_set_drive(m_fdc, 0);
		}
		else if (BIT(data, 3))
		{
			wd17xx_set_drive(m_fdc, 1);
		}

		m_fdc_drq_enable = BIT(data, 4);

		if (!m_fdc_drq_enable)
		{
			m_cdp1802_ef4 = 1;
		}

		wd17xx_set_side(m_fdc, BIT(data, 5));
	}
	else
	{
		// write data to WD1770
		wd17xx_w(m_fdc, m_fdc_addr, data);
	}
}

/* Printer */

UINT8 comx35_state::printer_r()
{
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

void comx35_state::printer_w(UINT8 data)
{
	int printer = input_port_read(machine, "PRINTER") & 0x07;

	switch (printer)
	{
	case PRINTER_PARALLEL:
	case PRINTER_PLOTTER:
		/*
            OUT 2 is used to send a byte to the printer
        */

		//printer_output(m_printer, data);
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

void comx35_state::get_active_bank(UINT8 data)
{
	static const char *const slotnames[] = { "", "SLOT1", "SLOT2", "SLOT3", "SLOT4" };

	if (is_expansion_box_installed())
	{
		// expansion box
		int i;

		for (i = 1; i < 5; i++)
		{
			if (BIT(data, i))
			{
				m_slot = i;
				break;
			}
		}

		if (m_slot > 0)
			m_bank = input_port_read(&m_machine, slotnames[m_slot]);
	}
	else
	{
		// no expansion box
		m_bank = read_expansion();
	}

	// RAM expansion bank
	m_rambank = (data >> 5) & 0x03;
}

void comx35_state::set_active_bank()
{
	address_space *program = cpu_get_address_space(m_maincpu, ADDRESS_SPACE_PROGRAM);
	int bank = m_bank;

	switch (m_bank)
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
			memory_install_read_bank(program, 0xc000, 0xc7ff, 0, 0, "bank1"); // ROM
			memory_unmap_write(program, 0xc000, 0xc7ff, 0, 0); // ROM
			memory_unmap_readwrite(program, 0xc800, 0xcfff, 0, 0);
			memory_install_ram(program, 0xd000, 0xd7ff, 0, 0, m_videoram);
			memory_unmap_read(program, 0xd800, 0xd800, 0, 0);
			memory_install_write8_device_handler(program, m_crtc, 0xd800, 0xd800, 0, 0, mc6845_address_w);
			memory_install_readwrite8_device_handler(program, m_crtc, 0xd801, 0xd801, 0, 0, mc6845_register_r, mc6845_register_w);
			memory_unmap_readwrite(program, 0xd802, 0xdfff, 0, 0);
		}
		break;

	case BANK_RAMCARD:
		{
			bank = BANK_RAMCARD + m_rambank;

			memory_install_readwrite_bank(program, 0xc000, 0xdfff, 0, 0, "bank1");
		}
		break;
	}

	memory_set_bank(machine, "bank1", bank);
}

WRITE8_MEMBER( comx35_state::bank_select_w )
{
	get_active_bank(data);
	set_active_bank();
}

READ8_MEMBER( comx35_state::io_r )
{
	UINT8 data = 0xff;

	switch (m_bank)
	{
	case BANK_NONE:
		break;

	case BANK_FLOPPY:
		data = fdc_r();
		break;

	case BANK_PRINTER_PARALLEL:
	case BANK_PRINTER_PARALLEL_FM:
	case BANK_PRINTER_SERIAL:
	case BANK_PRINTER_THERMAL:
		data = printer_r();
		break;

	case BANK_JOYCARD:
		data = input_port_read(&m_machine, "JOY1");
		break;

	case BANK_80_COLUMNS:
		break;

	case BANK_RAMCARD:
		break;
	}

	return data;
}

READ8_MEMBER( comx35_state::io2_r )
{
	UINT8 data = 0xff;

	switch (m_bank)
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
		data = input_port_read(&m_machine, "JOY2");
		break;

	case BANK_80_COLUMNS:
		break;

	case BANK_RAMCARD:
		break;
	}

	return data;
}

WRITE8_MEMBER( comx35_state::io_w )
{
	switch (m_bank)
	{
	case BANK_NONE:
		break;

	case BANK_FLOPPY:
		fdc_w(data);
		break;

	case BANK_PRINTER_PARALLEL:
	case BANK_PRINTER_PARALLEL_FM:
	case BANK_PRINTER_SERIAL:
	case BANK_PRINTER_THERMAL:
		printer_w(data);
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

	state->m_reset = 1;
}

DIRECT_UPDATE_HANDLER( comx35_opbase_handler )
{
	comx35_state *state = machine->driver_data<comx35_state>();

	if (address >= 0x0dd0 && address <= 0x0ddf)
	{
		if (state->is_dos_card_active())
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
	comx35_state *state = machine->driver_data<comx35_state>();

	state->set_active_bank();
}

void comx35_state::machine_start()
{
	address_space *program = cpu_get_address_space(m_maincpu, ADDRESS_SPACE_PROGRAM);

	/* opbase handling for DOS Card */
	program->set_direct_update_handler(direct_update_delegate_create_static(comx35_opbase_handler, *machine));

	/* BASIC ROM banking */
	memory_install_read_bank(program, 0x1000, 0x17ff, 0, 0, "bank2");
	memory_unmap_write(program, 0x1000, 0x17ff, 0, 0);
	memory_configure_bank(machine, "bank2", 0, 1, memory_region(&m_machine, CDP1802_TAG) + 0x1000, 0); // normal ROM
	memory_configure_bank(machine, "bank2", 1, 1, memory_region(&m_machine, CDP1802_TAG) + 0xe000, 0); // expansion box ROM

	memory_configure_bank(machine, "bank3", 0, 1, memory_region(&m_machine, CDP1802_TAG) + 0xe000, 0);
	memory_set_bank(machine, "bank3", 0);

	if (is_expansion_box_installed())
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
	memory_configure_bank(machine, "bank1", 0, 1, memory_region(&m_machine, CDP1802_TAG) + 0xc000, 0);
	memory_configure_bank(machine, "bank1", BANK_FLOPPY, 1, memory_region(&m_machine, "fdc"), 0);
	memory_configure_bank(machine, "bank1", BANK_PRINTER_PARALLEL, 1, memory_region(&m_machine, "printer"), 0);
	memory_configure_bank(machine, "bank1", BANK_PRINTER_PARALLEL_FM, 1, memory_region(&m_machine, "printer_fm"), 0);
	memory_configure_bank(machine, "bank1", BANK_PRINTER_SERIAL, 1, memory_region(&m_machine, "rs232"), 0);
	memory_configure_bank(machine, "bank1", BANK_PRINTER_THERMAL, 1, memory_region(&m_machine, "thermal"), 0);
	memory_configure_bank(machine, "bank1", BANK_JOYCARD, 1, memory_region(&m_machine, CDP1802_TAG), 0);
	memory_configure_bank(machine, "bank1", BANK_80_COLUMNS, 1, memory_region(&m_machine, "80column"), 0);
	memory_configure_bank(machine, "bank1", BANK_RAMCARD, 4, messram_get_ptr(m_ram), 0x2000);

	memory_set_bank(machine, "bank1", 0);

	if (!is_expansion_box_installed())
	{
		m_bank = read_expansion();
	}

	/* allocate reset timer */
	m_reset_timer = timer_alloc(machine, reset_tick, NULL);

	/* register for state saving */
	state_save_register_postload(machine, comx35_state_save_postload, NULL);
	state_save_register_global(machine, m_reset);
	state_save_register_global(machine, m_cdp1802_q);
	state_save_register_global(machine, m_cdp1802_ef4);
	state_save_register_global(machine, m_iden);
	state_save_register_global(machine, m_slot);
	state_save_register_global(machine, m_bank);
	state_save_register_global(machine, m_rambank);
	state_save_register_global(machine, m_dma);
	state_save_register_global(machine, m_fdc_addr);
	state_save_register_global(machine, m_fdc_irq);
	state_save_register_global(machine, m_fdc_drq_enable);
}

void comx35_state::machine_reset()
{
	int t = RES_K(27) * CAP_U(1) * 1000; // t = R1 * C1

	m_reset = 0;
	m_iden = 1;
	m_cdp1802_ef4 = 1;

	timer_adjust_oneshot(m_reset_timer, ATTOTIME_IN_MSEC(t), 0);
}

INPUT_CHANGED( comx35_reset )
{
	comx35_state *state = field->port->machine->driver_data<comx35_state>();

	if (BIT(input_port_read(field->port->machine, "RESET"), 0) && BIT(input_port_read(field->port->machine, "D6"), 7))
	{
		state->machine_reset();
	}
}
