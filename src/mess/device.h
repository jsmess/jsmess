/***************************************************************************

    device.h

    Definitions and manipulations for device structures

***************************************************************************/

#ifndef DEVICE_H
#define DEVICE_H

// MAME headers
#include "mamecore.h"
#include "devintrf.h"

// MESS headers
#include "osdmess.h"
#include "opresolv.h"
#include "image.h"


/*************************************
 *
 *  Device information constants
 *
 *************************************/

#define MESS_DEVICE		DEVICE_GET_INFO_NAME(mess_device)

#define MAX_DEV_INSTANCES	5

enum
{
	/* --- the following bits of info are returned as 64-bit signed integers --- */
	MESS_DEVINFO_INT_FIRST = 0x00000,

	MESS_DEVINFO_INT_TYPE,
	MESS_DEVINFO_INT_READABLE,
	MESS_DEVINFO_INT_WRITEABLE,
	MESS_DEVINFO_INT_CREATABLE,
	MESS_DEVINFO_INT_COUNT,
	MESS_DEVINFO_INT_MUST_BE_LOADED,
	MESS_DEVINFO_INT_RESET_ON_LOAD,
	MESS_DEVINFO_INT_LOAD_AT_INIT,
	MESS_DEVINFO_INT_CREATE_OPTCOUNT,

	MESS_DEVINFO_INT_DEV_SPECIFIC = 0x08000,					/* R/W: Device-specific values start here */

	/* --- the following bits of info are returned as pointers to data or functions --- */
	MESS_DEVINFO_PTR_FIRST = 0x10000,

	MESS_DEVINFO_PTR_START,
	MESS_DEVINFO_PTR_STOP,
	MESS_DEVINFO_PTR_LOAD,
	MESS_DEVINFO_PTR_UNLOAD,
	MESS_DEVINFO_PTR_CREATE,
	MESS_DEVINFO_PTR_DISPLAY,
	MESS_DEVINFO_PTR_PARTIAL_HASH,
	MESS_DEVINFO_PTR_VERIFY,
	MESS_DEVINFO_PTR_GET_DISPOSITIONS,
	MESS_DEVINFO_PTR_CREATE_OPTGUIDE,
	MESS_DEVINFO_PTR_CREATE_OPTSPEC,
	MESS_DEVINFO_PTR_VALIDITY_CHECK = MESS_DEVINFO_PTR_CREATE_OPTSPEC + DEVINFO_CREATE_OPTMAX,	/* R/O: int (*validity_check)(const mess_device_class *devclass) */

	MESS_DEVINFO_PTR_DEV_SPECIFIC = 0x18000,					/* R/W: Device-specific values start here */

	/* --- the following bits of info are returned as NULL-terminated strings --- */
	MESS_DEVINFO_STR_FIRST = 0x20000,

	MESS_DEVINFO_STR_DEV_FILE,
	MESS_DEVINFO_STR_DEV_TAG,
	MESS_DEVINFO_STR_FILE_EXTENSIONS,

	MESS_DEVINFO_STR_CREATE_OPTNAME,
	MESS_DEVINFO_STR_CREATE_OPTDESC = MESS_DEVINFO_STR_CREATE_OPTNAME + DEVINFO_CREATE_OPTMAX,
	MESS_DEVINFO_STR_CREATE_OPTEXTS = MESS_DEVINFO_STR_CREATE_OPTDESC + DEVINFO_CREATE_OPTMAX,

	MESS_DEVINFO_STR_NAME = MESS_DEVINFO_STR_CREATE_OPTEXTS + DEVINFO_CREATE_OPTMAX,
	MESS_DEVINFO_STR_SHORT_NAME = MESS_DEVINFO_STR_NAME + DEVINFO_CREATE_OPTMAX,
	MESS_DEVINFO_STR_DESCRIPTION = MESS_DEVINFO_STR_SHORT_NAME + MAX_DEV_INSTANCES,

	MESS_DEVINFO_STR_DEV_SPECIFIC = 0x28000,					/* R/W: Device-specific values start here */
};

#define TEMP_STRING_POOL_ENTRIES 16
static char temp_string_pool[TEMP_STRING_POOL_ENTRIES][256];
static int temp_string_pool_index;

typedef void (*device_getdispositions_func)(int id, unsigned int *readable, unsigned int *writeable, unsigned int *creatable);

struct _mess_device_class;
struct _machine_config;

union devinfo
{
	INT64	i;											/* generic integers */
	void *	p;											/* generic pointers */
	genf *  f;											/* generic function pointers */
	char *s;											/* generic strings */
	double	d;											/* generic floating points */

	device_start_func start;
	device_stop_func stop;
	device_image_load_func load;
	device_image_create_func create;
	device_image_unload_func unload;

	device_image_partialhash_func partialhash;
	device_image_verify_func imgverify;
	device_getdispositions_func getdispositions;

	device_display_func display;

	int (*validity_check)(const struct _mess_device_class *devclass);
};

typedef void (*device_getinfo_handler)(const struct _mess_device_class *devclass, UINT32 state, union devinfo *info);

typedef struct _mess_device_class
{
	device_getinfo_handler get_info;
	const game_driver *gamedrv;
} mess_device_class;



/*************************************
 *
 *   Device accessors
 *
 *************************************/

INLINE INT64 mess_device_get_info_int(const mess_device_class *devclass, UINT32 state)
{
	union devinfo info;
	info.i = 0;
	devclass->get_info(devclass, state, &info);
	return info.i;
}



INLINE void *mess_device_get_info_ptr(const mess_device_class *devclass, UINT32 state)
{
	union devinfo info;
	info.p = NULL;
	devclass->get_info(devclass, state, &info);
	return info.p;
}

INLINE genf *mess_device_get_info_fct(const mess_device_class *devclass, UINT32 state)
{
	union devinfo info;
	info.f = NULL;
	devclass->get_info(devclass, state, &info);
	return info.f;
}

INLINE const char *mess_device_get_info_string(const mess_device_class *devclass, UINT32 state)
{
	union devinfo info;
	info.s = NULL;
	devclass->get_info(devclass, state, &info);
	return info.s;
}

INLINE char *device_temp_str(void)
{
	char *string = &temp_string_pool[temp_string_pool_index++ % TEMP_STRING_POOL_ENTRIES][0];
	string[0] = 0;
	return string;
}



/*************************************
 *
 *  Other
 *
 *************************************/

/* interoperability with MAME devices */
DEVICE_GET_INFO(mess_device);
struct _machine_config *machine_config_alloc_with_mess_devices(const game_driver *gamedrv);

/* device naming */
const char *device_typename(iodevice_t type);
const char *device_brieftypename(iodevice_t type);
iodevice_t device_typeid(const char *name);

/* device allocation */
void mess_devices_setup(running_machine *machine, machine_config *config, const game_driver *gamedrv);

/* deprecated device access functions */
int image_index_in_device(const device_config *device);

/* deprecated device access functions that assume one device of any given type */
const device_config *image_from_devtype_and_index(running_machine *machine, iodevice_t type, int id);

/* diagnostics */
int device_valididtychecks(void);

#endif /* DEVICE_H */
