/******************************************************************************

    tiny.c

    messdriv.c substitute file for "tiny" MESS builds.

    The list of used drivers. Drivers have to be included here to be recognized
    by the executable.

    To save some typing, we use a hack here. This file is recursively #included
    twice, with different definitions of the DRIVER() macro. The first one
    declares external references to the drivers; the second one builds an array
    storing all the drivers.

******************************************************************************/

#include "emu.h"

#ifndef DRIVER_RECURSIVE

#define DRIVER_RECURSIVE

/* step 1: declare all external references */
#define DRIVER(NAME) extern const game_driver driver_##NAME;
#include "tiny.c"

/* step 2: define the drivers[] array */
#undef DRIVER
#define DRIVER(NAME) &driver_##NAME,
const game_driver * const drivers[] =
{
#include "tiny.c"
	0	/* end of array */
};

#else	/* DRIVER_RECURSIVE */

	/* COLECO */
	DRIVER( coleco )	/* ColecoVision (Original BIOS)                     */
	DRIVER( colecoa )	/* ColecoVision (Thick Characters)                  */
	DRIVER( colecob )	/* Spectravideo SVI-603 Coleco Game Adapter         */

#endif	/* DRIVER_RECURSIVE */
