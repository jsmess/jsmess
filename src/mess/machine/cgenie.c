/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine
  (RAM, ROM, interrupts, I/O ports)

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "includes/cgenie.h"
#include "machine/wd17xx.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "sound/ay8910.h"
#include "sound/dac.h"


#define AYWriteReg(chip,port,value) \
	ay8910_address_w(ay8910, 0,port);  \
	ay8910_data_w(ay8910, 0,value)

#define TAPE_HEADER "Colour Genie - Virtual Tape File"

UINT8 *cgenie_fontram;


int cgenie_tv_mode = -1;
static int port_ff = 0xff;

#define IRQ_TIMER		0x80
#define IRQ_FDC 		0x40
static UINT8 irq_status;

static UINT8 motor_drive;
static UINT8 head;

static UINT8 cass_level;
static UINT8 cass_bit;

static TIMER_CALLBACK( handle_cassette_input )
{
	UINT8 new_level = ( cassette_input( devtag_get_device(machine, "cassette") ) > 0.0 ) ? 1 : 0;

	if ( new_level != cass_level )
	{
		cass_level = new_level;
		cass_bit ^= 1;
	}
}


MACHINE_RESET( cgenie )
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	const device_config *ay8910 = devtag_get_device(machine, "ay8910");
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
			memory_install_read8_handler(space, 0xc000, 0xdfff, 0, 0, SMH_BANK(10));
			memory_install_write8_handler(space, 0xc000, 0xdfff, 0, 0, SMH_NOP);
			memory_set_bankptr(machine, 10, &ROM[0x0c000]);
			logerror("cgenie DOS enabled\n");
			memcpy(&ROM[0x0c000],&ROM[0x10000], 0x2000);
		}
		else
		{
			memory_install_read8_handler(space, 0xc000, 0xdfff, 0, 0, SMH_NOP);
			memory_install_write8_handler(space, 0xc000, 0xdfff, 0, 0, SMH_NOP);
			logerror("cgenie DOS disabled (no floppy image given)\n");
		}
	}
	else
	{
		memory_install_read8_handler(space, 0xc000, 0xdfff, 0, 0, SMH_NOP);
		memory_install_write8_handler(space, 0xc000, 0xdfff, 0, 0, SMH_NOP);
		logerror("cgenie DOS disabled\n");
		memset(&memory_region(machine, "maincpu")[0x0c000], 0x00, 0x2000);
	}

	/* copy EXT ROM, if enabled or wipe out that memory area */
	if( input_port_read(machine, "DSW0") & 0x20 )
	{
		memory_install_read8_handler(space, 0xe000, 0xefff, 0, 0, SMH_ROM);
		memory_install_write8_handler(space, 0xe000, 0xefff, 0, 0, SMH_ROM);
		logerror("cgenie EXT enabled\n");
		memcpy(&memory_region(machine, "maincpu")[0x0e000],
			   &memory_region(machine, "maincpu")[0x12000], 0x1000);
	}
	else
	{
		memory_install_read8_handler(space, 0xe000, 0xefff, 0, 0, SMH_NOP);
		memory_install_write8_handler(space, 0xe000, 0xefff, 0, 0, SMH_NOP);
		logerror("cgenie EXT disabled\n");
		memset(&memory_region(machine, "maincpu")[0x0e000], 0x00, 0x1000);
	}

	cass_level = 0;
	cass_bit = 1;
	timer_pulse(machine,  ATTOTIME_IN_HZ(11025), NULL, 0, handle_cassette_input );
}

MACHINE_START( cgenie )
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT8 *gfx = memory_region(machine, "gfx2");
	int i;

	/* initialize static variables */
	irq_status = 0;
	motor_drive = 0;
	head = 0;

	/*
	 * Every fifth cycle is a wait cycle, so I reduced
	 * the overlocking by one fitfth
	 * Underclocking causes the tape loading to not work.
	 */
//	cpunum_set_clockscale(machine, 0, 0.80);

	/* Initialize some patterns to be displayed in graphics mode */
	for( i = 0; i < 256; i++ )
		memset(gfx + i * 8, i, 8);

	/* set up RAM */
	memory_install_read8_handler(space, 0x4000, 0x4000 + mess_ram_size - 1, 0, 0, SMH_BANK(1));
	memory_install_write8_handler(space, 0x4000, 0x4000 + mess_ram_size - 1, 0, 0, cgenie_videoram_w);
	videoram = mess_ram;
	memory_set_bankptr(machine, 1, mess_ram);
}

/*************************************
 *
 *				Port handlers.
 *
 *************************************/

/* used bits on port FF */
#define FF_CAS	0x01		   /* tape output signal */
#define FF_BGD0 0x04		   /* background color enable */
#define FF_CHR1 0x08		   /* charset 0xc0 - 0xff 1:fixed 0:defined */
#define FF_CHR0 0x10		   /* charset 0x80 - 0xbf 1:fixed 0:defined */
#define FF_CHR	(FF_CHR0 | FF_CHR1)
#define FF_FGR	0x20		   /* 1: "hi" resolution graphics, 0: text mode */
#define FF_BGD1 0x40		   /* background color select 1 */
#define FF_BGD2 0x80		   /* background color select 2 */
#define FF_BGD	(FF_BGD0 | FF_BGD1 | FF_BGD2)

WRITE8_HANDLER( cgenie_port_ff_w )
{
	int port_ff_changed = port_ff ^ data;

	cassette_output ( devtag_get_device(space->machine, "cassette"), data & 0x01 ? -1.0 : 1.0 );

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
			if( cgenie_tv_mode == 0 )
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
		cgenie_font_offset[2] = (data & FF_CHR0) ? 0x00 : 0x80;
		cgenie_font_offset[3] = (data & FF_CHR1) ? 0x00 : 0x80;
	}

	/* graphics mode changed ? */
	if( port_ff_changed & FF_FGR )
	{
		cgenie_mode_select(data & FF_FGR);
	}

	port_ff = data;
}


READ8_HANDLER( cgenie_port_ff_r )
{
	UINT8	data = port_ff & ~0x01;

	data |= cass_bit;

	return data;
}

int cgenie_port_xx_r( int offset )
{
	return 0xff;
}

/*************************************
 *									 *
 *		Memory handlers 			 *
 *									 *
 *************************************/

static UINT8 psg_a_out = 0x00;
static UINT8 psg_b_out = 0x00;
static UINT8 psg_a_inp = 0x00;
static UINT8 psg_b_inp = 0x00;

READ8_HANDLER( cgenie_psg_port_a_r )
{
	return psg_a_inp;
}

READ8_HANDLER( cgenie_psg_port_b_r )
{
	if( psg_a_out < 0xd0 )
	{
		/* comparator value */
		psg_b_inp = 0x00;

		if( input_port_read(space->machine, "JOY0") > psg_a_out )
			psg_b_inp |= 0x80;

		if( input_port_read(space->machine, "JOY1") > psg_a_out )
			psg_b_inp |= 0x40;

		if( input_port_read(space->machine, "JOY2") > psg_a_out )
			psg_b_inp |= 0x20;

		if( input_port_read(space->machine, "JOY3") > psg_a_out )
			psg_b_inp |= 0x10;
	}
	else
	{
		/* read keypad matrix */
		psg_b_inp = 0xFF;

		if( !(psg_a_out & 0x01) )
			psg_b_inp &= ~input_port_read(space->machine, "KP0");

		if( !(psg_a_out & 0x02) )
			psg_b_inp &= ~input_port_read(space->machine, "KP1");

		if( !(psg_a_out & 0x04) )
			psg_b_inp &= ~input_port_read(space->machine, "KP2");

		if( !(psg_a_out & 0x08) )
			psg_b_inp &= ~input_port_read(space->machine, "KP3");

		if( !(psg_a_out & 0x10) )
			psg_b_inp &= ~input_port_read(space->machine, "KP4");

		if( !(psg_a_out & 0x20) )
			psg_b_inp &= ~input_port_read(space->machine, "KP5");
	}
	return psg_b_inp;
}

WRITE8_HANDLER( cgenie_psg_port_a_w )
{
	psg_a_out = data;
}

WRITE8_HANDLER( cgenie_psg_port_b_w )
{
	psg_b_out = data;
}

 READ8_HANDLER( cgenie_status_r )
{
	const device_config *fdc = devtag_get_device(space->machine, "wd179x");
	/* If the floppy isn't emulated, return 0 */
	if( (input_port_read(space->machine, "DSW0") & 0x80) == 0 )
		return 0;
	return wd17xx_status_r(fdc, offset);
}

 READ8_HANDLER( cgenie_track_r )
{
	const device_config *fdc = devtag_get_device(space->machine, "wd179x");
	/* If the floppy isn't emulated, return 0xff */
	if( (input_port_read(space->machine, "DSW0") & 0x80) == 0 )
		return 0xff;
	return wd17xx_track_r(fdc, offset);
}

 READ8_HANDLER( cgenie_sector_r )
{
	const device_config *fdc = devtag_get_device(space->machine, "wd179x");
	/* If the floppy isn't emulated, return 0xff */
	if( (input_port_read(space->machine, "DSW0") & 0x80) == 0 )
		return 0xff;
	return wd17xx_sector_r(fdc, offset);
}

 READ8_HANDLER(cgenie_data_r )
{
	const device_config *fdc = devtag_get_device(space->machine, "wd179x");
	/* If the floppy isn't emulated, return 0xff */
	if( (input_port_read(space->machine, "DSW0") & 0x80) == 0 )
		return 0xff;
	return wd17xx_data_r(fdc, offset);
}

WRITE8_HANDLER( cgenie_command_w )
{
	const device_config *fdc = devtag_get_device(space->machine, "wd179x");
	/* If the floppy isn't emulated, return immediately */
	if( (input_port_read(space->machine, "DSW0") & 0x80) == 0 )
		return;
	wd17xx_command_w(fdc, offset, data);
}

WRITE8_HANDLER( cgenie_track_w )
{
	const device_config *fdc = devtag_get_device(space->machine, "wd179x");
	/* If the floppy isn't emulated, ignore the write */
	if( (input_port_read(space->machine, "DSW0") & 0x80) == 0 )
		return;
	wd17xx_track_w(fdc, offset, data);
}

WRITE8_HANDLER( cgenie_sector_w )
{
	const device_config *fdc = devtag_get_device(space->machine, "wd179x");
	/* If the floppy isn't emulated, ignore the write */
	if( (input_port_read(space->machine, "DSW0") & 0x80) == 0 )
		return;
	wd17xx_sector_w(fdc, offset, data);
}

WRITE8_HANDLER( cgenie_data_w )
{
	const device_config *fdc = devtag_get_device(space->machine, "wd179x");
	/* If the floppy isn't emulated, ignore the write */
	if( (input_port_read(space->machine, "DSW0") & 0x80) == 0 )
		return;
	wd17xx_data_w(fdc, offset, data);
}

 READ8_HANDLER( cgenie_irq_status_r )
{
int result = irq_status;

	irq_status &= ~(IRQ_TIMER | IRQ_FDC);
	return result;
}

INTERRUPT_GEN( cgenie_timer_interrupt )
{
	if( (irq_status & IRQ_TIMER) == 0 )
	{
		irq_status |= IRQ_TIMER;
		cputag_set_input_line(device->machine, "maincpu", 0, HOLD_LINE);
	}
}

static  WD17XX_CALLBACK( cgenie_fdc_callback )
{
	/* if disc hardware is not enabled, do not cause an int */
	if (!( input_port_read(device->machine, "DSW0") & 0x80 ))
		return;

	switch( state )
	{
		case WD17XX_IRQ_CLR:
			irq_status &= ~IRQ_FDC;
			break;
		case WD17XX_IRQ_SET:
			if( (irq_status & IRQ_FDC) == 0 )
			{
				irq_status |= IRQ_FDC;
				cputag_set_input_line(device->machine, "maincpu", 0, HOLD_LINE);
			}
			break;
		case WD17XX_DRQ_CLR:
		case WD17XX_DRQ_SET:
			/* do nothing */
			break;
	}
}

const wd17xx_interface cgenie_wd17xx_interface = { cgenie_fdc_callback, NULL };

WRITE8_HANDLER( cgenie_motor_w )
{
	const device_config *fdc = devtag_get_device(space->machine, "wd179x");
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

	/* mask head select bit */
		head = (data >> 4) & 1;

	/* currently selected drive */
	motor_drive = drive;

	wd17xx_set_drive(fdc,drive);
	wd17xx_set_side(fdc,head);
}

/*************************************
 *		Keyboard					 *
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
 *		Video RAM					 *
 *************************************/

int cgenie_videoram_r( int offset )
{
	return videoram[offset];
}

WRITE8_HANDLER( cgenie_videoram_w )
{
	/* write to video RAM */
	if( data == videoram[offset] )
		return; 			   /* no change */
	videoram[offset] = data;
}

 READ8_HANDLER( cgenie_colorram_r )
{
	return colorram[offset] | 0xf0;
}

WRITE8_HANDLER( cgenie_colorram_w )
{
	/* only bits 0 to 3 */
	data &= 15;
	/* nothing changed ? */
	if( data == colorram[offset] )
		return;

	/* set new value */
	colorram[offset] = data;
	/* make offset relative to video frame buffer offset */
	offset = (offset + (cgenie_get_register(12) << 8) + cgenie_get_register(13)) & 0x3ff;
}

 READ8_HANDLER( cgenie_fontram_r )
{
	return cgenie_fontram[offset];
}

WRITE8_HANDLER( cgenie_fontram_w )
{
	UINT8 *dp;

	if( data == cgenie_fontram[offset] )
		return; 			   /* no change */

	/* store data */
	cgenie_fontram[offset] = data;

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
 *		Interrupt handlers.
 *
 *************************************/

INTERRUPT_GEN( cgenie_frame_interrupt )
{
	if( cgenie_tv_mode != (input_port_read(device->machine, "DSW0") & 0x10) )
	{
		cgenie_tv_mode = input_port_read(device->machine, "DSW0") & 0x10;
		/* force setting of background color */
		port_ff ^= FF_BGD0;
		cgenie_port_ff_w(cputag_get_address_space(device->machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0, port_ff ^ FF_BGD0);
	}
}
