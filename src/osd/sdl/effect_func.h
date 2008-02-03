#include "texsrc.h"

// texcopy function prototype
void FUNC_NAME(texcopy)(texture_info *texture, const render_texinfo *texsource);

void EFFECT_FUNC_NAME(texture_info *texture, const render_texinfo *texsource)
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
