/*

					  RCA CDP1861 Video Display Controller

								________________
				  CLK_	 1	---|	   \/		|---  24  Vdd
			     DMAO_	 2	---|				|---  23  CLEAR_
			      INT_	 3	---|				|---  22  SC1
				   TPA	 4	---|				|---  21  SC0
				   TPB	 5	---|				|---  20  DI7
			COMP SYNC_	 6	---|	CDP1861C	|---  19  DI6
				 VIDEO	 7	---|	top view	|---  18  DI5
				RESET_	 8	---|				|---  17  DI4
				  EFX_	 9	---|				|---  16  DI3
			   DISP ON	10	---|			    |---  15  DI2
			  DISP OFF	11	---|				|---  14  DI1
				   Vss	12	---|________________|---  13  DI0


			   http://homepage.mac.com/ruske/cosmacelf/cdp1861.pdf

*/

#ifndef __CDP1861__
#define __CDP1861__

#define CDP1861_VISIBLE_COLUMNS	64
#define CDP1861_VISIBLE_LINES	128

#define CDP1861_HBLANK_START	14 * 8
#define CDP1861_HBLANK_END		12
#define CDP1861_HSYNC_START		0
#define CDP1861_HSYNC_END		12
#define CDP1861_SCREEN_WIDTH	14 * 8

#define CDP1861_TOTAL_SCANLINES				262

#define CDP1861_SCANLINE_DISPLAY_START		80
#define CDP1861_SCANLINE_DISPLAY_END		208
#define CDP1861_SCANLINE_VBLANK_START		262
#define CDP1861_SCANLINE_VBLANK_END			16
#define CDP1861_SCANLINE_VSYNC_START		16
#define CDP1861_SCANLINE_VSYNC_END			0
#define CDP1861_SCANLINE_INT_START			CDP1861_SCANLINE_DISPLAY_START - 2
#define CDP1861_SCANLINE_INT_END			CDP1861_SCANLINE_DISPLAY_START
#define CDP1861_SCANLINE_EFX_TOP_START		CDP1861_SCANLINE_DISPLAY_START - 4
#define CDP1861_SCANLINE_EFX_TOP_END		CDP1861_SCANLINE_DISPLAY_START
#define CDP1861_SCANLINE_EFX_BOTTOM_START	CDP1861_SCANLINE_DISPLAY_END - 4
#define CDP1861_SCANLINE_EFX_BOTTOM_END		CDP1861_SCANLINE_DISPLAY_END

typedef void (*cdp1861_on_int_changed_func) (const device_config *device, int level);
#define CDP1861_ON_INT_CHANGED(name) void name(const device_config *device, int level)

typedef void (*cdp1861_on_dmao_changed_func) (const device_config *device, int level);
#define CDP1861_ON_DMAO_CHANGED(name) void name(const device_config *device, int level)

typedef void (*cdp1861_on_efx_changed_func) (const device_config *device, int level);
#define CDP1861_ON_EFX_CHANGED(name) void name(const device_config *device, int level)

#define CDP1861		DEVICE_GET_INFO_NAME(cdp1861)

/* interface */
typedef struct _cdp1861_interface cdp1861_interface;
struct _cdp1861_interface
{
	const char *screen_tag;		/* screen we are acting on */
	int clock;					/* the clock (pin 1) of the chip */

	/* this gets called for every change of the INT pin (pin 3) */
	cdp1861_on_int_changed_func		on_int_changed;

	/* this gets called for every change of the DMAO pin (pin 2) */
	cdp1861_on_dmao_changed_func	on_dmao_changed;

	/* this gets called for every change of the EFX pin (pin 9) */
	cdp1861_on_efx_changed_func		on_efx_changed;
};

/* device interface */
DEVICE_GET_INFO( cdp1861 );

/* display on */
READ8_DEVICE_HANDLER( cdp1861_dispon_r );

/* display off */
WRITE8_DEVICE_HANDLER( cdp1861_dispoff_w );

/* DMA write */
void cdp1861_dma_w(const device_config *device, UINT8 data);

/* screen update */
void cdp1861_update(const device_config *device, bitmap_t *bitmap, const rectangle *cliprect);

#endif
