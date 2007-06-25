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

// standard SDL headers
#include <SDL/SDL.h>

#if USE_OPENGL

// OpenGL headers
#include <SDL/SDL_opengl.h>

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
typedef GLvoid* (APIENTRYP PFNGLMAPBUFFERPROC) (GLenum, GLenum);
typedef GLboolean (APIENTRYP PFNGLUNMAPBUFFERPROC) (GLenum);
typedef void (APIENTRYP PFNGLDELETEBUFFERSPROC) (GLsizei, const GLuint *);
typedef void (APIENTRYP PFNGLACTIVETEXTUREPROC) (GLenum texture);
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

#ifndef GL_STATIC_DRAW_ARB
#define GL_STATIC_DRAW_ARB                0x88E4
#endif

#ifndef GL_PIXEL_UNPACK_BUFFER_ARB
#define GL_PIXEL_UNPACK_BUFFER_ARB        0x88EC
#endif

#endif // OPENGL

// standard C headers
#include <math.h>
#include <stdio.h>

// MAME headers
#include "render.h"
#include "options.h"

// OSD headers
#include "window.h"
#include "effect.h"

//============================================================
//  DEBUGGING
//============================================================

#define DEBUG_MODE_SCORES	0

//#define DEBUG_VERBOSE   1

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

#if USE_OPENGL

static int drawogl_window_init(sdl_window_info *window);
static int drawogl_window_draw(sdl_window_info *window, UINT32 dc, int update);

// VBO
static PFNGLGENBUFFERSPROC pfn_glGenBuffers       = NULL;
static PFNGLDELETEBUFFERSPROC pfn_glDeleteBuffers = NULL;
static PFNGLBINDBUFFERPROC pfn_glBindBuffer       = NULL;
static PFNGLBUFFERDATAPROC pfn_glBufferData       = NULL;
// PBO
static PFNGLMAPBUFFERPROC     pfn_glMapBuffer        = NULL;
static PFNGLUNMAPBUFFERPROC   pfn_glUnmapBuffer      = NULL;
static PFNGLACTIVETEXTUREPROC pfn_glActiveTexture	= NULL;

static int glsl_shader_feature = GLSL_SHADER_FEAT_PLAIN;

#endif // USE_OPENGL

// drawing helpers
void compute_blit_surface_size(sdl_window_info *window, int window_width, int window_height);

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
static texture_info *texture_create(sdl_info *sdl, const render_texinfo *texsource, UINT32 flags);
static void texture_set_data(sdl_info *sdl, texture_info *texture, const render_texinfo *texsource, UINT32 flags);
static texture_info *texture_find(sdl_info *sdl, const render_primitive *prim);
static texture_info * texture_update(sdl_info *sdl, const render_primitive *prim);
void texture_disable(sdl_info *sdl, texture_info * texture);
void drawsdl_destroy_all_textures(sdl_window_info *window);
#endif

// YUV overlays

void yuv_lookup_init(sdl_window_info *window);
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

	#if SDL_MULTIMON
	printf("Using SDL multi-window soft driver (SDL 1.3+)\n");
	#else
	printf("Using SDL single-window soft driver (SDL 1.2)\n");
	#endif
	return 0;
}

int drawogl_init(sdl_draw_callbacks *callbacks)
{
#if USE_OPENGL
	// fill in the callbacks
	callbacks->exit = drawsdl_exit;
	callbacks->window_init = drawogl_window_init;
	callbacks->window_get_primitives = drawsdl_window_get_primitives;
	callbacks->window_draw = drawogl_window_draw;
	callbacks->window_destroy = drawsdl_window_destroy;

	#if SDL_MULTIMON
	printf("Using SDL multi-window OpenGL driver (SDL 1.3+)\n");
	#else
	printf("Using SDL single-window OpenGL driver (SDL 1.2)\n");
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
	sdl->viewscreen  = render_target_get_view(window->target);

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
#if USE_OPENGL
	if (video_config.mode == VIDEO_MODE_OPENGL)
	{
		drawsdl_destroy_all_textures(window);
	}
#endif
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
		compute_blit_surface_size(window, window->sdlsurf->w, window->sdlsurf->h);
	}
	else
	{
		compute_blit_surface_size(window, window->monitor->center_width, window->monitor->center_height);
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

static void clear_surface(sdl_window_info *window)
{
	if (SDL_MUSTLOCK(window->sdlsurf)) SDL_LockSurface(window->sdlsurf);

	memset(window->sdlsurf->pixels, 0, window->sdlsurf->h * window->sdlsurf->pitch);
	if (SDL_MUSTLOCK(window->sdlsurf)) SDL_UnlockSurface(window->sdlsurf);

	SDL_Flip(window->sdlsurf);
}

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

//	printf("drawsdl_window_draw\n");

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
				fprintf(stderr, "SDL: ERROR! Unknown video mode: R=%08X G=%08X B=%08X\n", window->sdlsurf->format->Rmask, window->sdlsurf->format->Gmask, window->sdlsurf->format->Bmask);
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
	// sdl->usevbo=FALSE; // You may want to switch VBO and PBO off, by uncommenting this statement
	// sdl->usepbo=FALSE; // You may want to switch PBO off, by uncommenting this statement
	// sdl->useglsl=FALSE; // You may want to switch GLSL off, by uncommenting this statement

	if (! sdl->usevbo)
	{
		if(sdl->usepbo) // should never ever happen ;-)
		{
			printf("OpenGL: PBO not supported, no VBO support. (sdlmame error)\n");
			sdl->usepbo=FALSE;
		}
		if(sdl->useglsl) // should never ever happen ;-)
		{
			printf("OpenGL: GLSL not supported, no VBO support. (sdlmame error)\n");
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
	}
	// PBO:
	if ( sdl->usepbo )
	{
		pfn_glMapBuffer  = (PFNGLMAPBUFFERPROC) SDL_GL_GetProcAddress("glMapBuffer");
		pfn_glUnmapBuffer= (PFNGLUNMAPBUFFERPROC) SDL_GL_GetProcAddress("glUnmapBuffer");
	}

	if ( sdl->usevbo &&
	        ( !pfn_glGenBuffers || !pfn_glDeleteBuffers ||
	        !pfn_glBindBuffer || !pfn_glBufferData
	 ) )
	{
		sdl->usepbo=FALSE;
		printf("OpenGL: VBO not supported, missing: ");
		if (!pfn_glGenBuffers)
		{
		        printf("glGenBuffers, ");
		}
		if (!pfn_glDeleteBuffers)
		{
		        printf("glDeleteBuffers");
		}
		if (!pfn_glBindBuffer)
		{
		        printf("glBindBuffer, ");
		}
		if (!pfn_glBufferData)
		{
		        printf("glBufferData, ");
		}
		printf("\n");
		if ( sdl->usevbo )
		{
			printf("OpenGL: PBO not supported, no VBO support.\n");
			sdl->usepbo=FALSE;
		}
	}

	if ( sdl->usepbo && ( !pfn_glMapBuffer || !pfn_glUnmapBuffer ) )
	{
		sdl->usepbo=FALSE;
		printf("OpenGL: PBO not supported, missing: ");
		if (!pfn_glMapBuffer)
		{
		        printf("glMapBuffer, ");
		}
		if (!pfn_glUnmapBuffer)
		{
		        printf("glUnmapBuffer, ");
		}
		printf("\n");
	}

	if ( sdl->usevbo )
	{
		printf("OpenGL: VBO supported\n");
	}
	else
	{
		printf("OpenGL: VBO not supported\n");
	}

	if ( sdl->usepbo )
	{
		printf("OpenGL: PBO supported\n");
	}
	else
	{
		printf("OpenGL: PBO not supported\n");
	}

	if ( sdl->useglsl )
	{
		pfn_glActiveTexture = (PFNGLACTIVETEXTUREPROC) SDL_GL_GetProcAddress("glActiveTexture");

		if (!pfn_glActiveTexture)
		{
			printf("OpenGL: GLSL disabled, glActiveTexture not supported\n");
			sdl->useglsl = 0;
		}
	}

	if ( sdl->useglsl )
	{
		sdl->useglsl = !glsl_shader_init(sdl);

		if ( ! sdl->useglsl )
		{
			printf("OpenGL: GLSL supported, but shader instantiation failed - disabled\n");
		} 
		else 
		{
			glsl_shader_feature = GLSL_SHADER_FEAT_PLAIN;
			if ( 0 < video_config.glsl_filter && video_config.glsl_filter < GLSL_SHADER_FEAT_NUMBER )
			{
				glsl_shader_feature = video_config.glsl_filter;
				printf("OpenGL: GLSL using shader filter %d (vid filter: %d)\n", 
					glsl_shader_feature, video_config.filter);
			} 
			else 
			{
				printf("OpenGL: GLSL using default plain shader (vid filter: %d)\n", video_config.filter);
			}

			if ( sdl->glsl_vid_attributes )
			{
				printf("OpenGL: GLSL direct brightness, contrast setting for RGB games\n");
			}
			else
			{
				printf("OpenGL: GLSL paletted gamma, brightness, contrast setting for RGB games\n");
			}
		}
	} 
}

#define GL_NO_PRIMITIVE -1

static int drawogl_window_draw(sdl_window_info *window, UINT32 dc, int update)
{
	sdl_info *sdl = window->dxdata;
	render_primitive *prim;
	texture_info *texture;
	float vofs, hofs;
	int ch, cw;
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
             window->sdlsurf->w!= surf_w || window->sdlsurf->w!= surf_w 
           )
        {
		if ( !window->initialized )
		{
			loadGLExtensions(sdl);
		}

                surf_w=window->sdlsurf->w;
                surf_h=window->sdlsurf->h;

                // reset the current matrix to the identity
                glLoadIdentity();				   

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

                if ( ! window->initialized )
		{
                        glEnableClientState(GL_VERTEX_ARRAY);
                        glVertexPointer(2, GL_FLOAT, 0, texVerticex); // no VBO, since it's too volatile

			window->initialized = 1;
                }
        }

	// compute centering parameters
	vofs = hofs = 0.0f;
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

	osd_lock_acquire(window->primlist->lock);

	// now draw
	for (prim = window->primlist->head; prim != NULL; prim = prim->next)
	{
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

                                // first update/upload the textures
                                texture = texture_update(sdl, prim);

                                set_blendmode(sdl, PRIMFLAG_GET_BLENDMODE(prim->flags), texture);

                                glColor4f(prim->color.r, prim->color.g, prim->color.b, prim->color.a);

                                texVerticex[0]=prim->bounds.x0 + hofs;
                                texVerticex[1]=prim->bounds.y0 + vofs;
                                texVerticex[2]=prim->bounds.x1 + hofs;
                                texVerticex[3]=prim->bounds.y0 + vofs;
                                texVerticex[4]=prim->bounds.x1 + hofs;
                                texVerticex[5]=prim->bounds.y1 + vofs;
                                texVerticex[6]=prim->bounds.x0 + hofs;
                                texVerticex[7]=prim->bounds.y1 + vofs;

                                glDrawArrays(GL_QUADS, 0, 4);

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
//  compute_blit_surface_size
//============================================================

extern int complete_create(sdl_window_info *window);

void compute_blit_surface_size(sdl_window_info *window, int window_width, int window_height)
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

//	printf("Render target wants %d x %d, minimum is %d x %d\n", target_width, target_height, newwidth, newheight);

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
		clear_surface(window);
		clear_surface(window);
		clear_surface(window);
	}

	sdl->blitwidth = newwidth;
	sdl->blitheight = newheight;
}

//============================================================
//  pick_best_mode
//============================================================

void pick_best_mode(sdl_window_info *window, int *fswidth, int *fsheight)
{
	int minimum_width, minimum_height, target_width, target_height;
	SDL_Rect **modes;
	int i;
	float size_score, best_score = 0.0f;

	// determine the minimum width/height for the selected target
	render_target_get_minimum_size(window->target, &minimum_width, &minimum_height);

	// use those as the target for now
	target_width = minimum_width * MAX(1, video_config.prescale);
	target_height = minimum_height * MAX(1, video_config.prescale);

	// if we're not stretching, allow some slop on the minimum since we can handle it
	{
		minimum_width -= 4;
		minimum_height -= 4;
	}

	modes = SDL_ListModes(NULL, SDL_FULLSCREEN | SDL_DOUBLEBUF);

	if (modes == (SDL_Rect **)0)
	{
		printf("SDL: No modes available?!\n");
		exit(-1);
	}
	else if (modes == (SDL_Rect **)-1)	// all modes are possible
	{
		*fswidth = window->maxwidth;
		*fsheight = window->maxheight;
	}
	else
	{
		for (i = 0; modes[i]; ++i)
		{
			// compute initial score based on difference between target and current
			size_score = 1.0f / (1.0f + fabs((INT32)modes[i]->w - target_width) + fabs((INT32)modes[i]->h - target_height));

			// if the mode is too small, give a big penalty
			if (modes[i]->w < minimum_width || modes[i]->h < minimum_height)
				size_score *= 0.01f;

			// if mode is smaller than we'd like, it only scores up to 0.1
			if (modes[i]->w < target_width || modes[i]->h < target_height)
				size_score *= 0.1f;

			// if we're looking for a particular mode, that's a winner
			if (modes[i]->w == window->maxwidth && modes[i]->h == window->maxheight)
				size_score = 2.0f;

			printf("%4dx%4d -> %f\n", (int)modes[i]->w, (int)modes[i]->h, size_score);

			// best so far?
			if (size_score > best_score)
			{
				best_score = size_score;
				*fswidth = modes[i]->w;
				*fsheight = modes[i]->h;
			}

		}
	}
}

#if USE_OPENGL

#ifdef DEBUG_VERBOSE   // FYI .. 
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
#endif

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
		 texture->format==SDL_TEXFORMAT_PALETTE16 ||       // glsl idx lut 
	         texture->format==SDL_TEXFORMAT_RGB32_PALETTED ||  // glsl rgb lut
	         texture->format==SDL_TEXFORMAT_RGB15_PALETTED 
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
                                            int width, int height,
                                            int* p_width, int* p_height, int* p_width_create, int* p_height_create) 
{
        int width_create = width;
        int height_create= height;

	width_create  = get_valid_pow2_value (width,  texture->texpow2);
	height_create = get_valid_pow2_value (height, texture->texpow2);

	// don't prescale above max texture size
	while (texture->xprescale > 1 && width_create * texture->xprescale > sdl->texture_max_width)
		texture->xprescale--;
	while (texture->yprescale > 1 && height_create * texture->yprescale > sdl->texture_max_height)
		texture->yprescale--;
	if (PRIMFLAG_GET_SCREENTEX(flags) && (texture->xprescale != video_config.prescale || texture->yprescale != video_config.prescale))
		printf("SDL: adjusting prescale from %dx%d to %dx%d\n", video_config.prescale, video_config.prescale, texture->xprescale, texture->yprescale);

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
            if (!printed) fprintf(stderr, "Texture too big! (wanted: %dx%d, max is %dx%d)\n", finalwidth_create, finalheight_create, sdl->texture_max_width, sdl->texture_max_height);
            printed = TRUE;
        }

#ifdef DEBUG_VERBOSE   // FYI .. 
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
        printf("GL texture: copy %d - shader %d - dynamic %d -  %dx%d %dx%d [format: %s, Equal: %d, Palette: %d, scale %dx%d, borderpix %d, pitch %d,%d/%d], colors: %d bytesPerPixel %d\n", 
                !texture->nocopy, texture->type==TEXTURE_TYPE_SHADER, texture->type==TEXTURE_TYPE_DYNAMIC,
                finalwidth, finalheight, finalwidth_create, finalheight_create, 
                texfmt_to_string[texture->format], 
                (int)texture_copy_properties[texture->format][SDL_TEXFORMAT_SRC_EQUALS_DEST],
                (int)texture_copy_properties[texture->format][SDL_TEXFORMAT_SRC_HAS_PALETTE],
                texture->xprescale, texture->yprescale, texture->borderpix,
                texsource->rowpixels, finalwidth, sdl->texture_max_width,
		sdl->totalColors, (int)texture->texProperties[SDL_TEXFORMAT_PIXEL_SIZE]
	      );
        }
#endif

	// compute the U/V scale factors
	if (texture->borderpix)
	{
		int unscaledwidth = (finalwidth_create-2) / texture->xprescale + 2;
		int unscaledheight = (finalheight_create-2) / texture->yprescale + 2;
		texture->ustart = 1.0f / (float)(unscaledwidth);
		texture->ustop = (float)(texsource->width + 1) / (float)(unscaledwidth);
		texture->vstart = 1.0f / (float)(unscaledheight);
		texture->vstop = (float)(texsource->height + 1) / (float)(unscaledheight);
	}
	else
	{
		texture->ustart = 0.0f;
		texture->ustop = (float)(texsource->width*texture->xprescale) / (float)finalwidth_create;
		texture->vstart = 0.0f;
		texture->vstop = (float)(texsource->height*texture->yprescale) / (float)finalheight_create;
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

static int texture_shader_create(sdl_info *sdl, const render_texinfo *texsource, texture_info *texture, UINT32 flags)
{
	int uniform_location;
	int lut_table_width_pow2=0;
	int lut_table_height_pow2=0;
	render_container *container = render_container_get_screen(sdl->viewscreen);

	assert ( texture->type==TEXTURE_TYPE_SHADER );

	texture->lut_table_height = 1; // default ..

	switch(texture->format)
	{
		case SDL_TEXFORMAT_RGB32_PALETTED:
			if ( sdl->glsl_vid_attributes )
				texture->lut_glsl_program = glsl_programs[GLSL_SHADER_TYPE_RGB32_DIRECT][glsl_shader_feature];
			else
				texture->lut_glsl_program = glsl_programs[GLSL_SHADER_TYPE_RGB32_LUT][glsl_shader_feature];
			texture->lut_table_width  = 1 << 8; // 8 bits per component
			texture->lut_table_width *= 3;      // BGR ..
			break;

		case SDL_TEXFORMAT_RGB15_PALETTED:
			if ( sdl->glsl_vid_attributes )
				texture->lut_glsl_program = glsl_programs[GLSL_SHADER_TYPE_RGB32_DIRECT][glsl_shader_feature];
			else
				texture->lut_glsl_program = glsl_programs[GLSL_SHADER_TYPE_RGB32_LUT][glsl_shader_feature];
			texture->lut_table_width  = 1 << 5; // 5 bits per component
			texture->lut_table_width *= 3;      // BGR ..
			break;

		case SDL_TEXFORMAT_PALETTE16:
			texture->lut_glsl_program = glsl_programs[GLSL_SHADER_TYPE_IDX16][glsl_shader_feature];
		        texture->lut_table_width  = sdl->totalColors;
			break;

		default:
			// should never happen
			texture->lut_glsl_program = 0;
			assert(0);
			exit(1);
	}

	if ( texture->lut_table_width > sdl->texture_max_width )
	{
		texture->lut_table_height  = texture->lut_table_width / sdl->texture_max_width + 1;
		texture->lut_table_width   = sdl->texture_max_width;

	}
	lut_table_height_pow2 = get_valid_pow2_value (texture->lut_table_height, texture->texpow2);
	lut_table_width_pow2  = get_valid_pow2_value (texture->lut_table_width,  texture->texpow2);

#ifdef DEBUG_VERBOSE   // FYI .. 
	printf("lut_table_sz %dx%d, lut_table_sz_pow2 %dx%d\n",
			texture->lut_table_width, texture->lut_table_height,
			lut_table_width_pow2, lut_table_height_pow2);
#endif

	if ( lut_table_width_pow2 > sdl->texture_max_width || lut_table_height_pow2 > sdl->texture_max_height )
	{
		fprintf(stderr, "Need lut size %dx%d, but max text size is %dx%d, bail out\n",
			lut_table_width_pow2, lut_table_height_pow2,
			sdl->texture_max_width, sdl->texture_max_height);
		return -1;
	}

	GL_CHECK_ERROR_QUIET();

	pfn_glUseProgramObjectARB(texture->lut_glsl_program);
	GL_CHECK_ERROR_NORMAL();

	if ( sdl->glsl_vid_attributes && texture->format!=SDL_TEXFORMAT_PALETTE16 )
	{
		GLfloat vid_attributes[4]; // gamma, contrast, brightness, effect
		vid_attributes[0] = render_container_get_gamma(container);
		vid_attributes[1] = render_container_get_contrast(container);
		vid_attributes[2] = render_container_get_brightness(container);
		vid_attributes[3] = 0.0f;
		texture->uni_vid_attributes = pfn_glGetUniformLocationARB(texture->lut_glsl_program, "vid_attributes");
		pfn_glUniform4fvARB(texture->uni_vid_attributes, 1, &(vid_attributes[0]));
	} else {
		assert ( texture->uni_vid_attributes == 0 );
	}

	if ( !texture->uni_vid_attributes )
	{
		// GL_TEXTURE1
		uniform_location = pfn_glGetUniformLocationARB(texture->lut_glsl_program, "colortable_texture");
		pfn_glUniform1iARB(uniform_location, 1);
		GL_CHECK_ERROR_NORMAL();

		{
			GLfloat colortable_sz[2] = { (GLfloat)texture->lut_table_width, (GLfloat)texture->lut_table_height };
			uniform_location = pfn_glGetUniformLocationARB(texture->lut_glsl_program, "colortable_sz");
			pfn_glUniform2fvARB(uniform_location, 1, &(colortable_sz[0]));
			GL_CHECK_ERROR_NORMAL();
		}

		{
			GLfloat colortable_pow2_sz[2] = { (GLfloat)lut_table_width_pow2, (GLfloat)lut_table_height_pow2 };
			uniform_location = pfn_glGetUniformLocationARB(texture->lut_glsl_program, "colortable_pow2_sz");
			pfn_glUniform2fvARB(uniform_location, 1, &(colortable_pow2_sz[0]));
			GL_CHECK_ERROR_NORMAL();
		}
	}

	switch ( glsl_shader_feature)
	{
		case GLSL_SHADER_FEAT_PLAIN:
		case GLSL_SHADER_FEAT_BILINEAR:
			break;
		case GLSL_SHADER_FEAT_CONV_GAUSSIAN:
			{
				// don't change these magic numbers,
				// since we had to roll out our shader loop,
				// since at least one vendor won't support it ;-)
				#define KERNEL_SIZE 9
				#define  VAL(v) { v/16.0, v/16.0, v/16.0, 0.0 }
				const static GLfloat kernelValues[KERNEL_SIZE][4] = {  
						VAL( 1.0), VAL( 2.0), VAL( 1.0),
						VAL( 2.0), VAL( 4.0), VAL( 2.0),
						VAL( 1.0), VAL( 2.0), VAL( 1.0),
				};
				GLfloat offsets[KERNEL_SIZE][2] = {  // RECT values
					{ -1.0f, -1.0f }, { +0.0f, -1.0f }, { +1.0f, -1.0f },
					{ -1.0f, +0.0f }, { +0.0f, +0.0f }, { +1.0f, +0.0f },
					{ -1.0f, +1.0f }, { +0.0f, +1.0f }, { +1.0f, +1.0f },
				};
				
				uniform_location = pfn_glGetUniformLocationARB(texture->lut_glsl_program, "KernelValue");
				pfn_glUniform4fvARB(uniform_location, KERNEL_SIZE, &(kernelValues[0][0]));
				GL_CHECK_ERROR_NORMAL();

				{
					int i;
					GLfloat scale_x = 1.0/(GLfloat)texture->rawwidth_create;
					GLfloat scale_y = 1.0/(GLfloat)texture->rawheight_create;
					for(i=0; i<KERNEL_SIZE; i++)
					{
						offsets[i][0] *= scale_x;
						offsets[i][1] *= scale_y;
					}
				}

				uniform_location = pfn_glGetUniformLocationARB(texture->lut_glsl_program, "Offset");
				pfn_glUniform2fvARB(uniform_location, KERNEL_SIZE, &(offsets[0][0]));
				GL_CHECK_ERROR_NORMAL();
			}
			break;
	}

	// GL_TEXTURE0
	uniform_location = pfn_glGetUniformLocationARB(texture->lut_glsl_program, "color_texture");
	pfn_glUniform1iARB(uniform_location, 0);

	{
		GLfloat color_texture_sz[2] = { (GLfloat)texture->rawwidth, (GLfloat)texture->rawheight };
		uniform_location = pfn_glGetUniformLocationARB(texture->lut_glsl_program, "color_texture_sz");
		pfn_glUniform2fvARB(uniform_location, 1, &(color_texture_sz[0]));
		GL_CHECK_ERROR_NORMAL();
	}

	{
		GLfloat color_texture_pow2_sz[2] = { (GLfloat)texture->rawwidth_create, (GLfloat)texture->rawheight_create };
		uniform_location = pfn_glGetUniformLocationARB(texture->lut_glsl_program, "color_texture_pow2_sz");
		pfn_glUniform2fvARB(uniform_location, 1, &(color_texture_pow2_sz[0]));
		GL_CHECK_ERROR_NORMAL();
	}

	if ( !texture->uni_vid_attributes )
	{
		glGenTextures(1, (GLuint *)&texture->lut_texturename);
		glBindTexture(GL_TEXTURE_2D, texture->lut_texturename);

		glPixelStorei(GL_UNPACK_ROW_LENGTH, lut_table_width_pow2);

		{
			GLint _width, _height;
			if ( gl_texture_check_size(GL_TEXTURE_2D, 0, GL_RGBA8, lut_table_width_pow2, lut_table_height_pow2,
					  0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, &_width, &_height, 1) )
			{
				fprintf(stderr, "cannot create lut table texture, req: %dx%d, avail: %dx%d - bail out\n",
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

		assert ( texture->lut_texturename );
	}

	// get a name for this texture
	glGenTextures(1, (GLuint *)&texture->texturename);
	glBindTexture(GL_TEXTURE_2D, texture->texturename);

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
			fprintf(stderr, "cannot create lut table texture, req: %dx%d, avail: %dx%d - bail out\n",
				lut_table_width_pow2, lut_table_height_pow2, (int)_width, (int)_height);
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
			fprintf(stderr, "cannot create lut bitmap texture, req: %dx%d, avail: %dx%d - bail out\n",
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

static texture_info *texture_create(sdl_info *sdl, const render_texinfo *texsource, UINT32 flags)
{
	texture_info *texture;
	int    texCreated = FALSE;
	
	// allocate a new texture
	texture = malloc(sizeof(*texture));
	memset(texture, 0, sizeof(*texture));

	// fill in the core data
	texture->hash = texture_compute_hash(texsource, flags);
	texture->flags = flags;
	texture->texinfo = *texsource;
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
			fprintf(stderr, "Unknown textureformat %d\n", PRIMFLAG_GET_TEXFORMAT(flags));
	}

	// compute the size
	texture_compute_size_type(sdl, texsource, texture, flags);

        texture->pbo=0;

	if ( texture->type != TEXTURE_TYPE_SHADER && sdl->useglsl)
	{
		pfn_glUseProgramObjectARB(0); // back to fixed function pipeline
	}

	if(sdl->usepbo)
	{
		pfn_glBindBuffer( GL_PIXEL_UNPACK_BUFFER_ARB, 0); // unbound a prebound buffer
	}

	if ( texture->type==TEXTURE_TYPE_SHADER )
	{
		if ( texture_shader_create(sdl, texsource, texture, flags) )
                {
                        free(texture);
			return NULL;
                }
		texCreated = TRUE;
	} else if ( texture->type == TEXTURE_TYPE_DYNAMIC )
	{
                assert(sdl->usepbo);

                // create the PBO
                pfn_glGenBuffers(1, (GLuint *)&texture->pbo);
        } 

	if(!texCreated)
	{
		// get a name for this texture
		glGenTextures(1, (GLuint *)&texture->texturename);

		glEnable(texture->texTarget);

                // make sure we're operating on *this* texture
                glBindTexture(texture->texTarget, texture->texturename);

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

        if ( !texture->nocopy && texture->type!=TEXTURE_TYPE_DYNAMIC )
        {
            texture->data = malloc(texture->rawwidth* texture->rawheight * texture->texProperties[SDL_TEXFORMAT_PIXEL_SIZE]);
            texture->data_own=TRUE;
        }

	if (texture->prescale_effect && !texture_copy_properties[texture->format][SDL_TEXFORMAT_SRC_EQUALS_DEST])
		texture->effectbuf = malloc_or_die(texsource->width * 3 * texture->texProperties[SDL_TEXFORMAT_PIXEL_SIZE]);

	// copy the data to the texture
	texture_set_data(sdl, texture, texsource, flags);

	// add us to the texture list
	texture->next = sdl->texlist;
	sdl->texlist = texture;

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

                pfn_glBindBuffer( GL_PIXEL_UNPACK_BUFFER_ARB, texture->pbo);

                // set up the PBO dimension, ..
		pfn_glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, 
		                 texture->rawwidth * texture->rawheight * texture->texProperties[SDL_TEXFORMAT_PIXEL_SIZE],
				 NULL, GL_STREAM_DRAW);

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
		if ( texture->lut_texturename )
		{
			assert ( !texture->uni_vid_attributes );

			pfn_glActiveTexture(GL_TEXTURE1);
			glBindTexture(texture->texTarget, texture->lut_texturename); // we have bind

			glPixelStorei(GL_UNPACK_ROW_LENGTH, texture->lut_table_width);

			// give the card a hint
			glTexSubImage2D(texture->texTarget, 0, 0, 0, texture->lut_table_width, texture->lut_table_height,
				     GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, texsource->palette );

			pfn_glActiveTexture(GL_TEXTURE0);
			glBindTexture(texture->texTarget, texture->texturename); // we have to rebind .. 

		}  else {
			render_container *container = render_container_get_screen(sdl->viewscreen);
			GLfloat vid_attributes[4]; // gamma, contrast, brightness, effect

			assert ( sdl->glsl_vid_attributes );

			vid_attributes[0] = render_container_get_gamma(container);
			vid_attributes[1] = render_container_get_contrast(container);
			vid_attributes[2] = render_container_get_brightness(container);
			vid_attributes[3] = 0.0f;
			pfn_glUniform4fvARB(texture->uni_vid_attributes, 1, &(vid_attributes[0]));
		}

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
		glPixelStorei(GL_UNPACK_ROW_LENGTH, texture->rawwidth);

		// unmap the buffer from the CPU space so it can DMA
		pfn_glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB);

		// kick off the DMA
		glTexSubImage2D(texture->texTarget, 0, 0, 0, texture->rawwidth, texture->rawheight, 
			        texture->texProperties[SDL_TEXFORMAT_FORMAT], 
				texture->texProperties[SDL_TEXFORMAT_TYPE], NULL);
	} else {
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

static texture_info * texture_update(sdl_info *sdl, const render_primitive *prim)
{
        float du, dv;

	texture_info *texture = texture_find(sdl, prim);

#if 0 // FYI ..
	printf("0x%X: tx pbo: name %d, pbo %d, seq %d\n",
		pthread_self(), texture->texturename, texture->pbo, texture->texinfo.seqid);
#endif

	// if we didn't find one, create a new texture
	if (texture == NULL && prim->texture.base != NULL)
        {
                texture = texture_create(sdl, &prim->texture, prim->flags);
                du = texture->ustop - texture->ustart;
                dv = texture->vstop - texture->vstart;

                if ( texture->texTarget == GL_TEXTURE_RECTANGLE_ARB )
                {
                    // texture coordinates for TEXTURE_RECTANGLE are 0,0 -> w,h
                    // rather than 0,0 -> 1,1 as with normal OpenGL texturing
                    du *= (float)texture->rawwidth;
                    dv *= (float)texture->rawheight;
                }

                texture->texCoord[0]=texture->ustart + du * prim->texcoords.tl.u;
                texture->texCoord[1]=texture->vstart + dv * prim->texcoords.tl.v;
                texture->texCoord[2]=texture->ustart + du * prim->texcoords.tr.u;
                texture->texCoord[3]=texture->vstart + dv * prim->texcoords.tr.v;
                texture->texCoord[4]=texture->ustart + du * prim->texcoords.br.u;
                texture->texCoord[5]=texture->vstart + dv * prim->texcoords.br.v;
                texture->texCoord[6]=texture->ustart + du * prim->texcoords.bl.u;
                texture->texCoord[7]=texture->vstart + dv * prim->texcoords.bl.v;
		#if 0 
                printf("texCoord (%f/%f)/(%f/%f) (%f/%f)/(%f/%f)\n", 
                        texture->texCoord[0], texture->texCoord[1], texture->texCoord[2], texture->texCoord[3],
                        texture->texCoord[4], texture->texCoord[5], texture->texCoord[6], texture->texCoord[7]);
                #endif

                glEnableClientState(GL_TEXTURE_COORD_ARRAY);
                if(sdl->usevbo)
                {
                    // Generate And Bind The Texture Coordinate Buffer
                    pfn_glGenBuffers( 1, &(texture->texCoordBufferName) );
                    pfn_glBindBuffer( GL_ARRAY_BUFFER_ARB, texture->texCoordBufferName );
                    // Load The Data
                    pfn_glBufferData( GL_ARRAY_BUFFER_ARB, 4*2*sizeof(GLfloat), texture->texCoord, GL_STATIC_DRAW_ARB );
                    glTexCoordPointer( 2, GL_FLOAT, 0, (char *) NULL ); // we are using ARB VBO buffers 
                } else {
                    glTexCoordPointer(2, GL_FLOAT, 0, texture->texCoord);
                }
        } else  if (texture != NULL)
        {

		if ( texture->type == TEXTURE_TYPE_SHADER )
		{
			pfn_glUseProgramObjectARB(texture->lut_glsl_program); // back to our shader
                } else if ( texture->type == TEXTURE_TYPE_DYNAMIC )
                {
		        assert ( sdl->usepbo ) ;
                        pfn_glBindBuffer( GL_PIXEL_UNPACK_BUFFER_ARB, texture->pbo); 
                        glEnable(texture->texTarget);
		} else { 
                        glEnable(texture->texTarget);
                }

                glBindTexture(texture->texTarget, texture->texturename);

                if (prim->texture.base != NULL && texture->texinfo.seqid != prim->texture.seqid)
                {
                        texture->texinfo.seqid = prim->texture.seqid;
                        // if we found it, but with a different seqid, copy the data
                        texture_set_data(sdl, texture, &prim->texture, prim->flags);
                }

                glEnableClientState(GL_TEXTURE_COORD_ARRAY);
                if(sdl->usevbo)
                {
                    pfn_glBindBuffer( GL_ARRAY_BUFFER_ARB, texture->texCoordBufferName );
                    glTexCoordPointer( 2, GL_FLOAT, 0, (char *) NULL ); // we are using ARB VBO buffers 
                } else {
                    glTexCoordPointer(2, GL_FLOAT, 0, texture->texCoord);
                }
        }

        return texture;
}

void texture_disable(sdl_info *sdl, texture_info * texture)
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
        }

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

void drawsdl_destroy_all_textures(sdl_window_info *window)
{
        int lock=FALSE;

        if(window->primlist && window->primlist->lock) { lock=TRUE; osd_lock_acquire(window->primlist->lock); }
        {
                texture_info *next_texture=NULL, *texture = NULL;
                sdl_info *sdl = window->dxdata;

                if (sdl == NULL)
                        goto exit;

                texture = sdl->texlist;

                glDisableClientState(GL_VERTEX_ARRAY);

                texture_all_disable(sdl);
                glFinish();

                if(sdl->usepbo)
                {
                        pfn_glBindBuffer( GL_PIXEL_UNPACK_BUFFER_ARB, 0);
                }

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

			if(texture->lut_texturename)
				glDeleteTextures(1, (GLuint *)&texture->lut_texturename);

                        glDeleteTextures(1, (GLuint *)&texture->texturename);
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
		if (texture->borderpix)
			*dst++ = 0;
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
		if (texture->borderpix)
			*dst++ = 0;
	}
}

// YUV Blitters

#include "yuv_blit.c"

