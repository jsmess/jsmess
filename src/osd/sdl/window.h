//============================================================
//
//  window.h - SDL window handling
//
//  Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//  SDLMAME by Olivier Galibert and R. Belmont
//
//============================================================

#ifndef __SDL_WINDOW__
#define __SDL_WINDOW__

#include <SDL/SDL.h>
#include "video.h"
#include "render.h"


//============================================================
//  PARAMETERS
//============================================================

#ifndef MESS
#define HAS_WINDOW_MENU			FALSE
#else
#define HAS_WINDOW_MENU			TRUE
#endif

// I don't like this, but we're going to get spurious "cast to integer of different size" warnings on
// at least one architecture without doing it this way.
#ifdef PTR64
typedef UINT64 HashT;
#else
typedef UINT32 HashT;
#endif

//============================================================
//  TYPE DEFINITIONS
//============================================================

/* texture_info holds information about a texture */
typedef struct _texture_info texture_info;
struct _texture_info
{
	texture_info *			next;				// next texture in the list
	HashT				hash;				// hash value for the texture (must be >= pointer size)
	UINT32				flags;				// rendering flags
	render_texinfo			texinfo;			// copy of the texture info
	float				ustart, ustop;			// beginning/ending U coordinates
	float				vstart, vstop;			// beginning/ending V coordinates
	int				rawwidth, rawheight;		// raw width/height of the texture
	int				type;				// what type of texture are we?
	int				format;				// texture format
	int				borderpix;			// do we have a 1 pixel border?
	int				xprescale;			// what is our X prescale factor?
	int				yprescale;			// what is our Y prescale factor?
	int				prescale_effect;		// which prescale effect (if any) to use
	int				uploadedonce;			// were we uploaded once already?
	int				nocopy;				// must the texture date be copied?

	UINT32				texturename;			// OpenGL texture "name"/ID

	#ifdef OGL_PIXELBUFS
	UINT32				pbo;				// pixel buffer object for this texture (if DYNAMIC only!)
	#endif

	UINT32				*data;				// pixels for the texture
	UINT32				*effectbuf;			// buffer for intermediate effect results or NULL
};

typedef struct _sdl_window_info sdl_window_info;
struct _sdl_window_info
{
	sdl_window_info *	next;

	// window handle and info
	char				title[256];
	int				opengl;
	UINT32				extra_flags;

	// monitor info
	sdl_monitor_info *	monitor;
	int					fullscreen;
	int				minwidth, minheight;
	int					maxwidth, maxheight;
	int					depth;
	int					refresh;
	int				windowed_width;
	int				windowed_height;

	// rendering info
	osd_lock *			render_lock;
	render_target *		target;
	const render_primitive_list *primlist;

	// drawing data
	void *				dxdata;

	// SDL surface
	SDL_Surface 			*sdlsurf;

	// YUV overlay
	SDL_Overlay 			*yuvsurf;
	int				yuv_ovl_width;
	int				yuv_ovl_height;
	UINT32 				*yuv_lookup;
	UINT16				*yuv_bitmap;

	// threading stuff (multithread mode only)
	#if 0
	volatile INT32	  		command;		// command for the thread to execute
	UINT32				parm1, parm2;		// parameters for command
	SDL_cond			*cmdcond;		// signalled when "command" is valid
	SDL_mutex			*cmdlock;		// held while the window thread is busy (command executing)
	SDL_Thread			*wndthread;		// window thread ptr
	#endif
};

/* sdl_info is the information about SDL for the current screen */
typedef struct _sdl_info sdl_info;
struct _sdl_info
{
	INT32				blitwidth, blitheight;	// current blit width/height values

	// 3D info (GL mode only)
	texture_info *			texlist;		// list of active textures
	int				last_blendmode;		// previous blendmode
	INT32	   			texture_max_width;     	// texture maximum width
	INT32	   			texture_max_height;    	// texture maximum height
	int				forcepoweroftwo;	// must textures be power-of-2 sized?
	int				usepbo;			// runtime check if PBO is available
	int				usetexturerect;		// use ARB_texture_rectangle for non-power-of-2
};

typedef struct _sdl_draw_callbacks sdl_draw_callbacks;
struct _sdl_draw_callbacks
{
	void (*exit)(void);

	int (*window_init)(sdl_window_info *window);
	const render_primitive_list *(*window_get_primitives)(sdl_window_info *window);
	int (*window_draw)(sdl_window_info *window, UINT32 dc, int update);
	void (*window_destroy)(sdl_window_info *window);
};



//============================================================
//  GLOBAL VARIABLES
//============================================================

// windows
extern sdl_window_info *sdl_window_list;



//============================================================
//  PROTOTYPES
//============================================================

// core initialization
int sdlwindow_init(running_machine *machine);

// creation/deletion of windows
int sdlwindow_video_window_create(int index, sdl_monitor_info *monitor, const sdl_window_config *config);

void sdlwindow_update_cursor_state(void);
void sdlwindow_video_window_update(sdl_window_info *window);

void sdlwindow_toggle_full_screen(void);
void sdlwindow_modify_prescale(int dir);
void sdlwindow_modify_effect(int dir);
void sdlwindow_toggle_draw(void);

void sdlwindow_process_events_periodic(void);
void sdlwindow_process_events(int ingame);

#if HAS_WINDOW_MENU
//int sdl_create_menu(HMENU *menus);
#endif

//============================================================
//  win_has_menu
//============================================================

INLINE int sdl_has_menu(sdl_window_info *window)
{
	return FALSE;
}

#endif
