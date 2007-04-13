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

#ifndef __CDP1861_VIDEO__
#define __CDP1861_VIDEO__

#define CDP1861_VISIBLE_COLUMNS	64
#define CDP1861_VISIBLE_LINES	128

#define CDP1861_HBLANK_END		 4 * 8
#define CDP1861_HBLANK_START	12 * 8
#define CDP1861_HSYNC_START		 0 * 8
#define CDP1861_HSYNC_END		 3 * 8
#define CDP1861_SCREEN_WIDTH	14 * 8

#define CDP1861_TOTAL_SCANLINES				262

#define CDP1861_SCANLINE_VBLANK_START		208
#define CDP1861_SCANLINE_VBLANK_END			80
#define CDP1861_SCANLINE_VSYNC_START		16
#define CDP1861_SCANLINE_VSYNC_END			0
#define CDP1861_SCANLINE_INT_START			CDP1861_SCANLINE_VBLANK_END - 2
#define CDP1861_SCANLINE_INT_END			CDP1861_SCANLINE_VBLANK_END
#define CDP1861_SCANLINE_EFX_TOP_START		CDP1861_SCANLINE_VBLANK_END - 4
#define CDP1861_SCANLINE_EFX_TOP_END		CDP1861_SCANLINE_VBLANK_END
#define CDP1861_SCANLINE_EFX_BOTTOM_START	CDP1861_SCANLINE_VBLANK_START - 4
#define CDP1861_SCANLINE_EFX_BOTTOM_END		CDP1861_SCANLINE_VBLANK_START

#define CDP1861_CYCLES_INT_DELAY	29*8
#define CDP1861_CYCLES_DMAOUT_LOW	8*8
#define CDP1861_CYCLES_DMAOUT_HIGH	6*8

MACHINE_RESET( cdp1861 );
VIDEO_START( cdp1861 );
VIDEO_UPDATE( cdp1861 );

READ8_HANDLER( cdp1861_dispon_r );
WRITE8_HANDLER( cdp1861_dispoff_w );

void cdp1861_dma_w(UINT8 data);
void cdp1861_sc(int state);

#endif
