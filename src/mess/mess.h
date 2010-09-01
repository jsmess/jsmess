/*********************************************************************

    mess.h

    Core MESS headers

*********************************************************************/

#ifndef __MESS_H__
#define __MESS_H__

#include "options.h"

/******************************************************************************
 * MESS' version of the GAME() macros of MAME
 * CONS is for consoles
 * COMP is for computers
 ******************************************************************************/

#define CONS(YEAR,NAME,PARENT,COMPAT,MACHINE,INPUT,INIT,COMPANY,FULLNAME,FLAGS)	\
extern const game_driver GAME_NAME(NAME) =	\
{											\
	__FILE__,								\
	#PARENT,								\
	#NAME,									\
	FULLNAME,								\
	#YEAR,									\
	COMPANY,								\
	MACHINE_CONFIG_NAME(MACHINE),			\
	INPUT_PORTS_NAME(INPUT),				\
	DRIVER_INIT_NAME(INIT),					\
	ROM_NAME(NAME),							\
	#COMPAT,								\
	ROT0|(FLAGS),							\
	NULL									\
};

#define COMP(YEAR,NAME,PARENT,COMPAT,MACHINE,INPUT,INIT,COMPANY,FULLNAME,FLAGS)	\
extern const game_driver GAME_NAME(NAME) =	\
{											\
	__FILE__,								\
	#PARENT,								\
	#NAME,									\
	FULLNAME,								\
	#YEAR,									\
	COMPANY,								\
	MACHINE_CONFIG_NAME(MACHINE),			\
	INPUT_PORTS_NAME(INPUT),				\
	DRIVER_INIT_NAME(INIT),					\
	ROM_NAME(NAME),							\
	#COMPAT,								\
	ROT0|(FLAGS),				\
	NULL									\
};

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
