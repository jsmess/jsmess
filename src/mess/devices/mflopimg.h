/*********************************************************************

	mflopimg.h

	MESS interface to the floppy disk image abstraction code

*********************************************************************/

#ifndef MFLOPIMG_H
#define MFLOPIMG_H

#include "device.h"
#include "image.h"
#include "flopdrv.h"
#include "formats/flopimg.h"


enum
{
	MESS_DEVINFO_PTR_FLOPPY_OPTIONS = MESS_DEVINFO_PTR_DEV_SPECIFIC,
	MESS_DEVINFO_INT_KEEP_DRIVE_GEOMETRY = MESS_DEVINFO_INT_DEV_SPECIFIC
};

floppy_image *flopimg_get_image(const device_config *image);

void floppy_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);

/* hack for apple II; replace this when we think of something better */
void floppy_install_unload_proc(const device_config *image, void (*proc)(const device_config *image));

void floppy_install_load_proc(const device_config *image, void (*proc)(const device_config *image));

/* hack for TI99; replace this when we think of something better */
void floppy_install_tracktranslate_proc(const device_config *image, int (*proc)(const device_config *image, floppy_image *floppy, int physical_track));

#endif /* MFLOPIMG_H */
