/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

/* Core includes */
#include "driver.h"
#include "includes/trs80.h"

/* Components */
#include "cpu/z80/z80.h"
#include "machine/wd17xx.h"
#include "sound/speaker.h"

/* Devices */
#include "devices/basicdsk.h"
#include "devices/cassette.h"
#include "devices/flopdrv.h"


#ifdef MAME_DEBUG
#define VERBOSE 1
#else
#define VERBOSE 0
#endif

#define LOG(x)	do { if (VERBOSE) logerror x; } while (0)


UINT8 trs80_port_ff = 0;

#define IRQ_TIMER		0x80
#define IRQ_FDC 		0x40
static	UINT8			irq_status = 0;

#define MAX_LUMPS		192 	/* crude storage units - don't now much about it */
#define MAX_GRANULES	8		/* lumps consisted of granules.. aha */
#define MAX_SECTORS 	5		/* and granules of sectors */

#ifdef USE_TRACK
static UINT8 track[4] = {0, };				/* current track per drive */
#endif
static UINT8 head;							 /* current head per drive */
#ifdef USE_SECTOR
static UINT8 sector[4] = {0, }; 			/* current sector per drive */
#endif
static UINT8 irq_mask = 0;

#define FW TRS80_FONT_W
#define FH TRS80_FONT_H

static double old_cassette_val;
static UINT8 cassette_data;
static emu_timer *cassette_data_timer;

static TIMER_CALLBACK( cassette_data_callback )
{
	double new_val = cassette_input(device_list_find_by_tag( machine->config->devicelist, CASSETTE, "cassette" ));

	/* Check for HI-LO transition */
	if ( old_cassette_val > -0.2 && new_val < -0.2 )
	{
		cassette_data |= 0x80;
	}
	old_cassette_val = new_val;
}


QUICKLOAD_LOAD( trs80_cmd )
{
	const address_space *space = cpu_get_address_space(image->machine->cpu[0], ADDRESS_SPACE_PROGRAM);
	UINT16 entry = 0, block_ofs = 0, block_len = 0;
	unsigned offs = 0;
	UINT8 *cmd_buff;

	cmd_buff = malloc(quickload_size);
	if (!cmd_buff)
		return INIT_FAIL;

	while( quickload_size > 3 )
	{
		UINT8 data = cmd_buff[offs++];

		switch( data ) {
		case 0x01:		   /* CMD file header */
		case 0x07:		   /* another type of CMD file header */
			block_len = cmd_buff[offs++];
			/* on CMD files size=zero means size 256 */
			if( block_len == 0 )
				block_len += 256;
			block_ofs = cmd_buff[offs++];
			block_ofs += 256 * cmd_buff[offs++];
			block_len -= 2;
			if( block_len == 0 )
				block_len = 256;
			quickload_size -= 4;
			LOG(("trs80_cmd_load block ($%02X) %d at $%04X\n", data, block_len, block_ofs));
			while( block_len && quickload_size )
			{
				memory_write_byte(space, block_ofs, cmd_buff[offs]);
				offs++;
				block_ofs++;
				block_len--;
				quickload_size--;
			}
			break;
		case 0x02:
			block_len = cmd_buff[offs++];
			quickload_size -= 1;
			if (entry == 0)
			{
				entry = cmd_buff[offs++];
				entry += 256 * cmd_buff[offs++];
				LOG(("trs80_cmd_load entry ($%02X) at $%04X\n", data, entry));
			}
			else
			{
				UINT16 temp;
				temp = cmd_buff[offs++];
				temp += 256 * cmd_buff[offs++];
				LOG(("trs80_cmd_load 2nd entry ($%02X) at $%04X ignored\n", data, temp));
			}
			quickload_size -= 3;
			break;
		default:
			quickload_size--;
		}
	}
	cpu_set_reg(image->machine->cpu[0], Z80_PC, entry);

	free(cmd_buff);
	return INIT_PASS;
}

DEVICE_IMAGE_LOAD( trs80_floppy )
{
	static UINT8 pdrive[4*16];
	int i;
	int tracks; 	/* total tracks count per drive */
	int heads;		/* total heads count per drive */
	int spt;		/* sector per track count per drive */
	int dir_sector; /* first directory sector (aka DDSL) */
	int dir_length; /* length of directory in sectors (aka DDGA) */
	int id = image_index_in_device(image);

    if (device_load_basicdsk_floppy(image) != INIT_PASS)
		return INIT_FAIL;

    if (image_index_in_device(image) == 0)        /* first floppy? */
	{
		image_fseek(image, 0, SEEK_SET);
		image_fread(image, pdrive, 2);
#if 0
		if (pdrive[0] != 0x00 || pdrive[1] != 0xfe)
		{
			basicdsk_read_sectormap(image, &tracks[id], &heads[id], &spt[id]);
		}
		else
#endif

		image_fseek(image, 2 * 256, SEEK_SET);
		image_fread(image, pdrive, 4*16);
	}

	tracks = pdrive[id*16+3] + 1;
	heads = (pdrive[id*16+7] & 0x40) ? 2 : 1;
	spt = pdrive[id*16+4] / heads;
	dir_sector = 5 * pdrive[id*16+0] * pdrive[id*16+5];
	dir_length = 5 * pdrive[id*16+9];

    /* set geometry so disk image can be read */
	basicdsk_set_geometry(image, tracks, heads, spt, 256, 0, 0, FALSE);

	/* mark directory sectors with deleted data address mark */
	/* assumption dir_sector is a sector offset */
	for (i = 0; i < dir_length; i++)
	{
		int track, side, sector_id;
		int track_offset, sector_offset;

		/* calc sector offset */
		sector_offset = dir_sector + i;

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
    return INIT_PASS;
}

MACHINE_RESET( trs80 )
{
	cassette_data = 0x00;
	cassette_data_timer = timer_alloc(machine,  cassette_data_callback, NULL );
	timer_adjust_periodic( cassette_data_timer, attotime_zero, 0, ATTOTIME_IN_HZ(11025) );
}


DRIVER_INIT( trs80 )
{
	UINT8 *FNT = memory_region(machine, "gfx1");
	int i, y;

	for( i = 0x000; i < 0x080; i++ )
	{
		/* copy eight lines from the character generator */
		for (y = 0; y < 8; y++)
			FNT[i*FH+y] = FNT[0x0800+i*8+y] << 3;
		/* wipe out the lower lines (no descenders!) */
		for (y = 8; y < FH; y++)
			FNT[i*FH+y] = 0;
	}
	/* setup the 2x3 chunky block graphics (two times 64 characters) */
	for( i = 0x080; i < 0x100; i++ )
	{
		UINT8 b0, b1, b2, b3, b4, b5;
		b0 = (i & 0x01) ? 0xe0 : 0x00;
		b1 = (i & 0x02) ? 0x1c : 0x00;
		b2 = (i & 0x04) ? 0xe0 : 0x00;
		b3 = (i & 0x08) ? 0x1c : 0x00;
		b4 = (i & 0x10) ? 0xe0 : 0x00;
		b5 = (i & 0x20) ? 0x1c : 0x00;

		FNT[i*FH+ 0] = FNT[i*FH+ 1] = FNT[i*FH+ 2] = FNT[i*FH+ 3] = b0 | b1;
		FNT[i*FH+ 4] = FNT[i*FH+ 5] = FNT[i*FH+ 6] = FNT[i*FH+ 7] = b2 | b3;
		FNT[i*FH+ 8] = FNT[i*FH+ 9] = FNT[i*FH+10] = FNT[i*FH+11] = b4 | b5;
	}
}

DRIVER_INIT( lnw80 )
{
	UINT8 y, *FNT = memory_region(machine, "gfx1");
	UINT16 i, rows[] = { 0, 0x200, 0x100, 0x300, 1, 0x201, 0x101, 0x301 };

	for( i = 0; i < 0x80; i++ )
	{
		/* copy eight lines from the character generator */
		for (y = 0; y < 8; y++)
			FNT[i*FH+y] = BITSWAP8(FNT[0x800+(i<<1)+rows[y]], 2, 1, 6, 7, 5, 3, 4, 0); /* bits 0,3,4 are blank */
	}
	/* setup the 2x3 chunky block graphics (two times 64 characters) */
	for( i = 0x80; i < 0x100; i++ )
	{
		UINT8 b0, b1, b2, b3, b4, b5;
		b0 = (i & 0x01) ? 0xe0 : 0x00;
		b1 = (i & 0x02) ? 0x1c : 0x00;
		b2 = (i & 0x04) ? 0xe0 : 0x00;
		b3 = (i & 0x08) ? 0x1c : 0x00;
		b4 = (i & 0x10) ? 0xe0 : 0x00;
		b5 = (i & 0x20) ? 0x1c : 0x00;

		FNT[i*FH+ 0] = FNT[i*FH+ 1] = FNT[i*FH+ 2] = FNT[i*FH+ 3] = b0 | b1;
		FNT[i*FH+ 4] = FNT[i*FH+ 5] = FNT[i*FH+ 6] = FNT[i*FH+ 7] = b2 | b3;
		FNT[i*FH+ 8] = FNT[i*FH+ 9] = FNT[i*FH+10] = FNT[i*FH+11] = b4 | b5;
	}
}

DRIVER_INIT( ht1080z )
{
}


DRIVER_INIT( ht108064 )
{
	UINT8 *FNT = memory_region(machine, "gfx1");
	int i;
	for( i=0;i<0x800;i++) {
		FNT[i] &= 0xf8;
	}
}

/*************************************
 *
 *				Port handlers.
 *
 *************************************/


 READ8_HANDLER( trs80_port_xx_r )
{
	return 0;
}


WRITE8_HANDLER( trs80_port_ff_w )
{
	static const double levels[4] = { 0.0, -1.0, 0.0, 1.0 };
	const device_config *cass = device_list_find_by_tag(space->machine->config->devicelist, CASSETTE, "cassette" );

	cassette_change_state( cass, ( data & 0x04 ) ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR );

	cassette_output( cass, levels[data & 0x03]);

	cassette_data &= ~0x80;
	trs80_port_ff = data;
}


 READ8_HANDLER( trs80_port_ff_r )
{
	return cassette_data;// | 0x7F;
}

/*************************************
 *
 *		Interrupt handlers.
 *
 *************************************/

INTERRUPT_GEN( trs80_timer_interrupt )
{
	if( (irq_status & IRQ_TIMER) == 0 )
	{
		irq_status |= IRQ_TIMER;
		cpu_set_input_line(device, 0, HOLD_LINE);
	}
}

static void trs80_fdc_interrupt_internal(running_machine *machine)
{
	if ((irq_status & IRQ_FDC) == 0)
	{
		irq_status |= IRQ_FDC;
		cpu_set_input_line(machine->cpu[0], 0, HOLD_LINE);
	}
}

INTERRUPT_GEN( trs80_fdc_interrupt )
{
	trs80_fdc_interrupt_internal(device->machine);
}

static WD17XX_CALLBACK( trs80_fdc_callback )
{
	switch (state)
	{
		case WD17XX_IRQ_CLR:
			irq_status &= ~IRQ_FDC;
			break;
		case WD17XX_IRQ_SET:
			trs80_fdc_interrupt_internal(device->machine);
			break;
		case WD17XX_DRQ_CLR:
		case WD17XX_DRQ_SET:
			/* do nothing */
			break;
	}
}

const wd17xx_interface trs80_wd17xx_interface = { trs80_fdc_callback, NULL };


INTERRUPT_GEN( trs80_frame_interrupt )
{
}

/*************************************
 *				     *
 *		Memory handlers      *
 *				     *
 *************************************/

 READ8_HANDLER ( trs80_printer_r )
{
	/* nothing yet :( */
	return 0;
}

WRITE8_HANDLER( trs80_printer_w )
{
	/* nothing yet :( */
}

 READ8_HANDLER( trs80_irq_status_r )
{
	int result = irq_status;
	irq_status &= ~(IRQ_TIMER | IRQ_FDC);
	return result;
}

WRITE8_HANDLER( trs80_irq_mask_w )
{
	irq_mask = data;
}

WRITE8_HANDLER( trs80_motor_w )
{
	UINT8 drive = 255;
	device_config *fdc = (device_config*)device_list_find_by_tag( space->machine->config->devicelist, WD179X, "wd179x");

	LOG(("trs80 motor_w $%02X\n", data));

	switch (data)
	{
	case 1:
		drive = 0;
		head = 0;
		break;
	case 2:
		drive = 1;
		head = 0;
		break;
	case 4:
		drive = 2;
		head = 0;
		break;
	case 8:
		drive = 3;
		head = 0;
		break;
	case 9:
		drive = 0;
		head = 1;
		break;
	case 10:
		drive = 1;
		head = 1;
		break;
	case 12:
		drive = 2;
		head = 1;
		break;
	}

	if (drive > 3)
		return;

    wd17xx_set_drive(fdc,drive);
	wd17xx_set_side(fdc,head);

}

/*************************************
 *		Keyboard					 *
 *************************************/
READ8_HANDLER( trs80_keyboard_r )
{
	int result = 0;

	if (offset & 1)
		result |= input_port_read(space->machine, "LINE0");
	if (offset & 2)
		result |= input_port_read(space->machine, "LINE1");
	if (offset & 4)
		result |= input_port_read(space->machine, "LINE2");
	if (offset & 8)
		result |= input_port_read(space->machine, "LINE3");
	if (offset & 16)
		result |= input_port_read(space->machine, "LINE4");
	if (offset & 32)
		result |= input_port_read(space->machine, "LINE5");
	if (offset & 64)
		result |= input_port_read(space->machine, "LINE6");
	if (offset & 128)
		result |= input_port_read(space->machine, "LINE7");

	return result;
}


