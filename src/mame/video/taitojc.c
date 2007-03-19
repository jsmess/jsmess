#include "driver.h"
#include "video/poly.h"

UINT8 *taitojc_texture;
static mame_bitmap *framebuffer;
static mame_bitmap *zbuffer;

extern UINT32 *taitojc_vram;
extern UINT32 *taitojc_objlist;

//static int debug_tex_pal = 0;

static int tex_base_x;
static int tex_base_y;
static int tex_wrap_x;
static int tex_wrap_y;

static int taitojc_gfx_index;

static UINT8 *taitojc_dirty_map;
static UINT32 *taitojc_char_ram;
static UINT32 *taitojc_tile_ram;
static int taitojc_char_dirty = 1;
static tilemap *taitojc_tilemap;

#define TAITOJC_NUM_TILES		0x80

static const gfx_layout taitojc_char_layout =
{
	16,16,
	TAITOJC_NUM_TILES,
	4,
	{ 0,1,2,3 },
	{ 24,28,16,20,8,12,0,4, 56,60,48,52,40,44,32,36 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,8*64,9*64,10*64,11*64,12*64,13*64,14*64,15*64 },
	16*64
};

static void taitojc_tile_info(int tile_index)
{
	UINT32 val = taitojc_tile_ram[tile_index];
	int color = (val >> 22) & 0xff;
	int tile = (val >> 2) & 0x7f;
	SET_TILE_INFO(taitojc_gfx_index, tile, color, 0);
}

static void taitojc_tile_update(void)
{
	int i;
	if (taitojc_char_dirty)
	{
		for (i=0; i < TAITOJC_NUM_TILES; i++)
		{
			if (taitojc_dirty_map[i])
			{
				taitojc_dirty_map[i] = 0;
				decodechar(Machine->gfx[taitojc_gfx_index], i, (UINT8 *)taitojc_char_ram, &taitojc_char_layout);
			}
		}
		tilemap_mark_all_tiles_dirty(taitojc_tilemap);
		taitojc_char_dirty = 0;
	}
}

READ32_HANDLER(taitojc_tile_r)
{
	return taitojc_tile_ram[offset];
}

READ32_HANDLER(taitojc_char_r)
{
	return taitojc_char_ram[offset];
}

WRITE32_HANDLER(taitojc_tile_w)
{
	COMBINE_DATA(taitojc_tile_ram + offset);
	tilemap_mark_tile_dirty(taitojc_tilemap, offset);
}

WRITE32_HANDLER(taitojc_char_w)
{
	COMBINE_DATA(taitojc_char_ram + offset);
	taitojc_dirty_map[offset/32] = 1;
	taitojc_char_dirty = 1;
}

// Object data format:
//
// 0x00:   xxxxxx-- -------- -------- --------   Height
// 0x00:   ------xx xxxxxxxx -------- --------   Y
// 0x00:   -------- -------- xxxxxx-- --------   Width
// 0x00:   -------- -------- ------xx xxxxxxxx   X
// 0x01:   ---xxxxx xx------ -------- --------   Palette
// 0x01:   -------- --x----- -------- --------   Priority (0 = below 3D, 1 = above 3D)
// 0x01:   -------- -------- -xxxxxxx xxxxxxxx   VRAM data address

static void draw_object(mame_bitmap *bitmap, const rectangle *cliprect, UINT32 w1, UINT32 w2)
{
	int x, y, width, height, palette;
	int i, j;
	int x1, x2, y1, y2;
	int ix, iy;
	UINT32 address;
	UINT8 *v;

	address		= (w2 & 0x7fff) * 0x20;
	if (w2 & 0x4000)
		address |= 0x40000;

	x			= ((w1 >>  0) & 0x3ff);
	if (x & 0x200)
		x |= ~0x1ff;		// sign-extend

	y			= ((w1 >> 16) & 0x3ff);
	if (y & 0x200)
		y |= ~0x1ff;		// sign-extend

	width		= ((w1 >> 10) & 0x3f) * 16;
	height		= ((w1 >> 26) & 0x3f) * 16;
	palette		= ((w2 >> 22) & 0x7f) << 8;

	v = (UINT8*)&taitojc_vram[address/4];

	if (address >= 0xf8000 || width == 0 || height == 0)
		return;

	x1 = x;
	x2 = x + width;
	y1 = y;
	y2 = y + height;

	// trivial rejection
	if (x1 > cliprect->max_x || x2 < cliprect->min_x || y1 > cliprect->max_y || y2 < cliprect->min_y)
	{
		return;
	}

//  mame_printf_debug("draw_object: %08X %08X, X: %d, Y: %d, W: %d, H: %d\n", w1, w2, x, y, width, height);

	ix = 0;
	iy = 0;

	// clip
	if (x1 < cliprect->min_x)
	{
		ix = abs(cliprect->min_x - x1);
		x1 = cliprect->min_x;
	}
	if (x2 > cliprect->max_x)
	{
		x2 = cliprect->max_x;
	}
	if (y1 < cliprect->min_y)
	{
		iy = abs(cliprect->min_y - y1);
		y1 = cliprect->min_y;
	}
	if (y2 > cliprect->max_y)
	{
		y2 = cliprect->max_y;
	}

	for (j=y1; j < y2; j++)
	{
		UINT16 *d = BITMAP_ADDR16(bitmap, j,  0);
		int index = (iy * width) + ix;

		for (i=x1; i < x2; i++)
		{
			UINT8 pen = v[BYTE4_XOR_BE(index)];
			if (pen != 0)
			{
				d[i] = palette + pen;
			}
			index++;
		}

		iy++;
	}
}

VIDEO_START( taitojc )
{
 	/* find first empty slot to decode gfx */
	for (taitojc_gfx_index = 0; taitojc_gfx_index < MAX_GFX_ELEMENTS; taitojc_gfx_index++)
		if (Machine->gfx[taitojc_gfx_index] == 0)
			break;

	if (taitojc_gfx_index == MAX_GFX_ELEMENTS)
		return 1;

	taitojc_tilemap = tilemap_create(taitojc_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 16, 16, 64, 64);
	taitojc_dirty_map = auto_malloc(TAITOJC_NUM_TILES);

	tilemap_set_transparent_pen(taitojc_tilemap, 0);

	taitojc_char_ram = auto_malloc(0x4000);
	taitojc_tile_ram = auto_malloc(0x4000);

	memset(taitojc_char_ram, 0, 0x4000);
	memset(taitojc_tile_ram, 0, 0x4000);

	/* create the char set (gfx will then be updated dynamically from RAM) */
	Machine->gfx[taitojc_gfx_index] = allocgfx(&taitojc_char_layout);
	if (!Machine->gfx[taitojc_gfx_index])
		return 1;

	/* set the color information */
	if (Machine->drv->color_table_len)
	{
		Machine->gfx[taitojc_gfx_index]->colortable = Machine->remapped_colortable;
		Machine->gfx[taitojc_gfx_index]->total_colors = Machine->drv->color_table_len / 16;
	}
	else
	{
		Machine->gfx[taitojc_gfx_index]->colortable = Machine->pens;
		Machine->gfx[taitojc_gfx_index]->total_colors = Machine->drv->total_colors / 16;
	}

	taitojc_texture = auto_malloc(0x400000);

	framebuffer = auto_bitmap_alloc(Machine->screen[0].width, Machine->screen[0].height, Machine->screen[0].format);

	zbuffer = auto_bitmap_alloc(Machine->screen[0].width, Machine->screen[0].height, BITMAP_FORMAT_INDEXED16);

	return 0;
}

//static int tick = 0;
VIDEO_UPDATE( taitojc )
{
	int i;

	/*
    tick++;
    if( tick >= 5 ) {
        tick = 0;

        if( code_pressed(KEYCODE_O) )
            debug_tex_pal++;

        if( code_pressed(KEYCODE_I) )
            debug_tex_pal--;

        debug_tex_pal &= 0x7f;
    }
    */

	fillbitmap(bitmap, 0, cliprect);

	for (i=(0xc00/4)-2; i >= 0; i-=2)
	{
		UINT32 w1 = taitojc_objlist[i + 0];
		UINT32 w2 = taitojc_objlist[i + 1];

		if ((w2 & 0x200000) == 0)
		{
			draw_object(bitmap, cliprect, w1, w2);
		}
	}

	copybitmap(bitmap, framebuffer, 0, 0, 0, 0, cliprect, TRANSPARENCY_PEN, 0);

	for (i=(0xc00/4)-2; i >= 0; i-=2)
	{
		UINT32 w1 = taitojc_objlist[i + 0];
		UINT32 w2 = taitojc_objlist[i + 1];

		if ((w2 & 0x200000) != 0)
		{
			draw_object(bitmap, cliprect, w1, w2);
		}
	}

	taitojc_tile_update();
	tilemap_draw(bitmap, cliprect, taitojc_tilemap, 0,0);

	/*
    if (debug_tex_pal > 0)
    {
        int j;
        for (j=cliprect->min_y; j <= cliprect->max_y; j++)
        {
            UINT16 *d = BITMAP_ADDR16(bitmap, j, 0);
            int index = 2048 * j;

            for (i=cliprect->min_x; i <= cliprect->max_x; i++)
            {
                UINT8 t = taitojc_texture[index+i];
                UINT32 color;

                //color = 0xff000000 | (t << 16) | (t << 8) | (t);
                color = (debug_tex_pal << 8) | t;

                d[i] = color;
            }
        }

        {
            char string[200];
            sprintf(string, "Texture palette %d", debug_tex_pal);
            popmessage("%s", string);
        }
    }
    */

	return 0;
}




typedef struct
{
	int x, y, z;
	UINT32 color;
	int u, v;
} VERTEX;

static void render_solid_tri(VERTEX *v1, VERTEX *v2, VERTEX *v3)
{
	UINT16 color;
	int x, y;
	struct poly_vertex vert[3];
	const struct poly_scanline_data *scans;

	rectangle cliprect;

	cliprect.min_x = 0;
	cliprect.min_y = 0;
	cliprect.max_x = Machine->screen[0].width-1;
	cliprect.max_y = Machine->screen[0].height-1;

	vert[0].x = v1->x;	vert[0].y = v1->y;	vert[0].p[0] = v1->z;
	vert[1].x = v2->x;	vert[1].y = v2->y;	vert[1].p[0] = v2->z;
	vert[2].x = v3->x;	vert[2].y = v3->y;	vert[2].p[0] = v3->z;

	color = v1->color;

	scans = setup_triangle_1(&vert[0], &vert[1], &vert[2], &cliprect);

	if (scans)
	{
		INT64 dz = scans->dp[0];

		for (y = scans->sy; y <= scans->ey; y++)
		{
			int x1, x2;
			INT64 z;
			const struct poly_scanline *scan = &scans->scanline[y - scans->sy];
			UINT16 *fb = BITMAP_ADDR16(framebuffer, y, 0);
			UINT16 *zb = BITMAP_ADDR16(zbuffer, y, 0);

			x1 = scan->sx;
			x2 = scan->ex;

			z = scans->scanline[y - scans->sy].p[0];

			for (x = x1; x <= x2; x++)
			{
				int iz = (z >> 16) & 0xffff;

				if (iz <= zb[x])
				{
					fb[x] = color;
					zb[x] = iz;
				}

				z += dz;
			}
		}
	}
}

static void render_shade_tri(VERTEX *v1, VERTEX *v2, VERTEX *v3)
{
	int x, y;
	struct poly_vertex vert[3];
	const struct poly_scanline_data *scans;

	rectangle cliprect;

	cliprect.min_x = 0;
	cliprect.min_y = 0;
	cliprect.max_x = Machine->screen[0].width-1;
	cliprect.max_y = Machine->screen[0].height-1;

	vert[0].x = v1->x;	vert[0].y = v1->y;	vert[0].p[0] = v1->color;	vert[0].p[1] = v1->z;
	vert[1].x = v2->x;	vert[1].y = v2->y;	vert[1].p[0] = v2->color;	vert[1].p[1] = v2->z;
	vert[2].x = v3->x;	vert[2].y = v3->y;	vert[2].p[0] = v3->color;	vert[2].p[1] = v3->z;

	scans = setup_triangle_2(&vert[0], &vert[1], &vert[2], &cliprect);

	if (scans)
	{
		INT64 dcolor = scans->dp[0];
		INT64 dz = scans->dp[1];

		for (y = scans->sy; y <= scans->ey; y++)
		{
			int x1, x2;
			INT64 color, z;
			const struct poly_scanline *scan = &scans->scanline[y - scans->sy];
			UINT16 *fb = BITMAP_ADDR16(framebuffer, y, 0);
			UINT16 *zb = BITMAP_ADDR16(zbuffer, y, 0);

			x1 = scan->sx;
			x2 = scan->ex;

			color = scans->scanline[y - scans->sy].p[0];
			z = scans->scanline[y - scans->sy].p[1];

			for (x = x1; x <= x2; x++)
			{
				int ic = (color >> 16) & 0xffff;
				int iz = (z >> 16) & 0xffff;

				if (iz <= zb[x])
				{
					fb[x] = ic;
					zb[x] = iz;
				}

				color += dcolor;
				z += dz;
			}
		}
	}
}

static void render_texture_tri(VERTEX *v1, VERTEX *v2, VERTEX *v3)
{
	int x, y;
	struct poly_vertex vert[3];
	const struct poly_scanline_data *scans;

	rectangle cliprect;

	cliprect.min_x = 0;
	cliprect.min_y = 0;
	cliprect.max_x = Machine->screen[0].width-1;
	cliprect.max_y = Machine->screen[0].height-1;

	vert[0].x = v1->x;	vert[0].y = v1->y;
	vert[0].p[0] = v1->u;
	vert[0].p[1] = v1->v;
	vert[0].p[2] = v1->color;
	vert[0].p[3] = v1->z;

	vert[1].x = v2->x;	vert[1].y = v2->y;
	vert[1].p[0] = v2->u;
	vert[1].p[1] = v2->v;
	vert[1].p[2] = v2->color;
	vert[1].p[3] = v2->z;

	vert[2].x = v3->x;	vert[2].y = v3->y;
	vert[2].p[0] = v3->u;
	vert[2].p[1] = v3->v;
	vert[2].p[2] = v3->color;
	vert[2].p[3] = v3->z;

	scans = setup_triangle_4(&vert[0], &vert[1], &vert[2], &cliprect);

	if (scans)
	{
		INT64 du = scans->dp[0];
		INT64 dv = scans->dp[1];
		INT64 dcolor = scans->dp[2];
		INT64 dz = scans->dp[3];

		for (y = scans->sy; y <= scans->ey; y++)
		{
			int x1, x2;
			INT64 u, v, color, z;
			const struct poly_scanline *scan = &scans->scanline[y - scans->sy];
			UINT16 *fb = BITMAP_ADDR16(framebuffer, y, 0);
			UINT16 *zb = BITMAP_ADDR16(zbuffer, y, 0);

			x1 = scan->sx;
			x2 = scan->ex;

			u = scans->scanline[y - scans->sy].p[0];
			v = scans->scanline[y - scans->sy].p[1];
			color = scans->scanline[y - scans->sy].p[2];
			z = scans->scanline[y - scans->sy].p[3];

			for (x = x1; x <= x2; x++)
			{
				int iu, iv;
				UINT8 texel;
				int palette = ((color >> 16) & 0x7f) << 8;
				int iz = (z >> 16) & 0xffff;

				if (!tex_wrap_x)
				{
					iu = (u >> (16+4)) & 0x7ff;
				}
				else
				{
					iu = (tex_base_x + ((u >> (16+4)) & 0x3f)) & 0x7ff;
				}

				if (!tex_wrap_y)
				{
					iv = (v >> (16+4)) & 0x7ff;
				}
				else
				{
					iv = (tex_base_y + ((v >> (16+4)) & 0x3f)) & 0x7ff;
				}

				texel = taitojc_texture[(iv * 2048) + iu];

				if (iz <= zb[x] && texel != 0)
				{
					fb[x] = palette | texel;
					zb[x] = iz;
				}

				u += du;
				v += dv;
				color += dcolor;
				z += dz;
			}
		}
	}
}

INLINE void render_solid_quad(VERTEX *v1, VERTEX *v2, VERTEX *v3, VERTEX *v4)
{
	render_solid_tri(v1, v2, v3);
	render_solid_tri(v1, v3, v4);
}

INLINE void render_shade_quad(VERTEX *v1, VERTEX *v2, VERTEX *v3, VERTEX *v4)
{
	render_shade_tri(v1, v2, v3);
	render_shade_tri(v1, v3, v4);
}

INLINE void render_texture_quad(VERTEX *v1, VERTEX *v2, VERTEX *v3, VERTEX *v4)
{
	render_texture_tri(v1, v2, v3);
	render_texture_tri(v1, v3, v4);
}

void taitojc_render_polygons(UINT16 *polygon_fifo, int length)
{
	VERTEX vert[4];
	int i;
	int ptr;

	ptr = 0;
	while (ptr < length)
	{
		UINT16 cmd = polygon_fifo[ptr++];

		switch (cmd & 0x7)
		{
			case 0x03:		// Textured Triangle
			{
				// 0x00: Command ID (0x0003)
				// 0x01: Texture base
				// 0x02: Vertex 1 Palette
				// 0x03: Vertex 1 V
				// 0x04: Vertex 1 U
				// 0x05: Vertex 1 Y
				// 0x06: Vertex 1 X
				// 0x07: Vertex 1 Z
				// 0x08: Vertex 2 Palette
				// 0x09: Vertex 2 V
				// 0x0a: Vertex 2 U
				// 0x0b: Vertex 2 Y
				// 0x0c: Vertex 2 X
				// 0x0d: Vertex 2 Z
				// 0x0e: Vertex 3 Palette
				// 0x0f: Vertex 3 V
				// 0x10: Vertex 3 U
				// 0x11: Vertex 3 Y
				// 0x12: Vertex 3 X
				// 0x13: Vertex 3 Z

				UINT16 texbase;

				/*
                printf("CMD3: ");
                for (i=0; i < 0x13; i++)
                {
                    printf("%04X ", polygon_fifo[ptr+i]);
                }
                printf("\n");
                */

				texbase = polygon_fifo[ptr++];

				tex_base_x = ((texbase >> 0) & 0xff) << 4;
				tex_base_y = ((texbase >> 8) & 0xff) << 4;

				tex_wrap_x = (cmd & 0xc0) ? 1 : 0;
				tex_wrap_y = (cmd & 0x30) ? 1 : 0;

				for (i=0; i < 3; i++)
				{
					vert[i].color = polygon_fifo[ptr++];	// palette
					vert[i].v = (UINT16)(polygon_fifo[ptr++]);
					vert[i].u = (UINT16)(polygon_fifo[ptr++]);
					vert[i].y =  (INT16)(polygon_fifo[ptr++]);
					vert[i].x =  (INT16)(polygon_fifo[ptr++]);
					vert[i].z = (UINT16)(polygon_fifo[ptr++]);
				}

				if (vert[0].z < 0x8000 && vert[1].z < 0x8000 && vert[2].z < 0x8000)
				{
					render_texture_tri(&vert[0], &vert[1], &vert[2]);
				}
				break;
			}
			case 0x04:		// Gouraud shaded Quad
			{
				// 0x00: Command ID (0x0004)
				// 0x01: Vertex 0 color
				// 0x02: Vertex 0 Y
				// 0x03: Vertex 0 X
				// 0x04: Vertex 0 Z
				// 0x05: Vertex 1 color
				// 0x06: Vertex 1 Y
				// 0x07: Vertex 1 X
				// 0x08: Vertex 1 Z
				// 0x09: Vertex 2 color
				// 0x0a: Vertex 2 Y
				// 0x0b: Vertex 2 X
				// 0x0c: Vertex 2 Z
				// 0x0d: Vertex 3 color
				// 0x0e: Vertex 3 Y
				// 0x0f: Vertex 3 X
				// 0x10: Vertex 3 Z

				/*
                printf("CMD4: ");
                for (i=0; i < 0x10; i++)
                {
                    printf("%04X ", polygon_fifo[ptr+i]);
                }
                printf("\n");
                */

				for (i=0; i < 4; i++)
				{
					vert[i].color = polygon_fifo[ptr++];
					vert[i].y =  (INT16)(polygon_fifo[ptr++]);
					vert[i].x =  (INT16)(polygon_fifo[ptr++]);
					vert[i].z = (UINT16)(polygon_fifo[ptr++]);
				}

				if (vert[0].z < 0x8000 && vert[1].z < 0x8000 && vert[2].z < 0x8000 && vert[3].z < 0x8000)
				{
					if (vert[0].color == vert[1].color &&
						vert[1].color == vert[2].color &&
						vert[2].color == vert[3].color)
					{
						// optimization: all colours the same -> render solid
						render_solid_quad(&vert[0], &vert[1], &vert[2], &vert[3]);
					}
					else
					{
						render_shade_quad(&vert[0], &vert[1], &vert[2], &vert[3]);
					}
				}
				break;
			}
			case 0x06:		// Textured Quad
			{
				// 0x00: Command ID (0x0006)
				// 0x01: Texture base
				// 0x02: Vertex 1 Palette
				// 0x03: Vertex 1 V
				// 0x04: Vertex 1 U
				// 0x05: Vertex 1 Y
				// 0x06: Vertex 1 X
				// 0x07: Vertex 1 Z
				// 0x08: Vertex 2 Palette
				// 0x09: Vertex 2 V
				// 0x0a: Vertex 2 U
				// 0x0b: Vertex 2 Y
				// 0x0c: Vertex 2 X
				// 0x0d: Vertex 2 Z
				// 0x0e: Vertex 3 Palette
				// 0x0f: Vertex 3 V
				// 0x10: Vertex 3 U
				// 0x11: Vertex 3 Y
				// 0x12: Vertex 3 X
				// 0x13: Vertex 3 Z
				// 0x14: Vertex 4 Palette
				// 0x15: Vertex 4 V
				// 0x16: Vertex 4 U
				// 0x17: Vertex 4 Y
				// 0x18: Vertex 4 X
				// 0x19: Vertex 4 Z

				UINT16 texbase;

				/*
                printf("CMD6: ");
                for (i=0; i < 0x19; i++)
                {
                    printf("%04X ", polygon_fifo[ptr+i]);
                }
                printf("\n");
                */

				texbase = polygon_fifo[ptr++];

				tex_base_x = ((texbase >> 0) & 0xff) << 4;
				tex_base_y = ((texbase >> 8) & 0xff) << 4;

				tex_wrap_x = (cmd & 0xc0) ? 1 : 0;
				tex_wrap_y = (cmd & 0x30) ? 1 : 0;

				for (i=0; i < 4; i++)
				{
					vert[i].color = polygon_fifo[ptr++];	// palette
					vert[i].v = (UINT16)(polygon_fifo[ptr++]);
					vert[i].u = (UINT16)(polygon_fifo[ptr++]);
					vert[i].y =  (INT16)(polygon_fifo[ptr++]);
					vert[i].x =  (INT16)(polygon_fifo[ptr++]);
					vert[i].z = (UINT16)(polygon_fifo[ptr++]);
				}

				if (vert[0].z < 0x8000 && vert[1].z < 0x8000 && vert[2].z < 0x8000 && vert[3].z < 0x8000)
				{
					render_texture_quad(&vert[0], &vert[1], &vert[2], &vert[3]);
				}
				break;
			}
			case 0x00:
			{
				ptr += 6;
				break;
			}
			default:
			{
				//printf("render_polygons: unknown command %04X\n", cmd);
			}
		}
	};
}

void taitojc_clear_frame(void)
{
	rectangle cliprect;

	cliprect.min_x = 0;
	cliprect.min_y = 0;
	cliprect.max_x = Machine->screen[0].width-1;
	cliprect.max_y = Machine->screen[0].height-1;

	fillbitmap(framebuffer, 0, &cliprect);
	fillbitmap(zbuffer, 0xffff, &cliprect);
}
