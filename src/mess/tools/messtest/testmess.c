/*********************************************************************

	testmess.c

	MESS testing code

*********************************************************************/

#include <time.h>
#include <ctype.h>
#include <setjmp.h>

#include "testmess.h"
#include "inputx.h"
#include "pile.h"
#include "pool.h"
#include "sound/wavwrite.h"
#include "video/generic.h"
#include "render.h"
#include "messopts.h"

#include "debug/debugcpu.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define MESSTEST_ALWAYS_DUMP_SCREENSHOT		1



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef enum
{
	STATE_READY,
	STATE_INCOMMAND,
	STATE_ABORTED,
	STATE_DONE
} messtest_running_state_t;



typedef enum
{
	MESSTEST_COMMAND_END,
	MESSTEST_COMMAND_WAIT,
	MESSTEST_COMMAND_INPUT,
	MESSTEST_COMMAND_RAWINPUT,
	MESSTEST_COMMAND_SWITCH,
	MESSTEST_COMMAND_SCREENSHOT,
	MESSTEST_COMMAND_CHECKBLANK,
	MESSTEST_COMMAND_IMAGE_CREATE,
	MESSTEST_COMMAND_IMAGE_LOAD,
	MESSTEST_COMMAND_IMAGE_PRECREATE,
	MESSTEST_COMMAND_IMAGE_PRELOAD,
	MESSTEST_COMMAND_VERIFY_MEMORY,
	MESSTEST_COMMAND_VERIFY_IMAGE,
	MESSTEST_COMMAND_TRACE,
	MESSTEST_COMMAND_SOFT_RESET,
	MESSTEST_COMMAND_HARD_RESET
} messtest_command_type_t;



typedef struct _messtest_device_identity messtest_device_identity;
struct _messtest_device_identity
{
	const char *tag;
	iodevice_t type;
	int slot;
};



typedef struct _messtest_command messtest_command;
struct _messtest_command
{
	messtest_command_type_t command_type;
	union
	{
		attotime wait_time;
		struct
		{
			const char *input_chars;
			attotime rate;
		} input_args;
		struct
		{
			const char *mem_region;
			offs_t start;
			offs_t end;
			const void *verify_data;
			size_t verify_data_size;
			messtest_device_identity device_ident;
		} verify_args;
		struct
		{
			const char *filename;
			const char *format;
			messtest_device_identity device_ident;
		} image_args;
		struct
		{
			const char *name;
			const char *value;
		} switch_args;
	} u;
};



typedef struct _messtest_testcase messtest_testcase;
struct _messtest_testcase
{
	const char *name;
	const char *bios;
	const char *driver;
	attotime time_limit;	/* 0.0 = default */
	messtest_command *commands;

	/* options */
	UINT32 ram;
	unsigned int wavwrite : 1;
	unsigned int enabled : 1;
};



typedef struct _messtest_specific_state messtest_specific_state;
struct messtest_specific_state
{
	messtest_testcase testcase;
	int command_count;
	messtest_command current_command;
};



typedef enum
{
	MESSTEST_RESULT_SUCCESS,
	MESSTEST_RESULT_STARTFAILURE,
	MESSTEST_RESULT_RUNTIMEFAILURE
} messtest_result_t;



typedef struct _messtest_results messtest_results;
struct _messtest_results
{
	messtest_result_t rc;
	UINT64 runtime_hash;	/* A value that is a hash taken from certain runtime parameters; used to detect different execution paths */
};



/***************************************************************************
    LOCAL VARIABLES
***************************************************************************/

static messtest_running_state_t state;
static int had_failure;
static attotime wait_target;
static attotime final_time;
static const messtest_command *current_command;
static int test_flags;
static int screenshot_num;
static UINT64 runtime_hash;
static void *wavptr;
static render_target *target;

/* command list */
static mess_pile command_pile;
static object_pool *command_pool;
static int command_count;
static messtest_command new_command;

static messtest_testcase current_testcase;

static const options_entry win_mess_opts[] =
{
	{ NULL,							NULL,   OPTION_HEADER,		"WINDOWS MESS SPECIFIC OPTIONS" },
	{ "newui;nu",                   "1",    OPTION_BOOLEAN,		"use the new MESS UI" },
	{ "natural;nat",				"0",	OPTION_BOOLEAN,		"specifies whether to use a natural keyboard or not" },
	{ NULL }
};



/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

static astring *assemble_software_path(astring *str, const game_driver *gamedrv, const char *filename)
{
	if (osd_is_absolute_path(filename))
		astring_cpyc(str, filename);
	else
		astring_assemble_5(str, "software", PATH_SEPARATOR, gamedrv->name, PATH_SEPARATOR, filename);
	return str;
}



static void dump_screenshot(running_machine *machine, int write_file)
{
	file_error filerr;
	mame_file *fp;
	char buf[128];
	int is_blank = 0;

	if (write_file)
	{
		/* dump a screenshot */
		snprintf(buf, sizeof(buf) / sizeof(buf[0]),
			(screenshot_num >= 0) ? "_%s_%d.png" : "_%s.png",
			current_testcase.name, screenshot_num);
		filerr = mame_fopen(SEARCHPATH_SCREENSHOT, buf, OPEN_FLAG_WRITE | OPEN_FLAG_CREATE, &fp);
		if (filerr == FILERR_NONE)
		{
			/* choose a screen */
			const device_config *screen = video_screen_first(machine->config);
			while((screen != NULL) && !render_is_live_screen(screen))
			{
				screen = video_screen_next(screen);
			}

			/* did we find a live screen? */
			if (screen != NULL)
			{
				video_screen_save_snapshot(screen->machine, screen, fp);
				report_message(MSG_INFO, "Saved screenshot as %s", buf);
			}
			else
			{
				report_message(MSG_FAILURE, "Could not save screenshot; no live screen");
			}
			mame_fclose(fp);
		}
		else
		{
			/* report the error */
			report_message(MSG_FAILURE, "Could not save screenshot; error #%d", filerr);
		}

		if (screenshot_num >= 0)
			screenshot_num++;
	}

#if 0
	/* check to see if bitmap is blank */
	bitmap = scrbitmap[0];
	is_blank = 1;
	color = bitmap->read(bitmap, 0, 0);
	for (y = 0; is_blank && (y < bitmap->height); y++)
	{
		for (x = 0; is_blank && (x < bitmap->width); x++)
		{
			if (bitmap->read(bitmap, x, y) != color)
				is_blank = 0;
		}
	}
#endif
	if (is_blank)
	{
		had_failure = TRUE;
		report_message(MSG_FAILURE, "Screenshot is blank");
	}
}



static void messtest_output_error(void *param, const char *format, va_list argptr)
{
	char buffer[1024];
	char *s;
	int pos, nextpos;

	vsnprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), format, argptr);

	pos = 0;
	while(buffer[pos] != '\0')
	{
		s = strchr(&buffer[pos], '\n');
		if (s)
		{
			*s = '\0';
			nextpos = s + 1 - buffer;
		}
		else
		{
			nextpos = pos + strlen(&buffer[pos]);
		}
		report_message(MSG_FAILURE, "%s", &buffer[pos]);
		pos = nextpos;
	}
}



static messtest_result_t run_test(int flags, messtest_results *results)
{
	const game_driver *driver;
	messtest_result_t rc;
	clock_t begin_time;
	double real_run_time;
	astring *fullpath = NULL;
	const char *device_opt;
	const char *fake_argv[2];
	core_options *opts;

	/* lookup driver */
	driver = driver_get_name(current_testcase.driver);

	/* cannot find driver? */
	if (driver == NULL)
	{
		report_message(MSG_FAILURE, "Cannot find driver '%s'", current_testcase.driver);
		return MESSTEST_RESULT_STARTFAILURE;
	}

	/* prepare testing state */
	current_command = current_testcase.commands;
	state = STATE_READY;
	test_flags = flags;
	screenshot_num = 0;
	runtime_hash = 0;
	had_failure = FALSE;
	videoram = NULL;
	videoram_size = 0;

	/* set up options */
	opts = mame_options_init(win_mess_opts);
	options_set_string(opts, OPTION_GAMENAME, driver->name, OPTION_PRIORITY_CMDLINE);
	if( current_testcase.bios )
		options_set_string(opts, OPTION_BIOS, current_testcase.bios, OPTION_PRIORITY_CMDLINE);
	options_set_bool(opts, OPTION_SKIP_GAMEINFO, TRUE, OPTION_PRIORITY_CMDLINE);
	options_set_bool(opts, OPTION_THROTTLE, FALSE, OPTION_PRIORITY_CMDLINE);
	options_set_bool(opts, OPTION_SKIP_WARNINGS, TRUE, OPTION_PRIORITY_CMDLINE);
	if (current_testcase.ram != 0)
	{
		options_set_int(opts, OPTION_RAMSIZE, current_testcase.ram, OPTION_PRIORITY_CMDLINE);
	}

	/* ugh... hideous ugly fake arguments */
	fake_argv[0] = "MESSTEST";
	fake_argv[1] = driver->name;
	options_parse_command_line(opts, ARRAY_LENGTH(fake_argv), (char **) fake_argv, OPTION_PRIORITY_CMDLINE);

	/* preload any needed images */
	while(current_command->command_type == MESSTEST_COMMAND_IMAGE_PRELOAD)
	{
		/* get the path */
		fullpath = assemble_software_path(astring_alloc(), driver, current_command->u.image_args.filename);

		/* get the option name */
		device_opt = device_typename(current_command->u.image_args.device_ident.type);

		/* set the option */
		options_set_string(opts, device_opt, astring_c(fullpath), OPTION_PRIORITY_CMDLINE);

		/* cleanup */
		astring_free(fullpath);
		fullpath = NULL;

		/* next command */
		current_command++;
	}

	/* perform the test */
	report_message(MSG_INFO, "Beginning test (driver '%s')", current_testcase.driver);
	begin_time = clock();
	mame_set_output_channel(OUTPUT_CHANNEL_ERROR, messtest_output_error, NULL, NULL, NULL);
	mame_set_output_channel(OUTPUT_CHANNEL_WARNING, mame_null_output_callback, NULL, NULL, NULL);
	mame_set_output_channel(OUTPUT_CHANNEL_INFO, mame_null_output_callback, NULL, NULL, NULL);
	mame_set_output_channel(OUTPUT_CHANNEL_DEBUG, mame_null_output_callback, NULL, NULL, NULL);
	mame_set_output_channel(OUTPUT_CHANNEL_LOG, mame_null_output_callback, NULL, NULL, NULL);
	mame_execute(opts);
	real_run_time = ((double) (clock() - begin_time)) / CLOCKS_PER_SEC;

	/* what happened? */
	switch(state)
	{
		case STATE_ABORTED:
			report_message(MSG_FAILURE, "Test aborted");
			rc = MESSTEST_RESULT_RUNTIMEFAILURE;
			break;

		case STATE_DONE:
			if (had_failure)
			{
				report_message(MSG_FAILURE, "Test failed (real time %.2f; emu time %.2f [%i%%])",
					real_run_time, attotime_to_double(final_time), (int) ((attotime_to_double(final_time) / real_run_time) * 100));
				rc = MESSTEST_RESULT_RUNTIMEFAILURE;
			}
			else
			{
				report_message(MSG_INFO, "Test succeeded (real time %.2f; emu time %.2f [%i%%])",
					real_run_time, attotime_to_double(final_time), (int) ((attotime_to_double(final_time) / real_run_time) * 100));
				rc = MESSTEST_RESULT_SUCCESS;
			}
			break;

		default:
			state = STATE_ABORTED;
			report_message(MSG_FAILURE, "Abnormal termination");
			rc = MESSTEST_RESULT_STARTFAILURE;
			break;
	}

	if (results)
	{
		results->rc = rc;
		results->runtime_hash = runtime_hash;
	}

	options_free(opts);
	return rc;
}



static void testmess_exit(running_machine *machine)
{
	if (target != NULL)
	{
		render_target_free(target);
		target = NULL;
	}
}



void osd_init(running_machine *machine)
{
	add_exit_callback(machine, testmess_exit);
	target = render_target_alloc(machine, NULL, 0);
	render_target_set_orientation(target, 0);
}



#if 0
int osd_start_audio_stream(int stereo)
{
	char buf[256];

	if (current_testcase.wavwrite)
	{
		snprintf(buf, sizeof(buf) / sizeof(buf[0]), "snap/_%s.wav", current_testcase.name);
		wavptr = wav_open(buf, machine->sample_rate, 2);
	}
	else
	{
		wavptr = NULL;
	}
	samples_this_frame = (int) ((double)machine->sample_rate / (double)machine->screen[0].refresh);
	return samples_this_frame;
}



void osd_stop_audio_stream(void)
{
	if (wavptr)
	{
		wav_close(wavptr);
		wavptr = NULL;
	}
}
#endif



void osd_update_audio_stream(running_machine *machine, INT16 *buffer, int samples_this_frame)
{
	if (wavptr && (machine->sample_rate != 0))
		wav_add_data_16(wavptr, buffer, samples_this_frame * 2);
}



static const input_setting_config *find_switch(running_machine *machine, const char *switch_name,
	const char *switch_setting, int switch_type, int *found_field)
{
	int found = FALSE;
	const input_port_config *port;
	const input_field_config *field = NULL;
	const input_setting_config *setting = NULL;

	/* find switch with the name */
	found = FALSE;
	for (port = machine->portconfig; !found && (port != NULL); port = port->next)
	{
		for (field = port->fieldlist; !found && (field != NULL); field = field->next)
		{
			if (field->type == switch_type && input_field_name(field) && !mame_stricmp(input_field_name(field), switch_name))
				found = TRUE;
		}
	}

	/* did we find the field? */
	if (field != NULL)
	{
		/* we did find the field - now find the setting */
		for (setting = field->settinglist; setting != NULL; setting = setting->next)
		{
			if (setting->name != NULL && !mame_stricmp(setting->name, switch_setting))
				break;
		}
	}

	/* return whether the field was found at all, if we're asked that */
	if (found_field != NULL)
		*found_field = (field != NULL);

	return setting;
}


/* ----------------------------------------------------------------------- */

static void command_wait(running_machine *machine)
{
	attotime current_time = timer_get_time(machine);

	if (state == STATE_READY)
	{
		/* beginning a wait command */
		wait_target = attotime_add(current_time, current_command->u.wait_time);
		state = STATE_INCOMMAND;
	}
	else
	{
		/* during a wait command */
		state = (attotime_compare(current_time, wait_target) >= 0) ? STATE_READY : STATE_INCOMMAND;
	}
}



static void command_input(running_machine *machine)
{
	/* post a set of characters to the emulation */
	if (state == STATE_READY)
	{
		if (!inputx_can_post(machine))
		{
			state = STATE_ABORTED;
			report_message(MSG_FAILURE, "Natural keyboard input not supported for this driver");
			return;
		}

		/* input_chars can be NULL, so we should check for that */
		if (current_command->u.input_args.input_chars)
		{
			inputx_post_utf8_rate(machine, current_command->u.input_args.input_chars,
				current_command->u.input_args.rate);
		}
	}
	state = inputx_is_posting(machine) ? STATE_INCOMMAND : STATE_READY;
}



static void command_rawinput(running_machine *machine)
{
	//int parts;
	attotime current_time = timer_get_time(machine);
	static const char *position;
#if 0
	int i;
	double rate = ATTOTIME_IN_SEC(1);
	const char *s;
	char buf[256];
#endif

	if (state == STATE_READY)
	{
		/* beginning of a raw input command */
		//parts = 1;
		position = current_command->u.input_args.input_chars;
		wait_target = current_time;
		state = STATE_INCOMMAND;
	}
	else if (attotime_compare(current_time, wait_target) > 0)
	{
#if 0
		do
		{
			/* process the next command */
			while(!isspace(*position))
				position++;

			/* look up the input to trigger */
			for (i = 0; input_keywords[i].name; i++)
			{
				if (!strncmp(position, input_keywords[i].name, strlen(input_keywords[i].name)))
					break;
			}

			/* go to next command */
			position = strchr(position, ',');
			if (position)
				position++;
		}
		while(position && !input_keywords[i].name);

		current_fake_input = input_keywords[i].name ? &input_keywords[i] : NULL;
		if (position)
			wait_target = current_time + rate;
		else
			state = STATE_READY;
#endif
	}
}



static void command_screenshot(running_machine *machine)
{
	dump_screenshot(machine, TRUE);
}



static void command_checkblank(running_machine *machine)
{
	dump_screenshot(machine, FALSE);
}



static void command_switch(running_machine *machine)
{
	int found_field;
	const input_setting_config *setting;

	/* special hack until we support video targets natively */
	if (!strcmp(current_command->u.switch_args.name, "Video type"))
	{
		render_target *target = render_target_get_indexed(0);
		int view_index = 0;
		const char *view_name;

		while((view_name = render_target_get_view_name(target, view_index)) != NULL)
		{
			if (!strcmp(view_name, current_command->u.switch_args.value))
				break;
			view_index++;
		}

		if (view_name)
		{
			render_target_set_view(target, view_index);
			return;
		}
	}

	/* first try dip switches */
	setting = find_switch(machine, current_command->u.switch_args.name,
		current_command->u.switch_args.value, IPT_DIPSWITCH, &found_field);

	if (setting == NULL)
	{
		/* failing that, try configs */
		setting = find_switch(machine, current_command->u.switch_args.name,
			current_command->u.switch_args.value, IPT_CONFIG, &found_field);
	}

	if (setting == NULL)
	{
		/* now failing that, really fail - lets report a good message */
		if (found_field)
		{
			report_message(MSG_FAILURE, "Cannot find setting '%s' on switch '%s'",
				current_command->u.switch_args.value, current_command->u.switch_args.name);
		}
		else
		{
			report_message(MSG_FAILURE, "Cannot find switch named '%s'", current_command->u.switch_args.name);
		}
		state = STATE_ABORTED;
		return;
	}

	/* success! */
	/* FIXME */
	/* switch_name->default_value = setting->value; */
}


static void command_image_preload(running_machine *machine)
{
	state = STATE_ABORTED;
	report_message(MSG_FAILURE, "Image preloads must be at the beginning");
}



static const device_config *find_device_by_identity(running_machine *machine, const messtest_device_identity *ident)
{
	const device_config *device;

	/* look up the image slot */
	if (ident->type == IO_UNKNOWN)
	{
		/* no device_type was specified; use the new preferred mechanism */
		device = devtag_get_device(machine, ident->tag);
	}
	else
	{
		/* perform a legacy lookup by device type */
		device = image_from_devtype_and_index(machine, ident->type, ident->slot);
	}

	/* did the image slot lookup fail? */
	if (device == NULL)
	{
		state = STATE_ABORTED;
		report_message(MSG_FAILURE, "Device '%s %i' does not exist",
			device_typename(ident->type), ident->slot);
	}

	return device;
}



static void command_image_loadcreate(running_machine *machine)
{
	const device_config *image;
	const char *filename;
	const char *format_name;
	char buf[128];
	image_device_info info;
	const char *file_extensions;
	astring *filepath;
	int success;
	const game_driver *gamedrv;
	const image_device_format *format = NULL;

	/* look up the image slot */
	image = find_device_by_identity(machine, &current_command->u.image_args.device_ident);
	if (image == NULL)
		return;

	info = image_device_getinfo(machine->config, image);
	file_extensions = info.file_extensions;

	/* is an image format specified? */
	format_name = current_command->u.image_args.format;
	if (format_name != NULL)
	{
		if (current_command->command_type != MESSTEST_COMMAND_IMAGE_CREATE)
		{
			state = STATE_ABORTED;
			report_message(MSG_FAILURE, "Cannot specify format unless creating");
			return;
		}

		/* look up the format name */
		format = image_device_get_named_creatable_format(image, format_name);
		if (format == NULL)
		{
			state = STATE_ABORTED;
			report_message(MSG_FAILURE, "Unknown device format '%s'", format_name);
			return;
		}
	}

	/* figure out the filename */
	filename = current_command->u.image_args.filename;
	if (!filename)
	{
		snprintf(buf, sizeof(buf) / sizeof(buf[0]),	"%s.%s",
			current_testcase.name, file_extensions);
		osd_get_temp_filename(buf, ARRAY_LENGTH(buf), buf);
		filename = buf;
	}

	success = FALSE;
	for (gamedrv = machine->gamedrv; !success && gamedrv; gamedrv = mess_next_compatible_driver(gamedrv))
	{
		/* assemble the full path */
		filepath = assemble_software_path(astring_alloc(), gamedrv, filename);

		/* actually create or load the image */
		switch(current_command->command_type)
		{
			case MESSTEST_COMMAND_IMAGE_CREATE:
				success = (image_create(image, astring_c(filepath), format, NULL) == INIT_PASS);
				break;

			case MESSTEST_COMMAND_IMAGE_LOAD:
				success = (image_load(image, astring_c(filepath)) == INIT_PASS);
				break;

			default:
				fatalerror("Unexpected error");
				break;
		}
		astring_free(filepath);
	}
	if (!success)
	{
		state = STATE_ABORTED;
		report_message(MSG_FAILURE, "Failed to load/create image '%s': %s", filename, image_error(image));
		return;
	}
}



static void command_verify_memory(running_machine *machine)
{
	int i = 0;
	offs_t offset, offset_start, offset_end;
	const UINT8 *verify_data;
	size_t verify_data_size;
	const UINT8 *target_data;
	size_t target_data_size;
	const char *region;

	offset_start = current_command->u.verify_args.start;
	offset_end = current_command->u.verify_args.end;
	verify_data = (const UINT8 *) current_command->u.verify_args.verify_data;
	verify_data_size = current_command->u.verify_args.verify_data_size;

	if (offset_end == 0)
		offset_end = offset_start + verify_data_size - 1;

	/* what type of memory are we validating? */
	region = current_command->u.verify_args.mem_region;
	if (region)
	{
		/* we're validating a conventional memory region */
		target_data = memory_region(machine, region);
		target_data_size = memory_region_length(machine, region);
	}
	else
	{
		/* we're validating mess_ram */
		target_data = mess_ram;
		target_data_size = mess_ram_size;
	}

	/* sanity check the ranges */
	if (!verify_data || (verify_data_size <= 0))
	{
		state = STATE_ABORTED;
		report_message(MSG_FAILURE, "Invalid memory region during verify");
		return;
	}
	if (offset_start > offset_end)
	{
		state = STATE_ABORTED;
		report_message(MSG_FAILURE, "Invalid verify offset range (0x%x-0x%x)", offset_start, offset_end);
		return;
	}
	if (offset_end >= target_data_size)
	{
		state = STATE_ABORTED;
		report_message(MSG_FAILURE, "Verify memory range out of bounds");
		return;
	}

	/* loop through the memory, verifying it byte by byte */
	for (offset = offset_start; offset <= offset_end; offset++)
	{
		if (verify_data[i] != target_data[offset])
		{
			state = STATE_ABORTED;
			report_message(MSG_FAILURE, "Failed verification step (region %s; 0x%x-0x%x)",
				region, offset_start, offset_end);
			break;
		}
		i = (i + 1) % verify_data_size;
	}
}



static void command_verify_image(running_machine *machine)
{
	const UINT8 *verify_data;
	size_t verify_data_size;
	size_t offset, offset_start, offset_end;
	const char *filename;
	const device_config *image;
	FILE *f;
	UINT8 c;
	char filename_buf[512];

	verify_data = (const UINT8 *) current_command->u.verify_args.verify_data;
	verify_data_size = current_command->u.verify_args.verify_data_size;

	/* look up the device */
	image = find_device_by_identity(machine, &current_command->u.verify_args.device_ident);
	if (image == NULL)
		return;

	filename = image_filename(image);
	if (filename == NULL)
	{
		state = STATE_ABORTED;
		report_message(MSG_FAILURE, "Failed verification: Device Not Loaded");
		return;
	}

	/* very dirty hack - we unload the image because we cannot access it
	 * because the file is locked */
	strcpy(filename_buf, filename);
	image_unload(image);
	filename = filename_buf;

	f = fopen(filename, "r");
	if (!f)
	{
		state = STATE_ABORTED;
		report_message(MSG_FAILURE, "Failed verification: Cannot open image to verify");
		return;
	}

	offset_start = 0;
	offset_end = verify_data_size - 1;

	for (offset = offset_start; offset <= offset_end; offset++)
	{
		fseek(f, offset, SEEK_SET);
		c = (UINT8) fgetc(f);

		if (c != verify_data[offset])
		{
			state = STATE_ABORTED;
			report_message(MSG_FAILURE, "Failed verification step (%s; 0x%x-0x%x)",
				filename, (int)offset_start, (int)offset_end);
			break;
		}
	}
	fclose(f);
}



static void command_trace(running_machine *machine)
{
	const device_config *cpu;
	int cpunum = 0;
	FILE *file;
	char filename[256];

	for (cpu = cpu_first(machine->config); cpu != NULL; cpu = cpu_next(cpu))
	{
		if (cpu_next(cpu_first(machine->config)) == NULL)
			snprintf(filename, sizeof(filename) / sizeof(filename[0]), "_%s.tr", current_testcase.name);
		else
			snprintf(filename, sizeof(filename) / sizeof(filename[0]), "_%s.%d.tr", current_testcase.name, cpunum);

		file = fopen(filename, "w");
		if (file)
		{
			report_message(MSG_INFO, "Tracing CPU #%d: %s", cpunum, filename);
			debug_cpu_trace(cpu, file, FALSE, NULL);
			fclose(file);
		}

		cpunum++;
	}
}



static void command_soft_reset(running_machine *machine)
{
	mame_schedule_soft_reset(machine);
}



static void command_hard_reset(running_machine *machine)
{
	mame_schedule_hard_reset(machine);
}



static void command_end(running_machine *machine)
{
	/* at the end of our test */
	state = STATE_DONE;
	final_time = timer_get_time(machine);
	mame_schedule_exit(machine);
}



/* ----------------------------------------------------------------------- */

struct command_procmap_entry
{
	messtest_command_type_t command_type;
	void (*proc)(running_machine *machine);
};

static const struct command_procmap_entry commands[] =
{
	{ MESSTEST_COMMAND_WAIT,			command_wait },
	{ MESSTEST_COMMAND_INPUT,			command_input },
	{ MESSTEST_COMMAND_RAWINPUT,		command_rawinput },
	{ MESSTEST_COMMAND_SCREENSHOT,		command_screenshot },
	{ MESSTEST_COMMAND_CHECKBLANK,		command_checkblank },
	{ MESSTEST_COMMAND_SWITCH,			command_switch },
	{ MESSTEST_COMMAND_IMAGE_PRELOAD,	command_image_preload },
	{ MESSTEST_COMMAND_IMAGE_LOAD,		command_image_loadcreate },
	{ MESSTEST_COMMAND_IMAGE_CREATE,	command_image_loadcreate },
	{ MESSTEST_COMMAND_VERIFY_MEMORY,	command_verify_memory },
	{ MESSTEST_COMMAND_VERIFY_IMAGE,	command_verify_image },
	{ MESSTEST_COMMAND_TRACE,			command_trace },
	{ MESSTEST_COMMAND_SOFT_RESET,		command_soft_reset },
	{ MESSTEST_COMMAND_HARD_RESET,		command_hard_reset },
	{ MESSTEST_COMMAND_END,				command_end }
};

void osd_update(running_machine *machine, int skip_redraw)
{
	int i;
	attotime time_limit;
	attotime current_time;

	render_target_get_primitives(target);

	/* don't do anything if we are initializing! */
	switch(mame_get_phase(machine))
	{
		case MAME_PHASE_PREINIT:
		case MAME_PHASE_INIT:
		case MAME_PHASE_RESET:
			return;
	}

	/* if we have already aborted or completed, our work is done */
	if ((state == STATE_ABORTED) || (state == STATE_DONE))
	{
		mame_schedule_exit(machine);
		return;
	}

	/* have we hit the time limit? */
	current_time = timer_get_time(machine);
	time_limit = (attotime_compare(current_testcase.time_limit, attotime_zero) != 0) ? current_testcase.time_limit
		: ATTOTIME_IN_SEC(600);
	if (attotime_compare(current_time, time_limit) > 0)
	{
		state = STATE_ABORTED;
		report_message(MSG_FAILURE, "Time limit of %s attoseconds exceeded", attotime_string(time_limit, 9));
		return;
	}

	for (i = 0; i < sizeof(commands) / sizeof(commands[i]); i++)
	{
		if (current_command->command_type == commands[i].command_type)
		{
			commands[i].proc(machine);
			break;
		}
	}

	/* if we are ready for the next command, advance to it */
	if (state == STATE_READY)
	{
		/* if we are at the end, and we are dumping screenshots, and we didn't
		 * just dump a screenshot, dump one now
		 */
		if ((test_flags & MESSTEST_ALWAYS_DUMP_SCREENSHOT) &&
			(current_command[0].command_type != MESSTEST_COMMAND_SCREENSHOT) &&
			(current_command[1].command_type == MESSTEST_COMMAND_END))
		{
			dump_screenshot(machine, TRUE);
		}

		current_command++;
	}
}



char *rompath_extra;



static int append_command(void)
{
	if (pile_write(&command_pile, &new_command, sizeof(new_command)))
		return FALSE;
	current_testcase.commands = (messtest_command *) pile_getptr(&command_pile);
	command_count++;
	return TRUE;
}


#ifdef UNUSED_FUNCTION
static void command_end_handler(const void *buffer, size_t size)
{
	if (!append_command())
	{
		error_outofmemory();
		return;
	}
}
#endif


static void node_wait(xml_data_node *node)
{
	xml_attribute_node *attr_node;

	attr_node = xml_get_attribute(node, "time");
	if (!attr_node)
	{
		error_missingattribute("time");
		return;
	}

	memset(&new_command, 0, sizeof(new_command));
	new_command.command_type = MESSTEST_COMMAND_WAIT;
	new_command.u.wait_time = parse_time(attr_node->value);

	if (!append_command())
	{
		error_outofmemory();
		return;
	}
}



static void node_input(xml_data_node *node)
{
	/* <input> - inputs natural keyboard data into a system */
	xml_attribute_node *attr_node;
	attotime rate;

	memset(&new_command, 0, sizeof(new_command));
	new_command.command_type = MESSTEST_COMMAND_INPUT;
	attr_node = xml_get_attribute(node, "rate");
	rate = attr_node ? parse_time(attr_node->value) : attotime_make(0, 0);
	new_command.u.input_args.rate = rate;
	new_command.u.input_args.input_chars = node->value;

	if (!append_command())
	{
		error_outofmemory();
		return;
	}
}



static void node_rawinput(xml_data_node *node)
{
	/* <rawinput> - inputs raw data into a system */
	memset(&new_command, 0, sizeof(new_command));
	new_command.command_type = MESSTEST_COMMAND_RAWINPUT;

	if (!append_command())
	{
		error_outofmemory();
		return;
	}
}



static void node_switch(xml_data_node *node)
{
	xml_attribute_node *attr_node;
	const char *s1;
	const char *s2;

	/* <switch> - switches a DIP switch/config setting */
	memset(&new_command, 0, sizeof(new_command));
	new_command.command_type = MESSTEST_COMMAND_SWITCH;

	/* 'name' attribute */
	attr_node = xml_get_attribute(node, "name");
	if (!attr_node)
	{
		error_missingattribute("name");
		return;
	}
	s1 = attr_node->value;

	/* 'value' attribute */
	attr_node = xml_get_attribute(node, "value");
	if (!attr_node)
	{
		error_missingattribute("value");
		return;
	}
	s2 = attr_node->value;

	new_command.u.switch_args.name = s1;
	new_command.u.switch_args.value = s2;

	if (!append_command())
	{
		error_outofmemory();
		return;
	}
}



static void node_screenshot(xml_data_node *node)
{
	/* <screenshot> - dumps a screenshot */
	memset(&new_command, 0, sizeof(new_command));
	new_command.command_type = MESSTEST_COMMAND_SCREENSHOT;

	if (!append_command())
	{
		error_outofmemory();
		return;
	}
}



static void node_checkblank(xml_data_node *node)
{
	/* <checkblank> - checks to see if the screen is blank */
	memset(&new_command, 0, sizeof(new_command));
	new_command.command_type = MESSTEST_COMMAND_CHECKBLANK;

	if (!append_command())
	{
		error_outofmemory();
		return;
	}
}



static int get_device_identity_tags(xml_data_node *node, messtest_device_identity *ident)
{
	xml_attribute_node *attr_node;
	const char *s2;

	/* clear out the result */
	memset(ident, 0, sizeof(*ident));

	/* 'type' attribute */
	attr_node = xml_get_attribute(node, "type");
	if (attr_node != NULL)
	{
		s2 = attr_node->value;

		ident->type = device_typeid(s2);
		if (ident->type <= IO_UNKNOWN)
		{
			error_baddevicetype(s2);
			return FALSE;
		}
	}
	else
	{
		/* the device type was unspecified */
		ident->type = IO_UNKNOWN;
	}

	/* 'slot' attribute */
	attr_node = xml_get_attribute(node, "slot");
	ident->slot = (attr_node != NULL) ? atoi(attr_node->value) : 0;

	/* 'tag' attribute */
	attr_node = xml_get_attribute(node, "tag");
	ident->tag = (attr_node != NULL) ? attr_node->value : NULL;

	return TRUE;
}



static void node_image(xml_data_node *node, messtest_command_type_t command)
{
	xml_attribute_node *attr_node;
	int preload;

	memset(&new_command, 0, sizeof(new_command));
	new_command.command_type = command;

	/* 'preload' attribute */
	attr_node = xml_get_attribute(node, "preload");
	preload = attr_node ? atoi(attr_node->value) : 0;
	if (preload)
		new_command.command_type += 2;

	/* 'filename' attribute */
	attr_node = xml_get_attribute(node, "filename");
	new_command.u.image_args.filename = attr_node ? attr_node->value : NULL;

	/* 'tag', 'type', 'slot' attributes */
	if (!get_device_identity_tags(node, &new_command.u.image_args.device_ident))
		return;

	/* 'format' attribute */
	attr_node = xml_get_attribute(node, "format");
	new_command.u.image_args.format = attr_node ? attr_node->value : NULL;

	if (!append_command())
	{
		error_outofmemory();
		return;
	}
}



static void node_imagecreate(xml_data_node *node)
{
	/* <imagecreate> - creates an image */
	node_image(node, MESSTEST_COMMAND_IMAGE_CREATE);
}



static void node_imageload(xml_data_node *node)
{
	/* <imageload> - loads an image */
	node_image(node, MESSTEST_COMMAND_IMAGE_LOAD);
}



static void node_memverify(xml_data_node *node)
{
	xml_attribute_node *attr_node;
	const char *s1;
	const char *s2;
	void *new_buffer;
	mess_pile pile;

	/* <memverify> - verifies that a range of memory contains specific data */
	attr_node = xml_get_attribute(node, "start");
	s1 = attr_node ? attr_node->value : NULL;
	if (!s1)
	{
		error_missingattribute("start");
		return;
	}

	attr_node = xml_get_attribute(node, "end");
	s2 = attr_node ? attr_node->value : NULL;
	if (!s2)
		s2 = "0";

	attr_node = xml_get_attribute(node, "region");
	new_command.u.verify_args.mem_region = attr_node ? attr_node->value : NULL;

	memset(&new_command, 0, sizeof(new_command));
	new_command.command_type = MESSTEST_COMMAND_VERIFY_MEMORY;
	new_command.u.verify_args.start = parse_offset(s1);
	new_command.u.verify_args.end = parse_offset(s2);

	pile_init(&pile);
	messtest_get_data(node, &pile);
	new_buffer = pool_malloc(command_pool, pile_size(&pile));
	memcpy(new_buffer, pile_getptr(&pile), pile_size(&pile));
	new_command.u.verify_args.verify_data = new_buffer;
	new_command.u.verify_args.verify_data_size = pile_size(&pile);
	pile_delete(&pile);

	if (!append_command())
	{
		error_outofmemory();
		return;
	}
}



static void node_imageverify(xml_data_node *node)
{
	void *new_buffer;
	mess_pile pile;

	/* <imageverify> - verifies that an image contains specific data */
	memset(&new_command, 0, sizeof(new_command));
	new_command.command_type = MESSTEST_COMMAND_VERIFY_IMAGE;

	/* 'tag', 'type', 'slot' attributes */
	if (!get_device_identity_tags(node, &new_command.u.verify_args.device_ident))
		return;

	pile_init(&pile);
	messtest_get_data(node, &pile);
	new_buffer = pool_malloc(command_pool, pile_size(&pile));
	memcpy(new_buffer, pile_getptr(&pile), pile_size(&pile));
	new_command.u.verify_args.verify_data = new_buffer;
	new_command.u.verify_args.verify_data_size = pile_size(&pile);
	pile_delete(&pile);

	if (!append_command())
	{
		error_outofmemory();
		return;
	}
}


#ifdef UNUSED_FUNCTION
static void verify_end_handler(const void *buffer, size_t size)
{
	void *new_buffer;
	new_buffer = pool_malloc(command_pool, size);
	memcpy(new_buffer, buffer, size);

	new_command.u.verify_args.verify_data = new_buffer;
	new_command.u.verify_args.verify_data_size = size;
	command_end_handler(NULL, 0);

	if (!append_command())
	{
		error_outofmemory();
		return;
	}
}
#endif


static void node_trace(xml_data_node *node)
{
	/* <trace> - emit a trace file */
	memset(&new_command, 0, sizeof(new_command));
	new_command.command_type = MESSTEST_COMMAND_TRACE;

	if (!append_command())
	{
		error_outofmemory();
		return;
	}
}



static void node_soft_reset(xml_data_node *node)
{
	/* <reset> - perform a soft reset */
	memset(&new_command, 0, sizeof(new_command));
	new_command.command_type = MESSTEST_COMMAND_SOFT_RESET;

	if (!append_command())
	{
		error_outofmemory();
		return;
	}
}



static void node_hard_reset(xml_data_node *node)
{
	/* <hardreset> - perform a hard reset */
	memset(&new_command, 0, sizeof(new_command));
	new_command.command_type = MESSTEST_COMMAND_HARD_RESET;

	if (!append_command())
	{
		error_outofmemory();
		return;
	}
}



void node_testmess(xml_data_node *node)
{
	xml_data_node *child_node;
	xml_attribute_node *attr_node;
	int result;

	pile_init(&command_pile);
	command_pool = pool_alloc(NULL);

	memset(&new_command, 0, sizeof(new_command));
	command_count = 0;

	/* 'driver' attribute */
	attr_node = xml_get_attribute(node, "driver");
	if (!attr_node)
	{
		error_missingattribute("driver");
		return;
	}
	current_testcase.driver = attr_node->value;

	/* 'name' attribute */
	attr_node = xml_get_attribute(node, "name");
	current_testcase.name = attr_node ? attr_node->value : current_testcase.driver;

	/* 'bios' attribute */
	attr_node = xml_get_attribute(node, "bios");
	current_testcase.bios = attr_node ? attr_node->value : NULL;

	/* 'ramsize' attribute */
	attr_node = xml_get_attribute(node, "ramsize");
	current_testcase.ram = attr_node ? ram_parse_string(attr_node->value) : 0;

	/* 'wavwrite' attribute */
	attr_node = xml_get_attribute(node, "wavwrite");
	current_testcase.wavwrite = attr_node && (atoi(attr_node->value) != 0);

	/* 'enabled' attribute */
	attr_node = xml_get_attribute(node, "enabled");
	current_testcase.enabled = (!attr_node || atoi(attr_node->value)) ? TRUE : FALSE;

	/* report the beginning of the test case */
	report_testcase_begin(current_testcase.name);

	if (current_testcase.enabled)
	{
		current_testcase.commands = NULL;

		for (child_node = node->child; child_node; child_node = child_node->next)
		{
			if (!strcmp(child_node->name, "wait"))
				node_wait(child_node);
			else if (!strcmp(child_node->name, "input"))
				node_input(child_node);
			else if (!strcmp(child_node->name, "rawinput"))
				node_rawinput(child_node);
			else if (!strcmp(child_node->name, "switch"))
				node_switch(child_node);
			else if (!strcmp(child_node->name, "screenshot"))
				node_screenshot(child_node);
			else if (!strcmp(child_node->name, "checkblank"))
				node_checkblank(child_node);
			else if (!strcmp(child_node->name, "imagecreate"))
				node_imagecreate(child_node);
			else if (!strcmp(child_node->name, "imageload"))
				node_imageload(child_node);
			else if (!strcmp(child_node->name, "memverify"))
				node_memverify(child_node);
			else if (!strcmp(child_node->name, "imageverify"))
				node_imageverify(child_node);
			else if (!strcmp(child_node->name, "trace"))
				node_trace(child_node);
			else if (!strcmp(child_node->name, "reset"))
				node_soft_reset(child_node);
			else if (!strcmp(child_node->name, "hardreset"))
				node_hard_reset(child_node);
		}

		memset(&new_command, 0, sizeof(new_command));
		new_command.command_type = MESSTEST_COMMAND_END;
		if (!append_command())
		{
			error_outofmemory();
			return;
		}

		result = run_test(0, NULL);
	}
	else
	{
		/* report that the test case was skipped */
		report_message(MSG_INFO, "Test case skipped");
		result = 0;
	}

	report_testcase_ran(result);
	pile_delete(&command_pile);
	pool_free(command_pool);
}
