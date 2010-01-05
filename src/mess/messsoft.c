/******************************************************************************

  messsoft.c

  The list of all available software lists. Software lists have to be included
  here to be recognized by the executable.

  To save some typing, we use a hack here. This file is recursively #included
  twice, with different definitions of the SOFTWARE_LIST() macro. The first
  one declares external references to the software lists; the second one builds
  an array storing all the software lists.

******************************************************************************/

#include "driver.h"
#include "softlist.h"


#ifndef SOFTWARE_LIST_RECURSIVE

#define SOFTWARE_LIST_RECURSIVE

/* step 1: declare all external references */
#define SOFTWARE_LIST(NAME) extern const software_list software_list_##NAME;
#include "messsoft.c"

/* step 2: define the software_list[] array */
#undef SOFTWARE_LIST
#define SOFTWARE_LIST(NAME) &software_list_##NAME,
const software_list * const software_lists[] =
{
#include "messsoft.c"
  0             /* end of array */
};

#else /* SOFTWARE_LIST_RECURSIVE */

/****************SOFTWARE LISTS**********************************************/

	SOFTWARE_LIST( gamepock_cart )	/* Epoch Game Pocket Computer cartridges */

#endif /* SOFTWARE_LIST_RECURSIVE */
