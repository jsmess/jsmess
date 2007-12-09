//============================================================
//
//  window.c - SDL window handling
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//  SDLMAME by Olivier Galibert and R. Belmont
//
//============================================================

// standard SDL headers
#include <SDL/SDL.h>
#include <SDL/SDL_syswm.h>
#include <SDL/SDL_thread.h>

#include <time.h>
#if defined(SDLMAME_UNIX) && !defined(SDLMAME_DARWIN)
#include <sys/time.h>
#endif
#if defined(SDLMAME_MACOSX)
#include <sys/time.h>
#endif

#include "osd_opengl.h"

// standard C headers
#include <math.h>
#include <unistd.h>

// MAME headers
#include "osdepend.h"
#include "driver.h"
#include "window.h"
#include "video.h"
#include "input.h"
#include "options.h"
#include "ui.h"
#include "osdsdl.h"

#ifdef MESS
#include "menu.h"
#endif /* MESS */

//============================================================
//  PARAMETERS
//============================================================

#if 1
#define ASSERT_USE(x)	assert_always(SDL_ThreadID() == x, "Wrong Thread")
//#define ASSERT_USE(x)
#else
#define ASSERT_USE(x)	assert(SDL_ThreadID() == x)
#endif

#define ASSERT_REDRAW_THREAD() ASSERT_USE(window_threadid)
#define ASSERT_WINDOW_THREAD() ASSERT_USE(window_threadid)
#define ASSERT_MAIN_THREAD() ASSERT_USE(main_threadid)

// minimum window dimension
#define MIN_WINDOW_DIM					200

#ifndef SDLMAME_WIN32
#define WMSZ_TOP		(0)
#define WMSZ_BOTTOM		(1)
#define WMSZ_BOTTOMLEFT		(2)
#define WMSZ_BOTTOMRIGHT	(3)
#define WMSZ_LEFT		(4)
#define WMSZ_TOPLEFT		(5)
#define WMSZ_TOPRIGHT		(6)
#define WMSZ_RIGHT		(7)
#endif

//============================================================
//  GLOBAL VARIABLES
//============================================================

sdl_window_info *sdl_window_list;


//============================================================
//  LOCAL VARIABLES
//============================================================

static sdl_window_info **last_window_ptr;
static int shown_video_info = 0;

// event handling
static int multithreading_enabled;
static osd_work_queue *work_queue;
static int main_threadid;
static int window_threadid;
static int dll_loaded;


static UINT32 last_update_time;

static const char *yuv_mode_names[] = { "none", "yv12", "yv12x2", "yuy2", "yuy2x2" };

// debugger
//static int in_background;

//static int ui_temp_pause;
//static int ui_temp_was_paused;

//static UINT32 last_update_time;

static sdl_draw_callbacks draw;

typedef struct _worker_param worker_param;
struct _worker_param {
	sdl_window_info *window;
	const render_primitive_list *list;
	int resize_new_width;
	int resize_new_height;
	int times;
};


//============================================================
//  PROTOTYPES
//============================================================

static void sdlwindow_exit(running_machine *machine);
static void sdlwindow_video_window_destroy(sdl_window_info *window);
static void *draw_video_contents_wt(void *param, int threadid);
static void *sdlwindow_video_window_destroy_wt(void *param, int threadid);
static void *sdlwindow_resize_wt(void *param, int threadid);
static void *sdlwindow_toggle_full_screen_wt(void *param, int threadid);
#if USE_OPENGL
static void sdlwindow_init_ogl_context(void);
#endif
static void get_min_bounds(sdl_window_info *window, int *window_width, int *window_height, int constrain);

static void *complete_create_wt(void *param, int threadid);
static void set_starting_view(int index, sdl_window_info *window, const char *view);

//============================================================
//  clear the worker_param structure, inline - faster than memset
//============================================================

INLINE void clear_worker_param(worker_param *wp)
{
	wp->window=NULL;
	wp->list=NULL;
	wp->resize_new_width=0;
	wp->resize_new_height=0;
	wp->times=0;
}

//============================================================
//  execute_async
//============================================================

INLINE void execute_async(osd_work_callback callback, worker_param *wp)
{
	worker_param *wp_temp = NULL;
	
	if (wp)
	{
		wp_temp = (worker_param *) malloc(sizeof(worker_param));
		*wp_temp = *wp;
	}
	if (multithreading_enabled)
	{
		osd_work_item_queue(work_queue, callback, (void *) wp_temp, WORK_ITEM_FLAG_AUTO_RELEASE);
	} else
		callback((void *) wp_temp, 0);
}

//============================================================
//  execute_async_wait
//============================================================

INLINE void execute_async_wait(osd_work_callback callback, worker_param *wp)
{
	execute_async(callback, wp);
	sdlwindow_sync();
}

//============================================================
//  sdlwindow_thread_id
//  (window thread)
//============================================================

static void *sdlwindow_thread_id(void *param, int threadid)
{
	window_threadid = SDL_ThreadID();

	#ifdef SDLMAME_WIN32
	if (SDL_Init(SDL_INIT_TIMER|SDL_INIT_AUDIO| SDL_INIT_VIDEO| SDL_INIT_JOYSTICK|SDL_INIT_NOPARACHUTE)) {
		mame_printf_error("Could not initialize SDL: %s.\n", SDL_GetError());
		exit(-1);
	}
	#endif
	return NULL;
}

//============================================================
//  win_init_window
//  (main thread)
//============================================================

int sdlwindow_init(running_machine *machine)
{
	// determine if we are using multithreading or not
	multithreading_enabled = options_get_bool(mame_options(), SDLOPTION_MULTITHREADING);

	// get the main thread ID before anything else
	main_threadid = SDL_ThreadID();

	// ensure we get called on the way out
	add_exit_callback(machine, sdlwindow_exit);

	// if multithreading, create a thread to run the windows
	if (multithreading_enabled)
	{
		// create a thread to run the windows from
		work_queue = osd_work_queue_alloc(0);
		if (work_queue == NULL)
			return 1;
		osd_work_item_queue(work_queue, &sdlwindow_thread_id, NULL, WORK_ITEM_FLAG_AUTO_RELEASE);
	}
	else
	{
		// otherwise, treat the window thread as the main thread
		//window_threadid = main_threadid;
		sdlwindow_thread_id(NULL, 0);
	}

	// initialize the drawers
	if (video_config.mode == VIDEO_MODE_OPENGL)
	{
		if (drawogl_init(&draw))
			video_config.mode = VIDEO_MODE_SOFT;
	}
	if (video_config.mode == VIDEO_MODE_SOFT)
	{
		if (drawsdl_init(&draw))
			return 1;
	}

	// set up the window list
	last_window_ptr = &sdl_window_list;
	dll_loaded = 0;
	return 0;
}


//============================================================
//  sdlwindow_exit
//  (main thread)
//============================================================

void _sdlwindow_sync(const char *s, int line)
{
	if (multithreading_enabled)
	{
		osd_work_queue_wait(work_queue, osd_ticks_per_second()*10);
	}
}

//============================================================
//  sdlwindow_exit
//  (main thread)
//============================================================

#ifdef SDLMAME_WIN32
static void *sdlwindow_exit_wt(void *param, int threadid)
{
	SDL_Quit();
	return NULL;
}
#endif

static void sdlwindow_exit(running_machine *machine)
{
	ASSERT_MAIN_THREAD();

	// free all the windows
	while (sdl_window_list != NULL)
	{
		sdl_window_info *temp = sdl_window_list;
		sdl_window_list = temp->next;
		sdlwindow_video_window_destroy(temp);
	}
	
	// if we're multithreaded, clean up the window thread
	if (multithreading_enabled)
	{
		sdlwindow_sync();
	}

	// kill the drawers
	(*draw.exit)();

	#ifdef SDLMAME_WIN32
	execute_async_wait(&sdlwindow_exit_wt, NULL);
	#endif
	if (multithreading_enabled)
	{
		osd_work_queue_wait(work_queue, 100000);
		osd_work_queue_free(work_queue);
	}

}

//============================================================
//  sdlwindow_resize
//  (main thread)
//============================================================

void compute_blit_surface_size(sdl_window_info *window, int window_width, int window_height);
#if USE_OPENGL
void drawsdl_destroy_all_textures(sdl_window_info *window);
#endif

static void yuv_overlay_init(sdl_window_info *window)
{
	int minimum_width, minimum_height;

	render_target_get_minimum_size(window->target, &minimum_width, &minimum_height);
	if (window->yuvsurf != NULL)
		SDL_FreeYUVOverlay(window->yuvsurf);
	if (window->yuv_bitmap != NULL)
	{
		free(window->yuv_bitmap);	
	}
	window->yuv_bitmap = malloc_or_die(minimum_width*minimum_height*sizeof(UINT16));

	mame_printf_verbose("SDL: Creating YUV-Overlay ...\n");
	switch (video_config.yuv_mode)
	{
		case VIDEO_YUV_MODE_YV12: 
			window->yuvsurf = SDL_CreateYUVOverlay(minimum_width, minimum_height,
        			SDL_YV12_OVERLAY, window->sdlsurf);
			break;
		case VIDEO_YUV_MODE_YV12X2: 
			window->yuvsurf = SDL_CreateYUVOverlay(minimum_width*2, minimum_height*2,
	      			SDL_YV12_OVERLAY, window->sdlsurf);
			break;
		case VIDEO_YUV_MODE_YUY2: 
			window->yuvsurf = SDL_CreateYUVOverlay(minimum_width, minimum_height,
	      			SDL_YUY2_OVERLAY, window->sdlsurf);
			break;
		case VIDEO_YUV_MODE_YUY2X2: 
			window->yuvsurf = SDL_CreateYUVOverlay(minimum_width*2, minimum_height,
	      			SDL_YUY2_OVERLAY, window->sdlsurf);
			break;
	}
	if ( window->yuvsurf == NULL ) {
		mame_printf_error("SDL: Couldn't create SDL_yuv_overlay: %s\n", SDL_GetError());
		//return 1;
	}
	window->yuv_ovl_width = minimum_width;
	window->yuv_ovl_height = minimum_height;

	if (!shown_video_info)
	{
		mame_printf_verbose("YUV Mode         : %s\n", yuv_mode_names[video_config.yuv_mode]);
		mame_printf_verbose("YUV Overlay Size : %d x %d\n", minimum_width, minimum_height);
		mame_printf_verbose("YUV Acceleration : %s\n", window->yuvsurf->hw_overlay ? "Hardware" : "Software");
		shown_video_info = 1;
	}
}

static void *sdlwindow_resize_wt(void *param, int threadid)
{
	sdl_window_info *window = sdl_window_list;
	worker_param *wp = (worker_param *) param;

	ASSERT_WINDOW_THREAD();
	
#if USE_OPENGL
	drawsdl_destroy_all_textures(window);
#endif
#if (SDL_VERSIONNUM(SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL) < 1300)
	SDL_FreeSurface(window->sdlsurf);
#endif
	//printf("SetVideoMode %d %d\n", wp->resize_new_width, wp->resize_new_height);
	if (video_config.yuv_mode == VIDEO_YUV_MODE_NONE)
		window->sdlsurf = SDL_SetVideoMode(wp->resize_new_width, wp->resize_new_height, 0, SDL_SWSURFACE |
			SDL_DOUBLEBUF | SDL_ANYFORMAT | window->extra_flags);
	else
	{
		window->sdlsurf = SDL_SetVideoMode(wp->resize_new_width, wp->resize_new_height, 0, SDL_HWSURFACE |
			SDL_DOUBLEBUF | SDL_ANYFORMAT | window->extra_flags);
		yuv_overlay_init(window);
	}
#if USE_OPENGL
	if (window->opengl)
		sdlwindow_init_ogl_context();
#endif
	compute_blit_surface_size(window, wp->resize_new_width, wp->resize_new_height);
	free(wp);
	return NULL;
}

void sdlwindow_resize(INT32 width, INT32 height)
{
	sdl_window_info *window = sdl_window_list;
	worker_param wp;

	ASSERT_MAIN_THREAD();

	if (width == window->sdlsurf->w && height == window->sdlsurf->h)
		return;
	
	clear_worker_param(&wp);
	wp.resize_new_width = width;
	wp.resize_new_height = height;
	
	execute_async_wait(&sdlwindow_resize_wt, &wp);
}

//============================================================
//  sdlwindow_clear_surface
//  (window thread)
//============================================================

static void *sdlwindow_clear_surface_wt(void *param, int threadid)
{
	sdl_window_info *window = sdl_window_list;
	worker_param *wp = (worker_param *) param;
	int i;

	ASSERT_WINDOW_THREAD();

	for (i=0;i < wp->times; i++)
	{
		if (SDL_MUSTLOCK(window->sdlsurf)) SDL_LockSurface(window->sdlsurf);

		memset(window->sdlsurf->pixels, 0, window->sdlsurf->h * window->sdlsurf->pitch);
		if (SDL_MUSTLOCK(window->sdlsurf)) SDL_UnlockSurface(window->sdlsurf);

		SDL_Flip(window->sdlsurf);
	}

	free(wp);
	return NULL;
}

void sdlwindow_clear_surface(sdl_window_info *window, int times)
{
	worker_param *wp = malloc(sizeof(worker_param));

	clear_worker_param(wp);
	wp->window = window;
	wp->times = times;
	
	if (SDL_ThreadID() == main_threadid)
	{
		execute_async_wait(&sdlwindow_clear_surface_wt, wp);
		free(wp);
	}
	else 
		sdlwindow_clear_surface_wt( (void *) wp, 0);
}

//============================================================
//  sdlwindow_toggle_full_screen
//  (main thread)
//============================================================

static void *sdlwindow_toggle_full_screen_wt(void *param, int threadid)
{
	worker_param *wp = (worker_param *) param;
	sdl_window_info *window = wp->window;

	ASSERT_WINDOW_THREAD();

#ifdef MAME_DEBUG
	// if we are in debug mode, never go full screen
	if (options_get_bool(mame_options(), OPTION_DEBUG))
		return NULL;
#endif

	// If we are going fullscreen (leaving windowed) remember our windowed size
	if (!window->fullscreen)
	{
		window->windowed_width = window->sdlsurf->w;
		window->windowed_height = window->sdlsurf->h;
	}

	// toggle the window mode
	video_config.windowed ^= 1;

	(*draw.window_destroy)(window);

	window->fullscreen = !video_config.windowed;

	complete_create_wt(param, 0);

	return NULL;
}

void sdlwindow_toggle_full_screen(void)
{
	worker_param wp;
	sdl_window_info *window = sdl_window_list;

	ASSERT_MAIN_THREAD();

	clear_worker_param(&wp);
	wp.window = window;
	
	execute_async_wait(&sdlwindow_toggle_full_screen_wt, &wp);
}

static void sdlwindow_modify_yuv(int dir)
{
	sdl_window_info *window = sdl_window_list;
	int new_yuv_mode = video_config.yuv_mode;

	if (dir > 0 && video_config.yuv_mode < VIDEO_YUV_MODE_MAX)
		new_yuv_mode = video_config.yuv_mode + 1;
	if (dir < 0 && video_config.yuv_mode > VIDEO_YUV_MODE_MIN)
		new_yuv_mode = video_config.yuv_mode - 1;
	
	if (new_yuv_mode != video_config.yuv_mode)
	{
		worker_param wp;

		clear_worker_param(&wp);

		wp.window = window;

		execute_async_wait(&sdlwindow_video_window_destroy_wt, &wp);
			
		video_config.yuv_mode = new_yuv_mode;

		execute_async_wait(&complete_create_wt, &wp);

		ui_popup_time(1, "YUV mode %s", yuv_mode_names[video_config.yuv_mode]);
	}
}

#if USE_OPENGL
static void *destroy_all_textures_wt(void *param, int threadid)
{
	worker_param *wp = (worker_param *) param;

	sdl_window_info *window = wp->window;

	drawsdl_destroy_all_textures(window);

	free(wp);
	return NULL;
}
#endif

void sdlwindow_modify_prescale(int dir)
{
	sdl_window_info *window = sdl_window_list;
	worker_param wp;
	int new_prescale = video_config.prescale;

	clear_worker_param(&wp);

	wp.window = window;

	if (dir > 0 && video_config.prescale < 3)
		new_prescale = video_config.prescale + 1;
	if (dir < 0 && video_config.prescale > 1)
		new_prescale = video_config.prescale - 1;
	
	if (new_prescale != video_config.prescale)
	{
		if (!video_config.windowed && video_config.switchres)
		{
			execute_async_wait(&sdlwindow_video_window_destroy_wt, &wp);
			
			video_config.prescale = new_prescale;

			execute_async_wait(&complete_create_wt, &wp);

		}
		else
		{
#if USE_OPENGL
			execute_async_wait(destroy_all_textures_wt, &wp);
#endif
			video_config.prescale = new_prescale;
		}
		ui_popup_time(1, "Prescale %d", video_config.prescale);
	}
}

void sdlwindow_modify_effect(int dir)
{
#if USE_OPENGL
	worker_param wp;
	sdl_window_info *window = sdl_window_list;
#endif
	int new_prescale_effect = video_config.prescale_effect;
	const char *effect_names[] = { "none", "scale2x" };

	if (video_config.mode == VIDEO_MODE_SOFT) 
	{
		sdlwindow_modify_yuv(dir);
		return;
	}	

	if (dir > 0 && video_config.prescale_effect < 1)
		new_prescale_effect = video_config.prescale_effect + 1;
	if (dir < 0 && video_config.prescale_effect > 0)
		new_prescale_effect = video_config.prescale_effect - 1;
	
	if (new_prescale_effect != video_config.prescale_effect)
	{
#if USE_OPENGL
		clear_worker_param(&wp);
		wp.window = window;
		execute_async_wait(destroy_all_textures_wt, &wp);
#endif
		video_config.prescale_effect = new_prescale_effect;
		ui_popup_time(1, "Effect: %s", effect_names[video_config.prescale_effect]);
	}
}

void sdlwindow_toggle_draw(void)
{
	sdl_window_info *window = sdl_window_list;
	worker_param wp;

	// If we are not fullscreen (windowed) remember our windowed size
	if (!window->fullscreen)
	{
		window->windowed_width = window->sdlsurf->w;
		window->windowed_height = window->sdlsurf->h;
	}

	clear_worker_param(&wp);

	wp.window = window;
	execute_async_wait(&sdlwindow_video_window_destroy_wt, &wp);

	video_config.yuv_mode = VIDEO_YUV_MODE_NONE;

	if (video_config.mode == VIDEO_MODE_OPENGL)
	{
		video_config.mode = VIDEO_MODE_SOFT;
		drawsdl_init(&draw);
		ui_popup_time(1, "Using software rendering");
	}
	else
	{
		video_config.mode = VIDEO_MODE_OPENGL;
		drawogl_init(&draw);
		ui_popup_time(1, "Using OpenGL rendering");
	}

	execute_async_wait(&complete_create_wt, &wp);
}

//============================================================
//  sdlwindow_update_cursor_state
//  (main or window thread)
//============================================================

void sdlwindow_update_cursor_state(void)
{
	// do not do mouse capture if the debugger's enabled to avoid
	// the possibility of losing control
	#ifdef MAME_DEBUG
	if (!options_get_bool(mame_options(), OPTION_DEBUG))
	#endif
	{
		if (video_config.windowed && !sdlinput_should_hide_mouse())
		{
			SDL_ShowCursor(SDL_ENABLE);
			if (SDL_WM_GrabInput(SDL_GRAB_QUERY))
			{
				SDL_WM_GrabInput(SDL_GRAB_OFF);
			}
		}
		else
		{
			SDL_ShowCursor(SDL_DISABLE);
			if (!SDL_WM_GrabInput(SDL_GRAB_QUERY))
			{
				SDL_WM_GrabInput(SDL_GRAB_ON);
			}
		}
	}
}

//============================================================
//  sdlwindow_video_window_create
//  (main thread)
//============================================================

int sdlwindow_video_window_create(int index, sdl_monitor_info *monitor, const sdl_window_config *config)
{
	sdl_window_info *window;
	worker_param *wp = malloc(sizeof(worker_param));
	char option[20];
	int result;

	ASSERT_MAIN_THREAD();

	clear_worker_param(wp);

	// allocate a new window object
	window = malloc_or_die(sizeof(*window));
	memset(window, 0, sizeof(*window));
	window->maxwidth = config->width;
	window->maxheight = config->height;
	window->depth = config->depth;
	window->refresh = config->refresh;
	window->monitor = monitor;
	window->fullscreen = !video_config.windowed;
	if (!window->fullscreen)
	{
		window->windowed_width = config->width;
		window->windowed_height = config->height;
	}
  	window->totalColors = config->totalColors;

	// add us to the list
	*last_window_ptr = window;
	last_window_ptr = &window->next;

	// create a lock that we can use to skip blitting
	window->render_lock = osd_lock_alloc();

	// load the layout
	window->target = render_target_alloc(NULL, FALSE);
	if (window->target == NULL)
		goto error;

	// set the specific view				    
	sprintf(option, SDLOPTION_VIEW("%d"), index);
	set_starting_view(index, window, options_get_string(mame_options(), option));

	// make the window title
	if (video_config.numscreens == 1)
		sprintf(window->title, APPNAME ": %s [%s]", Machine->gamedrv->description, Machine->gamedrv->name);
	else
		sprintf(window->title, APPNAME ": %s [%s] - Screen %d", Machine->gamedrv->description, Machine->gamedrv->name, index);

	wp->window = window;
	if (multithreading_enabled)
	{
		osd_work_item *wi;		
		
		wi = osd_work_item_queue(work_queue, &complete_create_wt, (void *) wp, 0);
		sdlwindow_sync();
		result = *((int *) (osd_work_item_result)(wi));
		osd_work_item_release(wi);
	} 
	else
		result = *((int *) complete_create_wt((void *) wp, 0));

	// handle error conditions
	if (result == 1)
		goto error;

	return 0;

error:
	sdlwindow_video_window_destroy(window);
	return 1;
}

//============================================================
//  sdlwindow_video_window_destroy
//  (main thread)
//============================================================

static void *sdlwindow_video_window_destroy_wt(void *param, int threadid)
{
	worker_param *wp = (worker_param *) param;
	sdl_window_info *window = wp->window;

	ASSERT_WINDOW_THREAD();

	if (window->sdlsurf)
	{
#if (SDL_VERSIONNUM(SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL) < 1300)
		// Will be done by SDL on next call to SetVideoMode
		SDL_FreeSurface(window->sdlsurf);
#endif
		window->sdlsurf = NULL;
	}

	// free the textures etc
	(*draw.window_destroy)(window);

	free(wp);
	return NULL;
}

static void sdlwindow_video_window_destroy(sdl_window_info *window)
{
	sdl_window_info **prevptr;
	worker_param wp;

	ASSERT_MAIN_THREAD();
	if (multithreading_enabled)
	{
		sdlwindow_sync();
	}

	osd_lock_acquire(window->render_lock);

	// remove us from the list
	for (prevptr = &sdl_window_list; *prevptr != NULL; prevptr = &(*prevptr)->next)
		if (*prevptr == window)
		{
			*prevptr = window->next;
			break;
		}

	// free the textures etc
	clear_worker_param(&wp);
	wp.window = window;
	execute_async_wait(&sdlwindow_video_window_destroy_wt, &wp);

	// free the render target, after the textures!
	if (window->target != NULL)
		render_target_free(window->target);

	// free the lock
	osd_lock_free(window->render_lock);

	// free the window itself
	free(window);
}

//============================================================
//  pick_best_mode
//============================================================

static void pick_best_mode(sdl_window_info *window, int *fswidth, int *fsheight)
{
	int minimum_width, minimum_height, target_width, target_height;
	SDL_Rect **modes;
	int i;
	float size_score, best_score = 0.0f;

	// determine the minimum width/height for the selected target
	render_target_get_minimum_size(window->target, &minimum_width, &minimum_height);

	// use those as the target for now
	target_width = minimum_width * MAX(1, video_config.prescale);
	target_height = minimum_height * MAX(1, video_config.prescale);

	// if we're not stretching, allow some slop on the minimum since we can handle it
	{
		minimum_width -= 4;
		minimum_height -= 4;
	}

	modes = SDL_ListModes(NULL, SDL_FULLSCREEN | SDL_DOUBLEBUF);

	if (modes == (SDL_Rect **)0)
	{
		mame_printf_error("SDL: No modes available?!\n");
		exit(-1);
	}
	else if (modes == (SDL_Rect **)-1)	// all modes are possible
	{
		*fswidth = window->maxwidth;
		*fsheight = window->maxheight;
	}
	else
	{
		for (i = 0; modes[i]; ++i)
		{
			// compute initial score based on difference between target and current
			size_score = 1.0f / (1.0f + fabs((INT32)modes[i]->w - target_width) + fabs((INT32)modes[i]->h - target_height));

			// if the mode is too small, give a big penalty
			if (modes[i]->w < minimum_width || modes[i]->h < minimum_height)
				size_score *= 0.01f;

			// if mode is smaller than we'd like, it only scores up to 0.1
			if (modes[i]->w < target_width || modes[i]->h < target_height)
				size_score *= 0.1f;

			// if we're looking for a particular mode, that's a winner
			if (modes[i]->w == window->maxwidth && modes[i]->h == window->maxheight)
				size_score = 2.0f;

			mame_printf_verbose("%4dx%4d -> %f\n", (int)modes[i]->w, (int)modes[i]->h, size_score);

			// best so far?
			if (size_score > best_score)
			{
				best_score = size_score;
				*fswidth = modes[i]->w;
				*fsheight = modes[i]->h;
			}

		}
	}
}



//============================================================
//  sdlwindow_video_window_update
//  (main thread)
//============================================================

void sdlwindow_video_window_update(sdl_window_info *window)
{

	ASSERT_MAIN_THREAD();

	// adjust the cursor state
	sdlwindow_update_cursor_state();

	// if we're visible and running and not in the middle of a resize, draw
	if (window->target != NULL)
	{
		int got_lock = TRUE;
		int tempwidth, tempheight;
		
		// see if the games video mode has changed
		render_target_get_minimum_size(window->target, &tempwidth, &tempheight);
		if (tempwidth != window->minwidth || tempheight != window->minheight)
		{
			window->minwidth = tempwidth;
			window->minheight = tempheight;
			if (!window->fullscreen)
			{
				sdl_info *sdl = window->dxdata;
				compute_blit_surface_size(window, window->sdlsurf->w, window->sdlsurf->h);
				sdlwindow_resize(sdl->blitwidth, sdl->blitheight);
			}
			else if (video_config.switchres)
			{
				pick_best_mode(window, &tempwidth, &tempheight);
				sdlwindow_resize(tempwidth, tempheight);
			}

			if (video_config.yuv_mode != VIDEO_YUV_MODE_NONE)
			{
				yuv_overlay_init(window);
			}
		}

// this: 'osd_lock_acquire', just to 'see' if rendering was still happening
// is invalid.
// 'osd_lock_acquire' puts us to the waiting queue and we will miss to be in sync with real-time
// but throttle actually means to be in real time, i.e. to not render a few frames,
// while rendering is still in progress is all fine ..
#if 0
		// only block if we're throttled
		if (video_config.throttle) // || timeGetTime() - last_update_time > 250)
			osd_lock_acquire(window->render_lock);
		else
#endif
			got_lock = osd_lock_try(window->render_lock);

		// only render if we were able to get the lock
		if (got_lock)
		{
			worker_param wp;
			const render_primitive_list *primlist;

			clear_worker_param(&wp);

			// don't hold the lock; we just used it to see if rendering was still happening
			osd_lock_release(window->render_lock);

			// ensure the target bounds are up-to-date, and then get the primitives
			primlist = (*draw.window_get_primitives)(window);

			// and redraw now
			last_update_time = SDL_GetTicks();
			wp.list = primlist;
			wp.window = window;

			execute_async(&draw_video_contents_wt, &wp);
		}
	}
}


//============================================================
//  set_starting_view
//  (main thread)
//============================================================

static void set_starting_view(int index, sdl_window_info *window, const char *view)
{
	int viewindex = -1;

	ASSERT_MAIN_THREAD();

	// NULL is the same as auto
	if (view == NULL)
		view = "auto";

	// auto view just selects the nth view
	if (strcmp(view, "auto") != 0)
	{
		// scan for a matching view name
		for (viewindex = 0; ; viewindex++)
		{
			const char *name = render_target_get_view_name(window->target, viewindex);

			// stop scanning if we hit NULL
			if (name == NULL)
			{
				viewindex = -1;
				break;
			}
			if (mame_strnicmp(name, view, strlen(view)) == 0)
				break;
		}
	}

	// if we don't have a match, default to the nth view
	if (viewindex == -1)
	{
		int scrcount;

		// count the number of screens
		for (scrcount = 0; Machine->drv->screen[scrcount].tag != NULL; scrcount++) ;

		// if we have enough screens to be one per monitor, assign in order
		if (video_config.numscreens >= scrcount)
		{
			// find the first view with this screen and this screen only
			for (viewindex = 0; ; viewindex++)
			{
				UINT32 viewscreens = render_target_get_view_screens(window->target, viewindex);
				if (viewscreens == (1 << index))
					break;
				if (viewscreens == 0)
				{
					viewindex = -1;
					break;
				}
			}
		}

		// otherwise, find the first view that has all the screens
		if (viewindex == -1)
		{
			for (viewindex = 0; ; viewindex++)
			{
				UINT32 viewscreens = render_target_get_view_screens(window->target, viewindex);
				if (viewscreens == (1 << scrcount) - 1)
					break;
				if (viewscreens == 0)
					break;
			}
		}
	}

	// make sure it's a valid view
	if (render_target_get_view_name(window->target, viewindex) == NULL)
		viewindex = 0;

	// set the view
	render_target_set_view(window->target, viewindex);
	window->start_viewscreen=viewindex;
}



#if USE_OPENGL
static void sdlwindow_init_ogl_context(void)
{
	// do some one-time OpenGL setup
	glShadeModel(GL_SMOOTH);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
}
#endif

//============================================================
//  complete_create
//  (window thread)
//============================================================

static void *complete_create_wt(void *param, int threadid)
{
	
	worker_param *wp = (worker_param *) param;
	sdl_window_info *window = wp->window;
	int tempwidth, tempheight;
	sdl_info *sdl;
	static int result[2] = {0,1};

	ASSERT_WINDOW_THREAD();
	free(wp);

	// initialize the drawing backend
	if ((*draw.window_init)(window))
		return (void *) &result[1];

	sdl = window->dxdata;

	if (window->fullscreen)
	{

		window->extra_flags = SDL_FULLSCREEN;
		// default to the current mode exactly
		tempwidth = window->monitor->monitor_width;
		tempheight = window->monitor->monitor_height;

		// if we're allowed to switch resolutions, override with something better
		if (video_config.switchres)
			pick_best_mode(window, &tempwidth, &tempheight);
	}
	else if (window->windowed_width)
	{
		window->extra_flags = SDL_RESIZABLE;
		// if we have a remembered size force the new window size to it
		tempwidth = window->windowed_width;
		tempheight = window->windowed_height;
	}
	else
	{
		window->extra_flags = SDL_RESIZABLE;

		/* Create the window directly with the correct aspect
		   instead of letting compute_blit_surface_size() resize it
		   this stops the window from "flashing" from the wrong aspect
		   size to the right one at startup. */
		tempwidth = (window->maxwidth != 0) ? window->maxwidth : 640;
		tempheight = (window->maxheight != 0) ? window->maxheight : 480;

		get_min_bounds(window, &tempwidth, &tempheight, video_config.keepaspect );
	}

	if (window->opengl)
	{
		window->extra_flags |= SDL_OPENGL;

 		SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
		
		#if (SDL_VERSIONNUM(SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL) >= 1210)
		#if (SDL_VERSIONNUM(SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL) < 1300)
		if (options_get_bool(mame_options(), SDLOPTION_WAITVSYNC))
		{
			SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);
		}
		else
		{
			SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 0);
		}
		#endif
		#endif
	}

#ifndef NO_OPENGL
#ifdef USE_DISPATCH_GL
	if (window->opengl && !dll_loaded)
	{
		/*
		 *  directfb and and x11 use this env var
		 *   SDL_VIDEO_GL_DRIVER
		 */
		const char *e;
		e=getenv("SDLMAME_GL_LIB");
#ifdef SDLMAME_MACOSX
		/* Vas Crabb: Default GL-lib for MACOSX */
		if (!e)
			e = "/System/Library/Frameworks/OpenGL.framework/Libraries/libGL.dylib";
#endif
		if (SDL_GL_LoadLibrary(e) != 0) // Load library (default for e==NULL
		{
			fatalerror("Unable to load default library: %s\n", e);
		}
       	mame_printf_verbose("Loaded opengl shared library: %s\n", e ? e : "<default>");
    	gl_dispatch = auto_malloc(sizeof(osd_gl_dispatch));
        dll_loaded=1;
	}
#endif
#endif
	// create the SDL surface (which creates the window in windowed mode)
	if (video_config.yuv_mode == VIDEO_YUV_MODE_NONE)
		window->sdlsurf = SDL_SetVideoMode(tempwidth, tempheight, 
						   0, SDL_SWSURFACE | SDL_DOUBLEBUF | SDL_ANYFORMAT | window->extra_flags);
	else
		window->sdlsurf = SDL_SetVideoMode(tempwidth, tempheight, 
					   0, SDL_SWSURFACE | SDL_ANYFORMAT |window->extra_flags);

	if (!window->sdlsurf)
		return (void *) &result[1];
		
	if ( window->opengl && !(window->sdlsurf->flags & SDL_OPENGL) )
	{
		fprintf(stderr, "OpenGL not supported on this driver!\n");
		return (void *) &result[1];
	}

#if USE_OPENGL
	if (window->opengl)
	{
		char *extstr;
		char *vendor;
		int has_and_allow_texturerect = 0;
		
		/* load any GL function addresses
		 * this must be done here because we need a context
		 */ 
		sdlvideo_loadgl();

		extstr = (char *)glGetString(GL_EXTENSIONS);
		vendor = (char *)glGetString(GL_VENDOR);

		// print out the driver info for debugging
		if (!shown_video_info)
		{
			mame_printf_verbose("OpenGL: %s\nOpenGL: %s\nOpenGL: %s\n", vendor, (char *)glGetString(GL_RENDERER), (char *)glGetString(GL_VERSION));
		}

		sdl->usetexturerect = 0;
		sdl->texpoweroftwo = 1;
		sdl->usevbo = 0;
		sdl->usepbo = 0;
		sdl->usefbo = 0;
		sdl->useglsl = 0;

		if ( video_config.allowtexturerect &&
		     ( strstr(extstr, "GL_ARB_texture_rectangle") ||  strstr(extstr, "GL_EXT_texture_rectangle") )
		   )
		{
			has_and_allow_texturerect = 1;
                        if (!shown_video_info)
                        {
                                mame_printf_verbose("OpenGL: texture rectangle supported\n");
                        }
		}

		// does this card support non-power-of-two sized textures?  (they're faster, so use them if possible)
		if ( !video_config.forcepow2texture && strstr(extstr, "GL_ARB_texture_non_power_of_two"))
		{
			if (!shown_video_info)
			{
				mame_printf_verbose("OpenGL: non-power-of-2 textures supported (new method)\n");
			}
                        sdl->texpoweroftwo = 0;
		}
		else
		{
			// second chance: GL_ARB_texture_rectangle or GL_EXT_texture_rectangle (old version)
			if (has_and_allow_texturerect)
			{
				if (!shown_video_info)
				{
					mame_printf_verbose("OpenGL: non-power-of-2 textures supported (old method)\n");
				}
				sdl->usetexturerect = 1;
			}
			else
			{
				if (!shown_video_info)
				{
					mame_printf_verbose("OpenGL: forcing power-of-2 textures (creation, not copy)\n");
				}
			}
		}

		if (strstr(extstr, "GL_ARB_vertex_buffer_object"))
		{
                        sdl->usevbo = video_config.vbo;
			if (!shown_video_info)
			{
				if(sdl->usevbo)
					mame_printf_verbose("OpenGL: vertex buffer supported\n");
				else
					mame_printf_verbose("OpenGL: vertex buffer supported, but disabled\n");
			}
		}

		if (strstr(extstr, "GL_ARB_pixel_buffer_object"))
		{
			if( sdl->usevbo )
			{
				sdl->usepbo = video_config.pbo;
				if (!shown_video_info)
				{
					if(sdl->usepbo)
						mame_printf_verbose("OpenGL: pixel buffers supported\n");
					else
						mame_printf_verbose("OpenGL: pixel buffers supported, but disabled\n");
				}
			} else {
				if (!shown_video_info)
				{
					mame_printf_verbose("OpenGL: pixel buffers supported, but disabled due to disabled vbo\n");
				}
			}
		}
		else
		{
			if (!shown_video_info)
			{
				mame_printf_verbose("OpenGL: pixel buffers not supported\n");
			}
		}
		
		if (strstr(extstr, "GL_EXT_framebuffer_object"))
		{
                        sdl->usefbo = 1;
			if (!shown_video_info)
			{
				if(sdl->usefbo)
					mame_printf_verbose("OpenGL: framebuffer object supported\n");
				else
					mame_printf_verbose("OpenGL: framebuffer object not supported\n");
			}
		}

		if (strstr(extstr, "GL_ARB_shader_objects") &&
		    strstr(extstr, "GL_ARB_shading_language_100") &&
		    strstr(extstr, "GL_ARB_vertex_shader") &&
		    strstr(extstr, "GL_ARB_fragment_shader") 
		   )
		{
			sdl->useglsl = video_config.glsl;
			if (!shown_video_info)
			{
				if(sdl->useglsl)
					mame_printf_verbose("OpenGL: GLSL supported\n");
				else
					mame_printf_verbose("OpenGL: GLSL supported, but disabled\n");
			}
		} else {
			if (!shown_video_info)
			{
				mame_printf_verbose("OpenGL: GLSL not supported\n");
			}
		}

		sdl->glsl_vid_attributes = video_config.glsl_vid_attributes;

		if (getenv("SDLMAME_VMWARE"))
		{
			sdl->usetexturerect = 1;
			sdl->texpoweroftwo = 1;
		}
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint *)&sdl->texture_max_width);
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint *)&sdl->texture_max_height);
		if (!shown_video_info)
		{
			mame_printf_verbose("OpenGL: max texture size %d x %d\n", sdl->texture_max_width, sdl->texture_max_height);
		}

		shown_video_info = 1;

		sdlwindow_init_ogl_context();
	}
#endif
	if (video_config.yuv_mode != VIDEO_YUV_MODE_NONE)
	{
		drawsdl_yuv_init(window);
		yuv_overlay_init(window);
	}

	// set the window title
	SDL_WM_SetCaption(window->title, "SDLMAME");

	return (void *) &result[0];
}

//============================================================
//  draw_video_contents
//  (window thread)
//============================================================

#ifndef SDLMAME_WIN32
static int64_t getusecs(void)
{
    struct timeval tv;
    int64_t res=-1;

    if ( 0==gettimeofday(&tv, NULL) )
    {
        res  = tv.tv_sec;
        res *= 1000000L;
        res += tv.tv_usec;
    }

    return res;
}
#endif

static void *draw_video_contents_wt(void * param, int threadid)
{
	#ifndef SDLMAME_WIN32
	const unsigned long frames_skip4fps = 100;
	static int64_t lastTime=0, sumdt=0, startTime=0;
	static unsigned long frames = 0;
	int64_t currentTime, t0;
	double dt;
	#endif
	UINT32 	dc =		0;
	int 	update = 	1;
	worker_param *wp = (worker_param *) param;
	sdl_window_info *window = wp->window;
	
	ASSERT_REDRAW_THREAD();
	#ifdef SDLMAME_WIN32
	sdlinput_process_events_buf();
	#endif

	window->primlist = wp->list;
	osd_lock_acquire(window->render_lock);
	if (window->sdlsurf)
	{
		// if no bitmap, just fill
		if (window->primlist == NULL)
		{
		}

		// otherwise, render with our drawing system
		else
		{
			#ifndef SDLMAME_WIN32
			if( video_config.perftest )
			{
				t0 = getusecs();

				(*draw.window_draw)(window, dc, update);

				frames++;
				currentTime = getusecs();
				if(startTime==0||frames==frames_skip4fps)
					startTime=currentTime;
				if( frames>=frames_skip4fps )
					sumdt+=currentTime-t0;
				if( (currentTime-lastTime)>1L*1000000L && frames>frames_skip4fps )
				{
					dt = (double)( ((currentTime-startTime)/10000L) ) / 100.0 ; // in decimale sec.
					mame_printf_info("%6.2lfs, %4lu F, "
				          "avrg game: %5.2lf FPS %.2lf ms/f, "
				          "avrg video: %5.2lf FPS %.2lf ms/f, "
				          "last video: %5.2lf FPS %.2lf ms/f\n", 
					  dt, frames-frames_skip4fps, 
					  (double)(frames-frames_skip4fps)/dt,                             // avrg game fps
					  ( (currentTime-startTime) / ((frames-frames_skip4fps)) ) /1000.0,
					  (double)(frames-frames_skip4fps)/((double)(sumdt/10000L)/100.0), // avrg vid fps
					  ( sumdt / ((frames-frames_skip4fps)) ) /1000.0,
					  1.0/((currentTime-t0) / 1000000.0), // this vid fps
					  (currentTime-t0) / 1000.0
					);
					lastTime = currentTime;
				}
			} 
			else 
			#endif
			{
				(*draw.window_draw)(window, dc, update);
			}
		}
	}
	osd_lock_release(window->render_lock);
	free(wp);
	
	return NULL;
}

//============================================================
//  constrain_to_aspect_ratio
//  (window thread)
//============================================================

static void constrain_to_aspect_ratio(sdl_window_info *window, int *window_width, int *window_height, int adjustment)
{
	INT32 extrawidth = 0;
	INT32 extraheight = 0;
	INT32 propwidth, propheight;
	INT32 minwidth, minheight;
	INT32 maxwidth, maxheight;
	INT32 viswidth, visheight;
	float pixel_aspect;

	// make sure the monitor is up-to-date
	sdlvideo_monitor_refresh(window->monitor);

	// get the pixel aspect ratio for the target monitor
	pixel_aspect = sdlvideo_monitor_get_aspect(window->monitor);

	// determine the proposed width/height
	propwidth = *window_width - extrawidth;
	propheight = *window_height - extraheight;

	// based on which edge we are adjusting, take either the width, height, or both as gospel
	// and scale to fit using that as our parameter
	switch (adjustment)
	{
		case WMSZ_BOTTOM:
		case WMSZ_TOP:
			render_target_compute_visible_area(window->target, 10000, propheight, pixel_aspect, render_target_get_orientation(window->target), &propwidth, &propheight);
			break;

		case WMSZ_LEFT:
		case WMSZ_RIGHT:
			render_target_compute_visible_area(window->target, propwidth, 10000, pixel_aspect, render_target_get_orientation(window->target), &propwidth, &propheight);
			break;

		default:
			render_target_compute_visible_area(window->target, propwidth, propheight, pixel_aspect, render_target_get_orientation(window->target), &propwidth, &propheight);
			break;
	}

	// get the minimum width/height for the current layout
	render_target_get_minimum_size(window->target, &minwidth, &minheight);

	// clamp against the absolute minimum
	propwidth = MAX(propwidth, MIN_WINDOW_DIM);
	propheight = MAX(propheight, MIN_WINDOW_DIM);

	// clamp against the minimum width and height
	propwidth = MAX(propwidth, minwidth);
	propheight = MAX(propheight, minheight);

	// clamp against the maximum (fit on one screen for full screen mode)
	if (window->fullscreen)
	{
		maxwidth = window->monitor->center_width - extrawidth;
		maxheight = window->monitor->center_height - extraheight;
	}
	else
	{
		maxwidth = window->monitor->center_width - extrawidth;
		maxheight = window->monitor->center_height - extraheight;

		// further clamp to the maximum width/height in the window
		if (window->maxwidth != 0)
			maxwidth = MIN(maxwidth, window->maxwidth + extrawidth);
		if (window->maxheight != 0)
			maxheight = MIN(maxheight, window->maxheight + extraheight);
	}

	// clamp to the maximum
	propwidth = MIN(propwidth, maxwidth);
	propheight = MIN(propheight, maxheight);

	// compute the visible area based on the proposed rectangle
	render_target_compute_visible_area(window->target, propwidth, propheight, pixel_aspect, render_target_get_orientation(window->target), &viswidth, &visheight);

	*window_width = viswidth;
	*window_height = visheight;
}

//============================================================
//  get_min_bounds
//  (window thread)
//============================================================

static void get_min_bounds(sdl_window_info *window, int *window_width, int *window_height, int constrain)
{
	INT32 minwidth, minheight;

	// get the minimum target size
	render_target_get_minimum_size(window->target, &minwidth, &minheight);

	// expand to our minimum dimensions
	if (minwidth < MIN_WINDOW_DIM)
		minwidth = MIN_WINDOW_DIM;
	if (minheight < MIN_WINDOW_DIM)
		minheight = MIN_WINDOW_DIM;

	// if we want it constrained, figure out which one is larger
	if (constrain)
	{
		int test1w, test1h;
		int test2w, test2h;

		// first constrain with no height limit
		test1w = minwidth; test1h = 10000;
		constrain_to_aspect_ratio(window, &test1w, &test1h, WMSZ_BOTTOMRIGHT);

		// then constrain with no width limit
		test2w = 10000; test2h = minheight;
		constrain_to_aspect_ratio(window, &test2w, &test2h, WMSZ_BOTTOMRIGHT);

		// pick the larger
		if ( test1w > test2w )
		{
			minwidth = test1w;
			minheight = test1h;
		}
		else
		{
			minwidth = test2w;
			minheight = test2h;
		}
	}

	*window_width = minwidth;
	*window_height = minheight;
}
