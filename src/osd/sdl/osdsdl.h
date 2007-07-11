#ifndef _osdsdl_h_
#define _osdsdl_h_

/* Notes

	- changed all [f]printf to mame_printf_verbose|error|warning
	- removed obsolete frameskipping code
	- removed obsolete throttle code
	- removed fastforward
	- removed framestorun
	- introduced SDLOPTION constants for a number of options
	- add more verbose info for YUV 

	- removed dirty.h
	- removed ticker.h

	- remove win_trying_to_quit
	- changed win_use_mouse to static use_mouse
	- removed win_key_trans_table
	- removed keyboard typematic definitions from input.h
	- made sdl_monitor_list static
	- removed hwstretch (sdl_video_config)
	- removed syncrefresh (sdl_video_config)
	- removed triplebuf (sdl_video_config)
	- removed sdl_has_menu
	- fixed memory_leak (window.c)
	
	- moved prototypes from drawsdl.c to window.h 
	- removed joystick calibration code 
	- "#if 0" code which is unreachable  
	- "#if 0" code which is never used 
	- moved pick_best_mode to window.c	
	- removed pause_brightness option
	- added more SDLOPTION_ defines
*/

//============================================================
//	Defines
//============================================================

#define SDLOPTION_AUDIO_LATENCY			"audio_latency"
#define SDLOPTION_FMT_SCREEN			"screen%d"
#define SDLOPTION_FMT_ASPECT			"aspect%d"
#define SDLOPTION_ASPECT				"aspect"
#define SDLOPTION_SDLVIDEOFPS			"sdlvideofps"
#define SDLOPTION_KEEPASPECT			"keepaspect"
#define SDLOPTION_WINDOW				"window"
#define SDLOPTION_NUMSCREENS			"numscreens"
#define SDLOPTION_UNEVENSTRETCH			"unevenstretch"
#define SDLOPTION_USEALLHEADS			"useallheads"
#define SDLOPTION_EFFECT				"effect"
#define SDLOPTION_VIDEO					"video"
#define SDLOPTION_SWITCHRES				"switchres"
#define SDLOPTION_FILTER				"filter"
#define SDLOPTION_CENTERH				"centerh"
#define SDLOPTION_CENTERV				"centerv"
#define SDLOPTION_PRESCALE				"prescale"
#define SDLOPTION_PRESCALE_EFFECT		"prescale_effect"
#define SDLOPTION_YUVMODE				"yuvmode"
#define SDLOPTION_MULTITHREADING		"multithreading"
#define SDLOPTION_WAITVSYNC				"waitvsync"
#define SDLOPTION_KEYMAP				"keymap"
#define SDLOPTION_KEYMAP_FILE			"keymap_file"
#define SDLOPTION_JOYMAP				"joymap"
#define SDLOPTION_JOYMAP_FILE			"joymap_file"
#define SDLOPTION_UIMODEKEY				"uimodekey"
#define SDLOPTION_MOUSE					"mouse"
#define SDLOPTION_JOYSTICK				"joystick"
#define SDLOPTION_STEADYKEY				"steadykey"
#define SDLOPTION_A2D_DEADZONE			"a2d_deadzone"

//============================================================
//	sound.c
//============================================================

void sdl_init_audio(running_machine *machine);
void sdl_callback(void *userdata, Uint8 *stream, int len);

//============================================================
// sdltime.c
//============================================================
extern int sdl_use_rdtsc;

//============================================================
// keybled.c
//============================================================

void led_init(void);


#endif
