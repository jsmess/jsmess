#include <stdio.h>
#include <stdarg.h>

#include "driver.h"
#include "image.h"
#include "includes/cbm.h"

UINT8 *cbmb_memory;

static int general_cbm_loadsnap(mess_image *image, const char *file_type, int snapshot_size,
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
	else
	{
		goto error;
	}

	image_fread(image, &address, 2);
	address = LITTLE_ENDIANIZE_INT16(address);
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

/* ----------------------------------------------------------------------- */

INT8 cbm_c64_game;
INT8 cbm_c64_exrom;
CBM_ROM cbm_rom[0x20]= { {0} };

static DEVICE_UNLOAD(cbm_rom)
{
	int id = image_index_in_device(image);
	cbm_rom[id].size = 0;
	cbm_rom[id].chip = 0;
}

static const struct IODevice *cbm_rom_find_device(void)
{
	return device_find(Machine->devices, IO_CARTSLOT);
}

static DEVICE_INIT(cbm_rom)
{
	int id = image_index_in_device(image);
	if (id == 0)
	{
		cbm_c64_game = -1;
		cbm_c64_exrom = -1;
	}
	return INIT_PASS;
}

static DEVICE_LOAD(cbm_rom)
{
	int i;
	int size, j, read_;
	const char *filetype;
	int adr = 0;
	const struct IODevice *dev;

	for (i=0; (i<sizeof(cbm_rom) / sizeof(cbm_rom[0])) && (cbm_rom[i].size!=0); i++)
		;
	if (i >= sizeof(cbm_rom) / sizeof(cbm_rom[0]))
		return INIT_FAIL;

	dev = cbm_rom_find_device();

	size = image_length(image);

	filetype = image_filetype(image);
	if (filetype && !mame_stricmp(filetype, "prg"))
	{
		unsigned short in;

		image_fread (image, &in, 2);
		in = LITTLE_ENDIANIZE_INT16(in);
		logerror("rom prg %.4x\n", in);
		size -= 2;

		logerror("loading rom %s at %.4x size:%.4x\n", image_filename(image), in, size);

		cbm_rom[i].chip = (UINT8 *) image_malloc(image, size);
		if (!cbm_rom[i].chip)
			return INIT_FAIL;

		cbm_rom[i].addr=in;
		cbm_rom[i].size=size;
		read_ = image_fread (image, cbm_rom[i].chip, size);
		if (read_ != size)
			return INIT_FAIL;
	}
	else if (filetype && !mame_stricmp (filetype, "crt"))
	{
		unsigned short in;
		image_fseek(image, 0x18, SEEK_SET);
		image_fread(image, &cbm_c64_exrom, 1);
		image_fread(image, &cbm_c64_game, 1);
		image_fseek(image, 64, SEEK_SET);
		j = 64;

		logerror("loading rom %s size:%.4x\n", image_filename(image), size);

		while (j < size)
		{
			unsigned short segsize;
			unsigned char buffer[10], number;

			image_fread(image, buffer, 6);
			image_fread(image, &segsize, 2);
			segsize = BIG_ENDIANIZE_INT16(segsize);
			image_fread(image, buffer + 6, 3);
			image_fread(image, &number, 1);
			image_fread(image, &adr, 2);
			adr = BIG_ENDIANIZE_INT16(adr);
			image_fread(image, &in, 2);
			in = BIG_ENDIANIZE_INT16(in);
			logerror("%.4s %.2x %.2x %.4x %.2x %.2x %.2x %.2x %.4x:%.4x\n",
				buffer, buffer[4], buffer[5], segsize,
				buffer[6], buffer[7], buffer[8], number,
				adr, in);
			logerror("loading chip at %.4x size:%.4x\n", adr, in);

			cbm_rom[i].chip = (UINT8*) image_malloc(image, size);
			if (!cbm_rom[i].chip)
				return INIT_FAIL;

			cbm_rom[i].addr=adr;
			cbm_rom[i].size=in;
			read_ = image_fread(image, cbm_rom[i].chip, in);
			i++;
			if (read_ != in)
				return INIT_FAIL;

			j += 16 + in;
		}
	}
	else if (filetype)
	{
		if (mame_stricmp(filetype, "lo") == 0)
			adr = CBM_ROM_ADDR_LO;
		else if (mame_stricmp (filetype, "hi") == 0)
			adr = CBM_ROM_ADDR_HI;
		else if (mame_stricmp (filetype, "10") == 0)
			adr = 0x1000;
		else if (mame_stricmp (filetype, "20") == 0)
			adr = 0x2000;
		else if (mame_stricmp (filetype, "30") == 0)
			adr = 0x3000;
		else if (mame_stricmp (filetype, "40") == 0)
			adr = 0x4000;
		else if (mame_stricmp (filetype, "50") == 0)
			adr = 0x5000;
		else if (mame_stricmp (filetype, "60") == 0)
			adr = 0x6000;
		else if (mame_stricmp (filetype, "70") == 0)
			adr = 0x7000;
		else if (mame_stricmp (filetype, "80") == 0)
			adr = 0x8000;
		else if (mame_stricmp (filetype, "90") == 0)
			adr = 0x9000;
		else if (mame_stricmp (filetype, "a0") == 0)
			adr = 0xa000;
		else if (mame_stricmp (filetype, "b0") == 0)
			adr = 0xb000;
		else if (mame_stricmp (filetype, "e0") == 0)
			adr = 0xe000;
		else if (mame_stricmp (filetype, "f0") == 0)
			adr = 0xf000;
		else
			adr = CBM_ROM_ADDR_UNKNOWN;

		logerror("loading %s rom at %.4x size:%.4x\n",
				image_filename(image), adr, size);

		cbm_rom[i].chip = (UINT8*) image_malloc(image, size);
		if (!cbm_rom[i].chip)
			return INIT_FAIL;

		cbm_rom[i].addr=adr;
		cbm_rom[i].size=size;
		read_ = image_fread(image, cbm_rom[i].chip, size);

		if (read_ != size)
			return INIT_FAIL;
	}
	return INIT_PASS;
}



void cbmcartslot_device_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:					info->i = 2; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:		strcpy(info->s = device_temp_str(), "crt"); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_INIT:					info->init = device_init_cbm_rom; break;
		case DEVINFO_PTR_LOAD:					info->load = device_load_cbm_rom; break;
		case DEVINFO_PTR_UNLOAD:				info->unload = device_unload_cbm_rom; break;

		default: cartslot_device_getinfo(devclass, state, info);
	}
}

