#ifndef __UPD7220__
#define __UPD7220__

#include "devcb.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define UPD7220		DEVICE_GET_INFO_NAME(upd7220)

#define MDRV_UPD7220_ADD(_tag, _clock, _config) \
	MDRV_DEVICE_ADD(_tag, UPD7220, _clock) \
	MDRV_DEVICE_CONFIG(_config)

#define UPD7220_INTERFACE(name) const upd7220_interface (name)=

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef void (*upd7220_display_pixels_func)(const device_config *device, int y, int x, UINT16 ad, UINT8 lc, int atblink, int csr, int csr_image);
#define UPD7220_DISPLAY_PIXELS(name) void name(const device_config *device, int y, int x, UINT16 ad, UINT8 lc, int atblink, int csr, int csr_image)

typedef struct _upd7220_interface upd7220_interface;
struct _upd7220_interface
{
	const char *screen_tag;		/* screen we are acting on */

	int						hpixels_per_column;

	upd7220_display_pixels_func	display;

	/* this gets called for every video memory read */
	devcb_read8				in_data_func;

	/* this gets called for every video memory write */
	devcb_write8			out_data_func;

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

/* dma acknowledge with write */
WRITE8_DEVICE_HANDLER( upd7220_dack_w );

/* light pen strobe */
void upd7220_lpen_w(const device_config *device, int state);

#endif
