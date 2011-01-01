/**********************************************************************

    RCA CDP1862 COS/MOS Color Generator Controller emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************
                            _______  _______
                RD   1  ---|       \/       |---  24  Vdd
            _RESET   2  ---|                |---  23  R LUM
              _CON   3  ---|                |---  22  G LUM
             B CHR   4  ---|                |---  21  GD
             B LUM   5  ---|                |---  20  BLG LUM
               BKG   6  ---|    CDP1862C    |---  19  G CHR
           _LD CLK   7  ---|    top view    |---  18  R CHR
               STP   8  ---|                |---  17  BKG CHR
           CLK OUT   9  ---|                |---  16  BD
             _SYNC  10  ---|                |---  15  BURST
            LUM IN  11  ---|                |---  14  _XTAL
               Vss  12  ---|________________|---  13  XTAL


           http://homepage.mac.com/ruske/cosmacelf/cdp1862.pdf

**********************************************************************/

#ifndef __CDP1862__
#define __CDP1862__

#include "devcb.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define CPD1862_CLOCK	XTAL_7_15909MHz

DECLARE_LEGACY_DEVICE(CDP1862, cdp1862);

#define MCFG_CDP1862_ADD(_tag, _clock, _config) \
	MCFG_DEVICE_ADD(_tag, CDP1862, _clock) \
	MCFG_DEVICE_CONFIG(_config)

#define CDP1862_INTERFACE(name) \
	const cdp1862_interface (name) =

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _cdp1862_interface cdp1862_interface;
struct _cdp1862_interface
{
	const char *screen_tag;		/* screen we are acting on */

	devcb_read_line				in_rd_func;
	devcb_read_line				in_bd_func;
	devcb_read_line				in_gd_func;

	double lum_r;				/* red luminance resistor value */
	double lum_b;				/* blue luminance resistor value */
	double lum_g;				/* green luminance resistor value */
	double lum_bkg;				/* background luminance resistor value */

	double chr_r;				/* red chrominance resistor value */
	double chr_b;				/* blue chrominance resistor value */
	double chr_g;				/* green chrominance resistor value */
	double chr_bkg;				/* background chrominance resistor value */
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* step background color */
WRITE8_DEVICE_HANDLER( cdp1862_bkg_w ) ATTR_NONNULL(1);

/* color on */
WRITE_LINE_DEVICE_HANDLER( cdp1862_con_w ) ATTR_NONNULL(1);

/* DMA write */
WRITE8_DEVICE_HANDLER( cdp1862_dma_w ) ATTR_NONNULL(1);

/* screen update */
void cdp1862_update(device_t *device, bitmap_t *bitmap, const rectangle *cliprect) ATTR_NONNULL(1) ATTR_NONNULL(2) ATTR_NONNULL(3);

#endif
