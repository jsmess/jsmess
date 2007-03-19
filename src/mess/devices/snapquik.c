/*********************************************************************

	snapquik.h

	Snapshots and quickloads

*********************************************************************/

#include "devices/snapquik.h"
#include "mess.h"

/* ----------------------------------------------------------------------- */

struct snapquick_info
{
	const struct IODevice *dev;
	mess_image *img;
	int file_size;
	struct snapquick_info *next;
};

static struct snapquick_info *snapquick_infolist;



static void snapquick_processsnapshot(int arg)
{
	struct snapquick_info *si;
	snapquick_loadproc loadproc;
	const char *file_type;
	
	si = (struct snapquick_info *) arg;
	loadproc = (snapquick_loadproc) device_get_info_fct(&si->dev->devclass, DEVINFO_PTR_SNAPSHOT_LOAD);
	file_type = image_filetype(si->img);
	loadproc(si->img, file_type, si->file_size);
	image_unload(si->img);
}



static int device_load_snapquick(mess_image *image)
{
	const struct IODevice *dev;
	struct snapquick_info *si;
	double delay;
	int file_size;

	file_size = image_length(image);
	if (file_size <= 0)
		return INIT_FAIL;

	si = (struct snapquick_info *) image_malloc(image, sizeof(struct snapquick_info));
	if (!si)
		return INIT_FAIL;

	dev = image_device(image);
	assert(dev);

	si->dev = dev;
	si->img = image;
	si->file_size = file_size;
	si->next = snapquick_infolist;
	snapquick_infolist = si;

	delay = device_get_info_double(&si->dev->devclass, DEVINFO_FLOAT_SNAPSHOT_DELAY);

	timer_set(delay, (int) si, snapquick_processsnapshot);
	return INIT_PASS;
}



static void device_unload_snapquick(mess_image *image)
{
	struct snapquick_info **si = &snapquick_infolist;

	while(*si && ((*si)->img != image))
		si = &(*si)->next;
	if (*si)
		*si = (*si)->next;
}



/* ----------------------------------------------------------------------- */

static void snapquick_device_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:						info->i = 1; break;
		case DEVINFO_INT_READABLE:					info->i = 1; break;
		case DEVINFO_INT_WRITEABLE:					info->i = 0; break;
		case DEVINFO_INT_CREATABLE:					info->i = 0; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:						info->load = device_load_snapquick; break;
		case DEVINFO_PTR_UNLOAD:					info->unload = device_unload_snapquick; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_DEV_FILE:					strcpy(info->s = device_temp_str(), __FILE__); break;
	}
}



void snapshot_device_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:						info->i = IO_SNAPSHOT; break;

		/* --- the following bits of info are returned as doubles --- */
		case DEVINFO_FLOAT_SNAPSHOT_DELAY:			info->d = 0.0; break;

		default: snapquick_device_getinfo(devclass, state, info);
	}
}



void quickload_device_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:						info->i = IO_QUICKLOAD; break;

		/* --- the following bits of info are returned as doubles --- */
		case DEVINFO_FLOAT_QUICKLOAD_DELAY:			info->d = 0.0; break;

		default: snapquick_device_getinfo(devclass, state, info);
	}
}

