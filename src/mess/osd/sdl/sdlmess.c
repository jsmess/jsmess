#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
	
#include "osdmess.h"
#include "utils.h"

//============================================================
//	osd_getcurdir
//============================================================

mame_file_error osd_getcurdir(char *buffer, size_t buffer_len)
{
	mame_file_error filerr = FILERR_NONE;

	if (getcwd(buffer, buffer_len) != 0)
	{
		filerr = FILERR_FAILURE;
	}

	return filerr;
}

//============================================================
//	osd_setcurdir
//============================================================

mame_file_error osd_setcurdir(const char *dir)
{
	mame_file_error filerr = FILERR_NONE;

	if (chdir(dir) != 0)
	{
		filerr = FILERR_FAILURE;
	}

	return filerr;
}

//============================================================
//	osd_basename
//============================================================

char *osd_basename(char *filename)
{
	char *c;

	// NULL begets NULL
	if (!filename)
		return NULL;

	// start at the end and return when we hit a slash or colon
	for (c = filename + strlen(filename) - 1; c >= filename; c--)
		if (*c == '\\' || *c == '/' || *c == ':')
			return c + 1;

	// otherwise, return the whole thing
	return filename;
}

//============================================================
//	osd_dirname
//============================================================

char *osd_dirname(const char *filename)
{
	char *dirname;
	char *c;

	// NULL begets NULL
	if (!filename)
		return NULL;

	// allocate space for it
	dirname = malloc(strlen(filename) + 1);
	if (!dirname)
		return NULL;

	// copy in the name
	strcpy(dirname, filename);

	// search backward for a slash or a colon
	for (c = dirname + strlen(dirname) - 1; c >= dirname; c--)
		if (*c == '\\' || *c == '/' || *c == ':')
		{
			// found it: NULL terminate and return
			*(c + 1) = 0;
			return dirname;
		}

	// otherwise, return an empty string
	dirname[0] = 0;
	return dirname;
}

// no idea what these are for, just cheese them
int osd_num_devices(void)
{
	return 1;
}

const char *osd_get_device_name(int idx)
{
	return "/";
}

void osd_change_device(const char *device)
{
}

//============================================================
//	osd_get_temp_filename
//============================================================

mame_file_error osd_get_temp_filename(char *buffer, size_t buffer_len, const char *basename)
{
	char tempbuf[512];

	if (!basename)
		basename = "tempfile";

	strcpy(tempbuf, basename);
	strcat(tempbuf, "XXXXXX");
	if (!mktemp(tempbuf))
	{
		return FILERR_FAILURE;
	}

	strncpy(buffer, tempbuf, buffer_len);	

	return FILERR_NONE;
}


