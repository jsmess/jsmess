/*

			    CDP1864C COS/MOS PAL Compatible Color TV Interface

								________________
				INLACE	 1	---|	   \/		|---  40  Vdd
			   CLK IN_	 2	---|				|---  39  AUD
			  CLR OUT_	 3	---|				|---  38  CLR IN_
				   AOE	 4	---|				|---  37  DMA0_
				   SC1	 5	---|				|---  36  INT_
				   SC0	 6	---|				|---  35  TPA
				  MRD_	 7	---|				|---  34  TPB
				 BUS 7	 8	---|				|---  33  EVS
				 BUS 6	 9	---|				|---  32  V SYNC
				 BUS 5	10	---|	CDP1864C    |---  31  H SYNC
				 BUS 4	11	---|	top view	|---  30  C SYNC_
				 BUS 3	12	---|				|---  29  RED
				 BUS 2	13	---|				|---  28  BLUE
				 BUS 1	14	---|				|---  27  GREEN
				 BUS 0	15	---|				|---  26  BCK GND_
				  CON_	16	---|				|---  25  BURST
					N2	17	---|				|---  24  ALT
				   EF_	18	---|				|---  23  R DATA
					N0	19	---|				|---  22  G DATA
				   Vss	20	---|________________|---  21  B DATA


			   http://homepage.mac.com/ruske/cosmacelf/cdp1864.pdf

*/

#ifndef __CDP1864__
#define __CDP1864__

#define CDP1864_CLOCK	XTAL_1_75MHz

#define CDP1864_VISIBLE_COLUMNS	64
#define CDP1864_VISIBLE_LINES	192

#define CDP1864_HBLANK_END		 1 * 8
#define CDP1864_HBLANK_START	13 * 8
#define CDP1864_HSYNC_START		 0 * 8
#define CDP1864_HSYNC_END		 1 * 8
#define CDP1864_SCREEN_START	 4 * 8
#define CDP1864_SCREEN_END		12 * 8
#define CDP1864_SCREEN_WIDTH	14 * 8

#define CDP1864_TOTAL_SCANLINES				312

#define CDP1864_SCANLINE_VBLANK_START		CDP1864_TOTAL_SCANLINES - 4
#define CDP1864_SCANLINE_VBLANK_END			20
#define CDP1864_SCANLINE_VSYNC_START		0
#define CDP1864_SCANLINE_VSYNC_END			4
#define CDP1864_SCANLINE_DISPLAY_START		60 // ???
#define CDP1864_SCANLINE_DISPLAY_END		CDP1864_SCANLINE_DISPLAY_START + CDP1864_VISIBLE_LINES
#define CDP1864_SCANLINE_INT_START			CDP1864_SCANLINE_DISPLAY_START - 2
#define CDP1864_SCANLINE_INT_END			CDP1864_SCANLINE_DISPLAY_START
#define CDP1864_SCANLINE_EFX_TOP_START		CDP1864_SCANLINE_DISPLAY_START - 4
#define CDP1864_SCANLINE_EFX_TOP_END		CDP1864_SCANLINE_DISPLAY_START
#define CDP1864_SCANLINE_EFX_BOTTOM_START	CDP1864_SCANLINE_DISPLAY_END - 4
#define CDP1864_SCANLINE_EFX_BOTTOM_END		CDP1864_SCANLINE_DISPLAY_END

typedef void (*cdp1864_on_int_changed_func) (const device_config *device, int level);
#define CDP1864_ON_INT_CHANGED(name) void name(const device_config *device, int level)

typedef void (*cdp1864_on_dmao_changed_func) (const device_config *device, int level);
#define CDP1864_ON_DMAO_CHANGED(name) void name(const device_config *device, int level)

typedef void (*cdp1864_on_efx_changed_func) (const device_config *device, int level);
#define CDP1864_ON_EFX_CHANGED(name) void name(const device_config *device, int level)

#define CDP1864		DEVICE_GET_INFO_NAME(cdp1864)

#define MDRV_CDP1864_ADD(_tag, _clock, _config) \
	MDRV_DEVICE_ADD(_tag, CDP1864, _clock) \
	MDRV_DEVICE_CONFIG(_config)

typedef enum _cdp1864_format cdp1864_format;
enum _cdp1864_format {
	CDP1864_NON_INTERLACED = 0,
	CDP1864_INTERLACED
};

/* interface */
typedef struct _cdp1864_interface cdp1864_interface;
struct _cdp1864_interface
{
	const char *screen_tag;		/* screen we are acting on */

	cdp1864_format interlace;	/* interlace */

	/* this gets called for every change of the INT pin (pin 36) */
	cdp1864_on_int_changed_func		on_int_changed;

	/* this gets called for every change of the DMAO pin (pin 37) */
	cdp1864_on_dmao_changed_func	on_dmao_changed;

	/* this gets called for every change of the EFX pin (pin 18) */
	cdp1864_on_efx_changed_func		on_efx_changed;

	double res_r;				/* red output resistor value */
	double res_g;				/* green output resistor value */
	double res_b;				/* blue output resistor value */
	double res_bkg;				/* background output resistor value */
};
#define CDP1864_INTERFACE(name) const cdp1864_interface (name)=

/* device interface */
DEVICE_GET_INFO( cdp1864 );

/* display on (0x69) */
READ8_DEVICE_HANDLER( cdp1864_dispon_r );

/* display off (0x6c) */
READ8_DEVICE_HANDLER( cdp1864_dispoff_r );

/* step background color (0x61) */
WRITE8_DEVICE_HANDLER( cdp1864_step_bgcolor_w );

/* load tone latch (0x64) */
WRITE8_DEVICE_HANDLER( cdp1864_tone_latch_w );

/* audio output enable */
void cdp1864_aoe_w(const device_config *device, int level);

/* DMA write */
void cdp1864_dma_w(const device_config *device, UINT8 data, int color_on, int rdata, int gdata, int bdata);

/* screen update */
void cdp1864_update(const device_config *device, bitmap_t *bitmap, const rectangle *cliprect);

#endif
