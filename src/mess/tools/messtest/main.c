/*********************************************************************

    main.c

    MESS testing main module

*********************************************************************/

#include <time.h>

#include "core.h"
#include "hashfile.h"
#include "emuopts.h"
#include "../imgtool/imgtool.h"

extern int mame_validitychecks(int game);

static int test_count, failure_count;

static const options_entry messtest_option_entries[] =
{
	{ "<UNADORNED0>",              NULL,        OPTION_REPEATS,    NULL },
	{ "" },
	{ "dumpscreenshots;ds",		"0",	OPTION_BOOLEAN,	"always dump screenshots" },
	{ "preservedir;pd",			"0",	OPTION_BOOLEAN,	"preserve current directory" },
	{ NULL }
};




/*************************************
 *
 *  Main and argument parsing/handling
 *
 *************************************/

static void handle_arg(core_options *copts, const char *arg)
{
	int this_test_count;
	int this_failure_count;
	struct messtest_options opts;

	/* setup options */
	memset(&opts, 0, sizeof(opts));
	opts.script_filename = arg;
	if (options_get_bool(copts, "preservedir"))
		opts.preserve_directory = 1;
	if (options_get_bool(copts, "dumpscreenshots"))
		opts.dump_screenshots = 1;

	if (messtest(&opts, &this_test_count, &this_failure_count))
		exit(-1);

	test_count += this_test_count;
	failure_count += this_failure_count;
}



static void messtest_fail(const char *message)
{
	fprintf(stderr, "%s", message);
	exit(1);
}

int CLIB_DECL main(int argc, char *argv[])
{
	int result = -1;
	clock_t begin_time;
	double elapsed_time;
	core_options *messtest_options = NULL;

	test_count = 0;
	failure_count = 0;
	messtest_options = NULL;

	/* register options */
	messtest_options = options_create(messtest_fail);
	options_add_entries(messtest_options, messtest_option_entries);
	options_set_option_callback(messtest_options, OPTION_GAMENAME, handle_arg);

	/* run MAME's validity checks; if these fail cop out now */
	/* NPW 16-Sep-2006 - commenting this out because this cannot be run outside of MAME */
	//if (mame_validitychecks(-1))
	//  goto done;
	/* run Imgtool's validity checks; if these fail cop out now */
	if (imgtool_validitychecks())
		goto done;

	begin_time = clock();

	/* parse the commandline */
	if (options_parse_command_line(messtest_options, argc, argv, OPTION_PRIORITY_CMDLINE,TRUE))
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
	if (messtest_options)
		options_free(messtest_options);
	return result;
}

