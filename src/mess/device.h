/***************************************************************************

	device.h

	Definitions and manipulations for device structures

***************************************************************************/

#ifndef DEVICE_H
#define DEVICE_H

#include "mamecore.h"
#include "osdmess.h"
#include "opresolv.h"

/*************************************
 *
 *  Device information constants
 *
 *************************************/

#define MAX_DEV_INSTANCES	5

enum
{
	MESS_DEVINFO_CREATE_OPTMAX = 32,

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
	MESS_DEVINFO_INT_NOT_WORKING,
	MESS_DEVINFO_INT_CREATE_OPTCOUNT,

	MESS_DEVINFO_INT_DEV_SPECIFIC = 0x08000,					/* R/W: Device-specific values start here */

	/* --- the following bits of info are returned as pointers to data or functions --- */
	MESS_DEVINFO_PTR_FIRST = 0x10000,

	MESS_DEVINFO_PTR_INIT,
	MESS_DEVINFO_PTR_EXIT,
	MESS_DEVINFO_PTR_LOAD,
	MESS_DEVINFO_PTR_UNLOAD,
	MESS_DEVINFO_PTR_CREATE,
	MESS_DEVINFO_PTR_STATUS,
	MESS_DEVINFO_PTR_DISPLAY,
	MESS_DEVINFO_PTR_PARTIAL_HASH,
	MESS_DEVINFO_PTR_VERIFY,
	MESS_DEVINFO_PTR_GET_DISPOSITIONS,
	MESS_DEVINFO_PTR_CREATE_OPTGUIDE,
	MESS_DEVINFO_PTR_CREATE_OPTSPEC,
	MESS_DEVINFO_PTR_VALIDITY_CHECK = MESS_DEVINFO_PTR_CREATE_OPTSPEC + MESS_DEVINFO_CREATE_OPTMAX,	/* R/O: int (*validity_check)(const device_class *devclass) */

	MESS_DEVINFO_PTR_DEV_SPECIFIC = 0x18000,					/* R/W: Device-specific values start here */

	/* --- the following bits of info are returned as NULL-terminated strings --- */
	MESS_DEVINFO_STR_FIRST = 0x20000,

	MESS_DEVINFO_STR_DEV_FILE,
	MESS_DEVINFO_STR_DEV_TAG,
	MESS_DEVINFO_STR_FILE_EXTENSIONS,

	MESS_DEVINFO_STR_CREATE_OPTNAME,
	MESS_DEVINFO_STR_CREATE_OPTDESC = MESS_DEVINFO_STR_CREATE_OPTNAME + MESS_DEVINFO_CREATE_OPTMAX,
	MESS_DEVINFO_STR_CREATE_OPTEXTS = MESS_DEVINFO_STR_CREATE_OPTDESC + MESS_DEVINFO_CREATE_OPTMAX,

	MESS_DEVINFO_STR_NAME = MESS_DEVINFO_STR_CREATE_OPTEXTS + MESS_DEVINFO_CREATE_OPTMAX,
	MESS_DEVINFO_STR_SHORT_NAME = MESS_DEVINFO_STR_NAME + MESS_DEVINFO_CREATE_OPTMAX,
	MESS_DEVINFO_STR_DESCRIPTION = MESS_DEVINFO_STR_SHORT_NAME + MAX_DEV_INSTANCES,

	MESS_DEVINFO_STR_DEV_SPECIFIC = 0x28000,					/* R/W: Device-specific values start here */

	/* --- the following bits of info are returned as doubles --- */
	MESS_DEVINFO_FLOAT_FIRST = 0x30000,

	MESS_DEVINFO_FLOAT_DEV_SPECIFIC = 0x38000				/* R/W: Device-specific values start here */
};


struct IODevice;

typedef int (*device_init_handler)(mess_image *image);
typedef void (*device_exit_handler)(mess_image *image);
typedef int (*device_load_handler)(mess_image *image);
typedef int (*device_create_handler)(mess_image *image, int format_type, option_resolution *format_options);
typedef void (*device_unload_handler)(mess_image *image);
typedef int (*device_verify_handler)(const UINT8 *buf, size_t size);
typedef void (*device_partialhash_handler)(char *, const unsigned char *, unsigned long, unsigned int);
typedef void (*device_getdispositions_handler)(const struct IODevice *dev, int id,
	unsigned int *readable, unsigned int *writeable, unsigned int *creatable);
typedef void (*device_display_handler)(mess_image *image);
typedef const char *(*device_getname_handler)(const struct IODevice *dev, int id, char *buf, size_t bufsize);

struct _device_class;

union devinfo
{
	INT64	i;											/* generic integers */
	void *	p;											/* generic pointers */
	genf *  f;											/* generic function pointers */
	char *s;											/* generic strings */
	double	d;											/* generic floating points */

	device_init_handler init;
	device_exit_handler exit;
	device_load_handler load;
	device_create_handler create;
	device_unload_handler unload;

	device_partialhash_handler partialhash;
	device_verify_handler imgverify;
	device_getdispositions_handler getdispositions;

	device_display_handler display;
	device_getname_handler name;

	int (*validity_check)(const struct _device_class *devclass);
};

typedef void (*device_getinfo_handler)(const struct _device_class *devclass, UINT32 state, union devinfo *info);

typedef struct _device_class
{
	device_getinfo_handler get_info;
	const game_driver *gamedrv;
} device_class;



/*************************************
 *
 *   Device accessors
 *
 *************************************/

INLINE INT64 mess_device_get_info_int(const device_class *devclass, UINT32 state)
{
	union devinfo info;
	info.i = 0;
	devclass->get_info(devclass, state, &info);
	return info.i;
}



INLINE void *mess_device_get_info_ptr(const device_class *devclass, UINT32 state)
{
	union devinfo info;
	info.p = NULL;
	devclass->get_info(devclass, state, &info);
	return info.p;
}

INLINE genf *mess_device_get_info_fct(const device_class *devclass, UINT32 state)
{
	union devinfo info;
	info.f = NULL;
	devclass->get_info(devclass, state, &info);
	return info.f;
}

INLINE const char *mess_device_get_info_string(const device_class *devclass, UINT32 state)
{
	union devinfo info;
	info.s = NULL;
	devclass->get_info(devclass, state, &info);
	return info.s;
}

INLINE double mess_device_get_info_double(const device_class *devclass, UINT32 state)
{
	union devinfo info;
	info.d = 0.0;
	devclass->get_info(devclass, state, &info);
	return info.d;
}

INLINE char *device_temp_str(void)
{
	extern char *cpuintrf_temp_str(void);
	return cpuintrf_temp_str();
}

const char *device_instancename(const device_class *devclass, int id);
const char *device_briefinstancename(const device_class *devclass, int id);



/*************************************
 *
 *  Other
 *
 *************************************/

typedef enum
{
	/* List of all supported devices.  Refer to the device by these names only */
	IO_CARTSLOT,	/*  0 - Cartridge Port, as found on most console and on some computers */
	IO_FLOPPY,		/*  1 - Floppy Disk unit */
	IO_HARDDISK,	/*  2 - Hard Disk unit */
	IO_CYLINDER,	/*  3 - Magnetically-Coated Cylinder */
	IO_CASSETTE,	/*  4 - Cassette Recorder (common on early home computers) */
	IO_PUNCHCARD,	/*  5 - Card Puncher/Reader */
	IO_PUNCHTAPE,	/*  6 - Tape Puncher/Reader (reels instead of punchcards) */
	IO_PRINTER,		/*  7 - Printer device */
	IO_SERIAL,		/*  8 - Generic Serial Port */
	IO_PARALLEL,    /*  9 - Generic Parallel Port */
	IO_SNAPSHOT,	/* 10 - Complete 'snapshot' of the state of the computer */
	IO_QUICKLOAD,	/* 11 - Allow to load program/data into memory, without matching any actual device */
	IO_MEMCARD,		/* 12 - Memory card */
	IO_CDROM,		/* 13 - optical CD-ROM disc */
	IO_COUNT		/* 14 - Total Number of IO_devices for searching */
} iodevice_t;



struct CreateImageOptions
{
	const char *name;
	const char *description;
	const char *extensions;
	const char *optspec;
};

struct IODevice
{
	device_class devclass;

	/* the basics */
	const char *tag;
	iodevice_t type;
	int position;
	int count;
	const char *file_extensions;

	/* open dispositions */
	unsigned int readable : 1;
	unsigned int writeable : 1;
	unsigned int creatable : 1;

	/* miscellaneous flags */
	unsigned int reset_on_load : 1;
	unsigned int must_be_loaded : 1;
	unsigned int load_at_init : 1;
	unsigned int not_working : 1;

	/* image handling callbacks */
	device_init_handler init;
	device_exit_handler exit;
	device_load_handler load;
	device_create_handler create;
	device_unload_handler unload;
	int (*imgverify)(const UINT8 *buf, size_t size);
	device_partialhash_handler partialhash;
	void (*getdispositions)(const struct IODevice *dev, int id,
		unsigned int *readable, unsigned int *writeable, unsigned int *creatable);

	/* cosmetic/UI callbacks */
	void (*display)(mess_image *img);
	const char *(*name)(const struct IODevice *dev, int id, char *buf, size_t bufsize);

	/* image creation options */
	const struct OptionGuide *createimage_optguide;
	struct CreateImageOptions *createimage_options;
};


/* device naming */
const char *device_uiname(iodevice_t devtype);
const char *device_typename(iodevice_t type);
const char *device_brieftypename(iodevice_t type);
int device_typeid(const char *name);

/* device allocation */
const struct IODevice *devices_allocate(const game_driver *gamedrv);
void devices_free(const struct IODevice *devices);

/* device lookup */
const struct IODevice *device_find_tag(const struct IODevice *devices, const char *tag);
int device_count_tag(const struct IODevice *devices, const char *tag);

/* device lookup; both of these function assume only one of each type of device */
const struct IODevice *device_find(const struct IODevice *devices, iodevice_t type);
int device_count(iodevice_t type);

/* diagnostics */
int device_valididtychecks(void);

#endif /* DEVICE_H */
