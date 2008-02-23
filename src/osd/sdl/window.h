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

#include "osd_opengl.h"

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
	sdl_window_info *	next;

	// window handle and info
	char				title[256];
	UINT32				extra_flags;

	// monitor info
	sdl_monitor_info *	monitor;
	int					fullscreen;
	int					minwidth, minheight;
	int					maxwidth, maxheight;
	int					depth;
	int					refresh;
	int					windowed_width;
	int					windowed_height;

	// rendering info
	osd_lock *			render_lock;
	render_target *		target;
	const render_primitive_list *primlist;

	// drawing data
	void *				dxdata;

	// SDL surface
	SDL_Surface 		*sdlsurf;

	// YUV overlay
	SDL_Overlay 		*yuvsurf;
	int					yuv_ovl_width;
	int					yuv_ovl_height;
	UINT32 				*yuv_lookup;
	UINT16				*yuv_bitmap;

	int					totalColors;		// total colors from machine/sdl_window_config
	int					initialized;		// is everything well initialized, i.e. all GL stuff etc.
	int					start_viewscreen;

};

/* sdl_info is the information about SDL for the current screen */
typedef struct _sdl_info sdl_info;
struct _sdl_info
{
	INT32				blitwidth, blitheight;	// current blit width/height values

#if USE_OPENGL
	// 3D info (GL mode only)
	texture_info *	texlist;		// list of active textures
	int				last_blendmode;		// previous blendmode
	INT32	   		texture_max_width;     	// texture maximum width
	INT32	   		texture_max_height;    	// texture maximum height
	int				texpoweroftwo;	        // must textures be power-of-2 sized?
	int				usevbo;			// runtime check if VBO is available
	int				usepbo;			// runtime check if PBO is available
	int				usefbo;			// runtime check if FBO is available
	int				useglsl;		// runtime check if GLSL is available
	GLhandleARB		glsl_program[2*GLSL_SHADER_MAX];  // GLSL programs, or 0
	int				glsl_program_num;	// number of GLSL programs
	int				glsl_program_mb2sc;	// GLSL program idx, which transforms 
			                            // the mame-bitmap -> screen-bitmap (size/rotation/..)
										// All progs <= glsl_program_mb2sc using the mame bitmap
										// as input, otherwise the screen bitmap.
										// All progs >= glsl_program_mb2sc using the screen bitmap
										// as output, otherwise the mame bitmap.
	int				glsl_vid_attributes;// glsl brightness, contrast and gamma for RGB bitmaps
	int				usetexturerect;		// use ARB_texture_rectangle for non-power-of-2, general use
#endif

	int				totalColors;		// total colors from machine/sdl_window_config/sdl_window_info
};

typedef struct _sdl_draw_callbacks sdl_draw_callbacks;
struct _sdl_draw_callbacks
{
	void (*exit)(void);

	int (*window_init)(sdl_window_info *window);
	const render_primitive_list *(*window_get_primitives)(sdl_window_info *window);
	int (*window_draw)(sdl_window_info *window, UINT32 dc, int update);
	void (*window_destroy_all_textures)(sdl_window_info *window);
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
int sdlwindow_video_window_create(running_machine *machine, int index, sdl_monitor_info *monitor, const sdl_window_config *config);

void sdlwindow_video_window_update(sdl_window_info *window);

void sdlwindow_blit_surface_size(sdl_window_info *window, int window_width, int window_height);

void sdlwindow_toggle_full_screen(sdl_window_info *window);
void sdlwindow_modify_prescale(sdl_window_info *window, int dir);
void sdlwindow_modify_effect(sdl_window_info *window, int dir);
void sdlwindow_toggle_draw(sdl_window_info *window);
void sdlwindow_resize(sdl_window_info *window, INT32 width, INT32 height);

#if HAS_WINDOW_MENU
//int sdl_create_menu(HMENU *menus);
#endif

//============================================================
// PROTOTYPES - drawsdl.c
//============================================================

int drawsdl_init(sdl_draw_callbacks *callbacks);
int drawogl_init(sdl_draw_callbacks *callbacks);

//FIXME: This should go into callback!
void drawogl_init_ogl_context(void);

#endif /* __SDL_WINDOW__ */
