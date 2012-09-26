/***************************************************************************

    TTL74145

    BCD-to-Decimal decoder

***************************************************************************/

#ifndef __TTL74145_H__
#define __TTL74145_H__

#include "devcb.h"


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _ttl74145_interface ttl74145_interface;
struct _ttl74145_interface
{
	devcb_write_line output_line_0;
	devcb_write_line output_line_1;
	devcb_write_line output_line_2;
	devcb_write_line output_line_3;
	devcb_write_line output_line_4;
	devcb_write_line output_line_5;
	devcb_write_line output_line_6;
	devcb_write_line output_line_7;
	devcb_write_line output_line_8;
	devcb_write_line output_line_9;
};


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* standard handlers */
WRITE8_DEVICE_HANDLER( ttl74145_w );
READ16_DEVICE_HANDLER( ttl74145_r );


/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

DECLARE_LEGACY_DEVICE(TTL74145, ttl74145);

#define MCFG_TTL74145_ADD(_tag, _intf) \
	MCFG_DEVICE_ADD(_tag, TTL74145, 0) \
	MCFG_DEVICE_CONFIG(_intf)


/***************************************************************************
    DEFAULT INTERFACES
***************************************************************************/

extern const ttl74145_interface default_ttl74145;


#endif /* __TTL74145_H__ */
