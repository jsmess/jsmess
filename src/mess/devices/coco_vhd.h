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

#define MDRV_COCO_VHD_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, COCO_VHD, 0) \

#endif /* COCOVHD_H */
