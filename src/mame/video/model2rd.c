/********************************************************************

    Sega Model 2 3D rasterization functions

********************************************************************/

#undef MODEL2_CHECKER
#undef MODEL2_TEXTURED
#undef MODEL2_TRANSLUCENT

#ifndef MODEL2_FUNC
#error "Model 2 renderer: No function defined!"
#endif

#ifndef MODEL2_FUNC_NAME
#error "Model 2 renderer: No function name defined!"
#endif

#if MODEL2_FUNC == 0
#undef MODEL2_CHECKER
#undef MODEL2_TEXTURED
#undef MODEL2_TRANSLUCENT
#elif MODEL2_FUNC == 1
#undef MODEL2_CHECKER
#undef MODEL2_TEXTURED
#define MODEL2_TRANSLUCENT
#elif MODEL2_FUNC == 2
#undef MODEL2_CHECKER
#define MODEL2_TEXTURED
#undef MODEL2_TRANSLUCENT
#elif MODEL2_FUNC == 3
#undef MODEL2_CHECKER
#define MODEL2_TEXTURED
#define MODEL2_TRANSLUCENT
#elif MODEL2_FUNC == 4
#define MODEL2_CHECKER
#undef MODEL2_TEXTURED
#undef MODEL2_TRANSLUCENT
#elif MODEL2_FUNC == 5
#define MODEL2_CHECKER
#undef MODEL2_TEXTURED
#define MODEL2_TRANSLUCENT
#elif MODEL2_FUNC == 6
#define MODEL2_CHECKER
#define MODEL2_TEXTURED
#undef MODEL2_TRANSLUCENT
#elif MODEL2_FUNC == 7
#define MODEL2_CHECKER
#define MODEL2_TEXTURED
#define MODEL2_TRANSLUCENT
#elif
#error "Model 2 renderer: Invalif function selected!"
#endif

#ifndef MODEL2_TEXTURED
/* non-textured render path */
static void MODEL2_FUNC_NAME ( mame_bitmap *bitmap, triangle *tri, const rectangle *cliprect )
{
	struct poly_vertex vert[3];
	const struct poly_scanline_data *scans;

	/* if it's translucent, there's nothing to render */
#if defined( MODEL2_TRANSLUCENT)
	return;
#endif

	/* setup vertices to be scanned */
	vert[0].x = (INT32)tri->v[0].x;	vert[0].y = (INT32)tri->v[0].y;
	vert[1].x = (INT32)tri->v[1].x;	vert[1].y = (INT32)tri->v[1].y;
	vert[2].x = (INT32)tri->v[2].x;	vert[2].y = (INT32)tri->v[2].y;

	/* get the scan parameters */
	scans = setup_triangle_0(&vert[0], &vert[1], &vert[2], cliprect);

	/* see if we have something to render */
	if ( scans )
	{
		/* extract color information */
		const UINT16 *colortable_r = (const UINT16 *)&model2_colorxlat[0x0000/4];
		const UINT16 *colortable_g = (const UINT16 *)&model2_colorxlat[0x4000/4];
		const UINT16 *colortable_b = (const UINT16 *)&model2_colorxlat[0x8000/4];
		const UINT16 *lumaram = (const UINT16 *)model2_lumaram;
		const UINT16 *palram = (const UINT16 *)paletteram32;
		UINT32	lumabase = ((tri->texheader[1] & 0xFF) << 7) + ((tri->luma >> 5) ^ 0x07);
		UINT32	color = (tri->texheader[3] >> 6) & 0x3FF;
		UINT8	luma;
		UINT32	tr, tg, tb;
		int		x, y;

		luma = lumaram[BYTE_XOR_LE(lumabase + (0xf << 3))] & 0x3F;

		color = palram[BYTE_XOR_LE(color + 0x1000)] & 0x7fff;

		colortable_r += ((color >>  0) & 0x1f) << 8;
		colortable_g += ((color >>  5) & 0x1f) << 8;
		colortable_b += ((color >> 10) & 0x1f) << 8;

		/* we have the 6 bits of luma information along with 5 bits per color component */
		/* now build and index into the master color lookup table and extract the raw RGB values */

		tr = colortable_r[BYTE_XOR_LE(luma)] & 0xff;
		tg = colortable_g[BYTE_XOR_LE(luma)] & 0xff;
		tb = colortable_b[BYTE_XOR_LE(luma)] & 0xff;

		/* build the final color */
		color = MAKE_RGB(tr, tg, tb);

		/* go through our scan list and paint the lines */
		for(y = scans->sy; y <= scans->ey; y++)
		{
			const struct poly_scanline *scan = &scans->scanline[y - scans->sy];
			UINT32 *p = BITMAP_ADDR32(bitmap, y, 0);
			int x1, x2;

			x1 = scan->sx;
			x2 = scan->ex;

			for(x = x1; x <= x2; x++)
#if defined(MODEL2_CHECKER)
				if ((x^y) & 1) p[x] = color;
#else
				p[x] = color;
#endif
		}
	}
}

#else
/* textured render path */
static void MODEL2_FUNC_NAME ( mame_bitmap *bitmap, triangle *tri, const rectangle *cliprect )
{
	struct poly_vertex vert[3];
	const struct poly_scanline_data *scans;
	UINT32	tex_width	= (tri->texheader[0] >> 0) & 0x7;
	UINT32	tex_height	= (tri->texheader[0] >> 3) & 0x7;
	int i;

	tex_width = 32 << tex_width;		/* or 1 << (5 + tex_width) */
	tex_height = 32 << tex_height;

	for( i = 0; i < 3; i++ )
	{
		float ooz = 1.0 / (1.0 + tri->v[i].z);

		vert[i].x = (INT32)tri->v[i].x;
		vert[i].y = (INT32)tri->v[i].y;

		/* p[0] = 1/z in 1.3.28 format */
		vert[i].p[0] = (INT32)(ooz * (float)(1 << 28));

		/* p[1] and p[2] are u/z and v/z in 1.12.19 */
		vert[i].p[1] = ((INT64)tri->v[i].u * (INT64)vert[i].p[0]) >> 12;
		vert[i].p[2] = ((INT64)tri->v[i].v * (INT64)vert[i].p[0]) >> 12;
	}

	/* get the scan parameters */
	scans = setup_triangle_3(&vert[0], &vert[1], &vert[2], cliprect);

	/* see if we have something to render */
	if ( scans )
	{
		INT64 duoz, dvoz, dooz;

		/* extract color information */
		const UINT16 *colortable_r = (const UINT16 *)&model2_colorxlat[0x0000/4];
		const UINT16 *colortable_g = (const UINT16 *)&model2_colorxlat[0x4000/4];
		const UINT16 *colortable_b = (const UINT16 *)&model2_colorxlat[0x8000/4];
		const UINT16 *lumaram = (const UINT16 *)model2_lumaram;
		const UINT16 *palram = (const UINT16 *)paletteram32;
		UINT32	colorbase = (tri->texheader[3] >> 6) & 0x3FF;
		UINT32	lumabase = ((tri->texheader[1] & 0xFF) << 7) + ((tri->luma >> 5) ^ 0x7);
		UINT32	tex_x		= (tri->texheader[2] >> 0) & 0x1f;
		UINT32	tex_y		= (tri->texheader[2] >> 6) & 0x1f;
		UINT32	tex_x_mask, tex_y_mask;
		UINT32	tex_mirr_x	= (tri->texheader[0] >> 9) & 1;
		UINT32	tex_mirr_y	= (tri->texheader[0] >> 8) & 1;
		UINT32 *sheet;
		int		x, y;

		if ( tri->texheader[2] & 0x20 )
			tex_y += 0x20;

		tex_x = 32 * tex_x;
		tex_y = 32 * tex_y;
		tex_x_mask	= tex_width - 1;
		tex_y_mask	= tex_height - 1;

		sheet = model2_textureram0;

		if (tri->texheader[2] & 0x1000)
			sheet = model2_textureram1;

		colorbase = palram[BYTE_XOR_LE(colorbase + 0x1000)] & 0x7fff;

		colortable_r += ((colorbase >>  0) & 0x1f) << 8;
		colortable_g += ((colorbase >>  5) & 0x1f) << 8;
		colortable_b += ((colorbase >> 10) & 0x1f) << 8;

		dooz = scans->dp[0];
		duoz = scans->dp[1];
		dvoz = scans->dp[2];

		for(y = scans->sy; y <= scans->ey; y++)
		{
			const struct poly_scanline *scan = &scans->scanline[y - scans->sy];
			UINT32 *p = BITMAP_ADDR32(bitmap, y, 0);
			INT64 uoz, voz, ooz;
			int x1, x2;

			x1 = scan->sx;
			x2 = scan->ex;

			ooz = scans->scanline[y - scans->sy].p[0];
			uoz = scans->scanline[y - scans->sy].p[1];
			voz = scans->scanline[y - scans->sy].p[2];

			for(x = x1; x <= x2; x++, uoz += duoz, voz += dvoz, ooz += dooz)
			{
				/* ooz is in 1.3.44 format (1.3.28 plus an extra 0.0.16) */
				/* uoz and voz are in 1.12.35 format (1.12.19 plus an extra 0.0.16) */
				INT32 z = ((UINT64)1 << 60) / ooz;	/* 16.16 */
				INT32 u = ((uoz >> 16) * z) >> 27;	/* 12.8 */
				INT32 v = ((voz >> 16) * z) >> 27;	/* 12.8 */
				UINT32	tr, tg, tb;
				UINT16	t;
				UINT8 luma;
				int u2;
				int v2;

#if defined(MODEL2_CHECKER)
				if ( ((x^y) & 1) == 0 )
					continue;
#endif
				u2 = (u >> 8) & tex_x_mask;
				v2 = (v >> 8) & tex_y_mask;

				if ( tex_mirr_x )
					u2 = ( tex_width - 1 ) - u2;

				if ( tex_mirr_y )
					v2 = ( tex_height - 1 ) - v2;

				t = get_texel( tex_x, tex_y, u2, v2, sheet );

#if defined(MODEL2_TRANSLUCENT)
				if ( t == 0x0f )
					continue;
#endif
				luma = lumaram[BYTE_XOR_LE(lumabase + (t << 3))] & 0x3f;

				/* we have the 6 bits of luma information along with 5 bits per color component */
				/* now build and index into the master color lookup table and extract the raw RGB values */

				tr = colortable_r[BYTE_XOR_LE(luma)] & 0xff;
				tg = colortable_g[BYTE_XOR_LE(luma)] & 0xff;
				tb = colortable_b[BYTE_XOR_LE(luma)] & 0xff;

				p[x] = MAKE_RGB(tr, tg, tb);
			}
		}
	}
}

#endif
