/***************************************************************************

There are three IRQ sources:
- IRQ0
- IRQ1 = IRQA from the video PIA
- IRQ2 = IRQA from the IEEE488 PIA

Interrupt handling on the Osborne-1 is a bit akward. When an interrupt is
taken by the Z80 the ROMMODE is enabled on each fetch of an instruction
byte. During execution of an instruction the previous ROMMODE setting seems
to be used. Side effect of this is that when an interrupt is taken and the
stack pointer is pointing to 0000-3FFF then the return address will still
be written to RAM if RAM was switched in.

***************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/6821pia.h"
#include "machine/6850acia.h"
#include "machine/wd17xx.h"
#include "cpu/z80/z80daisy.h"
#include "sound/beep.h"
#include "includes/osborne1.h"
#include "devices/flopdrv.h"
#include "devices/messram.h"

#define RAMMODE		(0x01)


WRITE8_HANDLER( osborne1_0000_w )
{
	osborne1_state *state = space->machine->driver_data<osborne1_state>();
	/* Check whether regular RAM is enabled */
	if ( ! state->bank2_enabled || ( state->in_irq_handler && state->bankswitch == RAMMODE ) )
	{
		messram_get_ptr(space->machine->device("messram"))[ offset ] = data;
	}
}


WRITE8_HANDLER( osborne1_1000_w )
{
	osborne1_state *state = space->machine->driver_data<osborne1_state>();
	/* Check whether regular RAM is enabled */
	if ( ! state->bank2_enabled || ( state->in_irq_handler && state->bankswitch == RAMMODE ) )
	{
		messram_get_ptr(space->machine->device("messram"))[ 0x1000 + offset ] = data;
	}
}


READ8_HANDLER( osborne1_2000_r )
{
	osborne1_state *state = space->machine->driver_data<osborne1_state>();
	UINT8	data = 0xFF;
	running_device *fdc = space->machine->device("mb8877");
	running_device *pia_0 = space->machine->device("pia_0" );
	running_device *pia_1 = space->machine->device("pia_1" );

	/* Check whether regular RAM is enabled */
	if ( ! state->bank2_enabled )
	{
		data = messram_get_ptr(space->machine->device("messram"))[ 0x2000 + offset ];
	}
	else
	{
		switch( offset & 0x0F00 )
		{
		case 0x100:	/* Floppy */
			data = wd17xx_r( fdc, offset );
			break;
		case 0x200:	/* Keyboard */
			/* Row 0 */
			if ( offset & 0x01 )	data &= input_port_read(space->machine, "ROW0");
			/* Row 1 */
			if ( offset & 0x02 )	data &= input_port_read(space->machine, "ROW1");
			/* Row 2 */
			if ( offset & 0x04 )	data &= input_port_read(space->machine, "ROW3");
			/* Row 3 */
			if ( offset & 0x08 )	data &= input_port_read(space->machine, "ROW4");
			/* Row 4 */
			if ( offset & 0x10 )	data &= input_port_read(space->machine, "ROW5");
			/* Row 5 */
			if ( offset & 0x20 )	data &= input_port_read(space->machine, "ROW2");
			/* Row 6 */
			if ( offset & 0x40 )	data &= input_port_read(space->machine, "ROW6");
			/* Row 7 */
			if ( offset & 0x80 )	data &= input_port_read(space->machine, "ROW7");
			break;
		case 0x900:	/* IEEE488 PIA */
			data = pia6821_r( pia_0, offset & 0x03 );
			break;
		case 0xA00:	/* Serial */
			break;
		case 0xC00:	/* Video PIA */
			data = pia6821_r( pia_1, offset & 0x03 );
			break;
		}
	}
	return data;
}


WRITE8_HANDLER( osborne1_2000_w )
{
	osborne1_state *state = space->machine->driver_data<osborne1_state>();
	running_device *fdc = space->machine->device("mb8877");
	running_device *pia_0 = space->machine->device("pia_0" );
	running_device *pia_1 = space->machine->device("pia_1" );

	/* Check whether regular RAM is enabled */
	if ( ! state->bank2_enabled )
	{
		messram_get_ptr(space->machine->device("messram"))[ 0x2000 + offset ] = data;
	}
	else
	{
		if ( state->in_irq_handler && state->bankswitch == RAMMODE )
		{
			messram_get_ptr(space->machine->device("messram"))[ 0x2000 + offset ] = data;
		}
		/* Handle writes to the I/O area */
		switch( offset & 0x0F00 )
		{
		case 0x100:	/* Floppy */
			wd17xx_w( fdc, offset, data );
			break;
		case 0x900:	/* IEEE488 PIA */
			pia6821_w( pia_0, offset & 0x03, data );
			break;
		case 0xA00:	/* Serial */
			break;
		case 0xC00:	/* Video PIA */
			pia6821_w( pia_1, offset & 0x03, data );
			break;
		}
	}
}


WRITE8_HANDLER( osborne1_3000_w )
{
	osborne1_state *state = space->machine->driver_data<osborne1_state>();
	/* Check whether regular RAM is enabled */
	if ( ! state->bank2_enabled || ( state->in_irq_handler && state->bankswitch == RAMMODE ) )
	{
		messram_get_ptr(space->machine->device("messram"))[ 0x3000 + offset ] = data;
	}
}


WRITE8_HANDLER( osborne1_videoram_w )
{
	osborne1_state *state = space->machine->driver_data<osborne1_state>();
	/* Check whether the video attribute section is enabled */
	if ( state->bank3_enabled )
	{
		data |= 0x7F;
	}
	state->bank4_ptr[offset] = data;
}


WRITE8_HANDLER( osborne1_bankswitch_w )
{
	osborne1_state *state = space->machine->driver_data<osborne1_state>();
	switch( offset )
	{
	case 0x00:
		state->bank2_enabled = 1;
		state->bank3_enabled = 0;
		break;
	case 0x01:
		state->bank2_enabled = 0;
		state->bank3_enabled = 0;
		break;
	case 0x02:
		state->bank2_enabled = 1;
		state->bank3_enabled = 1;
		break;
	case 0x03:
		state->bank2_enabled = 1;
		state->bank3_enabled = 0;
		break;
	}
	if ( state->bank2_enabled )
	{
		memory_set_bankptr(space->machine,"bank1", memory_region(space->machine, "maincpu") );
		memory_set_bankptr(space->machine,"bank2", state->empty_4K );
		memory_set_bankptr(space->machine,"bank3", state->empty_4K );
	}
	else
	{
		memory_set_bankptr(space->machine,"bank1", messram_get_ptr(space->machine->device("messram")) );
		memory_set_bankptr(space->machine,"bank2", messram_get_ptr(space->machine->device("messram")) + 0x1000 );
		memory_set_bankptr(space->machine,"bank3", messram_get_ptr(space->machine->device("messram")) + 0x3000 );
	}
	state->bank4_ptr = messram_get_ptr(space->machine->device("messram")) + ( ( state->bank3_enabled ) ? 0x10000 : 0xF000 );
	memory_set_bankptr(space->machine,"bank4", state->bank4_ptr );
	state->bankswitch = offset;
	state->in_irq_handler = 0;
}


DIRECT_UPDATE_HANDLER( osborne1_opbase )
{
	osborne1_state *state = machine->driver_data<osborne1_state>();
	if ( ( address & 0xF000 ) == 0x2000 )
	{
		if ( ! state->bank2_enabled )
		{
			direct.explicit_configure(0x2000, 0x2fff, 0x0fff, messram_get_ptr(machine->device("messram")) + 0x2000);
			return ~0;
		}
	}
	return address;
}


static void osborne1_update_irq_state(running_machine *machine)
{
	osborne1_state *state = machine->driver_data<osborne1_state>();
	//logerror("Changing irq state; pia_0_irq_state = %s, pia_1_irq_state = %s\n", state->pia_0_irq_state ? "SET" : "CLEARED", state->pia_1_irq_state ? "SET" : "CLEARED" );

	if ( state->pia_1_irq_state )
	{
		cputag_set_input_line(machine, "maincpu", 0, ASSERT_LINE );
	}
	else
	{
		cputag_set_input_line(machine, "maincpu", 0, CLEAR_LINE );
	}
}


static WRITE_LINE_DEVICE_HANDLER( ieee_pia_irq_a_func )
{
	osborne1_state *drvstate = device->machine->driver_data<osborne1_state>();
	drvstate->pia_0_irq_state = state;
	osborne1_update_irq_state(device->machine);
}


const pia6821_interface osborne1_ieee_pia_config =
{
	DEVCB_NULL,							/* in_a_func */
	DEVCB_NULL,							/* in_b_func */
	DEVCB_NULL,							/* in_ca1_func */
	DEVCB_NULL,							/* in_cb1_func */
	DEVCB_NULL,							/* in_ca2_func */
	DEVCB_NULL,							/* in_cb2_func */
	DEVCB_NULL,							/* out_a_func */
	DEVCB_NULL,							/* out_b_func */
	DEVCB_NULL,							/* out_ca2_func */
	DEVCB_NULL,							/* out_cb2_func */
	DEVCB_LINE(ieee_pia_irq_a_func),	/* irq_a_func */
	DEVCB_NULL							/* irq_b_func */
};


static WRITE8_DEVICE_HANDLER( video_pia_out_cb2_dummy )
{
}


static WRITE8_DEVICE_HANDLER( video_pia_port_a_w )
{
	osborne1_state *state = device->machine->driver_data<osborne1_state>();
	running_device *fdc = device->machine->device("mb8877");

	state->new_start_x = data >> 1;
	wd17xx_dden_w(fdc, BIT(data, 0));

	//logerror("Video pia port a write: %02X, density set to %s\n", data, data & 1 ? "FM" : "MFM" );
}


static WRITE8_DEVICE_HANDLER( video_pia_port_b_w )
{
	osborne1_state *state = device->machine->driver_data<osborne1_state>();
	running_device *fdc = device->machine->device("mb8877");

	state->new_start_y = data & 0x1F;
	state->beep = ( data & 0x20 ) ? 1 : 0;
	if ( data & 0x40 )
	{
		wd17xx_set_drive( fdc, 0 );
	}
	else if ( data & 0x80 )
	{
		wd17xx_set_drive( fdc, 1 );
	}
	//logerror("Video pia port b write: %02X\n", data );
}


static WRITE_LINE_DEVICE_HANDLER( video_pia_irq_a_func )
{
	osborne1_state *drvstate = device->machine->driver_data<osborne1_state>();
	drvstate->pia_1_irq_state = state;
	osborne1_update_irq_state(device->machine);
}


const pia6821_interface osborne1_video_pia_config =
{
	DEVCB_NULL,								/* in_a_func */
	DEVCB_NULL,								/* in_b_func */
	DEVCB_NULL,								/* in_ca1_func */
	DEVCB_NULL,								/* in_cb1_func */
	DEVCB_NULL,								/* in_ca2_func */
	DEVCB_NULL,								/* in_cb2_func */
	DEVCB_HANDLER(video_pia_port_a_w),		/* out_a_func */
	DEVCB_HANDLER(video_pia_port_b_w),		/* out_b_func */
	DEVCB_NULL,								/* out_ca2_func */
	DEVCB_HANDLER(video_pia_out_cb2_dummy),	/* out_cb2_func */
	DEVCB_LINE(video_pia_irq_a_func),		/* irq_a_func */
	DEVCB_NULL								/* irq_b_func */
};


//static const struct aica6850_interface osborne1_6850_config =
//{
//  10, /* tx_clock */
//  10, /* rx_clock */
//  NULL,   /* rx_pin */
//  NULL,   /* tx_pin */
//  NULL,   /* cts_pin */
//  NULL,   /* rts_pin */
//  NULL,   /* dcd_pin */
//  NULL    /* int_callback */
//};


static TIMER_CALLBACK(osborne1_video_callback)
{
	osborne1_state *state = machine->driver_data<osborne1_state>();
	address_space* space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	running_device *speaker = space->machine->device("beep");
	running_device *pia_1 = space->machine->device("pia_1");
	int y = machine->primary_screen->vpos();

	/* Check for start of frame */
	if ( y == 0 )
	{
		/* Clear CA1 on video PIA */
		state->start_y = ( state->new_start_y - 1 ) & 0x1F;
		state->charline = 0;
		pia6821_ca1_w( pia_1, 0 );
	}
	if ( y == 240 )
	{
		/* Set CA1 on video PIA */
		pia6821_ca1_w( pia_1, 1 );
	}
	if ( y < 240 )
	{
		/* Draw a line of the display */
		UINT16 address = state->start_y * 128 + state->new_start_x + 11;
		UINT16 *p = BITMAP_ADDR16( machine->generic.tmpbitmap, y, 0 );
		int x;

		for ( x = 0; x < 52; x++ )
		{
			UINT8	character = messram_get_ptr(machine->device("messram"))[ 0xF000 + ( ( address + x ) & 0xFFF ) ];
			UINT8	cursor = character & 0x80;
			UINT8	dim = messram_get_ptr(machine->device("messram"))[ 0x10000 + ( ( address + x ) & 0xFFF ) ] & 0x80;
			UINT8	bits = state->charrom[ state->charline * 128 + ( character & 0x7F ) ];
			int		bit;

			if ( cursor && state->charline == 9 )
			{
				bits = 0xFF;
			}
			for ( bit = 0; bit < 8; bit++ )
			{
				p[x * 8 + bit] = ( bits & 0x80 ) ? ( dim ? 1 : 2 ) : 0;
				bits = bits << 1;
			}
		}

		state->charline += 1;
		if ( state->charline == 10 )
		{
			state->start_y += 1;
			state->charline = 0;
		}
	}

	if ( ( y % 10 ) == 2 || ( y % 10 ) == 6 )
	{
		beep_set_state( speaker, state->beep );
	}
	else
	{
		beep_set_state( speaker, 0 );
	}

	timer_adjust_oneshot(state->video_timer, machine->primary_screen->time_until_pos(y + 1, 0 ), 0);
}

static TIMER_CALLBACK( setup_osborne1 )
{
	running_device *speaker = machine->device("beep");
	running_device *pia_1 = machine->device("pia_1");

	beep_set_state( speaker, 0 );
	beep_set_frequency( speaker, 300 /* 60 * 240 / 2 */ );
	pia6821_ca1_w( pia_1, 0 );
}

static void osborne1_load_proc(device_image_interface &image)
{
	int size = image.length();
	running_device *fdc = image.device().machine->device("mb8877");

	switch( size )
	{
	case 40 * 10 * 256:
		wd17xx_dden_w(fdc, ASSERT_LINE);
		break;
	case 40 * 5 * 1024:
		wd17xx_dden_w(fdc, CLEAR_LINE);
		break;
	case 40 * 8 * 512:
		wd17xx_dden_w(fdc, ASSERT_LINE);
		break;
	case 40 * 18 * 128:
		wd17xx_dden_w(fdc, ASSERT_LINE);
		break;
	case 40 * 9 * 512:
		wd17xx_dden_w(fdc, CLEAR_LINE);
		break;
	}
}

MACHINE_RESET( osborne1 )
{
	osborne1_state *state = machine->driver_data<osborne1_state>();
	int drive;
	address_space* space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	/* Initialize memory configuration */
	osborne1_bankswitch_w( space, 0x00, 0 );

	state->pia_0_irq_state = FALSE;
	state->pia_1_irq_state = FALSE;

	state->pia_1_irq_state = 0;
	state->in_irq_handler = 0;

	state->charrom = memory_region( machine, "gfx1" );

	memset( messram_get_ptr(machine->device("messram")) + 0x10000, 0xFF, 0x1000 );

	for(drive=0;drive<2;drive++)
	{
		floppy_install_load_proc(floppy_get_device(machine, drive), osborne1_load_proc);
	}

	space->set_direct_update_handler(direct_update_delegate_create_static(osborne1_opbase, *machine));
}


DRIVER_INIT( osborne1 )
{
	osborne1_state *state = machine->driver_data<osborne1_state>();

	state->empty_4K = auto_alloc_array(machine, UINT8, 0x1000 );
	memset( state->empty_4K, 0xFF, 0x1000 );

	/* Configure the 6850 ACIA */
//  acia6850_config( 0, &osborne1_6850_config );
	state->video_timer = timer_alloc(machine,  osborne1_video_callback , NULL);
	timer_adjust_oneshot(state->video_timer, machine->primary_screen->time_until_pos(1, 0 ), 0);

	timer_set(machine,  attotime_zero, NULL, 0, setup_osborne1 );
}


/****************************************************************
    Osborne1 specific daisy chain code
****************************************************************/

//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  osborne1_daisy_device_config - constructor
//-------------------------------------------------

osborne1_daisy_device_config::osborne1_daisy_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
	: device_config(mconfig, static_alloc_device_config, "Osborne 1 daisy", tag, owner, clock),
	  device_config_z80daisy_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  static_alloc_device_config - allocate a new
//  configuration object
//-------------------------------------------------

device_config *osborne1_daisy_device_config::static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
{
	return global_alloc(osborne1_daisy_device_config(mconfig, tag, owner, clock));
}


//-------------------------------------------------
//  alloc_device - allocate a new device object
//-------------------------------------------------

device_t *osborne1_daisy_device_config::alloc_device(running_machine &machine) const
{
	return auto_alloc(&machine, osborne1_daisy_device(machine, *this));
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  z80ctc_device - constructor
//-------------------------------------------------

osborne1_daisy_device::osborne1_daisy_device(running_machine &_machine, const osborne1_daisy_device_config &_config)
	: device_t(_machine, _config),
	  device_z80daisy_interface(_machine, _config, *this),
	  m_config(_config)
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void osborne1_daisy_device::device_start()
{
}

//**************************************************************************
//  DAISY CHAIN INTERFACE
//**************************************************************************

//-------------------------------------------------
//  z80daisy_irq_state - return the overall IRQ
//  state for this device
//-------------------------------------------------

int osborne1_daisy_device::z80daisy_irq_state()
{
	osborne1_state *state = machine->driver_data<osborne1_state>();
	return ( state->pia_1_irq_state ? Z80_DAISY_INT : 0 );
}


//-------------------------------------------------
//  z80daisy_irq_ack - acknowledge an IRQ and
//  return the appropriate vector
//-------------------------------------------------

int osborne1_daisy_device::z80daisy_irq_ack()
{
	osborne1_state *state = machine->driver_data<osborne1_state>();
	/* Enable ROM and I/O when IRQ is acknowledged */
	UINT8 old_bankswitch = state->bankswitch;
	address_space* space = cputag_get_address_space(device().machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	osborne1_bankswitch_w( space, 0, 0 );
	state->bankswitch = old_bankswitch;
	state->in_irq_handler = 1;
	return 0xF8;
}

//-------------------------------------------------
//  z80daisy_irq_reti - clear the interrupt
//  pending state to allow other interrupts through
//-------------------------------------------------

void osborne1_daisy_device::z80daisy_irq_reti()
{
}

const device_type OSBORNE1_DAISY = osborne1_daisy_device_config::static_alloc_device_config;
