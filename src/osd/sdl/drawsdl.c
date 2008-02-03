//============================================================
//  
//  drawsdl.c - SDL software and OpenGL implementation
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//  SDLMAME by Olivier Galibert and R. Belmont
//
//  Note: D3D9 goes to a lot of trouble to fiddle with MODULATE
//        mode on textures.  That is the default in OpenGL so we
//        don't have to touch it.
//
//============================================================

// standard C headers
#include <math.h>
#include <stdio.h>

// MAME headers
#include "osdcomm.h"
#include "render.h"
#include "options.h"

// standard SDL headers
#include <SDL/SDL.h>

#if USE_OPENGL

// OpenGL headers
#include "osd_opengl.h"

#include "gl_shader_mgr.h"

#ifdef SDLMAME_WIN32
typedef ptrdiff_t GLsizeiptr;
#endif

#if defined(SDLMAME_MACOSX)
#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif

typedef void (APIENTRYP PFNGLGENBUFFERSPROC) (GLsizei n, GLuint *buffers);
typedef void (APIENTRYP PFNGLBINDBUFFERPROC) (GLenum, GLuint);
typedef void (APIENTRYP PFNGLBUFFERDATAPROC) (GLenum, GLsizeiptr, const GLvoid *, GLenum);
typedef void (APIENTRYP PFNGLBUFFERSUBDATAPROC) (GLenum, GLintptr, GLsizeiptr, const GLvoid *);
typedef GLvoid* (APIENTRYP PFNGLMAPBUFFERPROC) (GLenum, GLenum);
typedef GLboolean (APIENTRYP PFNGLUNMAPBUFFERPROC) (GLenum);
typedef void (APIENTRYP PFNGLDELETEBUFFERSPROC) (GLsizei, const GLuint *);
typedef void (APIENTRYP PFNGLACTIVETEXTUREPROC) (GLenum texture);
typedef GLboolean (APIENTRYP PFNGLISFRAMEBUFFEREXTPROC) (GLuint framebuffer);
typedef void (APIENTRYP PFNGLBINDFRAMEBUFFEREXTPROC) (GLenum target, GLuint framebuffer);
typedef void (APIENTRYP PFNGLDELETEFRAMEBUFFERSEXTPROC) (GLsizei n, const GLuint *framebuffers);
typedef void (APIENTRYP PFNGLGENFRAMEBUFFERSEXTPROC) (GLsizei n, GLuint *framebuffers);
typedef GLenum (APIENTRYP PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC) (GLenum target);
typedef void (APIENTRYP PFNGLFRAMEBUFFERTEXTURE2DEXTPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void (APIENTRYP PFNGLGENRENDERBUFFERSEXTPROC) (GLsizei n, GLuint *renderbuffers);
typedef void (APIENTRYP PFNGLBINDRENDERBUFFEREXTPROC) (GLenum target, GLuint renderbuffer);
typedef void (APIENTRYP PFNGLRENDERBUFFERSTORAGEEXTPROC) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
typedef void (APIENTRYP PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC) (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
typedef void (APIENTRYP PFNGLDELETERENDERBUFFERSEXTPROC) (GLsizei n, const GLuint *renderbuffers);
#endif

// make sure the extensions compile OK everywhere
#ifndef GL_TEXTURE_STORAGE_HINT_APPLE
#define GL_TEXTURE_STORAGE_HINT_APPLE     0x85bc
#endif

#ifndef GL_STORAGE_CACHED_APPLE
#define GL_STORAGE_CACHED_APPLE           0x85be
#endif

#ifndef GL_UNPACK_CLIENT_STORAGE_APPLE
#define GL_UNPACK_CLIENT_STORAGE_APPLE    0x85b2
#endif

#ifndef GL_TEXTURE_RECTANGLE_ARB
#define GL_TEXTURE_RECTANGLE_ARB          0x84F5
#endif

#ifndef GL_PIXEL_UNPACK_BUFFER_ARB
#define GL_PIXEL_UNPACK_BUFFER_ARB        0x88EC
#endif

#ifndef GL_STREAM_DRAW
#define GL_STREAM_DRAW                    0x88E0
#endif

#ifndef GL_WRITE_ONLY
#define GL_WRITE_ONLY                     0x88B9
#endif

#ifndef GL_ARRAY_BUFFER_ARB
#define GL_ARRAY_BUFFER_ARB               0x8892
#endif

#ifndef GL_PIXEL_UNPACK_BUFFER_ARB
#define GL_PIXEL_UNPACK_BUFFER_ARB        0x88EC
#endif

#ifndef GL_FRAMEBUFFER_EXT
#define GL_FRAMEBUFFER_EXT				0x8D40
#define GL_FRAMEBUFFER_COMPLETE_EXT			0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT	0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT	0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT	0x8CD8
#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT		0x8CD9
#define GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT		0x8CDA
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT	0x8CDB
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT	0x8CDC
#define GL_FRAMEBUFFER_UNSUPPORTED_EXT			0x8CDD
#define GL_RENDERBUFFER_EXT				0x8D41
#define GL_DEPTH_COMPONENT16				0x81A5
#define GL_DEPTH_COMPONENT24				0x81A6
#define GL_DEPTH_COMPONENT32				0x81A7
#endif


#endif // OPENGL

// OSD headers
#include "osdsdl.h"
#include "window.h"
#include "effect.h"

//============================================================
//  DEBUGGING
//============================================================

#define DEBUG_MODE_SCORES	0

//============================================================
//  CONSTANTS
//============================================================

enum
{
	TEXTURE_TYPE_NONE,
	TEXTURE_TYPE_PLAIN,
	TEXTURE_TYPE_DYNAMIC,
	TEXTURE_TYPE_SHADER,
	TEXTURE_TYPE_SURFACE
};


//============================================================
//  MACROS
//============================================================

#define FSWAP(var1, var2) do { float temp = var1; var1 = var2; var2 = temp; } while (0)


//============================================================
//  INLINES
//============================================================

#if USE_OPENGL
INLINE HashT texture_compute_hash(const render_texinfo *texture, UINT32 flags)
{
	return (HashT)texture->base ^ (flags & (PRIMFLAG_BLENDMODE_MASK | PRIMFLAG_TEXFORMAT_MASK));
}

INLINE void set_blendmode(sdl_info *sdl, int blendmode, texture_info *texture)
{
	// try to minimize texture state changes
	if (blendmode != sdl->last_blendmode)
	{
		switch (blendmode)
		{
			case BLENDMODE_NONE:
				glDisable(GL_BLEND);
				break;
			case BLENDMODE_ALPHA:		
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				break;
			case BLENDMODE_RGB_MULTIPLY:	
				glEnable(GL_BLEND);
				glBlendFunc(GL_DST_COLOR, GL_ZERO);
				break;
			case BLENDMODE_ADD:		
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				break;
	 	}

		sdl->last_blendmode = blendmode;
	}
}
#endif

INLINE int better_mode(int width0, int height0, int width1, int height1, float desired_aspect)
{
	float aspect0 = (float)width0 / (float)height0;
	float aspect1 = (float)width1 / (float)height1;
	return (fabs(desired_aspect - aspect0) < fabs(desired_aspect - aspect1)) ? 0 : 1;
}



//============================================================
//  PROTOTYPES
//============================================================

// core functions
static void drawsdl_exit(void);
static int drawsdl_window_init(sdl_window_info *window);
static void drawsdl_window_destroy(sdl_window_info *window);
static const render_primitive_list *drawsdl_window_get_primitives(sdl_window_info *window);
static int drawsdl_window_draw(sdl_window_info *window, UINT32 dc, int update);
static void drawsdl_destroy_all_textures(sdl_window_info *window);

#if USE_OPENGL

static void drawogl_exit(void);
static int drawogl_window_init(sdl_window_info *window);
static int drawogl_window_draw(sdl_window_info *window, UINT32 dc, int update);
static void drawogl_window_destroy(sdl_window_info *window);
static void drawogl_destroy_all_textures(sdl_window_info *window);

// OGL 1.3
static PFNGLACTIVETEXTUREPROC pfn_glActiveTexture	= NULL;

// VBO
static PFNGLGENBUFFERSPROC pfn_glGenBuffers		= NULL;
static PFNGLDELETEBUFFERSPROC pfn_glDeleteBuffers	= NULL;
static PFNGLBINDBUFFERPROC pfn_glBindBuffer		= NULL;
static PFNGLBUFFERDATAPROC pfn_glBufferData		= NULL;
static PFNGLBUFFERSUBDATAPROC pfn_glBufferSubData	= NULL;

// PBO
static PFNGLMAPBUFFERPROC     pfn_glMapBuffer		= NULL;
static PFNGLUNMAPBUFFERPROC   pfn_glUnmapBuffer		= NULL;

// FBO
static PFNGLISFRAMEBUFFEREXTPROC   pfn_glIsFramebuffer			= NULL;
static PFNGLBINDFRAMEBUFFEREXTPROC pfn_glBindFramebuffer		= NULL;
static PFNGLDELETEFRAMEBUFFERSEXTPROC pfn_glDeleteFramebuffers		= NULL;
static PFNGLGENFRAMEBUFFERSEXTPROC pfn_glGenFramebuffers		= NULL;
static PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC pfn_glCheckFramebufferStatus	= NULL;
static PFNGLFRAMEBUFFERTEXTURE2DEXTPROC pfn_glFramebufferTexture2D	= NULL;

static int glsl_shader_feature = GLSL_SHADER_FEAT_PLAIN;

#endif // USE_OPENGL

// texcopy functions
void texcopy_argb32(texture_info *texture, const render_texinfo *texsource);
void texcopy_rgb32(texture_info *texture, const render_texinfo *texsource);
void texcopy_rgb32_paletted(texture_info *texture, const render_texinfo *texsource);
void texcopy_palette16(texture_info *texture, const render_texinfo *texsource);
void texcopy_palette16a(texture_info *texture, const render_texinfo *texsource);
void texcopy_rgb15(texture_info *texture, const render_texinfo *texsource);
void texcopy_rgb15_paletted(texture_info *texture, const render_texinfo *texsource);
void texcopy_yuv16(texture_info *texture, const render_texinfo *texsource);
void texcopy_yuv16_paletted(texture_info *texture, const render_texinfo *texsource);
#ifdef SDLMAME_MACOSX
void texcopy_yuv16_apple(texture_info *texture, const render_texinfo *texsource);
void texcopy_yuv16_paletted_apple(texture_info *texture, const render_texinfo *texsource);
#endif
// 16 bpp destination texture texcopy functions
void texcopy_palette16_argb1555(texture_info *texture, const render_texinfo *texsource);
void texcopy_rgb15_argb1555(texture_info *texture, const render_texinfo *texsource);
void texcopy_rgb15_paletted_argb1555(texture_info *texture, const render_texinfo *texsource);

// soft rendering
void drawsdl_rgb888_draw_primitives(const render_primitive *primlist, void *dstdata, UINT32 width, UINT32 height, UINT32 pitch);
void drawsdl_bgr888_draw_primitives(const render_primitive *primlist, void *dstdata, UINT32 width, UINT32 height, UINT32 pitch);
void drawsdl_rgb565_draw_primitives(const render_primitive *primlist, void *dstdata, UINT32 width, UINT32 height, UINT32 pitch);
void drawsdl_rgb555_draw_primitives(const render_primitive *primlist, void *dstdata, UINT32 width, UINT32 height, UINT32 pitch);

// textures
#if USE_OPENGL
static texture_info *texture_create(sdl_info *sdl, sdl_window_info *window, const render_texinfo *texsource, UINT32 flags);
static void texture_set_data(sdl_info *sdl, texture_info *texture, const render_texinfo *texsource, UINT32 flags);
static texture_info *texture_find(sdl_info *sdl, const render_primitive *prim);
static texture_info * texture_update(sdl_info *sdl, sdl_window_info *window, const render_primitive *prim, int shaderIdx);
static void texture_all_disable(sdl_info *sdl);
static void texture_disable(sdl_info *sdl, texture_info * texture);
#endif

// YUV overlays

static void yuv_RGB_to_YV12(UINT16 *bitmap, int bw, sdl_window_info *window);
static void yuv_RGB_to_YV12X2(UINT16 *bitmap, int bw, sdl_window_info *window);
static void yuv_RGB_to_YUY2(UINT16 *bitmap, int bw, sdl_window_info *window);
static void yuv_RGB_to_YUY2X2(UINT16 *bitmap, int bw, sdl_window_info *window);


//============================================================
//  drawsdl_init
//============================================================

int drawsdl_init(sdl_draw_callbacks *callbacks)
{
	// fill in the callbacks
	callbacks->exit = drawsdl_exit;
	callbacks->window_init = drawsdl_window_init;
	callbacks->window_get_primitives = drawsdl_window_get_primitives;
	callbacks->window_draw = drawsdl_window_draw;
	callbacks->window_destroy = drawsdl_window_destroy;
	callbacks->window_destroy_all_textures = drawsdl_destroy_all_textures;

	#if SDL_MULTIMON
	mame_printf_verbose("Using SDL multi-window soft driver (SDL 1.3+)\n");
	#else
	mame_printf_verbose("Using SDL single-window soft driver (SDL 1.2)\n");
	#endif
	return 0;
}

//============================================================
//  drawogl_init
//============================================================

int drawogl_init(sdl_draw_callbacks *callbacks)
{
#if USE_OPENGL
	// fill in the callbacks
	callbacks->exit = drawogl_exit;
	callbacks->window_init = drawogl_window_init;
	callbacks->window_get_primitives = drawsdl_window_get_primitives;
	callbacks->window_draw = drawogl_window_draw;
	callbacks->window_destroy = drawogl_window_destroy;
	callbacks->window_destroy_all_textures = drawogl_destroy_all_textures;

	#if SDL_MULTIMON
	mame_printf_verbose("Using SDL multi-window OpenGL driver (SDL 1.3+)\n");
	#else
	mame_printf_verbose("Using SDL single-window OpenGL driver (SDL 1.2)\n");
	#endif
	return 0;
#else
	return drawsdl_init(callbacks);
#endif
}

//============================================================
//  drawsdl_exit
//============================================================

static void drawsdl_exit(void)
{
}

//============================================================
//  drawsdl_destroy_all_textures
//============================================================

static void drawsdl_destroy_all_textures(sdl_window_info *window)
{
	/* nothing to be done in soft mode */
}

//============================================================
//  drawsdl_window_init
//============================================================

static int drawsdl_window_init(sdl_window_info *window)
{
	sdl_info *sdl;

	// allocate memory for our structures
	sdl = malloc(sizeof(*sdl));
	memset(sdl, 0, sizeof(*sdl));
	window->dxdata = sdl;
	window->opengl = 0;
	window->yuv_lookup = NULL;
	window->yuv_bitmap = NULL;
	window->initialized = 0;
	sdl->totalColors = window->totalColors;

	return 0;
}

#if USE_OPENGL

static int drawogl_window_init(sdl_window_info *window)
{
	sdl_info *sdl;

	drawsdl_window_init(window);
	window->opengl = 1;

	sdl = window->dxdata; 

	// in case any textures try to come up before these are validated,
	// OpenGL guarantees all implementations can handle something this size.
	sdl->texture_max_width = 64;
	sdl->texture_max_height = 64;

	return 0;
}
#endif

//============================================================
//  drawsdl_window_destroy
//============================================================

static void drawsdl_window_destroy(sdl_window_info *window)
{
	sdl_info *sdl = window->dxdata;

	// skip if nothing
	if (sdl == NULL)
		return;

	// free the memory in the window

	free(sdl);
	window->dxdata = NULL;
	if (window->yuv_lookup != NULL)
	{
		free(window->yuv_lookup); 
		window->yuv_lookup = NULL;
	}
	if (window->yuv_bitmap != NULL)
	{
		free(window->yuv_bitmap);	
		window->yuv_bitmap = NULL;
	}
}

//============================================================
//  drawsdl_window_get_primitives
//============================================================

static const render_primitive_list *drawsdl_window_get_primitives(sdl_window_info *window)
{
	sdl_info *sdl = window->dxdata;
	if ((video_config.windowed) || (video_config.switchres))
	{
		drawsdl_blit_surface_size(window, window->sdlsurf->w, window->sdlsurf->h);
	}
	else
	{
		drawsdl_blit_surface_size(window, window->monitor->center_width, window->monitor->center_height);
	}
	if (video_config.yuv_mode == VIDEO_YUV_MODE_NONE)
		render_target_set_bounds(window->target, sdl->blitwidth, sdl->blitheight, sdlvideo_monitor_get_aspect(window->monitor));
	else
		render_target_set_bounds(window->target, window->yuv_ovl_width, window->yuv_ovl_height, 0);
	return render_target_get_primitives(window->target);
}

//============================================================
//  drawsdl_window_draw
//============================================================

static int drawsdl_window_draw(sdl_window_info *window, UINT32 dc, int update)
{
	sdl_info *sdl = window->dxdata;
	UINT8 *surfptr;
	INT32 vofs, hofs, blitwidth, blitheight, ch, cw;
	SDL_Rect r;

	if (video_config.novideo)
	{
		return 0;
	}

	// if we haven't been created, just punt
	if (sdl == NULL)
		return 1;

	// lock it if we need it
	if (SDL_MUSTLOCK(window->sdlsurf)) SDL_LockSurface(window->sdlsurf);
	if (video_config.yuv_mode != VIDEO_YUV_MODE_NONE)
	{
		SDL_LockYUVOverlay(window->yuvsurf);
		surfptr = (UINT8 *) window->yuv_bitmap;
	}
	else
		surfptr = (UINT8 *)window->sdlsurf->pixels;
	// get ready to center the image
	vofs = hofs = 0;
	blitwidth = sdl->blitwidth;
	blitheight = sdl->blitheight;

	// figure out what coordinate system to use for centering - in window mode it's always the
	// SDL surface size.  in fullscreen the surface covers all monitors, so center according to
	// the first one only
	if ((!video_config.windowed) && (!video_config.switchres))
	{
		ch = window->monitor->center_height;
		cw = window->monitor->center_width;
	}
	else
	{
		ch = window->sdlsurf->h;
		cw = window->sdlsurf->w;
	}

	// do not crash if the window's smaller than the blit area
	if (blitheight > ch)
	{
		  	blitheight = ch;
	}
	else if (video_config.centerv)
	{
		vofs = (ch - sdl->blitheight) / 2;
	}

	if (blitwidth > cw)
	{
		  	blitwidth = cw;
	}
	else if (video_config.centerh)
	{
		hofs = (cw - sdl->blitwidth) / 2;
	}

	osd_lock_acquire(window->primlist->lock);

	// render to it
	if (video_config.yuv_mode == VIDEO_YUV_MODE_NONE)
	{
		surfptr += ((vofs * window->sdlsurf->pitch) + (hofs * window->sdlsurf->format->BytesPerPixel));
		switch (window->sdlsurf->format->Rmask)
		{
			case 0x00ff0000:
				drawsdl_rgb888_draw_primitives(window->primlist->head, surfptr, blitwidth, blitheight, window->sdlsurf->pitch / 4);
				break;

			case 0x000000ff:
				drawsdl_bgr888_draw_primitives(window->primlist->head, surfptr, blitwidth, blitheight, window->sdlsurf->pitch / 4);
				break;

			case 0xf800:
				drawsdl_rgb565_draw_primitives(window->primlist->head, surfptr, blitwidth, blitheight, window->sdlsurf->pitch / 2);
				break;

			case 0x7c00:
				drawsdl_rgb555_draw_primitives(window->primlist->head, surfptr, blitwidth, blitheight, window->sdlsurf->pitch / 2);
				break;

			default:
				mame_printf_error("SDL: ERROR! Unknown video mode: R=%08X G=%08X B=%08X\n", window->sdlsurf->format->Rmask, window->sdlsurf->format->Gmask, window->sdlsurf->format->Bmask);
				break;
		}
	} 
	else
	{
		drawsdl_rgb555_draw_primitives(window->primlist->head, surfptr, window->yuv_ovl_width, window->yuv_ovl_height, window->yuv_ovl_width);
		switch (video_config.yuv_mode)
		{
			case VIDEO_YUV_MODE_YV12: 
				yuv_RGB_to_YV12((UINT16 *)surfptr, window->yuv_ovl_width, window);
				break;
			case VIDEO_YUV_MODE_YV12X2: 
				yuv_RGB_to_YV12X2((UINT16 *)surfptr, window->yuv_ovl_width, window);
				break;
			case VIDEO_YUV_MODE_YUY2: 
				yuv_RGB_to_YUY2((UINT16 *)surfptr, window->yuv_ovl_width, window);
				break;
			case VIDEO_YUV_MODE_YUY2X2: 
				yuv_RGB_to_YUY2X2((UINT16 *)surfptr, window->yuv_ovl_width, window);
				break;
		}
	}
	osd_lock_release(window->primlist->lock);

	// unlock and flip
	if (SDL_MUSTLOCK(window->sdlsurf)) SDL_UnlockSurface(window->sdlsurf);

	if (video_config.yuv_mode == VIDEO_YUV_MODE_NONE)
		SDL_Flip(window->sdlsurf);
	else
	{
		SDL_UnlockYUVOverlay(window->yuvsurf);
		r.x=hofs;
		r.y=vofs;
		r.w=blitwidth;
		r.h=blitheight;
		SDL_DisplayYUVOverlay(window->yuvsurf, &r);
	}
	return 0;
}

#if USE_OPENGL

static void loadGLExtensions(sdl_info *sdl)
{
	static int _once = 1;

	// sdl->usevbo=FALSE; // You may want to switch VBO and PBO off, by uncommenting this statement
	// sdl->usepbo=FALSE; // You may want to switch PBO off, by uncommenting this statement
	// sdl->useglsl=FALSE; // You may want to switch GLSL off, by uncommenting this statement

	if (! sdl->usevbo)
	{
		if(sdl->usepbo) // should never ever happen ;-)
		{
			if (_once)
			{
				mame_printf_warning("OpenGL: PBO not supported, no VBO support. (sdlmame error)\n");
			}
			sdl->usepbo=FALSE;
		}
		if(sdl->useglsl) // should never ever happen ;-)
		{
			if (_once)
			{
				mame_printf_warning("OpenGL: GLSL not supported, no VBO support. (sdlmame error)\n");
			}
			sdl->useglsl=FALSE;
		}
	}

	// Get Pointers To The GL Functions
	// VBO:
	if( sdl->usevbo )
	{
		pfn_glGenBuffers = (PFNGLGENBUFFERSPROC) SDL_GL_GetProcAddress("glGenBuffers");
		pfn_glDeleteBuffers = (PFNGLDELETEBUFFERSPROC) SDL_GL_GetProcAddress("glDeleteBuffers");
		pfn_glBindBuffer = (PFNGLBINDBUFFERPROC) SDL_GL_GetProcAddress("glBindBuffer");
		pfn_glBufferData = (PFNGLBUFFERDATAPROC) SDL_GL_GetProcAddress("glBufferData");
		pfn_glBufferSubData = (PFNGLBUFFERSUBDATAPROC) SDL_GL_GetProcAddress("glBufferSubData");
	}
	// PBO:
	if ( sdl->usepbo )
	{
		pfn_glMapBuffer  = (PFNGLMAPBUFFERPROC) SDL_GL_GetProcAddress("glMapBuffer");
		pfn_glUnmapBuffer= (PFNGLUNMAPBUFFERPROC) SDL_GL_GetProcAddress("glUnmapBuffer");
	}
	// FBO:
	if ( sdl->usefbo )
	{
		pfn_glIsFramebuffer = (PFNGLISFRAMEBUFFEREXTPROC) SDL_GL_GetProcAddress("glIsFramebufferEXT");
		pfn_glBindFramebuffer = (PFNGLBINDFRAMEBUFFEREXTPROC) SDL_GL_GetProcAddress("glBindFramebufferEXT");
		pfn_glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSEXTPROC) SDL_GL_GetProcAddress("glDeleteFramebuffersEXT");
		pfn_glGenFramebuffers = (PFNGLGENFRAMEBUFFERSEXTPROC) SDL_GL_GetProcAddress("glGenFramebuffersEXT");
		pfn_glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC) SDL_GL_GetProcAddress("glCheckFramebufferStatusEXT");
		pfn_glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC) SDL_GL_GetProcAddress("glFramebufferTexture2DEXT");
	}

	if ( sdl->usevbo &&
	        ( !pfn_glGenBuffers || !pfn_glDeleteBuffers ||
	        !pfn_glBindBuffer || !pfn_glBufferData || !pfn_glBufferSubData
	 ) )
	{
		sdl->usepbo=FALSE;
		if (_once)
		{
			mame_printf_warning("OpenGL: VBO not supported, missing: ");
			if (!pfn_glGenBuffers)
			{
				mame_printf_warning("glGenBuffers, ");
			}
			if (!pfn_glDeleteBuffers)
			{
				mame_printf_warning("glDeleteBuffers");
			}
			if (!pfn_glBindBuffer)
			{
				mame_printf_warning("glBindBuffer, ");
			}
			if (!pfn_glBufferData)
			{
				mame_printf_warning("glBufferData, ");
			}
			if (!pfn_glBufferSubData)
			{
				mame_printf_warning("glBufferSubData, ");
			}
			mame_printf_warning("\n");
		}
		if ( sdl->usevbo )
		{
			if (_once)
			{
				mame_printf_warning("OpenGL: PBO not supported, no VBO support.\n");
			}
			sdl->usepbo=FALSE;
		}
	}

	if ( sdl->usepbo && ( !pfn_glMapBuffer || !pfn_glUnmapBuffer ) )
	{
		sdl->usepbo=FALSE;
		if (_once)
		{
			mame_printf_warning("OpenGL: PBO not supported, missing: ");
			if (!pfn_glMapBuffer)
			{
				mame_printf_warning("glMapBuffer, ");
			}
			if (!pfn_glUnmapBuffer)
			{
				mame_printf_warning("glUnmapBuffer, ");
			}
			mame_printf_warning("\n");
		}
	}

	if ( sdl->usefbo && 
		( !pfn_glIsFramebuffer || !pfn_glBindFramebuffer || !pfn_glDeleteFramebuffers ||
		  !pfn_glGenFramebuffers || !pfn_glCheckFramebufferStatus || !pfn_glFramebufferTexture2D
	  ))
	{
		sdl->usefbo=FALSE;
		if (_once)
		{
			mame_printf_warning("OpenGL: FBO not supported, missing: ");
			if (!pfn_glIsFramebuffer)
			{
				mame_printf_warning("pfn_glIsFramebuffer, ");
			}
			if (!pfn_glBindFramebuffer)
			{
				mame_printf_warning("pfn_glBindFramebuffer, ");
			}
			if (!pfn_glDeleteFramebuffers)
			{
				mame_printf_warning("pfn_glDeleteFramebuffers, ");
			}
			if (!pfn_glGenFramebuffers)
			{
				mame_printf_warning("pfn_glGenFramebuffers, ");
			}
			if (!pfn_glCheckFramebufferStatus)
			{
				mame_printf_warning("pfn_glCheckFramebufferStatus, ");
			}
			if (!pfn_glFramebufferTexture2D)
			{
				mame_printf_warning("pfn_glFramebufferTexture2D, ");
			}
			mame_printf_warning("\n");
		}
	}

	if (_once)
	{
		if ( sdl->usevbo )
		{
			mame_printf_verbose("OpenGL: VBO supported\n");
		}
		else
		{
			mame_printf_warning("OpenGL: VBO not supported\n");
		}

		if ( sdl->usepbo )
		{
			mame_printf_verbose("OpenGL: PBO supported\n");
		}
		else
		{
			mame_printf_warning("OpenGL: PBO not supported\n");
		}

		if ( sdl->usefbo )
		{
			mame_printf_verbose("OpenGL: FBO supported\n");
		}
		else
		{
			mame_printf_warning("OpenGL: FBO not supported\n");
		}
	}
 
	if ( sdl->useglsl )
	{
		pfn_glActiveTexture = (PFNGLACTIVETEXTUREPROC) SDL_GL_GetProcAddress("glActiveTexture");
		if (!pfn_glActiveTexture)
		{
			if (_once)
			{
				mame_printf_warning("OpenGL: GLSL disabled, glActiveTexture not supported\n");
			}
			sdl->useglsl = 0;
		}
	}

	if ( sdl->useglsl )
	{
		sdl->useglsl = !glsl_shader_init(sdl);

		if ( ! sdl->useglsl )
		{
			if (_once)
			{
				mame_printf_warning("OpenGL: GLSL supported, but shader instantiation failed - disabled\n");
			}
		} 
	} 

	if ( sdl->useglsl )
	{
		if ( video_config.prescale != 1 )
		{
			sdl->useglsl = 0;
			if (_once)
			{
				mame_printf_warning("OpenGL: GLSL supported, but disabled due to: prescale !=1 \n");
			}
		}
		if ( video_config.prescale_effect != 0 )
		{
			sdl->useglsl = 0;
			if (_once)
			{
				mame_printf_warning("OpenGL: GLSL supported, but disabled due to: prescale_effect != 0\n");
			}
		}
	}

	if ( sdl->useglsl )
	{
		int i;
		video_config.filter = FALSE;
		glsl_shader_feature = GLSL_SHADER_FEAT_PLAIN;
		sdl->glsl_program_num = 0;
		sdl->glsl_program_mb2sc = 0;

		for(i=0; i<video_config.glsl_shader_mamebm_num; i++)
		{
			if ( !sdl->usefbo && sdl->glsl_program_num==1 )
			{
				if (_once)
				{
					mame_printf_verbose("OpenGL: GLSL multipass not supported, due to unsupported FBO. Skipping followup shader\n");
				}
				break;
			}

			if ( glsl_shader_add_mamebm(sdl, video_config.glsl_shader_mamebm[i], sdl->glsl_program_num) )
			{
				mame_printf_error("OpenGL: GLSL loading mame bitmap shader %d failed (%s)\n",
					i, video_config.glsl_shader_mamebm[i]);
			} else {
				glsl_shader_feature = GLSL_SHADER_FEAT_CUSTOM;
				if (_once)
				{
					mame_printf_verbose("OpenGL: GLSL using mame bitmap shader filter %d: '%s'\n", 
						sdl->glsl_program_num, video_config.glsl_shader_mamebm[i]);
				}
				sdl->glsl_program_mb2sc = sdl->glsl_program_num; // the last mame_bitmap (mb) shader does it.
				sdl->glsl_program_num++;
			}
		} 

		if ( video_config.glsl_shader_scrn_num > 0 && sdl->glsl_program_num==0 )
		{
			mame_printf_verbose("OpenGL: GLSL cannot use screen bitmap shader without bitmap shader\n");
		}

		for(i=0; sdl->usefbo && sdl->glsl_program_num>0 && i<video_config.glsl_shader_scrn_num; i++)
		{
			if ( glsl_shader_add_scrn(sdl, video_config.glsl_shader_scrn[i],
			                              sdl->glsl_program_num-1-sdl->glsl_program_mb2sc) )
			{
				mame_printf_error("OpenGL: GLSL loading screen bitmap shader %d failed (%s)\n",
					i, video_config.glsl_shader_scrn[i]);
			} else {
				if (_once)
				{
					mame_printf_verbose("OpenGL: GLSL using screen bitmap shader filter %d: '%s'\n", 
						sdl->glsl_program_num, video_config.glsl_shader_scrn[i]);
				}
				sdl->glsl_program_num++;
			}
		} 

		if ( 0==sdl->glsl_program_num &&
		     0 <= video_config.glsl_filter && video_config.glsl_filter < GLSL_SHADER_FEAT_INT_NUMBER )
		{
			sdl->glsl_program_mb2sc = sdl->glsl_program_num; // the last mame_bitmap (mb) shader does it.
			sdl->glsl_program_num++;
			glsl_shader_feature = video_config.glsl_filter;

			if (_once)
			{
				mame_printf_verbose("OpenGL: GLSL using shader filter '%s', idx: %d, num %d (vid filter: %d)\n", 
					glsl_shader_get_filter_name_mamebm(glsl_shader_feature),
					glsl_shader_feature, sdl->glsl_program_num, video_config.filter);
			}
		}

		if (_once)
		{
			if ( sdl->glsl_vid_attributes )
			{
				mame_printf_verbose("OpenGL: GLSL direct brightness, contrast setting for RGB games\n");
			}
			else
			{
				mame_printf_verbose("OpenGL: GLSL paletted gamma, brightness, contrast setting for RGB games\n");
			}
		}
	} else {
		if (_once)
		{
			mame_printf_verbose("OpenGL: using vid filter: %d\n", video_config.filter);
		}
	}

	_once = 0;
}

#define GL_NO_PRIMITIVE -1

static int drawogl_window_draw(sdl_window_info *window, UINT32 dc, int update)
{
	sdl_info *sdl = window->dxdata;
	render_primitive *prim;
	texture_info *texture=NULL;
	float vofs, hofs;
	static INT32 old_blitwidth = 0, old_blitheight = 0;
	static INT32 blittimer = 0;
	static INT32 surf_w = 0, surf_h = 0;
        static GLfloat texVerticex[8];
        int  pendingPrimitive=GL_NO_PRIMITIVE, curPrimitive=GL_NO_PRIMITIVE;

	if (video_config.novideo)
	{
		return 0;
	}

	// only clear if the geometry changes (and for 2 frames afterward to clear double and triple buffers)
	if ((blittimer > 0) || (video_config.isvector))
	{
		glClear(GL_COLOR_BUFFER_BIT);
		blittimer--;
	}

	if ((old_blitwidth != sdl->blitwidth) || (old_blitheight != sdl->blitheight))
	{
		glClear(GL_COLOR_BUFFER_BIT);

		old_blitwidth  = sdl->blitwidth;
		old_blitheight = sdl->blitheight;

		blittimer = 2;
	}

        if ( !window->initialized ||
             window->sdlsurf->w!= surf_w || window->sdlsurf->h!= surf_h 
           )
        {
		if ( !window->initialized )
		{
			loadGLExtensions(sdl);
		}

                surf_w=window->sdlsurf->w;
                surf_h=window->sdlsurf->h;

                // we're doing nothing 3d, so the Z-buffer is currently not interesting
                glDisable(GL_DEPTH_TEST);

                if (options_get_bool(mame_options(), OPTION_ANTIALIAS))
                {
                    // enable antialiasing for lines
                    glEnable(GL_LINE_SMOOTH);
                    // enable antialiasing for points
                    glEnable(GL_POINT_SMOOTH);

                    // prefer quality to speed
                    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
                    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
                }
                else
                {
                    glDisable(GL_LINE_SMOOTH);
                    glDisable(GL_POINT_SMOOTH);
                }

                // enable blending
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                sdl->last_blendmode = BLENDMODE_ALPHA;

                // set lines and points just barely above normal size to get proper results
                glLineWidth(video_config.beamwidth);
                glPointSize(video_config.beamwidth);

                // set up a nice simple 2D coordinate system, so GL behaves exactly how we'd like.
                //
                // (0,0)     (w,0)
                //   |~~~~~~~~~|
                //   |	       |
                //   |	       |
                //   |	       |
                //   |_________|
                // (0,h)     (w,h)

                glViewport(0.0, 0.0, (GLsizei)window->sdlsurf->w, (GLsizei)window->sdlsurf->h);
                glMatrixMode(GL_PROJECTION);
                glLoadIdentity();
                glOrtho(0.0, (GLdouble)window->sdlsurf->w, (GLdouble)window->sdlsurf->h, 0.0, 0.0, -1.0);
		glMatrixMode(GL_MODELVIEW);
                glLoadIdentity();

                if ( ! window->initialized )
		{
                        glEnableClientState(GL_VERTEX_ARRAY);
                        glVertexPointer(2, GL_FLOAT, 0, texVerticex); // no VBO, since it's too volatile

			window->initialized = 1;
                }
        }

	// compute centering parameters
	vofs = hofs = 0.0f;

	if (video_config.centerv || video_config.centerh)
	{
		int ch, cw;

		if ((!video_config.windowed) && (!video_config.switchres))
		{
			ch = window->monitor->center_height;
			cw = window->monitor->center_width;
		}
		else
		{
			ch = window->sdlsurf->h;
			cw = window->sdlsurf->w;
		}

		if (video_config.centerv)
		{
			vofs = (ch - sdl->blitheight) / 2.0f;
		}
		if (video_config.centerh)
		{
			hofs = (cw - sdl->blitwidth) / 2.0f;
		}
	}

	osd_lock_acquire(window->primlist->lock);

	// now draw
	for (prim = window->primlist->head; prim != NULL; prim = prim->next)
	{
		int i;

		switch (prim->type)
		{
			/**
			 * Try to stay in one Begin/End block as long as possible,
			 * since entering and leaving one is most expensive..
			 */
			case RENDER_PRIMITIVE_LINE:
				// check if it's really a point
				if (((prim->bounds.x1 - prim->bounds.x0) == 0) && ((prim->bounds.y1 - prim->bounds.y0) == 0))
				{
				    curPrimitive=GL_POINTS;
				} else {
				    curPrimitive=GL_LINES;
				}

                                if(pendingPrimitive!=GL_NO_PRIMITIVE && pendingPrimitive!=curPrimitive)
				{
				    glEnd();
				    pendingPrimitive=GL_NO_PRIMITIVE;
				}

                                if ( pendingPrimitive==GL_NO_PRIMITIVE )
                                    set_blendmode(sdl, PRIMFLAG_GET_BLENDMODE(prim->flags), 0);

				glColor4f(prim->color.r, prim->color.g, prim->color.b, prim->color.a);

                                if(pendingPrimitive!=curPrimitive)
                                {
                                    glBegin(curPrimitive);
                                    pendingPrimitive=curPrimitive;
                                }

				// check if it's really a point
				if (curPrimitive==GL_POINTS)
				{
                                    glVertex2f(prim->bounds.x0+hofs, prim->bounds.y0+vofs);
				}
				else
				{
                                    glVertex2f(prim->bounds.x0+hofs, prim->bounds.y0+vofs);
                                    glVertex2f(prim->bounds.x1+hofs, prim->bounds.y1+vofs);
				}
				break;

			case RENDER_PRIMITIVE_QUAD:

                                if(pendingPrimitive!=GL_NO_PRIMITIVE)
				{
				    glEnd();
				    pendingPrimitive=GL_NO_PRIMITIVE;
				}

                                glColor4f(prim->color.r, prim->color.g, prim->color.b, prim->color.a);

				set_blendmode(sdl, PRIMFLAG_GET_BLENDMODE(prim->flags), texture);

				texture = texture_update(sdl, window, prim, 0);

				if ( texture && texture->type==TEXTURE_TYPE_SHADER )
				{
					for(i=0; i<sdl->glsl_program_num; i++)
					{
						if ( i==sdl->glsl_program_mb2sc )
						{
							// i==sdl->glsl_program_mb2sc -> transformation mamebm->scrn
							texVerticex[0]=prim->bounds.x0 + hofs;
							texVerticex[1]=prim->bounds.y0 + vofs;
							texVerticex[2]=prim->bounds.x1 + hofs;
							texVerticex[3]=prim->bounds.y0 + vofs;
							texVerticex[4]=prim->bounds.x1 + hofs;
							texVerticex[5]=prim->bounds.y1 + vofs;
							texVerticex[6]=prim->bounds.x0 + hofs;
							texVerticex[7]=prim->bounds.y1 + vofs;
						} else {
							// 1:1 tex coord CCW (0/0) (1/0) (1/1) (0/1) on texture dimensions
							texVerticex[0]=(GLfloat)0.0;
							texVerticex[1]=(GLfloat)0.0;
							texVerticex[2]=(GLfloat)window->sdlsurf->w;
							texVerticex[3]=(GLfloat)0.0;
							texVerticex[4]=(GLfloat)window->sdlsurf->w;
							texVerticex[5]=(GLfloat)window->sdlsurf->h;
							texVerticex[6]=(GLfloat)0.0;
							texVerticex[7]=(GLfloat)window->sdlsurf->h;
						}

						if(i>0) // first fetch already done
						{
							texture = texture_update(sdl, window, prim, i);
						}
						glDrawArrays(GL_QUADS, 0, 4);
					}
				} else {
					texVerticex[0]=prim->bounds.x0 + hofs;
					texVerticex[1]=prim->bounds.y0 + vofs;
					texVerticex[2]=prim->bounds.x1 + hofs;
					texVerticex[3]=prim->bounds.y0 + vofs;
					texVerticex[4]=prim->bounds.x1 + hofs;
					texVerticex[5]=prim->bounds.y1 + vofs;
					texVerticex[6]=prim->bounds.x0 + hofs;
					texVerticex[7]=prim->bounds.y1 + vofs;

					glDrawArrays(GL_QUADS, 0, 4);
				}

                                if ( texture )
                                {
                                        texture_disable(sdl, texture);
                                        texture=NULL;
                                }
                                break;
		}
	}

        if(pendingPrimitive!=GL_NO_PRIMITIVE)
        {
                glEnd();
                pendingPrimitive=GL_NO_PRIMITIVE;
        }


	osd_lock_release(window->primlist->lock);

	SDL_GL_SwapBuffers();
	return 0;
}
#endif

//============================================================
//  drawsdl_blit_surface_size
//============================================================

void drawsdl_blit_surface_size(sdl_window_info *window, int window_width, int window_height)
{
	sdl_info *sdl = window->dxdata;
	INT32 newwidth, newheight;
	int xscale, yscale;
	float desired_aspect = 1.0f;
	INT32 target_width = window_width;
	INT32 target_height = window_height;

	// start with the minimum size
	render_target_get_minimum_size(window->target, &newwidth, &newheight);

	// compute the appropriate visible area if we're trying to keepaspect
	if (video_config.keepaspect)
	{
		// make sure the monitor is up-to-date
		sdlvideo_monitor_refresh(window->monitor);
		render_target_compute_visible_area(window->target, target_width, target_height, sdlvideo_monitor_get_aspect(window->monitor), render_target_get_orientation(window->target), &target_width, &target_height);
		desired_aspect = (float)target_width / (float)target_height;
	}

//	logerror("Render target wants %d x %d, minimum is %d x %d\n", target_width, target_height, newwidth, newheight);

        // don't allow below 1:1 size - this prevents the OutRunners "death spiral"
        // that would occur if you kept rotating it with fullstretch on in SDLMAME u10 test 2.
        if ((target_width >= newwidth) && (target_height >= newheight))
	{
                newwidth = target_width;
                newheight = target_height;
	}

        // non-integer scaling - often gives more pleasing results in full screen 
        if (!video_config.fullstretch)
	{
		// compute maximum integral scaling to fit the window
		xscale = (target_width + 2) / newwidth;
		yscale = (target_height + 2) / newheight;

		// try a little harder to keep the aspect ratio if desired
		if (video_config.keepaspect)
		{
			// if we could stretch more in the X direction, and that makes a better fit, bump the xscale
			while (newwidth * (xscale + 1) <= window_width &&
				better_mode(newwidth * xscale, newheight * yscale, newwidth * (xscale + 1), newheight * yscale, desired_aspect))
				xscale++;

			// if we could stretch more in the Y direction, and that makes a better fit, bump the yscale
			while (newheight * (yscale + 1) <= window_height &&
				better_mode(newwidth * xscale, newheight * yscale, newwidth * xscale, newheight * (yscale + 1), desired_aspect))
				yscale++;

			// now that we've maxed out, see if backing off the maximally stretched one makes a better fit
			if (window_width - newwidth * xscale < window_height - newheight * yscale)
			{
				while (better_mode(newwidth * xscale, newheight * yscale, newwidth * (xscale - 1), newheight * yscale, desired_aspect) && (xscale >= 0))
					xscale--;
			}
			else
			{
				while (better_mode(newwidth * xscale, newheight * yscale, newwidth * xscale, newheight * (yscale - 1), desired_aspect) && (yscale >= 0))
					yscale--;
			}
		}

		// ensure at least a scale factor of 1
		if (xscale <= 0) xscale = 1;
		if (yscale <= 0) yscale = 1;

		// apply the final scale
		newwidth *= xscale;
		newheight *= yscale;
	}

	if ((render_target_get_layer_config(window->target) & LAYER_CONFIG_ZOOM_TO_SCREEN)
		&& video_config.yuv_mode == VIDEO_YUV_MODE_NONE)
		newwidth = window_width;

	if (((sdl->blitwidth != newwidth) || (sdl->blitheight != newheight)) && !(window->opengl) && (window->sdlsurf))
	{
		sdlwindow_clear_surface(window, 3);
	}

	sdl->blitwidth = newwidth;
	sdl->blitheight = newheight;
}

#if USE_OPENGL

static const char * texfmt_to_string[9] = {
        "ARGB32",
        "RGB32",
        "RGB32_PALETTED",
        "YUV16",
        "YUV16_PALETTED",
        "PALETTE16",
        "RGB15",
        "RGB15_PALETTE",
        "PALETTE16A"
        };

//
// Note: if you change the following array order, change the matching defines in texsrc.h
//

enum { SDL_TEXFORMAT_SRC_EQUALS_DEST, SDL_TEXFORMAT_SRC_HAS_PALETTE };

static const GLint texture_copy_properties[9][2] = {
	{ TRUE,  FALSE },   // SDL_TEXFORMAT_ARGB32
	{ TRUE,  FALSE },   // SDL_TEXFORMAT_RGB32
	{ TRUE,  TRUE  },   // SDL_TEXFORMAT_RGB32_PALETTED
	{ FALSE, FALSE },   // SDL_TEXFORMAT_YUY16
	{ FALSE, TRUE  },   // SDL_TEXFORMAT_YUY16_PALETTED
	{ FALSE, TRUE  },   // SDL_TEXFORMAT_PALETTE16
	{ TRUE,  FALSE },   // SDL_TEXFORMAT_RGB15
	{ TRUE,  TRUE  },   // SDL_TEXFORMAT_RGB15_PALETTED
	{ FALSE, TRUE  }    // SDL_TEXFORMAT_PALETTE16A
};

// 6 properties (per format)
// right order according to glTexImage2D: internal, format, type, ..
enum { SDL_TEXFORMAT_INTERNAL, SDL_TEXFORMAT_FORMAT, SDL_TEXFORMAT_TYPE, SDL_TEXFORMAT_PIXEL_SIZE };

static const GLint texture_gl_properties_srcNative_intNative[9][6] = {
	{ GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, sizeof(UINT32) },    // SDL_TEXFORMAT_ARGB32
	{ GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, sizeof(UINT32) },    // SDL_TEXFORMAT_RGB32
	{ GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, sizeof(UINT32) },    // SDL_TEXFORMAT_RGB32_PALETTED
#ifdef SDLMAME_MACOSX
	{ GL_RGB8, GL_YCBCR_422_APPLE, GL_UNSIGNED_SHORT_8_8_REV_APPLE, sizeof(UINT16) }, // SDL_TEXFORMAT_YUY16
	{ GL_RGB8, GL_YCBCR_422_APPLE, GL_UNSIGNED_SHORT_8_8_REV_APPLE, sizeof(UINT16) }, // SDL_TEXFORMAT_YUY16_PALETTED
#else
	{ GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, sizeof(UINT32) },   // SDL_TEXFORMAT_YUY16
	{ GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, sizeof(UINT32) },   // SDL_TEXFORMAT_YUY16_PALETTED
#endif
	{ GL_RGB5_A1, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, sizeof(UINT16) }, // SDL_TEXFORMAT_PALETTE16
	{ GL_RGB5_A1, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, sizeof(UINT16) }, // SDL_TEXFORMAT_RGB15
	{ GL_RGB5_A1, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, sizeof(UINT16) }, // SDL_TEXFORMAT_RGB15_PALETTED
	{ GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, sizeof(UINT32) }    // SDL_TEXFORMAT_PALETTE16A
};

static const GLint texture_gl_properties_srcNative_int32bpp[9][6] = {
	{ GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, sizeof(UINT32) },    // SDL_TEXFORMAT_ARGB32
	{ GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, sizeof(UINT32) },    // SDL_TEXFORMAT_RGB32
	{ GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, sizeof(UINT32) },    // SDL_TEXFORMAT_RGB32_PALETTED
#ifdef SDLMAME_MACOSX
	{ GL_RGB8, GL_YCBCR_422_APPLE, GL_UNSIGNED_SHORT_8_8_REV_APPLE, sizeof(UINT16) }, // SDL_TEXFORMAT_YUY16
	{ GL_RGB8, GL_YCBCR_422_APPLE, GL_UNSIGNED_SHORT_8_8_REV_APPLE, sizeof(UINT16) }, // SDL_TEXFORMAT_YUY16_PALETTED
#else
	{ GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, sizeof(UINT32) },   // SDL_TEXFORMAT_YUY16
	{ GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, sizeof(UINT32) },   // SDL_TEXFORMAT_YUY16_PALETTED
#endif
	{ GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, sizeof(UINT32) },   // SDL_TEXFORMAT_PALETTE16
	{ GL_RGBA8, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, sizeof(UINT16) },   // SDL_TEXFORMAT_RGB15
	{ GL_RGBA8, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, sizeof(UINT16) },   // SDL_TEXFORMAT_RGB15_PALETTED
	{ GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, sizeof(UINT32) }    // SDL_TEXFORMAT_PALETTE16A
};

static const GLint texture_gl_properties_srcCopy_int32bpp[9][6] = {
	{ GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, sizeof(UINT32) },    // SDL_TEXFORMAT_ARGB32
	{ GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, sizeof(UINT32) },    // SDL_TEXFORMAT_RGB32
	{ GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, sizeof(UINT32) },    // SDL_TEXFORMAT_RGB32_PALETTED
#ifdef SDLMAME_MACOSX
	{ GL_RGB8, GL_YCBCR_422_APPLE, GL_UNSIGNED_SHORT_8_8_REV_APPLE, sizeof(UINT16) }, // SDL_TEXFORMAT_YUY16
	{ GL_RGB8, GL_YCBCR_422_APPLE, GL_UNSIGNED_SHORT_8_8_REV_APPLE, sizeof(UINT16) }, // SDL_TEXFORMAT_YUY16_PALETTED
#else
	{ GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, sizeof(UINT32) },   // SDL_TEXFORMAT_YUY16
	{ GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, sizeof(UINT32) },   // SDL_TEXFORMAT_YUY16_PALETTED
#endif
	{ GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, sizeof(UINT32) },   // SDL_TEXFORMAT_PALETTE16
	{ GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, sizeof(UINT32) },   // SDL_TEXFORMAT_RGB15
	{ GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, sizeof(UINT32) },   // SDL_TEXFORMAT_RGB15_PALETTED
	{ GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, sizeof(UINT32) }    // SDL_TEXFORMAT_PALETTE16A
};

static const texture_copy_func texcopy_dstNative_f[2][9] = { {
	texcopy_argb32,
	texcopy_rgb32,
	texcopy_rgb32_paletted,
#ifdef SDLMAME_MACOSX
	texcopy_yuv16_apple,
	texcopy_yuv16_paletted_apple,
#else
	texcopy_yuv16,
	texcopy_yuv16_paletted,
#endif

	texcopy_palette16_argb1555,
	texcopy_rgb15_argb1555,
	texcopy_rgb15_paletted_argb1555,
	texcopy_palette16a
	}, 
	{
	scale2x_argb32,
	scale2x_rgb32,
	scale2x_rgb32_paletted,
	scale2x_yuv16,
	scale2x_yuv16_paletted,

	scale2x_palette16_argb1555,
	scale2x_rgb15_argb1555,
	scale2x_rgb15_paletted_argb1555,
	scale2x_palette16a
	} };

static const texture_copy_func texcopy_dst32bpp_f[2][9] = { {
	texcopy_argb32,
	texcopy_rgb32,
	texcopy_rgb32_paletted,
#ifdef SDLMAME_MACOSX
	texcopy_yuv16_apple,
	texcopy_yuv16_paletted_apple,
#else
	texcopy_yuv16,
	texcopy_yuv16_paletted,
#endif

	texcopy_palette16,
	texcopy_rgb15,
	texcopy_rgb15_paletted,
	texcopy_palette16a
	}, 
	{
	scale2x_argb32,
	scale2x_rgb32,
	scale2x_rgb32_paletted,
	scale2x_yuv16,
	scale2x_yuv16_paletted,

	scale2x_palette16,
	scale2x_rgb15,
	scale2x_rgb15_paletted,
	scale2x_palette16a
	} };

//============================================================
//  drawogl_exit
//============================================================

static void drawogl_exit(void)
{
}

//============================================================
//  drawogl_window_destroy
//============================================================

static void drawogl_window_destroy(sdl_window_info *window)
{
	sdl_info *sdl = window->dxdata;

	// skip if nothing
	if (sdl == NULL)
		return;

	// free the memory in the window

	drawogl_destroy_all_textures(window);

	free(sdl);
	window->dxdata = NULL;
}

//============================================================
//  texture_compute_size and type
//============================================================

//
// glBufferData to push a nocopy texture to the GPU is slower than TexSubImage2D, 
// so don't use PBO here
//
// we also don't want to use PBO's in the case of nocopy==TRUE,
// since we now might have GLSL shaders - this decision simplifies out life ;-)
//
static void texture_compute_type_subroutine(sdl_info *sdl, const render_texinfo *texsource, texture_info *texture, UINT32 flags)
{
	texture->type = TEXTURE_TYPE_NONE;
	texture->nocopy = FALSE;

	if ( texture->type == TEXTURE_TYPE_NONE &&
	     !PRIMFLAG_GET_SCREENTEX(flags))
	{
		texture->type = TEXTURE_TYPE_PLAIN;
                texture->texTarget = (sdl->usetexturerect)?GL_TEXTURE_RECTANGLE_ARB:GL_TEXTURE_2D;
                texture->texpow2   = (sdl->usetexturerect)?0:sdl->texpoweroftwo;
	} 

	// currently glsl supports idx and rgb palette lookups,
	// no special quality scaling, so we could drop the prescale criteria below ..
	if ( texture->type == TEXTURE_TYPE_NONE &&
	     sdl->useglsl && 
	     (
		 texture->format==SDL_TEXFORMAT_PALETTE16 ||       // glsl idx16 lut 
	         texture->format==SDL_TEXFORMAT_RGB32_PALETTED ||  // glsl rgb32 lut/direct
	         texture->format==SDL_TEXFORMAT_RGB15_PALETTED     // glsl rgb15 lut/direct
	     ) &&
	     texture->xprescale == 1 && texture->yprescale == 1 &&
	     texsource->rowpixels <= sdl->texture_max_width
	    )
	{
		texture->type      = TEXTURE_TYPE_SHADER;
		texture->nocopy    = TRUE;
                texture->texTarget = GL_TEXTURE_2D; 
                texture->texpow2   = sdl->texpoweroftwo;
	}

	// determine if we can skip the copy step
	// if this was not already decided by the shader condition above
	if ( !texture->nocopy && 
	      texture_copy_properties[texture->format][SDL_TEXFORMAT_SRC_EQUALS_DEST] &&
	     !texture_copy_properties[texture->format][SDL_TEXFORMAT_SRC_HAS_PALETTE] &&
	      texture->xprescale == 1 && texture->yprescale == 1 &&
	     !texture->borderpix &&
	      texsource->rowpixels <= sdl->texture_max_width )
	{
		texture->nocopy = TRUE;
        } 

	if( texture->type == TEXTURE_TYPE_NONE &&
	    sdl->usepbo && !texture->nocopy )
	{

		texture->type      = TEXTURE_TYPE_DYNAMIC;
                texture->texTarget = (sdl->usetexturerect)?GL_TEXTURE_RECTANGLE_ARB:GL_TEXTURE_2D;
                texture->texpow2   = (sdl->usetexturerect)?0:sdl->texpoweroftwo;
	} 

	if( texture->type == TEXTURE_TYPE_NONE )
	{
		texture->type      = TEXTURE_TYPE_SURFACE;
                texture->texTarget = (sdl->usetexturerect)?GL_TEXTURE_RECTANGLE_ARB:GL_TEXTURE_2D;
                texture->texpow2   = (sdl->usetexturerect)?0:sdl->texpoweroftwo;
        }

        if ( texture->type!=TEXTURE_TYPE_SHADER && video_config.prefer16bpp_tex )
        {
                texture->texProperties = texture_gl_properties_srcNative_intNative[texture->format];
                texture->texCopyFn     = texcopy_dstNative_f[texture->prescale_effect][texture->format];
        } else if ( texture->nocopy )
        {
                texture->texProperties = texture_gl_properties_srcNative_int32bpp[texture->format];
                texture->texCopyFn     = NULL;
        } else {
                texture->texProperties = texture_gl_properties_srcCopy_int32bpp[texture->format];
                texture->texCopyFn     = texcopy_dst32bpp_f[texture->prescale_effect][texture->format];
        }
}

static inline int get_valid_pow2_value(int v, int needPow2)
{
	return (needPow2)?gl_round_to_pow2(v):v;
}

static void texture_compute_size_subroutine(sdl_info *sdl, texture_info *texture, UINT32 flags,
                                            UINT32 width, UINT32 height,
                                            int* p_width, int* p_height, int* p_width_create, int* p_height_create) 
{
        int width_create;
        int height_create;

	if ( texture->texpow2 ) 
        {
                width_create  = gl_round_to_pow2 (width);
                height_create = gl_round_to_pow2 (height);
        } else if ( texture->type==TEXTURE_TYPE_SHADER ) 
        {
                /**
                 * at least use a multiple of 8 for shader .. just in case
                 */
                width_create  = ( width  & ~0x07 ) + ( (width  & 0x07)? 8 : 0 ) ;
                height_create = ( height & ~0x07 ) + ( (height & 0x07)? 8 : 0 ) ;
        } else {
                width_create  = width  ;
                height_create = height ;
        }

	// don't prescale above max texture size
	while (texture->xprescale > 1 && width_create * texture->xprescale > sdl->texture_max_width)
		texture->xprescale--;
	while (texture->yprescale > 1 && height_create * texture->yprescale > sdl->texture_max_height)
		texture->yprescale--;
	if (PRIMFLAG_GET_SCREENTEX(flags) && (texture->xprescale != video_config.prescale || texture->yprescale != video_config.prescale))
		mame_printf_warning("SDL: adjusting prescale from %dx%d to %dx%d\n", video_config.prescale, video_config.prescale, texture->xprescale, texture->yprescale);

	width  *= texture->xprescale;
	height *= texture->yprescale;
	width_create  *= texture->xprescale;
	height_create *= texture->yprescale;

	// adjust the size for the border (must do this *after* the power of 2 clamp to satisfy
	// OpenGL semantics)
	if (texture->borderpix)
	{
		width += 2;
		height += 2;
		width_create += 2;
		height_create += 2;
	}
        *p_width=width;
        *p_height=height;
        *p_width_create=width_create;
        *p_height_create=height_create;
}

static void texture_compute_size_type(sdl_info *sdl, const render_texinfo *texsource, texture_info *texture, UINT32 flags)
{
	int finalheight, finalwidth;
	int finalheight_create, finalwidth_create;

	// if we're not wrapping, add a 1 pixel border on all sides
	texture->borderpix = 0; //!(texture->flags & PRIMFLAG_TEXWRAP_MASK);
	if (PRIMFLAG_GET_SCREENTEX(flags))
	{
		texture->borderpix = 0;	// don't border the screen right now, there's a bug
	}

        texture_compute_type_subroutine(sdl, texsource, texture, flags);
         
        texture_compute_size_subroutine(sdl, texture, flags, texsource->width, texsource->height, 
                                        &finalwidth, &finalheight, &finalwidth_create, &finalheight_create);

	// if we added pixels for the border, and that just barely pushed us over, take it back
	if (texture->borderpix &&
		((finalwidth > sdl->texture_max_width && finalwidth - 2 <= sdl->texture_max_width) ||
		 (finalheight > sdl->texture_max_height && finalheight - 2 <= sdl->texture_max_height)))
	{
		texture->borderpix = FALSE;

                texture_compute_type_subroutine(sdl, texsource, texture, flags);
                 
                texture_compute_size_subroutine(sdl, texture, flags, texsource->width, texsource->height, 
                                                &finalwidth, &finalheight, &finalwidth_create, &finalheight_create);
	}

	// if we're above the max width/height, do what?
        if (finalwidth_create > sdl->texture_max_width || finalheight_create > sdl->texture_max_height)
        {
            static int printed = FALSE;
            if (!printed) 
            	mame_printf_warning("Texture too big! (wanted: %dx%d, max is %dx%d)\n", finalwidth_create, finalheight_create, sdl->texture_max_width, sdl->texture_max_height);
            printed = TRUE;
        }

        if(!texture->nocopy || texture->type==TEXTURE_TYPE_DYNAMIC || texture->type==TEXTURE_TYPE_SHADER ||
            // any of the mame core's device generated bitmap types:
            texture->format==SDL_TEXFORMAT_RGB32  ||
            texture->format==SDL_TEXFORMAT_RGB32_PALETTED  ||
            texture->format==SDL_TEXFORMAT_RGB15  ||
            texture->format==SDL_TEXFORMAT_RGB15_PALETTED  ||
            texture->format==SDL_TEXFORMAT_PALETTE16  ||
            texture->format==SDL_TEXFORMAT_PALETTE16A
            )
        {
        mame_printf_verbose("GL texture: copy %d, shader %d, dynamic %d, %dx%d %dx%d [%s, Equal: %d, Palette: %d,\n"
	                    "            scale %dx%d (eff: %d), border %d, pitch %d,%d/%d], colors: %d, bytes/pix %d\n", 
                !texture->nocopy, texture->type==TEXTURE_TYPE_SHADER, texture->type==TEXTURE_TYPE_DYNAMIC,
                finalwidth, finalheight, finalwidth_create, finalheight_create, 
                texfmt_to_string[texture->format], 
                (int)texture_copy_properties[texture->format][SDL_TEXFORMAT_SRC_EQUALS_DEST],
                (int)texture_copy_properties[texture->format][SDL_TEXFORMAT_SRC_HAS_PALETTE],
                texture->xprescale, texture->yprescale, texture->prescale_effect,
		texture->borderpix, texsource->rowpixels, finalwidth, sdl->texture_max_width,
		sdl->totalColors, (int)texture->texProperties[SDL_TEXFORMAT_PIXEL_SIZE]
	      );
        }

	// set the final values
	texture->rawwidth = finalwidth;
	texture->rawheight = finalheight;
	texture->rawwidth_create = finalwidth_create;
	texture->rawheight_create = finalheight_create;
}

//============================================================
//  texture_create
//============================================================

static int gl_checkFramebufferStatus(void) 
{
    GLenum status;
    status=(GLenum)pfn_glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);
    switch(status) {
        case GL_FRAMEBUFFER_COMPLETE_EXT:
            return 0;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
            mame_printf_error("GL FBO: incomplete,incomplete attachment\n");
            return -1;
        case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
            mame_printf_error("GL FBO: Unsupported framebuffer format\n");
            return -1;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
            mame_printf_error("GL FBO: incomplete,missing attachment\n");
            return -1;
        case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
            mame_printf_error("GL FBO: incomplete,attached images must have same dimensions\n");
            return -1;
        case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
             mame_printf_error("GL FBO: incomplete,attached images must have same format\n");
            return -1;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
            mame_printf_error("GL FBO: incomplete,missing draw buffer\n");
            return -1;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
            mame_printf_error("GL FBO: incomplete,missing read buffer\n");
            return -1;
#ifdef GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT 
        case GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT:
            mame_printf_error("GL FBO: incomplete, duplicate attachment\n");
            return -1;
#endif
        case 0:
            mame_printf_error("GL FBO: incomplete, implementation fault\n");
            return -1;
        default:
            mame_printf_error("GL FBO: incomplete, implementation ERROR\n");
            return -1;
    }
    return -1;
}

static int texture_fbo_create(UINT32 text_unit, UINT32 text_name, UINT32 fbo_name, int width, int height)
{
	pfn_glActiveTexture(text_unit);
	pfn_glBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo_name);
	glBindTexture(GL_TEXTURE_2D, text_name);
	{
		GLint _width, _height;
		if ( gl_texture_check_size(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 
					   0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, &_width, &_height, 1) )
		{
			mame_printf_error("cannot create fbo texture, req: %dx%d, avail: %dx%d - bail out\n",
					  width, height, (int)_width, (int)_height);
			return -1;
		}

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 
		     0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, NULL );
	}
	// non-screen textures will never be filtered
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	pfn_glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, 
				   GL_TEXTURE_2D, text_name, 0);

	if ( gl_checkFramebufferStatus() )
	{
		mame_printf_error("FBO error fbo texture - bail out\n");
		return -1;
	}

	return 0;
}

static int texture_shader_create(sdl_info *sdl, sdl_window_info *window, 
                                 const render_texinfo *texsource, texture_info *texture, UINT32 flags)
{
	int uniform_location;
	int lut_table_width_pow2=0;
	int lut_table_height_pow2=0;
	int glsl_shader_type_rgb32 = ( sdl->glsl_vid_attributes ) ? GLSL_SHADER_TYPE_RGB32_DIRECT : GLSL_SHADER_TYPE_RGB32_LUT;
	int glsl_shader_type, i;
	int lut_texture_width;
	int surf_w_pow2  = get_valid_pow2_value (window->sdlsurf->w, texture->texpow2);
	int surf_h_pow2  = get_valid_pow2_value (window->sdlsurf->h, texture->texpow2);

	assert ( texture->type==TEXTURE_TYPE_SHADER );

	texture->lut_table_height = 1; // default ..

	switch(texture->format)
	{
		case SDL_TEXFORMAT_RGB32_PALETTED:
			glsl_shader_type          = glsl_shader_type_rgb32;
			texture->lut_table_width  = 1 << 8; // 8 bits per component
			texture->lut_table_width *= 3;      // BGR ..
			break;

		case SDL_TEXFORMAT_RGB15_PALETTED:
			glsl_shader_type          = glsl_shader_type_rgb32;
			texture->lut_table_width  = 1 << 5; // 5 bits per component
			texture->lut_table_width *= 3;      // BGR ..
			break;

		case SDL_TEXFORMAT_PALETTE16:
			glsl_shader_type          = GLSL_SHADER_TYPE_IDX16;
		        texture->lut_table_width  = sdl->totalColors;
			break;

		default:
			// should never happen
			assert(0);
			exit(1);
	}

	/**
	 * We experience some GLSL LUT calculation inaccuracy on some GL drivers.
	 * while using the correct lut calculations.
	 * This error is due to the color index value to GLSL/texture passing process:
	 *   mame:uint16_t -> OpenGL: GLfloat(alpha texture) -> GLSL:uint16_t (value regeneration)
	 * The latter inaccurate uint16_t value regeneration is buggy on some drivers/cards,
	 * therefor we always widen the lut size to pow2,
	 * and shape it equaly into 2D space (max texture size restriction).
	 * This is a practical GL driver workaround to minimize the chance for
	 * floating point arithmetic errors in the GLSL engine.
	 *
	 * Shape the lut texture to achieve texture max size compliance and equal 2D partitioning
	 */
	lut_texture_width  = sqrt((double)(texture->lut_table_width));
	lut_texture_width  = get_valid_pow2_value (lut_texture_width, 1);

	texture->lut_table_height = texture->lut_table_width / lut_texture_width;

	if ( lut_texture_width*texture->lut_table_height < texture->lut_table_width )
	{
		texture->lut_table_height  += 1;
	}

	texture->lut_table_width   = lut_texture_width;

	/**
	 * always use pow2 for LUT, to minimize the chance for floating point arithmetic errors 
	 * (->buggy GLSL engine)
	 */
	lut_table_height_pow2 = get_valid_pow2_value (texture->lut_table_height, 1 /* texture->texpow2 */);
	lut_table_width_pow2  = get_valid_pow2_value (texture->lut_table_width,  1 /* texture->texpow2 */);

	if ( !sdl->glsl_vid_attributes || texture->format==SDL_TEXFORMAT_PALETTE16 )
	{
		mame_printf_verbose("GL texture: lut_texture_width %d, lut_table_sz %dx%d, lut_table_sz_pow2 %dx%d\n",
				lut_texture_width, texture->lut_table_width, texture->lut_table_height,
				lut_table_width_pow2, lut_table_height_pow2);
	}


	if ( lut_table_width_pow2 > sdl->texture_max_width || lut_table_height_pow2 > sdl->texture_max_height )
	{
		mame_printf_error("Need lut size %dx%d, but max text size is %dx%d, bail out\n",
			lut_table_width_pow2, lut_table_height_pow2,
			sdl->texture_max_width, sdl->texture_max_height);
		return -1;
	}

	GL_CHECK_ERROR_QUIET();

	if( sdl->glsl_program_num > 1 )
	{
		// multipass mode
		assert(sdl->usefbo);

		// GL_TEXTURE3 GLSL Uniforms 
		texture->mpass_dest_idx = 0;
		texture->mpass_textureunit[0] = GL_TEXTURE3;
		texture->mpass_textureunit[1] = GL_TEXTURE2;
	}

	for(i=0; i<sdl->glsl_program_num; i++)
	{
		if ( i<=sdl->glsl_program_mb2sc )
		{
			sdl->glsl_program[i] = glsl_shader_get_program_mamebm(glsl_shader_type, glsl_shader_feature, i);
		} else {
			sdl->glsl_program[i] = glsl_shader_get_program_scrn(i-1-sdl->glsl_program_mb2sc);
		}
		pfn_glUseProgramObjectARB(sdl->glsl_program[i]);

		if ( i<=sdl->glsl_program_mb2sc && !(sdl->glsl_vid_attributes && texture->format!=SDL_TEXFORMAT_PALETTE16) )
		{
			// GL_TEXTURE1 GLSL Uniforms
			uniform_location = pfn_glGetUniformLocationARB(sdl->glsl_program[i], "colortable_texture");
			pfn_glUniform1iARB(uniform_location, 1);
			GL_CHECK_ERROR_NORMAL();

			{
				GLfloat colortable_sz[2] = { (GLfloat)texture->lut_table_width, (GLfloat)texture->lut_table_height };
				uniform_location = pfn_glGetUniformLocationARB(sdl->glsl_program[i], "colortable_sz");
				pfn_glUniform2fvARB(uniform_location, 1, &(colortable_sz[0]));
				GL_CHECK_ERROR_NORMAL();
			}

			{
				GLfloat colortable_pow2_sz[2] = { (GLfloat)lut_table_width_pow2, (GLfloat)lut_table_height_pow2 };
				uniform_location = pfn_glGetUniformLocationARB(sdl->glsl_program[i], "colortable_pow2_sz");
				pfn_glUniform2fvARB(uniform_location, 1, &(colortable_pow2_sz[0]));
				GL_CHECK_ERROR_NORMAL();
			}
		}

		if ( i<=sdl->glsl_program_mb2sc )
		{
			// GL_TEXTURE0 GLSL Uniforms
			uniform_location = pfn_glGetUniformLocationARB(sdl->glsl_program[i], "color_texture");
			pfn_glUniform1iARB(uniform_location, 0);
			GL_CHECK_ERROR_NORMAL();
		}

		{
			GLfloat color_texture_sz[2] = { (GLfloat)texture->rawwidth, (GLfloat)texture->rawheight };
			uniform_location = pfn_glGetUniformLocationARB(sdl->glsl_program[i], "color_texture_sz");
			pfn_glUniform2fvARB(uniform_location, 1, &(color_texture_sz[0]));
			GL_CHECK_ERROR_NORMAL();
		}

		{
			GLfloat color_texture_pow2_sz[2] = { (GLfloat)texture->rawwidth_create, (GLfloat)texture->rawheight_create };
			uniform_location = pfn_glGetUniformLocationARB(sdl->glsl_program[i], "color_texture_pow2_sz");
			pfn_glUniform2fvARB(uniform_location, 1, &(color_texture_pow2_sz[0]));
			GL_CHECK_ERROR_NORMAL();
		}
		if ( i>sdl->glsl_program_mb2sc )
		{
			{
				GLfloat screen_texture_sz[2] = { (GLfloat)window->sdlsurf->w, (GLfloat)window->sdlsurf->h };
				uniform_location = pfn_glGetUniformLocationARB(sdl->glsl_program[i], "screen_texture_sz");
				pfn_glUniform2fvARB(uniform_location, 1, &(screen_texture_sz[0]));
				GL_CHECK_ERROR_NORMAL();
			}

			{
				GLfloat screen_texture_pow2_sz[2] = { (GLfloat)surf_w_pow2, (GLfloat)surf_h_pow2 };
				uniform_location = pfn_glGetUniformLocationARB(sdl->glsl_program[i], "screen_texture_pow2_sz");
				pfn_glUniform2fvARB(uniform_location, 1, &(screen_texture_pow2_sz[0]));
				GL_CHECK_ERROR_NORMAL();
			}
		}
	}

	pfn_glUseProgramObjectARB(sdl->glsl_program[0]); // start with 1st shader

	if( sdl->glsl_program_num > 1 )
	{
		// multipass mode
		// GL_TEXTURE2/GL_TEXTURE3
		pfn_glGenFramebuffers(2, (GLuint *)&texture->mpass_fbo_mamebm[0]);
		glGenTextures(2, (GLuint *)&texture->mpass_texture_mamebm[0]);

		for (i=0; i<2; i++)
		{
			if ( texture_fbo_create(texture->mpass_textureunit[i], 
			                        texture->mpass_texture_mamebm[i],
						texture->mpass_fbo_mamebm[i],
						texture->rawwidth_create, texture->rawheight_create) )
			{
				return -1;
			}
		}

		pfn_glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);

		mame_printf_verbose("GL texture: mpass mame-bmp   2x %dx%d (pow2 %dx%d)\n",
			texture->rawwidth, texture->rawheight, texture->rawwidth_create, texture->rawheight_create);
	}

	if( sdl->glsl_program_num > 1 && sdl->glsl_program_mb2sc < sdl->glsl_program_num - 1 )
	{
		// multipass mode
		// GL_TEXTURE2/GL_TEXTURE3
		pfn_glGenFramebuffers(2, (GLuint *)&texture->mpass_fbo_scrn[0]);
		glGenTextures(2, (GLuint *)&texture->mpass_texture_scrn[0]);

		for (i=0; i<2; i++)
		{
			if ( texture_fbo_create(texture->mpass_textureunit[i], 
			                        texture->mpass_texture_scrn[i],
						texture->mpass_fbo_scrn[i],
						surf_w_pow2, surf_h_pow2) )
			{
				return -1;
			}
		}

		mame_printf_verbose("GL texture: mpass screen-bmp 2x %dx%d (pow2 %dx%d)\n",
			window->sdlsurf->w, window->sdlsurf->h, surf_w_pow2, surf_h_pow2);
	}

	if ( !(sdl->glsl_vid_attributes && texture->format!=SDL_TEXFORMAT_PALETTE16) )
	{
		// GL_TEXTURE1 
		glGenTextures(1, (GLuint *)&texture->lut_texture);
		pfn_glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, texture->lut_texture);

		glPixelStorei(GL_UNPACK_ROW_LENGTH, lut_table_width_pow2);

		{
			GLint _width, _height;
			if ( gl_texture_check_size(GL_TEXTURE_2D, 0, GL_RGBA8, lut_table_width_pow2, lut_table_height_pow2,
					  0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, &_width, &_height, 1) )
			{
				mame_printf_error("cannot create lut table texture, req: %dx%d, avail: %dx%d - bail out\n",
					lut_table_width_pow2, lut_table_height_pow2, (int)_width, (int)_height);
				return -1;
			}
		}

		{
			UINT32 * dummy = calloc(lut_table_width_pow2*lut_table_height_pow2, sizeof(UINT32)); // blank out the whole pal.
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, lut_table_width_pow2, lut_table_height_pow2,
				 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, dummy );
			glFinish(); // should not be necessary, .. but make sure we won't access the memory after free
			free(dummy);
		}

		glPixelStorei(GL_UNPACK_ROW_LENGTH, texture->lut_table_width);

		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texture->lut_table_width, texture->lut_table_height,
			     GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, texsource->palette );

		// non-screen textures will never be filtered
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

		assert ( texture->lut_texture );
	}

	// GL_TEXTURE0 
	// get a name for this texture
	glGenTextures(1, (GLuint *)&texture->texture);
	pfn_glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture->texture);

	glPixelStorei(GL_UNPACK_ROW_LENGTH, texture->rawwidth_create);

	if(texture->format!=SDL_TEXFORMAT_PALETTE16)
	{
		UINT32 * dummy = NULL;
		GLint _width, _height;
		if ( gl_texture_check_size(GL_TEXTURE_2D, 0, texture->texProperties[SDL_TEXFORMAT_INTERNAL],
					   texture->rawwidth_create, texture->rawheight_create, 
					   0,
					   texture->texProperties[SDL_TEXFORMAT_FORMAT], 
					   texture->texProperties[SDL_TEXFORMAT_TYPE], 
					   &_width, &_height, 1) )
		{
			mame_printf_error("cannot create bitmap texture, req: %dx%d, avail: %dx%d - bail out\n",
				texture->rawwidth_create, texture->rawheight_create, (int)_width, (int)_height);
			return -1;
		}

		dummy = calloc(texture->rawwidth_create * texture->rawheight_create, 
                               texture->texProperties[SDL_TEXFORMAT_PIXEL_SIZE]);
		glTexImage2D(GL_TEXTURE_2D, 0, texture->texProperties[SDL_TEXFORMAT_INTERNAL], 
		     texture->rawwidth_create, texture->rawheight_create, 
		     0,
		     texture->texProperties[SDL_TEXFORMAT_FORMAT], 
		     texture->texProperties[SDL_TEXFORMAT_TYPE], dummy);
                glFinish(); // should not be necessary, .. but make sure we won't access the memory after free
		free(dummy);

		if ((PRIMFLAG_GET_SCREENTEX(flags)) && video_config.filter)
		{
			assert( glsl_shader_feature == GLSL_SHADER_FEAT_PLAIN );

			// screen textures get the user's choice of filtering
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		else
		{
			// non-screen textures will never be filtered
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}

		// set wrapping mode appropriately
		if (texture->flags & PRIMFLAG_TEXWRAP_MASK)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		}
	} else {
		UINT32 * dummy = NULL;
		GLint _width, _height;
		if ( gl_texture_check_size(GL_TEXTURE_2D, 0, GL_ALPHA16, texture->rawwidth_create, texture->rawheight_create,
				  0, GL_ALPHA, GL_UNSIGNED_SHORT, &_width, &_height, 1) )
		{
			mame_printf_error("cannot create lut bitmap texture, req: %dx%d, avail: %dx%d - bail out\n",
				texture->rawwidth_create, texture->rawheight_create,
				(int)_width, (int)_height);
			return -1;
		}
		dummy = calloc(texture->rawwidth_create * texture->rawheight_create, sizeof(UINT16));
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA16,
		     texture->rawwidth_create, texture->rawheight_create, 
		     0,
		     GL_ALPHA, GL_UNSIGNED_SHORT, dummy);
                glFinish(); // should not be necessary, .. but make sure we won't access the memory after free
		free(dummy);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		if (texture->flags & PRIMFLAG_TEXWRAP_MASK)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		}
	}

	GL_CHECK_ERROR_NORMAL();

	return 0;
}

static texture_info *texture_create(sdl_info *sdl, sdl_window_info *window, const render_texinfo *texsource, UINT32 flags)
{
	texture_info *texture;
	
	// allocate a new texture
	texture = malloc(sizeof(*texture));
	memset(texture, 0, sizeof(*texture));

	// fill in the core data
	texture->hash = texture_compute_hash(texsource, flags);
	texture->flags = flags;
	texture->texinfo = *texsource;
	texture->texinfo.seqid = -1; // force set data
	if (PRIMFLAG_GET_SCREENTEX(flags))
	{
		texture->xprescale = video_config.prescale;
		texture->yprescale = video_config.prescale;
		texture->prescale_effect = video_config.prescale_effect;
	}
	else
	{
		texture->xprescale = 1;
		texture->yprescale = 1;
		texture->prescale_effect = 0;
	}

	// set the texture_format
        // 
        // src/emu/validity.c:validate_display() states, 
        // an emulated driver can only produce
        //      BITMAP_FORMAT_INDEXED16, BITMAP_FORMAT_RGB15 and BITMAP_FORMAT_RGB32
        // where only the first original paletted.
        //
        // other paletted formats, i.e.:
        //   SDL_TEXFORMAT_RGB32_PALETTED, SDL_TEXFORMAT_RGB15_PALETTED and SDL_TEXFORMAT_YUY16_PALETTED
        // add features like brightness etc by the mame core 
        //
        // all palette lookup may be implemented using shaders later on ..
        // that's why we keep the EQUAL flag TRUE, for all original true color bitmaps.
        //
	switch (PRIMFLAG_GET_TEXFORMAT(flags))
	{
		case TEXFORMAT_ARGB32:
			texture->format = SDL_TEXFORMAT_ARGB32;
			break;
		case TEXFORMAT_RGB32:
                        if (texsource->palette != NULL)
                                texture->format = SDL_TEXFORMAT_RGB32_PALETTED;
                        else
                                texture->format = SDL_TEXFORMAT_RGB32;
			break;
		case TEXFORMAT_PALETTE16:
			texture->format = SDL_TEXFORMAT_PALETTE16;
			break;
		case TEXFORMAT_RGB15:
                        if (texsource->palette != NULL)
                                texture->format = SDL_TEXFORMAT_RGB15_PALETTED;
                        else
                                texture->format = SDL_TEXFORMAT_RGB15;

			break;
		case TEXFORMAT_PALETTEA16:
			texture->format = SDL_TEXFORMAT_PALETTE16A;
			break;
		case TEXFORMAT_YUY16:
			if (texsource->palette != NULL)
				texture->format = SDL_TEXFORMAT_YUY16_PALETTED;
			else
				texture->format = SDL_TEXFORMAT_YUY16;
			break;

		default:
			mame_printf_error("Unknown textureformat %d\n", PRIMFLAG_GET_TEXFORMAT(flags));
	}

	// compute the size
	texture_compute_size_type(sdl, texsource, texture, flags);

        texture->pbo=0;

	if ( texture->type != TEXTURE_TYPE_SHADER && sdl->useglsl)
	{
		pfn_glUseProgramObjectARB(0); // back to fixed function pipeline
	}

	if ( texture->type==TEXTURE_TYPE_SHADER )
	{
		if ( texture_shader_create(sdl, window, texsource, texture, flags) )
                {
                        free(texture);
			return NULL;
                }
        } 
	else 
	{
		// get a name for this texture
		glGenTextures(1, (GLuint *)&texture->texture);

		glEnable(texture->texTarget);

                // make sure we're operating on *this* texture
                glBindTexture(texture->texTarget, texture->texture);

                // this doesn't actually upload, it just sets up the PBO's parameters
                glTexImage2D(texture->texTarget, 0, texture->texProperties[SDL_TEXFORMAT_INTERNAL], 
                     texture->rawwidth_create, texture->rawheight_create, 
                     texture->borderpix ? 1 : 0, 
                     texture->texProperties[SDL_TEXFORMAT_FORMAT], 
                     texture->texProperties[SDL_TEXFORMAT_TYPE], NULL);

		if ((PRIMFLAG_GET_SCREENTEX(flags)) && video_config.filter)
		{
			// screen textures get the user's choice of filtering
			glTexParameteri(texture->texTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(texture->texTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		else
		{
			// non-screen textures will never be filtered
			glTexParameteri(texture->texTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(texture->texTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}

		if( texture->texTarget==GL_TEXTURE_RECTANGLE_ARB )
		{
			// texture rectangles can't wrap
			glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);
		} else {
			// set wrapping mode appropriately
			if (texture->flags & PRIMFLAG_TEXWRAP_MASK)
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			}
			else
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
			}
		}
	}

	if ( texture->type == TEXTURE_TYPE_DYNAMIC )
	{
                assert(sdl->usepbo);

                // create the PBO
                pfn_glGenBuffers(1, (GLuint *)&texture->pbo);

                pfn_glBindBuffer( GL_PIXEL_UNPACK_BUFFER_ARB, texture->pbo);

                // set up the PBO dimension, ..
		pfn_glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, 
		                 texture->rawwidth * texture->rawheight * texture->texProperties[SDL_TEXFORMAT_PIXEL_SIZE],
				 NULL, GL_STREAM_DRAW);
        } 

        if ( !texture->nocopy && texture->type!=TEXTURE_TYPE_DYNAMIC )
        {
            texture->data = malloc(texture->rawwidth* texture->rawheight * texture->texProperties[SDL_TEXFORMAT_PIXEL_SIZE]);
            texture->data_own=TRUE;
        }

	if (texture->prescale_effect && !texture_copy_properties[texture->format][SDL_TEXFORMAT_SRC_EQUALS_DEST])
		texture->effectbuf = malloc_or_die(texsource->width * 3 * texture->texProperties[SDL_TEXFORMAT_PIXEL_SIZE]);

	// add us to the texture list
	texture->next = sdl->texlist;
	sdl->texlist = texture;

	if(sdl->usevbo)
	{
		// Generate And Bind The Texture Coordinate Buffer
		pfn_glGenBuffers( 1, &(texture->texCoordBufferName) );
		pfn_glBindBuffer( GL_ARRAY_BUFFER_ARB, texture->texCoordBufferName );
		// Load The Data
		pfn_glBufferData( GL_ARRAY_BUFFER_ARB, 4*2*sizeof(GLfloat), texture->texCoord, GL_STREAM_DRAW );
		glTexCoordPointer( 2, GL_FLOAT, 0, (char *) NULL ); // we are using ARB VBO buffers
        } else {
		glTexCoordPointer(2, GL_FLOAT, 0, texture->texCoord);
	}

	return texture;
}

//============================================================
//  texture_set_data
//============================================================

static void texture_set_data(sdl_info *sdl, texture_info *texture, const render_texinfo *texsource, UINT32 flags)
{
	if ( texture->type == TEXTURE_TYPE_DYNAMIC )
	{
		assert(texture->pbo);
		assert(!texture->nocopy);

		texture->data = pfn_glMapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY);
        }

	// note that nocopy and borderpix are mutually exclusive, IOW
	// they cannot be both true, thus this cannot lead to the
        // borderpix code below writing to texsource->base .
        if (texture->nocopy)
                texture->data = texsource->base;

	// always fill non-wrapping textures with an extra pixel on the top
	if (texture->borderpix)
        {
		memset(texture->data, 0, 
                       (texsource->width * texture->xprescale + 2) * texture->texProperties[SDL_TEXFORMAT_PIXEL_SIZE]);
        }

	// when nescesarry copy (and convert) the data
	if (!texture->nocopy)
        {
                assert(texture->texCopyFn);
                texture->texCopyFn(texture, texsource);
        }

	// always fill non-wrapping textures with an extra pixel on the bottom
	if (texture->borderpix)
	{
		memset((UINT8 *)texture->data + 
		       (texsource->height + 1) * texture->rawwidth * texture->texProperties[SDL_TEXFORMAT_PIXEL_SIZE], 
		       0,
			(texsource->width * texture->xprescale + 2) * texture->texProperties[SDL_TEXFORMAT_PIXEL_SIZE]);
	}

	if ( texture->type == TEXTURE_TYPE_SHADER )
	{
		if ( texture->lut_texture )
		{
			pfn_glActiveTexture(GL_TEXTURE1);
			glBindTexture(texture->texTarget, texture->lut_texture);

			glPixelStorei(GL_UNPACK_ROW_LENGTH, texture->lut_table_width);

			// give the card a hint
			glTexSubImage2D(texture->texTarget, 0, 0, 0, texture->lut_table_width, texture->lut_table_height,
				     GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, texsource->palette );
		}
		pfn_glActiveTexture(GL_TEXTURE0);
		glBindTexture(texture->texTarget, texture->texture);

		glPixelStorei(GL_UNPACK_ROW_LENGTH, texture->texinfo.rowpixels);

		// and upload the image 
		if(texture->format!=SDL_TEXFORMAT_PALETTE16)
		{
			glTexSubImage2D(texture->texTarget, 0, 0, 0, texture->rawwidth, texture->rawheight, 
					texture->texProperties[SDL_TEXFORMAT_FORMAT], 
					texture->texProperties[SDL_TEXFORMAT_TYPE], texture->data);
		} else {
			glTexSubImage2D(texture->texTarget, 0, 0, 0, texture->rawwidth, texture->rawheight, 
					GL_ALPHA, GL_UNSIGNED_SHORT, texture->data);
		}
	} else if ( texture->type == TEXTURE_TYPE_DYNAMIC )
	{
		glBindTexture(texture->texTarget, texture->texture);

		glPixelStorei(GL_UNPACK_ROW_LENGTH, texture->rawwidth);

		// unmap the buffer from the CPU space so it can DMA
		pfn_glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB);

		// kick off the DMA
		glTexSubImage2D(texture->texTarget, 0, 0, 0, texture->rawwidth, texture->rawheight, 
			        texture->texProperties[SDL_TEXFORMAT_FORMAT], 
				texture->texProperties[SDL_TEXFORMAT_TYPE], NULL);
	} else {
		glBindTexture(texture->texTarget, texture->texture);

		// give the card a hint
		if (texture->nocopy)
			glPixelStorei(GL_UNPACK_ROW_LENGTH, texture->texinfo.rowpixels);
		else
			glPixelStorei(GL_UNPACK_ROW_LENGTH, texture->rawwidth);

                // and upload the image 
                glTexSubImage2D(texture->texTarget, 0, 0, 0, texture->rawwidth, texture->rawheight, 
		                texture->texProperties[SDL_TEXFORMAT_FORMAT], 
				texture->texProperties[SDL_TEXFORMAT_TYPE], texture->data);
	}
}

//============================================================
//  texture_find
//============================================================

static texture_info *texture_find(sdl_info *sdl, const render_primitive *prim)
{
	HashT texhash = texture_compute_hash(&prim->texture, prim->flags);
	texture_info *texture;

	// find a match
	for (texture = sdl->texlist; texture != NULL; texture = texture->next)
		if (texture->hash == texhash &&
			texture->texinfo.base == prim->texture.base &&
			texture->texinfo.width == prim->texture.width &&
			texture->texinfo.height == prim->texture.height &&
			texture->texinfo.rowpixels == prim->texture.rowpixels &&
			((texture->flags ^ prim->flags) & (PRIMFLAG_BLENDMODE_MASK | PRIMFLAG_TEXFORMAT_MASK)) == 0)
			return texture;

	// nothing found
	return NULL;
}



//============================================================
//  texture_update
//============================================================

static void texture_coord_update(sdl_info *sdl, sdl_window_info *window, 
                                 texture_info *texture, const render_primitive *prim, int shaderIdx)
{
	float ustart, ustop;			// beginning/ending U coordinates
	float vstart, vstop;			// beginning/ending V coordinates
	float du, dv;

	if ( texture->type != TEXTURE_TYPE_SHADER ||
	     ( texture->type == TEXTURE_TYPE_SHADER && shaderIdx<=sdl->glsl_program_mb2sc ) )
        {
		// compute the U/V scale factors
		if (texture->borderpix)
		{
			int unscaledwidth = (texture->rawwidth_create-2) / texture->xprescale + 2;
			int unscaledheight = (texture->rawheight_create-2) / texture->yprescale + 2;
			ustart = 1.0f / (float)(unscaledwidth);
			ustop = (float)(prim->texture.width + 1) / (float)(unscaledwidth);
			vstart = 1.0f / (float)(unscaledheight);
			vstop = (float)(prim->texture.height + 1) / (float)(unscaledheight);
		}
		else
		{
			ustart = 0.0f;
			ustop  = (float)(prim->texture.width*texture->xprescale) / (float)texture->rawwidth_create;
			vstart = 0.0f;
			vstop  = (float)(prim->texture.height*texture->yprescale) / (float)texture->rawheight_create;
		}
	} else if ( texture->type == TEXTURE_TYPE_SHADER && shaderIdx>sdl->glsl_program_mb2sc )
	{
		int surf_w_pow2  = get_valid_pow2_value (window->sdlsurf->w, texture->texpow2);
		int surf_h_pow2  = get_valid_pow2_value (window->sdlsurf->h, texture->texpow2);

		ustart = 0.0f;
		ustop  = (float)(window->sdlsurf->w) / (float)surf_w_pow2;
		vstart = 0.0f;
		vstop  = (float)(window->sdlsurf->h) / (float)surf_h_pow2;
	} else {
		assert(0); // ??
	}

	du = ustop - ustart;
	dv = vstop - vstart;

	if ( texture->texTarget == GL_TEXTURE_RECTANGLE_ARB )
	{
	    // texture coordinates for TEXTURE_RECTANGLE are 0,0 -> w,h
	    // rather than 0,0 -> 1,1 as with normal OpenGL texturing
	    du *= (float)texture->rawwidth;
	    dv *= (float)texture->rawheight;
	}

	if ( texture->type == TEXTURE_TYPE_SHADER && shaderIdx!=sdl->glsl_program_mb2sc )
	{
		// 1:1 tex coord CCW (0/0) (1/0) (1/1) (0/1)
		// we must go CW here due to the mame bitmap order
		texture->texCoord[0]=ustart + du * 0.0;
		texture->texCoord[1]=vstart + dv * 1.0;
		texture->texCoord[2]=ustart + du * 1.0;
		texture->texCoord[3]=vstart + dv * 1.0;
		texture->texCoord[4]=ustart + du * 1.0;
		texture->texCoord[5]=vstart + dv * 0.0;
		texture->texCoord[6]=ustart + du * 0.0;
		texture->texCoord[7]=vstart + dv * 0.0;
	} else {
		// transformation: mamebm -> scrn
		texture->texCoord[0]=ustart + du * prim->texcoords.tl.u;
		texture->texCoord[1]=vstart + dv * prim->texcoords.tl.v;
		texture->texCoord[2]=ustart + du * prim->texcoords.tr.u;
		texture->texCoord[3]=vstart + dv * prim->texcoords.tr.v;
		texture->texCoord[4]=ustart + du * prim->texcoords.br.u;
		texture->texCoord[5]=vstart + dv * prim->texcoords.br.v;
		texture->texCoord[6]=ustart + du * prim->texcoords.bl.u;
		texture->texCoord[7]=vstart + dv * prim->texcoords.bl.v;
	}
}

static void texture_mpass_flip(sdl_info *sdl, sdl_window_info *window, texture_info *texture, int shaderIdx)
{
	UINT32 mpass_src_idx = texture->mpass_dest_idx;

	texture->mpass_dest_idx = (mpass_src_idx+1) % 2;

	if ( shaderIdx>0 )
	{
		int uniform_location;
		uniform_location = pfn_glGetUniformLocationARB(sdl->glsl_program[shaderIdx], "mpass_texture");
		pfn_glUniform1iARB(uniform_location, texture->mpass_textureunit[mpass_src_idx]-GL_TEXTURE0);
		GL_CHECK_ERROR_NORMAL();
	}

	pfn_glActiveTexture(texture->mpass_textureunit[mpass_src_idx]);
	if ( shaderIdx<=sdl->glsl_program_mb2sc )
	{
		glBindTexture(texture->texTarget, texture->mpass_texture_mamebm[mpass_src_idx]);
	} else {
		glBindTexture(texture->texTarget, texture->mpass_texture_scrn[mpass_src_idx]);
	}
	pfn_glActiveTexture(texture->mpass_textureunit[texture->mpass_dest_idx]);
	glBindTexture(texture->texTarget, 0);

	pfn_glActiveTexture(texture->mpass_textureunit[texture->mpass_dest_idx]);

	if ( shaderIdx<sdl->glsl_program_num-1 )
	{
		if ( shaderIdx>=sdl->glsl_program_mb2sc )
		{
			glBindTexture(texture->texTarget, texture->mpass_texture_scrn[texture->mpass_dest_idx]);
			pfn_glBindFramebuffer(GL_FRAMEBUFFER_EXT, texture->mpass_fbo_scrn[texture->mpass_dest_idx]);
		} else {
			glBindTexture(texture->texTarget, texture->mpass_texture_mamebm[texture->mpass_dest_idx]);
			pfn_glBindFramebuffer(GL_FRAMEBUFFER_EXT, texture->mpass_fbo_mamebm[texture->mpass_dest_idx]);
		}

		if ( shaderIdx==0 )
		{
			glPushAttrib(GL_VIEWPORT_BIT); 
			GL_CHECK_ERROR_NORMAL();
			glViewport(0.0, 0.0, (GLsizei)texture->rawwidth, (GLsizei)texture->rawheight);
		} else if ( shaderIdx==sdl->glsl_program_mb2sc )
		{
			assert ( sdl->glsl_program_mb2sc < sdl->glsl_program_num-1 );
			glPopAttrib(); // glViewport(0.0, 0.0, (GLsizei)window->sdlsurf->w, (GLsizei)window->sdlsurf->h)
			GL_CHECK_ERROR_NORMAL();
		}
		glClear(GL_COLOR_BUFFER_BIT); // make sure the whole texture is redrawn ..
	} else {
		glBindTexture(texture->texTarget, 0);
		pfn_glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);

		if ( sdl->glsl_program_mb2sc == sdl->glsl_program_num-1 )
		{
			glPopAttrib(); // glViewport(0.0, 0.0, (GLsizei)window->sdlsurf->w, (GLsizei)window->sdlsurf->h)
			GL_CHECK_ERROR_NORMAL();
		}

		pfn_glActiveTexture(GL_TEXTURE0);
		glBindTexture(texture->texTarget, 0);
	}
}

static void texture_shader_update(sdl_info *sdl, sdl_window_info *window, texture_info *texture, int shaderIdx)
{
	if ( !texture->lut_texture )
	{
		int uniform_location;
		render_container *container = render_container_get_screen(window->start_viewscreen);
		GLfloat vid_attributes[4]; // gamma, contrast, brightness, effect

		assert ( sdl->glsl_vid_attributes && texture->format!=SDL_TEXFORMAT_PALETTE16 );

		if (container!=NULL)
		{
			vid_attributes[0] = render_container_get_gamma(container);
			vid_attributes[1] = render_container_get_contrast(container);
			vid_attributes[2] = render_container_get_brightness(container);
			vid_attributes[3] = 0.0f;
			uniform_location = pfn_glGetUniformLocationARB(sdl->glsl_program[shaderIdx], "vid_attributes");
			pfn_glUniform4fvARB(uniform_location, 1, &(vid_attributes[shaderIdx]));
			if ( GL_CHECK_ERROR_QUIET() ) {
				mame_printf_verbose("GLSL: could not set 'vid_attributes' for shader prog idx %d\n", shaderIdx);
			}
		} else {
			mame_printf_verbose("GLSL: could not get render container for screen %d\n", window->start_viewscreen);
		}
	}
}

static texture_info * texture_update(sdl_info *sdl, sdl_window_info *window, const render_primitive *prim, int shaderIdx)
{
	texture_info *texture = texture_find(sdl, prim);
	int texBound = 0;

	// if we didn't find one, create a new texture
	if (texture == NULL && prim->texture.base != NULL)
        {
                texture = texture_create(sdl, window, &prim->texture, prim->flags); 

        } else if (texture != NULL)
        {
		if ( texture->type == TEXTURE_TYPE_SHADER )
		{
			pfn_glUseProgramObjectARB(sdl->glsl_program[shaderIdx]); // back to our shader
                } else if ( texture->type == TEXTURE_TYPE_DYNAMIC )
                {
		        assert ( sdl->usepbo ) ;
                        pfn_glBindBuffer( GL_PIXEL_UNPACK_BUFFER_ARB, texture->pbo); 
                        glEnable(texture->texTarget);
		} else { 
                        glEnable(texture->texTarget);
                }
        }

        if (texture != NULL)
	{
		if ( texture->type == TEXTURE_TYPE_SHADER )
		{
			texture_shader_update(sdl, window, texture, shaderIdx);
			if ( sdl->glsl_program_num>1 )
			{
				texture_mpass_flip(sdl, window, texture, shaderIdx);
			}
		}

		if ( shaderIdx==0 ) // redundant for subsequent multipass shader
		{
			if (prim->texture.base != NULL && texture->texinfo.seqid != prim->texture.seqid)
			{
				texture->texinfo.seqid = prim->texture.seqid;

				// if we found it, but with a different seqid, copy the data
				texture_set_data(sdl, texture, &prim->texture, prim->flags);
				texBound=1;
			}
		} 

		if (!texBound) {
			glBindTexture(texture->texTarget, texture->texture);
		}
		texture_coord_update(sdl, window, texture, prim, shaderIdx);

		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		if(sdl->usevbo)
		{
		    pfn_glBindBuffer( GL_ARRAY_BUFFER_ARB, texture->texCoordBufferName );
		    // Load The Data
		    pfn_glBufferSubData( GL_ARRAY_BUFFER_ARB, 0, 4*2*sizeof(GLfloat), texture->texCoord );
		    glTexCoordPointer( 2, GL_FLOAT, 0, (char *) NULL ); // we are using ARB VBO buffers 
		} else {
		    glTexCoordPointer(2, GL_FLOAT, 0, texture->texCoord);
		}
	} 

        return texture;
}

static void texture_disable(sdl_info *sdl, texture_info * texture)
{
        if ( texture->type == TEXTURE_TYPE_SHADER )
        {
                assert ( sdl->useglsl );
		pfn_glUseProgramObjectARB(0); // back to fixed function pipeline
        } else if ( texture->type == TEXTURE_TYPE_DYNAMIC ) 
        {
                pfn_glBindBuffer( GL_PIXEL_UNPACK_BUFFER_ARB, 0);
                glDisable(texture->texTarget);
        } else {
                glDisable(texture->texTarget);
        }
}

static void texture_all_disable(sdl_info *sdl)
{
	if ( sdl->useglsl )
        {
		pfn_glUseProgramObjectARB(0); // back to fixed function pipeline

		pfn_glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, 0);
		if ( sdl->usefbo ) pfn_glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
		pfn_glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, 0);
		if ( sdl->usefbo ) pfn_glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
		pfn_glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, 0);
		if ( sdl->usefbo ) pfn_glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
		pfn_glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
		if ( sdl->usefbo ) pfn_glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
        }
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

        if(sdl->usetexturerect)
        {
                glDisable(GL_TEXTURE_RECTANGLE_ARB);
        }
        glDisable(GL_TEXTURE_2D);

        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        if(sdl->usevbo)
        {
		pfn_glBindBuffer( GL_ARRAY_BUFFER_ARB, 0); // unbind ..
        }
	if ( sdl->usepbo )
	{
		pfn_glBindBuffer( GL_PIXEL_UNPACK_BUFFER_ARB, 0);
        }
}

void drawogl_destroy_all_textures(sdl_window_info *window)
{
        int lock=FALSE;

	if ( ! window->initialized )
	{
		return;
	}

        if(window->primlist && window->primlist->lock) { lock=TRUE; osd_lock_acquire(window->primlist->lock); }
        {
                texture_info *next_texture=NULL, *texture = NULL;
                sdl_info *sdl = window->dxdata;

                if (sdl == NULL)
                        goto exit;

                texture = sdl->texlist;

                glFinish();

                texture_all_disable(sdl);

                glFinish();

                glDisableClientState(GL_VERTEX_ARRAY);

                while (texture)
                {
                        next_texture = texture->next;

                        if(sdl->usevbo)
                        {
                            pfn_glDeleteBuffers( 1, &(texture->texCoordBufferName) );
                            texture->texCoordBufferName=0;
                        }

                        if(sdl->usepbo && texture->pbo)
                        {
                            pfn_glDeleteBuffers( 1, (GLuint *)&(texture->pbo) );
                            texture->pbo=0;
                        }

			if( sdl->glsl_program_num > 1 )
			{
				assert(sdl->usefbo);
				pfn_glDeleteFramebuffers(2, (GLuint *)&texture->mpass_fbo_mamebm[0]);
				glDeleteTextures(2, (GLuint *)&texture->mpass_texture_mamebm[0]);
			}

			if ( sdl->glsl_program_mb2sc < sdl->glsl_program_num - 1 )
			{
				assert(sdl->usefbo);
				pfn_glDeleteFramebuffers(2, (GLuint *)&texture->mpass_fbo_scrn[0]);
				glDeleteTextures(2, (GLuint *)&texture->mpass_texture_scrn[0]);
			}

			if(texture->lut_texture)
				glDeleteTextures(1, (GLuint *)&texture->lut_texture);

                        glDeleteTextures(1, (GLuint *)&texture->texture);
                        if ( texture->data_own )
                        {
                                free(texture->data);
                                texture->data=NULL;
                                texture->data_own=FALSE;
                        }
                        if (texture->effectbuf)
                        {
                                free(texture->effectbuf);
                        }
                        free(texture);
                        texture = next_texture;
                }
                sdl->texlist = NULL;

		if ( sdl->useglsl )
		{
			glsl_shader_free(sdl); 
		}

		window->initialized = 0;
        }
exit:
        if(lock) { osd_lock_release(window->primlist->lock); }
}
#endif

//============================================================
//  TEXCOPY FUNCS
//============================================================

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
//  SOFTWARE RENDERING
//============================================================

#define FUNC_PREFIX(x)		drawsdl_rgb888_##x
#define PIXEL_TYPE			UINT32
#define SRCSHIFT_R			0
#define SRCSHIFT_G			0
#define SRCSHIFT_B			0
#define DSTSHIFT_R			16
#define DSTSHIFT_G			8
#define DSTSHIFT_B			0

#include "rendersw.c"

#define FUNC_PREFIX(x)		drawsdl_bgr888_##x
#define PIXEL_TYPE			UINT32
#define SRCSHIFT_R			0
#define SRCSHIFT_G			0
#define SRCSHIFT_B			0
#define DSTSHIFT_R			0
#define DSTSHIFT_G			8
#define DSTSHIFT_B			16

#include "rendersw.c"

#define FUNC_PREFIX(x)		drawsdl_rgb565_##x
#define PIXEL_TYPE			UINT16
#define SRCSHIFT_R			3
#define SRCSHIFT_G			2
#define SRCSHIFT_B			3
#define DSTSHIFT_R			11
#define DSTSHIFT_G			5
#define DSTSHIFT_B			0

#include "rendersw.c"

#define FUNC_PREFIX(x)		drawsdl_rgb555_##x
#define PIXEL_TYPE			UINT16
#define SRCSHIFT_R			3
#define SRCSHIFT_G			3
#define SRCSHIFT_B			3
#define DSTSHIFT_R			10
#define DSTSHIFT_G			5
#define DSTSHIFT_B			0

#include "rendersw.c"

//============================================================
//  MANUAL TEXCOPY FUNCS 
//  (YUY format is weird and doesn't fit the assumptions of the
//   standard macros so we handle it here
//     
//  NOTE: must be after the rendersw.c includes to get ycc_to_rgb
//============================================================

void texcopy_yuv16(texture_info *texture, const render_texinfo *texsource)
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

void texcopy_yuv16_paletted(texture_info *texture, const render_texinfo *texsource)
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

//============================================================
// YUV Blitting
//============================================================

#define CU_CLAMP(v, a, b)	((v < a)? a: ((v > b)? b: v))
#define RGB2YUV_F(r,g,b,y,u,v) \
	 (y) = (0.299*(r) + 0.587*(g) + 0.114*(b) ); \
	 (u) = (-0.169*(r) - 0.331*(g) + 0.5*(b) + 128); \
	 (v) = (0.5*(r) - 0.419*(g) - 0.081*(b) + 128); \
	(y) = CU_CLAMP(y,0,255); \
	(u) = CU_CLAMP(u,0,255); \
	(v) = CU_CLAMP(v,0,255)

#define RGB2YUV(r,g,b,y,u,v) \
	 (y) = ((  8453*(r) + 16594*(g) +  3223*(b) +  524288) >> 15); \
	 (u) = (( -4878*(r) -  9578*(g) + 14456*(b) + 4210688) >> 15); \
	 (v) = (( 14456*(r) - 12105*(g) -  2351*(b) + 4210688) >> 15)

#ifdef LSB_FIRST	 
#define Y1MASK  0x000000FF
#define  UMASK  0x0000FF00
#define Y2MASK  0x00FF0000
#define  VMASK  0xFF000000
#define Y1SHIFT  0
#define  USHIFT  8
#define Y2SHIFT 16
#define  VSHIFT 24
#else
#define Y1MASK  0xFF000000
#define  UMASK  0x00FF0000
#define Y2MASK  0x0000FF00
#define  VMASK  0x000000FF
#define Y1SHIFT 24
#define  USHIFT 16
#define Y2SHIFT  8
#define  VSHIFT  0
#endif

#define YMASK  (Y1MASK|Y2MASK)
#define UVMASK (UMASK|VMASK)

static void yuv_lookup_set(sdl_window_info *window, unsigned int pen, unsigned char red, 
			unsigned char green, unsigned char blue)
{
	UINT32 y,u,v;

	RGB2YUV(red,green,blue,y,u,v);

	/* Storing this data in YUYV order simplifies using the data for
	   YUY2, both with and without smoothing... */
	window->yuv_lookup[pen]=(y<<Y1SHIFT)|(u<<USHIFT)|(y<<Y2SHIFT)|(v<<VSHIFT);
}

void drawsdl_yuv_init(sdl_window_info *window)
{
	unsigned char r,g,b;
	if (window->yuv_lookup == NULL)
		window->yuv_lookup = malloc_or_die(65536*sizeof(UINT32));
	for (r = 0; r < 32; r++)
		for (g = 0; g < 32; g++)
			for (b = 0; b < 32; b++)
			{
				int idx = (r << 10) | (g << 5) | b;
				yuv_lookup_set(window, idx, 
					(r << 3) | (r >> 2),
					(g << 3) | (g >> 2),
					(b << 3) | (b >> 2));
			}
}

static void yuv_RGB_to_YV12(UINT16 *bitmap, int bw, sdl_window_info *window)
{
	int x, y;
	UINT8 *dest_y;
	UINT8 *dest_u;
	UINT8 *dest_v;
	UINT16 *src; 
	UINT16 *src2;
	UINT32 *lookup = window->yuv_lookup;
	int yuv_pitch = window->yuvsurf->pitches[0];
	int u1,v1,y1,u2,v2,y2,u3,v3,y3,u4,v4,y4;	  /* 12 */
     
	for(y=0;y<window->yuv_ovl_height;y+=2)
	{
		src=bitmap + (y * bw) ;
		src2=src + bw;

		dest_y = window->yuvsurf->pixels[0] + y * window->yuvsurf->pitches[0];
		dest_v = window->yuvsurf->pixels[1] + (y>>1) * window->yuvsurf->pitches[1];
		dest_u = window->yuvsurf->pixels[2] + (y>>1) * window->yuvsurf->pitches[2];

		for(x=0;x<window->yuv_ovl_width;x+=2)
		{
			v1 = lookup[src[x]];
			y1 = (v1>>Y1SHIFT) & 0xff;
			u1 = (v1>>USHIFT)  & 0xff;
			v1 = (v1>>VSHIFT)  & 0xff;

			v2 = lookup[src[x+1]];
			y2 = (v2>>Y1SHIFT) & 0xff;
			u2 = (v2>>USHIFT)  & 0xff;
			v2 = (v2>>VSHIFT)  & 0xff;

			v3 = lookup[src2[x]];
			y3 = (v3>>Y1SHIFT) & 0xff;
			u3 = (v3>>USHIFT)  & 0xff;
			v3 = (v3>>VSHIFT)  & 0xff;

			v4 = lookup[src2[x+1]];
			y4 = (v4>>Y1SHIFT) & 0xff;
			u4 = (v4>>USHIFT)  & 0xff;
			v4 = (v4>>VSHIFT)  & 0xff;

			dest_y[x] = y1;
			dest_y[x+yuv_pitch] = y3;
			dest_y[x+1] = y2;
			dest_y[x+yuv_pitch+1] = y4;

			dest_u[x>>1] = (u1+u2+u3+u4)/4;
			dest_v[x>>1] = (v1+v2+v3+v4)/4;
			
		}
	}
}

static void yuv_RGB_to_YV12X2(UINT16 *bitmap, int bw, sdl_window_info *window)
{		/* this one is used when scale==2 */
	unsigned int x,y;
	UINT16 *dest_y;
	UINT8 *dest_u;
	UINT8 *dest_v;
	UINT16 *src;
	int yuv_pitch = window->yuvsurf->pitches[0];
	int u1,v1,y1;

	for(y=0;y<window->yuv_ovl_height;y++)
	{
		src=bitmap + (y * bw) ;

		dest_y = (UINT16 *)(window->yuvsurf->pixels[0] + 2*y * window->yuvsurf->pitches[0]);
		dest_v = window->yuvsurf->pixels[1] + y * window->yuvsurf->pitches[1];
		dest_u = window->yuvsurf->pixels[2] + y * window->yuvsurf->pitches[2];
		for(x=0;x<window->yuv_ovl_width;x++)
		{
			v1 = window->yuv_lookup[src[x]];
			y1 = (v1>>Y1SHIFT) & 0xff;
			u1 = (v1>>USHIFT)  & 0xff;
			v1 = (v1>>VSHIFT)  & 0xff;

			dest_y[x+yuv_pitch/2]=y1<<8|y1;
			dest_y[x]=y1<<8|y1;
			dest_u[x] = u1;
			dest_v[x] = v1;
		}
	}
}

static void yuv_RGB_to_YUY2(UINT16 *bitmap, int bw, sdl_window_info *window)
{		/* this one is used when scale==2 */
	unsigned int y;
	UINT32 *dest;
	UINT16 *src;
	UINT16 *end;
	UINT32 p1,p2,uv;
	UINT32 *lookup = window->yuv_lookup;
	int yuv_pitch = window->yuvsurf->pitches[0]/4;

	for(y=0;y<window->yuv_ovl_height;y++)
	{
		src=bitmap + (y * bw) ;
		end=src+window->yuv_ovl_width;

		dest = (UINT32 *) window->yuvsurf->pixels[0];
		dest += y * yuv_pitch;
		for(; src<end; src+=2)
		{
			p1  = lookup[src[0]];
			p2  = lookup[src[1]];
			uv = (p1&UVMASK)>>1;
			uv += (p2&UVMASK)>>1;
			*dest++ = (p1&Y1MASK)|(p2&Y2MASK)|(uv&UVMASK);
		}
	}
}

static void yuv_RGB_to_YUY2X2(UINT16 *bitmap, int bw, sdl_window_info *window)
{		/* this one is used when scale==2 */
	unsigned int y;
	UINT32 *dest;
	UINT16 *src;
	UINT16 *end;
	UINT32 *lookup = window->yuv_lookup;
	int yuv_pitch = window->yuvsurf->pitches[0]/4;

	for(y=0;y<window->yuv_ovl_height;y++)
	{
		src=bitmap + (y * bw) ;
		end=src+window->yuv_ovl_width;

		dest = (UINT32 *) window->yuvsurf->pixels[0];
		dest += (y * yuv_pitch);
		for(; src<end; src++)
		{
			dest[0] = lookup[src[0]];
			dest++;
		}
	}
}
