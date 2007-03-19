/******************************************************************************
	Acorn Electron driver

	MESS Driver By:

	Wilbert Pol

******************************************************************************/

#include "driver.h"
#include "includes/electron.h"
#include "sound/beep.h"
#include "devices/cassette.h"

ULA ula;
mame_timer *electron_tape_timer;

static mess_image *cassette_device_image( void ) {
	return image_from_devtype_and_index( IO_CASSETTE, 0 );
}

static void electron_tape_start( void ) {
	if ( ula.tape_running ) {
		return;
	}
	ula.tape_steps = 0;
	ula.tape_value = 0x80808080;
	ula.high_tone_set = 0;
	ula.bit_count = 0;
	ula.tape_running = 1;
	timer_adjust( electron_tape_timer, 0, 0, TIME_IN_HZ(4800) );
}

static void electron_tape_stop( void ) {
	ula.tape_running = 0;
	timer_reset( electron_tape_timer, TIME_NEVER );
}

#define TAPE_LOW	0x00;
#define TAPE_HIGH	0xFF;

static void electron_tape_timer_handler( int param ) {
	if ( ula.cassette_motor_mode ) {
		double tap_val;
		tap_val = cassette_input( cassette_device_image() );
		if ( tap_val < -0.5 ) {
			ula.tape_value = ( ula.tape_value << 8 ) | TAPE_LOW;
			ula.tape_steps++;
		} else if ( tap_val > 0.5 ) {
			ula.tape_value = ( ula.tape_value << 8 ) | TAPE_HIGH;
			ula.tape_steps++;
		} else {
			ula.tape_steps = 0;
			ula.bit_count = 0;
			ula.high_tone_set = 0;
			ula.tape_value = 0x80808080;
		}
		if ( ula.tape_steps > 2 && ( ula.tape_value == 0x0000FFFF || ula.tape_value == 0x00FF00FF ) ) {
			ula.tape_steps = 0;
			switch( ula.bit_count ) {
			case 0:	/* start bit */
				ula.start_bit = ( ( ula.tape_value == 0x0000FFFF ) ? 0 : 1 );
				//logerror( "++ Read start bit: %d\n", ula.start_bit );
				if ( ula.start_bit ) {
					if ( ula.high_tone_set ) {
						ula.bit_count--;
					}
				} else {
					ula.high_tone_set = 0;
				}
				break;
			case 1: case 2: case 3: case 4:
			case 5: case 6: case 7: case 8:
				//logerror( "++ Read regular bit: %d\n", ula.tape_value == 0x0000FFFF ? 0 : 1 );
				ula.tape_byte = ( ula.tape_byte >> 1 ) | ( ula.tape_value == 0x0000FFFF ? 0 : 0x80 );
				break;
			case 9: /* stop bit */
				ula.stop_bit = ( ( ula.tape_value == 0x0000FFFF ) ? 0 : 1 );
				//logerror( "++ Read stop bit: %d\n", ula.stop_bit );
				if ( ula.start_bit && ula.stop_bit && ula.tape_byte == 0xFF && ! ula.high_tone_set ) {
					electron_interrupt_handler( INT_SET, INT_HIGH_TONE );
					ula.high_tone_set = 1;
				} else if ( ! ula.start_bit && ula.stop_bit ) {
					//logerror( "-- Byte read from tape: %02x\n", ula.tape_byte );
					electron_interrupt_handler( INT_SET, INT_RECEIVE_FULL );
				} else {
					logerror( "Invalid start/stop bit combination detected: %d,%d\n", ula.start_bit, ula.stop_bit );
				}
				break;
			}
			ula.bit_count = ( ula.bit_count + 1 ) % 10;
		}
	}
}

READ8_HANDLER( electron_read_keyboard ) {
	UINT8 data = 0;
	int i;
	//logerror( "PC=%04x: keyboard read from paged rom area, address: %04x", activecpu_get_pc(), offset );
	for( i = 0; i < 14; i++ ) {
		if ( ! ( offset & 1 ) ) {
			data |= readinputport(i) & 0x0f;
		}
		offset = offset >> 1;
	}
	//logerror( ", data: %02x\n", data );
	return data;
}

READ8_HANDLER( electron_jim_r ) {
	return 0xff;
}

WRITE8_HANDLER( electron_jim_w ) {
}

READ8_HANDLER( electron_1mhz_r ) {
	return 0xff;
}

WRITE8_HANDLER( electron_1mhz_w ) {
}

READ8_HANDLER( electron_ula_r ) {
	UINT8 data = ((UINT8 *)memory_region(REGION_USER1))[0x43E00 + offset];
	switch ( offset & 0x0f ) {
	case 0x00:	/* Interrupt status */
		data = ula.interrupt_status;
		ula.interrupt_status &= ~0x02;
		break;
	case 0x01:	/* Unknown */
		break;
	case 0x04:	/* Casette data shift register */
		electron_interrupt_handler( INT_CLEAR, INT_RECEIVE_FULL );
		data = ula.tape_byte;
		break;
	}
	logerror( "ULA: read offset %02x: %02x\n", offset, data );
	return data;
}

static const int electron_palette_offset[4] = { 0, 4, 5, 1 };
static const UINT16 electron_screen_base[8] = { 0x3000, 0x3000, 0x3000, 0x4000, 0x5800, 0x5800, 0x6000, 0x5800 };

WRITE8_HANDLER( electron_ula_w ) {
	int i = electron_palette_offset[(( offset >> 1 ) & 0x03)];
	logerror( "ULA: write offset %02x <- %02x\n", offset & 0x0f, data );
	switch( offset & 0x0f ) {
	case 0x00:	/* Interrupt control */
		ula.interrupt_control = data;
		break;
	case 0x01:	/* Unknown */
		break;
	case 0x02:	/* Screen start address #1 */
		ula.screen_start = ( ula.screen_start & 0x7e00 ) | ( ( data & 0xe0 ) << 1 );
		logerror( "screen_start changed to %04x\n", ula.screen_start );
		break;
	case 0x03:	/* Screen start addres #2 */
		ula.screen_start = ( ula.screen_start & 0x1c0 ) | ( ( data & 0x3f ) << 9 );
		logerror( "screen_start changed to %04x\n", ula.screen_start );
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
		if ( ( ( ula.rompage & 0x0C ) != 0x08 ) || ( data & 0x08 ) ) {
			ula.rompage = data & 0x0f;
			if ( ula.rompage == 8 || ula.rompage == 9 ) {
				ula.rompage = 8;
				memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xbfff, 0, 0, electron_read_keyboard );
			} else {
				memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xbfff, 0, 0, MRA8_BANK2 );
			}
			memory_set_bank(2, ula.rompage);
		}
		if ( data & 0x10 ) {
			electron_interrupt_handler( INT_CLEAR, INT_DISPLAY_END );
		}
		if ( data & 0x20 ) {
			electron_interrupt_handler( INT_CLEAR, INT_RTC );
		}
		if ( data & 0x40 ) {
			electron_interrupt_handler( INT_CLEAR, INT_HIGH_TONE );
		}
		if ( data & 0x80 ) {
		}
		break;
	case 0x06:	/* Counter divider */
		if ( ula.communication_mode == 0x01) {
			beep_set_frequency( 0, 1000000 / ( 16 * ( data + 1 ) ) );
		}
		break;
	case 0x07:	/* Misc. */
		ula.communication_mode = ( data >> 1 ) & 0x03;
		switch( ula.communication_mode ) {
		case 0x00:	/* cassette input */
			beep_set_state( 0, 0 );
			electron_tape_start();
			break;
		case 0x01:	/* sound generation */
			beep_set_state( 0, 1 );
			electron_tape_stop();
			break;
		case 0x02:	/* cassette output */
			beep_set_state( 0, 0 );
			electron_tape_stop();
			break;
		case 0x03:	/* not used */
			beep_set_state( 0, 0 );
			electron_tape_stop();
			break;
		}
		ula.screen_mode = ( data >> 3 ) & 0x07;
		ula.screen_base = electron_screen_base[ ula.screen_mode ];
		ula.screen_size = 0x8000 - ula.screen_base;
		ula.vram = memory_get_read_ptr( 0, ADDRESS_SPACE_PROGRAM, ula.screen_base );
		logerror( "ULA: screen mode set to %d\n", ula.screen_mode );
		ula.cassette_motor_mode = ( data >> 6 ) & 0x01;
		cassette_change_state( cassette_device_image(), ula.cassette_motor_mode ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED, CASSETTE_MOTOR_DISABLED );
		ula.capslock_mode = ( data >> 7 ) & 0x01;
		break;
	case 0x08: case 0x0A: case 0x0C: case 0x0E:
		// video_update
		ula.current_pal[i+10] = (ula.current_pal[i+10] & 0x01) | (((data & 0x80) >> 5) | ((data & 0x08) >> 1));
		ula.current_pal[i+8] = (ula.current_pal[i+8] & 0x01) | (((data & 0x40) >> 4) | ((data & 0x04) >> 1));
		ula.current_pal[i+2] = (ula.current_pal[i+2] & 0x03) | ((data & 0x20) >> 3);
		ula.current_pal[i] = (ula.current_pal[i] & 0x03) | ((data & 0x10) >> 2);
		break;
	case 0x09: case 0x0B: case 0x0D: case 0x0F:
		// video_update
		ula.current_pal[i+10] = (ula.current_pal[i+10] & 0x06) | ((data & 0x08) >> 3);
		ula.current_pal[i+8] = (ula.current_pal[i+8] & 0x06) | ((data & 0x04) >> 2);
		ula.current_pal[i+2] = (ula.current_pal[i+2] & 0x04) | (((data & 0x20) >> 4) | ((data & 0x02) >> 1));  
		ula.current_pal[i] = (ula.current_pal[i] & 0x04) | (((data & 0x10) >> 3) | ((data & 0x01)));
		break;
	}
}

void electron_interrupt_handler(int mode, int interrupt) {
	if ( mode == INT_SET ) {
		ula.interrupt_status |= interrupt;
	} else {
		ula.interrupt_status &= ~interrupt;
	}
	if ( ula.interrupt_status & ula.interrupt_control & ~0x83 ) {
		ula.interrupt_status |= 0x01;
		cpunum_set_input_line( 0, 0, ASSERT_LINE );
	} else {
		ula.interrupt_status &= ~0x01;
		cpunum_set_input_line( 0, 0, CLEAR_LINE );
	}
}

/**************************************
   Machine Initialisation functions
***************************************/

static void setup_beep(int dummy) {
	beep_set_state( 0, 0 );
	beep_set_frequency( 0, 300 );
}

static void electron_reset(running_machine *machine)
{
	memory_set_bank(2, 0);

	ula.communication_mode = 0x04;
	ula.screen_mode = 0;
	ula.cassette_motor_mode = 0;
	ula.capslock_mode = 0;
	ula.scanline = 0;
	ula.screen_mode = 0;
	ula.screen_start = 0x3000;
	ula.screen_base = 0x3000;
	ula.screen_size = 0x8000 - 0x3000;
	ula.screen_addr = 0;
	ula.tape_running = 0;
	ula.vram = memory_get_read_ptr( 0, ADDRESS_SPACE_PROGRAM, ula.screen_base );
	electron_video_init();
}

MACHINE_START( electron )
{
	memory_configure_bank(2, 0, 16, memory_region(REGION_USER1), 0x4000);
	
	ula.interrupt_status = 0x82;
	ula.interrupt_control = 0x00;
	timer_set( 0.0, 0, setup_beep );
	electron_tape_timer = timer_alloc( electron_tape_timer_handler );
	add_reset_callback(machine, electron_reset);
	return 0;
}

