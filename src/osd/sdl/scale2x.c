
#ifndef SDL_TEXFORMAT

// 16 and 32 bpp versions of the core scale2x functions
// NOTE, advancemame has mmx asm versions of these, these could be easily
// integrated by someone interested, except for borderpix support.

#ifdef FUNC_NAME
#undef FUNC_NAME
#endif

#define DEST_TYPE UINT16
#define FUNC_NAME(name) name ## _16bpp
#include "scale2x_core.c"

#define DEST_TYPE UINT32
#define FUNC_NAME(name) name ## _32bpp
#include "scale2x_core.c"


// and versions for all texformats of the wrapper function called by drawsdl
#define EFFECT_FUNC_NAME FUNC_NAME(scale2x)
#define EFFECT_DEST_NAME DEST_NAME(scaleXxX)

#define SDL_TEXFORMAT SDL_TEXFORMAT_ARGB32
#include "scale2x.c"
#undef SDL_TEXFORMAT

#define SDL_TEXFORMAT SDL_TEXFORMAT_RGB32
#include "scale2x.c"
#undef SDL_TEXFORMAT

#define SDL_TEXFORMAT SDL_TEXFORMAT_RGB32_PALETTED
#include "scale2x.c"
#undef SDL_TEXFORMAT

#define SDL_TEXFORMAT SDL_TEXFORMAT_PALETTE16
#include "scale2x.c"
#undef SDL_TEXFORMAT

#define SDL_TEXFORMAT SDL_TEXFORMAT_PALETTE16A
#include "scale2x.c"
#undef SDL_TEXFORMAT

#define SDL_TEXFORMAT SDL_TEXFORMAT_RGB15
#include "scale2x.c"
#undef SDL_TEXFORMAT

#define SDL_TEXFORMAT SDL_TEXFORMAT_RGB15_PALETTED
#include "scale2x.c"
#undef SDL_TEXFORMAT

#define SDL_TEXFORMAT SDL_TEXFORMAT_PALETTE16_ARGB1555
#include "scale2x.c"
#undef SDL_TEXFORMAT

#define SDL_TEXFORMAT SDL_TEXFORMAT_RGB15_ARGB1555
#include "scale2x.c"
#undef SDL_TEXFORMAT

#define SDL_TEXFORMAT SDL_TEXFORMAT_RGB15_PALETTED_ARGB1555
#include "scale2x.c"
#undef SDL_TEXFORMAT

#undef EFFECT_FUNC_NAME
#undef EFFECT_DEST_NAME

//============================================================
//  MANUAL TEXCOPY FUNCS 
//  (YUY format is weird and doesn't fit the assumptions of the
//   standard macros so we handle it here
//     
//============================================================

static void scale2x_yuv16(texture_info *texture, const render_texinfo *texsource)
{
	fatalerror("ERROR: Scale2X not supported for YUV textures\n");
}

static void scale2x_yuv16_paletted(texture_info *texture, const render_texinfo *texsource)
{
	fatalerror("ERROR: Scale2X not supported for YUV textures\n");
}

#else // recursive include

#include "texsrc.h"

// texcopy function prototype
//void FUNC_NAME(texcopy)(texture_info *texture, const render_texinfo *texsource);

static void EFFECT_FUNC_NAME(texture_info *texture, const render_texinfo *texsource)
{
	int y;
	DEST_TYPE *dst = (DEST_TYPE *)texture->data + texture->borderpix * texture->rawwidth;
	TEXSRC_TYPE *src = (TEXSRC_TYPE *)texsource->base;
#ifndef SRC_EQUALS_DEST
	int x;
	DEST_TYPE *tmp, *effectbuf0, *effectbuf1, *effectbuf2;
#endif

	switch ((texture->xprescale << 8) | texture->yprescale)
	{
		case 0x0101:
		case 0x0102:
		case 0x0103:
		case 0x0201:
		case 0x0301:
			FUNC_NAME(texcopy)(texture, texsource);
			return;
	}

#ifndef SRC_EQUALS_DEST
	// First convert to a temporary buffer so that we do not convert each
	// pixel 4 times
	effectbuf1 = (DEST_TYPE *)texture->effectbuf;
	effectbuf2 = (DEST_TYPE *)texture->effectbuf + texsource->width;
	effectbuf0 = (DEST_TYPE *)texture->effectbuf + 2 * texsource->width;
	
	tmp = effectbuf1;
	for (x = 0; x < texsource->width; x++)
	{
		*tmp++ = TEXSRC_TO_DEST(*src);
		src++;
	}

	src = (TEXSRC_TYPE *)texsource->base + texsource->rowpixels;
	tmp = effectbuf2;
	for (x = 0; x < texsource->width; x++)
	{
		*tmp++ = TEXSRC_TO_DEST(*src);
		src++;
	}
	EFFECT_DEST_NAME(&dst, effectbuf1, effectbuf1, effectbuf2, texture,
		texsource);
#else
	EFFECT_DEST_NAME(&dst, src, src, src + texsource->rowpixels,
		texture, texsource);
#endif	

	// loop over Y
	for (y = 1; y < (texsource->height-1); y++)
	{
		dst += texture->rawwidth;
#ifndef SRC_EQUALS_DEST
		// First convert to a temporary buffer so that we do not
		// convert each pixel 4 times
		src = (TEXSRC_TYPE *)texsource->base +
			(y + 1) * texsource->rowpixels;
		tmp = effectbuf0;
		effectbuf0 = effectbuf1;
		effectbuf1 = effectbuf2;
		effectbuf2 = tmp;
		for (x = 0; x < texsource->width; x++)
		{
			*tmp++ = TEXSRC_TO_DEST(*src);
			src++;
		}
		EFFECT_DEST_NAME(&dst, effectbuf0, effectbuf1, effectbuf2, texture, texsource);
#else
		src += texsource->rowpixels;
		EFFECT_DEST_NAME(&dst, src - texsource->rowpixels, src,
			src + texsource->rowpixels, texture, texsource);
#endif
	}

	// last line
	dst += texture->rawwidth;
#ifndef SRC_EQUALS_DEST
	EFFECT_DEST_NAME(&dst, effectbuf1, effectbuf2, effectbuf2, texture, texsource);
#else
	EFFECT_DEST_NAME(&dst, src, src + texsource->rowpixels, src + texsource->rowpixels, texture, texsource);
#endif
}

#undef SDL_TEXFORMAT


#endif
