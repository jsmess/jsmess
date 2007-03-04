/**********************************************************************

  Copyright (C) Antoine Mine' 2006

  Thomson 8-bit computers

**********************************************************************/

#ifndef THOM_DSK
#define THOM_DSK

#include "device.h"
#include "devices/flopdrv.h"

/* recognized image formats:
   - .fd    one-side 5"1/4 (single and double density), 3"1/2
   - .qd    one-side QDD
   - .sap   one-side double-density 5"1/4, 3"1/2
*/

typedef enum {
  THOM_FLOPPY_NONE,
  THOM_FLOPPY_5_1_4,
  THOM_FLOPPY_3_1_2,
  THOM_FLOPPY_QDD,     /* 2"8 */
} thom_floppy_type;

extern int  thom_floppy_init   ( mess_image *image );
extern int  thom_floppy_load   ( mess_image* image );
extern void thom_floppy_unload ( mess_image *image);
extern int  thom_floppy_create ( mess_image *image, 
				 int create_format, option_resolution *args );

extern void    thom_floppy_set_density( DENSITY density );
extern DENSITY thom_floppy_get_density( void );

extern thom_floppy_type thom_floppy_get_type( int drive );

extern void thom_floppy_getinfo( const device_class *devclass, 
				 UINT32 state, union devinfo *info );

#endif
