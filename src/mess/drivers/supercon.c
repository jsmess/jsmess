/******************************************************************************
 Novag SuperConstellation Chess Computer
 2010 R. Schaefer


 CPU 6502
 Clock 4 MHz
 IRQ CLK 600 Hz

 RAM    0x0000, 0x0fff)
 ROM    0x2000, 0xffff)
 I/O    0x1c00              Unknown
        0x1d00              Unknown
        0x1e00              LED's and buttons
        0x1f00              LED's, buttons and buzzer


******************************************************************************/

#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "sound/beep.h"

#include "supercon.lh"

#define VERBOSE 0
#define LOG(x)	do { if (VERBOSE) logerror x; } while (0)

#define ClearBit(x,y)	( y &= ~( 1 << x ) )
#define SetBit(x,y)		( y |= (1 << x ) )
#define ToggleBit(x,y)	( y ^= (1 << x ) )

#define IsBitSet(x,y)	( y & (1<<x) )
#define IsBitClear(x,y)	( !( y & (1<<x) ) )


enum
{
	EM,		/*No piece*/
	BP,
	BN,
	BB,
	BR,
	BQ,
	BK,
	WP,
	WN,
	WB,
	WR,
	WQ,
	WK
};


#define LED_LINE_AH		0x10
#define LED_LINE_ST		0x20
#define LED_LINE_18		0x40


#define MAIN_CLOCK 4000000 /* 4 MHz */

#define FALSE	0
#define TRUE	1

#define NOT_VALID	99

#define NO_ACTION	0
#define	TAKE		1
#define SET			2

static UINT8 *supercon_ram;
static emu_timer* timer_update_irq;

static int emu_started=FALSE;

static int moving_piece=0;

static UINT8 data_1E00=0;
static UINT8 data_1F00=0;

static UINT8 LED_18=0;
static UINT8 LED_AH=0;
static UINT8 LED_ST=0;
static UINT8 *last_LED=NULL;
static UINT8 last_LED_value=0;

static int led_update=TRUE;
static int remove_led_flag=TRUE;

static int selecting=NO_ACTION;
static int confirm_board_click=FALSE;
static int *current_field;

static int m_board[64];
static int save_board[64];

/* artwork board */
static const int start_board[64] =
{
	BR, BN, BB, BQ, BK, BB, BN, BR,
	BP, BP, BP, BP, BP, BP, BP, BP,
	EM, EM, EM, EM, EM, EM, EM, EM,
	EM, EM, EM, EM, EM, EM, EM, EM,
	EM, EM, EM, EM, EM, EM, EM, EM,
	EM, EM, EM, EM, EM, EM, EM, EM,
	WP, WP, WP, WP, WP, WP, WP, WP,
	WR, WN, WB, WQ, WK, WB, WN, WR
};

static const UINT8 border_pieces[12] = {WK,WQ,WR,WB,WN,WP,BK,BQ,BR,BB,BN,BP,};


static void set_board( void )
{
	int i;
	for (i=0;i<64;i++)
		m_board[i]=start_board[i];
}

static void set_pieces (void)
{
	int i;
	for (i=0;i<64;i++)
		output_set_indexed_value("P", i, m_board[i]);
}

static void set_boarder_pieces (void)
{
	UINT8 i;

	for (i=0;i<12;i++)
		output_set_indexed_value("Q", i, border_pieces[i]);
}

static void clear_pieces()
{
	int i;
	for (i=0;i<64;i++)
	{
		output_set_indexed_value("P", i, EM);
		m_board[i]=EM;
	}
}

static int get_first_cleared_bit(UINT8 data)
{
	int i;

	for (i=0;i<8;i++)
		if (IsBitClear(i,data))
			return i;

	return NOT_VALID;
}

static int get_first_bit(UINT8 data)
{
	int i;

	for (i=0;i<8;i++)
		if (IsBitSet(i,data))
			return i;

	return NOT_VALID;
}


static void update_leds( void )
{
	int i;

	for (i=0;i<8;i++)
	{
		if (IsBitSet(i,LED_18))
			output_set_led_value(i+1,1);
		else
			output_set_led_value(i+1,0);

		if (IsBitSet(i,LED_AH))
			output_set_led_value(i+9,1);
		else
			output_set_led_value(i+9,0);

		if (IsBitSet(i,LED_ST))
			output_set_led_value(i+17,1);
		else
			output_set_led_value(i+17,0);
	}
}

static void mouse_update(running_machine *machine)
{
	UINT8 port_input, m_left;
	int i;

/* Set-remove piece after mouse release and confirmation of a board click = beep */

	 m_left=input_port_read(machine, "BUTTON_L");
	 if (!m_left && selecting)
	 {
		 if (confirm_board_click)
		 {
			if (selecting==SET)
			{
				*current_field=moving_piece;
				moving_piece=EM;
			}else
			{
				moving_piece=*current_field;
				*current_field=EM;
			}
			set_pieces();
			output_set_value("MOVING",moving_piece);
		 }

		 selecting=NO_ACTION;
		 confirm_board_click=FALSE;
	 }

/* Boarder pieces and moving pice */

	port_input=input_port_read(machine, "B_WHITE");
	if (port_input)
	{
		i=get_first_bit(port_input);
		moving_piece=border_pieces[i];
		output_set_value("MOVING",moving_piece);
		return;
	}


	port_input=input_port_read(machine, "B_BLACK");
	if (port_input)
	{
		i=get_first_bit(port_input);
		moving_piece=border_pieces[6+i];
		output_set_value("MOVING",moving_piece);
		return;
	}


	port_input=input_port_read(machine, "B_CLR");
	if (port_input)
	{
		if (moving_piece)
		{
			moving_piece=0;
			output_set_value("MOVING",moving_piece);
			return;
		}
	}
}

/* Driver initialization */

static DRIVER_INIT(supercon)
{
	LED_18=0;
	LED_AH=0;
	LED_ST=0;

	moving_piece=0;
}

/* Read 1C000 */

static READ8_HANDLER( supercon_port1_r )
{
	LOG(("Read from %04x \n",0x1C00));
    return 0xff;
}

/* Read 1D000 */

static READ8_HANDLER( supercon_port2_r )
{
	LOG(("Read from %04x \n",0x1D00));
    return 0xff;
}

/* Read 1E00 */

static READ8_HANDLER( supercon_port3_r )
{
	int i;
	UINT8 key_data=0;

	static const char *const status_lines[8] =
			{ "STATUS_1", "STATUS_2", "STATUS_3", "STATUS_4", "STATUS_5", "STATUS_6", "STATUS_7", "STATUS_8" };

	LOG(("Read from %04x \n",0x1E00));

/* remove last bit (only if it was not already set) */

	if ( data_1F00 & LED_LINE_AH )
	{
		if (last_LED_value != LED_AH)
			LED_AH=LED_AH & ~data_1E00;
	}
	else if ( data_1F00 & LED_LINE_ST)
	{
		if (last_LED_value != LED_ST)
			LED_ST=LED_ST & ~data_1E00;
	}
	else if ( data_1F00 & LED_LINE_18 )
	{
		if (last_LED_value != LED_18)
			LED_18=LED_18 & ~data_1E00;
	}


	LOG(("LED_18 from %02x \n",LED_18));
	LOG(("LED_AH from %02x \n",LED_AH));
	LOG(("LED_ST from %02x \n",LED_ST));

	if (led_update)			/*No LED Update if Port 1C00,1D00 was read */
		update_leds();

	remove_led_flag=TRUE;
	led_update=TRUE;

	LED_18=0;
	LED_AH=0;
	LED_ST=0;


/* Start */

	if (!emu_started)
		return 0xbf;
	else
		timer_adjust_periodic( timer_update_irq, attotime_zero, 0, ATTOTIME_IN_HZ(598) );  //HACK adjust timer after start ???


/* Buttons */

	i=get_first_bit(data_1E00);
	if (i==NOT_VALID)
		return 0xff;

	key_data=input_port_read(space->machine, status_lines[i]);

	if (key_data != 0xc0)
	{
		LOG(("%s, key_data: %02x \n",status_lines[i],key_data));

/* Button: New Game -> initialize board */

		if (i==0 && key_data==0x80)
		{
			set_board();
			set_pieces();

			emu_started=FALSE;
		}

/* Button: Clear Board -> remove all pieces */

		if (i==3 && key_data==0x80)
			clear_pieces();

		if (key_data != 0xff )
			return key_data;

	}

	return 0xc0;
}

/* Read Port $1F00 */

static READ8_HANDLER( supercon_port4_r )
{
	int i_18, i_AH;
	UINT8 key_data = 0x00;;

	static const char *const board_lines[8] =
			{ "BOARD_1", "BOARD_2", "BOARD_3", "BOARD_4", "BOARD_5", "BOARD_6", "BOARD_7", "BOARD_8" };

	LOG(("Read from %04x \n",0x1F00));

/* Board buttons */

	i_18=get_first_bit(data_1E00);
	if (i_18==NOT_VALID)
		return 0xff;

	key_data=input_port_read(space->machine, board_lines[i_18]);

	if (key_data != 0xff)
	{
		LOG(("%s key_data: %02x \n",board_lines[i_18],key_data));

		if (key_data)
		{

/* Set or remove pieces */

			i_AH=7-get_first_cleared_bit(key_data);
			LOG(("Press -> AH: %d 18: %d Piece: %d\n",i_AH,i_18,m_board[i_18*8 + i_AH]););

			if (selecting==NO_ACTION)
			{
				current_field=&m_board[i_18*8 + i_AH];

				if (moving_piece)
					selecting=SET;
				else
					selecting=TAKE;
			}
			return key_data;
		}
	}

	return 0xff;
}

/* Write Port $1C00 */

static WRITE8_HANDLER( supercon_port1_w )
{
	LOG(("Write from %04x data: %02x\n",0x1C00,data));
	led_update=FALSE;
}

/* Write Port $1D00 */

static WRITE8_HANDLER( supercon_port2_w )
{

	LOG(("Write from %04x data: %02x\n",0x1D00,data));
	led_update=FALSE;
}

/* Write Port $1E00 */

static WRITE8_HANDLER( supercon_port3_w )
{

	if (data)
		LOG(("Write from %04x data: %02x\n",0x1E00,data));

	if (data)
	{
		data_1E00=data;

/* Set bits for LED's */

		if ( data_1F00)
		{

			if (data_1F00 & LED_LINE_AH )
			{
				last_LED = &LED_AH;				/* save last value */
				last_LED_value = *last_LED;

				LED_AH=LED_AH | data_1E00;
			}
			else if (data_1F00 & LED_LINE_ST )
			{
				last_LED = &LED_ST;
				last_LED_value = *last_LED;

				LED_ST=LED_ST | data_1E00;
			}
			else if (data_1F00 &  LED_LINE_18)
			{
				last_LED = &LED_18;
				last_LED_value = *last_LED;

				LED_18=LED_18 | data_1E00;
			}
		}

	}

}

/* Write Port $1F00 */

static WRITE8_HANDLER( supercon_port4_w )
{
	running_device *speaker = space->machine->device("beep");

	if (data)
		LOG(("Write from %04x data: %02x\n",0x1F00,data));

	if (data)
		data_1F00=data;

/* Bit 7 is set -> Buzzer on */

	if ( data_1F00 & 0x80 )
	{
		beep_set_state(speaker,1);
		emu_started=TRUE;

		if (selecting)					/* fast mouse clicks are recognized by the artwork but not by the emulation  */
			confirm_board_click=TRUE;	/* therefore a beep must confirm each click on the board */
	}
	else
		beep_set_state(speaker,0);

}

static TIMER_CALLBACK( update_artwork )
{
	mouse_update(machine);
}

static TIMER_CALLBACK( update_irq )
{
	cputag_set_input_line(machine, "maincpu", M6502_IRQ_LINE, ASSERT_LINE);
	cputag_set_input_line(machine, "maincpu", M6502_IRQ_LINE, CLEAR_LINE);
}

/* Save state call backs */

static STATE_PRESAVE( m_board_presave )
{
	int i;
	for (i=0;i<64;i++)
		save_board[i]=m_board[i];
}

static STATE_POSTLOAD( m_board_postload )
{
	int i;
	for (i=0;i<64;i++)
		m_board[i]=save_board[i];

	set_pieces();
}

static MACHINE_START( supercon )
{
	timer_update_irq = timer_alloc(machine,update_irq,NULL);
	timer_adjust_periodic( timer_update_irq, attotime_zero, 0, ATTOTIME_IN_HZ(1000) );

	timer_pulse(machine, ATTOTIME_IN_HZ(20), NULL, 0, update_artwork);

	state_save_register_global_array(machine,save_board);
	state_save_register_postload(machine,m_board_postload,NULL);
	state_save_register_presave(machine,m_board_presave,NULL);
}

static MACHINE_RESET( supercon )
{
	set_board();
	set_pieces();
	set_boarder_pieces();

	emu_started=FALSE;
}

/* Address maps */

static ADDRESS_MAP_START(supercon_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x0fff) AM_RAM AM_BASE(&supercon_ram )
	AM_RANGE( 0x2000, 0x7fff) AM_ROM
    AM_RANGE( 0x8000, 0xffff) AM_ROM

	AM_RANGE( 0x1C00, 0x1C00) AM_WRITE ( supercon_port1_w )
	AM_RANGE( 0x1D00, 0x1D00) AM_WRITE ( supercon_port2_w )
	AM_RANGE( 0x1E00, 0x1E00) AM_WRITE ( supercon_port3_w )
	AM_RANGE( 0x1F00, 0x1F00) AM_WRITE ( supercon_port4_w )

	AM_RANGE( 0x1C00, 0x1C00) AM_READ ( supercon_port1_r )
	AM_RANGE( 0x1D00, 0x1D00) AM_READ ( supercon_port2_r )
	AM_RANGE( 0x1E00, 0x1E00) AM_READ ( supercon_port3_r )
	AM_RANGE( 0x1F00, 0x1F00) AM_READ ( supercon_port4_r )


ADDRESS_MAP_END

/*static ADDRESS_MAP_START(supercon_io, ADDRESS_SPACE_IO, 8)


ADDRESS_MAP_END
 */
/* Input ports */

static INPUT_PORTS_START( supercon )

	PORT_START("MOUSE_X")
	PORT_BIT( 0xffff, 0x00, IPT_MOUSE_X) PORT_SENSITIVITY(100)

	PORT_START("MOUSE_Y")
	PORT_BIT( 0xffff, 0x00, IPT_MOUSE_Y ) PORT_SENSITIVITY(100)


	PORT_START("BUTTON_L")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_CODE(MOUSECODE_BUTTON1) PORT_NAME("left button")

	PORT_START("BUTTON_R")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_CODE(MOUSECODE_BUTTON2) PORT_NAME("right button")

	PORT_START("BOARD_1")
	PORT_BIT(0x01,  IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x02,  IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x04,  IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x08,  IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD)

	PORT_START("BOARD_2")
	PORT_BIT(0x01,  IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x02,  IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x04,  IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x08,  IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD)

	PORT_START("BOARD_3")
	PORT_BIT(0x01,  IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x02,  IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x04,  IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x08,  IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD)

	PORT_START("BOARD_4")
	PORT_BIT(0x01,  IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x02,  IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x04,  IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x08,  IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD)

	PORT_START("BOARD_5")
	PORT_BIT(0x01,  IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x02,  IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x04,  IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x08,  IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD)

	PORT_START("BOARD_6")
	PORT_BIT(0x01,  IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x02,  IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x04,  IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x08,  IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD)

	PORT_START("BOARD_7")
	PORT_BIT(0x01,  IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x02,  IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x04,  IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x08,  IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD)

	PORT_START("BOARD_8")
	PORT_BIT(0x01,  IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x02,  IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x04,  IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x08,  IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD)

	PORT_START("STATUS_1")
	PORT_BIT(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD)

	PORT_START("STATUS_2")
	PORT_BIT(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD)

	PORT_START("STATUS_3")
	PORT_BIT(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD)

	PORT_START("STATUS_4")
	PORT_BIT(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD)

	PORT_START("STATUS_5")
	PORT_BIT(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD)

	PORT_START("STATUS_6")
	PORT_BIT(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD)

	PORT_START("STATUS_7")
	PORT_BIT(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD)

	PORT_START("STATUS_8")
	PORT_BIT(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD)


	PORT_START("B_WHITE")
	PORT_BIT(0x01,  IP_ACTIVE_HIGH, IPT_KEYBOARD)
	PORT_BIT(0x02,  IP_ACTIVE_HIGH, IPT_KEYBOARD)
	PORT_BIT(0x04,  IP_ACTIVE_HIGH, IPT_KEYBOARD)
	PORT_BIT(0x08,  IP_ACTIVE_HIGH, IPT_KEYBOARD)
	PORT_BIT(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD)
	PORT_BIT(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD)

	PORT_START("B_BLACK")
	PORT_BIT(0x01,  IP_ACTIVE_HIGH, IPT_KEYBOARD)
	PORT_BIT(0x02,  IP_ACTIVE_HIGH, IPT_KEYBOARD)
	PORT_BIT(0x04,  IP_ACTIVE_HIGH, IPT_KEYBOARD)
	PORT_BIT(0x08,  IP_ACTIVE_HIGH, IPT_KEYBOARD)
	PORT_BIT(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD)
	PORT_BIT(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD)

	PORT_START("B_CLR")
	PORT_BIT(0x01,  IP_ACTIVE_HIGH, IPT_KEYBOARD)

INPUT_PORTS_END

/* Machine driver */
static MACHINE_CONFIG_START( supercon, driver_device )

    /* basic machine hardware */
	MDRV_CPU_ADD("maincpu",M6502,MAIN_CLOCK)
    MDRV_CPU_PROGRAM_MAP(supercon_mem)

	MDRV_MACHINE_START( supercon )
	MDRV_MACHINE_RESET( supercon )

    /* video hardware */

	MDRV_DEFAULT_LAYOUT(layout_supercon)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("beep", BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)


MACHINE_CONFIG_END

/* ROM definition */

ROM_START(supercon)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("supercon_low.bin",  0x0000,  0x8000, CRC(b853cf6e) SHA1(1a759072a5023b92c07f1fac01b7a21f7b5b45d0 ))
	ROM_LOAD("supercon_high.bin", 0x8000,  0x8000, CRC(c8f82331) SHA1(f7fd039f9a3344db9749931490ded9e9e309cfbe ))


ROM_END

/* Driver */

/*    YEAR  NAME          PARENT  COMPAT  MACHINE    INPUT       INIT      COMPANY  FULLNAME                     FLAGS */
CONS( 1983, supercon,     0,      0,      supercon,  supercon,   supercon, "Novag", "SuperConstellation",       GAME_SUPPORTS_SAVE)
