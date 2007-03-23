/***************************************************************************

    emuopts.h

    Options file and command line management.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#pragma once

#ifndef __EMUOPTS_H__
#define __EMUOPTS_H__

#include "mamecore.h"
#include "options.h"



/***************************************************************************
    CONSTANTS
***************************************************************************/

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
    GLOBALS
***************************************************************************/

extern const options_entry mame_core_options[];


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

void mame_options_init(const options_entry *entries);
core_options *mame_options(void);

#endif	/* __EMUOPTS_H__ */
