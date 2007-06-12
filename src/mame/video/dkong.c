/***************************************************************************

  video.c

  Functions to emulate the video hardware of the machine.


***************************************************************************/

#include "driver.h"
#include "includes/dkong.h"

#define RADARSCP_BCK_COL_OFFSET			256
#define RADARSCP_GRID_COL_OFFSET		(RADARSCP_BCK_COL_OFFSET + 256)
#define RADARSCP_STAR_COL				(RADARSCP_GRID_COL_OFFSET + 8)

static tilemap *bg_tilemap;

static const UINT8 *color_codes;

static int line_cnt=512-VTOTAL;
static UINT8 sig30Hz = 0;
static UINT8 grid_sig = 0;
static UINT8 rflip_sig = 0;
static UINT8 star_ff = 0;

static double blue_level = 0;
static const double cd4049_vl = 1.5/5.0;
static const double cd4049_vh = 3.5/5.0;
static const double cd4049_al = 0.01;
static double cd4049_a, cd4049_b;

/* Save state relevant */

static UINT8 gfx_bank, palette_bank;
static UINT8 grid_on;
static UINT8	snd02_enable;
static UINT16	grid_col;

/***************************************************************************

  Convert the color PROMs into a more useable format.

  Donkey Kong has two 256x4 palette PROMs and one 256x4 PROM which contains
  the color codes to use for characters on a per row/column basis (groups of
  of 4 characters in the same column - actually row, since the display is
  rotated)
  The palette PROMs are connected to the RGB output this way:

    5V  -- 470 ohm resistor -- inverter  -- RED
  bit 3 -- 220 ohm resistor -- inverter  -- RED
        -- 470 ohm resistor -- inverter  -- RED
        -- 1  kohm resistor -- inverter  -- RED

   5V  -- 470 ohm resistor -- inverter  -- GREEN
  bit 0 -- 220 ohm resistor -- inverter  -- GREEN
  bit 3 -- 470 ohm resistor -- inverter  -- GREEN
        -- 1  kohm resistor -- inverter  -- GREEN
    5V  -- 680 ohm resistor -- inverter  -- BLUE
        -- 220 ohm resistor -- inverter  -- BLUE
  bit 0 -- 470 ohm resistor -- inverter  -- BLUE

  After the mixing stage there is a darlington circuit with approx. linear
  transfer function for red and green. Minimum output voltage is 0.9 Volt.
  Blue is transferred through just one transistor. Here we have to
  substract 0.7 Volts. This signal (0..5V) is inverted by a amplifier
  circuit with an emitter lowpass RC. The R is a variable resistor.
  So any calculations done here may be void in reality ...
***************************************************************************/

typedef struct _res_dac_channel_info res_dac_channel_info;
struct _res_dac_channel_info {
	double	minout;
	double	cut;
	double	vBias;
	double	rBias;
	double	rGnd;
	int		num;
	double R[8];
};

typedef struct _res_dac_info res_dac_info;
struct _res_dac_info {
	double	vcc;
	double	vOL;
	double	vOH;
	UINT8	OpenCol;
	res_dac_channel_info rgb[3];
};

static const res_dac_info dkong_dac_info =
{
	5, 0.35, 3.4, 1,
	{
		{ 0.9, 0.0, 5.0, 470, 0, 3, { 1000, 470, 220 } },
		{ 0.9, 0.0, 5.0, 470, 0, 3, { 1000, 470, 220 } },
		{ 0.0, 0.7, 5.0, 680, 0, 2, {  470, 220,   0 } }  // dkong
	}
};

static const res_dac_info dkong3_dac_info =
{
	5, 0.35, 3.4, 1,
	{
		{ 0.9, 0.0, 5.0, 470,      0, 4, { 2200, 1000, 470, 220 } },
		{ 0.9, 0.0, 5.0, 470,      0, 4, { 2200, 1000, 470, 220 } },
		{ 0.9, 0.0, 5.0, 470,      0, 4, { 2200, 1000, 470, 220 } }
	}
};

static const res_dac_info radarscp_dac_info =
{
	5, 0.35, 3.4, 1,
	{
		{ 0.9, 0.0, 3.4, 470,      0, 3, { 1000, 470, 220 } },
		{ 0.9, 0.0, 3.4, 470,      0, 3, { 1000, 470, 220 } },
		{ 0.9, 0.0, 3.4, 680, 150000, 2, {  470, 220,   0 } }    // radarscp
	}
};

/* Radarscp star color */

static const res_dac_info radarscp_stars_dac_info =
{
	5, 0.35, 3.4, 0,
	{
		{ 0.9, 0.0, 5.0, 4700, 470, 0, { 0 } },
		{ 0.9, 0.0, 5.0,    1,   0, 0, { 0 } },	// dummy
		{ 0.9, 0.0, 5.0,    1,   0, 0, { 0 } },	// dummy
	}
};

/* Dummy struct to generate background palette entries */

static const res_dac_info radarscp_bck_dac_info =
{
	5, 0.0, 5.0, 0,
	{
		{ 0.9, 0.0, 5.0,    1,   0, 0, { 0 } },	// bias/gnd exist in schematics, but not readable
		{ 0.9, 0.0, 5.0,    1,   0, 0, { 0 } },	// bias/gnd exist in schematics, but not readable
		{ 0.9, 0.0, 5.0,    0,   0, 8, { 128,64,32,16,8,4,2,1 } },	// dummy
	}
};

/* Dummy struct to generate grid palette entries */

static const res_dac_info radarscp_grid_dac_info =
{
	5, 0.35, 3.4, 0,
	{
		{ 0.9, 0.0, 5.0,    0,   0, 1, { 1 } },	// dummy
		{ 0.9, 0.0, 5.0,    0,   0, 1, { 1 } },	// dummy
		{ 0.9, 0.0, 5.0,    0,   0, 1, { 1 } },	// dummy
	}
};

static int ResDAC(int val, int channel, const res_dac_info *di)
{
	double rTotal=0.0;
	double v = 0;
	int i;

	for (i=0; i<di->rgb[channel].num; i++)
		if (di->rgb[channel].R[i] != 0.0)
		{
			if (di->OpenCol)
				rTotal += (((val >> i) & 1) ? 0 : 1.0 / di->rgb[channel].R[i]);
			else
			{
				rTotal += 1.0 / di->rgb[channel].R[i];
				v += (((val >> i) & 1) ? di->vOH : di->vOL ) / di->rgb[channel].R[i];
			}
		}
	if ( di->rgb[channel].rBias != 0.0 )
	{
		rTotal += 1.0 / di->rgb[channel].rBias;
		v += di->rgb[channel].vBias / di->rgb[channel].rBias;
	}
	if (di->rgb[channel].rGnd != 0.0)
		rTotal += 1.0 / di->rgb[channel].rGnd;
	rTotal = 1.0 / rTotal;
	v *= rTotal;
	v = MAX(di->rgb[channel].minout, v - di->rgb[channel].cut);
	v = di->vcc - v;
	return (int) (v *255 / di->vcc + 0.4);
}

/***************************************************************************

  PALETTE_INIT

***************************************************************************/

PALETTE_INIT( dkong )
{
	int i;

	for (i = 0;i < 256;i++)
	{
		int r,g,b;

		/* red component */
		r = ResDAC( (color_prom[256]>>1) & 0x07, 0, &dkong_dac_info );
		/* green component */
		g = ResDAC( ((color_prom[256]<<2) & 0x04) | ((color_prom[0]>>2) & 0x03), 1, &dkong_dac_info );
		/* blue component */
		b = ResDAC( (color_prom[0]>>0) & 0x03, 2, &dkong_dac_info );

		palette_set_color_rgb(machine,i,r,g,b);
		color_prom++;
	}

	color_prom += 256;
	/* color_prom now points to the beginning of the character color codes */
	color_codes = color_prom;	/* we'll need it later */
}

PALETTE_INIT( radarscp )
{
	int i;
	int r,g,b;

	for (i = 0;i < 256;i++)
	{

		/* red component */
		r = ResDAC( (color_prom[256]>>1) & 0x07, 0, &radarscp_dac_info );
		/* green component */
		g = ResDAC( ((color_prom[256]<<2) & 0x04) | ((color_prom[0]>>2) & 0x03), 1, &radarscp_dac_info );
		/* blue component */
		b = ResDAC( (color_prom[0]>>0) & 0x03, 2, &radarscp_dac_info );

		palette_set_color_rgb(machine,i,r,g,b);
		color_prom++;
	}
	/* Star color */
	r = ResDAC( 1, 0, &radarscp_stars_dac_info );
	g = ResDAC( 0, 1, &radarscp_stars_dac_info );
	b = ResDAC( 0, 2, &radarscp_stars_dac_info );
	palette_set_color_rgb(machine,RADARSCP_STAR_COL,r,g,b);

	/* Oscillating background */
	for (i = 0;i < 256;i++)
	{
		r = ResDAC( 0, 0, &radarscp_bck_dac_info );
		g = ResDAC( 0, 1, &radarscp_bck_dac_info );
		b = ResDAC( i, 2, &radarscp_bck_dac_info );

		palette_set_color_rgb(machine,RADARSCP_BCK_COL_OFFSET + i,r,g,b);
	}

	/* Grid */
	for (i = 0;i < 8;i++)
	{
		r = ResDAC( i & 1, 0, &radarscp_grid_dac_info );
		g = ResDAC( (i>>1) & 1, 1, &radarscp_grid_dac_info );
		b = ResDAC( (i>>2) & 1, 2, &radarscp_grid_dac_info );

		palette_set_color_rgb(machine,RADARSCP_GRID_COL_OFFSET + i,r,g,b);
	}



	color_prom += 256;
	/* color_prom now points to the beginning of the character color codes */
	color_codes = color_prom;	/* we'll need it later */
}
/***************************************************************************

  Convert the color PROMs into a more useable format.

  Donkey Kong 3 has two 512x8 palette PROMs and one 256x4 PROM which contains
  the color codes to use for characters on a per row/column basis (groups of
  of 4 characters in the same column - actually row, since the display is
  rotated)
  Interstingly, bytes 0-255 of the palette PROMs contain an inverted palette,
  as other Nintendo games like Donkey Kong, while bytes 256-511 contain a non
  inverted palette. This was probably done to allow connection to both the
  special Nintendo and a standard monitor.
  I don't know the exact values of the resistors between the PROMs and the
  RGB output, but they are probably the usual:

  Couriersud: Checked against the schematics.
              The proms are open collector proms and there is a pullup to
              +5V of 470 ohm.

  bit 7 -- 220 ohm resistor -- inverter  -- RED
        -- 470 ohm resistor -- inverter  -- RED
        -- 1  kohm resistor -- inverter  -- RED
        -- 2.2kohm resistor -- inverter  -- RED
        -- 220 ohm resistor -- inverter  -- GREEN
        -- 470 ohm resistor -- inverter  -- GREEN
        -- 1  kohm resistor -- inverter  -- GREEN
  bit 0 -- 2.2kohm resistor -- inverter  -- GREEN

  bit 3 -- 220 ohm resistor -- inverter  -- BLUE
        -- 470 ohm resistor -- inverter  -- BLUE
        -- 1  kohm resistor -- inverter  -- BLUE
  bit 0 -- 2.2kohm resistor -- inverter  -- BLUE

***************************************************************************/
PALETTE_INIT( dkong3 )
{

	int i;
	for (i = 0;i < 256;i++)
	{
		int r,g,b;

		/* red component */
		r = ResDAC( ( color_prom[0]>>4) & 0x0F, 0, &dkong3_dac_info );
		/* green component */
		g = ResDAC( ( color_prom[0]>>0) & 0x0F, 1, &dkong3_dac_info );
		/* blue component */
		b = ResDAC( (color_prom[256]>>0) & 0x0F, 2, &dkong3_dac_info );

		palette_set_color_rgb(machine,i,r,g,b);
		color_prom++;
	}

	color_prom += 256;
	/* color_prom now points to the beginning of the character color codes */
	color_codes = color_prom;	/* we'll need it later */
}

static TILE_GET_INFO( get_bg_tile_info )
{
	int code = videoram[tile_index] + 256 * gfx_bank;
	int color = (color_codes[tile_index % 32 + 32 * (tile_index / 32 / 4)] & 0x0f) + 0x10 * palette_bank;

	SET_TILE_INFO(0, code, color, 0)
}

/***************************************************************************

 I/O Handling

***************************************************************************/

WRITE8_HANDLER( dkong_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE8_HANDLER( radarscp_snd02_w )
{
	snd02_enable = data & 0x01;
}

WRITE8_HANDLER( dkongjr_gfxbank_w )
{
	if (gfx_bank != (data & 0x01))
	{
		gfx_bank = data & 0x01;
		tilemap_mark_all_tiles_dirty(bg_tilemap);
	}
}

WRITE8_HANDLER( dkong3_gfxbank_w )
{
	if (gfx_bank != (~data & 0x01))
	{
		gfx_bank = ~data & 0x01;
		tilemap_mark_all_tiles_dirty(bg_tilemap);
	}
}

WRITE8_HANDLER( dkong_palettebank_w )
{
	int newbank;

	newbank = palette_bank;

	if (data & 1)
		newbank |= 1 << offset;
	else
		newbank &= ~(1 << offset);

	if (palette_bank != newbank)
	{
		palette_bank = newbank;
		tilemap_mark_all_tiles_dirty(bg_tilemap);
	}
}

WRITE8_HANDLER( radarscp_grid_enable_w )
{
	grid_on = data & 0x01;
}

WRITE8_HANDLER( radarscp_grid_color_w )
{
	grid_col = (data & 0x07) ^ 0x07;
	popmessage("Gridcol: %d", grid_col);
}

WRITE8_HANDLER( dkong_flipscreen_w )
{
	flip_screen_set(~data & 0x01);
}

/***************************************************************************

  Draw the game screen in the given mame_bitmap.

***************************************************************************/

static void draw_sprites(mame_bitmap *bitmap, UINT32 mask_bank, UINT32 shift_bits)
{
	int offs;

	/* Draw the sprites. */
	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		if (spriteram[offs])
		{
			/* spriteram[offs + 2] & 0x40 is used by Donkey Kong 3 only */
			/* spriteram[offs + 2] & 0x30 don't seem to be used (they are */
			/* probably not part of the color code, since Mario Bros, which */
			/* has similar hardware, uses a memory mapped port to change */
			/* palette bank, so it's limited to 16 color codes) */

			int x,y;

			x = spriteram[offs + 3] - 8;
			y = 240 - spriteram[offs] + 7;

			if (flip_screen)
			{
				x = 240 - x;
				y = 240 - y;

				drawgfx(bitmap,Machine->gfx[1],
						(spriteram[offs + 1] & 0x7f) + ((spriteram[offs + 2] & mask_bank) << shift_bits),
						(spriteram[offs + 2] & 0x0f) + 16 * palette_bank,
						!(spriteram[offs + 2] & 0x80),!(spriteram[offs + 1] & 0x80),
						x,y,
						&Machine->screen[0].visarea,TRANSPARENCY_PEN,0);

				/* draw with wrap around - this fixes the 'beheading' bug */
				drawgfx(bitmap,Machine->gfx[1],
						(spriteram[offs + 1] & 0x7f) + ((spriteram[offs + 2] & mask_bank) << shift_bits),
						(spriteram[offs + 2] & 0x0f) + 16 * palette_bank,
						(spriteram[offs + 2] & 0x80),(spriteram[offs + 1] & 0x80),
						x-256,y,
						&Machine->screen[0].visarea,TRANSPARENCY_PEN,0);
			}
			else
			{
				drawgfx(bitmap,Machine->gfx[1],
						(spriteram[offs + 1] & 0x7f) + ((spriteram[offs + 2] & mask_bank) << shift_bits),
						(spriteram[offs + 2] & 0x0f) + 16 * palette_bank,
						(spriteram[offs + 2] & 0x80),(spriteram[offs + 1] & 0x80),
						x,y,
						&Machine->screen[0].visarea,TRANSPARENCY_PEN,0);

				/* draw with wrap around - this fixes the 'beheading' bug */
				drawgfx(bitmap,Machine->gfx[1],
						(spriteram[offs + 1] & 0x7f) + ((spriteram[offs + 2] & mask_bank) << shift_bits),
						(spriteram[offs + 2] & 0x0f) + 16 * palette_bank,
						(spriteram[offs + 2] & 0x80),(spriteram[offs + 1] & 0x80),
						x+256,y,
						&Machine->screen[0].visarea,TRANSPARENCY_PEN,0);
			}
		}
	}
}

/* The hardware designer must have been a real CD4049 fan
 * The blue signal is created by "abusing" the 4049
 * as an amplifier by operating it in the undefined area
 * between roughly 1.5 and 3.5V. The transfer function
 * below is an approximation but datasheets state
 * a wide range for the transfer function.
 *
 * SwitcherCad was not a real help since it can not
 * adequately model the 4049
 *
 * Sound02 will mix in noise into the periodic fluctuation
 * of the background: The 30Hz signal is used
 * to generate noise with an LS164. This signal is going
 * through a NAND (Signal RFLIP to video) and then XOR with 128V.
 * This should really be emulated using the discrete sound interface.
 * TODO: This should be part of the vblank routine
 */

INLINE double CD4049(double x)
{
	if (x>0)
	 	return exp(-cd4049_a * pow(x,cd4049_b));
	else
		return 1.0;
}

static void radarscp_step(void)
{
	/* Condensator is illegible in schematics for TRS2 board.
     * TRS1 board states 3.3u.
     */
	const double RC1= 2.2e3 * 22e-6;// 22e-6;
	const double RC2= 10e3 * 33e-6;
	const double RC31= 18e3 * 33e-6;
	const double RC32= (18e3 + 68e3) * 33e-6;
	const double RC4= 90e3 * 0.47e-6;
	const double dt= 1./60./(double) VTOTAL;
	const int period2 = ((long long)(PIXEL_CLOCK) * ( 33L * 68L )) / (long)10000000L / 3;  // period/2 in pixel ...
	static double cv1=0,cv2=0,vg1=0,vg2=0,vg3=0,vg3i=0,cv3=0,cv4=0;
	static int pixelcnt = 0;
	double diff;
	int sig;

	/* sound2 mixes in a 30Hz noise signal.
     * With the current model this has no real effect
     * Included for completeness
     */

	line_cnt++;
	if (line_cnt>=512)
		line_cnt=512-VTOTAL;
	if ( ( !(line_cnt & 0x40) && ((line_cnt+1) & 0x40) ) && (rand() > RAND_MAX/2))
		sig30Hz = (1-sig30Hz);
	rflip_sig = snd02_enable & sig30Hz;
	sig = rflip_sig ^ ((line_cnt & 0x80)>>7);
	if  (sig) // 128VF
		diff = (0.0 - cv1);
	else
		diff = (3.4 - cv1);
	diff = diff - diff*exp(0.0 - (1.0/RC1 * dt) );
	cv1 += diff;

	diff = (cv1 - cv2 - vg1);
	diff = diff - diff*exp(0.0 - (1.0/RC2 * dt) );
	cv2 += diff;

	vg1 = (cv1 - cv2)*0.9 + 0.1 * vg2;
	vg2 = 5*CD4049(vg1/5);

	/* on the real hardware, the gain would be 1.
     * This will not work here.
     */
	vg3i = 0.9*vg2 + 0.1 * vg3;
	vg3 = 5*CD4049(vg3i/5);

	blue_level = vg3;

	// Grid signal
	if (grid_on)
	{
		diff = (0.0 - cv3);
		diff = diff - diff*exp(0.0 - (1.0/RC32 * dt) );
	}
	else
	{
		diff = (5.0 - cv3);
		diff = diff - diff*exp(0.0 - (1.0/RC31 * dt) );
	}
	cv3 += diff;

	diff = (vg2 - 0.8 * cv3 - cv4);
	diff = diff - diff*exp(0.0 - (1.0/RC4 * dt) );
	cv4 += diff;

	if (CD4049(CD4049(vg2 - cv4))>2.4/5.0) //TTL - Level
		grid_sig = 0;
	else
		grid_sig = 1;

	/* stars */
	pixelcnt += HTOTAL;
	if (pixelcnt > period2 )
	{
		star_ff = !star_ff;
		pixelcnt = pixelcnt - period2;
	}

}

/* Actually the sound noise is a multivibrator with
 * a period of roughly 4.4 ms
 */

static void radarscp_draw_background(mame_bitmap *bitmap)
{
	const UINT8 *table = memory_region(REGION_GFX3);
	const int 		table_len = memory_region_length(REGION_GFX3);
	int 			x,y,counter,offset;
	UINT8 			draw_ok;
	UINT16 			*pixel;

	counter = 0;
	for (y=0; y<VBEND; y++)
		radarscp_step();
	y = Machine->screen[0].visarea.min_y;
	while (y <= Machine->screen[0].visarea.max_y)
	{
		radarscp_step();
		offset = ((-flip_screen) ^ rflip_sig) ? 0x000 : 0x400;
		x = Machine->screen[0].visarea.min_x;
		while ((counter < table_len) && ( x > 4 * (table[counter|offset] & 0x7f)))
		counter++;
		while (x <= Machine->screen[0].visarea.max_x)
		{
			pixel = BITMAP_ADDR16(bitmap, y, x);
			draw_ok = !(*pixel & 0x01) && !(*pixel & 0x02);
			if ((counter < table_len) && (x == 4 * (table[counter|offset] & 0x7f)))
			{
				if (draw_ok)
				{
					if ( star_ff && (table[counter|offset] & 0x80) )	/* star */
						*pixel = Machine->pens[RADARSCP_STAR_COL];
					else if (grid_sig && !(table[counter|offset] & 0x80))			/* radar */
						*pixel = Machine->pens[RADARSCP_GRID_COL_OFFSET+grid_col];
					else
						*pixel = Machine->pens[RADARSCP_BCK_COL_OFFSET + (int)(blue_level/5.0*255)];
				}
				counter++;
			}
			else if (draw_ok)
				*pixel = Machine->pens[RADARSCP_BCK_COL_OFFSET + (int)(blue_level/5.0*255)];
			x++;
		}
		while ((counter < table_len) && ( Machine->screen[0].visarea.max_x < 4 * (table[counter|offset] & 0x7f)))
			counter++;
		y++;
	}
	for (y=VBSTART; y<VTOTAL; y++)
		radarscp_step();
}

VIDEO_START( dkong )
{

	cd4049_b = (log(0.0 - log(cd4049_al)) - log(0.0 - log((1.0-cd4049_al))) ) / log(cd4049_vh/cd4049_vl);
	cd4049_a = log(0.0 - log(cd4049_al)) - cd4049_b * log(cd4049_vh);

	gfx_bank = 0;
	palette_bank = 0;

	state_save_register_global(gfx_bank);
	state_save_register_global(palette_bank);
	state_save_register_global(grid_on);

	/* this must be registered here - hmmm */
	state_save_register_global(flip_screen);

	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 8, 8, 32, 32);
	tilemap_set_scrolldx(bg_tilemap, 0, 128);
}

VIDEO_UPDATE( radarscp )
{

	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	draw_sprites(bitmap, 0x40, 1);
	radarscp_draw_background(bitmap);

	return 0;
}

VIDEO_UPDATE( dkong )
{
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	draw_sprites(bitmap, 0x40, 1);
	return 0;
}

VIDEO_UPDATE( pestplce )
{
	int offs;

	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);

	/* Draw the sprites. */
	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		if (spriteram[offs])
		{
			drawgfx(bitmap,machine->gfx[1],
					spriteram[offs + 2],
					(spriteram[offs + 1] & 0x0f) + 16 * palette_bank,
					spriteram[offs + 1] & 0x80,spriteram[offs + 1] & 0x40,
					spriteram[offs + 3] - 8,240 - spriteram[offs] + 8,
					cliprect,TRANSPARENCY_PEN,0);
		}
	}
	return 0;
}

VIDEO_UPDATE( spclforc )
{
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);

	/* it uses spriteram[offs + 2] & 0x10 for sprite bank */
	draw_sprites(bitmap, 0x10, 3);
	return 0;
}
