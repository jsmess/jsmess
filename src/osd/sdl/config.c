//============================================================
//
//  config.c - SDLMAME configuration routines
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//  SDLMAME by Olivier Galibert and R. Belmont
//
//============================================================

// standard headers
#include <SDL/SDL.h>

// standard C headers
#include <stdarg.h>
#include <ctype.h>
#include <time.h>

// MAME headers
#include "osdepend.h"
#include "driver.h"
#include "video.h"
#include "render.h"
#include "rendutil.h"
#include "debug/debugcpu.h"
#include "debug/debugcon.h"
#include "emuopts.h"


//============================================================
//  EXTERNALS
//============================================================

int frontend_listxml(FILE *output);
int frontend_listfull(FILE *output);
int frontend_listsource(FILE *output);
int frontend_listclones(FILE *output);
int frontend_listcrc(FILE *output);
int frontend_listroms(FILE *output);
int frontend_listsamples(FILE *output);
int frontend_verifyroms(FILE *output);
int frontend_verifysamples(FILE *output);
int frontend_romident(FILE *output);
int frontend_isknown(FILE *output);

#ifdef MESS
int frontend_listdevices(FILE *output);
void sdl_mess_options_parse(void);
#endif /* MESS */

void set_pathlist(int file_type, const char *new_rawpath);


//============================================================
//  GLOBALS
//============================================================

int win_erroroslog;

extern int audio_latency;
extern const char *wavwrite;



//============================================================
//  PROTOTYPES
//============================================================

static void parse_ini_file(const char *name);
static void execute_simple_commands(void);
static void execute_commands(const char *argv0);
static void display_help(void);
static char *extract_base_name(const char *name, char *dest, int destsize);
static char *extract_path(const char *name, char *dest, int destsize);



//============================================================
//  OPTIONS
//============================================================

// struct definitions
static const options_entry sdl_opts[] =
{
	// debugging options
	{ NULL,                       NULL,       OPTION_HEADER,     "DEBUGGING OPTIONS" },
	{ "oslog",                    "0",        OPTION_BOOLEAN,    "output error.log data to the system debugger" },
	{ "verbose;v",                "0",        OPTION_BOOLEAN,    "display additional diagnostic information" },

	// performance options
	{ NULL,                       NULL,       OPTION_HEADER,     "PERFORMANCE OPTIONS" },
	#if defined(SDLMAME_WIN32) || (defined(SDLMAME_UNIX) && defined(X86_ASM))
	{ "rdtsc",                    "0",    OPTION_BOOLEAN, "use the RDTSC instruction for timing on x86; faster but may result in uneven performance" },
	#endif
	#if defined(SDLMAME_DARWIN) && !defined(X86_ASM)
	{ "machtmr",                  "0",    OPTION_BOOLEAN, "use Mach timers for timing on PowerPC; alternative is SDL's built-in" },
	#endif
	{ "multithreading;mt",        "0",        OPTION_BOOLEAN,    "enable multithreading; this enables rendering and blitting on a separate thread" },

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
	{ "16bpp_texfmt",             "auto",     0,                 "how 16bpp data is send to the card: auto, argb1555, rgba5551, rgb565, argb8" },

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
	{ NULL }
};

static const options_entry config_unsupported_opts[] =
{
	{ NULL,                       NULL,       OPTION_HEADER,     "UNSUPPORTED OPTIONS" },
	{ "prescale_effect;pse",      "0",        0,                 "use effect X during prescaling (opengl only)" },
	{ NULL }
};

#ifdef MESS
#include "configms.h"
#endif



//============================================================
//  INLINES
//============================================================

INLINE int is_directory_separator(char c)
{
	return (c == '\\' || c == '/' || c == ':');
}


//============================================================
//  sdl_options_init
//============================================================

static void sdl_options_init(void)
{
	mame_options_init(sdl_opts);
}


//============================================================
//  cli_frontend_init
//============================================================

int cli_frontend_init(int argc, char **argv)
{
	const char *gamename;
	machine_config drv;
	char basename[20];
	char buffer[512];
	int drvnum = -1;

	// initialize the options manager
	sdl_options_init();
	if (getenv("SDLMAME_UNSUPPORTED"))
		options_add_entries(mame_options(),config_unsupported_opts);

	// override the default inipath with more appropriate values for SDLMAME
#if defined(SDLMAME_WIN32) || defined(SDLMAME_MACOSX)
	options_set_option_default_value(mame_options(),"inipath", ".;ini");
#else
#if defined(INI_PATH)
	options_set_option_default_value(mame_options(),"inipath", INI_PATH);
#else
#ifdef MESS
	options_set_option_default_value(mame_options(),"inipath", "$HOME/.mess;.;ini");
#else
	options_set_option_default_value(mame_options(),"inipath", "$HOME/.mame;.;ini");
#endif // MESS
#endif // INI_PATH
#endif // MACOSX

	// parse the command line first; if we fail here, we're screwed
	if (options_parse_command_line(mame_options(), argc, argv))
		exit(1);

#ifdef MESS
	sdl_mess_options_parse();
#endif

	// parse the simple commmands before we go any further
	execute_simple_commands();

	// find out what game we might be referring to
	gamename = options_get_string(mame_options(),OPTION_GAMENAME);
	if (gamename != NULL)
		drvnum = driver_get_index(extract_base_name(gamename, basename, ARRAY_LENGTH(basename)));

	// now parse the core set of INI files
	parse_ini_file(CONFIGNAME);
	parse_ini_file(extract_base_name(argv[0], buffer, ARRAY_LENGTH(buffer)));
#ifdef MAME_DEBUG
	parse_ini_file("debug");
#endif

	// if we have a valid game driver, parse game-specific INI files
	if (drvnum != -1)
	{
		const game_driver *driver = drivers[drvnum];
		const game_driver *parent = driver_get_clone(driver);
		const game_driver *gparent = (parent != NULL) ? driver_get_clone(parent) : NULL;

		// expand the machine driver to look at the info
		expand_machine_driver(driver->drv, &drv);

		// parse vector.ini for vector games
		if (drv.video_attributes & VIDEO_TYPE_VECTOR)
			parse_ini_file("vector");

		// then parse sourcefile.ini
		parse_ini_file(extract_base_name(driver->source_file, buffer, ARRAY_LENGTH(buffer)));

		// then parent the grandparent, parent, and game-specific INIs
		if (gparent != NULL)
			parse_ini_file(gparent->name);
		if (parent != NULL)
			parse_ini_file(parent->name);
		parse_ini_file(driver->name);
	}

	// reparse the command line to ensure its options override all
	// note that we re-fetch the gamename here as it will get overridden
	options_parse_command_line(mame_options(),argc, argv);
	gamename = options_get_string(mame_options(),OPTION_GAMENAME);

	// execute any commands specified
	execute_commands(argv[0]);

	// if no driver specified, display help
	if (gamename == NULL)
	{
		display_help();
		exit(1);
	}

	// if we don't have a valid driver selected, offer some suggestions
	if (drvnum == -1)
	{
		int matches[10];
		fprintf(stderr, "\n\"%s\" approximately matches the following\n"
				"supported " GAMESNOUN " (best match first):\n\n", basename);
		driver_get_approx_matches(basename, ARRAY_LENGTH(matches), matches);
		for (drvnum = 0; drvnum < ARRAY_LENGTH(matches); drvnum++)
			if (matches[drvnum] != -1)
				fprintf(stderr, "%-10s%s\n", drivers[matches[drvnum]]->name, drivers[matches[drvnum]]->description);
		exit(1);
	}

	// check if we are running in unsupported mode and warn if we are
	if (getenv("SDLMAME_UNSUPPORTED"))
	{
		fprintf(stderr,
			"WARNING: unsupported features have been enabled by the \"SDLMAME_UNSUPPORTED\"\n"
			"   environment variable. Do NOT report any bugs in this mode!\n"
			"   If you experience problems try to reproduce them with \"SDLMAME_UNSUPPORTED\"\n"
			"   unset and ONLY report them if you can reproduce them this way.\n");
	}
	
	// extract the directory name
	extract_path(gamename, buffer, ARRAY_LENGTH(buffer));
	if (buffer[0] != 0)
	{
		// okay, we got one; prepend it to the rompath
		const char *rompath = options_get_string(mame_options(),"rompath");
		if (rompath == NULL)
			options_set_string(mame_options(),"rompath", buffer);
		else
		{
			char *newpath = malloc_or_die(strlen(rompath) + strlen(buffer) + 2);
			sprintf(newpath, "%s;%s", buffer, rompath);
			options_set_string(mame_options(),"rompath", newpath);
			free(newpath);
		}
	}

{
	extern int verbose;
	verbose = options_get_bool(mame_options(),"verbose");
}

	return drvnum;
}



//============================================================
//  cli_frontend_exit
//============================================================

void cli_frontend_exit(void)
{
	// free the options that we added previously
	options_free(mame_options());
}



//============================================================
//  parse_ini_file
//============================================================

static void parse_ini_file(const char *name)
{
	file_error filerr;
	mame_file *file;
	char *fname;

	// don't parse if it has been disabled
	if (!options_get_bool(mame_options(),"readconfig"))
		return;

	// open the file; if we fail, that's ok
	fname = assemble_2_strings(name, ".ini");
	filerr = mame_fopen(SEARCHPATH_INI, fname, OPEN_FLAG_READ, &file);
	free(fname);
	if (filerr != FILERR_NONE)
		return;

	// parse the file and close it
	options_parse_ini_file(mame_options(), mame_core_file(file));
	mame_fclose(file);
}



//============================================================
//  mame_puts_info
//============================================================

static void mame_puts_info(const char *s)
{
	mame_printf_info("%s", s);
}

	
//============================================================
//  execute_simple_commands
//============================================================

static void execute_simple_commands(void)
{
	// help?
	if (options_get_bool(mame_options(),"help"))
	{
		display_help();
		exit(0);
	}

	// showusage?
	if (options_get_bool(mame_options(),"showusage"))
	{
		printf("Usage: mame [game] [options]\n\nOptions:\n");
			options_output_help(mame_options(), mame_puts_info);
		exit(0);
	}

	// validate?
	if (options_get_bool(mame_options(),"validate"))
	{
		extern int mame_validitychecks(int game);
		exit(mame_validitychecks(-1));
	}
}



//============================================================
//  execute_commands
//============================================================

static void execute_commands(const char *argv0)
{
	// frontend options
	static const struct
	{
		const char *option;
		int (*function)(FILE *output);
	} frontend_options[] =
	{
		{ "listxml",		frontend_listxml },
		{ "listfull",		frontend_listfull },
		{ "listsource",		frontend_listsource },
		{ "listclones",		frontend_listclones },
		{ "listcrc",		frontend_listcrc },
#ifdef MESS
		{ "listdevices",	frontend_listdevices },
#endif
		{ "listroms",		frontend_listroms },
		{ "listsamples",	frontend_listsamples },
		{ "verifyroms",		frontend_verifyroms },
		{ "verifysamples",	frontend_verifysamples },
		{ "romident",		frontend_romident },
		{ "isknown",		frontend_isknown  }
	};
	int i;

	// createconfig?
	if (options_get_bool(mame_options(),"createconfig"))
	{
		char basename[128];
		mame_file *file;
		file_error filerr;

		// make the base name
		extract_base_name(argv0, basename, ARRAY_LENGTH(basename) - 5);
		strcat(basename, ".ini");
		filerr = mame_fopen(NULL, basename, OPEN_FLAG_WRITE | OPEN_FLAG_CREATE | OPEN_FLAG_CREATE_PATHS, &file);

		// error if unable to create the file
		if (file == NULL)
		{
			fprintf(stderr, "Unable to create file %s\n", basename);
			exit(1);
		}

		// output the configuration and exit cleanly
		options_output_ini_file(mame_options(), mame_core_file(file));
		mame_fclose(file);
		exit(0);
	}

	// showconfig?
	if (options_get_bool(mame_options(),"showconfig"))
	{
		options_output_ini_stdfile(mame_options(), stdout);
		exit(0);
	}

	// frontend options?
	for (i = 0; i < ARRAY_LENGTH(frontend_options); i++)
		if (options_get_bool(mame_options(),frontend_options[i].option))
		{
			int result;

			init_resource_tracking();
			cpuintrf_init(NULL);
			sndintrf_init(NULL);
			result = (*frontend_options[i].function)(stdout);
			exit_resource_tracking();
			exit(result);
		}
}



//============================================================
//  display_help
//============================================================

static void display_help(void)
{
#ifndef MESS
	printf("M.A.M.E. v%s - Multiple Arcade Machine Emulator\n"
		   "Copyright (C) 1996-2007 by Nicola Salmoria and the MAME Team\n\n",build_version);
	printf("%s\n", mame_disclaimer);
	printf("Usage:  MAME gamename [options]\n\n"
		   "        MAME -showusage    for a brief list of options\n"
		   "        MAME -showconfig   for a list of configuration options\n"
		   "        MAME -createconfig to create a mame.ini\n\n"
		   "For usage instructions, please consult the file windows.txt\n");
#else
	showmessinfo();
#endif
}

//============================================================
//  extract_base_name
//============================================================

static char *extract_base_name(const char *name, char *dest, int destsize)
{
	const char *start;
	int i;

	// extract the base of the name
	start = name + strlen(name);
	while (start > name && !is_directory_separator(start[-1]))
		start--;

	// copy in the base name
	for (i = 0; i < destsize; i++)
	{
		if (start[i] == 0 || start[i] == '.')
			break;
		else
			dest[i] = start[i];
	}

	// NULL terminate
	if (i < destsize)
		dest[i] = 0;
	else
		dest[destsize - 1] = 0;

	return dest;
}



//============================================================
//  extract_path
//============================================================

static char *extract_path(const char *name, char *dest, int destsize)
{
	const char *start;

	// find the base of the name
	start = name + strlen(name);
	while (start > name && !is_directory_separator(start[-1]))
		start--;

	// handle the null path case
	if (start == name)
		*dest = 0;

	// else just copy the path part
	else
	{
		int bytes = start - 1 - name;
		bytes = MIN(destsize - 1, bytes);
		memcpy(dest, name, bytes);
		dest[bytes] = 0;
	}
	return dest;
}
