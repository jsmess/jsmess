//============================================================
//
//  video.h - SDL implementation of MAME video routines
//
//  Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//  SDLMAME by Olivier Galibert and R. Belmont
//
//============================================================

#ifndef __SDL_VIDEO__
#define __SDL_VIDEO__

#include "osd_cpu.h"
#include "mamecore.h"

//============================================================
//  CONSTANTS
//============================================================

#define MAX_VIDEO_WINDOWS   	(4)

#define VIDEO_MODE_SOFT		0
#define VIDEO_MODE_OPENGL	1

#define SDL_MULTIMON	(0)

#define VIDEO_YUV_MODE_NONE	0
#define VIDEO_YUV_MODE_YV12	1
#define VIDEO_YUV_MODE_YV12X2	2
#define VIDEO_YUV_MODE_YUY2	3
#define VIDEO_YUV_MODE_YUY2X2	4
#define VIDEO_YUV_MODE_MAX	VIDEO_YUV_MODE_YUY2X2
#define VIDEO_YUV_MODE_MIN	VIDEO_YUV_MODE_NONE


// lots of texture formats, we need atleast 2 different 16 bpp dest formats.
// Because not all cards can handle all 16bpp formats efficiently, I've opted
// to add 4 possible dest formats for 16 bpp srcs for the largest flexibility.
// This used to be an enum, but these are now defines so we can use them as
// preprocessor conditionals
#define SDL_TEXFORMAT_ARGB32			 0	// non-16-bit textures or specials
#define SDL_TEXFORMAT_RGB32			 1
#define SDL_TEXFORMAT_RGB32_PALETTED		 2
#define SDL_TEXFORMAT_YUY16			 3
#define SDL_TEXFORMAT_YUY16_PALETTED		 4
#define SDL_TEXFORMAT_PALETTE16			 5
#define SDL_TEXFORMAT_RGB15			 6
#define SDL_TEXFORMAT_RGB15_PALETTED		 7
#define SDL_TEXFORMAT_PALETTE16A		 8

//============================================================
//  TYPE DEFINITIONS
//============================================================

typedef struct _sdl_monitor_info sdl_monitor_info;
struct _sdl_monitor_info
{
	sdl_monitor_info  *	next;					// pointer to next monitor in list
	UINT32			handle;					// handle to the monitor
	int			monitor_width;
	int			monitor_height;
	char			monitor_device[64];
	float			aspect;					// computed/configured aspect ratio of the physical device
	int			reqwidth;				// requested width for this monitor
	int			reqheight;				// requested height for this monitor
	int			center_width;				// width of first physical screen for centering
	int			center_height;				// height of first physical screen for centering
};


typedef struct _sdl_window_config sdl_window_config;
struct _sdl_window_config
{
	float				aspect;						// decoded aspect ratio
	int					width;						// decoded width
	int					height;						// decoded height
	int					depth;						// decoded depth
	int					refresh;					// decoded refresh
};


typedef struct _sdl_video_config sdl_video_config;
struct _sdl_video_config
{
	// performance options
	int					fastforward;				// fast forward?
	int					autoframeskip;				// autoframeskip?
	int					frameskip;					// explicit frameskip
	int					throttle;					// throttle speed?
	int					sleep;						// allow sleeping?
	int					novideo;				// don't draw, for pure CPU benchmarking

	// misc options
	int					framestorun;				// number of frames to run

	// global configuration
	int					windowed;	 			// start windowed?
	int					prescale;				// prescale factor (not currently supported)
	int					keepaspect;	 			// keep aspect ratio?
	int					numscreens;	 			// number of screens
	int					layerconfig;				// default configuration of layers
	int					centerh;
	int					centerv;


	// per-window configuration
	sdl_window_config	window[MAX_VIDEO_WINDOWS];		// configuration data per-window

	// hardware options
	int					mode;						// output mode
	int					waitvsync;					// spin until vsync
	int					syncrefresh;				// sync only to refresh rate
	int					triplebuf;					// triple buffer
	int					switchres;					// switch resolutions

	int					fullstretch;

	// ddraw options
	int					hwstretch;					// stretch using the hardware

	// d3d options
	int					filter;						// enable filtering
	int					prescale_effect;

	// vector options
	float					beamwidth;				// beam width

	// X11 options
	int					restrictonemonitor;			// in fullscreen, confine to Xinerama monitor 0

	// YUV options	
	int					yuv_mode;
};



//============================================================
//  GLOBAL VARIABLES
//============================================================

extern sdl_monitor_info *sdl_monitor_list;

extern int video_orientation;


extern sdl_video_config video_config;


//============================================================
//  PROTOTYPES
//============================================================

int sdlvideo_init(running_machine *machine);

void sdlvideo_monitor_refresh(sdl_monitor_info *monitor);
float sdlvideo_monitor_get_aspect(sdl_monitor_info *monitor);
sdl_monitor_info *sdlvideo_monitor_from_handle(UINT32 monitor);

void sdlvideo_set_frameskip(int frameskip);		// <0 = auto
int sdlvideo_get_frameskip(void);	   		// <0 = auto

#endif
