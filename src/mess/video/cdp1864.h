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

#ifndef __CDP1864_VIDEO__
#define __CDP1864_VIDEO__

#define CDP1864_CLK_FREQ		1750000.0
#define CDP1864_DEFAULT_LATCH	0x35

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
#define CDP1864_SCANLINE_DISPLAY_START		80 // ???
#define CDP1864_SCANLINE_DISPLAY_END		CDP1864_SCANLINE_DISPLAY_START + CDP1864_VISIBLE_LINES
#define CDP1864_SCANLINE_INT_START			CDP1864_SCANLINE_DISPLAY_START - 2
#define CDP1864_SCANLINE_INT_END			CDP1864_SCANLINE_DISPLAY_START
#define CDP1864_SCANLINE_EFX_TOP_START		CDP1864_SCANLINE_DISPLAY_START - 4
#define CDP1864_SCANLINE_EFX_TOP_END		CDP1864_SCANLINE_DISPLAY_START
#define CDP1864_SCANLINE_EFX_BOTTOM_START	CDP1864_SCANLINE_DISPLAY_END - 4
#define CDP1864_SCANLINE_EFX_BOTTOM_END		CDP1864_SCANLINE_DISPLAY_END

#define CDP1864_CYCLES_DMA_START	2*8
#define CDP1864_CYCLES_DMA_ACTIVE	8*8
#define CDP1864_CYCLES_DMA_WAIT		6*8

typedef struct CDP1864_interface
{
	double res_r; // red
	double res_g; // green
	double res_b; // blue
	double res_bkg; // background
	int (*colorram_r)(UINT16 addr);
} CDP1864_interface;

MACHINE_RESET( cdp1864 );
VIDEO_START( cdp1864 );
VIDEO_UPDATE( cdp1864 );

 READ8_HANDLER( cdp1864_dispon_r );
WRITE8_HANDLER( cdp1864_dispoff_w );

void cdp1864_dma_w(UINT8 data);

void cdp1864_audio_output_enable(int value);
void cdp1864_reset(void);

void cdp1864_configure(const CDP1864_interface *intf);

READ8_HANDLER( cdp1864_dispon_r );
READ8_HANDLER( cdp1864_dispoff_r );
WRITE8_HANDLER( cdp1864_step_bgcolor_w );
WRITE8_HANDLER( cdp1864_tone_latch_w );

VIDEO_UPDATE( cdp1864 );

#endif
