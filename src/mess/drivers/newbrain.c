#include "driver.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "cpu/cop400/cop400.h"
#include "machine/upd765.h"
#include "machine/6850acia.h"
#include "machine/adc080x.h"
#include "machine/z80ctc.h"
#include "machine/z80sio.h"
#include "includes/newbrain.h"
#include "machine/serial.h"
#include "devices/flopdrv.h"
#include "formats/basicdsk.h"
#include "devices/cassette.h"
#include "machine/rescap.h"
#include "devices/messram.h"
#include "newbrain.lh"

/*

    NewBrain
    Grundy Business Systems Ltd.

    32K RAM
    28K ROM

    Z80 @ 2MHz
    COP420 @ 2MHz

    Z80 @ 4MHz (416): INT/NMI=+5V, WAIT=EXTBUSRQ|BUSAKD, RESET=_FDC RESET,
    NEC 765AC @ 4 MHz (418)
    MC6850 ACIA (459)
    Z80CTC (458)
    ADC0809 (427)
    DAC0808 (461)

    Models according to the Software Technical Manual:

    Model M: 'Page Register', Expansion Port onto Z80 bus, Video, ACIA/CTC, User Port
    Model A: Expansion Port, Video, no User Port but has software driver serial port - s/w Printer, s/w V24
    Model V: ACIA/CTC, User Port

*/

/*

    TODO:

    - bitmapped graphics mode
    - COP420 microbus access
    - escape key is missing
    - CP/M 2.2 ROMs
    - floppy disc controller
    - convert FDC into a device
    - convert EIM into a device

    - Micropage ROM/RAM card
    - Z80 PIO board
    - peripheral (PI) box
    - sound card

*/

static void check_interrupt(running_machine *machine)
{
	newbrain_state *state = machine->driver_data;

	int level = (!state->clkint || !state->copint) ? ASSERT_LINE : CLEAR_LINE;

	cputag_set_input_line(machine, Z80_TAG, INPUT_LINE_IRQ0, level);
}

/* Bank Switching */

#define memory_install_unmapped(program, bank, bank_start, bank_end) \
	memory_unmap_readwrite(program, bank_start, bank_end, 0, 0);

#define memory_install_rom_helper(program, bank, bank_start, bank_end) \
	memory_install_read_bank(program, bank_start, bank_end, 0, 0, bank); \
	memory_unmap_write(program, bank_start, bank_end, 0, 0);

#define memory_install_ram_helper(program, bank, bank_start, bank_end) \
	memory_install_readwrite_bank(program, bank_start, bank_end, 0, 0, bank);

static void newbrain_eim_bankswitch(running_machine *machine)
{
	newbrain_state *state = machine->driver_data;

	const address_space *program = cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM);
	int bank;

	for (bank = 1; bank < 9; bank++)
	{
		int page = (state->a16 << 3) | bank;
		UINT8 data = ~state->pr[page];
		int ch = (data >> 3) & 0x03;
		int eim_bank = data & 0x07;

		UINT16 eim_bank_start = eim_bank * 0x2000;
		UINT16 bank_start = (bank - 1) * 0x2000;
		UINT16 bank_end = bank_start + 0x1fff;
		char bank_name[10];
		sprintf(bank_name,"bank%d",bank);
		switch (ch)
		{
		case 0:
			/* ROM */
			memory_install_rom_helper(program, bank_name, bank_start, bank_end);
			memory_configure_bank(machine, bank_name, 0, 1, memory_region(machine, "eim") + eim_bank_start, 0);
			break;

		case 2:
			/* RAM */
			memory_install_ram_helper(program, bank_name, bank_start, bank_end);
			memory_configure_bank(machine, bank_name, 0, 1, state->eim_ram + eim_bank_start, 0);
			break;

		default:
			logerror("Invalid memory channel %u!\n", ch);
			break;
		}
	}
}

static void newbrain_a_bankswitch(running_machine *machine)
{
	newbrain_state *state = machine->driver_data;

	const address_space *program = cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM);
	int bank;

	if (state->paging)
	{
		/* expansion interface module paging */
		newbrain_eim_bankswitch(machine);
		return;
	}

	for (bank = 1; bank < 9; bank++)
	{
		UINT16 bank_start = (bank - 1) * 0x2000;
		UINT16 bank_end = bank_start + 0x1fff;
		char bank_name[10];
		sprintf(bank_name,"bank%d",bank);

		if (state->pwrup)
		{
			/* all banks point to ROM at 0xe000 */
			memory_install_rom_helper(program, bank_name, bank_start, bank_end);
			memory_configure_bank(machine, bank_name, 0, 1, memory_region(machine, Z80_TAG) + 0xe000, 0);
		}
		else
		{
			memory_configure_bank(machine, bank_name, 0, 1, memory_region(machine, Z80_TAG) + bank_start, 0);

			if (bank < 5)
			{
				/* bank is RAM */
				memory_install_ram_helper(program, bank_name, bank_start, bank_end);
			}
			else if (bank == 5)
			{
				/* 0x8000-0x9fff */
				if (memory_region(machine, "eim"))
				{
					/* expansion interface ROM */
					memory_install_rom_helper(program, bank_name, bank_start, bank_end);
					memory_configure_bank(machine, bank_name, 0, 1, memory_region(machine, "eim") + 0x4000, 0);
				}
				else
				{
					/* mirror of 0xa000-0xbfff */
					if (memory_region(machine, Z80_TAG)[0xa001] == 0)
					{
						/* unmapped on the M model */
						memory_install_unmapped(program, bank_name, bank_start, bank_end);
					}
					else
					{
						/* bank is ROM on the A model */
						memory_install_rom_helper(program, bank_name, bank_start, bank_end);
					}

					memory_configure_bank(machine, bank_name, 0, 1, memory_region(machine, Z80_TAG) + 0xa000, 0);
				}
			}
			else if (bank == 6)
			{
				/* 0xa000-0xbfff */
				if (memory_region(machine, Z80_TAG)[0xa001] == 0)
				{
					/* unmapped on the M model */
					memory_install_unmapped(program, bank_name, bank_start, bank_end);
				}
				else
				{
					/* bank is ROM on the A model */
					memory_install_rom_helper(program, bank_name, bank_start, bank_end);
				}
			}
			else
			{
				/* bank is ROM */
				memory_install_rom_helper(program, bank_name, bank_start, bank_end);
			}
		}

		memory_set_bank(machine, bank_name, 0);
	}
}

/* Enable/Status */

static WRITE8_HANDLER( enrg1_w )
{
	/*

        bit     signal      description

        0       _CLK        enable frame frequency clock interrupts
        1                   enable user interrupt
        2       TVP         enable video display
        3                   enable V24
        4                   V24 Select Receive Bit 0
        5                   V24 Select Receive Bit 1
        6                   V24 Select Transmit Bit 0
        7                   V24 Select Transmit Bit 1

    */

	newbrain_state *state = space->machine->driver_data;

	state->enrg1 = data;
}

static WRITE8_HANDLER( a_enrg1_w )
{
	/*

        bit     signal      description

        0       _CLK        Clock Enable
        1
        2       TVP         TV Enable
        3       IOPOWER
        4       _CTS        Clear to Send V24
        5       DO          Transmit Data V24
        6
        7       PO          Transmit Data Printer

    */

	newbrain_state *state = space->machine->driver_data;

	state->enrg1 = data;
}

#ifdef UNUSED_FUNCTION
static READ8_HANDLER( ust_r )
{
	/*

        bit     signal      description

        0       variable
        1       variable
        2       MNS         mains present
        3       _USRINT0    user status
        4       _USRINT     user interrupt
        5       _CLKINT     clock interrupt
        6       _ACINT      ACIA interrupt
        7       _COPINT     COP interrupt

    */

	newbrain_state *state = space->machine->driver_data;

	UINT8 data = (state->copint << 7) | (state->aciaint << 6) | (state->clkint << 5) | (state->userint << 4) | 0x04;

	switch ((state->enrg1 & NEWBRAIN_ENRG1_UST_BIT_0_MASK) >> 6)
	{
	case 0:
		// excess, 1=24, 0=4
		if (state->tvctl & NEWBRAIN_VIDEO_32_40)
		{
			data |= 0x01;
		}
		break;

	case 1:
		// characters per line, 1=40, 0=80
		if (state->tvctl & NEWBRAIN_VIDEO_80L)
		{
			data |= 0x01;
		}
		break;

	case 2:
		// tape in
		break;

	case 3:
		// calling indicator
		break;
	}

	switch ((state->enrg1 & NEWBRAIN_ENRG1_UST_BIT_1_MASK) >> 4)
	{
	case 0:
		// PWRUP, if set indicates that power is supplied to Z80 and memory
		if (state->pwrup)
		{
			data |= 0x02;
		}
		break;

	case 1:
		// TVCNSL, if set then processor has video device as primary console output
		if (state->tvcnsl)
		{
			data |= 0x02;
		}
		break;

	case 2:
		// _BEE, if set then processor is Model A type
		if (state->bee)
		{
			data |= 0x02;
		}
		break;

	case 3:
		// _CALLIND, calling indicator
		data |= 0x02;
		break;
	}

	return data;
}
#endif /* UNUSED_FUNCTION */

static READ8_HANDLER( a_ust_r )
{
	/*

        bit     signal      description

        0                   +5V
        1       PWRUP
        2
        3
        4
        5       _CLKINT     clock interrupt
        6
        7       _COPINT     COP interrupt

    */

	newbrain_state *state = space->machine->driver_data;

	return (state->copint << 7) | (state->clkint << 5) | (state->pwrup << 1) | 0x01;
}

static READ8_HANDLER( user_r )
{
	/*

        bit     signal      description

        0       RDDK        Received Data V24
        1       _CTSD       _Clear to Send V24
        2
        3
        4
        5       TPIN        Tape in
        6
        7       _CTSP       _Clear to Send Printer

    */

	newbrain_state *state = space->machine->driver_data;

	state->user = 0;

	return 0xff;
}

static WRITE8_HANDLER( user_w )
{
	newbrain_state *state = space->machine->driver_data;

	state->user = data;
}

/* Interrupts */

static READ8_HANDLER( clclk_r )
{
	newbrain_state *state = space->machine->driver_data;

	state->clkint = 1;
	check_interrupt(space->machine);

	return 0xff;
}

static WRITE8_HANDLER( clclk_w )
{
	newbrain_state *state = space->machine->driver_data;

	state->clkint = 1;
	check_interrupt(space->machine);
}

static READ8_HANDLER( clusr_r )
{
	newbrain_state *state = space->machine->driver_data;

	state->userint = 1;

	return 0xff;
}

static WRITE8_HANDLER( clusr_w )
{
	newbrain_state *state = space->machine->driver_data;

	state->userint = 1;
}

/* COP420 */

static READ8_HANDLER( newbrain_cop_l_r )
{
	// connected to the Z80 data bus

	newbrain_state *state = space->machine->driver_data;

	return state->cop_bus;
}

static WRITE8_HANDLER( newbrain_cop_l_w )
{
	// connected to the Z80 data bus

	newbrain_state *state = space->machine->driver_data;

	state->cop_bus = data;
}

static WRITE8_HANDLER( newbrain_cop_g_w )
{
	/*

        bit     description

        G0      _COPINT
        G1      _TM1
        G2      not connected
        G3      _TM2

    */

	newbrain_state *state = space->machine->driver_data;

	state->copint = BIT(data, 0);
	check_interrupt(space->machine);

	/* tape motor enable */

	cassette_change_state(state->cassette1, BIT(data, 1) ? CASSETTE_MOTOR_DISABLED : CASSETTE_MOTOR_ENABLED, CASSETTE_MASK_MOTOR);
	cassette_change_state(state->cassette2, BIT(data, 3) ? CASSETTE_MOTOR_DISABLED : CASSETTE_MOTOR_ENABLED, CASSETTE_MASK_MOTOR);
}

static READ8_HANDLER( newbrain_cop_g_r )
{
	/*

        bit     description

        G0      not connected
        G1      K9
        G2      K7
        G3      K3

    */

	newbrain_state *state = space->machine->driver_data;

	return (BIT(state->keydata, 3) << 3) | (BIT(state->keydata, 0) << 2) | (BIT(state->keydata, 1) << 1);
}

static WRITE8_HANDLER( newbrain_cop_d_w )
{
	/*

        bit     description

        D0      inverted to K4 -> CD4024 pin 2 (reset)
        D1      TDO
        D2      inverted to K6 -> CD4024 pin 1 (clock), CD4076 pin 7 (clock), inverted to DS8881 pin 3 (enable)
        D3      not connected

    */

	newbrain_state *state = space->machine->driver_data;

	static const char *const keynames[] = { "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7",
										"D8", "D9", "D10", "D11", "D12", "D13", "D14", "D15" };

	/* keyboard row reset */

	if (!BIT(data, 0))
	{
		state->keylatch = 0;
	}

	/* tape data output */

	state->cop_tdo = BIT(data, 1);

	cassette_output(state->cassette1, state->cop_tdo ? -1.0 : +1.0);
	cassette_output(state->cassette2, state->cop_tdo ? -1.0 : +1.0);

	/* keyboard and display clock */

	if (!BIT(data, 2))
	{
		state->keylatch++;

		if (state->keylatch == 16)
		{
			state->keylatch = 0;
		}

		state->keydata = input_port_read(space->machine, keynames[state->keylatch]);

		output_set_digit_value(state->keylatch, state->segment_data[state->keylatch]);
	}
}

static READ8_HANDLER( newbrain_cop_in_r )
{
	/*

        bit     description

        IN0     K8
        IN1     _RD
        IN2     _COP
        IN3     _WR

    */

	newbrain_state *state = space->machine->driver_data;

	return (state->cop_wr << 3) | (state->cop_access << 2) | (state->cop_rd << 1) | BIT(state->keydata, 2);
}

static WRITE8_HANDLER( newbrain_cop_so_w )
{
	// connected to K1

	newbrain_state *state = space->machine->driver_data;

	state->cop_so = data;
}

static WRITE8_HANDLER( newbrain_cop_sk_w )
{
	// connected to K2

	newbrain_state *state = space->machine->driver_data;

	state->segment_data[state->keylatch] >>= 1;
	state->segment_data[state->keylatch] = (state->cop_so << 15) | (state->segment_data[state->keylatch] & 0x7fff);
}

static READ8_HANDLER( newbrain_cop_si_r )
{
	// connected to TDI

	newbrain_state *state = space->machine->driver_data;

	state->cop_tdi = ((cassette_input(state->cassette1) > +1.0) || (cassette_input(state->cassette2) > +1.0)) ^ state->cop_tdo;

	return state->cop_tdi;
}

/* Video */

static void newbrain_tvram_w(running_machine *machine, UINT8 data, int a6)
{
	newbrain_state *state = machine->driver_data;

	/* latch video address counter bits A5-A0 */
	state->tvram = (state->tvctl & NEWBRAIN_VIDEO_80L) ? 0x04 : 0x02;

	/* latch video address counter bit A6 */
	state->tvram |= a6 << 6;

	/* latch data to video address counter bits A14-A7 */
	state->tvram = (data << 7);
}

static READ8_HANDLER( tvl_r )
{
	UINT8 data = 0xff;

	newbrain_tvram_w(space->machine, data, !offset);

	return data;
}

static WRITE8_HANDLER( tvl_w )
{
	newbrain_tvram_w(space->machine, data, !offset);
}

static WRITE8_HANDLER( tvctl_w )
{
	/*

        bit     signal      description

        0       RV          1 reverses video over entire field, ie. black on white
        1       FS          0 generates 128 characters and 128 reverse field characters from 8 bit character code. 1 generates 256 characters from 8 bit character code
        2       32/_40      0 generates 320 or 640 horizontal dots in pixel graphics mode. 1 generates 256 or 512 horizontal dots in pixel graphics mode
        3       UCR         0 selects 256 characters expressed in an 8x10 matrix, and 25 lines (max) displayed. 1 selects 256 characters in an 8x8 matrix, and 31 lines (max) displayed
        4
        5
        6       80L         0 selects 40 character line length. 1 selects 80 character line length
        7

    */

	newbrain_state *state = space->machine->driver_data;

	state->tvctl = data;
}

/* Disc Controller */

static WRITE8_HANDLER( fdc_auxiliary_w )
{
	/*

        bit     description

        0       MOTON
        1       765 RESET
        2       TC
        3
        4
        5       PA15
        6
        7

    */

	newbrain_state *state = space->machine->driver_data;

	floppy_mon_w(floppy_get_device(space->machine, 0), !BIT(data, 0));
	floppy_drive_set_ready_state(floppy_get_device(space->machine, 0), 1, 0);

	upd765_reset_w(state->upd765, BIT(data, 1));

	upd765_tc_w(state->upd765, BIT(data, 2));
}

static READ8_HANDLER( fdc_control_r )
{
	/*

        bit     description

        0
        1
        2
        3
        4
        5       FDC INT
        6       PAGING
        7       FDC ATT

    */

	newbrain_state *state = space->machine->driver_data;

	return (state->fdc_att << 7) | (state->paging << 6) | (state->fdc_int << 5);
}

#ifdef UNUSED_FUNCTION
static READ8_HANDLER( ust2_r )
{
	/*

        bit     description

        0       RDDK (V24 RxD)
        1       _CTSD (V24 Clear to Send)
        2
        3
        4
        5       TPIN
        6
        7       _CTSP (Printer Clear to Send)

    */

	return 0;
}
#endif /* UNUSED_FUNCTION */

#define NEWBRAIN_COPCMD_NULLCOM		0xd0
#define NEWBRAIN_COPCMD_DISPCOM		0xa0
//#define NEWBRAIN_COPCMD_TIMCOM        0x
#define NEWBRAIN_COPCMD_PDNCOM		0xb8
#define NEWBRAIN_COPCMD_TAPECOM		0x80

#define NEWBRAIN_COP_TAPE_RECORD	0x00
#define NEWBRAIN_COP_TAPE_PLAYBK	0x04
#define NEWBRAIN_COP_TAPE_MOTOR1	0x08
#define NEWBRAIN_COP_TAPE_MOTOR2	0x02

#define NEWBRAIN_COP_TIMER0			0x01

#define NEWBRAIN_COP_NO_DATA		0x01
#define NEWBRAIN_COP_BREAK_PRESSED	0x02

#define NEWBRAIN_COP_REGINT			0x00
/*#define NEWBRAIN_COP_CASSERR      0x
#define NEWBRAIN_COP_CASSIN         0x
#define NEWBRAIN_COP_KBD            0x
#define NEWBRAIN_COP_CASSOUT        0x*/

enum
{
	NEWBRAIN_COP_STATE_COMMAND = 0,
	NEWBRAIN_COP_STATE_DATA
};

static UINT8 copdata;
static int copstate, copbytes, copregint = 1;

static READ8_HANDLER( cop_r )
{
	newbrain_state *state = space->machine->driver_data;

	state->copint = 1;
	check_interrupt(space->machine);

	return copdata;
}

static WRITE8_HANDLER( cop_w )
{
	newbrain_state *state = space->machine->driver_data;

	copdata = data;

	switch (copstate)
	{
	case NEWBRAIN_COP_STATE_COMMAND:
		logerror("COP command %02x\n", data);

		switch (data)
		{
		case NEWBRAIN_COPCMD_NULLCOM:
			break;

		case NEWBRAIN_COPCMD_DISPCOM:
			copregint = 0;
			copbytes = 18;
			copstate = NEWBRAIN_COP_STATE_DATA;

			copdata = NEWBRAIN_COP_NO_DATA;
			state->copint = 0;
			check_interrupt(space->machine);

			break;

#if 0
		case NEWBRAIN_COPCMD_TIMCOM:
			copregint = 0;
			copbytes = 6;
			copstate = NEWBRAIN_COP_STATE_DATA;
			break;
#endif
		case NEWBRAIN_COPCMD_PDNCOM:
			/* power down */
			copregint = 0;
			break;

		default:
			if (data & NEWBRAIN_COPCMD_TAPECOM)
			{
				copregint = 0;
			}
		}
		break;

	case NEWBRAIN_COP_STATE_DATA:
		logerror("COP data %02x\n", data);
		copbytes--;

		if (copbytes == 0)
		{
			copstate = NEWBRAIN_COP_STATE_COMMAND;
			copregint = 1;
		}

		copdata = NEWBRAIN_COP_NO_DATA;
		state->copint = 0;
		check_interrupt(space->machine);

		break;
	}
}

static TIMER_DEVICE_CALLBACK( cop_regint_tick )
{
	newbrain_state *state = timer->machine->driver_data;

	if (copregint)
	{
		logerror("COP REGINT\n");
		state->copint = 0;
		check_interrupt(timer->machine);
	}
}

/* Expansion Interface Module */

static WRITE8_HANDLER( ei_enrg2_w )
{
	/*

        bit     signal      description

        0       _USERP      0 enables user data bus interrupt and also parallel latched data output (or centronics printer) interrupt
        1       ANP         1 enables ADC conversion complete interrupt and also calling indicator interrupt
        2       MLTMD       1 enables serial receive clock into multiplier input of DAC and signals data terminal not ready
        3       MSPD        1 enables 50K Baud serial data rate to be obtained ie. CTC input clock of 800 kHz. 0 selects xxx.692 kHz
        4       ENOR        1 enables serial receive clock to sound output summer, and also selects serial input from the printer port. 0 selects serial input from the comms port
        5       ANSW        1 enables second bank of 4 analogue inputs (voltage, non-ratiometric), ie. ch4-7, and enabled sound output, 0 selects ch03
        6       ENOT        1 enables serial transmit clock to sound ouput summer, and also selects serial output to the printer port. 0 selects serial output to the comms port
        7                   9th output bit for centronics printer port

    */

	newbrain_state *state = space->machine->driver_data;

	state->enrg2 = data;
}

static WRITE8_HANDLER( ei_pr_w )
{
	/*

        bit     signal      description

        0       HP0
        1       HP1
        2       HP2
        3       HP3
        4       HP4
        5       HP5
        6       HP6
        7       HP11

        HP0-HP2 are decoded to _ROM0..._ROM7 signals
        HP3-HP4 are decoded to _CH0..._CH3 signals

    */

	newbrain_state *state = space->machine->driver_data;

	int page = (BIT(offset, 12) >> 9) | (BIT(offset, 15) >> 13) | (BIT(offset, 14) >> 13) | (BIT(offset, 13) >> 13);
	int bank = (BIT(offset, 11) >> 3) | (data & 0x7f);

	state->pr[page] = bank;

	newbrain_a_bankswitch(space->machine);
}

static READ8_HANDLER( ei_user_r )
{
	newbrain_state *state = space->machine->driver_data;

	state->user = 0xff;

	return 0xff;
}

static WRITE8_HANDLER( ei_user_w )
{
	newbrain_state *state = space->machine->driver_data;

	state->user = data;
}

static READ8_HANDLER( ei_anout_r )
{
	return 0xff;
}

static WRITE8_HANDLER( ei_anout_w )
{
}

static READ8_HANDLER( ei_anin_r )
{
//  int channel = offset & 0x03;

	return 0;
}

static WRITE8_HANDLER( ei_anio_w )
{
//  int channel = offset & 0x03;
}

static READ8_HANDLER( ei_st0_r )
{
	/*

        bit     signal      description

        0                   fixed at 1 - indicates excess of 24 or 48, obsolete
        1       PWRUP       1 indicates power up from 'cold' - necessary in battery machines with power switching
        2                   1 indicates analogue or calling indicator interrupts
        3       _USRINT0    0 indicates centronics printer (latched output data) port interrupt
        4       _USRINT     0 indicates parallel data bus port interrupt
        5       _CLKINT     0 indicates frame frequency clock interrupt
        6       _ACINT      0 indicates ACIA interrupt
        7       _COPINT     0 indicates interrupt from micro-controller COP420M

    */

	newbrain_state *state = space->machine->driver_data;

	return (state->copint << 7) | (state->aciaint << 6) | (state->clkint << 5) | (state->userint << 4) | (state->userint0 << 3) | (state->pwrup) << 1 | 0x01;
}

static READ8_HANDLER( ei_st1_r )
{
	/*

        bit     signal      description

        0
        1
        2       N/_RV       1 selects normal video on power up (white on black), 0 selects reversed video (appears as D0 on the first 200 EI's)
        3       ANCH        1 indicates power is being taken from the mains supply
        4       40/_80      1 indicates that 40 column video is selected on power up. 0 selects 80 column video
        5
        6       TVCNSL      1 indicates that a video display is required on power up
        7

    */

	newbrain_state *state = space->machine->driver_data;

	return (state->tvcnsl << 6) | 0x10 | 0x08 | 0x04;
}

static READ8_HANDLER( ei_st2_r )
{
	/*

        bit     signal      description

        0                   received serial data from communications port
        1                   0 indicates 'clear-to-send' condition at communications port
        2
        3
        4
        5                   logic level tape input
        6
        7                   0 indicates 'clear-to-send' condition at printer port

    */

	return 0;
}

static READ8_HANDLER( ei_usbs_r )
{
	return 0xff;
}

static WRITE8_HANDLER( ei_usbs_w )
{
}

static WRITE8_HANDLER( ei_paging_w )
{
	newbrain_state *state = space->machine->driver_data;

	if (BIT(offset, 8))
	{
		// expansion interface module

		/*

            bit     signal      description

            0       PG          1 enables paging circuits
            1       WPL         unused
            2       A16         1 sets local A16 to 1 (ie. causes second set of 8 page registers to select addressed memory)
            3       _MPM        0 selects multi-processing mode. among other effects this extends the page registers from 8 to 12 bits in length
            4       HISLT       1 isolates the local machine. this is used in multi-processing mode
            5
            6
            7

        */

		state->paging = BIT(data, 0);
		state->a16 = BIT(data, 2);
		state->mpm = BIT(data, 3);
	}
	else if (BIT(offset, 9))
	{
		// disc controller

		/*

            bit     signal      description

            0       PAGING      1 enables paging circuits
            1
            2       HA16        1 sets local A16 to 1 (ie. causes second set of 8 page registers to select addressed memory)
            3       MPM         0 selects multi-processing mode. among other effects this extends the page registers from 8 to 12 bits in length
            4
            5       _FDC RESET
            6
            7       FDC ATT

        */

		cputag_set_input_line(space->machine, FDC_Z80_TAG, INPUT_LINE_RESET, BIT(data, 5) ? HOLD_LINE : CLEAR_LINE);

		state->paging = BIT(data, 0);
		state->a16 = BIT(data, 2);
		state->mpm = BIT(data, 3);
		state->fdc_att = BIT(data, 7);
	}
	else if (BIT(offset, 10))
	{
		// network controller
	}
}

/* A/D Converter */

static ADC080X_ON_EOC_CHANGED( newbrain_adc_on_eoc_changed )
{
	newbrain_state *state = device->machine->driver_data;

	state->anint = level;
}

static ADC080X_VREF_POSITIVE_READ( newbrain_adc_vref_pos_r )
{
	return 5.0;
}

static ADC080X_VREF_NEGATIVE_READ( newbrain_adc_vref_neg_r )
{
	return 0.0;
}

static ADC080X_INPUT_READ( newbrain_adc_input_r )
{
	return 0.0;
}

static ADC080X_INTERFACE( newbrain_adc0809_intf )
{
	newbrain_adc_on_eoc_changed,
	newbrain_adc_vref_pos_r,
	newbrain_adc_vref_neg_r,
	newbrain_adc_input_r
};

/* Memory Maps */

static ADDRESS_MAP_START( newbrain_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x1fff) AM_RAMBANK("bank1")
	AM_RANGE(0x2000, 0x3fff) AM_RAMBANK("bank2")
	AM_RANGE(0x4000, 0x5fff) AM_RAMBANK("bank3")
	AM_RANGE(0x6000, 0x7fff) AM_RAMBANK("bank4")
	AM_RANGE(0x8000, 0x9fff) AM_RAMBANK("bank5")
	AM_RANGE(0xa000, 0xbfff) AM_RAMBANK("bank6")
	AM_RANGE(0xc000, 0xdfff) AM_RAMBANK("bank7")
	AM_RANGE(0xe000, 0xffff) AM_RAMBANK("bank8")
ADDRESS_MAP_END

static ADDRESS_MAP_START( newbrain_ei_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00, 0x00) AM_MIRROR(0xff00) AM_READWRITE(clusr_r, clusr_w)
	AM_RANGE(0x01, 0x01) AM_MIRROR(0xff00) AM_WRITE(ei_enrg2_w)
	AM_RANGE(0x02, 0x02) AM_MIRROR(0xff00) AM_MASK(0xff00) AM_WRITE(ei_pr_w)
	AM_RANGE(0x03, 0x03) AM_MIRROR(0xff00) AM_READWRITE(ei_user_r, ei_user_w)
	AM_RANGE(0x04, 0x04) AM_MIRROR(0xff00) AM_READWRITE(clclk_r, clclk_w)
	AM_RANGE(0x05, 0x05) AM_MIRROR(0xff00) AM_READWRITE(ei_anout_r, ei_anout_w)
	AM_RANGE(0x06, 0x06) AM_MIRROR(0xff00) AM_READWRITE(cop_r, cop_w)
	AM_RANGE(0x07, 0x07) AM_MIRROR(0xff00) AM_WRITE(enrg1_w)
	AM_RANGE(0x08, 0x09) AM_MIRROR(0xff02) AM_READWRITE(tvl_r, tvl_w)
	AM_RANGE(0x0c, 0x0c) AM_MIRROR(0xff03) AM_WRITE(tvctl_w)
	AM_RANGE(0x10, 0x13) AM_MIRROR(0xff00) AM_READWRITE(ei_anin_r, ei_anio_w)
	AM_RANGE(0x14, 0x14) AM_MIRROR(0xff00) AM_READ(ei_st0_r)
	AM_RANGE(0x15, 0x15) AM_MIRROR(0xff00) AM_READ(ei_st1_r)
	AM_RANGE(0x16, 0x16) AM_MIRROR(0xff00) AM_READ(ei_st2_r)
	AM_RANGE(0x17, 0x17) AM_MIRROR(0xff00) AM_READWRITE(ei_usbs_r, ei_usbs_w)
	AM_RANGE(0x18, 0x18) AM_MIRROR(0xff00) AM_DEVREADWRITE(MC6850_TAG, acia6850_stat_r, acia6850_ctrl_w)
	AM_RANGE(0x19, 0x19) AM_MIRROR(0xff00) AM_DEVREADWRITE(MC6850_TAG, acia6850_data_r, acia6850_data_w)
	AM_RANGE(0x1c, 0x1f) AM_MIRROR(0xff00) AM_DEVREADWRITE(Z80CTC_TAG, z80ctc_r, z80ctc_w)
	AM_RANGE(0xff, 0xff) AM_MIRROR(0xff00) AM_MASK(0xff00) AM_WRITE(ei_paging_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( newbrain_a_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00, 0x00) AM_MIRROR(0xffc0) AM_READWRITE(clusr_r, clusr_w)
	AM_RANGE(0x03, 0x03) AM_MIRROR(0xffc0) AM_WRITE(user_w)
	AM_RANGE(0x04, 0x04) AM_MIRROR(0xffc0) AM_READWRITE(clclk_r, clclk_w)
	AM_RANGE(0x06, 0x06) AM_MIRROR(0xffc0) AM_READWRITE(cop_r, cop_w)
	AM_RANGE(0x07, 0x07) AM_MIRROR(0xffc0) AM_WRITE(a_enrg1_w)
	AM_RANGE(0x08, 0x09) AM_MIRROR(0xffc2) AM_READWRITE(tvl_r, tvl_w)
	AM_RANGE(0x0c, 0x0c) AM_MIRROR(0xffc3) AM_WRITE(tvctl_w)
	AM_RANGE(0x14, 0x14) AM_MIRROR(0xffc3) AM_READ(a_ust_r)
	AM_RANGE(0x16, 0x16) AM_MIRROR(0xffc0) AM_READ(user_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( newbrain_cop_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(COP400_PORT_L, COP400_PORT_L) AM_READWRITE(newbrain_cop_l_r, newbrain_cop_l_w)
	AM_RANGE(COP400_PORT_G, COP400_PORT_G) AM_READWRITE(newbrain_cop_g_r, newbrain_cop_g_w)
	AM_RANGE(COP400_PORT_D, COP400_PORT_D) AM_WRITE(newbrain_cop_d_w)
	AM_RANGE(COP400_PORT_IN, COP400_PORT_IN) AM_READ(newbrain_cop_in_r)
	AM_RANGE(COP400_PORT_SK, COP400_PORT_SK) AM_WRITE(newbrain_cop_sk_w)
	AM_RANGE(COP400_PORT_SIO, COP400_PORT_SIO) AM_READWRITE(newbrain_cop_si_r, newbrain_cop_so_w)
	AM_RANGE(COP400_PORT_CKO, COP400_PORT_CKO) AM_READNOP
ADDRESS_MAP_END

static ADDRESS_MAP_START( newbrain_fdc_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x1fff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( newbrain_fdc_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_DEVREAD(UPD765_TAG, upd765_status_r)
	AM_RANGE(0x01, 0x01) AM_DEVREADWRITE(UPD765_TAG, upd765_data_r, upd765_data_w)
	AM_RANGE(0x20, 0x20) AM_WRITE(fdc_auxiliary_w)
	AM_RANGE(0x40, 0x40) AM_READ(fdc_control_r)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( newbrain )
	PORT_START("D0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("STOP") PORT_CODE(KEYCODE_END) PORT_CHAR(UCHAR_MAMEKEY(END))
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("D1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x93") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x92") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x91") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))

	PORT_START("D2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')

	PORT_START("D3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')

	PORT_START("D4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')

	PORT_START("D5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('(') PORT_CHAR('[')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')

	PORT_START("D6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR(')') PORT_CHAR(']')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')

	PORT_START("D7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR(':')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("* \xC2\xA3") PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR('*') PORT_CHAR(0x00A3)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')

	PORT_START("D8")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("VIDEO TEXT") PORT_CODE(KEYCODE_RALT) PORT_CHAR(UCHAR_MAMEKEY(RALT)) // Vd
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')

	PORT_START("D9")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')

	PORT_START("D10")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('=') PORT_CHAR('@')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')

	PORT_START("D11")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('-') PORT_CHAR('\\')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')

	PORT_START("D12")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('+') PORT_CHAR('^')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("INSERT") PORT_CODE(KEYCODE_INSERT) PORT_CHAR(UCHAR_MAMEKEY(INSERT))

	PORT_START("D13")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("NEW LINE") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13) // NL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')

	PORT_START("D14")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("HOME") PORT_CODE(KEYCODE_HOME) PORT_CHAR(UCHAR_MAMEKEY(HOME)) // CH

	PORT_START("D15")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1) // SH
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("GRAPHICS") PORT_CODE(KEYCODE_LALT) PORT_CHAR(UCHAR_MAMEKEY(LALT)) // GR
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("REPEAT") // RPT
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CONTROL") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL)) // GL
INPUT_PORTS_END

/* Machine Initialization */

static READ_LINE_DEVICE_HANDLER( acia_rx )
{
	newbrain_state *driver_state = device->machine->driver_data;

	return driver_state->acia_rxd;
}

static WRITE_LINE_DEVICE_HANDLER( acia_tx )
{
	newbrain_state *driver_state = device->machine->driver_data;

	driver_state->acia_txd = state;
}

static WRITE_LINE_DEVICE_HANDLER( acia_interrupt )
{
	newbrain_state *driver_state = device->machine->driver_data;

	driver_state->aciaint = state;
}

static ACIA6850_INTERFACE( newbrain_acia_intf )
{
	0,
	0,
	DEVCB_LINE(acia_rx),
	DEVCB_LINE(acia_tx),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_LINE(acia_interrupt)
};

static WRITE_LINE_DEVICE_HANDLER( newbrain_fdc_interrupt )
{
	newbrain_state *driver_state = device->machine->driver_data;

	driver_state->fdc_int = state;
}

static const upd765_interface newbrain_upd765_interface =
{
	DEVCB_LINE(newbrain_fdc_interrupt),
	NULL,
	NULL,
	UPD765_RDY_PIN_NOT_CONNECTED,
	{FLOPPY_0,FLOPPY_1, NULL, NULL}
};

static WRITE_LINE_DEVICE_HANDLER( ctc_z0_w )
{
	newbrain_state *driver_state = device->machine->driver_data;

	/* connected to the ACIA receive clock */
	if (state) acia6850_rx_clock_in(driver_state->mc6850);
}

static WRITE_LINE_DEVICE_HANDLER( ctc_z1_w )
{
	newbrain_state *driver_state = device->machine->driver_data;

	/* connected to the ACIA transmit clock */
	if (state) acia6850_tx_clock_in(driver_state->mc6850);
}

static WRITE_LINE_DEVICE_HANDLER( ctc_z2_w )
{
	newbrain_state *driver_state = device->machine->driver_data;

	/* connected to CTC channel 0/1 clock inputs */
	z80ctc_trg0_w(driver_state->z80ctc, state);
	z80ctc_trg1_w(driver_state->z80ctc, state);
}

static Z80CTC_INTERFACE( newbrain_ctc_intf )
{
	0,              		/* timer disables */
	DEVCB_NULL,			 	/* interrupt handler */
	DEVCB_LINE(ctc_z0_w),	/* ZC/TO0 callback */
	DEVCB_LINE(ctc_z1_w),  	/* ZC/TO1 callback */
	DEVCB_LINE(ctc_z2_w)	/* ZC/TO2 callback */
};

static TIMER_DEVICE_CALLBACK( ctc_c2_tick )
{
	newbrain_state *state = timer->machine->driver_data;

	z80ctc_trg2_w(state->z80ctc, 1);
	z80ctc_trg2_w(state->z80ctc, 0);
}

INLINE int get_reset_t(void)
{
	return RES_K(220) * CAP_U(10) * 1000; // t = R128 * C125 = 2.2s
}

static TIMER_CALLBACK( reset_tick )
{
	cputag_set_input_line(machine, Z80_TAG, INPUT_LINE_RESET, CLEAR_LINE);
	cputag_set_input_line(machine, COP420_TAG, INPUT_LINE_RESET, CLEAR_LINE);
}

INLINE int get_pwrup_t(void)
{
	return RES_K(560) * CAP_U(10) * 1000; // t = R129 * C127 = 5.6s
}

static TIMER_CALLBACK( pwrup_tick )
{
	newbrain_state *state = machine->driver_data;

	state->pwrup = 0;
	newbrain_a_bankswitch(machine);
}

static MACHINE_START( newbrain )
{
	newbrain_state *state = machine->driver_data;

	/* find devices */
	state->cassette1 = devtag_get_device(machine, CASSETTE1_TAG);
	state->cassette2 = devtag_get_device(machine, CASSETTE2_TAG);

	/* allocate reset timer */
	state->reset_timer = timer_alloc(machine, reset_tick, NULL);
	timer_adjust_oneshot(state->reset_timer, ATTOTIME_IN_USEC(get_reset_t()), 0);

	/* allocate power up timer */
	state->pwrup_timer = timer_alloc(machine, pwrup_tick, NULL);
	timer_adjust_oneshot(state->pwrup_timer, ATTOTIME_IN_USEC(get_pwrup_t()), 0);

	/* initialize variables */
	state->pwrup = 1;
	state->userint = 1;
	state->userint0 = 1;
	state->clkint = 1;
	state->aciaint = 1;
	state->copint = 1;
	state->bee = 1;
	state->tvcnsl = 1;

	/* set up memory banking */
	newbrain_a_bankswitch(machine);

	/* register for state saving */
	state_save_register_global(machine, state->pwrup);
	state_save_register_global(machine, state->userint);
	state_save_register_global(machine, state->userint0);
	state_save_register_global(machine, state->clkint);
	state_save_register_global(machine, state->aciaint);
	state_save_register_global(machine, state->copint);
	state_save_register_global(machine, state->anint);
	state_save_register_global(machine, state->bee);
	state_save_register_global(machine, state->enrg1);
	state_save_register_global(machine, state->enrg2);
	state_save_register_global(machine, state->cop_bus);
	state_save_register_global(machine, state->cop_so);
	state_save_register_global(machine, state->cop_tdo);
	state_save_register_global(machine, state->cop_tdi);
	state_save_register_global(machine, state->cop_rd);
	state_save_register_global(machine, state->cop_wr);
	state_save_register_global(machine, state->cop_access);
	state_save_register_global(machine, state->keylatch);
	state_save_register_global(machine, state->keydata);
	state_save_register_global(machine, state->paging);
	state_save_register_global(machine, state->mpm);
	state_save_register_global(machine, state->a16);
	state_save_register_global_array(machine, state->pr);
	state_save_register_global(machine, state->fdc_int);
	state_save_register_global(machine, state->fdc_att);
	state_save_register_global(machine, state->user);
}

static MACHINE_START( newbrain_eim )
{
	newbrain_state *state = machine->driver_data;

	MACHINE_START_CALL(newbrain);

	/* allocate expansion RAM */
	state->eim_ram = auto_alloc_array(machine, UINT8, NEWBRAIN_EIM_RAM_SIZE);

	/* find devices */
	state->z80ctc = devtag_get_device(machine, Z80CTC_TAG);
	state->mc6850 = devtag_get_device(machine, MC6850_TAG);
	state->upd765 = devtag_get_device(machine, UPD765_TAG);

	/* register for state saving */
	state_save_register_global_pointer(machine, state->eim_ram, NEWBRAIN_EIM_RAM_SIZE);
}

static MACHINE_RESET( newbrain )
{
	newbrain_state *state = machine->driver_data;

	cputag_set_input_line(machine, Z80_TAG, INPUT_LINE_RESET, HOLD_LINE);
	cputag_set_input_line(machine, COP420_TAG, INPUT_LINE_RESET, HOLD_LINE);

	timer_adjust_oneshot(state->reset_timer, ATTOTIME_IN_MSEC(get_reset_t()), 0);
}

static INTERRUPT_GEN( newbrain_interrupt )
{
	newbrain_state *state = device->machine->driver_data;

	if (!(state->enrg1 & NEWBRAIN_ENRG1_CLK))
	{
		state->clkint = 0;
		check_interrupt(device->machine);
	}
}

static COP400_INTERFACE( newbrain_cop_intf )
{
	COP400_CKI_DIVISOR_16, // ???
	COP400_CKO_OSCILLATOR_OUTPUT,
	COP400_MICROBUS_ENABLED
};

/* Machine Drivers */

static const cassette_config newbrain_cassette_config =
{
	cassette_default_formats,
	NULL,
	CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_MUTED
};


static DEVICE_IMAGE_LOAD( newbrain_serial )
{
	if (device_load_serial(image)==INIT_PASS)
	{
		serial_device_setup(image, 9600, 8, 1, SERIAL_PARITY_NONE);

		serial_device_set_transmit_state(image, 1);

		return INIT_PASS;
	}

	return INIT_FAIL;
}


static DEVICE_GET_INFO( newbrain_serial )
{
	switch ( state )
	{
		case DEVINFO_FCT_IMAGE_LOAD:		        info->f = (genf *) DEVICE_IMAGE_LOAD_NAME( newbrain_serial );    break;
		case DEVINFO_STR_NAME:		                strcpy(info->s, "Newbrain serial port");	                    break;
		case DEVINFO_STR_IMAGE_FILE_EXTENSIONS:	    strcpy(info->s, "txt");                                         break;
		case DEVINFO_INT_IMAGE_READABLE:            info->i = 1;                                        	break;
		case DEVINFO_INT_IMAGE_WRITEABLE:			info->i = 0;                                        	break;
		case DEVINFO_INT_IMAGE_CREATABLE:	     	info->i = 0;                                        	break;		
		default: 									DEVICE_GET_INFO_CALL(serial);	break;
	}
}

#define NEWBRAIN_SERIAL	DEVICE_GET_INFO_NAME(newbrain_serial)

#define MDRV_NEWBRAIN_SERIAL_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, NEWBRAIN_SERIAL, 0)

static MACHINE_DRIVER_START( newbrain_a )
	MDRV_DRIVER_DATA(newbrain_state)

	/* basic system hardware */
	MDRV_CPU_ADD(Z80_TAG, Z80, XTAL_16MHz/8)
	MDRV_CPU_PROGRAM_MAP(newbrain_map)
	MDRV_CPU_IO_MAP(newbrain_a_io_map)
	MDRV_CPU_VBLANK_INT(SCREEN_TAG, newbrain_interrupt)

	MDRV_CPU_ADD(COP420_TAG, COP420, XTAL_16MHz/8) // COP420-GUW/N
	MDRV_CPU_IO_MAP(newbrain_cop_io_map)
	MDRV_CPU_CONFIG(newbrain_cop_intf)

	MDRV_TIMER_ADD_PERIODIC("cop_regint", cop_regint_tick, MSEC(12.5)) // HACK

	MDRV_MACHINE_START(newbrain)
	MDRV_MACHINE_RESET(newbrain)

	/* video hardware */
	MDRV_DEFAULT_LAYOUT(layout_newbrain)
	MDRV_IMPORT_FROM(newbrain_video)

	/* cassette */
	MDRV_CASSETTE_ADD("cassette1", newbrain_cassette_config)
	MDRV_CASSETTE_ADD("cassette2", newbrain_cassette_config)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("32K")
	
	MDRV_NEWBRAIN_SERIAL_ADD("serial")
MACHINE_DRIVER_END

static FLOPPY_OPTIONS_START(newbrain)
	// 180K img
FLOPPY_OPTIONS_END

static const floppy_config newbrain_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(newbrain),
	DO_NOT_KEEP_GEOMETRY
};

static MACHINE_DRIVER_START( newbrain_eim )
	MDRV_IMPORT_FROM(newbrain_a)

	/* basic system hardware */
	MDRV_CPU_MODIFY(Z80_TAG)
	MDRV_CPU_IO_MAP(newbrain_ei_io_map)

	MDRV_CPU_ADD(FDC_Z80_TAG, Z80, XTAL_4MHz)
	MDRV_CPU_PROGRAM_MAP(newbrain_fdc_map)
	MDRV_CPU_IO_MAP(newbrain_fdc_io_map)

	MDRV_MACHINE_START(newbrain_eim)

	/* Z80 CTC */
	MDRV_Z80CTC_ADD(Z80CTC_TAG, XTAL_16MHz/8, newbrain_ctc_intf)
	MDRV_TIMER_ADD_PERIODIC("z80ctc_c2", ctc_c2_tick, HZ(XTAL_16MHz/4/13))

	/* AD-DA converters */
	MDRV_ADC0809_ADD(ADC0809_TAG, 500000, newbrain_adc0809_intf)

	/* MC6850 */
	MDRV_ACIA6850_ADD(MC6850_TAG, newbrain_acia_intf)

	/* UPD765 */
	MDRV_UPD765A_ADD(UPD765_TAG, newbrain_upd765_interface)

	MDRV_FLOPPY_2_DRIVES_ADD(newbrain_floppy_config)

	/* internal ram */
	MDRV_RAM_MODIFY("messram")
	MDRV_RAM_DEFAULT_SIZE("96K")
MACHINE_DRIVER_END

/* ROMs */

ROM_START( newbrain )
	ROM_REGION( 0x10000, Z80_TAG, 0 )
	ROM_DEFAULT_BIOS( "rom20" )

	ROM_SYSTEM_BIOS( 0, "issue1", "Issue 1 (v?)" )
	ROMX_LOAD( "aben.ic6",     0xa000, 0x2000, CRC(308f1f72) SHA1(a6fd9945a3dca47636887da2125fde3f9b1d4e25), ROM_BIOS(1) )
	ROMX_LOAD( "cd iss 1.ic7", 0xc000, 0x2000, CRC(6b4d9429) SHA1(ef688be4e75aced61f487c928258c8932a0ae00a), ROM_BIOS(1) )
	ROMX_LOAD( "ef iss 1.ic8", 0xe000, 0x2000, CRC(20dd0b49) SHA1(74b517ca223cefb588e9f49e72ff2d4f1627efc6), ROM_BIOS(1) )

	ROM_SYSTEM_BIOS( 1, "issue2", "Issue 2 (v1.9)" )
	ROMX_LOAD( "aben19.ic6",   0xa000, 0x2000, CRC(d0283eb1) SHA1(351d248e69a77fa552c2584049006911fb381ff0), ROM_BIOS(2) )
	ROMX_LOAD( "cdi2.ic7",     0xc000, 0x2000, CRC(6b4d9429) SHA1(ef688be4e75aced61f487c928258c8932a0ae00a), ROM_BIOS(2) )
	ROMX_LOAD( "ef iss 1.ic8", 0xe000, 0x2000, CRC(20dd0b49) SHA1(74b517ca223cefb588e9f49e72ff2d4f1627efc6), ROM_BIOS(2) )

	ROM_SYSTEM_BIOS( 2, "issue3", "Issue 3 (v1.91)" )
	ROMX_LOAD( "aben191.ic6",  0xa000, 0x2000, CRC(b7be8d89) SHA1(cce8d0ae7aa40245907ea38b7956c62d039d45b7), ROM_BIOS(3) )
	ROMX_LOAD( "cdi3.ic7",	   0xc000, 0x2000, CRC(6b4d9429) SHA1(ef688be4e75aced61f487c928258c8932a0ae00a), ROM_BIOS(3) )
	ROMX_LOAD( "ef iss 1.ic8", 0xe000, 0x2000, CRC(20dd0b49) SHA1(74b517ca223cefb588e9f49e72ff2d4f1627efc6), ROM_BIOS(3) )

	ROM_SYSTEM_BIOS( 3, "series2", "Series 2 (v?)" )
	ROMX_LOAD( "abs2.ic6",	   0xa000, 0x2000, CRC(9a042acb) SHA1(80d83a2ea3089504aa68b6cf978d80d296cd9bda), ROM_BIOS(4) )
	ROMX_LOAD( "cds2.ic7",	   0xc000, 0x2000, CRC(6b4d9429) SHA1(ef688be4e75aced61f487c928258c8932a0ae00a), ROM_BIOS(4) )
	ROMX_LOAD( "efs2.ic8",	   0xe000, 0x2000, CRC(b222d798) SHA1(c0c816b4d4135b762f2c5f1b24209d0096f22e56), ROM_BIOS(4) )

	ROM_SYSTEM_BIOS( 4, "rom20", "? (v2.0)" )
	ROMX_LOAD( "aben20.rom",   0xa000, 0x2000, CRC(3d76d0c8) SHA1(753b4530a518ad832e4b81c4e5430355ba3f62e0), ROM_BIOS(5) )
	ROMX_LOAD( "cd20tci.rom",  0xc000, 0x4000, CRC(f65b2350) SHA1(1ada7fbf207809537ec1ffb69808524300622ada), ROM_BIOS(5) )

	ROM_REGION( 0x400, COP420_TAG, 0 )
	ROM_LOAD( "cop420.419", 0x000, 0x400, NO_DUMP )

	ROM_REGION( 0x1000, "chargen", 0 )
	ROM_LOAD( "char eprom iss 1.ic453", 0x0000, 0x0a01, BAD_DUMP CRC(46ecbc65) SHA1(3fe064d49a4de5e3b7383752e98ad35a674e26dd) ) // 8248R7
ROM_END

#define rom_newbraia rom_newbrain

ROM_START( newbraie )
	ROM_REGION( 0x10000, Z80_TAG, 0 )
	ROM_DEFAULT_BIOS( "rom20" )

	ROM_SYSTEM_BIOS( 0, "issue1", "Issue 1 (v?)" )
	ROMX_LOAD( "aben.ic6",     0xa000, 0x2000, CRC(308f1f72) SHA1(a6fd9945a3dca47636887da2125fde3f9b1d4e25), ROM_BIOS(1) )
	ROMX_LOAD( "cd iss 1.ic7", 0xc000, 0x2000, CRC(6b4d9429) SHA1(ef688be4e75aced61f487c928258c8932a0ae00a), ROM_BIOS(1) )
	ROMX_LOAD( "ef iss 1.ic8", 0xe000, 0x2000, CRC(20dd0b49) SHA1(74b517ca223cefb588e9f49e72ff2d4f1627efc6), ROM_BIOS(1) )

	ROM_SYSTEM_BIOS( 1, "issue2", "Issue 2 (v1.9)" )
	ROMX_LOAD( "aben19.ic6",   0xa000, 0x2000, CRC(d0283eb1) SHA1(351d248e69a77fa552c2584049006911fb381ff0), ROM_BIOS(2) )
	ROMX_LOAD( "cdi2.ic7",     0xc000, 0x2000, CRC(6b4d9429) SHA1(ef688be4e75aced61f487c928258c8932a0ae00a), ROM_BIOS(2) )
	ROMX_LOAD( "ef iss 1.ic8", 0xe000, 0x2000, CRC(20dd0b49) SHA1(74b517ca223cefb588e9f49e72ff2d4f1627efc6), ROM_BIOS(2) )

	ROM_SYSTEM_BIOS( 2, "issue3", "Issue 3 (v1.91)" )
	ROMX_LOAD( "aben191.ic6",  0xa000, 0x2000, CRC(b7be8d89) SHA1(cce8d0ae7aa40245907ea38b7956c62d039d45b7), ROM_BIOS(3) )
	ROMX_LOAD( "cdi3.ic7",	   0xc000, 0x2000, CRC(6b4d9429) SHA1(ef688be4e75aced61f487c928258c8932a0ae00a), ROM_BIOS(3) )
	ROMX_LOAD( "ef iss 1.ic8", 0xe000, 0x2000, CRC(20dd0b49) SHA1(74b517ca223cefb588e9f49e72ff2d4f1627efc6), ROM_BIOS(3) )

	ROM_SYSTEM_BIOS( 3, "series2", "Series 2 (v?)" )
	ROMX_LOAD( "abs2.ic6",	   0xa000, 0x2000, CRC(9a042acb) SHA1(80d83a2ea3089504aa68b6cf978d80d296cd9bda), ROM_BIOS(4) )
	ROMX_LOAD( "cds2.ic7",	   0xc000, 0x2000, CRC(6b4d9429) SHA1(ef688be4e75aced61f487c928258c8932a0ae00a), ROM_BIOS(4) )
	ROMX_LOAD( "efs2.ic8",	   0xe000, 0x2000, CRC(b222d798) SHA1(c0c816b4d4135b762f2c5f1b24209d0096f22e56), ROM_BIOS(4) )

	ROM_SYSTEM_BIOS( 4, "rom20", "? (v2.0)" )
	ROMX_LOAD( "aben20.rom",   0xa000, 0x2000, CRC(3d76d0c8) SHA1(753b4530a518ad832e4b81c4e5430355ba3f62e0), ROM_BIOS(5) )
	ROMX_LOAD( "cd20tci.rom",  0xc000, 0x4000, CRC(f65b2350) SHA1(1ada7fbf207809537ec1ffb69808524300622ada), ROM_BIOS(5) )

	ROM_REGION( 0x400, COP420_TAG, 0 )
	ROM_LOAD( "cop420.419",   0x000, 0x400, NO_DUMP )

	ROM_REGION( 0x1000, "chargen", 0 )
	ROM_LOAD( "char eprom iss 1.ic453", 0x0000, 0x0a01, BAD_DUMP CRC(46ecbc65) SHA1(3fe064d49a4de5e3b7383752e98ad35a674e26dd) ) // 8248R7

	ROM_REGION( 0x10000, "eim", 0 ) // Expansion Interface Module
	ROM_LOAD( "e415-2.rom", 0x4000, 0x2000, CRC(5b0e390c) SHA1(0f99cae57af2e64f3f6b02e5325138d6ba015e72) )
	ROM_LOAD( "e415-3.rom", 0x4000, 0x2000, CRC(2f88bae5) SHA1(04e03f230f4b368027442a7c2084dae877f53713) ) // 18/8/83.aci
	ROM_LOAD( "e416-3.rom", 0x6000, 0x2000, CRC(8b5099d8) SHA1(19b0cfce4c8b220eb1648b467f94113bafcb14e0) ) // 10/8/83.mtv
	ROM_LOAD( "e417-2.rom", 0x8000, 0x2000, CRC(6a7afa20) SHA1(f90db4f8318777313a862b3d5bab83c2fd260010) )

	ROM_REGION( 0x10000, FDC_Z80_TAG, 0 ) // Floppy Disk Controller
	ROM_LOAD( "d413-2.rom", 0x0000, 0x2000, CRC(097591f1) SHA1(c2aa1d27d4f3a24ab0c8135df746a4a44201a7f4) )
	ROM_LOAD( "d417-1.rom", 0x0000, 0x2000, CRC(40fad31c) SHA1(5137be4cc026972c0ffd4fa6990e8583bdfce163) )
	ROM_LOAD( "d417-2.rom", 0x0000, 0x2000, CRC(e8bda8b9) SHA1(c85a76a5ff7054f4ef4a472ce99ebaed1abd269c) )
ROM_END

ROM_START( newbraim )
	ROM_REGION( 0x10000, Z80_TAG, 0 )
	ROM_LOAD( "cdmd.rom", 0xc000, 0x2000, CRC(6b4d9429) SHA1(ef688be4e75aced61f487c928258c8932a0ae00a) )
	ROM_LOAD( "efmd.rom", 0xe000, 0x2000, CRC(20dd0b49) SHA1(74b517ca223cefb588e9f49e72ff2d4f1627efc6) )

	ROM_REGION( 0x400, COP420_TAG, 0 )
	ROM_LOAD( "cop420.419", 0x000, 0x400, NO_DUMP )

	ROM_REGION( 0x1000, "chargen", 0 )
	ROM_LOAD( "char eprom iss 1.ic453", 0x0000, 0x0a01, BAD_DUMP CRC(46ecbc65) SHA1(3fe064d49a4de5e3b7383752e98ad35a674e26dd) ) // 8248R7
ROM_END
	
/* System Drivers */

//    YEAR  NAME        PARENT      COMPAT  MACHINE         INPUT       INIT    COMPANY                         FULLNAME        FLAGS
COMP( 1981, newbrain,	0,			0,		newbrain_a,		newbrain,   0, 		"Grundy Business Systems Ltd.",	"NewBrain AD",	GAME_NOT_WORKING | GAME_NO_SOUND )
COMP( 1981, newbraie,	newbrain,	0,		newbrain_eim,	newbrain,   0, 		"Grundy Business Systems Ltd.",	"NewBrain AD with Expansion Interface",	GAME_NOT_WORKING | GAME_NO_SOUND )
COMP( 1981, newbraia,	newbrain,	0,		newbrain_a,		newbrain,   0, 		"Grundy Business Systems Ltd.",	"NewBrain A",	GAME_NOT_WORKING | GAME_NO_SOUND )
COMP( 1981, newbraim,	newbrain,	0,		newbrain_a,		newbrain,   0, 		"Grundy Business Systems Ltd.",	"NewBrain MD",	GAME_NOT_WORKING | GAME_NO_SOUND )
