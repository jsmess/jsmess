/***************************************************************************

    options.c

    Options file and command line management.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "mamecore.h"
#include "mame.h"
#include "fileio.h"
#include "options.h"

#include <ctype.h>



/***************************************************************************
    CONSTANTS
***************************************************************************/

#define MAX_ENTRY_NAMES		4



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _options_data options_data;
struct _options_data
{
	options_data *			next;				/* link to the next data */
	const char *			names[MAX_ENTRY_NAMES]; /* array of possible names */
	UINT32					flags;				/* flags from the entry */
	UINT32					seqid;				/* sequence ID; bumped on each change */
	int						error_reported;		/* have we reported an error on this option yet? */
	const char *			data;				/* data for this item */
	const char *			defdata;			/* default data for this item */
	const char *			description;		/* description for this item */
	void					(*callback)(const char *arg);	/* callback to be invoked when parsing */
};



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

const char *option_unadorned[MAX_UNADORNED_OPTIONS] =
{
	"<UNADORNED0>",
	"<UNADORNED1>",
	"<UNADORNED2>",
	"<UNADORNED3>",
	"<UNADORNED4>",
	"<UNADORNED5>",
	"<UNADORNED6>",
	"<UNADORNED7>",
	"<UNADORNED8>",
	"<UNADORNED9>",
	"<UNADORNED10>",
	"<UNADORNED11>",
	"<UNADORNED12>",
	"<UNADORNED13>",
	"<UNADORNED14>",
	"<UNADORNED15>"
};



/***************************************************************************
    BUILT-IN (CORE) OPTIONS
***************************************************************************/

static const options_entry core_options[] =
{
	// unadorned options - only a single one supported at the moment
	{ "<UNADORNED0>",                NULL,        0,                 NULL },

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

	{ NULL }
};



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

static options_data *		datalist;
static options_data **		datalist_nextptr = NULL;



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static options_data *find_entry_data(const char *string, int is_command_line);
static void update_data(options_data *data, const char *newdata);



/***************************************************************************
    CORE IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    copy_string - allocate a copy of a string
-------------------------------------------------*/

static const char *copy_string(const char *start, const char *end)
{
	char *result;

	/* copy of a NULL is NULL */
	if (start == NULL)
		return NULL;

	/* if no end, figure it out */
	if (end == NULL)
		end = start + strlen(start);

	/* allocate, copy, and NULL-terminate */
	result = malloc_or_die((end - start) + 1);
	memcpy(result, start, end - start);
	result[end - start] = 0;

	return result;
}


/*-------------------------------------------------
    separate_names - separate a list of semicolon
    separated names into individual strings
-------------------------------------------------*/

static int separate_names(const char *srcstring, const char *results[], int maxentries)
{
	const char *start;
	int curentry;

	/* start with the original string and loop over entries */
	start = srcstring;
	for (curentry = 0; curentry < maxentries; curentry++)
	{
		/* find the end of this entry and copy the string */
		const char *end = strchr(start, ';');
		results[curentry] = copy_string(start, end);

		/* if we hit the end of the source, stop */
		if (end == NULL)
			break;
		start = end + 1;
	}
	return curentry;
}


/*-------------------------------------------------
    options_init - initialize the options system
-------------------------------------------------*/

void options_init(const options_entry *entrylist)
{
	/* free anything we currently have */
	options_free_entries();
	datalist_nextptr = &datalist;

	/* add core options and optionally add options that are passed to us */
	options_add_entries(core_options);
	if (entrylist != NULL)
		options_add_entries(entrylist);
}


/*-------------------------------------------------
    options_add_entries - add entries to the
    current options sets
-------------------------------------------------*/

void options_add_entries(const options_entry *entrylist)
{
	assert_always(datalist_nextptr != NULL, "Missing call to options_init()!");

	/* loop over entries until we hit a NULL name */
	for ( ; entrylist->name != NULL || (entrylist->flags & OPTION_HEADER); entrylist++)
	{
		options_data *match = NULL;
		int i;

		/* allocate a new item */
		options_data *data = malloc_or_die(sizeof(*data));
		memset(data, 0, sizeof(*data));

		/* separate the names */
		if (entrylist->name != NULL)
			separate_names(entrylist->name, data->names, ARRAY_LENGTH(data->names));

		/* do we match an existing entry? */
		for (i = 0; i < ARRAY_LENGTH(data->names) && match == NULL; i++)
			if (data->names[i] != NULL)
				match = find_entry_data(data->names[i], FALSE);

		/* if so, throw away this entry and replace the data */
		if (match != NULL)
		{
			/* free what we've allocated so far */
			for (i = 0; i < ARRAY_LENGTH(data->names); i++)
				if (data->names[i] != NULL)
					free((void *)data->names[i]);
			free(data);

			/* update the data and default data */
			data = match;
			if (data->data != NULL)
				free((void *)data->data);
			if (data->defdata != NULL)
				free((void *)data->defdata);
			data->data = copy_string(entrylist->defvalue, NULL);
			data->defdata = copy_string(entrylist->defvalue, NULL);
		}

		/* otherwise, finish making the new entry */
		else
		{
			/* copy the flags, and set the value equal to the default */
			data->flags = entrylist->flags;
			data->data = copy_string(entrylist->defvalue, NULL);
			data->defdata = copy_string(entrylist->defvalue, NULL);
			data->description = entrylist->description;

			/* fill it in and add to the end of the list */
			*datalist_nextptr = data;
			datalist_nextptr = &data->next;
		}
	}
}


/*-------------------------------------------------
    options_set_option_default_value - change the
    default value of an option
-------------------------------------------------*/

void options_set_option_default_value(const char *name, const char *defvalue)
{
	options_data *data = find_entry_data(name, TRUE);
	assert(data != NULL);

	/* free the existing value and make a copy for the new one */
	/* note that we assume this is called before any data is processed */
	if (data->data != NULL)
		free((void *)data->data);
	if (data->defdata != NULL)
		free((void *)data->defdata);
	data->data = copy_string(defvalue, NULL);
	data->defdata = copy_string(defvalue, NULL);
}


/*-------------------------------------------------
    options_set_option_callback - specifies a
    callback to be invoked when parsing options
-------------------------------------------------*/

void options_set_option_callback(const char *name, void (*callback)(const char *arg))
{
	options_data *data = find_entry_data(name, TRUE);
	assert(data != NULL);
	data->callback = callback;
}


/*-------------------------------------------------
    options_free_entries - free all the entries
    that were added
-------------------------------------------------*/

void options_free_entries(void)
{
	/* free all of the registered data */
	while (datalist != NULL)
	{
		options_data *temp = datalist;
		int i;

		datalist = temp->next;

		/* free names and data, and finally the element itself */
		for (i = 0; i < ARRAY_LENGTH(temp->names); i++)
			if (temp->names[i] != NULL)
				free((void *)temp->names[i]);
		if (temp->defdata != NULL)
			free((void *)temp->defdata);
		if (temp->data != NULL)
			free((void *)temp->data);
		free(temp);
	}

	/* reset the nextptr */
	datalist_nextptr = &datalist;
}


/*-------------------------------------------------
    options_parse_command_line - parse a series
    of command line arguments
-------------------------------------------------*/

int options_parse_command_line(int argc, char **argv)
{
	int unadorned_index = 0;
	int arg;

	/* loop over commands, looking for options */
	for (arg = 1; arg < argc; arg++)
	{
		options_data *data;
		const char *optionname;
		const char *newdata;

		/* determine the entry name to search for */
		if (argv[arg][0] == '-')
			optionname = &argv[arg][1];
		else
		{
			optionname = OPTION_UNADORNED(unadorned_index);
			unadorned_index++;
		}

		/* find our entry */
		data = find_entry_data(optionname, TRUE);
		if (data == NULL)
		{
			mame_printf_error("Error: unknown option: %s\n", argv[arg]);
			return 1;
		}
		if ((data->flags & (OPTION_DEPRECATED | OPTION_INTERNAL)) != 0)
			continue;

		/* get the data for this argument, special casing booleans */
		if ((data->flags & (OPTION_BOOLEAN | OPTION_COMMAND)) != 0)
			newdata = (strncmp(&argv[arg][1], "no", 2) == 0) ? "0" : "1";
		else if (argv[arg][0] != '-')
			newdata = argv[arg];
		else if (arg + 1 < argc)
			newdata = argv[++arg];
		else
		{
			mame_printf_error("Error: option %s expected a parameter\n", argv[arg]);
			return 1;
		}

		/* invoke callback, if present */
		if (data->callback != NULL)
			(*data->callback)(newdata);

		/* allocate a new copy of data for this */
		update_data(data, newdata);
	}
	return 0;
}


/*-------------------------------------------------
    options_parse_ini_file - parse a series
    of entries in an INI file
-------------------------------------------------*/

int options_parse_ini_file(mame_file *inifile)
{
	/* loop over data */
	while (mame_fgets(giant_string_buffer, GIANT_STRING_BUFFER_SIZE, inifile) != NULL)
	{
		char *optionname, *optiondata, *temp;
		options_data *data;
		int inquotes = FALSE;

		/* find the name */
		for (optionname = giant_string_buffer; *optionname != 0; optionname++)
			if (!isspace(*optionname))
				break;

		/* skip comments */
		if (*optionname == 0 || *optionname == '#')
			continue;

		/* scan forward to find the first space */
		for (temp = optionname; *temp != 0; temp++)
			if (isspace(*temp))
				break;

		/* if we hit the end early, print a warning and continue */
		if (*temp == 0)
		{
			mame_printf_warning("Warning: invalid line in INI: %s", giant_string_buffer);
			continue;
		}

		/* NULL-terminate */
		*temp++ = 0;
		optiondata = temp;

		/* scan the data, stopping when we hit a comment */
		for (temp = optiondata; *temp != 0; temp++)
		{
			if (*temp == '"')
				inquotes = !inquotes;
			if (*temp == '#' && !inquotes)
				break;
		}
		*temp = 0;

		/* find our entry */
		data = find_entry_data(optionname, FALSE);
		if (data == NULL)
		{
			mame_printf_warning("Warning: unknown option in INI: %s\n", optionname);
			continue;
		}
		if ((data->flags & (OPTION_DEPRECATED | OPTION_INTERNAL)) != 0)
			continue;

		/* allocate a new copy of data for this */
		update_data(data, optiondata);
	}
	return 0;
}


/*-------------------------------------------------
    options_output_ini_file - output the current
    state to an INI file
-------------------------------------------------*/

void options_output_ini_file(FILE *inifile)
{
	options_data *data;

	/* loop over all items */
	for (data = datalist; data != NULL; data = data->next)
	{
		/* header: just print */
		if ((data->flags & OPTION_HEADER) != 0)
			fprintf(inifile, "\n#\n# %s\n#\n", data->description);

		/* otherwise, output entries for all non-deprecated and non-command items */
		else if ((data->flags & (OPTION_DEPRECATED | OPTION_INTERNAL | OPTION_COMMAND)) == 0 && data->names[0][0] != 0)
		{
			if (data->data == NULL)
				fprintf(inifile, "# %-23s <NULL> (not set)\n", data->names[0]);
			else if (strchr(data->data, ' ') != NULL)
				fprintf(inifile, "%-25s \"%s\"\n", data->names[0], data->data);
			else
				fprintf(inifile, "%-25s %s\n", data->names[0], data->data);
		}
	}
}


/*-------------------------------------------------
    options_output_ini_mame_file - output the current
    state to an INI file
-------------------------------------------------*/

void options_output_ini_mame_file(mame_file *inifile)
{
	options_data *data;

	/* loop over all items */
	for (data = datalist; data != NULL; data = data->next)
	{
		/* header: just print */
		if ((data->flags & OPTION_HEADER) != 0)
			mame_fprintf(inifile, "\n#\n# %s\n#\n", data->description);

		/* otherwise, output entries for all non-deprecated and non-command items */
		else if ((data->flags & (OPTION_DEPRECATED | OPTION_INTERNAL | OPTION_COMMAND)) == 0 && data->names[0][0] != 0)
		{
			if (data->data == NULL)
				mame_fprintf(inifile, "# %-23s <NULL> (not set)\n", data->names[0]);
			else if (strchr(data->data, ' ') != NULL)
				mame_fprintf(inifile, "%-25s \"%s\"\n", data->names[0], data->data);
			else
				mame_fprintf(inifile, "%-25s %s\n", data->names[0], data->data);
		}
	}
}


/*-------------------------------------------------
    options_output_help - output option help to
    a file
-------------------------------------------------*/

void options_output_help(void)
{
	options_data *data;

	/* loop over all items */
	for (data = datalist; data != NULL; data = data->next)
	{
		/* header: just print */
		if ((data->flags & OPTION_HEADER) != 0)
			mame_printf_info("\n#\n# %s\n#\n", data->description);

		/* otherwise, output entries for all non-deprecated items */
		else if ((data->flags & (OPTION_DEPRECATED | OPTION_INTERNAL)) == 0 && data->description != NULL)
			mame_printf_info("-%-20s%s\n", data->names[0], data->description);
	}
}


/*-------------------------------------------------
    options_get_string - return data formatted
    as a string
-------------------------------------------------*/

const char *options_get_string(const char *name)
{
	options_data *data = find_entry_data(name, FALSE);
	assert(data != NULL);
	return (data == NULL) ? "" : data->data;
}


/*-------------------------------------------------
    options_get_seqid - return the seqid for an
    entry
-------------------------------------------------*/

UINT32 options_get_seqid(const char *name)
{
	options_data *data = find_entry_data(name, FALSE);
	return (data == NULL) ? 0 : data->seqid;
}


/*-------------------------------------------------
    options_get_bool - return data formatted as
    a boolean
-------------------------------------------------*/

int options_get_bool(const char *name)
{
	options_data *data = find_entry_data(name, FALSE);
	int value = FALSE;

	if (data == NULL)
		mame_printf_error("Unexpected boolean option %s queried\n", name);
	else if (data->data == NULL || sscanf(data->data, "%d", &value) != 1 || value < 0 || value > 1)
	{
		if (data->defdata != NULL)
		{
			options_set_string(name, data->defdata);
			sscanf(data->data, "%d", &value);
		}
		if (!data->error_reported)
		{
			mame_printf_error("Illegal boolean value for %s; reverting to %d\n", data->names[0], value);
			data->error_reported = TRUE;
		}
	}
	return value;
}


/*-------------------------------------------------
    options_get_int - return data formatted as
    an integer
-------------------------------------------------*/

int options_get_int(const char *name)
{
	options_data *data = find_entry_data(name, FALSE);
	int value = 0;

	if (data == NULL)
		mame_printf_error("Unexpected integer option %s queried\n", name);
	else if (data->data == NULL || sscanf(data->data, "%d", &value) != 1)
	{
		if (data->defdata != NULL)
		{
			options_set_string(name, data->defdata);
			sscanf(data->data, "%d", &value);
		}
		if (!data->error_reported)
		{
			mame_printf_error("Illegal integer value for %s; reverting to %d\n", data->names[0], value);
			data->error_reported = TRUE;
		}
	}
	return value;
}


/*-------------------------------------------------
    options_get_float - return data formatted as
    a float
-------------------------------------------------*/

float options_get_float(const char *name)
{
	options_data *data = find_entry_data(name, FALSE);
	float value = 0;

	if (data == NULL)
		mame_printf_error("Unexpected float option %s queried\n", name);
	else if (data->data == NULL || sscanf(data->data, "%f", &value) != 1)
	{
		if (data->defdata != NULL)
		{
			options_set_string(name, data->defdata);
			sscanf(data->data, "%f", &value);
		}
		if (!data->error_reported)
		{
			mame_printf_error("Illegal float value for %s; reverting to %f\n", data->names[0], (double)value);
			data->error_reported = TRUE;
		}
	}
	return value;
}


/*-------------------------------------------------
    options_get_int_range - return data formatted
    as an integer and clamped to within the given
    range
-------------------------------------------------*/

int options_get_int_range(const char *name, int minval, int maxval)
{
	options_data *data = find_entry_data(name, FALSE);
	int value = 0;

	if (data == NULL)
		mame_printf_error("Unexpected integer option %s queried\n", name);
	else if (data->data == NULL || sscanf(data->data, "%d", &value) != 1)
	{
		if (data->defdata != NULL)
		{
			options_set_string(name, data->defdata);
			value = options_get_int(name);
		}
		if (!data->error_reported)
		{
			mame_printf_error("Illegal integer value for %s; reverting to %d\n", data->names[0], value);
			data->error_reported = TRUE;
		}
	}
	else if (value < minval || value > maxval)
	{
		options_set_string(name, data->defdata);
		value = options_get_int(name);
		if (!data->error_reported)
		{
			mame_printf_error("Invalid %s value (must be between %d and %d); reverting to %d\n", data->names[0], minval, maxval, value);
			data->error_reported = TRUE;
		}
	}

	return value;
}


/*-------------------------------------------------
    options_get_float_range - return data formatted
    as a float and clamped to within the given
    range
-------------------------------------------------*/

float options_get_float_range(const char *name, float minval, float maxval)
{
	options_data *data = find_entry_data(name, FALSE);
	float value = 0;

	if (data == NULL)
		mame_printf_error("Unexpected float option %s queried\n", name);
	else if (data->data == NULL || sscanf(data->data, "%f", &value) != 1)
	{
		if (data->defdata != NULL)
		{
			options_set_string(name, data->defdata);
			value = options_get_float(name);
		}
		if (!data->error_reported)
		{
			mame_printf_error("Illegal float value for %s; reverting to %f\n", data->names[0], (double)value);
			data->error_reported = TRUE;
		}
	}
	else if (value < minval || value > maxval)
	{
		options_set_string(name, data->defdata);
		value = options_get_float(name);
		if (!data->error_reported)
		{
			mame_printf_error("Invalid %s value (must be between %f and %f); reverting to %f\n", data->names[0], (double)minval, (double)maxval, (double)value);
			data->error_reported = TRUE;
		}
	}
	return value;
}


/*-------------------------------------------------
    options_set_string - set a string value
-------------------------------------------------*/

void options_set_string(const char *name, const char *value)
{
	options_data *data = find_entry_data(name, FALSE);
	assert(data != NULL);
	update_data(data, value);
}


/*-------------------------------------------------
    options_set_bool - set a boolean value
-------------------------------------------------*/

void options_set_bool(const char *name, int value)
{
	char temp[4];
	assert(value == TRUE || value == FALSE);
	sprintf(temp, "%d", value);
	options_set_string(name, temp);
}


/*-------------------------------------------------
    options_set_int - set an integer value
-------------------------------------------------*/

void options_set_int(const char *name, int value)
{
	char temp[20];
	sprintf(temp, "%d", value);
	options_set_string(name, temp);
}


/*-------------------------------------------------
    options_set_float - set a float value
-------------------------------------------------*/

void options_set_float(const char *name, float value)
{
	char temp[100];
	sprintf(temp, "%f", value);
	options_set_string(name, temp);
}


/*-------------------------------------------------
    find_entry_data - locate an entry whose name
    matches the given string
-------------------------------------------------*/

static options_data *find_entry_data(const char *string, int is_command_line)
{
	options_data *data;
	int has_no_prefix;

	assert_always(datalist_nextptr != NULL, "Missing call to options_init()!");

	/* determine up front if we should look for "no" boolean options */
	has_no_prefix = (is_command_line && string[0] == 'n' && string[1] == 'o');

	/* scan all entries */
	for (data = datalist; data != NULL; data = data->next)
		if (!(data->flags & OPTION_HEADER))
		{
			const char *compareme = (has_no_prefix && (data->flags & OPTION_BOOLEAN)) ? &string[2] : &string[0];
			int namenum;

			/* loop over names */
			for (namenum = 0; namenum < ARRAY_LENGTH(data->names); namenum++)
				if (data->names[namenum] != NULL && strcmp(compareme, data->names[namenum]) == 0)
					return data;
		}

	/* didn't find it at all */
	return NULL;
}


/*-------------------------------------------------
    update_data - update the data value for a
    given entry
-------------------------------------------------*/

static void update_data(options_data *data, const char *newdata)
{
	const char *dataend = newdata + strlen(newdata) - 1;
	const char *datastart = newdata;

	/* strip off leading/trailing spaces */
	while (isspace(*datastart) && datastart <= dataend)
		datastart++;
	while (isspace(*dataend) && datastart <= dataend)
		dataend--;

	/* strip off quotes */
	if (datastart != dataend && *datastart == '"' && *dataend == '"')
		datastart++, dataend--;

	/* allocate a copy of the data */
	if (data->data)
		free((void *)data->data);
	data->data = copy_string(datastart, dataend + 1);

	/* bump the seqid and clear the error reporting */
	data->seqid++;
	data->error_reported = FALSE;
}
