/***************************************************************************

    APRIDISK disk image format

***************************************************************************/

#ifndef __APRIDISK_H__
#define __APRIDISK_H__

#include "imagedev/flopimg.h"

FLOPPY_IDENTIFY( apridisk_identify );
FLOPPY_CONSTRUCT( apridisk_construct );

#endif /* __APRIDISK_H__ */
