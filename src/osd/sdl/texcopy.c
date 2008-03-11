
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

#ifdef SDLMAME_MACOSX /* native MacOS X composite texture format */

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
		break;
		
		// always fill non-wrapping textures with an extra pixel on the right
		#if 0 
		if (texture->borderpix)
			*dst++ = 0;
		#endif
	}
}

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
	
			*dst++ = ycc_to_rgb(texsource->palette[0x000 + (srcpix0 >> 8)], cb, cr);
			*dst++ = ycc_to_rgb(texsource->palette[0x000 + (srcpix1 >> 8)], cb, cr);
		}
		break;
		
		// always fill non-wrapping textures with an extra pixel on the right
		#if 0
		if (texture->borderpix)
			*dst++ = 0;
		#endif
	}
}

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
