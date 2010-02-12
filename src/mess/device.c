/***************************************************************************

    device.c

    Definitions and manipulations for device structures

***************************************************************************/

#include <stddef.h>

#include "emu.h"
#include "device.h"

typedef struct _mess_device_type_info mess_device_type_info;
struct _mess_device_type_info
{
	iodevice_t type;
	const char *name;
	const char *shortname;
};

/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

/* The List of Devices, with Associated Names - Be careful to ensure that   *
 * this list matches the ENUM from device.h, so searches can use IO_COUNT   */
static const mess_device_type_info device_info_array[] =
{
	{ IO_CARTSLOT,	"cartridge",	"cart" }, /*  0 */
	{ IO_FLOPPY,	"floppydisk",	"flop" }, /*  1 */
	{ IO_HARDDISK,	"harddisk",		"hard" }, /*  2 */
	{ IO_CYLINDER,	"cylinder",		"cyln" }, /*  3 */
	{ IO_CASSETTE,	"cassette",		"cass" }, /*  4 */
	{ IO_PUNCHCARD,	"punchcard",	"pcrd" }, /*  5 */
	{ IO_PUNCHTAPE,	"punchtape",	"ptap" }, /*  6 */
	{ IO_PRINTER,	"printer",		"prin" }, /*  7 */
	{ IO_SERIAL,	"serial",		"serl" }, /*  8 */
	{ IO_PARALLEL,	"parallel",		"parl" }, /*  9 */
	{ IO_SNAPSHOT,	"snapshot",		"dump" }, /* 10 */
	{ IO_QUICKLOAD,	"quickload",	"quik" }, /* 11 */
	{ IO_MEMCARD,	"memcard",		"memc" }, /* 12 */	
	{ IO_CDROM,     "cdrom",        "cdrm" }, /* 13 */
	{ IO_MAGTAPE,	"magtape",		"magt" }, /* 14 */
};


/***************************************************************************
    CORE IMPLEMENTATION
***************************************************************************/

static const mess_device_type_info *find_device_type(iodevice_t type)
{
	int i;
	for (i = 0; i < ARRAY_LENGTH(device_info_array); i++)
	{
		if (device_info_array[i].type == type)
			return &device_info_array[i];
	}
	return NULL;
}



const char *device_typename(iodevice_t type)
{
	const mess_device_type_info *info = find_device_type(type);
	return (info != NULL) ? info->name : NULL;
}



const char *device_brieftypename(iodevice_t type)
{
	const mess_device_type_info *info = find_device_type(type);
	return (info != NULL) ? info->shortname : NULL;
}



iodevice_t device_typeid(const char *name)
{
	int i;
	for (i = 0; i < ARRAY_LENGTH(device_info_array); i++)
	{
		if (!mame_stricmp(name, device_info_array[i].name) || !mame_stricmp(name, device_info_array[i].shortname))
			return device_info_array[i].type;
	}
	return (iodevice_t)-1;
}
