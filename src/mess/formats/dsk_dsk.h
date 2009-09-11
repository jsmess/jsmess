/*********************************************************************

	formats/dsk_dsk.h

	DSK disk images

*********************************************************************/

#ifndef __DSK_DSK_H__
#define __DSK_DSK_H__

#include "formats/flopimg.h"


FLOPPY_OPTIONS_EXTERN(dsk);

FLOPPY_IDENTIFY(dsk_dsk_identify);
FLOPPY_CONSTRUCT(dsk_dsk_construct);


#endif /* __DSK_DSK_H__ */
