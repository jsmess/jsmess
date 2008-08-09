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

#ifndef __SDLWINDOW__
#define __SDLWINDOW__

#include <SDL/SDL.h>
#include "video.h"
#include "render.h"
#include "sdlsync.h"

#include "osd_opengl.h"

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

typedef struct _texture_info texture_info;

#if USE_OPENGL
typedef void (*texture_copy_func)(texture_info *texture, const render_texinfo *texsource);
#endif
	
/* texture_info holds information about a texture */
struct _texture_info
{
	texture_info *		next;				// next texture in the list
	HashT				hash;				// hash value for the texture (must be >= pointer size)
	UINT32				flags;				// rendering flags
	render_texinfo		texinfo;			// copy of the texture info
	int					rawwidth, rawheight;	// raw width/height of the texture
	int					rawwidth_create;	// raw width/height, pow2 compatible, if needed
	int                 rawheight_create;	// (create and initial set the texture, not for copy!)
	int					type;				// what type of texture are we?
	int					format;				// texture format
	int					borderpix;			// do we have a 1 pixel border?
	int					xprescale;			// what is our X prescale factor?
	int					yprescale;			// what is our Y prescale factor?
	int					prescale_effect;	// which prescale effect (if any) to use
	int					nocopy;				// must the texture date be copied?
	
	UINT32				texture;			// OpenGL texture "name"/ID
#if USE_OPENGL
	const GLint *		texProperties;		// texture properties
	texture_copy_func	texCopyFn;			// texture copy function, !=NULL if !nocopy
	GLenum				texTarget;			// OpenGL texture target
	int					texpow2;			// Is this texture pow2
	
	UINT32				mpass_dest_idx;			// Multipass dest idx [0..1]
	UINT32				mpass_textureunit[2];	// texture unit names for GLSL
	
	UINT32				mpass_texture_mamebm[2];// Multipass OpenGL texture "name"/ID for the shader
	UINT32				mpass_fbo_mamebm[2];	// framebuffer object for this texture, multipass
	UINT32				mpass_texture_scrn[2];	// Multipass OpenGL texture "name"/ID for the shader
	UINT32				mpass_fbo_scrn[2];		// framebuffer object for this texture, multipass
	
	UINT32				lut_texture;			// LUT OpenGL texture "name"/ID for the shader
	int					lut_table_width;		// LUT table width 
	int					lut_table_height;		// LUT table height
	
	UINT32				pbo;					// pixel buffer object for this texture (DYNAMIC only!)
	UINT32				*data;					// pixels for the texture
	int					data_own;				// do we own / allocated it ?
	UINT32				*effectbuf;				// buffer for intermediate effect results or NULL
	GLfloat				texCoord[8];
	GLuint				texCoordBufferName;
#endif
};

typedef struct _sdl_window_info sdl_window_info;
struct _sdl_window_info
{
	// Pointer to next window
	sdl_window_info *	next;
	
	// Draw Callbacks
	int (*create)(sdl_window_info *window, int width, int height);
	void (*resize)(sdl_window_info *window, int width, int height);
	int (*draw)(sdl_window_info *window, UINT32 dc, int update);
	const render_primitive_list *(*get_primitives)(sdl_window_info *window);
	int (*xy_to_render_target)(sdl_window_info *window, int x, int y, int *xt, int *yt);
	void (*destroy_all_textures)(sdl_window_info *window);
	void (*destroy)(sdl_window_info *window);
	void (*clear)(sdl_window_info *window);

	// window handle and info
	char				title[256];

	// monitor info
	sdl_monitor_info *	monitor;
	int					fullscreen;
	
	// diverse flags
	int					minwidth, minheight;
	int					maxwidth, maxheight;
	int					depth;
	int					refresh;
	int					windowed_width;
	int					windowed_height;
	int					startmaximized;

	// rendering info
	osd_event *			rendered_event;
	render_target *		target;
	const render_primitive_list *primlist;

	// drawing data
	void *				dxdata;

	// cache of physical width and height
	int					width;
	int					height;

	// current blitwidth and height
	int					blitwidth;
	int					blitheight;

	int					totalColors;		// total colors from machine/sdl_window_config
	int					start_viewscreen;
	
	// per window modes ...
	int					scale_mode;
	
	// GL specific
	int					prescale;
	int					prescale_effect;
	
#if (SDL_VERSION_ATLEAST(1,3,0))
	// Needs to be here as well so we can identify window
	SDL_WindowID		window_id;
#endif
};

typedef struct _sdl_draw_info sdl_draw_info;
struct _sdl_draw_info
{
	void (*exit)(void);
	void (*attach)(sdl_draw_info *info, sdl_window_info *window);
};

//============================================================
//  GLOBAL VARIABLES
//============================================================

// window - list
extern sdl_window_info *sdl_window_list;

//============================================================
//  PROTOTYPES
//============================================================

// core initialization
int sdlwindow_init(running_machine *machine);

// creation/deletion of windows
int sdlwindow_video_window_create(running_machine *machine, int index, sdl_monitor_info *monitor, const sdl_window_config *config);
void sdlwindow_video_window_update(running_machine *machine, sdl_window_info *window);
void sdlwindow_blit_surface_size(sdl_window_info *window, int window_width, int window_height);
void sdlwindow_toggle_full_screen(running_machine *machine, sdl_window_info *window);
void sdlwindow_modify_prescale(running_machine *machine, sdl_window_info *window, int dir);
void sdlwindow_modify_effect(running_machine *machine, sdl_window_info *window, int dir);
void sdlwindow_toggle_draw(running_machine *machine, sdl_window_info *window);
void sdlwindow_resize(sdl_window_info *window, INT32 width, INT32 height);

//============================================================
// PROTOTYPES - drawsdl.c
//============================================================

int drawsdl_init(sdl_draw_info *callbacks);
const char *drawsdl_scale_mode_str(int index);
int drawsdl_scale_mode(const char *s);

//============================================================
// PROTOTYPES - drawsdl.c
//============================================================

int drawogl_init(sdl_draw_info *callbacks);

#endif /* __SDLWINDOW__ */
