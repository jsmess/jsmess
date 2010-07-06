/*********************************************************************

    z80bin.c

    A binary quickload format used by the Microbee, the Exidy Sorcerer
    VZ200/300 and the Super 80

*********************************************************************/

#include "emu.h"
#include "z80bin.h"
#include "snapquik.h"



/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    z80bin_load_file - load a z80bin file into
    memory
-------------------------------------------------*/

static int z80bin_load_file(device_image_interface *image, const char *file_type, UINT16 *exec_addr, UINT16 *start_addr, UINT16 *end_addr )
{
	int ch;
	UINT16 args[3];
	UINT16 i=0, j, size;
	UINT8 data;
	char pgmname[256];
	char message[256];

	image->fseek(7, SEEK_SET);

	while((ch = image->fgetc()) != 0x1A)
	{
		if (ch == EOF)
		{
			image->seterror(IMAGE_ERROR_INVALIDIMAGE, "Unexpected EOF while getting file name");
			image->message(" Unexpected EOF while getting file name");
			return IMAGE_INIT_FAIL;
		}

		if (ch != '\0')
		{
			if (i >= (ARRAY_LENGTH(pgmname) - 1))
			{
				image->seterror(IMAGE_ERROR_INVALIDIMAGE, "File name too long");
				image->message(" File name too long");
				return IMAGE_INIT_FAIL;
			}

			pgmname[i] = ch;	/* build program name */
			i++;
		}
	}

	pgmname[i] = '\0';	/* terminate string with a null */

	if (image->fread(args, sizeof(args)) != sizeof(args))
	{
		image->seterror(IMAGE_ERROR_INVALIDIMAGE, "Unexpected EOF while getting file size");
		image->message(" Unexpected EOF while getting file size");
		return IMAGE_INIT_FAIL;
	}

	exec_addr[0] = LITTLE_ENDIANIZE_INT16(args[0]);
	start_addr[0] = LITTLE_ENDIANIZE_INT16(args[1]);
	end_addr[0]	= LITTLE_ENDIANIZE_INT16(args[2]);

	size = (end_addr[0] - start_addr[0] + 1) & 0xffff;

	/* display a message about the loaded quickload */
	image->message(" %s\nsize=%04X : start=%04X : end=%04X : exec=%04X",pgmname,size,start_addr[0],end_addr[0],exec_addr[0]);

	for (i = 0; i < size; i++)
	{
		j = (start_addr[0] + i) & 0xffff;
		if (image->fread(&data, 1) != 1)
		{
			snprintf(message, ARRAY_LENGTH(message), "%s: Unexpected EOF while writing byte to %04X", pgmname, (unsigned) j);
			image->seterror(IMAGE_ERROR_INVALIDIMAGE, message);
			image->message("%s: Unexpected EOF while writing byte to %04X", pgmname, (unsigned) j);
			return IMAGE_INIT_FAIL;
		}
		memory_write_byte(cputag_get_address_space(image->device().machine,"maincpu",ADDRESS_SPACE_PROGRAM), j, data);
	}

	return IMAGE_INIT_PASS;
}



/*-------------------------------------------------
    QUICKLOAD_LOAD( z80bin )
-------------------------------------------------*/

static QUICKLOAD_LOAD( z80bin )
{
	const z80bin_config *config;
	UINT16 exec_addr, start_addr, end_addr;
	int autorun;

	/* load the binary into memory */
	if (z80bin_load_file(&image, file_type, &exec_addr, &start_addr, &end_addr) == IMAGE_INIT_FAIL)
		return IMAGE_INIT_FAIL;

	/* is this file executable? */
	if (exec_addr != 0xffff)
	{
		config = (const z80bin_config *)downcast<const legacy_device_config_base &>(image.device().baseconfig()).inline_config();

		/* check to see if autorun is on (I hate how this works) */
		autorun = input_port_read_safe(image.device().machine, "CONFIG", 0xFF) & 1;

		/* start program */
		if (config->execute != NULL)
		{
			(*config->execute)(image.device().machine, start_addr, end_addr, exec_addr, autorun);
		}
		else
		{
			if (autorun)
				cpu_set_reg(devtag_get_device(image.device().machine, "maincpu"), STATE_GENPC, exec_addr);
		}
	}

	return IMAGE_INIT_PASS;
}



/*-------------------------------------------------
    DEVICE_GET_INFO(z80bin)
-------------------------------------------------*/

DEVICE_GET_INFO(z80bin)
{
	/* quickload */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = sizeof(z80bin_config); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__); break;
		case DEVINFO_STR_IMAGE_FILE_EXTENSIONS:			strcpy(info->s, "bin"); break;

		/* --- the following bits of info are returned as pointers to functions --- */
		case DEVINFO_FCT_SNAPSHOT_QUICKLOAD_LOAD:		info->f = (genf *) quickload_load_z80bin; break;

		default:	DEVICE_GET_INFO_CALL(quickload); break;
	}
}

DEFINE_LEGACY_IMAGE_DEVICE(Z80BIN, z80bin);
