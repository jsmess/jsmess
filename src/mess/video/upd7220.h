/**********************************************************************

    NEC uPD7220 Graphics Display Controller emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************
                            _____   _____
                2xWCLK   1 |*    \_/     | 40  Vcc
                 _DBIN   2 |             | 39  A17
                 HSYNC   3 |             | 38  A16
            V/EXT SYNC   4 |             | 37  AD15
                 BLANK   5 |             | 36  AD14
                   ALE   6 |             | 35  AD13
                   DRQ   7 |             | 34  AD12
                 _DACK   8 |             | 33  AD11
                   _RD   9 |             | 32  AD10
                   _WR  10 |   uPD7220   | 31  AD9
                    A0  11 |   	82720    | 30  AD8
                   DB0  12 |             | 29  AD7
                   DB1  13 |             | 28  AD6
                   DB2  14 |             | 27  AD5
                   DB3  15 |             | 26  AD4
                   DB4  16 |             | 25  AD3
                   DB5  17 |             | 24  AD2
                   DB6  18 |             | 23  AD1
                   DB7  19 |             | 22  AD0
                   GND  20 |_____________| 21  LPEN

**********************************************************************/

#ifndef __UPD7220__
#define __UPD7220__

#include "devcb.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define UPD7220		DEVICE_GET_INFO_NAME(upd7220)

#define MDRV_UPD7220_ADD(_tag, _clock, _config, _map) \
	MDRV_DEVICE_ADD(_tag, UPD7220, _clock) \
	MDRV_DEVICE_CONFIG(_config) \
	MDRV_DEVICE_ADDRESS_MAP(0, _map)

#define UPD7220_INTERFACE(name) \
	const upd7220_interface (name) =

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef void (*upd7220_display_pixels_func)(const device_config *device, bitmap_t *bitmap, int y, int x, UINT32 address, UINT16 data);
#define UPD7220_DISPLAY_PIXELS(name) void name(const device_config *device, bitmap_t *bitmap, int y, int x, UINT32 address, UINT16 data)

typedef struct _upd7220_interface upd7220_interface;
struct _upd7220_interface
{
	const char *screen_tag;		/* screen we are acting on */

	upd7220_display_pixels_func	display_func;

	/* this gets called for every change of the DRQ pin (pin 7) */
	devcb_write_line		out_drq_func;

	/* this gets called for every change of the HSYNC pin (pin 3) */
	devcb_write_line		out_hsync_func;

	/* this gets called for every change of the VSYNC pin (pin 4) */
	devcb_write_line		out_vsync_func;

	/* this gets called for every change of the BLANK pin (pin 5) */
	devcb_write_line		out_blank_func;
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( upd7220 );

/* register read */
READ8_DEVICE_HANDLER( upd7220_r );

/* register write */
WRITE8_DEVICE_HANDLER( upd7220_w );

/* dma acknowledge */
READ8_DEVICE_HANDLER( upd7220_dack_r );
WRITE8_DEVICE_HANDLER( upd7220_dack_w );

/* external synchronization */
WRITE_LINE_DEVICE_HANDLER( upd7220_ext_sync_w );

/* light pen strobe */
WRITE_LINE_DEVICE_HANDLER( upd7220_lpen_w );

/* screen update */
void upd7220_update(const device_config *device, bitmap_t *bitmap, const rectangle *cliprect);

#endif
