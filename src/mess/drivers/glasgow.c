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


Made playable by Robbbert in Nov 2009.

How to play (quick guide)
1. You are the white player.
2. Click on the piece to move (LED starts flashing), then click where it goes
3. Computer plays black, it will work out its move and beep.
4. Read the move in the display, or look for the flashing LEDs.
5. Move the computer's piece in the same way you moved yours.
6. If a piece is being taken, firstly click on the piece then click the blank
    area at bottom right. This causes the piece to disappear. After that,
    move the piece that took the other piece.
7. You'll need to read the official user manual for advanced features, or if
    you get messages such as "Err1".

Note about clickable artwork: it seems the horizontal coordinates can vary
    between computers. The supplied artwork at svn r6463 works with my
    computer; it may be out of alignment on yours.


***************************************************************************/

#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "glasgow.lh"
#include "sound/beep.h"


static UINT8 lcd_shift_counter;
// static UINT8 led_status;
static UINT8 led7;
static UINT8	key_select,
		irq_flag,
		lcd_invert,
		key_selector;
static UINT8 board_value;
static UINT16 beeper;

static UINT16 Line18_LED;
static UINT16 Line18_REED;

static UINT8 read_board_flag;
static UINT8 mouse_hold = 0;

typedef struct {
	UINT8 field;
	UINT8 piece;
} BOARD_FIELD;

static BOARD_FIELD m_board[8][8];

/* starts at bottom left corner */
static const BOARD_FIELD start_board[8][8] =
{
	{ {7,10}, {6,8}, {5,9}, {4,11}, {3,12}, {2,9}, {1,8}, {0,10} },
	{ {15,7}, {14,7}, {13,7}, {12,7}, { 11,7}, {10,7}, {9,7}, {8,7} },

	{ {23,0}, {22,0}, {21,0}, {20,0}, {19,0}, {18,0}, {17,0}, {16,0} },
	{ {31,0}, {30,0}, {29,0}, {28,0}, {27,0}, {26,0}, {25,0}, {24,0} },
	{ {39,0}, {38,0}, {37,0}, {36,0}, {35,0}, {34,0}, {33,0}, {32,0} },
	{ {47,0}, {46,0}, {45,0}, {44,0}, {43,0}, {42,0}, {41,0}, {40,0} },

	{ {55,1}, {54,1}, {53,1}, {52,1}, {51,1}, {50,1}, {49,1}, {48,1} },
	{ {63,4}, {62,2}, {61,3}, {60,5}, {59,6}, {58,3}, {57,2}, {56,4} }
};

INLINE UINT8 pos_to_num(UINT8 val)
{
	switch (val)
	{
		case 0xfe: return 7;
		case 0xfd: return 6;
		case 0xfb: return 5;
		case 0xf7: return 4;
		case 0xef: return 3;
		case 0xdf: return 2;
		case 0xbf: return 1;
		case 0x7f: return 0;
		default: return 0xff;
	}
}

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

static void glasgow_pieces_w ( running_machine *machine )
{
	/* This causes the pieces to display on-screen */
	UINT8 i_18, i_AH;

	for (i_18 = 0; i_18 < 8; i_18++)
		for (i_AH = 0; i_AH < 8; i_AH++)
			output_set_indexed_value("P", 63 - m_board[i_18][i_AH].field, m_board[i_18][i_AH].piece);
}

static WRITE16_HANDLER( glasgow_lcd_w )
{
	UINT8 lcd_data = data >> 8;

	if (led7 == 0)
		output_set_digit_value(lcd_shift_counter, lcd_data);

	lcd_shift_counter--;
	lcd_shift_counter &= 3;
}

static WRITE16_HANDLER( glasgow_lcd_flag_w )
{
	running_device *speaker = space->machine->device("beep");
	UINT16 lcd_flag = data & 0x8100;

	beep_set_state(speaker, (lcd_flag & 0x100) ? 1 : 0);

	if (lcd_flag)
		led7 = 255;
	else
	{
		led7 = 0;
		key_selector = 1;
	}
}

static READ16_HANDLER( glasgow_keys_r )
{
	UINT8 data = 0xff;

	static const char *const keynames[] = { "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7", "LINE8", "LINE9" };
	static UINT8 board_row = 0;
	static UINT16 mouse_down = 0;
	UINT8 pos2num_res = 0;
	board_row++;
	board_row &= 7;

	/* See if we are moving a piece */
	data = input_port_read_safe(space->machine, keynames[board_row], 0xff);

	if ((data != 0xff) && (!mouse_down))
	{
		pos2num_res = pos_to_num(data);

		if (!(pos2num_res < 8))
			logerror("Position out of bound!");
		else if ((mouse_hold) && (!m_board[board_row][pos2num_res].piece))
		{
			/* Moving a piece onto a blank */
			m_board[board_row][pos2num_res].piece = mouse_hold;
			mouse_hold = 0;
		}
		else if ((!mouse_hold) && (m_board[board_row][pos2num_res].piece))
		{
			/* Picking up a piece */
			mouse_hold = m_board[board_row][pos2num_res].piece;
			m_board[board_row][pos2num_res].piece = 0;
		}

		mouse_down = board_row + 1;
	}
	else if ((data == 0xff) && (mouse_down == (board_row + 1)))	/* Wait for mouse to be released */
		mouse_down = 0;

	/* See if we are taking a piece off the board */
	if (!input_port_read_safe(space->machine, "LINE10", 0xff))
		mouse_hold = 0;

	/* See if any keys pressed */
	data = 3;

	if (key_select == input_port_read(space->machine, "LINE0"))
		data &= 1;

	if (key_select == input_port_read(space->machine, "LINE1"))
		data &= 2;

	return data << 8;
}

static WRITE16_HANDLER( glasgow_keys_w )
{
	key_select = data >> 8;
	glasgow_pieces_w(space->machine);
}

static READ16_HANDLER( glasgow_board_r )
{
	UINT8 i_AH, data = 0;

	if (Line18_REED < 8)
	{
		// if there is a piece on the field -> set bit in data
		for (i_AH = 0; i_AH < 8; i_AH++)
		{
			if (!m_board[Line18_REED][i_AH].piece)
				data |= (1 << i_AH);
		}
	}

	read_board_flag = TRUE;

	return data << 8;
}

static WRITE16_HANDLER( glasgow_board_w )
{
	//UINT8 beep_flag;
	Line18_REED = pos_to_num(data >> 8) ^ 7;

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

	lcd_invert = 1;
	//beep_flag = data >> 8;
	//if ((beep_flag & 02) == 0) key_selector = 0; else key_selector = 1;
	//logerror("Write Beeper   = %x \n  ",data);
	beeper = data;

}

static WRITE16_HANDLER( glasgow_beeper_w )
{
	UINT8 i_AH, i_18;
	UINT16 LineAH = 0;
	UINT8 LED;

	LineAH = data >> 8;

	if (LineAH && Line18_LED)
	{
		for (i_AH = 0; i_AH < 8; i_AH++)
		{
			if (LineAH & (1 << i_AH))
			{
				for (i_18 = 0; i_18 < 8; i_18++)
				{
					if (!(Line18_LED & (1 << i_18)))
					{
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

static WRITE16_HANDLER( write_beeper )
{
	UINT8 beep_flag;

	lcd_invert = 1;
	beep_flag = data >> 8;
//  if ((beep_flag & 02) == 0) key_selector = 0; else key_selector = 1;
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
//  The key function in the rom expects after writing to
//  the chess board a value from  the first key row;
	logerror("Write Board = %x \n", data >> 8);
}


static WRITE16_HANDLER( write_irq_flag )
{
	running_device *speaker = space->machine->device("beep");

	beep_set_state(speaker, data & 0x100);
	logerror("Write 0x800004 = %x \n", data);
	irq_flag = 1;
	beeper = data;
}

static READ16_HANDLER( read_newkeys16 )  //Amsterdam, Roma
{
	UINT16 data;

	if (key_selector == 0)
		data = input_port_read(space->machine, "LINE0");
	else
		data = input_port_read(space->machine, "LINE1");

	logerror("read Keyboard Offset = %x Data = %x Select = %x \n", offset, data, key_selector);
	data <<= 8;
	return data ;
}


#ifdef UNUSED_FUNCTION
static READ16_HANDLER(read_test)
{
	logerror("read test Offset = %x Data = %x\n  ",offset,data);
	return 0xffff;    // Mephisto need it for working
}
#endif

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
	data <<= 24;
	return data ;
}

static READ32_HANDLER( read_board32 )
{
	logerror("read board 32 Offset = %x \n", offset);
	return 0;
}

#ifdef UNUSED_FUNCTION
static READ16_HANDLER(read_board_amsterd)
{
	logerror("read board amsterdam Offset = %x \n  ", offset);
	return 0xffff;
}
#endif

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
	running_device *speaker = space->machine->device("beep");
	beep_set_state(speaker, data & 0x01000000);
	logerror("Write 0x8000004 = %x \n", data);
	irq_flag = 1;
	beeper = data;
}

//static int irq_edge = 0x00;

static TIMER_CALLBACK( update_nmi )
{
	//cputag_set_input_line_and_vector(machine, "maincpu", M68K_IRQ_7, irq_edge & 0xff ? CLEAR_LINE : ASSERT_LINE, M68K_INT_ACK_AUTOVECTOR);
		cputag_set_input_line_and_vector(machine, "maincpu", M68K_IRQ_7, ASSERT_LINE, M68K_INT_ACK_AUTOVECTOR);
		cputag_set_input_line_and_vector(machine, "maincpu", M68K_IRQ_7, CLEAR_LINE, M68K_INT_ACK_AUTOVECTOR);
	// irq_edge = ~irq_edge;
}

static TIMER_CALLBACK( update_nmi32 )
{
	// cputag_set_input_line_and_vector(machine, "maincpu", M68K_IRQ_7, irq_edge & 0xff ? CLEAR_LINE : ASSERT_LINE, M68K_INT_ACK_AUTOVECTOR);
		cputag_set_input_line_and_vector(machine, "maincpu", M68K_IRQ_7, ASSERT_LINE, M68K_INT_ACK_AUTOVECTOR);
		cputag_set_input_line_and_vector(machine, "maincpu", M68K_IRQ_7, CLEAR_LINE, M68K_INT_ACK_AUTOVECTOR);
	// irq_edge = ~irq_edge;
}

static MACHINE_START( glasgow )
{
	running_device *speaker = machine->device("beep");

	key_selector = 0;
	irq_flag = 0;
	lcd_shift_counter = 3;
	timer_pulse(machine, ATTOTIME_IN_HZ(50), NULL, 0, update_nmi);
	beep_set_frequency(speaker, 44);
}


static MACHINE_START( dallas32 )
{
	running_device *speaker = machine->device("beep");

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
	ADDRESS_MAP_GLOBAL_MASK(0x1FFFF)
	AM_RANGE(0x00000000, 0x0000ffff) AM_ROM
	AM_RANGE(0x00010000, 0x00010001) AM_WRITE( glasgow_lcd_w )
	AM_RANGE(0x00010002, 0x00010003) AM_READWRITE( glasgow_keys_r, glasgow_keys_w )
	AM_RANGE(0x00010004, 0x00010005) AM_WRITE( glasgow_lcd_flag_w )
	AM_RANGE(0x00010006, 0x00010007) AM_READWRITE( glasgow_board_r, glasgow_beeper_w )
	AM_RANGE(0x00010008, 0x00010009) AM_WRITE( glasgow_board_w )
	AM_RANGE(0x0001c000, 0x0001ffff) AM_RAM		// 16KB
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

static INPUT_PORTS_START( glasgow )
	PORT_INCLUDE( old_keyboard )
	PORT_START("LINE2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_START("LINE3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_START("LINE4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_START("LINE5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_START("LINE6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_START("LINE7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_START("LINE8")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_START("LINE9")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_START("LINE10")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_KEYBOARD)
INPUT_PORTS_END

static MACHINE_CONFIG_START( glasgow, driver_device )
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
MACHINE_CONFIG_END


static MACHINE_CONFIG_DERIVED( amsterd, glasgow )

	/* basic machine hardware */
	MDRV_CPU_MODIFY("maincpu")
	MDRV_CPU_PROGRAM_MAP(amsterd_mem)
MACHINE_CONFIG_END


static MACHINE_CONFIG_DERIVED( dallas32, glasgow )

	/* basic machine hardware */
	MDRV_CPU_REPLACE("maincpu", M68020, 14000000)
	MDRV_CPU_PROGRAM_MAP(dallas32_mem)
	MDRV_MACHINE_START( dallas32 )
MACHINE_CONFIG_END


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

/*     YEAR, NAME,     PARENT,   COMPAT, MACHINE,     INPUT,          INIT, COMPANY,                      FULLNAME,                 FLAGS */
CONS(  1984, glasgow,  0,        0,    glasgow,       glasgow,        0,	"Hegener & Glaser Muenchen",  "Mephisto III S Glasgow", 0)
CONS(  1984, amsterd,  glasgow,  0,    amsterd,       new_keyboard,   0,	"Hegener & Glaser Muenchen",  "Mephisto Amsterdam",     0)
CONS(  1984, dallas,   glasgow,  0,    glasgow,       old_keyboard,   0,	"Hegener & Glaser Muenchen",  "Mephisto Dallas",        0)
CONS(  1984, roma,     glasgow,  0,    glasgow,       new_keyboard,   0,	"Hegener & Glaser Muenchen",  "Mephisto Roma",          GAME_NOT_WORKING)
CONS(  1984, dallas32, glasgow,  0,    dallas32,      new_keyboard,   0,	"Hegener & Glaser Muenchen",  "Mephisto Dallas 32 Bit", 0)
CONS(  1984, roma32,   glasgow,  0,    dallas32,      new_keyboard,   0,	"Hegener & Glaser Muenchen",  "Mephisto Roma 32 Bit",   0)
CONS(  1984, dallas16, glasgow,  0,    amsterd,       new_keyboard,   0,	"Hegener & Glaser Muenchen",  "Mephisto Dallas 16 Bit", 0)
