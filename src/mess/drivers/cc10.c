/***************************************************************************

        Fidelity Chess Challenger 10

        More data :
            http://home.germany.net/nils.eilers/cc10.htm

        30/08/2010 Skeleton driver

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/i8255a.h"
#include "debugger.h"
#include "sound/beep.h"
#include "cc10.lh"

class cc10_state : public driver_device
{
public:
	cc10_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 kb_matrix;
	UINT8 out_index;
	UINT8 out_state;

	device_t *beep;
};

static ADDRESS_MAP_START(cc10_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x0fff) AM_ROM
	AM_RANGE(0x3000, 0x31ff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( cc10_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0xf4, 0xf7) AM_DEVREADWRITE("i8255", i8255a_r, i8255a_w)
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( cc10 )
	PORT_START("LEVEL")
		PORT_CONFNAME( 0x80, 0x00, "Number of levels" )
		PORT_CONFSETTING( 0x00, "10" )
		PORT_CONFSETTING( 0x80, "3" )

	PORT_START("LINE1")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("RE") PORT_CODE(KEYCODE_R)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("LV") PORT_CODE(KEYCODE_V)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("A1") PORT_CODE(KEYCODE_1)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("E5") PORT_CODE(KEYCODE_5)

	PORT_START("LINE2")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("CB") PORT_CODE(KEYCODE_C)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("DM") PORT_CODE(KEYCODE_D)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("B2") PORT_CODE(KEYCODE_2)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("F6") PORT_CODE(KEYCODE_6)

	PORT_START("LINE3")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("CL") PORT_CODE(KEYCODE_L)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("PB") PORT_CODE(KEYCODE_B)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("C3") PORT_CODE(KEYCODE_3)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("G7") PORT_CODE(KEYCODE_7)

	PORT_START("LINE4")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("EN") PORT_CODE(KEYCODE_E)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("PV") PORT_CODE(KEYCODE_O)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("D4") PORT_CODE(KEYCODE_4)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("H8") PORT_CODE(KEYCODE_8)
INPUT_PORTS_END

static MACHINE_START(cc10)
{
	cc10_state *state = machine->driver_data<cc10_state>();

	state->beep = machine->device("beep");
}

static MACHINE_RESET(cc10)
{
	cc10_state *state = machine->driver_data<cc10_state>();

	state->kb_matrix = 0;
	state->out_index = 0;
	state->out_state = 0;
}

static WRITE8_DEVICE_HANDLER( cc10_out_data_w )
{
	cc10_state *state = device->machine->driver_data<cc10_state>();
	UINT8 digit_value = BITSWAP8( data,7,0,1,2,3,4,5,6 ) & 0x7f;

	switch (state->out_index)
	{
		case 0x00:
			output_set_digit_value(0, digit_value);
			break;
		case 0x04:
			output_set_digit_value(1, digit_value);
			break;
		case 0x08:
			output_set_digit_value(2, digit_value);
			break;
		case 0x10:
			output_set_digit_value(3, digit_value);
			break;
		case 0x20:
			beep_set_state(state->beep, (data & 0x80) ? 0 : 1);
			break;
		case 0x01:
		case 0x02:
		case 0x40:
		case 0x80:
			// unknown, probably two bit are used for the LEDs
			break;
	}
}

static WRITE8_DEVICE_HANDLER( cc10_out_index_w )
{
	cc10_state *state = device->machine->driver_data<cc10_state>();

	if (!state->out_state)
		state->out_index = data;

	state->out_state = data;
}

static READ8_DEVICE_HANDLER( cc10_kb_r )
{
	cc10_state *state = device->machine->driver_data<cc10_state>();

	UINT8 data = 0;

	switch (state->kb_matrix & 0xf0)
	{
		case 0xe0:
			data = input_port_read(device->machine, "LINE1");
			break;
		case 0xd0:
			data = input_port_read(device->machine, "LINE2");
			break;
		case 0xb0:
			data = input_port_read(device->machine, "LINE3");
			break;
		case 0x70:
			data = input_port_read(device->machine, "LINE4");
			break;
	}

	return data;
}

static WRITE8_DEVICE_HANDLER( cc10_kb_matrix_w )
{
	cc10_state *state = device->machine->driver_data<cc10_state>();

	state->kb_matrix = data;
}

static READ8_DEVICE_HANDLER( cc10_level_r )
{
	/*
	x--- ---- level select
	-xxx xxxx ??
	*/
	return input_port_read(device->machine, "LEVEL");
}

static const i8255a_interface cc10_i8255a_intf =
{
	DEVCB_NULL,
	DEVCB_HANDLER(cc10_level_r),
	DEVCB_HANDLER(cc10_kb_r),
	DEVCB_HANDLER(cc10_out_data_w),
	DEVCB_HANDLER(cc10_out_index_w),
	DEVCB_HANDLER(cc10_kb_matrix_w)
};

static MACHINE_CONFIG_START( cc10, cc10_state )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu", Z80, XTAL_16MHz / 4)
    MDRV_CPU_PROGRAM_MAP(cc10_mem)
	MDRV_CPU_IO_MAP(cc10_io)

	MDRV_MACHINE_START(cc10)
    MDRV_MACHINE_RESET(cc10)

	MDRV_I8255A_ADD("i8255", cc10_i8255a_intf)

    /* video hardware */
	MDRV_DEFAULT_LAYOUT(layout_cc10)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO( "mono" )
	MDRV_SOUND_ADD( "beep", BEEP, 0 )
	MDRV_SOUND_ROUTE( ALL_OUTPUTS, "mono", 1.00 )
MACHINE_CONFIG_END


/* ROM definition */
ROM_START( cc10 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "cc10.bin",   0x0000, 0x1000, CRC(bb9e6055) SHA1(18276e57cf56465a6352239781a828c5f3d5ba63))
ROM_END

/* Driver */

/*   YEAR  NAME    PARENT  COMPAT   MACHINE  INPUT  INIT        COMPANY   FULLNAME       FLAGS */
COMP( 1978, cc10,  0,       0,	cc10,	cc10,	 0, 	  "Fidelity",   "Chess Challenger 10",		GAME_NOT_WORKING | GAME_NO_SOUND)
