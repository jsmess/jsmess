#include "driver.h"
#include "includes/mikromik.h"
#include "cpu/i8085/i8085.h"
#include "devices/basicdsk.h"
#include "video/i8275.h"

/*

	This system is pretty useless without the boot disks, and the chargen ROM.

	Video chips
	-----------
	- Intel 8275H
	- NEC uPD7220

*/

/* Video */

static I8275_DMA_REQUEST( mm1_i8275_dma_request )
{
	logerror("i8275 DMA\n");
}

static I8275_INT_REQUEST( mm1_i8275_int_request )
{
	logerror("i8275 INT\n");
}

static I8275_DISPLAY_PIXELS( mm1_i8275_display_pixels )
{
/*	int i;
	bitmap_t *bitmap = tmpbitmap;
	UINT8 *charmap = memory_region(device->machine, "gfx1") + (mikrosha_font_page & 1) * 0x400;
	UINT8 pixels = charmap[(linecount & 7) + (charcode << 3)] ^ 0xff;
	if (vsp) {
		pixels = 0;
	}
	if (lten) {
		pixels = 0xff;
	}
	if (rvv) {
		pixels ^= 0xff;
	}
	for(i=0;i<6;i++) {
		*BITMAP_ADDR16(bitmap, y, x + i) = (pixels >> (5-i)) & 1 ? (hlgt ? 2 : 1) : 0;
	}*/
}

static const i8275_interface mm1_i8275_intf = 
{
	SCREEN_TAG,
	8,
	0,
	mm1_i8275_dma_request,
	mm1_i8275_int_request,
	mm1_i8275_display_pixels
};

static VIDEO_UPDATE( mm1 )
{
	mm1_state *state = screen->machine->driver_data;

	i8275_update(state->i8275, bitmap, cliprect);

	return 0;
}

/* Memory Maps */

static ADDRESS_MAP_START( mm1_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_ROM // is this banked out somehow?
	AM_RANGE(0x1000, 0xffff) AM_RAM
//	AM_RANGE(0x4000, 0x4fff) AM_RAM // videoram?
ADDRESS_MAP_END

static ADDRESS_MAP_START( mm1_io_map, ADDRESS_SPACE_IO, 8 )
//    AM_RANGE(0xd000, 0xd001) AM_DEVREADWRITE(I8275_TAG, i8275_r, i8275_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( mm1 )
INPUT_PORTS_END

/* Machine Initialization */

static MACHINE_START( mm1 )
{
	mm1_state *state = machine->driver_data;

	/* look up devices */

	state->i8275 = devtag_get_device(machine, I8275_TAG);

	/* register for state saving */

	//state_save_register_global(machine, state->);
}

/* Machine Drivers */

static MACHINE_DRIVER_START( mm1 )
	MDRV_DRIVER_DATA(mm1_state)

	/* basic system hardware */
	MDRV_CPU_ADD(I8085_TAG, 8085A, 2000000)
	MDRV_CPU_PROGRAM_MAP(mm1_map, 0)
	MDRV_CPU_IO_MAP(mm1_io_map, 0)

	MDRV_MACHINE_START(mm1)

	/* video hardware */
	MDRV_SCREEN_ADD( SCREEN_TAG, RASTER )
	MDRV_SCREEN_REFRESH_RATE( 50 )
	MDRV_SCREEN_FORMAT( BITMAP_FORMAT_INDEXED16 )
	MDRV_SCREEN_SIZE( 800, 327 )
	MDRV_SCREEN_VISIBLE_AREA( 0, 800-1, 0, 327-1 )

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_UPDATE(mm1)

	MDRV_I8275_ADD(I8275_TAG, mm1_i8275_intf)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( mm1m6 )
	ROM_REGION( 0x1000, I8085_TAG, 0 )
	ROM_LOAD( "mm1.bin", 0x0000, 0x1000, CRC(07400e72) SHA1(354ff97817a607ca38d296af8b2813878d092a08) )

	ROM_REGION( 0x1000, "chargen", 0 )
	ROM_LOAD( "chargen", 0x0000, 0x1000, NO_DUMP )
ROM_END

#define rom_mm1m7 rom_mm1m6

/* System Configuration */

static DEVICE_IMAGE_LOAD( mm1_floppy )
{
	if (image_has_been_created(image))
		return INIT_FAIL;

	if (device_load_basicdsk_floppy(image) == INIT_PASS)
	{
		int size = image_length(image);

		if (size == 80*2*16*256)  // 640KB
		{
			/* image, tracks, heads, sectors per track, sector length, first sector id, offset track zero, track skipping */
			basicdsk_set_geometry(image, 80, 2, 16, 256, 1, 0, FALSE);

			return INIT_PASS;
		}
	}

	return INIT_FAIL;
}

#ifdef UNUSED_CODE
static DEVICE_IMAGE_LOAD( mm2_floppy )
{
	if (image_has_been_created(image))
		return INIT_FAIL;

	if (device_load_basicdsk_floppy(image) == INIT_PASS)
	{
		int size = image_length(image);

		switch (size)
		{
		case 40*2*9*512: // 360KB
			/* image, tracks, heads, sectors per track, sector length, first sector id, offset track zero, track skipping */
			basicdsk_set_geometry(image, 40, 2, 9, 512, 1, 0, FALSE);
			break;

		case 40*2*18*512: // 720KB (number of sectors has been doubled, instead of tracks as in 720KB DOS floppies)
			/* image, tracks, heads, sectors per track, sector length, first sector id, offset track zero, track skipping */
			basicdsk_set_geometry(image, 40, 2, 18, 512, 1, 0, FALSE);
			break;

		default:
			return INIT_FAIL;
		}

		return INIT_PASS;
	}

	return INIT_FAIL;
}
#endif

static void dual_640kb_floppy(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:					info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:						info->load = DEVICE_IMAGE_LOAD_NAME(mm1_floppy); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:			strcpy(info->s = device_temp_str(), "img"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}

static SYSTEM_CONFIG_START( mm1m6 )
	CONFIG_RAM_DEFAULT	(64 * 1024)
	CONFIG_DEVICE(dual_640kb_floppy)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START( mm1m7 )
	CONFIG_RAM_DEFAULT	(64 * 1024)
	CONFIG_DEVICE(dual_640kb_floppy)
	// 5MB Winchester
SYSTEM_CONFIG_END

/* System Drivers */

//    YEAR  NAME		PARENT  COMPAT	MACHINE		INPUT		INIT	CONFIG    COMPANY			FULLNAME				FLAGS
COMP( 1981, mm1m6,		0,		0,		mm1,		mm1,		0, 		mm1m6,	  "Nokia Data",		"MikroMikko 1 M6",		GAME_NOT_WORKING )
COMP( 1981, mm1m7,		mm1m6,	0,		mm1,		mm1,		0, 		mm1m7,	  "Nokia Data",		"MikroMikko 1 M7",		GAME_NOT_WORKING )
