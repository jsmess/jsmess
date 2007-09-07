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

	- renamed void yuv_lookup_init to drawsdl_yuv_init (global namespace)
	- rmoved some obsolete code
	- add SDL1.3 compatibility 
	
	- fixed some compile issues
	- moved clear_surface into window thread
	- got SDL1.3 -mt working - still crashing on exit
	
	- removed "digital" option
	- removed device selection options
	- added more SDLOPTION defines
*/

//============================================================
//	Defines
//============================================================

#define SDLOPTION_AUDIO_LATENCY			"audio_latency"
#define SDLOPTION_SCREEN(x)				"screen" x
#define SDLOPTION_ASPECT(x)				"aspect" x
#define SDLOPTION_RESOLUTION(x)			"resolution" x
#define SDLOPTION_VIEW(x)				"view" x 
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
#define SDLOPTION_JOYMAP				"remapjoys"
#define SDLOPTION_JOYMAP_FILE			"remapjoyfile"
#define SDLOPTION_UIMODEKEY				"uimodekey"

#define SDLOPTION_SHADER_MAME(x)		"glsl_shader_mame" x
#define SDLOPTION_SHADER_SCREEN(x)		"glsl_shader_screen" x
#define SDLOPTION_GLSL_FILTER			"gl_glsl_filter"
#define SDLOPTION_GL_GLSL				"gl_glsl"
#define SDLOPTION_GL_PBO				"gl_pbo"
#define SDLOPTION_GL_VBO				"gl_vbo"
#define SDLOPTION_GL_NOTEXTURERECT		"gl_notexturerect"
#define SDLOPTION_GL_FORCEPOW2TEXTURE	"gl_forcepow2texture"
#define SDLOPTION_GL_GLSL_VID_ATTR		"gl_glsl_vid_attr"

//============================================================
//	sound.c
//============================================================

void sdl_init_audio(running_machine *machine);
void sdl_callback(void *userdata, Uint8 *stream, int len);

//============================================================
// keybled.c
//============================================================

void led_init(void);


#endif
