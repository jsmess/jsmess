/***************************************************************************

    coco_vhd.h

    Color Computer Virtual Hard Drives

***************************************************************************/

#ifndef COCOVHD_H
#define COCOVHD_H

#include "image.h"

READ8_DEVICE_HANDLER(coco_vhd_io_r);
WRITE8_DEVICE_HANDLER(coco_vhd_io_w);


DECLARE_LEGACY_IMAGE_DEVICE(COCO_VHD, coco_vhd);

#define MCFG_COCO_VHD_ADD(_tag) \
	MCFG_DEVICE_ADD(_tag, COCO_VHD, 0) \

#endif /* COCOVHD_H */
