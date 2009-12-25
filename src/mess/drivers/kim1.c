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

#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "sound/dac.h"
#include "machine/6530miot.h"
#include "devices/cassette.h"
#include "formats/kim1_cas.h"
#include "kim1.lh"


static UINT8		kim1_u2_port_b;
static UINT8		kim1_311_output;
static UINT32		kim1_cassette_high_count;
static UINT8		kim1_led_time[6];


static ADDRESS_MAP_START ( kim1_map , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x03ff) AM_MIRROR(0xe000) AM_RAM
	AM_RANGE(0x1700, 0x173f) AM_MIRROR(0xe000) AM_DEVREADWRITE("miot_u3", miot6530_r, miot6530_w )
	AM_RANGE(0x1740, 0x177f) AM_MIRROR(0xe000) AM_DEVREADWRITE("miot_u2", miot6530_r, miot6530_w )
	AM_RANGE(0x1780, 0x17bf) AM_MIRROR(0xe000) AM_RAM
	AM_RANGE(0x17c0, 0x17ff) AM_MIRROR(0xe000) AM_RAM
	AM_RANGE(0x1800, 0x1bff) AM_MIRROR(0xe000) AM_ROM
	AM_RANGE(0x1c00, 0x1fff) AM_MIRROR(0xe000) AM_ROM
ADDRESS_MAP_END


static INPUT_CHANGED( kim1_reset )
{
	if (newval == 0)
		device_reset(field->port->machine->firstcpu);
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


static UINT8 kim1_u2_read_a(const device_config *device, UINT8 olddata)
{
	UINT8	data = 0xff;

	switch( ( kim1_u2_port_b >> 1 ) & 0x0f )
	{
	case 0:
		data = input_port_read(device->machine, "LINE0");
		break;
	case 1:
		data = input_port_read(device->machine, "LINE1");
		break;
	case 2:
		data = input_port_read(device->machine, "LINE2");
		break;
	}
	return data;
}


static void kim1_u2_write_a(const device_config *device, UINT8 newdata, UINT8 olddata)
{
	UINT8 idx = ( kim1_u2_port_b >> 1 ) & 0x0f;

	if ( idx >= 4 && idx < 10 )
	{
		if ( newdata & 0x80 )
		{
			output_set_digit_value( idx-4, newdata & 0x7f );
			kim1_led_time[idx - 4] = 15;
		}
	}
}


static UINT8 kim1_u2_read_b(const device_config *device, UINT8 olddata)
{
	if ( miot6530_portb_out_get(device) & 0x20 )
		return 0xFF;

	return 0x7F | ( kim1_311_output ^ 0x80 );
}


static void kim1_u2_write_b(const device_config *device, UINT8 newdata, UINT8 olddata)
{
	kim1_u2_port_b = newdata;

	if ( newdata & 0x20 )
	{
		/* cassette write/speaker update */
		cassette_output( devtag_get_device(device->machine, "cassette"), ( newdata & 0x80 ) ? -1.0 : 1.0 );
	}

	/* Set IRQ when bit 7 is cleared */
}


static const miot6530_interface kim1_u2_miot6530_interface =
{
	kim1_u2_read_a,
	kim1_u2_read_b,
	kim1_u2_write_a,
	kim1_u2_write_b
};


static UINT8 kim1_u3_read_a(const device_config *device, UINT8 olddata)
{
	return 0xFF;
}


static void kim1_u3_write_a(const device_config *device, UINT8 newdata, UINT8 olddata)
{
}


static UINT8 kim1_u3_read_b(const device_config *device, UINT8 olddata)
{
	return 0xFF;
}


static void kim1_u3_write_b(const device_config *device, UINT8 newdata, UINT8 olddata)
{
}


static const miot6530_interface kim1_u3_miot6530_interface =
{
	kim1_u3_read_a,
	kim1_u3_read_b,
	kim1_u3_write_a,
	kim1_u3_write_b
};


static TIMER_CALLBACK( kim1_cassette_input )
{
	double tap_val = cassette_input( devtag_get_device(machine, "cassette") );

	if ( tap_val <= 0 )
	{
		if ( kim1_cassette_high_count )
		{
			kim1_311_output = ( kim1_cassette_high_count < 8 ) ? 0x80 : 0;
			kim1_cassette_high_count = 0;
		}
	}

	if ( tap_val > 0 )
	{
		kim1_cassette_high_count++;
	}
}


static TIMER_CALLBACK( kim1_update_leds )
{
	int i;

	for ( i = 0; i < 6; i++ )
	{
		if ( kim1_led_time[i] )
			kim1_led_time[i]--;
		else
			output_set_digit_value( i, 0 );
	}
}


static MACHINE_START( kim1 )
{
	state_save_register_item(machine, "kim1", NULL, 0, kim1_u2_port_b );
	state_save_register_item(machine, "kim1", NULL, 0, kim1_311_output );
	state_save_register_item(machine, "kim1", NULL, 0, kim1_cassette_high_count );
	timer_pulse(machine,  ATTOTIME_IN_HZ(60), NULL, 0, kim1_update_leds );
	timer_pulse(machine,  ATTOTIME_IN_HZ(44100), NULL, 0, kim1_cassette_input );
}


static MACHINE_RESET( kim1 )
{
	int i;


	for ( i = 0; i < 6; i++ )
	{
		kim1_led_time[i] = 0;
	}

	kim1_311_output = 0;
	kim1_cassette_high_count = 0;
}


static const cassette_config kim1_cassette_config =
{
	kim1_cassette_formats,
	NULL,
	CASSETTE_STOPPED
};


static MACHINE_DRIVER_START( kim1 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", M6502, 1000000)        /* 1 MHz */
	MDRV_CPU_PROGRAM_MAP(kim1_map)
	MDRV_QUANTUM_TIME(HZ(60))

	MDRV_MACHINE_START( kim1 )
	MDRV_MACHINE_RESET( kim1 )

	MDRV_MIOT6530_ADD( "miot_u2", 1000000, kim1_u2_miot6530_interface )
	MDRV_MIOT6530_ADD( "miot_u3", 1000000, kim1_u3_miot6530_interface )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD("dac", DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MDRV_CASSETTE_ADD( "cassette", kim1_cassette_config )

	/* video */
	MDRV_DEFAULT_LAYOUT( layout_kim1 )
MACHINE_DRIVER_END


ROM_START(kim1)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("6530-003.bin",    0x1800, 0x0400, CRC(a2a56502) SHA1(60b6e48f35fe4899e29166641bac3e81e3b9d220))
	ROM_LOAD("6530-002.bin",    0x1c00, 0x0400, CRC(2b08e923) SHA1(054f7f6989af3a59462ffb0372b6f56f307b5362))
ROM_END


/*    YEAR  NAME      PARENT    COMPAT  MACHINE   INPUT     INIT      CONFIG  COMPANY   FULLNAME */
COMP( 1975, kim1,	  0, 		0,		kim1,	  kim1, 	0,		  0,	  "MOS Technologies",  "KIM-1" , GAME_SUPPORTS_SAVE)
