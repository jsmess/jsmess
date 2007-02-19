//============================================================
//
//  video.c - Win32 video handling
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

// needed for multimonitor
#define _WIN32_WINNT 0x501

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// standard C headers
#include <math.h>

// Windows 95/NT4 multimonitor stubs
#ifdef WIN95_MULTIMON
#include "multidef.h"
#endif

// MAME headers
#include "osdepend.h"
#include "driver.h"
#include "profiler.h"
#include "video/vector.h"
#include "render.h"
#include "rendutil.h"
#include "ui.h"

// MAMEOS headers
#include "winmain.h"
#include "video.h"
#include "window.h"
#include "input.h"
#include "debugwin.h"
#include "strconv.h"

#ifdef MESS
#include "menu.h"
#endif


//============================================================
//  CONSTANTS
//============================================================

// frameskipping
#define FRAMESKIP_LEVELS			12

// refresh rate while paused
#define PAUSED_REFRESH_RATE			60


//============================================================
//  TYPE DEFINITIONS
//============================================================


//============================================================
//  GLOBAL VARIABLES
//============================================================

int video_orientation;
win_video_config video_config;



//============================================================
//  LOCAL VARIABLES
//============================================================

// monitor info
win_monitor_info *win_monitor_list;
static win_monitor_info *primary_monitor;

static mame_bitmap *effect_bitmap;

// average FPS calculation
static osd_ticks_t fps_start_time;
static osd_ticks_t fps_end_time;
static int fps_frames_displayed;

// throttling calculations
static osd_ticks_t throttle_last_ticks;
static mame_time throttle_realtime;
static mame_time throttle_emutime;

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

static const int waittable[FRAMESKIP_LEVELS][FRAMESKIP_LEVELS] =
{
	{ 1,1,1,1,1,1,1,1,1,1,1,1 },
	{ 2,1,1,1,1,1,1,1,1,1,1,0 },
	{ 2,1,1,1,1,0,2,1,1,1,1,0 },
	{ 2,1,1,0,2,1,1,0,2,1,1,0 },
	{ 2,1,0,2,1,0,2,1,0,2,1,0 },
	{ 2,0,2,1,0,2,0,2,1,0,2,0 },
	{ 2,0,2,0,2,0,2,0,2,0,2,0 },
	{ 2,0,2,0,0,3,0,2,0,0,3,0 },
	{ 3,0,0,3,0,0,3,0,0,3,0,0 },
	{ 4,0,0,0,4,0,0,0,4,0,0,0 },
	{ 6,0,0,0,0,0,6,0,0,0,0,0 },
	{12,0,0,0,0,0,0,0,0,0,0,0 }
};



//============================================================
//  PROTOTYPES
//============================================================

static void video_exit(running_machine *machine);
static void init_monitors(void);
static BOOL CALLBACK monitor_enum_callback(HMONITOR handle, HDC dc, LPRECT rect, LPARAM data);
static win_monitor_info *pick_monitor(int index);

static void update_throttle(mame_time emutime);
static void update_fps(mame_time emutime);
static void update_autoframeskip(void);
static void check_osd_inputs(void);

static void extract_video_config(void);
static void load_effect_overlay(const char *filename);
static float get_aspect(const char *name, int report_error);
static void get_resolution(const char *name, win_window_config *config, int report_error);



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
//  winvideo_init
//============================================================

int winvideo_init(running_machine *machine)
{
	const char *stemp;
	int index;

	// ensure we get called on the way out
	add_exit_callback(machine, video_exit);

	// extract data from the options
	extract_video_config();

	// set up monitors first
	init_monitors();

	// initialize the window system so we can make windows
	if (winwindow_init(machine))
		goto error;

	// create the windows
	for (index = 0; index < video_config.numscreens; index++)
		if (winwindow_video_window_create(index, pick_monitor(index), &video_config.window[index]))
			goto error;
	if (video_config.mode != VIDEO_MODE_NONE)
		SetForegroundWindow(win_window_list->hwnd);

	// possibly create the debug window, but don't show it yet
#ifdef MAME_DEBUG
	if (options.mame_debug)
		if (debugwin_init_windows())
			return 1;
#endif

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

	// possibly kill the debug window
#ifdef MAME_DEBUG
	if (options.mame_debug)
		debugwin_destroy_windows();
#endif

	// free all of our monitor information
	while (win_monitor_list != NULL)
	{
		win_monitor_info *temp = win_monitor_list;
		win_monitor_list = temp->next;
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
//  winvideo_monitor_refresh
//============================================================

void winvideo_monitor_refresh(win_monitor_info *monitor)
{
	HRESULT result;

	// fetch the latest info about the monitor
	monitor->info.cbSize = sizeof(monitor->info);
	result = GetMonitorInfo(monitor->handle, (LPMONITORINFO)&monitor->info);
	assert(result);
}



//============================================================
//  winvideo_monitor_get_aspect
//============================================================

float winvideo_monitor_get_aspect(win_monitor_info *monitor)
{
	// refresh the monitor information and compute the aspect
	if (video_config.keepaspect)
	{
		int width, height;
		winvideo_monitor_refresh(monitor);
		width = rect_width(&monitor->info.rcMonitor);
		height = rect_height(&monitor->info.rcMonitor);
		return monitor->aspect / ((float)width / (float)height);
	}
	return 0.0f;
}



//============================================================
//  winvideo_monitor_from_handle
//============================================================

win_monitor_info *winvideo_monitor_from_handle(HMONITOR hmonitor)
{
	win_monitor_info *monitor;

	// find the matching monitor
	for (monitor = win_monitor_list; monitor != NULL; monitor = monitor->next)
		if (monitor->handle == hmonitor)
			return monitor;
	return NULL;
}



//============================================================
//  winvideo_set_frameskip
//============================================================

void winvideo_set_frameskip(int value)
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
//  winvideo_get_frameskip
//============================================================

int winvideo_get_frameskip(void)
{
	return video_config.autoframeskip ? -1 : video_config.frameskip;
}



//============================================================
//  osd_update
//============================================================

int osd_update(mame_time emutime)
{
	win_window_info *window;

	// if we're throttling, paused, or if the UI is up, synchronize
	if (effective_throttle())
		update_throttle(emutime);

	// update the FPS computations
	update_fps(emutime);

	// update all the windows, but only if we're not skipping this frame
	if (!skiptable[effective_frameskip()][frameskip_counter])
	{
		profiler_mark(PROFILER_BLIT);
		for (window = win_window_list; window != NULL; window = window->next)
			winwindow_video_window_update(window);
		profiler_mark(PROFILER_END);
	}

	// if we're throttling and autoframeskip is on, adjust
	if (effective_throttle() && effective_autoframeskip() && frameskip_counter == 0)
		update_autoframeskip();

	// poll the joystick values here
	winwindow_process_events(TRUE);
	wininput_poll();
	check_osd_inputs();

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
		dest += sprintf(dest, "Paused");
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
//  init_monitors
//============================================================

static void init_monitors(void)
{
	win_monitor_info **tailptr;

	// make a list of monitors
	win_monitor_list = NULL;
	tailptr = &win_monitor_list;
	EnumDisplayMonitors(NULL, NULL, monitor_enum_callback, (LPARAM)&tailptr);

	// if we're verbose, print the list of monitors
	{
		win_monitor_info *monitor;
		for (monitor = win_monitor_list; monitor != NULL; monitor = monitor->next)
		{
			char *utf8_device = utf8_from_tstring(monitor->info.szDevice);
			if (utf8_device != NULL)
			{
				verbose_printf("Video: Monitor %p = \"%s\" %s\n", monitor->handle, utf8_device, (monitor == primary_monitor) ? "(primary)" : "");
				free(utf8_device);
			}
		}
	}
}



//============================================================
//  monitor_enum_callback
//============================================================

static BOOL CALLBACK monitor_enum_callback(HMONITOR handle, HDC dc, LPRECT rect, LPARAM data)
{
	win_monitor_info ***tailptr = (win_monitor_info ***)data;
	win_monitor_info *monitor;
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
	monitor->handle = handle;
	monitor->info = info;

	// guess the aspect ratio assuming square pixels
	monitor->aspect = (float)(info.rcMonitor.right - info.rcMonitor.left) / (float)(info.rcMonitor.bottom - info.rcMonitor.top);

	// save the primary monitor handle
	if (monitor->info.dwFlags & MONITORINFOF_PRIMARY)
		primary_monitor = monitor;

	// hook us into the list
	**tailptr = monitor;
	*tailptr = &monitor->next;

	// enumerate all the available monitors so to list their names in verbose mode
	return TRUE;
}



//============================================================
//  pick_monitor
//============================================================

static win_monitor_info *pick_monitor(int index)
{
	const char *scrname, *scrname2;
	win_monitor_info *monitor;
	int moncount = 0;
	char option[20];
	float aspect;

	// get the screen option
	scrname = options_get_string("screen");
	sprintf(option, "screen%d", index);
	scrname2 = options_get_string(option);

	// decide which one we want to use
	if (scrname2 != NULL && strcmp(scrname2, "auto") != 0)
		scrname = scrname2;

	// get the aspect ratio
	sprintf(option, "aspect%d", index);
	aspect = get_aspect(option, TRUE);

	// look for a match in the name first
	if (scrname != NULL)
		for (monitor = win_monitor_list; monitor != NULL; monitor = monitor->next)
		{
			char *utf8_device;
			int rc = 1;

			moncount++;

			utf8_device = utf8_from_tstring(monitor->info.szDevice);
			if (utf8_device != NULL)
			{
				rc = strcmp(scrname, utf8_device);
				free(utf8_device);
			}
			if (rc == 0)
				goto finishit;
		}

	// didn't find it; alternate monitors until we hit the jackpot
	index %= moncount;
	for (monitor = win_monitor_list; monitor != NULL; monitor = monitor->next)
		if (index-- == 0)
			goto finishit;

	// return the primary just in case all else fails
	monitor = primary_monitor;

finishit:
	if (aspect != 0)
		monitor->aspect = aspect;
	return monitor;
}



//============================================================
//  update_throttle
//============================================================

static void update_throttle(mame_time emutime)
{
	static double ticks_per_sleep_msec = 0;
	osd_ticks_t target, curr, cps, diffticks;
	int allowed_to_sleep;
	subseconds_t subsecs_per_cycle;
	int paused = mame_is_paused(Machine);

	// if we're only syncing to the refresh, bail now
	if (video_config.syncrefresh)
		return;

	// if we're paused, emutime will not advance; explicitly resync and set us backwards
	// 1/60th of a second
	if (paused)
		throttle_realtime = throttle_emutime = sub_subseconds_from_mame_time(emutime, MAX_SUBSECONDS / PAUSED_REFRESH_RATE);

	// if time moved backwards (reset), or if it's been more than 1 second in emulated time, resync
	if (compare_mame_times(emutime, throttle_emutime) < 0 || sub_mame_times(emutime, throttle_emutime).seconds > 0)
		goto resync;

	// get the current realtime; if it's been more than 1 second realtime, just resync
	cps = osd_ticks_per_second();
	diffticks = osd_ticks() - throttle_last_ticks;
	throttle_last_ticks += diffticks;
	if (diffticks >= cps)
		goto resync;

	// add the time that has passed to the real time
	subsecs_per_cycle = MAX_SUBSECONDS / cps;
	throttle_realtime = add_subseconds_to_mame_time(throttle_realtime, diffticks * subsecs_per_cycle);

	// update the emulated time
	throttle_emutime = emutime;

	// if we're behind, just sync
	if (compare_mame_times(throttle_emutime, throttle_realtime) <= 0)
		goto resync;

	// determine our target ticks value
	target = throttle_last_ticks + sub_mame_times(throttle_emutime, throttle_realtime).subseconds / subsecs_per_cycle;

	// initialize the ticks per sleep
	if (ticks_per_sleep_msec == 0)
		ticks_per_sleep_msec = (double)(cps / 1000);

	// this counts as idle time
	profiler_mark(PROFILER_IDLE);

	// determine whether or not we are allowed to sleep
	allowed_to_sleep = video_config.sleep && (!effective_autoframeskip() || effective_frameskip() == 0);

	// sync
	for (curr = osd_ticks(); curr - target < 0; curr = osd_ticks())
	{
		// if we have enough time to sleep, do it
		// ...but not if we're autoframeskipping and we're behind
		if (paused || (allowed_to_sleep && (target - curr) > (osd_ticks_t)(ticks_per_sleep_msec * 1.1)))
		{
			osd_ticks_t next;

			// keep track of how long we actually slept
			Sleep(1);
			next = osd_ticks();
			ticks_per_sleep_msec = (ticks_per_sleep_msec * 0.90) + ((double)(next - curr) * 0.10);
		}
	}

	// idle time done
	profiler_mark(PROFILER_END);

	// update realtime
	diffticks = osd_ticks() - throttle_last_ticks;
	throttle_last_ticks += diffticks;
	throttle_realtime = add_subseconds_to_mame_time(throttle_realtime, diffticks * subsecs_per_cycle);
	return;

resync:
	// reset realtime and emutime to the same value
	throttle_realtime = throttle_emutime = emutime;
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
		winwindow_toggle_full_screen();

#ifdef MESS
	// check for toggling menu bar
	if (input_ui_pressed(IPT_OSD_2))
		win_toggle_menubar();
#endif

	// check for fast forward
	video_config.fastforward = input_port_type_pressed(IPT_OSD_3, 0);
	if (video_config.fastforward)
		ui_show_fps_temp(0.5);
}



//============================================================
//  extract_video_config
//============================================================

static void extract_video_config(void)
{
	const char *stemp;

	// performance options: extract the data
	video_config.autoframeskip = options_get_bool("autoframeskip");
	video_config.frameskip     = options_get_int_range("frameskip", 0, FRAMESKIP_LEVELS);
	video_config.throttle      = options_get_bool("throttle");
	video_config.sleep         = options_get_bool("sleep");

	// misc options: extract the data
	video_config.framestorun   = options_get_int("frames_to_run");

	// global options: extract the data
	video_config.windowed      = options_get_bool("window");
	video_config.prescale      = options_get_int("prescale");
	video_config.keepaspect    = options_get_bool("keepaspect");
	video_config.numscreens    = options_get_int_range("numscreens", 1, MAX_WINDOWS);
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

	// video options: extract the data
	stemp = options_get_string("video");
	if (strcmp(stemp, "d3d") == 0)
		video_config.mode = VIDEO_MODE_D3D;
	else if (strcmp(stemp, "ddraw") == 0)
		video_config.mode = VIDEO_MODE_DDRAW;
	else if (strcmp(stemp, "gdi") == 0)
		video_config.mode = VIDEO_MODE_GDI;
	else if (strcmp(stemp, "none") == 0)
	{
		video_config.mode = VIDEO_MODE_NONE;
		if (video_config.framestorun == 0)
			fprintf(stderr, "Warning: -video none doesn't make much sense without -frames_to_run\n");
	}
	else
	{
		fprintf(stderr, "Invalid video value %s; reverting to gdi\n", stemp);
		video_config.mode = VIDEO_MODE_GDI;
	}
	video_config.waitvsync     = options_get_bool("waitvsync");
	video_config.syncrefresh   = options_get_bool("syncrefresh");
	video_config.triplebuf     = options_get_bool("triplebuffer");
	video_config.switchres     = options_get_bool("switchres");

	// ddraw options: extract the data
	video_config.hwstretch     = options_get_bool("hwstretch");

	// d3d options: extract the data
	video_config.filter        = options_get_bool("filter");
	if (video_config.prescale == 0)
		video_config.prescale = 1;

	// misc options: sanity check values

	// per-window options: sanity check values

	// d3d options: sanity check values
	options_get_int_range("d3dversion", 8, 9);

	options_get_float_range("full_screen_brightness", 0.1f, 2.0f);
	options_get_float_range("full_screen_contrast", 0.1f, 2.0f);
	options_get_float_range("full_screen_gamma", 0.1f, 3.0f);
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
	const char *defdata = options_get_string("aspect");
	const char *data = options_get_string(name);
	int num = 0, den = 1;

	if (strcmp(data, "auto") == 0)
	{
		if (strcmp(defdata, "auto") == 0)
			return 0;
		data = defdata;
	}
	if (sscanf(data, "%d:%d", &num, &den) != 2 && report_error)
		fprintf(stderr, "Illegal aspect ratio value for %s = %s\n", name, data);
	return (float)num / (float)den;
}



//============================================================
//  get_resolution
//============================================================

static void get_resolution(const char *name, win_window_config *config, int report_error)
{
	const char *defdata = options_get_string("resolution");
	const char *data = options_get_string(name);

	config->width = config->height = config->refresh = 0;
	if (strcmp(data, "auto") == 0)
	{
		if (strcmp(defdata, "auto") == 0)
			return;
		data = defdata;
	}
	if (sscanf(data, "%dx%d@%d", &config->width, &config->height, &config->refresh) < 2 && report_error)
		fprintf(stderr, "Illegal resolution value for %s = %s\n", name, data);
}
