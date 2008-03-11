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
//  INLINES
//============================================================

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

// soft rendering
static void drawsdl_rgb888_draw_primitives(const render_primitive *primlist, void *dstdata, UINT32 width, UINT32 height, UINT32 pitch);
static void drawsdl_bgr888_draw_primitives(const render_primitive *primlist, void *dstdata, UINT32 width, UINT32 height, UINT32 pitch);
static void drawsdl_rgb565_draw_primitives(const render_primitive *primlist, void *dstdata, UINT32 width, UINT32 height, UINT32 pitch);
static void drawsdl_rgb555_draw_primitives(const render_primitive *primlist, void *dstdata, UINT32 width, UINT32 height, UINT32 pitch);

// YUV overlays

static void drawsdl_yuv_init(sdl_window_info *window);
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
	window->yuv_lookup = NULL;
	window->yuv_bitmap = NULL;
	window->initialized = 0;
	sdl->totalColors = window->totalColors;

	drawsdl_yuv_init(window);
	return 0;
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
		sdlwindow_blit_surface_size(window, window->sdlsurf->w, window->sdlsurf->h);
	}
	else
	{
		sdlwindow_blit_surface_size(window, window->monitor->center_width, window->monitor->center_height);
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

static void yuv_lookup_set(sdl_window_info *window, unsigned int pen, unsigned char red, 
			unsigned char green, unsigned char blue)
{
	UINT32 y,u,v;

	RGB2YUV(red,green,blue,y,u,v);

	/* Storing this data in YUYV order simplifies using the data for
	   YUY2, both with and without smoothing... */
	window->yuv_lookup[pen]=(y<<Y1SHIFT)|(u<<USHIFT)|(y<<Y2SHIFT)|(v<<VSHIFT);
}

static void drawsdl_yuv_init(sdl_window_info *window)
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
{		
	/* this one is used when scale==2 */
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
{		
	/* this one is used when scale==2 */
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
{		
	/* this one is used when scale==2 */
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
