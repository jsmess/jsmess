/******************************************************************************
    Acorn Electron driver

    MESS Driver By:

    Wilbert Pol

******************************************************************************/

#include "emu.h"
#include "includes/electron.h"
#include "sound/beep.h"
#include "devices/cassette.h"

ULA electron_ula;
static emu_timer *electron_tape_timer;

static running_device *cassette_device_image( running_machine *machine )
{
	return machine->device("cassette");
}

static void electron_tape_start( void )
{
	if ( electron_ula.tape_running )
	{
		return;
	}
	electron_ula.tape_steps = 0;
	electron_ula.tape_value = 0x80808080;
	electron_ula.high_tone_set = 0;
	electron_ula.bit_count = 0;
	electron_ula.tape_running = 1;
	timer_adjust_periodic(electron_tape_timer, attotime_zero, 0, ATTOTIME_IN_HZ(4800));
}

static void electron_tape_stop( void )
{
	electron_ula.tape_running = 0;
	timer_reset( electron_tape_timer, attotime_never );
}

#define TAPE_LOW	0x00;
#define TAPE_HIGH	0xFF;

static TIMER_CALLBACK(electron_tape_timer_handler)
{
	if ( electron_ula.cassette_motor_mode )
	{
		double tap_val;
		tap_val = cassette_input( cassette_device_image( machine ) );
		if ( tap_val < -0.5 )
		{
			electron_ula.tape_value = ( electron_ula.tape_value << 8 ) | TAPE_LOW;
			electron_ula.tape_steps++;
		}
		else if ( tap_val > 0.5 )
		{
			electron_ula.tape_value = ( electron_ula.tape_value << 8 ) | TAPE_HIGH;
			electron_ula.tape_steps++;
		}
		else
		{
			electron_ula.tape_steps = 0;
			electron_ula.bit_count = 0;
			electron_ula.high_tone_set = 0;
			electron_ula.tape_value = 0x80808080;
		}
		if ( electron_ula.tape_steps > 2 && ( electron_ula.tape_value == 0x0000FFFF || electron_ula.tape_value == 0x00FF00FF ) )
		{
			electron_ula.tape_steps = 0;
			switch( electron_ula.bit_count )
			{
			case 0:	/* start bit */
				electron_ula.start_bit = ( ( electron_ula.tape_value == 0x0000FFFF ) ? 0 : 1 );
				//logerror( "++ Read start bit: %d\n", electron_ula.start_bit );
				if ( electron_ula.start_bit )
				{
					if ( electron_ula.high_tone_set )
					{
						electron_ula.bit_count--;
					}
				}
				else
				{
					electron_ula.high_tone_set = 0;
				}
				break;
			case 1: case 2: case 3: case 4:
			case 5: case 6: case 7: case 8:
				//logerror( "++ Read regular bit: %d\n", electron_ula.tape_value == 0x0000FFFF ? 0 : 1 );
				electron_ula.tape_byte = ( electron_ula.tape_byte >> 1 ) | ( electron_ula.tape_value == 0x0000FFFF ? 0 : 0x80 );
				break;
			case 9: /* stop bit */
				electron_ula.stop_bit = ( ( electron_ula.tape_value == 0x0000FFFF ) ? 0 : 1 );
				//logerror( "++ Read stop bit: %d\n", electron_ula.stop_bit );
				if ( electron_ula.start_bit && electron_ula.stop_bit && electron_ula.tape_byte == 0xFF && ! electron_ula.high_tone_set )
				{
					electron_interrupt_handler( machine, INT_SET, INT_HIGH_TONE );
					electron_ula.high_tone_set = 1;
				}
				else if ( ! electron_ula.start_bit && electron_ula.stop_bit )
				{
					//logerror( "-- Byte read from tape: %02x\n", electron_ula.tape_byte );
					electron_interrupt_handler( machine, INT_SET, INT_RECEIVE_FULL );
				}
				else
				{
					logerror( "Invalid start/stop bit combination detected: %d,%d\n", electron_ula.start_bit, electron_ula.stop_bit );
				}
				break;
			}
			electron_ula.bit_count = ( electron_ula.bit_count + 1 ) % 10;
		}
	}
}

static READ8_HANDLER( electron_read_keyboard )
{
	UINT8 data = 0;
	int i;
	static const char *const keynames[] = {
		"LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6",
		"LINE7", "LINE8", "LINE9", "LINE10", "LINE11", "LINE12", "LINE13"
	};

	//logerror( "PC=%04x: keyboard read from paged rom area, address: %04x", activecpu_get_pc(), offset );
	for( i = 0; i < 14; i++ )
	{
		if( !(offset & 1) )
			data |= input_port_read(space->machine, keynames[i]) & 0x0f;

		offset = offset >> 1;
	}
	//logerror( ", data: %02x\n", data );
	return data;
}

READ8_HANDLER( electron_jim_r )
{
	return 0xff;
}

WRITE8_HANDLER( electron_jim_w )
{
}

READ8_HANDLER( electron_1mhz_r )
{
	return 0xff;
}

WRITE8_HANDLER( electron_1mhz_w )
{
}

READ8_HANDLER( electron_ula_r )
{
	UINT8 data = ((UINT8 *)memory_region(space->machine, "user1"))[0x43E00 + offset];
	switch ( offset & 0x0f )
	{
	case 0x00:	/* Interrupt status */
		data = electron_ula.interrupt_status;
		electron_ula.interrupt_status &= ~0x02;
		break;
	case 0x01:	/* Unknown */
		break;
	case 0x04:	/* Casette data shift register */
		electron_interrupt_handler( space->machine, INT_CLEAR, INT_RECEIVE_FULL );
		data = electron_ula.tape_byte;
		break;
	}
	logerror( "ULA: read offset %02x: %02x\n", offset, data );
	return data;
}

static const int electron_palette_offset[4] = { 0, 4, 5, 1 };
static const UINT16 electron_screen_base[8] = { 0x3000, 0x3000, 0x3000, 0x4000, 0x5800, 0x5800, 0x6000, 0x5800 };

WRITE8_HANDLER( electron_ula_w )
{
	running_device *speaker = space->machine->device("beep");
	int i = electron_palette_offset[(( offset >> 1 ) & 0x03)];
	logerror( "ULA: write offset %02x <- %02x\n", offset & 0x0f, data );
	switch( offset & 0x0f )
	{
	case 0x00:	/* Interrupt control */
		electron_ula.interrupt_control = data;
		break;
	case 0x01:	/* Unknown */
		break;
	case 0x02:	/* Screen start address #1 */
		electron_ula.screen_start = ( electron_ula.screen_start & 0x7e00 ) | ( ( data & 0xe0 ) << 1 );
		logerror( "screen_start changed to %04x\n", electron_ula.screen_start );
		break;
	case 0x03:	/* Screen start addres #2 */
		electron_ula.screen_start = ( electron_ula.screen_start & 0x1c0 ) | ( ( data & 0x3f ) << 9 );
		logerror( "screen_start changed to %04x\n", electron_ula.screen_start );
		break;
	case 0x04:	/* Cassette data shift register */
		break;
	case 0x05:	/* Interrupt clear and paging */
		/* rom page requests are honoured when currently bank 0-7 or 12-15 is switched in,
         * or when 8-11 is currently switched in only switching to bank 8-15 is allowed.
         *
         * Rompages 10 and 11 both select the Basic ROM.
         * Rompages 8 and 9 both select the keyboard.
         */
		if ( ( ( electron_ula.rompage & 0x0C ) != 0x08 ) || ( data & 0x08 ) )
		{
			electron_ula.rompage = data & 0x0f;
			if ( electron_ula.rompage == 8 || electron_ula.rompage == 9 )
			{
				electron_ula.rompage = 8;
				memory_install_read8_handler( space, 0x8000, 0xbfff, 0, 0, electron_read_keyboard );
			}
			else
			{
				memory_install_read_bank( space, 0x8000, 0xbfff, 0, 0, "bank2");
			}
			memory_set_bank(space->machine, "bank2", electron_ula.rompage);
		}
		if ( data & 0x10 )
		{
			electron_interrupt_handler( space->machine, INT_CLEAR, INT_DISPLAY_END );
		}
		if ( data & 0x20 )
		{
			electron_interrupt_handler( space->machine, INT_CLEAR, INT_RTC );
		}
		if ( data & 0x40 )
		{
			electron_interrupt_handler( space->machine, INT_CLEAR, INT_HIGH_TONE );
		}
		if ( data & 0x80 )
		{
		}
		break;
	case 0x06:	/* Counter divider */
		if ( electron_ula.communication_mode == 0x01)
		{
			beep_set_frequency( speaker, 1000000 / ( 16 * ( data + 1 ) ) );
		}
		break;
	case 0x07:	/* Misc. */
		electron_ula.communication_mode = ( data >> 1 ) & 0x03;
		switch( electron_ula.communication_mode )
		{
		case 0x00:	/* cassette input */
			beep_set_state( speaker, 0 );
			electron_tape_start();
			break;
		case 0x01:	/* sound generation */
			beep_set_state( speaker, 1 );
			electron_tape_stop();
			break;
		case 0x02:	/* cassette output */
			beep_set_state( speaker, 0 );
			electron_tape_stop();
			break;
		case 0x03:	/* not used */
			beep_set_state( speaker, 0 );
			electron_tape_stop();
			break;
		}
		electron_ula.screen_mode = ( data >> 3 ) & 0x07;
		electron_ula.screen_base = electron_screen_base[ electron_ula.screen_mode ];
		electron_ula.screen_size = 0x8000 - electron_ula.screen_base;
		electron_ula.vram = (UINT8 *)space->get_read_ptr(electron_ula.screen_base );
		logerror( "ULA: screen mode set to %d\n", electron_ula.screen_mode );
		electron_ula.cassette_motor_mode = ( data >> 6 ) & 0x01;
		cassette_change_state( cassette_device_image( space->machine ), electron_ula.cassette_motor_mode ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED, CASSETTE_MOTOR_DISABLED );
		electron_ula.capslock_mode = ( data >> 7 ) & 0x01;
		break;
	case 0x08: case 0x0A: case 0x0C: case 0x0E:
		// video_update
		electron_ula.current_pal[i+10] = (electron_ula.current_pal[i+10] & 0x01) | (((data & 0x80) >> 5) | ((data & 0x08) >> 1));
		electron_ula.current_pal[i+8] = (electron_ula.current_pal[i+8] & 0x01) | (((data & 0x40) >> 4) | ((data & 0x04) >> 1));
		electron_ula.current_pal[i+2] = (electron_ula.current_pal[i+2] & 0x03) | ((data & 0x20) >> 3);
		electron_ula.current_pal[i] = (electron_ula.current_pal[i] & 0x03) | ((data & 0x10) >> 2);
		break;
	case 0x09: case 0x0B: case 0x0D: case 0x0F:
		// video_update
		electron_ula.current_pal[i+10] = (electron_ula.current_pal[i+10] & 0x06) | ((data & 0x08) >> 3);
		electron_ula.current_pal[i+8] = (electron_ula.current_pal[i+8] & 0x06) | ((data & 0x04) >> 2);
		electron_ula.current_pal[i+2] = (electron_ula.current_pal[i+2] & 0x04) | (((data & 0x20) >> 4) | ((data & 0x02) >> 1));
		electron_ula.current_pal[i] = (electron_ula.current_pal[i] & 0x04) | (((data & 0x10) >> 3) | ((data & 0x01)));
		break;
	}
}

void electron_interrupt_handler(running_machine *machine, int mode, int interrupt)
{
	if ( mode == INT_SET )
	{
		electron_ula.interrupt_status |= interrupt;
	}
	else
	{
		electron_ula.interrupt_status &= ~interrupt;
	}
	if ( electron_ula.interrupt_status & electron_ula.interrupt_control & ~0x83 )
	{
		electron_ula.interrupt_status |= 0x01;
		cputag_set_input_line(machine, "maincpu", 0, ASSERT_LINE );
	}
	else
	{
		electron_ula.interrupt_status &= ~0x01;
		cputag_set_input_line(machine, "maincpu", 0, CLEAR_LINE );
	}
}

/**************************************
   Machine Initialisation functions
***************************************/

static TIMER_CALLBACK(setup_beep)
{
	running_device *speaker = machine->device("beep");
	beep_set_state( speaker, 0 );
	beep_set_frequency( speaker, 300 );
}

static void electron_reset(running_machine &machine)
{
	memory_set_bank(&machine, "bank2", 0);

	electron_ula.communication_mode = 0x04;
	electron_ula.screen_mode = 0;
	electron_ula.cassette_motor_mode = 0;
	electron_ula.capslock_mode = 0;
	electron_ula.screen_mode = 0;
	electron_ula.screen_start = 0x3000;
	electron_ula.screen_base = 0x3000;
	electron_ula.screen_size = 0x8000 - 0x3000;
	electron_ula.screen_addr = 0;
	electron_ula.tape_running = 0;
	electron_ula.vram = (UINT8 *)cputag_get_address_space(&machine, "maincpu", ADDRESS_SPACE_PROGRAM)->get_read_ptr(electron_ula.screen_base);
}

MACHINE_START( electron )
{
	memory_configure_bank(machine, "bank2", 0, 16, memory_region(machine, "user1"), 0x4000);

	electron_ula.interrupt_status = 0x82;
	electron_ula.interrupt_control = 0x00;
	timer_set(machine, attotime_zero, NULL, 0, setup_beep );
	electron_tape_timer = timer_alloc(machine,  electron_tape_timer_handler, NULL );
	machine->add_notifier(MACHINE_NOTIFY_RESET, electron_reset);
}

