/***************************************************************************

    IBM-PC printer interface

***************************************************************************/

#ifndef __PC_LPT_H__
#define __PC_LPT_H__

#include "devcb.h"


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _pc_lpt_interface pc_lpt_interface;
struct _pc_lpt_interface
{
	devcb_write_line out_irq_func;
};


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/
READ8_DEVICE_HANDLER( pc_lpt_r );
WRITE8_DEVICE_HANDLER( pc_lpt_w );

READ8_DEVICE_HANDLER( pc_lpt_data_r );
WRITE8_DEVICE_HANDLER( pc_lpt_data_w );
READ8_DEVICE_HANDLER( pc_lpt_status_r );
READ8_DEVICE_HANDLER( pc_lpt_control_r );
WRITE8_DEVICE_HANDLER( pc_lpt_control_w );


/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

DECLARE_LEGACY_DEVICE(PC_LPT, pc_lpt);

#define MCFG_PC_LPT_ADD(_tag, _intf) \
	MCFG_DEVICE_ADD(_tag, PC_LPT, 0) \
	MCFG_DEVICE_CONFIG(_intf)

#define MCFG_PC_LPT_REMOVE(_tag) \
	MCFG_DEVICE_REMOVE(_tag)


#endif /* __PC_LPT__ */
