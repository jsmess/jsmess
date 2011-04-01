/******************************************************************************
    KIM-1

    system driver

    Juergen Buchmueller, Oct 1999

    KIM-1 memory map

    range     short     description
    0000-03FF RAM
    1400-16FF ???
    1700-173F 6530-003  see 6530
    1740-177F 6530-002  see 6530
    1780-17BF RAM       internal 6530-003
    17C0-17FF RAM       internal 6530-002
    1800-1BFF ROM       internal 6530-003
    1C00-1FFF ROM       internal 6530-002

    6530
    offset  R/W short   purpose
    0       X   DRA     Data register A
    1       X   DDRA    Data direction register A
    2       X   DRB     Data register B
    3       X   DDRB    Data direction register B
    4       W   CNT1T   Count down from value, divide by 1, disable IRQ
    5       W   CNT8T   Count down from value, divide by 8, disable IRQ
    6       W   CNT64T  Count down from value, divide by 64, disable IRQ
            R   LATCH   Read current counter value, disable IRQ
    7       W   CNT1KT  Count down from value, divide by 1024, disable IRQ
            R   STATUS  Read counter statzs, bit 7 = 1 means counter overflow
    8       X   DRA     Data register A
    9       X   DDRA    Data direction register A
    A       X   DRB     Data register B
    B       X   DDRB    Data direction register B
    C       W   CNT1I   Count down from value, divide by 1, enable IRQ
    D       W   CNT8I   Count down from value, divide by 8, enable IRQ
    E       W   CNT64I  Count down from value, divide by 64, enable IRQ
            R   LATCH   Read current counter value, enable IRQ
    F       W   CNT1KI  Count down from value, divide by 1024, enable IRQ
            R   STATUS  Read counter statzs, bit 7 = 1 means counter overflow

    6530-002 (U2)
        DRA bit write               read
        ---------------------------------------------
            0-6 segments A-G        key columns 1-7
            7   PTR                 KBD

        DRB bit write               read
        ---------------------------------------------
            0   PTR                 -/-
            1-4 dec 1-3 key row 1-3
                    4   RW3
                    5-9 7-seg   1-6
    6530-003 (U3)
        DRA bit write               read
        ---------------------------------------------
            0-7 bus PA0-7           bus PA0-7

        DRB bit write               read
        ---------------------------------------------
            0-7 bus PB0-7           bus PB0-7


The cassette interface
======================

The KIM-1 stores data on cassette using 2 frequencies: ~3700Hz (high) and ~2400Hz
(low). A high tone is output for 9 cycles and a low tone for 6 cycles. A logic bit
is encoded using 3 sequences of high and low tones. It always starts with a high
tone and ends with a low tone. The middle tone is high for a logic 0 and low for
0 logic 1.

These high and low tone signals are fed to a circuit containing a LM565 PLL and
a 311 comparator. For a high tone a 1 is passed to DB7 of 6530-U2 for a low tone
a 0 is passed. The KIM-1 software measures the time it takes for the signal to
change from 1 to 0.


******************************************************************************/

#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "sound/dac.h"
#include "machine/mos6530.h"
#include "imagedev/cassette.h"
#include "formats/kim1_cas.h"
#include "kim1.lh"


class kim1_state : public driver_device
{
public:
	kim1_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 m_u2_port_b;
	UINT8 m_311_output;
	UINT32 m_cassette_high_count;
	UINT8 m_led_time[6];
};





static ADDRESS_MAP_START ( kim1_map , AS_PROGRAM, 8)
	AM_RANGE(0x0000, 0x03ff) AM_MIRROR(0xe000) AM_RAM
	AM_RANGE(0x1700, 0x173f) AM_MIRROR(0xe000) AM_DEVREADWRITE("miot_u3", mos6530_r, mos6530_w )
	AM_RANGE(0x1740, 0x177f) AM_MIRROR(0xe000) AM_DEVREADWRITE("miot_u2", mos6530_r, mos6530_w )
	AM_RANGE(0x1780, 0x17bf) AM_MIRROR(0xe000) AM_RAM
	AM_RANGE(0x17c0, 0x17ff) AM_MIRROR(0xe000) AM_RAM
	AM_RANGE(0x1800, 0x1bff) AM_MIRROR(0xe000) AM_ROM
	AM_RANGE(0x1c00, 0x1fff) AM_MIRROR(0xe000) AM_ROM
ADDRESS_MAP_END


static INPUT_CHANGED( kim1_reset )
{
	if (newval == 0)
		field->port->machine().firstcpu->reset();
}


static INPUT_PORTS_START( kim1 )
	PORT_START("LINE0")			/* IN0 keys row 0 */
	PORT_BIT( 0x80, 0x00, IPT_UNUSED )
	PORT_BIT( 0x40, 0x40, IPT_KEYBOARD ) PORT_NAME("0.6: 0") PORT_CODE(KEYCODE_0)
	PORT_BIT( 0x20, 0x20, IPT_KEYBOARD ) PORT_NAME("0.5: 1") PORT_CODE(KEYCODE_1)
	PORT_BIT( 0x10, 0x10, IPT_KEYBOARD ) PORT_NAME("0.4: 2") PORT_CODE(KEYCODE_2)
	PORT_BIT( 0x08, 0x08, IPT_KEYBOARD ) PORT_NAME("0.3: 3") PORT_CODE(KEYCODE_3)
	PORT_BIT( 0x04, 0x04, IPT_KEYBOARD ) PORT_NAME("0.2: 4") PORT_CODE(KEYCODE_4)
	PORT_BIT( 0x02, 0x02, IPT_KEYBOARD ) PORT_NAME("0.1: 5") PORT_CODE(KEYCODE_5)
	PORT_BIT( 0x01, 0x01, IPT_KEYBOARD ) PORT_NAME("0.0: 6") PORT_CODE(KEYCODE_6)

	PORT_START("LINE1")			/* IN1 keys row 1 */
	PORT_BIT( 0x80, 0x00, IPT_UNUSED )
	PORT_BIT( 0x40, 0x40, IPT_KEYBOARD ) PORT_NAME("1.6: 7") PORT_CODE(KEYCODE_7)
	PORT_BIT( 0x20, 0x20, IPT_KEYBOARD ) PORT_NAME("1.5: 8") PORT_CODE(KEYCODE_8)
	PORT_BIT( 0x10, 0x10, IPT_KEYBOARD ) PORT_NAME("1.4: 9") PORT_CODE(KEYCODE_9)
	PORT_BIT( 0x08, 0x08, IPT_KEYBOARD ) PORT_NAME("1.3: A") PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x04, 0x04, IPT_KEYBOARD ) PORT_NAME("1.2: B") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x02, 0x02, IPT_KEYBOARD ) PORT_NAME("1.1: C") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x01, 0x01, IPT_KEYBOARD ) PORT_NAME("1.0: D") PORT_CODE(KEYCODE_D)

	PORT_START("LINE2")			/* IN2 keys row 2 */
	PORT_BIT( 0x80, 0x00, IPT_UNUSED )
	PORT_BIT( 0x40, 0x40, IPT_KEYBOARD ) PORT_NAME("2.6: E") PORT_CODE(KEYCODE_E)
	PORT_BIT( 0x20, 0x20, IPT_KEYBOARD ) PORT_NAME("2.5: F") PORT_CODE(KEYCODE_F)
	PORT_BIT( 0x10, 0x10, IPT_KEYBOARD ) PORT_NAME("2.4: AD (F1)") PORT_CODE(KEYCODE_F1)
	PORT_BIT( 0x08, 0x08, IPT_KEYBOARD ) PORT_NAME("2.3: DA (F2)") PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x04, 0x04, IPT_KEYBOARD ) PORT_NAME("2.2: +  (CR)") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT( 0x02, 0x02, IPT_KEYBOARD ) PORT_NAME("2.1: GO (F5)") PORT_CODE(KEYCODE_F5)
	PORT_BIT( 0x01, 0x01, IPT_KEYBOARD ) PORT_NAME("2.0: PC (F6)") PORT_CODE(KEYCODE_F6)

	PORT_START("LINE3")			/* IN3 STEP and RESET keys, MODE switch */
	PORT_BIT( 0x80, 0x00, IPT_UNUSED )
	PORT_BIT( 0x40, 0x40, IPT_KEYBOARD ) PORT_NAME("sw1: ST (F7)") PORT_CODE(KEYCODE_F7)
	PORT_BIT( 0x20, 0x20, IPT_KEYBOARD ) PORT_NAME("sw2: RS (F3)") PORT_CODE(KEYCODE_F3) PORT_CHANGED(kim1_reset, NULL)
	PORT_DIPNAME(0x10, 0x10, "sw3: SS (NumLock)") PORT_CODE(KEYCODE_NUMLOCK) PORT_TOGGLE
	PORT_DIPSETTING( 0x00, "single step")
	PORT_DIPSETTING( 0x10, "run")
	PORT_BIT( 0x08, 0x00, IPT_UNUSED )
	PORT_BIT( 0x04, 0x00, IPT_UNUSED )
	PORT_BIT( 0x02, 0x00, IPT_UNUSED )
	PORT_BIT( 0x01, 0x00, IPT_UNUSED )
INPUT_PORTS_END


static READ8_DEVICE_HANDLER( kim1_u2_read_a )
{
	kim1_state *state = device->machine().driver_data<kim1_state>();
	UINT8	data = 0xff;

	switch( ( state->m_u2_port_b >> 1 ) & 0x0f )
	{
	case 0:
		data = input_port_read(device->machine(), "LINE0");
		break;
	case 1:
		data = input_port_read(device->machine(), "LINE1");
		break;
	case 2:
		data = input_port_read(device->machine(), "LINE2");
		break;
	}
	return data;
}


static WRITE8_DEVICE_HANDLER( kim1_u2_write_a )
{
	kim1_state *state = device->machine().driver_data<kim1_state>();
	UINT8 idx = ( state->m_u2_port_b >> 1 ) & 0x0f;

	if ( idx >= 4 && idx < 10 )
	{
		if ( data & 0x80 )
		{
			output_set_digit_value( idx-4, data & 0x7f );
			state->m_led_time[idx - 4] = 15;
		}
	}
}

static READ8_DEVICE_HANDLER( kim1_u2_read_b )
{
	kim1_state *state = device->machine().driver_data<kim1_state>();
	if ( mos6530_portb_out_get(device) & 0x20 )
		return 0xFF;

	return 0x7F | ( state->m_311_output ^ 0x80 );
}


static WRITE8_DEVICE_HANDLER( kim1_u2_write_b )
{
	kim1_state *state = device->machine().driver_data<kim1_state>();
	state->m_u2_port_b = data;

	if ( data & 0x20 )
	{
		/* cassette write/speaker update */
		cassette_output( device->machine().device("cassette"), ( data & 0x80 ) ? -1.0 : 1.0 );
	}

	/* Set IRQ when bit 7 is cleared */
}


static MOS6530_INTERFACE( kim1_u2_mos6530_interface )
{
	DEVCB_HANDLER(kim1_u2_read_a),
	DEVCB_HANDLER(kim1_u2_write_a),
	DEVCB_HANDLER(kim1_u2_read_b),
	DEVCB_HANDLER(kim1_u2_write_b)
};

static MOS6530_INTERFACE( kim1_u3_mos6530_interface )
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};


static TIMER_DEVICE_CALLBACK( kim1_cassette_input )
{
	kim1_state *state = timer.machine().driver_data<kim1_state>();
	double tap_val = cassette_input( timer.machine().device("cassette") );

	if ( tap_val <= 0 )
	{
		if ( state->m_cassette_high_count )
		{
			state->m_311_output = ( state->m_cassette_high_count < 8 ) ? 0x80 : 0;
			state->m_cassette_high_count = 0;
		}
	}

	if ( tap_val > 0 )
	{
		state->m_cassette_high_count++;
	}
}


static TIMER_DEVICE_CALLBACK( kim1_update_leds )
{
	kim1_state *state = timer.machine().driver_data<kim1_state>();
	int i;

	for ( i = 0; i < 6; i++ )
	{
		if ( state->m_led_time[i] )
			state->m_led_time[i]--;
		else
			output_set_digit_value( i, 0 );
	}
}


static MACHINE_START( kim1 )
{
	kim1_state *state = machine.driver_data<kim1_state>();
	state_save_register_item(machine, "kim1", NULL, 0, state->m_u2_port_b );
	state_save_register_item(machine, "kim1", NULL, 0, state->m_311_output );
	state_save_register_item(machine, "kim1", NULL, 0, state->m_cassette_high_count );
}


static MACHINE_RESET( kim1 )
{
	kim1_state *state = machine.driver_data<kim1_state>();
	int i;


	for ( i = 0; i < 6; i++ )
	{
		state->m_led_time[i] = 0;
	}

	state->m_311_output = 0;
	state->m_cassette_high_count = 0;
}


static const cassette_config kim1_cassette_config =
{
	kim1_cassette_formats,
	NULL,
	(cassette_state)(CASSETTE_STOPPED),
	NULL
};


static MACHINE_CONFIG_START( kim1, kim1_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", M6502, 1000000)        /* 1 MHz */
	MCFG_CPU_PROGRAM_MAP(kim1_map)
	MCFG_QUANTUM_TIME(attotime::from_hz(60))

	MCFG_MACHINE_START( kim1 )
	MCFG_MACHINE_RESET( kim1 )

	MCFG_MOS6530_ADD( "miot_u2", 1000000, kim1_u2_mos6530_interface )
	MCFG_MOS6530_ADD( "miot_u3", 1000000, kim1_u3_mos6530_interface )

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")

	MCFG_SOUND_ADD("dac", DAC, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MCFG_CASSETTE_ADD( "cassette", kim1_cassette_config )

	/* video */
	MCFG_DEFAULT_LAYOUT( layout_kim1 )

	MCFG_TIMER_ADD_PERIODIC("led_timer", kim1_update_leds, attotime::from_hz(60))
	MCFG_TIMER_ADD_PERIODIC("cassette_timer", kim1_cassette_input, attotime::from_hz(44100))
MACHINE_CONFIG_END


ROM_START(kim1)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("6530-003.bin",    0x1800, 0x0400, CRC(a2a56502) SHA1(60b6e48f35fe4899e29166641bac3e81e3b9d220))
	ROM_LOAD("6530-002.bin",    0x1c00, 0x0400, CRC(2b08e923) SHA1(054f7f6989af3a59462ffb0372b6f56f307b5362))
ROM_END


/*    YEAR  NAME      PARENT    COMPAT  MACHINE   INPUT     INIT    COMPANY   FULLNAME */
COMP( 1975, kim1,	  0,		0,		kim1,	  kim1, 	0,		"MOS Technologies",  "KIM-1" , GAME_SUPPORTS_SAVE)
