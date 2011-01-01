/**********************************************************************

    NEC uPD3301 Programmable CRT Controller emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************
                            _____   _____
                  VRTC   1 |*    \_/     | 40  Vcc
                   RVV   2 |             | 39  SL0
                   CSR   3 |             | 38  LC0
                  LPEN   4 |             | 37  LC1
                   INT   5 |             | 36  LC2
                   DRQ   6 |             | 35  LC3
                 _DACK   7 |             | 34  VSP
                    A0   8 |             | 33  SL12
                   _RD   9 |             | 32  GPA
                   _WR  10 |   uPD3301   | 31  HLGT
                   _CS  11 |             | 30  CC7
                   DB0  12 |             | 29  CC6
                   DB1  13 |             | 28  CC5
                   DB2  14 |             | 27  CC4
                   DB3  15 |             | 26  CC3
                   DB4  16 |             | 25  CC2
                   DB5  17 |             | 24  CC1
                   DB6  18 |             | 23  CC0
                   DB7  19 |             | 22  CCLK
                   GND  20 |_____________| 21  HRTC

**********************************************************************/

#ifndef __UPD3301__
#define __UPD3301__

#include "devcb.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

DECLARE_LEGACY_DEVICE(UPD3301, upd3301);

#define MCFG_UPD3301_ADD(_tag, _clock, _intrf) \
	MCFG_DEVICE_ADD(_tag, UPD3301, _clock) \
	MCFG_DEVICE_CONFIG(_intrf)

#define UPD3301_INTERFACE(name) \
	const upd3301_interface (name) =

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef void (*upd3301_display_pixels_func)(device_t *device, bitmap_t *bitmap, int y, int sx, UINT8 cc, UINT8 lc, int hlgt, int rvv, int vsp, int sl0, int sl12, int csr, int gpa);
#define UPD3301_DISPLAY_PIXELS(name) void name(device_t *device, bitmap_t *bitmap, int y, int sx, UINT8 cc, UINT8 lc, int hlgt, int rvv, int vsp, int sl0, int sl12, int csr, int gpa)

typedef struct _upd3301_interface upd3301_interface;
struct _upd3301_interface
{
	const char *screen_tag;		/* screen we are acting on */
	int width;					/* char width in pixels */

	upd3301_display_pixels_func	display_func;

	devcb_write_line		out_int_func;
	devcb_write_line		out_drq_func;
	devcb_write_line		out_hrtc_func;
	devcb_write_line		out_vrtc_func;
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* register access */
READ8_DEVICE_HANDLER( upd3301_r );
WRITE8_DEVICE_HANDLER( upd3301_w );

/* light pen */
WRITE_LINE_DEVICE_HANDLER( upd3301_lpen_w );

/* dma acknowledge */
WRITE8_DEVICE_HANDLER( upd3301_dack_w );

/* horizontal retrace */
READ_LINE_DEVICE_HANDLER( upd3301_hrtc_r );

/* vertical retrace */
READ_LINE_DEVICE_HANDLER( upd3301_vrtc_r );

/* screen update */
void upd3301_update(device_t *device, bitmap_t *bitmap, const rectangle *cliprect);

#endif
