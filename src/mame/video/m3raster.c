static void draw_triangle_tex1555(VERTEX v1, VERTEX v2, VERTEX v3)
{
	int x, y;
	struct poly_vertex vert[3];
	const struct poly_scanline_data *scans;

	v1.z = (1.0 / v1.z) * ZBUFFER_SCALE;
	v2.z = (1.0 / v2.z) * ZBUFFER_SCALE;
	v3.z = (1.0 / v3.z) * ZBUFFER_SCALE;
	v1.u = (UINT32)((UINT64)(v1.u * v1.z) >> ZDIVIDE_SHIFT);
	v1.v = (UINT32)((UINT64)(v1.v * v1.z) >> ZDIVIDE_SHIFT);
	v2.u = (UINT32)((UINT64)(v2.u * v2.z) >> ZDIVIDE_SHIFT);
	v2.v = (UINT32)((UINT64)(v2.v * v2.z) >> ZDIVIDE_SHIFT);
	v3.u = (UINT32)((UINT64)(v3.u * v3.z) >> ZDIVIDE_SHIFT);
	v3.v = (UINT32)((UINT64)(v3.v * v3.z) >> ZDIVIDE_SHIFT);

	vert[0].x = v1.x;	vert[0].y = v1.y;	vert[0].p[0] = (UINT32)v1.z;	vert[0].p[1] = v1.u;	vert[0].p[2] = v1.v;
	vert[1].x = v2.x;	vert[1].y = v2.y;	vert[1].p[0] = (UINT32)v2.z;	vert[1].p[1] = v2.u;	vert[1].p[2] = v2.v;
	vert[2].x = v3.x;	vert[2].y = v3.y;	vert[2].p[0] = (UINT32)v3.z;	vert[2].p[1] = v3.u;	vert[2].p[2] = v3.v;

	scans = setup_triangle_3(&vert[0], &vert[1], &vert[2], &clip3d);

	if(scans)
	{
		INT64 du, dv, dz;
		dz = scans->dp[0];
		du = scans->dp[1];
		dv = scans->dp[2];
		for(y = scans->sy; y <= scans->ey; y++) {
			int x1, x2;
			INT64 u, v, z;
			INT64 u2, v2;
			const struct poly_scanline *scan = &scans->scanline[y - scans->sy];
			UINT16 *p = BITMAP_ADDR16(bitmap3d, y, 0);
			UINT32 *d = BITMAP_ADDR32(zbuffer, y, 0);

			x1 = scan->sx;
			x2 = scan->ex;

			z = scans->scanline[y - scans->sy].p[0];
			u = scans->scanline[y - scans->sy].p[1];
			v = scans->scanline[y - scans->sy].p[2];

			for(x = x1; x <= x2; x++) {
//              UINT16 pix;
				int iu, iv;

				UINT32 iz = z >> 16;

				if (iz) {
					u2 = (u << ZDIVIDE_SHIFT) / iz;
					v2 = (v << ZDIVIDE_SHIFT) / iz;
				} else {
					u2 = 0;
					v2 = 0;
				}

				iz |= viewport_priority;

				if(iz > d[x]) {
					iu = texture_u_table[(u2 >> texture_coord_shift) & texture_width_mask];
					iv = texture_v_table[(v2 >> texture_coord_shift) & texture_height_mask];
#if BILINEAR
					{
						int iu2 = texture_u_table[((u2 >> texture_coord_shift) + 1) & texture_width_mask];
						int iv2 = texture_v_table[((v2 >> texture_coord_shift) + 1) & texture_height_mask];
						UINT32 sr[4], sg[4], sb[4];
						UINT32 ur[2], ug[2], ub[2];
						UINT32 fr, fg, fb;
						UINT16 pix0 = texture_ram[texture_page][(texture_y+iv) * 2048 + (texture_x+iu)];
						UINT16 pix1 = texture_ram[texture_page][(texture_y+iv) * 2048 + (texture_x+iu2)];
						UINT16 pix2 = texture_ram[texture_page][(texture_y+iv2) * 2048 + (texture_x+iu)];
						UINT16 pix3 = texture_ram[texture_page][(texture_y+iv2) * 2048 + (texture_x+iu2)];
						int u_sub1 = (u2 >> (texture_coord_shift-16)) & 0xffff;
						int v_sub1 = (v2 >> (texture_coord_shift-16)) & 0xffff;
						int u_sub0 = 0xffff - u_sub1;
						int v_sub0 = 0xffff - v_sub1;
						sr[0] = (pix0 & 0x7c00);
						sg[0] = (pix0 & 0x03e0);
						sb[0] = (pix0 & 0x001f);
						sr[1] = (pix1 & 0x7c00);
						sg[1] = (pix1 & 0x03e0);
						sb[1] = (pix1 & 0x001f);
						sr[2] = (pix2 & 0x7c00);
						sg[2] = (pix2 & 0x03e0);
						sb[2] = (pix2 & 0x001f);
						sr[3] = (pix3 & 0x7c00);
						sg[3] = (pix3 & 0x03e0);
						sb[3] = (pix3 & 0x001f);

						/* Calculate weighted U-samples */
						ur[0] = (((sr[0] * u_sub0) >> 16) + ((sr[1] * u_sub1) >> 16));
						ug[0] = (((sg[0] * u_sub0) >> 16) + ((sg[1] * u_sub1) >> 16));
						ub[0] = (((sb[0] * u_sub0) >> 16) + ((sb[1] * u_sub1) >> 16));
						ur[1] = (((sr[2] * u_sub0) >> 16) + ((sr[3] * u_sub1) >> 16));
						ug[1] = (((sg[2] * u_sub0) >> 16) + ((sg[3] * u_sub1) >> 16));
						ub[1] = (((sb[2] * u_sub0) >> 16) + ((sb[3] * u_sub1) >> 16));
						/* Calculate the final sample */
						fr = (((ur[0] * v_sub0) >> 16) + ((ur[1] * v_sub1) >> 16));
						fg = (((ug[0] * v_sub0) >> 16) + ((ug[1] * v_sub1) >> 16));
						fb = (((ub[0] * v_sub0) >> 16) + ((ub[1] * v_sub1) >> 16));

						// apply intensity
						fr = (fr * polygon_intensity) >> 8;
						fg = (fg * polygon_intensity) >> 8;
						fb = (fb * polygon_intensity) >> 8;

						p[x] = (fr & 0x7c00) | (fg & 0x3e0) | (fb & 0x1f);
					}
#else
					pix = texture_ram[texture_page][(texture_y+iv) * 2048 + (texture_x+iu)];
					p[x] = pix & 0x7fff;
#endif
					d[x] = iz;		/* write new zbuffer value */
				}

				z += dz;
				u += du;
				v += dv;
			}
		}
	}
}

static void draw_triangle_tex1555_trans(VERTEX v1, VERTEX v2, VERTEX v3)
{
	int x, y;
	struct poly_vertex vert[3];
	const struct poly_scanline_data *scans;

	v1.z = (1.0 / v1.z) * ZBUFFER_SCALE;
	v2.z = (1.0 / v2.z) * ZBUFFER_SCALE;
	v3.z = (1.0 / v3.z) * ZBUFFER_SCALE;
	v1.u = (UINT32)((UINT64)(v1.u * v1.z) >> ZDIVIDE_SHIFT);
	v1.v = (UINT32)((UINT64)(v1.v * v1.z) >> ZDIVIDE_SHIFT);
	v2.u = (UINT32)((UINT64)(v2.u * v2.z) >> ZDIVIDE_SHIFT);
	v2.v = (UINT32)((UINT64)(v2.v * v2.z) >> ZDIVIDE_SHIFT);
	v3.u = (UINT32)((UINT64)(v3.u * v3.z) >> ZDIVIDE_SHIFT);
	v3.v = (UINT32)((UINT64)(v3.v * v3.z) >> ZDIVIDE_SHIFT);

	vert[0].x = v1.x;	vert[0].y = v1.y;	vert[0].p[0] = (UINT32)v1.z;	vert[0].p[1] = v1.u;	vert[0].p[2] = v1.v;
	vert[1].x = v2.x;	vert[1].y = v2.y;	vert[1].p[0] = (UINT32)v2.z;	vert[1].p[1] = v2.u;	vert[1].p[2] = v2.v;
	vert[2].x = v3.x;	vert[2].y = v3.y;	vert[2].p[0] = (UINT32)v3.z;	vert[2].p[1] = v3.u;	vert[2].p[2] = v3.v;

	scans = setup_triangle_3(&vert[0], &vert[1], &vert[2], &clip3d);

	if(scans)
	{
		INT64 du, dv, dz;
		dz = scans->dp[0];
		du = scans->dp[1];
		dv = scans->dp[2];
		for(y = scans->sy; y <= scans->ey; y++) {
			int x1, x2;
			INT64 u, v, z;
			INT64 u2, v2;
			const struct poly_scanline *scan = &scans->scanline[y - scans->sy];
			UINT16 *p = BITMAP_ADDR16(bitmap3d, y, 0);
			UINT32 *d = BITMAP_ADDR32(zbuffer, y, 0);

			x1 = scan->sx;
			x2 = scan->ex;

			z = scans->scanline[y - scans->sy].p[0];
			u = scans->scanline[y - scans->sy].p[1];
			v = scans->scanline[y - scans->sy].p[2];

			for(x = x1; x <= x2; x++) {
//              UINT16 pix;
				int iu, iv;

				UINT32 iz = z >> 16;

				if (iz) {
					u2 = (u << ZDIVIDE_SHIFT) / iz;
					v2 = (v << ZDIVIDE_SHIFT) / iz;
				} else {
					u2 = 0;
					v2 = 0;
				}

				iz |= viewport_priority;

				if(iz > d[x])
				{
					iu = texture_u_table[(u2 >> texture_coord_shift) & texture_width_mask];
					iv = texture_v_table[(v2 >> texture_coord_shift) & texture_height_mask];
#if BILINEAR
					{
						int iu2 = texture_u_table[((u2 >> texture_coord_shift) + 1) & texture_width_mask];
						int iv2 = texture_v_table[((v2 >> texture_coord_shift) + 1) & texture_height_mask];
						UINT32 sr[4], sg[4], sb[4];
						UINT32 ur[2], ug[2], ub[2];
						UINT32 fr, fg, fb;
						UINT32 pr, pg, pb;
						UINT16 pix0 = texture_ram[texture_page][(texture_y+iv) * 2048 + (texture_x+iu)];
						UINT16 pix1 = texture_ram[texture_page][(texture_y+iv) * 2048 + (texture_x+iu2)];
						UINT16 pix2 = texture_ram[texture_page][(texture_y+iv2) * 2048 + (texture_x+iu)];
						UINT16 pix3 = texture_ram[texture_page][(texture_y+iv2) * 2048 + (texture_x+iu2)];
						int u_sub1 = (u2 >> (texture_coord_shift-16)) & 0xffff;
						int v_sub1 = (v2 >> (texture_coord_shift-16)) & 0xffff;
						int u_sub0 = 0xffff - u_sub1;
						int v_sub0 = 0xffff - v_sub1;
						sr[0] = (pix0 & 0x7c00);
						sg[0] = (pix0 & 0x03e0);
						sb[0] = (pix0 & 0x001f);
						sr[1] = (pix1 & 0x7c00);
						sg[1] = (pix1 & 0x03e0);
						sb[1] = (pix1 & 0x001f);
						sr[2] = (pix2 & 0x7c00);
						sg[2] = (pix2 & 0x03e0);
						sb[2] = (pix2 & 0x001f);
						sr[3] = (pix3 & 0x7c00);
						sg[3] = (pix3 & 0x03e0);
						sb[3] = (pix3 & 0x001f);

						/* Calculate weighted U-samples */
						ur[0] = (((sr[0] * u_sub0) >> 16) + ((sr[1] * u_sub1) >> 16));
						ug[0] = (((sg[0] * u_sub0) >> 16) + ((sg[1] * u_sub1) >> 16));
						ub[0] = (((sb[0] * u_sub0) >> 16) + ((sb[1] * u_sub1) >> 16));
						ur[1] = (((sr[2] * u_sub0) >> 16) + ((sr[3] * u_sub1) >> 16));
						ug[1] = (((sg[2] * u_sub0) >> 16) + ((sg[3] * u_sub1) >> 16));
						ub[1] = (((sb[2] * u_sub0) >> 16) + ((sb[3] * u_sub1) >> 16));
						/* Calculate the final sample */
						fr = (((ur[0] * v_sub0) >> 16) + ((ur[1] * v_sub1) >> 16));
						fg = (((ug[0] * v_sub0) >> 16) + ((ug[1] * v_sub1) >> 16));
						fb = (((ub[0] * v_sub0) >> 16) + ((ub[1] * v_sub1) >> 16));

						// apply intensity
						fr = (fr * polygon_intensity) >> 8;
						fg = (fg * polygon_intensity) >> 8;
						fb = (fb * polygon_intensity) >> 8;

						/* Blend with existing framebuffer pixels */
						pr = (p[x] & 0x7c00);
						pg = (p[x] & 0x03e0);
						pb = (p[x] & 0x001f);
						fr = ((pr * (31-polygon_transparency)) + (fr * polygon_transparency)) >> 5;
						fg = ((pg * (31-polygon_transparency)) + (fg * polygon_transparency)) >> 5;
						fb = ((pb * (31-polygon_transparency)) + (fb * polygon_transparency)) >> 5;

						p[x] = (fr & 0x7c00) | (fg & 0x3e0) | (fb & 0x1f);
					}
#else
					pix = texture_ram[texture_page][(texture_y+iv) * 2048 + (texture_x+iu)];
					p[x] = pix & 0x7fff;
#endif
				}

				z += dz;
				u += du;
				v += dv;
			}
		}
	}
}

static void draw_triangle_tex1555_alpha(VERTEX v1, VERTEX v2, VERTEX v3)
{
	int x, y;
	struct poly_vertex vert[3];
	const struct poly_scanline_data *scans;

	v1.z = (1.0 / v1.z) * ZBUFFER_SCALE;
	v2.z = (1.0 / v2.z) * ZBUFFER_SCALE;
	v3.z = (1.0 / v3.z) * ZBUFFER_SCALE;
	v1.u = (UINT32)((UINT64)(v1.u * v1.z) >> ZDIVIDE_SHIFT);
	v1.v = (UINT32)((UINT64)(v1.v * v1.z) >> ZDIVIDE_SHIFT);
	v2.u = (UINT32)((UINT64)(v2.u * v2.z) >> ZDIVIDE_SHIFT);
	v2.v = (UINT32)((UINT64)(v2.v * v2.z) >> ZDIVIDE_SHIFT);
	v3.u = (UINT32)((UINT64)(v3.u * v3.z) >> ZDIVIDE_SHIFT);
	v3.v = (UINT32)((UINT64)(v3.v * v3.z) >> ZDIVIDE_SHIFT);

	vert[0].x = v1.x;	vert[0].y = v1.y;	vert[0].p[0] = (UINT32)v1.z;	vert[0].p[1] = v1.u;	vert[0].p[2] = v1.v;
	vert[1].x = v2.x;	vert[1].y = v2.y;	vert[1].p[0] = (UINT32)v2.z;	vert[1].p[1] = v2.u;	vert[1].p[2] = v2.v;
	vert[2].x = v3.x;	vert[2].y = v3.y;	vert[2].p[0] = (UINT32)v3.z;	vert[2].p[1] = v3.u;	vert[2].p[2] = v3.v;

	scans = setup_triangle_3(&vert[0], &vert[1], &vert[2], &clip3d);

	if(scans)
	{
		INT64 du, dv, dz;
		dz = scans->dp[0];
		du = scans->dp[1];
		dv = scans->dp[2];
		for(y = scans->sy; y <= scans->ey; y++) {
			int x1, x2;
			INT64 u, v, z;
			INT64 u2, v2;
			const struct poly_scanline *scan = &scans->scanline[y - scans->sy];
			UINT16 *p = BITMAP_ADDR16(bitmap3d, y, 0);
			UINT32 *d = BITMAP_ADDR32(zbuffer, y, 0);

			x1 = scan->sx;
			x2 = scan->ex;
			z = scans->scanline[y - scans->sy].p[0];
			u = scans->scanline[y - scans->sy].p[1];
			v = scans->scanline[y - scans->sy].p[2];

			for(x = x1; x <= x2; x++) {
	//          UINT16 pix;
				int iu, iv;

				UINT32 iz = z >> 16;

				if (iz) {
					u2 = (u << ZDIVIDE_SHIFT) / iz;
					v2 = (v << ZDIVIDE_SHIFT) / iz;
				} else {
					u2 = 0;
					v2 = 0;
				}

				iz |= viewport_priority;

				if(iz >= d[x]) {
					iu = texture_u_table[(u2 >> texture_coord_shift) & texture_width_mask];
					iv = texture_v_table[(v2 >> texture_coord_shift) & texture_height_mask];
#if BILINEAR
					{
						int iu2 = texture_u_table[((u2 >> texture_coord_shift) + 1) & texture_width_mask];
						int iv2 = texture_v_table[((v2 >> texture_coord_shift) + 1) & texture_height_mask];
						UINT32 sr[4], sg[4], sb[4], sa[4];
						UINT32 ur[2], ug[2], ub[2], ua[4];
						UINT32 fr, fg, fb, fa;
						UINT32 pr, pg, pb;
						UINT16 pix0 = texture_ram[texture_page][(texture_y+iv) * 2048 + (texture_x+iu)];
						UINT16 pix1 = texture_ram[texture_page][(texture_y+iv) * 2048 + (texture_x+iu2)];
						UINT16 pix2 = texture_ram[texture_page][(texture_y+iv2) * 2048 + (texture_x+iu)];
						UINT16 pix3 = texture_ram[texture_page][(texture_y+iv2) * 2048 + (texture_x+iu2)];
						int u_sub1 = (u2 >> (texture_coord_shift-16)) & 0xffff;
						int v_sub1 = (v2 >> (texture_coord_shift-16)) & 0xffff;
						int u_sub0 = 0xffff - u_sub1;
						int v_sub0 = 0xffff - v_sub1;
						sr[0] = (pix0 & 0x7c00);
						sg[0] = (pix0 & 0x03e0);
						sb[0] = (pix0 & 0x001f);
						sa[0] = (pix0 & 0x8000) ? 0 : 16;
						sr[1] = (pix1 & 0x7c00);
						sg[1] = (pix1 & 0x03e0);
						sb[1] = (pix1 & 0x001f);
						sa[1] = (pix1 & 0x8000) ? 0 : 16;
						sr[2] = (pix2 & 0x7c00);
						sg[2] = (pix2 & 0x03e0);
						sb[2] = (pix2 & 0x001f);
						sa[2] = (pix2 & 0x8000) ? 0 : 16;
						sr[3] = (pix3 & 0x7c00);
						sg[3] = (pix3 & 0x03e0);
						sb[3] = (pix3 & 0x001f);
						sa[3] = (pix3 & 0x8000) ? 0 : 16;

						/* Calculate weighted U-samples */
						ur[0] = (((sr[0] * u_sub0) >> 16) + ((sr[1] * u_sub1) >> 16));
						ug[0] = (((sg[0] * u_sub0) >> 16) + ((sg[1] * u_sub1) >> 16));
						ub[0] = (((sb[0] * u_sub0) >> 16) + ((sb[1] * u_sub1) >> 16));
						ua[0] = (((sa[0] * u_sub0) >> 16) + ((sa[1] * u_sub1) >> 16));
						ur[1] = (((sr[2] * u_sub0) >> 16) + ((sr[3] * u_sub1) >> 16));
						ug[1] = (((sg[2] * u_sub0) >> 16) + ((sg[3] * u_sub1) >> 16));
						ub[1] = (((sb[2] * u_sub0) >> 16) + ((sb[3] * u_sub1) >> 16));
						ua[1] = (((sa[2] * u_sub0) >> 16) + ((sa[3] * u_sub1) >> 16));
						/* Calculate the final sample */
						fr = (((ur[0] * v_sub0) >> 16) + ((ur[1] * v_sub1) >> 16));
						fg = (((ug[0] * v_sub0) >> 16) + ((ug[1] * v_sub1) >> 16));
						fb = (((ub[0] * v_sub0) >> 16) + ((ub[1] * v_sub1) >> 16));
						fa = (((ua[0] * v_sub0) >> 16) + ((ua[1] * v_sub1) >> 16));

						// apply intensity
						fr = (fr * polygon_intensity) >> 8;
						fg = (fg * polygon_intensity) >> 8;
						fb = (fb * polygon_intensity) >> 8;

						/* Blend with existing framebuffer pixels */
						pr = (p[x] & 0x7c00);
						pg = (p[x] & 0x03e0);
						pb = (p[x] & 0x001f);
						fr = ((pr * (16 - fa)) + (fr * fa)) >> 4;
						fg = ((pg * (16 - fa)) + (fg * fa)) >> 4;
						fb = ((pb * (16 - fa)) + (fb * fa)) >> 4;

						p[x] = (fr & 0x7c00) | (fg & 0x3e0) | (fb & 0x1f);
					}
#else
					pix = texture_ram[texture_page][(texture_y+iv) * 2048 + (texture_x+iu)];
					p[x] = pix & 0x7fff;
#endif
				}

				z += dz;
				u += du;
				v += dv;
			}
		}
	}
}

static void draw_triangle_tex4444(VERTEX v1, VERTEX v2, VERTEX v3)
{
	int x, y;
	struct poly_vertex vert[3];
	const struct poly_scanline_data *scans;

	v1.z = (1.0 / v1.z) * ZBUFFER_SCALE;
	v2.z = (1.0 / v2.z) * ZBUFFER_SCALE;
	v3.z = (1.0 / v3.z) * ZBUFFER_SCALE;
	v1.u = (UINT32)((UINT64)(v1.u * v1.z) >> ZDIVIDE_SHIFT);
	v1.v = (UINT32)((UINT64)(v1.v * v1.z) >> ZDIVIDE_SHIFT);
	v2.u = (UINT32)((UINT64)(v2.u * v2.z) >> ZDIVIDE_SHIFT);
	v2.v = (UINT32)((UINT64)(v2.v * v2.z) >> ZDIVIDE_SHIFT);
	v3.u = (UINT32)((UINT64)(v3.u * v3.z) >> ZDIVIDE_SHIFT);
	v3.v = (UINT32)((UINT64)(v3.v * v3.z) >> ZDIVIDE_SHIFT);

	vert[0].x = v1.x;	vert[0].y = v1.y;	vert[0].p[0] = (UINT32)v1.z;	vert[0].p[1] = v1.u;	vert[0].p[2] = v1.v;
	vert[1].x = v2.x;	vert[1].y = v2.y;	vert[1].p[0] = (UINT32)v2.z;	vert[1].p[1] = v2.u;	vert[1].p[2] = v2.v;
	vert[2].x = v3.x;	vert[2].y = v3.y;	vert[2].p[0] = (UINT32)v3.z;	vert[2].p[1] = v3.u;	vert[2].p[2] = v3.v;
	scans = setup_triangle_3(&vert[0], &vert[1], &vert[2], &clip3d);

	if(scans)
	{
		INT64 du, dv, dz;
		dz = scans->dp[0];
		du = scans->dp[1];
		dv = scans->dp[2];
		for(y = scans->sy; y <= scans->ey; y++) {
			int x1, x2;
			INT64 u, v, z;
			INT64 u2, v2;
			const struct poly_scanline *scan = &scans->scanline[y - scans->sy];
			UINT16 *p = BITMAP_ADDR16(bitmap3d, y, 0);
			UINT32 *d = BITMAP_ADDR32(zbuffer, y, 0);

			x1 = scan->sx;
			x2 = scan->ex;

			z = scans->scanline[y - scans->sy].p[0];
			u = scans->scanline[y - scans->sy].p[1];
			v = scans->scanline[y - scans->sy].p[2];

			for(x = x1; x <= x2; x++) {
//              UINT16 pix;
//              UINT16 r,g,b;
				int iu, iv;

				UINT32 iz = z >> 16;

				if (iz) {
					u2 = (u << ZDIVIDE_SHIFT) / iz;
					v2 = (v << ZDIVIDE_SHIFT) / iz;
				} else {
					u2 = 0;
					v2 = 0;
				}

				iz |= viewport_priority;

				if(iz > d[x]) {
					iu = texture_u_table[(u2 >> texture_coord_shift) & texture_width_mask];
					iv = texture_v_table[(v2 >> texture_coord_shift) & texture_height_mask];
#if BILINEAR
					{
						int iu2 = texture_u_table[((u2 >> texture_coord_shift) + 1) & texture_width_mask];
						int iv2 = texture_v_table[((v2 >> texture_coord_shift) + 1) & texture_height_mask];
						UINT32 sr[4], sg[4], sb[4];
						UINT32 ur[2], ug[2], ub[2];
						UINT32 fr, fg, fb;
						UINT16 pix0 = texture_ram[texture_page][(texture_y+iv) * 2048 + (texture_x+iu)];
						UINT16 pix1 = texture_ram[texture_page][(texture_y+iv) * 2048 + (texture_x+iu2)];
						UINT16 pix2 = texture_ram[texture_page][(texture_y+iv2) * 2048 + (texture_x+iu)];
						UINT16 pix3 = texture_ram[texture_page][(texture_y+iv2) * 2048 + (texture_x+iu2)];
						int u_sub1 = (u2 >> (texture_coord_shift-16)) & 0xffff;
						int v_sub1 = (v2 >> (texture_coord_shift-16)) & 0xffff;
						int u_sub0 = 0xffff - u_sub1;
						int v_sub0 = 0xffff - v_sub1;
						sr[0] = (pix0 & 0xf000);
						sg[0] = (pix0 & 0x0f00);
						sb[0] = (pix0 & 0x00f0);
						sr[1] = (pix1 & 0xf000);
						sg[1] = (pix1 & 0x0f00);
						sb[1] = (pix1 & 0x00f0);
						sr[2] = (pix2 & 0xf000);
						sg[2] = (pix2 & 0x0f00);
						sb[2] = (pix2 & 0x00f0);
						sr[3] = (pix3 & 0xf000);
						sg[3] = (pix3 & 0x0f00);
						sb[3] = (pix3 & 0x00f0);

						/* Calculate weighted U-samples */
						ur[0] = (((sr[0] * u_sub0) >> 16) + ((sr[1] * u_sub1) >> 16));
						ug[0] = (((sg[0] * u_sub0) >> 16) + ((sg[1] * u_sub1) >> 16));
						ub[0] = (((sb[0] * u_sub0) >> 16) + ((sb[1] * u_sub1) >> 16));
						ur[1] = (((sr[2] * u_sub0) >> 16) + ((sr[3] * u_sub1) >> 16));
						ug[1] = (((sg[2] * u_sub0) >> 16) + ((sg[3] * u_sub1) >> 16));
						ub[1] = (((sb[2] * u_sub0) >> 16) + ((sb[3] * u_sub1) >> 16));
						/* Calculate the final sample */
						fr = (((ur[0] * v_sub0) >> 16) + ((ur[1] * v_sub1) >> 16));
						fg = (((ug[0] * v_sub0) >> 16) + ((ug[1] * v_sub1) >> 16));
						fb = (((ub[0] * v_sub0) >> 16) + ((ub[1] * v_sub1) >> 16));

						// apply intensity
						fr = (fr * polygon_intensity) >> 8;
						fg = (fg * polygon_intensity) >> 8;
						fb = (fb * polygon_intensity) >> 8;

						p[x] = ((fr & 0xf800) >> 1) | ((fg & 0x0f80) >> 2) | ((fb & 0x00f8) >> 3);
					}
#else
					pix = texture_ram[texture_page][(texture_y+iv) * 2048 + (texture_x+iu)];
					r = (pix & 0xf000) >> 1;
					g = (pix & 0x0f00) >> 2;
					b = (pix & 0x00f0) >> 3;
					p[x] = r | g | b;
#endif
					d[x] = iz;		/* write new zbuffer value */
				}

				z += dz;
				u += du;
				v += dv;
			}
		}
	}
}

static void draw_triangle_tex4444_alpha(VERTEX v1, VERTEX v2, VERTEX v3)
{
	int x, y;
	struct poly_vertex vert[3];
	const struct poly_scanline_data *scans;

	v1.z = (1.0 / v1.z) * ZBUFFER_SCALE;
	v2.z = (1.0 / v2.z) * ZBUFFER_SCALE;
	v3.z = (1.0 / v3.z) * ZBUFFER_SCALE;
	v1.u = (UINT32)((UINT64)(v1.u * v1.z) >> ZDIVIDE_SHIFT);
	v1.v = (UINT32)((UINT64)(v1.v * v1.z) >> ZDIVIDE_SHIFT);
	v2.u = (UINT32)((UINT64)(v2.u * v2.z) >> ZDIVIDE_SHIFT);
	v2.v = (UINT32)((UINT64)(v2.v * v2.z) >> ZDIVIDE_SHIFT);
	v3.u = (UINT32)((UINT64)(v3.u * v3.z) >> ZDIVIDE_SHIFT);
	v3.v = (UINT32)((UINT64)(v3.v * v3.z) >> ZDIVIDE_SHIFT);

	vert[0].x = v1.x;	vert[0].y = v1.y;	vert[0].p[0] = (UINT32)v1.z;	vert[0].p[1] = v1.u;	vert[0].p[2] = v1.v;
	vert[1].x = v2.x;	vert[1].y = v2.y;	vert[1].p[0] = (UINT32)v2.z;	vert[1].p[1] = v2.u;	vert[1].p[2] = v2.v;
	vert[2].x = v3.x;	vert[2].y = v3.y;	vert[2].p[0] = (UINT32)v3.z;	vert[2].p[1] = v3.u;	vert[2].p[2] = v3.v;
	scans = setup_triangle_3(&vert[0], &vert[1], &vert[2], &clip3d);

	if(scans)
	{
		INT64 du, dv, dz;
		dz = scans->dp[0];
		du = scans->dp[1];
		dv = scans->dp[2];
		for(y = scans->sy; y <= scans->ey; y++) {
			int x1, x2;
			INT64 u, v, z;
			INT64 u2, v2;
			const struct poly_scanline *scan = &scans->scanline[y - scans->sy];
			UINT16 *p = BITMAP_ADDR16(bitmap3d, y, 0);
			UINT32 *d = BITMAP_ADDR32(zbuffer, y, 0);

			x1 = scan->sx;
			x2 = scan->ex;
			z = scans->scanline[y - scans->sy].p[0];
			u = scans->scanline[y - scans->sy].p[1];
			v = scans->scanline[y - scans->sy].p[2];

			for(x = x1; x <= x2; x++) {
	//          UINT16 pix;
	//          UINT16 r,g,b;
				int iu, iv;

				UINT32 iz = z >> 16;

				if (iz) {
					u2 = (u << ZDIVIDE_SHIFT) / iz;
					v2 = (v << ZDIVIDE_SHIFT) / iz;
				} else {
					u2 = 0;
					v2 = 0;
				}

				iz |= viewport_priority;

				if(iz >= d[x]) {
					iu = texture_u_table[(u2 >> texture_coord_shift) & texture_width_mask];
					iv = texture_v_table[(v2 >> texture_coord_shift) & texture_height_mask];
#if BILINEAR
					{
						int iu2 = texture_u_table[((u2 >> texture_coord_shift) + 1) & texture_width_mask];
						int iv2 = texture_v_table[((v2 >> texture_coord_shift) + 1) & texture_height_mask];
						UINT32 sr[4], sg[4], sb[4], sa[4];
						UINT32 ur[2], ug[2], ub[2], ua[4];
						UINT32 pr, pg, pb;//, br, bg, bb;
						UINT32 fr, fg, fb, fa;
						UINT16 pix0 = texture_ram[texture_page][(texture_y+iv) * 2048 + (texture_x+iu)];
						UINT16 pix1 = texture_ram[texture_page][(texture_y+iv) * 2048 + (texture_x+iu2)];
						UINT16 pix2 = texture_ram[texture_page][(texture_y+iv2) * 2048 + (texture_x+iu)];
						UINT16 pix3 = texture_ram[texture_page][(texture_y+iv2) * 2048 + (texture_x+iu2)];
						int u_sub1 = (u2 >> (texture_coord_shift-16)) & 0xffff;
						int v_sub1 = (v2 >> (texture_coord_shift-16)) & 0xffff;
						int u_sub0 = 0xffff - u_sub1;
						int v_sub0 = 0xffff - v_sub1;
						sr[0] = (pix0 & 0xf000);
						sg[0] = (pix0 & 0x0f00);
						sb[0] = (pix0 & 0x00f0);
						sa[0] = (pix0 & 0x000f) + ((pix0 >> 1) & 1);
						sr[1] = (pix1 & 0xf000);
						sg[1] = (pix1 & 0x0f00);
						sb[1] = (pix1 & 0x00f0);
						sa[1] = (pix1 & 0x000f) + ((pix1 >> 1) & 1);
						sr[2] = (pix2 & 0xf000);
						sg[2] = (pix2 & 0x0f00);
						sb[2] = (pix2 & 0x00f0);
						sa[2] = (pix2 & 0x000f) + ((pix2 >> 1) & 1);
						sr[3] = (pix3 & 0xf000);
						sg[3] = (pix3 & 0x0f00);
						sb[3] = (pix3 & 0x00f0);
						sa[3] = (pix3 & 0x000f) + ((pix3 >> 1) & 1);

						/* Calculate weighted U-samples */
						ur[0] = ((sr[0] * u_sub0) + (sr[1] * u_sub1)) >> 16;
						ug[0] = ((sg[0] * u_sub0) + (sg[1] * u_sub1)) >> 16;
						ub[0] = ((sb[0] * u_sub0) + (sb[1] * u_sub1)) >> 16;
						ua[0] = ((sa[0] * u_sub0) + (sa[1] * u_sub1)) >> 16;
						ur[1] = ((sr[2] * u_sub0) + (sr[3] * u_sub1)) >> 16;
						ug[1] = ((sg[2] * u_sub0) + (sg[3] * u_sub1)) >> 16;
						ub[1] = ((sb[2] * u_sub0) + (sb[3] * u_sub1)) >> 16;
						ua[1] = ((sa[2] * u_sub0) + (sa[3] * u_sub1)) >> 16;
						/* Calculate the final sample */
						fr = ((ur[0] * v_sub0) + (ur[1] * v_sub1)) >> 16;
						fg = ((ug[0] * v_sub0) + (ug[1] * v_sub1)) >> 16;
						fb = ((ub[0] * v_sub0) + (ub[1] * v_sub1)) >> 16;
						fa = ((ua[0] * v_sub0) + (ua[1] * v_sub1)) >> 16;

						// apply intensity
						fr = (fr * polygon_intensity) >> 8;
						fg = (fg * polygon_intensity) >> 8;
						fb = (fb * polygon_intensity) >> 8;

						fa = (polygon_transparency * fa) >> 5;

						/* Blend with existing framebuffer pixels */
						pr = (p[x] & 0x7c00) << 1;
						pg = (p[x] & 0x03e0) << 2;
						pb = (p[x] & 0x001f) << 3;
						fr = ((pr * (16 - fa)) + (fr * fa)) >> 4;
						fg = ((pg * (16 - fa)) + (fg * fa)) >> 4;
						fb = ((pb * (16 - fa)) + (fb * fa)) >> 4;

						p[x] = ((fr & 0xf800) >> 1) | ((fg & 0x0f80) >> 2) | ((fb & 0x00f8) >> 3);
					}
#else
					pix = texture_ram[texture_page][(texture_y+iv) * 2048 + (texture_x+iu)];
					r = (pix & 0xf000) >> 1;
					g = (pix & 0x0f00) >> 2;
					b = (pix & 0x00f0) >> 3;
					p[x] = r | g | b;
#endif
				}

				z += dz;
				u += du;
				v += dv;
			}
		}
	}
}

static void draw_triangle_color(VERTEX v1, VERTEX v2, VERTEX v3, UINT16 color)
{
	int x, y;
	struct poly_vertex vert[3];
	const struct poly_scanline_data *scans;

	v1.z = (1.0 / v1.z) * ZBUFFER_SCALE;
	v2.z = (1.0 / v2.z) * ZBUFFER_SCALE;
	v3.z = (1.0 / v3.z) * ZBUFFER_SCALE;

	vert[0].x = v1.x;	vert[0].y = v1.y;	vert[0].p[0] = (UINT32)v1.z;
	vert[1].x = v2.x;	vert[1].y = v2.y;	vert[1].p[0] = (UINT32)v2.z;
	vert[2].x = v3.x;	vert[2].y = v3.y;	vert[2].p[0] = (UINT32)v3.z;

	scans = setup_triangle_1(&vert[0], &vert[1], &vert[2], &clip3d);

	if(scans)
	{
		INT64 dz = scans->dp[0];
		for(y = scans->sy; y <= scans->ey; y++) {
			int x1, x2;
			INT64 z;
			const struct poly_scanline *scan = &scans->scanline[y - scans->sy];
			UINT16 *p = BITMAP_ADDR16(bitmap3d, y, 0);
			UINT32 *d = BITMAP_ADDR32(zbuffer, y, 0);

			x1 = scan->sx;
			x2 = scan->ex;

			z = scans->scanline[y - scans->sy].p[0];

			for(x = x1; x <= x2; x++) {
				UINT32 fr, fg, fb;
				UINT32 iz = z >> 16;

				iz |= viewport_priority;

				if(iz > d[x]) {
					fr = color & 0x7c00;
					fg = color & 0x03e0;
					fb = color & 0x001f;

					// apply intensity
					fr = (fr * polygon_intensity) >> 8;
					fg = (fg * polygon_intensity) >> 8;
					fb = (fb * polygon_intensity) >> 8;

					p[x] = (fr & 0x7c00) | (fg & 0x03e0) | (fb & 0x1f);
					d[x] = iz;		/* write new zbuffer value */
				}
				z += dz;
			}
		}
	}
}

static void draw_triangle_color_trans(VERTEX v1, VERTEX v2, VERTEX v3, UINT16 color)
{
	int x, y;
	struct poly_vertex vert[3];
	const struct poly_scanline_data *scans;

	v1.z = (1.0 / v1.z) * ZBUFFER_SCALE;
	v2.z = (1.0 / v2.z) * ZBUFFER_SCALE;
	v3.z = (1.0 / v3.z) * ZBUFFER_SCALE;

	vert[0].x = v1.x;	vert[0].y = v1.y;	vert[0].p[0] = (UINT32)v1.z;
	vert[1].x = v2.x;	vert[1].y = v2.y;	vert[1].p[0] = (UINT32)v2.z;
	vert[2].x = v3.x;	vert[2].y = v3.y;	vert[2].p[0] = (UINT32)v3.z;

	scans = setup_triangle_1(&vert[0], &vert[1], &vert[2], &clip3d);

	if(scans)
	{
		INT64 dz = scans->dp[0];
		for(y = scans->sy; y <= scans->ey; y++) {
			int x1, x2;
			INT64 z;
			UINT32 fr, fg, fb;
			UINT32 pr, pg, pb;
			const struct poly_scanline *scan = &scans->scanline[y - scans->sy];
			UINT16 *p = BITMAP_ADDR16(bitmap3d, y, 0);
			UINT32 *d = BITMAP_ADDR32(zbuffer, y, 0);

			x1 = scan->sx;
			x2 = scan->ex;

			z = scans->scanline[y - scans->sy].p[0];

			for(x = x1; x <= x2; x++) {
				UINT32 iz = z >> 16;

				iz |= viewport_priority;

				if(iz > d[x]) {
					fr = color & 0x7c00;
					fg = color & 0x03e0;
					fb = color & 0x001f;

					// apply intensity
					fr = (fr * polygon_intensity) >> 8;
					fg = (fg * polygon_intensity) >> 8;
					fb = (fb * polygon_intensity) >> 8;

					/* Blend with existing framebuffer pixels */
					pr = (p[x] & 0x7c00);
					pg = (p[x] & 0x03e0);
					pb = (p[x] & 0x001f);
					fr = ((pr * (31 - polygon_transparency)) + (fr * polygon_transparency)) >> 5;
					fg = ((pg * (31 - polygon_transparency)) + (fg * polygon_transparency)) >> 5;
					fb = ((pb * (31 - polygon_transparency)) + (fb * polygon_transparency)) >> 5;
					p[x] = (fr & 0x7c00) | (fg & 0x03e0) | (fb & 0x1f);
				}
				z += dz;
			}
		}
	}
}
