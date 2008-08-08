#include "driver.h"
#include "deprecat.h"
#include "cpu/z80/z80daisy.h"
#include "cpu/cop400/cop400.h"
#include "machine/nec765.h"
#include "machine/6850acia.h"
#include "machine/z80ctc.h"
#include "machine/z80dart.h"
#include "machine/z80sio.h"
#include "includes/newbrain.h"
#include "includes/serial.h"
#include "devices/basicdsk.h"
#include "devices/cassette.h"
#include "rescap.h"

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

	Model M: 'Page Register', Expansion Port onto Z80 bus, Video, ACIA/CTC, User Port
	Model A: Expansion Port, Video, no User Port but has software driver serial port - s/w Printer, s/w V24
	Model V: ACIA/CTC, User Port

*/

/*

	TODO:

	- escape key
	- COP420 microbus
	- video
	- tape
	- layout for the 16-segment displays

	- floppy disc controller
	- CP/M 2.2
	- expansion box
	- Micropage ROM/RAM card
	- Z80 PIO board
	- peripheral (PI) box
	- sound card

*/

static int acia_irq;
static UINT8 acia_rxd = 1, acia_txd = 1;

static const device_config *cassette_device_image(int index)
{
	return image_from_devtype_and_index(IO_CASSETTE, index);
}

/* Enable/Status */

#define NEWBRAIN_ENRG1_CLK	0x01
#define NEWBRAIN_ENRG1_TVP	0x04
#define NEWBRAIN_ENRG1_CTS	0x10
#define NEWBRAIN_ENRG1_DO	0x20
#define NEWBRAIN_ENRG1_PO	0x80

static WRITE8_HANDLER( enrg1_w )
{
	/*

		bit		signal		description

		0		_CLK		enable frame frequency clock interrupts
		1					enable user interrupt
		2		TVP			enable video display
		3					enable V24
		4					V24 Select Receive Bit 0
		5					V24 Select Receive Bit 1
		6					V24 Select Transmit Bit 0
		7					V24 Select Transmit Bit 1

	*/

	newbrain_state *state = machine->driver_data;

	state->enrg1 = data;
}

static WRITE8_HANDLER( a_enrg1_w )
{
	/*

		bit		signal		description

		0		_CLK		Clock Enable
		1		
		2		TVP			TV Enable
		3		
		4		_CTS		Clear to Send V24
		5		DO			Transmit Data V24
		6		
		7		PO			Transmit Data Printer

	*/

	newbrain_state *state = machine->driver_data;

	state->enrg1 = data;
}

static READ8_HANDLER( ust_r )
{
	/*

		bit		signal		description

		0		
		1		
		2					mains present
		3					user status
		4					user interrupt
		5					clock interrupt
		6					ACIA interrupt
		7		_COPINT		COP interrupt

	*/

	newbrain_state *state = machine->driver_data;

	UINT8 data = (state->copint << 7) | (state->aciaint << 6) | (state->clkint << 5) | (state->userint << 4) | 0x04;

	switch (state->enrg1 >> 6)
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

	switch (state->enrg1 >> 4)
	{
	case 0:
		// PWRUP, if set indicates that power is supplied to Z80 and memory
		if (!state->pwrup)
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

static READ8_HANDLER( a_ust_r )
{
	/*

		bit		signal		description

		0					+5V
		1		PWRUP
		2		
		3		
		4		
		5		_CLKINT		clock interrupt
		6					
		7		_COPINT		COP interrupt

	*/

	newbrain_state *state = machine->driver_data;

	return (state->copint << 7) | (state->clkint << 5) | (state->pwrup << 1) | 0x01;
}

static READ8_HANDLER( user_r )
{
	/*

		bit		signal		description

		0		RDDK		Received Data V24
		1		_CTSD		_Clear to Send V24
		2					
		3					
		4					
		5		TPIN		Tape in
		6					
		7		_CTSP		_Clear to Send Printer

	*/

	newbrain_state *state = machine->driver_data;

	state->user = 0;

	return 0xff;
}

static WRITE8_HANDLER( user_w )
{
	newbrain_state *state = machine->driver_data;

	state->user = data;
}

/* Interrupts */

static READ8_HANDLER( clclk_r )
{
	newbrain_state *state = machine->driver_data;

	state->clkint = 1;

	return 0xff;
}

static WRITE8_HANDLER( clclk_w )
{
	newbrain_state *state = machine->driver_data;

	state->clkint = 1;
}

static READ8_HANDLER( clusr_r )
{
	newbrain_state *state = machine->driver_data;

	state->userint = 1;

	return 0xff;
}

static WRITE8_HANDLER( clusr_w )
{
	newbrain_state *state = machine->driver_data;

	state->userint = 1;
}

/* COP420 */

static READ8_HANDLER( newbrain_cop_l_r )
{
	// connected to the Z80 data bus

	newbrain_state *state = machine->driver_data;

	return state->cop_bus;
}

static WRITE8_HANDLER( newbrain_cop_l_w )
{
	// connected to the Z80 data bus

	newbrain_state *state = machine->driver_data;

	state->cop_bus = data;
}

static WRITE8_HANDLER( newbrain_cop_g_w )
{
	/*

		bit		description
		
		G0		_COPINT
		G1		_TM1
		G2		not connected
		G3		_TM2

	*/

	// _Z80INT = ((_CLK | _CLKINT) & _COPINT)
	//cpunum_set_input_line(machine, 0, INPUT_LINE_IRQ0, BIT(data, 0) ? HOLD_LINE : CLEAR_LINE);

	newbrain_state *state = machine->driver_data;

	state->copint = BIT(data, 0);

	/* tape motor enable */

	cassette_change_state(cassette_device_image(0), BIT(data, 1) ? CASSETTE_MOTOR_DISABLED : CASSETTE_MOTOR_ENABLED, CASSETTE_MASK_MOTOR);
	cassette_change_state(cassette_device_image(1), BIT(data, 3) ? CASSETTE_MOTOR_DISABLED : CASSETTE_MOTOR_ENABLED, CASSETTE_MASK_MOTOR);
}

static READ8_HANDLER( newbrain_cop_g_r )
{
	/*

		bit		description
		
		G0		not connected
		G1		K9
		G2		K7
		G3		K3

	*/

	newbrain_state *state = machine->driver_data;

	return (BIT(state->keydata, 3) << 3) | (BIT(state->keydata, 0) << 2) | (BIT(state->keydata, 1) << 1);
}

static WRITE8_HANDLER( newbrain_cop_d_w )
{
	/*

		bit		description
		
		D0		inverted to K4 -> CD4024 pin 2 (reset)
		D1		TDO
		D2		inverted to K6 -> CD4024 pin 1 (clock), CD4076 pin 7 (clock), inverted to DS8881 pin 3 (enable)
		D3		not connected

	*/

	newbrain_state *state = machine->driver_data;
	static const char *keynames[] = { "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7",
										"D8", "D9", "D10", "D11", "D12", "D13", "D14", "D15" };

	/* keyboard row reset */

	if (!BIT(data, 0))
	{
		state->keylatch = 0;
	}

	/* tape data output */

	state->cop_tdo = BIT(data, 1);

	cassette_output(cassette_device_image(0), state->cop_tdo ? -1.0 : +1.0);
	cassette_output(cassette_device_image(1), state->cop_tdo ? -1.0 : +1.0);

	/* keyboard and display clock */

	if (!BIT(data, 2))
	{
		state->keylatch++;
		
		if (state->keylatch == 16)
		{
			state->keylatch = 0;
		}

		state->keydata = input_port_read(machine, keynames[state->keylatch]);

		output_set_digit_value(state->keylatch, state->segment_data[state->keylatch]);
	}
}

static READ8_HANDLER( newbrain_cop_in_r )
{
	/*

		bit		description
		
		IN0		K8
		IN1		_RD
		IN2		_COP
		IN3		_WR

	*/

	newbrain_state *state = machine->driver_data;

	return (state->cop_rd << 3) | (state->cop_access << 2) | (state->cop_rd << 1) | BIT(state->keydata, 2);
}

static WRITE8_HANDLER( newbrain_cop_so_w )
{
	// connected to K1

	newbrain_state *state = machine->driver_data;

	state->cop_so = data;
}

static WRITE8_HANDLER( newbrain_cop_sk_w )
{
	// connected to K2

	newbrain_state *state = machine->driver_data;

	state->segment_data[state->keylatch] >>= 1;
	state->segment_data[state->keylatch] = (state->cop_so << 15) | (state->segment_data[state->keylatch] & 0x7fff);
}

static READ8_HANDLER( newbrain_cop_si_r )
{
	// connected to TDI

	newbrain_state *state = machine->driver_data;

	state->cop_tdi = ((cassette_input(cassette_device_image(0)) > +1.0) || (cassette_input(cassette_device_image(1)) > +1.0)) ^ state->cop_tdo;

	return state->cop_tdi;
}

/* Paging */

static WRITE8_HANDLER( m_pr_w )
{
	/*

		bit		description

		0		memory module identifier
		1		memory module identifier
		2		memory module identifier
		3		memory module identifier
		4		memory module identifier
		5		memory module identifier
		6		memory module identifier
		7		ROM/RAM identifier

	*/

//	int page = (offset >> 13) & 0x07;
//	int bank = data & 0x7f;

	if (BIT(data, 7))
	{
		// RAM
	}
	else
	{
		// ROM
	}
}

/* Video */

static READ8_HANDLER( tvl_r )
{
	newbrain_state *state = machine->driver_data;

	if (offset)
	{
		state->tvram = 0xff;
	}
	else
	{
		state->tvram = 0x1ff;
	}

	return 0xff;
}

static WRITE8_HANDLER( tvl_w )
{
	newbrain_state *state = machine->driver_data;

	if (offset)
	{
		state->tvram = data;
	}
	else
	{
		state->tvram = 0x100 | data;
	}
}

static WRITE8_HANDLER( tvctl_w )
{
	/*

		bit		signal		description

		0		RV			1 reverses video over entire field, ie. black on white
		1		FS			0 generates 128 characters and 128 reverse field characters from 8 bit character code. 1 generates 256 characters from 8 bit character code
		2		32/_40		0 generates 320 or 640 horizontal dots in pixel graphics mode. 1 generates 256 or 512 horizontal dots in pixel graphics mode
		3		UCR			0 selects 256 characters expressed in an 8x10 matrix, and 25 lines (max) displayed. 1 selects 256 characters in an 8x8 matrix, and 31 lines (max) displayed
		4		
		5		
		6		80L			0 selects 40 character line length. 1 selects 80 character line length
		7		

	*/

	newbrain_state *state = machine->driver_data;

	state->tvctl = data;
}

/* Disc Controller */

static WRITE8_HANDLER( fdc_auxiliary_w )
{
	/*

		bit		description

		0		MOTON
		1		765 RESET
		2		TC
		3		
		4		
		5		PA15
		6		
		7		

	*/

	floppy_drive_set_motor_state(image_from_devtype_and_index(IO_FLOPPY, 0), BIT(data, 0));
	floppy_drive_set_ready_state(image_from_devtype_and_index(IO_FLOPPY, 0), 1, 0);

	nec765_set_reset_state(machine, BIT(data, 1));

	nec765_set_tc_state(machine, BIT(data, 2));
}

static READ8_HANDLER( fdc_control_r )
{
	/*

		bit		description

		0		
		1		
		2		
		3		
		4		
		5		FDC INT
		6		PAGING
		7		FDC ATT

	*/

	newbrain_state *state = machine->driver_data;

	return (state->fdc_att << 7) | (state->paging << 6) | (state->fdc_int << 5);
}

static READ8_HANDLER( ust2_r )
{
	/*

		bit		description

		0		RDDK (V24 RxD)
		1		_CTSD (V24 Clear to Send)
		2		
		3		
		4		
		5		TPIN
		6		
		7		_CTSP (Printer Clear to Send)

	*/

	return 0;
}

static READ8_HANDLER( cop_r )
{
	newbrain_state *state = machine->driver_data;
	UINT8 data;

	state->cop_rd = 0;
	state->cop_wr = 1;
	state->cop_access = 0;

	data = state->cop_bus;

	state->cop_rd = 1;
	state->cop_wr = 1;
	state->cop_access = 1;

	return data;
}

static WRITE8_HANDLER( cop_w )
{
	newbrain_state *state = machine->driver_data;

	state->cop_rd = 1;
	state->cop_wr = 0;
	state->cop_access = 0;

	state->cop_bus = data;

	state->cop_rd = 1;
	state->cop_wr = 1;
	state->cop_access = 1;
}

/* Expansion Interface Module */

static WRITE8_HANDLER( ei_enrg2_w )
{
	/*

		bit		signal		description

		0		_USERP		0 enables user data bus interrupt and also parallel latched data output (or centronics printer) interrupt
		1		ANP			1 enables ADC conversion complete interrupt and also calling indicator interrupt
		2		MLTMD		1 enables serial receive clock into multiplier input of DAC and signals data terminal not ready
		3		MSPD		1 enables 50K Baud serial data rate to be obtained ie. CTC input clock of 800KHz. 0 selects xxx.692KHz
		4		ENOR		1 enables serial receive clock to sound output summer, and also selects serial input from the printer port. 0 selects serial input from the comms port
		5		ANSW		1 enables second bank of 4 analogue inputs (voltage, non-ratiometric), ie. ch4-7, and enabled sound output, 0 selects ch03
		6		ENOT		1 enables serial transmit clock to sound ouput summer, and also selects serial output to the printer port. 0 selects serial output to the comms port
		7					9th output bit for centronics printer port

	*/

	newbrain_state *state = machine->driver_data;

	state->enrg2 = data;
}

static WRITE8_HANDLER( ei_pr_w )
{
//	newbrain_state *state = machine->driver_data;

//	int page = (BIT(offset, 12) >> 9) | (BIT(offset, 15) >> 13) | (BIT(offset, 14) >> 13) | (BIT(offset, 13) >> 13);
//	int bank = (BIT(offset, 11) >> 3) | (data & 0x7f);
}

static READ8_HANDLER( ei_user_r )
{
	newbrain_state *state = machine->driver_data;

	state->user = 0xff;

	return 0xff;
}

static WRITE8_HANDLER( ei_user_w )
{
	newbrain_state *state = machine->driver_data;

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
//	int channel = offset & 0x03;

	return 0;
}

static WRITE8_HANDLER( ei_anio_w )
{
//	int channel = offset & 0x03;
}

static READ8_HANDLER( ei_st0_r )
{
	/*

		bit		signal		description

		0					fixed at 1 - indicates excess of 24 or 48, obsolete
		1		PWRUP		1 indicates power up from 'cold' - necessary in battery machines with power switching
		2					1 indicates analogue or calling indicator interrupts
		3		_USRINT0	0 indicates centronics printer (latched output data) port interrupt
		4		_USRINT		0 indicates parallel data bus port interrupt
		5		_CLKINT		0 indicates frame frequency clock interrupt
		6		_ACINT		0 indicates ACIA interrupt
		7		_COPINT		0 indicates interrupt from micro-controller COP420M

	*/

	return acia_irq << 6;
}

static READ8_HANDLER( ei_st1_r )
{
	/*

		bit		signal		description

		0		
		1		
		2		N/_RV		1 selects normal video on power up (white on black), 0 selects reversed video (appears as D0 on the first 200 EI's)
		3		ANCH		1 indicates power is being taken from the mains supply
		4		40/_80		1 indicates that 40 column video is selected on power up. 0 selects 80 column video
		5		
		6		TVCNSL		1 indicates that a video display is required on power up
		7		

	*/

	newbrain_state *state = machine->driver_data;

	return (state->tvcnsl << 6);
}

static READ8_HANDLER( ei_st2_r )
{
	/*

		bit		signal		description

		0					received serial data from communications port
		1					0 indicates 'clear-to-send' condition at communications port
		2		
		3		
		4		
		5					logic level tape input
		6		
		7					0 indicates 'clear-to-send' condition at printer port

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
	newbrain_state *state = machine->driver_data;

	state->paging = data;

	if (BIT(offset, 8))
	{
		// expansion interface module

		/*

			bit		signal		description

			0		PG
			1		WPL
			2		A16
			3		_MPM
			4		HISLT
			5
			6
			7

		*/
	}
	else if (BIT(offset, 9))
	{
		// disc controller

		/*

			bit		signal		description

			0		PAGING
			1		
			2		HA16
			3		MPM
			4		
			5		_FDC RESET
			6		
			7		FDC ATT

		*/

		cpunum_set_input_line(machine, 2, INPUT_LINE_RESET, BIT(data, 5) ? HOLD_LINE : CLEAR_LINE);

		state->fdc_att = BIT(data, 7);
	}
	else if (BIT(offset, 10))
	{
		// network controller
	}
}

/* Memory Maps */

static ADDRESS_MAP_START( newbrain_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x1fff) AM_RAMBANK(1)
	AM_RANGE(0x2000, 0x3fff) AM_RAMBANK(2)
	AM_RANGE(0x4000, 0x5fff) AM_RAMBANK(3)
	AM_RANGE(0x6000, 0x7fff) AM_RAMBANK(4)
	AM_RANGE(0x8000, 0x9fff) AM_RAMBANK(5)
	AM_RANGE(0xa000, 0xbfff) AM_RAMBANK(6)
	AM_RANGE(0xc000, 0xdfff) AM_RAMBANK(7)
	AM_RANGE(0xe000, 0xffff) AM_RAMBANK(8)
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
	AM_RANGE(0x18, 0x18) AM_MIRROR(0xff00) AM_READWRITE(acia6850_0_stat_r, acia6850_0_ctrl_w)
	AM_RANGE(0x19, 0x19) AM_MIRROR(0xff00) AM_READWRITE(acia6850_0_data_r, acia6850_0_data_w)
	AM_RANGE(0x1c, 0x1f) AM_MIRROR(0xff00) AM_READWRITE(z80ctc_0_r, z80ctc_0_w)
	AM_RANGE(0xff, 0xff) AM_MIRROR(0xff00) AM_MASK(0xff00) AM_WRITE(ei_paging_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( newbrain_m_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00, 0x00) AM_MIRROR(0xffc0) AM_READWRITE(clusr_r, clusr_w)
	AM_RANGE(0x02, 0x02) AM_MIRROR(0xffc0) AM_MASK(0xff00) AM_WRITE(m_pr_w)
	AM_RANGE(0x03, 0x03) AM_MIRROR(0xffc0) AM_WRITE(user_w)
	AM_RANGE(0x04, 0x04) AM_MIRROR(0xffc0) AM_READWRITE(clclk_r, clclk_w)
	AM_RANGE(0x06, 0x06) AM_MIRROR(0xffc0) AM_READWRITE(cop_r, cop_w)
	AM_RANGE(0x07, 0x07) AM_MIRROR(0xffc0) AM_WRITE(a_enrg1_w)
	AM_RANGE(0x08, 0x09) AM_MIRROR(0xffc2) AM_READWRITE(tvl_r, tvl_w)
	AM_RANGE(0x0c, 0x0c) AM_MIRROR(0xffc3) AM_WRITE(tvctl_w)
	AM_RANGE(0x14, 0x14) AM_MIRROR(0xffc0) AM_READ(ust_r)
	AM_RANGE(0x15, 0x15) AM_MIRROR(0xffc2) AM_READ(user_r)
	AM_RANGE(0x16, 0x16) AM_MIRROR(0xffc0) AM_READ(ust2_r)
	AM_RANGE(0x18, 0x18) AM_MIRROR(0xffc0) AM_READWRITE(acia6850_0_stat_r, acia6850_0_ctrl_w)
	AM_RANGE(0x19, 0x19) AM_MIRROR(0xffc0) AM_READWRITE(acia6850_0_data_r, acia6850_0_data_w)
	AM_RANGE(0x1c, 0x1c) AM_MIRROR(0xffc0) AM_READWRITE(z80ctc_0_r, z80ctc_0_w)
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
	AM_RANGE(0x14, 0x14) AM_MIRROR(0xffc0) AM_READ(a_ust_r)
	AM_RANGE(0x16, 0x16) AM_MIRROR(0xffc0) AM_READ(ust2_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( newbrain_v_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00, 0x00) AM_MIRROR(0xffc0) AM_READWRITE(clusr_r, clusr_w)
	AM_RANGE(0x03, 0x03) AM_MIRROR(0xffc0) AM_WRITE(user_w)
	AM_RANGE(0x04, 0x04) AM_MIRROR(0xffc0) AM_READWRITE(clclk_r, clclk_w)
	AM_RANGE(0x06, 0x06) AM_MIRROR(0xffc0) AM_READWRITE(cop_r, cop_w)
	AM_RANGE(0x07, 0x07) AM_MIRROR(0xffc0) AM_WRITE(enrg1_w)
	AM_RANGE(0x08, 0x09) AM_MIRROR(0xffc2) AM_WRITE(tvl_w)
	AM_RANGE(0x0c, 0x0c) AM_MIRROR(0xffc3) AM_WRITE(tvctl_w)
	AM_RANGE(0x14, 0x14) AM_MIRROR(0xffc0) AM_READ(ust_r)
	AM_RANGE(0x15, 0x15) AM_MIRROR(0xffc2) AM_READ(user_r)
	AM_RANGE(0x18, 0x18) AM_MIRROR(0xffc0) AM_READWRITE(acia6850_0_stat_r, acia6850_0_ctrl_w)
	AM_RANGE(0x19, 0x19) AM_MIRROR(0xffc0) AM_READWRITE(acia6850_0_data_r, acia6850_0_data_w)
	AM_RANGE(0x1c, 0x1c) AM_MIRROR(0xffc0) AM_READWRITE(z80ctc_0_r, z80ctc_0_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( newbrain_cop_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x000, 0x3ff) AM_ROM
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
	AM_RANGE(0x00, 0x00) AM_READ(nec765_status_r)
	AM_RANGE(0x01, 0x01) AM_READWRITE(nec765_data_r, nec765_data_w)
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
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	
	PORT_START("D3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('N')

	PORT_START("D4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M')

	PORT_START("D5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('(') PORT_CHAR('[')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	
	PORT_START("D6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(')') PORT_CHAR(']')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	
	PORT_START("D7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CHAR(';') PORT_CHAR(':')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("* \xC2\xA3") PORT_CHAR('*') PORT_CHAR(0x00A3)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	
	PORT_START("D8")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("VIDEO TEXT") PORT_CODE(KEYCODE_RALT) PORT_CHAR(UCHAR_MAMEKEY(RALT)) // Vd
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	
	PORT_START("D9")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	
	PORT_START("D10")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('=') PORT_CHAR('@')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('X')

	PORT_START("D11")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('\\')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	
	PORT_START("D12")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CHAR('+') PORT_CHAR('^')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("INSERT") PORT_CODE(KEYCODE_INSERT) PORT_CHAR(UCHAR_MAMEKEY(INSERT))
	
	PORT_START("D13")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("NEW LINE") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13) // NL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	
	PORT_START("D14")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CHAR('/') PORT_CHAR('?')
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

static void acia_interrupt(int state)
{
	acia_irq = state;
}

static const struct acia6850_interface newbrain_acia_intf =
{
	500000, // ???
	500000, // ???
	&acia_rxd,
	&acia_txd,
	NULL,
	NULL,
	NULL,
	acia_interrupt
};

static void newbrain_fdc_interrupt(int state)
{
	newbrain_state *driver_state = Machine->driver_data;

	driver_state->fdc_int = state;
}

static const struct nec765_interface newbrain_nec765_interface =
{
	newbrain_fdc_interrupt,
	NULL
};

static z80ctc_interface newbrain_ctc_intf =
{
	1,						/* clock */
	0,              		/* timer disables */
	NULL,			  		/* interrupt handler */
	NULL,					/* ZC/TO0 callback */
	NULL,             		/* ZC/TO1 callback */
	NULL					/* ZC/TO2 callback */
};

static TIMER_CALLBACK( reset_tick )
{
	cpunum_set_input_line(machine, 0, INPUT_LINE_RESET, CLEAR_LINE);
	cpunum_set_input_line(machine, 1, INPUT_LINE_RESET, CLEAR_LINE);
}

static TIMER_CALLBACK( pwrup_tick )
{
	newbrain_state *state = machine->driver_data;
	int bank;

	state->pwrup = 0;

	for (bank = 1; bank < 9; bank++)
	{
		memory_configure_bank(bank, 0, 1, memory_region(machine, "main") + (bank - 1) * 0x2000, 0);
		memory_set_bank(bank, 0);
	}
}

INLINE int get_reset_t(void)
{
	return RES_K(220) * CAP_U(10) * 1000; // t = R128 * C125
}

INLINE int get_pwrup_t(void)
{
	return RES_K(560) * CAP_U(10) * 1000; // t = R129 * C127
}

static MACHINE_START( newbrain )
{
	newbrain_state *state = machine->driver_data;
	int bank;

	/* memory banking */

	for (bank = 1; bank < 9; bank++)
	{
		memory_configure_bank(bank, 0, 1, memory_region(machine, "main") + 0xe000, 0);
		memory_set_bank(bank, 0);
	}

	/* initialize devices */

	nec765_init(machine, &newbrain_nec765_interface, NEC765A, NEC765_RDY_PIN_NOT_CONNECTED);
	acia6850_config(0, &newbrain_acia_intf);
	z80ctc_init(0, &newbrain_ctc_intf);

	/* allocate reset timer */
	
	state->reset_timer = timer_alloc(reset_tick, NULL);

	/* allocate power up timer */
	
	state->pwrup_timer = timer_alloc(pwrup_tick, NULL);
	timer_adjust_oneshot(state->reset_timer, ATTOTIME_IN_MSEC(get_pwrup_t()), 0);
	state->pwrup = 1;

	/* initialize variables */

	state->userint = 1;
	state->clkint = 1;
	state->aciaint = 1;
	state->copint = 1;
	state->bee = 1;
	state->tvcnsl = 1;

	/* register for state saving */

}

static MACHINE_RESET( newbrain )
{
	newbrain_state *state = machine->driver_data;

	cpunum_set_input_line(machine, 0, INPUT_LINE_RESET, HOLD_LINE);
	cpunum_set_input_line(machine, 1, INPUT_LINE_RESET, HOLD_LINE);

	timer_adjust_oneshot(state->reset_timer, ATTOTIME_IN_MSEC(get_reset_t()), 0);
}

static const struct z80_irq_daisy_chain newbrain_daisy_chain[] =
{
	{ z80ctc_reset, z80ctc_irq_state, z80ctc_irq_ack, z80ctc_irq_reti, 0 },
	{ 0, 0, 0, 0, -1 }
};

static INTERRUPT_GEN( newbrain_interrupt )
{
	newbrain_state *state = machine->driver_data;
	
	if (state->enrg1 & NEWBRAIN_ENRG1_CLK)
	{
		cpunum_set_input_line(machine, 0, INPUT_LINE_IRQ0, HOLD_LINE);

		state->clkint = 0;
	}
}

static COP400_INTERFACE( newbrain_cop_intf )
{
	COP400_CKI_DIVISOR_16, // ???
	COP400_CKO_OSCILLATOR_OUTPUT,
	COP400_MICROBUS_ENABLED
};

/* Machine Drivers */

static MACHINE_DRIVER_START( newbrain )
	MDRV_DRIVER_DATA(newbrain_state)

	// basic system hardware

	MDRV_CPU_ADD("main", Z80, XTAL_16MHz/8)
	MDRV_CPU_PROGRAM_MAP(newbrain_map, 0)
	MDRV_CPU_IO_MAP(newbrain_ei_io_map, 0)
	MDRV_CPU_VBLANK_INT("main", newbrain_interrupt)

	MDRV_CPU_ADD("cop", COP420, XTAL_16MHz/8) // COP420-GUW/M
	MDRV_CPU_PROGRAM_MAP(newbrain_cop_map, 0)
	MDRV_CPU_IO_MAP(newbrain_cop_io_map, 0)
	MDRV_CPU_CONFIG(newbrain_cop_intf)

	MDRV_CPU_ADD("fdc", Z80, XTAL_4MHz)
	MDRV_CPU_PROGRAM_MAP(newbrain_fdc_map, 0)
	MDRV_CPU_IO_MAP(newbrain_fdc_io_map, 0)

	MDRV_MACHINE_START(newbrain)
	MDRV_MACHINE_RESET(newbrain)

	// video hardware

	MDRV_IMPORT_FROM(newbrain_video)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( newbraim )
	MDRV_DRIVER_DATA(newbrain_state)

	// basic system hardware

	MDRV_CPU_ADD("main", Z80, XTAL_16MHz/8)
	MDRV_CPU_PROGRAM_MAP(newbrain_map, 0)
	MDRV_CPU_IO_MAP(newbrain_m_io_map, 0)
	MDRV_CPU_VBLANK_INT("main", newbrain_interrupt)

	MDRV_CPU_ADD("cop", COP420, XTAL_16MHz/8) // COP420-GUW/M
	MDRV_CPU_PROGRAM_MAP(newbrain_cop_map, 0)
	MDRV_CPU_IO_MAP(newbrain_cop_io_map, 0)
	MDRV_CPU_CONFIG(newbrain_cop_intf)

	MDRV_MACHINE_START(newbrain)
	MDRV_MACHINE_RESET(newbrain)

	// video hardware

	MDRV_IMPORT_FROM(newbrain_video)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( newbraia )
	MDRV_DRIVER_DATA(newbrain_state)

	// basic system hardware

	MDRV_CPU_ADD("main", Z80, XTAL_16MHz/8)
	MDRV_CPU_PROGRAM_MAP(newbrain_map, 0)
	MDRV_CPU_IO_MAP(newbrain_a_io_map, 0)
	MDRV_CPU_VBLANK_INT("main", newbrain_interrupt)

	MDRV_CPU_ADD("cop", COP420, XTAL_16MHz/8) // COP420-GUW/M
	MDRV_CPU_PROGRAM_MAP(newbrain_cop_map, 0)
	MDRV_CPU_IO_MAP(newbrain_cop_io_map, 0)
	MDRV_CPU_CONFIG(newbrain_cop_intf)

	MDRV_MACHINE_START(newbrain)
	MDRV_MACHINE_RESET(newbrain)

	// video hardware

	MDRV_IMPORT_FROM(newbrain_video)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( newbraiv )
	MDRV_DRIVER_DATA(newbrain_state)

	// basic system hardware

	MDRV_CPU_ADD("main", Z80, XTAL_16MHz/8)
	MDRV_CPU_PROGRAM_MAP(newbrain_map, 0)
	MDRV_CPU_IO_MAP(newbrain_v_io_map, 0)
	MDRV_CPU_VBLANK_INT("main", newbrain_interrupt)

	MDRV_CPU_ADD("cop", COP420, XTAL_16MHz/8) // COP420-GUW/M
	MDRV_CPU_PROGRAM_MAP(newbrain_cop_map, 0)
	MDRV_CPU_IO_MAP(newbrain_cop_io_map, 0)
	MDRV_CPU_CONFIG(newbrain_cop_intf)

	MDRV_MACHINE_START(newbrain)
	MDRV_MACHINE_RESET(newbrain)

	// video hardware

	MDRV_IMPORT_FROM(newbrain_video)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( newbrain )
	ROM_REGION( 0x10000, "main", 0 )
	ROM_LOAD( "d413-2.rom", 0x8000, 0x2000, CRC(097591f1) SHA1(c2aa1d27d4f3a24ab0c8135df746a4a44201a7f4) )
	ROM_LOAD( "cdmd.rom", 0xc000, 0x2000, CRC(6b4d9429) SHA1(ef688be4e75aced61f487c928258c8932a0ae00a) )
	ROM_LOAD( "efmd.rom", 0xe000, 0x2000, CRC(20dd0b49) SHA1(74b517ca223cefb588e9f49e72ff2d4f1627efc6) )

	ROM_REGION( 0x400, "cop", 0 )
	ROM_LOAD( "cop420.419", 0x000, 0x400, NO_DUMP )

	ROM_REGION( 0x10000, "eim", 0 ) // Expansion Interface Module
	ROM_LOAD( "e415-2.rom", 0x4000, 0x2000, CRC(5b0e390c) SHA1(0f99cae57af2e64f3f6b02e5325138d6ba015e72) )
	ROM_LOAD( "e415-3.rom", 0x4000, 0x2000, CRC(2f88bae5) SHA1(04e03f230f4b368027442a7c2084dae877f53713) )
	ROM_LOAD( "e416-3.rom", 0x6000, 0x2000, CRC(8b5099d8) SHA1(19b0cfce4c8b220eb1648b467f94113bafcb14e0) ) // 10/8/83.mtv
	ROM_LOAD( "e417-2.rom", 0x8000, 0x2000, CRC(6a7afa20) SHA1(f90db4f8318777313a862b3d5bab83c2fd260010) )

	ROM_REGION( 0x10000, "fdc", 0 ) // Floppy Disk Controller
	ROM_LOAD( "d417-1.rom", 0x0000, 0x2000, CRC(40fad31c) SHA1(5137be4cc026972c0ffd4fa6990e8583bdfce163) )
	ROM_LOAD( "d417-2.rom", 0x0000, 0x2000, CRC(e8bda8b9) SHA1(c85a76a5ff7054f4ef4a472ce99ebaed1abd269c) )

	ROM_REGION( 0x1000, "chargen", 0 )
	ROM_LOAD( "8248r7.453", 0x0000, 0x1000, NO_DUMP )
ROM_END

ROM_START( newbraim )
	ROM_REGION( 0x10000, "main", 0 )
	ROM_LOAD( "cdmd.rom", 0xc000, 0x2000, CRC(6b4d9429) SHA1(ef688be4e75aced61f487c928258c8932a0ae00a) )
	ROM_LOAD( "efmd.rom", 0xe000, 0x2000, CRC(20dd0b49) SHA1(74b517ca223cefb588e9f49e72ff2d4f1627efc6) )

	ROM_REGION( 0x400, "cop", 0 )
	ROM_LOAD( "cop420.419", 0x000, 0x400, NO_DUMP )

	ROM_REGION( 0x1000, "chargen", 0 )
	ROM_LOAD( "8248r7.453", 0x0000, 0x1000, NO_DUMP )
ROM_END

ROM_START( newbraia )
	ROM_SYSTEM_BIOS( 0, "rom20", "ROM 2.0" )
	ROMX_LOAD( "aben20.rom", 0xa000, 0x2000, CRC(3d76d0c8) SHA1(753b4530a518ad832e4b81c4e5430355ba3f62e0), ROM_BIOS(1) )
	ROMX_LOAD( "cd20tci.rom", 0xc000, 0x4000, CRC(f65b2350) SHA1(1ada7fbf207809537ec1ffb69808524300622ada), ROM_BIOS(1) )
	
	ROM_SYSTEM_BIOS( 1, "rom191", "ROM 1.91" )
	ROMX_LOAD( "aben191.rom", 0xa000, 0x2000, CRC(b7be8d89) SHA1(cce8d0ae7aa40245907ea38b7956c62d039d45b7), ROM_BIOS(2) )
	ROMX_LOAD( "cd.rom", 0xc000, 0x2000, CRC(6b4d9429) SHA1(ef688be4e75aced61f487c928258c8932a0ae00a), ROM_BIOS(2) )
	ROMX_LOAD( "ef1x.rom", 0xe000, 0x2000, CRC(20dd0b49) SHA1(74b517ca223cefb588e9f49e72ff2d4f1627efc6), ROM_BIOS(2) )
	
	ROM_SYSTEM_BIOS( 2, "rom19", "ROM 1.9" )
	ROMX_LOAD( "aben19.rom", 0xa000, 0x2000, CRC(d0283eb1) SHA1(351d248e69a77fa552c2584049006911fb381ff0), ROM_BIOS(3) )
	ROMX_LOAD( "cd.rom", 0xc000, 0x2000, CRC(6b4d9429) SHA1(ef688be4e75aced61f487c928258c8932a0ae00a), ROM_BIOS(3) )
	ROMX_LOAD( "ef1x.rom", 0xe000, 0x2000, CRC(20dd0b49) SHA1(74b517ca223cefb588e9f49e72ff2d4f1627efc6), ROM_BIOS(3) )
	
	ROM_SYSTEM_BIOS( 3, "rom14", "ROM 1.4" )
	ROMX_LOAD( "aben14.rom", 0xa000, 0x2000, CRC(d0283eb1) SHA1(351d248e69a77fa552c2584049006911fb381ff0), ROM_BIOS(4) )
	ROMX_LOAD( "cd.rom", 0xc000, 0x2000, CRC(6b4d9429) SHA1(ef688be4e75aced61f487c928258c8932a0ae00a), ROM_BIOS(4) )
	ROMX_LOAD( "ef1x.rom", 0xe000, 0x2000, CRC(20dd0b49) SHA1(74b517ca223cefb588e9f49e72ff2d4f1627efc6), ROM_BIOS(4) )

	ROM_REGION( 0x400, "cop", 0 )
	ROM_LOAD( "cop420.419", 0x000, 0x400, NO_DUMP )

	ROM_REGION( 0x1000, "chargen", 0 )
	ROM_LOAD( "8248r7.453", 0x0000, 0x1000, NO_DUMP )
ROM_END

#define rom_newbraiv rom_newbraia

/* System Configuration */

static void newbrain_cassette_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:					info->i = 2; break;
		case MESS_DEVINFO_INT_CASSETTE_DEFAULT_STATE:	info->i = CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_MUTED; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}

static DEVICE_IMAGE_LOAD( newbrain_floppy )
{
	if (image_has_been_created(image))
		return INIT_FAIL;

	if (device_load_basicdsk_floppy(image) == INIT_PASS)
	{
		// 180K
	}

	return INIT_PASS;
}

static void newbrain_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:					info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:						info->load = DEVICE_IMAGE_LOAD_NAME(newbrain_floppy); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:			strcpy(info->s = device_temp_str(), "img"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}

static DEVICE_IMAGE_LOAD( newbrain_serial )
{
	/* filename specified */
	if (device_load_serial_device(image)==INIT_PASS)
	{
		/* setup transmit parameters */
		serial_device_setup(image, 9600, 8, 1, SERIAL_PARITY_NONE);

		serial_device_set_protocol(image, SERIAL_PROTOCOL_NONE);

		/* and start transmit */
		serial_device_set_transmit_state(image, 1);

		return INIT_PASS;
	}

	return INIT_FAIL;
}

static void newbrain_serial_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* serial */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_TYPE:							info->i = IO_SERIAL; break;
		case MESS_DEVINFO_INT_COUNT:						info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_START:						info->start = DEVICE_START_NAME(serial_device); break;
		case MESS_DEVINFO_PTR_LOAD:							info->load = DEVICE_IMAGE_LOAD_NAME(newbrain_serial); break;
		case MESS_DEVINFO_PTR_UNLOAD:						info->unload = DEVICE_IMAGE_UNLOAD_NAME(serial_device); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "txt"); break;
	}
}

static SYSTEM_CONFIG_START( newbrain )
	CONFIG_RAM_DEFAULT	(32 * 1024)
	CONFIG_DEVICE(newbrain_cassette_getinfo)
	CONFIG_DEVICE(newbrain_floppy_getinfo)
	CONFIG_DEVICE(newbrain_serial_getinfo)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START( newbraim )
	CONFIG_RAM_DEFAULT	(32 * 1024)
	CONFIG_DEVICE(newbrain_cassette_getinfo)
	CONFIG_DEVICE(newbrain_serial_getinfo)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START( newbraia )
	CONFIG_RAM_DEFAULT	(32 * 1024)
	CONFIG_DEVICE(newbrain_cassette_getinfo)
	CONFIG_DEVICE(newbrain_serial_getinfo)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START( newbraiv )
	CONFIG_RAM_DEFAULT	(32 * 1024)
	CONFIG_DEVICE(newbrain_serial_getinfo)
SYSTEM_CONFIG_END

/* System Drivers */

//    YEAR  NAME		PARENT		COMPAT	MACHINE		INPUT		INIT	CONFIG		COMPANY							FULLNAME		FLAGS
COMP( 1981, newbrain,	0,			0,		newbrain,	newbrain,   0, 		newbrain,	"Grundy Business Systems Ltd.",	"NewBrain MD with Expansion Interface",	GAME_NOT_WORKING | GAME_NO_SOUND )
COMP( 1981, newbraim,	0,			0,		newbraim,	newbrain,   0, 		newbraim,	"Grundy Business Systems Ltd.",	"NewBrain MD",	GAME_NOT_WORKING | GAME_NO_SOUND )
COMP( 1981, newbraia,	newbrain,	0,		newbraia,	newbrain,   0, 		newbraia,	"Grundy Business Systems Ltd.",	"NewBrain AD",	GAME_NOT_WORKING | GAME_NO_SOUND )
COMP( 1981, newbraiv,	newbrain,	0,		newbraiv,	newbrain,   0, 		newbraiv,	"Grundy Business Systems Ltd.",	"NewBrain VD",	GAME_NOT_WORKING | GAME_NO_SOUND )
