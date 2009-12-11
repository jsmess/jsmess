/*********************************************************************

    formats/g64_dsk.h

    Floppy format code for Commodore 1541 GCR disk images

*********************************************************************/

#ifndef __G64_DSK__
#define __G64_DSK__

#include "formats/flopimg.h"

FLOPPY_IDENTIFY( g64_dsk_identify );
FLOPPY_CONSTRUCT( g64_dsk_construct );

#endif
