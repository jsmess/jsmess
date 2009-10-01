/**********************************************************************

    Sinclair ZX8302 emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************
                            _____   _____
                 ERASE   1 |*    \_/     | 40
               EXTINTL   2 |             | 39  DB4
                MDRDWL   3 |             | 38  DB5
                         4 |             | 37  DB0
                BAUDX4   5 |             | 36  DB1
                NETOUT   6 |             | 35  COMDATA
                         7 |             | 34  MDSELDH
                 DCSML   8 |             | 33  MDSELCKH
                         9 |             | 32  VSYNCH
                 PCENL  10 |    ZX8302   | 31  XTAL2
                        11 |     ULA     | 30  XTAL1
                   DB2  12 |             | 29
                        13 |             | 28  RESETOUTL
                        14 |             | 27  DB7
                    A5  15 |             | 26  IPL1L
                 NETIN  16 |             | 25  CLKCPU
                    A1  17 |             | 24  DB3
                    A0  18 |             | 23  DB6
                  RAW2  19 |             | 22  COMCTL
             RESETOUTL  20 |_____________| 21  RAW1

**********************************************************************/

#ifndef __ZX8302__
#define __ZX8302__

#include "devcb.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define ZX8302 DEVICE_GET_INFO_NAME( zx8302 )

#define MDRV_ZX8302_ADD(_tag, _clock, _config) \
	MDRV_DEVICE_ADD(_tag, ZX8302, _clock) \
	MDRV_DEVICE_CONFIG(_config)

#define ZX8302_INTERFACE(name) \
	const zx8302_interface(name) =

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _zx8302_interface zx8302_interface;
struct _zx8302_interface
{
	int rtc_clock;				/* the RTC clock (pin 30) of the chip */

	const char *mdv1_tag;		/* microdrive 1 */
	const char *mdv2_tag;		/* microdrive 2 */

	/* this gets called for every change of the IPL1L pin (pin 26) */
	devcb_write_line	out_ipl1l_func;

	/* this gets called for every change of the BAUDX4 pin (pin 5) */
	devcb_write_line	out_baudx4_func;

	/* this gets called for every write of the COMDATA pin (pin 35) */
	devcb_write_line	out_comdata_func;
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( zx8302 );

/* real time clock */
READ8_DEVICE_HANDLER( zx8302_rtc_r );
WRITE8_DEVICE_HANDLER( zx8302_rtc_w );

/* control */
WRITE8_DEVICE_HANDLER( zx8302_control_w );

/* microdrive track data */
READ8_DEVICE_HANDLER( zx8302_mdv_track_r );

/* status */
READ8_DEVICE_HANDLER( zx8302_status_r );

/* IPC command */
WRITE8_DEVICE_HANDLER( zx8302_ipc_command_w );

/* microdrive control */
WRITE8_DEVICE_HANDLER( zx8302_mdv_control_w );

/* interrupt status */
READ8_DEVICE_HANDLER( zx8302_irq_status_r );

/* interrupt acknowledge */
WRITE8_DEVICE_HANDLER( zx8302_irq_acknowledge_w );

/* serial data */
WRITE8_DEVICE_HANDLER( zx8302_data_w );

/* vertical sync */
WRITE_LINE_DEVICE_HANDLER( zx8302_vsync_w );

/* communication control */
WRITE_LINE_DEVICE_HANDLER( zx8302_comctl_w );

/* communication data */
WRITE_LINE_DEVICE_HANDLER( zx8302_comdata_w );

#endif
