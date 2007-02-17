#include "driver.h"
#include "video/generic.h"
#include "cpu/cdp1802/cdp1802.h"
#include "video/cdp186x.h"
#include "sound/beep.h"

/*

	RCA CDP1864 Video Chip

	http://homepage.mac.com/ruske/cosmacelf/cdp1864.pdf

*/

static CDP1864_CONFIG cdp1864;

PALETTE_INIT( tmc2000 )
{
	int background_color_sequence[] = { 5, 7, 6, 3 };

	palette_set_color(machine,  0, 0x4c, 0x96, 0x1c ); // white
	palette_set_color(machine,  1, 0x4c, 0x00, 0x1c ); // purple
	palette_set_color(machine,  2, 0x00, 0x96, 0x1c ); // cyan
	palette_set_color(machine,  3, 0x00, 0x00, 0x1c ); // blue
	palette_set_color(machine,  4, 0x4c, 0x96, 0x00 ); // yellow
	palette_set_color(machine,  5, 0x4c, 0x00, 0x00 ); // red
	palette_set_color(machine,  6, 0x00, 0x96, 0x00 ); // green
	palette_set_color(machine,  7, 0x00, 0x00, 0x00 ); // black

	cdp1864_set_background_color_sequence_w(background_color_sequence);
}

PALETTE_INIT( tmc2000e )	// TODO: incorrect colors?
{
	int background_color_sequence[] = { 2, 0, 1, 4 };

	palette_set_color(machine, 0, 0x00, 0x00, 0x00 ); // black		  0 % of max luminance
	palette_set_color(machine, 1, 0x00, 0x97, 0x00 ); // green		 59
	palette_set_color(machine, 2, 0x00, 0x00, 0x1c ); // blue		 11
	palette_set_color(machine, 3, 0x00, 0xb3, 0xb3 ); // cyan		 70
	palette_set_color(machine, 4, 0x4c, 0x00, 0x00 ); // red		 30
	palette_set_color(machine, 5, 0xe3, 0xe3, 0x00 ); // yellow		 89
	palette_set_color(machine, 6, 0x68, 0x00, 0x68 ); // magenta	 41
	palette_set_color(machine, 7, 0xff, 0xff, 0xff ); // white		100
	
	cdp1864_set_background_color_sequence_w(background_color_sequence);
}

void cdp1864_set_background_color_sequence_w(int color[])
{
	int i;
	for (i = 0; i < 4; i++)
		cdp1864.bgcolseq[i] = color[i];
}

WRITE8_HANDLER( cdp1864_step_background_color_w )
{
	cdp1864.display = 1;
	if (++cdp1864.bgcolor > 3) cdp1864.bgcolor = 0;
}

WRITE8_HANDLER( cdp1864_tone_divisor_latch_w )
{
	beep_set_frequency(0, CDP1864_CLK_FREQ / 8 / 4 / (data + 1) / 2);
}

void cdp1864_audio_output_w(int value)
{
	beep_set_state(0, value);
}

READ8_HANDLER( cdp1864_audio_enable_r )
{
	beep_set_state(0, 1);
	return 0;
}

READ8_HANDLER( cdp1864_audio_disable_r )
{
	beep_set_state(0, 0);
	return 0;
}

void cdp1864_enable_w(int value)
{
	cdp1864.display = value;
}

void cdp1864_reset_w(void)
{
	int i;

	cdp1864.bgcolor = 0;

	for (i = 0; i < 4; i++)
		cdp1864.bgcolseq[i] = i;

	cdp1864_audio_output_w(0);
	cdp1864_tone_divisor_latch_w(0, CDP1864_DEFAULT_LATCH);
}

VIDEO_UPDATE( cdp1864 )
{
	fillbitmap(bitmap, cdp1864.bgcolseq[cdp1864.bgcolor], cliprect);
	// TODO: draw videoram to screen using DMA
	return 0;
}

/*

	RCA CDP1869/70/76 Video Interface System (VIS)

	http://homepage.mac.com/ruske/cosmacelf/cdp1869.pdf

*/

static tilemap *normal_tilemap, *double_tilemap;

static CDP1869_CONFIG cdp1869;

WRITE8_HANDLER( cdp1869_videoram_w )
{
	videoram[offset] = data;
	colorram[offset] = cdp1869.fgcolor;
}

static unsigned short colortable_cdp1869[] =
{
	0x00, 0x00,	0x00, 0x01,	0x00, 0x02,	0x00, 0x03,	0x00, 0x04,	0x00, 0x05,	0x00, 0x06,	0x00, 0x07
};

PALETTE_INIT( cdp1869 )
{
	palette_set_color(machine, 0, 0x00, 0x00, 0x00 ); // black		  0 % of max luminance
	palette_set_color(machine, 1, 0x00, 0x97, 0x00 ); // green		 59
	palette_set_color(machine, 2, 0x00, 0x00, 0x1c ); // blue		 11
	palette_set_color(machine, 3, 0x00, 0xb3, 0xb3 ); // cyan		 70
	palette_set_color(machine, 4, 0x4c, 0x00, 0x00 ); // red		 30
	palette_set_color(machine, 5, 0xe3, 0xe3, 0x00 ); // yellow		 89
	palette_set_color(machine, 6, 0x68, 0x00, 0x68 ); // magenta	 41
	palette_set_color(machine, 7, 0xff, 0xff, 0xff ); // white		100

	memcpy(colortable, colortable_cdp1869, sizeof(colortable_cdp1869));
}

static int blink;

static void get_normal_tile_info(int tile_index)
{
	int offs = tile_index + cdp1869.hma;
	int addr = (offs >= 960) ? (offs - 960) : offs;
	int code = videoram[addr];
	int color = colorram[addr] & 0x07;

	// HACK to make the cursor blink, cdp1869.pdf doesn't cover blinking so it's all guesswork
	if (colorram[addr] & 0x08)
	{
		if (blink > 50)	// one second hidden
		{
			color = cdp1869.bkg;
		}
		if (blink > 100) // one second visible
		{
			blink = 0;
		}
	}

	SET_TILE_INFO(0, code, color, 0)
}

static void get_double_tile_info(int tile_index)
{
	int offs = tile_index + cdp1869.hma;
	int addr = (offs >= 240) ? (offs - 240) : offs;
	int code = videoram[addr];
	int color = colorram[addr] & 0x07;

	if (colorram[addr] & 0x08)
	{
		if (blink > 50)
		{
			color = cdp1869.bkg;
		}
		if (blink > 100)
		{
			blink = 0;
		}
	}

	SET_TILE_INFO(1, code, color, 0)
}

VIDEO_START( cdp1869 )
{
	colorram = auto_malloc(videoram_size);
	memset(colorram, 0, videoram_size);

	normal_tilemap = tilemap_create(get_normal_tile_info, tilemap_scan_rows, 
		TILEMAP_TRANSPARENT, 6, 9, 40, 24);

	double_tilemap = tilemap_create(get_double_tile_info, tilemap_scan_rows, 
		TILEMAP_TRANSPARENT, 12, 18, 20, 24);

	tilemap_set_transparent_pen(normal_tilemap, 0);
	tilemap_set_transparent_pen(double_tilemap, 0);

	tilemap_set_scrolldx(normal_tilemap, 60, 0);
	tilemap_set_scrolldy(normal_tilemap, 44, 0);
	tilemap_set_scrolldx(double_tilemap, 60, 0);
	tilemap_set_scrolldy(double_tilemap, 44, 0);

	cdp1869.ntsc_pal = 1;

	return 0;
}

VIDEO_UPDATE( cdp1869 )
{
	rectangle clip;

	clip.min_x = Machine->screen[0].visarea.min_x + 60;
	clip.max_x = Machine->screen[0].visarea.max_x - 36;
	clip.min_y = Machine->screen[0].visarea.min_y + 44;
	clip.max_y = Machine->screen[0].visarea.max_y - 48;

	tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);

	if (cdp1869.dispoff)
		fillbitmap(bitmap, get_black_pen(machine), cliprect);
	else
	{
		fillbitmap(bitmap, cdp1869.bkg, cliprect);

		if (cdp1869.freshorz && cdp1869.fresvert)
			tilemap_draw(bitmap, &clip, normal_tilemap, 0, 0);
		else
			tilemap_draw(bitmap, &clip, double_tilemap, 0, 0);
	}

	blink++;
	return 0;
}

void cdp1869_set_tone_volume(int which, int value)
{
	beep_set_volume(0, value);
}

void cdp1869_set_tone_frequency(int which, int value)
{
	beep_set_frequency(0, value);
}

void cdp1869_set_noise_volume(int which, int value)
{
	// TODO: white noise
}

void cdp1869_set_noise_frequency(int which, int value)
{
	// TODO: white noise
}

void cdp1869_instruction_w(int which, int n, int data)
{
	int cpuclk = cdp1869.ntsc_pal ? CDP1869_CPU_CLK_PAL : CDP1869_CPU_CLK_NTSC;

	switch (n)
	{
	case 2:
		// set character color
		cdp1869.fgcolor = data;
		break;
	case 3:
		/*
			  bit	description

				0	bkg green
				1	bkg blue
				2	bkg red
				3	cfc
				4	disp off
				5	colb0
				6	colb1
				7	fres horz
		*/

		cdp1869.bkg = (data & 0x07);
		cdp1869.cfc = (data & 0x08) >> 3;
		cdp1869.dispoff = (data & 0x10) >> 4;
		cdp1869.col = (data & 0x60) >> 5;
		cdp1869.freshorz = (data & 0x80) >> 7;
		break;

	case 4:
		/*
			  bit	description

			    0	tone amp 2^0
				1	tone amp 2^1
				2	tone amp 2^2
				3	tone amp 2^3
				4	tone freq sel0
				5	tone freq sel1
				6	tone freq sel2
				7	tone off
				8	tone / 2^0
				9	tone / 2^1
			   10	tone / 2^2
			   11	tone / 2^3
			   12	tone / 2^4
			   13	tone / 2^5
			   14	tone / 2^6
			   15	always 0
		*/

		cdp1869.toneamp = (data & 0x0f);
		cdp1869.tonefreq = (data & 0x70) >> 4;
		cdp1869.toneoff = (data & 0x80) >> 7;
		cdp1869.tonediv = (data & 0x7f00) >> 8;

		cdp1869_set_tone_volume(0, cdp1869.toneoff ? 0 : (1.666 * cdp1869.toneamp));
		cdp1869_set_tone_frequency(0, cpuclk / (512 >> cdp1869.tonefreq) / (cdp1869.tonediv + 1));
		break;

	case 5:
		/*
			  bit	description

			    0	cmem access mode
				1	x
				2	x
				3	9-line
				4	x
				5	16 line hi-res
				6	double page
				7	fres vert
				8	wn amp 2^0
				9	wn amp 2^1
			   10	wn amp 2^2
			   11	wn amp 2^3
			   12	wn freq sel0
			   13	wn freq sel1
			   14	wn freq sel2
			   15	wn off
		*/

		cdp1869.cmem = (data & 0x01);
		cdp1869.line9 = (data & 0x08) >> 3;
		cdp1869.line16 = (data & 0x20) >> 5;
		cdp1869.dblpage = (data & 0x40) >> 6;
		cdp1869.fresvert = (data & 0x80) >> 7;
		cdp1869.wnamp = (data & 0x0f00) >> 8;
		cdp1869.wnfreq = (data & 0x7000) >> 12;
		cdp1869.wnoff = (data & 0x8000) >> 15;

		cdp1869_set_noise_volume(0, cdp1869.wnoff ? 0 : (1.666 * cdp1869.wnamp));
		cdp1869_set_noise_frequency(0, cpuclk / (4096 >> cdp1869.wnfreq));
		break;

	case 6:
		/*
			  bit	description

			    0	pma0 reg
			    1	pma1 reg
			    2	pma2 reg
			    3	pma3 reg
			    4	pma4 reg
				5	pma5 reg
			    6	pma6 reg
			    7	pma7 reg
			    8	pma8 reg
			    9	pma9 reg
			   10	pma10 reg
			   11	x
			   12	x
			   13	x
			   14	x
			   15	x
		*/

		cdp1869.pma = data & 0x7ff;
		break;

	case 7:
		/*
			  bit	description

			    0	x
			    1	x
			    2	hma2 reg
			    3	hma3 reg
			    4	hma4 reg
				5	hma5 reg
			    6	hma6 reg
			    7	hma7 reg
			    8	hma8 reg
			    9	hma9 reg
			   10	hma10 reg
			   11	x
			   12	x
			   13	x
			   14	x
			   15	x
		*/

		cdp1869.hma = data & 0x7fc;
		break;
	}
}
