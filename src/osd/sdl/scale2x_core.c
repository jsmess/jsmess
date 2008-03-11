/* scale2x algorithm (Andrea Mazzoleni, http://advancemame.sourceforge.net):
 *
 * A 9-pixel rectangle is taken from the source bitmap:
 *
 *  a b c
 *  d e f
 *  g h i
 *
 * The central pixel e is expanded into four new pixels,
 *
 *  e0 e1
 *  e2 e3
 *
 * where
 *
 *  e0 = (d == b && b != f && d != h) ? d : e;
 *  e1 = (b == f && b != d && f != h) ? f : e;
 *  e2 = (d == h && d != b && h != f) ? d : e;
 *  e3 = (h == f && d != h && b != f) ? f : e;
 *
 * Copyright stuff:
 * 
 * 1st According to the copyright header in the original C file this code is
 * DUAL licensed under:
 * A) GPL (with an extra clause)(see file COPYING)
 * B) you are allowed to use this code in your program with these conditions:
 *    - the program is not used in commercial activities.
 *    - the whole source code of the program is released with the binary.
 *    - derivative works of the program are allowed.
 * IANAL but IMHO xmame matches the demands for B.
 *
 * 2nd the version of the GPL used contains this extra clause:
 *
 ****************************************************************************
 * 
 * The AdvanceMAME/MESS sources are released under the
 * GNU General Public License (GPL) with this special exception
 * added to every source file :
 * 
 *     "In addition, as a special exception, Andrea Mazzoleni
 *     gives permission to link the code of this program with
 *     the MAME library (or with modified versions of MAME that use the
 *     same license as MAME), and distribute linked combinations including
 *     the two.  You must obey the GNU General Public License in all
 *     respects for all of the code used other than MAME.  If you modify
 *     this file, you may extend this exception to your version of the
 *     file, but you are not obligated to do so.  If you do not wish to
 *     do so, delete this exception statement from your version."
 * 
 * This imply that if you distribute a binary version of AdvanceMAME/MESS
 * linked with the MAME and MESS sources you must also follow the MAME
 * License.
 *
 ****************************************************************************
 *
 * So basicly this code may be distributed under the MAME license, as is
 * xmame
 *
 * 3th The person who originaly incorperated this effect into
 * xmame, got explicited permission, see permission.txt.
 *
 * J.W.R. de Goede, Rotterdam the Netherlands, 17 december 2004.
 */

INLINE void FUNC_NAME(scale2x_border)(DEST_TYPE *dst, DEST_TYPE *src0,
	DEST_TYPE *src1, DEST_TYPE *src2, int count,
	int borderpix)
{
	/* first pixel */
	if (borderpix)
		*dst++ = 0;
	
	if (src0[0] != src2[0] && src1[0] != src1[1]) {
		*dst++ = src1[0] == src0[0] ? src0[0] : src1[0];
		*dst++ = src1[1] == src0[0] ? src0[0] : src1[0];
	} else {
		*dst++ = src1[0];
		*dst++ = src1[0];
	}
	++src0;
	++src1;
	++src2;

	/* central pixels */
	count -= 2;
	while (count) {
		if (src1[-1] == src0[0] && src2[0] != src0[0] && src1[ 1] != src0[0])
			*dst++ = src0[0];
		else *dst++ = src1[0];

		if (src1[ 1] == src0[0] && src2[0] != src0[0] && src1[-1] != src0[0])
			*dst++ = src0[0];
		else *dst++ = src1[0];

		src0++;
		src1++;
		src2++;
		count--;
	}

	/* last pixel */
	if (src0[0] != src2[0] && src1[-1] != src1[0]) {
		*dst++ = src1[-1] == src0[0] ? src0[0] : src1[0];
		*dst++ = src1[0] == src0[0] ? src0[0] : src1[0];
	} else {
		*dst++ = src1[0];
		*dst++ = src1[0];
	}
	if (borderpix)
		*dst++ = 0;
}

INLINE void FUNC_NAME(scale2x_center)(DEST_TYPE *dst, DEST_TYPE *src0,
	DEST_TYPE *src1, DEST_TYPE *src2, int count,
	int borderpix)
{
	/* first pixel */
	if (borderpix)
		*dst++ = 0;
	
	if (src0[0] != src2[0] && src1[0] != src1[1]) {
		*dst++ = src1[0];
		*dst++ = (src1[1] == src0[0] && src1[0] != src2[1]) || (src1[1] == src2[0] && src1[0] != src0[1]) ? src1[1] : src1[0];
	} else {
		*dst++ = src1[0];
		*dst++ = src1[0];
	}
	++src0;
	++src1;
	++src2;

	/* central pixels */
	count -= 2;
	while (count) {
		if (src0[0] != src2[0] && src1[-1] != src1[1]) {
			*dst++ = (src1[-1] == src0[0] && src1[0] != src2[-1]) || (src1[-1] == src2[0] && src1[0] != src0[-1]) ? src1[-1] : src1[0];
			*dst++ = (src1[1] == src0[0] && src1[0] != src2[1]) || (src1[1] == src2[0] && src1[0] != src0[1]) ? src1[1] : src1[0];
		} else {
			*dst++ = src1[0];
			*dst++ = src1[0];
		}

		++src0;
		++src1;
		++src2;
		--count;
	}

	/* last pixel */
	if (src0[0] != src2[0] && src1[-1] != src1[0]) {
		*dst++ = (src1[-1] == src0[0] && src1[0] != src2[-1]) || (src1[-1] == src2[0] && src1[0] != src0[-1]) ? src1[-1] : src1[0];
		*dst++ = src1[0];
	} else {
		*dst++ = src1[0];
		*dst++ = src1[0];
	}
	if (borderpix)
		*dst++ = 0;
}

INLINE void FUNC_NAME(scale3x_border)(DEST_TYPE *dst, DEST_TYPE *src0,
	DEST_TYPE *src1, DEST_TYPE *src2, int count,
	int borderpix)
{
	/* first pixel */
	if (borderpix)
		*dst++ = 0;
	
	if (src0[0] != src2[0] && src1[0] != src1[1]) {
		*dst++ = src1[0];
		*dst++ = (src1[0] == src0[0] && src1[0] != src0[1]) || (src1[1] == src0[0] && src1[0] != src0[0]) ? src0[0] : src1[0];
		*dst++ = src1[1] == src0[0] ? src1[1] : src1[0];
	} else {
		*dst++ = src1[0];
		*dst++ = src1[0];
		*dst++ = src1[0];
	}
	++src0;
	++src1;
	++src2;

	/* central pixels */
	count -= 2;
	while (count) {
		if (src0[0] != src2[0] && src1[-1] != src1[1]) {
			*dst++ = src1[-1] == src0[0] ? src1[-1] : src1[0];
			*dst++ = (src1[-1] == src0[0] && src1[0] != src0[1]) || (src1[1] == src0[0] && src1[0] != src0[-1]) ? src0[0] : src1[0];
			*dst++ = src1[1] == src0[0] ? src1[1] : src1[0];
		} else {
			*dst++ = src1[0];
			*dst++ = src1[0];
			*dst++ = src1[0];
		}

		++src0;
		++src1;
		++src2;
		--count;
	}

	/* last pixel */
	if (src0[0] != src2[0] && src1[-1] != src1[0]) {
		*dst++ = src1[-1] == src0[0] ? src1[-1] : src1[0];
		*dst++ = (src1[-1] == src0[0] && src1[0] != src0[0]) || (src1[0] == src0[0] && src1[0] != src0[-1]) ? src0[0] : src1[0];
		*dst++ = src1[0];
	} else {
		*dst++ = src1[0];
		*dst++ = src1[0];
		*dst++ = src1[0];
	}
	if (borderpix)
		*dst++ = 0;
}

INLINE void FUNC_NAME(scale3x_center)(DEST_TYPE *dst, DEST_TYPE *src0,
	DEST_TYPE *src1, DEST_TYPE *src2, int count,
	int borderpix)
{
	/* first pixel */
	if (borderpix)
		*dst++ = 0;
	
	if (src0[0] != src2[0] && src1[0] != src1[1]) {
		*dst++ = (src1[0] == src0[0] && src1[0] != src2[0]) || (src1[0] == src2[0] && src1[0] != src0[0]) ? src1[0] : src1[0];
		*dst++ = src1[0];
		*dst++ = (src1[1] == src0[0] && src1[0] != src2[1]) || (src1[1] == src2[0] && src1[0] != src0[1]) ? src1[1] : src1[0];
	} else {
		*dst++ = src1[0];
		*dst++ = src1[0];
		*dst++ = src1[0];
	}
	++src0;
	++src1;
	++src2;

	/* central pixels */
	count -= 2;
	while (count) {
		if (src0[0] != src2[0] && src1[-1] != src1[1]) {
			*dst++ = (src1[-1] == src0[0] && src1[0] != src2[-1]) || (src1[-1] == src2[0] && src1[0] != src0[-1]) ? src1[-1] : src1[0];
			*dst++ = src1[0];
			*dst++ = (src1[1] == src0[0] && src1[0] != src2[1]) || (src1[1] == src2[0] && src1[0] != src0[1]) ? src1[1] : src1[0];
		} else {
			*dst++ = src1[0];
			*dst++ = src1[0];
			*dst++ = src1[0];
		}

		++src0;
		++src1;
		++src2;
		--count;
	}

	/* last pixel */
	if (src0[0] != src2[0] && src1[-1] != src1[0]) {
		*dst++ = (src1[-1] == src0[0] && src1[0] != src2[-1]) || (src1[-1] == src2[0] && src1[0] != src0[-1]) ? src1[-1] : src1[0];
		*dst++ = src1[0];
		*dst++ = (src1[0] == src0[0] && src1[0] != src2[0]) || (src1[0] == src2[0] && src1[0] != src0[0]) ? src1[0] : src1[0];
	} else {
		*dst++ = src1[0];
		*dst++ = src1[0];
		*dst++ = src1[0];
	}
	if (borderpix)
		*dst++ = 0;
}

INLINE void FUNC_NAME(scale2x2)(DEST_TYPE **dst, DEST_TYPE *src0,
	DEST_TYPE *src1, DEST_TYPE *src2, texture_info *texture,
	const render_texinfo *texsource)
{
	FUNC_NAME(scale2x_border)(*dst, src0, src1, src2, texsource->width,
		texture->borderpix);
	FUNC_NAME(scale2x_border)(*dst += texture->rawwidth, src2,
		src1, src0, texsource->width, texture->borderpix);
}

INLINE void FUNC_NAME(scale2x3)(DEST_TYPE **dst, DEST_TYPE *src0,
	DEST_TYPE *src1, DEST_TYPE *src2, texture_info *texture,
	const render_texinfo *texsource)
{
	FUNC_NAME(scale2x_border)(*dst, src0, src1, src2, texsource->width,
		texture->borderpix);
	FUNC_NAME(scale2x_center)(*dst += texture->rawwidth, src0,
		src1, src2, texsource->width, texture->borderpix);
	FUNC_NAME(scale2x_border)(*dst += texture->rawwidth, src2,
		src1, src0, texsource->width, texture->borderpix);
}

INLINE void FUNC_NAME(scale3x2)(DEST_TYPE **dst, DEST_TYPE *src0,
	DEST_TYPE *src1, DEST_TYPE *src2, texture_info *texture,
	const render_texinfo *texsource)
{
	FUNC_NAME(scale3x_border)(*dst, src0, src1, src2, texsource->width,
		texture->borderpix);
	FUNC_NAME(scale3x_border)(*dst += texture->rawwidth, src2,
		src1, src0, texsource->width, texture->borderpix);
}

INLINE void FUNC_NAME(scale3x3)(DEST_TYPE **dst, DEST_TYPE *src0,
	DEST_TYPE *src1, DEST_TYPE *src2, texture_info *texture,
	const render_texinfo *texsource)
{
	FUNC_NAME(scale3x_border)(*dst, src0, src1, src2, texsource->width,
		texture->borderpix);
	FUNC_NAME(scale3x_center)(*dst += texture->rawwidth, src0,
		src1, src2, texsource->width, texture->borderpix);
	FUNC_NAME(scale3x_border)(*dst += texture->rawwidth, src2,
		src1, src0, texsource->width, texture->borderpix);
}

INLINE void FUNC_NAME(scaleXxX)(DEST_TYPE **dst, DEST_TYPE *src0,
	DEST_TYPE *src1, DEST_TYPE *src2, texture_info *texture,
	const render_texinfo *texsource)
{
	switch ((texture->xprescale << 8) | texture->yprescale)
	{
		case 0x0202:
			FUNC_NAME(scale2x2)(dst, src0, src1, src2, texture,
				texsource);
			break;
		case 0x0203:
			FUNC_NAME(scale2x3)(dst, src0, src1, src2, texture,
				texsource);
			break;
		case 0x0302:
			FUNC_NAME(scale3x2)(dst, src0, src1, src2, texture,
				texsource);
			break;
		case 0x0303:
			FUNC_NAME(scale3x3)(dst, src0, src1, src2, texture,
				texsource);
			break;
	}
}

#undef DEST_TYPE
#undef FUNC_NAME
