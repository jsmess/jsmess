/*********************************************************************

    formats/d64_dsk.h

    Floppy format code for Commodore 1541 D64 disk images

*********************************************************************/

#ifndef __D64_DSK__
#define __D64_DSK__

#include "formats/flopimg.h"

FLOPPY_IDENTIFY( d64_dsk_identify );
FLOPPY_IDENTIFY( d71_dsk_identify );
FLOPPY_CONSTRUCT( d64_dsk_construct );

#endif
