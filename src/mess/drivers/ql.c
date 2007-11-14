#include "driver.h"
#include "cpu/i8039/i8039.h"
#include "cpu/m68000/m68000.h"
#include "devices/cartslot.h"
#include "includes/serial.h"
#include "video/generic.h"
#include "video/zx8301.h"
#include "inputx.h"

/*

	Sinclair QL

	MESS Driver by Curt Coder


	TODO:

	- get COMCTL from i8048 WR output
	- IPC <-> ZX8302 communication
	- correct frequency for RTC timer
	- keyboard/joystick connections
	- speaker sound
	- accurate screen timings
	- microdrive
	- serial I/O

*/

#define X1 15000000.0
#define X2 32768.0
#define X3 4436000.0
#define X4 11000000.0

/* Peripheral Chip (ZX8302) */

#define ZX8302_IPC_START	0
#define ZX8302_IPC_STOP		5

static struct ZX8302
{
	int comdata;
	int comctl;
	int baudx4;
	UINT8 tcr;
	UINT8 tdr;
	UINT8 intr;
	UINT16 ctr;
	UINT8 idr;
	int ipc_bits;
	int ser1_rxd, ser1_cts;
	int ser2_txd, ser2_dtr;
} zx8302;

static mame_timer *zx8302_txd_timer, *zx8302_ipc_timer, *zx8302_rtc_timer;

static TIMER_CALLBACK( zx8302_txd_tick )
{
}

static void hack_comctl(void)
{
	/*
		This function hacks in the COMCTL line state. The real solution would be 
		to add a WR output line to the i8039 cpu core and monitor that instead.
	*/

	offs_t pc = cpunum_get_physical_pc_byte(1);

	switch (pc)
	{
	case 0x0758:
	case 0x075a:
	case 0x076b:
	case 0x0774:
		zx8302.comctl = 0;
	
	default:
		zx8302.comctl = 1;
	}

	logerror("PC: %x, comctl: %u\n", pc, zx8302.comctl);
}

static TIMER_CALLBACK( zx8302_ipc_tick )
{
	zx8302.baudx4 = zx8302.baudx4 ? 0 : 1;

	hack_comctl(); // HACK

	if (zx8302.baudx4)
	{
		if (zx8302.comctl)
		{
			// send 4 bits of data to the IPC

			switch (zx8302.ipc_bits)
			{
			case ZX8302_IPC_START:
				zx8302.comdata = 0;
				zx8302.ipc_bits++;
				break;

			default:
				zx8302.comdata = BIT(zx8302.idr, 0);
				zx8302.idr >>= 1;
				zx8302.ipc_bits++;
				break;

			case ZX8302_IPC_STOP:
				zx8302.comdata = 1;
				break;
			}
		}
	}
}

static TIMER_CALLBACK( zx8302_rtc_tick )
{
	zx8302.ctr++;
}

static READ8_HANDLER( zx8302_rtc_r )
{
	if (offset)
	{
		return zx8302.ctr & 0xff;
	}
	else
	{
		return zx8302.ctr >> 8;
	}
}

static WRITE8_HANDLER( zx8302_rtc_w )
{
	if (offset)
	{
		zx8302.ctr = (zx8302.ctr & 0xff00) | data;
	}
	else
	{
		zx8302.ctr = (data << 8) | (zx8302.ctr & 0xff);
	}
}

static WRITE8_HANDLER( zx8302_control_w )
{
	int baud = 19200 >> (data & 0x07);
	int baudx4 = baud * 4;

	zx8302.tcr = data;

	mame_timer_adjust(zx8302_txd_timer, time_zero, 0, MAME_TIME_IN_HZ(baud));
	mame_timer_adjust(zx8302_ipc_timer, time_zero, 0, MAME_TIME_IN_HZ(baudx4));
}

static READ8_HANDLER( zx8302_ipc_r )
{
	/*
		
		bit		description
		
		0		Network port
		1		Microdrive buffer full
		2		
		3		
		4		DTR
		5		CTS
		6		COMCTL
		7		COMDATA

	*/

	return (zx8302.comdata << 7) | (zx8302.comctl << 6);
}

static WRITE8_HANDLER( zx8302_ipc_w )
{
	zx8302.idr = data;
	zx8302.ipc_bits = ZX8302_IPC_START;
}

static WRITE8_HANDLER( zx8302_mdv_control_w )
{
	/*
	MDV control
	at reset; and after data transfer of any mdv
		(02 00) 8 times			stop all motors
	start an mdvN:
		(03 01) (02 00) N-1 times	start motorN
	format (erase):
		0a
		(0e 0e) 0a per header and per sector
	format (verify):
		02
		(02 02) after finding pulses per header/sector
	format (write directory):
		0a (0e 0e) 02 write sector
	format (find sector 0):
		(02 02) after pulses
	format (write directory):
		0a (0e 0e) 02 write sector
		(02 00) 8 times 		stop motor

	read sector:
		(02 02) after pulses
		get 4 bytes
		(02 02) indication to skip PLL sequence: 6*00,2*ff
	*/
}

static READ8_HANDLER( zx8302_irq_r )
{
	/*
		
		bit		description
		
		0		Gap interrupt
		1		Interface interrupt
		2		Transmit interrupt
		3		Frame interrupt
		4		External interrupt
		5		
		6		
		7		

	*/

	return 0;
}

static WRITE8_HANDLER( zx8302_irq_w )
{
	zx8302.intr = data;
}

static READ8_HANDLER( zx8302_mdv_track1_r )
{
	return 0;
}

static READ8_HANDLER( zx8302_mdv_track2_r )
{
	return 0;
}

static WRITE8_HANDLER( zx8302_txdata_w )
{
	zx8302.tdr = data;
}

/* Intelligent Peripheral Controller (IPC) */

static struct IPC
{
	UINT8 keylatch;
	int ser1_txd, ser1_dtr;
	int ser2_rxd, ser2_cts;
} ipc;

static WRITE8_HANDLER( ipc_port1_w )
{
	/*
		
		bit		description
		
		0		Keyboard column output (KBO0)
		1		Keyboard column output (KBO1)
		2		Keyboard column output (KBO2)
		3		Keyboard column output (KBO3)
		4		Keyboard column output (KBO4)
		5		Keyboard column output (KBO5)
		6		Keyboard column output (KBO6)
		7		Keyboard column output (KBO7)

	*/

	ipc.keylatch = data;
}

static WRITE8_HANDLER( ipc_port2_w )
{
	/*
		
		bit		description
		
		0		Serial data input (SER2 RxD, SER1 TxD)
		1		Speaker output
		2		Interrupt output (IPL0-2)
		3		Interrupt output (IPL1)
		4		Serial Clear-to-Send output (SER2 CTS)
		5		Serial Data Terminal Ready output (SER1 DTR)
		6		not connected
		7		ZX8302 serial link input/output (COMDATA)

	*/

	int ipl = (BIT(data, 2) << 1) | BIT(data, 3);

	switch (ipl)
	{
	case 0:
		cpunum_set_input_line(0, MC68000_IRQ_2, CLEAR_LINE);
		cpunum_set_input_line(0, MC68000_IRQ_5, CLEAR_LINE);
		cpunum_set_input_line(0, MC68000_IRQ_7, ASSERT_LINE);
		break;
	case 1:
		cpunum_set_input_line(0, MC68000_IRQ_2, CLEAR_LINE);
		cpunum_set_input_line(0, MC68000_IRQ_5, ASSERT_LINE);
		cpunum_set_input_line(0, MC68000_IRQ_7, CLEAR_LINE);
		break;
	case 2:
		cpunum_set_input_line(0, MC68000_IRQ_2, ASSERT_LINE);
		cpunum_set_input_line(0, MC68000_IRQ_5, CLEAR_LINE);
		cpunum_set_input_line(0, MC68000_IRQ_7, CLEAR_LINE);
		break;
	case 3:
		// do nothing
		break;
	}

	ipc.ser2_cts = BIT(data, 4);
	ipc.ser1_dtr = BIT(data, 5);

	zx8302.comdata = BIT(data, 7);
}

static READ8_HANDLER( ipc_port2_r )
{
	/*
		
		bit		description
		
		0		Serial data input (SER2 RxD, SER1 TxD)
		1		Speaker output
		2		Interrupt output (IPL0-2)
		3		Interrupt output (IPL1)
		4		Serial Clear-to-Send output (SER2 CTS)
		5		Serial Data Terminal Ready output (SER1 DTR)
		6		not connected
		7		ZX8302 serial link input/output (COMDATA)

	*/

	int irq = (ipc.ser2_rxd | ipc.ser1_txd);

	cpunum_set_input_line(1, INPUT_LINE_IRQ0, irq);

	return (zx8302.comdata << 7) | irq;
}

static READ8_HANDLER( ipc_t1_r )
{
	return zx8302.baudx4;
}

static READ8_HANDLER( ipc_bus_r )
{
	/*
		
		bit		description
		
		0		Keyboard row input (KBI0)
		1		Keyboard row input (KBI1)
		2		Keyboard row input (KBI2)
		3		Keyboard row input (KBI3)
		4		Keyboard row input (KBI4)
		5		Keyboard row input (KBI5)
		6		Keyboard row input (KBI6)
		7		Keyboard row input (KBI7)

	*/

	if (BIT(ipc.keylatch, 0)) return readinputportbytag("ROW0") | readinputportbytag("JOY0");
	if (BIT(ipc.keylatch, 1)) return readinputportbytag("ROW1") | readinputportbytag("JOY1");
	if (BIT(ipc.keylatch, 2)) return readinputportbytag("ROW2");
	if (BIT(ipc.keylatch, 3)) return readinputportbytag("ROW3");
	if (BIT(ipc.keylatch, 4)) return readinputportbytag("ROW4");
	if (BIT(ipc.keylatch, 5)) return readinputportbytag("ROW5");
	if (BIT(ipc.keylatch, 6)) return readinputportbytag("ROW6");
	if (BIT(ipc.keylatch, 7)) return readinputportbytag("ROW7") | readinputportbytag("SPECIAL");

	return 0;
}

/* Memory Maps */

static ADDRESS_MAP_START( ql_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x000000, 0x00bfff) AM_ROM	// System ROM
	AM_RANGE(0x00c000, 0x00ffff) AM_ROM // Cartridge ROM
	AM_RANGE(0x018000, 0x018001) AM_READWRITE(zx8302_rtc_r, zx8302_rtc_w)
	AM_RANGE(0x018002, 0x018002) AM_WRITE(zx8302_control_w)
	AM_RANGE(0x018003, 0x018003) AM_WRITE(zx8302_ipc_w)
	AM_RANGE(0x018020, 0x018020) AM_READWRITE(zx8302_ipc_r, zx8302_mdv_control_w)
	AM_RANGE(0x018020, 0x018020) AM_READWRITE(zx8302_irq_r, zx8302_irq_w)
	AM_RANGE(0x018022, 0x018022) AM_READWRITE(zx8302_mdv_track1_r, zx8302_txdata_w)
	AM_RANGE(0x018023, 0x018023) AM_READ(zx8302_mdv_track2_r)
	AM_RANGE(0x018063, 0x018063) AM_WRITE(zx8301_control_w)
	AM_RANGE(0x020000, 0x02ffff) AM_RAM AM_BASE(&videoram)
	AM_RANGE(0x030000, 0x03ffff) AM_RAM // onboard RAM
	AM_RANGE(0x040000, 0x0bffff) AM_RAM // 512KB add-on RAM
	AM_RANGE(0x0c0000, 0x0dffff) AM_NOP // 8x16KB device slots
	AM_RANGE(0x0e0000, 0x0fffff) AM_ROM // add-on ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( ipc_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x000, 0x7ff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( ipc_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x27, 0x28) AM_READNOP // IPC reads these to set P0 (bus) to Hi-Z mode
	AM_RANGE(I8039_p1, I8039_p1) AM_WRITE(ipc_port1_w)
	AM_RANGE(I8039_p2, I8039_p2) AM_READWRITE(ipc_port2_r, ipc_port2_w)
	AM_RANGE(I8039_t1, I8039_t1) AM_READ(ipc_t1_r)
	AM_RANGE(I8039_bus, I8039_bus) AM_READ(ipc_bus_r)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( ql )
	PORT_START_TAG("ROW0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F1) PORT_NAME("F1")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F2) PORT_NAME("F2")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F3) PORT_NAME("F3")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('&')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F4) PORT_NAME("F4")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F5) PORT_NAME("F5")

	PORT_START_TAG("ROW1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('@')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('^')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('*')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR(')')

	PORT_START_TAG("ROW2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_TAB) PORT_CHAR('\t') PORT_NAME("TABULATE")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('_')

	PORT_START_TAG("ROW3")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('P')

	PORT_START_TAG("ROW4")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CAPSLOCK) PORT_NAME("CAPS LOCK")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR(':')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('=') PORT_CHAR('+')

	PORT_START_TAG("ROW5")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('\'') PORT_CHAR('\"')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_TILDE) PORT_CHAR(0x00A3) PORT_CHAR('~')

	PORT_START_TAG("ROW6")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')

	PORT_START_TAG("ROW7") // where is UP?
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT)) PORT_NAME("\xE2\x86\x90") 
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_ESC) PORT_CHAR(UCHAR_MAMEKEY(ESC)) PORT_NAME("ESC \xC2\xA9") 
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT)) PORT_NAME("\xE2\x86\x92") 
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ') PORT_NAME("SPACE") 
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN)) PORT_NAME("\xE2\x86\x93") 
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13) PORT_NAME("ENTER")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\') PORT_CHAR('|')

	PORT_START_TAG("SPECIAL")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1) PORT_NAME("SHIFT")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(ESC)) PORT_NAME("CTRL")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_LALT) PORT_CODE(KEYCODE_RALT) PORT_CHAR(UCHAR_MAMEKEY(LALT)) PORT_CHAR(UCHAR_MAMEKEY(RALT)) PORT_NAME("ALT")

	PORT_START_TAG("JOY0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )	PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )	PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )	PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 )		PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("JOY1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )	PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )	PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 )		PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )	PORT_PLAYER(2) PORT_8WAY
INPUT_PORTS_END

/* Machine Drivers */

static INTERRUPT_GEN( zx8302_int )
{
	cpunum_set_input_line(0, MC68000_IRQ_2, ASSERT_LINE);
}

static MACHINE_START( ql )
{
	// ZX8302

	state_save_register_global(zx8302.comdata);
	state_save_register_global(zx8302.comctl);
	state_save_register_global(zx8302.baudx4);
	state_save_register_global(zx8302.tcr);
	state_save_register_global(zx8302.tdr);
	state_save_register_global(zx8302.intr);
	state_save_register_global(zx8302.ctr);
	state_save_register_global(zx8302.idr);
	state_save_register_global(zx8302.ipc_bits);
	state_save_register_global(zx8302.ser1_rxd);
	state_save_register_global(zx8302.ser1_cts);
	state_save_register_global(zx8302.ser2_txd);
	state_save_register_global(zx8302.ser2_dtr);

	memset(&zx8302, 0, sizeof(zx8302));
	zx8302.comctl = 1;
	zx8302.comdata = 1;

	zx8302_txd_timer = mame_timer_alloc(zx8302_txd_tick);
	zx8302_ipc_timer = mame_timer_alloc(zx8302_ipc_tick);
	zx8302_rtc_timer = mame_timer_alloc(zx8302_rtc_tick);

	mame_timer_adjust(zx8302_rtc_timer, time_zero, 0, MAME_TIME_IN_HZ(X2));

	// IPC

	state_save_register_global(ipc.keylatch);
	state_save_register_global(ipc.ser1_txd);
	state_save_register_global(ipc.ser1_dtr);
	state_save_register_global(ipc.ser2_rxd);
	state_save_register_global(ipc.ser2_cts);

	memset(&ipc, 0, sizeof(ipc));
}

static MACHINE_DRIVER_START( ql )
	// basic machine hardware
	MDRV_CPU_ADD(M68008, X1/2)
	MDRV_CPU_PROGRAM_MAP(ql_map, 0)
	MDRV_CPU_VBLANK_INT(zx8302_int, 1)

	MDRV_CPU_ADD(I8048, X4) // i8749
	MDRV_CPU_PROGRAM_MAP(ipc_map, 0)
	MDRV_CPU_IO_MAP(ipc_io_map, 0)

	MDRV_MACHINE_START(ql)

	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)

    // video hardware
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_SCREEN_VISIBLE_AREA(0, 512-1, 0, 256-1)

	MDRV_PALETTE_LENGTH(8)

	MDRV_PALETTE_INIT(zx8301)
	MDRV_VIDEO_START(zx8301)
	MDRV_VIDEO_UPDATE(zx8301)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ql_ntsc )
	MDRV_IMPORT_FROM(ql)
	MDRV_SCREEN_REFRESH_RATE(60)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( ql )
    ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_SYSTEM_BIOS( 0, "js", "v1.10 (JS)" )
	ROMX_LOAD( "ql.js 0000.ic33", 0x000000, 0x008000, CRC(1bbad3b8) SHA1(59fd4372771a630967ee102760f4652904d7d5fa), ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(1) )
    ROMX_LOAD( "ql.js 8000.ic34", 0x008000, 0x004000, CRC(c970800e) SHA1(b8c9203026a7de6a44bd0942ec9343e8b222cb41), ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(1) )

	ROM_SYSTEM_BIOS( 1, "tb", "v1.0? (TB)" )
	ROMX_LOAD( "tb.ic33", 0x000000, 0x008000, BAD_DUMP CRC(1c86d688) SHA1(7df8028e6671afc4ebd5f65bf6c2d6019181f239), ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(2) )
    ROMX_LOAD( "tb.ic34", 0x008000, 0x004000, BAD_DUMP CRC(de7f9669) SHA1(9d6bc0b794541a4cec2203256ae92c7e68d1011d), ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(2) )

	ROM_SYSTEM_BIOS( 2, "jm", "v1.03 (JM)" )
	ROMX_LOAD( "ql.jm 0000.ic33", 0x000000, 0x008000, CRC(1f8e840a) SHA1(7929e716dfe88318bbe99e34f47d039957fe3cc0), ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(3) )
    ROMX_LOAD( "ql.jm 8000.ic34", 0x008000, 0x004000, CRC(9168a2e9) SHA1(1e7c47a59fc40bd96dfefc2f4d86827c15f0199e), ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(3) )

	ROM_SYSTEM_BIOS( 3, "ah", "v1.02 (AH)" )
	ROMX_LOAD( "ah.ic33.1", 0x000000, 0x004000, BAD_DUMP CRC(a9b4d2df) SHA1(142d6f01a9621aff5e0ad678bd3cbf5cde0db801), ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(4) )
    ROMX_LOAD( "ah.ic33.2", 0x004000, 0x004000, BAD_DUMP CRC(36488e4e) SHA1(ff6f597b30ea03ce480a3d6728fd1d858da34d6a), ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(4) )
	ROMX_LOAD( "ah.ic34",   0x008000, 0x004000, BAD_DUMP CRC(61259d4c) SHA1(bdd10d111e7ba488551a27c8d3b2743917ff1307), ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(4) )

	ROM_SYSTEM_BIOS( 4, "pm", "v1.01 (PM)" )
	ROMX_LOAD( "pm.ic33", 0x000000, 0x008000, NO_DUMP, ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(5) )
    ROMX_LOAD( "pm.ic34", 0x008000, 0x004000, NO_DUMP, ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(5) )

	ROM_SYSTEM_BIOS( 5, "fb", "v1.00 (FB)" )
    ROMX_LOAD( "fb.ic33", 0x000000, 0x008000, NO_DUMP, ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(6) )
    ROMX_LOAD( "fb.ic34", 0x008000, 0x004000, NO_DUMP, ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(6) )

	ROM_SYSTEM_BIOS( 6, "tyche", "v2.05 (Tyche)" )
    ROMX_LOAD( "tyche.rom", 0x000000, 0x010000, BAD_DUMP CRC(8724b495) SHA1(5f33a1bc3f23fd09c31844b65bc3aca7616f180a), ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(7) )

	ROM_SYSTEM_BIOS( 7, "min189", "Minerva v1.89" )
    ROMX_LOAD( "minerva.rom", 0x000000, 0x00c000, CRC(930befe3) SHA1(84a99c4df13b97f90baf1ec8cb6c2e52e3e1bb4d), ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(8) )

	ROM_REGION( 0x800, REGION_CPU2, 0 )
	ROM_LOAD( "v07.ic24",	  0x0000, 0x0800, CRC(051111f9) SHA1(83ed562464df89b9fdd9740db51d45884a512696) ) // V0.7
	ROM_LOAD( "ipc8049.ic24", 0x0000, 0x0800, CRC(6a0d1f20) SHA1(fcb1c97ee7c66e5b6d8fbb57c06fd2f6509f2e1b) )
ROM_END

ROM_START( ql_jsu )
    ROM_REGION( 0x400000, REGION_CPU1, 0 )
    ROM_LOAD16_WORD_SWAP( "jsu.ic33", 0x000000, 0x008000, BAD_DUMP CRC(e397f49f) SHA1(c06f92eabaf3e6dd298c51cb7f7535d8ef0ef9c5) )
    ROM_LOAD16_WORD_SWAP( "jsu.ic34", 0x008000, 0x004000, BAD_DUMP CRC(3debbacc) SHA1(9fbc3e42ec463fa42f9c535d63780ff53a9313ec) )

	ROM_REGION( 0x800, REGION_CPU2, 0 )
	ROM_LOAD( "ipc8049.ic24", 0x0000, 0x0800, CRC(6a0d1f20) SHA1(fcb1c97ee7c66e5b6d8fbb57c06fd2f6509f2e1b) )
ROM_END

ROM_START( ql_mge )
    ROM_REGION( 0x400000, REGION_CPU1, 0 )
    ROM_LOAD16_WORD_SWAP( "mge.ic33", 0x000000, 0x008000, BAD_DUMP CRC(d5293bde) SHA1(bf5af7e53a472d4e9871f182210787d601db0634) )
    ROM_LOAD16_WORD_SWAP( "mge.ic34", 0x008000, 0x004000, BAD_DUMP CRC(a694f8d7) SHA1(bd2868656008de85d7c191598588017ae8aa3339) )

	ROM_REGION( 0x800, REGION_CPU2, 0 )
	ROM_LOAD( "ipc8049.ic24", 0x0000, 0x0800, CRC(6a0d1f20) SHA1(fcb1c97ee7c66e5b6d8fbb57c06fd2f6509f2e1b) )
ROM_END

ROM_START( ql_mgf )
    ROM_REGION( 0x400000, REGION_CPU1, 0 )
    ROM_LOAD16_WORD_SWAP( "mgf.ic33", 0x000000, 0x008000, NO_DUMP )
    ROM_LOAD16_WORD_SWAP( "mgf.ic34", 0x008000, 0x004000, NO_DUMP )

	ROM_REGION( 0x800, REGION_CPU2, 0 )
	ROM_LOAD( "ipc8049.ic24", 0x0000, 0x0800, CRC(6a0d1f20) SHA1(fcb1c97ee7c66e5b6d8fbb57c06fd2f6509f2e1b) )
ROM_END

ROM_START( ql_mgg )
    ROM_REGION( 0x400000, REGION_CPU1, 0 )
    ROM_LOAD16_WORD_SWAP( "mgg.ic33", 0x000000, 0x008000, BAD_DUMP CRC(b4e468fd) SHA1(cd02a3cd79af90d48b65077d0571efc2f12f146e) )
    ROM_LOAD16_WORD_SWAP( "mgg.ic34", 0x008000, 0x004000, BAD_DUMP CRC(54959d40) SHA1(ffc0be9649f26019d7be82925c18dc699259877f) )

	ROM_REGION( 0x800, REGION_CPU2, 0 )
	ROM_LOAD( "ipc8049.ic24", 0x0000, 0x0800, CRC(6a0d1f20) SHA1(fcb1c97ee7c66e5b6d8fbb57c06fd2f6509f2e1b) )
ROM_END

ROM_START( ql_mgi )
    ROM_REGION( 0x400000, REGION_CPU1, 0 )
    ROM_LOAD16_WORD_SWAP( "mgi.ic33", 0x000000, 0x008000, BAD_DUMP CRC(d5293bde) SHA1(bf5af7e53a472d4e9871f182210787d601db0634) )
    ROM_LOAD16_WORD_SWAP( "mgi.ic34", 0x008000, 0x004000, BAD_DUMP CRC(a2fdfb83) SHA1(162b1052737500f3c13497cdf0f813ba006bdae9) )

	ROM_REGION( 0x800, REGION_CPU2, 0 )
	ROM_LOAD( "ipc8049.ic24", 0x0000, 0x0800, CRC(6a0d1f20) SHA1(fcb1c97ee7c66e5b6d8fbb57c06fd2f6509f2e1b) )
ROM_END

ROM_START( ql_mgs )
    ROM_REGION( 0x400000, REGION_CPU1, 0 )
    ROM_LOAD16_WORD_SWAP( "mgs.ic33", 0x000000, 0x008000, NO_DUMP )
    ROM_LOAD16_WORD_SWAP( "mgs.ic34", 0x008000, 0x004000, NO_DUMP )

	ROM_REGION( 0x800, REGION_CPU2, 0 )
	ROM_LOAD( "ipc8049.ic24", 0x0000, 0x0800, CRC(6a0d1f20) SHA1(fcb1c97ee7c66e5b6d8fbb57c06fd2f6509f2e1b) )
ROM_END

ROM_START( ql_efp )
    ROM_REGION( 0x400000, REGION_CPU1, 0 )
    ROM_LOAD16_WORD_SWAP( "efp.ic33", 0x000000, 0x008000, BAD_DUMP CRC(eb181641) SHA1(43c1e0215cf540cbbda240b1048910ff55681059) )
    ROM_LOAD16_WORD_SWAP( "efp.ic34", 0x008000, 0x004000, BAD_DUMP CRC(4c3b34b7) SHA1(f9dc571d2d4f68520b306ecc7516acaeea69ec0d) )

	ROM_REGION( 0x800, REGION_CPU2, 0 )
	ROM_LOAD( "ipc8049.ic24", 0x0000, 0x0800, CRC(6a0d1f20) SHA1(fcb1c97ee7c66e5b6d8fbb57c06fd2f6509f2e1b) )
ROM_END

/* System Configuration */

static DEVICE_LOAD( ql_microdrive )
{
	int	filesize = image_length(image);

	if (filesize == 174930)
	{
		return INIT_PASS;
	}

	return INIT_FAIL;
}

static void ql_microdrive_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:							info->i = IO_CASSETTE; break;
		case DEVINFO_INT_COUNT:							info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_ql_microdrive; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "mdv"); break;
	}
}

static DEVICE_LOAD( ql_serial )
{
	/* filename specified */
	if (serial_device_load(image)==INIT_PASS)
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

static void ql_serial_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* serial */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:							info->i = IO_SERIAL; break;
		case DEVINFO_INT_COUNT:							info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_INIT:							info->init = serial_device_init; break;
		case DEVINFO_PTR_LOAD:							info->load = device_load_ql_serial; break;
		case DEVINFO_PTR_UNLOAD:						info->unload = serial_device_unload; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "txt"); break;
	}
}

static DEVICE_LOAD( ql_cart )
{
	int	filesize = image_length(image);

	if (filesize <= 16 * 1024)
	{
		return INIT_PASS;
	}

	return INIT_FAIL;
}

static void ql_cartslot_getinfo( const device_class *devclass, UINT32 state, union devinfo *info )
{
	switch( state )
	{
	case DEVINFO_INT_COUNT:
		info->i = 1;
		break;
	case DEVINFO_PTR_LOAD:
		info->load = device_load_ql_cart;
		break;
	case DEVINFO_STR_FILE_EXTENSIONS:
		strcpy(info->s = device_temp_str(), "qlc");
		break;
	default:
		cartslot_device_getinfo( devclass, state, info );
		break;
	}
}

SYSTEM_CONFIG_START( ql )
	CONFIG_RAM_DEFAULT	(128 * 1024)
	CONFIG_RAM			(640 * 1024)
	CONFIG_DEVICE(ql_microdrive_getinfo)
	CONFIG_DEVICE(ql_serial_getinfo)
	CONFIG_DEVICE(ql_cartslot_getinfo)
SYSTEM_CONFIG_END

/* Computer Drivers */

/*    YEAR  NAME    PARENT  COMPAT  MACHINE     INPUT   INIT    CONFIG  COMPANY                     FULLNAME        FLAGS */
COMP( 1984, ql,     0,      0,      ql,         ql,     0,      ql,     "Sinclair Research Ltd",    "QL (UK)",      GAME_NOT_WORKING )
COMP( 1985, ql_jsu, ql,     0,      ql_ntsc,    ql,     0,      ql,     "Sinclair Research Ltd",    "QL (USA)",     GAME_NOT_WORKING )
COMP( 1985, ql_mge, ql,     0,      ql,         ql,     0,      ql,     "Sinclair Research Ltd",    "QL (Spain)",   GAME_NOT_WORKING )
COMP( 1985, ql_mgf, ql,     0,      ql,         ql,     0,      ql,     "Sinclair Research Ltd",    "QL (France)",  GAME_NOT_WORKING )
COMP( 1985, ql_mgg, ql,     0,      ql,         ql,     0,      ql,     "Sinclair Research Ltd",    "QL (Germany)", GAME_NOT_WORKING )
COMP( 1985, ql_mgi, ql,     0,      ql,         ql,     0,      ql,     "Sinclair Research Ltd",    "QL (Italy)",   GAME_NOT_WORKING )
COMP( 1985, ql_mgs, ql,     0,      ql,         ql,     0,      ql,     "Sinclair Research Ltd",    "QL (Sweden)",  GAME_NOT_WORKING )
COMP( 1985, ql_efp, ql,     0,      ql,         ql,     0,      ql,     "Sinclair Research Ltd",    "QL (Greece)",  GAME_NOT_WORKING )
