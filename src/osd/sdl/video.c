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

#ifdef SDLMAME_X11
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

#ifdef SDLMAME_OS2
#define INCL_WIN
#include <os2.h>
#endif

// standard C headers
#include <math.h>
#include <unistd.h>

// MAME headers
#include "rendutil.h"
#include "profiler.h"
#include "debugwin.h"
#include "ui.h"

// MAMEOS headers
#include "video.h"
#include "window.h"
#include "input.h"

#include "osdsdl.h"

#include "osd_opengl.h"

#ifdef MESS
#include "menu.h"
#endif

//============================================================
//  CONSTANTS
//============================================================

// refresh rate while paused
#define PAUSED_REFRESH_RATE			(60)


//============================================================
//  GLOBAL VARIABLES
//============================================================

sdl_video_config video_config;

#ifndef NO_OPENGL
#ifdef USE_DISPATCH_GL
osd_gl_dispatch *gl_dispatch;
#endif
#endif

//============================================================
//  LOCAL VARIABLES
//============================================================

static sdl_monitor_info *primary_monitor;
static sdl_monitor_info *sdl_monitor_list;

static bitmap_t *effect_bitmap;

//============================================================
//  PROTOTYPES
//============================================================

static void video_exit(running_machine *machine);
static void init_monitors(void);
static sdl_monitor_info *pick_monitor(int index);

static void check_osd_inputs(void);

static void extract_video_config(running_machine *machine);
static void load_effect_overlay(running_machine *machine, const char *filename);
static float get_aspect(const char *name, int report_error);
static void get_resolution(const char *name, sdl_window_config *config, int report_error);


//============================================================
//  sdlvideo_init
//============================================================

int sdlvideo_init(running_machine *machine)
{
	int index, tc;

	// extract data from the options
	extract_video_config(machine);
	
	// ensure we get called on the way out
	add_exit_callback(machine, video_exit);

	// set up monitors first
	init_monitors();

	// we need the beam width in a float, contrary to what the core does.
	video_config.beamwidth = options_get_float(mame_options(), OPTION_BEAM);

	if (machine->config->video_attributes & SCREEN_TYPE_VECTOR)
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

	tc = machine->config->total_colors;

	// create the windows
	for (index = 0; index < video_config.numscreens; index++)
	{
		video_config.window[index].totalColors = tc;
		if (sdlwindow_video_window_create(machine, index, pick_monitor(index), &video_config.window[index]))
			goto error;
	}


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
    #elif defined(SDLMAME_X11) || defined(SDLMAME_NO_X11)       // X11 version
	{
		#if defined(SDLMAME_X11)
		//FIXME: The code below fails for SDL1.3
		#if (!SDL_VERSION_ATLEAST(1,3,0))
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
		#endif
 		#endif
		{
			static int first_call=0;
			static int cw, ch;
			
			SDL_VideoDriverName(monitor->monitor_device, sizeof(monitor->monitor_device)-1);
			if (first_call==0) 
			{
				char *dimstr = getenv(SDLENV_DESKTOPDIM);
				const SDL_VideoInfo *sdl_vi;
				
				sdl_vi = SDL_GetVideoInfo();
				#if (SDL_VERSION_ATLEAST(1,2,10))
				//FIXME: File a bug for SDL 1.3
				#if (SDL_VERSION_ATLEAST(1,3,0))
				cw = 0;
				ch = 0;
				#else
				cw = sdl_vi->current_w;
				ch = sdl_vi->current_h;
				#endif
				#endif
				first_call=1;
				if ((cw==0) || (ch==0))
				{
					if (dimstr != NULL)
					{
						sscanf(dimstr, "%dx%d", &cw, &ch);
					}
					if ((cw==0) || (ch==0))
					{
						mame_printf_warning("WARNING: SDL_GetVideoInfo() for driver <%s> is broken.\n", monitor->monitor_device);
						mame_printf_warning("         You should set SDLMAME_DESKTOPDIM to your desktop size.\n");
						mame_printf_warning("            e.g. export SDLMAME_DESKTOPDIM=800x600\n");
						mame_printf_warning("         Assuming 1024x768 now!\n");
						cw=1024;
						ch=768;
					}
				}
			}
			monitor->monitor_width = cw;
			monitor->monitor_height = ch;
			monitor->center_width = cw;
			monitor->center_height = ch;
		}	
	}
	#elif defined(SDLMAME_OS2) 		// OS2 version
	monitor->center_width = monitor->monitor_width = WinQuerySysValue( HWND_DESKTOP, SV_CXSCREEN );
	monitor->center_height = monitor->monitor_height = WinQuerySysValue( HWND_DESKTOP, SV_CYSCREEN );
	strcpy(monitor->monitor_device, "OS/2 display");
	#else
	#error Unknown SDLMAME_xx OS type!
	#endif

	{
		static int info_shown=0;
		if (!info_shown) 
		{
			mame_printf_verbose("SDL Device Driver     : %s\n", monitor->monitor_device);
			mame_printf_verbose("SDL Monitor Dimensions: %d x %d\n", monitor->monitor_width, monitor->monitor_height);
			info_shown = 1;
		}
	}
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

void osd_update(running_machine *machine, int skip_redraw)
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
	sdlinput_poll(machine);
	check_osd_inputs();

#ifdef ENABLE_DEBUGGER
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
	sprintf(option, SDLOPTION_SCREEN("%d"), index);
	scrname = options_get_string(mame_options(), option);
	#endif

	// get the aspect ratio
	sprintf(option, SDLOPTION_ASPECT("%d"), index);
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
	{
		monitor->aspect = aspect;
	}
	return monitor;
}

//============================================================
//  check_osd_inputs
//============================================================

static void check_osd_inputs(void)
{
	sdl_window_info *window = sdl_window_list;

	// check for toggling fullscreen mode
	if (input_ui_pressed(IPT_OSD_1))
		sdlwindow_toggle_full_screen(window);
	
	if (input_ui_pressed(IPT_OSD_2))
	{
		video_config.fullstretch = !video_config.fullstretch;
		ui_popup_time(1, "Uneven stretch %s", video_config.fullstretch? "enabled":"disabled");
	}
	
	if (input_ui_pressed(IPT_OSD_4))
	{
		video_config.keepaspect = !video_config.keepaspect;
		ui_popup_time(1, "Keepaspect %s", video_config.keepaspect? "enabled":"disabled");
	}
	
#if USE_OPENGL
	if (input_ui_pressed(IPT_OSD_5))
	{
		video_config.filter = !video_config.filter;
		ui_popup_time(1, "Filter %s", video_config.filter? "enabled":"disabled");
	}
#endif

	if (input_ui_pressed(IPT_OSD_6))
		sdlwindow_modify_prescale(window, -1);

	if (input_ui_pressed(IPT_OSD_7))
		sdlwindow_modify_prescale(window, 1);

	if (input_ui_pressed(IPT_OSD_8))
		sdlwindow_modify_effect(window, -1);

	if (input_ui_pressed(IPT_OSD_9))
		sdlwindow_modify_effect(window, 1);

	if (input_ui_pressed(IPT_OSD_10))
		sdlwindow_toggle_draw(window);
}



//============================================================
//  extract_video_config
//============================================================

static void extract_video_config(running_machine *machine)
{
	const char *stemp;

	video_config.perftest    = options_get_bool(mame_options(), SDLOPTION_SDLVIDEOFPS);

	// global options: extract the data
	video_config.windowed      = options_get_bool(mame_options(), SDLOPTION_WINDOW);
	video_config.keepaspect    = options_get_bool(mame_options(), SDLOPTION_KEEPASPECT);
	video_config.numscreens    = options_get_int(mame_options(), SDLOPTION_NUMSCREENS);
	video_config.fullstretch   = options_get_bool(mame_options(), SDLOPTION_UNEVENSTRETCH);
	#ifdef SDLMAME_X11
	video_config.restrictonemonitor = !options_get_bool(mame_options(), SDLOPTION_USEALLHEADS);
	#endif


#ifdef ENABLE_DEBUGGER
	// if we are in debug mode, never go full screen
	if (options_get_bool(mame_options(), OPTION_DEBUG))
		video_config.windowed = TRUE;
#endif
	stemp = options_get_string(mame_options(), SDLOPTION_EFFECT);
	if (stemp != NULL && strcmp(stemp, "none") != 0)
		load_effect_overlay(machine, stemp);

	// per-window options: extract the data
	get_resolution(SDLOPTION_RESOLUTION("0"), &video_config.window[0], TRUE);
	get_resolution(SDLOPTION_RESOLUTION("1"), &video_config.window[1], TRUE);
	get_resolution(SDLOPTION_RESOLUTION("2"), &video_config.window[2], TRUE);
	get_resolution(SDLOPTION_RESOLUTION("3"), &video_config.window[3], TRUE);

	// default to working video please
	video_config.novideo = 0;

	// d3d options: extract the data
	stemp = options_get_string(mame_options(), SDLOPTION_VIDEO);
	if (strcmp(stemp, SDLOPTVAL_SOFT) == 0)
		video_config.mode = VIDEO_MODE_SOFT;
	else if (strcmp(stemp, SDLOPTVAL_NONE) == 0)
	{
		video_config.mode = VIDEO_MODE_SOFT;
		video_config.novideo = 1;

		if (options_get_int(mame_options(), OPTION_SECONDS_TO_RUN) == 0)
			mame_printf_warning("Warning: -video none doesn't make much sense without -seconds_to_run\n");
	}
#if USE_OPENGL
	else if (strcmp(stemp, SDLOPTVAL_OPENGL) == 0)
		video_config.mode = VIDEO_MODE_OPENGL;
	else if (strcmp(stemp, SDLOPTVAL_OPENGL16) == 0)
	{
 		video_config.mode = VIDEO_MODE_OPENGL;
 		video_config.prefer16bpp_tex = 1;
	}
#endif
	else
	{
		mame_printf_warning("Invalid video value %s; reverting to software\n", stemp);
		video_config.mode = VIDEO_MODE_SOFT;
	}
	
	video_config.switchres     = options_get_bool(mame_options(), SDLOPTION_SWITCHRES);
	video_config.centerh       = options_get_bool(mame_options(), SDLOPTION_CENTERH);
	video_config.centerv       = options_get_bool(mame_options(), SDLOPTION_CENTERV);

#if USE_OPENGL
	video_config.prescale      = options_get_int(mame_options(), SDLOPTION_PRESCALE);
	if (video_config.prescale < 1 || video_config.prescale > 3)
	{
		mame_printf_warning("Invalid prescale option, reverting to '1'\n");
		video_config.prescale = 1;
	}
	// default to working video please
	video_config.prefer16bpp_tex = 0;
	video_config.filter        = options_get_bool(mame_options(), SDLOPTION_FILTER);
	video_config.forcepow2texture = options_get_bool(mame_options(), SDLOPTION_GL_FORCEPOW2TEXTURE)==1;
	video_config.allowtexturerect = options_get_bool(mame_options(), SDLOPTION_GL_NOTEXTURERECT)==0;
	video_config.vbo         = options_get_bool(mame_options(), SDLOPTION_GL_VBO);
	video_config.pbo         = options_get_bool(mame_options(), SDLOPTION_GL_PBO);
	video_config.glsl        = options_get_bool(mame_options(), SDLOPTION_GL_GLSL);
	if ( video_config.glsl )
	{
		int i;
		static char buffer[20]; // gl_glsl_filter[0..9]?

		video_config.glsl_filter = options_get_int (mame_options(), SDLOPTION_GLSL_FILTER);

		video_config.glsl_shader_mamebm_num=0;

		for(i=0; i<GLSL_SHADER_MAX; i++)
		{
			snprintf(buffer, 18, SDLOPTION_SHADER_MAME("%d"), i); buffer[17]=0;

			stemp = options_get_string(mame_options(), buffer);
			if (stemp && strcmp(stemp, SDLOPTVAL_NONE) != 0 && strlen(stemp)>0)
			{
				video_config.glsl_shader_mamebm[i] = (char *) malloc(strlen(stemp)+1);
				strcpy(video_config.glsl_shader_mamebm[i], stemp);
				video_config.glsl_shader_mamebm_num++;
			} else {
				video_config.glsl_shader_mamebm[i] = NULL;
			}
		}

		video_config.glsl_shader_scrn_num=0;

		for(i=0; i<GLSL_SHADER_MAX; i++)
		{
			snprintf(buffer, 20, SDLOPTION_SHADER_SCREEN("%d"), i); buffer[19]=0;

			stemp = options_get_string(mame_options(), buffer);
			if (stemp && strcmp(stemp, SDLOPTVAL_NONE) != 0 && strlen(stemp)>0)
			{
				video_config.glsl_shader_scrn[i] = (char *) malloc(strlen(stemp)+1);
				strcpy(video_config.glsl_shader_scrn[i], stemp);
				video_config.glsl_shader_scrn_num++;
			} else {
				video_config.glsl_shader_scrn[i] = NULL;
			}
		}

		video_config.glsl_vid_attributes = options_get_int (mame_options(), SDLOPTION_GL_GLSL_VID_ATTR);
		{
			// Disable feature: glsl_vid_attributes, as long we have the gamma calculation
			// disabled within the direct shaders .. -> too slow.
			// IMHO the gamma setting should be done global anyways, and for the whole system,
			// not just MAME ..
			float gamma = options_get_float(mame_options(), OPTION_GAMMA);
			if (gamma != 1.0 && video_config.glsl_vid_attributes && video_config.glsl)
			{
				video_config.glsl_vid_attributes = FALSE;
				mame_printf_warning("OpenGL: GLSL - disable handling of brightness and contrast, gamma is set to %f\n", gamma);
			}
		}
	} else {
		int i;
		video_config.glsl_filter = 0;
		video_config.glsl_shader_mamebm_num=0;
		for(i=0; i<GLSL_SHADER_MAX; i++)
		{
			video_config.glsl_shader_mamebm[i] = NULL;
		}
		video_config.glsl_shader_scrn_num=0;
		for(i=0; i<GLSL_SHADER_MAX; i++)
		{
			video_config.glsl_shader_scrn[i] = NULL;
		}
		video_config.glsl_vid_attributes = 0;
	}

	if (sdl_use_unsupported())
		video_config.prescale_effect = options_get_int(mame_options(), SDLOPTION_PRESCALE_EFFECT);
	else
		video_config.prescale_effect = 0;
#endif
	// misc options: sanity check values

	// global options: sanity check values
	if (video_config.numscreens < 1 || video_config.numscreens > 1) //MAX_VIDEO_WINDOWS)
	{
		mame_printf_warning("Invalid numscreens value %d; reverting to 1\n", video_config.numscreens);
		video_config.numscreens = 1;
	}

	// yuv settings ...
	stemp = options_get_string(mame_options(), SDLOPTION_YUVMODE);
	if (strcmp(stemp, SDLOPTVAL_NONE) == 0)
		video_config.yuv_mode = VIDEO_YUV_MODE_NONE;
	else if (strcmp(stemp, SDLOPTVAL_YV12) == 0)
		video_config.yuv_mode = VIDEO_YUV_MODE_YV12;
	else if (strcmp(stemp, SDLOPTVAL_YV12x2) == 0)
		video_config.yuv_mode = VIDEO_YUV_MODE_YV12X2;
	else if (strcmp(stemp, SDLOPTVAL_YUY2) == 0)
		video_config.yuv_mode = VIDEO_YUV_MODE_YUY2;
	else if (strcmp(stemp, SDLOPTVAL_YUY2x2) == 0)
		video_config.yuv_mode = VIDEO_YUV_MODE_YUY2X2;
	else
	{
		mame_printf_warning("Invalid yuvmode value %s; reverting to none\n", stemp);
		video_config.yuv_mode = VIDEO_YUV_MODE_NONE;
	}
	if ( (video_config.mode != VIDEO_MODE_SOFT) && (video_config.yuv_mode != VIDEO_YUV_MODE_NONE) )
	{
		mame_printf_warning("yuvmode is only for -video soft, overriding\n");
		video_config.yuv_mode = VIDEO_YUV_MODE_NONE;
	}
}


//============================================================
//  load_effect_overlay
//============================================================

static void load_effect_overlay(running_machine *machine, const char *filename)
{
	char *tempstr = malloc_or_die(strlen(filename) + 5);
	int scrnum;
	int numscreens;
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
		mame_printf_error("Unable to load PNG file '%s'\n", tempstr);
		free(tempstr);
		return;
	}

	// set the overlay on all screens
	numscreens = video_screen_count(machine->config);
	for (scrnum = 0; scrnum < numscreens; scrnum++)
		render_container_set_overlay(render_container_get_screen(scrnum), effect_bitmap);

	free(tempstr);
}


//============================================================
//  get_aspect
//============================================================

static float get_aspect(const char *name, int report_error)
{
	const char *defdata = options_get_string(mame_options(), SDLOPTION_ASPECT(""));
	const char *data = options_get_string(mame_options(), name);
	int num = 0, den = 1;

	if (strcmp(data, SDLOPTVAL_AUTO) == 0)
	{
		if (strcmp(defdata,SDLOPTVAL_AUTO) == 0)
			return 0;
		data = defdata;
	}
	if (sscanf(data, "%d:%d", &num, &den) != 2 && report_error)
		mame_printf_error("Illegal aspect ratio value for %s = %s\n", name, data);
	return (float)num / (float)den;
}



//============================================================
//  get_resolution
//============================================================

static void get_resolution(const char *name, sdl_window_config *config, int report_error)
{
	const char *data = options_get_string(mame_options(), name);

	config->width = config->height = config->depth = config->refresh = 0;
	if (strcmp(data, SDLOPTVAL_AUTO) == 0)
	{
		return;
	}
	else if (sscanf(data, "%dx%dx%d@%d", &config->width, &config->height, &config->depth, &config->refresh) < 2 && report_error)
		mame_printf_error("Illegal resolution value for %s = %s\n", name, data);
}

