/******************************************************************************

  Exidy Sorcerer system driver

    The UART controls rs232 and cassette i/o. The chip is a AY-3-1014A or AY-3-1015.


    port fc:
    ========
    input/output:
        hd6402 uart data

    port fd:
    ========
    input: hd6402 uart status
        bit 4: parity error (RPE)
        bit 3: framing error (RFE)
        bit 2: over-run (RDP)
        bit 1: data available (RDA)
        bit 0: transmit buffer empty (TPMT)

    output:
        bit 4: no parity (NPB)
        bit 3: parity type (POE)
        bit 2: number of stop bits (NSB)
        bit 1: number of bits per char bit 2 (NDB2)
        bit 0: number of bits per char bit 1 (NDB1)

    port fe:
    ========

    output:

        bit 7: rs232 enable (1=rs232, 0=cassette)
        bit 6: baud rate (1=1200, 0=300)
        bit 5: cassette motor 2
        bit 4: cassette motor 1
        bit 3..0: keyboard line select

    input:
        bit 7..6: parallel control (not emulated)
                7: must be 1 to read data from parallel port via PARIN
                6: must be 1 to send data out of parallel port via PAROUT
        bit 5: vsync
        bit 4..0: keyboard line data

    port ff:
    ========
      parallel port in/out

    -------------------------------------------------------------------------------------

    When cassette is selected, it is connected to the uart data input via the cassette
    interface hardware.

    The cassette interface hardware converts square-wave pulses into bits which the uart receives.


    Sound:

    external speaker connected to the parallel port
    speaker is connected to all pins. All pins need to be toggled at the same time.


    Kevin Thacker [MESS driver]

 ******************************************************************************

    The Sorcerer comes with a non-standard Serial Port with connections for
    rs232 and the 2 cassette players. The connections for cassette 1 are duplicated
    on a set of phono plugs.

    The CPU clock speed is 2.106mhz, which was increased to 4.0mhz on the last production runs.

    The Sorcerer has a bus connection for S100 equipment. This allows the connection
    of disk drives, provided that suitable driver/boot software is loaded.

    The driver "exidy" emulates a Sorcerer with 4 floppy disk drives fitted, and 32k of ram.

    The driver "exidyd" emulates the most common form, a cassette-based system with 48k of ram.


********************************************************************************/
#include "driver.h"
#include "includes/exidy.h"
#include "machine/centroni.h"
#include "machine/hd6402.h"
#include "cpu/z80/z80.h"
#include "machine/wd17xx.h"
#include "devices/basicdsk.h"
#include "devices/cassette.h"
#include "devices/snapquik.h"
#include "devices/cartslot.h"
#include "devices/printer.h"
#include "devices/z80bin.h"
#include "sound/speaker.h"


static DEVICE_IMAGE_LOAD( exidy_floppy )
{
	if (device_load_basicdsk_floppy(image)==INIT_PASS)
	{
		/* not correct */
		basicdsk_set_geometry(image, 80, 2, 9, 512, 1, 0, FALSE);
		return INIT_PASS;
	}

	return INIT_FAIL;
}


static unsigned char exidy_fe;
static int exidy_keyboard_line;
static unsigned long exidy_hd6402_state;

static WRITE8_HANDLER(exidy_fe_port_w);

/* timer for exidy serial chip transmit and receive */
static emu_timer *serial_timer;

static TIMER_CALLBACK(exidy_serial_timer_callback)
{
	/* if rs232 is enabled, uart is connected to clock defined by bit6 of port fe.
    Transmit and receive clocks are connected to the same clock */

	/* if rs232 is disabled, receive clock is linked to cassette hardware */
	if (exidy_fe & 0x080)
	{
		/* trigger transmit clock on hd6402 */
		hd6402_transmit_clock();
		/* trigger receive clock on hd6402 */
		hd6402_receive_clock();
	}
}



/* timer to read cassette waveforms */
static void *cassette_timer;

/* cassette data is converted into bits which are clocked into serial chip */
static struct serial_connection cassette_serial_connection;

/* two flip-flops used to store state of bit coming from cassette */
static int cassette_input_ff[2];
/* state of clock, used to clock in bits */
static int cassette_clock_state;
/* a up-counter. when this reaches a fixed value, the cassette clock state is changed */
static int cassette_clock_counter;

/*  1. the cassette format: "frequency shift" is converted
    into the uart data format "non-return to zero"

    2. on cassette a 1 data bit is stored as a high frequency
    and a 0 data bit as a low frequency
    - At 1200 baud, a logic 1 is 1 cycle of 1200hz and a logic 0 is 1/2 cycle of 600hz.
    - At 300 baud, a logic 1 is 8 cycles of 2400hz and a logic 0 is 4 cycles of 1200hz.

    Attenuation is applied to the signal and the square wave edges are rounded.

    A manchester encoder is used. A flip-flop synchronises input
    data on the positive-edge of the clock pulse.

    Interestingly the data on cassette is stored in xmodem-checksum.


*/

/* only works for one cassette so far */
static TIMER_CALLBACK(exidy_cassette_timer_callback)
{
	cassette_clock_counter++;

	if (cassette_clock_counter==(4800/1200))
	{
		/* reset counter */
		cassette_clock_counter = 0;

		/* toggle state of clock */
		cassette_clock_state^=0x0ffffffff;

		/* previously was 0, now gone 1 */
		/* +ve edge detected */
		if (cassette_clock_state)
		{
			int bit;

			/* clock bits into cassette flip flops */
			/* bit is inverted! */

			/* detect level */
			bit = 1;
			if (cassette_input(image_from_devtype_and_index(IO_CASSETTE, 0)) > 0.0038)
				bit = 0;
			cassette_input_ff[0] = bit;
			/* detect level */
			bit = 1;
			if (cassette_input(image_from_devtype_and_index(IO_CASSETTE, 1)) > 0.0038)
				bit = 0;

			cassette_input_ff[1] = bit;

			logerror("cassette bit: %0d\n",bit);

			/* set out data bit */
			set_out_data_bit(cassette_serial_connection.State, cassette_input_ff[0]);
			/* output through serial connection */
			serial_connection_out(&cassette_serial_connection);

			/* update hd6402 receive clock */
			hd6402_receive_clock();
		}
	}
}

static void exidy_hd6402_callback(int mask, int data)
{
	exidy_hd6402_state &=~mask;
	exidy_hd6402_state |= (data & mask);

	logerror("hd6402 state: %04x %04x\n",mask,data);
}

/* after the first 4 bytes have been read from ROM, switch the ram back in */
static TIMER_CALLBACK( exidy_reset )
{
	memory_set_bank(1, 0);
}

static void exidy_printer_handshake_in(int number, int data, int mask)
{
	if (mask & CENTRONICS_ACKNOWLEDGE)
	{
		if (data & CENTRONICS_ACKNOWLEDGE)
		{
		}
	}
}

static const CENTRONICS_CONFIG exidy_cent_config[1]={
	{
		PRINTER_CENTRONICS,
		exidy_printer_handshake_in
	},
};

static void cassette_serial_in(int id, unsigned long state)
{
	cassette_serial_connection.input_state = state;
}

static MACHINE_START( exidyd )
{
	serial_timer = timer_alloc(exidy_serial_timer_callback, NULL);
	cassette_timer = timer_alloc(exidy_cassette_timer_callback, NULL);
}

static MACHINE_START( exidy )
{
	MACHINE_START_CALL( exidyd );
	wd17xx_init(machine, WD_TYPE_179X, NULL, NULL);
}

static MACHINE_RESET( exidyd )
{
	hd6402_init();
	hd6402_set_callback(exidy_hd6402_callback);
	hd6402_reset();

	centronics_config(0, exidy_cent_config);
	/* assumption: select is tied low */
	centronics_write_handshake(0, CENTRONICS_SELECT | CENTRONICS_NO_RESET, CENTRONICS_SELECT| CENTRONICS_NO_RESET);

	serial_connection_init(&cassette_serial_connection);
	serial_connection_set_in_callback(&cassette_serial_connection, cassette_serial_in);

	exidy_fe_port_w(machine, 0, 0);

	timer_set(ATTOTIME_IN_USEC(10), NULL, 0, exidy_reset);
	memory_set_bank(1, 1);
}

static MACHINE_RESET( exidy )
{
	floppy_drive_set_geometry(image_from_devtype_and_index(IO_FLOPPY, 0), FLOPPY_DRIVE_DS_80);
	MACHINE_RESET_CALL( exidyd );
}


static  READ8_HANDLER ( exidy_wd179x_r )
{
	switch (offset & 0x03)
	{
	case 0:
		return wd17xx_status_r(machine, offset);
	case 1:
		return wd17xx_track_r(machine, offset);
	case 2:
		return wd17xx_sector_r(machine, offset);
	case 3:
		return wd17xx_data_r(machine, offset);
	default:
		return 0xff;
	}
}

static WRITE8_HANDLER ( exidy_wd179x_w )
{
	switch (offset & 0x03)
	{
	case 0:
		wd17xx_command_w(machine, offset, data);
		return;
	case 1:
		wd17xx_track_w(machine, offset, data);
		return;
	case 2:
		wd17xx_sector_w(machine, offset, data);
		return;
	case 3:
		wd17xx_data_w(machine, offset, data);
		return;
	default:
		break;
	}
}


static WRITE8_HANDLER(exidy_fc_port_w)
{
	logerror("exidy fc w: %04x %02x\n",offset,data);

	hd6402_set_input(HD6402_INPUT_TBRL, HD6402_INPUT_TBRL);
	hd6402_data_w(machine, offset, data);
}


static WRITE8_HANDLER(exidy_fd_port_w)
{
	logerror("exidy fd w: %04x %02x\n",offset,data);

	/* bit 0,1: char length select */
	if (data & 1)
		hd6402_set_input(HD6402_INPUT_CLS1, HD6402_INPUT_CLS1);
	else
		hd6402_set_input(HD6402_INPUT_CLS1, 0);

	if (data & 2)
		hd6402_set_input(HD6402_INPUT_CLS2, HD6402_INPUT_CLS2);
	else
		hd6402_set_input(HD6402_INPUT_CLS2, 0);

	/* bit 2: stop bit count */
	if (data & 4)
		hd6402_set_input(HD6402_INPUT_SBS, HD6402_INPUT_SBS);
	else
		hd6402_set_input(HD6402_INPUT_SBS, 0);

	/* bit 3: parity type select */
	if (data & 8)
		hd6402_set_input(HD6402_INPUT_EPE, HD6402_INPUT_EPE);
	else
		hd6402_set_input(HD6402_INPUT_EPE, 0);

	/* bit 4: inhibit parity (no parity) */
	if (data & 16)
		hd6402_set_input(HD6402_INPUT_PI, HD6402_INPUT_PI);
	else
		hd6402_set_input(HD6402_INPUT_PI, 0);
}

#define EXIDY_CASSETTE_MOTOR_MASK ((1<<4)|(1<<5))


static WRITE8_HANDLER(exidy_fe_port_w)
{
	int changed_bits;
	/* bits 0..3 */
	exidy_keyboard_line = data & 0x0f;

	changed_bits = exidy_fe^data;
	/* bits 4..5 */
	/* either cassette motor state changed */
	if ((changed_bits & EXIDY_CASSETTE_MOTOR_MASK)!=0)
	{
		/* cassette 1 motor */
		cassette_change_state(image_from_devtype_and_index(IO_CASSETTE, 0),
			(data & 0x10) ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED,
			CASSETTE_MASK_MOTOR);

		/* cassette 2 motor */
		cassette_change_state(image_from_devtype_and_index(IO_CASSETTE, 1),
			(data & 0x20) ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED,
			CASSETTE_MASK_MOTOR);

		if ((data & EXIDY_CASSETTE_MOTOR_MASK)==0)
		{
			/* both are now off */

			/* stop timer */
			timer_adjust_oneshot(cassette_timer, attotime_zero, 0);
		}
		else
		{
			/* if both motors were off previously, at least one motor has been switched on */
			if ((exidy_fe & EXIDY_CASSETTE_MOTOR_MASK)==0)
			{
				cassette_clock_counter = 0;
				cassette_clock_state = 0;
				/* start timer */
				/* the correct baud rate should be being used here (see bit 6 below) */
				timer_adjust_periodic(cassette_timer, attotime_zero, 0, ATTOTIME_IN_HZ(4800));
			}
		}
	}
	/* bit 7 */
	if (data & 0x080)
	{
		/* connect to serial device (not yet emulated) */
	/* Due to bugs in the hardware and software of a real Sorcerer, the serial
	interface misbehaves.
	1. Sorcerer I had a hardware problem causing rs232 idle to be a space (+9v)
	instead of mark (-9v). Fixed in Sorcerer II.
	2. When you select a different baud for rs232, it was "remembered" but not
	sent to port fe. It only gets sent when motor on was requested. Motor on is
	only meaningful in a cassette operation.
	3. The monitor software always resets the device to cassette whenever the
	keyboard is scanned, motors altered, or an error occurred.
	4. The above problems make rs232 communication impractical unless you write
	your own routines or create a corrected monitor rom. */
	}
	else
	{
		/* connect to tape */
		hd6402_connect(&cassette_serial_connection);
	}
	/* bit 6 */
	if ((changed_bits & (1<<6))!=0)
	{
		int baud_rate;

		baud_rate = 300;
		if (data & (1<<6))
		{
			baud_rate = 1200;
		}

		timer_adjust_periodic(serial_timer, attotime_zero, 0, ATTOTIME_IN_HZ(baud_rate));
	}

	exidy_fe = data;
}

static WRITE8_HANDLER(exidy_ff_port_w)
{
	/* reading the config switch */
	switch ((input_port_read(machine, "CONFIG")>>1) & 0x01)
	{
		case 0: /* speaker */
			speaker_level_w(0, (data) ? 1 : 0);
			break;

		case 1: /* printer */
			/* bit 7 = strobe, bit 6..0 = data */
			centronics_write_handshake(0, CENTRONICS_SELECT | CENTRONICS_NO_RESET, CENTRONICS_SELECT| CENTRONICS_NO_RESET);
			centronics_write_handshake(0, (data>>7) & 0x01, CENTRONICS_STROBE);
			centronics_write_data(0, data & 0x7f);
			break;
	}
}

static READ8_HANDLER(exidy_fc_port_r)
{
	UINT8 data;

	hd6402_set_input(HD6402_INPUT_DRR, HD6402_INPUT_DRR);
	data = hd6402_data_r(machine, offset);

	logerror("exidy fc r: %04x %02x\n",offset,data);

	return data;
}

static READ8_HANDLER(exidy_fd_port_r)
{
	/* set unused bits high */
	UINT8 data = 0xe0;

	/* bit 4: parity error */
	if (exidy_hd6402_state & HD6402_OUTPUT_PE) data |= 16;

	/* bit 3: framing error */
	if (exidy_hd6402_state & HD6402_OUTPUT_FE) data |= 8;

	/* bit 2: over-run error */
	if (exidy_hd6402_state & HD6402_OUTPUT_OE) data |= 4;

	/* bit 1: data receive ready - data ready in receive reg */
	if (exidy_hd6402_state & HD6402_OUTPUT_DR) data |= 2;

	/* bit 0: transmitter buffer receive empty */
	/* if this BIT is forced high, then the cassette save routine does not hang */
	if (exidy_hd6402_state & HD6402_OUTPUT_TBRE) data |= 1;

	logerror("exidy fd r: %04x %02x\n",offset,data);

	return data;
}

static  READ8_HANDLER(exidy_fe_port_r)
{
	/* bits 6..7
     - hardware handshakes from user port
     - not emulated
     - tied high, allowing PARIN and PAROUT bios routines to run */

	UINT8 data=0xc0;
	char port[7];

	/* bit 5 - vsync (inverted) */
	data |= (((~input_port_read(machine, "VS")) & 0x01)<<5);

	/* bits 4..0 - keyboard data */
	sprintf(port, "LINE%d", exidy_keyboard_line);
	data |= (input_port_read(machine, port) & 0x01f);

	return data;
}

static const device_config *printer_device(running_machine *machine)
{
	return device_list_find_by_tag(machine->config->devicelist, PRINTER, "printer");
}

static READ8_HANDLER(exidy_ff_port_r)
{
	/* The use of the parallel port as a general purpose port is not emulated.
	Currently the only use is to read the printer status in the Centronics CENDRV bios routine.
	This uses bit 7. The other bits have been set high (=nothing plugged in).
	This fixes those games that use a joystick. */

	UINT8 data=0x7f;

	/* bit 7 = printer busy
	0 = printer is not busy */

	if (printer_is_ready(printer_device(machine))==0 )
		data |= 0x080;

	return data;
}


static READ8_HANDLER( exidy_read_ff ) { return 0xff; }

static ADDRESS_MAP_START( exidy_mem , ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x07ff) AM_RAMBANK(1)
	AM_RANGE(0x0800, 0x7fff) AM_RAM		/* ram 32k machine */
	AM_RANGE(0x8000, 0xbbff) AM_READWRITE(exidy_read_ff, SMH_NOP)
	AM_RANGE(0xbc00, 0xbcff) AM_ROM		/* disk bios */
	AM_RANGE(0xbd00, 0xbdff) AM_READWRITE(exidy_read_ff, SMH_NOP)
	AM_RANGE(0xbe00, 0xbe03) AM_READWRITE(exidy_wd179x_r, exidy_wd179x_w)
	AM_RANGE(0xbe04, 0xbfff) AM_READWRITE(exidy_read_ff, SMH_NOP)
	AM_RANGE(0xc000, 0xefff) AM_ROM		/* rom pac and bios */
	AM_RANGE(0xf000, 0xf7ff) AM_RAM		/* screen ram */
	AM_RANGE(0xf800, 0xfbff) AM_ROM		/* char rom */
	AM_RANGE(0xfc00, 0xffff) AM_RAM		/* programmable chars */
ADDRESS_MAP_END

static ADDRESS_MAP_START( exidyd_mem , ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x07ff) AM_RAMBANK(1)
	AM_RANGE(0x0800, 0xbfff) AM_RAM		/* ram 48k diskless machine */
	AM_RANGE(0xc000, 0xefff) AM_ROM		/* rom pac and bios */
	AM_RANGE(0xf000, 0xf7ff) AM_RAM		/* screen ram */
	AM_RANGE(0xf800, 0xfbff) AM_ROM		/* char rom */
	AM_RANGE(0xfc00, 0xffff) AM_RAM		/* programmable chars */
ADDRESS_MAP_END

static ADDRESS_MAP_START( exidy_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0xfc, 0xfc) AM_READWRITE( exidy_fc_port_r, exidy_fc_port_w )
	AM_RANGE(0xfd, 0xfd) AM_READWRITE( exidy_fd_port_r, exidy_fd_port_w )
	AM_RANGE(0xfe, 0xfe) AM_READWRITE( exidy_fe_port_r, exidy_fe_port_w )
	AM_RANGE(0xff, 0xff) AM_READWRITE( exidy_ff_port_r, exidy_ff_port_w )
ADDRESS_MAP_END

static INPUT_PORTS_START(exidy)
	PORT_START_TAG("VS")
	/* vblank */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_VBLANK)

	/* line 0 */
	PORT_START_TAG("LINE0")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift Lock") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Control") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Control") PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Graphic") PORT_CODE(KEYCODE_F1)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Stop") PORT_CODE(KEYCODE_ESC) PORT_CHAR(27)
	/* line 1 */
	PORT_START_TAG("LINE1")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(Sel)") PORT_CODE(KEYCODE_F2) PORT_CHAR(27)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Skip") PORT_CODE(KEYCODE_TAB) PORT_CHAR(9)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Repeat") PORT_CODE(KEYCODE_F4)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Clear") PORT_CODE(KEYCODE_F5) PORT_CHAR(12)
	/* line 2 */
	PORT_START_TAG("LINE2")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	/* line 3 */
	PORT_START_TAG("LINE3")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('\"')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	/* line 4 */
	PORT_START_TAG("LINE4")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	/* line 5 */
	PORT_START_TAG("LINE5")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	/* line 6 */
	PORT_START_TAG("LINE6")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	/* line 7 */
	PORT_START_TAG("LINE7")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	/* line 8 */
	PORT_START_TAG("LINE8")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(",") PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	/* line 9 */
	PORT_START_TAG("LINE9")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("; +") PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(". >") PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/ ?") PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	/* line 10 */
	PORT_START_TAG("LINE10")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(": *") PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("[ {") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[')  PORT_CHAR('{')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("] }") PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']')  PORT_CHAR('}')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("@ `") PORT_CODE(KEYCODE_F7) PORT_CHAR('@') PORT_CHAR('`')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\\ |") PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\')  PORT_CHAR('|')
	/* line 11 */
	PORT_START_TAG("LINE11")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("- =") PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("^ ~") PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('^') PORT_CHAR('~')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LINE FEED") PORT_CODE(KEYCODE_F6) PORT_CHAR(10)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CODE(KEYCODE_ENTER_PAD) PORT_CHAR(13)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("_ Rub") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR('_') PORT_CHAR(8)
	/* line 12 */
	PORT_START_TAG("LINE12")
	PORT_BIT (0x10, 0x10, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("- (PAD)") PORT_CODE(KEYCODE_MINUS_PAD) PORT_CHAR(UCHAR_MAMEKEY(MINUS_PAD))
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/ (PAD)") PORT_CODE(KEYCODE_SLASH_PAD)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("* (PAD)") PORT_CODE(KEYCODE_ASTERISK)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("+ (PAD)") PORT_CODE(KEYCODE_PLUS_PAD) PORT_CHAR(UCHAR_MAMEKEY(PLUS_PAD))
	/* line 13 */
	PORT_START_TAG("LINE13")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7 (PAD)") PORT_CODE(KEYCODE_7_PAD) PORT_CHAR(UCHAR_MAMEKEY(7_PAD))
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8 (PAD)") PORT_CODE(KEYCODE_8_PAD) PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP)) PORT_CHAR(UCHAR_MAMEKEY(8_PAD))
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4 (PAD)") PORT_CODE(KEYCODE_4_PAD) PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT)) PORT_CHAR(UCHAR_MAMEKEY(4_PAD))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1 (PAD)") PORT_CODE(KEYCODE_1_PAD) PORT_CHAR(UCHAR_MAMEKEY(1_PAD))
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0 (PAD)") PORT_CODE(KEYCODE_0_PAD) PORT_CHAR(UCHAR_MAMEKEY(0_PAD))
	/* line 14 */
	PORT_START_TAG("LINE14")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9 (PAD)") PORT_CODE(KEYCODE_9_PAD) PORT_CHAR(UCHAR_MAMEKEY(9_PAD))
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6 (PAD)") PORT_CODE(KEYCODE_6_PAD) PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT)) PORT_CHAR(UCHAR_MAMEKEY(6_PAD))
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5 (PAD)") PORT_CODE(KEYCODE_5_PAD) PORT_CHAR(UCHAR_MAMEKEY(5_PAD))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2 (PAD)") PORT_CODE(KEYCODE_2_PAD) PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN)) PORT_CHAR(UCHAR_MAMEKEY(2_PAD))
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(". (PAD)") PORT_CODE(KEYCODE_DEL_PAD) PORT_CHAR(UCHAR_MAMEKEY(DEL_PAD))
	/* line 15 */
	PORT_START_TAG("LINE15")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3 (PAD)") PORT_CODE(KEYCODE_3_PAD) PORT_CHAR(UCHAR_MAMEKEY(3_PAD))
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("= (PAD)") PORT_CODE(KEYCODE_NUMLOCK)
	PORT_BIT (0x04, 0x04, IPT_UNUSED)
	PORT_BIT (0x02, 0x02, IPT_UNUSED)
	PORT_BIT (0x01, 0x01, IPT_UNUSED)

	/* Enhanced options not available on real hardware */
	PORT_START_TAG("CONFIG")
	PORT_CONFNAME( 0x01, 0x01, "Autorun on Quickload")
	PORT_CONFSETTING(    0x00, DEF_STR(No))
	PORT_CONFSETTING(    0x01, DEF_STR(Yes))
	/* hardware connected to printer port */
	PORT_CONFNAME( 0x02, 0x00, "Parallel port" )
	PORT_CONFSETTING(    0x00, "Speaker" )
	PORT_CONFSETTING(    0x02, "Printer" )
//	PORT_CONFNAME( 0x08, 0x08, "Cassette Speaker")
//	PORT_CONFSETTING(    0x08, DEF_STR(On))
//	PORT_CONFSETTING(    0x00, DEF_STR(Off))
INPUT_PORTS_END

/**********************************************************************************************************/

static MACHINE_DRIVER_START( exidy )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 12638000/6)
	MDRV_CPU_PROGRAM_MAP(exidy_mem, 0)
	MDRV_CPU_IO_MAP(exidy_io, 0)

	MDRV_MACHINE_START( exidy )
	MDRV_MACHINE_RESET( exidy )

	/* video hardware */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(200))
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(EXIDY_SCREEN_WIDTH, EXIDY_SCREEN_HEIGHT)
	MDRV_SCREEN_VISIBLE_AREA(0, EXIDY_SCREEN_WIDTH-1, 0, EXIDY_SCREEN_HEIGHT-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_UPDATE( exidy )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	/* printer */
	MDRV_DEVICE_ADD("printer", PRINTER)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( exidyd )
	MDRV_IMPORT_FROM( exidy )

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(exidyd_mem, 0)

	MDRV_MACHINE_START( exidyd )
	MDRV_MACHINE_RESET( exidyd )
MACHINE_DRIVER_END

static DRIVER_INIT( exidy )
{
	UINT8 *RAM = memory_region(REGION_CPU1);
	memory_configure_bank(1, 0, 2, &RAM[0x0000], 0xe000);
//	timer_pulse(ATTOTIME_IN_HZ(200000),NULL,0,exidy_timer);		/* timer for cassette */
}

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(exidy)
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD("exmo1-1.dat", 0xe000, 0x0800, CRC(ac924f67) SHA1(72fcad6dd1ed5ec0527f967604401284d0e4b6a1) ) /* monitor roms */
	ROM_LOAD("exmo1-2.dat", 0xe800, 0x0800, CRC(ead1d0f6) SHA1(c68bed7344091bca135e427b4793cc7d49ca01be) )
	ROM_LOAD("exchr-1.dat", 0xf800, 0x0400, CRC(4a7e1cdd) SHA1(2bf07a59c506b6e0c01ec721fb7b747b20f5dced) ) /* char rom */
	ROM_LOAD_OPTIONAL("diskboot.dat",0xbc00, 0x0100, BAD_DUMP CRC(d82a40d6) SHA1(cd1ef5fb0312cd1640e0853d2442d7d858bc3e3b))
	ROM_CART_LOAD(0, "rom", 0xc000, 0x2000, ROM_FILL_FF | ROM_OPTIONAL)

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD_OPTIONAL("bruce.dat",   0x0000, 0x0020, CRC(fae922cb) SHA1(470a86844cfeab0d9282242e03ff1d8a1b2238d1)) /* video prom */
ROM_END

ROM_START(exidyd)
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD("exmo1-1.dat", 0xe000, 0x0800, CRC(ac924f67) SHA1(72fcad6dd1ed5ec0527f967604401284d0e4b6a1) ) /* monitor roms */
	ROM_LOAD("exmo1-2.dat", 0xe800, 0x0800, CRC(ead1d0f6) SHA1(c68bed7344091bca135e427b4793cc7d49ca01be) )
	ROM_LOAD("exchr-1.dat", 0xf800, 0x0400, CRC(4a7e1cdd) SHA1(2bf07a59c506b6e0c01ec721fb7b747b20f5dced) ) /* char rom */
	ROM_CART_LOAD(0, "rom", 0xc000, 0x2000, ROM_FILL_FF | ROM_OPTIONAL)

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD_OPTIONAL("bruce.dat",   0x0000, 0x0020, CRC(fae922cb) SHA1(470a86844cfeab0d9282242e03ff1d8a1b2238d1)) /* video prom */
ROM_END

static QUICKLOAD_LOAD( exidy )
{
	UINT8 sw = input_port_read(image->machine, "CONFIG") & 1;			/* reading the dipswitch: 1 = autorun */
	UINT16 exec_addr, start_addr, end_addr;

	if (z80bin_load_file(image, file_type, &exec_addr, &start_addr, &end_addr ) == INIT_FAIL)
		return INIT_FAIL;

	if (exec_addr == 0xffff) return INIT_PASS;			/* data file */

	if ((exec_addr >= 0xc000) && (exec_addr <= 0xdfff) && (program_read_byte(0xdffa) != 0xc3))
		return INIT_PASS;					/* can't run a program if the cartridge isn't in */

	/* Since Exidy Basic is by Microsoft, it needs some preprocessing before it can be run.
	1. A start address of 01D5 indicates a basic program which needs its pointers fixed up.
	2. If autorunning, jump to C689 (command processor), else jump to C3DD (READY prompt).
	Important addresses:
		01D5 = start (load) address of a conventional basic program
		C858 = an autorun basic program will have this exec address on the tape
		C3DD = part of basic that displays READY and lets user enter input */

	if ((start_addr == 0x1d5) || (exec_addr == 0xc858))
	{
		UINT8 i, data[]={
			0xcd, 0x26, 0xc4,	// CALL C426	;set up other pointers
			0x21, 0xd4, 1,		// LD HL,01D4	;start of program address (used by C689)
			0x36, 0,		// LD (HL),00	;make sure dummy end-of-line is there
			0xc3, 0x89, 0xc6,};	// JP C689	;run program

		for (i = 0; i < 11; i++) program_write_byte(0xf01f + i, data[i]);
		if (!sw) program_write_word_16le(0xf028,0xc3dd);
		program_write_byte(0x1b7, end_addr&0xff);		/* Tell BASIC where program ends */
		program_write_byte(0x1b8, end_addr>>8);
		if ((exec_addr != 0xc858) && (sw)) program_write_word_16le(0xf028,exec_addr);
		cpunum_set_reg(0, REG_PC, 0xf01f);
	}
	else
	if (sw) cpunum_set_reg(0, REG_PC, exec_addr);

	return INIT_PASS;
}

static void exidy_quickload_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* quickload */
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_DEV_FILE:		strcpy(info->s = device_temp_str(), __FILE__); break;
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:	strcpy(info->s = device_temp_str(), "bin"); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_QUICKLOAD_LOAD:	info->f = (genf *) quickload_load_exidy; break;

		default:				quickload_device_getinfo(devclass, state, info); break;
	}
}

static void exidy_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 4; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:							info->load = DEVICE_IMAGE_LOAD_NAME(exidy_floppy); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "dsk"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}

static void exidy_cassette_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 2; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(exidy)
	CONFIG_DEVICE(exidy_floppy_getinfo)
	CONFIG_DEVICE(cartslot_device_getinfo)
	CONFIG_DEVICE(exidy_cassette_getinfo)		// use of cassette causes a hang
	CONFIG_DEVICE(exidy_quickload_getinfo)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(exidyd)
	CONFIG_DEVICE(cartslot_device_getinfo)
	CONFIG_DEVICE(exidy_cassette_getinfo)		// use of cassette causes a hang
	CONFIG_DEVICE(exidy_quickload_getinfo)
SYSTEM_CONFIG_END


/*    YEAR  NAME    PARENT  COMPAT      MACHINE INPUT   INIT    CONFIG  COMPANY        FULLNAME */
COMP(1979, exidy,   0,		0,	exidy,	exidy,	exidy,	exidy,	"Exidy Inc", "Sorcerer", 0 )
COMP(1979, exidyd,  exidy,	0,	exidyd,	exidy,	exidy,	exidyd,	"Exidy Inc", "Sorcerer (Cassette only)", 0 )

