/***************************************************************************

	device.c

	Definitions and manipulations for device structures

***************************************************************************/

#include <stddef.h>

#include "device.h"
#include "deprecat.h"
#include "mslegacy.h"
#include "driver.h"
#include "pool.h"



/***************************************************************************
    CONSTANTS
***************************************************************************/

#define MESS_DEVICE		DEVICE_GET_INFO_NAME(mess_device)



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/* this is placed in the inline_config of the MAME device structure */
typedef struct _mess_device_config mess_device_config;
struct _mess_device_config
{
	struct IODevice io_device;
	char string_buffer[1024];
	struct CreateImageOptions createimage_options[MESS_DEVINFO_CREATE_OPTMAX + 1];
};



struct Devices
{
	iodevice_t type;
	const char *name;
	const char *shortname;
};



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

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



/***************************************************************************
    CORE IMPLEMENTATION
***************************************************************************/

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



static const char *internal_device_instancename(const mess_device_class *devclass, int id,
	UINT32 base, const char *(*get_dev_typename)(iodevice_t))
{
	iodevice_t type;
	int count;
	const char *result;
	char *s;

	/* retrieve info about the device instance */
	result = mess_device_get_info_string(devclass, base + id);
	if (!result)
	{
		/* not specified? default to device names based on the device type */
		type = (iodevice_t) (int) mess_device_get_info_int(devclass, MESS_DEVINFO_INT_TYPE);
		count = (int) mess_device_get_info_int(devclass, MESS_DEVINFO_INT_COUNT);
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



const char *device_instancename(const mess_device_class *devclass, int id)
{
	return internal_device_instancename(devclass, id, MESS_DEVINFO_STR_NAME, device_typename);
}



const char *device_briefinstancename(const mess_device_class *devclass, int id)
{
	return internal_device_instancename(devclass, id, MESS_DEVINFO_STR_SHORT_NAME, device_brieftypename);
}



/*************************************
 *
 *	Device structure construction and destruction
 *
 *************************************/

const char *device_uiname(iodevice_t devtype)
{
	return ui_getstring((UI_cartridge - IO_CARTSLOT) + devtype);
}



static const char *default_device_name(const struct IODevice *dev, int id,
	char *buf, size_t bufsize)
{
	const char *name;

	/* use the cool new device string technique */
	name = mess_device_get_info_string(&dev->devclass, MESS_DEVINFO_STR_DESCRIPTION+id);
	if (name)
	{
		snprintf(buf, bufsize, "%s", name);
		return buf;
	}

	name = device_uiname(dev->type);
	if (dev->multiple)
	{
		/* for the average user counting starts at #1 ;-) */
		snprintf(buf, bufsize, "%s #%d", name, id + 1);
		name = buf;
	}
	return name;
}



/*-------------------------------------------------
    DEVICE_START(mess_device) - device start
    callback
-------------------------------------------------*/

static DEVICE_START(mess_device)
{
}



/*-------------------------------------------------
    DEVICE_GET_INFO(mess_device) - device get info
    callback
-------------------------------------------------*/

DEVICE_GET_INFO(mess_device)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_INLINE_CONFIG_BYTES:	info->i = sizeof(mess_device_config);			break;
		case DEVINFO_INT_TOKEN_BYTES:			info->i = 1;									break;
		case DEVINFO_INT_CLASS:					info->i = DEVICE_CLASS_OTHER;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_SET_INFO:				/* Nothing */									break;
		case DEVINFO_FCT_START:					info->start = DEVICE_START_NAME(mess_device);	break;
		case DEVINFO_FCT_STOP:					/* Nothing */									break;
		case DEVINFO_FCT_RESET:					/* Nothing */									break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:					info->s = "Legacy MESS Device";					break;
		case DEVINFO_STR_FAMILY:				info->s = "Legacy MESS Device";					break;
		case DEVINFO_STR_VERSION:				info->s = "1.0";								break;
		case DEVINFO_STR_SOURCE_FILE:			info->s = __FILE__;								break;
		case DEVINFO_STR_CREDITS:				info->s = "Copyright the MESS Team";			break;
	}
}



/*-------------------------------------------------
    string_buffer_putc
-------------------------------------------------*/

static void string_buffer_putc(char *buffer, size_t buffer_length, size_t *buffer_pos, char c)
{
	size_t pos;
	
	/* sanity check */
	assert_always(*buffer_pos < buffer_length, "Buffer too small");
	
	pos = (*buffer_pos)++;
	buffer[pos] = c;
}



/*-------------------------------------------------
    string_buffer_putstr
-------------------------------------------------*/

static char *string_buffer_putstr(char *buffer, size_t buffer_length, size_t *buffer_pos, const char *s)
{
	char *result = NULL;
	size_t i = 0;

	if (s != NULL)
	{
		result = &buffer[*buffer_pos];
		do
		{
			string_buffer_putc(buffer, buffer_length, buffer_pos, s[i]);
		}
		while(s[i++] != '\0');
	}

	return result;
}



/*-------------------------------------------------
    create_mess_device - allocates a single legacy
	MESS device
-------------------------------------------------*/

static void create_mess_device(device_config **listheadptr, device_getinfo_handler handler, const game_driver *gamedrv,
	int count_override, int *position)
{
	mess_device_class mess_devclass;
	const char *mess_tag;
	const char *mame_tag;
	const char *info_string;
	char dynamic_tag[32];
	device_config *device;
	mess_device_config *mess_device;
	const char *file_extensions;
	int i, j, count;
	size_t string_buffer_pos = 0;
	int createimage_optcount;
	device_getdispositions_handler getdispositions;
	unsigned int readable, writeable, creatable;

	/* set up MESS's device class */
	mess_devclass.get_info = handler;
	mess_devclass.gamedrv = gamedrv;

	/* determine device count */
	count = mess_device_get_info_int(&mess_devclass, MESS_DEVINFO_INT_COUNT);

	/* create one MAME device for each of these */
	for (i = 0; i < count; i++)
	{
		/* determine the legacy MESS device tag */
		mess_tag = mess_device_get_info_string(&mess_devclass, MESS_DEVINFO_STR_DEV_TAG);

		/* create a MAME device tag based on it */
		if (mess_tag != NULL)
		{
			snprintf(dynamic_tag, ARRAY_LENGTH(dynamic_tag), "%s_%d", mess_tag, i);
			mame_tag = dynamic_tag;
		}
		else
		{
			/* create a default tag */
			snprintf(dynamic_tag, ARRAY_LENGTH(dynamic_tag), "mess_device_%d", *position);
			mame_tag = dynamic_tag;
		}

		/* create a bonafide MAME device */
		device = device_list_add(listheadptr, MESS_DEVICE, mame_tag);
		mess_device = (mess_device_config *) device->inline_config;

		/* we need to copy the mess_tag into the structure */
		info_string = mess_tag;
		mess_tag = string_buffer_putstr(mess_device->string_buffer, ARRAY_LENGTH(mess_device->string_buffer), &string_buffer_pos, info_string);

		/* convert file extensions from comma delimited to null delimited */
		file_extensions = mess_device_get_info_string(&mess_devclass, MESS_DEVINFO_STR_FILE_EXTENSIONS);
		if (file_extensions != NULL)
		{
			const char *file_extensions_copy = &mess_device->string_buffer[string_buffer_pos];

			/* copy the file extensions over to the buffer */
			for (j = 0; file_extensions[j] != '\0'; j++)
			{
				string_buffer_putc(mess_device->string_buffer, ARRAY_LENGTH(mess_device->string_buffer), &string_buffer_pos,
					(file_extensions[j] != ',') ? file_extensions[j] : '\0');
			}
			string_buffer_putc(mess_device->string_buffer, ARRAY_LENGTH(mess_device->string_buffer), &string_buffer_pos, '\0');
			string_buffer_putc(mess_device->string_buffer, ARRAY_LENGTH(mess_device->string_buffer), &string_buffer_pos, '\0');

			file_extensions = file_extensions_copy;
		}

		/* populate the legacy MESS device */
		mess_device->io_device.tag					= mess_tag;
		mess_device->io_device.devconfig			= device;
		mess_device->io_device.devclass				= mess_devclass;
		mess_device->io_device.type					= mess_device_get_info_int(&mess_device->io_device.devclass, MESS_DEVINFO_INT_TYPE);
		mess_device->io_device.position				= *position;
		mess_device->io_device.index_in_device		= i;
		mess_device->io_device.file_extensions		= file_extensions;

		mess_device->io_device.reset_on_load		= mess_device_get_info_int(&mess_device->io_device.devclass, MESS_DEVINFO_INT_RESET_ON_LOAD) ? 1 : 0;
		mess_device->io_device.must_be_loaded		= mess_device_get_info_int(&mess_device->io_device.devclass, MESS_DEVINFO_INT_MUST_BE_LOADED) ? 1 : 0;
		mess_device->io_device.load_at_init			= mess_device_get_info_int(&mess_device->io_device.devclass, MESS_DEVINFO_INT_LOAD_AT_INIT) ? 1 : 0;

		mess_device->io_device.init					= (device_init_handler) mess_device_get_info_fct(&mess_device->io_device.devclass, MESS_DEVINFO_PTR_INIT);
		mess_device->io_device.exit					= (device_exit_handler) mess_device_get_info_fct(&mess_device->io_device.devclass, MESS_DEVINFO_PTR_EXIT);
		mess_device->io_device.load					= (device_load_handler) mess_device_get_info_fct(&mess_device->io_device.devclass, MESS_DEVINFO_PTR_LOAD);
		mess_device->io_device.create				= (device_create_handler) mess_device_get_info_fct(&mess_device->io_device.devclass, MESS_DEVINFO_PTR_CREATE);
		mess_device->io_device.unload				= (device_unload_handler) mess_device_get_info_fct(&mess_device->io_device.devclass, MESS_DEVINFO_PTR_UNLOAD);
		mess_device->io_device.imgverify			= (device_verify_handler) mess_device_get_info_fct(&mess_device->io_device.devclass, MESS_DEVINFO_PTR_VERIFY);
		mess_device->io_device.partialhash			= (device_partialhash_handler) mess_device_get_info_fct(&mess_device->io_device.devclass, MESS_DEVINFO_PTR_PARTIAL_HASH);

		mess_device->io_device.display				= (device_display_handler) mess_device_get_info_fct(&mess_device->io_device.devclass, MESS_DEVINFO_PTR_DISPLAY);
		mess_device->io_device.name					= default_device_name;

		mess_device->io_device.createimage_optguide	= (const struct OptionGuide *) mess_device_get_info_ptr(&mess_device->io_device.devclass, MESS_DEVINFO_PTR_CREATE_OPTGUIDE);

		createimage_optcount = (int) mess_device_get_info_int(&mess_device->io_device.devclass, MESS_DEVINFO_INT_CREATE_OPTCOUNT);
		if (createimage_optcount > 0)
		{
			if (createimage_optcount > MESS_DEVINFO_CREATE_OPTMAX)
				fatalerror("MESS_DEVINFO_INT_CREATE_OPTCOUNT: Too many options");

			/* set up each option in the list */
			for (j = 0; j < createimage_optcount; j++)
			{
				info_string = mess_device_get_info_string(&mess_device->io_device.devclass, MESS_DEVINFO_STR_CREATE_OPTNAME + j);
				mess_device->createimage_options[j].name		= string_buffer_putstr(mess_device->string_buffer, ARRAY_LENGTH(mess_device->string_buffer), &string_buffer_pos, info_string);

				info_string = mess_device_get_info_string(&mess_device->io_device.devclass, MESS_DEVINFO_STR_CREATE_OPTDESC + j);
				mess_device->createimage_options[j].description	= string_buffer_putstr(mess_device->string_buffer, ARRAY_LENGTH(mess_device->string_buffer), &string_buffer_pos, info_string);
				
				info_string = mess_device_get_info_string(&mess_device->io_device.devclass, MESS_DEVINFO_STR_CREATE_OPTEXTS + j);
				mess_device->createimage_options[j].extensions	= string_buffer_putstr(mess_device->string_buffer, ARRAY_LENGTH(mess_device->string_buffer), &string_buffer_pos, info_string);
				
				mess_device->createimage_options[j].optspec		= mess_device_get_info_ptr(&mess_device->io_device.devclass, MESS_DEVINFO_PTR_CREATE_OPTSPEC + j);
			}

			/* terminate the list */
			memset(&mess_device->createimage_options[createimage_optcount], 0, sizeof(mess_device->createimage_options[createimage_optcount]));

			/* assign the options */
			mess_device->io_device.createimage_options = mess_device->createimage_options;
		}

		/* determine the dispositions */
		getdispositions = (device_getdispositions_handler) mess_device_get_info_fct(&mess_device->io_device.devclass, MESS_DEVINFO_PTR_GET_DISPOSITIONS);
		if (getdispositions != NULL)
		{
			getdispositions(&mess_device->io_device, i, &readable, &writeable, &creatable);
		}
		else
		{
			readable = mess_device_get_info_int(&mess_device->io_device.devclass, MESS_DEVINFO_INT_READABLE) ? 1 : 0;
			writeable = mess_device_get_info_int(&mess_device->io_device.devclass, MESS_DEVINFO_INT_WRITEABLE) ? 1 : 0;
			creatable = mess_device_get_info_int(&mess_device->io_device.devclass, MESS_DEVINFO_INT_CREATABLE) ? 1 : 0;
		}
		mess_device->io_device.readable = readable;
		mess_device->io_device.writeable = writeable;
		mess_device->io_device.creatable = creatable;

		/* bump the position */
		(*position)++;

		/* overriding the count? */
		//if (count_override != 0)
		//	mess_device->io_device.count = count_override;

		/* any problems? */
		assert((mess_device->io_device.type >= 0) && (mess_device->io_device.type < IO_COUNT));
	}
}



/*-------------------------------------------------
    mess_devices_setup - allocate devices
-------------------------------------------------*/

void mess_devices_setup(machine_config *config, const game_driver *gamedrv)
{
	device_getinfo_handler handlers[64];
	int count_overrides[sizeof(handlers) / sizeof(handlers[0])];
	int i, position = 0;

	memset(handlers, 0, sizeof(handlers));
	memset(count_overrides, 0, sizeof(count_overrides));

	/* call the "sysconfig" constructor, if present */
	if (gamedrv->sysconfig_ctor != NULL)
	{
		struct SystemConfigurationParamBlock params;

		memset(&params, 0, sizeof(params));
		params.device_slotcount = sizeof(handlers) / sizeof(handlers[0]);
		params.device_handlers = handlers;
		params.device_countoverrides = count_overrides;
		gamedrv->sysconfig_ctor(&params);
	}

	/* loop through all handlers */
	for (i = 0; handlers[i] != NULL; i++)
	{
		create_mess_device(&config->devicelist, handlers[i], gamedrv, count_overrides[i], &position);
	}
}



/*-------------------------------------------------
    machine_config_alloc_with_mess_devices -
	allocate a machine_config with MESS devices
-------------------------------------------------*/

machine_config *machine_config_alloc_with_mess_devices(const game_driver *gamedrv)
{
	machine_config *config = machine_config_alloc(gamedrv->machine_config);
	mess_devices_setup(config, gamedrv);
	return config;
}



/*************************************
 *
 *	Device enumeration
 *
 *************************************/

const struct IODevice *mess_device_first_from_machine(const running_machine *machine)
{
	const device_config *devconfig;
	devconfig = device_list_first(machine->config->devicelist, MESS_DEVICE);
	return (devconfig != NULL) ? devconfig->inline_config : NULL;
}



const struct IODevice *mess_device_next(const struct IODevice *dev)
{
	const device_config *devconfig;
	devconfig = device_list_next(dev->devconfig, MESS_DEVICE);
	return (devconfig != NULL) ? devconfig->inline_config : NULL;
}



/*************************************
 *
 *	Device lookup
 *
 *************************************/

const struct IODevice *mess_device_from_core_device(const device_config *device)
{
	const mess_device_config *mess_device = (device != NULL) ? (const mess_device_config *) device->inline_config : NULL;
	return (mess_device != NULL) ? &mess_device->io_device : NULL;
}



int device_count_tag_from_machine(const running_machine *machine, const char *tag)
{
	int count = 0;
	const device_config *device;

	for (device = device_list_first(machine->config->devicelist, MESS_DEVICE); device != NULL; device = device_list_next(device, MESS_DEVICE))
	{
		const struct IODevice *iodev = mess_device_from_core_device(device);
		if (!strcmp(tag, iodev->tag))
		{
			count++;
		}
	}
	return count;
}



/* this function is deprecated */
const struct IODevice *device_find_from_machine(const running_machine *machine, iodevice_t type)
{
	const device_config *device;

	for (device = device_list_first(machine->config->devicelist, MESS_DEVICE); device != NULL; device = device_list_next(device, MESS_DEVICE))
	{
		const struct IODevice *iodev = mess_device_from_core_device(device);
		if (iodev->type == type)
		{
			return iodev;
		}
	}
	return NULL;
}



/* this function is deprecated */
int device_count(iodevice_t type)
{
	int count = 0;
	const device_config *device;

	for (device = device_list_first(Machine->config->devicelist, MESS_DEVICE); device != NULL; device = device_list_next(device, MESS_DEVICE))
	{
		const struct IODevice *iodev = mess_device_from_core_device(device);
		if (iodev->type == type)
		{
			count++;
		}
	}
	return count;
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

