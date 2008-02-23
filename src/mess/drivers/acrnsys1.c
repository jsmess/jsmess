/******************************************************************************

 Acorn System 1 (Microcomputer Kit)

 Skeleton driver

 Driver by Dirk Best

******************************************************************************/

/* Core includes */
#include "driver.h"

/* Components */
#include "machine/ins8154.h"
#include "machine/74145.h"

/* Layout */
#include "acrnsys1.lh"


static UINT8 key_digit;


static READ8_HANDLER( ins8154_b1_port_a_r )
{
	char port[11];
	UINT8 data;
	sprintf(port, "keyboard_%d", key_digit);
	data = readinputportbytag(port);
	logerror("Reading %02x from row %d\n", data, key_digit);
	return data;
}

static WRITE8_HANDLER( ins8154_b1_port_a_w )
{
	ttl74145_0_w(0, data & 0x07);
}

static WRITE8_HANDLER( acrnsys1_led_segment_w )
{
	logerror("led %d segment data: %02x\n", key_digit, data);
	
	output_set_digit_value(key_digit + 0, data);
}

static const ins8154_interface ins8154_b1 =
{
	ins8154_b1_port_a_r,
	NULL,
	ins8154_b1_port_a_w,
	acrnsys1_led_segment_w,
	NULL
};

static void acrnsys1_7445_output_0_w(int state) { if (state) key_digit = 0; }
static void acrnsys1_7445_output_1_w(int state) { if (state) key_digit = 1; }
static void acrnsys1_7445_output_2_w(int state) { if (state) key_digit = 2; }
static void acrnsys1_7445_output_3_w(int state) { if (state) key_digit = 3; }
static void acrnsys1_7445_output_4_w(int state) { if (state) key_digit = 4; }
static void acrnsys1_7445_output_5_w(int state) { if (state) key_digit = 5; }
static void acrnsys1_7445_output_6_w(int state) { if (state) key_digit = 6; }
static void acrnsys1_7445_output_7_w(int state) { if (state) key_digit = 7; }

static const ttl74145_interface ic8_7445 =
{
	acrnsys1_7445_output_0_w,  /* digit and keyboard column 0 */
	acrnsys1_7445_output_1_w,  /* digit and keyboard column 1 */
	acrnsys1_7445_output_2_w,  /* digit and keyboard column 2 */
	acrnsys1_7445_output_3_w,  /* digit and keyboard column 3 */
	acrnsys1_7445_output_4_w,  /* digit and keyboard column 4 */
	acrnsys1_7445_output_5_w,  /* digit and keyboard column 5 */
	acrnsys1_7445_output_6_w,  /* digit and keyboard column 6 */
	acrnsys1_7445_output_7_w,  /* digit and keyboard column 7 */
	NULL,  /* not connected */
	NULL,  /* not connected */
};

static DRIVER_INIT( acrnsys1 )
{
	ins8154_config(0, &ins8154_b1);
	ttl74145_config(0, &ic8_7445);
}

static MACHINE_RESET( acrnsys1 )
{
	ins8154_reset(0);
	ttl74145_reset(0);
}


/******************************************************************************
 Memory Maps
******************************************************************************/


static ADDRESS_MAP_START( acrnsys1_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_RAM
	AM_RANGE(0x0e00, 0x0e7f) AM_MIRROR(0x100) AM_READWRITE(ins8154_0_r, ins8154_0_w)
	AM_RANGE(0x0e80, 0x0eff) AM_MIRROR(0x100) AM_RAM
	AM_RANGE(0xfe00, 0xffff) AM_MIRROR(0x600) AM_ROM
ADDRESS_MAP_END



/******************************************************************************
 Input Ports
******************************************************************************/


static INPUT_PORTS_START( acrnsys1 )
	PORT_START_TAG("keyboard_0")
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT(0xc7, IP_ACTIVE_LOW, IPT_UNUSED)
	
	PORT_START_TAG("keyboard_1")
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9) PORT_CHAR('9')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1) PORT_CHAR('1')
	PORT_BIT(0xc7, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START_TAG("keyboard_2")
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2) PORT_CHAR('2')
	PORT_BIT(0xc7, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START_TAG("keyboard_3")
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3) PORT_CHAR('3')
	PORT_BIT(0xc7, IP_ACTIVE_LOW, IPT_UNUSED)
	
	PORT_START_TAG("keyboard_4")
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4) PORT_CHAR('4')
	PORT_BIT(0xc7, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START_TAG("keyboard_5")
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_C) PORT_CHAR('R')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5) PORT_CHAR('5')
	PORT_BIT(0xc7, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START_TAG("keyboard_6")
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xE2\x86\x91") PORT_CODE(KEYCODE_UP)   PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6) PORT_CHAR('6')
	PORT_BIT(0xc7, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START_TAG("keyboard_7")
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xE2\x86\x93") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7) PORT_CHAR('7')
	PORT_BIT(0xc7, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START_TAG("reset")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RST") PORT_CODE(KEYCODE_F3) PORT_CHAR(UCHAR_MAMEKEY(F3))
	PORT_BIT(0xfe, IP_ACTIVE_LOW, IPT_UNUSED)
	
	PORT_START_TAG("switch")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Switch") PORT_CODE(KEYCODE_F3) PORT_CHAR(UCHAR_MAMEKEY(F1))
	PORT_BIT(0xfe, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START_TAG("config")
	PORT_CONFNAME( 0x03, 0x00, "Switch connected to")
	PORT_CONFSETTING( 0x00, "IRQ" )
	PORT_CONFSETTING( 0x01, "NMI" )
	PORT_CONFSETTING( 0x02, "RST" )
INPUT_PORTS_END



/******************************************************************************
 Machine Drivers
******************************************************************************/


static MACHINE_DRIVER_START( acrnsys1 )
	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, 1008000)  /* 1.008 MHz */
	MDRV_CPU_PROGRAM_MAP(acrnsys1_map, 0)

	MDRV_MACHINE_RESET(acrnsys1)
	
	/* "video" hardware */
	MDRV_DEFAULT_LAYOUT(layout_acrnsys1)
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
MACHINE_DRIVER_END



/******************************************************************************
 ROM Definitions
******************************************************************************/


ROM_START( acrnsys1 )
	ROM_REGION(0x10000, REGION_CPU1, 0)
	ROM_LOAD("acrnsys1.bin", 0xfe00, 0x0200, CRC(43dcfc16) SHA1(b987354c55beb5e2ced761970c3339b895a8c09d))
ROM_END



/******************************************************************************
 System Config
******************************************************************************/


SYSTEM_CONFIG_START( acrnsys1 )
SYSTEM_CONFIG_END



/******************************************************************************
 Drivers
******************************************************************************/


/*    YEAR  NAME      PARENT COMPAT MACHINE   INPUT     INIT        CONFIG    COMPANY  FULLNAME    FLAGS */
COMP( 1978, acrnsys1, 0,     0,     acrnsys1, acrnsys1, acrnsys1,   acrnsys1, "Acorn", "System 1", GAME_NOT_WORKING )
