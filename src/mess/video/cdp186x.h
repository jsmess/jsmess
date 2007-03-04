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

#define CDP1864_CLK_FREQ		1750000
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

/*

						________________											________________
		   TPA	 1	---|	   \/		|---  40  Vdd		   PREDISPLAY_	 1	---|	   \/		|---  40  Vdd
		   TPB	 2	---|				|---  39  PMSEL	 		 *DISPLAY_	 2	---|				|---  39  PAL/NTSC_
		  MRD_	 3	---|				|---  38  PMWR_				   PCB	 3	---|				|---  38  CPUCLK
		  MWR_	 4	---|				|---  37  *CMSEL			  CCB1	 4	---|				|---  37  XTAL (DOT)
		 MA0/8	 5	---|				|---  36  CMWR_				  BUS7	 5	---|				|---  36  XTAL (DOT)_
		 MA1/9	 6	---|				|---  35  PMA0				  CCB0	 6	---|				|---  35  *ADDRSTB_
		MA2/10	 7	---|				|---  34  PMA1				  BUS6	 7	---|				|---  34  MRD_
		MA3/11	 8	---|				|---  33  PMA2				  CDB5	 8	---|				|---  33  TPB
		MA4/12	 9	---|				|---  32  PMA3				  BUS5	 9	---|				|---  32  *CMSEL
		MA5/13	10	---|	CDP1869C    |---  31  PMA4				  CDB4	10	---|  CDP1870/76C   |---  31  BURST
		MA6/14	11	---|	top view	|---  30  PMA5				  BUS4	11	---|	top view	|---  30  *H SYNC_
		MA7/15	12	---|				|---  29  PMA6				  CDB3	12	---|				|---  29  COMPSYNC_
			N0	13	---|				|---  28  PMA7				  BUS3	13	---|				|---  28  LUM /	(RED)^
			N1	14	---|				|---  27  PMA8				  CDB2	14	---|				|---  27  PAL CHROM	/ (BLUE)^
			N2	15	---|				|---  26  PMA9				  BUS2	15	---|				|---  26  NTSC CHROM / (GREEN)^
	  *H SYNC_	16	---|				|---  25  CMA3/PMA10		  CDB1 	16	---|				|---  25  XTAL_ (CHROM)
	 *DISPLAY_	17	---|				|---  24  CMA2				  BUS1	17	---|				|---  24  XTAL (CHROM)
	 *ADDRSTB_	18	---|				|---  23  CMA1				  CDB0	18	---|				|---  23  EMS_
		 SOUND	19	---|				|---  22  CMA0				  BUS0	19	---|				|---  22  EVS_
		   VSS	20	---|________________|---  21  *N=3_				   Vss	20	---|________________|---  21  *N=3_


				 * = INTERCHIP CONNECTIONS		^ = FOR THE RGB BOND-OUT OPTION (CDP1876C)		_ = ACTIVE LOW

*/

#define CDP1869_DOT_CLK_PAL			5626000
#define CDP1869_DOT_CLK_NTSC		5670000
#define CDP1869_COLOR_CLK_PAL		8867236
#define CDP1869_COLOR_CLK_NTSC		7159090

#define CDP1869_CPU_CLK_PAL			CDP1869_DOT_CLK_PAL / 2
#define CDP1869_CPU_CLK_NTSC		CDP1869_DOT_CLK_NTSC / 2

#define CDP1869_CHAR_WIDTH			6
#define CDP1869_CHAR_HEIGHT_PAL		9
#define CDP1869_CHAR_HEIGHT_NTSC	8

#define CDP1869_SCREEN_WIDTH		(60 * CDP1869_CHAR_WIDTH)
#define CDP1869_SCANLINES_PAL		312
#define CDP1869_SCANLINES_NTSC		262

#define CDP1869_FPS_PAL				(CDP1869_DOT_CLK_PAL / CDP1869_SCREEN_WIDTH) / CDP1869_SCANLINES_PAL
#define CDP1869_FPS_NTSC			(CDP1869_DOT_CLK_NTSC / CDP1869_SCREEN_WIDTH) / CDP1869_SCANLINES_NTSC

typedef struct
{
	bool ntsc_pal;	// 0 = NTSC, 1 = PAL
	bool dispoff, toneoff, wnoff;
	UINT8 tonediv, tonefreq, toneamp;
	UINT8 wnfreq, wnamp;
	bool fresvert, freshorz;
	bool dblpage, line16, line9, cmem, cfc;
	UINT8 col, bkg;
	UINT16 pma, hma;
	UINT8 fgcolor; // HACK
} CDP1869_CONFIG;

void cdp1869_instruction_w(int which, int n, int data);

WRITE8_HANDLER( cdp1869_videoram_w );

PALETTE_INIT( cdp1869 );
VIDEO_START( cdp1869 );
VIDEO_UPDATE( cdp1869 );
