/***************************************************************************
Mephisto Glasgow 3 S chess computer
Dirk V.
sp_rinter@gmx.de

68000 CPU
64 KB ROM
16 KB RAM
4 Digit LC Display

3* 74LS138  Decoder/Multiplexer
1*74LS74    Dual positive edge triggered D Flip Flop
1*74LS139 1of4 Demultiplexer
1*74LS05    HexInverter
1*NE555     R=100K C=10uF
2*74LS04  Hex Inverter
1*74LS164   8 Bit Shift register
1*74121 Monostable Multivibrator with Schmitt Trigger Inputs
1*74LS20 Dual 4 Input NAND GAte
1*74LS367 3 State Hex Buffers


***************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "glasgow.lh"
#include "sound/beep.h"


static UINT8 lcd_shift_counter;
// static UINT8 led_status;
static UINT8 led7;
static UINT8 key_low,
		key_hi,
		key_select,
		irq_flag,
		lcd_invert,
		key_selector;
static UINT8 board_value;
static UINT16 beeper;

static UINT16 Line18_LED;
static UINT16 Line18_REED;

static UINT8 read_board_flag;

static const char EMP[4] = "EMP";

typedef struct {
	unsigned int field;
	unsigned int x;
	unsigned int y;
	unsigned char piece[4];
} BOARD_FIELD;

static BOARD_FIELD m_board[8][8];

static const BOARD_FIELD start_board[8][8] =
{
	{ { 7,44,434,"WR1"}, { 6,100,434,"WN1"}, { 5,156,434,"WB1"}, { 4,212,434,"WQ1"}, { 3,268,434,"WK"}, { 2,324,434,"WB2"}, { 1,380,434,"WN2"}, { 0,436,434,"WR2"} },
	{ {15,44,378,"WP1"}, {14,100,378,"WP2"}, {13,156,378,"WP3"}, {12,212,378,"WP4"}, {11,268,378,"WP5"}, {10,324,378,"WP6"}, { 9,380,378,"WP7"}, { 8,436,378,"WP8"} },

	{ {23,44,322, "EMP"}, {22,100,322, "EMP"}, {21,156,322, "EMP"}, {20,212,322, "EMP"}, {19,268,322, "EMP"}, {18,324,322, "EMP"}, {17,380,322, "EMP"}, {16,436,322, "EMP"} },
	{ {31,44,266, "EMP"}, {30,100,266, "EMP"}, {29,156,266, "EMP"}, {28,212,266, "EMP"}, {27,268,266, "EMP"}, {26,324,266, "EMP"}, {25,380,266, "EMP"}, {24,436,266, "EMP"} },
	{ {39,44,210, "EMP"}, {38,100,210, "EMP"}, {37,156,210, "EMP"}, {36,212,210, "EMP"}, {35,268,210, "EMP"}, {34,324,210, "EMP"}, {33,380,210, "EMP"}, {32,436,210, "EMP"} },
	{ {47,44,154, "EMP"}, {46,100,154, "EMP"}, {45,156,154, "EMP"}, {44,212,154, "EMP"}, {43,268,154, "EMP"}, {42,324,154, "EMP"}, {41,380,154, "EMP"}, {40,436,154, "EMP"} },

	{ {55,44,100,"BP1"}, {54,100,100,"BP2"}, {53,156,100,"BP3"}, {52,212,100,"BP4"}, {51,268,100,"BP5"}, {50,324,100,"BP6"}, {49,380,100,"BP7"}, {48,436,100,"BP8"} },
	{ {63,44,44 ,"BR1"}, {62,100,44 ,"BN1"}, {61,156,44 ,"BB1"}, {60,212,44 ,"BQ1"}, {59,268,44 ,"BK"}, {58,324,44 ,"BB2"}, {57,380,44 ,"BN2"}, {56,436,44 ,"BR2"} }
};

static void set_board( void )
{
	UINT8 i_AH, i_18;

	for (i_AH = 0; i_AH < 8; i_AH++)
	{
		for (i_18 = 0; i_18 < 8; i_18++)
		{
			// copy start postition to m_board
			m_board[i_18][i_AH] = start_board[i_18][i_AH];
		}
	}
}


static int irq_edge = 0x00;


// Used by Glasgow and Dallas
static WRITE16_HANDLER( write_lcd_gg )
{
	UINT8 lcd_data = data >> 8;

	if (led7 == 0) 
		output_set_digit_value(lcd_shift_counter, lcd_data);

	lcd_shift_counter--;
	lcd_shift_counter &= 3;
//	logerror("LCD Offset = %d Data low = %x \n", offset, lcd_data);
}

static WRITE16_HANDLER( write_beeper )
{
	UINT8 beep_flag;

	lcd_invert = 1;
	beep_flag = data >> 8;
//	if ((beep_flag & 02) == 0) key_selector = 0; else key_selector = 1;
	logerror("Write Beeper = %x \n", data);
	beeper = data;
}

static WRITE16_HANDLER( write_lcd )
{
	UINT8 lcd_data = data >> 8;

	output_set_digit_value(lcd_shift_counter, lcd_invert & 1 ? lcd_data^0xff : lcd_data);
	lcd_shift_counter--;
	lcd_shift_counter &= 3;
	logerror("LCD Offset = %d Data low = %x \n", offset, lcd_data);
}

static WRITE16_HANDLER( write_lcd_flag )
{
	UINT8 lcd_flag;
	lcd_invert = 0;
	lcd_flag=data >> 8;
	//beep_set_state(0, lcd_flag & 1 ? 1 : 0);
	if (lcd_flag == 0) 
		key_selector = 1;

 // The key function in the rom expects after writing to
 // the  a value from the second key row;
	if (lcd_flag != 0) 
		led7 = 255;
	else 
		led7 = 0;

	logerror("LCD Flag 16 = %x \n", data);
}

static WRITE16_HANDLER( write_lcd_flag_gg )
{
	const device_config *speaker = devtag_get_device(space->machine, "beep");
	UINT8 lcd_flag = data >> 8;

	beep_set_state(speaker, lcd_flag & 1 ? 1 : 0);

	if (lcd_flag == 0) 
		key_selector = 1;

	if (lcd_flag != 0) 
		led7 = 255;
	else 
		led7 = 0;

//	logerror("LCD Flag gg = %x \n", lcd_flag);
}

static WRITE16_HANDLER( write_keys )
{
	key_select = data >> 8;
//	logerror("Write Key = %x \n", data);
}

static READ16_HANDLER( read_board )
{
	return 0xff00;	// Mephisto need it for working
}

static WRITE16_HANDLER( write_board )
{
	UINT8 board = data >> 8;

	board_value = board;

	if (board == 0xff) 
		key_selector = 0;
//	The key function in the rom expects after writing to
//	the chess board a value from  the first key row;
	logerror("Write Board = %x \n", data >> 8);
}

static WRITE16_HANDLER( write_board_gg )
{
	UINT8 beep_flag;

	Line18_REED = data >> 8;

	// LED's or REED's ?
	if (read_board_flag)
	{
		Line18_LED = 0;
		read_board_flag = 0;
	}
	else
	{
		Line18_LED = data >> 8;
	}

	//logerror("write_board Line18_LED   = %x \n  ", Line18_LED);
	//logerror("write_board Line18_REED   = %x \n  ",Line18_REED);

	lcd_invert = 1;
	beep_flag = data >> 8;
	//if ((beep_flag & 02) == 0) key_selector = 0; else key_selector = 1;
	//logerror("Write Beeper   = %x \n  ",data);
	beeper = data;
}

static WRITE16_HANDLER( write_irq_flag )
{
	const device_config *speaker = devtag_get_device(space->machine, "beep");

	beep_set_state(speaker, data & 0x100);
	logerror("Write 0x800004 = %x \n", data);
	irq_flag = 1;
	beeper = data;
}

static READ16_HANDLER( read_keys ) // Glasgow, Dallas
{
	UINT16 data = 0x0300;

	key_low = input_port_read(space->machine, "LINE0");
	key_hi = input_port_read(space->machine, "LINE1");

	if (key_select == key_low)
		data = data & 0x100;

	if (key_select == key_hi)
		data = data & 0x200;

	//logerror("Keyboard Offset = %x Data = %x\n  ", offset, data);
	return data;
}

static READ16_HANDLER( read_newkeys16 )  //Amsterdam, Roma
{
	UINT16 data;

	if (key_selector == 0) 
		data = input_port_read(space->machine, "LINE0");
	else 
		data = input_port_read(space->machine, "LINE1");

	logerror("read Keyboard Offset = %x Data = %x Select = %x \n", offset, data, key_selector);
	data = data << 8;
	return data ;
}

static READ16_HANDLER( read_board_gg )
{
	UINT8 i_18, i_AH;
	UINT16 data = 0xff;

	for (i_18 = 0; i_18 < 8; i_18++)
	{
		// Looking for cleared bit in Line18 -> current line
		if (!(Line18_REED & (1 << i_18)))
		{
			// if there is a piece on the field -> set bit in data
			for (i_AH = 0; i_AH < 8; i_AH++)
			{
				if (strcmp ((char *)m_board[i_18][i_AH].piece, EMP))
					data &= ~(1 << i_AH);  // clear bit
			}
		}
	}

	data = data << 8;
	read_board_flag = TRUE;
	logerror("read_board_data = %x \n", data);

//	return 0xff00;   // Mephisto need it for working

	return data;
}

static WRITE16_HANDLER( write_beeper_gg )
{
//  logerror("Write Board   = %x \n  ",data>>8);
	UINT8 i_AH, i_18;
	UINT16 LineAH = 0;
	UINT8 LED;

//	logerror("Write Board   = %x \n  ",data);

	LineAH = data >> 8;

	if (LineAH && Line18_LED)
	{
//	logerror("Line18_LED   = %x \n  ",Line18_LED);
//	logerror("LineAH   = %x \n  ",LineAH);

		for (i_AH = 0; i_AH < 8; i_AH++)
		{
			if (LineAH & (1 << i_AH))
			{
				for (i_18 = 0; i_18 < 8; i_18++)
				{
					if (!(Line18_LED & (1 << i_18)))
					{
//						logerror("i_18   = %d \n  ",i_18);
//						logerror("i_AH   = %d \n  ",i_AH);
//						logerror("LED an:   = %d \n  ",m_board[i_18][i_AH]);

						LED = m_board[i_18][i_AH].field;
						output_set_led_value(LED, 1);
						//  LED on
					}
					else
					{
					//  LED off

					}
				}
			}
		}
	}
	else
	{
		//  No LED  -> all LED's off
		for (i_AH = 0; i_AH < 8; i_AH++)
		{
			for (i_18 = 0; i_18 < 8; i_18++)
			{
				// LED off
				LED = m_board[i_18][i_AH].field;
				output_set_led_value(LED, 0);
			}
		}
	}
}

/*
static READ16_HANDLER(read_test)
{
  logerror("read test Offset = %x Data = %x\n  ",offset,data);
  return 0xffff;    // Mephisto need it for working
}

*/

/*

    *****           32 Bit Read and write Handler   ***********

*/

static WRITE32_HANDLER( write_lcd32 )
{
	UINT8 lcd_data = data >> 8;

	output_set_digit_value(lcd_shift_counter, lcd_invert & 1 ? lcd_data^0xff : lcd_data);
	lcd_shift_counter--;
	lcd_shift_counter &= 3;
	//logerror("LCD Offset = %d Data   = %x \n  ", offset, lcd_data);
}

static WRITE32_HANDLER( write_lcd_flag32 )
{
	UINT8 lcd_flag = data >> 24;

	lcd_invert = 0;

	if (lcd_flag == 0) 
		key_selector = 1;

	logerror("LCD Flag 32 = %x \n", lcd_flag);
	//beep_set_state(0, lcd_flag & 1 ? 1 : 0);

	if (lcd_flag != 0) 
		led7 = 255;
	else 
		led7 = 0;
}


static WRITE32_HANDLER( write_keys32 )
{
	lcd_invert = 1;
	key_select = data;
	logerror("Write Key = %x \n", key_select);
}

static READ32_HANDLER( read_newkeys32 ) // Dallas 32, Roma 32
{
	UINT32 data;

	if (key_selector == 0) 
		data = input_port_read(space->machine, "LINE0");
	else 
		data = input_port_read(space->machine, "LINE1");
	//if (key_selector == 1) data = input_port_read(machine, "LINE0"); else data = 0;
	logerror("read Keyboard Offset = %x Data = %x\n", offset, data);
	data = data << 24;
	return data ;
}

static READ32_HANDLER( read_board32 )
{
	logerror("read board 32 Offset = %x \n", offset);
	return 0x00000000;
}

/*
static READ16_HANDLER(read_board_amsterd)
{
  logerror("read board amsterdam Offset = %x \n  ", offset);
  return 0xffff;
}
*/

static WRITE32_HANDLER( write_board32 )
{
	UINT8 board;
	board = data >> 24;
	if (board == 0xff) 
		key_selector = 0;
	logerror("Write Board = %x \n", data);
}


static WRITE32_HANDLER ( write_beeper32 )
{
	const device_config *speaker = devtag_get_device(space->machine, "beep");
	beep_set_state(speaker, data & 0x01000000);
	logerror("Write 0x8000004 = %x \n", data);
	irq_flag = 1;
	beeper = data;
}

static TIMER_CALLBACK( update_nmi )
{
	//cputag_set_input_line_and_vector(machine, "maincpu", M68K_IRQ_7, irq_edge & 0xff ? CLEAR_LINE : ASSERT_LINE, M68K_INT_ACK_AUTOVECTOR);
		cputag_set_input_line_and_vector(machine, "maincpu", M68K_IRQ_7, ASSERT_LINE, M68K_INT_ACK_AUTOVECTOR);
		cputag_set_input_line_and_vector(machine, "maincpu", M68K_IRQ_7, CLEAR_LINE, M68K_INT_ACK_AUTOVECTOR);
   	irq_edge = ~irq_edge;
}

static TIMER_CALLBACK( update_nmi32 )
{
	// cputag_set_input_line_and_vector(machine, "maincpu", M68K_IRQ_7, irq_edge & 0xff ? CLEAR_LINE : ASSERT_LINE, M68K_INT_ACK_AUTOVECTOR);
		cputag_set_input_line_and_vector(machine, "maincpu", M68K_IRQ_7, ASSERT_LINE, M68K_INT_ACK_AUTOVECTOR);
		cputag_set_input_line_and_vector(machine, "maincpu", M68K_IRQ_7, CLEAR_LINE, M68K_INT_ACK_AUTOVECTOR);
    irq_edge = ~irq_edge;
}

static MACHINE_START( glasgow )
{
	const device_config *speaker = devtag_get_device(machine, "beep");

	key_selector = 0;
	irq_flag = 0;
	lcd_shift_counter = 3;
	timer_pulse(machine, ATTOTIME_IN_HZ(50), NULL, 0, update_nmi);
	beep_set_frequency(speaker, 44);
}


static MACHINE_START( dallas32 )
{
	const device_config *speaker = devtag_get_device(machine, "beep");

	lcd_shift_counter = 3;
	timer_pulse(machine, ATTOTIME_IN_HZ(50), NULL, 0, update_nmi32);
	beep_set_frequency(speaker, 44);
}


static MACHINE_RESET( glasgow )
{
	lcd_shift_counter = 3;
	set_board();
}


static ADDRESS_MAP_START(glasgow_mem, ADDRESS_SPACE_PROGRAM, 16)
	AM_RANGE(0x00000000, 0x0000ffff) AM_ROM
	AM_RANGE(0x00ff0000, 0x00ff0001) AM_MIRROR(0xfe0000) AM_WRITE( write_lcd_gg )
	AM_RANGE(0x00ff0002, 0x00ff0003) AM_MIRROR(0xfe0002) AM_READWRITE( read_keys,write_keys )
	AM_RANGE(0x00ff0004, 0x00ff0005) AM_MIRROR(0xfe0004) AM_WRITE( write_lcd_flag_gg )
	AM_RANGE(0x00ff0006, 0x00ff0007) AM_MIRROR(0xfe0006) AM_READWRITE( read_board_gg, write_beeper_gg )
	AM_RANGE(0x00ff0008, 0x00ff0009) AM_MIRROR(0xfe0008) AM_WRITE( write_board_gg )
	AM_RANGE(0x00ffc000, 0x00ffffff) AM_MIRROR(0xfec000) AM_RAM		// 16KB
ADDRESS_MAP_END


static ADDRESS_MAP_START(amsterd_mem, ADDRESS_SPACE_PROGRAM, 16)
	// ADDRESS_MAP_GLOBAL_MASK(0x7FFFF)
	AM_RANGE(0x00000000, 0x0000ffff) AM_ROM
	AM_RANGE(0x00800002, 0x00800003) AM_WRITE( write_lcd )
	AM_RANGE(0x00800008, 0x00800009) AM_WRITE( write_lcd_flag )
	AM_RANGE(0x00800004, 0x00800005) AM_WRITE( write_irq_flag )
	AM_RANGE(0x00800010, 0x00800011) AM_WRITE( write_board )
	AM_RANGE(0x00800020, 0x00800021) AM_READ( read_board )
	AM_RANGE(0x00800040, 0x00800041) AM_READ( read_newkeys16 )
	AM_RANGE(0x00800088, 0x00800089) AM_WRITE( write_beeper )
	AM_RANGE(0x00ffc000, 0x00ffffff) AM_RAM		// 16KB
ADDRESS_MAP_END


static ADDRESS_MAP_START(dallas32_mem, ADDRESS_SPACE_PROGRAM, 32)
	// ADDRESS_MAP_GLOBAL_MASK(0x1FFFF)
	AM_RANGE(0x00000000, 0x0000ffff) AM_ROM
	AM_RANGE(0x00800000, 0x00800003) AM_WRITE( write_lcd32 )
	AM_RANGE(0x00800004, 0x00800007) AM_WRITE( write_beeper32 )
	AM_RANGE(0x00800008, 0x0080000B) AM_WRITE( write_lcd_flag32 )
	AM_RANGE(0x00800010, 0x00800013) AM_WRITE( write_board32 )
	AM_RANGE(0x00800020, 0x00800023) AM_READ( read_board32 )
	AM_RANGE(0x00800040, 0x00800043) AM_READ( read_newkeys32 )
	AM_RANGE(0x00800088, 0x0080008b) AM_WRITE( write_keys32 )
	AM_RANGE(0x0010000, 0x001ffff) AM_RAM	// 64KB
ADDRESS_MAP_END

static INPUT_PORTS_START( new_keyboard ) //Amsterdam, Dallas 32, Roma, Roma 32
	PORT_START("LINE0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("A 1") PORT_CODE(KEYCODE_A) PORT_CODE(KEYCODE_1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("B 2") PORT_CODE(KEYCODE_B) PORT_CODE(KEYCODE_2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("C 3") PORT_CODE(KEYCODE_C) PORT_CODE(KEYCODE_3)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("D 4") PORT_CODE(KEYCODE_D) PORT_CODE(KEYCODE_4)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("E 5") PORT_CODE(KEYCODE_E) PORT_CODE(KEYCODE_5)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F 6") PORT_CODE(KEYCODE_F) PORT_CODE(KEYCODE_6)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_9)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_0)

	PORT_START("LINE1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("INF") PORT_CODE(KEYCODE_F1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("POS") PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("LEV") PORT_CODE(KEYCODE_F3)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("MEM") PORT_CODE(KEYCODE_F4)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CLR") PORT_CODE(KEYCODE_F5)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ENT") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("G 7") PORT_CODE(KEYCODE_G) PORT_CODE(KEYCODE_7)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("H 8") PORT_CODE(KEYCODE_H) PORT_CODE(KEYCODE_8)
INPUT_PORTS_END

static INPUT_PORTS_START( old_keyboard )   //Glasgow,Dallas
	PORT_START("LINE0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CL") PORT_CODE(KEYCODE_F5)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C 3") PORT_CODE(KEYCODE_C) PORT_CODE(KEYCODE_3)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ENT") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D 4") PORT_CODE(KEYCODE_D) PORT_CODE(KEYCODE_4)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A 1") PORT_CODE(KEYCODE_A) PORT_CODE(KEYCODE_1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F 6") PORT_CODE(KEYCODE_F) PORT_CODE(KEYCODE_6)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B 2") PORT_CODE(KEYCODE_B) PORT_CODE(KEYCODE_2)

	PORT_START("LINE1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E 5") PORT_CODE(KEYCODE_E) PORT_CODE(KEYCODE_5)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("INF") PORT_CODE(KEYCODE_F1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("POS") PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H 8") PORT_CODE(KEYCODE_H) PORT_CODE(KEYCODE_8)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LEV") PORT_CODE(KEYCODE_F3)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G 7") PORT_CODE(KEYCODE_G) PORT_CODE(KEYCODE_7)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("MEM") PORT_CODE(KEYCODE_F4)
INPUT_PORTS_END



static MACHINE_DRIVER_START(glasgow )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", M68000, 12000000)
	MDRV_CPU_PROGRAM_MAP(glasgow_mem)
	MDRV_MACHINE_START(glasgow)
	MDRV_MACHINE_RESET(glasgow)

	/* video hardware */
	MDRV_DEFAULT_LAYOUT(layout_glasgow)
	
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("beep", BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START(amsterd )
	MDRV_IMPORT_FROM( glasgow )

	/* basic machine hardware */
	MDRV_CPU_MODIFY("maincpu")
	MDRV_CPU_PROGRAM_MAP(amsterd_mem)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START(dallas32 )
	MDRV_IMPORT_FROM( glasgow )

	/* basic machine hardware */
	MDRV_CPU_REPLACE("maincpu", M68020, 14000000)
	MDRV_CPU_PROGRAM_MAP(dallas32_mem)
	MDRV_MACHINE_START( dallas32 )
MACHINE_DRIVER_END


/***************************************************************************
  ROM definitions
***************************************************************************/

ROM_START( glasgow )
	ROM_REGION( 0x10000, "maincpu", 0 )
	//ROM_LOAD("glasgow.rom", 0x000000, 0x10000, CRC(3e73eff3) )
	ROM_LOAD16_BYTE("me3_3_1u.410",0x00000, 0x04000,CRC(bc8053ba) SHA1(57ea2d5652bfdd77b17d52ab1914de974bd6be12))
	ROM_LOAD16_BYTE("me3_1_1l.410",0x00001, 0x04000,CRC(d5263c39) SHA1(1bef1cf3fd96221eb19faecb6ec921e26ac10ac4))
	ROM_LOAD16_BYTE("me3_4_2u.410",0x08000, 0x04000,CRC(8dba504a) SHA1(6bfab03af835cdb6c98773164d32c76520937efe))
	ROM_LOAD16_BYTE("me3_2_2l.410",0x08001, 0x04000,CRC(b3f27827) SHA1(864ba897d24024592d08c4ae090aa70a2cc5f213))
ROM_END

ROM_START( amsterd )
	ROM_REGION16_BE( 0x1000000, "maincpu", 0 )
	//ROM_LOAD16_BYTE("output.bin", 0x000000, 0x10000, CRC(3e73eff3) )
	ROM_LOAD16_BYTE("amsterda-u.bin",0x00000, 0x05a00,CRC(16cefe29) SHA1(9f8c2896e92fbfd47159a59cb5e87706092c86f4))
	ROM_LOAD16_BYTE("amsterda-l.bin",0x00001, 0x05a00,CRC(c859dfde) SHA1(b0bca6a8e698c322a8c597608db6735129d6cdf0))
ROM_END


ROM_START( dallas )
	ROM_REGION16_BE( 0x1000000, "maincpu", 0 )
	ROM_LOAD16_BYTE("dal_g_pr.dat",0x00000, 0x04000,CRC(66deade9) SHA1(07ec6b923f2f053172737f1fc94aec84f3ea8da1))
	ROM_LOAD16_BYTE("dal_g_pl.dat",0x00001, 0x04000,CRC(c5b6171c) SHA1(663167a3839ed7508ecb44fd5a1b2d3d8e466763))
	ROM_LOAD16_BYTE("dal_g_br.dat",0x08000, 0x04000,CRC(e24d7ec7) SHA1(a936f6fcbe9bfa49bf455f2d8a8243d1395768c1))
	ROM_LOAD16_BYTE("dal_g_bl.dat",0x08001, 0x04000,CRC(144a15e2) SHA1(c4fcc23d55fa5262f5e01dbd000644a7feb78f32))
ROM_END

ROM_START( dallas16 )
	ROM_REGION16_BE( 0x1000000, "maincpu", 0 )
	ROM_LOAD16_BYTE("dallas-u.bin",0x00000, 0x06f00,CRC(8c1462b4) SHA1(8b5f5a774a835446d08dceacac42357b9e74cfe8))
	ROM_LOAD16_BYTE("dallas-l.bin",0x00001, 0x06f00,CRC(f0d5bc03) SHA1(4b1b9a71663d5321820b4cf7da205e5fe5d3d001))
ROM_END

ROM_START( roma )
	ROM_REGION16_BE( 0x1000000, "maincpu", 0 )
	ROM_LOAD("roma32.bin", 0x000000, 0x10000, CRC(587d03bf) SHA1(504e9ff958084700076d633f9c306fc7baf64ffd))
ROM_END

ROM_START( dallas32 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD("dallas32.epr", 0x000000, 0x10000, CRC(83b9ff3f) SHA1(97bf4cb3c61f8ec328735b3c98281bba44b30a28) )
ROM_END

ROM_START( roma32 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD("roma32.bin", 0x000000, 0x10000, CRC(587d03bf) SHA1(504e9ff958084700076d633f9c306fc7baf64ffd) )
ROM_END


/***************************************************************************
  Game drivers
***************************************************************************/

/*     YEAR, NAME,     PARENT,   COMPAT, MACHINE,     INPUT,          INIT,     CONFIG, COMPANY,                      FULLNAME,                 FLAGS */
CONS(  1984, glasgow,  0,        0,    glasgow,       old_keyboard,   0,	0,   "Hegener & Glaser Muenchen",  "Mephisto III S Glasgow", 0)
CONS(  1984, amsterd,  glasgow,  0,    amsterd,       new_keyboard,   0,	0,   "Hegener & Glaser Muenchen",  "Mephisto Amsterdam",     0)
CONS(  1984, dallas,   glasgow,  0,    glasgow,       old_keyboard,   0,	0,   "Hegener & Glaser Muenchen",  "Mephisto Dallas",        0)
CONS(  1984, roma,     glasgow,  0,    glasgow,       new_keyboard,   0,	0,   "Hegener & Glaser Muenchen",  "Mephisto Roma",          GAME_NOT_WORKING)
CONS(  1984, dallas32, glasgow,  0,    dallas32,      new_keyboard,   0,	0,   "Hegener & Glaser Muenchen",  "Mephisto Dallas 32 Bit", 0)
CONS(  1984, roma32,   glasgow,  0,    dallas32,      new_keyboard,   0,	0,   "Hegener & Glaser Muenchen",  "Mephisto Roma 32 Bit",   0)
CONS(  1984, dallas16, glasgow,  0,    amsterd,       new_keyboard,   0,	0,   "Hegener & Glaser Muenchen",  "Mephisto Dallas 16 Bit", 0)
