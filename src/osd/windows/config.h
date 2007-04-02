//============================================================
//
//  config.h - Win32 configuration routines
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

#ifndef _WIN_CONFIG__
#define _WIN_CONFIG__


//============================================================
//  CONSTANTS
//============================================================

// video options
#define WINOPTION_D3DVERSION			"d3dversion"
#define WINOPTION_VIDEO					"video"
#define WINOPTION_PRESCALE				"prescale"
#define WINOPTION_NUMSCREENS			"numscreens"
#define WINOPTION_WAITVSYNC				"waitvsync"
#define WINOPTION_TRIPLEBUFFER			"triplebuffer"
#define WINOPTION_HWSTRETCH				"hwstretch"
#define WINOPTION_SWITCHRES				"switchres"
#define WINOPTION_KEEPASPECT			"keepaspect"
#define WINOPTION_SYNCREFRESH			"syncrefresh"
#define WINOPTION_FULLSCREENGAMMA		"full_screen_gamma"
#define WINOPTION_FULLSCREENBRIGHTNESS	"full_screen_brightness"
#define WINOPTION_FULLLSCREENCONTRAST	"full_screen_contrast"
#define WINOPTION_EFFECT				"effect"
#define WINOPTION_ASPECT				"aspect"
#define WINOPTION_RESOLUTION			"resolution"
#define WINOPTION_RESOLUTION0			"resolution0"
#define WINOPTION_RESOLUTION1			"resolution1"
#define WINOPTION_RESOLUTION2			"resolution2"
#define WINOPTION_RESOLUTION3			"resolution3"
#define WINOPTION_FILTER				"filter"
#define WINOPTION_SCREEN				"screen"

// window options
#define WINOPTION_VIEW					"view"
#define WINOPTION_MAXIMIZE				"maximize"
#define WINOPTION_WINDOW				"window"

// input options
#define WINOPTION_CTRLR					"ctrlr"
#define WINOPTION_MOUSE					"mouse"
#define WINOPTION_JOYSTICK				"joystick"
#define WINOPTION_JOY_DEADZONE			"joy_deadzone"
#define WINOPTION_JOY_SATURATION		"joy_saturation"
#define WINOPTION_STEADYKEY				"steadykey"
#define WINOPTION_LIGHTGUN				"lightgun"
#define WINOPTION_DUAL_LIGHTGUN			"dual_lightgun"
#define WINOPTION_OFFSCREEN_RELOAD		"offscreen_reload"
#define WINOPTION_DIGITAL				"digital"
#define WINOPTION_PADDLE_DEVICE			"paddle_device"
#define WINOPTION_ADSTICK_DEVICE		"adstick_device"
#define WINOPTION_PEDAL_DEVICE			"pedal_device"
#define WINOPTION_DIAL_DEVICE			"dial_device"
#define WINOPTION_TRACKBALL_DEVICE		"trackball_device"
#define WINOPTION_LIGHTGUN_DEVICE		"lightgun_device"
#define WINOPTION_POSITIONAL_DEVICE		"positional_device"
#define WINOPTION_MOUSE_DEVICE			"mouse_device"

// misc options
#define WINOPTION_AUDIO_LATENCY			"audio_latency"
#define WINOPTION_PRIORITY				"priority"
#define WINOPTION_MULTITHREADING		"multithreading"
#define WINOPTION_VERBOSE				"verbose"
#define WINOPTION_OSLOG					"oslog"

// cli options
#define CLIOPTION_CREATECONFIG			"createconfig"
#define CLIOPTION_READCONFIG			"readconfig"
#define CLIOPTION_SHOWCONFIG			"showconfig"
#define CLIOPTION_SHOWUSAGE				"showusage"
#define CLIOPTION_VALIDATE				"validate"
#define CLIOPTION_HELP					"help"
#define CLIOPTION_LISTXML				"listxml"
#define CLIOPTION_LISTFULL				"listfull"
#define CLIOPTION_LISTSOURCE			"listsource"
#define CLIOPTION_LISTCLONES			"listclones"
#define CLIOPTION_LISTCRC				"listcrc"
#define CLIOPTION_LISTDEVICES			"listdevices"
#define CLIOPTION_LISTROMS				"listroms"
#define CLIOPTION_LISTSAMPLES			"listsamples"
#define CLIOPTION_VERIFYROMS			"verifyroms"
#define CLIOPTION_VERIFYSAMPLES			"verifysamples"
#define CLIOPTION_ROMIDENT				"romident"
#define CLIOPTION_ISKNOWN				"isknown"



//============================================================
//  GLOBALS
//============================================================

extern const options_entry mame_win_options[];


//============================================================
//  PROTOTYPES
//============================================================

// Initializes Win32 MAME options
void win_options_init(void);

// Initializes and exits the configuration system
int  cli_frontend_init (int argc, char **argv);
void cli_frontend_exit (void);

#endif // _WIN_CONFIG__
