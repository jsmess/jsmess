/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine
  (RAM, ROM, interrupts, I/O ports)

***************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "includes/cgenie.h"
#include "machine/wd17xx.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "sound/ay8910.h"
#include "sound/dac.h"
#include "devices/flopdrv.h"
#include "devices/messram.h"

#define AYWriteReg(chip,port,value) \
	ay8910_address_w(ay8910, 0,port);  \
	ay8910_data_w(ay8910, 0,value)

#define TAPE_HEADER "Colour Genie - Virtual Tape File"




#define IRQ_TIMER		0x80
#define IRQ_FDC 		0x40



static TIMER_CALLBACK( handle_cassette_input )
{
	cgenie_state *state = machine->driver_data<cgenie_state>();
	UINT8 new_level = ( cassette_input( machine->device("cassette") ) > 0.0 ) ? 1 : 0;

	if ( new_level != state->cass_level )
	{
		state->cass_level = new_level;
		state->cass_bit ^= 1;
	}
}


MACHINE_RESET( cgenie )
{
	cgenie_state *state = machine->driver_data<cgenie_state>();
	address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	running_device *ay8910 = machine->device("ay8910");
	UINT8 *ROM = memory_region(machine, "maincpu");

	/* reset the AY8910 to be quiet, since the cgenie BIOS doesn't */
	AYWriteReg(0, 0, 0);
	AYWriteReg(0, 1, 0);
	AYWriteReg(0, 2, 0);
	AYWriteReg(0, 3, 0);
	AYWriteReg(0, 4, 0);
	AYWriteReg(0, 5, 0);
	AYWriteReg(0, 6, 0);
	AYWriteReg(0, 7, 0x3f);
	AYWriteReg(0, 8, 0);
	AYWriteReg(0, 9, 0);
	AYWriteReg(0, 10, 0);

	/* wipe out color RAM */
	memset(&ROM[0x0f000], 0x00, 0x0400);

	/* wipe out font RAM */
	memset(&ROM[0x0f400], 0xff, 0x0400);

	if( input_port_read(machine, "DSW0") & 0x80 )
	{
		logerror("cgenie floppy discs enabled\n");
	}
	else
	{
		logerror("cgenie floppy discs disabled\n");
	}

	/* copy DOS ROM, if enabled or wipe out that memory area */
	if( input_port_read(machine, "DSW0") & 0x40 )
	{
		if ( input_port_read(machine, "DSW0") & 0x80 )
		{
			memory_install_read_bank(space, 0xc000, 0xdfff, 0, 0, "bank10");
			memory_nop_write(space, 0xc000, 0xdfff, 0, 0);
			memory_set_bankptr(machine, "bank10", &ROM[0x0c000]);
			logerror("cgenie DOS enabled\n");
			memcpy(&ROM[0x0c000],&ROM[0x10000], 0x2000);
		}
		else
		{
			memory_nop_readwrite(space, 0xc000, 0xdfff, 0, 0);
			logerror("cgenie DOS disabled (no floppy image given)\n");
		}
	}
	else
	{
		memory_nop_readwrite(space, 0xc000, 0xdfff, 0, 0);
		logerror("cgenie DOS disabled\n");
		memset(&memory_region(machine, "maincpu")[0x0c000], 0x00, 0x2000);
	}

	/* copy EXT ROM, if enabled or wipe out that memory area */
	if( input_port_read(machine, "DSW0") & 0x20 )
	{
		memory_install_rom(space, 0xe000, 0xefff, 0, 0, 0); // mess 0135u3 need to check
		logerror("cgenie EXT enabled\n");
		memcpy(&memory_region(machine, "maincpu")[0x0e000],
			   &memory_region(machine, "maincpu")[0x12000], 0x1000);
	}
	else
	{
		memory_nop_readwrite(space, 0xe000, 0xefff, 0, 0);
		logerror("cgenie EXT disabled\n");
		memset(&memory_region(machine, "maincpu")[0x0e000], 0x00, 0x1000);
	}

	state->cass_level = 0;
	state->cass_bit = 1;
}

MACHINE_START( cgenie )
{
	cgenie_state *state = machine->driver_data<cgenie_state>();
	address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT8 *gfx = memory_region(machine, "gfx2");
	int i;

	/* initialize static variables */
	state->irq_status = 0;
	state->motor_drive = 0;
	state->head = 0;
	state->tv_mode = -1;
	state->port_ff = 0xff;

	/*
     * Every fifth cycle is a wait cycle, so I reduced
     * the overlocking by one fitfth
     * Underclocking causes the tape loading to not work.
     */
//  cpunum_set_clockscale(machine, 0, 0.80);

	/* Initialize some patterns to be displayed in state->graphics mode */
	for( i = 0; i < 256; i++ )
		memset(gfx + i * 8, i, 8);

	/* set up RAM */
	memory_install_read_bank(space, 0x4000, 0x4000 + messram_get_size(machine->device("messram")) - 1, 0, 0, "bank1");
	memory_install_write8_handler(space, 0x4000, 0x4000 + messram_get_size(machine->device("messram")) - 1, 0, 0, cgenie_videoram_w);
	state->videoram = messram_get_ptr(machine->device("messram"));
	memory_set_bankptr(machine, "bank1", messram_get_ptr(machine->device("messram")));
	timer_pulse(machine,  ATTOTIME_IN_HZ(11025), NULL, 0, handle_cassette_input );
}

/*************************************
 *
 *              Port handlers.
 *
 *************************************/

/* used bits on port FF */
#define FF_CAS	0x01		   /* tape output signal */
#define FF_BGD0 0x04		   /* background color enable */
#define FF_CHR1 0x08		   /* charset 0xc0 - 0xff 1:fixed 0:defined */
#define FF_CHR0 0x10		   /* charset 0x80 - 0xbf 1:fixed 0:defined */
#define FF_CHR	(FF_CHR0 | FF_CHR1)
#define FF_FGR	0x20		   /* 1: "hi" resolution state->graphics, 0: text mode */
#define FF_BGD1 0x40		   /* background color select 1 */
#define FF_BGD2 0x80		   /* background color select 2 */
#define FF_BGD	(FF_BGD0 | FF_BGD1 | FF_BGD2)

WRITE8_HANDLER( cgenie_port_ff_w )
{
	cgenie_state *state = space->machine->driver_data<cgenie_state>();
	int port_ff_changed = state->port_ff ^ data;

	cassette_output ( space->machine->device("cassette"), data & 0x01 ? -1.0 : 1.0 );

	/* background bits changed ? */
	if( port_ff_changed & FF_BGD )
	{
		unsigned char r, g, b;

		if( data & FF_BGD0 )
		{
			r = 112;
			g = 0;
			b = 112;
		}
		else
		{
			if( state->tv_mode == 0 )
			{
				switch( data & (FF_BGD1 + FF_BGD2) )
				{
				case FF_BGD1:
					r = 112;
					g = 40;
					b = 32;
					break;
				case FF_BGD2:
					r = 40;
					g = 112;
					b = 32;
					break;
				case FF_BGD1 + FF_BGD2:
					r = 72;
					g = 72;
					b = 72;
					break;
				default:
					r = 0;
					g = 0;
					b = 0;
					break;
				}
			}
			else
			{
				r = 15;
				g = 15;
				b = 15;
			}
		}
		palette_set_color_rgb(space->machine, 0, r, g, b);
	}

	/* character mode changed ? */
	if( port_ff_changed & FF_CHR )
	{
		state->font_offset[2] = (data & FF_CHR0) ? 0x00 : 0x80;
		state->font_offset[3] = (data & FF_CHR1) ? 0x00 : 0x80;
	}

	/* state->graphics mode changed ? */
	if( port_ff_changed & FF_FGR )
	{
		cgenie_mode_select(space->machine, data & FF_FGR);
	}

	state->port_ff = data;
}


READ8_HANDLER( cgenie_port_ff_r )
{
	cgenie_state *state = space->machine->driver_data<cgenie_state>();
	UINT8	data = state->port_ff & ~0x01;

	data |= state->cass_bit;

	return data;
}

int cgenie_port_xx_r( int offset )
{
	return 0xff;
}

/*************************************
 *                                   *
 *      Memory handlers              *
 *                                   *
 *************************************/


READ8_HANDLER( cgenie_psg_port_a_r )
{
	cgenie_state *state = space->machine->driver_data<cgenie_state>();
	return state->psg_a_inp;
}

READ8_HANDLER( cgenie_psg_port_b_r )
{
	cgenie_state *state = space->machine->driver_data<cgenie_state>();
	if( state->psg_a_out < 0xd0 )
	{
		/* comparator value */
		state->psg_b_inp = 0x00;

		if( input_port_read(space->machine, "JOY0") > state->psg_a_out )
			state->psg_b_inp |= 0x80;

		if( input_port_read(space->machine, "JOY1") > state->psg_a_out )
			state->psg_b_inp |= 0x40;

		if( input_port_read(space->machine, "JOY2") > state->psg_a_out )
			state->psg_b_inp |= 0x20;

		if( input_port_read(space->machine, "JOY3") > state->psg_a_out )
			state->psg_b_inp |= 0x10;
	}
	else
	{
		/* read keypad matrix */
		state->psg_b_inp = 0xFF;

		if( !(state->psg_a_out & 0x01) )
			state->psg_b_inp &= ~input_port_read(space->machine, "KP0");

		if( !(state->psg_a_out & 0x02) )
			state->psg_b_inp &= ~input_port_read(space->machine, "KP1");

		if( !(state->psg_a_out & 0x04) )
			state->psg_b_inp &= ~input_port_read(space->machine, "KP2");

		if( !(state->psg_a_out & 0x08) )
			state->psg_b_inp &= ~input_port_read(space->machine, "KP3");

		if( !(state->psg_a_out & 0x10) )
			state->psg_b_inp &= ~input_port_read(space->machine, "KP4");

		if( !(state->psg_a_out & 0x20) )
			state->psg_b_inp &= ~input_port_read(space->machine, "KP5");
	}
	return state->psg_b_inp;
}

WRITE8_HANDLER( cgenie_psg_port_a_w )
{
	cgenie_state *state = space->machine->driver_data<cgenie_state>();
	state->psg_a_out = data;
}

WRITE8_HANDLER( cgenie_psg_port_b_w )
{
	cgenie_state *state = space->machine->driver_data<cgenie_state>();
	state->psg_b_out = data;
}

 READ8_HANDLER( cgenie_status_r )
{
	running_device *fdc = space->machine->device("wd179x");
	/* If the floppy isn't emulated, return 0 */
	if( (input_port_read(space->machine, "DSW0") & 0x80) == 0 )
		return 0;
	return wd17xx_status_r(fdc, offset);
}

 READ8_HANDLER( cgenie_track_r )
{
	running_device *fdc = space->machine->device("wd179x");
	/* If the floppy isn't emulated, return 0xff */
	if( (input_port_read(space->machine, "DSW0") & 0x80) == 0 )
		return 0xff;
	return wd17xx_track_r(fdc, offset);
}

 READ8_HANDLER( cgenie_sector_r )
{
	running_device *fdc = space->machine->device("wd179x");
	/* If the floppy isn't emulated, return 0xff */
	if( (input_port_read(space->machine, "DSW0") & 0x80) == 0 )
		return 0xff;
	return wd17xx_sector_r(fdc, offset);
}

 READ8_HANDLER(cgenie_data_r )
{
	running_device *fdc = space->machine->device("wd179x");
	/* If the floppy isn't emulated, return 0xff */
	if( (input_port_read(space->machine, "DSW0") & 0x80) == 0 )
		return 0xff;
	return wd17xx_data_r(fdc, offset);
}

WRITE8_HANDLER( cgenie_command_w )
{
	running_device *fdc = space->machine->device("wd179x");
	/* If the floppy isn't emulated, return immediately */
	if( (input_port_read(space->machine, "DSW0") & 0x80) == 0 )
		return;
	wd17xx_command_w(fdc, offset, data);
}

WRITE8_HANDLER( cgenie_track_w )
{
	running_device *fdc = space->machine->device("wd179x");
	/* If the floppy isn't emulated, ignore the write */
	if( (input_port_read(space->machine, "DSW0") & 0x80) == 0 )
		return;
	wd17xx_track_w(fdc, offset, data);
}

WRITE8_HANDLER( cgenie_sector_w )
{
	running_device *fdc = space->machine->device("wd179x");
	/* If the floppy isn't emulated, ignore the write */
	if( (input_port_read(space->machine, "DSW0") & 0x80) == 0 )
		return;
	wd17xx_sector_w(fdc, offset, data);
}

WRITE8_HANDLER( cgenie_data_w )
{
	running_device *fdc = space->machine->device("wd179x");
	/* If the floppy isn't emulated, ignore the write */
	if( (input_port_read(space->machine, "DSW0") & 0x80) == 0 )
		return;
	wd17xx_data_w(fdc, offset, data);
}

 READ8_HANDLER( cgenie_irq_status_r )
{
	cgenie_state *state = space->machine->driver_data<cgenie_state>();
int result = state->irq_status;

	state->irq_status &= ~(IRQ_TIMER | IRQ_FDC);
	return result;
}

INTERRUPT_GEN( cgenie_timer_interrupt )
{
	cgenie_state *state = device->machine->driver_data<cgenie_state>();
	if( (state->irq_status & IRQ_TIMER) == 0 )
	{
		state->irq_status |= IRQ_TIMER;
		cputag_set_input_line(device->machine, "maincpu", 0, HOLD_LINE);
	}
}

static WRITE_LINE_DEVICE_HANDLER( cgenie_fdc_intrq_w )
{
	cgenie_state *drvstate = device->machine->driver_data<cgenie_state>();
	/* if disc hardware is not enabled, do not cause an int */
	if (!( input_port_read(device->machine, "DSW0") & 0x80 ))
		return;

	if (state)
	{
		if( (drvstate->irq_status & IRQ_FDC) == 0 )
		{
			drvstate->irq_status |= IRQ_FDC;
			cputag_set_input_line(device->machine, "maincpu", 0, HOLD_LINE);
		}
	}
	else
	{
		drvstate->irq_status &= ~IRQ_FDC;
	}
}

const wd17xx_interface cgenie_wd17xx_interface =
{
	DEVCB_NULL,
	DEVCB_LINE(cgenie_fdc_intrq_w),
	DEVCB_NULL,
	{FLOPPY_0, FLOPPY_1, FLOPPY_2, FLOPPY_3}
};

WRITE8_HANDLER( cgenie_motor_w )
{
	cgenie_state *state = space->machine->driver_data<cgenie_state>();
	running_device *fdc = space->machine->device("wd179x");
	UINT8 drive = 255;

	logerror("cgenie motor_w $%02X\n", data);

	if( data & 1 )
		drive = 0;
	if( data & 2 )
		drive = 1;
	if( data & 4 )
		drive = 2;
	if( data & 8 )
		drive = 3;

	if( drive > 3 )
		return;

	/* mask state->head select bit */
		state->head = (data >> 4) & 1;

	/* currently selected drive */
	state->motor_drive = drive;

	wd17xx_set_drive(fdc,drive);
	wd17xx_set_side(fdc,state->head);
}

/*************************************
 *      Keyboard                     *
 *************************************/
 READ8_HANDLER( cgenie_keyboard_r )
{
	int result = 0;

	if( offset & 0x01 )
		result |= input_port_read(space->machine, "ROW0");

	if( offset & 0x02 )
		result |= input_port_read(space->machine, "ROW1");

	if( offset & 0x04 )
		result |= input_port_read(space->machine, "ROW2");

	if( offset & 0x08 )
		result |= input_port_read(space->machine, "ROW3");

	if( offset & 0x10 )
		result |= input_port_read(space->machine, "ROW4");

	if( offset & 0x20 )
		result |= input_port_read(space->machine, "ROW5");

	if( offset & 0x40 )
		result |= input_port_read(space->machine, "ROW6");

	if( offset & 0x80 )
		result |= input_port_read(space->machine, "ROW7");

	return result;
}

/*************************************
 *      Video RAM                    *
 *************************************/

int cgenie_videoram_r( running_machine *machine, int offset )
{
	cgenie_state *state = machine->driver_data<cgenie_state>();
	UINT8 *videoram = state->videoram;
	return videoram[offset];
}

WRITE8_HANDLER( cgenie_videoram_w )
{
	cgenie_state *state = space->machine->driver_data<cgenie_state>();
	UINT8 *videoram = state->videoram;
	/* write to video RAM */
	if( data == videoram[offset] )
		return; 			   /* no change */
	videoram[offset] = data;
}

 READ8_HANDLER( cgenie_colorram_r )
{
	cgenie_state *state = space->machine->driver_data<cgenie_state>();
	return state->colorram[offset] | 0xf0;
}

WRITE8_HANDLER( cgenie_colorram_w )
{
	cgenie_state *state = space->machine->driver_data<cgenie_state>();
	/* only bits 0 to 3 */
	data &= 15;
	/* nothing changed ? */
	if( data == state->colorram[offset] )
		return;

	/* set new value */
	state->colorram[offset] = data;
	/* make offset relative to video frame buffer offset */
	offset = (offset + (cgenie_get_register(space->machine, 12) << 8) + cgenie_get_register(space->machine, 13)) & 0x3ff;
}

 READ8_HANDLER( cgenie_fontram_r )
{
	cgenie_state *state = space->machine->driver_data<cgenie_state>();
	return state->fontram[offset];
}

WRITE8_HANDLER( cgenie_fontram_w )
{
	cgenie_state *state = space->machine->driver_data<cgenie_state>();
	UINT8 *dp;

	if( data == state->fontram[offset] )
		return; 			   /* no change */

	/* store data */
	state->fontram[offset] = data;

	/* convert eight pixels */
	dp = &space->machine->gfx[0]->gfxdata[(256 * 8 + offset) * space->machine->gfx[0]->width];
	dp[0] = (data & 0x80) ? 1 : 0;
	dp[1] = (data & 0x40) ? 1 : 0;
	dp[2] = (data & 0x20) ? 1 : 0;
	dp[3] = (data & 0x10) ? 1 : 0;
	dp[4] = (data & 0x08) ? 1 : 0;
	dp[5] = (data & 0x04) ? 1 : 0;
	dp[6] = (data & 0x02) ? 1 : 0;
	dp[7] = (data & 0x01) ? 1 : 0;
}

/*************************************
 *
 *      Interrupt handlers.
 *
 *************************************/

INTERRUPT_GEN( cgenie_frame_interrupt )
{
	cgenie_state *state = device->machine->driver_data<cgenie_state>();
	if( state->tv_mode != (input_port_read(device->machine, "DSW0") & 0x10) )
	{
		state->tv_mode = input_port_read(device->machine, "DSW0") & 0x10;
		/* force setting of background color */
		state->port_ff ^= FF_BGD0;
		cgenie_port_ff_w(cputag_get_address_space(device->machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0, state->port_ff ^ FF_BGD0);
	}
}


READ8_DEVICE_HANDLER( cgenie_sh_control_port_r )
{
	cgenie_state *state = device->machine->driver_data<cgenie_state>();
	return state->control_port;
}

WRITE8_DEVICE_HANDLER( cgenie_sh_control_port_w )
{
	cgenie_state *state = device->machine->driver_data<cgenie_state>();
	state->control_port = data;
	ay8910_address_w(device, offset, data);
}
