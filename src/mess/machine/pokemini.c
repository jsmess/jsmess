#include "driver.h"
#include "includes/pokemini.h"
#include "cpu/minx/minx.h"

struct TIMERS {
	UINT8	seconds_running;	/* Seconds timer running */
	UINT32	seconds_counter;	/* Seconds timer counter */
	mame_timer	*seconds_timer;
	UINT8	hz256_running;		/* 256Hz timer running */
	UINT8	hz256_counter;		/* 256Hz timer counter */
	mame_timer	*hz256_timer;
	UINT8	timers_enabled;		/* Timers 1 to 3 enabled */
};

UINT8 pokemini_hwreg[0x100];
struct VDP vdp;
static struct TIMERS timers;

static void pokemini_seconds_timer_callback( int dummy ) {
	timers.seconds_counter += 1;
}

static void pokemini_256hz_timer_callback( int dummy ) {
	timers.hz256_counter += 1;
}

MACHINE_RESET( pokemini ) {
	memset( &vdp, 0, sizeof(vdp) );
	memset( &timers, 0, sizeof(timers) );
	timers.seconds_timer = mame_timer_alloc( pokemini_seconds_timer_callback );
	timers.hz256_timer = mame_timer_alloc( pokemini_256hz_timer_callback );
}

WRITE8_HANDLER( pokemini_hwreg_w ) {
	logerror( "%0X: Write to hardware address: %02X, %02X\n", activecpu_get_pc(), offset, data );
	switch( offset ) {
	case 0x00:	/* start-up contrast
			   Bit 0-1 R/W Must be 1(?)
			   Bit 2-7 R/W Start up contrast (doesn't affect contrast until after reboot)
			*/
	case 0x01:	/* CPU related?
			   Bit 0-7 R/W Unknown
			*/
	case 0x02:	/* CPU related?
			   Bit 0-7 R/W Unknown
			*/
		logerror( "%0X: Write to unknown hardware address: %02X, %02X\n", activecpu_get_pc(), offset, data );
		break;
	case 0x08:	/* Seconds-timer control
			   Bit 0   R/W Timer enable
			   Bit 1   W   Timer reset
			   Bit 2-7     Unused
			*/
		if ( data & 0x01 ) {	/* enable timer */
			if ( ! timers.seconds_running ) {
				timer_adjust( timers.seconds_timer, TIME_IN_SEC(1), 0, 0 );
				timers.seconds_running = 1;
			}
		} else {		/* pause timer */
			if ( timers.seconds_running ) {
				timer_reset( timers.seconds_timer, TIME_NEVER );
				timers.seconds_running = 0;
			}
		}
		if ( data & 0x02 ) {
			timers.seconds_counter = 0;
			data &= ~0x02;
		}
		break;
	case 0x09:	/* Seconds-timer (low)
			   Bit 0-7 R   Seconds timer bit 0-7
			*/
	case 0x0A:	/* Seconds-timer (mid)
			   Bit 0-7 R   Seconds timer bit 8-15
			*/
	case 0x0B:	/* Seconds-timer (high)
			   Bit 0-7 R   Seconds timer bit 16-23
			*/
		/* ignore writes to these registers */
		return;
	case 0x10:	/* Low power detector
			   Bit 0-4 R/W Unknown
			   Bit 5   R   Battery status: 0 - battery OK, 1 - battery low
			   Bit 6-7     Unused
			*/
	case 0x18:	/* Timer 1 pre-scale + enable
			   Bit 0-2 R/W 000 - 2000000 Hz - 2 cycles
			               001 -  500000 Hz - 8 cycles
			               010 -  125000 Hz - 32 cycles
			               011 -   62500 Hz - 64 cycles
			               100 -   31250 Hz - 128 cycles
			               101 -   15625 Hz - 256 cycles
			               110 -    3906.25 Hz - 1024 cycles
			               111 -     976.5625 Hz - 4096 cycles
			   Bit 3   R/W Enable counting
			   Bit 4-7 R/W Unused
			*/
		logerror( "%0X: Write to unknown hardware address: %02X, %02X\n", activecpu_get_pc(), offset, data );
		break;
	case 0x19:	/* Timers 1 to 3 enabler
			   Bit 0   R/W SET IT TO "0"
			   Bit 1-3     Unused
			   Bit 4   R/W SET IT TO "0"
			   Bit 5   R/W Enable timers 1 to 3 circuits
			   Bit 6-7     Unused
			*/
		timers.timers_enabled = ( data & 0x20 ) ? 1 : 0;
		break;
	case 0x1A:	/* Timer 2 pre-scale + enable
			   Bit 0-2 R/W 000 - 2000000 Hz - 2 cycles
			               001 -  500000 Hz - 8 cycles
			               010 -  125000 Hz - 32 cycles
			               011 -   62500 Hz - 64 cycles
			               100 -   31250 Hz - 128 cycles
			               101 -   15625 Hz - 256 cycles
			               110 -    3906.25 Hz - 1024 cycles
			               111 -     976.5625 Hz - 4096 cycles
			   Bit 3   R/W Enable counting
			   Bit 4-7     Unused
			*/
	case 0x1C:	/* Timer 3 pre-scale + enable
			   Bit 0-2 R/W 000 - 2000000 Hz - 2 cycles
			               001 -  500000 Hz - 8 cycles
			               010 -  125000 Hz - 32 cycles
			               011 -   62500 Hz - 64 cycles
			               100 -   31250 Hz - 128 cycles
			               101 -   15625 Hz - 256 cycles
			               110 -    3906.25 Hz - 1024 cycles
			               111 -     976.5625 Hz - 4096 cycles
			   Bit 3   R/W Enable counting
			   Bit 4-7     Unused
			*/
		logerror( "%0X: Write to unknown hardware address: %02X, %02X\n", activecpu_get_pc(), offset, data );
		break;
	case 0x20:	/* Event #1-#8 primary enable
			   Bit 0-1 R/W Timer 3 overflow Interrupt #7-#8
			   Bit 2-3 R/W Timer 1 overflow Interrupt #5-#6
			   Bit 4-5 R/W Timer 2 overflow Interrupt #3-#4
			   Bit 6-7 R/W VDraw/VBlank trigger Interrupt #1-#2
			*/
		break;
	case 0x21:	/* Event #15-#22 primary enable
			   Bit 0-1 R/W Unknown
			   Bit 2-3 R/W All keypad interrupts - Interrupt #15-#22
			   Bit 4-7 R/W Unknown
			*/
		break;
	case 0x22:	/* Event #9-#14 primary enable
			   Bit 0-1 R/W All #9 - #14 events - Interrupt #9-#14
			   Bit 2-7     Unused
			*/
		break;
	case 0x23:	/* Event #1-#8 secondary enable
			   Bit 0   R/W Timer 3 overflow (mirror) - Enable Interrupt #8
			   Bit 1   R/W Timer 3 overflow - Enable Interrupt #7
			   Bit 2   R/W Not called... - Enable Interrupt #6
			   Bit 3   R/W Timer 1 overflow - Enable Interrupt #5
			   Bit 4   R/W Not called... - Enable Interrupt #4
			   Bit 5   R/W Timer 2 overflow - Enable Interrupt #3
			   Bit 6   R/W V-Draw trigger - Enable Interrupt #2
			   Bit 7   R/W V-Blank trigger - Enable Interrupt #1
			*/
		break;
	case 0x24:	/* Event #9-#12 secondary enable
			   Bit 0-5 R/W Unknown
			   Bit 6-7     Unused
			*/
		break;
	case 0x25:	/* Event #15-#22 secondary enable
			   Bit 0   R/W Press key "A" event - Enable interrupt #22
			   Bit 1   R/W Press key "B" event - Enable interrupt #21
			   Bit 2   R/W Press key "C" event - Enable interrupt #20
			   Bit 3   R/W Press D-pad up key event - Enable interrupt #19
			   Bit 4   R/W Press D-pad down key event - Enable interrupt #18
			   Bit 5   R/W Press D-pad left key event - Enable interrupt #17
			   Bit 6   R/W Press D-pad right key event - Enable interrupt #16
			   Bit 7   R/W Press power button event - Enable interrupt #15
			*/
		break;
	case 0x26:	/* Event #13-#14 secondary enable
			   Bit 0-2 R/W Unknown
			   Bit 3       Unused
			   Bit 4-5 R/W Unknown
			   Bit 6   R/W Shock detector trigger - Enable interrupt #14
			   Bit 7   R/W IR receiver - low to high trigger - Enable interrupt #13
			*/
		break;
	case 0x27:	/* Interrupt flag #1-#8
			   Bit 0       Timer 3 overflow (mirror) / Clear interrupt #8
			   Bit 1       Timer 3 overflow / Clear interrupt #7
			   Bit 2       Not called ... / Clear interrupt #6
			   Bit 3       Timer 1 overflow / Clear interrupt #5
			   Bit 4       Not called ... / Clear interrupt #4
			   Bit 5       Timer 2 overflow / Clear interrupt #3
			   Bit 6       VDraw trigger / Clear interrupt #2
			   Bit 7       VBlank trigger / Clear interrupt #1
			*/
	case 0x28:	/* Interrupt flag #9-#12
			   Bit 0-1     Unknown
			   Bit 2       Unknown / Clear interrupt #12
			   Bit 3       Unknown / Clear interrupt #11
			   Bit 4       Unknown / Clear interrupt #10
			   Bit 5       Unknown / Clear interrupt #9
			   Bit 6-7     Unknown
			*/
	case 0x29:	/* Interrupt flag #15-#22
			   Bit 0       Press key "A" event / Clear interrupt #22
			   Bit 1       Press key "B" event / Clear interrupt #21
			   Bit 2       Press key "C" event / Clear interrupt #20
			   Bit 3       Press D-pad up key event / Clear interrupt #19
			   Bit 4       Press D-pad down key event / Clear interrupt #18
			   Bit 5       Press D-pad left key event / Clear interrupt #17
			   Bit 6       Press D-pad right key event / Clear interrupt #16
			   Bit 7       Press power button event / Clear interrupt #15
			*/
	case 0x2A:	/* Interrupt flag #13-#14
			   Bit 0-5     Unknown
			   Bit 6       Shock detector trigger / Clear interrupt #14
			   Bit 7       Unknown / Clear interrupt #13
			*/
	case 0x30:	/* Timer 1 control 1
			   Bit 0   R/W Unknown
			   Bit 1       Unused
			   Bit 2   W   Preset timer
			   Bit 3-6     Unused
			   Bit 7   R/W Enable timer 1 counting
			*/
	case 0x31:	/* Timer 1 control 2
			   Bit 0-2     Unused
			   Bit 3   R/W Unknown
			   Bit 4-7     Unused
			*/
	case 0x32:	/* Timer 1 preset value (low)
			   Bit 0-7 R/W Timer 1 preset value bit 0-7
			*/
	case 0x33:	/* Timer 1 preset value (high)
			   Bit 0-7 R/W Timer 1 preset value bit 8-15
			*/
	case 0x34:	/* Timer 1 sound-pivot (low, unused)
			*/
	case 0x35:	/* Timer 1 sound-pivot (high, unused)
			*/
	case 0x36:	/* Timer 1 counter (low)
			   Bit 0-7 R/W Timer 1 counter value bit 0-7
			*/
	case 0x37:	/* Timer 1 counter (high)
			   Bit 0-7 R/W Timer 1 counter value bit 8-15
			*/
	case 0x38:	/* Timer 2 control 1
			   Bit 0   R/W Unknown
			   Bit 1       Unused
			   Bit 2   W   Preset Timer
			   Bit 3-6     Unused
			   Bit 7   R/W Enable timer 2 counting
			*/
	case 0x39:	/* Timer 2 control 2
			   Bit 0-2     Unused
			   Bit 3   R/W Unknown
			   Bit 4-7     Unused
			*/
	case 0x3A:	/* Timer 2 preset value (low)
			   Bit 0-7 R/W Timer 2 preset value bit 0-7
			*/
	case 0x3B:	/* Timer 2 preset value (high)
			   Bit 0-7 R/W Timer 2 preset value bit 8-15
			*/
	case 0x3C:	/* Timer 2 sound-pivot (low, unused)
			*/
	case 0x3D:	/* Timer 2 sound-pivot (high, unused)
			*/
	case 0x3E:	/* Timer 2 counter (low)
			   Bit 0-7 R/W Timer 2 counter value bit 0-7
			*/
	case 0x3F:	/* Timer 2 counter (high)
			   Bit 0-7 R/W Timer 2 counter value bit 8-15
			*/
		logerror( "%0X: Write to unknown hardware address: %02X, %02X\n", activecpu_get_pc(), offset, data );
		break;
	case 0x40:	/* 256Hz timer control
			   Bit 0   R/W Enable Timer
			   Bit 1   W   Reset Timer
			   Bit 2-7     Unused
			*/
		if ( data & 0x01 ) {	/* enable timer */
			if ( ! timers.hz256_running ) {
				timer_adjust( timers.hz256_timer, TIME_IN_HZ(256), 0, 0 );
				timers.hz256_running = 1;
			}
		} else {		/* pause timer */
			if ( timers.hz256_running ) {
				timer_reset( timers.hz256_timer, TIME_NEVER );
				timers.hz256_running = 0;
			}
		}
		if ( data & 0x02 ) {
			timers.hz256_counter = 0;
			data &= ~0x02;
		}
		break;
	case 0x41:	/* 256Hz timer counter
			   Bit 0-7 R   256Hz timer counter
			*/
		return;
	case 0x48:	/* Timer 3 control 1
			   Bit 0   R/W Unknown
			   Bit 1       Unused
			   Bit 2   W   Preset timer
			   Bit 3-6     Unused
			   Bit 7   R/W Enable timer 3 counting
			*/
	case 0x49:	/* Timer 3 control 2
			   Bit 0-2     Unused
			   Bit 3   R/W Unknown
			   Bit 4-7     Unused
			*/
	case 0x4A:	/* Timer 3 preset value (low)
			   Bit 0-7 R/W Timer 3 preset value bit 0-7
			*/
	case 0x4B:	/* Timer 3 preset value (high)
			   Bit 0-7 R/W Timer 3 preset value bit 8-15
			*/
	case 0x4C:	/* Timer 3 sound-pivot (low)
			   Bit 0-7 R/W Timer 3 sound-pivot value bit 0-7
			*/
	case 0x4D:	/* Timer 3 sound-pivot (high)
			   Bit 0-7 R/W Timer 3 sound-pivot value bit 8-15

			   Sound-pivot location:
			   Pulse-Width of 0% = 0x0000
			   Pulse-Width of 50% = Half of preset-value
			   Pulse-Width of 100% = Same as preset-value
			*/
	case 0x4E:	/* Timer 3 counter (low)
			   Bit 0-7 R/W Timer 3 counter value bit 0-7
			*/
	case 0x4F:	/* Timer 3 counter (high)
			   Bit 0-7 R/W Timer 3 counter value bit 8-15
			*/
		logerror( "%0X: Write to unknown hardware address: %02X, %02X\n", activecpu_get_pc(), offset, data );
		break;
	case 0x52:	/* Keypad status
			   Bit 0   R   Key "A"
			   Bit 1   R   Key "B"
			   Bit 2   R   Key "C"
			   Bit 3   R   D-pad up
			   Bit 4   R   D-pad down
			   Bit 5   R   D-pad left
			   Bit 6   R   D-pad right
			   Bit 7   R   Power button
			*/
		return;
	case 0x60:	/* I/O peripheral circuit select
			   Bit 0   R/W Unknown
			   bit 1   R/W IR receive / transmit
			   Bit 2   R/W EEPROM / RTC data
			   Bit 3   R/W EEPROM / RTC clock
			   Bit 4   R/W Rumble controller
			   Bit 5   R/W IR enable/disable
			   Bit 6   R/W Unknown
			   Bit 7   R/W Unknown
			*/
	case 0x61:	/* I/O peripheral status control
			   Bit 0   R/W IR received bit (if device not selected: 0)
			   Bit 1   R/W IR transmit (if device not selected: 0)
			   Bit 2   R/W EEPROM / RTC data (if device not selected: 1)
			   Bit 3   R/W EEPROM / RTC clock (if device not selected: 0)
			   Bit 4   R/W Rumble on/off (if device not selected: 0)
			   Bit 5   R/W IR disable (receive & transmit) (if device not selected: 1)
			   Bit 6       Always 1
			   Bit 7   R/W IR received bit (mirror, if device not selected: 0)
			*/
	case 0x71:	/* Sound volume
			   Bit 0-1 R/W Sound volume
			               00 - 0%
			               01 - 50%
			               10 - 50%
			               11 - 100%
			   Bit 2   R/W Always set to 0
			   Bit 3-7     Unused
			*/
		logerror( "%0X: Write to unknown hardware address: %02X, %02X\n", activecpu_get_pc(), offset, data );
		break;
	case 0x80:	/* LCD control
			   Bit 0   R/W Invert colors; 0 - normal, 1 - inverted
			   Bit 1   R/W Enable rendering of background
			   Bit 2   R/W Enable rendering of sprites
			   Bit 3   R/W Global rendering enable
			   Bit 4-5 R/W Map size
			               00 - 12x16
			               01 - 16x12
			               10 - 24x8
			               11 - 24x8 (prohibited code)
			  Bit 6-7      Unused
			*/
		vdp.colors_inverted = ( data & 0x01 ) ? 1 : 0;
		vdp.background_enabled = ( data & 0x02 ) ? 1 : 0;
		vdp.sprites_enabled = ( data & 0x04 ) ? 1 : 0;
		vdp.rendering_enabled = ( data & 0x08 ) ? 1 : 0;
		vdp.map_size = ( data >> 4 ) & 0x03;
		break;
	case 0x81:	/* LCD render refresh rate
			   Bit 0   R/W Unknown
			   Bit 1-3 R/W LCD refresh rate divider
			               000 - 60Hz / 3 = 20Hz (0 - 2)
			               001 - 60Hz / 6 = 10Hz (0 - 5)
			               010 - 60Hz / 9 = 6,6Hz (0 - 8)
			               011 - 60Hz / 12 = 5Hz (0 - 8)
			               100 - 60Hz / 2 = 30Hz (0 - 1)
			               101 - 60Hz / 4 = 15Hz (0 - 3)
			               110 - 60Hz / 6 = 10Hz (0 - 5)
			               111 - 60Hz / 8 = 7,5Hz (0 - 7)
			   Bit 4-7 R   Divider position, when overflow the LCD is updated
			*/
	case 0x82:	/* BG tile data memory offset (low)
			   Bit 0-2     Always "0"
			   Bit 3-7 R/W BG tile data memory offset bit 3-7
			*/
	case 0x83:	/* BG tile data memory offset (mid)
			   Bit 0-7 R/W BG tile data memory offset bit 8-15
			*/
	case 0x84:	/* BG tile data memory offset (high)
			   Bit 0-4 R/W BG tile data memory offset bit 16-20
			   Bit 5-7     Unused
			*/
	case 0x85:	/* BG vertical move
			   Bit 0-6 R/W Move the background up, move range:
			               Map size 0: 0x00 to 0x40
			               Map size 1: 0x00 to 0x20
			               Map size 2: move ignored
			   Bit 7       Unused
			*/
	case 0x86:	/* BG horizontal move
			   Bit 0-6 R/W Move the background left, move range:
			               Map size 0: move ignored
			               Map size 1: 0x00 to 0x20
			               Map size 2: 0x00 to 0x60
			   Bit 7       Unused
			*/
	case 0x87:	/* Sprite tile data memory offset (low)
			   Bit 0-5     Always "0"
			   Bit 6-7 R/W Sprite tile data memory offset bit 6-7
			*/
	case 0x88:	/* Sprite tile data memory offset (med)
			   Bit 0-7 R/W Sprite tile data memory offset bit 8-15
			*/
	case 0x89:	/* Sprite tile data memory offset (high)
			   Bit 0-4 R/W Sprite tile data memory offset bit 16-20
			   Bit 5-7     Unused
			*/
	case 0x8A:	/* LCD status
			   Bit 0   R   Unknown
			   Bit 1   R   Unknown
			   Bit 2   R   Unknown
			   Bit 3   R   Unknown
			   Bit 4   R   LCD during V-Sync
			   Bit 5   R   Unknown
			   Bit 6-7     Unused
			*/
	case 0xFE:	/* Direct LCD control / data
			   Bit 0-7 R/W Direct LCD command or data
			*/
	case 0xFF:	/* Direct LCD data
			   Bit 0-7 R/W Direct LCD data
			*/
	default:
		logerror( "%0X: Write to unknown hardware address: %02X, %02X\n", activecpu_get_pc(), offset, data );
		break;
	}
	pokemini_hwreg[offset] = data;
}

READ8_HANDLER( pokemini_hwreg_r ) {
	logerror( "%0X: Read from unknown hardware address: %02X\n", activecpu_get_pc(), offset );
	switch( offset ) {
	case 0x09:	return timers.seconds_counter & 0xFF;
	case 0x0A:	return ( timers.seconds_counter >> 8 ) & 0xFF;
	case 0x0B:	return ( timers.seconds_counter >> 16 ) & 0xFF;
	case 0x41:	return timers.hz256_counter;
	case 0x52:	return readinputport( 0 );
	}
	return pokemini_hwreg[offset];
}

DEVICE_INIT( pokemini_cart ) {
	return INIT_PASS;
}

DEVICE_LOAD( pokemini_cart ) {
	return INIT_PASS;
}

