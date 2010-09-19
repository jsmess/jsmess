#include "emu.h"
#include "abc77.h"
#include "cpu/mcs48/mcs48.h"
#include "sound/discrete.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define I8035_TAG	"z16"

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _abc77_t abc77_t;
struct _abc77_t
{
	devcb_resolved_write_line	out_txd_func;
	devcb_resolved_write_line	out_clock_func;
	devcb_resolved_write_line	out_keydown_func;

	running_device *cpu;		/* CPU of the 8035 */

	int txd;						/* transmit data */
	int keylatch;					/* keyboard row latch */
	int clock;						/* transmit clock */
	int hys;						/* hysteresis */
	int reset;						/* reset */

	/* timers */
	emu_timer *reset_timer;			/* reset timer */
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE abc77_t *get_safe_token(running_device *device)
{
	assert(device != NULL);
	return (abc77_t *)downcast<legacy_device_base *>(device)->token();
}

INLINE const abc77_interface *get_interface(running_device *device)
{
	assert(device != NULL);
	assert((device->type() == ABC77));
	return (const abc77_interface *) device->baseconfig().static_config();
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    DISCRETE_SOUND( abc77 )
-------------------------------------------------*/

static const discrete_555_desc abc77_ne556_a =
{
	DISC_555_OUT_SQW | DISC_555_OUT_DC,
	5,		// B+ voltage of 555
	DEFAULT_555_VALUES
};

static DISCRETE_SOUND_START( abc77 )
	DISCRETE_INPUT_LOGIC(NODE_01)
	DISCRETE_555_ASTABLE(NODE_02, NODE_01, RES_K(2.7), RES_K(15), CAP_N(22), &abc77_ne556_a)
	DISCRETE_OUTPUT(NODE_02, 5000)
DISCRETE_SOUND_END

/*-------------------------------------------------
    TIMER_DEVICE_CALLBACK( clock_tick )
-------------------------------------------------*/

static TIMER_DEVICE_CALLBACK( clock_tick )
{
	running_device *device = timer.machine->device(ABC77_TAG);
	abc77_t *abc77 = get_safe_token(device);

	abc77->clock = !abc77->clock;

	devcb_call_write_line(&abc77->out_clock_func, abc77->clock);
}

/*-------------------------------------------------
    TIMER_CALLBACK( reset_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( reset_tick )
{
	running_device *device = (running_device *)ptr;
	abc77_t *abc77 = get_safe_token(device);

	cpu_set_input_line(abc77->cpu, INPUT_LINE_RESET, CLEAR_LINE);
}

/*-------------------------------------------------
    abc77_clock_r - read serial clock
-------------------------------------------------*/

static READ8_HANDLER( abc77_clock_r )
{
	running_device *device = space->machine->device(ABC77_TAG);
	abc77_t *abc77 = get_safe_token(device);

	return abc77->clock;
}

/*-------------------------------------------------
    abc77_data_r - read keyboard matrix
-------------------------------------------------*/

static READ8_HANDLER( abc77_data_r )
{
	running_device *device = space->machine->device(ABC77_TAG);
	abc77_t *abc77 = get_safe_token(device);

	static const char *const keynames[] = { "ABC77_X0", "ABC77_X1", "ABC77_X2", "ABC77_X3", "ABC77_X4", "ABC77_X5", "ABC77_X6", "ABC77_X7", "ABC77_X8", "ABC77_X9", "ABC77_X10", "ABC77_X11" };

	return input_port_read(device->machine, keynames[abc77->keylatch]);
}

/*-------------------------------------------------
    abc77_data_w - keyboard control
-------------------------------------------------*/

static WRITE8_HANDLER( abc77_data_w )
{
	running_device *device = space->machine->device(ABC77_TAG);
	running_device *discrete = space->machine->device("discrete");
	abc77_t *abc77 = get_safe_token(device);

	abc77->keylatch = data & 0x0f;

	if (abc77->keylatch == 1)
	{
		watchdog_reset(device->machine);
	}

	/* beep */
	discrete_sound_w(discrete, NODE_01, BIT(data, 4));

	/* transmit data */
	devcb_call_write_line(&abc77->out_txd_func, BIT(data, 5));
	abc77->txd = BIT(data, 5);

	/* key down */
	devcb_call_write_line(&abc77->out_keydown_func, BIT(data, 6));

	/* hysteresis */
	abc77->hys = BIT(data, 7);
}

/*-------------------------------------------------
    ADDRESS_MAP( abc77_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( abc77_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x000, 0xfff) AM_ROM
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( abc77_io_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( abc77_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x3f) AM_RAM
	AM_RANGE(MCS48_PORT_P1, MCS48_PORT_P1) AM_READ(abc77_data_r)
	AM_RANGE(MCS48_PORT_P2, MCS48_PORT_P2) AM_WRITE(abc77_data_w)
	AM_RANGE(MCS48_PORT_T1, MCS48_PORT_T1) AM_READ(abc77_clock_r)
	AM_RANGE(MCS48_PORT_BUS, MCS48_PORT_BUS) AM_READ_PORT("ABC77_DSW")
ADDRESS_MAP_END

/*-------------------------------------------------
    INPUT_PORTS( abc77 )
-------------------------------------------------*/

INPUT_PORTS_START( abc77 )
	PORT_START("ABC77_X0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CAPS LOCK") PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK)) PORT_TOGGLE
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RIGHT SHIFT") PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("LEFT SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)

	PORT_START("ABC77_X1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("ABC77_X2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('<') PORT_CHAR('>')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CHAR('\r')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x92") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))

	PORT_START("ABC77_X3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR(0x00E9) PORT_CHAR(0x00C9)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('+') PORT_CHAR('?')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR(0x00E5) PORT_CHAR(0x00C5)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(0x00FC) PORT_CHAR(0x00DC)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(0x00E4) PORT_CHAR(0x00C4)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\'') PORT_CHAR('*')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')

	PORT_START("ABC77_X4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR('=')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(0x00F6) PORT_CHAR(0x00D6)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR(':')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('-') PORT_CHAR('_')

	PORT_START("ABC77_X5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('/')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR(';')

	PORT_START("ABC77_X6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')

	PORT_START("ABC77_X7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("4 \xC2\xA4") PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR(0x00A4)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')

	PORT_START("ABC77_X8")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')

	PORT_START("ABC77_X9")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("KEYPAD 9") PORT_CODE(KEYCODE_9_PAD) PORT_CHAR(UCHAR_MAMEKEY(9_PAD))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("KEYPAD +") PORT_CODE(KEYCODE_PLUS_PAD) PORT_CHAR(UCHAR_MAMEKEY(PLUS_PAD))
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("KEYPAD 6") PORT_CODE(KEYCODE_6_PAD) PORT_CHAR(UCHAR_MAMEKEY(6_PAD))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("KEYPAD -") PORT_CODE(KEYCODE_MINUS_PAD) PORT_CHAR(UCHAR_MAMEKEY(MINUS_PAD))
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("KEYPAD 3") PORT_CODE(KEYCODE_3_PAD) PORT_CHAR(UCHAR_MAMEKEY(3_PAD))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("KEYPAD RETURN") PORT_CODE(KEYCODE_ENTER_PAD) PORT_CHAR(UCHAR_MAMEKEY(ENTER_PAD))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("KEYPAD .") PORT_CODE(KEYCODE_DEL_PAD) PORT_CHAR(UCHAR_MAMEKEY(DEL_PAD))

	PORT_START("ABC77_X10")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("KEYPAD 7") PORT_CODE(KEYCODE_7_PAD) PORT_CHAR(UCHAR_MAMEKEY(7_PAD))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("KEYPAD 8") PORT_CODE(KEYCODE_8_PAD) PORT_CHAR(UCHAR_MAMEKEY(8_PAD))
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("KEYPAD 4") PORT_CODE(KEYCODE_4_PAD) PORT_CHAR(UCHAR_MAMEKEY(4_PAD))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("KEYPAD 5") PORT_CODE(KEYCODE_5_PAD) PORT_CHAR(UCHAR_MAMEKEY(5_PAD))
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("KEYPAD 1") PORT_CODE(KEYCODE_1_PAD) PORT_CHAR(UCHAR_MAMEKEY(1_PAD))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("KEYPAD 2") PORT_CODE(KEYCODE_2_PAD) PORT_CHAR(UCHAR_MAMEKEY(2_PAD))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("KEYPAD 0") PORT_CODE(KEYCODE_0_PAD) PORT_CHAR(UCHAR_MAMEKEY(0_PAD))
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("ABC77_X11")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PF1") PORT_CODE(KEYCODE_F1) PORT_CHAR(UCHAR_MAMEKEY(F1))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PF2") PORT_CODE(KEYCODE_F2) PORT_CHAR(UCHAR_MAMEKEY(F2))
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PF3") PORT_CODE(KEYCODE_F3) PORT_CHAR(UCHAR_MAMEKEY(F3))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PF4") PORT_CODE(KEYCODE_F4) PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PF5") PORT_CODE(KEYCODE_F5) PORT_CHAR(UCHAR_MAMEKEY(F5))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PF6") PORT_CODE(KEYCODE_F6) PORT_CHAR(UCHAR_MAMEKEY(F6))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PF7") PORT_CODE(KEYCODE_F7) PORT_CHAR(UCHAR_MAMEKEY(F7))
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PF8") PORT_CODE(KEYCODE_F8) PORT_CHAR(UCHAR_MAMEKEY(F8))

	PORT_START("ABC77_DSW")
	PORT_DIPNAME( 0x01, 0x01, "Keyboard Program" )
	PORT_DIPSETTING(    0x00, "Internal (8048)" )
	PORT_DIPSETTING(    0x01, "External PROM" )
	PORT_DIPNAME( 0x02, 0x00, "Character Set" )
	PORT_DIPSETTING(    0x00, "Swedish" )
	PORT_DIPSETTING(    0x02, "US ASCII" )
	PORT_DIPNAME( 0x04, 0x04, "External PROM" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, "Keyboard Type" )
	PORT_DIPSETTING(    0x00, "Danish" )
	PORT_DIPSETTING(    0x10, DEF_STR( French ) )
	PORT_DIPSETTING(    0x08, DEF_STR( German ) )
	PORT_DIPSETTING(    0x18, DEF_STR( Spanish ) )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED)
INPUT_PORTS_END

/*-------------------------------------------------
    MACHINE_DRIVER( abc77 )
-------------------------------------------------*/

static MACHINE_CONFIG_FRAGMENT( abc77 )
	/* keyboard cpu */
	MDRV_CPU_ADD(I8035_TAG, I8035, XTAL_4_608MHz)
	MDRV_CPU_PROGRAM_MAP(abc77_map)
	MDRV_CPU_IO_MAP(abc77_io_map)

	/* watchdog */
	MDRV_WATCHDOG_TIME_INIT(HZ(XTAL_4_608MHz/(3*5)))

	/* serial clock timer */
	MDRV_TIMER_ADD_PERIODIC("serial", clock_tick, HZ(XTAL_4_608MHz/(3*5)/16))

	/* discrete sound */
	MDRV_SOUND_ADD("discrete", DISCRETE, 0)
	MDRV_SOUND_CONFIG_DISCRETE(abc77)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.80)
MACHINE_CONFIG_END

/*-------------------------------------------------
    ROM( abc77 )
-------------------------------------------------*/

ROM_START( abc77 )
	ROM_REGION( 0x1000, I8035_TAG, 0 )
	ROM_LOAD( "65-02486.z10", 0x0000, 0x0800, NO_DUMP ) /* 2716 ABC55/77 keyboard controller Swedish EPROM */
	ROM_LOAD( "keyboard.z14", 0x0800, 0x0800, NO_DUMP ) /* 2716 ABC55/77 keyboard controller non-Swedish EPROM */
	ROM_FILL( 0, 0x1000, 0 )
ROM_END

/*-------------------------------------------------
    abc77_rxd_w - keyboard serial data in
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( abc77_rxd_w )
{
	abc77_t *abc77 = get_safe_token(device);

	cpu_set_input_line(abc77->cpu, MCS48_INPUT_IRQ, state ? CLEAR_LINE : ASSERT_LINE);
}

/*-------------------------------------------------
    abc77_txd_r - keyboard serial data out
-------------------------------------------------*/

READ_LINE_DEVICE_HANDLER( abc77_txd_r )
{
	abc77_t *abc77 = get_safe_token(device);

	return abc77->txd;
}

/*-------------------------------------------------
    abc77_reset_w - keyboard reset
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( abc77_reset_w )
{
	abc77_t *abc77 = get_safe_token(device);

	if (abc77->reset && !state)
	{
		int t = 1.1 * RES_K(100) * CAP_N(100) * 1000; // t = 1.1 * R1 * C1
		int ea = BIT(input_port_read(device->machine, "ABC77_DSW"), 7);

		/* trigger reset */
		cpu_set_input_line(abc77->cpu, INPUT_LINE_RESET, ASSERT_LINE);
		timer_adjust_oneshot(abc77->reset_timer, ATTOTIME_IN_MSEC(t), 0);

		cpu_set_input_line(abc77->cpu, MCS48_INPUT_EA, ea ? CLEAR_LINE : ASSERT_LINE);
	}

	abc77->reset = state;
}

/*-------------------------------------------------
    DEVICE_START( abc77 )
-------------------------------------------------*/

static DEVICE_START( abc77 )
{
	abc77_t *abc77 = (abc77_t *)downcast<legacy_device_base *>(device)->token();
	const abc77_interface *intf = get_interface(device);

	/* resolve callbacks */
	devcb_resolve_write_line(&abc77->out_txd_func, &intf->out_txd_func, device);
	devcb_resolve_write_line(&abc77->out_clock_func, &intf->out_clock_func, device);
	devcb_resolve_write_line(&abc77->out_keydown_func, &intf->out_keydown_func, device);

	/* find our CPU */
	abc77->cpu = device->subdevice(I8035_TAG);

	/* allocate reset timer */
	abc77->reset_timer = timer_alloc(device->machine, reset_tick, (FPTR *) device);

	/* register for state saving */
	state_save_register_device_item(device, 0, abc77->keylatch);
	state_save_register_device_item(device, 0, abc77->clock);
	state_save_register_device_item(device, 0, abc77->hys);
	state_save_register_device_item(device, 0, abc77->txd);
	state_save_register_device_item(device, 0, abc77->reset);
}

/*-------------------------------------------------
    DEVICE_GET_INFO( abc77 )
-------------------------------------------------*/

DEVICE_GET_INFO( abc77 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(abc77_t);					break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = rom_abc77;				break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(abc77); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(abc77);		break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							/* Nothing */								break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Luxor ABC-77");			break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Luxor ABC");				break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team"); break;
	}
}

DEFINE_LEGACY_DEVICE(ABC77, abc77);
