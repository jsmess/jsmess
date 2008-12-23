/*

				RCA CDP1862 COS/MOS Color Generator Controller

								_______  _______
					RD	 1	---|	   \/		|---  24  Vdd
			    _RESET	 2	---|				|---  23  R LUM
			      _CON	 3	---|				|---  22  G LUM
				 B CHR	 4	---|				|---  21  GD
				 B LUM	 5	---|				|---  20  BLG LUM
				   BKG	 6	---|	CDP1862C	|---  19  G CHR
			   _LD CLK	 7	---|	top view	|---  18  R CHR
				   STP	 8	---|				|---  17  BKG CHR
			   CLK OUT	 9	---|				|---  16  BD
			     _SYNC	10	---|			    |---  15  BURST
			    LUM IN	11	---|				|---  14  _XTAL
				   Vss	12	---|________________|---  13  XTAL


			   http://homepage.mac.com/ruske/cosmacelf/cdp1862.pdf

*/

#ifndef __CDP1862__
#define __CDP1862__

#define CPD1862_CLOCK	XTAL_7_15909MHz

#define CDP1862		DEVICE_GET_INFO_NAME(cdp1862)

/* interface */
typedef struct _cdp1862_interface cdp1862_interface;
struct _cdp1862_interface
{
	const char *screen_tag;		/* screen we are acting on */

	double res_r;				/* red output resistor value */
	double res_g;				/* green output resistor value */
	double res_b;				/* blue output resistor value */
	double res_bkg;				/* background output resistor value */
};
#define CDP1862_INTERFACE(name) const cdp1862_interface (name)=

/* device interface */
DEVICE_GET_INFO( cdp1862 );

/* step background color */
WRITE8_DEVICE_HANDLER( cdp1862_bkg_w );

/* DMA write */
void cdp1862_dma_w(const device_config *device, UINT8 data, int color_on, int rdata, int gdata, int bdata);

/* screen update */
void cdp1862_update(const device_config *device, bitmap_t *bitmap, const rectangle *cliprect);

#endif
