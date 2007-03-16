/***************************************************************************

    options.h

    Options file and command line management.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#pragma once

#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#include "mamecore.h"



/***************************************************************************
    CONSTANTS
***************************************************************************/

#define OPTION_BOOLEAN				0x0001			/* option is a boolean value */
#define OPTION_DEPRECATED			0x0002			/* option is deprecated */
#define OPTION_COMMAND				0x0004			/* option is a command */
#define OPTION_HEADER				0x0008			/* text-only header */
#define OPTION_INTERNAL				0x0010			/* option is internal-only */
#define OPTION_REPEATS				0x0020			/* unadorned option repeats */

/* unadorned option names */
#define MAX_UNADORNED_OPTIONS		16
#define OPTION_UNADORNED(x)			(((x) < MAX_UNADORNED_OPTIONS) ? option_unadorned[x] : "")

/* core options */
#define OPTION_GAMENAME				OPTION_UNADORNED(0)

/* core search path options */
#define OPTION_ROMPATH				"rompath"
#ifdef MESS
#define OPTION_HASHPATH				"hashpath"
#endif
#define OPTION_SAMPLEPATH			"samplepath"
#define OPTION_ARTPATH				"artpath"
#define OPTION_CTRLRPATH			"ctrlrpath"
#define OPTION_INIPATH				"inipath"
#define OPTION_FONTPATH				"fontpath"

/* core directory options */
#define OPTION_CFG_DIRECTORY		"cfg_directory"
#define OPTION_NVRAM_DIRECTORY		"nvram_directory"
#define OPTION_MEMCARD_DIRECTORY	"memcard_directory"
#define OPTION_INPUT_DIRECTORY		"input_directory"
#define OPTION_STATE_DIRECTORY		"state_directory"
#define OPTION_SNAPSHOT_DIRECTORY	"snapshot_directory"
#define OPTION_DIFF_DIRECTORY		"diff_directory"
#define OPTION_COMMENT_DIRECTORY	"comment_directory"

/* core filename options */
#define OPTION_CHEAT_FILE			"cheat_file"

/* core state/playback options */
#define OPTION_STATE				"state"
#define OPTION_AUTOSAVE				"autosave"
#define OPTION_PLAYBACK				"playback"
#define OPTION_RECORD				"record"
#define OPTION_MNGWRITE				"mngwrite"
#define OPTION_WAVWRITE				"wavwrite"

/* core performance options */
#define OPTION_AUTOFRAMESKIP		"autoframeskip"
#define OPTION_FRAMESKIP			"frameskip"
#define OPTION_SECONDS_TO_RUN		"seconds_to_run"
#define OPTION_THROTTLE				"throttle"
#define OPTION_SLEEP				"sleep"

/* core rotation options */
#define OPTION_ROTATE				"rotate"
#define OPTION_ROR					"ror"
#define OPTION_ROL					"rol"
#define OPTION_AUTOROR				"autoror"
#define OPTION_AUTOROL				"autorol"
#define OPTION_FLIPX				"flipx"
#define OPTION_FLIPY				"flipy"

/* core artwork options */
#define OPTION_ARTWORK_CROP			"artwork_crop"
#define OPTION_USE_BACKDROPS		"use_backdrops"
#define OPTION_USE_OVERLAYS			"use_overlays"
#define OPTION_USE_BEZELS			"use_bezels"

/* core screen options */
#define OPTION_BRIGHTNESS			"brightness"
#define OPTION_CONTRAST				"contrast"
#define OPTION_GAMMA				"gamma"
#define OPTION_PAUSE_BRIGHTNESS		"pause_brightness"

/* core vector options */
#define OPTION_ANTIALIAS			"antialias"
#define OPTION_BEAM					"beam"
#define OPTION_FLICKER				"flicker"

/* core sound options */
#define OPTION_SOUND				"sound"
#define OPTION_SAMPLERATE			"samplerate"
#define OPTION_SAMPLES				"samples"
#define OPTION_VOLUME				"volume"

/* core input options */
#define OPTION_CTRLR				"ctrlr"

/* core debugging options */
#define OPTION_LOG					"log"
#define OPTION_DEBUG				"debug"
#define OPTION_DEBUGSCRIPT			"debugscript"

/* core misc options */
#define OPTION_BIOS					"bios"
#define OPTION_CHEAT				"cheat"
#define OPTION_SKIP_GAMEINFO		"skip_gameinfo"



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef int (*options_parser)(const char *arg, const char *valid, void *resultdata);


typedef struct _options_entry options_entry;
struct _options_entry
{
	const char *		name;				/* name on the command line */
	const char *		defvalue;			/* default value of this argument */
	UINT32				flags;				/* flags to describe the option */
	const char *		description;		/* description for -showusage */
};



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

extern const char *option_unadorned[MAX_UNADORNED_OPTIONS];



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

void options_init(const options_entry *entrylist);
void options_add_entries(const options_entry *entrylist);
void options_set_option_default_value(const char *name, const char *defvalue);
void options_set_option_callback(const char *name, void (*callback)(const char *arg));
void options_free_entries(void);

int options_parse_command_line(int argc, char **argv);
int options_parse_ini_file(mame_file *inifile);

void options_output_ini_file(FILE *inifile);
void options_output_ini_mame_file(mame_file *inifile);

void options_output_help(void);

const char *options_get_string(const char *name);
UINT32 options_get_seqid(const char *name);
int options_get_bool(const char *name);
int options_get_int(const char *name);
float options_get_float(const char *name);
int options_get_int_range(const char *name, int minval, int maxval);
float options_get_float_range(const char *name, float minval, float maxval);

void options_set_string(const char *name, const char *value);
void options_set_bool(const char *name, int value);
void options_set_int(const char *name, int value);
void options_set_float(const char *name, float value);

#endif	/* __OPTIONS_H__ */
