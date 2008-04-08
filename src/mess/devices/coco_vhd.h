/***************************************************************************

	coco_vhd.h

	Color Computer Virtual Hard Drives

***************************************************************************/

#ifndef COCOVHD_H
#define COCOVHD_H

#include "image.h"

READ8_DEVICE_HANDLER(coco_vhd_io_r);
WRITE8_DEVICE_HANDLER(coco_vhd_io_w);

DEVICE_GET_INFO(coco_vhd);

#define COCO_VHD	DEVICE_GET_INFO_NAME(coco_vhd)

#endif /* COCOVHD_H */
