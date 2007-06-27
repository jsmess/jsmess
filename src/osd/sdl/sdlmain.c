//============================================================
//
//   sdlmain.c -- SDL main program
//
//============================================================

// standard sdl header
#include <SDL/SDL.h>
#include <SDL/SDL_version.h>

// standard includes
#include <time.h>
#include <ctype.h>
#include <stdlib.h>

// MAME headers
#include "driver.h"
#include "window.h"
#include "options.h"
#include "clifront.h"
#include "input.h"

#include "osdsdl.h"

// we override SDL's normal startup on Win32
#ifdef SDLMAME_WIN32
#undef main
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#endif

#ifdef SDLMAME_UNIX
#include <signal.h>
#include <unistd.h>
#endif

#if defined(SDLMAME_X11) && (SDL_MAJOR_VERSION == 1) && (SDL_MINOR_VERSION == 2)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#ifdef SDLMAME_OS2
#define INCL_DOS
#include <os2.h>

void MorphToPM()
{
  PPIB pib;
  PTIB tib;

  DosGetInfoBlocks(&tib, &pib);

  // Change flag from VIO to PM:
  if (pib->pib_ultype==2) pib->pib_ultype = 3;
}
#endif

#ifdef MESS
static char cwd[512];
#endif

//============================================================
//	GLOBAL VARIABLES
//============================================================

int verbose;

//============================================================
//	LOCAL VARIABLES
//============================================================

//============================================================
//  OPTIONS
//============================================================

static const options_entry mame_sdl_options[] =
{
#if defined(SDLMAME_WIN32) || defined(SDLMAME_MACOSX) || defined(SDLMAME_OS2)
	{ "inipath",                     ".;ini",     0,                 "path to ini files" },
#else
#if defined(INI_PATH)
	{ "inipath",                     INI_PATH,     0,                "path to ini files" },
#else
#ifdef MESS
	{ "inipath",                     "$HOME/.mess;.;ini",     0,     "path to ini files" },
#else
	{ "inipath",                     "$HOME/.mame;.;ini",     0,     "path to ini files" },
#endif // MESS
#endif // INI_PATH
#endif // MACOSX
	

	// debugging options
	{ NULL,                       NULL,       OPTION_HEADER,     "DEBUGGING OPTIONS" },
	{ "oslog",                    "0",        OPTION_BOOLEAN,    "output error.log data to the system debugger" },
	{ "verbose;v",                "0",        OPTION_BOOLEAN,    "display additional diagnostic information" },

	// performance options
	{ NULL,                       NULL,       OPTION_HEADER,     "PERFORMANCE OPTIONS" },
	{ "multithreading;mt",        "0",        OPTION_BOOLEAN,    "enable multithreading; this enables rendering and blitting on a separate thread" },
	#ifndef SDLMAME_WIN32
	{ "sdlvideofps",              "0",        OPTION_BOOLEAN,    "show sdl video performance" },
	#endif

	// video options
	{ NULL,                       NULL,       OPTION_HEADER,     "VIDEO OPTIONS" },
	{ "video",                    "soft",     0,                 "video output method: soft or opengl" },
	{ "numscreens",               "1",        0,                 "number of screens to create; SDLMAME only supports 1 at this time" },
	{ "window;w",                 "0",        OPTION_BOOLEAN,    "enable window mode; otherwise, full screen mode is assumed" },
	{ "maximize;max",             "1",        OPTION_BOOLEAN,    "default to maximized windows; otherwise, windows will be minimized" },
	{ "keepaspect;ka",            "1",        OPTION_BOOLEAN,    "constrain to the proper aspect ratio" },
	{ "unevenstretch;ues",        "1",        OPTION_BOOLEAN,    "allow non-integer stretch factors" },
	{ "effect",                   "none",     0,                 "name of a PNG file to use for visual effects, or 'none'" },
	{ "pause_brightness",         "0.65",     0,                 "amount to scale the screen brightness when paused" },
	{ "centerh",                  "1",        OPTION_BOOLEAN,    "center horizontally within the view area" },
	{ "centerv",                  "1",        OPTION_BOOLEAN,    "center vertically within the view area" },
	#if (SDL_VERSIONNUM(SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL) >= 1210)
	{ "waitvsync",                "0",        OPTION_BOOLEAN,    "enable waiting for the start of VBLANK before flipping screens; reduces tearing effects" },
	#endif
	{ "yuvmode;ym",               "none",     0,                 "YUV mode: none, yv12, yuy2, yv12x2, yuy2x2 (-video soft only)" },

	// OpenGL specific options
	{ NULL,                       NULL,   OPTION_HEADER,  "OpenGL-SPECIFIC OPTIONS" },
	{ "filter;glfilter;flt",      "1",    OPTION_BOOLEAN, "enable bilinear filtering on screen output" },
	{ "prescale",                 "1",        0,                 "scale screen rendering by this amount in software" },
	{ "gl_forcepow2texture",      "0",    OPTION_BOOLEAN, "force power of two textures  (default no)" },
	{ "gl_notexturerect",         "0",    OPTION_BOOLEAN, "don't use OpenGL GL_ARB_texture_rectangle (default on)" },
	{ "gl_vbo",                   "1",    OPTION_BOOLEAN, "enable OpenGL VBO,  if available (default on)" },
	{ "gl_pbo",                   "1",    OPTION_BOOLEAN, "enable OpenGL PBO,  if available (default on)" },
	{ "gl_glsl",                  "0",    OPTION_BOOLEAN, "enable OpenGL GLSL, if available (default off)" },
	{ "gl_glsl_filter",           "1",        0,          "enable OpenGL GLSL filtering instead of FF filtering (default 1) 0-plain, 1-bilinear, 2-gaussian/blurry)" },
	{ "gl_glsl_vid_attr",         "1",    OPTION_BOOLEAN, "enable OpenGL GLSL handling of brightness and contrast. Better RGB game performance for free. (default)" },

	// per-window options
	{ NULL,                       NULL,       OPTION_HEADER,     "PER-WINDOW VIDEO OPTIONS" },
	{ "screen",                   "auto",     0,                 "explicit name of the first screen; 'auto' here will try to make a best guess" },
	{ "aspect;screen_aspect",     "auto",     0,                 "aspect ratio for all screens; 'auto' here will try to make a best guess" },
	{ "resolution;r",             "auto",     0,                 "preferred resolution for all screens; format is <width>x<height>[@<refreshrate>] or 'auto'" },
	{ "view",                     "auto",     0,                 "preferred view for all screens" },

	{ "screen0",                  "auto",     0,                 "explicit name of the first screen; 'auto' here will try to make a best guess" },
	{ "aspect0",                  "auto",     0,                 "aspect ratio of the first screen; 'auto' here will try to make a best guess" },
	{ "resolution0;r0",           "auto",     0,                 "preferred resolution of the first screen; format is <width>x<height>[@<refreshrate>] or 'auto'" },
	{ "view0",                    "auto",     0,                 "preferred view for the first screen" },

	{ "screen1",                  "auto",     0,                 "explicit name of the second screen; 'auto' here will try to make a best guess" },
	{ "aspect1",                  "auto",     0,                 "aspect ratio of the second screen; 'auto' here will try to make a best guess" },
	{ "resolution1;r1",           "auto",     0,                 "preferred resolution of the second screen; format is <width>x<height>[@<refreshrate>] or 'auto'" },
	{ "view1",                    "auto",     0,                 "preferred view for the second screen" },

	{ "screen2",                  "auto",     0,                 "explicit name of the third screen; 'auto' here will try to make a best guess" },
	{ "aspect2",                  "auto",     0,                 "aspect ratio of the third screen; 'auto' here will try to make a best guess" },
	{ "resolution2;r2",           "auto",     0,                 "preferred resolution of the third screen; format is <width>x<height>[@<refreshrate>] or 'auto'" },
	{ "view2",                    "auto",     0,                 "preferred view for the third screen" },

	{ "screen3",                  "auto",     0,                 "explicit name of the fourth screen; 'auto' here will try to make a best guess" },
	{ "aspect3",                  "auto",     0,                 "aspect ratio of the fourth screen; 'auto' here will try to make a best guess" },
	{ "resolution3;r3",           "auto",     0,                 "preferred resolution of the fourth screen; format is <width>x<height>[@<refreshrate>] or 'auto'" },
	{ "view3",                    "auto",     0,                 "preferred view for the fourth screen" },

	// full screen options
	{ NULL,                       NULL,       OPTION_HEADER,     "FULL SCREEN OPTIONS" },
	{ "switchres",                "0",        OPTION_BOOLEAN,    "enable resolution switching" },
	#ifdef SDLMAME_X11
	{ "useallheads",	      "0",	  OPTION_BOOLEAN,    "split full screen image across monitors" },
	#endif

	// sound options
	{ NULL,                       NULL,       OPTION_HEADER,     "SOUND OPTIONS" },
	{ "audio_latency",            "3",        0,                 "set audio latency (increase to reduce glitches, decrease for responsiveness)" },

	// input options
	{ NULL,                       NULL,       OPTION_HEADER,     "INPUT DEVICE OPTIONS" },
	{ "mouse",                    "0",        OPTION_BOOLEAN,    "enable mouse input" },
	{ "joystick;joy",             "0",        OPTION_BOOLEAN,    "enable joystick input" },
	{ "steadykey;steady",         "0",        OPTION_BOOLEAN,    "enable steadykey support" },
	{ "a2d_deadzone;a2d",         "0.3",      0,                 "minimal analog value for digital input" },
	{ "digital",                  "none",     0,                 "mark certain joysticks or axes as digital (none|all|j<N>*|j<N>a<M>[,...])" },

	{ NULL,                       NULL,       OPTION_HEADER,     "AUTOMATIC DEVICE SELECTION OPTIONS" },
	{ "paddle_device;paddle",     "keyboard", 0,                 "enable (keyboard|mouse|joystick) if a paddle control is present" },
	{ "adstick_device;adstick",   "keyboard", 0,                 "enable (keyboard|mouse|joystick) if an analog joystick control is present" },
	{ "pedal_device;pedal",       "keyboard", 0,                 "enable (keyboard|mouse|joystick) if a pedal control is present" },
	{ "dial_device;dial",         "keyboard", 0,                 "enable (keyboard|mouse|joystick) if a dial control is present" },
	{ "trackball_device;trackball","keyboard", 0,                "enable (keyboard|mouse|joystick) if a trackball control is present" },
	{ "lightgun_device",          "keyboard", 0,                 "enable (keyboard|mouse|joystick) if a lightgun control is present" },
	{ "positional_device",        "keyboard", 0,                 "enable (keyboard|mouse|joystick) if a positional control is present" },
#ifdef MESS
	{ "mouse_device",             "mouse",    0,                 "enable (keyboard|mouse|joystick) if a mouse control is present" },
#endif

	// keyboard mapping
	{ NULL, 		      NULL,       OPTION_HEADER,     "SDL KEYBOARD MAPPING" },
	{ "keymap",                   "0",        OPTION_BOOLEAN,    "enable keymap" },
	{ "keymap_file",              "keymap.dat", 0,               "keymap filename" },

	// joystick mapping
	{ NULL, 		      NULL,       OPTION_HEADER,     "SDL JOYSTICK MAPPING" },
	{ "joymap",                   "0",        OPTION_BOOLEAN,    "enable physical to logical joystick mapping" },
	{ "joymap_file",              "joymap.dat", 0,               "joymap filename" },
	{ NULL }
};

//============================================================
//  verbose_printf
//============================================================

void verbose_printf(const char *text, ...)
{
	if (verbose)
	{
		va_list arg;

		/* dump to the buffer */
		va_start(arg, text);
		vprintf(text, arg);
		va_end(arg);
	}
}

//============================================================
//	main
//============================================================

// we do some special sauce on Win32...
#ifdef SDLMAME_WIN32
int SDL_main(int argc, char **argv);

/* Show an error message */
static void ShowError(const char *title, const char *message)
{
	fprintf(stderr, "%s: %s\n", title, message);
}

static BOOL OutOfMemory(void)
{
	ShowError("Fatal Error", "Out of memory - aborting");
	return FALSE;
}

int main(int argc, char *argv[])
{
	size_t n;
	char *bufp, *appname;
	int status;

	/* Get the class name from argv[0] */
	appname = argv[0];
	if ( (bufp=SDL_strrchr(argv[0], '\\')) != NULL ) {
		appname = bufp+1;
	} else
	if ( (bufp=SDL_strrchr(argv[0], '/')) != NULL ) {
		appname = bufp+1;
	}

	if ( (bufp=SDL_strrchr(appname, '.')) == NULL )
		n = SDL_strlen(appname);
	else
		n = (bufp-appname);

	bufp = SDL_stack_alloc(char, n+1);
	if ( bufp == NULL ) {
		return OutOfMemory();
	}
	SDL_strlcpy(bufp, appname, n+1);
	appname = bufp;

	/* Load SDL dynamic link library */
	if ( SDL_Init(SDL_INIT_NOPARACHUTE) < 0 ) {
		ShowError("WinMain() error", SDL_GetError());
		return(FALSE);
	}


	/* Sam:
	   We still need to pass in the application handle so that
	   DirectInput will initialize properly when SDL_RegisterApp()
	   is called later in the video initialization.
	 */
	SDL_SetModuleHandle(GetModuleHandle(NULL));

	/* Run the application main() code */
	status = SDL_main(argc, argv);

	/* Exit cleanly, calling atexit() functions */
	exit(status);

	/* Hush little compiler, don't you cry... */
	return status;
}
#endif

#ifndef SDLMAME_WIN32
int main(int argc, char **argv)
#else
int SDL_main(int argc, char **argv)
#endif
{
	int res = 0;

	#ifdef SDLMAME_OS2
	MorphToPM();
	#endif

	#ifdef MESS
	getcwd(cwd, 511);
	#endif

#if defined(SDLMAME_X11) && (SDL_MAJOR_VERSION == 1) && (SDL_MINOR_VERSION == 2)
	if (SDL_Linked_Version()->patch < 10)
	/* workaround for SDL choosing a 32-bit ARGB visual */
	{
		Display *display;
		if ((display = XOpenDisplay(NULL)) && (DefaultDepth(display, DefaultScreen(display)) >= 24))
		{
			XVisualInfo vi;
			char buf[130];
			if (XMatchVisualInfo(display, DefaultScreen(display), 24, TrueColor, &vi)) {
				snprintf(buf, sizeof(buf), "0x%lx", vi.visualid);
				setenv("SDL_VIDEO_X11_VISUALID", buf, 0);
			}
		}
		if (display)
			XCloseDisplay(display);
	}
#endif 

	res = cli_execute(argc, argv, mame_sdl_options);

#ifdef MALLOC_DEBUG
	{
		void check_unfreed_mem(void);
		check_unfreed_mem();
	}
#endif

	SDL_Quit();
	
	exit(res);

	return res;
}



//============================================================
//  output_oslog
//============================================================

static void output_oslog(running_machine *machine, const char *buffer)
{
	fputs(buffer, stderr);
}



//============================================================
//	osd_exit
//============================================================

static void osd_exit(running_machine *machine)
{

	#ifndef SDLMAME_WIN32
	SDL_Quit();
	#endif
}



//============================================================
//	osd_init
//============================================================
int osd_init(running_machine *machine)
{
	int result;

	#ifndef SDLMAME_WIN32
	if (SDL_Init(SDL_INIT_TIMER|SDL_INIT_AUDIO| SDL_INIT_VIDEO| SDL_INIT_JOYSTICK|SDL_INIT_NOPARACHUTE)) {
		fprintf(stderr, "Could not initialize SDL: %s.\n", SDL_GetError());
		exit(-1);
	}
	#endif
	// must be before sdlvideo_init!
	add_exit_callback(machine, osd_exit);

	sdlvideo_init(machine);
	if (getenv("SDLMAME_UNSUPPORTED"))
		led_init();

	result = sdlinput_init(machine);

	sdl_init_audio(machine);

	if (options_get_bool(mame_options(), "oslog"))
		add_logerror_callback(machine, output_oslog);

	#ifdef MESS
	SDL_EnableUNICODE(1);
	#endif

	return result;
}

#ifdef MESS
char *osd_get_startup_cwd(void)
{
	return cwd;
}
#endif
