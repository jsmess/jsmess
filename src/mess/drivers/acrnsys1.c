/******************************************************************************

 Acorn System 1 (Microcomputer Kit)

 Skeleton driver

 Driver by Dirk Best

******************************************************************************/

#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "machine/ins8154.h"
#include "machine/74145.h"
#include "acrnsys1.lh"


class acrnsys1_state : public driver_device
{
public:
	acrnsys1_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};



/***************************************************************************
    KEYBOARD HANDLING
***************************************************************************/

static READ8_DEVICE_HANDLER( ins8154_b1_port_a_r )
{
	device_t *ttl74145 = device->machine().device("ic8_7445");
	UINT8 key_line = ttl74145_r(ttl74145, 0, 0);

	switch (key_line)
	{
	case 1 << 0: return input_port_read(device->machine(), "keyboard_0");
	case 1 << 1: return input_port_read(device->machine(), "keyboard_1");
	case 1 << 2: return input_port_read(device->machine(), "keyboard_2");
	case 1 << 3: return input_port_read(device->machine(), "keyboard_3");
	case 1 << 4: return input_port_read(device->machine(), "keyboard_4");
	case 1 << 5: return input_port_read(device->machine(), "keyboard_5");
	case 1 << 6: return input_port_read(device->machine(), "keyboard_6");
	case 1 << 7: return input_port_read(device->machine(), "keyboard_7");
	}

	/* should never reach this */
	return 0xff;
}

static WRITE8_DEVICE_HANDLER( ins8154_b1_port_a_w )
{
	ttl74145_w(device->machine().device("ic8_7445"), 0, data & 0x07);
}


/***************************************************************************
    LED DISPLAY
***************************************************************************/

static WRITE8_DEVICE_HANDLER( acrnsys1_led_segment_w )
{
	device_t *ttl74145 = device->machine().device("ic8_7445");
	UINT8 key_line = ttl74145_r(ttl74145, 0, 0);

	output_set_digit_value(key_line, data);
}


/***************************************************************************
    ADDRESS MAPS
***************************************************************************/

static ADDRESS_MAP_START( acrnsys1_map, AS_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_RAM
	AM_RANGE(0x0e00, 0x0e7f) AM_MIRROR(0x100) AM_DEVREADWRITE("b1", ins8154_r, ins8154_w)
	AM_RANGE(0x0e80, 0x0eff) AM_MIRROR(0x100) AM_RAM
	AM_RANGE(0xf800, 0xffff) AM_ROM
ADDRESS_MAP_END


/***************************************************************************
    INPUT PORTS
***************************************************************************/

static INPUT_PORTS_START( acrnsys1 )
	PORT_START("keyboard_0")
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT(0xc7, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("keyboard_1")
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9) PORT_CHAR('9')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1) PORT_CHAR('1')
	PORT_BIT(0xc7, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("keyboard_2")
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2) PORT_CHAR('2')
	PORT_BIT(0xc7, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("keyboard_3")
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3) PORT_CHAR('3')
	PORT_BIT(0xc7, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("keyboard_4")
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4) PORT_CHAR('4')
	PORT_BIT(0xc7, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("keyboard_5")
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_C) PORT_CHAR('R')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5) PORT_CHAR('5')
	PORT_BIT(0xc7, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("keyboard_6")
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(UTF8_UP) PORT_CODE(KEYCODE_UP)   PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6) PORT_CHAR('6')
	PORT_BIT(0xc7, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("keyboard_7")
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(UTF8_DOWN) PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7) PORT_CHAR('7')
	PORT_BIT(0xc7, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("reset")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RST") PORT_CODE(KEYCODE_F3) PORT_CHAR(UCHAR_MAMEKEY(F3))
	PORT_BIT(0xfe, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("switch")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Switch") PORT_CODE(KEYCODE_F3) PORT_CHAR(UCHAR_MAMEKEY(F1))
	PORT_BIT(0xfe, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("config")
	PORT_CONFNAME( 0x03, 0x00, "Switch connected to")
	PORT_CONFSETTING( 0x00, "IRQ" )
	PORT_CONFSETTING( 0x01, "NMI" )
	PORT_CONFSETTING( 0x02, "RST" )
INPUT_PORTS_END


/***************************************************************************
    MACHINE DRIVERS
***************************************************************************/

static const ins8154_interface ins8154_b1 =
{
	DEVCB_HANDLER(ins8154_b1_port_a_r),
	DEVCB_HANDLER(ins8154_b1_port_a_w),
	DEVCB_NULL,
	DEVCB_HANDLER(acrnsys1_led_segment_w),
	DEVCB_NULL
};

static MACHINE_CONFIG_START( acrnsys1, acrnsys1_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", M6502, 1008000)  /* 1.008 MHz */
	MCFG_CPU_PROGRAM_MAP(acrnsys1_map)

	MCFG_DEFAULT_LAYOUT(layout_acrnsys1)

	/* devices */
	MCFG_INS8154_ADD("b1", ins8154_b1)
	MCFG_TTL74145_ADD("ic8_7445", default_ttl74145)
MACHINE_CONFIG_END


/***************************************************************************
    ROM DEFINITIONS
***************************************************************************/

ROM_START( acrnsys1 )
	ROM_REGION(0x10000, "maincpu", 0)
	ROM_LOAD("acrnsys1.bin", 0xfe00, 0x0200, CRC(43dcfc16) SHA1(b987354c55beb5e2ced761970c3339b895a8c09d))
	ROM_COPY("maincpu", 0xfe00, 0xf800, 0x0200)
	ROM_COPY("maincpu", 0xfe00, 0xfa00, 0x0200)
	ROM_COPY("maincpu", 0xfe00, 0xfc00, 0x0200)
ROM_END


/***************************************************************************
    GAME DRIVERS
***************************************************************************/

/*    YEAR  NAME      PARENT COMPAT MACHINE   INPUT     INIT  COMPANY  FULLNAME    FLAGS */
COMP( 1978, acrnsys1, 0,     0,     acrnsys1, acrnsys1, 0,    "Acorn", "System 1", GAME_NOT_WORKING | GAME_NO_SOUND )
