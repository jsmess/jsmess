/****************************************************************************

	image.c

	Code for handling devices/software images

	The UI can call image_load and image_unload to associate and disassociate
	with disk images on disk.  In fact, devices that unmount on their own (like
	Mac floppy drives) may call this from within a driver.

****************************************************************************/

#include <ctype.h>

#include "mame.h"
#include "image.h"
#include "mess.h"
#include "unzip.h"
#include "devices/flopdrv.h"
#include "utils.h"
#include "hashfile.h"
#include "mamecore.h"
#include "messopts.h"



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _image_slot_data image_slot_data;
struct _image_slot_data
{
	/* variables that persist across image mounts */
	object_pool *mempool;
	const device_config *dev;

	/* callbacks */
	device_image_load_func load;
	device_image_create_func create;
	device_image_unload_func unload;

	/* error related info */
	image_error_t err;
	char *err_message;

	/* variables that are only non-zero when an image is mounted */
	core_file *file;
	char *name;
	char *dir;
	char *hash;
	char *basename_noext;

	/* flags */
	unsigned int writeable : 1;
	unsigned int created : 1;
	unsigned int is_loading : 1;

	/* info read from the hash file */
	char *longname;
	char *manufacturer;
	char *year;
	char *playable;
	char *extrainfo;

	/* working directory; persists across mounts */
	char *working_directory;

	/* pointer */
	void *ptr;
};



struct _images_private
{
	UINT32 multiple_dev_mask;
	int slot_count;
	image_slot_data slots[1];
};



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static void image_exit(running_machine *machine);
static void image_clear(image_slot_data *image);
static void image_clear_error(image_slot_data *image);
static void image_unload_internal(image_slot_data *slot);



/***************************************************************************
    CORE IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    memory_error - report a memory error
-------------------------------------------------*/

static void memory_error(const char *message)
{
	fatalerror("%s", message);
}



/*-------------------------------------------------
    image_init - initialize the core image system
-------------------------------------------------*/

void image_init(running_machine *machine)
{
	int count, indx;
	UINT32 mask, dev_mask = 0, multiple_dev_mask = 0;
	const device_config *dev;
	const struct IODevice *iodev;
	size_t private_size;
	image_slot_data *slot;

	/* sanity checks */
	assert(DEVINFO_FCT_IMAGE_FIRST > DEVINFO_FCT_FIRST);
	assert(DEVINFO_FCT_IMAGE_LAST < DEVINFO_FCT_DEVICE_SPECIFIC);

	/* first count all images, and identify multiply defined devices */
	count = 0;
	for (dev = image_device_first(machine->config); dev != NULL; dev = image_device_next(dev))
	{
		/* weird check only relevant on the first legacy MESS device */
		iodev = mess_device_from_core_device(dev);
		if ((iodev != NULL) && (iodev->index_in_device == 0))
		{
			/* check to see if this device type is used multiple times */
			mask = 1 << iodev->type;
			if (dev_mask & mask)
				multiple_dev_mask |= mask;
			else
				dev_mask |= mask;
		}

		/* increment the count */
		count++;
	}

	/* allocate the private structure */
	private_size = sizeof(*machine->images_data) + ((count - 1)
		* sizeof(machine->images_data->slots[0]));
	machine->images_data = (images_private *) auto_malloc(private_size);
	memset(machine->images_data, '\0', private_size);

	/* some setup */
	machine->images_data->multiple_dev_mask = multiple_dev_mask;
	machine->images_data->slot_count = count;

	/* initialize the devices */
	indx = 0;
	for (dev = image_device_first(machine->config); dev != NULL; dev = image_device_next(dev))
	{
		slot = &machine->images_data->slots[indx];

		/* create a memory pool */
		slot->mempool = pool_alloc(memory_error);

		/* setup the device */
		machine->images_data->slots[indx].dev = dev;

		/* callbacks */
		slot->load = (device_image_load_func) device_get_info_fct(slot->dev, DEVINFO_FCT_IMAGE_LOAD);
		slot->create = (device_image_create_func) device_get_info_fct(slot->dev, DEVINFO_FCT_IMAGE_CREATE);
		slot->unload = (device_image_unload_func) device_get_info_fct(slot->dev, DEVINFO_FCT_IMAGE_UNLOAD);

		indx++;
	}

	/* add a callback for when we shut down */
	add_exit_callback(machine, image_exit);
}



/*-------------------------------------------------
    image_exit - shut down the core image system
-------------------------------------------------*/

static void image_exit(running_machine *machine)
{
	int i;
	image_slot_data *slot;

	if (machine->images_data != NULL)
	{
		/* extract the options */
		mess_options_extract(machine);

		for (i = 0; i < machine->images_data->slot_count; i++)
		{
			/* identify the image slot */
			slot = &machine->images_data->slots[i];

			/* unload this image */
			image_unload_internal(slot);

			/* free the working directory */
			if (slot->working_directory != NULL)
			{
				free(slot->working_directory);
				slot->working_directory = NULL;
			}

			/* free the memory pool */
			pool_free(slot->mempool);
			slot->mempool = NULL;
		}

		machine->images_data = NULL;
	}
}



/****************************************************************************
    IMAGE DEVICE ENUMERATION
****************************************************************************/

/*-------------------------------------------------
    is_image_device - determines if a particular
	device supports images or not
-------------------------------------------------*/

static int is_image_device(const device_config *device)
{
	return (device->type == MESS_DEVICE);
}



/*-------------------------------------------------
    image_device_first - return the first device in
	the list that supports images
-------------------------------------------------*/

const device_config *image_device_first(const machine_config *config)
{
	const device_config *device = device_list_first(config->devicelist, DEVICE_TYPE_WILDCARD);
	while((device != NULL) && !is_image_device(device))
	{
		device = device_list_next(device, DEVICE_TYPE_WILDCARD);
	}
	return device;	
}



/*-------------------------------------------------
    image_device_next - return the next device in
	the list that supports images
-------------------------------------------------*/

const device_config *image_device_next(const device_config *prevdevice)
{
	const device_config *device = device_list_next(prevdevice, DEVICE_TYPE_WILDCARD);
	while((device != NULL) && !is_image_device(device))
	{
		device = device_list_next(device, DEVICE_TYPE_WILDCARD);
	}
	return device;	
}



/*-------------------------------------------------
    image_device_count - counts the number of
	devices that support images
-------------------------------------------------*/

int image_device_count(const machine_config *config)
{
	int count = 0;
	const device_config *device;
	for (device = image_device_first(config); device != NULL; device = image_device_next(device))
	{
		count++;
	}
	return count;
}



/****************************************************************************
    IMAGE LOADING
****************************************************************************/

/*-------------------------------------------------
    set_image_filename - specifies the filename of
	an image
-------------------------------------------------*/

static image_error_t set_image_filename(image_slot_data *image, const char *filename, const char *zippath)
{
	image_error_t err = IMAGE_ERROR_SUCCESS;
	astring *alloc_filename = NULL;
	char *new_name;
	char *new_dir;
	char *new_working_directory;
	int pos;

	/* create the directory string */
	new_dir = filename ? image_strdup(image->dev, filename) : NULL;
	for (pos = strlen(new_dir); (pos > 0); pos--)
	{
		if (strchr(":\\/", new_dir[pos - 1]))
		{
			new_dir[pos] = '\0';
			break;
		}
	}

	/* do we have to concatenate the names? */
	if (zippath)
	{
		alloc_filename = astring_assemble_3(astring_alloc(), filename, PATH_SEPARATOR, zippath);
		filename = astring_c(alloc_filename);
	}

	/* copy the string */
	new_name = image_strdup(image->dev, filename);
	if (!new_name)
	{
		err = IMAGE_ERROR_OUTOFMEMORY;
		goto done;
	}

	/* copy the working directory */
	new_working_directory = mame_strdup(new_dir);
	if (!new_working_directory)
	{
		err = IMAGE_ERROR_OUTOFMEMORY;
		goto done;
	}

	/* set the new name and dir */
	if (image->name != NULL)
		image_freeptr(image->dev, image->name);
	if (image->dir != NULL)
		image_freeptr(image->dev, image->dir);
	if (image->working_directory != NULL)
		free(image->working_directory);
	image->name = new_name;
	image->dir = new_dir;
	image->working_directory = new_working_directory;

done:
	if (alloc_filename != NULL)
		astring_free(alloc_filename);
	return err;
}



/*-------------------------------------------------
    is_loaded - quick check to determine whether an
	image is loaded
-------------------------------------------------*/

static int is_loaded(image_slot_data *image)
{
	return (image->file != NULL) || (image->ptr != NULL);
}



/*-------------------------------------------------
    load_zip_path - loads a ZIP file with a
	specific path
-------------------------------------------------*/

static image_error_t load_zip_path(image_slot_data *image, const char *path)
{
	image_error_t err = IMAGE_ERROR_FILENOTFOUND;
	zip_file *zip = NULL;
	zip_error ziperr;
	const zip_file_header *header;
	const char *zip_extension = ".zip";
	char *path_copy;
	const char *zip_entry;
	void *ptr;
	int pos;

	/* create our own copy of the path */
	path_copy = mame_strdup(path);
	if (!path_copy)
	{
		err = IMAGE_ERROR_OUTOFMEMORY;
		goto done;
	}

	/* loop through the path and try opening zip files */
	ziperr = ZIPERR_FILE_ERROR;
	zip_entry = NULL;
	pos = strlen(path_copy);
	while(pos > strlen(zip_extension))
	{
		/* is this a potential zip path? */
		if ((path_copy[pos] == '\0') || !strncmp(&path_copy[pos], PATH_SEPARATOR, strlen(PATH_SEPARATOR)))
		{
			/* parse out the zip path */
			if (path_copy[pos] == '\0')
			{
				/* no zip path */
				zip_entry = NULL;
			}
			else
			{
				/* we are at a zip path */
				path_copy[pos] = '\0';
				zip_entry = &path_copy[pos + strlen(PATH_SEPARATOR)];
			}

			/* try to open the zip file */
			ziperr = zip_file_open(path_copy, &zip);
			if (ziperr != ZIPERR_FILE_ERROR)
				break;

			/* restore the path if we changed */
			if (zip_entry)
				path_copy[pos] = PATH_SEPARATOR[0];
		}
		pos--;
	}

	/* did we succeed in opening up a zip file? */
	if (ziperr == ZIPERR_NONE)
	{
		/* iterate through the zip file */
		header = zip_file_first_file(zip);

		/* if we specified a zip partial path, find it */
		if (zip_entry != NULL)
		{
			while(header != NULL)
			{
				if (!mame_stricmp(header->filename, zip_entry))
					break;
				header = zip_file_next_file(zip);
			}
		}

		/* were we successful? */
		if (header != NULL)
		{
			/* if no zip path was specified, we have to change the name */
			if (!zip_entry)
			{
				/* use the first entry; tough part is we have to change the name */
				err = set_image_filename(image, image->name, header->filename);
				if (err)
					goto done;
			}

			/* allocate space for this zip file */
			ptr = image_malloc(image->dev, header->uncompressed_length);
			if (!ptr)
			{
				err = IMAGE_ERROR_OUTOFMEMORY;
				goto done;
			}

			ziperr = zip_file_decompress(zip, ptr, header->uncompressed_length);
			if (ziperr == ZIPERR_NONE)
			{
				/* success! */
				err = IMAGE_ERROR_SUCCESS;
				core_fopen_ram(ptr, header->uncompressed_length, OPEN_FLAG_READ, &image->file);
			}
		}
	}

done:
	if (path_copy)
		free(path_copy);
	if (zip)
		zip_file_close(zip);
	return err;
}



/*-------------------------------------------------
    load_image_by_path - loads an image with a
	specific path
-------------------------------------------------*/

static image_error_t load_image_by_path(image_slot_data *image, const char *software_path,
	const game_driver *gamedrv, UINT32 open_flags, const char *path)
{
	file_error filerr = FILERR_NOT_FOUND;
	image_error_t err = IMAGE_ERROR_FILENOTFOUND;
	astring *full_path = NULL;
	const char *file_extension;

	/* assemble the path */
	if (software_path)
	{
		full_path = astring_assemble_5(astring_alloc(), software_path, PATH_SEPARATOR, gamedrv->name, PATH_SEPARATOR, path);
		path = astring_c(full_path);
	}

	/* quick check to see if the file is a ZIP file */
	file_extension = strrchr(path, '.');
	if (!file_extension || mame_stricmp(file_extension, ".zip"))
	{
		filerr = core_fopen(path, open_flags, &image->file);

		/* did the open succeed? */
		switch(filerr)
		{
			case FILERR_NONE:
				/* success! */
				image->writeable = (open_flags & OPEN_FLAG_WRITE) ? 1 : 0;
				image->created = (open_flags & OPEN_FLAG_CREATE) ? 1 : 0;
				err = IMAGE_ERROR_SUCCESS;
				break;

			case FILERR_NOT_FOUND:
			case FILERR_ACCESS_DENIED:
				/* file not found (or otherwise cannot open); continue */
				err = IMAGE_ERROR_FILENOTFOUND;
				break;

			case FILERR_OUT_OF_MEMORY:
				/* out of memory */
				err = IMAGE_ERROR_OUTOFMEMORY;
				break;

			case FILERR_ALREADY_OPEN:
				/* this shouldn't happen */
				err = IMAGE_ERROR_ALREADYOPEN;
				break;

			case FILERR_FAILURE:
			case FILERR_TOO_MANY_FILES:
			case FILERR_INVALID_DATA:
			default:
				/* other errors */
				err = IMAGE_ERROR_INTERNAL;
				break;
		}
	}

	/* special case for ZIP files */
	if ((err == IMAGE_ERROR_FILENOTFOUND) && (open_flags == OPEN_FLAG_READ))
		err = load_zip_path(image, path);

	/* check to make sure that our reported error is reflective of the actual status */
	if (err)
		assert(!is_loaded(image));
	else
		assert(is_loaded(image));

	/* free up memory, and exit */
	if (full_path != NULL)
		astring_free(full_path);
	return err;
}



/*-------------------------------------------------
    determine_open_plan - determines which open
	flags to use, and in what order
-------------------------------------------------*/

static void determine_open_plan(image_slot_data *image, int is_create, UINT32 *open_plan)
{
	unsigned int readable, writeable, creatable;
	int i = 0;
	const struct IODevice *iodev;

	/* access legacy MESS device */
	iodev = mess_device_from_core_device(image->dev);

	/* determine disposition */
	readable = iodev->readable;
	writeable = iodev->writeable;
	creatable = iodev->creatable;

	/* emit flags */
	if (!is_create && readable && writeable)
		open_plan[i++] = OPEN_FLAG_READ | OPEN_FLAG_WRITE;
	if (!is_create && !readable && writeable)
		open_plan[i++] = OPEN_FLAG_WRITE;
	if (!is_create && readable)
		open_plan[i++] = OPEN_FLAG_READ;
	if (writeable && creatable)
		open_plan[i++] = OPEN_FLAG_READ | OPEN_FLAG_WRITE | OPEN_FLAG_CREATE;
	open_plan[i] = 0;
}



/*-------------------------------------------------
    find_image_slot - locates the slot for an
	image
-------------------------------------------------*/

static image_slot_data *find_image_slot(const device_config *image)
{
	int i;
	images_private *images_data = image->machine->images_data;

	for (i = 0; i < images_data->slot_count; i++)
	{
		if (images_data->slots[i].dev == image)
		{
			return &images_data->slots[i];
		}
	}
	return NULL;
}



/*-------------------------------------------------
    image_load_internal - core image loading
-------------------------------------------------*/

static int image_load_internal(const device_config *image, const char *path,
	int is_create, int create_format, option_resolution *create_args)
{
	running_machine *machine = image->machine;
	image_error_t err;
	const char *software_path;
	char *software_path_list = NULL;
	const void *buffer;
	const game_driver *gamedrv;
	UINT32 open_plan[4];
	int i;
	image_slot_data *slot = find_image_slot(image);
	const struct IODevice *iodev = mess_device_from_core_device(image);

	/* sanity checks */
	assert_always(image != NULL, "image_load(): image is NULL");
	assert_always(path != NULL, "image_load(): path is NULL");

	/* we are now loading */
	slot->is_loading = 1;

	/* first unload the image */
	image_unload(image);

	/* record the filename */
	slot->err = set_image_filename(slot, path, NULL);
	if (slot->err)
		goto done;

	/* do we need to reset the CPU? */
	if ((attotime_compare(timer_get_time(), attotime_zero) > 0) && iodev->reset_on_load)
		mame_schedule_hard_reset(machine);

	/* determine open plan */
	determine_open_plan(slot, is_create, open_plan);

	/* attempt to open the file in various ways */
	for (i = 0; !slot->file && open_plan[i]; i++)
	{
		software_path = software_path_list;
		do
		{
			gamedrv = machine->gamedrv;
			while(!is_loaded(slot) && (gamedrv != NULL))
			{
				/* open the file */
				slot->err = load_image_by_path(slot, software_path, gamedrv, open_plan[i], path);
				if (slot->err && (slot->err != IMAGE_ERROR_FILENOTFOUND))
					goto done;

				/* move on to the next driver */
				gamedrv = mess_next_compatible_driver(gamedrv);
			}

			/* move on to the next entry in the software path; if we can */
			if (software_path)
				software_path += strlen(software_path) + 1;
		}
		while(!is_loaded(slot) && software_path && *software_path);
	}

	/* did we fail to find the file? */
	if (!is_loaded(slot))
	{
		slot->err = IMAGE_ERROR_FILENOTFOUND;
		goto done;
	}

	/* if applicable, call device verify */
	if (iodev->imgverify && !image_has_been_created(image))
	{
		/* access the memory */
		buffer = image_ptr(image);
		if (!buffer)
		{
			slot->err = IMAGE_ERROR_OUTOFMEMORY;
			goto done;
		}

		/* verify the file */
		err = iodev->imgverify(buffer, core_fsize(slot->file));
		if (err)
		{
			slot->err = IMAGE_ERROR_INVALIDIMAGE;
			goto done;
		}
	}

	/* call device load or create */
	if (image_has_been_created(image) && (slot->create != NULL))
	{
		err = slot->create(image, create_format, create_args);
		if (err)
		{
			if (!slot->err)
				slot->err = IMAGE_ERROR_UNSPECIFIED;
			goto done;
		}
	}
	else if (slot->load != NULL)
	{
		/* using device load */
		err = slot->load(image);
		if (err)
		{
			if (!slot->err)
				slot->err = IMAGE_ERROR_UNSPECIFIED;
			goto done;
		}
	}

	/* success! */

done:
	if (software_path_list)
		free(software_path_list);
	if (slot->err)
		image_clear(slot);
	slot->is_loading = 1;
	return slot->err ? INIT_FAIL : INIT_PASS;
}



/*-------------------------------------------------
    image_load - load an image into MESS
-------------------------------------------------*/

int image_load(const device_config *image, const char *path)
{
	return image_load_internal(image, path, FALSE, 0, NULL);
}



/*-------------------------------------------------
    image_create - create a MESS image
-------------------------------------------------*/

int image_create(const device_config *image, const char *path, int create_format, option_resolution *create_args)
{
	return image_load_internal(image, path, TRUE, create_format, create_args);
}



/*-------------------------------------------------
    image_clear - clear all internal data pertaining
	to an image
-------------------------------------------------*/

static void image_clear(image_slot_data *image)
{
	if (image->file)
	{
		core_fclose(image->file);
		image->file = NULL;
	}

	pool_clear(image->mempool);
	image->writeable = 0;
	image->created = 0;
	image->name = NULL;
	image->dir = NULL;
	image->hash = NULL;
	image->longname = NULL;
	image->manufacturer = NULL;
	image->year = NULL;
	image->playable = NULL;
	image->extrainfo = NULL;
	image->basename_noext = NULL;
	image->err_message = NULL;
	image->ptr = NULL;
}



/*-------------------------------------------------
    image_unload_internal - internal call to unload
	images
-------------------------------------------------*/

static void image_unload_internal(image_slot_data *slot)
{
	/* is there an actual image loaded? */
	if (is_loaded(slot))
	{
		/* call the unload function */
		if (slot->unload != NULL)
			slot->unload(slot->dev);

		image_clear(slot);
		image_clear_error(slot);
	}
}



/*-------------------------------------------------
    image_unload - main call to unload an image
-------------------------------------------------*/

void image_unload(const device_config *image)
{
	image_slot_data *slot = find_image_slot(image);
	image_unload_internal(slot);
}



/****************************************************************************
    ERROR HANDLING
****************************************************************************/

/*-------------------------------------------------
    image_clear_error - clear out any specified
	error
-------------------------------------------------*/

static void image_clear_error(image_slot_data *image)
{
	image->err = IMAGE_ERROR_SUCCESS;
	if (image->err_message != NULL)
	{
		image_freeptr(image->dev, image->err_message);
		image->err_message = NULL;
	}
}



/*-------------------------------------------------
    image_error - returns the error text for an image
	error
-------------------------------------------------*/

const char *image_error(const device_config *image)
{
	static const char *const messages[] =
	{
		NULL,
		"Internal error",
		"Unsupported operation",
		"Out of memory",
		"File not found",
		"Invalid image",
		"File already open",
		"Unspecified error"
	};

	image_slot_data *slot = find_image_slot(image);
	return slot->err_message ? slot->err_message : messages[slot->err];
}



/*-------------------------------------------------
    image_seterror - specifies an error on an image
-------------------------------------------------*/

void image_seterror(const device_config *image, image_error_t err, const char *message)
{
	image_slot_data *slot = find_image_slot(image);

	image_clear_error(slot);
	slot->err = err;
	if (message != NULL)
	{
		slot->err_message = image_strdup(image, message);
	}
}



/****************************************************************************
  Hash info loading

  If the hash is not checked and the relevant info not loaded, force that info
  to be loaded
****************************************************************************/

static int read_hash_config(const char *sysname, image_slot_data *image)
{
	hash_file *hashfile = NULL;
	const struct hash_info *info = NULL;

	/* open the hash file */
	hashfile = hashfile_open(sysname, FALSE, NULL);
	if (!hashfile)
		goto done;

	/* look up this entry in the hash file */
	info = hashfile_lookup(hashfile, image->hash);
	if (!info)
		goto done;

	/* copy the relevant entries */
	image->longname		= info->longname		? image_strdup(image->dev, info->longname)		: NULL;
	image->manufacturer	= info->manufacturer	? image_strdup(image->dev, info->manufacturer)	: NULL;
	image->year			= info->year			? image_strdup(image->dev, info->year)			: NULL;
	image->playable		= info->playable		? image_strdup(image->dev, info->playable)		: NULL;
	image->extrainfo	= info->extrainfo		? image_strdup(image->dev, info->extrainfo)		: NULL;

done:
	if (hashfile != NULL)
		hashfile_close(hashfile);
	return !hashfile || !info;
}



static void run_hash(const device_config *image,
	void (*partialhash)(char *, const unsigned char *, unsigned long, unsigned int),
	char *dest, unsigned int hash_functions)
{
	UINT32 size;
	UINT8 *buf = NULL;

	*dest = '\0';
	size = (UINT32) image_length(image);

	buf = (UINT8 *) malloc_or_die(size);

	/* read the file */
	image_fseek(image, 0, SEEK_SET);
	image_fread(image, buf, size);

	if (partialhash)
		partialhash(dest, buf, size, hash_functions);
	else
		hash_compute(dest, buf, size, hash_functions);

	/* cleanup */
	free(buf);
	image_fseek(image, 0, SEEK_SET);
}



static int image_checkhash(image_slot_data *image)
{
	const game_driver *drv;
	const struct IODevice *dev;
	char hash_string[HASH_BUF_SIZE];
	int rc;

	/* this call should not be made when the image is not loaded */
	assert(is_loaded(image));

	/* only calculate CRC if it hasn't been calculated, and the open_mode is read only */
	if (!image->hash && !image->writeable && !image->created)
	{
		/* initialize key variables */
		dev = mess_device_from_core_device(image->dev);

		/* do not cause a linear read of 600 megs please */
		/* TODO: use SHA/MD5 in the CHD header as the hash */
		if (dev->type == IO_CDROM)
			return FALSE;

		run_hash(image->dev, dev->partialhash, hash_string, HASH_CRC | HASH_MD5 | HASH_SHA1);

		image->hash = image_strdup(image->dev, hash_string);

		/* now read the hash file */
		drv = image->dev->machine->gamedrv;
		do
		{
			rc = read_hash_config(drv->name, image);
			drv = mess_next_compatible_driver(drv);
		}
		while(rc && (drv != NULL));
	}
	return TRUE;
}



/****************************************************************************
  Accessor functions

  These provide information about the device; and about the mounted image
****************************************************************************/

/*-------------------------------------------------
    image_exists
-------------------------------------------------*/

int image_exists(const device_config *image)
{
	return image_filename(image) != NULL;
}



/*-------------------------------------------------
    image_slotexists
-------------------------------------------------*/

int image_slotexists(const device_config *image)
{
	return TRUE;
}



/*-------------------------------------------------
    image_filename
-------------------------------------------------*/

const char *image_filename(const device_config *image)
{
	image_slot_data *slot = find_image_slot(image);
	return slot->name;
}



/*-------------------------------------------------
    image_basename
-------------------------------------------------*/

const char *image_basename(const device_config *image)
{
	return osd_basename((char *) image_filename(image));
}



/*-------------------------------------------------
    image_basename_noext
-------------------------------------------------*/

const char *image_basename_noext(const device_config *image)
{
	const char *s;
	char *ext;
	image_slot_data *slot = find_image_slot(image);

	if (!slot->basename_noext)
	{
		s = image_basename(image);
		if (s)
		{
			slot->basename_noext = image_strdup(image, s);
			ext = strrchr(slot->basename_noext, '.');
			if (ext)
				*ext = '\0';
		}
	}
	return slot->basename_noext;
}



/*-------------------------------------------------
    image_filetype
-------------------------------------------------*/

const char *image_filetype(const device_config *image)
{
	const char *s;
	s = image_filename(image);
	if (s != NULL)
		s = strrchr(s, '.');
	return s ? s+1 : NULL;
}



/*-------------------------------------------------
    image_filedir
-------------------------------------------------*/

const char *image_filedir(const device_config *image)
{
	image_slot_data *slot = find_image_slot(image);
	return slot->dir;
}



/*-------------------------------------------------
    image_core_file
-------------------------------------------------*/

core_file *image_core_file(const device_config *image)
{
	image_slot_data *slot = find_image_slot(image);
	return slot->file;
}



/*-------------------------------------------------
    image_typename_id
-------------------------------------------------*/

const char *image_typename_id(const device_config *image)
{
	const struct IODevice *dev;
	int id;
	static char buf[64];

	dev = mess_device_from_core_device(image);
	id = image_index_in_device(image);
	return dev->name(dev, id, buf, sizeof(buf) / sizeof(buf[0]));
}



/*-------------------------------------------------
    check_for_file
-------------------------------------------------*/

static void check_for_file(image_slot_data *image)
{
	assert_always(image->file != NULL, "Illegal operation on unmounted image");
}



/*-------------------------------------------------
    image_length
-------------------------------------------------*/

UINT64 image_length(const device_config *image)
{
	image_slot_data *slot = find_image_slot(image);
	check_for_file(slot);
	return core_fsize(slot->file);
}



/*-------------------------------------------------
    image_hash
-------------------------------------------------*/

const char *image_hash(const device_config *image)
{
	image_slot_data *slot = find_image_slot(image);
	image_checkhash(slot);
	return slot->hash;
}



/*-------------------------------------------------
    image_crc
-------------------------------------------------*/

UINT32 image_crc(const device_config *image)
{
	const char *hash_string;
	UINT32 crc = 0;

	hash_string = image_hash(image);
	if (hash_string != NULL)
		crc = hash_data_extract_crc32(hash_string);

	return crc;
}



/*-------------------------------------------------
    image_is_writable
-------------------------------------------------*/

int image_is_writable(const device_config *image)
{
	image_slot_data *slot = find_image_slot(image);
	return slot->writeable;
}



/*-------------------------------------------------
    image_has_been_created
-------------------------------------------------*/

int image_has_been_created(const device_config *image)
{
	image_slot_data *slot = find_image_slot(image);
	return slot->created;
}



/*-------------------------------------------------
    image_make_readonly
-------------------------------------------------*/

void image_make_readonly(const device_config *image)
{
	image_slot_data *slot = find_image_slot(image);
	slot->writeable = 0;
}



/*-------------------------------------------------
    image_fread
-------------------------------------------------*/

UINT32 image_fread(const device_config *image, void *buffer, UINT32 length)
{
	image_slot_data *slot = find_image_slot(image);
	check_for_file(slot);
	return core_fread(slot->file, buffer, length);
}



/*-------------------------------------------------
    image_fwrite
-------------------------------------------------*/

UINT32 image_fwrite(const device_config *image, const void *buffer, UINT32 length)
{
	image_slot_data *slot = find_image_slot(image);
	check_for_file(slot);
	return core_fwrite(slot->file, buffer, length);
}



/*-------------------------------------------------
    image_fseek
-------------------------------------------------*/

int image_fseek(const device_config *image, INT64 offset, int whence)
{
	image_slot_data *slot = find_image_slot(image);
	check_for_file(slot);
	return core_fseek(slot->file, offset, whence);
}



/*-------------------------------------------------
    image_ftell
-------------------------------------------------*/

UINT64 image_ftell(const device_config *image)
{
	image_slot_data *slot = find_image_slot(image);
	check_for_file(slot);
	return core_ftell(slot->file);
}



/*-------------------------------------------------
    image_fgetc
-------------------------------------------------*/

int image_fgetc(const device_config *image)
{
	char ch;
	if (image_fread(image, &ch, 1) != 1)
		ch = '\0';
	return ch;
}



/*-------------------------------------------------
    image_feof
-------------------------------------------------*/

int image_feof(const device_config *image)
{
	image_slot_data *slot = find_image_slot(image);
	check_for_file(slot);
	return core_feof(slot->file);
}



/*-------------------------------------------------
    image_ptr
-------------------------------------------------*/

void *image_ptr(const device_config *image)
{
	image_slot_data *slot = find_image_slot(image);
	check_for_file(slot);
	return (void *) core_fbuffer(slot->file);
}



/***************************************************************************
    WORKING DIRECTORIES
***************************************************************************/

/*-------------------------------------------------
    try_change_working_directory - tries to change
	the working directory, but only if the directory
	actually exists
-------------------------------------------------*/

static int try_change_working_directory(image_slot_data *image, const char *subdir)
{
	osd_directory *directory;
	const osd_directory_entry *entry;
	int success = FALSE;
	int done = FALSE;
	char *new_working_directory;

	directory = osd_opendir(image->working_directory);
	if (directory != NULL)
	{
		while(!done && (entry = osd_readdir(directory)) != NULL)
		{
			if (!mame_stricmp(subdir, entry->name))
			{
				done = TRUE;
				success = entry->type == ENTTYPE_DIR;
			}
		}

		osd_closedir(directory);
	}

	/* did we successfully identify the directory? */
	if (success)
	{
		new_working_directory = malloc_or_die(strlen(image->working_directory)
			+ strlen(PATH_SEPARATOR) + strlen(subdir) + strlen(PATH_SEPARATOR) + 1);
		strcpy(new_working_directory, image->working_directory);

		/* remove final path separator, if present */
		if ((strlen(new_working_directory) >= strlen(PATH_SEPARATOR))
			&& !strcmp(new_working_directory + strlen(new_working_directory) - strlen(PATH_SEPARATOR), PATH_SEPARATOR))
		{
			new_working_directory[strlen(new_working_directory) - strlen(PATH_SEPARATOR)] = '\0';
		}

		strcat(new_working_directory, PATH_SEPARATOR);
		strcat(new_working_directory, subdir);
		strcat(new_working_directory, PATH_SEPARATOR);

		free(image->working_directory);
		image->working_directory = new_working_directory;
	}
	return success;
}



/*-------------------------------------------------
    setup_working_directory - sets up the working
	directory according to a few defaults
-------------------------------------------------*/

static void setup_working_directory(image_slot_data *image)
{
	char mess_directory[1024];
	const game_driver *gamedrv;

	/* first set up the working directory to be the MESS directory */
	osd_get_emulator_directory(mess_directory, ARRAY_LENGTH(mess_directory));
	image->working_directory = mame_strdup(mess_directory);

	/* now try browsing down to "software" */
	if (try_change_working_directory(image, "software"))
	{
		/* now down to a directory for this computer */
		gamedrv = image->dev->machine->gamedrv;
		while(gamedrv && !try_change_working_directory(image, gamedrv->name))
		{
			gamedrv = mess_next_compatible_driver(gamedrv);
		}
	}
}



/*-------------------------------------------------
    image_working_directory - returns the working
	directory to use for this image; this is
	valid even if not mounted
-------------------------------------------------*/

const char *image_working_directory(const device_config *image)
{
	image_slot_data *slot = find_image_slot(image);

	/* check to see if we've never initialized the working directory */
	if (slot->working_directory == NULL)
		setup_working_directory(slot);

	return slot->working_directory;
}



/*-------------------------------------------------
    image_set_working_directory - sets the working
	directory to use for this image
-------------------------------------------------*/

void image_set_working_directory(const device_config *image, const char *working_directory)
{
	image_slot_data *slot = find_image_slot(image);

	char *new_working_directory = mame_strdup(working_directory);
	if (slot->working_directory)
		free(slot->working_directory);
	slot->working_directory = new_working_directory;
}



/****************************************************************************
  Memory allocators

  These allow memory to be allocated for the lifetime of a mounted image.
  If these (and the above accessors) are used well enough, they should be
  able to eliminate the need for a unload function.
****************************************************************************/

void *image_malloc(const device_config *image, size_t size)
{
	return image_realloc(image, NULL, size);
}



void *image_realloc(const device_config *image, void *ptr, size_t size)
{
	image_slot_data *slot = find_image_slot(image);

	/* sanity checks */
	assert(is_loaded(slot) || slot->is_loading);

	return pool_realloc(slot->mempool, ptr, size);
}



char *image_strdup(const device_config *image, const char *src)
{
	image_slot_data *slot = find_image_slot(image);

	/* sanity checks */
	assert(is_loaded(slot) || slot->is_loading);

	return pool_strdup(slot->mempool, src);
}



void image_freeptr(const device_config *image, void *ptr)
{
	/* should really do something better here */
	image_realloc(image, ptr, 0);
}



/****************************************************************************
  CRC Accessor functions

  When an image is mounted; these functions provide access to the information
  pertaining to that image in the CRC database
****************************************************************************/

const char *image_longname(const device_config *image)
{
	image_slot_data *slot = find_image_slot(image);
	image_checkhash(slot);
	return slot->longname;
}



const char *image_manufacturer(const device_config *image)
{
	image_slot_data *slot = find_image_slot(image);
	image_checkhash(slot);
	return slot->manufacturer;
}



const char *image_year(const device_config *image)
{
	image_slot_data *slot = find_image_slot(image);
	image_checkhash(slot);
	return slot->year;
}



const char *image_playable(const device_config *image)
{
	image_slot_data *slot = find_image_slot(image);
	image_checkhash(slot);
	return slot->playable;
}



const char *image_extrainfo(const device_config *image)
{
	image_slot_data *slot = find_image_slot(image);
	image_checkhash(slot);
	return slot->extrainfo;
}



/****************************************************************************
  Battery functions

  These functions provide transparent access to battery-backed RAM on an
  image; typically for cartridges.
****************************************************************************/

/*-------------------------------------------------
    open_battery_file - opens the battery backed
	NVRAM file for an image
-------------------------------------------------*/

static file_error open_battery_file(const device_config *image, UINT32 openflags, mame_file **file)
{
	file_error filerr;
	char *basename_noext;
	astring *fname;

	basename_noext = strip_extension(image_basename(image));
	if (!basename_noext)
		return FILERR_OUT_OF_MEMORY;
	fname = astring_assemble_4(astring_alloc(), image->machine->gamedrv->name, PATH_SEPARATOR, basename_noext, ".nv");
	filerr = mame_fopen(SEARCHPATH_NVRAM, astring_c(fname), openflags, file);
	astring_free(fname);
	free(basename_noext);
	return filerr;
}



/*-------------------------------------------------
    image_battery_load - retrieves the battery
	backed RAM for an image
-------------------------------------------------*/

void image_battery_load(const device_config *image, void *buffer, int length)
{
	file_error filerr;
	mame_file *file;
	int bytes_read = 0;

	assert_always(buffer && (length > 0), "Must specify sensical buffer/length");

	/* try to open the battery file and read it in, if possible */
	filerr = open_battery_file(image, OPEN_FLAG_READ, &file);
	if (filerr == FILERR_NONE)
	{
		bytes_read = mame_fread(file, buffer, length);
		mame_fclose(file);
	}

	/* fill remaining bytes (if necessary) */
	memset(((char *) buffer) + bytes_read, '\0', length - bytes_read);
}



/*-------------------------------------------------
    image_battery_save - stores the battery
	backed RAM for an image
-------------------------------------------------*/

void image_battery_save(const device_config *image, const void *buffer, int length)
{
	file_error filerr;
	mame_file *file;

	assert_always(buffer && (length > 0), "Must specify sensical buffer/length");

	/* try to open the battery file and write it out, if possible */
	filerr = open_battery_file(image, OPEN_FLAG_WRITE | OPEN_FLAG_CREATE | OPEN_FLAG_CREATE_PATHS, &file);
	if (filerr == FILERR_NONE)
	{
		mame_fwrite(file, buffer, length);
		mame_fclose(file);
	}
}



/****************************************************************************
  Indexing functions

  These provide various ways of indexing images
****************************************************************************/

int image_absolute_index(const device_config *image)
{
	image_slot_data *slot = find_image_slot(image);
	return slot - image->machine->images_data->slots;
}



const device_config *image_from_absolute_index(running_machine *machine, int absolute_index)
{
	return machine->images_data->slots[absolute_index].dev;
}
