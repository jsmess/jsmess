/*********************************************************************

	snapquik.h

	Snapshots and quickloads

*********************************************************************/

#ifndef SNAPQUIK_H
#define SNAPQUIK_H

#include "device.h"
#include "image.h"


enum
{
	MESS_DEVINFO_PTR_SNAPSHOT_LOAD = MESS_DEVINFO_PTR_DEV_SPECIFIC,
	MESS_DEVINFO_PTR_QUICKLOAD_LOAD = MESS_DEVINFO_PTR_DEV_SPECIFIC,

	MESS_DEVINFO_FLOAT_SNAPSHOT_DELAY = MESS_DEVINFO_FLOAT_DEV_SPECIFIC,
	MESS_DEVINFO_FLOAT_QUICKLOAD_DELAY = MESS_DEVINFO_FLOAT_DEV_SPECIFIC
};

typedef int (*snapquick_loadproc)(mess_image *image, const char *file_type, int file_size);

#define SNAPSHOT_LOAD(name)		int snapshot_load_##name(mess_image *image, const char *file_type, int snapshot_size)
#define QUICKLOAD_LOAD(name)	int quickload_load_##name(mess_image *image, const char *file_type, int quickload_size)

void snapshot_device_getinfo(const device_class *devclass, UINT32 state, union devinfo *info);
void quickload_device_getinfo(const device_class *devclass, UINT32 state, union devinfo *info);


#endif /* SNAPQUIK_H */
