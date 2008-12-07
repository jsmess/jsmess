/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine
  (RAM, ROM, interrupts, I/O ports)

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "includes/cgenie.h"
#include "machine/wd17xx.h"
#include "devices/basicdsk.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "sound/ay8910.h"
#include "sound/dac.h"


#define AYWriteReg(chip,port,value) \
	ay8910_control_port_0_w(space, 0,port);  \
	ay8910_write_port_0_w(space, 0,value)

#define TAPE_HEADER "Colour Genie - Virtual Tape File"

UINT8 *cgenie_fontram;


int cgenie_tv_mode = -1;
static int port_ff = 0xff;

#define CGENIE_DRIVE_INFO



/********************************************************
   abbreviations used:
   GPL	Granules Per Lump
   GAT	Granule Allocation Table
   GATL GAT Length
   GATM GAT Mask
   DDGA Disk Directory Granule Allocation
*********************************************************/
typedef struct
{
	UINT8 DDSL; 	 /* Disk Directory Start Lump (lump number of GAT) */
	UINT8 GATL; 	 /* # of bytes used in the Granule Allocation Table sector */
	UINT8 STEPRATE;  /* step rate and somet SD/DD flag ... */
	UINT8 TRK;		 /* number of tracks */
	UINT8 SPT;		 /* sectors per track (both heads counted!) */
	UINT8 GATM; 	 /* number of used bits per byte in the GAT sector (GAT mask) */
	UINT8 P7;		 /* ???? always zero */
	UINT8 FLAGS;	 /* ???? some flags (SS/DS bit 6) */
	UINT8 GPL;		 /* Sectors per granule (always 5 for the Colour Genie) */
	UINT8 DDGA; 	 /* Disk Directory Granule allocation (number of driectory granules) */
}	PDRIVE;

static const PDRIVE pd_list[12] = {
	{0x14, 0x28, 0x07, 0x28, 0x0A, 0x02, 0x00, 0x00, 0x05, 0x02}, /* CMD"<0=A" 40 tracks, SS, SD */
	{0x14, 0x28, 0x07, 0x28, 0x14, 0x04, 0x00, 0x40, 0x05, 0x04}, /* CMD"<0=B" 40 tracks, DS, SD */
	{0x18, 0x30, 0x53, 0x28, 0x12, 0x03, 0x00, 0x03, 0x05, 0x03}, /* CMD"<0=C" 40 tracks, SS, DD */
	{0x18, 0x30, 0x53, 0x28, 0x24, 0x06, 0x00, 0x43, 0x05, 0x06}, /* CMD"<0=D" 40 tracks, DS, DD */
	{0x14, 0x28, 0x07, 0x28, 0x0A, 0x02, 0x00, 0x04, 0x05, 0x02}, /* CMD"<0=E" 40 tracks, SS, SD */
	{0x14, 0x28, 0x07, 0x28, 0x14, 0x04, 0x00, 0x44, 0x05, 0x04}, /* CMD"<0=F" 40 tracks, DS, SD */
	{0x18, 0x30, 0x53, 0x28, 0x12, 0x03, 0x00, 0x07, 0x05, 0x03}, /* CMD"<0=G" 40 tracks, SS, DD */
	{0x18, 0x30, 0x53, 0x28, 0x24, 0x06, 0x00, 0x47, 0x05, 0x06}, /* CMD"<0=H" 40 tracks, DS, DD */
	{0x28, 0x50, 0x07, 0x50, 0x0A, 0x02, 0x00, 0x00, 0x05, 0x02}, /* CMD"<0=I" 80 tracks, SS, SD */
	{0x28, 0x50, 0x07, 0x50, 0x14, 0x04, 0x00, 0x40, 0x05, 0x04}, /* CMD"<0=J" 80 tracks, DS, SD */
	{0x30, 0x60, 0x53, 0x50, 0x12, 0x03, 0x00, 0x03, 0x05, 0x03}, /* CMD"<0=K" 80 tracks, SS, DD */
	{0x30, 0x60, 0x53, 0x50, 0x24, 0x06, 0x00, 0x43, 0x05, 0x06}, /* CMD"<0=L" 80 tracks, DS, DD */
};

#define IRQ_TIMER		0x80
#define IRQ_FDC 		0x40
static UINT8 irq_status;

static UINT8 motor_drive;
static UINT8 head;

static UINT8 cass_level;
static UINT8 cass_bit;

static TIMER_CALLBACK( handle_cassette_input )
{
	UINT8 new_level = ( cassette_input( device_list_find_by_tag( machine->config->devicelist, CASSETTE, "cassette" ) ) > 0.0 ) ? 1 : 0;

	if ( new_level != cass_level )
	{
		cass_level = new_level;
		cass_bit ^= 1;
	}
}


static void cgenie_fdc_callback(running_machine *machine, wd17xx_state_t event, void *param);

MACHINE_RESET( cgenie )
{
	const address_space *space = cpu_get_address_space( machine->cpu[0], ADDRESS_SPACE_PROGRAM );
	UINT8 *ROM = memory_region(machine, "main");

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
			memory_install_read8_handler(space, 0xc000, 0xdfff, 0, 0, SMH_BANK10);
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
		memset(&memory_region(machine, "main")[0x0c000], 0x00, 0x2000);
	}

	/* copy EXT ROM, if enabled or wipe out that memory area */
	if( input_port_read(machine, "DSW0") & 0x20 )
	{
		memory_install_read8_handler(space, 0xe000, 0xefff, 0, 0, SMH_ROM);
		memory_install_write8_handler(space, 0xe000, 0xefff, 0, 0, SMH_ROM);
		logerror("cgenie EXT enabled\n");
		memcpy(&memory_region(machine, "main")[0x0e000],
			   &memory_region(machine, "main")[0x12000], 0x1000);
	}
	else
	{
		memory_install_read8_handler(space, 0xe000, 0xefff, 0, 0, SMH_NOP);
		memory_install_write8_handler(space, 0xe000, 0xefff, 0, 0, SMH_NOP);
		logerror("cgenie EXT disabled\n");
		memset(&memory_region(machine, "main")[0x0e000], 0x00, 0x1000);
	}

	cass_level = 0;
	cass_bit = 1;
	timer_pulse(machine,  ATTOTIME_IN_HZ(11025), NULL, 0, handle_cassette_input );
}

MACHINE_START( cgenie )
{
	const address_space *space = cpu_get_address_space( machine->cpu[0], ADDRESS_SPACE_PROGRAM );
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
	memory_install_read8_handler(space, 0x4000, 0x4000 + mess_ram_size - 1, 0, 0, SMH_BANK1);
	memory_install_write8_handler(space, 0x4000, 0x4000 + mess_ram_size - 1, 0, 0, cgenie_videoram_w);
	videoram = mess_ram;
	memory_set_bankptr(machine, 1, mess_ram);

	/* set up FDC */
	wd17xx_init(machine, WD_TYPE_179X, cgenie_fdc_callback, NULL);
}


/* basic-dsk is a disk image format which has the tracks and sectors
 * stored in order, no information is stored which details the number
 * of tracks, number of sides, number of sectors etc, so we need to
 * set that up here
 */
DEVICE_IMAGE_LOAD( cgenie_floppy )
{
	int i, j, dir_offset;
	UINT8 buff[16];
	UINT8 tracks = 0;
	UINT8 heads = 0;
	UINT8 spt = 0;
	short dir_sector = 0;
	short dir_length = 0;

	/* A Floppy Isnt manditory, so return if none */
	if (device_load_basicdsk_floppy(image) != INIT_PASS)
		return INIT_FAIL;

	/* determine image geometry */
	image_fseek(image, 0, SEEK_SET);

	/* determine geometry from disk contents */
	for( i = 0; i < 12; i++ )
	{
		image_fseek(image, pd_list[i].SPT * 256, SEEK_SET);
		image_fread(image, buff, 16);
		/* find an entry with matching DDSL */
		if (buff[0] != 0x00 || buff[1] != 0xfe || buff[2] != pd_list[i].DDSL)
			continue;
		logerror("cgenie: checking format #%d\n", i);

		dir_sector = pd_list[i].DDSL * pd_list[i].GATM * pd_list[i].GPL + pd_list[i].SPT;
		dir_length = pd_list[i].DDGA * pd_list[i].GPL;

		/* scan directory for DIR/SYS or NCW1983/JHL files */
		/* look into sector 2 and 3 first entry relative to DDSL */
		for( j = 16; j < 32; j += 8 )
		{
			dir_offset = dir_sector * 256 + j * 32;
			if( image_fseek(image, dir_offset, SEEK_SET) < 0 )
				break;
			if( image_fread(image, buff, 16) != 16 )
				break;
			if( !strncmp((char*)buff + 5, "DIR     SYS", 11) ||
				!strncmp((char*)buff + 5, "NCW1983 JHL", 11) )
			{
				tracks = pd_list[i].TRK;
				heads = (pd_list[i].SPT > 18) ? 2 : 1;
				spt = pd_list[i].SPT / heads;
				dir_sector = pd_list[i].DDSL * pd_list[i].GATM * pd_list[i].GPL + pd_list[i].SPT;
				dir_length = pd_list[i].DDGA * pd_list[i].GPL;
				memcpy(memory_region(image->machine, "main") + 0x5A71 + image_index_in_device(image) * sizeof(PDRIVE), &pd_list[i], sizeof(PDRIVE));
				break;
			}
		}

		logerror("cgenie: geometry %d tracks, %d heads, %d sec/track\n", tracks, heads, spt);
		/* set geometry so disk image can be read */
		basicdsk_set_geometry(image, tracks, heads, spt, 256, 0, 0, FALSE);

		logerror("cgenie: directory sectors %d - %d (%d sectors)\n", dir_sector, dir_sector + dir_length - 1, dir_length);
		/* mark directory sectors with deleted data address mark */
		/* assumption dir_sector is a sector offset */
		for (j = 0; j < dir_length; j++)
		{
			UINT8 track;
			UINT8 side;
			UINT8 sector_id;
			UINT16 track_offset;
			UINT16 sector_offset;

			/* calc sector offset */
			sector_offset = dir_sector + j;

			/* get track offset */
			track_offset = sector_offset / spt;

			/* calc track */
			track = track_offset / heads;

			/* calc side */
			side = track_offset % heads;

			/* calc sector id - first sector id is 0! */
			sector_id = sector_offset % spt;

			/* set deleted data address mark for sector specified */
			basicdsk_set_ddam(image, track, side, sector_id, 1);
		}

	}
	return INIT_PASS;
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

	cassette_output ( device_list_find_by_tag( space->machine->config->devicelist, CASSETTE, "cassette" ), data & 0x01 ? -1.0 : 1.0 );

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
	/* If the floppy isn't emulated, return 0 */
	if( (input_port_read(space->machine, "DSW0") & 0x80) == 0 )
		return 0;
	return wd17xx_status_r(space, offset);
}

 READ8_HANDLER( cgenie_track_r )
{
	/* If the floppy isn't emulated, return 0xff */
	if( (input_port_read(space->machine, "DSW0") & 0x80) == 0 )
		return 0xff;
	return wd17xx_track_r(space, offset);
}

 READ8_HANDLER( cgenie_sector_r )
{
	/* If the floppy isn't emulated, return 0xff */
	if( (input_port_read(space->machine, "DSW0") & 0x80) == 0 )
		return 0xff;
	return wd17xx_sector_r(space, offset);
}

 READ8_HANDLER(cgenie_data_r )
{
	/* If the floppy isn't emulated, return 0xff */
	if( (input_port_read(space->machine, "DSW0") & 0x80) == 0 )
		return 0xff;
	return wd17xx_data_r(space, offset);
}

WRITE8_HANDLER( cgenie_command_w )
{
	/* If the floppy isn't emulated, return immediately */
	if( (input_port_read(space->machine, "DSW0") & 0x80) == 0 )
		return;
	wd17xx_command_w(space, offset, data);
}

WRITE8_HANDLER( cgenie_track_w )
{
	/* If the floppy isn't emulated, ignore the write */
	if( (input_port_read(space->machine, "DSW0") & 0x80) == 0 )
		return;
	wd17xx_track_w(space, offset, data);
}

WRITE8_HANDLER( cgenie_sector_w )
{
	/* If the floppy isn't emulated, ignore the write */
	if( (input_port_read(space->machine, "DSW0") & 0x80) == 0 )
		return;
	wd17xx_sector_w(space, offset, data);
}

WRITE8_HANDLER( cgenie_data_w )
{
	/* If the floppy isn't emulated, ignore the write */
	if( (input_port_read(space->machine, "DSW0") & 0x80) == 0 )
		return;
	wd17xx_data_w(space, offset, data);
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
		cpu_set_input_line(device->machine->cpu[0], 0, HOLD_LINE);
	}
}

void cgenie_fdc_callback(running_machine *machine, wd17xx_state_t event, void *param)
{
	/* if disc hardware is not enabled, do not cause an int */
	if (!( input_port_read(machine, "DSW0") & 0x80 ))
		return;

	switch( event )
	{
		case WD17XX_IRQ_CLR:
			irq_status &= ~IRQ_FDC;
			break;
		case WD17XX_IRQ_SET:
			if( (irq_status & IRQ_FDC) == 0 )
			{
				irq_status |= IRQ_FDC;
				cpu_set_input_line(machine->cpu[0], 0, HOLD_LINE);
			}
			break;
		case WD17XX_DRQ_CLR:
		case WD17XX_DRQ_SET:
			/* do nothing */
			break;
	}
}

WRITE8_HANDLER( cgenie_motor_w )
{
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

	wd17xx_set_drive(drive);
	wd17xx_set_side(head);
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
		cgenie_port_ff_w(cpu_get_address_space(device->machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0, port_ff ^ FF_BGD0);
	}
}
