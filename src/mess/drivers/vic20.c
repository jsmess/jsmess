/*

	TODO:

	- C1540 is unreliable (VIA timing issues?)
	- devicify VIC6560/6561
	- clean up inputs
	- clean up VIA interface
	- access violation in vic6560.c
		* In the Chips (Japan, USA).60
		* K-Star Patrol (Europe).60
		* Seafox (Japan, USA).60
	- SHIFT LOCK
	- restore key
	- light pen
	- VIC21 (built in 21K ram)
	- new cart system (rpk)

*/

#include "emu.h"
#include "includes/vic20.h"
#include "includes/cbm.h"
#include "cpu/m6502/m6502.h"
#include "devices/cartslot.h"
#include "devices/messram.h"
#include "machine/6522via.h"
#include "machine/c1541.h"
#include "machine/cbmiec.h"
#include "machine/ieee488.h"
#include "machine/vic1112.h"
#include "video/vic6560.h"

/* Memory Maps */

static ADDRESS_MAP_START( vic20_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_RAM
//	AM_RANGE(0x0400, 0x07ff) RAM1
//	AM_RANGE(0x0800, 0x0bff) RAM2
//	AM_RANGE(0x0c00, 0x0fff) RAM3
	AM_RANGE(0x1000, 0x1fff) AM_RAM
//	AM_RANGE(0x2000, 0x3fff) BLK1
//	AM_RANGE(0x4000, 0x5fff) BLK2
//	AM_RANGE(0x6000, 0x7fff) BLK3
	AM_RANGE(0x8000, 0x8fff) AM_ROM
	AM_RANGE(0x9000, 0x900f) AM_READWRITE(vic6560_port_r, vic6560_port_w)
	AM_RANGE(0x9110, 0x911f) AM_DEVREADWRITE(M6522_0_TAG, via_r, via_w)
	AM_RANGE(0x9120, 0x912f) AM_DEVREADWRITE(M6522_1_TAG, via_r, via_w)
	AM_RANGE(0x9400, 0x97ff) AM_RAM
//	AM_RANGE(0x9800, 0x9bff) I/O2
//	AM_RANGE(0x9c00, 0x9fff) I/O3
//	AM_RANGE(0xa000, 0xbfff) BLK5
	AM_RANGE(0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END

/* Input Ports */

static CUSTOM_INPUT( vic_custom_inputs )
{
	int bit_mask = (FPTR)param;
	UINT8 port = 0;

	if ((input_port_read(field->port->machine, "CTRLSEL") & 0xf0) == 0x10)
		port |= (input_port_read(field->port->machine, "FAKE0") & bit_mask) ? 1 : 0;

	if ((input_port_read(field->port->machine, "CTRLSEL") & 0xf0) == 0x00)
		port |= (input_port_read(field->port->machine, "FAKE1") & bit_mask) ? 1 : 0;

	return port;
}

static INPUT_PORTS_START( vic20 )
	PORT_START("ROW0")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Del Inst") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8) PORT_CHAR(UCHAR_MAMEKEY(INSERT))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH2) PORT_CHAR('\xA3')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('+')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')

	PORT_START("ROW1")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Return") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR('*')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_TILDE) PORT_CHAR(0x2190)

	PORT_START("ROW2")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Crsr Right Left") PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_MAMEKEY(RIGHT)) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(';') PORT_CHAR(']')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_TAB) PORT_CHAR(UCHAR_SHIFT_2)

	PORT_START("ROW3")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Crsr Down Up") PORT_CODE(KEYCODE_RALT) PORT_CHAR(UCHAR_MAMEKEY(DOWN)) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Shift (Left)") PORT_CODE(KEYCODE_LSHIFT)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Stop Run") PORT_CODE(KEYCODE_HOME)

	PORT_START("ROW4")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F1) PORT_CHAR(UCHAR_MAMEKEY(F1)) PORT_CHAR(UCHAR_MAMEKEY(F2))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Shift (Right)") PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')

	PORT_START("ROW5")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F2) PORT_CHAR(UCHAR_MAMEKEY(F3)) PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('=')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(':') PORT_CHAR('[')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CBM") PORT_CODE(KEYCODE_LCONTROL)

	PORT_START("ROW6")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F3) PORT_CHAR(UCHAR_MAMEKEY(F5)) PORT_CHAR(UCHAR_MAMEKEY(F6))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x91  Pi") PORT_CODE(KEYCODE_DEL) PORT_CHAR(0x2191) PORT_CHAR(0x03C0)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE)  PORT_CHAR('@')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')

	PORT_START("ROW7")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F4) PORT_CHAR(UCHAR_MAMEKEY(F7)) PORT_CHAR(UCHAR_MAMEKEY(F8))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Home  Clr") PORT_CODE(KEYCODE_INSERT) PORT_CHAR(UCHAR_MAMEKEY(HOME))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('-')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')

	PORT_START( "SPECIAL" )  /* special keys */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Restore") PORT_CODE(KEYCODE_PRTSCR) PORT_WRITE_LINE_DEVICE(M6522_0_TAG, via_ca1_w)
	PORT_DIPNAME( 0x01, 0x00, "Shift Lock (switch)") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x01, DEF_STR( On ) )

	PORT_START( "CTRLSEL" )
	PORT_CATEGORY_CLASS( 0xf0, 0x00, DEF_STR( Controller ) )
	PORT_CATEGORY_ITEM( 0x00, DEF_STR( Joystick ), 10 )
	PORT_CATEGORY_ITEM( 0x10, "Paddles", 11 )
	PORT_CATEGORY_ITEM( 0x20, "Lightpen", 12 )

	PORT_START( "JOY" )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(vic_custom_inputs, (void *)0x02)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("Lightpen Signal") PORT_CODE(KEYCODE_LALT) PORT_CATEGORY(12)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(vic_custom_inputs, (void *)0x01)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_CATEGORY(10)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_CATEGORY(10)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_CATEGORY(10)

	PORT_START( "FAKE0" )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("Paddle 2 Button") PORT_CODE(KEYCODE_DEL) PORT_CATEGORY(11)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("Paddle 1 Button") PORT_CODE(KEYCODE_INSERT) PORT_CATEGORY(11)

	PORT_START( "FAKE1" )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_CATEGORY(10)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_CATEGORY(10)

	PORT_START( "PADDLE0" )
	PORT_BIT( 0xff,128,IPT_PADDLE) PORT_SENSITIVITY(30) PORT_KEYDELTA(20) PORT_MINMAX(0,255) PORT_CATEGORY(11) PORT_CODE_DEC(KEYCODE_HOME) PORT_CODE_INC(KEYCODE_PGUP) PORT_REVERSE

	PORT_START( "PADDLE1" )
	PORT_BIT( 0xff,128,IPT_PADDLE) PORT_SENSITIVITY(30) PORT_KEYDELTA(20) PORT_MINMAX(0,255) PORT_CATEGORY(11) PORT_CODE_DEC(KEYCODE_END) PORT_CODE_INC(KEYCODE_PGDN) PORT_PLAYER(2) PORT_REVERSE
INPUT_PORTS_END

static INPUT_PORTS_START( vic20s )
	PORT_INCLUDE( vic20 )

	PORT_MODIFY( "ROW0" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH2) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-')
	
	PORT_MODIFY( "ROW1" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR('@')
	
	PORT_MODIFY( "ROW2" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(0x00C4)
	
	PORT_MODIFY( "ROW5" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(0x00D6)
	
	PORT_MODIFY( "ROW6" )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR(0x00C5)
	
	PORT_MODIFY( "ROW7" )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('=')
INPUT_PORTS_END

/* VIA 0 Interface */

static READ8_DEVICE_HANDLER( via0_pa_r )
{
	/*

        bit     description

        PA0     SERIAL CLK IN
        PA1		SERIAL DATA IN
        PA2		JOY 0
        PA3		JOY 1
        PA4		JOY 2
        PA5		LITE PEN
        PA6		CASS SWITCH
        PA7		SERIAL ATN OUT

    */

	vic20_state *state = (vic20_state *)device->machine->driver_data;
	UINT8 data = 0xfc;

	/* serial clock in */
	data |= cbm_iec_clk_r(state->iec);

	/* serial data in */
	data |= cbm_iec_data_r(state->iec) << 1;

	/* joystick */
	data &= ~(input_port_read(device->machine, "JOY") & 0x3c);

	/* cassette switch */
	if ((cassette_get_state(state->cassette) & CASSETTE_MASK_UISTATE) != CASSETTE_STOPPED) 
		data &= ~0x40;
	else
		data |=  0x40;

	return data;
}

static WRITE8_DEVICE_HANDLER( via0_pa_w )
{
	/*

        bit     description

        PA0     SERIAL CLK IN
        PA1		SERIAL DATA IN
        PA2		JOY 0
        PA3		JOY 1
        PA4		JOY 2
        PA5		LITE PEN
        PA6		CASS SWITCH
        PA7		SERIAL ATN OUT

    */

	vic20_state *state = (vic20_state *)device->machine->driver_data;

	/* serial attention out */
	cbm_iec_atn_w(state->iec, device, !BIT(data, 7));
}

static READ8_DEVICE_HANDLER( via0_pb_r )
{
	/*

        bit     description

        PB0		USER PORT C
        PB1		USER PORT D
        PB2		USER PORT E
        PB3		USER PORT F
        PB4		USER PORT H
        PB5		USER PORT J
        PB6		USER PORT K
        PB7		USER PORT L

    */

	return 0;
}

static WRITE8_DEVICE_HANDLER( via0_pb_w )
{
	/*

        bit     description

        PB0		USER PORT C
        PB1		USER PORT D
        PB2		USER PORT E
        PB3		USER PORT F
        PB4		USER PORT H
        PB5		USER PORT J
        PB6		USER PORT K
        PB7		USER PORT L

    */
}

static WRITE8_DEVICE_HANDLER( via0_ca2_w )
{
	vic20_state *state = (vic20_state *)device->machine->driver_data;

	if (!BIT(data, 0))
	{
		cassette_change_state(state->cassette, CASSETTE_MOTOR_ENABLED, CASSETTE_MASK_MOTOR);
		timer_device_adjust_periodic(state->cassette_timer, attotime_zero, 0, ATTOTIME_IN_HZ(44100));
	}
	else
	{
		cassette_change_state(state->cassette, CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);
		timer_device_reset(state->cassette_timer);
	}
}

static const via6522_interface vic20_via0_intf =
{
	DEVCB_HANDLER(via0_pa_r),
	DEVCB_HANDLER(via0_pb_r),
	DEVCB_NULL, // RESTORE
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(via0_pa_w),
	DEVCB_HANDLER(via0_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(via0_ca2_w), // CASS MOTOR
	DEVCB_NULL,
	DEVCB_CPU_INPUT_LINE(M6502_TAG, INPUT_LINE_NMI)
};

/* VIA 1 Interface */

static READ8_DEVICE_HANDLER( via1_pa_r )
{
	/*

        bit     description

        PA0     ROW 0
        PA1     ROW 1
        PA2     ROW 2
        PA3     ROW 3
        PA4     ROW 4
        PA5     ROW 5
        PA6     ROW 6
        PA7     ROW 7

    */

	vic20_state *state = (vic20_state *)device->machine->driver_data;
	UINT8 data = 0xff;

	if (!BIT(state->key_col, 0)) data &= input_port_read(device->machine, "ROW0");
	if (!BIT(state->key_col, 1)) data &= input_port_read(device->machine, "ROW1");
	if (!BIT(state->key_col, 2)) data &= input_port_read(device->machine, "ROW2");
	if (!BIT(state->key_col, 3)) data &= input_port_read(device->machine, "ROW3");
	if (!BIT(state->key_col, 4)) data &= input_port_read(device->machine, "ROW4");
	if (!BIT(state->key_col, 5)) data &= input_port_read(device->machine, "ROW5");
	if (!BIT(state->key_col, 6)) data &= input_port_read(device->machine, "ROW6");
	if (!BIT(state->key_col, 7)) data &= input_port_read(device->machine, "ROW7");

	return data;
}

static READ8_DEVICE_HANDLER( via1_pb_r )
{
	/*

        bit     description

        PB0     COL 0
        PB1     COL 1
        PB2     COL 2
        PB3     COL 3, CASS WRITE
        PB4     COL 4
        PB5     COL 5
        PB6     COL 6
        PB7     COL 7, JOY 3

    */

	UINT8 data = 0xff;

	/* joystick */
	data &= ~(input_port_read(device->machine, "JOY") & 0x80);

	return data;
}

static WRITE8_DEVICE_HANDLER( via1_pb_w )
{
	/*

        bit     description

        PB0     COL 0
        PB1     COL 1
        PB2     COL 2
        PB3     COL 3, CASS WRITE
        PB4     COL 4
        PB5     COL 5
        PB6     COL 6
        PB7     COL 7, JOY 3

    */

	vic20_state *state = (vic20_state *)device->machine->driver_data;

	/* cassette write */
	cassette_output(devtag_get_device(device->machine, "cassette"), BIT(data, 3) ? -(0x5a9e >> 1) : +(0x5a9e >> 1));

	/* keyboard column */
	state->key_col = data;
}

static WRITE_LINE_DEVICE_HANDLER( via1_ca2_w )
{
	vic20_state *driver_state = (vic20_state *)device->machine->driver_data;

	/* serial clock out */
	cbm_iec_clk_w(driver_state->iec, device, !state);
}

static WRITE_LINE_DEVICE_HANDLER( via1_cb2_w )
{
	vic20_state *driver_state = (vic20_state *)device->machine->driver_data;

	/* serial data out */
	cbm_iec_data_w(driver_state->iec, device, !state);
}

static const via6522_interface vic20_via1_intf =
{
	DEVCB_HANDLER(via1_pa_r),
	DEVCB_HANDLER(via1_pb_r),
	DEVCB_NULL, /* CASS READ */
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_NULL,
	DEVCB_HANDLER(via1_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_LINE(via1_ca2_w),
	DEVCB_LINE(via1_cb2_w),

	DEVCB_CPU_INPUT_LINE(M6502_TAG, M6502_IRQ_LINE)
};

/* Cassette Interface */

static TIMER_DEVICE_CALLBACK( cassette_tick )
{
	vic20_state *state = (vic20_state *)timer->machine->driver_data;
	int data = (cassette_input(state->cassette) > +0.0) ? 1 : 0;
	
	via_ca1_w(state->via1, data);
}

/* IEC Serial Bus */

static CBM_IEC_DAISY( cbm_iec_daisy )
{
	{ M6522_0_TAG },
	{ M6522_1_TAG, DEVCB_DEVICE_LINE(M6522_1_TAG, via_cb1_w) },
	{ C1541_IEC(C1540_TAG) },
	{ NULL}
};

/* IEEE-488 Bus */

static IEEE488_DAISY( ieee488_daisy )
{
	{ VIC1112_IEEE488 },
	{ C2031_IEEE488(C2031_TAG) },
	{ NULL}
};

/* VIC6560 Interface */

#define VC20ADDR2VIC6560ADDR(a) (((a) > 0x8000) ? ((a) & 0x1fff) : ((a) | 0x2000))
#define VIC6560ADDR2VC20ADDR(a) (((a) > 0x2000) ? ((a) & 0x1fff) : ((a) | 0x8000))

static int vic6560_dma_read_color( running_machine *machine, int offset )
{
	const address_space *program = cputag_get_address_space(machine, M6502_TAG, ADDRESS_SPACE_PROGRAM);

	return memory_read_byte(program, 0x9400 | (offset & 0x3ff));
}

static int vic6560_dma_read( running_machine *machine, int offset )
{
	const address_space *program = cputag_get_address_space(machine, M6502_TAG, ADDRESS_SPACE_PROGRAM);
	
	return memory_read_byte(program, VIC6560ADDR2VC20ADDR(offset));
}

/* Machine Initialization */

static MACHINE_START( vic20_common )
{
	vic20_state *state = (vic20_state *)machine->driver_data;
	const address_space *program = cputag_get_address_space(machine, M6502_TAG, ADDRESS_SPACE_PROGRAM);

	/* find devices */
	state->via0 = devtag_get_device(machine, M6522_0_TAG);
	state->via1 = devtag_get_device(machine, M6522_1_TAG);
	state->iec = devtag_get_device(machine, IEC_TAG);
	state->cassette = devtag_get_device(machine, CASSETTE_TAG);
	state->cassette_timer = devtag_get_device(machine, TIMER_C1530_TAG);

	/* set VIA clocks */
	state->via0->set_clock(cputag_get_clock(machine, M6502_TAG));
	state->via1->set_clock(cputag_get_clock(machine, M6502_TAG));

	/* memory expansions */
	switch (messram_get_size(devtag_get_device(machine, "messram")))
	{
	case 32*1024:
		memory_install_ram(program, 0x6000, 0x7fff, 0, 0, NULL);
		/* fallthru */
	case 24*1024:
		memory_install_ram(program, 0x4000, 0x5fff, 0, 0, NULL);
		/* fallthru */
	case 16*1024:
		memory_install_ram(program, 0x2000, 0x3fff, 0, 0, NULL);
		/* fallthru */
	case 8*1024:
		memory_install_ram(program, 0x0400, 0x0fff, 0, 0, NULL);
	}

	/* register for state saving */
	state_save_register_global(machine, state->key_col);
}

static MACHINE_START( vic20_ntsc )
{
	MACHINE_START_CALL(vic20_common);

	/* initialize VIC6560 */
	vic6560_init(vic6560_dma_read, vic6560_dma_read_color);
}

static MACHINE_START( vic20_pal )
{
	MACHINE_START_CALL(vic20_common);

	/* initialize VIC6561 */
	vic6561_init(vic6560_dma_read, vic6560_dma_read_color);
}

/* Cartridge */

static DEVICE_IMAGE_LOAD( vic20_cart )
{
	const address_space *program = cputag_get_address_space(image->machine, M6502_TAG, ADDRESS_SPACE_PROGRAM);
	const char *filetype = image_filetype(image);
	int address = 0;
	int size = image_length(image);
	UINT8 *ptr;

	if (!mame_stricmp(filetype, "20"))
		address = 0x2000;
	else if (!mame_stricmp(filetype, "40"))
		address = 0x4000;
	else if (!mame_stricmp(filetype, "60"))
		address = 0x6000;
	else if (!mame_stricmp(filetype, "70"))
		address = 0x7000;
	else if (!mame_stricmp(filetype, "a0"))
		address = 0xa000;
	else if (!mame_stricmp(filetype, "b0"))
		address = 0xb000;

	ptr = memory_region(image->machine, M6502_TAG);
	
	if (size == 0x4000 && address != 0x4000)
	{
		image_fread(image, ptr + address, 0x2000);
		image_fread(image, ptr + 0xa000, 0x2000);

		memory_install_rom(program, address, address + 0x1fff, 0, 0, ptr + address);
		memory_install_rom(program, 0xa000, 0xbfff, 0, 0, ptr + 0xa000);
	}
	else
	{
		image_fread(image, ptr + address, size);

		memory_install_rom(program, address, (address + size) - 1, 0, 0, ptr + address);
	}

	return INIT_PASS;
}

/* Machine Driver */

static MACHINE_DRIVER_START( vic20_common )
	MDRV_DRIVER_DATA(vic20_state)

	MDRV_TIMER_ADD_PERIODIC(TIMER_C1530_TAG, cassette_tick, HZ(44100))

	/* devices */
	MDRV_VIA6522_ADD(M6522_0_TAG, 0, vic20_via0_intf)
	MDRV_VIA6522_ADD(M6522_1_TAG, 0, vic20_via1_intf)

	MDRV_QUICKLOAD_ADD("quickload", cbm_vc20, "p00,prg", 0)
	MDRV_CASSETTE_ADD(CASSETTE_TAG, cbm_cassette_config )
	MDRV_CBM_IEC_ADD(IEC_TAG, cbm_iec_daisy)
	MDRV_C1540_ADD(C1540_TAG, IEC_TAG, 8)
/*
	MDRV_IEEE488_ADD(IEEE488_TAG, ieee488_daisy)
	MDRV_VIC1112_ADD(IEEE488_TAG)
	MDRV_C2031_ADD(C2031_TAG, IEEE488_TAG, 9)
*/
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("20,40,60,70,a0,b0")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_LOAD(vic20_cart)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("5K")
	MDRV_RAM_EXTRA_OPTIONS("8K,16K,24K,32K")
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( vic20_ntsc )
	MDRV_IMPORT_FROM( vic20_common )

	/* basic machine hardware */
    MDRV_CPU_ADD(M6502_TAG, M6502, VIC6560_CLOCK)
    MDRV_CPU_PROGRAM_MAP(vic20_mem)
	MDRV_CPU_PERIODIC_INT(vic656x_raster_interrupt, VIC656X_HRETRACERATE)

    MDRV_MACHINE_START(vic20_ntsc)

    /* video & sound hardware */
	MDRV_IMPORT_FROM( vic6560_video )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( vic20_pal )
	MDRV_IMPORT_FROM( vic20_common )

	/* basic machine hardware */
    MDRV_CPU_ADD(M6502_TAG, M6502, VIC6561_CLOCK)
    MDRV_CPU_PROGRAM_MAP(vic20_mem)
	MDRV_CPU_PERIODIC_INT(vic656x_raster_interrupt, VIC656X_HRETRACERATE)

	MDRV_MACHINE_START(vic20_pal)

    /* video & sound hardware */
	MDRV_IMPORT_FROM( vic6561_video )
MACHINE_DRIVER_END

/* ROMs */

ROM_START( vic1001 )
	ROM_REGION( 0x10000, M6502_TAG, 0 )
	ROM_LOAD( "901460-02", 0x8000, 0x1000, CRC(fcfd8a4b) SHA1(dae61ac03065aa2904af5c123ce821855898c555) )
	ROM_LOAD( "901486-01", 0xc000, 0x2000, CRC(db4c43c1) SHA1(587d1e90950675ab6b12d91248a3f0d640d02e8d) )
	ROM_LOAD( "901486-02", 0xe000, 0x2000, CRC(336900d7) SHA1(c9ead45e6674d1042ca6199160e8583c23aeac22) )
ROM_END

ROM_START( vic20 )
	ROM_REGION( 0x10000, M6502_TAG, 0 )
	ROM_LOAD( "901460-03.ud7",  0x8000, 0x1000, CRC(83e032a6) SHA1(4fd85ab6647ee2ac7ba40f729323f2472d35b9b4) )
	ROM_LOAD( "901486-01.ue11", 0xc000, 0x2000, CRC(db4c43c1) SHA1(587d1e90950675ab6b12d91248a3f0d640d02e8d) )
	ROM_LOAD( "901486-06.ue12", 0xe000, 0x2000, CRC(e5e7c174) SHA1(06de7ec017a5e78bd6746d89c2ecebb646efeb19) )
ROM_END

ROM_START( vic20p )
	ROM_REGION( 0x10000, M6502_TAG, 0 )
	ROM_LOAD( "901460-03.ud7",  0x8000, 0x1000, CRC(83e032a6) SHA1(4fd85ab6647ee2ac7ba40f729323f2472d35b9b4) )
	ROM_LOAD( "901486-01.ue11", 0xc000, 0x2000, CRC(db4c43c1) SHA1(587d1e90950675ab6b12d91248a3f0d640d02e8d) )
	ROM_LOAD( "901486-07.ue12", 0xe000, 0x2000, CRC(4be07cb4) SHA1(ce0137ed69f003a299f43538fa9eee27898e621e) )
ROM_END

ROM_START( vic20s )
	ROM_REGION( 0x10000, M6502_TAG, 0 )
	ROM_LOAD( "nec22101.207",   0x8000, 0x1000, CRC(d808551d) SHA1(f403f0b0ce5922bd61bbd768bdd6f0b38e648c9f) )
	ROM_LOAD( "901486-01.ue11",	0xc000, 0x2000, CRC(db4c43c1) SHA1(587d1e90950675ab6b12d91248a3f0d640d02e8d) )
	ROM_LOAD( "nec22081.206",   0xe000, 0x2000, CRC(b2a60662) SHA1(cb3e2f6e661ea7f567977751846ce9ad524651a3) )
ROM_END

/* System Drivers */

/*    YEAR  NAME		PARENT		COMPAT  MACHINE		INPUT   INIT    COMPANY							FULLNAME					FLAGS */
COMP( 1980, vic1001,	0,			0,		vic20_ntsc,	vic20,	0,		"Commodore Business Machines",	"VIC-1001 (Japan)",			GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE )
COMP( 1981, vic20,		vic1001,	0,		vic20_ntsc,	vic20,	0,		"Commodore Business Machines",	"VIC-20 (NTSC)",			GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE )
COMP( 1981, vic20p,		vic1001,	0,		vic20_pal,	vic20,	0,		"Commodore Business Machines",	"VIC-20 / VC-20 (PAL)",		GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE )
COMP( 1981, vic20s,		vic1001,	0,		vic20_pal,	vic20s,	0,		"Commodore Business Machines",	"VIC-20 (Sweden/Finland)",	GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE )
