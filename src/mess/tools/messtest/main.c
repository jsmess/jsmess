/*********************************************************************

	main.c

	MESS testing main module

*********************************************************************/

#include <time.h>

#include "core.h"
#include "hashfile.h"
#include "options.h"
#include "../imgtool/imgtool.h"

#ifdef WIN32
#include <windows.h>
#include "glob.h"
#endif /* WIN32 */

extern int mame_validitychecks(int game);

static int test_count, failure_count;

static const options_entry messtest_opts[] =
{
	{ "<UNADORNED0>",              NULL,        OPTION_REPEATS,    NULL },
	{ "" },
	{ "dumpscreenshots;ds",		"0",	OPTION_BOOLEAN,	"always dump screenshots" },
	{ "preservedir;pd",			"0",	OPTION_BOOLEAN,	"preserve current directory" },
	{ "rdtsc",					"0",	OPTION_BOOLEAN, "use the RDTSC instruction for timing; faster but may result in uneven performance" },
	{ "priority",				"0",	0,				"thread priority for the main game thread; range from -15 to 1" },

	// file and directory options
	{ NULL,                          NULL,        OPTION_HEADER,     "CORE SEARCH PATH OPTIONS" },
	{ "rompath;rp;biospath;bp",      "roms",      0,                 "path to ROMsets and hard disk images" },
#ifdef MESS
	{ "hashpath;hash_directory;hash","hash",      0,                 "path to hash files" },
#endif /* MESS */
	{ "samplepath;sp",               "samples",   0,                 "path to samplesets" },
	{ "artpath;artwork_directory",   "artwork",   0,                 "path to artwork files" },
	{ "ctrlrpath;ctrlr_directory",   "ctrlr",     0,                 "path to controller definitions" },
	{ "inipath",                     ".;ini",     0,                 "path to ini files" },
	{ "fontpath",                    ".",         0,                 "path to font files" },

	{ NULL,                          NULL,        OPTION_HEADER,     "CORE OUTPUT DIRECTORY OPTIONS" },
	{ "cfg_directory",               "cfg",       0,                 "directory to save configurations" },
	{ "nvram_directory",             "nvram",     0,                 "directory to save nvram contents" },
	{ "memcard_directory",           "memcard",   0,                 "directory to save memory card contents" },
	{ "input_directory",             "inp",       0,                 "directory to save input device logs" },
	{ "state_directory",             "sta",       0,                 "directory to save states" },
	{ "snapshot_directory",          "snap",      0,                 "directory to save screenshots" },
	{ "diff_directory",              "diff",      0,                 "directory to save hard drive image difference files" },
	{ "comment_directory",           "comments",  0,                 "directory to save debugger comments" },

	{ NULL,                          NULL,        OPTION_HEADER,     "CORE FILENAME OPTIONS" },
	{ "cheat_file",                  "cheat.dat", 0,                 "cheat filename" },

	{ NULL,                          NULL,        OPTION_HEADER,     "CORE PERFORMANCE OPTIONS" },
	{ "autoframeskip;afs",           "0",         OPTION_BOOLEAN,    "enable automatic frameskip selection" },
	{ "frameskip;fs",                "0",         0,                 "set frameskip to fixed value, 0-12 (autoframeskip must be disabled)" },
	{ "seconds_to_run;str",          "0",         0,                 "number of emulated seconds to run before automatically exiting" },
	{ "throttle",                    "0",         OPTION_BOOLEAN,    "enable throttling to keep game running in sync with real time" },
	{ "sleep",                       "1",         OPTION_BOOLEAN,    "enable sleeping, which gives time back to other applications when idle" },

	{ NULL }
};



/*************************************
 *
 *	Main and argument parsing/handling
 *
 *************************************/

static void handle_arg(const char *arg)
{
	int this_test_count;
	int this_failure_count;
	struct messtest_options opts;

	/* setup options */
	memset(&opts, 0, sizeof(opts));
	opts.script_filename = arg;
	if (options_get_bool("preservedir"))
		opts.preserve_directory = 1;
	if (options_get_bool("dumpscreenshots"))
		opts.dump_screenshots = 1;

	if (messtest(&opts, &this_test_count, &this_failure_count))
		exit(-1);

	test_count += this_test_count;
	failure_count += this_failure_count;
}



#ifdef WIN32
static void win_expand_wildcards(int *argc, char **argv[])
{
	int i;
	glob_t g;

	memset(&g, 0, sizeof(g));

	for (i = 0; i < *argc; i++)
		glob((*argv)[i], (g.gl_pathc > 0) ? GLOB_APPEND|GLOB_NOCHECK : GLOB_NOCHECK, NULL, &g);

	*argc = g.gl_pathc;
	*argv = g.gl_pathv;
}
#endif /* WIN32 */



#ifdef WIN32
int CLIB_DECL utf8_main(int argc, char *argv[])
#else
int CLIB_DECL main(int argc, char *argv[])
#endif
{
	int result = -1;
	clock_t begin_time;
	double elapsed_time;

#ifdef WIN32
	/* expand wildcards so '*' can be used; this is not UNIX */
	win_expand_wildcards(&argc, &argv);
#endif /* WIN32 */

	test_count = 0;
	failure_count = 0;

	/* since the cpuintrf and sndintrf structures are filled dynamically now, we
	 * have to init first */
	cpuintrf_init(NULL);
	sndintrf_init(NULL);
	
	/* register options */
	options_init(NULL);
	options_free_entries();
	options_add_entries(messtest_opts);
	options_set_option_callback(OPTION_UNADORNED(0), handle_arg);

	/* run MAME's validity checks; if these fail cop out now */
	/* NPW 16-Sep-2006 - commenting this out because this cannot be run outside of MAME */
	//if (mame_validitychecks(-1))
	//	goto done;
	/* run Imgtool's validity checks; if these fail cop out now */
	if (imgtool_validitychecks())
		goto done;

	begin_time = clock();

	/* parse the commandline */
	if (options_parse_command_line(argc, argv))
	{
		fprintf(stderr, "Error while parsing cmdline\n");
		goto done;
	}

	if (test_count > 0)
	{
		elapsed_time = ((double) (clock() - begin_time)) / CLOCKS_PER_SEC;

		fprintf(stderr, "Tests complete; %i test(s), %i failure(s), elapsed time %.2f\n",
			test_count, failure_count, elapsed_time);
	}
	else
	{
		fprintf(stderr, "Usage: %s [test1] [test2]...\n", argv[0]);
	}
	result = failure_count;

done:
	return result;
}

