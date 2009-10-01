/**********************************************************************

    Sinclair ZX8301 emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************
                            _____   _____
                DTACKL   1 |*    \_/     | 40  WEL
                   A17   2 |             | 39  PCENL
                   A16   3 |             | 38  VDA
                  RDWL   4 |             | 37  ROWL
                 DSMCL   5 |             | 36  TX0EL
                   VCC   6 |             | 35  XTAL2
                CLKCPU   7 |             | 34  XTAL1
                  RASL   8 |             | 33  ROM0EH
                 CAS0L   9 |             | 32  BLUE
                 CAS1L  10 |    ZX8301   | 31  GREEN
                VSYNCH  11 |     ULA     | 30  RED
                CSYNCL  12 |             | 29  DB7
                   DA0  13 |             | 28  DA7
                   DB0  14 |             | 27  DA6
                   VDD  15 |             | 26  DB6
                   DB1  16 |             | 25  DB5
                   DA1  17 |             | 24  DA5
                   DA2  18 |             | 23  DB4
                   DB2  19 |             | 22  DA4
                   DA3  20 |_____________| 21  DB3

**********************************************************************/

#ifndef __ZX8301__
#define __ZX8301__

#include "driver.h"
#include "devcb.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define ZX8301 DEVICE_GET_INFO_NAME( zx8301 )

#define MDRV_ZX8301_ADD(_tag, _clock, _config) \
	MDRV_DEVICE_ADD(_tag, ZX8301, _clock) \
	MDRV_DEVICE_CONFIG(_config)

#define ZX8301_INTERFACE(name) \
	const zx8301_interface(name) =

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _zx8301_interface zx8301_interface;
struct _zx8301_interface
{
	const char *cpu_tag;		/* cpu we are working with */
	const char *screen_tag;		/* screen we are acting on */

	/* this gets called for every memory read */
	devcb_read8			in_ram_func;

	/* this gets called for every memory write */
	devcb_write8		out_ram_func;

	/* this gets called for every change of the VSYNC pin (pin 11) */
	devcb_write_line	out_vsync_func;
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( zx8301 );

/* register access */
WRITE8_DEVICE_HANDLER( zx8301_control_w );

/* palette initialization */
PALETTE_INIT( zx8301 );

/* memory access */
READ8_DEVICE_HANDLER( zx8301_ram_r );
WRITE8_DEVICE_HANDLER( zx8301_ram_w );

/* screen update */
void zx8301_update(const device_config *device, bitmap_t *bitmap, const rectangle *cliprect);

#endif
