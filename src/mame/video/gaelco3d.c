/*************************************************************************

    Driver for Gaelco 3D games

    driver by Aaron Giles

**************************************************************************/

#include "driver.h"
#include "gaelco3d.h"
#include "video/poly.h"

UINT8 *gaelco3d_texture;
UINT8 *gaelco3d_texmask;
UINT32 gaelco3d_texture_size;
UINT32 gaelco3d_texmask_size;


#define MAX_POLYGONS		4096
#define MAX_POLYDATA		(MAX_POLYGONS * 21)

#define BILINEAR_FILTER		1			/* should be 1, but can be set to 0 for speed */

#define DISPLAY_TEXTURE		0
#define LOG_POLYGONS		0
#define DISPLAY_STATS		0

#define IS_POLYEND(x)		(((x) ^ ((x) >> 1)) & 0x4000)


static mame_bitmap *screenbits;
static mame_bitmap *zbuffer;
static UINT16 *palette;
static UINT32 *polydata_buffer;
static UINT32 polydata_start;
static UINT32 polydata_count;

static int polygons;
static int lastscan;
static int video_changed;

static osd_work_queue *work_queue;

static int this_skip;



/*************************************
 *
 *  Video init
 *
 *************************************/

static void gaelco3d_exit(running_machine *machine)
{
	osd_work_queue_free(work_queue);
}


VIDEO_START( gaelco3d )
{
	screenbits = auto_bitmap_alloc(machine->screen[0].width, machine->screen[0].height, machine->screen[0].format);

	zbuffer = auto_bitmap_alloc(machine->screen[0].width, machine->screen[0].height, BITMAP_FORMAT_INDEXED16);

	palette = auto_malloc(32768 * sizeof(palette[0]));
	polydata_buffer = auto_malloc(MAX_POLYDATA * sizeof(polydata_buffer[0]));

	work_queue = osd_work_queue_alloc(0);
	add_exit_callback(machine, gaelco3d_exit);

	/* save states */

	state_save_register_global_pointer(palette, 32768);
	state_save_register_global_pointer(polydata_buffer, MAX_POLYDATA);
	state_save_register_global(polydata_start);
	state_save_register_global(polydata_count);

	state_save_register_global(polygons);
	state_save_register_global(lastscan);

	state_save_register_bitmap("video", 0, "screenbits", screenbits);
	state_save_register_bitmap("video", 0, "zbuffer", zbuffer);
}



/*************************************
 *
 *  TMS32031 floating point conversion
 *
 *************************************/

typedef union int_double
{
	double d;
	float f[2];
	UINT32 i[2];
} int_double;


static float dsp_to_float(UINT32 val)
{
	INT32 _mantissa = val << 8;
	INT8 _exponent = (INT32)val >> 24;
	int_double id;

	if (_mantissa == 0 && _exponent == -128)
		return 0;
	else if (_mantissa >= 0)
	{
		int exponent = (_exponent + 127) << 23;
		id.i[0] = exponent + (_mantissa >> 8);
	}
	else
	{
		int exponent = (_exponent + 127) << 23;
		INT32 man = -_mantissa;
		id.i[0] = 0x80000000 + exponent + ((man >> 8) & 0x00ffffff);
	}
	return id.f[0];
}


/*************************************
 *
 *  Polygon rendering
 *
 *************************************/

static void *render_poly(void *param)
{
	UINT32 *polydata = param;

	/* these three parameters combine via A * x + B * y + C to produce a 1/z value */
	float ooz_dx = dsp_to_float(polydata[4]) * GAELCO3D_RESOLUTION_DIVIDE;
	float ooz_dy = dsp_to_float(polydata[3]) * GAELCO3D_RESOLUTION_DIVIDE;
	float ooz_base = dsp_to_float(polydata[8]);

	/* these three parameters combine via A * x + B * y + C to produce a u/z value */
	float uoz_dx = dsp_to_float(polydata[6]) * GAELCO3D_RESOLUTION_DIVIDE;
	float uoz_dy = dsp_to_float(polydata[5]) * GAELCO3D_RESOLUTION_DIVIDE;
	float uoz_base = dsp_to_float(polydata[9]);

	/* these three parameters combine via A * x + B * y + C to produce a v/z value */
	float voz_dx = dsp_to_float(polydata[2]) * GAELCO3D_RESOLUTION_DIVIDE;
	float voz_dy = dsp_to_float(polydata[1]) * GAELCO3D_RESOLUTION_DIVIDE;
	float voz_base = dsp_to_float(polydata[7]);

	/* this parameter is used to scale 1/z to a formal Z buffer value */
	float z0 = dsp_to_float(polydata[0]);

	/* these two parameters are the X,Y origin of the texture data */
	int xorigin = (INT32)polydata[12] >> 16;
	int yorigin = (INT32)(polydata[12] << 18) >> 18;

	int color = (polydata[10] & 0x7f) << 8;
	int midx = Machine->screen[0].width/2;
	int midy = Machine->screen[0].height/2;
	struct poly_vertex vert[3];
	rectangle clip;
	int i;

	// shut up the compiler
	(void)xorigin;
	(void)yorigin;

	if (LOG_POLYGONS && input_code_pressed(KEYCODE_LSHIFT))
	{
		int t;
		mame_printf_debug("poly: %12.2f %12.2f %12.2f %12.2f %12.2f %12.2f %12.2f %12.2f %12.2f %12.2f %08X %08X (%4d,%4d) %08X",
				(double)dsp_to_float(polydata[0]),
				(double)dsp_to_float(polydata[1]),
				(double)dsp_to_float(polydata[2]),
				(double)dsp_to_float(polydata[3]),
				(double)dsp_to_float(polydata[4]),
				(double)dsp_to_float(polydata[5]),
				(double)dsp_to_float(polydata[6]),
				(double)dsp_to_float(polydata[7]),
				(double)dsp_to_float(polydata[8]),
				(double)dsp_to_float(polydata[9]),
				polydata[10],
				polydata[11],
				(INT16)(polydata[12] >> 16), (INT16)(polydata[12] << 2) >> 2, polydata[12]);

		mame_printf_debug(" (%4d,%4d) %08X %08X", (INT16)(polydata[13] >> 16), (INT16)(polydata[13] << 2) >> 2, polydata[13], polydata[14]);
		for (t = 15; !IS_POLYEND(polydata[t - 2]); t += 2)
			mame_printf_debug(" (%4d,%4d) %08X %08X", (INT16)(polydata[t] >> 16), (INT16)(polydata[t] << 2) >> 2, polydata[t], polydata[t+1]);
		mame_printf_debug("\n");
	}

	/* compute the adjusted clip */
	clip.min_x = Machine->screen[0].visarea.min_x - midx;
	clip.min_y = midy - Machine->screen[0].visarea.max_y;
	clip.max_x = Machine->screen[0].visarea.max_x - midx;
	clip.max_y = midy - Machine->screen[0].visarea.min_y;

	/* extract the first two vertices */
	vert[0].x = (((INT32)polydata[13] >> 16) + (GAELCO3D_RESOLUTION_DIVIDE/2)) / GAELCO3D_RESOLUTION_DIVIDE;
	vert[0].y = (((INT32)(polydata[13] << 18) >> 18) + (GAELCO3D_RESOLUTION_DIVIDE/2)) / GAELCO3D_RESOLUTION_DIVIDE;
	vert[1].x = (((INT32)polydata[15] >> 16) + (GAELCO3D_RESOLUTION_DIVIDE/2)) / GAELCO3D_RESOLUTION_DIVIDE;
	vert[1].y = (((INT32)(polydata[15] << 18) >> 18) + (GAELCO3D_RESOLUTION_DIVIDE/2)) / GAELCO3D_RESOLUTION_DIVIDE;

	/* loop over the remaining verticies */
	for (i = 17; !IS_POLYEND(polydata[i - 2]) && i < 1000; i += 2)
	{
		offs_t endmask = gaelco3d_texture_size - 1;
		const struct poly_scanline_data *scans;
		UINT32 tex = polydata[11];
		int x, y;

		/* extract vertex 2 */
		vert[2].x = (((INT32)polydata[i] >> 16) + (GAELCO3D_RESOLUTION_DIVIDE/2)) / GAELCO3D_RESOLUTION_DIVIDE;
		vert[2].y = (((INT32)(polydata[i] << 18) >> 18) + (GAELCO3D_RESOLUTION_DIVIDE/2)) / GAELCO3D_RESOLUTION_DIVIDE;

		/* compute the scanning parameters */
		scans = setup_triangle_0(&vert[0], &vert[1], &vert[2], &clip);
		if (scans)
		{
			int pixeloffs, zbufval, u, v;
#if (BILINEAR_FILTER)
			int paldata, r, g, b, f, tf;
#endif
			/* special case: no Z buffering and no perspective correction */
			if (color != 0x7f00 && z0 < 0 && ooz_dx == 0 && ooz_dy == 0)
			{
				float zbase = 1.0f / ooz_base;
				float uoz_step = uoz_dx * zbase;
				float voz_step = voz_dx * zbase;
				zbufval = (int)(-z0 * zbase);

				/* loop over scanlines */
				for (y = scans->sy; y <= scans->ey; y++)
				{
					const struct poly_scanline *scan = &scans->scanline[y - scans->sy];
					UINT16 *dest = BITMAP_ADDR16(screenbits, midy - y, midx);
					UINT16 *zbuf = BITMAP_ADDR16(zbuffer, midy - y, midx);
					float uoz = (uoz_dy * y + scan->sx * uoz_dx + uoz_base) * zbase;
					float voz = (voz_dy * y + scan->sx * voz_dx + voz_base) * zbase;

					for (x = scan->sx; x <= scan->ex; x++)
					{
#if (!BILINEAR_FILTER)
						u = (int)(uoz + 0.5); v = (int)(voz + 0.5);
						pixeloffs = (tex + v * 4096 + u) & endmask;
						if (pixeloffs >= gaelco3d_texmask_size || !gaelco3d_texmask[pixeloffs])
						{
							dest[x] = palette[color | gaelco3d_texture[pixeloffs]];
							zbuf[x] = zbufval;
						}
#else
						u = (int)(uoz * 256.0); v = (int)(voz * 256.0);
						pixeloffs = (tex + (v >> 8) * 4096 + (u >> 8)) & endmask;
						if (pixeloffs >= gaelco3d_texmask_size || !gaelco3d_texmask[pixeloffs])
						{
							paldata = palette[color | gaelco3d_texture[pixeloffs]];
							tf = f = (~u & 0xff) * (~v & 0xff);
							r = (paldata & 0x7c00) * f; g = (paldata & 0x03e0) * f; b = (paldata & 0x001f) * f;

							paldata = palette[color | gaelco3d_texture[(pixeloffs + 1) & endmask]];
							tf += f = (u & 0xff) * (~v & 0xff);
							r += (paldata & 0x7c00) * f; g += (paldata & 0x03e0) * f; b += (paldata & 0x001f) * f;

							paldata = palette[color | gaelco3d_texture[(pixeloffs + 4096) & endmask]];
							tf += f = (~u & 0xff) * (v & 0xff);
							r += (paldata & 0x7c00) * f; g += (paldata & 0x03e0) * f; b += (paldata & 0x001f) * f;

							paldata = palette[color | gaelco3d_texture[(pixeloffs + 4097) & endmask]];
							f = 0x10000 - tf;
							r += (paldata & 0x7c00) * f; g += (paldata & 0x03e0) * f; b += (paldata & 0x001f) * f;

							dest[x] = ((r >> 16) & 0x7c00) | ((g >> 16) & 0x03e0) | (b >> 16);
							zbuf[x] = zbufval;
						}
#endif
						/* advance texture params to the next pixel */
						uoz += uoz_step;
						voz += voz_step;
					}
				}
			}

			/* general case: non-alpha blended */
			else if (color != 0x7f00)
			{
				/* loop over scanlines */
				for (y = scans->sy; y <= scans->ey; y++)
				{
					const struct poly_scanline *scan = &scans->scanline[y - scans->sy];
					UINT16 *dest = BITMAP_ADDR16(screenbits, midy - y, midx);
					UINT16 *zbuf = BITMAP_ADDR16(zbuffer, midy - y, midx);
					float ooz = ooz_dy * y + scan->sx * ooz_dx + ooz_base;
					float uoz = uoz_dy * y + scan->sx * uoz_dx + uoz_base;
					float voz = voz_dy * y + scan->sx * voz_dx + voz_base;

					for (x = scan->sx; x <= scan->ex; x++)
					{
						if (ooz > 0)
						{
							/* compute Z and check the Z buffer value first */
							float z = 1.0f / ooz;
							zbufval = (int)(z0 * z);
							if (zbufval < zbuf[x] || zbufval < 0)
							{
#if (!BILINEAR_FILTER)
								u = (int)(uoz * z + 0.5); v = (int)(voz * z + 0.5);
								pixeloffs = (tex + v * 4096 + u) & endmask;
								if (pixeloffs >= gaelco3d_texmask_size || !gaelco3d_texmask[pixeloffs])
								{
									dest[x] = palette[color | gaelco3d_texture[pixeloffs]];
									zbuf[x] = (zbufval < 0) ? -zbufval : zbufval;
								}
#else
								u = (int)(uoz * z * 256.0); v = (int)(voz * z * 256.0);
								pixeloffs = (tex + (v >> 8) * 4096 + (u >> 8)) & endmask;
								if (pixeloffs >= gaelco3d_texmask_size || !gaelco3d_texmask[pixeloffs])
								{
									paldata = palette[color | gaelco3d_texture[pixeloffs]];
									tf = f = (~u & 0xff) * (~v & 0xff);
									r = (paldata & 0x7c00) * f; g = (paldata & 0x03e0) * f; b = (paldata & 0x001f) * f;

									paldata = palette[color | gaelco3d_texture[(pixeloffs + 1) & endmask]];
									tf += f = (u & 0xff) * (~v & 0xff);
									r += (paldata & 0x7c00) * f; g += (paldata & 0x03e0) * f; b += (paldata & 0x001f) * f;

									paldata = palette[color | gaelco3d_texture[(pixeloffs + 4096) & endmask]];
									tf += f = (~u & 0xff) * (v & 0xff);
									r += (paldata & 0x7c00) * f; g += (paldata & 0x03e0) * f; b += (paldata & 0x001f) * f;

									paldata = palette[color | gaelco3d_texture[(pixeloffs + 4097) & endmask]];
									f = 0x10000 - tf;
									r += (paldata & 0x7c00) * f; g += (paldata & 0x03e0) * f; b += (paldata & 0x001f) * f;

									dest[x] = ((r >> 16) & 0x7c00) | ((g >> 16) & 0x03e0) | (b >> 16);
									zbuf[x] = (zbufval < 0) ? -zbufval : zbufval;
								}
#endif
							}
						}

						/* advance texture params to the next pixel */
						ooz += ooz_dx;
						uoz += uoz_dx;
						voz += voz_dx;
					}
				}
			}

			/* color 0x7f seems to be hard-coded as a 50% alpha blend */
			else
			{
				/* loop over scanlines */
				for (y = scans->sy; y <= scans->ey; y++)
				{
					const struct poly_scanline *scan = &scans->scanline[y - scans->sy];
					UINT16 *dest = BITMAP_ADDR16(screenbits, midy - y, midx);
					UINT16 *zbuf = BITMAP_ADDR16(zbuffer, midy - y, midx);
					float ooz = ooz_dy * y + scan->sx * ooz_dx + ooz_base;
					float uoz = uoz_dy * y + scan->sx * uoz_dx + uoz_base;
					float voz = voz_dy * y + scan->sx * voz_dx + voz_base;

					for (x = scan->sx; x <= scan->ex; x++)
					{
						if (ooz > 0)
						{
							/* compute Z and check the Z buffer value first */
							float z = 1.0f / ooz;
							zbufval = (int)(z0 * z);
							if (zbufval < zbuf[x] || zbufval < 0)
							{
#if (!BILINEAR_FILTER)
								u = (int)(uoz * z + 0.5); v = (int)(voz * z + 0.5);
								pixeloffs = (tex + v * 4096 + u) & endmask;
								if (pixeloffs >= gaelco3d_texmask_size || !gaelco3d_texmask[pixeloffs])
								{
									dest[x] = ((dest[x] >> 1) & 0x3def) + ((palette[color | gaelco3d_texture[pixeloffs]] >> 1) & 0x3def);
									zbuf[x] = (zbufval < 0) ? -zbufval : zbufval;
								}
#else
								u = (int)(uoz * z * 256.0); v = (int)(voz * z * 256.0);
								pixeloffs = (tex + (v >> 8) * 4096 + (u >> 8)) & endmask;
								if (pixeloffs >= gaelco3d_texmask_size || !gaelco3d_texmask[pixeloffs])
								{
									paldata = palette[color | gaelco3d_texture[pixeloffs]];
									tf = f = (~u & 0xff) * (~v & 0xff);
									r = (paldata & 0x7c00) * f; g = (paldata & 0x03e0) * f; b = (paldata & 0x001f) * f;

									paldata = palette[color | gaelco3d_texture[(pixeloffs + 1) & endmask]];
									tf += f = (u & 0xff) * (~v & 0xff);
									r += (paldata & 0x7c00) * f; g += (paldata & 0x03e0) * f; b += (paldata & 0x001f) * f;

									paldata = palette[color | gaelco3d_texture[(pixeloffs + 4096) & endmask]];
									tf += f = (~u & 0xff) * (v & 0xff);
									r += (paldata & 0x7c00) * f; g += (paldata & 0x03e0) * f; b += (paldata & 0x001f) * f;

									paldata = palette[color | gaelco3d_texture[(pixeloffs + 4097) & endmask]];
									f = 0x10000 - tf;
									r += (paldata & 0x7c00) * f; g += (paldata & 0x03e0) * f; b += (paldata & 0x001f) * f;

									paldata = ((r >> 17) & 0x7c00) | ((g >> 17) & 0x03e0) | (b >> 17);
									dest[x] = ((dest[x] >> 1) & 0x3def) + (paldata & 0x3def);
									zbuf[x] = (zbufval < 0) ? -zbufval : zbufval;
								}
#endif
							}
						}

						/* advance texture params to the next pixel */
						ooz += ooz_dx;
						uoz += uoz_dx;
						voz += voz_dx;
					}
				}
			}
		}

		/* copy vertex 2 to vertex 1 -- this hardware draws in fans */
		vert[1] = vert[2];
	}

	polygons++;
	return NULL;
}



/*************************************
 *
 *  Scene rendering
 *
 *************************************/

void gaelco3d_render(void)
{
	/* wait for any queued stuff to complete */
	osd_work_queue_wait(work_queue, 100 * osd_ticks_per_second());

	/* Every 2nd frame video data is written. Only skip if two frames are skipped */
	this_skip = video_skip_this_frame();

#if DISPLAY_STATS
{
	int scan = video_screen_get_vpos(0);
	popmessage("Polys = %4d  Timeleft = %3d", polygons, (lastscan < scan) ? (scan - lastscan) : (scan + (lastscan - Machine->screen[0].visarea.max_y)));
}
#endif

	polydata_start = polydata_count = 0;
	polygons = 0;
	lastscan = -1;
}



/*************************************
 *
 *  Renderer access
 *
 *************************************/

WRITE32_HANDLER( gaelco3d_render_w )
{
	/* append the data to our buffer */
	polydata_buffer[polydata_count++] = data;
	if (polydata_count >= MAX_POLYDATA)
		fatalerror("Out of polygon buffer space!");

	/* if we've accumulated a completed poly set of data, queue it */
	if (!this_skip)
	{
		int index = polydata_count - 1 - polydata_start;
		if (index >= 17 && (index % 2) == 1 && IS_POLYEND(data))
		{
			osd_work_item_queue(work_queue, render_poly, &polydata_buffer[polydata_start], WORK_ITEM_FLAG_AUTO_RELEASE);
			polydata_start += index + 2;
		}
		video_changed = TRUE;
	}

#if DISPLAY_STATS
	lastscan = video_screen_get_vpos(0);
#endif
}



/*************************************
 *
 *  Palette access
 *
 *************************************/

INLINE void update_palette_entry(offs_t offset, UINT16 data)
{
	int r = (data >> 10) & 0x1f;
	int g = (data >> 5) & 0x1f;
	int b = (data >> 0) & 0x1f;
	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);
	palette[offset] = data;
}


WRITE16_HANDLER( gaelco3d_paletteram_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	update_palette_entry(offset, paletteram16[offset]);
}


WRITE32_HANDLER( gaelco3d_paletteram_020_w )
{
	COMBINE_DATA(&paletteram32[offset]);
	update_palette_entry(offset*2+0, paletteram32[offset] >> 16);
	update_palette_entry(offset*2+1, paletteram32[offset]);
}



/*************************************
 *
 *  Video update
 *
 *************************************/

VIDEO_UPDATE( gaelco3d )
{
	int x, y, ret;

	if (DISPLAY_TEXTURE && (input_code_pressed(KEYCODE_Z) || input_code_pressed(KEYCODE_X)))
	{
		static int xv = 0, yv = 0x1000;
		UINT8 *base = gaelco3d_texture;
		int length = gaelco3d_texture_size;

		if (input_code_pressed(KEYCODE_X))
		{
			base = gaelco3d_texmask;
			length = gaelco3d_texmask_size;
		}

		if (input_code_pressed(KEYCODE_LEFT) && xv >= 4)
			xv -= 4;
		if (input_code_pressed(KEYCODE_RIGHT) && xv < 4096 - 4)
			xv += 4;

		if (input_code_pressed(KEYCODE_UP) && yv >= 4)
			yv -= 4;
		if (input_code_pressed(KEYCODE_DOWN) && yv < 0x40000)
			yv += 4;

		for (y = cliprect->min_y; y <= cliprect->max_y; y++)
		{
			UINT16 *dest = BITMAP_ADDR16(bitmap, y, 0);
			for (x = cliprect->min_x; x <= cliprect->max_x; x++)
			{
				int offs = (yv + y - cliprect->min_y) * 4096 + xv + x - cliprect->min_x;
				if (offs < length)
					dest[x] = base[offs];
				else
					dest[x] = 0;
			}
		}
		popmessage("(%04X,%04X)", xv, yv);
	}
	else
	{
		if (video_changed)
			copybitmap(bitmap, screenbits, 0,0, 0,0, cliprect, TRANSPARENCY_NONE, 0);
		ret = video_changed;
		video_changed = FALSE;
	}

	logerror("---update---\n");
	return (!ret);
}
