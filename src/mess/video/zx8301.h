/**********************************************************************

    Sinclair ZX8301 emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************
							_____   _____
				DTACKL   1 |*    \_/     | 40  WEL
				   A17   2 |			 | 39  PCENL
				   A16   3 |			 | 38  VDA
				  RDWL   4 |			 | 37  ROWL
				 DSMCL   5 |			 | 36  TX0EL
				   VCC   6 |			 | 35  XTAL2
				CLKCPU   7 |			 | 34  XTAL1
				  RASL   8 |			 | 33  ROM0EH
				 CAS0L   9 |			 | 32  BLUE
				 CAS1L  10 |	ZX8301	 | 31  GREEN
				VSYNCH  11 |	 ULA	 | 30  RED
				CSYNCL  12 |			 | 29  DB7
				   DA0  13 |			 | 28  DA7
				   DB0  14 |			 | 27  DA6
				   VDD  15 |			 | 26  DB6
				   DB1  16 |			 | 25  DB5
				   DA1  17 |			 | 24  DA5
				   DA2  18 |			 | 23  DB4
				   DB2  19 |			 | 22  DA4
				   DA3  20 |_____________| 21  DB3

**********************************************************************/

#ifndef __ZX8301__
#define __ZX8301__

#include "driver.h"

typedef void (*zx8301_on_vsync_changed_func) (const device_config *device, int level);
#define ZX8301_ON_VSYNC_CHANGED(name) void name(const device_config *device, int level)

typedef UINT8 (*zx8301_ram_read_func)(const device_config *device, UINT32 da);
#define ZX8301_RAM_READ(name) UINT8 name(const device_config *device, UINT32 da)

typedef void (*zx8301_ram_write_func)(const device_config *device, UINT32 da, UINT8 data);
#define ZX8301_RAM_WRITE(name) void name(const device_config *device, UINT32 da, UINT8 data)

#define ZX8301 DEVICE_GET_INFO_NAME( zx8301 )

#define MDRV_ZX8301_ADD(_tag, _clock, _config) \
	MDRV_DEVICE_ADD(_tag, ZX8301, _clock) \
	MDRV_DEVICE_CONFIG(_config)

/* interface */
typedef struct _zx8301_interface zx8301_interface;
struct _zx8301_interface
{
	const char *screen_tag;		/* screen we are acting on */

	/* this gets called for every change of the VSYNC pin (pin 11) */
	zx8301_on_vsync_changed_func	on_vsync_changed;

	/* this gets called for every memory read */
	zx8301_ram_read_func			ram_r;

	/* this gets called for every memory write */
	zx8301_ram_write_func			ram_w;
};
#define ZX8301_INTERFACE(name) const zx8301_interface(name) =

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
