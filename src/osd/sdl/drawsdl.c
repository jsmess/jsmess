//============================================================
//  
//  drawsdl.c - SDL software and OpenGL implementation
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//  SDLMAME by Olivier Galibert and R. Belmont
//
//============================================================

// standard C headers
#include <math.h>
#include <stdio.h>

// MAME headers
#include "osdcomm.h"
#include "render.h"
#include "options.h"
#include "driver.h"

// standard SDL headers
#include <SDL/SDL.h>

// OSD headers
#include "osdsdl.h"
#include "window.h"

//============================================================
//  DEBUGGING
//============================================================

//============================================================
//  CONSTANTS
//============================================================

//============================================================
//  MACROS
//============================================================


//============================================================
//  TYPES
//============================================================

/* sdl_info is the information about SDL for the current screen */
typedef struct _sdl_info sdl_info;
struct _sdl_info
{
	INT32 				blittimer;
	UINT32				extra_flags;

#if (SDL_VERSION_ATLEAST(1,3,0))
	SDL_TextureID		texture_id;
#else
	// SDL surface
	SDL_Surface 		*sdlsurf;
	SDL_Overlay 		*yuvsurf;
#endif

	// YUV overlay
	UINT32 				*yuv_lookup;
	UINT16				*yuv_bitmap;
	
	// if we leave scaling to SDL and the underlying driver, this 
	// is the render_target_width/height to use
	
	int					hw_scale_width;
	int					hw_scale_height;
	int					last_hofs;
	int					last_vofs;
};

//============================================================
//  INLINES
//============================================================

//============================================================
//  PROTOTYPES
//============================================================

// core functions
static void drawsdl_exit(void);
static void drawsdl_attach(sdl_draw_info *info, sdl_window_info *window);
static int drawsdl_window_create(sdl_window_info *window, int width, int height);
static void drawsdl_window_resize(sdl_window_info *window, int width, int height);
static void drawsdl_window_destroy(sdl_window_info *window);
static const render_primitive_list *drawsdl_window_get_primitives(sdl_window_info *window);
static int drawsdl_window_draw(sdl_window_info *window, UINT32 dc, int update);
static void drawsdl_destroy_all_textures(sdl_window_info *window);
static void drawsdl_window_clear(sdl_window_info *window);
static int drawsdl_xy_to_render_target(sdl_window_info *window, int x, int y, int *xt, int *yt);

#if (SDL_VERSION_ATLEAST(1,3,0))
static void setup_texture(sdl_window_info *window, int tempwidth, int tempheight);
#endif

// soft rendering
static void drawsdl_rgb888_draw_primitives(const render_primitive *primlist, void *dstdata, UINT32 width, UINT32 height, UINT32 pitch);
static void drawsdl_bgr888_draw_primitives(const render_primitive *primlist, void *dstdata, UINT32 width, UINT32 height, UINT32 pitch);
static void drawsdl_rgb565_draw_primitives(const render_primitive *primlist, void *dstdata, UINT32 width, UINT32 height, UINT32 pitch);
static void drawsdl_rgb555_draw_primitives(const render_primitive *primlist, void *dstdata, UINT32 width, UINT32 height, UINT32 pitch);

// YUV overlays

static void drawsdl_yuv_init(sdl_info *sdl);
static void yuv_RGB_to_YV12(UINT16 *bitmap, sdl_info *sdl, UINT8 *ptr, int pitch);
static void yuv_RGB_to_YV12X2(UINT16 *bitmap, sdl_info *sdl, UINT8 *ptr, int pitch);
static void yuv_RGB_to_YUY2(UINT16 *bitmap, sdl_info *sdl, UINT8 *ptr, int pitch);
static void yuv_RGB_to_YUY2X2(UINT16 *bitmap, sdl_info *sdl, UINT8 *ptr, int pitch);

// Static declarations

#if (!SDL_VERSION_ATLEAST(1,3,0))
static int shown_video_info = 0;
static const char *scale_mode_names[] = { "none", "hwblit", "yv12", "yv12x2", "yuy2", "yuy2x2" };
#endif

//============================================================
//  drawsdl_init
//============================================================

int drawsdl_init(sdl_draw_info *callbacks)
{
	// fill in the callbacks
	callbacks->exit = drawsdl_exit;
	callbacks->attach = drawsdl_attach;

	if (SDL_VERSION_ATLEAST(1,3,0))
		mame_printf_verbose("Using SDL multi-window soft driver (SDL 1.3+)\n");
	else
		mame_printf_verbose("Using SDL single-window soft driver (SDL 1.2)\n");

	return 0;
}

//============================================================
//  drawsdl_exit
//============================================================

static void drawsdl_exit(void)
{
}

//============================================================
//  drawsdl_attach
//============================================================

static void drawsdl_attach(sdl_draw_info *info, sdl_window_info *window)
{
	// fill in the callbacks
	window->create = drawsdl_window_create;
	window->resize = drawsdl_window_resize;
	window->get_primitives = drawsdl_window_get_primitives;
	window->draw = drawsdl_window_draw;
	window->destroy = drawsdl_window_destroy;
	window->destroy_all_textures = drawsdl_destroy_all_textures;
	window->clear = drawsdl_window_clear;
	window->xy_to_render_target = drawsdl_xy_to_render_target;
}

//============================================================
//  drawsdl_destroy_all_textures
//============================================================

static void drawsdl_destroy_all_textures(sdl_window_info *window)
{
	/* nothing to be done in soft mode */
}

//============================================================
//  setup_texture for window
//============================================================

#if (SDL_VERSION_ATLEAST(1,3,0))
static void setup_texture(sdl_window_info *window, int tempwidth, int tempheight)
{
	sdl_info *	sdl = window->dxdata;
	SDL_DisplayMode mode;
	
	// Determine preferred pixelformat and set up yuv if necessary
	SDL_GetCurrentDisplayMode(&mode);
	
	if (sdl->yuv_bitmap)
	{
		free(sdl->yuv_bitmap);
		sdl->yuv_bitmap = NULL;	
	}
	
	switch (window->scale_mode)
	{
		case VIDEO_SCALE_MODE_NONE:
			sdl->texture_id = SDL_CreateTexture(mode.format, SDL_TEXTUREACCESS_STREAMING, 
					tempwidth, tempheight);
			SDL_SetTextureScaleMode(sdl->texture_id, SDL_TEXTURESCALEMODE_NONE);
			break;
		case VIDEO_SCALE_MODE_HWBLIT:
			render_target_get_minimum_size(window->target, &sdl->hw_scale_width, &sdl->hw_scale_height);
			sdl->texture_id = SDL_CreateTexture(mode.format, SDL_TEXTUREACCESS_STREAMING, 
					sdl->hw_scale_width, sdl->hw_scale_height);
			break;
		case VIDEO_SCALE_MODE_YV12: 
			render_target_get_minimum_size(window->target, &sdl->hw_scale_width, &sdl->hw_scale_height);
			sdl->yuv_bitmap = malloc_or_die(sdl->hw_scale_width * sdl->hw_scale_height * sizeof(UINT16));
			sdl->texture_id = SDL_CreateTexture(SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, 
					sdl->hw_scale_width, sdl->hw_scale_height);
			break;
		case VIDEO_SCALE_MODE_YV12X2: 
			render_target_get_minimum_size(window->target, &sdl->hw_scale_width, &sdl->hw_scale_height);
			sdl->yuv_bitmap = malloc_or_die(sdl->hw_scale_width * sdl->hw_scale_height * sizeof(UINT16));
			sdl->texture_id = SDL_CreateTexture(SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, 
					sdl->hw_scale_width * 2, sdl->hw_scale_height * 2);
			break;
		case VIDEO_SCALE_MODE_YUY2: 
			render_target_get_minimum_size(window->target, &sdl->hw_scale_width, &sdl->hw_scale_height);
			sdl->yuv_bitmap = malloc_or_die(sdl->hw_scale_width * sdl->hw_scale_height * sizeof(UINT16));
			sdl->texture_id = SDL_CreateTexture(SDL_PIXELFORMAT_YUY2, SDL_TEXTUREACCESS_STREAMING, 
					sdl->hw_scale_width, sdl->hw_scale_height);
			break;
		case VIDEO_SCALE_MODE_YUY2X2: 
			render_target_get_minimum_size(window->target, &sdl->hw_scale_width, &sdl->hw_scale_height);
			sdl->yuv_bitmap = malloc_or_die(sdl->hw_scale_width * sdl->hw_scale_height * sizeof(UINT16));
			sdl->texture_id = SDL_CreateTexture(SDL_PIXELFORMAT_YUY2, SDL_TEXTUREACCESS_STREAMING, 
					sdl->hw_scale_width * 2, sdl->hw_scale_height);
			break;
	}
}
#endif

//============================================================
//  yuv_overlay_init
//============================================================

#if (!SDL_VERSION_ATLEAST(1,3,0))
static void yuv_overlay_init(sdl_window_info *window)
{
	sdl_info *sdl = window->dxdata;
	int minimum_width, minimum_height;

	render_target_get_minimum_size(window->target, &minimum_width, &minimum_height);
	if (sdl->yuvsurf != NULL)
	{
		SDL_FreeYUVOverlay(sdl->yuvsurf);
		sdl->yuvsurf = NULL;
	}
	
	if (sdl->yuv_bitmap != NULL)
	{
		free(sdl->yuv_bitmap);	
	}
	sdl->yuv_bitmap = malloc_or_die(minimum_width*minimum_height*sizeof(UINT16));

	mame_printf_verbose("SDL: Creating %d x %d YUV-Overlay ...\n", minimum_width, minimum_height);
	switch (window->scale_mode)
	{
		case VIDEO_SCALE_MODE_YV12: 
			sdl->yuvsurf = SDL_CreateYUVOverlay(minimum_width, minimum_height,
        			SDL_YV12_OVERLAY, sdl->sdlsurf);
			break;
		case VIDEO_SCALE_MODE_YV12X2: 
			sdl->yuvsurf = SDL_CreateYUVOverlay(minimum_width*2, minimum_height*2,
	      			SDL_YV12_OVERLAY, sdl->sdlsurf);
			break;
		case VIDEO_SCALE_MODE_YUY2: 
			sdl->yuvsurf = SDL_CreateYUVOverlay(minimum_width, minimum_height,
	      			SDL_YUY2_OVERLAY, sdl->sdlsurf);
			break;
		case VIDEO_SCALE_MODE_YUY2X2: 
			sdl->yuvsurf = SDL_CreateYUVOverlay(minimum_width*2, minimum_height,
	      			SDL_YUY2_OVERLAY, sdl->sdlsurf);
			break;
	}
	if ( sdl->yuvsurf == NULL ) {
		mame_printf_error("SDL: Couldn't create SDL_yuv_overlay: %s\n", SDL_GetError());
		//return 1;
	}
	sdl->hw_scale_width = minimum_width;
	sdl->hw_scale_height = minimum_height;

	if (!shown_video_info)
	{
		mame_printf_verbose("YUV Mode         : %s\n", scale_mode_names[window->scale_mode]);
		mame_printf_verbose("YUV Overlay Size : %d x %d\n", minimum_width, minimum_height);
		mame_printf_verbose("YUV Acceleration : %s\n", sdl->yuvsurf->hw_overlay ? "Hardware" : "Software");
		shown_video_info = 1;
	}
}
#endif

//============================================================
//  drawsdl_window_create
//============================================================

static int drawsdl_window_create(sdl_window_info *window, int width, int height)
{
	sdl_info *sdl = window->dxdata;

	// allocate memory for our structures
	sdl = malloc(sizeof(*sdl));
	memset(sdl, 0, sizeof(*sdl));

	window->dxdata = sdl;
	
#if (SDL_VERSION_ATLEAST(1,3,0))
	sdl->extra_flags = (window->fullscreen ?  
			SDL_WINDOW_BORDERLESS | SDL_WINDOW_INPUT_FOCUS : SDL_WINDOW_RESIZABLE);

	SDL_SelectVideoDisplay(window->monitor->handle);

	if (window->fullscreen && video_config.switchres) 
	{
		SDL_DisplayMode mode;
		SDL_GetCurrentDisplayMode(&mode);
		mode.w = width;
		mode.h = height;
		SDL_SetFullscreenDisplayMode(&mode);	// Try to set mode
	}
	else
		SDL_SetFullscreenDisplayMode(NULL);	// Use desktop
	
	window->window_id = SDL_CreateWindow(window->title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			width, height, sdl->extra_flags);
	SDL_ShowWindow(window->window_id);
	SDL_RaiseWindow(window->window_id);
	
	// create a texture
	SDL_CreateRenderer(window->window_id, -1, SDL_RENDERER_SINGLEBUFFER | SDL_RENDERER_PRESENTDISCARD);
    SDL_SelectRenderer(window->window_id);
	SDL_SetWindowFullscreen(window->window_id, window->fullscreen);
	SDL_GetWindowSize(window->window_id, &window->width, &window->height);
	setup_texture(window, width, height);
#else
	sdl->extra_flags = (window->fullscreen ?  SDL_FULLSCREEN : SDL_RESIZABLE);
	// create the SDL surface (which creates the window in windowed mode)
	if (!IS_YUV_MODE(window->scale_mode))
		sdl->extra_flags |= SDL_DOUBLEBUF;

	sdl->sdlsurf = SDL_SetVideoMode(width, height, 
				   0, SDL_SWSURFACE | SDL_ANYFORMAT | sdl->extra_flags);

	if (!sdl->sdlsurf)
		return 1;

	window->width = sdl->sdlsurf->w;
	window->height = sdl->sdlsurf->h;
	
	if (IS_YUV_MODE(window->scale_mode))
	{
		yuv_overlay_init(window);
	}

	// set the window title
	SDL_WM_SetCaption(window->title, "SDLMAME");
#endif
	sdl->yuv_lookup = NULL;
	sdl->blittimer = 0;
	
	drawsdl_yuv_init(sdl);
	return 0;
}

//============================================================
//  drawsdl_window_resize
//============================================================

static void drawsdl_window_resize(sdl_window_info *window, int width, int height)
{
	sdl_info *sdl = window->dxdata;

#if (SDL_VERSION_ATLEAST(1,3,0))
	SDL_SetWindowSize(window->window_id, width, height);
	SDL_GetWindowSize(window->window_id, &window->width, &window->height);

	// create a texture
	SDL_SelectRenderer(window->window_id);
	SDL_DestroyTexture(sdl->texture_id);

	setup_texture(window, window->width, window->height);
#else
	if (sdl->yuvsurf != NULL)
	{
		SDL_FreeYUVOverlay(sdl->yuvsurf);
		sdl->yuvsurf = NULL;
	}
	SDL_FreeSurface(sdl->sdlsurf);
	//printf("SetVideoMode %d %d\n", wp->resize_new_width, wp->resize_new_height);

	sdl->sdlsurf = SDL_SetVideoMode(width, height, 0, 
			SDL_SWSURFACE | SDL_ANYFORMAT | sdl->extra_flags);
	window->width = sdl->sdlsurf->w;
	window->height = sdl->sdlsurf->h;

	if (IS_YUV_MODE(window->scale_mode))
	{
		yuv_overlay_init(window);
	}
	
#endif
}


//============================================================
//  drawsdl_window_destroy
//============================================================

static void drawsdl_window_destroy(sdl_window_info *window)
{
	sdl_info *sdl = window->dxdata;

	// skip if nothing
	if (sdl == NULL)
		return;

#if (SDL_VERSION_ATLEAST(1,3,0))
	SDL_SelectRenderer(window->window_id);
	SDL_DestroyTexture(sdl->texture_id);
	//SDL_DestroyRenderer(window->window_id);
	SDL_DestroyWindow(window->window_id);
#else
	if (sdl->yuvsurf != NULL)
	{
		SDL_FreeYUVOverlay(sdl->yuvsurf);
		sdl->yuvsurf = NULL;
	}

	if (sdl->sdlsurf)
	{
		SDL_FreeSurface(sdl->sdlsurf);
		sdl->sdlsurf = NULL;
	}
#endif
	// free the memory in the window

	if (sdl->yuv_lookup != NULL)
	{
		free(sdl->yuv_lookup); 
		sdl->yuv_lookup = NULL;
	}
	if (sdl->yuv_bitmap != NULL)
	{
		free(sdl->yuv_bitmap);	
		sdl->yuv_bitmap = NULL;
	}
	window->dxdata = NULL;
	free(sdl);
}

//============================================================
//  drawsdl_window_clear
//============================================================

static void drawsdl_window_clear(sdl_window_info *window)
{
	sdl_info *sdl = window->dxdata;
	
	sdl->blittimer = 3;
}

//============================================================
//  drawsdl_xy_to_render_target
//============================================================

static int drawsdl_xy_to_render_target(sdl_window_info *window, int x, int y, int *xt, int *yt)
{
	sdl_info *sdl = window->dxdata;

	*xt = x - sdl->last_hofs;
	*yt = y - sdl->last_vofs;
	if (*xt<0 || *xt >= window->blitwidth)
		return 0;
	if (*yt<0 || *xt >= window->blitheight)
		return 0;
	if (window->scale_mode == VIDEO_SCALE_MODE_NONE || (!SDL_VERSION_ATLEAST(1,3,0) && !IS_YUV_MODE(window->scale_mode)))
	{
		return 1;
	}
	/* Rescale */
	*xt = (*xt * sdl->hw_scale_width) / window->blitwidth;
	*yt = (*yt * sdl->hw_scale_height) / window->blitheight;
	return 1;
}

//============================================================
//  drawsdl_window_get_primitives
//============================================================

static const render_primitive_list *drawsdl_window_get_primitives(sdl_window_info *window)
{
	sdl_info *sdl = window->dxdata;
	if ((!window->fullscreen) || (video_config.switchres))
	{
		sdlwindow_blit_surface_size(window, window->width, window->height);
	}
	else
	{
		sdlwindow_blit_surface_size(window, window->monitor->center_width, window->monitor->center_height);
	}
	if (window->scale_mode == VIDEO_SCALE_MODE_NONE || (!SDL_VERSION_ATLEAST(1,3,0) && !IS_YUV_MODE(window->scale_mode)))
		render_target_set_bounds(window->target, window->blitwidth, window->blitheight, sdlvideo_monitor_get_aspect(window->monitor));
	else
		render_target_set_bounds(window->target, sdl->hw_scale_width, sdl->hw_scale_height, 0);
	return render_target_get_primitives(window->target);
}

//============================================================
//  drawsdl_window_draw
//============================================================

static int drawsdl_window_draw(sdl_window_info *window, UINT32 dc, int update)
{
	sdl_info *sdl = window->dxdata;
	UINT8 *surfptr;
	INT32 pitch;
	int bpp;
	Uint32 rmask, gmask, bmask, amask;
	INT32 vofs, hofs, blitwidth, blitheight, ch, cw;

	if (video_config.novideo)
	{
		return 0;
	}

	// if we haven't been created, just punt
	if (sdl == NULL)
		return 1;

	// lock it if we need it
#if (!SDL_VERSION_ATLEAST(1,3,0))

	pitch = sdl->sdlsurf->pitch;
	bpp = sdl->sdlsurf->format->BytesPerPixel;
	rmask = sdl->sdlsurf->format->Rmask;
	gmask = sdl->sdlsurf->format->Gmask;
	bmask = sdl->sdlsurf->format->Bmask;
	amask = sdl->sdlsurf->format->Amask;
	
	if (SDL_MUSTLOCK(sdl->sdlsurf)) SDL_LockSurface(sdl->sdlsurf);
	// Clear if necessary

	if (sdl->blittimer > 0)
	{
		memset(sdl->sdlsurf->pixels, 0, window->height * sdl->sdlsurf->pitch);
		sdl->blittimer--;
	}

	
	if (IS_YUV_MODE(window->scale_mode))
	{
		SDL_LockYUVOverlay(sdl->yuvsurf);
		surfptr = sdl->yuvsurf->pixels[0]; // (UINT8 *) sdl->yuv_bitmap;
		pitch = sdl->yuvsurf->pitches[0]; // (UINT8 *) sdl->yuv_bitmap;
	}
	else
		surfptr = (UINT8 *)sdl->sdlsurf->pixels;
#else
	SDL_SelectRenderer(window->window_id);
	{
        Uint32 format;
        int access, w, h;
		
        SDL_QueryTexture(sdl->texture_id, &format, &access, &w, &h);
		SDL_PixelFormatEnumToMasks(format, &bpp, &rmask, &gmask, &bmask, &amask);
		bpp = bpp / 8; /* convert to bytes per pixels */
	}

	SDL_LockTexture(sdl->texture_id, NULL, 1, (void **) &surfptr, &pitch);
	// Clear if necessary
	if (window->scale_mode == VIDEO_SCALE_MODE_NONE)
		if (sdl->blittimer > 0)
		{	
			memset(surfptr, 0, window->height * pitch);
			sdl->blittimer--;
		}

#endif
	// get ready to center the image
	vofs = hofs = 0;
	blitwidth = window->blitwidth;
	blitheight = window->blitheight;

	// figure out what coordinate system to use for centering - in window mode it's always the
	// SDL surface size.  in fullscreen the surface covers all monitors, so center according to
	// the first one only
	if ((window->fullscreen) && (!video_config.switchres))
	{
		ch = window->monitor->center_height;
		cw = window->monitor->center_width;
	}
	else
	{
		ch = window->height;
		cw = window->width;
	}

	// do not crash if the window's smaller than the blit area
	if (blitheight > ch)
	{
		  	blitheight = ch;
	}
	else if (video_config.centerv)
	{
		vofs = (ch - window->blitheight) / 2;
	}

	if (blitwidth > cw)
	{
		  	blitwidth = cw;
	}
	else if (video_config.centerh)
	{
		hofs = (cw - window->blitwidth) / 2;
	}

	sdl->last_hofs = hofs;
	sdl->last_vofs = vofs;
	
	osd_lock_acquire(window->primlist->lock);

	// render to it
	if (!IS_YUV_MODE(window->scale_mode))
	{
		int mamewidth, mameheight;
		
		if (window->scale_mode == VIDEO_SCALE_MODE_NONE || !SDL_VERSION_ATLEAST(1,3,0))
		{
			mamewidth = blitwidth;
			mameheight = blitheight;
			surfptr += ((vofs * pitch) + (hofs * bpp));
		}
		else
		{
			mamewidth = sdl->hw_scale_width;
			mameheight = sdl->hw_scale_height;
		}
		switch (rmask)
		{
			case 0x00ff0000:
				drawsdl_rgb888_draw_primitives(window->primlist->head, surfptr, mamewidth, mameheight, pitch / 4);
				break;

			case 0x000000ff:
				drawsdl_bgr888_draw_primitives(window->primlist->head, surfptr, mamewidth, mameheight, pitch / 4);
				break;

			case 0xf800:
				drawsdl_rgb565_draw_primitives(window->primlist->head, surfptr, mamewidth, mameheight, pitch / 2);
				break;

			case 0x7c00:
				drawsdl_rgb555_draw_primitives(window->primlist->head, surfptr, mamewidth, mameheight, pitch / 2);
				break;

			default:
				mame_printf_error("SDL: ERROR! Unknown video mode: R=%08X G=%08X B=%08X\n", rmask, gmask, bmask);
				break;
		}
	} 
	else
	{
		assert (sdl->yuv_bitmap != NULL);
		assert (surfptr != NULL);
		drawsdl_rgb555_draw_primitives(window->primlist->head, sdl->yuv_bitmap, sdl->hw_scale_width, sdl->hw_scale_height, sdl->hw_scale_width);
		switch (window->scale_mode)
		{
			case VIDEO_SCALE_MODE_YV12: 
				yuv_RGB_to_YV12((UINT16 *)sdl->yuv_bitmap, sdl, surfptr, pitch);
				break;
			case VIDEO_SCALE_MODE_YV12X2: 
				yuv_RGB_to_YV12X2((UINT16 *)sdl->yuv_bitmap, sdl, surfptr, pitch);
				break;
			case VIDEO_SCALE_MODE_YUY2: 
				yuv_RGB_to_YUY2((UINT16 *)sdl->yuv_bitmap, sdl, surfptr, pitch);
				break;
			case VIDEO_SCALE_MODE_YUY2X2: 
				yuv_RGB_to_YUY2X2((UINT16 *)sdl->yuv_bitmap, sdl, surfptr, pitch);
				break;
		}
	}

	osd_lock_release(window->primlist->lock);

	// unlock and flip
#if (!SDL_VERSION_ATLEAST(1,3,0))
	if (SDL_MUSTLOCK(sdl->sdlsurf)) SDL_UnlockSurface(sdl->sdlsurf);
	if (!IS_YUV_MODE(window->scale_mode))
	{
		SDL_Flip(sdl->sdlsurf);
	}
	else
	{
		SDL_Rect r;

		SDL_UnlockYUVOverlay(sdl->yuvsurf);
		r.x=hofs;
		r.y=vofs;
		r.w=blitwidth;
		r.h=blitheight;
		SDL_DisplayYUVOverlay(sdl->yuvsurf, &r);
	}
#else
	SDL_UnlockTexture(sdl->texture_id);
	if (window->scale_mode == VIDEO_SCALE_MODE_NONE)
	{
		//SDL_UpdateTexture(sdl->sdltex, NULL, sdl->sdlsurf->pixels, pitch);
		SDL_RenderCopy(sdl->texture_id, NULL, NULL);
		SDL_RenderPresent();
	}
	else
	{
		SDL_Rect r;
		r.x=hofs;
		r.y=vofs;
		r.w=blitwidth;
		r.h=blitheight;
		//SDL_UpdateTexture(sdl->sdltex, NULL, sdl->sdlsurf->pixels, pitch);
		SDL_RenderCopy(sdl->texture_id, NULL, &r);
		SDL_RenderPresent();
	}
#endif
	return 0;
}

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

static void yuv_lookup_set(sdl_info *sdl, unsigned int pen, unsigned char red, 
			unsigned char green, unsigned char blue)
{
	UINT32 y,u,v;

	RGB2YUV(red,green,blue,y,u,v);

	/* Storing this data in YUYV order simplifies using the data for
	   YUY2, both with and without smoothing... */
	sdl->yuv_lookup[pen]=(y<<Y1SHIFT)|(u<<USHIFT)|(y<<Y2SHIFT)|(v<<VSHIFT);
}

static void drawsdl_yuv_init(sdl_info *sdl)
{
	unsigned char r,g,b;
	if (sdl->yuv_lookup == NULL)
		sdl->yuv_lookup = malloc_or_die(65536*sizeof(UINT32));
	for (r = 0; r < 32; r++)
		for (g = 0; g < 32; g++)
			for (b = 0; b < 32; b++)
			{
				int idx = (r << 10) | (g << 5) | b;
				yuv_lookup_set(sdl, idx, 
					(r << 3) | (r >> 2),
					(g << 3) | (g >> 2),
					(b << 3) | (b >> 2));
			}
}

static void yuv_RGB_to_YV12(UINT16 *bitmap, sdl_info *sdl, UINT8 *ptr, int pitch)
{
	int x, y;
	UINT8 *dest_y;
	UINT8 *dest_u;
	UINT8 *dest_v;
	UINT16 *src; 
	UINT16 *src2;
	UINT32 *lookup = sdl->yuv_lookup;
	UINT8 *pixels[3];
	int u1,v1,y1,u2,v2,y2,u3,v3,y3,u4,v4,y4;	  /* 12 */
    
	pixels[0] = ptr;
	pixels[1] = ptr + pitch * sdl->hw_scale_height;
	pixels[2] = pixels[1] + pitch * sdl->hw_scale_height / 4;
		
	for(y=0;y<sdl->hw_scale_height;y+=2)
	{
		src=bitmap + (y * sdl->hw_scale_width) ;
		src2=src + sdl->hw_scale_width;

		dest_y = pixels[0] + y * pitch;
		dest_v = pixels[1] + (y>>1) * pitch / 2;
		dest_u = pixels[2] + (y>>1) * pitch / 2;

		for(x=0;x<sdl->hw_scale_width;x+=2)
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
			dest_y[x+pitch] = y3;
			dest_y[x+1] = y2;
			dest_y[x+pitch+1] = y4;

			dest_u[x>>1] = (u1+u2+u3+u4)/4;
			dest_v[x>>1] = (v1+v2+v3+v4)/4;
			
		}
	}
}

static void yuv_RGB_to_YV12X2(UINT16 *bitmap, sdl_info *sdl, UINT8 *ptr, int pitch)
{		
	/* this one is used when scale==2 */
	unsigned int x,y;
	UINT16 *dest_y;
	UINT8 *dest_u;
	UINT8 *dest_v;
	UINT16 *src;
	int u1,v1,y1;
	UINT8 *pixels[3];

	pixels[0] = ptr;
	pixels[1] = ptr + pitch * sdl->hw_scale_height * 2;
	pixels[2] = pixels[1] + pitch * sdl->hw_scale_height / 2;

	for(y=0;y<sdl->hw_scale_height;y++)
	{
		src = bitmap + (y * sdl->hw_scale_width) ;

		dest_y = (UINT16 *)(pixels[0] + 2 * y * pitch);
		dest_v = pixels[1] + y * pitch / 2;
		dest_u = pixels[2] + y * pitch / 2;
		for(x=0;x<sdl->hw_scale_width;x++)
		{
			v1 = sdl->yuv_lookup[src[x]];
			y1 = (v1 >> Y1SHIFT) & 0xff;
			u1 = (v1 >> USHIFT)  & 0xff;
			v1 = (v1 >> VSHIFT)  & 0xff;

			dest_y[x + pitch/2] = y1 << 8 | y1;
			dest_y[x] = y1 << 8 | y1;
			dest_u[x] = u1;
			dest_v[x] = v1;
		}
	}
}

static void yuv_RGB_to_YUY2(UINT16 *bitmap, sdl_info *sdl, UINT8 *ptr, int pitch)
{		
	/* this one is used when scale==2 */
	unsigned int y;
	UINT32 *dest;
	UINT16 *src;
	UINT16 *end;
	UINT32 p1,p2,uv;
	UINT32 *lookup = sdl->yuv_lookup;
	int yuv_pitch = pitch/4;

	for(y=0;y<sdl->hw_scale_height;y++)
	{
		src=bitmap + (y * sdl->hw_scale_width) ;
		end=src+sdl->hw_scale_width;

		dest = (UINT32 *) ptr;
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

static void yuv_RGB_to_YUY2X2(UINT16 *bitmap, sdl_info *sdl, UINT8 *ptr, int pitch)
{		
	/* this one is used when scale==2 */
	unsigned int y;
	UINT32 *dest;
	UINT16 *src;
	UINT16 *end;
	UINT32 *lookup = sdl->yuv_lookup;
	int yuv_pitch = pitch / 4;

	for(y=0;y<sdl->hw_scale_height;y++)
	{
		src=bitmap + (y * sdl->hw_scale_width) ;
		end=src+sdl->hw_scale_width;

		dest = (UINT32 *) ptr;
		dest += (y * yuv_pitch);
		for(; src<end; src++)
		{
			dest[0] = lookup[src[0]];
			dest++;
		}
	}
}
