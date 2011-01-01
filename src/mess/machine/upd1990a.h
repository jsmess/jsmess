/**********************************************************************

    NEC uPD1990AC Serial I/O Calendar & Clock emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************
                            _____   _____
                    C2   1 |*    \_/     | 14  Vdd
                    C1   2 |             | 13  XTAL
                    C0   3 |             | 12  _XTAL
                   STB   4 |  uPD1990AC  | 11  OUT ENBL
                    CS   5 |             | 10  TP
               DATA IN   6 |             | 9   DATA OUT
                   GND   7 |_____________| 8   CLK

**********************************************************************/

#ifndef __UPD1990A_H__
#define __UPD1990A_H__

#include "devcb.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

DECLARE_LEGACY_DEVICE(UPD1990A, upd1990a);

#define MCFG_UPD1990A_ADD(_tag, _clock, _config) \
	MCFG_DEVICE_ADD(_tag, UPD1990A, _clock) \
	MCFG_DEVICE_CONFIG(_config)

#define MCFG_UPD1990A_REMOVE(_tag) \
	MCFG_DEVICE_REMOVE(_tag)

#define UPD1990A_INTERFACE(name) \
	const upd1990a_interface (name)=

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _upd1990a_interface upd1990a_interface;
struct _upd1990a_interface
{
	/* this gets called for every change of the DATA OUT pin (pin 9) */
	devcb_write_line		out_data_func;

	/* this gets called for every change of the TP pin (pin 10) */
	devcb_write_line		out_tp_func;
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* output enable */
WRITE_LINE_DEVICE_HANDLER( upd1990a_oe_w );

/* chip select */
WRITE_LINE_DEVICE_HANDLER( upd1990a_cs_w );

/* strobe */
WRITE_LINE_DEVICE_HANDLER( upd1990a_stb_w );

/* clock */
WRITE_LINE_DEVICE_HANDLER( upd1990a_clk_w );

/* command select */
WRITE_LINE_DEVICE_HANDLER( upd1990a_c0_w );
WRITE_LINE_DEVICE_HANDLER( upd1990a_c1_w );
WRITE_LINE_DEVICE_HANDLER( upd1990a_c2_w );

/* data in */
WRITE_LINE_DEVICE_HANDLER( upd1990a_data_in_w );

/* data out */
READ_LINE_DEVICE_HANDLER( upd1990a_data_out_r );

/* timing pulse */
READ_LINE_DEVICE_HANDLER( upd1990a_tp_r );

#endif /* __UPD1990A_H__ */
