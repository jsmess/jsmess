/***************************************************************************

There are three IRQ sources:
- IRQ0
- IRQ1 = IRQA from the video PIA
- IRQ2 = IRQA from the IEEE488 PIA

Interrupt handling on the Osborne-1 is a bit akward. When an interrupt is
taken by the Z80 the ROMMODE is enabled on each fetch of an instruction
byte. During execution of an instruction the previous ROMMODE setting
is used.

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "machine/6821pia.h"
#include "machine/6850acia.h"
#include "machine/wd17xx.h"
#include "devices/basicdsk.h"
#include "cpu/z80/z80daisy.h"
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
	UINT8	beep;
	/* bankswitch setting */
	UINT8	bankswitch;
	UINT8	in_irq_handler;
} osborne1;

/* Prototypes */
static void osborne1_z80_reset(int);
static int osborne1_z80_irq_state(int);
static int osborne1_z80_irq_ack(int);
static void osborne1_z80_irq_reti(int);

const struct z80_irq_daisy_chain osborne1_daisy_chain[] = {
	{ osborne1_z80_reset, osborne1_z80_irq_state, osborne1_z80_irq_ack, osborne1_z80_irq_reti, 0 },
	{ NULL, NULL, NULL, NULL, -1 }
};


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
		data = mess_ram[ 0x2000 + offset ];
	} else {
		switch( offset & 0x0F00 ) {
		case 0x100:	/* Floppy */
			data = wd17xx_r( offset );
			break;
		case 0x200:	/* Keyboard */
			/* Row 0 */
			if ( offset & 0x01 )	data &= readinputport(0);
			/* Row 1 */
			if ( offset & 0x02 )	data &= readinputport(1);
			/* Row 2 */
			if ( offset & 0x04 )	data &= readinputport(3);
			/* Row 3 */
			if ( offset & 0x08 )	data &= readinputport(4);
			/* Row 4 */
			if ( offset & 0x10 )	data &= readinputport(5);
			/* Row 5 */
			if ( offset & 0x20 )	data &= readinputport(2);
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
	}
	return data;
}

WRITE8_HANDLER( osborne1_2000_w ) {
	/* Check whether regular RAM is enabled */
	if ( ! osborne1.bank2_enabled ) {
		mess_ram[ 0x2000 + offset ] = data;
	} else {
		/* Handle writes to the I/O area */
		switch( offset & 0x0F00 ) {
		case 0x100:	/* Floppy */
			wd17xx_w( offset, data );
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
	osborne1.bankswitch = offset;
}

static OPBASE_HANDLER( osborne1_opbase ) {
	if ( ( address & 0xF000 ) && osborne1.in_irq_handler ) {
		osborne1.in_irq_handler = 0;
		osborne1_bankswitch_w( osborne1.bankswitch, 0 );
	}
	return address;
}

static void osborne1_z80_reset(int param) {
	osborne1.pia_1_irq_state = 0;
	osborne1.in_irq_handler = 0;
}

static int osborne1_z80_irq_state(int param) {
	return ( osborne1.pia_1_irq_state ? Z80_DAISY_INT : 0 );
}

static int osborne1_z80_irq_ack(int param) {
	/* Enable ROM and I/O when IRQ is acknowledged */
	UINT8	old_bankswitch = osborne1.bankswitch;
	osborne1.in_irq_handler = 1;
	osborne1_bankswitch_w( 0, 0 );
	osborne1.bankswitch = old_bankswitch;
	return 0xF8;
}

static void osborne1_z80_irq_reti(int param) {
}

static void osborne1_update_irq_state(void) {
	//logerror("Changing irq state; pia_0_irq_state = %s, pia_1_irq_state = %s\n", osborne1.pia_0_irq_state ? "SET" : "CLEARED", osborne1.pia_1_irq_state ? "SET" : "CLEARED" );

	if ( osborne1.pia_1_irq_state ) {
		cpunum_set_input_line( 0, 0, ASSERT_LINE );
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

static WRITE8_HANDLER( video_pia_out_cb2_dummy ) {
}

static WRITE8_HANDLER( video_pia_port_a_w ) {
	osborne1.new_start_x = data >> 1;
	wd17xx_set_density( ( data & 0x01 ) ? DEN_FM_LO : DEN_FM_HI );

	//logerror("Video pia port a write: %02X, density set to %s\n", data, data & 1 ? "DEN_FM_LO" : "DEN_FM_HI" );
}

static WRITE8_HANDLER( video_pia_port_b_w ) {
	osborne1.new_start_y = data & 0x1F;
	osborne1.beep = data & 0x20;
	if ( data & 0x40 ) {
		wd17xx_set_drive( 0 );
	} else if ( data & 0x80 ) {
		wd17xx_set_drive( 1 );
	}
	//logerror("Video pia port b write: %02X\n", data );
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
	video_pia_out_cb2_dummy,	/* out_cb2_func */
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
		osborne1.start_y = ( osborne1.new_start_y - 1 ) & 0x1F;
		osborne1.charline = 0;
		pia_1_ca1_w( 0, 0 );
	}
	if ( y == 240 ) {
		/* Set CA1 on video PIA */
		pia_1_ca1_w( 0, 0xFF );
	}
	if ( y < 240 ) {
		/* Draw a line of the display */
		UINT16 address = osborne1.start_y * 128 + osborne1.new_start_x + 11;
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

/*
 * The Osborne-1 supports the following disc formats:
 * - Osborne single density: 40 tracks, 10 sectors per track, 256-byte sectors (100 KByte)
 * - Osborne double density: 40 tracks, 5 sectors per track, 1024-byte sectors (200 KByte)
 * - IBM Personal Computer: 40 tracks, 8 sectors per track, 512-byte sectors (160 KByte)
 * - Xerox 820 Computer: 40 tracks, 18 sectors per track, 128-byte sectors (90 KByte)
 * - DEC 1820 double density: 40 tracks, 9 sectors per track, 512-byte sectors (180 KByte)
 *
 */
DEVICE_LOAD( osborne1_floppy ) {
	int size, sectors, sectorsize;

	if ( ! image_has_been_created( image ) ) {
		size = image_length( image );

		switch( size ) {
		case 40 * 10 * 256:
			sectors = 10;
			sectorsize = 256;
			wd17xx_set_density( DEN_FM_LO );
			break;
		case 40 * 5 * 1024:
			sectors = 5;
			sectorsize = 1024;
			wd17xx_set_density( DEN_FM_HI );
			break;
		case 40 * 8 * 512:
			sectors = 8;
			sectorsize = 512;
			wd17xx_set_density( DEN_FM_LO );
			return INIT_FAIL;
		case 40 * 18 * 128:
			sectors = 18;
			sectorsize = 128;
			wd17xx_set_density( DEN_FM_LO );
			return INIT_FAIL;
		case 40 * 9 * 512:
			sectors = 9;
			sectorsize = 512;
			wd17xx_set_density( DEN_FM_HI );
			return INIT_FAIL;
		default:
			return INIT_FAIL;
		}
	} else {
		return INIT_FAIL;
	}

	if ( device_load_basicdsk_floppy( image ) != INIT_PASS ) {
		return INIT_FAIL;
	}

	basicdsk_set_geometry( image, 40, 1, sectors, sectorsize, 1, 0, FALSE );

	return INIT_PASS;
}

MACHINE_RESET( osborne1 ) {
	/* Initialize memory configuration */
	osborne1_bankswitch_w( 0x00, 0 );

	osborne1.pia_0_irq_state = FALSE;
	osborne1.pia_1_irq_state = FALSE;

	osborne1.charrom = memory_region( REGION_GFX1 );

	osborne1.video_timer = mame_timer_alloc( osborne1_video_callback );
	mame_timer_adjust( osborne1.video_timer, video_screen_get_time_until_pos( 0, 1, 0 ), 0, time_never );
	pia_1_ca1_w( 0, 0 );
}

DRIVER_INIT( osborne1 ) {
	memset( &osborne1, 0, sizeof( osborne1 ) );

	osborne1.empty_4K = auto_malloc( 0x1000 );
	memset( osborne1.empty_4K, 0xFF, 0x1000 );

	/* configure the 6821 PIAs */
	pia_config( 0, &osborne1_ieee_pia_config );
	pia_config( 1, &osborne1_video_pia_config );

	/* Configure the 6850 ACIA */
//	acia6850_config( 0, &osborne1_6850_config );

	/* Configure the floppy disk interface */
	wd17xx_init( WD_TYPE_MB8877, NULL, NULL );

	/* Set opbase handler to handle special irq cases */
	memory_set_opbase_handler( 0, osborne1_opbase );
}

