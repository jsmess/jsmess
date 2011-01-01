/**********************************************************************

    COM8116 - Dual Baud Rate Generator (Programmable Divider) emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************
                            _____   _____
             XTAL/EXT1   1 |*    \_/     | 18  XTAL/EXT2
                   +5V   2 |             | 17  fT
                    fR   3 |             | 16  Ta
                    Ra   4 |   COM8116   | 15  Tb
                    Rb   5 |   COM8116T  | 14  Tc
                    Rc   6 |   COM8136   | 13  Td
                    Rd   7 |   COM8136T  | 12  STT
                   STR   8 |             | 11  GND
                    NC   9 |_____________| 10  fX/4

**********************************************************************/

#ifndef __COM8116__
#define __COM8116__

#include "devcb.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

DECLARE_LEGACY_DEVICE(COM8116, com8116);

#define MCFG_COM8116_ADD(_tag, _clock, _config) \
	MCFG_DEVICE_ADD(_tag, COM8116, _clock) \
	MCFG_DEVICE_CONFIG(_config)

#define COM8116_INTERFACE(name) \
	const com8116_interface (name)=

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _com8116_interface com8116_interface;
struct _com8116_interface
{
	/* this gets called for every change of the fX/4 pin (pin 10) */
	devcb_write_line		out_fx4_func;

	/* this gets called for every change of the fR pin (pin 3) */
	devcb_write_line		out_fr_func;

	/* this gets called for every change of the fT pin (pin 17) */
	devcb_write_line		out_ft_func;

	/* receiver divisor ROM (19-bit) */
	UINT32 fr_divisors[16];

	/* transmitter divisor ROM (19-bit) */
	UINT32 ft_divisors[16];
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* receiver strobe */
WRITE8_DEVICE_HANDLER( com8116_str_w );

/* transmitter strobe */
WRITE8_DEVICE_HANDLER( com8116_stt_w );

#endif /* __COM8116__ */
