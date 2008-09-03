#include <stdio.h>
#include <stdarg.h>

#include "driver.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"

#include "includes/cbm.h"
#include "formats/cbm_tap.h"



/***********************************************

	CBM Quickloads

***********************************************/


static int general_cbm_loadsnap(const device_config *image, const char *file_type, int snapshot_size,
	offs_t offset, void (*cbm_sethiaddress)(UINT16 hiaddress))
{
	char buffer[7];
	UINT8 *data = NULL;
	UINT32 bytesread;
	UINT16 address = 0;
	int i;

	if (!file_type)
		goto error;

	if (!mame_stricmp(file_type, "prg"))
	{
		/* prg files */
	}
	else if (!mame_stricmp(file_type, "p00"))
	{
		/* p00 files */
		if (image_fread(image, buffer, sizeof(buffer)) != sizeof(buffer))
			goto error;
		if (memcmp(buffer, "C64File", sizeof(buffer)))
			goto error;
		image_fseek(image, 26, SEEK_SET);
		snapshot_size -= 26;
	}
	else if (!mame_stricmp(file_type, "t64"))
	{
		/* t64 files - for GB64 Single T64s loading to x0801 - header is always the same size */
		if (image_fread(image, buffer, sizeof(buffer)) != sizeof(buffer))
			goto error;
		if (memcmp(buffer, "C64 tape image file", sizeof(buffer)))
			goto error;
		image_fseek(image, 94, SEEK_SET);
		snapshot_size -= 94;
	}
	else
	{
		goto error;
	}

	image_fread(image, &address, 2);
	address = LITTLE_ENDIANIZE_INT16(address);
	if (!mame_stricmp(file_type, "t64"))
		address = 2049;
	snapshot_size -= 2;

	data = malloc(snapshot_size);
	if (!data)
		goto error;

	bytesread = image_fread(image, data, snapshot_size);
	if (bytesread != snapshot_size)
		goto error;

	for (i = 0; i < snapshot_size; i++)
		program_write_byte(address + i + offset, data[i]);

	cbm_sethiaddress(address + snapshot_size);
	free(data);
	return INIT_PASS;

error:
	if (data)
		free(data);
	return INIT_FAIL;
}

static void cbm_quick_sethiaddress(UINT16 hiaddress)
{
	program_write_byte(0x31, hiaddress & 0xff);
	program_write_byte(0x2f, hiaddress & 0xff);
	program_write_byte(0x2d, hiaddress & 0xff);
	program_write_byte(0x32, hiaddress >> 8);
	program_write_byte(0x30, hiaddress >> 8);
	program_write_byte(0x2e, hiaddress >> 8);
}

QUICKLOAD_LOAD( cbm_c16 )
{
	return general_cbm_loadsnap(image, file_type, quickload_size, 0, cbm_quick_sethiaddress);
}

QUICKLOAD_LOAD( cbm_c64 )
{
	return general_cbm_loadsnap(image, file_type, quickload_size, 0, cbm_quick_sethiaddress);
}

QUICKLOAD_LOAD( cbm_vc20 )
{
	return general_cbm_loadsnap(image, file_type, quickload_size, 0, cbm_quick_sethiaddress);
}

static void cbm_pet_quick_sethiaddress(UINT16 hiaddress)
{
	program_write_byte(0x2e, hiaddress & 0xff);
	program_write_byte(0x2c, hiaddress & 0xff);
	program_write_byte(0x2a, hiaddress & 0xff);
	program_write_byte(0x2f, hiaddress >> 8);
	program_write_byte(0x2d, hiaddress >> 8);
	program_write_byte(0x2b, hiaddress >> 8);
}

QUICKLOAD_LOAD( cbm_pet )
{
	return general_cbm_loadsnap(image, file_type, quickload_size, 0, cbm_pet_quick_sethiaddress);
}

static void cbm_pet1_quick_sethiaddress(UINT16 hiaddress)
{
	program_write_byte(0x80, hiaddress & 0xff);
	program_write_byte(0x7e, hiaddress & 0xff);
	program_write_byte(0x7c, hiaddress & 0xff);
	program_write_byte(0x81, hiaddress >> 8);
	program_write_byte(0x7f, hiaddress >> 8);
	program_write_byte(0x7d, hiaddress >> 8);
}

QUICKLOAD_LOAD( cbm_pet1 )
{
	return general_cbm_loadsnap(image, file_type, quickload_size, 0, cbm_pet1_quick_sethiaddress);
}

static void cbmb_quick_sethiaddress(UINT16 hiaddress)
{
	program_write_byte(0xf0046, hiaddress & 0xff);
	program_write_byte(0xf0047, hiaddress >> 8);
}

QUICKLOAD_LOAD( cbmb )
{
	return general_cbm_loadsnap(image, file_type, quickload_size, 0x10000, cbmb_quick_sethiaddress);
}

QUICKLOAD_LOAD( cbm500 )
{
	return general_cbm_loadsnap(image, file_type, quickload_size, 0, cbmb_quick_sethiaddress);
}

static void cbm_c65_quick_sethiaddress(UINT16 hiaddress)
{
	program_write_byte(0x82, hiaddress & 0xff);
	program_write_byte(0x83, hiaddress >> 8);
}

QUICKLOAD_LOAD( cbm_c65 )
{
	return general_cbm_loadsnap(image, file_type, quickload_size, 0, cbm_c65_quick_sethiaddress);
}


/***********************************************

	CBM Cartridges

***********************************************/


/*	All the cartridge specific code has been moved
	to machine/ drivers. Once more informations 
	surface about the cart expansions for systems 
	in c65.c, c128.c, cbmb.c and pet.c, the shared 
	code could be refactored to have here the 
	common functions								*/



/***********************************************

	CBM Datasette Tapes

***********************************************/

void datasette_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
	case MESS_DEVINFO_INT_COUNT:					info->i = 1; break;

	case MESS_DEVINFO_INT_CASSETTE_DEFAULT_STATE:	info->i = CASSETTE_STOPPED | CASSETTE_MOTOR_DISABLED | CASSETTE_SPEAKER_ENABLED; break;

	default:										cassette_device_getinfo(devclass, state, info); break;
	}
}
