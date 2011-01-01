/**********************************************************************

    Fairchild DM9368 7-Segment Decoder/Driver/Latch emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************
                            _____   _____
                    A1   1 |*    \_/     | 16  Vcc
                    A2   2 |             | 15  F
                   _LE   3 |             | 14  G
                  _RBO   4 |   DM9368    | 13  A
                  _RBI   5 |             | 12  B
                    A3   6 |             | 11  C
                    A0   7 |             | 10  D
                   GND   8 |_____________| 9   E

**********************************************************************/

#ifndef __DM9368__
#define __DM9368__

#include "emu.h"
#include "devcb.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

DECLARE_LEGACY_DEVICE(DM9368, dm9368);

#define MCFG_DM9368_ADD(_tag, _digit, _rbo_tag) \
	MCFG_DEVICE_ADD((_tag), DM9368, 0) \
	MCFG_DEVICE_CONFIG_DATA32(dm9368_config, digit, _digit) \
	MCFG_DEVICE_CONFIG_DATAPTR(dm9368_config, rbo_tag, _rbo_tag) \

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _dm9368_config dm9368_config;
struct _dm9368_config
{
	const char *rbo_tag;		/* device to connect RBO pin to */

	int digit;					/* output digit */
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/
/* latch input */
WRITE8_DEVICE_HANDLER( dm9368_w );

/* ripple blanking input */
WRITE_LINE_DEVICE_HANDLER( dm9368_rbi_w );

#endif
