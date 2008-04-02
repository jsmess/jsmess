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
#include "device.h"
#include "osdmess.h"



/***************************************************************************
    CONSTANTS
***************************************************************************/

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

struct _images_private;
typedef struct _images_private images_private;


/* state constants specific for mountable images */
enum
{
	/* --- the following bits of info are returned as pointers to functions --- */
	DEVINFO_FCT_IMAGE_FIRST = DEVINFO_FCT_FIRST + 0x7000,
	DEVINFO_FCT_IMAGE_LOAD,										/* R/O: device_load_handler */
	DEVINFO_FCT_IMAGE_CREATE,									/* R/O: device_create_handler */
	DEVINFO_FCT_IMAGE_UNLOAD,									/* R/O: device_unload_handler */
	DEVINFO_FCT_DISPLAY,										/* R/O: device_display_func */
	DEVINFO_FCT_IMAGE_LAST = DEVINFO_FCT_FIRST + 0x0fff
};




/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* ----- core implementation ----- */

/* core initialization */
int image_init(running_machine *machine);


/* ----- image device enumeration ----- */

/* return the first device in the list that supports images */
const device_config *image_device_first(const machine_config *config);

/* return the next device in the list that supports images */
const device_config *image_device_next(const device_config *prevdevice);

/* counts the number of devices that support images */
int image_device_count(const machine_config *config);



/****************************************************************************
  Device loading and unloading functions

  The UI can call image_load and image_unload to associate and disassociate
  with disk images on disk.  In fact, devices that unmount on their own (like
  Mac floppy drives) may call this from within a driver.
****************************************************************************/

/* can be called by front ends */
int image_load(const device_config *img, const char *name);
int image_create(const device_config *img, const char *name, int create_format, option_resolution *create_args);
void image_unload(const device_config *img);

/* used to retrieve error information during image loading */
const char *image_error(const device_config *img);

/* used for driver init and machine init */
void image_unload_all(int ispreload);

/* used to set the error that occured during image loading */
void image_seterror(const device_config *img, image_error_t err, const char *message);



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
int image_feof(const device_config *image);

void *image_ptr(const device_config *image);



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

const char *image_longname(const device_config *img);
const char *image_manufacturer(const device_config *img);
const char *image_year(const device_config *img);
const char *image_playable(const device_config *img);
const char *image_extrainfo(const device_config *img);



/****************************************************************************
  Battery functions

  These functions provide transparent access to battery-backed RAM on an
  image; typically for cartridges.
****************************************************************************/

void image_battery_load(const device_config *img, void *buffer, int length);
void image_battery_save(const device_config *img, const void *buffer, int length);



/****************************************************************************
  Indexing functions

  These provide various ways of indexing images
****************************************************************************/

int image_absolute_index(const device_config *image);
const device_config *image_from_absolute_index(int absolute_index);



/****************************************************************************
  Deprecated functions

  The usage of these functions is to be phased out.  The first group because
  they reflect the outdated fixed relationship between devices and their
  type/id.
****************************************************************************/

int image_index_in_device(const device_config *img);
const device_config *image_from_device(const struct IODevice *dev);
const device_config *image_from_devtag_and_index(const char *devtag, int id);

/* deprecated; as there can be multiple devices of a certain type */
iodevice_t image_devtype(const device_config *img);
const device_config *image_from_devtype_and_index(iodevice_t type, int id);




/****************************************************************************
  Macros for declaring device callbacks
****************************************************************************/

#define DEVICE_IMAGE_LOAD_NAME(name)	device_load_##name
#define DEVICE_IMAGE_LOAD(name)			int DEVICE_IMAGE_LOAD_NAME(name)(const device_config *image)

#define DEVICE_IMAGE_CREATE_NAME(name)	device_create_##name
#define DEVICE_IMAGE_CREATE(name)		int DEVICE_IMAGE_CREATE_NAME(name)(const device_config *image, int create_format, option_resolution *create_args)

#define DEVICE_IMAGE_UNLOAD_NAME(name)	device_unload_##name
#define DEVICE_IMAGE_UNLOAD(name)		void DEVICE_IMAGE_UNLOAD_NAME(name)(const device_config *image)

#endif /* __IMAGE_H__ */
