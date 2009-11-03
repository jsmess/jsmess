/***************************************************************************

	device.c

	Definitions and manipulations for device structures

***************************************************************************/

#include <stddef.h>

#include "driver.h"
#include "device.h"
#include "mslegacy.h"
#include "pool.h"
#include "tagpool.h"



/***************************************************************************
    CONSTANTS
***************************************************************************/

#define MESS_DEVICE		DEVICE_GET_INFO_NAME(mess_device)



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _legacy_mess_device legacy_mess_device;
struct _legacy_mess_device
{
	mess_device_class devclass;
	const device_config *devconfig;

	/* the basics */
	const char *tag;
	iodevice_t type;
	int position;
	int index_in_device;

	/* open dispositions */
	unsigned int readable : 1;
	unsigned int writeable : 1;
	unsigned int creatable : 1;

	/* miscellaneous flags */
	unsigned int reset_on_load : 1;
	unsigned int load_at_init : 1;
	unsigned int multiple : 1;
};



/* this is placed in the inline_config of the MAME device structure */
typedef struct _mess_device_config mess_device_config;
struct _mess_device_config
{
	legacy_mess_device io_device;
	char string_buffer[1024];
};



typedef struct _mess_device_token mess_device_token;
struct _mess_device_token
{
	tag_pool tagpool;
};



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
 * this list matches the ENUM from device.h, so searches can use IO_COUNT	*/
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
};



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static const legacy_mess_device *mess_device_from_core_device(const device_config *device);



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
	return -1;
}



/*************************************
 *
 *	Device structure construction and destruction
 *
 *************************************/

static const char *device_uiname(iodevice_t devtype)
{
	return ui_getstring((UI_cartridge - IO_CARTSLOT) + devtype);
}



/*-------------------------------------------------
    DEVICE_START(mess_device) - device start
    callback
-------------------------------------------------*/

static DEVICE_START(mess_device)
{
	mess_device_config *mess_device = (mess_device_config *) device->inline_config;
	mess_device_token *token = (mess_device_token *) device->token;
	device_start_func inner_start;

	/* initialize the tag pool */
	tagpool_init(&token->tagpool);

	/* if present, invoke the start handler */
	inner_start = (device_start_func) mess_device_get_info_fct(&mess_device->io_device.devclass, MESS_DEVINFO_PTR_START);
	if (inner_start != NULL)
		(*inner_start)(device);
}



/*-------------------------------------------------
    DEVICE_STOP(mess_device) - device stop
    callback
-------------------------------------------------*/

static DEVICE_STOP(mess_device)
{
	mess_device_config *mess_device = (mess_device_config *) device->inline_config;
	mess_device_token *token = (mess_device_token *) device->token;
	device_stop_func inner_stop;

	/* if present, invoke the stop handler */
	inner_stop = (device_stop_func) mess_device_get_info_fct(&mess_device->io_device.devclass, MESS_DEVINFO_PTR_STOP);
	if (inner_stop != NULL)
		(*inner_stop)(device);

	/* tear down the tag pool */
	tagpool_exit(&token->tagpool);
}



/*-------------------------------------------------
    DEVICE_GET_NAME(mess_device) - device get name
    callback
-------------------------------------------------*/

static DEVICE_GET_NAME(mess_device)
{
	const char *name;
	const legacy_mess_device *iodev = mess_device_from_core_device(device);
	int id = iodev->index_in_device;

	/* use the cool new device string technique */
	name = mess_device_get_info_string(&iodev->devclass, MESS_DEVINFO_STR_DESCRIPTION + id);
	if (name != NULL)
	{
		snprintf(buffer, buffer_length, "%s", name);
		return buffer;
	}

	name = device_uiname(iodev->type);
	if (iodev->multiple)
	{
		/* for the average user counting starts at #1 ;-) */
		snprintf(buffer, buffer_length, "%s #%d", name, id + 1);
		name = buffer;
	}
	return name;
}



/*-------------------------------------------------
    safe_strcpy - hack
-------------------------------------------------*/

static void safe_strcpy(char *dst, const char *src)
{
	strcpy(dst, src ? src : "");
}



/*-------------------------------------------------
    DEVICE_GET_INFO(mess_device) - device get info
    callback
-------------------------------------------------*/

DEVICE_GET_INFO(mess_device)
{
	mess_device_config *mess_device = (device != NULL) ? (mess_device_config *) device->inline_config : NULL;

	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_INLINE_CONFIG_BYTES:	info->i = sizeof(mess_device_config);			break;
		case DEVINFO_INT_TOKEN_BYTES:			info->i = sizeof(mess_device_token);			break;
		case DEVINFO_INT_CLASS:					info->i = DEVICE_CLASS_OTHER;					break;
		case DEVINFO_INT_IMAGE_READABLE:		info->i = mess_device->io_device.readable;		break;
		case DEVINFO_INT_IMAGE_WRITEABLE:		info->i = mess_device->io_device.writeable;		break;
		case DEVINFO_INT_IMAGE_CREATABLE:		info->i = mess_device->io_device.creatable;		break;
		case DEVINFO_INT_IMAGE_TYPE:			info->i = mess_device_get_info_int(&mess_device->io_device.devclass, MESS_DEVINFO_INT_TYPE); break;
		case DEVINFO_INT_IMAGE_MUST_BE_LOADED:	info->i = mess_device_get_info_int(&mess_device->io_device.devclass, MESS_DEVINFO_INT_MUST_BE_LOADED); break;
		case DEVINFO_INT_IMAGE_RESET_ON_LOAD:	info->i = mess_device_get_info_int(&mess_device->io_device.devclass, MESS_DEVINFO_INT_RESET_ON_LOAD); break;
		case DEVINFO_INT_IMAGE_CREATE_OPTCOUNT:	info->i = mess_device_get_info_int(&mess_device->io_device.devclass, MESS_DEVINFO_INT_CREATE_OPTCOUNT); break;

		/* --- the following bits of info are returned as pointers to data --- */
		case DEVINFO_PTR_IMAGE_CREATE_OPTGUIDE:	info->p = mess_device_get_info_ptr(&mess_device->io_device.devclass, MESS_DEVINFO_PTR_CREATE_OPTGUIDE); break;

		/* --- the following bits of info are returned as pointers to functions --- */
		case DEVINFO_FCT_START:					info->start = DEVICE_START_NAME(mess_device);	break;
		case DEVINFO_FCT_STOP:					info->stop = DEVICE_STOP_NAME(mess_device);		break;
		case DEVINFO_FCT_RESET:					/* Nothing */									break;
		case DEVINFO_FCT_IMAGE_LOAD:			info->f = mess_device_get_info_fct(&mess_device->io_device.devclass, MESS_DEVINFO_PTR_LOAD); break;
		case DEVINFO_FCT_IMAGE_CREATE:			info->f	= mess_device_get_info_fct(&mess_device->io_device.devclass, MESS_DEVINFO_PTR_CREATE); break;
		case DEVINFO_FCT_IMAGE_UNLOAD:			info->f = mess_device_get_info_fct(&mess_device->io_device.devclass, MESS_DEVINFO_PTR_UNLOAD); break;
		case DEVINFO_FCT_IMAGE_VERIFY:			info->f = mess_device_get_info_fct(&mess_device->io_device.devclass, MESS_DEVINFO_PTR_VERIFY); break;
		case DEVINFO_FCT_DISPLAY:				info->f = mess_device_get_info_fct(&mess_device->io_device.devclass, MESS_DEVINFO_PTR_DISPLAY); break;
		case DEVINFO_FCT_IMAGE_PARTIAL_HASH:	info->f = mess_device_get_info_fct(&mess_device->io_device.devclass, MESS_DEVINFO_PTR_PARTIAL_HASH); break;
		case DEVINFO_FCT_GET_NAME:				info->f = (genf *) DEVICE_GET_NAME_NAME(mess_device);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:					strcpy(info->s, "Legacy MESS Device");					break;
		case DEVINFO_STR_FAMILY:				strcpy(info->s, "Legacy MESS Device");					break;
		case DEVINFO_STR_VERSION:				strcpy(info->s, "1.0");								break;
		case DEVINFO_STR_SOURCE_FILE:			strcpy(info->s, __FILE__);								break;
		case DEVINFO_STR_CREDITS:				strcpy(info->s, "Copyright the MESS Team");			break;
		case DEVINFO_STR_IMAGE_FILE_EXTENSIONS:	safe_strcpy(info->s, mess_device_get_info_string(&mess_device->io_device.devclass, MESS_DEVINFO_STR_FILE_EXTENSIONS)); break;
		case DEVINFO_STR_IMAGE_INSTANCE_NAME:	safe_strcpy(info->s, mess_device_get_info_string(&mess_device->io_device.devclass, MESS_DEVINFO_STR_NAME + mess_device->io_device.index_in_device)); break;
		case DEVINFO_STR_IMAGE_BRIEF_INSTANCE_NAME:	safe_strcpy(info->s, mess_device_get_info_string(&mess_device->io_device.devclass, MESS_DEVINFO_STR_SHORT_NAME + mess_device->io_device.index_in_device)); break;

		default:
			if ((state >= DEVINFO_PTR_IMAGE_CREATE_OPTSPEC) && (state < DEVINFO_PTR_IMAGE_CREATE_OPTSPEC + DEVINFO_CREATE_OPTMAX))
				info->p = mess_device_get_info_ptr(&mess_device->io_device.devclass, state - DEVINFO_PTR_IMAGE_CREATE_OPTSPEC + MESS_DEVINFO_PTR_CREATE_OPTSPEC);
			else if ((state >= DEVINFO_STR_IMAGE_CREATE_OPTNAME) && (state < DEVINFO_STR_IMAGE_CREATE_OPTNAME + DEVINFO_CREATE_OPTMAX))
				safe_strcpy(info->s, mess_device_get_info_ptr(&mess_device->io_device.devclass, state - DEVINFO_STR_IMAGE_CREATE_OPTNAME + MESS_DEVINFO_STR_CREATE_OPTNAME));
			else if ((state >= DEVINFO_STR_IMAGE_CREATE_OPTDESC) && (state < DEVINFO_STR_IMAGE_CREATE_OPTDESC + DEVINFO_CREATE_OPTMAX))
				safe_strcpy(info->s, mess_device_get_info_ptr(&mess_device->io_device.devclass, state - DEVINFO_STR_IMAGE_CREATE_OPTDESC + MESS_DEVINFO_STR_CREATE_OPTDESC));
			else if ((state >= DEVINFO_STR_IMAGE_CREATE_OPTEXTS) && (state < DEVINFO_STR_IMAGE_CREATE_OPTEXTS + DEVINFO_CREATE_OPTMAX))
				safe_strcpy(info->s, mess_device_get_info_ptr(&mess_device->io_device.devclass, state - DEVINFO_STR_IMAGE_CREATE_OPTEXTS + MESS_DEVINFO_STR_CREATE_OPTEXTS));
			break;
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

static void create_mess_device(running_machine *machine, device_config **listheadptr, device_getinfo_handler handler, const game_driver *gamedrv,
	int count_override, int *position)
{
	mess_device_class mess_devclass;
	const char *mess_tag;
	const char *mame_tag;
	const char *info_string;
	char dynamic_tag[32];
	device_config *device;
	mess_device_config *mess_device;
	int i, count;
	size_t string_buffer_pos = 0;
	device_getdispositions_func getdispositions;
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
		device = device_list_add(listheadptr, NULL, MESS_DEVICE, mame_tag, 0);
		device->machine = machine;
		mess_device = (mess_device_config *) device->inline_config;

		/* we need to copy the mess_tag into the structure */
		info_string = mess_tag;
		mess_tag = string_buffer_putstr(mess_device->string_buffer, ARRAY_LENGTH(mess_device->string_buffer), &string_buffer_pos, info_string);

		/* populate the legacy MESS device */
		mess_device->io_device.tag					= mess_tag;
		mess_device->io_device.devconfig			= device;
		mess_device->io_device.devclass				= mess_devclass;
		mess_device->io_device.type					= mess_device_get_info_int(&mess_device->io_device.devclass, MESS_DEVINFO_INT_TYPE);
		mess_device->io_device.position				= *position;
		mess_device->io_device.index_in_device		= i;

		mess_device->io_device.reset_on_load		= mess_device_get_info_int(&mess_device->io_device.devclass, MESS_DEVINFO_INT_RESET_ON_LOAD) ? 1 : 0;
		mess_device->io_device.load_at_init			= mess_device_get_info_int(&mess_device->io_device.devclass, MESS_DEVINFO_INT_LOAD_AT_INIT) ? 1 : 0;

		/* determine the dispositions */
		getdispositions = (device_getdispositions_func) mess_device_get_info_fct(&mess_device->io_device.devclass, MESS_DEVINFO_PTR_GET_DISPOSITIONS);
		if (getdispositions != NULL)
		{
			getdispositions(i, &readable, &writeable, &creatable);
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

void mess_devices_setup(running_machine *machine, machine_config *config, const game_driver *gamedrv)
{
	device_getinfo_handler handlers[64];
	int count_overrides[ARRAY_LENGTH(handlers)];
	int i, position = 0;

	memset(handlers, 0, sizeof(handlers));
	memset(count_overrides, 0, sizeof(count_overrides));

	/* call the "sysconfig" constructor, if present */
	if (gamedrv->sysconfig_ctor != NULL)
	{
		struct SystemConfigurationParamBlock params;

		memset(&params, 0, sizeof(params));
		params.device_slotcount = ARRAY_LENGTH(handlers);
		params.device_handlers = handlers;
		params.device_countoverrides = count_overrides;
		gamedrv->sysconfig_ctor(&params);
	}

	/* loop through all handlers */
	for (i = 0; handlers[i] != NULL; i++)
	{
		create_mess_device(machine, &config->devicelist, handlers[i], gamedrv, count_overrides[i], &position);
	}
}



/*-------------------------------------------------
    machine_config_alloc_with_mess_devices -
	allocate a machine_config with MESS devices
-------------------------------------------------*/

machine_config *machine_config_alloc_with_mess_devices(const game_driver *gamedrv)
{
	machine_config *config = machine_config_alloc(gamedrv->machine_config);
	mess_devices_setup(NULL/*machine?*/, config, gamedrv);
	return config;
}



/*************************************
 *
 *	Device lookup
 *
 *************************************/

static const legacy_mess_device *mess_device_from_core_device(const device_config *device)
{
	const mess_device_config *mess_device = (device != NULL) ? (const mess_device_config *) device->inline_config : NULL;
	return (mess_device != NULL) ? &mess_device->io_device : NULL;
}

/*************************************
 *
 *	Deprecated device access functions
 *
 *************************************/

int image_index_in_device(const device_config *image)
{
	const legacy_mess_device *iodev = mess_device_from_core_device(image);
	assert(iodev != NULL);
	return iodev->index_in_device;
}


const device_config *image_from_devtype_and_index(running_machine *machine, iodevice_t type, int id)
{
	const device_config *image = NULL;
	const device_config *dev;
	const legacy_mess_device *iodev;

	for (dev = device_list_first(machine->config->devicelist, MESS_DEVICE); dev != NULL; dev = device_list_next(dev, MESS_DEVICE))
	{
		iodev = mess_device_from_core_device(dev);
		if ((type == iodev->type) && (iodev->index_in_device == id))
		{
			/* if 'image' is not null, that means that there are multiple devices of this iodevice_t type */
			assert(image == NULL);

			image = dev;
			break;
		}
	}

	/* by calling this function, the caller assumes that there will be a matching device */
	assert(image != NULL);
	return image;
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

	/* Check the device struct array */
	for (i = 0; i < ARRAY_LENGTH(device_info_array); i++)
	{
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

