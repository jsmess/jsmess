/**********************************************************************

    MOS Technology 6529 Single Port Interface Adapter emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************
                            _____   _____
                   R/W   1 |*    \_/     | 20  Vdd
                    P0   2 |             | 19  _CS
                    P1   3 |             | 18  D0
                    P2   4 |             | 17  D1
                    P3   5 |    6529     | 16  D2
                    P4   6 |             | 15  D3
                    P5   7 |             | 14  D4
                    P6   8 |             | 13  D5
                    P7   9 |             | 12  D6
                   Vss  10 |_____________| 11  D7

**********************************************************************/

#ifndef __MOS6529__
#define __MOS6529__

#include "emu.h"
#include "devcb.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

DECLARE_LEGACY_DEVICE(MOS6529, mos6529);

#define MCFG_MOS6529_ADD(_tag, _clock, _config) \
	MCFG_DEVICE_ADD((_tag), MOS6529, _clock)	\
	MCFG_DEVICE_CONFIG(_config)

#define MOS6529_INTERFACE(name) \
	const mos6529_interface (name) =

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _mos6529_interface mos6529_interface;
struct _mos6529_interface
{
	devcb_read8				in_p_func;
	devcb_write8			out_p_func;
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* port access */
READ8_DEVICE_HANDLER( mos6529_r );
WRITE8_DEVICE_HANDLER( mos6529_w );

#endif
