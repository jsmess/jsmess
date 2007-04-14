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

// we override SDL's normal startup on Win32
#ifdef SDLMAME_WIN32
#undef main
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifdef SDLMAME_UNIX
#include <signal.h>
#include <unistd.h>
#endif

#if defined(SDLMAME_X11) && (SDL_MAJOR_VERSION == 1) && (SDL_MINOR_VERSION == 2)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#ifdef MESS
static char cwd[512];
#endif

// from config.c
int  cli_frontend_init (int argc, char **argv);
void cli_frontend_exit (void);

// from sdltime.c
extern int sdl_use_rdtsc;

// from sound.c
int sdl_init_audio(running_machine *machine);

//============================================================
//	GLOBAL VARIABLES
//============================================================

int verbose;

//============================================================
//	LOCAL VARIABLES
//============================================================

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
	int game_index;
	int res = 0;

	#ifdef MESS
	getcwd(cwd, 511);
	#endif

	// parse config and cmdline options
	game_index = cli_frontend_init (argc, argv);


	#ifdef SDLMAME_DARWIN

	#ifdef X86_ASM	// Intel OS X only
	sdl_use_rdtsc = options_get_bool(mame_options(), "rdtsc");
	#else
	sdl_use_rdtsc = options_get_bool(mame_options(), "machtmr");
	#endif	// X86_ASM
	#else	// DARWIN
	#ifdef X86_ASM	// Intel only
	sdl_use_rdtsc = options_get_bool(mame_options(), "rdtsc");
	#endif
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

	// have we decided on a game?
	if (game_index != -1)
		res = run_game(game_index);
	else
		res = -1;

	cli_frontend_exit();

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
void led_init(void);

int osd_init(running_machine *machine)
{
	extern int win_init_input(running_machine *machine);
	extern int win_erroroslog;
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

	result = wininput_init(machine);

	sdl_init_audio(machine);

	#ifdef MESS
	SDL_EnableUNICODE(1);
	#endif

	if(win_erroroslog)
		add_logerror_callback(machine, output_oslog);

	return result;
}

#ifdef MESS
char *osd_get_startup_cwd(void)
{
	return cwd;
}
#endif
