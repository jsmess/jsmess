#include "window.h"
#include "texsrc.h"

void FUNC_NAME(texcopy)(texture_info *texture, const render_texinfo *texsource)
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
