#include "driver.h"
#include "video/segaic24.h"
#include "video/poly.h"

typedef struct
{
	int x, y, width, height;
} WINDOW;

typedef struct
{
	float x, y, z;
	UINT16 u, v;
} VERTEX;

typedef struct
{
	VERTEX v[3];
	WINDOW window;
	float z;
	int luminance;
	UINT16 r, g, b;
	UINT16 transfer_map;
	UINT16 tex_x, tex_y, tex_width, tex_height;
	UINT32 flags;
} TRIANGLE;

typedef struct {
	float x,y,z,d;
} PLANE;


typedef struct
{
	UINT32 diffuse;
	UINT32 ambient;
	UINT32 reflection;
	float c;
} TEXTURE_PARAMETER;

#define TRI_FLAG_TEXTURE				0x1
#define TRI_FLAG_TEXTURE_TRANS			0x2
#define TRI_FLAG_EVEN_BANK				0x4
#define TRI_FLAG_CHECKER				0x8
#define TRI_FLAG_MICROTEX				0x10
#define TRI_FLAG_MIRROR_X				0x20
#define TRI_FLAG_MIRROR_Y				0x40
#define TRI_FLAG_WRAP_X					0x80
#define TRI_FLAG_WRAP_Y					0x100





static void parse_display_list(void);
static void render_triangles(mame_bitmap *bitmap, const rectangle *cliprect);


#define TRIBUFFER_SIZE		16384

static TRIANGLE *tribuffer;
static int tribuffer_pos;

extern UINT32 geo_read_start_address;
extern UINT32 geo_write_start_address;
extern UINT32 *model2_bufferram;
extern UINT32 *model2_textureram0;
extern UINT32 *model2_textureram1;
extern UINT32 *model2_colorxlat;
extern UINT32 *model2_lumaram;

static float matrix[12];
static float light_vector[3];

static WINDOW window;
static float focus_x, focus_y;

static UINT32 *polygon_rom;
static UINT32 *polygon_ram;
static UINT16 *texture_rom;
static UINT16 *texture_ram;

static TEXTURE_PARAMETER texture_parameter[32];

VIDEO_START(model2)
{
	sys24_tile_vh_start(machine, 0x3fff);

	tribuffer = auto_malloc(TRIBUFFER_SIZE * sizeof(TRIANGLE));

	polygon_rom = (UINT32*)memory_region(REGION_USER2);
	polygon_ram = auto_malloc(0x8000 * sizeof(UINT32));

	texture_rom = (UINT16*)memory_region(REGION_USER3);
	texture_ram = auto_malloc(0x10000 * sizeof(UINT16));
}

VIDEO_UPDATE(model2)
{
	sys24_tile_update(machine);
	fillbitmap(bitmap, machine->pens[0], &machine->screen[0].visarea);

	sys24_tile_draw(machine, bitmap, cliprect, 7, 0, 0);
	sys24_tile_draw(machine, bitmap, cliprect, 6, 0, 0);
	sys24_tile_draw(machine, bitmap, cliprect, 5, 0, 0);
	sys24_tile_draw(machine, bitmap, cliprect, 4, 0, 0);

	tribuffer_pos = 0;
	parse_display_list();
	render_triangles(bitmap, cliprect);

	sys24_tile_draw(machine, bitmap, cliprect, 3, 0, 0);
	sys24_tile_draw(machine, bitmap, cliprect, 2, 0, 0);
	sys24_tile_draw(machine, bitmap, cliprect, 1, 0, 0);
	sys24_tile_draw(machine, bitmap, cliprect, 0, 0, 0);

	/*{
        int i;
        FILE *file = fopen("buffer.bin", "wb");
        for (i=0; i < 0x20000/4; i++)
        {
            fputc((model2_bufferram[i] >> 24) & 0xff, file);
            fputc((model2_bufferram[i] >> 16) & 0xff, file);
            fputc((model2_bufferram[i] >>  8) & 0xff, file);
            fputc((model2_bufferram[i] >>  0) & 0xff, file);
        }
        fclose(file);
    }*/
	return 0;
}

static void draw_solid_triangle(mame_bitmap *bitmap, const rectangle *rect, TRIANGLE *tri)
{
	int x, y;
	struct poly_vertex vert[3];
	const struct poly_scanline_data *scans;
	int r, g, b;
	int luminance;
	UINT16 color;

//  UINT16 *colortable_r = (UINT16*)&model2_colorxlat[0x0000/4];
//  UINT16 *colortable_g = (UINT16*)&model2_colorxlat[0x4000/4];
//  UINT16 *colortable_b = (UINT16*)&model2_colorxlat[0x8000/4];

	vert[0].x = tri->v[0].x;	vert[0].y = tri->v[0].y;
	vert[1].x = tri->v[1].x;	vert[1].y = tri->v[1].y;
	vert[2].x = tri->v[2].x;	vert[2].y = tri->v[2].y;

	scans = setup_triangle_0(&vert[0], &vert[1], &vert[2], rect);

	r = tri->r;
	g = tri->g;
	b = tri->b;
	luminance = tri->luminance;

	r = ((r * luminance) >> 6) & 0x1f;
	g = ((g * luminance) >> 6) & 0x1f;
	b = ((b * luminance) >> 6) & 0x1f;
	color = (r << 10) | (g << 5) | (b);

	if(scans)
	{
		for(y = scans->sy; y <= scans->ey; y++)
		{
			int x1, x2;
			const struct poly_scanline *scan = &scans->scanline[y - scans->sy];
			UINT16 *p = BITMAP_ADDR16(bitmap, y, 0);

			x1 = scan->sx;
			x2 = scan->ex;

			for(x = x1; x <= x2; x++)
			{
				/*
                int tr = colortable_r[(r << 8) | luminance];
                int tg = colortable_g[(g << 8) | luminance];
                int tb = colortable_b[(b << 8) | luminance];

                p[x] = (((tr >> 3) << 10) & 0x7c00) | (((tg >> 3) << 5) & 0x03e0) | (((tb >> 3) << 0) & 0x001f);
                */

				p[x] = color;
			}
		}
	}
}

static void draw_textured_triangle(mame_bitmap *bitmap, const rectangle *rect, TRIANGLE *tri)
{
	const int texel_x[4] = {0, 8, 4, 12};

	int x, y;
	struct poly_vertex vert[3];
	const struct poly_scanline_data *scans;
	int r, g, b;
	int luminance;
	UINT32 tex_x		= 32 * tri->tex_x;
	UINT32 tex_y		= 32 * tri->tex_y;
	UINT32 tex_width	= 32 << tri->tex_width;
	UINT32 tex_height	= 32 << tri->tex_height;
	UINT32 tex_x_mask	= tex_width - 1;
	UINT32 tex_y_mask	= tex_height - 1;
	UINT16 *sheet	= (tri->flags & TRI_FLAG_EVEN_BANK)
					? ((UINT16*)model2_textureram1) : ((UINT16*)model2_textureram0);

	//mame_printf_debug("Tex X: %d, Tex Y: %d, Tex Width: %d, Tex Height: %d\n", tex_x, tex_y, tex_width, tex_height);
	//mame_printf_debug("U0: %d, V0: %d, U1: %d, V1: %d, U2: %d, V2: %d\n", tri->v[0].u, tri->v[0].v, tri->v[1].u, tri->v[1].v, tri->v[2].u, tri->v[2].v);

//  UINT16 *colortable_r = (UINT16*)&model2_colorxlat[0x0000/4];
//  UINT16 *colortable_g = (UINT16*)&model2_colorxlat[0x4000/4];
//  UINT16 *colortable_b = (UINT16*)&model2_colorxlat[0x8000/4];

	vert[0].x = tri->v[0].x;	vert[0].y = tri->v[0].y;	vert[0].p[0] = tri->v[0].u;		vert[0].p[1] = tri->v[0].v;
	vert[1].x = tri->v[1].x;	vert[1].y = tri->v[1].y;	vert[1].p[0] = tri->v[1].u;		vert[1].p[1] = tri->v[1].v;
	vert[2].x = tri->v[2].x;	vert[2].y = tri->v[2].y;	vert[2].p[0] = tri->v[2].u;		vert[2].p[1] = tri->v[2].v;

	scans = setup_triangle_2(&vert[0], &vert[1], &vert[2], rect);

	r = tri->r;
	g = tri->g;
	b = tri->b;
	luminance = tri->luminance;

	if(scans)
	{
		INT64 du, dv;
		du = scans->dp[0];
		dv = scans->dp[1];

		for(y = scans->sy; y <= scans->ey; y++)
		{
			INT64 u, v;
			int x1, x2;
			const struct poly_scanline *scan = &scans->scanline[y - scans->sy];
			UINT16 *p = BITMAP_ADDR16(bitmap, y, 0);

			x1 = scan->sx;
			x2 = scan->ex;

			u = scans->scanline[y - scans->sy].p[0];
			v = scans->scanline[y - scans->sy].p[1];

			for(x = x1; x <= x2; x++)
			{
				int tr, tg, tb;
				int u2 = (int)(u >> (16+3)) & tex_x_mask;
				int v2 = (int)(v >> (16+3)) & tex_y_mask;
				UINT16 t = sheet[(((tex_y + v2) * 1024) + (tex_x + u2)) / 4];
				t = (t >> (texel_x[u2&3])) & 0xf;

				t = ((u2 + v2) & 1) ? 0xf : 0;

				tr = ((t * r) >> 4) & 0x1f;
				tg = ((t * g) >> 4) & 0x1f;
				tb = ((t * b) >> 4) & 0x1f;

				p[x] = (tr << 10) | (tg << 5) | (tb);
				u += du;
				v += dv;
			}
		}
	}
}

static int zsort(const void * a, const void * b)
{
	if (((TRIANGLE*)(a))->z < ((TRIANGLE*)(b))->z)
		return 1;
	else if (((TRIANGLE*)(a))->z > ((TRIANGLE*)(b))->z)
		return -1;
	else
		return 0;
}

static void render_triangles(mame_bitmap *bitmap, const rectangle *cliprect)
{
	int i;//, j;

	qsort(tribuffer, tribuffer_pos, sizeof(TRIANGLE), zsort);

	for (i=0; i < tribuffer_pos; i++)
	{
		rectangle rect;
		rect.min_x = tribuffer[i].window.x;
		rect.max_x = tribuffer[i].window.x + tribuffer[i].window.width;
		rect.min_y = tribuffer[i].window.y;
		rect.max_y = tribuffer[i].window.y + tribuffer[i].window.height;

		if (rect.min_x < cliprect->min_x)	rect.min_x = cliprect->min_x;
		if (rect.min_x > cliprect->max_x)	rect.min_x = cliprect->max_x;
		if (rect.max_x < cliprect->min_x)	rect.max_x = cliprect->min_x;
		if (rect.max_x > cliprect->max_x)	rect.max_x = cliprect->max_x;
		if (rect.min_y < cliprect->min_y)	rect.min_y = cliprect->min_y;
		if (rect.min_y > cliprect->max_y)	rect.min_y = cliprect->max_y;
		if (rect.max_y < cliprect->min_y)	rect.max_y = cliprect->min_y;
		if (rect.max_y > cliprect->max_y)	rect.max_y = cliprect->max_y;

		if (tribuffer[i].flags & TRI_FLAG_TEXTURE)
		{
			//draw_textured_triangle(bitmap, &rect, &tribuffer[i]);
			draw_solid_triangle(bitmap, &rect, &tribuffer[i]);
		}
		else
		{
			draw_solid_triangle(bitmap, &rect, &tribuffer[i]);
		}
	}
}

INLINE int is_point_inside(float x, float y, float z, PLANE cp)
{
	float s = (x * cp.x) + (y * cp.y) + (z * cp.z) + cp.d;
	if (s >= 0.0f)
		return 1;
	else
		return 0;
}

INLINE float line_plane_intersection(VERTEX *v1, VERTEX *v2, PLANE cp)
{
	float x = v1->x - v2->x;
	float y = v1->y - v2->y;
	float z = v1->z - v2->z;
	float t = ((cp.x * v1->x) + (cp.y * v1->y) + (cp.z * v1->z)) / ((cp.x * x) + (cp.y * y) + (cp.z * z));
	return t;
}

static int clip_polygon(VERTEX *v, int num_vertices, PLANE cp, VERTEX *vout)
{
	VERTEX clipv[10];
	int clip_verts = 0;
	float t;
	int i;

	int pv = num_vertices - 1;

	for (i=0; i < num_vertices; i++)
	{
		int v1_in = is_point_inside(v[i].x, v[i].y, v[i].z, cp);
		int v2_in = is_point_inside(v[pv].x, v[pv].y, v[pv].z, cp);

		if (v1_in && v2_in)			/* edge is completely inside the volume */
		{
			memcpy(&clipv[clip_verts], &v[i], sizeof(VERTEX));
			++clip_verts;
		}
		else if (!v1_in && v2_in)	/* edge is entering the volume */
		{
			/* insert vertex at intersection point */
			t = line_plane_intersection(&v[i], &v[pv], cp);
			clipv[clip_verts].x = v[i].x + ((v[pv].x - v[i].x) * t);
			clipv[clip_verts].y = v[i].y + ((v[pv].y - v[i].y) * t);
			clipv[clip_verts].z = v[i].z + ((v[pv].z - v[i].z) * t);
			clipv[clip_verts].u = (UINT32)((float)v[i].u + (((float)v[pv].u - (float)v[i].u) * t));
			clipv[clip_verts].v = (UINT32)((float)v[i].v + (((float)v[pv].v - (float)v[i].v) * t));
			++clip_verts;
		}
		else if (v1_in && !v2_in)	/* edge is leaving the volume */
		{
			/* insert vertex at intersection point */
			t = line_plane_intersection(&v[i], &v[pv], cp);
			clipv[clip_verts].x = v[i].x + ((v[pv].x - v[i].x) * t);
			clipv[clip_verts].y = v[i].y + ((v[pv].y - v[i].y) * t);
			clipv[clip_verts].z = v[i].z + ((v[pv].z - v[i].z) * t);
			clipv[clip_verts].u = (UINT32)((float)v[i].u + (((float)v[pv].u - (float)v[i].u) * t));
			clipv[clip_verts].v = (UINT32)((float)v[i].v + (((float)v[pv].v - (float)v[i].v) * t));
			++clip_verts;

			/* insert the existing vertex */
			memcpy(&clipv[clip_verts], &v[i], sizeof(VERTEX));
			++clip_verts;
		}

		pv = i;
	}
	memcpy(&vout[0], &clipv[0], sizeof(VERTEX) * clip_verts);
	return clip_verts;
}

static void push_triangle(TRIANGLE *tri)
{
	int i, j;
	int clipped_verts;
	VERTEX verts[10];
	PLANE clip_plane = { 0.0f, 0.0f, 1.0f, -1.0f };
	clipped_verts = clip_polygon(tri->v, 3, clip_plane, verts);

	for (i=2; i < clipped_verts; i++)
	{
		TRIANGLE *dt = &tribuffer[tribuffer_pos];

		memcpy(dt, tri, sizeof(TRIANGLE));

		memcpy(&dt->v[0], &verts[0], sizeof(VERTEX));
		memcpy(&dt->v[1], &verts[i-1], sizeof(VERTEX));
		memcpy(&dt->v[2], &verts[i], sizeof(VERTEX));
		memcpy(&dt->window, &window, sizeof(WINDOW));

		for (j=0; j < 3; j++)
		{
			float ooz = 1.0f / -dt->v[j].z;
			dt->v[j].x = -((dt->v[j].x * ooz) * focus_x) + (float)(window.x + (window.width / 2));
			dt->v[j].y = ((dt->v[j].y * ooz) * focus_y) + (float)(window.y + (window.height / 2));
		}

		tribuffer_pos++;
		if (tribuffer_pos >= TRIBUFFER_SIZE)
		{
			fatalerror("push_triangle: tribuffer overflow");
		}
	}
}

static void push_direct_triangle(TRIANGLE *tri)
{
	TRIANGLE *dt = &tribuffer[tribuffer_pos];

	memcpy(dt, tri, sizeof(TRIANGLE));
	memcpy(&dt->window, &window, sizeof(WINDOW));
}



static void draw_object(UINT32 address, UINT32 num_polys, UINT32 tex_point_addr, UINT32 tex_hdr_addr)
{
	int i;
	int end = 0;
	int polynum = 0;
	VERTEX prev_p1 = { 0 }, prev_p2 = { 0 };

	UINT16 *thdr;
	UINT16 *tpoint;
	UINT32 *object;
	UINT16 *lumatable = (UINT16*)model2_lumaram;

	int th_addr = 0;
	int tp_addr = 0;
	float previous_zvalue = 0.0f;

	if (address < 0x800000)
	{
		object = polygon_ram;
		address = address - 4;
	}
	else
	{
		object = polygon_rom;
		address = (address & 0x7fffff) - 4;
	}

	if (tex_hdr_addr < 0x800000)
	{
		thdr = texture_rom;
		th_addr = tex_hdr_addr;
	}
	else
	{
		thdr = texture_ram;
		th_addr = tex_hdr_addr & 0x7fffff;
	}

	if (tex_point_addr < 0x800000)
	{
		tpoint = texture_rom;
		tp_addr = tex_point_addr;
	}
	else
	{
		tpoint = texture_ram;
		tp_addr = tex_point_addr & 0x7fffff;
	}

	while (!end && polynum < num_polys)
	{
		TEXTURE_PARAMETER *texparam;
		VERTEX v[4], p1, p2;
		float nx, ny, nz;
		UINT32 attr;
		UINT16 color, rgb;
		UINT16 tex_header[4];
		UINT16 s[4], t[4];
		int polytype, linktype, tex_hdr_offset, zsort_type, texture_code;

		s[3] = t[3] = 0;	// fix compiler "used uninitialized" warning

		attr = object[address++];
		polytype = attr & 0x3;

		if (polytype == 0)
		{
			return;
		}

		linktype = (attr >> 8) & 0x3;
		if (linktype != ((attr >> 23) & 0x3))
		{
			//return;
			//fatalerror("Linktype not matching, attribute = %08X at %08X\n", attr, address*4);
		}

		tex_hdr_offset = (attr >> 12) & 0x1f;
		if (tex_hdr_offset & 0x10)	tex_hdr_offset |= -16;

		zsort_type = (attr >> 10) & 0x3;
		texture_code = (attr >> 12) & 0x1f;

		texparam = &texture_parameter[texture_code];



		// load vertex data and polygon normal
		nx = *(float*)(&object[address++]);
		ny = *(float*)(&object[address++]);
		nz = *(float*)(&object[address++]);
		p1.x = *(float*)(&object[address++]);
		p1.y = *(float*)(&object[address++]);
		p1.z = *(float*)(&object[address++]);
		p2.x = *(float*)(&object[address++]);
		p2.y = *(float*)(&object[address++]);
		p2.z = *(float*)(&object[address++]);



		// load texture parameters
		tex_header[0] = thdr[th_addr+0];
		tex_header[1] = thdr[th_addr+1];
		tex_header[2] = thdr[th_addr+2];
		tex_header[3] = thdr[th_addr+3];

		if (tex_header[0] & 0x4000)
		{
			color = (tex_header[3] >> 6) & 0x3ff;
			rgb = Machine->remapped_colortable[4096+color];
		}
		else
		{
			rgb = 0x7fff;
		}

		th_addr += tex_hdr_offset * 4;



		// load texture coordinates
		if (polytype == 2)
		{
			s[0] = tpoint[tp_addr+0];
			t[0] = tpoint[tp_addr+1];
			s[1] = tpoint[tp_addr+2];
			t[1] = tpoint[tp_addr+3];
			s[2] = tpoint[tp_addr+4];
			t[2] = tpoint[tp_addr+5];
			tp_addr += 6;
		}
		else
		{
			s[0] = tpoint[tp_addr+0];
			t[0] = tpoint[tp_addr+1];
			s[1] = tpoint[tp_addr+2];
			t[1] = tpoint[tp_addr+3];
			s[2] = tpoint[tp_addr+4];
			t[2] = tpoint[tp_addr+5];
			s[3] = tpoint[tp_addr+6];
			t[3] = tpoint[tp_addr+7];
			tp_addr += 8;
		}



		// transform and push the polygon to render buffer
		if (linktype != 0)
		{

			float dot, intensity;
			float zvalue;
			TRIANGLE tri1, tri2;
			v[0].x = p1.x;			v[0].y = p1.y;			v[0].z = p1.z;			v[0].u = s[0];	v[0].v = t[0];
			v[1].x = p2.x;			v[1].y = p2.y;			v[1].z = p2.z;			v[1].u = s[1];	v[1].v = t[1];
			v[2].x = prev_p1.x;		v[2].y = prev_p1.y;		v[2].z = prev_p1.z;		v[2].u = s[2];	v[2].v = t[2];
			v[3].x = prev_p2.x;		v[3].y = prev_p2.y;		v[3].z = prev_p2.z;		v[3].u = s[3];	v[3].v = t[3];

			// transform with the current matrix
			for (i=0; i < 4; i++)
			{
				float tx =	(v[i].x * matrix[0]) + (v[i].y * matrix[3]) + (v[i].z * matrix[6]) + (matrix[9]);
				float ty =	(v[i].x * matrix[1]) + (v[i].y * matrix[4]) + (v[i].z * matrix[7]) + (matrix[10]);
				float tz =	(v[i].x * matrix[2]) + (v[i].y * matrix[5]) + (v[i].z * matrix[8]) + (matrix[11]);
				v[i].x = tx;
				v[i].y = ty;
				v[i].z = tz;
			}

			dot = (-light_vector[0] * nx) + (-light_vector[1] * ny) + (-light_vector[2] * nz);
			if (dot < 0.0f)
				dot = 0.0f;
			if (dot > 1.0f)
				dot = 1.0f;

			intensity = (dot * (float)texparam->diffuse);
			intensity += (float)texparam->ambient;

			if (intensity < 0)
				intensity = 0;
			if (intensity > 255.0f)
				intensity = 255.0f;

			memcpy(&tri1.v[0], &v[0], sizeof(VERTEX));
			memcpy(&tri1.v[1], &v[1], sizeof(VERTEX));
			memcpy(&tri1.v[2], &v[2], sizeof(VERTEX));
			memcpy(&tri2.v[0], &v[1], sizeof(VERTEX));
			memcpy(&tri2.v[1], &v[2], sizeof(VERTEX));
			memcpy(&tri2.v[2], &v[3], sizeof(VERTEX));

			switch (zsort_type)
			{
				case 0:			// Previous value
				{
					zvalue = previous_zvalue;
					break;
				}
				case 1:			// Min value
				{
					zvalue = v[0].z;
					zvalue = MIN(zvalue, v[1].z);
					zvalue = MIN(zvalue, v[2].z);
					zvalue = MIN(zvalue, v[3].z);
					break;
				}
				case 2:			// Max value
				{
					zvalue = v[0].z;
					zvalue = MAX(zvalue, v[1].z);
					zvalue = MAX(zvalue, v[2].z);
					zvalue = MAX(zvalue, v[3].z);
					break;
				}
				default:	mame_printf_error("draw_object: unknown zsort mode %d!\n", zsort_type);
					zvalue = previous_zvalue;	/* complete hack */
					break;
			}

			previous_zvalue = zvalue;

			tri1.flags = 0;
			if (tex_header[0] & 0x8000)		tri1.flags |= TRI_FLAG_CHECKER;
			if (tex_header[0] & 0x4000)		tri1.flags |= TRI_FLAG_TEXTURE;
			if (tex_header[0] & 0x2000)		tri1.flags |= TRI_FLAG_TEXTURE_TRANS;
			if (tex_header[0] & 0x1000)		tri1.flags |= TRI_FLAG_MICROTEX;
			if (tex_header[0] & 0x0200)		tri1.flags |= TRI_FLAG_MIRROR_X;
			if (tex_header[0] & 0x0100)		tri1.flags |= TRI_FLAG_MIRROR_Y;
			if (tex_header[0] & 0x0080)		tri1.flags |= TRI_FLAG_WRAP_X;
			if (tex_header[0] & 0x0040)		tri1.flags |= TRI_FLAG_WRAP_Y;
			if (tex_header[2] & 0x1000)		tri1.flags |= TRI_FLAG_EVEN_BANK;
			tri2.flags = tri1.flags;

			tri1.tex_x = tri2.tex_x				= (tex_header[2] >> 6) & 0x3f;
			tri1.tex_y = tri2.tex_y				= (tex_header[2] >> 0) & 0x3f;
			tri1.tex_width = tri2.tex_width		= (tex_header[0] >> 3) & 0x7;
			tri1.tex_height = tri2.tex_height	= (tex_header[0] >> 0) & 0x7;
			tri1.transfer_map = tri2.transfer_map = tex_header[1] & 0xff;

			tri1.z = tri2.z = zvalue;
			tri1.r = tri2.r = (rgb >> 10) & 0x1f;
			tri1.g = tri2.g = (rgb >>  5) & 0x1f;
			tri1.b = tri2.b = (rgb >>  0) & 0x1f;
			tri1.luminance = tri2.luminance = lumatable[(int)(intensity)];

			//if (polytype == 1)
			{
				push_triangle(&tri1);
				push_triangle(&tri2);
			}
			//else
			//{
			//  push_triangle(&tri1);
			//}
		}

		switch (linktype)
		{
			case 0:
			case 2:
			{
				memcpy(&prev_p1, &p1, sizeof(VERTEX));
				memcpy(&prev_p2, &p2, sizeof(VERTEX));
				break;
			}

			case 1:
			{
				// prev P1 = prev P1
				memcpy(&prev_p2, &p1, sizeof(VERTEX));
				break;
			}

			case 3:
			{
				// prev P2 = prev P2
				memcpy(&prev_p1, &p2, sizeof(VERTEX));
				break;
			}
		}
	}
}

static int draw_direct_data(UINT32 *mem)
{
	int offset = 0;
	int end = 0;
	VERTEX prev_p1, prev_p2;
	UINT32 texture_point_address;
	UINT32 texture_header_address;

	texture_point_address	= mem[offset++];
	texture_header_address	= mem[offset++];
	prev_p1.x = *(float*)(&mem[offset++]);
	prev_p1.y = *(float*)(&mem[offset++]);
	prev_p1.z = *(float*)(&mem[offset++]);
	prev_p2.x = *(float*)(&mem[offset++]);
	prev_p2.y = *(float*)(&mem[offset++]);
	prev_p2.z = *(float*)(&mem[offset++]);

	while (!end)
	{
		UINT32 luminance_data;
		float d;
		VERTEX p1, p2;
		UINT32 attr;
		int polytype, linktype;

		attr = mem[offset++];
		polytype = attr & 0x3;

		if (polytype == 0)
		{
			return offset;
		}

		linktype = (attr >> 8) & 0x3;
		if (linktype != ((attr >> 23) & 0x3))
		{
			//return;
			fatalerror("draw_direct_data: Linktype not matching, attribute = %08X\n", attr);
		}

		luminance_data = mem[offset++];
		d		= *(float*)(&mem[offset++]);
		p1.x	= *(float*)(&mem[offset++]);
		p1.y	= *(float*)(&mem[offset++]);
		p1.z	= *(float*)(&mem[offset++]);
		p2.x	= *(float*)(&mem[offset++]);
		p2.y	= *(float*)(&mem[offset++]);
		p2.z	= *(float*)(&mem[offset++]);

		if (linktype != 0)
		{
			TRIANGLE tri1, tri2;
			tri1.v[0].x = p1.x;			tri1.v[0].y = p1.y;			tri1.v[0].z = p1.z;
			tri1.v[1].x = p2.x;			tri1.v[1].y = p2.y;			tri1.v[1].z = p2.z;
			tri1.v[2].x = prev_p1.x;	tri1.v[2].y = prev_p1.y;	tri1.v[2].z = prev_p1.z;
			tri2.v[0].x = p2.x;			tri2.v[0].y = p2.y;			tri2.v[0].z = p2.z;
			tri2.v[1].x = prev_p1.x;	tri2.v[1].y = prev_p1.y;	tri2.v[1].z = prev_p1.z;
			tri2.v[2].x = prev_p2.x;	tri2.v[2].y = prev_p2.y;	tri2.v[2].z = prev_p2.z;

			if (polytype == 1)
			{
				push_direct_triangle(&tri1);
				push_direct_triangle(&tri2);
			}
			else
			{
				push_direct_triangle(&tri1);
			}
		}

		switch (linktype)
		{
			case 0:
			case 2:
			{
				memcpy(&prev_p1, &p1, sizeof(VERTEX));
				memcpy(&prev_p2, &p2, sizeof(VERTEX));
				break;
			}

			case 1:
			{
				// prev P1 = prev P1
				memcpy(&prev_p2, &p1, sizeof(VERTEX));
				break;
			}

			case 3:
			{
				// prev P2 = prev P2
				memcpy(&prev_p1, &p2, sizeof(VERTEX));
				break;
			}
		}
	}

	return offset;
}

static void parse_display_list(void)
{
	UINT32 address, length, tex_point_addr, tex_hdr_addr;
	int i;
	int end;
	int dlptr;

	dlptr = geo_read_start_address / 4;

	end = 0;
	while (!end && dlptr < (0x20000/4))
	{
		UINT32 op = model2_bufferram[dlptr++];
		int cmd3 = (op >>  0) & 0xff;
		int cmd2 = (op >>  8) & 0xff;
		int cmd1 = (op >> 23) & 0xff;

		if (op & 0x80000000)
		{
			// Jump command
		}
		else
		{
			if (cmd2 != cmd1 || cmd3 != cmd1)
			{
				//mame_printf_debug("parse_display_list: CMD bits not matching: %08X at %08X\n", op, dlptr);
				//return;
				/*{
                    int i;
                    FILE *file = fopen("buffer2.bin", "wb");
                    for (i=0; i < 0x20000/4; i++)
                    {
                        fputc((model2_bufferram[i] >> 24) & 0xff, file);
                        fputc((model2_bufferram[i] >> 16) & 0xff, file);
                        fputc((model2_bufferram[i] >>  8) & 0xff, file);
                        fputc((model2_bufferram[i] >>  0) & 0xff, file);
                    }
                    fclose(file);
                }*/

				//fatalerror("parse_display_list: CMD bits not matching: %08X at %08X\n", op, dlptr*4);
			}

			switch (cmd1)
			{
				case 0x00:		// NOP
				{
					break;
				}
				case 0x01:		// Object Data
				{
					tex_point_addr	= model2_bufferram[dlptr+0];
					tex_hdr_addr	= model2_bufferram[dlptr+1];
					address			= model2_bufferram[dlptr+2];
					length			= model2_bufferram[dlptr+3];

					draw_object(address, length, tex_point_addr, tex_hdr_addr);

					dlptr += 4;
					break;
				}
				case 0x02:		// Direct Data
				{
					length = draw_direct_data(&model2_bufferram[dlptr]);
					dlptr += length;
					break;
				}
				case 0x03:		// Window Data
				{
					int x1 = (INT16)(model2_bufferram[dlptr+0] >> 16);
					int y1 = (INT16)(model2_bufferram[dlptr+0] >>  0);
					int x2 = (INT16)(model2_bufferram[dlptr+1] >> 16);
					int y2 = (INT16)(model2_bufferram[dlptr+1] >>  0);

					window.x = x1;
					window.y = 474 - y2;
					window.width = x2 - x1;
					window.height = (474 - y1) - window.y;

					dlptr += 6;
					break;
				}
				case 0x04:		// Texture Data Write
				{
					address		= model2_bufferram[dlptr++];
					length		= model2_bufferram[dlptr++];

					if ((address & 0x800000) == 0)
					{
						fatalerror("Trying to write log data at texture data write");
					}

					address &= 0x7fffff;

					for (i=0; i < length; i++)
					{
						texture_ram[address] = model2_bufferram[dlptr++];
						address++;
					}
					break;
				}
				case 0x05:		// Object Data Write
				{
					address		= model2_bufferram[dlptr++] & 0x7fff;
					length		= model2_bufferram[dlptr++] & 0xffff;

					for (i=0; i < length; i++)
					{
						polygon_ram[address] = model2_bufferram[dlptr++];
						address++;
					}
					break;
				}
				case 0x06:		// Texture Parameter Write
				{
					address		= model2_bufferram[dlptr++];
					length		= model2_bufferram[dlptr++];

					if (address > 0x1f)
					{
						mame_printf_debug("texture_parameter_write: address = %08X\n", address);
						break;
					}

					if (length > 0x20)
					{
						mame_printf_debug("texture_parameter_write: length = %08X\n", length);
						break;
					}

					for (i=0; i < length; i++)
					{
						UINT32 data = model2_bufferram[dlptr++];
						texture_parameter[i].diffuse	= (data >>  0) & 0xff;
						texture_parameter[i].ambient	= (data >>  8) & 0xff;
						texture_parameter[i].reflection	= (data >> 16) & 0xff;
						texture_parameter[i].c = *(float*)&model2_bufferram[dlptr++];
					}
					break;
				}
				case 0x07:		// Operation Mode
				{
					dlptr++;
					break;
				}
				case 0x08:		// Z-Sort Mode
				{
					dlptr++;
					break;
				}
				case 0x09:		// Focal Distance
				{
					focus_x = *(float*)&model2_bufferram[dlptr+0];
					focus_y = *(float*)&model2_bufferram[dlptr+1];
					dlptr += 2;
					break;
				}
				case 0x0a:		// Light Source Vector
				{
					light_vector[0] = *(float*)&model2_bufferram[dlptr++];
					light_vector[1] = *(float*)&model2_bufferram[dlptr++];
					light_vector[2] = *(float*)&model2_bufferram[dlptr++];
					break;
				}
				case 0x0b:		// Matrix
				{
					matrix[ 0] = *(float*)&model2_bufferram[dlptr++];
					matrix[ 1] = *(float*)&model2_bufferram[dlptr++];
					matrix[ 2] = *(float*)&model2_bufferram[dlptr++];
					matrix[ 3] = *(float*)&model2_bufferram[dlptr++];
					matrix[ 4] = *(float*)&model2_bufferram[dlptr++];
					matrix[ 5] = *(float*)&model2_bufferram[dlptr++];
					matrix[ 6] = *(float*)&model2_bufferram[dlptr++];
					matrix[ 7] = *(float*)&model2_bufferram[dlptr++];
					matrix[ 8] = *(float*)&model2_bufferram[dlptr++];
					matrix[ 9] = *(float*)&model2_bufferram[dlptr++];
					matrix[10] = *(float*)&model2_bufferram[dlptr++];
					matrix[11] = *(float*)&model2_bufferram[dlptr++];
					break;
				}
				case 0x0c:		// Parallel Transfer Vector
				{
					matrix[ 9] = *(float*)&model2_bufferram[dlptr++];
					matrix[10] = *(float*)&model2_bufferram[dlptr++];
					matrix[11] = *(float*)&model2_bufferram[dlptr++];
					break;
				}
				case 0x0e:		// TGP Test
				{
					dlptr += 32;
					length = model2_bufferram[dlptr++];
					dlptr += length;
					break;
				}
				case 0x0f:		// End
				{
					end = 1;
					break;
				}
				case 0x10:		// Dummy
				{
					dlptr++;
					break;
				}
				case 0x14:		// Log Data Write
				{
					dlptr++;
					length = model2_bufferram[dlptr++];
					dlptr += length;
					break;
				}
				case 0x16:		// LOD
				{
					dlptr++;
					break;
				}
				default:
				{
					/*
                    {
                        int i;
                        FILE *file = fopen("buffer2.bin", "wb");
                        for (i=0; i < 0x20000/4; i++)
                        {
                            fputc((model2_bufferram[i] >> 24) & 0xff, file);
                            fputc((model2_bufferram[i] >> 16) & 0xff, file);
                            fputc((model2_bufferram[i] >>  8) & 0xff, file);
                            fputc((model2_bufferram[i] >>  0) & 0xff, file);
                        }
                        fclose(file);
                    }
                    fatalerror("parse_display_list: unknown command %02X (%08X) at %08X\n", cmd1, op, dlptr*4);
                    */
				}
			}
		}
	};
}
