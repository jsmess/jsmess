#ifndef FM7_DSK_H_
#define FM7_DSK_H_

/*
 *   Fujitsu FM-7 series D77 disk images
 * 
 */

#include "formats/flopimg.h"

FLOPPY_OPTIONS_EXTERN(fm7);

FLOPPY_IDENTIFY(fm7_d77_identify);
FLOPPY_CONSTRUCT(fm7_d77_construct);


#endif /*FM7_DSK_H_*/
