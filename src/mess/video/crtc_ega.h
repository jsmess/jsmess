/**********************************************************************

    IBM EGA CRT Controller emulation

**********************************************************************/

#ifndef __CRTC_EGA__
#define __CRTC_EGA__


DECLARE_LEGACY_DEVICE(CRTC_EGA, crtc_ega);

#define MCFG_CRTC_EGA_ADD(_tag, _clock, _intrf) \
	MCFG_DEVICE_ADD(_tag, CRTC_EGA, _clock) \
	MCFG_DEVICE_CONFIG(_intrf)

/* callback definitions */
typedef void * (*crtc_ega_begin_update_func)(device_t *device, bitmap_t *bitmap, const rectangle *cliprect);
#define CRTC_EGA_BEGIN_UPDATE(name)	void *name(device_t *device, bitmap_t *bitmap, const rectangle *cliprect)

typedef void (*crtc_ega_update_row_func)(device_t *device, bitmap_t *bitmap,
									   const rectangle *cliprect, UINT16 ma, UINT8 ra,
									   UINT16 y, UINT8 x_count, INT8 cursor_x, void *param);
#define CRTC_EGA_UPDATE_ROW(name)		void name(device_t *device, bitmap_t *bitmap,	\
											  const rectangle *cliprect, UINT16 ma, UINT8 ra,					\
											  UINT16 y, UINT8 x_count, INT8 cursor_x, void *param)

typedef void (*crtc_ega_end_update_func)(device_t *device, bitmap_t *bitmap, const rectangle *cliprect, void *param);
#define CRTC_EGA_END_UPDATE(name)		void name(device_t *device, bitmap_t *bitmap, const rectangle *cliprect, void *param)

typedef void (*crtc_ega_on_de_changed_func)(device_t *device, int display_enabled);
#define CRTC_EGA_ON_DE_CHANGED(name)	void name(device_t *device, int display_enabled)

typedef void (*crtc_ega_on_hsync_changed_func)(device_t *device, int hsync);
#define CRTC_EGA_ON_HSYNC_CHANGED(name)	void name(device_t *device, int hsync)

typedef void (*crtc_ega_on_vsync_changed_func)(device_t *device, int vsync);
#define CRTC_EGA_ON_VSYNC_CHANGED(name)	void name(device_t *device, int vsync)

typedef void (*crtc_ega_on_vblank_changed_func)(device_t *device, int vblank);
#define CRTC_EGA_ON_VBLANK_CHANGED(name) void name(device_t *device, int vblank)


/* interface */
typedef struct _crtc_ega_interface crtc_ega_interface;
struct _crtc_ega_interface
{
	const char *screen_tag;		/* screen we are acting on */
	int hpixels_per_column;		/* number of pixels per video memory address */

	/* if specified, this gets called before any pixel update,
       optionally return a pointer that will be passed to the
       update and tear down callbacks */
	crtc_ega_begin_update_func		begin_update;

	/* this gets called for every row, the driver must output
       x_count * hpixels_per_column pixels.
       cursor_x indicates the character position where the cursor is, or -1
       if there is no cursor on this row */
	crtc_ega_update_row_func		update_row;

	/* if specified, this gets called after all row updating is complete */
	crtc_ega_end_update_func			end_update;

	/* if specified, this gets called for every change of the disply enable signal */
	crtc_ega_on_de_changed_func		on_de_changed;

	/* if specified, this gets called for every change of the HSYNC signal */
	crtc_ega_on_hsync_changed_func	on_hsync_changed;

	/* if specified, this gets called for every change of the VSYNC signal */
	crtc_ega_on_vsync_changed_func	on_vsync_changed;

	/* if specificed, this gets called for every change of the VBLANK signal */
	crtc_ega_on_vblank_changed_func	on_vblank_changed;
};

/* select one of the registers for reading or writing */
WRITE8_DEVICE_HANDLER( crtc_ega_address_w );

/* read from the currently selected register */
READ8_DEVICE_HANDLER( crtc_ega_register_r );

/* write to the currently selected register */
WRITE8_DEVICE_HANDLER( crtc_ega_register_w );

/* return the current value on the MA0-MA15 pins */
UINT16 crtc_ega_get_ma(device_t *device);

/* return the current value on the RA0-RA4 pins */
UINT8 crtc_ega_get_ra(device_t *device);

/* simulates the LO->HI clocking of the light pen pin (pin 3) */
void crtc_ega_assert_light_pen_input(device_t *device);

/* set the clock (pin 21) of the chip */
void crtc_ega_set_clock(device_t *device, int clock);

/* set number of pixels per video memory address */
void crtc_ega_set_hpixels_per_column(device_t *device, int hpixels_per_column);

/* updates the screen -- this will call begin_update(),
   followed by update_row() reapeatedly and after all row
   updating is complete, end_update() */
void crtc_ega_update(device_t *device, bitmap_t *bitmap, const rectangle *cliprect);


#endif
