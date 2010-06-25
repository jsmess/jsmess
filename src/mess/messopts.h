/***************************************************************************

    messopts.h - MESS specific option handling

****************************************************************************/

#ifndef __MESSOPTS_H__
#define __MESSOPTS_H__

#include "image.h"
#include "options.h"

/***************************************************************************
    CONSTANTS
***************************************************************************/

#define OPTION_RAMSIZE			"ramsize"
#define OPTION_WRITECONFIG		"writeconfig"



/***************************************************************************
    GLOBALS
***************************************************************************/

extern const options_entry mess_core_options[];


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* initialize MESS specific options (called from MAME core) */
void mess_options_init(core_options *opts);
#endif /* __MESSOPTS_H__ */
