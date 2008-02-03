#ifndef __SDL_EFFECT__
#define __SDL_EFFECT__

void scale2x_argb32(texture_info *texture, const render_texinfo *texsource);
void scale2x_rgb32(texture_info *texture, const render_texinfo *texsource);
void scale2x_rgb32_paletted(texture_info *texture, const render_texinfo *texsource);
void scale2x_palette16(texture_info *texture, const render_texinfo *texsource);
void scale2x_palette16a(texture_info *texture, const render_texinfo *texsource);
void scale2x_rgb15(texture_info *texture, const render_texinfo *texsource);
void scale2x_rgb15_paletted(texture_info *texture, const render_texinfo *texsource);
void scale2x_yuv16(texture_info *texture, const render_texinfo *texsource);
void scale2x_yuv16_paletted(texture_info *texture, const render_texinfo *texsource);
void scale2x_palette16_argb1555(texture_info *texture, const render_texinfo *texsource);
void scale2x_rgb15_argb1555(texture_info *texture, const render_texinfo *texsource);
void scale2x_rgb15_paletted_argb1555(texture_info *texture, const render_texinfo *texsource);

#endif
