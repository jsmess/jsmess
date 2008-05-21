/***************************************************************************

  Enhanced Graphics Adapter (EGA) section

TODO - Write documentation
     - Get proper clock setting

***************************************************************************/

#include "driver.h"
#include "video/pc_ega.h"
#include "video/mc6845.h"
#include "video/pc_video.h"
#include "memconv.h"


#define VERBOSE_EGA		1


static struct
{
	const device_config	*mc6845;
	mc6845_update_row_func	update_row;

	/* Attribute registers AR00 - AR14 
	*/
	struct {
		UINT8	index;
		UINT8	data[32];
	} attribute;

	/* Sequencer registers SR00 - SR04

	  SR00 - 7 6 5 4 3 2 1 0 - Reset Control Register
	         | | | | | | | |
	         | | | | | | | +-- Must be 1 for normal operation
	         | | | | | | +---- Must be 1 for normal operation
	         | | | | | +------ reserved/unused
	         | | | | +-------- reserved/unused
	         | | | +---------- reserved/unused
	         | | +------------ reserved/unused
	         | +-------------- reserved/unused
	         +---------------- reserved/unused

	  SR01 - 7 6 5 4 3 2 1 0 - Clocking Mode
	         | | | | | | | |
	         | | | | | | | +-- 0 = 9 dots per char, 1 = 8 dots per char
	         | | | | | | +---- clock frequency, 0 = 4 out of 5 memory cycles, 1 = 2 out of 5 memory cycles
	         | | | | | +------ shift load
	         | | | | +-------- 0 = normal dot clock, 1 = master dot clock / 2
	         | | | +---------- reserved/unused
	         | | +------------ reserved/unused
	         | +-------------- reserved/unused
	         +---------------- reserved/unused

	  SR02 - 7 6 5 4 3 2 1 0 - Map Mask
	         | | | | | | | |
	         | | | | | | | +-- 1 = enable map 0
	         | | | | | | +---- 1 = enable map 1
	         | | | | | +------ 1 = enable map 2
	         | | | | +-------- 1 = enable map 3
	         | | | +---------- reserved/unused
	         | | +------------ reserved/unused
	         | +-------------- reserved/unused
	         +---------------- reserved/unused

	  SR03 - 7 6 5 4 3 2 1 0 - Character Map Select
	         | | | | | | | |
	         | | | | | | | +-- select plane for character map B
	         | | | | | | +---- select plane for character map B
	         | | | | | +------ select plane for character map A
	         | | | | +-------- select plane for character map A
	         | | | +---------- reserved/unused
	         | | +------------ reserved/unused
	         | +-------------- reserved/unused
	         +---------------- reserved/unused
	       Meaning of the plane selection bits:
	       00 - 1st 8K plane 2 bank 0
	       01 - 1st 8K plane 2 bank 1
	       10 - 1st 8K plane 2 bank 2
	       11 - 1st 8K plane 2 bank 3

	  SR04 - 7 6 5 4 3 2 1 0 - Memory Mode Register
	         | | | | | | | |
	         | | | | | | | +-- 0 = graphics mode, 1 = text mode
	         | | | | | | +---- 0 = no memory extension, 1 = memory extension
	         | | | | | +------ 0 = odd/even storage, 1 = sequential storage
	         | | | | +-------- reserved/unused
	         | | | +---------- reserved/unused
	         | | +------------ reserved/unused
	         | +-------------- reserved/unused
	         +---------------- reserved/unused

	*/
	struct {
		UINT8	index;
		UINT8	data[8];
	} sequencer;

	/* Graphics controller registers GR00 - GR08
	*/
	struct {
		UINT8	index;
		UINT8	data[16];
	} graphics_controller;

	UINT8	frame_cnt;
	UINT8	vsync;
	UINT8	hsync;
} ega;


static VIDEO_START( pc_ega );
static VIDEO_UPDATE( mc6845_ega );
static PALETTE_INIT( pc_ega );
static MC6845_UPDATE_ROW( ega_update_row );
static MC6845_ON_HSYNC_CHANGED( ega_hsync_changed );
static MC6845_ON_VSYNC_CHANGED( ega_vsync_changed );
static READ8_HANDLER( pc_ega8_3d0_r );
static WRITE8_HANDLER( pc_ega8_3d0_w );
static READ16_HANDLER( pc_ega16le_3d0_r );
static WRITE16_HANDLER( pc_ega16le_3d0_w );
static READ32_HANDLER( pc_ega32le_3d0_r );
static WRITE32_HANDLER( pc_ega32le_3d0_w );
static READ8_HANDLER( pc_ega8_3c0_r );
static WRITE8_HANDLER( pc_ega8_3c0_w );
static READ16_HANDLER( pc_ega16le_3c0_r );
static WRITE16_HANDLER( pc_ega16le_3c0_w );
static READ32_HANDLER( pc_ega32le_3c0_r );
static WRITE32_HANDLER( pc_ega32le_3c0_w );


static const mc6845_interface mc6845_ega_intf =
{
	EGA_SCREEN_NAME,	/* screen number */
	16257000/8,			/* clock */
	8,					/* numbers of pixels per video memory address */
	NULL,				/* begin_update */
	ega_update_row,		/* update_row */
	NULL,				/* end_update */
	NULL,				/* on_de_chaged */
	ega_hsync_changed,	/* on_hsync_changed */
	ega_vsync_changed	/* on_vsync_changed */
};


MACHINE_DRIVER_START( pcvideo_ega )
	MDRV_SCREEN_ADD(EGA_SCREEN_NAME, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_RAW_PARAMS(XTAL_14_31818MHz,912,0,640,262,0,200)
	MDRV_PALETTE_LENGTH( 64 )

	MDRV_PALETTE_INIT(pc_ega)

	MDRV_DEVICE_ADD(EGA_MC6845_NAME, MC6845)
	MDRV_DEVICE_CONFIG( mc6845_ega_intf )

	MDRV_VIDEO_START( pc_ega )
	MDRV_VIDEO_UPDATE( mc6845_ega )
MACHINE_DRIVER_END


static PALETTE_INIT( pc_ega )
{
	int i;

	for ( i = 0; i < 64; i++ )
	{
		UINT8 r = ( ( i & 0x04 ) ? 0xAA : 0x00 ) + ( ( i & 0x20 ) ? 0x55 : 0x00 );
		UINT8 g = ( ( i & 0x02 ) ? 0xAA : 0x00 ) + ( ( i & 0x10 ) ? 0x55 : 0x00 );
		UINT8 b = ( ( i & 0x01 ) ? 0xAA : 0x00 ) + ( ( i & 0x08 ) ? 0x55 : 0x00 );

		palette_set_color_rgb( machine, i, r, g, b );
	}
}


static VIDEO_START( pc_ega )
{
	int buswidth;

	buswidth = cputype_databus_width(machine->config->cpu[0].type, ADDRESS_SPACE_PROGRAM);
	switch(buswidth)
	{
		case 8:
			memory_install_read8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xb8000, 0xbffff, 0, 0, SMH_BANK11 );
			memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xb8000, 0xbffff, 0, 0, pc_video_videoram_w );
			memory_install_read8_handler(machine, 0, ADDRESS_SPACE_IO, 0x3d0, 0x3db, 0, 0, pc_ega8_3d0_r );
			memory_install_write8_handler(machine, 0, ADDRESS_SPACE_IO, 0x3d0, 0x3db, 0, 0, pc_ega8_3d0_w );
			memory_install_read8_handler(machine, 0, ADDRESS_SPACE_IO, 0x3c0, 0x3cf, 0, 0, pc_ega8_3c0_r );
			memory_install_write8_handler(machine, 0, ADDRESS_SPACE_IO, 0x3c0, 0x3cf, 0, 0, pc_ega8_3c0_w );
			break;

		case 16:
			memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xb8000, 0xbffff, 0, 0, SMH_BANK11 );
			memory_install_write16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xb8000, 0xbffff, 0, 0, pc_video_videoram16le_w );
			memory_install_read16_handler(machine, 0, ADDRESS_SPACE_IO, 0x3d0, 0x3db, 0, 0, pc_ega16le_3d0_r );
			memory_install_write16_handler(machine, 0, ADDRESS_SPACE_IO, 0x3d0, 0x3db, 0, 0, pc_ega16le_3d0_w );
			memory_install_read16_handler(machine, 0, ADDRESS_SPACE_IO, 0x3c0, 0x3cf, 0, 0, pc_ega16le_3c0_r );
			memory_install_write16_handler(machine, 0, ADDRESS_SPACE_IO, 0x3c0, 0x3cf, 0, 0, pc_ega16le_3c0_w );
			break;

		case 32:
			memory_install_read32_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xb8000, 0xbffff, 0, 0, SMH_BANK11 );
			memory_install_write32_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xb8000, 0xbffff, 0, 0, pc_video_videoram32_w );
			memory_install_read32_handler(machine, 0, ADDRESS_SPACE_IO, 0x3d0, 0x3db, 0, 0, pc_ega32le_3d0_r );
			memory_install_write32_handler(machine, 0, ADDRESS_SPACE_IO, 0x3d0, 0x3db, 0, 0, pc_ega32le_3d0_w );
			memory_install_read32_handler(machine, 0, ADDRESS_SPACE_IO, 0x3c0, 0x3cf, 0, 0, pc_ega32le_3c0_r );
			memory_install_write32_handler(machine, 0, ADDRESS_SPACE_IO, 0x3c0, 0x3cf, 0, 0, pc_ega32le_3c0_w );
			break;

		default:
			fatalerror("EGA:  Bus width %d not supported\n", buswidth);
			break;
	}

	/* 256KB Video ram max on an EGA card */
	videoram_size = 0x40000;

	videoram = auto_malloc(videoram_size);

	memory_set_bankptr(11, videoram);

	ega.mc6845 = device_list_find_by_tag(machine->config->devicelist, MC6845, EGA_MC6845_NAME);
	ega.update_row = NULL;

	/* Set up default palette */
	ega.attribute.data[0] = 0;
	ega.attribute.data[1] = 1;
	ega.attribute.data[2] = 2;
	ega.attribute.data[3] = 3;
	ega.attribute.data[4] = 4;
	ega.attribute.data[5] = 5;
	ega.attribute.data[6] = 0x14;
	ega.attribute.data[7] = 7;
	ega.attribute.data[8] = 0x38;
	ega.attribute.data[9] = 0x39;
	ega.attribute.data[10] = 0x3A;
	ega.attribute.data[11] = 0x3B;
	ega.attribute.data[12] = 0x3C;
	ega.attribute.data[13] = 0x3D;
	ega.attribute.data[14] = 0x3E;
	ega.attribute.data[15] = 0x3F;
}


static VIDEO_UPDATE( mc6845_ega )
{
	mc6845_update( ega.mc6845, bitmap, cliprect);
	return 0;
}


static MC6845_UPDATE_ROW( ega_update_row )
{
	if ( ega.update_row )
	{
		ega.update_row( device, bitmap, cliprect, ma, ra, y, x_count, cursor_x, param );
	}
}


static MC6845_ON_HSYNC_CHANGED( ega_hsync_changed )
{
	ega.hsync = hsync ? 1 : 0;
}


static MC6845_ON_VSYNC_CHANGED( ega_vsync_changed )
{
	ega.vsync = vsync ? 8 : 0;
	if ( vsync )
	{
		ega.frame_cnt++;
	}
}


static READ8_HANDLER( pc_ega8_3d0_r )
{
	int data = 0xff;

	if ( VERBOSE_EGA )
	{
		logerror("pc_ega_3b0_r: offset = %02x\n", offset );
	}

	switch ( offset )
	{
	/* CRT Controller - address register */
	case 0: case 2: case 4: case 6:
		/* return last written mc6845 address value here? */
		break;

	/* CRT Controller - data register */
	case 1: case 3: case 5: case 7:
		data = mc6845_register_r( ega.mc6845, offset );
		break;

	/* Display status */
	case 10:
		data = ega.vsync | ega.hsync;
		break;
	}

	return data;
}


static WRITE8_HANDLER( pc_ega8_3d0_w )
{
	if ( VERBOSE_EGA )
	{
		logerror("pc_ega_3b0_w: offset = %02x, data = %02x\n", offset, data );
	}

	switch ( offset )
	{
	/* CRT Controller - address register */
	case 0: case 2: case 4: case 6:
		mc6845_address_w( ega.mc6845, offset, data );
		break;

	/* CRT Controller - data register */
	case 1: case 3: case 5: case 7:
		mc6845_register_w( ega.mc6845, offset, data );
		break;

	/* Set Light Pen Flip Flop */
	case 9:
		break;

	/* Feature Control */
	case 10:
		break;

	/* Clear Light Pen Flip Flop */
	case 11:
		break;
	}
}


static READ8_HANDLER( pc_ega8_3c0_r )
{
	int data = 0xff;

	if ( VERBOSE_EGA )
	{
		logerror("pc_ega_3c0_r: offset = %02x\n", offset );
	}

	switch ( offset )
	{
	/* Attributes Controller */
	case 0:
		data = ega.attribute.index;
		break;
	case 1:
		if ( ( ega.attribute.index & 0x30 ) != 0x20 )
		{
			data = ega.attribute.data[ ega.attribute.index & 0x1F ];
		}
		break;

	/* Feature Read */
	case 2:
		break;

	/* Sequencer */
	case 4:
		data = ega.sequencer.index;
		break;
	case 5:
		data = ega.sequencer.data[ ega.sequencer.index & 0x07 ];
		break;

	/* Graphics Controller */
	case 14:
		data = ega.graphics_controller.index;
		break;
	case 15:
		data = ega.graphics_controller.data[ ega.graphics_controller.index & 0x0F];
		break;
	}
	return data;
}


static WRITE8_HANDLER( pc_ega8_3c0_w )
{
	if ( VERBOSE_EGA )
	{
		logerror("pc_ega_3c0_w: offset = %02x, data = %02x\n", offset, data );
	}

	switch ( offset )
	{
	/* Attributes Controller */
	case 0:
		ega.attribute.index = data;
		break;
	case 1:
		if ( ( ega.attribute.index & 0x30 ) != 0x20 )
		{
//logerror("AR%02x = 0x%02x\n", ega.attribute.index & 0x1F, data );
			ega.attribute.data[ ega.attribute.index & 0x1F ] = data;
		}
		break;

	/* Misccellaneous Output */
	case 2:
		break;

	/* Sequencer */
	case 4:
		ega.sequencer.index = data;
		break;
	case 5:
//logerror("SR%02x = 0x%02x\n", ega.graphics_controller.index & 0x07, data );
		ega.sequencer.data[ ega.sequencer.index & 0x07 ] = data;
		break;

	/* Graphics Controller */
	case 14:
		ega.graphics_controller.index = data;
		break;
	case 15:
//logoerror("GR%02x = 0x%02x\n", ega.graphics_controller.index & 0x0F, data );
		ega.graphics_controller.data[ ega.graphics_controller.index & 0x0F ] = data;
		break;
	}
}

static READ16_HANDLER( pc_ega16le_3d0_r ) { return read16le_with_read8_handler(pc_ega8_3d0_r,machine,  offset, mem_mask); }
static WRITE16_HANDLER( pc_ega16le_3d0_w ) { write16le_with_write8_handler(pc_ega8_3d0_w, machine, offset, data, mem_mask); }
static READ32_HANDLER( pc_ega32le_3d0_r ) { return read32le_with_read8_handler(pc_ega8_3d0_r, machine, offset, mem_mask); }
static WRITE32_HANDLER( pc_ega32le_3d0_w ) { write32le_with_write8_handler(pc_ega8_3d0_w, machine, offset, data, mem_mask); }

static READ16_HANDLER( pc_ega16le_3c0_r ) { return read16le_with_read8_handler(pc_ega8_3c0_r,machine,  offset, mem_mask); }
static WRITE16_HANDLER( pc_ega16le_3c0_w ) { write16le_with_write8_handler(pc_ega8_3c0_w, machine, offset, data, mem_mask); }
static READ32_HANDLER( pc_ega32le_3c0_r ) { return read32le_with_read8_handler(pc_ega8_3c0_r, machine, offset, mem_mask); }
static WRITE32_HANDLER( pc_ega32le_3c0_w ) { write32le_with_write8_handler(pc_ega8_3c0_w, machine, offset, data, mem_mask); }

