/***************************************************************************

    options.h

    Core options code code

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#include "osdcore.h"
#include "corefile.h"



/***************************************************************************
    CONSTANTS
***************************************************************************/

/* unadorned option names */
#define MAX_UNADORNED_OPTIONS		16
#define OPTION_UNADORNED(x)			(((x) < MAX_UNADORNED_OPTIONS) ? option_unadorned[x] : "")

#define OPTION_BOOLEAN				0x0001			/* option is a boolean value */
#define OPTION_DEPRECATED			0x0002			/* option is deprecated */
#define OPTION_COMMAND				0x0004			/* option is a command */
#define OPTION_HEADER				0x0008			/* text-only header */
#define OPTION_INTERNAL				0x0010			/* option is internal-only */
#define OPTION_REPEATS				0x0020			/* unadorned option repeats */



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _core_options core_options;

typedef struct _options_entry options_entry;
struct _options_entry
{
	const char *		name;				/* name on the command line */
	const char *		defvalue;			/* default value of this argument */
	UINT32				flags;				/* flags to describe the option */
	const char *		description;		/* description for -showusage */
};

typedef enum _options_message options_message;
enum _options_message
{
	OPTMSG_INFO,
	OPTMSG_WARNING,
	OPTMSG_ERROR,
	OPTMSG_COUNT
};



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

extern const char *option_unadorned[MAX_UNADORNED_OPTIONS];



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

core_options *options_create(void (*fail)(const char *message));
void options_free(core_options *opts);

void options_set_output_callback(core_options *opts, options_message msgtype, void (*callback)(const char *s));

int options_add_entries(core_options *opts, const options_entry *entrylist);
int options_set_option_default_value(core_options *opts, const char *name, const char *defvalue);
int options_set_option_callback(core_options *opts, const char *name, void (*callback)(core_options *opts, const char *arg));

int options_parse_command_line(core_options *opts, int argc, char **argv);
int options_parse_ini_file(core_options *opts, core_file *inifile);
void options_output_ini_file(core_options *opts, core_file *inifile);
void options_output_ini_stdfile(core_options *opts, FILE *inifile);
void options_output_help(core_options *opts, void (*output)(const char *s));

int options_copy(core_options *dest_opts, core_options *src_opts);
int options_equal(core_options *opts1, core_options *opts2);

const char *options_get_string(core_options *opts, const char *name);
int options_get_bool(core_options *opts, const char *name);
int options_get_int(core_options *opts, const char *name);
float options_get_float(core_options *opts, const char *name);
int options_get_int_range(core_options *opts, const char *name, int minval, int maxval);
float options_get_float_range(core_options *opts, const char *name, float minval, float maxval);
UINT32 options_get_seqid(core_options *opts, const char *name);

void options_set_string(core_options *opts, const char *name, const char *value);
void options_set_bool(core_options *opts, const char *name, int value);
void options_set_int(core_options *opts, const char *name, int value);
void options_set_float(core_options *opts, const char *name, float value);

#endif /* __OPTIONS_H__ */
