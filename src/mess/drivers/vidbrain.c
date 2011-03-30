/***************************************************************************

    VideoBrain FamilyComputer

    http://www.atariprotos.com/othersystems/videobrain/videobrain.htm
    http://www.seanriddle.com/vbinfo.html
    http://www.seanriddle.com/videobrain.html
    http://www.google.com/patents?q=4232374

    07/04/2009 Skeleton driver.

****************************************************************************/

/*

    TODO:

    - use machine/f3853.h
    - discrete sound
    - joystick scan timer 555
    - reset on cartridge eject
    - expander 1 (cassette, RS-232)
    - expander 2 (modem)

*/

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "includes/vidbrain.h"
#include "cpu/f8/f8.h"
#include "imagedev/cartslot.h"
#include "imagedev/cassette.h"
#include "machine/ram.h"
#include "sound/discrete.h"



//**************************************************************************
//  READ/WRITE HANDLERS
//**************************************************************************

//-------------------------------------------------
//  keyboard_w - keyboard column write
//-------------------------------------------------

WRITE8_MEMBER( vidbrain_state::keyboard_w )
{
	/*

        bit     description

        0       keyboard column 0, sound data 0
        1       keyboard column 1, sound data 1
        2       keyboard column 2
        3       keyboard column 3
        4       keyboard column 4
        5       keyboard column 5
        6       keyboard column 6
        7       keyboard column 7

    */

	m_keylatch = data;
}


//-------------------------------------------------
//  keyboard_r - keyboard row read
//-------------------------------------------------

READ8_MEMBER( vidbrain_state::keyboard_r )
{
	/*

        bit     description

        0       keyboard row 0
        1       keyboard row 1
        2       keyboard row 2
        3       keyboard row 3
        4
        5
        6
        7

    */

	UINT8 data = 0;//input_port_read(machine, "JOY-R");

	if (BIT(m_keylatch, 0)) data |= input_port_read(m_machine, "IO00");
	if (BIT(m_keylatch, 1)) data |= input_port_read(m_machine, "IO01");
	if (BIT(m_keylatch, 2)) data |= input_port_read(m_machine, "IO02");
	if (BIT(m_keylatch, 3)) data |= input_port_read(m_machine, "IO03");
	if (BIT(m_keylatch, 4)) data |= input_port_read(m_machine, "IO04");
	if (BIT(m_keylatch, 5)) data |= input_port_read(m_machine, "IO05");
	if (BIT(m_keylatch, 6)) data |= input_port_read(m_machine, "IO06");
	if (BIT(m_keylatch, 7)) data |= input_port_read(m_machine, "IO07");
	if (!BIT(m_cmd, 4)) data |= input_port_read(m_machine, "UV201-31");

	return data;
}


//-------------------------------------------------
//  sound_w - sound clock write
//-------------------------------------------------

WRITE8_MEMBER( vidbrain_state::sound_w )
{
	/*

        bit     description

        0       sound Q0
        1       sound Q1
        2
        3
        4       sound clock
        5
        6
        7       joystick enable

    */

	// sound clock
	int sound_clk = BIT(data, 7);

	if (!m_sound_clk && sound_clk)
	{
		discrete_sound_w(m_discrete, NODE_01, BIT(m_keylatch, 0));
		discrete_sound_w(m_discrete, NODE_02, BIT(m_keylatch, 1));
	}

	m_sound_clk = sound_clk;

	// joystick enable
	m_joy_enable = BIT(data, 7);
}


//-------------------------------------------------
//  interrupt_check - check interrupts
//-------------------------------------------------

void vidbrain_state::interrupt_check()
{
	int interrupt = CLEAR_LINE;

	switch (m_int_enable)
	{
	case 1:
		if (m_ext_int_latch) interrupt = ASSERT_LINE;
		break;

	case 3:
		if (m_timer_int_latch) interrupt = ASSERT_LINE;
		break;
	}

	device_set_input_line(m_maincpu, F8_INPUT_LINE_INT_REQ, interrupt);
}


//-------------------------------------------------
//  f3853_w - F3853 SMI write
//-------------------------------------------------

WRITE8_MEMBER( vidbrain_state::f3853_w )
{
	switch (offset)
	{
	case 0:
		// interrupt vector address high
		m_vector = (data << 8) | (m_vector & 0xff);
		logerror("F3853 Interrupt Vector %04x\n", m_vector);
		break;

	case 1:
		// interrupt vector address low
		m_vector = (m_vector & 0xff00) | data;
		logerror("F3853 Interrupt Vector %04x\n", m_vector);
		break;

	case 2:
		// interrupt control
		m_int_enable = data & 0x03;
		logerror("F3853 Interrupt Control %u\n", m_int_enable);
		interrupt_check();

		if (m_int_enable == 0x03) fatalerror("F3853 Timer not supported!");
		break;

	case 3:
		// timer 8-bit polynomial counter
		fatalerror("F3853 Timer not supported!");
		break;
	}
}



//**************************************************************************
//  ADDRESS MAPS
//**************************************************************************

//-------------------------------------------------
//  ADDRESS_MAP( vidbrain_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( vidbrain_mem, AS_PROGRAM, 8, vidbrain_state )
	AM_RANGE(0x0000, 0x07ff) AM_MIRROR(0xc000) AM_ROM
	AM_RANGE(0x0800, 0x08ff) AM_READWRITE(vlsi_r, vlsi_w)
	AM_RANGE(0x0c00, 0x0fff) AM_MIRROR(0xe000) AM_RAM
	AM_RANGE(0x1000, 0x1fff) AM_MIRROR(0xe000) AM_ROM
	AM_RANGE(0x2000, 0x27ff) AM_MIRROR(0xc000) AM_ROM
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( vidbrain_io )
//-------------------------------------------------

static ADDRESS_MAP_START( vidbrain_io, AS_IO, 8, vidbrain_state )
	AM_RANGE(0x00, 0x00) AM_WRITE(keyboard_w)
	AM_RANGE(0x01, 0x01) AM_READWRITE(keyboard_r, sound_w)
	AM_RANGE(0x0c, 0x0f) AM_WRITE(f3853_w)
//  AM_RANGE(0x0c, 0x0f) AM_DEVREADWRITE_LEGACY(F3853_TAG, f3853_r, f3853_w)
ADDRESS_MAP_END



//**************************************************************************
//  INPUT PORTS
//**************************************************************************

//-------------------------------------------------
//  INPUT_CHANGED( trigger_reset )
//-------------------------------------------------

static INPUT_CHANGED( trigger_reset )
{
	cputag_set_input_line(field->port->machine(), F3850_TAG, INPUT_LINE_RESET, newval ? CLEAR_LINE : ASSERT_LINE);
}


//-------------------------------------------------
//  INPUT_PORTS( vidbrain )
//-------------------------------------------------

static INPUT_PORTS_START( vidbrain )
	PORT_START("IO00")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('I') PORT_CHAR('"')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('O') PORT_CHAR('#')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("P \xC2\xA2") PORT_CODE(KEYCODE_P) PORT_CHAR('P') PORT_CHAR(0x00a2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("; \xC2\xB6") PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR(0x00b6)

	PORT_START("IO01")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('U') PORT_CHAR('!')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('K') PORT_CHAR(')')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('L') PORT_CHAR('$')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR('\'') PORT_CHAR('*')

	PORT_START("IO02")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Y \xC3\xB7") PORT_CODE(KEYCODE_Y) PORT_CHAR('Y') PORT_CHAR(0x00f7)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('J') PORT_CHAR('(')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M') PORT_CHAR(':')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)

	PORT_START("IO03")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("T \xC3\x97") PORT_CODE(KEYCODE_T) PORT_CHAR('T') PORT_CHAR(0x00d7)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('H') PORT_CHAR('/')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('N') PORT_CHAR(',')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("ERASE RESTART") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)

	PORT_START("IO04")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('R') PORT_CHAR('9')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('G') PORT_CHAR('-')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B') PORT_CHAR('=')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("SPACE RUN/STOP") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')

	PORT_START("IO05")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E') PORT_CHAR('8')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F') PORT_CHAR('6')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('V') PORT_CHAR('+')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("SPECIAL ALARM") PORT_CODE(KEYCODE_F4)

	PORT_START("IO06")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('W') PORT_CHAR('7')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D') PORT_CHAR('5')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C') PORT_CHAR('3')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("NEXT CLOCK") PORT_CODE(KEYCODE_F3)

	PORT_START("IO07")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q') PORT_CHAR('%')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('S') PORT_CHAR('4')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('X') PORT_CHAR('2')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("PREVIOUS COLOR") PORT_CODE(KEYCODE_F2)

	PORT_START("UV201-31")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A') PORT_CHAR('.')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z') PORT_CHAR('1')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('?') PORT_CHAR('0')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("BACK TEXT") PORT_CODE(KEYCODE_F1)

	PORT_START("RESET")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("MASTER CONTROL") PORT_CODE(KEYCODE_F5) PORT_CHAR(UCHAR_MAMEKEY(F5)) PORT_CHANGED(trigger_reset, 0)

	PORT_START("JOY1-X")
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_X ) PORT_SENSITIVITY(25) PORT_PLAYER(1)

	PORT_START("JOY1-Y")
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y ) PORT_SENSITIVITY(25) PORT_PLAYER(1)

	PORT_START("JOY2-X")
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_X ) PORT_SENSITIVITY(25) PORT_PLAYER(2)

	PORT_START("JOY2-Y")
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y ) PORT_SENSITIVITY(25) PORT_PLAYER(2)

	PORT_START("JOY3-X")
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_X ) PORT_SENSITIVITY(25) PORT_PLAYER(3)

	PORT_START("JOY3-Y")
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y ) PORT_SENSITIVITY(25) PORT_PLAYER(3)

	PORT_START("JOY4-X")
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_X ) PORT_SENSITIVITY(25) PORT_PLAYER(4)

	PORT_START("JOY4-Y")
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y ) PORT_SENSITIVITY(25) PORT_PLAYER(4)

	PORT_START("JOY-R")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
INPUT_PORTS_END



//**************************************************************************
//  SOUND
//**************************************************************************

//-------------------------------------------------
//  DISCRETE_SOUND( vidbrain )
//-------------------------------------------------

static DISCRETE_SOUND_START( vidbrain )
	DISCRETE_INPUT_LOGIC(NODE_01)
	DISCRETE_INPUT_LOGIC(NODE_02)
	DISCRETE_OUTPUT(NODE_02, 5000)
DISCRETE_SOUND_END



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  f3853_interface smi_intf
//-------------------------------------------------
/*
static void f3853_int_req_w(device_t *device, UINT16 addr, int level)
{
    device_set_input_line_vector(device->machine().device(F3850_TAG), 0, addr);

    cputag_set_input_line(device->machine(), F3850_TAG, F8_INPUT_LINE_INT_REQ, level);
}

static const f3853_interface smi_intf =
{
    f3853_int_req_w
};
*/


//-------------------------------------------------
//  cassette_config vidbrain_cassette_config
//-------------------------------------------------

static const cassette_config vidbrain_cassette_config =
{
	cassette_default_formats,
	NULL,
	(cassette_state)(CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_MUTED),
	NULL
};


//**************************************************************************
//  MACHINE INITIALIZATION
//**************************************************************************

//-------------------------------------------------
//  IRQ_CALLBACK( vidbrain_int_ack )
//-------------------------------------------------

static IRQ_CALLBACK( vidbrain_int_ack )
{
	vidbrain_state *state = device->machine().driver_data<vidbrain_state>();

	switch (state->m_int_enable)
	{
	case 1:
		state->m_ext_int_latch = 0;
		break;

	case 3:
		state->m_timer_int_latch = 0;
		break;
	}

	state->interrupt_check();

	return state->m_vector;
}


//-------------------------------------------------
//  MACHINE_START( vidbrain )
//-------------------------------------------------

void vidbrain_state::machine_start()
{
	// register IRQ callback
	device_set_irq_callback(m_maincpu, vidbrain_int_ack);

	// register for state saving
	state_save_register_global(m_machine, m_vector);
	state_save_register_global(m_machine, m_int_enable);
	state_save_register_global(m_machine, m_ext_int_latch);
	state_save_register_global(m_machine, m_timer_int_latch);
	state_save_register_global(m_machine, m_keylatch);
	state_save_register_global(m_machine, m_joy_enable);
	state_save_register_global(m_machine, m_sound_clk);
}



//**************************************************************************
//  MACHINE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  MACHINE_CONFIG( vidbrain )
//-------------------------------------------------

static MACHINE_CONFIG_START( vidbrain, vidbrain_state )
    // basic machine hardware
    MCFG_CPU_ADD(F3850_TAG, F8, XTAL_14_31818MHz/8)
    MCFG_CPU_PROGRAM_MAP(vidbrain_mem)
    MCFG_CPU_IO_MAP(vidbrain_io)

	// video hardware
	MCFG_FRAGMENT_ADD(vidbrain_video)

	// sound hardware
	MCFG_SPEAKER_STANDARD_MONO("mono")

	MCFG_SOUND_ADD(DISCRETE_TAG, DISCRETE, 0)
	MCFG_SOUND_CONFIG_DISCRETE(vidbrain)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.80)

	// devices
//  MCFG_F3853_ADD(F3853_TAG, XTAL_14_31818MHz/8, smi_intf)
	MCFG_CASSETTE_ADD(CASSETTE_TAG, vidbrain_cassette_config)

	// cartridge
	MCFG_CARTSLOT_ADD("cart")
	MCFG_CARTSLOT_EXTENSION_LIST("bin")
	MCFG_CARTSLOT_INTERFACE("vidbrain_cart")

	// software lists
	MCFG_SOFTWARE_LIST_ADD("cart_list", "vidbrain")

	// internal ram
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("1K")
MACHINE_CONFIG_END



//**************************************************************************
//  ROMS
//**************************************************************************

//-------------------------------------------------
//  ROM( vidbrain )
//-------------------------------------------------

ROM_START( vidbrain )
    ROM_REGION( 0x3000, F3850_TAG, 0 )
	ROM_LOAD( "uvres 1n.d67", 0x0000, 0x0800, CRC(065fe7c2) SHA1(9776f9b18cd4d7142e58eff45ac5ee4bc1fa5a2a) )
	ROM_CART_LOAD( "cart", 0x1000, 0x1000, 0 )
	ROM_LOAD( "resn2.e5",     0x2000, 0x0800, CRC(1d85d7be) SHA1(26c5a25d1289dedf107fa43aa8dfc14692fd9ee6) )
ROM_END



//**************************************************************************
//  SYSTEM DRIVERS
//**************************************************************************

//    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT       INIT    COMPANY                         FULLNAME                        FLAGS
COMP( 1977, vidbrain,	0,		0,		vidbrain,	vidbrain,	0,		"VideoBrain Computer Company",	"VideoBrain FamilyComputer",	GAME_NOT_WORKING | GAME_NO_SOUND )
