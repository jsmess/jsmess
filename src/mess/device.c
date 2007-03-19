/***************************************************************************

	device.c

	Definitions and manipulations for device structures

***************************************************************************/

#include <assert.h>

#include "device.h"
#include "uitext.h"
#include "driver.h"


/*************************************
 *
 *	Names and shortnames
 *
 *************************************/

struct Devices
{
	iodevice_t type;
	const char *name;
	const char *shortname;
};

/* The List of Devices, with Associated Names - Be careful to ensure that   *
 * this list matches the ENUM from device.h, so searches can use IO_COUNT	*/
static const struct Devices device_info_array[] =
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
};

const char *device_typename(iodevice_t type)
{
	assert(type >= 0);
	assert(type < IO_COUNT);
	return device_info_array[type].name;
}



const char *device_brieftypename(iodevice_t type)
{
	assert(type >= 0);
	assert(type < IO_COUNT);
	return device_info_array[type].shortname;
}



int device_typeid(const char *name)
{
	int i;
	for (i = 0; i < sizeof(device_info_array) / sizeof(device_info_array[0]); i++)
	{
		if (!mame_stricmp(name, device_info_array[i].name) || !mame_stricmp(name, device_info_array[i].shortname))
			return i;
	}
	return -1;
}



static const char *internal_device_instancename(const device_class *devclass, int id,
	UINT32 base, const char *(*get_dev_typename)(iodevice_t))
{
	iodevice_t type;
	int count;
	const char *result;
	char *s;

	/* retrieve info about the device instance */
	result = device_get_info_string(devclass, base + id);
	if (!result)
	{
		/* not specified? default to device names based on the device type */
		type = (iodevice_t) (int) device_get_info_int(devclass, DEVINFO_INT_TYPE);
		count = (int) device_get_info_int(devclass, DEVINFO_INT_COUNT);
		result = get_dev_typename(type);

		/* need to number if there is more than one device */
		if (count > 1)
		{
			s = device_temp_str();
			sprintf(s, "%s%d", result, id + 1);
			result = s;
		}
	}
	return result;
}



const char *device_instancename(const device_class *devclass, int id)
{
	return internal_device_instancename(devclass, id, DEVINFO_STR_NAME, device_typename);
}



const char *device_briefinstancename(const device_class *devclass, int id)
{
	return internal_device_instancename(devclass, id, DEVINFO_STR_SHORT_NAME, device_brieftypename);
}



/*************************************
 *
 *	Device structure construction and destruction
 *
 *************************************/

static const char *default_device_name(const struct IODevice *dev, int id,
	char *buf, size_t bufsize)
{
	const char *name;

	/* use the cool new device string technique */
	name = device_get_info_string(&dev->devclass, DEVINFO_STR_DESCRIPTION+id);
	if (name)
	{
		snprintf(buf, bufsize, "%s", name);
		return buf;
	}

	name = ui_getstring((UI_cartridge - IO_CARTSLOT) + dev->type);
	if (dev->count > 1)
	{
		/* for the average user counting starts at #1 ;-) */
		snprintf(buf, bufsize, "%s #%d", name, id + 1);
		name = buf;
	}
	return name;
}



static void default_device_getdispositions(const struct IODevice *dev, int id,
	unsigned int *readable, unsigned int *writeable, unsigned int *creatable)
{
	*readable = dev->readable;
	*writeable = dev->writeable;
	*creatable = dev->creatable;
}



struct IODevice *devices_allocate(const game_driver *gamedrv)
{
	struct SystemConfigurationParamBlock params;
	device_getinfo_handler handlers[64];
	int count_overrides[sizeof(handlers) / sizeof(handlers[0])];
	int createimage_optcount, count, i, j, position;
	const char *file_extensions, *info_string;
	char *converted_file_extensions;
	struct IODevice *devices = NULL;

	memset(handlers, 0, sizeof(handlers));
	memset(count_overrides, 0, sizeof(count_overrides));

	if (gamedrv->sysconfig_ctor)
	{
		memset(&params, 0, sizeof(params));
		params.device_slotcount = sizeof(handlers) / sizeof(handlers[0]);
		params.device_handlers = handlers;
		params.device_countoverrides = count_overrides;
		gamedrv->sysconfig_ctor(&params);
	}

	/* count the amount of handlers that we have available */
	for (count = 0; handlers[count]; count++)
		;
	count++; /* for our purposes, include the tailing empty device */

	devices = (struct IODevice *) auto_malloc(count * sizeof(struct IODevice));
	memset(devices, 0, count * sizeof(struct IODevice));

	position = 0;

	for (i = 0; i < count; i++)
	{
		devices[i].type = IO_COUNT;

		if (handlers[i])
		{
			devices[i].devclass.get_info = handlers[i];
			devices[i].devclass.gamedrv = gamedrv;
			
			/* convert file extensions from comma delimited to null delimited */
			converted_file_extensions = NULL;
			file_extensions = device_get_info_string(&devices[i].devclass, DEVINFO_STR_FILE_EXTENSIONS);
			if (file_extensions)
			{
				converted_file_extensions = auto_malloc(strlen(file_extensions) + 2);
				for (j = 0; file_extensions[j]; j++)
					converted_file_extensions[j] = (file_extensions[j] != ',') ? file_extensions[j] : '\0';
				converted_file_extensions[j + 0] = '\0';
				converted_file_extensions[j + 1] = '\0';
			}
			
			info_string = device_get_info_string(&devices[i].devclass, DEVINFO_STR_DEV_TAG);
			devices[i].tag					= info_string ? auto_strdup(info_string) : NULL;
			devices[i].type					= device_get_info_int(&devices[i].devclass, DEVINFO_INT_TYPE);
			devices[i].count				= device_get_info_int(&devices[i].devclass, DEVINFO_INT_COUNT);
			devices[i].position				= position;
			devices[i].file_extensions		= converted_file_extensions;

			devices[i].readable				= device_get_info_int(&devices[i].devclass, DEVINFO_INT_READABLE) ? 1 : 0;
			devices[i].writeable			= device_get_info_int(&devices[i].devclass, DEVINFO_INT_WRITEABLE) ? 1 : 0;
			devices[i].creatable			= device_get_info_int(&devices[i].devclass, DEVINFO_INT_CREATABLE) ? 1 : 0;
			devices[i].reset_on_load		= device_get_info_int(&devices[i].devclass, DEVINFO_INT_RESET_ON_LOAD) ? 1 : 0;
			devices[i].must_be_loaded		= device_get_info_int(&devices[i].devclass, DEVINFO_INT_MUST_BE_LOADED) ? 1 : 0;
			devices[i].load_at_init			= device_get_info_int(&devices[i].devclass, DEVINFO_INT_LOAD_AT_INIT) ? 1 : 0;
			devices[i].not_working			= device_get_info_int(&devices[i].devclass, DEVINFO_INT_NOT_WORKING) ? 1 : 0;

			devices[i].init					= (device_init_handler) device_get_info_fct(&devices[i].devclass, DEVINFO_PTR_INIT);
			devices[i].exit					= (device_exit_handler) device_get_info_fct(&devices[i].devclass, DEVINFO_PTR_EXIT);
			devices[i].load					= (device_load_handler) device_get_info_fct(&devices[i].devclass, DEVINFO_PTR_LOAD);
			devices[i].create				= (device_create_handler) device_get_info_fct(&devices[i].devclass, DEVINFO_PTR_CREATE);
			devices[i].unload				= (device_unload_handler) device_get_info_fct(&devices[i].devclass, DEVINFO_PTR_UNLOAD);
			devices[i].imgverify			= (device_verify_handler) device_get_info_fct(&devices[i].devclass, DEVINFO_PTR_VERIFY);
			devices[i].partialhash			= (device_partialhash_handler) device_get_info_fct(&devices[i].devclass, DEVINFO_PTR_PARTIAL_HASH);
			devices[i].getdispositions		= (device_getdispositions_handler) device_get_info_fct(&devices[i].devclass, DEVINFO_PTR_GET_DISPOSITIONS);

			devices[i].display				= (device_display_handler) device_get_info_fct(&devices[i].devclass, DEVINFO_PTR_DISPLAY);
			devices[i].name					= default_device_name;

			devices[i].createimage_optguide	= (const struct OptionGuide *) device_get_info_ptr(&devices[i].devclass, DEVINFO_PTR_CREATE_OPTGUIDE);
			
			createimage_optcount = (int) device_get_info_int(&devices[i].devclass, DEVINFO_INT_CREATE_OPTCOUNT);
			if (createimage_optcount > 0)
			{
				if (createimage_optcount > DEVINFO_CREATE_OPTMAX)
					fatalerror("DEVINFO_INT_CREATE_OPTCOUNT: Too many options");

				devices[i].createimage_options = auto_malloc((createimage_optcount + 1) *
					sizeof(*devices[i].createimage_options));

				for (j = 0; j < createimage_optcount; j++)
				{
					info_string = device_get_info_string(&devices[i].devclass, DEVINFO_STR_CREATE_OPTNAME + j);
					devices[i].createimage_options[j].name			= info_string ? auto_strdup(info_string) : NULL;
					info_string = device_get_info_string(&devices[i].devclass, DEVINFO_STR_CREATE_OPTDESC + j);
					devices[i].createimage_options[j].description	= info_string ? auto_strdup(info_string) : NULL;
					info_string = device_get_info_string(&devices[i].devclass, DEVINFO_STR_CREATE_OPTEXTS + j);
					devices[i].createimage_options[j].extensions	= info_string ? auto_strdup(info_string) : NULL;
					devices[i].createimage_options[j].optspec		= device_get_info_ptr(&devices[i].devclass, DEVINFO_PTR_CREATE_OPTSPEC + j);
				}

				/* terminate the list */
				memset(&devices[i].createimage_options[createimage_optcount], 0,
					sizeof(devices[i].createimage_options[createimage_optcount]));
			}

			position += devices[i].count;

			/* overriding the count? */
			if (count_overrides[i])
				devices[i].count = count_overrides[i];

			/* any problems? */
			if ((devices[i].type < 0) || (devices[i].type >= IO_COUNT))
				goto error;
			if ((devices[i].count < 0) || (devices[i].count > MAX_DEV_INSTANCES))
				goto error;

			/* fill in defaults */
			if (!devices[i].getdispositions)
				devices[i].getdispositions = default_device_getdispositions;
		}
	}

	return devices;

error:
	return NULL;
}



/*************************************
 *
 *	Device lookup
 *
 *************************************/

const struct IODevice *device_find_tag(const struct IODevice *devices, const char *tag)
{
	int i;
	for (i = 0; devices[i].type != IO_COUNT; i++)
	{
		if (devices[i].tag && !strcmp(devices[i].tag, tag))
			return &devices[i];
	}
	return NULL;
}



int device_count_tag(const struct IODevice *devices, const char *tag)
{
	const struct IODevice *dev;
	dev = device_find_tag(devices, tag);
	return dev ? dev->count : 0;
}



/* this function is deprecated */
const struct IODevice *device_find(const struct IODevice *devices, iodevice_t type)
{
	int i;
	for (i = 0; devices[i].type != IO_COUNT; i++)
	{
		if (devices[i].type == type)
			return &devices[i];
	}
	return NULL;
}



/* this function is deprecated */
int device_count(iodevice_t type)
{
	const struct IODevice *dev = NULL;
	if (Machine->devices)
		dev = device_find(Machine->devices, type);
	return dev ? dev->count : 0;
}



/*************************************
 *
 *	Diagnostics
 *
 *************************************/

int device_valididtychecks(void)
{
	int error = 0;
	int i;

	if ((sizeof(device_info_array) / sizeof(device_info_array[0])) != IO_COUNT)
	{
		mame_printf_error("device_info_array array should match size of IO_* enum\n");
		error = 1;
	}

	/* Check the device struct array */
	for (i = 0; i < sizeof(device_info_array) / sizeof(device_info_array[0]); i++)
	{
		if (device_info_array[i].type != i)
		{
			mame_printf_error("Device struct array order mismatch\n");
			error = 1;
			break;
		}

		if (!device_info_array[i].name)
		{
			mame_printf_error("device_info_array[%d].name appears to be NULL\n", i);
			error = 1;
		}

		if (!device_info_array[i].shortname)
		{
			mame_printf_error("device_info_array[%d].shortname appears to be NULL\n", i);
			error = 1;
		}
	}
	return error;
}

