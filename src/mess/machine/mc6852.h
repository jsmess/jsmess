/**********************************************************************

    Motorola MC6852 Synchronous Serial Data Adapter emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************
                            _____   _____
                   Vss   1 |*    \_/     | 24  _CTS
               Rx DATA   2 |             | 23  _DCD
                Rx CLK   3 |             | 22  D0
                Tx CLK   4 |             | 21  D1
               SM/_DTR   5 |             | 20  D2
               Tx DATA   6 |   MC6852    | 19  D3
                  _IRQ   7 |             | 18  D4
                   TUF   8 |             | 17  D5
                _RESET   9 |             | 16  D6
                   _CS   9 |             | 15  D7
                    RS   9 |             | 14  E
                   Vcc  10 |_____________| 13  R/_W

**********************************************************************/

#ifndef __MC6852__
#define __MC6852__

#include "emu.h"
#include "devcb.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

DECLARE_LEGACY_DEVICE(MC6852, mc6852);

#define MCFG_MC6852_ADD(_tag, _clock, _config) \
	MCFG_DEVICE_ADD((_tag), MC6852, _clock)	\
	MCFG_DEVICE_CONFIG(_config)

#define MC6852_INTERFACE(name) \
	const mc6852_interface (name) =

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _mc6852_interface mc6852_interface;
struct _mc6852_interface
{
	UINT32					rx_clock;
	UINT32					tx_clock;

	devcb_read_line			in_rx_data_func;
	devcb_write_line		out_tx_data_func;

	devcb_write_line		out_irq_func;

	devcb_read_line			in_cts_func;
	devcb_read_line			in_dcd_func;
	devcb_write_line		out_sm_dtr_func;
	devcb_write_line		out_tuf_func;
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/
/* register access */
READ8_DEVICE_HANDLER( mc6852_r );
WRITE8_DEVICE_HANDLER( mc6852_w );

/* clock */
WRITE_LINE_DEVICE_HANDLER( mc6852_rx_clk_w );
WRITE_LINE_DEVICE_HANDLER( mc6852_tx_clk_w );

/* clear to send */
WRITE_LINE_DEVICE_HANDLER( mc6852_cts_w );

/* data carrier detect */
WRITE_LINE_DEVICE_HANDLER( mc6852_dcd_w );

/* sync match/data terminal ready */
READ_LINE_DEVICE_HANDLER( mc6852_sm_dtr_r );

/* transmitter underflow */
READ_LINE_DEVICE_HANDLER( mc6852_tuf_r );

#endif
