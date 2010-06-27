/*********************************************************************

    mess.h

    Core MESS headers

*********************************************************************/

#ifndef __MESS_H__
#define __MESS_H__

#include "options.h"

/***************************************************************************
    CONSTANTS
***************************************************************************/

#define OPTION_RAMSIZE			"ramsize"
#define OPTION_WRITECONFIG		"writeconfig"
#define LCD_FRAMES_PER_SECOND	30


/***************************************************************************
    GLOBALS
***************************************************************************/

extern const options_entry mess_core_options[];

extern const char layout_lcd[];	/* generic 1:1 lcd screen layout */
extern const char layout_lcd_rot[];	/* same, for use with ROT90 or ROT270 */

extern const char mess_disclaimer[];

/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

void mess_display_help(void);

/* code used by print_mame_xml() */
void print_mess_game_xml(FILE *out, const game_driver *game, const machine_config *config);

/* initialize MESS specific options (called from MAME core) */
void mess_options_init(core_options *opts);

#endif /* __MESS_H__ */
