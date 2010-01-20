/****************************************************************************

    image.h

    Code for handling devices/software images

****************************************************************************/

#ifndef __IMAGE_H__
#define __IMAGE_H__

#include <stdlib.h>

#include "fileio.h"
#include "utils.h"
#include "opresolv.h"
#include "osd/osdmess.h"
#include "softlist.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define DEVINFO_CREATE_OPTMAX   32



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef int (*device_image_load_func)(const device_config *image);
typedef int (*device_image_create_func)(const device_config *image, int format_type, option_resolution *format_options);
typedef void (*device_image_unload_func)(const device_config *image);
typedef int (*device_image_verify_func)(const UINT8 *buf, size_t size);
typedef void (*device_display_func)(const device_config *image);
typedef void (*device_image_partialhash_func)(char *, const unsigned char *, unsigned long, unsigned int);
typedef const char *(*device_get_name_func)(const device_config *device, char *buffer, size_t buffer_length);
typedef void (*device_get_image_devices_func)(const device_config *device, device_list *devlist);

typedef enum
{
    /* List of all supported devices.  Refer to the device by these names only */
    IO_UNKNOWN,
    IO_CARTSLOT,    /*  1 - Cartridge Port, as found on most console and on some computers */
    IO_FLOPPY,      /*  2 - Floppy Disk unit */
    IO_HARDDISK,    /*  3 - Hard Disk unit */
    IO_CYLINDER,    /*  4 - Magnetically-Coated Cylinder */
    IO_CASSETTE,    /*  5 - Cassette Recorder (common on early home computers) */
    IO_PUNCHCARD,   /*  6 - Card Puncher/Reader */
    IO_PUNCHTAPE,   /*  7 - Tape Puncher/Reader (reels instead of punchcards) */
    IO_PRINTER,     /*  8 - Printer device */
    IO_SERIAL,      /*  9 - Generic Serial Port */
    IO_PARALLEL,    /* 10 - Generic Parallel Port */
    IO_SNAPSHOT,    /* 11 - Complete 'snapshot' of the state of the computer */
    IO_QUICKLOAD,   /* 12 - Allow to load program/data into memory, without matching any actual device */
    IO_MEMCARD,     /* 13 - Memory card */
    IO_CDROM,       /* 14 - optical CD-ROM disc */
	IO_MAGTAPE,     /* 15 - Magentic tape */
    IO_COUNT        /* 16 - Total Number of IO_devices for searching */
} iodevice_t;



typedef enum
{
    IMAGE_ERROR_SUCCESS,
    IMAGE_ERROR_INTERNAL,
    IMAGE_ERROR_UNSUPPORTED,
    IMAGE_ERROR_OUTOFMEMORY,
    IMAGE_ERROR_FILENOTFOUND,
    IMAGE_ERROR_INVALIDIMAGE,
    IMAGE_ERROR_ALREADYOPEN,
    IMAGE_ERROR_UNSPECIFIED
} image_error_t;

typedef struct _image_device_info image_device_info;
struct _image_device_info
{
    iodevice_t type : 8;
    unsigned int readable : 1;
    unsigned int writeable : 1;
    unsigned int creatable : 1;
    unsigned int must_be_loaded : 1;
    unsigned int reset_on_load : 1;
    unsigned int has_partial_hash : 1;
    char name[62];
    char file_extensions[256];
    char instance_name[32];
    char brief_instance_name[16];
};

typedef struct _image_device_format image_device_format;
struct _image_device_format
{
    image_device_format *next;
    int index;
    const char *name;
    const char *description;
    const char *extensions;
    const char *optspec;
};

struct _images_private;
typedef struct _images_private images_private;


/* state constants specific for mountable images */
enum
{
    /* --- the following bits of info are returned as integers --- */
    DEVINFO_INT_IMAGE_FIRST = DEVINFO_INT_FIRST + 0x7000,
    DEVINFO_INT_IMAGE_TYPE,
    DEVINFO_INT_IMAGE_READABLE,
    DEVINFO_INT_IMAGE_WRITEABLE,
    DEVINFO_INT_IMAGE_CREATABLE,
    DEVINFO_INT_IMAGE_MUST_BE_LOADED,
    DEVINFO_INT_IMAGE_RESET_ON_LOAD,
    DEVINFO_INT_IMAGE_CREATE_OPTCOUNT,
    DEVINFO_INT_IMAGE_LAST = DEVINFO_INT_IMAGE_FIRST + 0x0fff,

    /* --- the following bits of info are returned as pointers --- */
    DEVINFO_PTR_IMAGE_FIRST = DEVINFO_PTR_FIRST + 0x7000,
    DEVINFO_PTR_IMAGE_CREATE_OPTGUIDE,
    DEVINFO_PTR_IMAGE_CREATE_OPTSPEC,

    /* --- the following bits of info are returned as pointers to functions --- */
    DEVINFO_FCT_IMAGE_FIRST = DEVINFO_FCT_FIRST + 0x7000,
    DEVINFO_FCT_IMAGE_LOAD,                                     /* R/O: device_image_load_func */
    DEVINFO_FCT_IMAGE_CREATE,                                   /* R/O: device_image_create_func */
    DEVINFO_FCT_IMAGE_UNLOAD,                                   /* R/O: device_image_unload_func */
    DEVINFO_FCT_IMAGE_VERIFY,                                   /* R/O: device_image_verify_func */
    DEVINFO_FCT_DISPLAY,                                        /* R/O: device_display_func */
    DEVINFO_FCT_IMAGE_PARTIAL_HASH,                             /* R/O: device_image_partialhash_func */
    DEVINFO_FCT_GET_NAME,                                       /* R/O: device_get_name_func */
    DEVINFO_FCT_GET_IMAGE_DEVICES,                              /* R/O: device_get_image_devices_func */
    DEVINFO_FCT_IMAGE_LAST = DEVINFO_FCT_FIRST + 0x0fff,

    /* --- the following bits of info are returned as NULL-terminated strings --- */
    DEVINFO_STR_IMAGE_FIRST = DEVINFO_STR_FIRST + 0x7000,
    DEVINFO_STR_IMAGE_FILE_EXTENSIONS,
    DEVINFO_STR_IMAGE_INSTANCE_NAME,
    DEVINFO_STR_IMAGE_BRIEF_INSTANCE_NAME,
    DEVINFO_STR_IMAGE_CREATE_OPTNAME,
    DEVINFO_STR_IMAGE_CREATE_OPTDESC = DEVINFO_STR_IMAGE_CREATE_OPTNAME + DEVINFO_CREATE_OPTMAX,
    DEVINFO_STR_IMAGE_CREATE_OPTEXTS = DEVINFO_STR_IMAGE_CREATE_OPTDESC + DEVINFO_CREATE_OPTMAX,
	DEVINFO_STR_SOFTWARE_LIST,
    DEVINFO_STR_IMAGE_LAST = DEVINFO_STR_IMAGE_FIRST + 0x0fff
};




/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* ----- core implementation ----- */

/* core initialization */
void image_init(running_machine *machine);
void image_unload_all(running_machine *machine);


/* ----- image device enumeration ----- */

/* return the first device in the list that supports images */
const device_config *image_device_first(const machine_config *config);

/* return the next device in the list that supports images */
const device_config *image_device_next(const device_config *prevdevice);

/* counts the number of devices that support images */
int image_device_count(const machine_config *config);

/* ----- analysis ----- */

/* returns info on a device - can be called by front end code */
image_device_info image_device_getinfo(const machine_config *config, const device_config *device);

/* checks to see if a particular devices uses a certain file extension */
int image_device_uses_file_extension(const device_config *device, const char *file_extension);

/* compute a hash, using this device's partial hash if appropriate */
void image_device_compute_hash(char *dest, const device_config *device,
    const void *data, size_t length, unsigned int functions);

/* ----- creation formats ----- */

/* accesses the creation option guide */
const option_guide *image_device_get_creation_option_guide(const device_config *device);

/* accesses the image formats available for image creation */
const image_device_format *image_device_get_creatable_formats(const device_config *device);

/* accesses a specific image format available for image creation by index */
const image_device_format *image_device_get_indexed_creatable_format(const device_config *device, int index);

/* accesses a specific image format available for image creation by name */
const image_device_format *image_device_get_named_creatable_format(const device_config *device, const char *format_name);



/****************************************************************************
  Device loading and unloading functions

  The UI can call image_load and image_unload to associate and disassociate
  with disk images on disk.  In fact, devices that unmount on their own (like
  Mac floppy drives) may call this from within a driver.
****************************************************************************/

/* can be called by front ends */
int image_load(const device_config *img, const char *name);
int image_create(const device_config *img, const char *name, const image_device_format *create_format, option_resolution *create_args);
void image_unload(const device_config *img);

/* special call - only use from core */
int image_finish_load(const device_config *device);

/* used to retrieve error information during image loading */
const char *image_error(const device_config *img);

/* used to set the error that occured during image loading */
void image_seterror(const device_config *img, image_error_t err, const char *message);

/* used to display a message while loading */
void image_message(const device_config *device, const char *format, ...) ATTR_PRINTF(2,3);



/****************************************************************************
  Accessor functions

  These provide information about the device; and about the mounted image
****************************************************************************/

int image_exists(const device_config *image);
int image_slotexists(const device_config *image);

core_file *image_core_file(const device_config *image);
const char *image_typename_id(const device_config *image);
const char *image_filename(const device_config *image);
const char *image_basename(const device_config *image);
const char *image_basename_noext(const device_config *image);
const char *image_filetype(const device_config *image);
const char *image_filedir(const device_config *image);
const char *image_working_directory(const device_config *image);
void image_set_working_directory(const device_config *image, const char *working_directory);
UINT64 image_length(const device_config *image);
const char *image_hash(const device_config *image);
UINT32 image_crc(const device_config *image);

int image_is_writable(const device_config *image);
int image_has_been_created(const device_config *image);
void image_make_readonly(const device_config *image);

UINT32 image_fread(const device_config *image, void *buffer, UINT32 length);
UINT32 image_fwrite(const device_config *image, const void *buffer, UINT32 length);
int image_fseek(const device_config *image, INT64 offset, int whence);
UINT64 image_ftell(const device_config *image);
int image_fgetc(const device_config *image);
char *image_fgets(const device_config *image, char *s, UINT32 length);
int image_feof(const device_config *image);

void *image_ptr(const device_config *image);


UINT8 *image_get_software_region(const device_config *image, const char *tag);
UINT32 image_get_software_region_length(const device_config *image, const char *tag);
const software_entry *image_software_entry(const device_config *image);



/****************************************************************************
  Memory allocators

  These allow memory to be allocated for the lifetime of a mounted image.
  If these (and the above accessors) are used well enough, they should be
  able to eliminate the need for a unload function.
****************************************************************************/

void *image_malloc(const device_config *img, size_t size) ATTR_MALLOC;
char *image_strdup(const device_config *img, const char *src) ATTR_MALLOC;
void *image_realloc(const device_config *img, void *ptr, size_t size);
void image_freeptr(const device_config *img, void *ptr);



/****************************************************************************
  CRC Accessor functions

  When an image is mounted; these functions provide access to the information
  pertaining to that image in the CRC database
****************************************************************************/

const char *image_longname(const device_config *device);
const char *image_manufacturer(const device_config *device);
const char *image_year(const device_config *device);
const char *image_playable(const device_config *device);
const char *image_pcb(const device_config *device);
const char *image_extrainfo(const device_config *device);



/****************************************************************************
  Battery functions

  These functions provide transparent access to battery-backed RAM on an
  image; typically for cartridges.
****************************************************************************/

void image_battery_load_by_name(const char *filename, void *buffer, int length);
void image_battery_save_by_name(const char *filename, const void *buffer, int length);

void image_battery_load(const device_config *img, void *buffer, int length);
void image_battery_save(const device_config *img, const void *buffer, int length);


/****************************************************************************
  Indexing functions

  These provide various ways of indexing images
****************************************************************************/

int image_absolute_index(const device_config *image);
const device_config *image_from_absolute_index(running_machine *machine, int absolute_index);



/****************************************************************************
  Macros for declaring device callbacks
****************************************************************************/

#define DEVICE_IMAGE_LOAD_NAME(name)        device_load_##name
#define DEVICE_IMAGE_LOAD(name)             int DEVICE_IMAGE_LOAD_NAME(name)(const device_config *image)

#define DEVICE_IMAGE_CREATE_NAME(name)      device_create_##name
#define DEVICE_IMAGE_CREATE(name)           int DEVICE_IMAGE_CREATE_NAME(name)(const device_config *image, int create_format, option_resolution *create_args)

#define DEVICE_IMAGE_UNLOAD_NAME(name)      device_unload_##name
#define DEVICE_IMAGE_UNLOAD(name)           void DEVICE_IMAGE_UNLOAD_NAME(name)(const device_config *image)

#define DEVICE_GET_NAME_NAME(name)          device_get_name_##name
#define DEVICE_GET_NAME(name)               const char *DEVICE_GET_NAME_NAME(name)(const device_config *device, char *buffer, size_t buffer_length)

#define DEVICE_GET_IMAGE_DEVICES_NAME(name) device_get_image_devices_##name
#define DEVICE_GET_IMAGE_DEVICES(name)      void DEVICE_GET_IMAGE_DEVICES_NAME(name)(const device_config *device, device_list *devlist)

#endif /* __IMAGE_H__ */
