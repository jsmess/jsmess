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

#include "emu.h"
#include "includes/vidbrain.h"
#include "cpu/f8/f8.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "devices/messram.h"
#include "sound/discrete.h"

/***************************************************************************
    READ/WRITE HANDLERS
***************************************************************************/

/*-------------------------------------------------
    keyboard_w - keyboard column write
-------------------------------------------------*/

static WRITE8_HANDLER( keyboard_w )
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

	vidbrain_state *state = space->machine->driver_data<vidbrain_state>();

	state->keylatch = data;
}

/*-------------------------------------------------
    keyboard_r - keyboard row read
-------------------------------------------------*/

static READ8_HANDLER( keyboard_r )
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

	vidbrain_state *state = space->machine->driver_data<vidbrain_state>();

	UINT8 data = 0;//input_port_read(space->machine, "JOY-R");

	if (BIT(state->keylatch, 0)) data |= input_port_read(space->machine, "IO00");
	if (BIT(state->keylatch, 1)) data |= input_port_read(space->machine, "IO01");
	if (BIT(state->keylatch, 2)) data |= input_port_read(space->machine, "IO02");
	if (BIT(state->keylatch, 3)) data |= input_port_read(space->machine, "IO03");
	if (BIT(state->keylatch, 4)) data |= input_port_read(space->machine, "IO04");
	if (BIT(state->keylatch, 5)) data |= input_port_read(space->machine, "IO05");
	if (BIT(state->keylatch, 6)) data |= input_port_read(space->machine, "IO06");
	if (BIT(state->keylatch, 7)) data |= input_port_read(space->machine, "IO07");
	if (!BIT(state->cmd, 4)) data |= input_port_read(space->machine, "UV201-31");

	return data;
}

/*-------------------------------------------------
    sound_w - sound clock write
-------------------------------------------------*/

static WRITE8_HANDLER( sound_w )
{
	/*

        bit     description

        0
        1
        2
        3
        4       sound clock
        5
        6
        7       joystick enable

    */

	vidbrain_state *state = space->machine->driver_data<vidbrain_state>();

	/* sound clock */
	int sound_clk = BIT(data, 7);
	
	if (!state->sound_clk && sound_clk)
	{
		discrete_sound_w(state->discrete, NODE_01, BIT(state->keylatch, 0));
		discrete_sound_w(state->discrete, NODE_02, BIT(state->keylatch, 1));
	}

	state->sound_clk = sound_clk;

	/* joystick enable */
	state->joy_enable = BIT(data, 7);
}

/*-------------------------------------------------
    vidbrain_interrupt_check - check interrupts
-------------------------------------------------*/

void vidbrain_interrupt_check(running_machine *machine)
{
	vidbrain_state *state = machine->driver_data<vidbrain_state>();

	int interrupt = CLEAR_LINE;

	switch (state->int_enable)
	{
	case 1:
		if (state->ext_int_latch) interrupt = ASSERT_LINE;
		break;

	case 3:
		if (state->timer_int_latch) interrupt = ASSERT_LINE;
		break;
	}

	cputag_set_input_line(machine, F3850_TAG, F8_INPUT_LINE_INT_REQ, interrupt);
}

/*-------------------------------------------------
    f3853_w - F3853 SMI write
-------------------------------------------------*/

static WRITE8_HANDLER( f3853_w )
{
	vidbrain_state *state = space->machine->driver_data<vidbrain_state>();

	switch (offset)
	{
	case 0:
		// interrupt vector address high
		state->vector = (data << 8) | (state->vector & 0xff);
		logerror("F3853 Interrupt Vector %04x\n", state->vector);
		break;

	case 1:
		// interrupt vector address low
		state->vector = (state->vector & 0xff00) | data;
		logerror("F3853 Interrupt Vector %04x\n", state->vector);
		break;

	case 2:
		// interrupt control
		state->int_enable = data & 0x03;
		logerror("F3853 Interrupt Control %u\n", state->int_enable);
		vidbrain_interrupt_check(space->machine);

		if (state->int_enable == 0x03) fatalerror("F3853 Timer not supported!");
		break;

	case 3:
		// timer 8-bit polynomial counter
		fatalerror("F3853 Timer not supported!");
		break;
	}
}

/***************************************************************************
    MEMORY MAPS
***************************************************************************/

/*-------------------------------------------------
    ADDRESS_MAP( vidbrain_mem )
-------------------------------------------------*/

static ADDRESS_MAP_START( vidbrain_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_MIRROR(0xc000) AM_ROM
	AM_RANGE(0x0800, 0x08ff) AM_READWRITE(vidbrain_vlsi_r, vidbrain_vlsi_w)
	AM_RANGE(0x0c00, 0x0fff) AM_MIRROR(0xe000) AM_RAM
	AM_RANGE(0x1000, 0x1fff) AM_MIRROR(0xe000) AM_ROM
	AM_RANGE(0x2000, 0x27ff) AM_MIRROR(0xc000) AM_ROM
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( vidbrain_io )
-------------------------------------------------*/

static ADDRESS_MAP_START( vidbrain_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_WRITE(keyboard_w)
	AM_RANGE(0x01, 0x01) AM_READWRITE(keyboard_r, sound_w)
	AM_RANGE(0x0c, 0x0f) AM_WRITE(f3853_w)
//	AM_RANGE(0x0c, 0x0f) AM_DEVREADWRITE(F3853_TAG, f3853_r, f3853_w)
ADDRESS_MAP_END

/***************************************************************************
    INPUT PORTS
***************************************************************************/

/*-------------------------------------------------
    INPUT_PORTS( vidbrain )
-------------------------------------------------*/

static INPUT_CHANGED( trigger_reset )
{
	cputag_set_input_line(field->port->machine, F3850_TAG, INPUT_LINE_RESET, newval ? CLEAR_LINE : ASSERT_LINE);
}

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

/***************************************************************************
    SOUND
***************************************************************************/

/*-------------------------------------------------
    DISCRETE_SOUND( vidbrain )
-------------------------------------------------*/

static DISCRETE_SOUND_START( vidbrain )
	DISCRETE_INPUT_LOGIC(NODE_01)
	DISCRETE_INPUT_LOGIC(NODE_02)
	DISCRETE_OUTPUT(NODE_02, 5000)
DISCRETE_SOUND_END

/***************************************************************************
    DEVICE CONFIGURATION
***************************************************************************/

/*-------------------------------------------------
    f3853_interface smi_intf
-------------------------------------------------*/
/*
static void f3853_int_req_w(running_device *device, UINT16 addr, int level)
{
	cpu_set_input_line_vector(device->machine->device(F3850_TAG), 0, addr);

	cputag_set_input_line(device->machine, F3850_TAG, F8_INPUT_LINE_INT_REQ, level);
}

static const f3853_interface smi_intf =
{
	f3853_int_req_w
};
*/
/*-------------------------------------------------
    cassette_config vidbrain_cassette_config
-------------------------------------------------*/

static const cassette_config vidbrain_cassette_config =
{
	cassette_default_formats,
	NULL,
	(cassette_state)(CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_MUTED),
	NULL
};

/*-------------------------------------------------
    cart loading (+ software list)
 -------------------------------------------------*/

static DEVICE_IMAGE_LOAD( vidbrain_cart )
{
	UINT32 size;
	UINT8 *ptr = memory_region(image.device().machine, F3850_TAG) + 0x1000;

	if (image.software_entry() == NULL)
	{
		size = image.length();
		if (image.fread(ptr, size) != size)
			return IMAGE_INIT_FAIL;
	}
	else
	{
		size = image.get_software_region_length("rom");
		memcpy(ptr, image.get_software_region("rom"), size);
	}

	return IMAGE_INIT_PASS;
}

/***************************************************************************
    MACHINE INITIALIZATION
***************************************************************************/

static IRQ_CALLBACK( vidbrain_int_ack )
{
	vidbrain_state *state = device->machine->driver_data<vidbrain_state>();

	switch (state->int_enable)
	{
	case 1:
		state->ext_int_latch = 0;
		break;

	case 3:
		state->timer_int_latch = 0;
		break;
	}

	vidbrain_interrupt_check(device->machine);

	return state->vector;
}

/*-------------------------------------------------
    MACHINE_START( vidbrain )
-------------------------------------------------*/

static MACHINE_START( vidbrain )
{
	vidbrain_state *state = machine->driver_data<vidbrain_state>();

	/* register IRQ callback */
	cpu_set_irq_callback(machine->device(F3850_TAG), vidbrain_int_ack);

	/* find devices */
	state->discrete = machine->device(DISCRETE_TAG);

	/* register for state saving */
	state_save_register_global(machine, state->vector);
	state_save_register_global(machine, state->int_enable);
	state_save_register_global(machine, state->ext_int_latch);
	state_save_register_global(machine, state->timer_int_latch);
	state_save_register_global(machine, state->keylatch);
	state_save_register_global(machine, state->joy_enable);
	state_save_register_global(machine, state->sound_clk);
}

/***************************************************************************
    MACHINE DRIVERS
***************************************************************************/

/*-------------------------------------------------
    MACHINE_CONFIG( vidbrain )
-------------------------------------------------*/

static MACHINE_CONFIG_START( vidbrain, vidbrain_state )
    /* basic machine hardware */
    MDRV_CPU_ADD(F3850_TAG, F8, XTAL_14_31818MHz/8)
    MDRV_CPU_PROGRAM_MAP(vidbrain_mem)
    MDRV_CPU_IO_MAP(vidbrain_io)

    MDRV_MACHINE_START(vidbrain)

	/* video hardware */
	MDRV_FRAGMENT_ADD(vidbrain_video)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(DISCRETE_TAG, DISCRETE, 0)
	MDRV_SOUND_CONFIG_DISCRETE(vidbrain)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.80)

	/* devices */
//	MDRV_F3853_ADD(F3853_TAG, XTAL_14_31818MHz/8, smi_intf)
	MDRV_CASSETTE_ADD(CASSETTE_TAG, vidbrain_cassette_config)

	/* cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("bin")
	MDRV_CARTSLOT_INTERFACE("vidbrain_cart")
	MDRV_CARTSLOT_LOAD(vidbrain_cart)

	/* software lists */
	MDRV_SOFTWARE_LIST_ADD("cart_list", "vidbrain")

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("1K")
MACHINE_CONFIG_END

/***************************************************************************
    ROMS
***************************************************************************/

ROM_START( vidbrain )
    ROM_REGION( 0x3000, F3850_TAG, 0 )
	ROM_LOAD( "uvres 1n.d67", 0x0000, 0x0800, CRC(065fe7c2) SHA1(9776f9b18cd4d7142e58eff45ac5ee4bc1fa5a2a) )
	ROM_LOAD( "resn2.e5", 0x2000, 0x0800, CRC(1d85d7be) SHA1(26c5a25d1289dedf107fa43aa8dfc14692fd9ee6) )
ROM_END

/***************************************************************************
    SYSTEM DRIVERS
***************************************************************************/

/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT       INIT    COMPANY                         FULLNAME                        FLAGS */
COMP( 1977, vidbrain,	0,		0,		vidbrain,	vidbrain,	0,		"VideoBrain Computer Company",	"VideoBrain FamilyComputer",	GAME_NOT_WORKING | GAME_NO_SOUND )
