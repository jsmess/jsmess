/*********************************************************************

	z80bin.c

	A binary quickload format used by the Microbee, the Exidy Sorcerer
	VZ200/300 and the Super 80

	NOTE: INIT_FAIL == 1, INIT_PASS == 0, EOF == 0xffffffff

	Return status: 0 = good data, 1 = corrupted quickload file.

*********************************************************************/

#include "driver.h"
#include "ui.h"
#include "z80bin.h"
#include "snapquik.h"


int z80bin_load_file( running_machine *machine, const device_config *image, const char *file_type, UINT16 *exec_addr, UINT16 *start_addr, UINT16 *end_addr )
{
	int ch;
	UINT16 args[3];
	UINT16 i=0, j, size;
	UINT8 data;
	char * pgmname;
	pgmname = (char *) malloc(256);

	image_fseek(image, 7, SEEK_SET);

	while((ch = image_fgetc(image)) != 0x1A)
	{
		if (ch == EOF)
		{
			ui_popup_time(10,"QUICKLOAD: Unexpected EOF while getting file name");	/* 10 seconds */
			free(pgmname);
			return INIT_FAIL;
		}

		if (ch)
		{
			pgmname[i] = ch;	/* build program name */
			i++;
		}
	}

	pgmname[i]=0;	/* terminate string with a null */

	if (image_fread(image, args, sizeof(args)) != sizeof(args))
	{
		ui_popup_time(10,"QUICKLOAD: %s\nUnexpected EOF while getting file size",pgmname);
		free(pgmname);
		return INIT_FAIL;
	}

	exec_addr[0]	= LITTLE_ENDIANIZE_INT16(args[0]);
	start_addr[0]	= LITTLE_ENDIANIZE_INT16(args[1]);
	end_addr[0]	= LITTLE_ENDIANIZE_INT16(args[2]);

	size = (end_addr[0] - start_addr[0] + 1) & 0xffff;

	popmessage("%s\nsize=%04X : start=%04X : end=%04X : exec=%04X",pgmname,size,start_addr[0],end_addr[0],exec_addr[0]);

	for (i = 0; i < size; i++)
	{
		j = (start_addr[0] + i) & 0xffff;
		if (image_fread(image, &data, 1) != 1)
		{
			ui_popup_time(10,"QUICKLOAD: %s\nUnexpected EOF while writing byte to %04X",pgmname,j);
			free(pgmname);
			return INIT_FAIL;
		}
		program_write_byte(j, data);
	}

	free(pgmname);

	return INIT_PASS;
}


static QUICKLOAD_LOAD( z80bin )
{
	UINT16 exec_addr, start_addr, end_addr;

	if (z80bin_load_file( machine, image, file_type, &exec_addr, &start_addr, &end_addr ) == INIT_FAIL)
		return INIT_FAIL;

	if (exec_addr == 0xffff) return INIT_PASS;			/* non-executable data file */
	cpunum_set_reg(0, REG_PC, exec_addr);				/* start program */
	return INIT_PASS;
}


void z80bin_quickload_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* quickload */
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_DEV_FILE:						strcpy(info->s = device_temp_str(), __FILE__); break;
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "bin"); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_QUICKLOAD_LOAD:				info->f = (genf *) quickload_load_z80bin; break;

		default:							quickload_device_getinfo(devclass, state, info); break;
	}
}

