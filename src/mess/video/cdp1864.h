/*

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


*/

#define CDP1864_CLK_FREQ		1750000.0
#define CDP1864_DEFAULT_LATCH	0x35

#define CDP1864_SCREEN_WIDTH	14 * 8
#define CDP1864_SCANLINES		312

#define CDP1864_FPS				(CDP1864_CLK_FREQ / CDP1864_SCREEN_WIDTH) / CDP1864_SCANLINES

typedef struct
{
	int display;
	int bgcolor;
	int bgcolseq[4];
} CDP1864_CONFIG;

void cdp1864_set_background_color_sequence_w(int color[]);
void cdp1864_audio_output_w(int value);
void cdp1864_enable_w(int value);
void cdp1864_reset_w(void);

 READ8_HANDLER( cdp1864_audio_enable_r );
 READ8_HANDLER( cdp1864_audio_disable_r );
WRITE8_HANDLER( cdp1864_step_background_color_w );
WRITE8_HANDLER( cdp1864_tone_divisor_latch_w );

PALETTE_INIT( tmc2000 );
PALETTE_INIT( tmc2000e );
VIDEO_UPDATE( cdp1864 );
