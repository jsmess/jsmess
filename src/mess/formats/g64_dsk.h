/*********************************************************************

    formats/g64_dsk.h

    Floppy format code for Commodore 1541 GCR disk images

*********************************************************************/

#ifndef __G64_DSK__
#define __G64_DSK__

#include "formats/flopimg.h"

floperr_t g64_get_track_length(floppy_image *floppy, int track, UINT64 *length);

FLOPPY_IDENTIFY( g64_dsk_identify );
FLOPPY_CONSTRUCT( g64_dsk_construct );

#endif
