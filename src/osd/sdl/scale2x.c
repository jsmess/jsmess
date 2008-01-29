#include "effect.h"

// 16 and 32 bpp versions of the core scale2x functions
// NOTE, advancemame has mmx asm versions of these, these could be easily
// integrated by someone interested, except for borderpix support.
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
#include "effect_func.h"
#undef SDL_TEXFORMAT

#define SDL_TEXFORMAT SDL_TEXFORMAT_RGB32
#include "effect_func.h"
#undef SDL_TEXFORMAT

#define SDL_TEXFORMAT SDL_TEXFORMAT_RGB32_PALETTED
#include "effect_func.h"
#undef SDL_TEXFORMAT

#define SDL_TEXFORMAT SDL_TEXFORMAT_PALETTE16
#include "effect_func.h"
#undef SDL_TEXFORMAT

#define SDL_TEXFORMAT SDL_TEXFORMAT_PALETTE16A
#include "effect_func.h"
#undef SDL_TEXFORMAT

#define SDL_TEXFORMAT SDL_TEXFORMAT_RGB15
#include "effect_func.h"
#undef SDL_TEXFORMAT

#define SDL_TEXFORMAT SDL_TEXFORMAT_RGB15_PALETTED
#include "effect_func.h"
#undef SDL_TEXFORMAT

#define SDL_TEXFORMAT SDL_TEXFORMAT_PALETTE16_ARGB1555
#include "effect_func.h"
#undef SDL_TEXFORMAT

#define SDL_TEXFORMAT SDL_TEXFORMAT_RGB15_ARGB1555
#include "effect_func.h"
#undef SDL_TEXFORMAT

#define SDL_TEXFORMAT SDL_TEXFORMAT_RGB15_PALETTED_ARGB1555
#include "effect_func.h"
#undef SDL_TEXFORMAT

#undef EFFECT_FUNC_NAME
#undef EFFECT_DEST_NAME

//============================================================
//  MANUAL TEXCOPY FUNCS 
//  (YUY format is weird and doesn't fit the assumptions of the
//   standard macros so we handle it here
//     
//============================================================

void scale2x_yuv16(texture_info *texture, const render_texinfo *texsource)
{
	fatalerror("ERROR: Scale2X not supported for YUV textures\n");
}

void scale2x_yuv16_paletted(texture_info *texture, const render_texinfo *texsource)
{
	fatalerror("ERROR: Scale2X not supported for YUV textures\n");
}
