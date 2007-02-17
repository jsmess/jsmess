/****************************************************************************

	image.h

	Code for handling devices/software images

****************************************************************************/

#ifndef IMAGE_H
#define IMAGE_H

#include <stdlib.h>
#include "fileio.h"
#include "utils.h"
#include "opresolv.h"
#include "driver.h"

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



/****************************************************************************
  A mess_image pointer represents one device entry; this could be an instance
  of a floppy drive or a printer.  The defining property is that either at
  startup or at runtime, it can become associated with a file on the hosting
  machine (for the examples above, a disk image or printer output file
  repsectively).  In fact, mess_image might be better renamed mess_device.

  MESS drivers declare what device types each system had and how many.  For
  each of these, a mess_image is allocated.  Devices can have init functions
  that allow the devices to set up any relevant internal data structures.
  It is recommended that devices use the tag allocation system described
  below.

  There are also various other accessors for other information about a
  device, from whether it is mounted or not, or information stored in a CRC
  file.
****************************************************************************/

/* not to be called by anything other than core */
int image_init(void);



/****************************************************************************
  Device loading and unloading functions

  The UI can call image_load and image_unload to associate and disassociate
  with disk images on disk.  In fact, devices that unmount on their own (like
  Mac floppy drives) may call this from within a driver.
****************************************************************************/

/* can be called by front ends */
int image_load(mess_image *img, const char *name);
int image_create(mess_image *img, const char *name, int create_format, option_resolution *create_args);
void image_unload(mess_image *img);

/* used to retrieve error information during image loading */
const char *image_error(mess_image *img);

/* used for driver init and machine init */
int image_load_all(const game_driver *gamedrv, int ispreload);
void image_unload_all(int ispreload);

/* used to set the error that occured during image loading */
void image_seterror(mess_image *img, image_error_t err, const char *message);



/****************************************************************************
  Tag management functions.
  
  When devices have private data structures that need to be associated with a
  device, it is recommended that image_alloctag() be called in the device
  init function.  If the allocation succeeds, then a pointer will be returned
  to a block of memory of the specified size that will last for the lifetime
  of the emulation.  This pointer can be retrieved with image_lookuptag().

  Note that since image_lookuptag() is used to index preallocated blocks of
  memory, image_lookuptag() cannot fail legally.  In fact, an assert will be
  raised if this happens
****************************************************************************/

void *image_alloctag(mess_image *img, const char *tag, size_t size);
void *image_lookuptag(mess_image *img, const char *tag);



/****************************************************************************
  Accessor functions

  These provide information about the device; and about the mounted image
****************************************************************************/

const struct IODevice *image_device(mess_image *image);
int image_exists(mess_image *image);
int image_slotexists(mess_image *image);

const char *image_typename_id(mess_image *image);
const char *image_filename(mess_image *image);
const char *image_basename(mess_image *image);
const char *image_basename_noext(mess_image *image);
const char *image_filetype(mess_image *image);
const char *image_filedir(mess_image *image);
UINT64 image_length(mess_image *image);
const char *image_hash(mess_image *image);
UINT32 image_crc(mess_image *image);

int image_is_writable(mess_image *image);
int image_has_been_created(mess_image *image);
int image_get_open_mode(mess_image *image);
void image_make_readonly(mess_image *image);

UINT32 image_fread(mess_image *image, void *buffer, UINT32 length);
UINT32 image_fwrite(mess_image *image, const void *buffer, UINT32 length);
int image_fseek(mess_image *image, INT64 offset, int whence);
UINT64 image_ftell(mess_image *image);
int image_fgetc(mess_image *image);
int image_feof(mess_image *image);

void *image_ptr(mess_image *image);



/****************************************************************************
  Memory allocators

  These allow memory to be allocated for the lifetime of a mounted image.
  If these (and the above accessors) are used well enough, they should be
  able to eliminate the need for a unload function.
****************************************************************************/

void *image_malloc(mess_image *img, size_t size) ATTR_MALLOC;
char *image_strdup(mess_image *img, const char *src) ATTR_MALLOC;
void *image_realloc(mess_image *img, void *ptr, size_t size);
void image_freeptr(mess_image *img, void *ptr);



/****************************************************************************
  CRC Accessor functions

  When an image is mounted; these functions provide access to the information
  pertaining to that image in the CRC database
****************************************************************************/

const char *image_longname(mess_image *img);
const char *image_manufacturer(mess_image *img);
const char *image_year(mess_image *img);
const char *image_playable(mess_image *img);
const char *image_extrainfo(mess_image *img);



/****************************************************************************
  Battery functions

  These functions provide transparent access to battery-backed RAM on an
  image; typically for cartridges.
****************************************************************************/

void image_battery_load(mess_image *img, void *buffer, int length);
void image_battery_save(mess_image *img, const void *buffer, int length);



/****************************************************************************
  Indexing functions

  These provide various ways of indexing images
****************************************************************************/

int image_absolute_index(mess_image *image);
mess_image *image_from_absolute_index(int absolute_index);



/****************************************************************************
  Deprecated functions

  The usage of these functions is to be phased out.  The first group because
  they reflect the outdated fixed relationship between devices and their
  type/id.
****************************************************************************/

int image_index_in_device(mess_image *img);
mess_image *image_from_device_and_index(const struct IODevice *dev, int id);
mess_image *image_from_devtag_and_index(const char *devtag, int id);

/* deprecated; as there can be multiple devices of a certain type */
iodevice_t image_devtype(mess_image *img);
mess_image *image_from_devtype_and_index(iodevice_t type, int id);




/****************************************************************************
  Macros for declaring device callbacks
****************************************************************************/

#define	DEVICE_INIT(name)	int device_init_##name(mess_image *image)
#define DEVICE_EXIT(name)	void device_exit_##name(mess_image *image)
#define DEVICE_LOAD(name)	int device_load_##name(mess_image *image)
#define DEVICE_CREATE(name)	int device_create_##name(mess_image *image, int create_format, option_resolution *create_args)
#define DEVICE_UNLOAD(name)	void device_unload_##name(mess_image *image)

#endif /* IMAGE_H */
