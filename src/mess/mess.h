/*********************************************************************

	mess.h

	Core MESS headers

*********************************************************************/

#ifndef __MESS_H__
#define __MESS_H__

#include <stdarg.h>

struct SystemConfigurationParamBlock;

#include "image.h"
#include "artworkx.h"
#include "memory.h"
#include "configms.h"
#include "messopts.h"



/***************************************************************************

	Constants

***************************************************************************/

#define LCD_FRAMES_PER_SECOND	30


/**************************************************************************/

/* Win32 defines this for vararg functions */
#ifndef DECL_SPEC
#define DECL_SPEC
#endif


/**************************************************************************/

/* mess specific layout files */
extern const char layout_lcd[];	/* generic 1:1 lcd screen layout */


/***************************************************************************/

extern const char mess_disclaimer[];



/***************************************************************************/

/* IODevice Initialisation return values.  Use these to determine if */
/* the emulation can continue if IODevice initialisation fails */
#define INIT_PASS 0
#define INIT_FAIL 1
#define IMAGE_VERIFY_PASS 0
#define IMAGE_VERIFY_FAIL 1

/* runs checks to see if device code is proper */
int mess_validitychecks(void);

/* these are called from mame.c */
void mess_predevice_init(running_machine *machine);
void mess_postdevice_init(running_machine *machine);


/* --------------------------------------------------------------------------------------------- */

/* This call is used to return the next compatible driver with respect to
 * software images.  It is usable both internally and from front ends
 */
const game_driver *mess_next_compatible_driver(const game_driver *drv);
int mess_count_compatible_drivers(const game_driver *drv);

/* --------------------------------------------------------------------------------------------- */


#endif /* __MESS_H__ */
