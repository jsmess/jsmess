//============================================================
//
//  config.c - Win32 configuration routines
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// standard C headers
#include <stdarg.h>
#include <ctype.h>

// MAME headers
#include "osdepend.h"
#include "driver.h"
#include "render.h"
#include "rendutil.h"
#include "debug/debugcpu.h"
#include "debug/debugcon.h"

// MAMEOS headers
#include "winmain.h"
#include "video.h"


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
#endif /* MESS */

void set_pathlist(int file_type, const char *new_rawpath);



//============================================================
//  GLOBALS
//============================================================

int win_erroroslog;

// fix me - need to have the core call osd_set_mastervolume with this value
// instead of relying on the name of an osd variable
extern int attenuation;
extern int audio_latency;
extern const char *wavwrite;



//============================================================
//  PROTOTYPES
//============================================================

static void extract_options(const game_driver *driver, machine_config *drv);
static void parse_ini_file(const char *name);
static void execute_simple_commands(void);
static void execute_commands(const char *argv0);
static void display_help(void);
static void extract_options(const game_driver *driver, machine_config *drv);
static void setup_playback(const char *filename, const game_driver *driver);
static void setup_record(const char *filename, const game_driver *driver);
static char *extract_base_name(const char *name, char *dest, int destsize);
static char *extract_path(const char *name, char *dest, int destsize);



//============================================================
//  OPTIONS
//============================================================

// struct definitions
static const options_entry windows_opts[] =
{
	// core commands
	{ NULL,                       NULL,       OPTION_HEADER,     "CORE COMMANDS" },
	{ "help;h;?",                 "0",        OPTION_COMMAND,    "show help message" },
	{ "validate;valid",           "0",        OPTION_COMMAND,    "perform driver validation on all game drivers" },

	// configuration commands
	{ NULL,                       NULL,       OPTION_HEADER,     "CONFIGURATION COMMANDS" },
	{ "createconfig;cc",          "0",        OPTION_COMMAND,    "create the default configuration file" },
	{ "showconfig;sc",            "0",        OPTION_COMMAND,    "display running parameters" },
	{ "showusage;su",             "0",        OPTION_COMMAND,    "show this help" },

	// frontend commands
	{ NULL,                       NULL,       OPTION_HEADER,     "FRONTEND COMMANDS" },
	{ "listxml;lx",               "0",        OPTION_COMMAND,    "all available info on driver in XML format" },
	{ "listfull;ll",              "0",        OPTION_COMMAND,    "short name, full name" },
	{ "listsource;ls",            "0",        OPTION_COMMAND,    "driver sourcefile" },
	{ "listclones;lc",            "0",        OPTION_COMMAND,    "show clones" },
	{ "listcrc",                  "0",        OPTION_COMMAND,    "CRC-32s" },
	{ "listroms",                 "0",        OPTION_COMMAND,    "list required roms for a driver" },
	{ "listsamples",              "0",        OPTION_COMMAND,    "list     OPTIONal samples for a driver" },
	{ "verifyroms",               "0",        OPTION_COMMAND,    "report romsets that have problems" },
	{ "verifysamples",            "0",        OPTION_COMMAND,    "report samplesets that have problems" },
	{ "romident",                 "0",        OPTION_COMMAND,    "compare files with known MAME roms" },
	{ "isknown",                  "0",        OPTION_COMMAND,    "compare files with known MAME roms (brief)" },
#ifdef MESS
	{ "listdevices",              "0",        OPTION_COMMAND,    "list available devices" },
#endif

	// config options
	{ NULL,                       NULL,       OPTION_HEADER,     "CONFIGURATION OPTIONS" },
	{ "readconfig;rc",            "1",        OPTION_BOOLEAN,    "enable loading of configuration files" },
	{ "skip_gameinfo",            "0",        OPTION_BOOLEAN,    "skip displaying the information screen at startup" },

	// misc options
	{ NULL,                       NULL,       OPTION_HEADER,     "MISC OPTIONS" },
	{ "bios",                     "default",  0,                 "select the system BIOS to use" },
	{ "cheat;c",                  "0",        OPTION_BOOLEAN,    "enable cheat subsystem" },

	// state/playback options
	{ NULL,                       NULL,       OPTION_HEADER,     "STATE/PLAYBACK OPTIONS" },
	{ "state",                    NULL,       0,                 "saved state to load" },
	{ "autosave",                 "0",        OPTION_BOOLEAN,    "enable automatic restore at startup, and automatic save at exit time" },
	{ "playback;pb",              NULL,       0,                 "playback an input file" },
	{ "record;rec",               NULL,       0,                 "record an input file" },
	{ "mngwrite",                 NULL,       0,                 "optional filename to write a MNG movie of the current session" },
	{ "wavwrite",                 NULL,       0,                 "optional filename to write a WAV file of the current session" },

	// debugging options
	{ NULL,                       NULL,       OPTION_HEADER,     "DEBUGGING OPTIONS" },
	{ "log",                      "0",        OPTION_BOOLEAN,    "generate an error.log file" },
	{ "oslog",                    "0",        OPTION_BOOLEAN,    "output error.log data to the system debugger" },
	{ "verbose;v",                "0",        OPTION_BOOLEAN,    "display additional diagnostic information" },
#ifdef MAME_DEBUG
	{ "debug;d",                  "1",        OPTION_BOOLEAN,    "enable/disable debugger" },
	{ "debugscript",              NULL,       0,                 "script for debugger" },
#else
	{ "debug;d",                  "1",        OPTION_DEPRECATED, "(debugger-only command)" },
	{ "debugscript",              NULL,       OPTION_DEPRECATED, "(debugger-only command)" },
#endif

	// performance options
	{ NULL,                       NULL,       OPTION_HEADER,     "PERFORMANCE OPTIONS" },
	{ "autoframeskip;afs",        "0",        OPTION_BOOLEAN,    "enable automatic frameskip selection" },
	{ "frameskip;fs",             "0",        0,                 "set frameskip to fixed value, 0-12 (autoframeskip must be disabled)" },
	{ "frames_to_run;ftr",        "0",        0,                 "number of frames to run before automatically exiting" },
	{ "throttle",                 "1",        OPTION_BOOLEAN,    "enable throttling to keep game running in sync with real time" },
	{ "sleep",                    "1",        OPTION_BOOLEAN,    "enable sleeping, which gives time back to other applications when idle" },
	{ "rdtsc",                    "0",        OPTION_BOOLEAN,    "use the RDTSC instruction for timing; faster but may result in uneven performance" },
	{ "priority",                 "0",        0,                 "thread priority for the main game thread; range from -15 to 1" },
	{ "multithreading;mt",        "0",        OPTION_BOOLEAN,    "enable multithreading; this enables rendering and blitting on a separate thread" },

	// video options
	{ NULL,                       NULL,       OPTION_HEADER,     "VIDEO OPTIONS" },
	{ "video",                    "d3d",      0,                 "video output method: gdi, ddraw, or d3d" },
	{ "numscreens",               "1",        0,                 "number of screens to create; usually, you want just one" },
	{ "window;w",                 "0",        OPTION_BOOLEAN,    "enable window mode; otherwise, full screen mode is assumed" },
	{ "maximize;max",             "1",        OPTION_BOOLEAN,    "default to maximized windows; otherwise, windows will be minimized" },
	{ "keepaspect;ka",            "1",        OPTION_BOOLEAN,    "constrain to the proper aspect ratio" },
	{ "prescale",                 "1",        0,                 "scale screen rendering by this amount in software" },
	{ "effect",                   "none",     0,                 "name of a PNG file to use for visual effects, or 'none'" },
	{ "pause_brightness",         "0.65",     0,                 "amount to scale the screen brightness when paused" },
	{ "waitvsync",                "0",        OPTION_BOOLEAN,    "enable waiting for the start of VBLANK before flipping screens; reduces tearing effects" },
	{ "syncrefresh",              "0",        OPTION_BOOLEAN,    "enable using the start of VBLANK for throttling instead of the game time" },

	// video rotation options
	{ NULL,                       NULL,       OPTION_HEADER,     "VIDEO ROTATION OPTIONS" },
	{ "rotate",                   "1",        OPTION_BOOLEAN,    "rotate the game screen according to the game's orientation needs it" },
	{ "ror",                      "0",        OPTION_BOOLEAN,    "rotate screen clockwise 90 degrees" },
	{ "rol",                      "0",        OPTION_BOOLEAN,    "rotate screen counterclockwise 90 degrees" },
	{ "autoror",                  "0",        OPTION_BOOLEAN,    "automatically rotate screen clockwise 90 degrees if vertical" },
	{ "autorol",                  "0",        OPTION_BOOLEAN,    "automatically rotate screen counterclockwise 90 degrees if vertical" },
	{ "flipx",                    "0",        OPTION_BOOLEAN,    "flip screen left-right" },
	{ "flipy",                    "0",        OPTION_BOOLEAN,    "flip screen upside-down" },

	// DirectDraw-specific options
	{ NULL,                       NULL,       OPTION_HEADER,     "DIRECTDRAW-SPECIFIC OPTIONS" },
	{ "hwstretch;hws",            "1",        OPTION_BOOLEAN,    "enable hardware stretching" },

	// Direct3D-specific options
	{ NULL,                       NULL,       OPTION_HEADER,     "DIRECT3D-SPECIFIC OPTIONS" },
	{ "d3dversion",               "9",        0,                 "specify the preferred Direct3D version (8 or 9)" },
	{ "filter;d3dfilter;flt",     "1",        OPTION_BOOLEAN,    "enable bilinear filtering on screen output" },

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
	{ "triplebuffer;tb",          "0",        OPTION_BOOLEAN,    "enable triple buffering" },
	{ "switchres",                "0",        OPTION_BOOLEAN,    "enable resolution switching" },
	{ "full_screen_brightness;fsb","1.0",     0,                 "brightness value in full screen mode" },
	{ "full_screen_contrast;fsc", "1.0",      0,                 "contrast value in full screen mode" },
	{ "full_screen_gamma;fsg",    "1.0",      0,                 "gamma value in full screen mode" },

	// game screen options
	{ NULL,                       NULL,       OPTION_HEADER,     "GAME SCREEN OPTIONS" },
	{ "brightness",               "1.0",      0,                 "default game screen brightness correction" },
	{ "contrast",                 "1.0",      0,                 "default game screen contrast correction" },
	{ "gamma",                    "1.0",      0,                 "default game screen gamma correction" },

	// vector rendering options
	{ NULL,                       NULL,       OPTION_HEADER,     "VECTOR RENDERING OPTIONS" },
	{ "antialias;aa",             "1",        OPTION_BOOLEAN,    "use antialiasing when drawing vectors" },
	{ "beam",                     "1.0",      0,                 "set vector beam width" },
	{ "flicker",                  "0",        0,                 "set vector flicker effect" },

	// artwork options
	{ NULL,                       NULL,       OPTION_HEADER,     "ARTWORK OPTIONS" },
	{ "artwork_crop;artcrop",     "0",        OPTION_BOOLEAN,    "crop artwork to game screen size" },
	{ "use_backdrops;backdrop",   "1",        OPTION_BOOLEAN,    "enable backdrops if artwork is enabled and available" },
	{ "use_overlays;overlay",     "1",        OPTION_BOOLEAN,    "enable overlays if artwork is enabled and available" },
	{ "use_bezels;bezel",         "1",        OPTION_BOOLEAN,    "enable bezels if artwork is enabled and available" },

	// sound options
	{ NULL,                       NULL,       OPTION_HEADER,     "SOUND OPTIONS" },
	{ "sound",                    "1",        OPTION_BOOLEAN,    "enable sound output" },
	{ "samplerate;sr",            "48000",    0,                 "set sound output sample rate" },
	{ "samples",                  "1",        OPTION_BOOLEAN,    "enable the use of external samples if available" },
	{ "volume",                   "0",        0,                 "sound volume in decibels (-32 min, 0 max)" },
	{ "audio_latency",            "1",        0,                 "set audio latency (increase to reduce glitches)" },

	// input options
	{ NULL,                       NULL,       OPTION_HEADER,     "INPUT DEVICE OPTIONS" },
	{ "ctrlr",                    NULL,       0,                 "preconfigure for specified controller" },
	{ "mouse",                    "0",        OPTION_BOOLEAN,    "enable mouse input" },
	{ "joystick;joy",             "0",        OPTION_BOOLEAN,    "enable joystick input" },
	{ "lightgun;gun",             "0",        OPTION_BOOLEAN,    "enable lightgun input" },
	{ "dual_lightgun;dual",       "0",        OPTION_BOOLEAN,    "enable dual lightgun input" },
	{ "offscreen_reload;reload",  "0",        OPTION_BOOLEAN,    "offscreen shots reload" },
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
	options_init(windows_opts);
#ifdef MESS
	win_mess_options_init();
#endif // MESS

	// parse the command line first; if we fail here, we're screwed
	if (options_parse_command_line(argc, argv))
		exit(1);

	// parse the simple commmands before we go any further
	execute_simple_commands();

	// find out what game we might be referring to
	gamename = options_get_string(OPTION_GAMENAME);
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
	options_parse_command_line(argc, argv);
	gamename = options_get_string(OPTION_GAMENAME);

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

	// extract the directory name
	extract_path(gamename, buffer, ARRAY_LENGTH(buffer));
	if (buffer[0] != 0)
	{
		// okay, we got one; prepend it to the rompath
		const char *rompath = options_get_string("rompath");
		if (rompath == NULL)
			options_set_string("rompath", buffer);
		else
		{
			char *newpath = malloc_or_die(strlen(rompath) + strlen(buffer) + 2);
			sprintf(newpath, "%s;%s", buffer, rompath);
			options_set_string("rompath", newpath);
			free(newpath);
		}
	}

	// extract options information
	extract_options(drivers[drvnum], &drv);
	return drvnum;
}



//============================================================
//  cli_frontend_exit
//============================================================

void cli_frontend_exit(void)
{
	// close open files
	if (options.logfile != NULL)
		mame_fclose(options.logfile);
	options.logfile = NULL;

	if (options.playback != NULL)
		mame_fclose(options.playback);
	options.playback = NULL;

	if (options.record != NULL)
		mame_fclose(options.record);
	options.record = NULL;

	if (options.language_file != NULL)
		mame_fclose(options.language_file);
	options.language_file = NULL;

	// free the options that we added previously
	options_free_entries();
}



//============================================================
//  parse_ini_file
//============================================================

static void parse_ini_file(const char *name)
{
	mame_file_error filerr;
	mame_file *file;
	char *fname;

	// don't parse if it has been disabled
	if (!options_get_bool("readconfig"))
		return;

	// open the file; if we fail, that's ok
	fname = assemble_2_strings(name, ".ini");
	filerr = mame_fopen(SEARCHPATH_INI, fname, OPEN_FLAG_READ, &file);
	free(fname);
	if (filerr != FILERR_NONE)
		return;

	// parse the file and close it
	options_parse_ini_file(file);
	mame_fclose(file);
}



//============================================================
//  execute_simple_commands
//============================================================

static void execute_simple_commands(void)
{
	// help?
	if (options_get_bool("help"))
	{
		display_help();
		exit(0);
	}

	// showusage?
	if (options_get_bool("showusage"))
	{
		mame_printf_info("Usage: mame [game] [options]\n\nOptions:\n");
		options_output_help();
		exit(0);
	}

	// validate?
	if (options_get_bool("validate"))
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
	if (options_get_bool("createconfig"))
	{
		char basename[128];
		mame_file *file;
		mame_file_error filerr;

		// make the base name
		extract_base_name(argv0, basename, ARRAY_LENGTH(basename) - 5);
		strcat(basename, ".ini");
		filerr = mame_fopen(NULL, basename, OPEN_FLAG_WRITE | OPEN_FLAG_CREATE | OPEN_FLAG_CREATE_PATHS, &file);

		// error if unable to create the file
		if (filerr)
		{
			fprintf(stderr, "Unable to create file %s\n", basename);
			exit(1);
		}

		// output the configuration and exit cleanly
		options_output_ini_mame_file(file);
		mame_fclose(file);
		exit(0);
	}

	// showconfig?
	if (options_get_bool("showconfig"))
	{
		options_output_ini_file(stdout);
		exit(0);
	}

	// frontend options?
	for (i = 0; i < ARRAY_LENGTH(frontend_options); i++)
		if (options_get_bool(frontend_options[i].option))
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
	mame_printf_info("M.A.M.E. v%s - Multiple Arcade Machine Emulator\n"
		   "Copyright (C) 1997-2007 by Nicola Salmoria and the MAME Team\n\n",build_version);
	mame_printf_info("%s\n", mame_disclaimer);
	mame_printf_info("Usage:  MAME gamename [options]\n\n"
		   "        MAME -showusage    for a brief list of options\n"
		   "        MAME -showconfig   for a list of configuration options\n"
		   "        MAME -createconfig to create a mame.ini\n\n"
		   "For usage instructions, please consult the file windows.txt\n");
#else
	showmessinfo();
#endif
}



//============================================================
//  extract_options
//============================================================

static void extract_options(const game_driver *driver, machine_config *drv)
{
	const char *stemp;

	// clear all core options
	memset(&options, 0, sizeof(options));

	// video options
	video_orientation = ROT0;

	// override if no rotation requested
	if (!options_get_bool("rotate"))
		video_orientation = orientation_reverse(driver->flags & ORIENTATION_MASK);

	// rotate right
	if (options_get_bool("ror") || (options_get_bool("autoror") && (driver->flags & ORIENTATION_SWAP_XY)))
		video_orientation = orientation_add(ROT90, video_orientation);

	// rotate left
	if (options_get_bool("rol") || (options_get_bool("autorol") && (driver->flags & ORIENTATION_SWAP_XY)))
		video_orientation = orientation_add(ROT270, video_orientation);

	// flip X/Y
	if (options_get_bool("flipx"))
		video_orientation ^= ORIENTATION_FLIP_X;
	if (options_get_bool("flipy"))
		video_orientation ^= ORIENTATION_FLIP_Y;

	// brightness
	options.brightness = options_get_float_range("brightness", 0.1f, 2.0f);
	options.contrast = options_get_float_range("contrast", 0.1f, 2.0f);
	options.pause_bright = options_get_float_range("pause_brightness", 0.0f, 1.0f);
	options.gamma = options_get_float_range("gamma", 0.1f, 3.0f);

	// vector options
	options.antialias = options_get_bool("antialias");
	options.beam = (int)(options_get_float("beam") * 65536.0f);
	options.vector_flicker = options_get_float("flicker");

	// sound options
	options.samplerate = options_get_bool("sound") ? options_get_int_range("samplerate", 1000, 1000000) : 0;
	options.use_samples = options_get_bool("samples");
	attenuation = options_get_int("volume");
	audio_latency = options_get_int("audio_latency");
	wavwrite = options_get_string("wavwrite");

	// misc options
	options.bios = (char *)options_get_string("bios");
	options.cheat = options_get_bool("cheat");
	options.skip_gameinfo = options_get_bool("skip_gameinfo");

#ifdef MESS
	win_mess_extract_options();
#endif /* MESS */

	// save states and input recording
	stemp = options_get_string("playback");
	if (stemp != NULL)
		setup_playback(stemp, driver);
	stemp = options_get_string("record");
	if (stemp != NULL)
		setup_record(stemp, driver);
	options.savegame = options_get_string("state");
	options.auto_save = options_get_bool("autosave");

	// debugging options
	if (options_get_bool("log"))
	{
		mame_file_error filerr = mame_fopen(SEARCHPATH_DEBUGLOG, "error.log", OPEN_FLAG_WRITE | OPEN_FLAG_CREATE | OPEN_FLAG_CREATE_PATHS, &options.logfile);
		assert_always(filerr == FILERR_NONE, "unable to open log file");
	}
	win_erroroslog = options_get_bool("oslog");
{
	extern int verbose;
	verbose = options_get_bool("verbose");
}
#ifdef MAME_DEBUG
	options.mame_debug = options_get_bool("debug");
	stemp = options_get_string("debugscript");
	if (stemp != NULL)
		debug_source_script(stemp);
#endif

	// thread priority
	if (!options.mame_debug)
		SetThreadPriority(GetCurrentThread(), options_get_int_range("priority", -15, 1));
}



//============================================================
//  setup_playback
//============================================================

static void setup_playback(const char *filename, const game_driver *driver)
{
	mame_file_error filerr;
	inp_header inp_header;

	// open the playback file
	filerr = mame_fopen(SEARCHPATH_INPUTLOG, filename, OPEN_FLAG_READ, &options.playback);
	assert_always(filerr == FILERR_NONE, "Failed to open file for playback");

	// read playback header
	mame_fread(options.playback, &inp_header, sizeof(inp_header));

	// if the first byte is not alphanumeric, it's an old INP file with no header
	if (!isalnum(inp_header.name[0]))
		mame_fseek(options.playback, 0, SEEK_SET);

	// else verify the header against the current game
	else if (strcmp(driver->name, inp_header.name) != 0)
		fatalerror("Input file is for " GAMENOUN " '%s', not for current " GAMENOUN " '%s'\n", inp_header.name, driver->name);

	// otherwise, print a message indicating what's happening
	else
		mame_printf_info("Playing back previously recorded " GAMENOUN " %s\n", driver->name);
}



//============================================================
//  setup_record
//============================================================

static void setup_record(const char *filename, const game_driver *driver)
{
	mame_file_error filerr;
	inp_header inp_header;

	// open the record file
	filerr = mame_fopen(SEARCHPATH_INPUTLOG, filename, OPEN_FLAG_WRITE | OPEN_FLAG_CREATE | OPEN_FLAG_CREATE_PATHS, &options.record);
	assert_always(filerr == FILERR_NONE, "Failed to open file for recording");

	// create a header
	memset(&inp_header, '\0', sizeof(inp_header));
	strcpy(inp_header.name, driver->name);
	mame_fwrite(options.record, &inp_header, sizeof(inp_header));
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
