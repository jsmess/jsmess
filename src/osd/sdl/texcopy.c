
#ifndef SDL_TEXFORMAT

#define SDL_TEXFORMAT SDL_TEXFORMAT_ARGB32
#include "texcopy.c"

#define SDL_TEXFORMAT SDL_TEXFORMAT_RGB32
#include "texcopy.c"

#define SDL_TEXFORMAT SDL_TEXFORMAT_RGB32_PALETTED
#include "texcopy.c"

#define SDL_TEXFORMAT SDL_TEXFORMAT_PALETTE16
#include "texcopy.c"

#define SDL_TEXFORMAT SDL_TEXFORMAT_PALETTE16A
#include "texcopy.c"

#define SDL_TEXFORMAT SDL_TEXFORMAT_RGB15
#include "texcopy.c"

#define SDL_TEXFORMAT SDL_TEXFORMAT_RGB15_PALETTED
#include "texcopy.c"

#define SDL_TEXFORMAT SDL_TEXFORMAT_PALETTE16_ARGB1555
#include "texcopy.c"

#define SDL_TEXFORMAT SDL_TEXFORMAT_RGB15_ARGB1555
#include "texcopy.c"

#define SDL_TEXFORMAT SDL_TEXFORMAT_RGB15_PALETTED_ARGB1555
#include "texcopy.c"

#if 0 //def SDLMAME_MACOSX /* native MacOS X composite texture format */

#define SDL_TEXFORMAT SDL_TEXFORMAT_YUY16
#include "texcopy.c"

#define SDL_TEXFORMAT SDL_TEXFORMAT_YUY16_PALETTED
#include "texcopy.c"

#endif

//============================================================
//  MANUAL TEXCOPY FUNCS 
//  (YUY format is weird and doesn't fit the assumptions of the
//   standard macros so we handle it here
//============================================================
#if 1 //ndef SDLMAME_MACOSX
static void texcopy_yuv16(texture_info *texture, const render_texinfo *texsource)
{
	int x, y;
	UINT32 *dst;
	UINT16 *src;

	// loop over Y
	for (y = 0; y < texsource->height; y++)
	{
		src = (UINT16 *)texsource->base + y * texsource->rowpixels;
		dst = (UINT32 *)texture->data + (y * texture->yprescale + texture->borderpix) * texture->rawwidth;

		// always fill non-wrapping textures with an extra pixel on the left
		if (texture->borderpix)
			*dst++ = 0;

		// we don't support prescale for YUV textures
		for (x = 0; x < texsource->width/2; x++)
		{
			UINT16 srcpix0 = *src++;
			UINT16 srcpix1 = *src++;
			UINT8 cb = srcpix0 & 0xff;
			UINT8 cr = srcpix1 & 0xff;
	
			*dst++ = ycc_to_rgb(srcpix0 >> 8, cb, cr);
			*dst++ = ycc_to_rgb(srcpix1 >> 8, cb, cr);
		}
		
		// always fill non-wrapping textures with an extra pixel on the right
		#if 0 
		if (texture->borderpix)
			*dst++ = 0;
		#endif
	}
}

#define CLSH(x)		(((int) x < 0) ? 0 : x >> 8)

static void texcopy_yuv16_paletted(texture_info *texture, const render_texinfo *texsource)
{
	int x, y;
	UINT32 *dst;
	UINT16 *src;

	// loop over Y
	for (y = 0; y < texsource->height; y++)
	{
		src = (UINT16 *)texsource->base + y * texsource->rowpixels;
		dst = (UINT32 *)texture->data + (y * texture->yprescale + texture->borderpix) * texture->rawwidth;

		// always fill non-wrapping textures with an extra pixel on the left
		if (texture->borderpix)
			*dst++ = 0;

		// we don't support prescale for YUV textures
		for (x = 0; x < texsource->width/2; x++)
		{
			UINT16 srcpix0 = *src++;
			UINT16 srcpix1 = *src++;
			UINT8 cb = srcpix0 & 0xff;
			UINT8 cr = srcpix1 & 0xff;
			
#if 0	
			*dst++ = ycc_to_rgb(texsource->palette[0x000 + (srcpix0 >> 8)], cb, cr);
			*dst++ = ycc_to_rgb(texsource->palette[0x000 + (srcpix1 >> 8)], cb, cr);
#else
			unsigned int r1, r2, g1, g2, b1, b2;
			unsigned int y1 = texsource->palette[(srcpix0 >> 8)] * 298;
			unsigned int y2 = texsource->palette[(srcpix1 >> 8)] * 298;

			r1 = r2 =               409 * cr - 56992;
			g1 = g2 =  - 100 * cb - 208 * cr + 34784;
			b1 = b2 =  + 516 * cb - 70688;
			r1 += y1; g1 += y1; b1 += y1;
			r2 += y2; g2 += y2; b2 += y2;
			*dst++ = MAKE_RGB(CLSH(r1), CLSH(g1), CLSH(b1));
			*dst++ = MAKE_RGB(CLSH(r2), CLSH(g2), CLSH(b2));
#endif
		}
		
		// always fill non-wrapping textures with an extra pixel on the right
		#if 0
		if (texture->borderpix)
			*dst++ = 0;
		#endif
	}
}
#undef CLSH

#endif

#else // recursive include

#include "texsrc.h"

static void FUNC_NAME(texcopy)(texture_info *texture, const render_texinfo *texsource)
{
	int x, y;
	DEST_TYPE *dst;
	TEXSRC_TYPE *src;

	// loop over Y
	for (y = 0; y < texsource->height; y++)
	{
		src = (TEXSRC_TYPE *)texsource->base + y * texsource->rowpixels;
		dst = (DEST_TYPE *)texture->data + (y * texture->yprescale + texture->borderpix) * texture->rawwidth;

		// always fill non-wrapping textures with an extra pixel on the left
		if (texture->borderpix)
			*dst++ = 0;

		switch(texture->xprescale)
		{
		case 1:
			for (x = 0; x < texsource->width; x++)
			{
				*dst++ = TEXSRC_TO_DEST(*src);
				src++;
			}
			break;
		case 2:
			for (x = 0; x < texsource->width; x++)
			{
				DEST_TYPE pixel = TEXSRC_TO_DEST(*src);
				*dst++ = pixel;
				*dst++ = pixel;
				src++;
			}
			break;
		case 3:
			for (x = 0; x < texsource->width; x++)
			{
				DEST_TYPE pixel = TEXSRC_TO_DEST(*src);
				*dst++ = pixel;
				*dst++ = pixel;
				*dst++ = pixel;
				src++;
			}
			break;
		}
		
		// always fill non-wrapping textures with an extra pixel on the right
		if (texture->borderpix)
			*dst++ = 0;
		
		/* abuse x var to act as line counter while copying */
		for (x = 1; x < texture->yprescale; x++)
		{
			DEST_TYPE *src1 = (DEST_TYPE *)texture->data + (y * texture->yprescale + texture->borderpix) * texture->rawwidth;
			dst = (DEST_TYPE *)texture->data + (y * texture->yprescale + texture->borderpix + x) * texture->rawwidth;
			memcpy(dst, src1, (texture->rawwidth + 2*texture->borderpix) * sizeof(DEST_TYPE));
		}
	}
}

#undef SDL_TEXFORMAT
#endif
