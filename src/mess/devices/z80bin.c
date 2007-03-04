/*********************************************************************

	z80bin.c

	A binary quickload format used by the Microbee, the Exidy Sorcerer
	VZ200/300 and the Super 80

*********************************************************************/

#include "mame.h"
#include "driver.h"
#include "snapquik.h"
#include "z80bin.h"


static QUICKLOAD_LOAD( z80bin )
{
	int ch;
	UINT16 args[3];
	UINT16 start_addr, end_addr, exec_addr, i;
	UINT8 data;

	image_fseek(image, 7, SEEK_SET);

	while((ch = image_fgetc(image)) != 0x1A)
	{
		if (ch == EOF)
			return INIT_FAIL;
	}

	if (image_fread(image, args, sizeof(args)) != sizeof(args))
		return INIT_FAIL;

	exec_addr	= LITTLE_ENDIANIZE_INT16(args[0]);
	start_addr	= LITTLE_ENDIANIZE_INT16(args[1]);
	end_addr	= LITTLE_ENDIANIZE_INT16(args[2]);

	for (i = start_addr; i <= end_addr; i++)
	{
		if (image_fread(image, &data, 1) != 1)
			return INIT_FAIL;
		program_write_byte(i, data);
	}
	cpunum_set_reg(0, REG_PC, exec_addr);
	return INIT_PASS;
}



void z80bin_quickload_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* quickload */
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_DEV_FILE:						strcpy(info->s = device_temp_str(), __FILE__); break;
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "bin"); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_QUICKLOAD_LOAD:				info->f = (genf *) quickload_load_z80bin; break;

		default:										quickload_device_getinfo(devclass, state, info); break;
	}
}

