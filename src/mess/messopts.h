/***************************************************************************

    messopts.h - MESS specific option handling

****************************************************************************/

#ifndef __MESSOPTS_H__
#define __MESSOPTS_H__

#include "image.h"

/***************************************************************************
    CONSTANTS
***************************************************************************/

#define OPTION_RAMSIZE			"ramsize"
#define OPTION_WRITECONFIG		"writeconfig"
#define OPTION_SKIP_WARNINGS	"skip_warnings"
#define OPTION_NATURAL_KEYBOARD	"natural"



/***************************************************************************
    GLOBALS
***************************************************************************/

extern const options_entry mess_core_options[];


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* initialize MESS specific options (called from MAME core) */
void mess_options_init(core_options *opts);

/* extract device options out of core into the options */
void mess_options_extract(running_machine *machine);

/* add the device options for a specified device */
void mess_add_device_options(core_options *opts, const game_driver *driver);

/* accesses a device option, by device and index */
const char *mess_get_device_option(const image_device_info *info);

#endif /* __MESSOPTS_H__ */
