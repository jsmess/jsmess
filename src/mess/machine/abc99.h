#ifndef __ABC99__
#define __ABC99__

#include "emu.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define ABC99_TAG	"abc99"

DECLARE_LEGACY_DEVICE(ABC99, abc99);

#define MDRV_ABC99_ADD \
	MDRV_DEVICE_ADD(ABC99_TAG, ABC99, 0)

#endif
