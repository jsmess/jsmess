/***************************************************************************

There are three IRQ sources:
- IRQ0
- IRQ1 = IRQA from the video PIA
- IRQ2 = IRQA from the IEEE488 PIA

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "machine/6821pia.h"
#include "machine/6850acia.h"
#include "includes/osborne1.h"

static struct osborne1 {
	UINT8	bank2_enabled;
	UINT8	bank3_enabled;
	UINT8	*bank4_ptr;
	UINT8	*empty_4K;
	/* IRQ states */
	int		pia_0_irq_state;
	int		pia_1_irq_state;
	/* video related */
	UINT8	new_start_x;
	UINT8	new_start_y;
	mame_timer	*video_timer;
	UINT8	*charrom;
	UINT8	charline;
	UINT8	start_y;
	/* other outputs from the video pia */
	UINT8	dden;
	UINT8	beep;
	UINT8	drive1;
	UINT8	drive2;
} osborne1;

WRITE8_HANDLER( osborne1_0000_w ) {
	/* Check whether regular RAM is enabled */
	if ( ! osborne1.bank2_enabled ) {
		mess_ram[ offset ] = data;
	}
}

WRITE8_HANDLER( osborne1_1000_w ) {
	/* Check whether regular RAM is enabled */
	if ( ! osborne1.bank2_enabled ) {
		mess_ram[ 0x1000 + offset ] = data;
	}
}

READ8_HANDLER( osborne1_2000_r ) {
	UINT8	data = 0xFF;

	/* Check whether regular RAM is enabled */
	if ( ! osborne1.bank2_enabled ) {
		return mess_ram[ 0x2000 + offset ];
	}
	switch( offset & 0x0F00 ) {
	case 0x100:	/* Floppy */
		break;
	case 0x200:	/* Keyboard */
		/* Row 0 */
		if ( offset & 0x01 )	data &= readinputport(0);
		/* Row 1 */
		if ( offset & 0x02 )	data &= readinputport(1);
		/* Row 2 */
		if ( offset & 0x04 )	data &= readinputport(2);
		/* Row 3 */
		if ( offset & 0x08 )	data &= readinputport(3);
		/* Row 4 */
		if ( offset & 0x10 )	data &= readinputport(4);
		/* Row 5 */
		if ( offset & 0x20 )	data &= readinputport(5);
		/* Row 6 */
		if ( offset & 0x40 )	data &= readinputport(6);
		/* Row 7 */
		if ( offset & 0x80 )	data &= readinputport(7);
		break;
	case 0x900:	/* IEEE488 PIA */
		data = pia_0_r( offset & 0x03 );
		break;
	case 0xA00:	/* Serial */
		break;
	case 0xC00:	/* Video PIA */
		data = pia_1_r( offset & 0x03 );
		break;
	}
	return data;
}

WRITE8_HANDLER( osborne1_2000_w ) {
	/* Check whether regular RAM is enabled */
	if ( ! osborne1.bank2_enabled ) {
		mess_ram[ 0x2000 + offset ] = data;
	}
	/* Handle writes to the I/O area */
	switch( offset & 0x0F00 ) {
	case 0x100:	/* Floppy */
		break;
	case 0x900:	/* IEEE488 PIA */
		pia_0_w( offset & 0x03, data );
		break;
	case 0xA00:	/* Serial */
		break;
	case 0xC00:	/* Video PIA */
		pia_1_w( offset & 0x03, data );
		break;
	}
}

WRITE8_HANDLER( osborne1_3000_w ) {
	/* Check whether regular RAM is enabled */
	if ( ! osborne1.bank2_enabled ) {
		mess_ram[ 0x3000 + offset ] = data;
	}
}

WRITE8_HANDLER( osborne1_videoram_w ) {
	/* Check whether the video attribute section is enabled */
	if ( osborne1.bank3_enabled ) {
		data |= 0x7F;
	}
	osborne1.bank4_ptr[offset] = data;
}

WRITE8_HANDLER( osborne1_bankswitch_w ) {
	switch( offset ) {
	case 0x00:
		osborne1.bank2_enabled = 1;
		osborne1.bank3_enabled = 0;
		break;
	case 0x01:
		osborne1.bank2_enabled = 0;
		osborne1.bank3_enabled = 0;
		break;
	case 0x02:
		osborne1.bank2_enabled = 1;
		osborne1.bank3_enabled = 1;
		break;
	case 0x03:
		osborne1.bank2_enabled = 1;
		osborne1.bank3_enabled = 0;
		break;
	}
	if ( osborne1.bank2_enabled ) {
		memory_set_bankptr( 1, memory_region(REGION_CPU1) );
		memory_set_bankptr( 2, osborne1.empty_4K );
		memory_set_bankptr( 3, osborne1.empty_4K );
	} else {
		memory_set_bankptr( 1, mess_ram );
		memory_set_bankptr( 2, mess_ram + 0x1000 );
		memory_set_bankptr( 3, mess_ram + 0x3000 );
	}
	osborne1.bank4_ptr = mess_ram + ( ( osborne1.bank3_enabled ) ? 0x10000 : 0xF000 );
	memory_set_bankptr( 4, osborne1.bank4_ptr );
}

static void osborne1_update_irq_state(void) {
	logerror("Changing irq state; pia_0_irq_state = %s, pia_1_irq_state = %s\n", osborne1.pia_0_irq_state ? "SET" : "CLEARED", osborne1.pia_1_irq_state ? "SET" : "CLEARED" );

	if ( osborne1.pia_1_irq_state ) {
		cpunum_set_input_line_and_vector( 0, 0, ASSERT_LINE, 0xF8 );
	} else {
		cpunum_set_input_line( 0, 0, CLEAR_LINE );
	}
}

static void ieee_pia_irq_a_func(int state) {
	osborne1.pia_0_irq_state = state;
	osborne1_update_irq_state();
}

static const pia6821_interface osborne1_ieee_pia_config = {
	NULL,	/* in_a_func */
	NULL,	/* in_b_func */
	NULL,	/* in_ca1_func */
	NULL,	/* in_cb1_func */
	NULL,	/* in_ca2_func */
	NULL,	/* in_cb2_func */
	NULL,	/* out_a_func */
	NULL,	/* out_b_func */
	NULL,	/* out_ca2_func */
	NULL,	/* out_cb2_func */
	ieee_pia_irq_a_func,	/* irq_a_func */
	NULL	/* irq_b_func */
};

static WRITE8_HANDLER( video_pia_port_a_w ) {
	osborne1.new_start_x = data >> 1;
	osborne1.dden = data & 0x01;

	logerror("Video pia port a write: %02X\n", data );
}

static WRITE8_HANDLER( video_pia_port_b_w ) {
	osborne1.new_start_y = data & 0x1F;
	osborne1.beep = data & 0x20;
	osborne1.drive1 = data & 0x40;
	osborne1.drive2 = data & 0x80;

	logerror("Video pia port b write: %02X\n", data );
}

static void video_pia_irq_a_func(int state) {
	osborne1.pia_1_irq_state = state;
	osborne1_update_irq_state();
}

static const pia6821_interface osborne1_video_pia_config = {
	NULL,	/* in_a_func */
	NULL,	/* in_b_func */
	NULL,	/* in_ca1_func */
	NULL,	/* in_cb1_func */
	NULL,	/* in_ca2_func */
	NULL,	/* in_cb2_func */
	video_pia_port_a_w,	/* out_a_func */
	video_pia_port_b_w,	/* out_b_func */
	NULL,	/* out_ca2_func */
	NULL,	/* out_cb2_func */
	video_pia_irq_a_func,	/* irq_a_func */
	NULL	/* irq_b_func */
};

//static struct aica6850_interface osborne1_6850_config = {
//	10,	/* tx_clock */
//	10,	/* rx_clock */
//	NULL,	/* rx_pin */
//	NULL,	/* tx_pin */
//	NULL,	/* cts_pin */
//	NULL,	/* rts_pin */
//	NULL,	/* dcd_pin */
//	NULL	/* int_callback */
//};

static TIMER_CALLBACK(osborne1_video_callback) {
	int y = video_screen_get_vpos(0);

	/* Check for start of frame */
	if ( y == 0 ) {
		/* Clear CA1 on video PIA */
		osborne1.start_y = osborne1.new_start_y;
		osborne1.charline = 0;
		logerror("Clear CA1 on video PIA\n");
		pia_1_ca1_w( 0, 0 );
	}
	if ( y == 240 ) {
		/* Set CA1 on video PIA */
		logerror("Set CA1 on video PIA\n");
		pia_1_ca1_w( 0, 0xFF );
	}
	if ( y < 240 ) {
		/* Draw a line of the display */
		UINT16 address = osborne1.start_y * 128 + osborne1.new_start_x + 10;
		UINT16 *p = BITMAP_ADDR16( tmpbitmap, y, 0 );
		int x;

		for ( x = 0; x < 52; x++ ) {
			UINT8	character = mess_ram[ 0xF000 + ( ( address + x ) & 0xFFF ) ];
			UINT8	cursor = character & 0x80;
			UINT8	dim = mess_ram[ 0x10000 + ( ( address + x ) & 0xFFF ) ];
			UINT8	bits = osborne1.charrom[ osborne1.charline * 128 + character ];
			int		bit;

			if ( cursor && osborne1.charline == 8 ) {
				bits = 0xFF;
			}
			for ( bit = 0; bit < 8; bit++ ) {
				p[x * 8 + bit] = ( bits & 0x80 ) ? ( dim ? 2 : 1 ) : 0;
				bits = bits << 1;
			}
		}

		osborne1.charline += 1;
		if ( osborne1.charline == 10 ) {
			osborne1.start_y += 1;
			osborne1.charline = 0;
		}
	}

	mame_timer_adjust( osborne1.video_timer, video_screen_get_time_until_pos( 0, y + 1, 0 ), 0, time_never );
}

MACHINE_RESET( osborne1 ) {
	/* Initialize memory configuration */
	osborne1_bankswitch_w( 0x00, 0 );

	osborne1.pia_0_irq_state = FALSE;
	osborne1.pia_1_irq_state = FALSE;

	osborne1.charrom = memory_region( REGION_GFX1 );

	osborne1.video_timer = mame_timer_alloc( osborne1_video_callback );
	mame_timer_adjust( osborne1.video_timer, video_screen_get_time_until_pos( 0, 240, 0 ), 0, time_never );
	pia_1_ca1_w( 0, 0 );
}

DRIVER_INIT( osborne1 ) {
	osborne1.empty_4K = auto_malloc( 0x1000 );
	memset( osborne1.empty_4K, 0xFF, 0x1000 );

	/* configure the 6821 PIAs */
	pia_config( 0, &osborne1_ieee_pia_config );
	pia_config( 1, &osborne1_video_pia_config );

	/* Configure the 6850 ACIA */
//	acia6850_config( 0, &osborne1_6850_config );
}

