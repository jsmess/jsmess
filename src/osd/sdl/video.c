//============================================================
//
//  video.c - SDL video handling
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//  SDLMAME by Olivier Galibert and R. Belmont
//
//============================================================

#include <SDL/SDL.h>
// on win32 this includes windows.h by itself and breaks us!
#ifndef SDLMAME_WIN32
#include <SDL/SDL_syswm.h>
#endif

#if defined(SDLMAME_LINUX) || defined(SDLMAME_FREEBSD)
#include <X11/extensions/Xinerama.h>
#endif

#ifdef SDLMAME_MACOSX
#include <Carbon/Carbon.h>
#endif

#ifdef SDLMAME_WIN32
// for multimonitor
#define _WIN32_WINNT 0x501
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// standard C headers
#include <math.h>
#include <unistd.h>

// MAME headers
#include "osdepend.h"
#include "driver.h"
#include "profiler.h"
#include "video/vector.h"
#include "render.h"
#include "rendutil.h"
#include "options.h"
#include "ui.h"
#include "debugwin.h"

// MAMEOS headers
#include "video.h"
#include "window.h"
#include "input.h"
#include "options.h"

#ifdef MESS
#include "menu.h"
#endif

//============================================================
//  IMPORTS
//============================================================

//============================================================
//  DEBUGGING
//============================================================

#define LOG_THROTTLE				(1)



//============================================================
//  CONSTANTS
//============================================================

// refresh rate while paused
#define PAUSED_REFRESH_RATE			(60)


//============================================================
//  TYPE DEFINITIONS
//============================================================

typedef struct _throttle_info throttle_info;
struct _throttle_info
{
	osd_ticks_t 		last_ticks;				/* last osd_ticks() value we read */
	mame_time 		realtime;				/* current realtime */
	mame_time 		emutime;				/* current emulated time */
	UINT32			history;				/* history of in-time frames */
};


//============================================================
//  GLOBAL VARIABLES
//============================================================

sdl_video_config video_config;

//============================================================
//  LOCAL VARIABLES
//============================================================

sdl_monitor_info *sdl_monitor_list;
static sdl_monitor_info *primary_monitor;

static mame_bitmap *effect_bitmap;

//============================================================
//  PROTOTYPES
//============================================================

static void video_exit(running_machine *machine);
static void init_monitors(void);
static sdl_monitor_info *pick_monitor(int index);

static void check_osd_inputs(void);

static void extract_video_config(void);
static void load_effect_overlay(const char *filename);
static float get_aspect(const char *name, int report_error);
static void get_resolution(const char *name, sdl_window_config *config, int report_error);



//============================================================
//  INLINES
//============================================================

INLINE int effective_autoframeskip(void)
{
	return video_config.autoframeskip && !video_config.fastforward;
}


INLINE int effective_frameskip(void)
{
	return video_config.fastforward ? (FRAMESKIP_LEVELS - 1) : video_config.frameskip;
}


INLINE int effective_throttle(void)
{
	return !video_config.fastforward && (video_config.throttle || mame_is_paused(Machine) || ui_is_menu_active() || ui_is_slider_active());
}

//============================================================
//  sdlvideo_init
//============================================================

int sdlvideo_init(running_machine *machine)
{
	int index;

	// ensure we get called on the way out
	add_exit_callback(machine, video_exit);

	// extract data from the options
	extract_video_config();

	// set up monitors first
	init_monitors();

	// we need the beam width in a float, contrary to what the core does.
	video_config.beamwidth = options_get_float("beam");

	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
	{
		video_config.isvector = 1;
	}
	else
	{
		video_config.isvector = 0;
	}

	// initialize the window system so we can make windows
	if (sdlwindow_init(machine))
		goto error;

	// create the windows
	for (index = 0; index < video_config.numscreens; index++)
		if (sdlwindow_video_window_create(index, pick_monitor(index), &video_config.window[index]))
			goto error;


	return 0;

error:
	return 1;
}


//============================================================
//  video_exit
//============================================================

static void video_exit(running_machine *machine)
{
	// free the overlay effect
	if (effect_bitmap != NULL)
		bitmap_free(effect_bitmap);
	effect_bitmap = NULL;

	// free all of our monitor information
	while (sdl_monitor_list != NULL)
	{
		sdl_monitor_info *temp = sdl_monitor_list;
		sdl_monitor_list = temp->next;
		free(temp);
	}
}



//============================================================
//  sdlvideo_monitor_refresh
//============================================================

void sdlvideo_monitor_refresh(sdl_monitor_info *monitor)
{
	#if defined(SDLMAME_WIN32)	// Win32 version
	MONITORINFOEX info;
	info.cbSize = sizeof(info);	
        GetMonitorInfo((HMONITOR)monitor->handle, (LPMONITORINFO)&info);
	monitor->center_width = monitor->monitor_width = info.rcMonitor.right - info.rcMonitor.left;
	monitor->center_height = monitor->monitor_height = info.rcMonitor.bottom - info.rcMonitor.top;
	strcpy(monitor->monitor_device, info.szDevice);
	#elif defined(SDLMAME_MACOSX)	// Mac OS X Core Imaging version
	CGDirectDisplayID primary;
	CGRect dbounds;

	// get the main display
	primary = CGMainDisplayID();
	dbounds = CGDisplayBounds(primary);

	monitor->center_width = monitor->monitor_width = dbounds.size.width - dbounds.origin.x;
	monitor->center_height = monitor->monitor_height = dbounds.size.height - dbounds.origin.y;
	strcpy(monitor->monitor_device, "Mac OS X display");
        #elif defined(SDLMAME_LINUX) || defined(SDLMAME_FREEBSD)        // X11 version
	{
		// X11 version
		int screen;
		SDL_SysWMinfo info;
		SDL_VERSION(&info.version);
		
		if ( SDL_GetWMInfo(&info) && (info.subsystem == SDL_SYSWM_X11) )
		{
			screen = DefaultScreen(info.info.x11.display);
			SDL_VideoDriverName(monitor->monitor_device, sizeof(monitor->monitor_device)-1);
			monitor->monitor_width = DisplayWidth(info.info.x11.display, screen);
			monitor->monitor_height = DisplayHeight(info.info.x11.display, screen);

			if ((XineramaIsActive(info.info.x11.display)) && video_config.restrictonemonitor)
			{
				XineramaScreenInfo *xineinfo;
				int numscreens;

      				xineinfo = XineramaQueryScreens(info.info.x11.display, &numscreens);
			
				monitor->center_width = xineinfo[0].width;
				monitor->center_height = xineinfo[0].height;

				XFree(xineinfo);
			}
			else
			{
				monitor->center_width = monitor->monitor_width;
				monitor->center_height = monitor->monitor_height;
			}
		}
 		else
		{
			static int first_call=0;
			static int cw, ch;
			
			SDL_VideoDriverName(monitor->monitor_device, sizeof(monitor->monitor_device)-1);
			if (first_call==0) 
			{
				char *dimstr = getenv("SDLMAME_DESKTOPDIM");
				const SDL_VideoInfo *sdl_vi;
				
				if ( (video_config.mode == VIDEO_MODE_OPENGL) &&
					(!strcmp(monitor->monitor_device,"directfb") 
					  || !strcmp(monitor->monitor_device,"fbcon") ) )
				{
					printf("WARNING: OpenGL not supported for driver <%s>\n",monitor->monitor_device);
					printf("         Falling back to soft mode\n");
					video_config.mode = VIDEO_MODE_SOFT;
				}
				sdl_vi = SDL_GetVideoInfo();
				#if (SDL_VERSIONNUM(SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL) >= 1210)
				cw = sdl_vi->current_w;
				ch = sdl_vi->current_h;
				#endif
				first_call=1;
				if ((cw==0) || (ch==0))
				{
					if (dimstr)
					{
						sscanf(dimstr, "%dx%d", &cw, &ch);
					}
					if ((cw==0) || (ch==0))
					{
						printf("WARNING: SDL_GetVideoInfo() for driver <%s> is broken.\n", monitor->monitor_device);
						printf("         You should set SDLMAME_DESKTOPDIM to your desktop size.\n");
						printf("            e.g. export SDLMAME_DESKTOPDIM=800x600\n");
						printf("         Assuming 1024x768 now!\n");
						cw=1024;
						ch=768;
					}
				}
				printf("Monitor Dim: %d x %d\n", cw, ch);
			}
			monitor->monitor_width = cw;
			monitor->monitor_height = ch;
			monitor->center_width = cw;
			monitor->center_height = ch;
		}	
	}
	#else
	#error Unknown SDLMAME_xx OS type!
	#endif
}



//============================================================
//  sdlvideo_monitor_get_aspect
//============================================================

float sdlvideo_monitor_get_aspect(sdl_monitor_info *monitor)
{
	// refresh the monitor information and compute the aspect
	if (video_config.keepaspect)
	{
		sdlvideo_monitor_refresh(monitor);
		return monitor->aspect / ((float)monitor->monitor_width / (float)monitor->monitor_height);
	}
	return 0.0f;
}



//============================================================
//  sdlvideo_monitor_from_handle
//============================================================

sdl_monitor_info *sdlvideo_monitor_from_handle(UINT32 hmonitor)
{
	sdl_monitor_info *monitor;

	// find the matching monitor
	for (monitor = sdl_monitor_list; monitor != NULL; monitor = monitor->next)
		if (monitor->handle == hmonitor)
			return monitor;
	return NULL;
}

//============================================================
//  osd_update
//============================================================

void osd_update(int skip_redraw)
{
	sdl_window_info *window;

	// if we're not skipping this redraw, update all windows
	if (!skip_redraw)
	{
		profiler_mark(PROFILER_BLIT);
		for (window = sdl_window_list; window != NULL; window = window->next)
			sdlwindow_video_window_update(window);
		profiler_mark(PROFILER_END);
	}

	// poll the joystick values here
	win_process_events();
	check_osd_inputs();

#ifdef MAME_DEBUG
	debugwin_update_during_game();
#endif
}

//============================================================
//  add_primary_monitor
//============================================================
#ifndef SDLMAME_WIN32
static void add_primary_monitor(void *data)
{
	sdl_monitor_info ***tailptr = (sdl_monitor_info ***)data;
	sdl_monitor_info *monitor;

	// allocate a new monitor info
	monitor = malloc_or_die(sizeof(*monitor));
	memset(monitor, 0, sizeof(*monitor));

	// copy in the data
	monitor->handle = 1;

	sdlvideo_monitor_refresh(monitor);

	// guess the aspect ratio assuming square pixels
	monitor->aspect = (float)(monitor->monitor_width) / (float)(monitor->monitor_height);

	// save the primary monitor handle
	primary_monitor = monitor;

	// hook us into the list
	**tailptr = monitor;
	*tailptr = &monitor->next;
} 
#endif

//============================================================
//  monitor_enum_callback
//============================================================
#ifdef SDLMAME_WIN32
static BOOL CALLBACK monitor_enum_callback(HMONITOR handle, HDC dc, LPRECT rect, LPARAM data)
{
	sdl_monitor_info ***tailptr = (sdl_monitor_info ***)data;
	sdl_monitor_info *monitor;
	MONITORINFOEX info;
	BOOL result;

	// get the monitor info
	info.cbSize = sizeof(info);
	result = GetMonitorInfo(handle, (LPMONITORINFO)&info);
	assert(result);

	// allocate a new monitor info
	monitor = malloc_or_die(sizeof(*monitor));
	memset(monitor, 0, sizeof(*monitor));

	// copy in the data
        monitor->handle = (UINT32)handle;
	monitor->monitor_width = info.rcMonitor.right - info.rcMonitor.left;
	monitor->monitor_height = info.rcMonitor.bottom - info.rcMonitor.top;
	strcpy(monitor->monitor_device, info.szDevice);

	// guess the aspect ratio assuming square pixels
	monitor->aspect = (float)(info.rcMonitor.right - info.rcMonitor.left) / (float)(info.rcMonitor.bottom - info.rcMonitor.top);

	// save the primary monitor handle
	if (info.dwFlags & MONITORINFOF_PRIMARY)
		primary_monitor = monitor;

	// hook us into the list
	**tailptr = monitor;
	*tailptr = &monitor->next;

	// enumerate all the available monitors so to list their names in verbose mode
	return TRUE;
}
#endif

//============================================================
//  init_monitors
//============================================================

static void init_monitors(void)
{
	sdl_monitor_info **tailptr;

	// make a list of monitors
	sdl_monitor_list = NULL;
	tailptr = &sdl_monitor_list;

	#ifdef SDLMAME_WIN32
	EnumDisplayMonitors(NULL, NULL, monitor_enum_callback, (LPARAM)&tailptr);
	#else
	add_primary_monitor((void *)&tailptr);
	#endif
}

//============================================================
//  pick_monitor
//============================================================

static sdl_monitor_info *pick_monitor(int index)
{
	sdl_monitor_info *monitor;
	#if SDL_MULTIMON || defined(SDLMAME_WIN32)
	const char *scrname;
	int moncount = 0;
	#endif
	char option[20];
	float aspect;

	// get the screen option
	#if SDL_MULTIMON || defined(SDLMAME_WIN32)
	sprintf(option, "screen%d", index);
	scrname = options_get_string(option);
	#endif

	// get the aspect ratio
	sprintf(option, "aspect%d", index);
	aspect = get_aspect(option, TRUE);

	// look for a match in the name first
	#if SDL_MULTIMON || defined(SDLMAME_WIN32)
	if (scrname != NULL)
                for (monitor = sdl_monitor_list; monitor != NULL; monitor = monitor->next)
		{
			moncount++;
			if (strcmp(scrname, monitor->monitor_device) == 0)
				goto finishit;
		}

	// didn't find it; alternate monitors until we hit the jackpot
	index %= moncount;
        for (monitor = sdl_monitor_list; monitor != NULL; monitor = monitor->next)
		if (index-- == 0)
			goto finishit;
	#endif

	// return the primary just in case all else fails
	monitor = primary_monitor;

#if SDL_MULTIMON || defined(SDLMAME_WIN32)
finishit:
#endif
	if (aspect != 0)
		monitor->aspect = aspect;
	return monitor;
}

//============================================================
//  check_osd_inputs
//============================================================

static void check_osd_inputs(void)
{
	// check for toggling fullscreen mode
	if (input_ui_pressed(IPT_OSD_1))
		sdlwindow_toggle_full_screen();
	
	if (input_ui_pressed(IPT_OSD_3))
	{
		video_config.fullstretch = !video_config.fullstretch;
		ui_popup_time(1, "Uneven stretch %s", video_config.fullstretch? "enabled":"disabled");
	}
	
	if (input_ui_pressed(IPT_OSD_4))
	{
		video_config.keepaspect = !video_config.keepaspect;
		ui_popup_time(1, "Keepaspect %s", video_config.keepaspect? "enabled":"disabled");
	}
	
	if (input_ui_pressed(IPT_OSD_5))
	{
		video_config.filter = !video_config.filter;
		ui_popup_time(1, "Filter %s", video_config.filter? "enabled":"disabled");
	}
		
	if (input_ui_pressed(IPT_OSD_6))
		sdlwindow_modify_prescale(-1);

	if (input_ui_pressed(IPT_OSD_7))
		sdlwindow_modify_prescale(1);

	if (input_ui_pressed(IPT_OSD_8))
		sdlwindow_modify_effect(-1);

	if (input_ui_pressed(IPT_OSD_9))
		sdlwindow_modify_effect(1);

	if (input_ui_pressed(IPT_OSD_10))
		sdlwindow_toggle_draw();
}



//============================================================
//  extract_video_config
//============================================================

static void extract_video_config(void)
{
	const char *stemp;

	// performance options: extract the data
	video_config.autoframeskip = options_get_bool("autoframeskip");
	video_config.frameskip     = options_get_int("frameskip");
	video_config.throttle      = options_get_bool("throttle");
	video_config.sleep         = options_get_bool("sleep");

	// misc options: extract the data
	video_config.framestorun   = options_get_int("frames_to_run");

	// global options: extract the data
	video_config.windowed      = options_get_bool("window");
	video_config.keepaspect    = options_get_bool("keepaspect");
	video_config.numscreens    = options_get_int("numscreens");
	video_config.fullstretch   = options_get_bool("unevenstretch");
	#ifdef SDLMAME_LINUX
	video_config.restrictonemonitor = !options_get_bool("useallheads");
	#endif


#ifdef MAME_DEBUG
	// if we are in debug mode, never go full screen
	if (options_get_bool(OPTION_DEBUG))
		video_config.windowed = TRUE;
#endif
	stemp                      = options_get_string("effect");
	if (stemp != NULL && strcmp(stemp, "none") != 0)
		load_effect_overlay(stemp);

	// per-window options: extract the data
	get_resolution("resolution0", &video_config.window[0], TRUE);
	get_resolution("resolution1", &video_config.window[1], TRUE);
	get_resolution("resolution2", &video_config.window[2], TRUE);
	get_resolution("resolution3", &video_config.window[3], TRUE);

	// default to working video please
	video_config.novideo = 0;

	// d3d options: extract the data
	stemp = options_get_string("video");
	if (strcmp(stemp, "opengl") == 0)
		video_config.mode = VIDEO_MODE_OPENGL;
	else if (strcmp(stemp, "soft") == 0)
		video_config.mode = VIDEO_MODE_SOFT;
	else if (strcmp(stemp, "none") == 0)
	{
		video_config.mode = VIDEO_MODE_SOFT;
		video_config.novideo = 1;

		if (video_config.framestorun == 0)
			fprintf(stderr, "Warning: -video none doesn't make much sense without -frames_to_run\n");
	}
	else
	{
		fprintf(stderr, "Invalid video value %s; reverting to software\n", stemp);
		video_config.mode = VIDEO_MODE_SOFT;
	}
	
	video_config.switchres     = options_get_bool("switchres");
	video_config.filter        = options_get_bool("filter");
	video_config.centerh       = options_get_bool("centerh");
	video_config.centerv       = options_get_bool("centerv");
	video_config.prescale      = options_get_int_range("prescale", 1, 3);
	if (getenv("SDLMAME_UNSUPPORTED"))
		video_config.prescale_effect = options_get_int_range("prescale_effect", 0, 1);
	else
		video_config.prescale_effect = 0;

	// performance options: sanity check values
	if (video_config.frameskip < 0 || video_config.frameskip > FRAMESKIP_LEVELS)
	{
		fprintf(stderr, "Invalid frameskip value %d; reverting to 0\n", video_config.frameskip);
		video_config.frameskip = 0;
	}

	// misc options: sanity check values

	// global options: sanity check values
	if (video_config.numscreens < 1 || video_config.numscreens > 1) //MAX_VIDEO_WINDOWS)
	{
		fprintf(stderr, "Invalid numscreens value %d; reverting to 1\n", video_config.numscreens);
		video_config.numscreens = 1;
	}

	// yuv settings ...
	stemp = options_get_string("yuvmode");
	if (strcmp(stemp, "none") == 0)
		video_config.yuv_mode = VIDEO_YUV_MODE_NONE;
	else if (strcmp(stemp, "yv12") == 0)
		video_config.yuv_mode = VIDEO_YUV_MODE_YV12;
	else if (strcmp(stemp, "yv12x2") == 0)
		video_config.yuv_mode = VIDEO_YUV_MODE_YV12X2;
	else if (strcmp(stemp, "yuy2") == 0)
		video_config.yuv_mode = VIDEO_YUV_MODE_YUY2;
	else if (strcmp(stemp, "yuy2x2") == 0)
		video_config.yuv_mode = VIDEO_YUV_MODE_YUY2X2;
	else
	{
		fprintf(stderr, "Invalid yuvmode value %s; reverting to none\n", stemp);
		video_config.yuv_mode = VIDEO_YUV_MODE_NONE;
	}
	if ( (video_config.mode != VIDEO_MODE_SOFT) && (video_config.yuv_mode != VIDEO_YUV_MODE_NONE) )
	{
		fprintf(stderr, "yuvmode is only for -video soft, overriding\n");
		video_config.yuv_mode = VIDEO_YUV_MODE_NONE;
	}

}


//============================================================
//  load_effect_overlay
//============================================================

static void load_effect_overlay(const char *filename)
{
	char *tempstr = malloc_or_die(strlen(filename) + 5);
	int scrnum;
	char *dest;

	// append a .PNG extension
	strcpy(tempstr, filename);
	dest = strrchr(tempstr, '.');
	if (dest == NULL)
		dest = &tempstr[strlen(tempstr)];
	strcpy(dest, ".png");

	// load the file
	effect_bitmap = render_load_png(NULL, tempstr, NULL, NULL);
	if (effect_bitmap == NULL)
	{
		fprintf(stderr, "Unable to load PNG file '%s'\n", tempstr);
		free(tempstr);
		return;
	}

	// set the overlay on all screens
	for (scrnum = 0; scrnum < MAX_SCREENS; scrnum++)
		if (Machine->drv->screen[scrnum].tag != NULL)
			render_container_set_overlay(render_container_get_screen(scrnum), effect_bitmap);

	free(tempstr);
}


//============================================================
//  get_aspect
//============================================================

static float get_aspect(const char *name, int report_error)
{
	const char *data = options_get_string(name);
	int num = 0, den = 1;

	if (strcmp(data, "auto") == 0)
		return 0;
	else if (sscanf(data, "%d:%d", &num, &den) != 2 && report_error)
		fprintf(stderr, "Illegal aspect ratio value for %s = %s\n", name, data);
	return (float)num / (float)den;
}



//============================================================
//  get_resolution
//============================================================

static void get_resolution(const char *name, sdl_window_config *config, int report_error)
{
	const char *data = options_get_string(name);

	config->width = config->height = config->depth = config->refresh = 0;
	if (strcmp(data, "auto") == 0)
		return;
	else if (sscanf(data, "%dx%dx%d@%d", &config->width, &config->height, &config->depth, &config->refresh) < 2 && report_error)
		fprintf(stderr, "Illegal resolution value for %s = %s\n", name, data);
}
