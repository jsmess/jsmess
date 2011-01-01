/**********************************************************************

    Fairchild MM74C922/MM74C923 16/20-Key Encoder emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************
                            _____   _____
                ROW Y1   1 |*    \_/     | 18  Vcc
                ROW Y2   2 |             | 17  DATA OUT A
                ROW Y3   3 |             | 16  DATA OUT B
                ROW Y4   4 |             | 15  DATA OUT C
            OSCILLATOR   5 |  MM74C922   | 14  DATA OUT D
        KEYBOUNCE MASK   6 |             | 13  _OUTPUT ENABLE
             COLUMN X4   7 |             | 12  DATA AVAILABLE
             COLUMN X3   8 |             | 11  COLUMN X1
                   GND   9 |_____________| 10  COLUMN X2

                            _____   _____
                ROW Y1   1 |*    \_/     | 20  Vcc
                ROW Y2   2 |             | 19  DATA OUT A
                ROW Y3   3 |             | 18  DATA OUT B
                ROW Y4   4 |             | 17  DATA OUT C
                ROW Y5   5 |  MM74C923   | 16  DATA OUT D
            OSCILLATOR   6 |             | 15  DATA OUT E
        KEYBOUNCE MASK   7 |             | 14  _OUTPUT ENABLE
             COLUMN X4   8 |             | 13  DATA AVAILABLE
             COLUMN X3   9 |             | 12  COLUMN X1
                   GND  10 |_____________| 11  COLUMN X2

**********************************************************************/

#ifndef __MM74C922__
#define __MM74C922__

#include "emu.h"
#include "devcb.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

DECLARE_LEGACY_DEVICE(MM74C922, mm74c922);
DECLARE_LEGACY_DEVICE(MM74C923, mm74c923);

#define MCFG_MM74C922_ADD(_tag, _intrf) \
	MCFG_DEVICE_ADD(_tag, MM74C922, 0) \
	MCFG_DEVICE_CONFIG(_intrf)

#define MCFG_MM74C923_ADD(_tag, _intrf) \
	MCFG_DEVICE_ADD(_tag, MM74C923, 0) \
	MCFG_DEVICE_CONFIG(_intrf)

#define MM74C922_INTERFACE(name) \
	const mm74c922_interface (name)=

#define MM74C923_INTERFACE(name) \
	const mm74c922_interface (name)=

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _mm74c922_interface mm74c922_interface;
struct _mm74c922_interface
{
	devcb_read8			in_x1_func;
	devcb_read8			in_x2_func;
	devcb_read8			in_x3_func;
	devcb_read8			in_x4_func;
	devcb_read8			in_x5_func;

	devcb_write_line	out_da_func;

	double				cap_osc;
	double				cap_debounce;
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/
/* data read */
READ8_DEVICE_HANDLER( mm74c922_r );

#endif
