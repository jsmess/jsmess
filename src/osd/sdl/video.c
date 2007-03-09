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

// frameskipping
#define FRAMESKIP_LEVELS			12

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

int video_orientation;
sdl_video_config video_config;

//============================================================
//  LOCAL VARIABLES
//============================================================

sdl_monitor_info *sdl_monitor_list;
static sdl_monitor_info *primary_monitor;

static mame_bitmap *effect_bitmap;

// average FPS calculation
static osd_ticks_t fps_start_time;
static osd_ticks_t fps_end_time;
static int fps_frames_displayed;

// throttling calculations
static throttle_info throttle;

// frameskipping
static int frameskip_counter;
static int frameskipadjust;

// frameskipping tables
static const int skiptable[FRAMESKIP_LEVELS][FRAMESKIP_LEVELS] =
{
	{ 0,0,0,0,0,0,0,0,0,0,0,0 },
	{ 0,0,0,0,0,0,0,0,0,0,0,1 },
	{ 0,0,0,0,0,1,0,0,0,0,0,1 },
	{ 0,0,0,1,0,0,0,1,0,0,0,1 },
	{ 0,0,1,0,0,1,0,0,1,0,0,1 },
	{ 0,1,0,0,1,0,1,0,0,1,0,1 },
	{ 0,1,0,1,0,1,0,1,0,1,0,1 },
	{ 0,1,0,1,1,0,1,0,1,1,0,1 },
	{ 0,1,1,0,1,1,0,1,1,0,1,1 },
	{ 0,1,1,1,0,1,1,1,0,1,1,1 },
	{ 0,1,1,1,1,1,0,1,1,1,1,1 },
	{ 0,1,1,1,1,1,1,1,1,1,1,1 }
};


//============================================================
//  PROTOTYPES
//============================================================

static void video_exit(running_machine *machine);
static void init_monitors(void);
static sdl_monitor_info *pick_monitor(int index);

static void update_throttle(mame_time emutime);
static osd_ticks_t throttle_until_ticks(osd_ticks_t target_ticks);
static void update_fps(mame_time emutime);
static void update_autoframeskip(void);
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
	const char *stemp;
	int index;

	// ensure we get called on the way out
	add_exit_callback(machine, video_exit);

	// extract data from the options
	extract_video_config();

	// set up monitors first
	init_monitors();

	// we need the beam width in a float, contrary to what the core does.
	video_config.beamwidth = options_get_float("beam");

	// initialize the window system so we can make windows
	if (sdlwindow_init(machine))
		goto error;

	// create the windows
	for (index = 0; index < video_config.numscreens; index++)
		if (sdlwindow_video_window_create(index, pick_monitor(index), &video_config.window[index]))
			goto error;

	// start recording movie
	stemp = options_get_string("mngwrite");
	if (stemp != NULL)
		video_movie_begin_recording(stemp);

	// if we're running < 5 minutes, allow us to skip warnings to facilitate benchmarking/validation testing
	if (video_config.framestorun > 0 && video_config.framestorun < 60*60*5)
		options.skip_warnings = options.skip_gameinfo = options.skip_disclaimer = TRUE;

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

	// print a final result to the stdout
	if (fps_frames_displayed != 0)
	{
		osd_ticks_t tps = osd_ticks_per_second();
		mame_printf_info("Average FPS: %f (%d frames)\n", (double)tps / (fps_end_time - fps_start_time) * fps_frames_displayed, fps_frames_displayed);
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
				cw = sdl_vi->current_w;
				ch = sdl_vi->current_h;
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
//  sdlvideo_set_frameskip
//============================================================

void sdlvideo_set_frameskip(int value)
{
	if (value >= 0)
	{
		video_config.frameskip = value;
		video_config.autoframeskip = 0;
	}
	else
	{
		video_config.frameskip = 0;
		video_config.autoframeskip = 1;
	}
}



//============================================================
//  sdlvideo_get_frameskip
//============================================================

int sdlvideo_get_frameskip(void)
{
	return video_config.autoframeskip ? -1 : video_config.frameskip;
}



//============================================================
//  osd_update
//============================================================

int osd_update(mame_time emutime)
{
	sdl_window_info *window;

	// if we're throttling, paused, or if the UI is up, synchronize
	if (effective_throttle())
		update_throttle(emutime);

	// update the FPS computations
	update_fps(emutime);

	// update all the windows, but only if we're not skipping this frame
	if (!skiptable[effective_frameskip()][frameskip_counter])
	{
		profiler_mark(PROFILER_BLIT);
		for (window = sdl_window_list; window != NULL; window = window->next)
			sdlwindow_video_window_update(window);
		profiler_mark(PROFILER_END);
	}

	// if we're throttling and autoframeskip is on, adjust
	if (effective_throttle() && effective_autoframeskip() && frameskip_counter == 0)
		update_autoframeskip();

	// poll the joystick values here
	win_process_events();
	check_osd_inputs();

#ifdef MAME_DEBUG
	debugwin_update_during_game();
#endif

	// increment the frameskip counter
	frameskip_counter = (frameskip_counter + 1) % FRAMESKIP_LEVELS;

	// return whether or not to skip the next frame
	return skiptable[effective_frameskip()][frameskip_counter];
}



//============================================================
//  osd_get_fps_text
//============================================================

const char *osd_get_fps_text(const performance_info *performance)
{
	static char buffer[1024];
	char *dest = buffer;

	// if we're paused, display less info
	if (mame_is_paused(Machine))
		dest += sprintf(dest, "%s%2d%4d/%d fps",
				effective_autoframeskip() ? "auto" : "fskp", effective_frameskip(),
				(int)(performance->frames_per_second + 0.5),
				PAUSED_REFRESH_RATE);
	else
	{
		dest += sprintf(dest, "%s%2d%4d%%%4d/%d fps",
				effective_autoframeskip() ? "auto" : "fskp", effective_frameskip(),
				(int)(performance->game_speed_percent + 0.5),
				(int)(performance->frames_per_second + 0.5),
				(int)(Machine->screen[0].refresh + 0.5));

		/* for vector games, add the number of vector updates */
		if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
			dest += sprintf(dest, "\n %d vector updates", performance->vector_updates_last_second);
		else if (performance->partial_updates_this_frame > 1)
			dest += sprintf(dest, "\n %d partial updates", performance->partial_updates_this_frame);
	}

	/* return a pointer to the static buffer */
	return buffer;
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
//  update_throttle
//============================================================

static void update_throttle(mame_time emutime)
{
	/*

        Throttling theory:

        This routine is called periodically with an up-to-date emulated time.
        The idea is to synchronize real time with emulated time. We do this
        by "throttling", or waiting for real time to catch up with emulated
        time.

        In an ideal world, it will take less real time to emulate and render
        each frame than the emulated time, so we need to slow things down to
        get both times in sync.

        There are many complications to this model:

            * some games run too slow, so each frame we get further and
                further behind real time; our only choice here is to not
                throttle

            * some games have very uneven frame rates; one frame will take
                a long time to emulate, and the next frame may be very fast

            * we run on top of multitasking OSes; sometimes execution time
                is taken away from us, and this means we may not get enough
                time to emulate one frame

            * we may be paused, and emulated time may not be marching
                forward

            * emulated time could jump due to resetting the machine or
                restoring from a saved state

    */
	static const UINT8 popcount[256] =
	{
		0,1,1,2,1,2,2,3, 1,2,2,3,2,3,3,4, 1,2,2,3,2,3,3,4, 2,3,3,4,3,4,4,5,
		1,2,2,3,2,3,3,4, 2,3,3,4,3,4,4,5, 2,3,3,4,3,4,4,5, 3,4,4,5,4,5,5,6,
		1,2,2,3,2,3,3,4, 2,3,3,4,3,4,4,5, 2,3,3,4,3,4,4,5, 3,4,4,5,4,5,5,6,
		2,3,3,4,3,4,4,5, 3,4,4,5,4,5,5,6, 3,4,4,5,4,5,5,6, 4,5,5,6,5,6,6,7,
		1,2,2,3,2,3,3,4, 2,3,3,4,3,4,4,5, 2,3,3,4,3,4,4,5, 3,4,4,5,4,5,5,6,
		2,3,3,4,3,4,4,5, 3,4,4,5,4,5,5,6, 3,4,4,5,4,5,5,6, 4,5,5,6,5,6,6,7,
		2,3,3,4,3,4,4,5, 3,4,4,5,4,5,5,6, 3,4,4,5,4,5,5,6, 4,5,5,6,5,6,6,7,
		3,4,4,5,4,5,5,6, 4,5,5,6,5,6,6,7, 4,5,5,6,5,6,6,7, 5,6,6,7,6,7,7,8
	};
	subseconds_t real_delta_subseconds;
	subseconds_t emu_delta_subseconds;
	subseconds_t real_is_ahead_subseconds;
	subseconds_t subseconds_per_tick;
	osd_ticks_t ticks_per_second;
	osd_ticks_t target_ticks;
	osd_ticks_t diff_ticks;

	/* if we're only syncing to the refresh, bail now */
	if (video_config.syncrefresh)
		return;

	/* compute conversion factors up front */
	ticks_per_second = osd_ticks_per_second();
	subseconds_per_tick = MAX_SUBSECONDS / ticks_per_second;

	/* if we're paused, emutime will not advance; instead, we subtract a fixed
       amount of time (1/60th of a second) from the emulated time that was passed in,
       and explicitly reset our tracked real and emulated timers to that value ...
       this means we pretend that the last update was exactly 1/60th of a second
       ago, and was in sync in both real and emulated time */
	if (mame_is_paused(Machine))
	{
		throttle.emutime = sub_subseconds_from_mame_time(emutime, MAX_SUBSECONDS / PAUSED_REFRESH_RATE);
		throttle.realtime = throttle.emutime;
	}

	/* attempt to detect anomalies in the emulated time by subtracting the previously
       reported value from our current value; this should be a small value somewhere
       between 0 and 1/10th of a second ... anything outside of this range is obviously
       wrong and requires a resync */
	emu_delta_subseconds = mame_time_to_subseconds(sub_mame_times(emutime, throttle.emutime));
	if (emu_delta_subseconds < 0 || emu_delta_subseconds > MAX_SUBSECONDS / 10)
	{
		if (LOG_THROTTLE)
			logerror("Resync due to weird emutime delta\n");
		goto resync;
	}

	/* now determine the current real time in OSD-specified ticks; we have to be careful
       here because counters can wrap, so we only use the difference between the last
       read value and the current value in our computations */
	diff_ticks = osd_ticks() - throttle.last_ticks;
	throttle.last_ticks += diff_ticks;

	/* if it has been more than a full second of real time since the last call to this
       function, we just need to resynchronize */
	if (diff_ticks >= ticks_per_second)
	{
		if (LOG_THROTTLE)
			logerror("Resync due to real time advancing by more than 1 second\n");
		goto resync;
	}

	/* convert this value into subseconds for easier comparison */
	real_delta_subseconds = diff_ticks * subseconds_per_tick;

	/* now update our real and emulated timers with the current values */
	throttle.emutime = emutime;
	throttle.realtime = add_subseconds_to_mame_time(throttle.realtime, real_delta_subseconds);

	/* keep a history of whether or not emulated time beat real time over the last few
       updates; this can be used for future heuristics */
	throttle.history = (throttle.history << 1) | (emu_delta_subseconds > real_delta_subseconds);

	/* determine how far ahead real time is versus emulated time; note that we use the
       accumulated times for this instead of the deltas for the current update because
       we want to track time over a longer duration than a single update */
	real_is_ahead_subseconds = mame_time_to_subseconds(sub_mame_times(throttle.emutime, throttle.realtime));

	/* if we're more than 1/10th of a second out, or if we are behind at all and emulation
       is taking longer than the real frame, we just need to resync */
	if (real_is_ahead_subseconds < -MAX_SUBSECONDS / 10 ||
		(real_is_ahead_subseconds < 0 && popcount[throttle.history & 0xff] < 6))
	{
		if (LOG_THROTTLE)
			logerror("Resync due to being behind (history=%08X)\n", throttle.history);
		goto resync;
	}

	/* if we're behind, it's time to just get out */
	if (real_is_ahead_subseconds < 0)
		return;

	/* compute the target real time, in ticks, where we want to be */
	target_ticks = throttle.last_ticks + real_is_ahead_subseconds / subseconds_per_tick;

	/* throttle until we read the target, and update real time to match the final time */
	diff_ticks = throttle_until_ticks(target_ticks) - throttle.last_ticks;
	throttle.last_ticks += diff_ticks;
	throttle.realtime = add_subseconds_to_mame_time(throttle.realtime, diff_ticks * subseconds_per_tick);
	return;

resync:
	/* reset realtime and emutime to the same value */
	throttle.realtime = throttle.emutime = emutime;
}



//============================================================
//  update_fps
//============================================================

static void update_fps(mame_time emutime)
{
	osd_ticks_t curr = osd_ticks();

	// update stats for the FPS average calculation
	if (fps_start_time == 0)
	{
		// start the timer going 1 second into the game
		if (emutime.seconds > 1)
			fps_start_time = osd_ticks();
	}
	else
	{
		fps_frames_displayed++;
		if (fps_frames_displayed == video_config.framestorun)
		{
			mame_file_error filerr;
			mame_file *fp;
			char name[20];

			// make a filename with an underscore prefix
			sprintf(name, "_%.8s.png", Machine->gamedrv->name);

			// write out the screenshot
			filerr = mame_fopen(SEARCHPATH_SCREENSHOT, name, OPEN_FLAG_WRITE | OPEN_FLAG_CREATE | OPEN_FLAG_CREATE_PATHS, &fp);
			if (filerr == FILERR_NONE)
			{
				video_screen_save_snapshot(fp, 0);
				mame_fclose(fp);
			}
			mame_schedule_exit(Machine);
		}
		fps_end_time = curr;
	}
}


//============================================================
//  throttle_until_ticks
//============================================================

static osd_ticks_t throttle_until_ticks(osd_ticks_t target_ticks)
{
	static osd_ticks_t ticks_per_sleep_unit;
	osd_ticks_t current_ticks = osd_ticks();
	osd_ticks_t new_ticks;
	int allowed_to_sleep;

	/* we're allowed to sleep via the OSD code only if we're configured to do so
       and we're not frameskipping due to autoframeskip, or if we're paused */
	allowed_to_sleep = (video_config.sleep && (!effective_autoframeskip() || effective_frameskip() == 0)) || mame_is_paused(Machine);

	/* make sure we have a valid default ticks_per_sleep_unit */
	if (ticks_per_sleep_unit == 0)
		ticks_per_sleep_unit = osd_ticks_per_second() / 1000;

	/* loop until we reach our target */
	profiler_mark(PROFILER_IDLE);
	while (current_ticks < target_ticks)
	{
		osd_ticks_t delta = target_ticks - current_ticks;
		int slept = FALSE;

		/* see if we can sleep */
		if (allowed_to_sleep && delta < ticks_per_sleep_unit * 3 / 2)
		{
			#ifdef SDLMAME_WIN32
			Sleep(1);  	// 1 ms
			#else
			usleep(1000);	// 1000 uSec (1 ms)
			#endif
			slept = TRUE;
		}

		/* read the new value */
		new_ticks = osd_ticks();
		if (slept)
			ticks_per_sleep_unit = (ticks_per_sleep_unit * 9 + (new_ticks - current_ticks)) / 10;
		current_ticks = new_ticks;
	}

	return current_ticks;
}

//============================================================
//  update_autoframeskip
//============================================================

static void update_autoframeskip(void)
{
	// skip if paused
	if (mame_is_paused(Machine))
		return;

	// don't adjust frameskip if we're paused or if the debugger was
	// visible this cycle or if we haven't run yet
	if (cpu_getcurrentframe() > 2 * FRAMESKIP_LEVELS)
	{
		const performance_info *performance = mame_get_performance_info();

		// if we're too fast, attempt to increase the frameskip
		if (performance->game_speed_percent >= 99.5)
		{
			frameskipadjust++;

			// but only after 3 consecutive frames where we are too fast
			if (frameskipadjust >= 3)
			{
				frameskipadjust = 0;
				if (video_config.frameskip > 0) video_config.frameskip--;
			}
		}

		// if we're too slow, attempt to increase the frameskip
		else
		{
			// if below 80% speed, be more aggressive
			if (performance->game_speed_percent < 80)
				frameskipadjust -= (90 - performance->game_speed_percent) / 5;

			// if we're close, only force it up to frameskip 8
			else if (video_config.frameskip < 8)
				frameskipadjust--;

			// perform the adjustment
			while (frameskipadjust <= -2)
			{
				frameskipadjust += 2;
				if (video_config.frameskip < FRAMESKIP_LEVELS - 1)
					video_config.frameskip++;
			}
		}
	}
}



//============================================================
//  check_osd_inputs
//============================================================

static void check_osd_inputs(void)
{
	// increment frameskip?
	if (input_ui_pressed(IPT_UI_FRAMESKIP_INC))
	{
		// if autoframeskip, disable auto and go to 0
		if (video_config.autoframeskip)
		{
			video_config.autoframeskip = 0;
			video_config.frameskip = 0;
		}

		// wrap from maximum to auto
		else if (video_config.frameskip == FRAMESKIP_LEVELS - 1)
		{
			video_config.frameskip = 0;
			video_config.autoframeskip = 1;
		}

		// else just increment
		else
			video_config.frameskip++;

		// display the FPS counter for 2 seconds
		ui_show_fps_temp(2.0);

		// reset the frame counter so we'll measure the average FPS on a consistent status
		fps_frames_displayed = 0;
	}

	// decrement frameskip?
	if (input_ui_pressed(IPT_UI_FRAMESKIP_DEC))
	{
		// if autoframeskip, disable auto and go to max
		if (video_config.autoframeskip)
		{
			video_config.autoframeskip = 0;
			video_config.frameskip = FRAMESKIP_LEVELS-1;
		}

		// wrap from 0 to auto
		else if (video_config.frameskip == 0)
			video_config.autoframeskip = 1;

		// else just decrement
		else
			video_config.frameskip--;

		// display the FPS counter for 2 seconds
		ui_show_fps_temp(2.0);

		// reset the frame counter so we'll measure the average FPS on a consistent status
		fps_frames_displayed = 0;
	}

	// toggle throttle?
	if (input_ui_pressed(IPT_UI_THROTTLE))
	{
		video_config.throttle = !video_config.throttle;

		// reset the frame counter so we'll measure the average FPS on a consistent status
		fps_frames_displayed = 0;
	}

	// check for toggling fullscreen mode
	if (input_ui_pressed(IPT_OSD_1))
		sdlwindow_toggle_full_screen();
	
	// check for fast forward
	video_config.fastforward = input_port_type_pressed(IPT_OSD_2, 0);
	if (video_config.fastforward)
		ui_show_fps_temp(0.5);

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
	if (options.mame_debug)
		video_config.windowed = TRUE;
#endif
	stemp                      = options_get_string("effect");
	if (stemp != NULL && strcmp(stemp, "none") != 0)
		load_effect_overlay(stemp);

	// configure layers
	video_config.layerconfig = LAYER_CONFIG_DEFAULT;	
	if (!options_get_bool("use_backdrops")) video_config.layerconfig &= ~LAYER_CONFIG_ENABLE_BACKDROP;
	if (!options_get_bool("use_overlays")) video_config.layerconfig &= ~LAYER_CONFIG_ENABLE_OVERLAY;
	if (!options_get_bool("use_bezels")) video_config.layerconfig &= ~LAYER_CONFIG_ENABLE_BEZEL;
	if (options_get_bool("artwork_crop")) video_config.layerconfig |= LAYER_CONFIG_ZOOM_TO_SCREEN;

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
