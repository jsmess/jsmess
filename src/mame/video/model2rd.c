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
		UINT32	color = (tri->texheader[3] >> 6) & 0x3FF;
		UINT32	lumabase = (tri->texheader[1] & 0xFF);
		UINT32	lumaindex = (lumabase << 8) + 0xF0 + (((tri->luma >> 4) & 0x0f) ^ 0x0f);
		UINT32	luma = model2_lumaram[lumaindex/4];
		UINT32	r, g, b, ri, gi, bi, tr, tg, tb, pval;
		UINT32 *colortable_r = &model2_colorxlat[0x0000/4];
		UINT32 *colortable_g = &model2_colorxlat[0x4000/4];
		UINT32 *colortable_b = &model2_colorxlat[0x8000/4];
		int		x, y;

		if ( lumaindex & 2 )
			luma >>= 16;

		luma &= 0x3F;

		pval = paletteram32[(0x2000/4)+(color >> 1)];

		if ( color & 1 )
			color = (pval >> 16);
		else
			color = pval;

		color &= 0x7fff;

		r = (color >>  0) & 0x1f;
		g = (color >>  5) & 0x1f;
		b = (color >> 10) & 0x1f;

		/* we have the 6 bits of luma information along with 5 bits per color component */
		/* now build and index into the master color lookup table and extract the raw RGB values */

		ri = (r<<9) + (luma<<1);
		gi = (g<<9) + (luma<<1);
		bi = (b<<9) + (luma<<1);

		tr = colortable_r[ri>>2];
		tg = colortable_g[gi>>2];
		tb = colortable_b[bi>>2];

		if ( ri & 2 ) tr >>= 16;
		if ( gi & 2 ) tg >>= 16;
		if ( bi & 2 ) tb >>= 16;

		tr &= 0xff;
		tg &= 0xff;
		tb &= 0xff;

		/* build the final color */
		color = (tr << 16) | (tg << 8) | (tb);

		/* go through our scan list and paint the lines */
		for(y = scans->sy; y <= scans->ey; y++)
		{
			int x1, x2;
			const struct poly_scanline *scan = &scans->scanline[y - scans->sy];
			UINT32 *p = BITMAP_ADDR32(bitmap, y, 0);

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
	float	zu[3], zv[3], tz[3];
	int i;

	tex_width = 32 << tex_width;
	tex_height = 32 << tex_height;

	for( i = 0; i < 3; i++ )
	{
		zu[i] = ((float)tri->v[i].u) / 8.0f;
		zv[i] = ((float)tri->v[i].v) / 8.0f;

		zu[i] /= (float)tex_width;
		zv[i] /= (float)tex_height;

		/* now u,v are within 0-1 range */

		tz[i] = 1.0/(1.0+tri->v[i].z);

		zu[i] *= tz[i];
		zv[i] *= tz[i];

		tz[i] *= 1048576.0f;
		zu[i] *= 1048576.0f;
		zv[i] *= 1048576.0f;
	}

	vert[0].x = (INT32)tri->v[0].x;		vert[0].y = (INT32)tri->v[0].y;		vert[0].p[0] = (INT32)zu[0];	vert[0].p[1] = (INT32)zv[0];	vert[0].p[2] = (INT32)tz[0];
	vert[1].x = (INT32)tri->v[1].x;		vert[1].y = (INT32)tri->v[1].y;		vert[1].p[0] = (INT32)zu[1];	vert[1].p[1] = (INT32)zv[1];	vert[1].p[2] = (INT32)tz[1];
	vert[2].x = (INT32)tri->v[2].x;		vert[2].y = (INT32)tri->v[2].y;		vert[2].p[0] = (INT32)zu[2];	vert[2].p[1] = (INT32)zv[2];	vert[2].p[2] = (INT32)tz[2];

	/* get the scan parameters */
	scans = setup_triangle_3(&vert[0], &vert[1], &vert[2], cliprect);

	/* see if we have something to render */
	if ( scans )
	{
		INT64 du, dv, dz;

		/* extract color information */
		UINT32	r, g, b, pval;
		UINT32	colorbase = (tri->texheader[3] >> 6) & 0x3FF;
		UINT32	lumabase = (tri->texheader[1] & 0xFF );
		UINT32	tex_x		= (tri->texheader[2] >> 0) & 0x1f;
		UINT32	tex_y		= (tri->texheader[2] >> 6) & 0x1f;
		UINT32	tex_x_mask, tex_y_mask;
		UINT32	tex_mirr_x	= (tri->texheader[0] >> 9) & 1;
		UINT32	tex_mirr_y	= (tri->texheader[0] >> 8) & 1;
		UINT32 *colortable_r = &model2_colorxlat[0x0000/4];
		UINT32 *colortable_g = &model2_colorxlat[0x4000/4];
		UINT32 *colortable_b = &model2_colorxlat[0x8000/4];
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

		pval = paletteram32[(0x2000/4)+(colorbase >> 1)];

		if ( colorbase & 1 )
			colorbase = (pval >> 16);
		else
			colorbase = pval;

		colorbase &= 0x7fff;

		r = (colorbase >>  0) & 0x1f;
		g = (colorbase >>  5) & 0x1f;
		b = (colorbase >> 10) & 0x1f;

		du = scans->dp[0];
		dv = scans->dp[1];
		dz = scans->dp[2];

		for(y = scans->sy; y <= scans->ey; y++)
		{
			INT64 u, v, z;
			int x1, x2;
			UINT32	lumaindex;
			const struct poly_scanline *scan = &scans->scanline[y - scans->sy];
			UINT32 *p = BITMAP_ADDR32(bitmap, y, 0);

			x1 = scan->sx;
			x2 = scan->ex;

			u = scans->scanline[y - scans->sy].p[0];
			v = scans->scanline[y - scans->sy].p[1];
			z = scans->scanline[y - scans->sy].p[2];

			for(x = x1; x <= x2; x++, u += du, v += dv, z += dz)
			{
				UINT32	tr, tg, tb, luma, ri, gi, bi;
				UINT16	t;
				float	uf = ((float)(u>>16)) / 1048576.0;
				float	vf = ((float)(v>>16)) / 1048576.0;
				float	zf = ((float)(z>>16)) / 1048576.0;
				int u2;
				int v2;

#if defined(MODEL2_CHECKER)
				if ( ((x^y) & 1) == 0 )
					continue;
#endif
				u2 = (int)((uf/zf)*tex_width) & tex_x_mask;
				v2 = (int)((vf/zf)*tex_height) & tex_y_mask;

				if ( tex_mirr_x )
					u2 = ( tex_width - 1 ) - u2;

				if ( tex_mirr_y )
					v2 = ( tex_height - 1 ) - v2;

				t = get_texel( tex_x, tex_y, u2, v2, sheet );

#if defined(MODEL2_TRANSLUCENT)
				if ( t == 0x0f )
					continue;
#endif
				lumaindex = (lumabase << 8) + ( t << 4 ) + (((tri->luma >> 4) & 0x0f) ^ 0x0f);
				luma = model2_lumaram[lumaindex/4];

				if ( lumaindex & 2 )
					luma >>= 16;

				luma &= 0x3F;

				/* we have the 6 bits of luma information along with 5 bits per color component */
				/* now build and index into the master color lookup table and extract the raw RGB values */

				ri = (r<<9) + (luma<<1);
				gi = (g<<9) + (luma<<1);
				bi = (b<<9) + (luma<<1);

				tr = colortable_r[ri>>2];
				tg = colortable_g[gi>>2];
				tb = colortable_b[bi>>2];

				if ( ri & 2 ) tr >>= 16;
				if ( gi & 2 ) tg >>= 16;
				if ( bi & 2 ) tb >>= 16;

				tr &= 0xff;
				tg &= 0xff;
				tb &= 0xff;

				p[x] = (tr << 16) | (tg << 8) | (tb);
			}
		}
	}
}

#endif
