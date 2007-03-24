/***************************************************************************

	messopts.h - MESS specific option handling

****************************************************************************/

#ifndef __MESSOPTS_H__
#define __MESSOPTS_H__

/***************************************************************************
    CONSTANTS
***************************************************************************/

#define OPTION_RAMSIZE			"ramsize"
#define OPTION_WRITECONFIG		"writeconfig"
#define OPTION_SKIP_WARNINGS	"skip_warnings"



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* initialize MESS specific options (called from MAME core) */
void mess_options_init(void);

/* extract device options out of core into the options */
void mess_options_extract(void);

#endif /* __MESSOPTS_H__ */

