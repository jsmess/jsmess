/***************************************************************************

    mesvalid.c

    Validity checks on internal MESS data structures.

***************************************************************************/

#include <ctype.h>

#include "emu.h"
#include "mess.h"
#include "device.h"
#include "inputx.h"

/*************************************
 *
 *  MESS validity checks
 *
 *************************************/

int mess_validitychecks(void)
{
	return mess_validate_natural_keyboard_statics();
}
