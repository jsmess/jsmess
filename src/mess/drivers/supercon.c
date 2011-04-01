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


class supercon_state : public driver_device
{
public:
	supercon_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *m_ram;
	emu_timer* m_timer_update_irq;
	emu_timer* m_timer_mouse_click;
	int m_emu_started;
	int m_moving_piece;
	UINT8 m_data_1E00;
	UINT8 m_data_1F00;
	UINT8 m_LED_18;
	UINT8 m_LED_AH;
	UINT8 m_LED_ST;
	UINT8 *m_last_LED;
	UINT8 m_last_LED_value;
	int m_led_update;
	int m_remove_led_flag;
	int m_selecting;
	int m_save_key_data;
	attotime m_wait_time; 
	int *m_current_field;
	int m_confirm_board_click;
	int m_board[64];
	int m_save_board[64];
};


#define VERBOSE 0
#define LOG(x)	do { if (VERBOSE) logerror x; } while (0)

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


static void set_board( supercon_state *state )
{
	int i;
	for (i=0;i<64;i++)
		state->m_board[i]=start_board[i];
}

static void set_pieces (supercon_state *state)
{
	int i;
	for (i=0;i<64;i++)
		output_set_indexed_value("P", i, state->m_board[i]);
}

static void set_boarder_pieces (void)
{
	UINT8 i;

	for (i=0;i<12;i++)
		output_set_indexed_value("Q", i, border_pieces[i]);
}

static void clear_pieces(supercon_state *state)
{
	int i;
	for (i=0;i<64;i++)
	{
		output_set_indexed_value("P", i, EM);
		state->m_board[i]=EM;
	}
}

static int get_first_cleared_bit(UINT8 data)
{
	int i;

	for (i = 0; i < 8; i++)
		if (!BIT(data, i))
			return i;

	return NOT_VALID;
}

static int get_first_bit(UINT8 data)
{
	int i;

	for (i = 0; i < 8; i++)
		if (BIT(data, i))
			return i;

	return NOT_VALID;
}


static void update_leds( supercon_state *state )
{
	int i;

	for (i = 0; i < 8; i++)
	{
		if (BIT(state->m_LED_18, i))
			output_set_led_value(i + 1, 1);
		else
			output_set_led_value(i + 1, 0);

		if (BIT(state->m_LED_AH, i))
			output_set_led_value(i + 9, 1);
		else
			output_set_led_value(i + 9, 0);

		if (BIT(state->m_LED_ST, i))
			output_set_led_value(i + 17, 1);
		else
			output_set_led_value(i + 17, 0);
	}
}

static void mouse_update(running_machine &machine)
{
	supercon_state *state = machine.driver_data<supercon_state>();
	UINT8 port_input; // m_left;
	int i;

/* Boarder pieces and moving pice */

	port_input=input_port_read(machine, "B_WHITE");
	if (port_input)
	{
		i=get_first_bit(port_input);
		state->m_moving_piece=border_pieces[i];
		output_set_value("MOVING",state->m_moving_piece);
		return;
	}


	port_input=input_port_read(machine, "B_BLACK");
	if (port_input)
	{
		i=get_first_bit(port_input);
		state->m_moving_piece=border_pieces[6+i];
		output_set_value("MOVING",state->m_moving_piece);
		return;
	}


	port_input=input_port_read(machine, "B_CLR");
	if (port_input)
	{
		if (state->m_moving_piece)
		{
			state->m_moving_piece=0;
			output_set_value("MOVING",state->m_moving_piece);
			return;
		}
	}
}

/* Driver initialization */

static DRIVER_INIT(supercon)
{
	supercon_state *state = machine.driver_data<supercon_state>();
	state->m_LED_18=0;
	state->m_LED_AH=0;
	state->m_LED_ST=0;

	state->m_wait_time =  attotime::from_hz(4);
	state->m_save_key_data = 0xff;

	state->m_moving_piece=0;
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
	supercon_state *state = space->machine().driver_data<supercon_state>();
	int i;
	UINT8 key_data=0;

	static const char *const status_lines[8] =
			{ "STATUS_1", "STATUS_2", "STATUS_3", "STATUS_4", "STATUS_5", "STATUS_6", "STATUS_7", "STATUS_8" };

	LOG(("Read from %04x \n",0x1E00));

/* remove last bit (only if it was not already set) */

	if ( state->m_data_1F00 & LED_LINE_AH )
	{
		if (state->m_last_LED_value != state->m_LED_AH)
			state->m_LED_AH=state->m_LED_AH & ~state->m_data_1E00;
	}
	else if ( state->m_data_1F00 & LED_LINE_ST)
	{
		if (state->m_last_LED_value != state->m_LED_ST)
			state->m_LED_ST=state->m_LED_ST & ~state->m_data_1E00;
	}
	else if ( state->m_data_1F00 & LED_LINE_18 )
	{
		if (state->m_last_LED_value != state->m_LED_18)
			state->m_LED_18=state->m_LED_18 & ~state->m_data_1E00;
	}


	LOG(("LED_18 from %02x \n",state->m_LED_18));
	LOG(("LED_AH from %02x \n",state->m_LED_AH));
	LOG(("LED_ST from %02x \n",state->m_LED_ST));

	if (state->m_led_update)			/*No LED Update if Port 1C00,1D00 was read */
		update_leds(state);

	state->m_remove_led_flag=TRUE;
	state->m_led_update=TRUE;

	state->m_LED_18=0;
	state->m_LED_AH=0;
	state->m_LED_ST=0;


/* Start */

	if (!state->m_emu_started)
		return 0xbf;
	else
		state->m_timer_update_irq->adjust( attotime::zero, 0, attotime::from_hz(598) );  //HACK adjust timer after start ???


/* Buttons */

	i=get_first_bit(state->m_data_1E00);
	if (i==NOT_VALID)
		return 0xff;

	key_data=input_port_read(space->machine(), status_lines[i]);

	if (key_data != 0xc0)
	{
		LOG(("%s, key_data: %02x \n",status_lines[i],key_data));

/* Button: New Game -> initialize board */

		if (i==0 && key_data==0x80)
		{
			set_board(state);
			set_pieces(state);

			state->m_emu_started=FALSE;
		}

/* Button: Clear Board -> remove all pieces */

		if (i==3 && key_data==0x80)
			clear_pieces(state);

		if (key_data != 0xff )
			return key_data;

	}

	return 0xc0;
}

/* Read Port $1F00 */

static READ8_HANDLER( supercon_port4_r )
{
	supercon_state *state = space->machine().driver_data<supercon_state>();
	int i_18, i_AH;
	UINT8 key_data = 0x00;;

	static const char *const board_lines[8] =
			{ "BOARD_1", "BOARD_2", "BOARD_3", "BOARD_4", "BOARD_5", "BOARD_6", "BOARD_7", "BOARD_8" };

	LOG(("Read from %04x \n",0x1F00));

/* Board buttons */

	i_18=get_first_bit(state->m_data_1E00);
	if (i_18==NOT_VALID)
		return 0xff;

/* if a button was pressed wait til timer -timer_mouse_click- is fired */

	if (state->m_selecting)
		return state->m_save_key_data;
	else
	{
		set_pieces(state);
		output_set_value("MOVING",state->m_moving_piece);
	}

	key_data=input_port_read(space->machine(), board_lines[i_18]);

	if (key_data != 0xff)
	{
		LOG(("%s key_data: %02x \n",board_lines[i_18],key_data));

/* Only if valid data and mouse button is pressed */

		if (key_data && input_port_read(space->machine(), "BUTTON_L")) 
		{

/* Set or remove pieces */

			i_AH=7-get_first_cleared_bit(key_data);
			LOG(("Press -> AH: %d 18: %d Piece: %d\n",i_AH,i_18,state->m_board[i_18*8 + i_AH]););

			state->m_current_field=&state->m_board[i_18*8 + i_AH];

			if (state->m_moving_piece)
			{
				*state->m_current_field = state->m_moving_piece;
				state->m_moving_piece = EM;
			}
			else
			{
				state->m_moving_piece = *state->m_current_field;
				*state->m_current_field = EM;
			}
			state->m_selecting=TRUE;				/* Flag is removed in timer -timer_mouse_click- */
			state->m_save_key_data=key_data;		/* return same key_data til flag selecting is removed */

			state->m_timer_mouse_click->adjust(state->m_wait_time, 0);

			return key_data;
		}
	}

	return 0xff;
}

/* Write Port $1C00 */

static WRITE8_HANDLER( supercon_port1_w )
{
	supercon_state *state = space->machine().driver_data<supercon_state>();
	LOG(("Write from %04x data: %02x\n",0x1C00,data));
	state->m_led_update=FALSE;
}

/* Write Port $1D00 */

static WRITE8_HANDLER( supercon_port2_w )
{
	supercon_state *state = space->machine().driver_data<supercon_state>();

	LOG(("Write from %04x data: %02x\n",0x1D00,data));
	state->m_led_update=FALSE;
}

/* Write Port $1E00 */

static WRITE8_HANDLER( supercon_port3_w )
{
	supercon_state *state = space->machine().driver_data<supercon_state>();

	if (data)
		LOG(("Write from %04x data: %02x\n",0x1E00,data));

	if (data)
	{
		state->m_data_1E00=data;

/* Set bits for LED's */

		if ( state->m_data_1F00)
		{

			if (state->m_data_1F00 & LED_LINE_AH )
			{
				state->m_last_LED = &state->m_LED_AH;				/* save last value */
				state->m_last_LED_value = *state->m_last_LED;

				state->m_LED_AH=state->m_LED_AH | state->m_data_1E00;
			}
			else if (state->m_data_1F00 & LED_LINE_ST )
			{
				state->m_last_LED = &state->m_LED_ST;
				state->m_last_LED_value = *state->m_last_LED;

				state->m_LED_ST=state->m_LED_ST | state->m_data_1E00;
			}
			else if (state->m_data_1F00 &  LED_LINE_18)
			{
				state->m_last_LED = &state->m_LED_18;
				state->m_last_LED_value = *state->m_last_LED;

				state->m_LED_18=state->m_LED_18 | state->m_data_1E00;
			}
		}

	}

}

/* Write Port $1F00 */

static WRITE8_HANDLER( supercon_port4_w )
{
	supercon_state *state = space->machine().driver_data<supercon_state>();
	device_t *speaker = space->machine().device("beep");

	if (data)
		LOG(("Write from %04x data: %02x\n",0x1F00,data));

	if (data)
		state->m_data_1F00=data;

/* Bit 7 is set -> Buzzer on */

	if ( state->m_data_1F00 & 0x80 )
	{
		beep_set_state(speaker,1);
		state->m_emu_started=TRUE;
	}
	else
		beep_set_state(speaker,0);

}

static TIMER_CALLBACK( mouse_click )
{
	supercon_state *state = machine.driver_data<supercon_state>();

	if (input_port_read_safe(machine, "BUTTON_L",0) )				/* wait for mouse release */
		state->m_timer_mouse_click->adjust(state->m_wait_time, 0);
	else
		state->m_selecting=FALSE;
}

static TIMER_DEVICE_CALLBACK( update_artwork )
{
	mouse_update(timer.machine());
}

static TIMER_CALLBACK( update_irq )
{
	cputag_set_input_line(machine, "maincpu", M6502_IRQ_LINE, ASSERT_LINE);
	cputag_set_input_line(machine, "maincpu", M6502_IRQ_LINE, CLEAR_LINE);
}

/* Save state call backs */

static STATE_PRESAVE( m_board_presave )
{
    supercon_state *state = machine.driver_data<supercon_state>();
    int i;
    for (i=0;i<64;i++)
        state->m_save_board[i]=state->m_board[i];
}

static STATE_POSTLOAD( m_board_postload )
{
    supercon_state *state = machine.driver_data<supercon_state>();
    int i;
    for (i=0;i<64;i++)
        state->m_board[i]=state->m_save_board[i];

    set_pieces(state);
}

static MACHINE_START( supercon )
{
	supercon_state *state = machine.driver_data<supercon_state>();

	state->m_timer_update_irq = machine.scheduler().timer_alloc(FUNC(update_irq));
	state->m_timer_update_irq->adjust( attotime::zero, 0, attotime::from_hz(1000) );

	state->m_timer_mouse_click =  machine.scheduler().timer_alloc(FUNC(mouse_click),NULL);


    state->save_item(NAME(state->m_save_board));
    machine.state().register_postload(m_board_postload,NULL);
    machine.state().register_presave(m_board_presave,NULL);

}

static MACHINE_RESET( supercon )
{
	supercon_state *state = machine.driver_data<supercon_state>();
	set_board(state);
	set_pieces(state);
	set_boarder_pieces();

	state->m_emu_started=FALSE;
}

/* Address maps */

static ADDRESS_MAP_START(supercon_mem, AS_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x0fff) AM_RAM AM_BASE_MEMBER(supercon_state, m_ram )
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

/*static ADDRESS_MAP_START(supercon_io, AS_IO, 8)


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
static MACHINE_CONFIG_START( supercon, supercon_state )

    /* basic machine hardware */
	MCFG_CPU_ADD("maincpu",M6502,MAIN_CLOCK)
    MCFG_CPU_PROGRAM_MAP(supercon_mem)

	MCFG_MACHINE_START( supercon )
	MCFG_MACHINE_RESET( supercon )

    /* video hardware */

	//MCFG_DEFAULT_LAYOUT(layout_supercon)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("beep", BEEP, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MCFG_TIMER_ADD_PERIODIC("artwork_timer", update_artwork, attotime::from_hz(20))

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
