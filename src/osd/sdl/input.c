//============================================================
//
//  input.c - SDL implementation of MAME input routines
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//  SDLMAME by Olivier Galibert and R. Belmont
//
//============================================================

// standard sdl header
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#include <ctype.h>
#include <stddef.h>

// MAME headers
#include "osdepend.h"
#include "driver.h"
#include "ui.h"

// MAMEOS headers
#include "window.h"
#include "input.h"
#include "debugwin.h"
#include "video.h"
#include "config.h"
#include "osdsdl.h"

#ifdef MESS
#include "uimess.h"
#include "inputx.h"
#endif

//============================================================
//  PARAMETERS
//============================================================

enum
{
	POVDIR_LEFT = 0,
	POVDIR_RIGHT,
	POVDIR_UP,
	POVDIR_DOWN
};

#define MAX_KEYS			256
#define MAX_AXES			8
#define MAX_BUTTONS			32
#define MAX_HATS			8
#define MAX_POV				4
#define MAX_JOYSTICKS			16
#define MAX_JOYMAP 		(MAX_JOYSTICKS*2)

#define MAME_KEY			0
#define SDL_KEY				1
#define VIRTUAL_KEY			2
#define ASCII_KEY			3

//============================================================
//  MACROS
//============================================================


//============================================================
//  TYPEDEFS
//============================================================

// state information for a keyboard
typedef struct _keyboard_state keyboard_state;
struct _keyboard_state
{
	INT32	state[0x3ff];                                 	// must be INT32!
	INT8	oldkey[MAX_KEYS];
	INT8	currkey[MAX_KEYS];
};


// state information for a mouse
typedef struct _mouse_state mouse_state;
struct _mouse_state
{
	INT32 lX, lY;
	INT32 buttons[MAX_BUTTONS];
};


// state information for a joystick; DirectInput state must be first element
typedef struct _joystick_state joystick_state;
struct _joystick_state
{
	SDL_Joystick *device[MAX_JOYSTICKS];
	INT32 axes[MAX_AXES];
	INT32 buttons[MAX_BUTTONS];
	INT32 hatsU[MAX_HATS], hatsD[MAX_HATS], hatsL[MAX_HATS], hatsR[MAX_HATS];
};

// generic device information
typedef struct _device_info device_info;
struct _device_info
{
	// device information
	device_info **			head;
	device_info *			next;
	char 	*			name;
	
	// MAME information
	input_device *			device;

	// device state
	union
	{
		keyboard_state		keyboard;
		mouse_state		mouse;
		joystick_state		joystick;
	};
};

//============================================================
//  LOCAL VARIABLES
//============================================================

// global states
static osd_lock *			input_lock;
static UINT8				input_paused;

#ifdef SDLMAME_WIN32
// input buffer
#define MAX_BUF_EVENTS 			(100)
static SDL_Event			event_buf[MAX_BUF_EVENTS];
static int				event_buf_count;
#endif

// keyboard states
static device_info *			keyboard_list;

// mouse states
static UINT8				mouse_enabled;
static device_info *			mouse_list;

// lightgun states
static UINT8				lightgun_enabled;
static device_info *			lightgun_list;

// joystick states
static device_info *			joystick_list;

// joystick mapper
static struct 
{
	int		logical;
	int		ismapped;
	char		*name;
} joy_map[MAX_JOYMAP];

static int joy_logical[MAX_JOYSTICKS];

//============================================================
//  PROTOTYPES
//============================================================

static void sdlinput_pause(running_machine *machine, int paused);
static void sdlinput_exit(running_machine *machine);

// deivce list management
static void device_list_reset_devices(device_info *devlist_head);
static int device_list_count(device_info *devlist_head);
static void device_list_free_devices(device_info *devlist_head);

// generic device management
static device_info *generic_device_alloc(device_info **devlist_head_ptr, const char *name);
static void generic_device_free(device_info *devinfo);
static int generic_device_index(device_info *devlist_head, device_info *devinfo);
static void generic_device_reset(device_info *devinfo);
static INT32 generic_button_get_state(void *device_internal, void *item_internal);
static INT32 generic_axis_get_state(void *device_internal, void *item_internal);
static device_info *generic_device_find_index(device_info *devlist_head, int index);

// Win32-specific input code
static void sdl_init(running_machine *machine);
static void sdl_exit(running_machine *machine);
static void sdl_keyboard_poll(device_info *devinfo);
static void sdl_lightgun_poll(device_info *devinfo);

static const char *default_button_name(int which);
static const char *default_pov_name(int which);



//============================================================
//  KEYBOARD/JOYSTICK LIST
//============================================================

// master keyboard translation table
static const int sdl_key_trans_table[][4] =
{
	// MAME key			SDL key			vkey	ascii
	{ ITEM_ID_ESC, 			SDLK_ESCAPE,	     	27,	27 },
	{ ITEM_ID_1, 			SDLK_1,		     	'1',	'1' },
	{ ITEM_ID_2, 			SDLK_2,		     	'2',	'2' },
	{ ITEM_ID_3, 			SDLK_3,		     	'3',	'3' },
	{ ITEM_ID_4, 			SDLK_4,		     	'4',	'4' },
	{ ITEM_ID_5, 			SDLK_5,		     	'5',	'5' },
	{ ITEM_ID_6, 			SDLK_6,		     	'6',	'6' },
	{ ITEM_ID_7, 			SDLK_7,		     	'7',	'7' },
	{ ITEM_ID_8, 			SDLK_8,		     	'8',	'8' },
	{ ITEM_ID_9, 			SDLK_9,		     	'9',	'9' },
	{ ITEM_ID_0, 			SDLK_0,		     	'0',	'0' },
	{ ITEM_ID_MINUS, 		SDLK_MINUS, 	     	0xbd,	'-' },
	{ ITEM_ID_EQUALS, 		SDLK_EQUALS,	      	0xbb,	'=' },
	{ ITEM_ID_BACKSPACE,		SDLK_BACKSPACE,         8, 	8 },
	{ ITEM_ID_TAB, 			SDLK_TAB, 	     	9, 	9 },
	{ ITEM_ID_Q, 			SDLK_q,		     	'Q',	'Q' },
	{ ITEM_ID_W, 			SDLK_w,		     	'W',	'W' },
	{ ITEM_ID_E, 			SDLK_e,		     	'E',	'E' },
	{ ITEM_ID_R, 			SDLK_r,		     	'R',	'R' },
	{ ITEM_ID_T, 			SDLK_t,		     	'T',	'T' },
	{ ITEM_ID_Y, 			SDLK_y,		     	'Y',	'Y' },
	{ ITEM_ID_U, 			SDLK_u,		     	'U',	'U' },
	{ ITEM_ID_I, 			SDLK_i,		     	'I',	'I' },
	{ ITEM_ID_O, 			SDLK_o,		     	'O',	'O' },
	{ ITEM_ID_P, 			SDLK_p,		     	'P',	'P' },
	{ ITEM_ID_OPENBRACE,		SDLK_LEFTBRACKET, 	0xdb,	'[' },
	{ ITEM_ID_CLOSEBRACE,		SDLK_RIGHTBRACKET, 	0xdd,	']' },
	{ ITEM_ID_ENTER, 		SDLK_RETURN, 		13, 	13 },
	{ ITEM_ID_LCONTROL, 		SDLK_LCTRL, 	        0, 	0 },
	{ ITEM_ID_A, 			SDLK_a,		     	'A',	'A' },
	{ ITEM_ID_S, 			SDLK_s,		     	'S',	'S' },
	{ ITEM_ID_D, 			SDLK_d,		     	'D',	'D' },
	{ ITEM_ID_F, 			SDLK_f,		     	'F',	'F' },
	{ ITEM_ID_G, 			SDLK_g,		     	'G',	'G' },
	{ ITEM_ID_H, 			SDLK_h,		     	'H',	'H' },
	{ ITEM_ID_J, 			SDLK_j,		     	'J',	'J' },
	{ ITEM_ID_K, 			SDLK_k,		     	'K',	'K' },
	{ ITEM_ID_L, 			SDLK_l,		     	'L',	'L' },
	{ ITEM_ID_COLON, 		SDLK_SEMICOLON,		0xba,	';' },
	{ ITEM_ID_QUOTE, 		SDLK_QUOTE,		0xde,	'\'' },
	{ ITEM_ID_TILDE, 		SDLK_BACKQUOTE,      	0xc0,	'`' },
	{ ITEM_ID_LSHIFT, 		SDLK_LSHIFT, 		0, 	0 },
	{ ITEM_ID_BACKSLASH,		SDLK_BACKSLASH, 	0xdc,	'\\' },
	{ ITEM_ID_Z, 			SDLK_z,		     	'Z',	'Z' },
	{ ITEM_ID_X, 			SDLK_x,		     	'X',	'X' },
	{ ITEM_ID_C, 			SDLK_c,		     	'C',	'C' },
	{ ITEM_ID_V, 			SDLK_v,		     	'V',	'V' },
	{ ITEM_ID_B, 			SDLK_b,		     	'B',	'B' },
	{ ITEM_ID_N, 			SDLK_n,		     	'N',	'N' },
	{ ITEM_ID_M, 			SDLK_m,		     	'M',	'M' },
	{ ITEM_ID_COMMA, 		SDLK_COMMA,	     	0xbc,	',' },
	{ ITEM_ID_STOP, 		SDLK_PERIOD, 		0xbe,	'.' },
	{ ITEM_ID_SLASH, 		SDLK_SLASH, 	     	0xbf,	'/' },
	{ ITEM_ID_RSHIFT, 		SDLK_RSHIFT, 		0, 	0 },
	{ ITEM_ID_ASTERISK, 		SDLK_KP_MULTIPLY,    	'*',	'*' },
	{ ITEM_ID_LALT, 		SDLK_LALT, 		0, 	0 },
	{ ITEM_ID_SPACE, 		SDLK_SPACE, 		' ',	' ' },
	{ ITEM_ID_CAPSLOCK, 		SDLK_CAPSLOCK, 	     	0, 	0 },
	{ ITEM_ID_F1, 			SDLK_F1,		0, 	0 },
	{ ITEM_ID_F2, 			SDLK_F2,		0, 	0 },
	{ ITEM_ID_F3, 			SDLK_F3,		0, 	0 },
	{ ITEM_ID_F4, 			SDLK_F4,		0, 	0 },
	{ ITEM_ID_F5, 			SDLK_F5,		0, 	0 },
	{ ITEM_ID_F6, 			SDLK_F6,		0, 	0 },
	{ ITEM_ID_F7, 			SDLK_F7,		0, 	0 },
	{ ITEM_ID_F8, 			SDLK_F8,		0, 	0 },
	{ ITEM_ID_F9, 			SDLK_F9,	    	0, 	0 },
	{ ITEM_ID_F10, 			SDLK_F10,		0, 	0 },
	{ ITEM_ID_NUMLOCK, 		SDLK_NUMLOCK,		0, 	0 },
	{ ITEM_ID_SCRLOCK, 		SDLK_SCROLLOCK,		0, 	0 },
	{ ITEM_ID_7_PAD, 		SDLK_KP7,		0, 	0 },
	{ ITEM_ID_8_PAD, 		SDLK_KP8,		0, 	0 },
	{ ITEM_ID_9_PAD, 		SDLK_KP9,		0, 	0 },
	{ ITEM_ID_MINUS_PAD,		SDLK_KP_MINUS,	   	0, 	0 },
	{ ITEM_ID_4_PAD, 		SDLK_KP4,		0, 	0 },
	{ ITEM_ID_5_PAD, 		SDLK_KP5,		0, 	0 },
	{ ITEM_ID_6_PAD, 		SDLK_KP6,		0, 	0 },
	{ ITEM_ID_PLUS_PAD, 		SDLK_KP_PLUS,	   	0, 	0 },
	{ ITEM_ID_1_PAD, 		SDLK_KP1,		0, 	0 },
	{ ITEM_ID_2_PAD, 		SDLK_KP2,		0, 	0 },
	{ ITEM_ID_3_PAD, 		SDLK_KP3,		0, 	0 },
	{ ITEM_ID_0_PAD, 		SDLK_KP0,		0, 	0 },
	{ ITEM_ID_DEL_PAD, 		SDLK_KP_PERIOD,		0, 	0 },
	{ ITEM_ID_F11, 			SDLK_F11,		0,     	0 },
	{ ITEM_ID_F12, 			SDLK_F12,		0,     	0 },
	{ ITEM_ID_F13, 			SDLK_F13,		0,     	0 },
	{ ITEM_ID_F14, 			SDLK_F14,		0,     	0 },
	{ ITEM_ID_F15, 			SDLK_F15,		0,     	0 },
	{ ITEM_ID_ENTER_PAD,		SDLK_KP_ENTER,		0, 	0 },
	{ ITEM_ID_RCONTROL, 		SDLK_RCTRL,		0, 	0 },
	{ ITEM_ID_SLASH_PAD,		SDLK_KP_DIVIDE,		0, 	0 },
	{ ITEM_ID_PRTSCR, 		SDLK_PRINT, 		0, 	0 },
	{ ITEM_ID_RALT, 		SDLK_RALT,		0,     	0 },
	{ ITEM_ID_HOME, 		SDLK_HOME,		0,     	0 },
	{ ITEM_ID_UP, 			SDLK_UP,		0,     	0 },
	{ ITEM_ID_PGUP, 		SDLK_PAGEUP,		0,     	0 },
	{ ITEM_ID_LEFT, 		SDLK_LEFT,		0,     	0 },
	{ ITEM_ID_RIGHT, 		SDLK_RIGHT,		0,     	0 },
	{ ITEM_ID_END, 			SDLK_END,		0,     	0 },
	{ ITEM_ID_DOWN, 		SDLK_DOWN,		0,     	0 },
	{ ITEM_ID_PGDN, 		SDLK_PAGEDOWN,		0,     	0 },
	{ ITEM_ID_INSERT, 		SDLK_INSERT,		0, 	0 },
	{ ITEM_ID_DEL, 			SDLK_DELETE,		0, 	0 },
	{ ITEM_ID_LWIN, 		SDLK_LSUPER,		0,     	0 },
	{ ITEM_ID_RWIN, 		SDLK_RSUPER,		0,     	0 },
	{ ITEM_ID_MENU, 		SDLK_MENU,		0,     	0 },
	{ ITEM_ID_BACKSLASH2,           SDLK_HASH,              0xdc,   '\\' },
};

typedef struct _code_string_table code_string_table;
struct _code_string_table
{
	UINT32					code;
	const char *			string;
};

static const code_string_table id_to_name[] =
{
	/* standard keyboard codes */
	{ ITEM_ID_A,             "A" },
	{ ITEM_ID_B,             "B" },
	{ ITEM_ID_C,             "C" },
	{ ITEM_ID_D,             "D" },
	{ ITEM_ID_E,             "E" },
	{ ITEM_ID_F,             "F" },
	{ ITEM_ID_G,             "G" },
	{ ITEM_ID_H,             "H" },
	{ ITEM_ID_I,             "I" },
	{ ITEM_ID_J,             "J" },
	{ ITEM_ID_K,             "K" },
	{ ITEM_ID_L,             "L" },
	{ ITEM_ID_M,             "M" },
	{ ITEM_ID_N,             "N" },
	{ ITEM_ID_O,             "O" },
	{ ITEM_ID_P,             "P" },
	{ ITEM_ID_Q,             "Q" },
	{ ITEM_ID_R,             "R" },
	{ ITEM_ID_S,             "S" },
	{ ITEM_ID_T,             "T" },
	{ ITEM_ID_U,             "U" },
	{ ITEM_ID_V,             "V" },
	{ ITEM_ID_W,             "W" },
	{ ITEM_ID_X,             "X" },
	{ ITEM_ID_Y,             "Y" },
	{ ITEM_ID_Z,             "Z" },
	{ ITEM_ID_0,             "0" },
	{ ITEM_ID_1,             "1" },
	{ ITEM_ID_2,             "2" },
	{ ITEM_ID_3,             "3" },
	{ ITEM_ID_4,             "4" },
	{ ITEM_ID_5,             "5" },
	{ ITEM_ID_6,             "6" },
	{ ITEM_ID_7,             "7" },
	{ ITEM_ID_8,             "8" },
	{ ITEM_ID_9,             "9" },
	{ ITEM_ID_F1,            "F1" },
	{ ITEM_ID_F2,            "F2" },
	{ ITEM_ID_F3,            "F3" },
	{ ITEM_ID_F4,            "F4" },
	{ ITEM_ID_F5,            "F5" },
	{ ITEM_ID_F6,            "F6" },
	{ ITEM_ID_F7,            "F7" },
	{ ITEM_ID_F8,            "F8" },
	{ ITEM_ID_F9,            "F9" },
	{ ITEM_ID_F10,           "F10" },
	{ ITEM_ID_F11,           "F11" },
	{ ITEM_ID_F12,           "F12" },
	{ ITEM_ID_F13,           "F13" },
	{ ITEM_ID_F14,           "F14" },
	{ ITEM_ID_F15,           "F15" },
	{ ITEM_ID_ESC,           "ESC" },
	{ ITEM_ID_TILDE,         "TILDE" },
	{ ITEM_ID_MINUS,         "MINUS" },
	{ ITEM_ID_EQUALS,        "EQUALS" },
	{ ITEM_ID_BACKSPACE,     "BACKSPACE" },
	{ ITEM_ID_TAB,           "TAB" },
	{ ITEM_ID_OPENBRACE,     "OPENBRACE" },
	{ ITEM_ID_CLOSEBRACE,    "CLOSEBRACE" },
	{ ITEM_ID_ENTER,         "ENTER" },
	{ ITEM_ID_COLON,         "COLON" },
	{ ITEM_ID_QUOTE,         "QUOTE" },
	{ ITEM_ID_BACKSLASH,     "BACKSLASH" },
	{ ITEM_ID_BACKSLASH2,    "BACKSLASH2" },
	{ ITEM_ID_COMMA,         "COMMA" },
	{ ITEM_ID_STOP,          "STOP" },
	{ ITEM_ID_SLASH,         "SLASH" },
	{ ITEM_ID_SPACE,         "SPACE" },
	{ ITEM_ID_INSERT,        "INSERT" },
	{ ITEM_ID_DEL,           "DEL" },
	{ ITEM_ID_HOME,          "HOME" },
	{ ITEM_ID_END,           "END" },
	{ ITEM_ID_PGUP,          "PGUP" },
	{ ITEM_ID_PGDN,          "PGDN" },
	{ ITEM_ID_LEFT,          "LEFT" },
	{ ITEM_ID_RIGHT,         "RIGHT" },
	{ ITEM_ID_UP,            "UP" },
	{ ITEM_ID_DOWN,          "DOWN" },
	{ ITEM_ID_0_PAD,         "0_PAD" },
	{ ITEM_ID_1_PAD,         "1_PAD" },
	{ ITEM_ID_2_PAD,         "2_PAD" },
	{ ITEM_ID_3_PAD,         "3_PAD" },
	{ ITEM_ID_4_PAD,         "4_PAD" },
	{ ITEM_ID_5_PAD,         "5_PAD" },
	{ ITEM_ID_6_PAD,         "6_PAD" },
	{ ITEM_ID_7_PAD,         "7_PAD" },
	{ ITEM_ID_8_PAD,         "8_PAD" },
	{ ITEM_ID_9_PAD,         "9_PAD" },
	{ ITEM_ID_SLASH_PAD,     "SLASH_PAD" },
	{ ITEM_ID_ASTERISK,      "ASTERISK" },
	{ ITEM_ID_MINUS_PAD,     "MINUS_PAD" },
	{ ITEM_ID_PLUS_PAD,      "PLUS_PAD" },
	{ ITEM_ID_DEL_PAD,       "DEL_PAD" },
	{ ITEM_ID_ENTER_PAD,     "ENTER_PAD" },
	{ ITEM_ID_PRTSCR,        "PRTSCR" },
	{ ITEM_ID_PAUSE,         "PAUSE" },
	{ ITEM_ID_LSHIFT,        "LSHIFT" },
	{ ITEM_ID_RSHIFT,        "RSHIFT" },
	{ ITEM_ID_LCONTROL,      "LCONTROL" },
	{ ITEM_ID_RCONTROL,      "RCONTROL" },
	{ ITEM_ID_LALT,          "LALT" },
	{ ITEM_ID_RALT,          "RALT" },
	{ ITEM_ID_SCRLOCK,       "SCRLOCK" },
	{ ITEM_ID_NUMLOCK,       "NUMLOCK" },
	{ ITEM_ID_CAPSLOCK,      "CAPSLOCK" },
	{ ITEM_ID_LWIN,          "LWIN" },
	{ ITEM_ID_RWIN,          "RWIN" },
	{ ITEM_ID_MENU,          "MENU" },
	{ ITEM_ID_CANCEL,        "CANCEL" },

	{ ~0,                    NULL }
};

typedef struct _key_lookup_table key_lookup_table;

struct _key_lookup_table 
{
	int code;
	const char *name;
};

#define KE(x) { x, #x }

static key_lookup_table sdl_lookup[] =
{
	KE(SDLK_UNKNOWN),
	KE(SDLK_FIRST),
	KE(SDLK_BACKSPACE),
	KE(SDLK_TAB),
	KE(SDLK_CLEAR),
	KE(SDLK_RETURN),
	KE(SDLK_PAUSE),
	KE(SDLK_ESCAPE),
	KE(SDLK_SPACE),
	KE(SDLK_EXCLAIM),
	KE(SDLK_QUOTEDBL),
	KE(SDLK_HASH),
	KE(SDLK_DOLLAR),
	KE(SDLK_AMPERSAND),
	KE(SDLK_QUOTE),
	KE(SDLK_LEFTPAREN),
	KE(SDLK_RIGHTPAREN),
	KE(SDLK_ASTERISK),
	KE(SDLK_PLUS),
	KE(SDLK_COMMA),
	KE(SDLK_MINUS),
	KE(SDLK_PERIOD),
	KE(SDLK_SLASH),
	KE(SDLK_0),
	KE(SDLK_1),
	KE(SDLK_2),
	KE(SDLK_3),
	KE(SDLK_4),
	KE(SDLK_5),
	KE(SDLK_6),
	KE(SDLK_7),
	KE(SDLK_8),
	KE(SDLK_9),
	KE(SDLK_COLON),
	KE(SDLK_SEMICOLON),
	KE(SDLK_LESS),
	KE(SDLK_EQUALS),
	KE(SDLK_GREATER),
	KE(SDLK_QUESTION),
	KE(SDLK_AT),
	KE(SDLK_LEFTBRACKET),
	KE(SDLK_BACKSLASH),
	KE(SDLK_RIGHTBRACKET),
	KE(SDLK_CARET),
	KE(SDLK_UNDERSCORE),
	KE(SDLK_BACKQUOTE),
	KE(SDLK_a),
	KE(SDLK_b),
	KE(SDLK_c),
	KE(SDLK_d),
	KE(SDLK_e),
	KE(SDLK_f),
	KE(SDLK_g),
	KE(SDLK_h),
	KE(SDLK_i),
	KE(SDLK_j),
	KE(SDLK_k),
	KE(SDLK_l),
	KE(SDLK_m),
	KE(SDLK_n),
	KE(SDLK_o),
	KE(SDLK_p),
	KE(SDLK_q),
	KE(SDLK_r),
	KE(SDLK_s),
	KE(SDLK_t),
	KE(SDLK_u),
	KE(SDLK_v),
	KE(SDLK_w),
	KE(SDLK_x),
	KE(SDLK_y),
	KE(SDLK_z),
	KE(SDLK_DELETE),
	KE(SDLK_WORLD_0),
	KE(SDLK_WORLD_1),
	KE(SDLK_WORLD_2),
	KE(SDLK_WORLD_3),
	KE(SDLK_WORLD_4),
	KE(SDLK_WORLD_5),
	KE(SDLK_WORLD_6),
	KE(SDLK_WORLD_7),
	KE(SDLK_WORLD_8),
	KE(SDLK_WORLD_9),
	KE(SDLK_WORLD_10),
	KE(SDLK_WORLD_11),
	KE(SDLK_WORLD_12),
	KE(SDLK_WORLD_13),
	KE(SDLK_WORLD_14),
	KE(SDLK_WORLD_15),
	KE(SDLK_WORLD_16),
	KE(SDLK_WORLD_17),
	KE(SDLK_WORLD_18),
	KE(SDLK_WORLD_19),
	KE(SDLK_WORLD_20),
	KE(SDLK_WORLD_21),
	KE(SDLK_WORLD_22),
	KE(SDLK_WORLD_23),
	KE(SDLK_WORLD_24),
	KE(SDLK_WORLD_25),
	KE(SDLK_WORLD_26),
	KE(SDLK_WORLD_27),
	KE(SDLK_WORLD_28),
	KE(SDLK_WORLD_29),
	KE(SDLK_WORLD_30),
	KE(SDLK_WORLD_31),
	KE(SDLK_WORLD_32),
	KE(SDLK_WORLD_33),
	KE(SDLK_WORLD_34),
	KE(SDLK_WORLD_35),
	KE(SDLK_WORLD_36),
	KE(SDLK_WORLD_37),
	KE(SDLK_WORLD_38),
	KE(SDLK_WORLD_39),
	KE(SDLK_WORLD_40),
	KE(SDLK_WORLD_41),
	KE(SDLK_WORLD_42),
	KE(SDLK_WORLD_43),
	KE(SDLK_WORLD_44),
	KE(SDLK_WORLD_45),
	KE(SDLK_WORLD_46),
	KE(SDLK_WORLD_47),
	KE(SDLK_WORLD_48),
	KE(SDLK_WORLD_49),
	KE(SDLK_WORLD_50),
	KE(SDLK_WORLD_51),
	KE(SDLK_WORLD_52),
	KE(SDLK_WORLD_53),
	KE(SDLK_WORLD_54),
	KE(SDLK_WORLD_55),
	KE(SDLK_WORLD_56),
	KE(SDLK_WORLD_57),
	KE(SDLK_WORLD_58),
	KE(SDLK_WORLD_59),
	KE(SDLK_WORLD_60),
	KE(SDLK_WORLD_61),
	KE(SDLK_WORLD_62),
	KE(SDLK_WORLD_63),
	KE(SDLK_WORLD_64),
	KE(SDLK_WORLD_65),
	KE(SDLK_WORLD_66),
	KE(SDLK_WORLD_67),
	KE(SDLK_WORLD_68),
	KE(SDLK_WORLD_69),
	KE(SDLK_WORLD_70),
	KE(SDLK_WORLD_71),
	KE(SDLK_WORLD_72),
	KE(SDLK_WORLD_73),
	KE(SDLK_WORLD_74),
	KE(SDLK_WORLD_75),
	KE(SDLK_WORLD_76),
	KE(SDLK_WORLD_77),
	KE(SDLK_WORLD_78),
	KE(SDLK_WORLD_79),
	KE(SDLK_WORLD_80),
	KE(SDLK_WORLD_81),
	KE(SDLK_WORLD_82),
	KE(SDLK_WORLD_83),
	KE(SDLK_WORLD_84),
	KE(SDLK_WORLD_85),
	KE(SDLK_WORLD_86),
	KE(SDLK_WORLD_87),
	KE(SDLK_WORLD_88),
	KE(SDLK_WORLD_89),
	KE(SDLK_WORLD_90),
	KE(SDLK_WORLD_91),
	KE(SDLK_WORLD_92),
	KE(SDLK_WORLD_93),
	KE(SDLK_WORLD_94),
	KE(SDLK_WORLD_95),
	KE(SDLK_KP0),
	KE(SDLK_KP1),
	KE(SDLK_KP2),
	KE(SDLK_KP3),
	KE(SDLK_KP4),
	KE(SDLK_KP5),
	KE(SDLK_KP6),
	KE(SDLK_KP7),
	KE(SDLK_KP8),
	KE(SDLK_KP9),
	KE(SDLK_KP_PERIOD),
	KE(SDLK_KP_DIVIDE),
	KE(SDLK_KP_MULTIPLY),
	KE(SDLK_KP_MINUS),
	KE(SDLK_KP_PLUS),
	KE(SDLK_KP_ENTER),
	KE(SDLK_KP_EQUALS),
	KE(SDLK_UP),
	KE(SDLK_DOWN),
	KE(SDLK_RIGHT),
	KE(SDLK_LEFT),
	KE(SDLK_INSERT),
	KE(SDLK_HOME),
	KE(SDLK_END),
	KE(SDLK_PAGEUP),
	KE(SDLK_PAGEDOWN),
	KE(SDLK_F1),
	KE(SDLK_F2),
	KE(SDLK_F3),
	KE(SDLK_F4),
	KE(SDLK_F5),
	KE(SDLK_F6),
	KE(SDLK_F7),
	KE(SDLK_F8),
	KE(SDLK_F9),
	KE(SDLK_F10),
	KE(SDLK_F11),
	KE(SDLK_F12),
	KE(SDLK_F13),
	KE(SDLK_F14),
	KE(SDLK_F15),
	KE(SDLK_NUMLOCK),
	KE(SDLK_CAPSLOCK),
	KE(SDLK_SCROLLOCK),
	KE(SDLK_RSHIFT),
	KE(SDLK_LSHIFT),
	KE(SDLK_RCTRL),
	KE(SDLK_LCTRL),
	KE(SDLK_RALT),
	KE(SDLK_LALT),
	KE(SDLK_RMETA),
	KE(SDLK_LMETA),
	KE(SDLK_LSUPER),
	KE(SDLK_RSUPER),
	KE(SDLK_MODE),
	KE(SDLK_COMPOSE),
	KE(SDLK_HELP),
	KE(SDLK_PRINT),
	KE(SDLK_SYSREQ),
	KE(SDLK_BREAK),
	KE(SDLK_MENU),
	KE(SDLK_POWER),
	KE(SDLK_EURO),
	KE(SDLK_UNDO),
	KE(SDLK_LAST),
	KE(-1)
};

static key_lookup_table mame_lookup[] =
{
	KE(KEYCODE_A),
 	KE(KEYCODE_B),
 	KE(KEYCODE_C),
 	KE(KEYCODE_D),
 	KE(KEYCODE_E),
 	KE(KEYCODE_F),
	KE(KEYCODE_G),
 	KE(KEYCODE_H),
 	KE(KEYCODE_I),
 	KE(KEYCODE_J),
 	KE(KEYCODE_K),
 	KE(KEYCODE_L),
	KE(KEYCODE_M),
 	KE(KEYCODE_N),
 	KE(KEYCODE_O),
 	KE(KEYCODE_P),
 	KE(KEYCODE_Q),
 	KE(KEYCODE_R),
	KE(KEYCODE_S),
 	KE(KEYCODE_T),
 	KE(KEYCODE_U),
 	KE(KEYCODE_V),
 	KE(KEYCODE_W),
 	KE(KEYCODE_X),
	KE(KEYCODE_Y),
 	KE(KEYCODE_Z),
 	KE(KEYCODE_0),
 	KE(KEYCODE_1),
 	KE(KEYCODE_2),
 	KE(KEYCODE_3),
	KE(KEYCODE_4),
 	KE(KEYCODE_5),
 	KE(KEYCODE_6),
 	KE(KEYCODE_7),
 	KE(KEYCODE_8),
 	KE(KEYCODE_9),
	KE(KEYCODE_F1),
 	KE(KEYCODE_F2),
 	KE(KEYCODE_F3),
 	KE(KEYCODE_F4),
 	KE(KEYCODE_F5),
	KE(KEYCODE_F6),
 	KE(KEYCODE_F7),
 	KE(KEYCODE_F8),
 	KE(KEYCODE_F9),
 	KE(KEYCODE_F10),
	KE(KEYCODE_F11),
 	KE(KEYCODE_F12),
 	KE(KEYCODE_F13),
 	KE(KEYCODE_F14),
 	KE(KEYCODE_F15),
	KE(KEYCODE_ESC),
 	KE(KEYCODE_TILDE),
 	KE(KEYCODE_MINUS),
 	KE(KEYCODE_EQUALS),
 	KE(KEYCODE_BACKSPACE),
	KE(KEYCODE_TAB),
 	KE(KEYCODE_OPENBRACE),
 	KE(KEYCODE_CLOSEBRACE),
 	KE(KEYCODE_ENTER),
 	KE(KEYCODE_COLON),
	KE(KEYCODE_QUOTE),
 	KE(KEYCODE_BACKSLASH),
 	KE(KEYCODE_BACKSLASH2),
 	KE(KEYCODE_COMMA),
 	KE(KEYCODE_STOP),
	KE(KEYCODE_SLASH),
 	KE(KEYCODE_SPACE),
 	KE(KEYCODE_INSERT),
 	KE(KEYCODE_DEL),
	KE(KEYCODE_HOME),
 	KE(KEYCODE_END),
 	KE(KEYCODE_PGUP),
 	KE(KEYCODE_PGDN),
	KE(KEYCODE_LEFT),
	KE(KEYCODE_RIGHT),
 	KE(KEYCODE_UP),
 	KE(KEYCODE_DOWN),
	KE(KEYCODE_0_PAD),
 	KE(KEYCODE_1_PAD),
 	KE(KEYCODE_2_PAD),
 	KE(KEYCODE_3_PAD),
 	KE(KEYCODE_4_PAD),
	KE(KEYCODE_5_PAD),
 	KE(KEYCODE_6_PAD),
 	KE(KEYCODE_7_PAD),
 	KE(KEYCODE_8_PAD),
 	KE(KEYCODE_9_PAD),
	KE(KEYCODE_SLASH_PAD),
 	KE(KEYCODE_ASTERISK),
 	KE(KEYCODE_MINUS_PAD),
 	KE(KEYCODE_PLUS_PAD),
	KE(KEYCODE_DEL_PAD),
 	KE(KEYCODE_ENTER_PAD),
 	KE(KEYCODE_PRTSCR),
 	KE(KEYCODE_PAUSE),
	KE(KEYCODE_LSHIFT),
 	KE(KEYCODE_RSHIFT),
 	KE(KEYCODE_LCONTROL),
 	KE(KEYCODE_RCONTROL),
	KE(KEYCODE_LALT),
 	KE(KEYCODE_RALT),
 	KE(KEYCODE_SCRLOCK),
 	KE(KEYCODE_NUMLOCK),
 	KE(KEYCODE_CAPSLOCK),
	KE(KEYCODE_LWIN),
 	KE(KEYCODE_RWIN),
 	KE(KEYCODE_MENU),
 	KE(-1)
};

#ifdef MESS
extern int win_use_natural_keyboard;
static const INT32 mess_keytrans[][2] =
{
	{ SDLK_ESCAPE,	UCHAR_MAMEKEY(ESC) },
	{ SDLK_F1,		UCHAR_MAMEKEY(F1) },
	{ SDLK_F2,		UCHAR_MAMEKEY(F2) },
	{ SDLK_F3,		UCHAR_MAMEKEY(F3) },
	{ SDLK_F4,		UCHAR_MAMEKEY(F4) },
	{ SDLK_F5,		UCHAR_MAMEKEY(F5) },
	{ SDLK_F6,		UCHAR_MAMEKEY(F6) },
	{ SDLK_F7,		UCHAR_MAMEKEY(F7) },
	{ SDLK_F8,		UCHAR_MAMEKEY(F8) },
	{ SDLK_F9,		UCHAR_MAMEKEY(F9) },
	{ SDLK_F10,		UCHAR_MAMEKEY(F10) },
	{ SDLK_F11,		UCHAR_MAMEKEY(F11) },
	{ SDLK_F12,		UCHAR_MAMEKEY(F12) },
	{ SDLK_LCTRL,	        UCHAR_MAMEKEY(LCONTROL) },
	{ SDLK_RCTRL,	        UCHAR_MAMEKEY(RCONTROL) },
	{ SDLK_NUMLOCK,	        UCHAR_MAMEKEY(NUMLOCK) },
	{ SDLK_CAPSLOCK,	UCHAR_MAMEKEY(CAPSLOCK) },
	{ SDLK_SCROLLOCK,	UCHAR_MAMEKEY(SCRLOCK) },
	{ SDLK_KP0,	UCHAR_MAMEKEY(0_PAD) },
	{ SDLK_KP1,	UCHAR_MAMEKEY(1_PAD) },
	{ SDLK_KP2,	UCHAR_MAMEKEY(2_PAD) },
	{ SDLK_KP3,	UCHAR_MAMEKEY(3_PAD) },
	{ SDLK_KP4,	UCHAR_MAMEKEY(4_PAD) },
	{ SDLK_KP5,	UCHAR_MAMEKEY(5_PAD) },
	{ SDLK_KP6,	UCHAR_MAMEKEY(6_PAD) },
	{ SDLK_KP7,	UCHAR_MAMEKEY(7_PAD) },
	{ SDLK_KP8,	UCHAR_MAMEKEY(8_PAD) },
	{ SDLK_KP9,	UCHAR_MAMEKEY(9_PAD) },
	{ SDLK_KP_PERIOD,	UCHAR_MAMEKEY(DEL_PAD) },
	{ SDLK_KP_PLUS,		UCHAR_MAMEKEY(PLUS_PAD) },
	{ SDLK_KP_MINUS,	UCHAR_MAMEKEY(MINUS_PAD) },
	{ SDLK_INSERT,	UCHAR_MAMEKEY(INSERT) },
	{ SDLK_DELETE,	UCHAR_MAMEKEY(DEL) },
	{ SDLK_HOME,		UCHAR_MAMEKEY(HOME) },
	{ SDLK_END,		UCHAR_MAMEKEY(END) },
	{ SDLK_PAGEUP,		UCHAR_MAMEKEY(PGUP) },
	{ SDLK_PAGEDOWN, 	UCHAR_MAMEKEY(PGDN) },
	{ SDLK_UP,		UCHAR_MAMEKEY(UP) },
	{ SDLK_DOWN,		UCHAR_MAMEKEY(DOWN) },
	{ SDLK_LEFT,		UCHAR_MAMEKEY(LEFT) },
	{ SDLK_RIGHT,		UCHAR_MAMEKEY(RIGHT) },
	{ SDLK_PAUSE,		UCHAR_MAMEKEY(PAUSE) },
};
#endif

//============================================================
//  INLINE FUNCTIONS
//============================================================

INLINE INT32 normalize_absolute_axis(INT32 raw, INT32 rawmin, INT32 rawmax)
{
	INT32 center = (rawmax + rawmin) / 2;

	// make sure we have valid data
	if (rawmin >= rawmax)
		return raw;
	
	// above center
	if (raw >= center)
	{
		INT32 result = (INT64)(raw - center) * (INT64)INPUT_ABSOLUTE_MAX / (INT64)(rawmax - center);
		return MIN(result, INPUT_ABSOLUTE_MAX);
	}
	
	// below center
	else
	{
		INT32 result = -((INT64)(center - raw) * (INT64)-INPUT_ABSOLUTE_MIN / (INT64)(center - rawmin));
		return MAX(result, INPUT_ABSOLUTE_MIN);
	}
}

static int joy_map_leastfree(void)
{
	int i,j,found;
	for (i=0;i<MAX_JOYSTICKS;i++)
	{
		found = 0;
		for (j=0;j<MAX_JOYMAP;j++)
			if (joy_map[j].logical == i)
				found = 1;
		if (!found)
			return i;
	}
	return -1;
}

static char *remove_spaces(const char *s)
{
	char *r, *p;
	static const char *def_name[] = { "Unknown" };

	while (*s && *s == ' ')
		s++;

	if (strlen(s) == 0)
	{
		r = auto_malloc(strlen((char *)def_name)+1);
		strcpy(r, (char *)def_name);
		return r;
	}

	r = auto_malloc(strlen(s));
	p = r;
	while (*s)
	{
		if (*s != ' ')
			*p++ = *s++;
		else
		{
			while (*s && *s == ' ')
				s++;
			if (*s)
				*p++ = ' ';
		}
	}
	*p = 0;
	return r;
}

static char *alloc_name(char *instr)
{
	char *out;

	out = (char *)malloc(strlen(instr)+1);
	memset(out, 0, strlen(instr)+1);
	strcpy(out, instr);

	return out;
}

//============================================================
//  init_joymap
//============================================================

static void init_joymap(void)
{
	int stick;

	for (stick=0; stick < MAX_JOYMAP; stick++)
	{
		joy_map[stick].name = (char *)"";
		joy_map[stick].logical = -1;
		joy_map[stick].ismapped = 0;
	}
		
	if (options_get_bool(mame_options(), SDLOPTION_JOYMAP))
	{
		FILE *joymap_file;
		int line = 1;
		char *joymap_filename;
	
		joymap_filename = (char *)options_get_string(mame_options(), SDLOPTION_JOYMAP_FILE);
		mame_printf_verbose("Joymap: Start reading joymap_file %s\n", joymap_filename);

		joymap_file = fopen(joymap_filename, "r");

		if (joymap_file == NULL)
		{
			mame_printf_warning( "Joymap: Unable to open joymap %s - using default mapping\n", joymap_filename);
		}
		else
		{
			int i, cnt, logical;
			char buf[256];
			char name[65];
			
			cnt=0;
			while (!feof(joymap_file) && fgets(buf, 255, joymap_file))
			{
				if (*buf && buf[0] && buf[0] != '#')
				{
					buf[255]=0;
					i=strlen(buf);
					if (i && buf[i-1] == '\n')
						buf[i-1] = 0;
					memset(name, 0, 65);
					logical = -1;
					sscanf(buf, "%x %64c\n", &logical, name);
					if ((logical >=0 ) && (logical < MAX_JOYSTICKS))
					{
						joy_map[cnt].logical = logical;
						joy_map[cnt].name = remove_spaces(name);
						mame_printf_verbose("Joymap: Logical id %d: %s\n", logical, joy_map[cnt].name);
						cnt++;
					}
					else
						mame_printf_warning("Joymap: Error reading map: Line %d: %s\n", line, buf);
				}
				line++;
			}
			fclose(joymap_file);
			mame_printf_verbose("Joymap: Processed %d lines\n", line);
		}
	}
}

//============================================================
//  sdlinput_register_joysticks
//============================================================

static void sdlinput_register_joysticks(running_machine *machine)
{
	device_info *devinfo;
	int physical_stick, axis, button, hat, stick, first_free, i;
	char tempname[512];
	SDL_Joystick *joy;

	init_joymap();

	mame_printf_verbose("Joystick: Start initialization\n");
	for (physical_stick = 0; physical_stick < SDL_NumJoysticks(); physical_stick++)
	{
		char *joy_name = remove_spaces(SDL_JoystickName(physical_stick));
	
		devinfo = generic_device_alloc(&joystick_list, joy_name);
		devinfo->device = input_device_add(DEVICE_CLASS_JOYSTICK, devinfo->name, devinfo);

		// loop over all axes
		joy = SDL_JoystickOpen(physical_stick);
		devinfo->joystick.device[physical_stick] = joy;

		stick = -1;
		first_free = 0;
		for (i=0;i<MAX_JOYSTICKS;i++)
		{
			if (!first_free && !joy_map[i].name[0])
				first_free = i;
			if (!joy_map[i].ismapped && !strcmp(joy_name,joy_map[i].name))
			{
				stick = joy_map[i].logical;
				joy_map[i].ismapped = 1;
			}
		}
		if (stick == -1)
		{
			stick = joy_map_leastfree();
			joy_map[first_free].logical = stick;
			joy_map[first_free].name = joy_name;
			joy_map[first_free].ismapped = 1;
		}
		
		joy_logical[physical_stick] = stick;

		mame_printf_verbose("Joystick: %s\n", joy_name);
		mame_printf_verbose("Joystick:   ...  %d axes, %d buttons %d hats\n", SDL_JoystickNumAxes(joy), SDL_JoystickNumButtons(joy), SDL_JoystickNumHats(joy));

		// loop over all axes
		for (axis = 0; axis < SDL_JoystickNumAxes(joy); axis++)
		{
			sprintf(tempname, "J%d A%d %s", stick + 1, axis, joy_name);
			input_device_item_add(devinfo->device, tempname, &devinfo->joystick.axes[axis], ITEM_ID_XAXIS + axis, generic_axis_get_state);
		}

		// loop over all buttons
		for (button = 0; button < SDL_JoystickNumButtons(joy); button++)
		{
			sprintf(tempname, "J%d button %d", stick + 1, button);
			input_device_item_add(devinfo->device, tempname, &devinfo->joystick.buttons[button], ITEM_ID_BUTTON1 + button, generic_button_get_state);
		}

		// loop over all hats
		for (hat = 0; hat < SDL_JoystickNumHats(joy); hat++)
		{
			sprintf(tempname, "J%d hat %d Up", stick + 1, hat);
			input_device_item_add(devinfo->device, tempname, &devinfo->joystick.hatsU[hat], ITEM_ID_OTHER_SWITCH, generic_button_get_state);
			sprintf(tempname, "J%d hat %d Down", stick + 1, hat);
			input_device_item_add(devinfo->device, tempname, &devinfo->joystick.hatsD[hat], ITEM_ID_OTHER_SWITCH, generic_button_get_state);
			sprintf(tempname, "J%d hat %d Left", stick + 1, hat);
			input_device_item_add(devinfo->device, tempname, &devinfo->joystick.hatsL[hat], ITEM_ID_OTHER_SWITCH, generic_button_get_state);
			sprintf(tempname, "J%d hat %d Right", stick + 1, hat);
			input_device_item_add(devinfo->device, tempname, &devinfo->joystick.hatsR[hat], ITEM_ID_OTHER_SWITCH, generic_button_get_state);
		}
	}
	mame_printf_verbose("Joystick: End initialization\n");
}

//============================================================
//  sdlinput_init
//============================================================

static int lookup_key_code(const key_lookup_table *kt, char *kn)
{
	int i=0;
	if (!kn)
		return -1;
	while (kt[i].code>=0)
	{
		if (!strcmp(kn, kt[i].name))
			return kt[i].code;
		i++;
	}
	return -1;
}

void sdlinput_init(running_machine *machine)
{
	device_info *devinfo;
	int keynum, button;
	char defname[20];
	int (*key_trans_table)[4] = NULL;
	char **ui_name = NULL;

	keyboard_list = NULL;
	joystick_list = NULL;
	mouse_list = NULL;
	lightgun_list = NULL;

	// we need pause and exit callbacks
	add_pause_callback(machine, sdlinput_pause);
	add_exit_callback(machine, sdlinput_exit);

	if (options_get_bool(mame_options(), SDLOPTION_KEYMAP))
	{
		char *keymap_filename;
		FILE *keymap_file;
		int line = 1;

		keymap_filename = (char *)options_get_string(mame_options(), SDLOPTION_KEYMAP_FILE);
		mame_printf_verbose("Keymap: Start reading keymap_file %s\n", keymap_filename);

		keymap_file = fopen(keymap_filename, "r");

		if (keymap_file == NULL)
		{
			mame_printf_warning( "Keymap: Unable to open keymap %s, using default\n", keymap_filename);
			key_trans_table = (int (*)[4])sdl_key_trans_table;
		}
		else
		{
			int index,i, mk, sk, vk, ak;
			char buf[256];
			char mks[21];
			char sks[21];
			char kns[21];
			
			i = ARRAY_LENGTH(sdl_key_trans_table);
			key_trans_table = malloc_or_die( sizeof(int) * 4 * (i+1));
			memcpy(key_trans_table, sdl_key_trans_table, sizeof(int) * 4 * (i+1));
			ui_name = malloc_or_die( sizeof(char *) * (i+1));
			memset(ui_name, 0, sizeof(char *) * (i+1));
			while (!feof(keymap_file))
			{
				fgets(buf, 255, keymap_file);
				if (*buf && buf[0] && buf[0] != '#')
				{
					buf[255]=0;
					i=strlen(buf);
					if (i && buf[i-1] == '\n')
						buf[i-1] = 0;
					mks[0]=0;
					sks[0]=0;
					memset(kns, 0, 21);
					sscanf(buf, "%20s %20s %x %x %20c\n",
							mks, sks, &vk, &ak, kns);
					index=-1;
					i=0;
					mk = lookup_key_code(mame_lookup, mks);
					sk = lookup_key_code(sdl_lookup, sks);
					if ( sk >= 0 && mk >=0) 
					{
						while (key_trans_table[i][MAME_KEY]>=0 )
						{
							if (key_trans_table[i][MAME_KEY]==mk)
						  	{
								index=i;
							}	
							i++;
						}
						if (index>=0)
						{
							key_trans_table[index][SDL_KEY] = sk;
							key_trans_table[index][VIRTUAL_KEY] = vk;
							key_trans_table[index][ASCII_KEY] = ak;
							ui_name[index] = auto_malloc(strlen(kns)+1);
							strcpy(ui_name[index], kns);
							mame_printf_verbose("Keymap: Mapped <%s> to <%s> with ui-text <%s>\n", sks, mks, kns);
						}
					}
					else
						mame_printf_warning("Keymap: Error on line %d: %s\n", line, buf);
				}
				line++;
			}
			fclose(keymap_file);
			mame_printf_verbose("Keymap: Processed %d lines\n", line);
		}
	}
	else
	{
		key_trans_table = (int (*)[4])sdl_key_trans_table;
	}

	// allocate a lock for input synchronizations
	input_lock = osd_lock_alloc();
	assert_always(input_lock != NULL, "Failed to allocate input_lock");

	// SDL 1.2 only has 1 keyboard (1.3+ will have multiple, this must be revisited then)
	// add it now
	devinfo = generic_device_alloc(&keyboard_list, alloc_name((char *)"System keyboard"));
	devinfo->device = input_device_add(DEVICE_CLASS_KEYBOARD, devinfo->name, devinfo);

	// populate it
	for (keynum = 0; keynum < ARRAY_LENGTH(sdl_key_trans_table); keynum++)
	{
		input_item_id itemid;
		int id;
		
		itemid = key_trans_table[keynum][MAME_KEY];

		// generate the default name
		snprintf(defname, sizeof(defname)-1, "Scan%03d", keynum);

		// see if we have a better one
		for (id = 0; id < ARRAY_LENGTH(id_to_name); id++)
		{
			if (id_to_name[id].code == itemid)
			{
				memset(defname, 0, sizeof(defname));
				strcpy(defname, id_to_name[id].string);
				break;
			}
		}
		
		// add the item to the device
		input_device_item_add(devinfo->device, defname, &devinfo->keyboard.state[key_trans_table[keynum][SDL_KEY]], itemid, generic_button_get_state);
	}

	// SDL 1.2 has only 1 mouse - 1.3+ will also change that, so revisit this then
	devinfo = generic_device_alloc(&mouse_list, alloc_name((char *)"System mouse"));
	devinfo->device = input_device_add(DEVICE_CLASS_MOUSE, devinfo->name, devinfo);

	// add the axes
	input_device_item_add(devinfo->device, "Mouse X", &devinfo->mouse.lX, ITEM_ID_XAXIS, generic_axis_get_state);
	input_device_item_add(devinfo->device, "Mouse Y", &devinfo->mouse.lY, ITEM_ID_YAXIS, generic_axis_get_state);

	for (button = 0; button < 4; button++)
	{
		sprintf(defname, "Mouse B%d", button + 1);

		input_device_item_add(devinfo->device, defname, &devinfo->mouse.buttons[button], ITEM_ID_BUTTON1+button, generic_button_get_state);
	}

	// register the joysticks
	sdlinput_register_joysticks(machine);

	// now reset all devices
	device_list_reset_devices(keyboard_list);
	device_list_reset_devices(mouse_list);
	device_list_reset_devices(joystick_list);
}


//============================================================
//  sdlinput_pause
//============================================================

static void sdlinput_pause(running_machine *machine, int paused)
{
	// keep track of the paused state
	input_paused = paused;
}


//============================================================
//  sdlinput_exit
//============================================================

static void sdlinput_exit(running_machine *machine)
{
	// free the lock
	osd_lock_free(input_lock);

	// free all devices
	device_list_free_devices(keyboard_list);
	device_list_free_devices(mouse_list);
	device_list_free_devices(joystick_list);
}


//============================================================
//  sdlinput_poll
//============================================================

#ifdef SDLMAME_WIN32
void sdlinput_process_events_buf(void)
{
	SDL_Event event;

	osd_lock_acquire(input_lock);
	while(SDL_PollEvent(&event)) 
	{
		if (event_buf_count < MAX_BUF_EVENTS)
			event_buf[event_buf_count++] = event;
		else
			mame_printf_warning("Event Buffer Overflow!\n");	
	}
	osd_lock_release(input_lock);
}
#endif

void sdlinput_poll(void)
{
	device_info *devinfo;
	SDL_Event event;
#ifdef SDLMAME_WIN32
	SDL_Event			loc_event_buf[MAX_BUF_EVENTS];
	int					loc_event_buf_count;
	int bufp;
#endif

	devinfo = mouse_list;
	devinfo->mouse.lX = 0;
	devinfo->mouse.lY = 0;

#ifdef SDLMAME_WIN32
	osd_lock_acquire(input_lock);
	memcpy(loc_event_buf, event_buf, sizeof(event_buf));
	loc_event_buf_count = event_buf_count;
	event_buf_count = 0;
	osd_lock_release(input_lock);
	bufp = 0;
	while (bufp < loc_event_buf_count) {
		event = loc_event_buf[bufp++];
#else
	while(SDL_PollEvent(&event)) {
#endif

		if (event.type == SDL_KEYUP && 
		    event.key.keysym.sym == SDLK_CAPSLOCK)
		{
			/* more caps-lock hack */
			event.type = SDL_KEYDOWN;
		}
		switch(event.type) {
		case SDL_QUIT:
			mame_schedule_exit(Machine);
			break;
		case SDL_KEYDOWN:
			#ifdef MESS
			if (win_use_natural_keyboard)
			{
				int translated = 0, i;

				for (i = 0; i < sizeof(mess_keytrans) / sizeof(mess_keytrans[0]); i++)
				{
					if (event.key.keysym.sym == mess_keytrans[i][0])
					{
						translated = 1;
						inputx_postc(mess_keytrans[i][1]);
					}
				}

				if (!translated)
				{
					inputx_postc(event.key.keysym.unicode);
				}
			}
			#endif

			devinfo = keyboard_list;
			devinfo->keyboard.state[event.key.keysym.sym] = 0x80;
			break;
		case SDL_KEYUP:
			devinfo = keyboard_list;
			devinfo->keyboard.state[event.key.keysym.sym] = 0;
			break;
		case SDL_JOYAXISMOTION:
			devinfo = generic_device_find_index(joystick_list, event.jaxis.which);
			if (devinfo)
			{
				devinfo->joystick.axes[event.jaxis.axis] = (event.jaxis.value * 2); 
			}
			break;
		case SDL_JOYHATMOTION:
			devinfo = generic_device_find_index(joystick_list, event.jhat.which);
			if (devinfo)
			{
				if (event.jhat.value == SDL_HAT_UP)
				{
					devinfo->joystick.hatsU[event.jhat.hat] = 0x80;
				}
				else
				{
					devinfo->joystick.hatsU[event.jhat.hat] = 0;
				}
				if (event.jhat.value == SDL_HAT_DOWN)
				{
					devinfo->joystick.hatsD[event.jhat.hat] = 0x80;
				}
				else
				{
					devinfo->joystick.hatsD[event.jhat.hat] = 0;
				}
				if (event.jhat.value == SDL_HAT_LEFT)
				{
					devinfo->joystick.hatsL[event.jhat.hat] = 0x80;
				}
				else
				{
					devinfo->joystick.hatsL[event.jhat.hat] = 0;
				}
				if (event.jhat.value == SDL_HAT_RIGHT)
				{
					devinfo->joystick.hatsR[event.jhat.hat] = 0x80;
				}
				else
				{
					devinfo->joystick.hatsR[event.jhat.hat] = 0;
				}
			}
			break;
		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
			devinfo = generic_device_find_index(joystick_list, event.jbutton.which);
			if (devinfo)
			{
				devinfo->joystick.buttons[event.jbutton.button] = (event.jbutton.state == SDL_PRESSED) ? 0x80 : 0; 
			}
			break;
		case SDL_MOUSEBUTTONDOWN:
			devinfo = mouse_list;
			devinfo->mouse.buttons[event.button.button-1] = 0x80; 
			break;
		case SDL_MOUSEBUTTONUP:
			devinfo = mouse_list;
			devinfo->mouse.buttons[event.button.button-1] = 0; 
			break;
		case SDL_MOUSEMOTION:
			devinfo = mouse_list;
			devinfo->mouse.lX = event.motion.xrel * INPUT_RELATIVE_PER_PIXEL; 
			devinfo->mouse.lY = event.motion.yrel * INPUT_RELATIVE_PER_PIXEL; 
			break;
		case SDL_VIDEORESIZE:
			sdlwindow_resize(event.resize.w, event.resize.h);
			break;
		}
	}
}


//============================================================
//  sdlinput_should_hide_mouse
//============================================================

int sdlinput_should_hide_mouse(void)
{
	// if we are paused, no
	if (input_paused)
		return FALSE;
	
	// if neither mice nor lightguns enabled in the core, then no
	if (!mouse_enabled && !lightgun_enabled)
		return FALSE;
	
	// otherwise, yes
	return TRUE;
}


//============================================================
//  osd_customize_inputport_list
//============================================================

void osd_customize_inputport_list(input_port_default_entry *defaults)
{
//	static input_seq no_alt_tab_seq = SEQ_DEF_5(KEYCODE_TAB, CODE_NOT, KEYCODE_LALT, CODE_NOT, KEYCODE_RALT);
	input_port_default_entry *idef = defaults;

	// loop over all the defaults
	while (idef->type != IPT_END)
	{
		// map in some OSD-specific keys
		switch (idef->type)
		{
			#ifdef MESS
			// configurable UI mode switch
			case IPT_UI_TOGGLE_UI:
				input_seq_set_1(&idef->defaultseq, lookup_key_code(mame_lookup, (char *)options_get_string(mame_options(), SDLOPTION_UIMODEKEY)));
				break;
			#endif
			// alt-enter for fullscreen
			case IPT_OSD_1:
				idef->token = "TOGGLE_FULLSCREEN";
				idef->name = "Toggle Fullscreen";
				input_seq_set_2(&idef->defaultseq, KEYCODE_ENTER, KEYCODE_LALT);
				break;

			// disable UI_SELECT when LALT is down, this stops selecting
			// things in the menu when toggling fullscreen with LALT+ENTER
/*			case IPT_UI_SELECT:
				input_seq_set_3(&idef->defaultseq, KEYCODE_ENTER, CODE_NOT, KEYCODE_LALT);
				break;*/

			// page down for fastforward (must be OSD_3 as per src/emu/ui.c)
			case IPT_OSD_3:
				idef->token = "FAST_FORWARD";
				idef->name = "Fast Forward";
				input_seq_set_1(&idef->defaultseq, KEYCODE_PGDN);
				break;
			
			// OSD hotkeys use LCTRL and start at F3, they start at
			// F3 because F1-F2 are hardcoded into many drivers to
			// various dipswitches, and pressing them together with
			// LCTRL will still press/toggle these dipswitches.

			// LCTRL-F3 to toggle fullstretch
			case IPT_OSD_2:
				idef->token = "TOGGLE_FULLSTRETCH";
				idef->name = "Toggle Uneven stretch";
				input_seq_set_2(&idef->defaultseq, KEYCODE_F3, KEYCODE_LCONTROL);
				break;
			// add a Not lcrtl condition to the reset key
/*			case IPT_UI_SOFT_RESET:
				input_seq_set_5(&idef->defaultseq, KEYCODE_F3, CODE_NOT, KEYCODE_LCONTROL, CODE_NOT, KEYCODE_LSHIFT);
				break;*/
			
			// LCTRL-F4 to toggle keep aspect
			case IPT_OSD_4:
				idef->token = "TOGGLE_KEEP_ASPECT";
				idef->name = "Toggle Keepaspect";
				input_seq_set_2(&idef->defaultseq, KEYCODE_F4, KEYCODE_LCONTROL);
				break;
			// add a Not lcrtl condition to the show gfx key
/*			case IPT_UI_SHOW_GFX:
				input_seq_set_3(&idef->defaultseq, KEYCODE_F4, CODE_NOT, KEYCODE_LCONTROL);
				break;*/
			
			// LCTRL-F5 to toggle OpenGL filtering
			case IPT_OSD_5:
				idef->token = "TOGGLE_FILTER";
				idef->name = "Toggle Filter";
				input_seq_set_2(&idef->defaultseq, KEYCODE_F5, KEYCODE_LCONTROL);
				break;
			// add a Not lcrtl condition to the toggle debug key
/*			case IPT_UI_TOGGLE_DEBUG:
				input_seq_set_3(&idef->defaultseq, KEYCODE_F5, CODE_NOT, KEYCODE_LCONTROL);
				break;*/
			
			// LCTRL-F6 to decrease OpenGL prescaling
			case IPT_OSD_6:
				idef->token = "DECREASE_PRESCALE";
				idef->name = "Decrease Prescaling";
				input_seq_set_2(&idef->defaultseq, KEYCODE_F6, KEYCODE_LCONTROL);
				break;
			// add a Not lcrtl condition to the toggle cheat key
/*			case IPT_UI_TOGGLE_CHEAT:
				input_seq_set_3(&idef->defaultseq, KEYCODE_F6, CODE_NOT, KEYCODE_LCONTROL);
				break;*/
			
			// LCTRL-F7 to increase OpenGL prescaling
			case IPT_OSD_7:
				idef->token = "INCREASE_PRESCALE";
				idef->name = "Increase Prescaling";
				input_seq_set_2(&idef->defaultseq, KEYCODE_F7, KEYCODE_LCONTROL);
				break;
			// add a Not lcrtl condition to the load state key
/*			case IPT_UI_LOAD_STATE:
				input_seq_set_5(&idef->defaultseq, KEYCODE_F7, CODE_NOT, KEYCODE_LCONTROL, CODE_NOT, KEYCODE_LSHIFT);
				break;*/
			
			// LCTRL-F8 to decrease prescaling effect #
			case IPT_OSD_8:
				if (getenv("SDLMAME_UNSUPPORTED")) {
					idef->token = "DECREASE_EFFECT";
					idef->name = "Decrease Effect";
					input_seq_set_2(&idef->defaultseq, KEYCODE_F8, KEYCODE_LCONTROL);
				}
				break;
			// add a Not lcrtl condition to frameskip decrease
/*			case IPT_UI_FRAMESKIP_DEC:
				if (getenv("SDLMAME_UNSUPPORTED"))
					input_seq_set_3(&idef->defaultseq, KEYCODE_F8, CODE_NOT, KEYCODE_LCONTROL);
				break;*/
			
			// LCTRL-F9 to increase prescaling effect #
			case IPT_OSD_9:
				if (getenv("SDLMAME_UNSUPPORTED")) {
					idef->token = "INCREASE_EFFECT";
					idef->name = "Increase Effect";
					input_seq_set_2(&idef->defaultseq, KEYCODE_F9, KEYCODE_LCONTROL);
				}
				break;
			// add a Not lcrtl condition to frameskip increase
/*			case IPT_UI_FRAMESKIP_INC:
				if (getenv("SDLMAME_UNSUPPORTED"))
					input_seq_set_3(&idef->defaultseq, KEYCODE_F9, CODE_NOT, KEYCODE_LCONTROL);
				break;*/
			
			// LCTRL-F10 to toggle the renderer (software vs opengl)
			case IPT_OSD_10:
				idef->token = "TOGGLE_RENDERER";
				idef->name = "Toggle OpenGL/software rendering";
				input_seq_set_2(&idef->defaultseq, KEYCODE_F10, KEYCODE_LCONTROL);
				break;
			// add a Not lcrtl condition to the throttle key
/*			case IPT_UI_THROTTLE:
				input_seq_set_3(&idef->defaultseq, KEYCODE_F10, CODE_NOT, KEYCODE_LCONTROL);
				break;*/
			
			// disable the config menu if the ALT key is down
			// (allows ALT-TAB to switch between windows apps)
			/*
			case IPT_UI_CONFIGURE:
				seq_copy(&idef->defaultseq, &no_alt_tab_seq);
				break;
			*/
		}

		// find the next one
		idef++;
	}
}

//============================================================
//  device_list_reset_devices
//============================================================

static void device_list_reset_devices(device_info *devlist_head)
{
	device_info *curdev;
	
	for (curdev = devlist_head; curdev != NULL; curdev = curdev->next)
		generic_device_reset(curdev);
}

//============================================================
//  device_list_free_devices
//============================================================

static void device_list_free_devices(device_info *devlist_head)
{
	device_info *curdev, *next;
	
	for (curdev = devlist_head; curdev != NULL; )
	{
		next = curdev->next;
		generic_device_free(curdev);
		curdev = next;
	}
}

//============================================================
//  device_list_count
//============================================================

static int device_list_count(device_info *devlist_head)
{
	device_info *curdev;
	int count = 0;
	
	for (curdev = devlist_head; curdev != NULL; curdev = curdev->next)
		count++;
	return count;
}


//============================================================
//  generic_device_alloc
//============================================================

static device_info *generic_device_alloc(device_info **devlist_head_ptr, const char *name)
{
	device_info **curdev_ptr;
	device_info *devinfo;

	// allocate memory for the device object
	devinfo = malloc_or_die(sizeof(*devinfo));
	memset(devinfo, 0, sizeof(*devinfo));
	devinfo->head = devlist_head_ptr;
	
	// allocate a UTF8 copy of the name
	devinfo->name = (char *)malloc(strlen(name)+1);
	if (devinfo->name == NULL)
		goto error;
	strcpy(devinfo->name, (char *)name);
	
	// append us to the list
	for (curdev_ptr = devinfo->head; *curdev_ptr != NULL; curdev_ptr = &(*curdev_ptr)->next) ;
	*curdev_ptr = devinfo;
	
	return devinfo;

error:
	free(devinfo);
	return NULL;
}


//============================================================
//  generic_device_free
//============================================================

static void generic_device_free(device_info *devinfo)
{
	device_info **curdev_ptr;

	// remove us from the list
	for (curdev_ptr = devinfo->head; *curdev_ptr != devinfo && *curdev_ptr != NULL; curdev_ptr = &(*curdev_ptr)->next) ;
	if (*curdev_ptr == devinfo)
		*curdev_ptr = devinfo->next;
	
	// free the copy of the name if present
	if (devinfo->name != NULL)
	{
		free((void *)devinfo->name);
	}
	devinfo->name = NULL;

	// and now free the info
	free(devinfo);
}


//============================================================
//  generic_device_index
//============================================================

static int generic_device_index(device_info *devlist_head, device_info *devinfo)
{
	int index = 0;
	while (devlist_head != NULL)
	{
		if (devlist_head == devinfo)
			return index;
		index++;
		devlist_head = devlist_head->next;
	}
	return -1;
}

static device_info *generic_device_find_index(device_info *devlist_head, int index)
{
	device_info *orig_head = devlist_head;

	while (devlist_head != NULL)
	{
		if (generic_device_index(orig_head, devlist_head) == index)
		{
			return devlist_head;
		}

		devlist_head = devlist_head->next;
	}
	return NULL;
}


//============================================================
//  generic_device_reset
//============================================================

static void generic_device_reset(device_info *devinfo)
{
	// keyboard case
	if (devinfo->head == &keyboard_list)
		memset(&devinfo->keyboard, 0, sizeof(devinfo->keyboard));
	
	// mouse/lightgun case
	else if (devinfo->head == &mouse_list || devinfo->head == &lightgun_list)
		memset(&devinfo->mouse, 0, sizeof(devinfo->mouse));
		
	// joystick case
	else if (devinfo->head == &joystick_list)
	{
		memset(&devinfo->joystick, 0, sizeof(devinfo->joystick));
	}
}


//============================================================
//  generic_button_get_state
//============================================================

static INT32 generic_button_get_state(void *device_internal, void *item_internal)
{
	UINT8 *itemdata = item_internal;
	
	// return the current state
	return *itemdata >> 7;
}


//============================================================
//  generic_axis_get_state
//============================================================

static INT32 generic_axis_get_state(void *device_internal, void *item_internal)
{
	INT32 *axisdata = item_internal;
	
	// return the current state
	return *axisdata;
}


//============================================================
//  sdl_init
//============================================================

static void sdl_init(running_machine *machine)
{
}


//============================================================
//  sdl_exit
//============================================================

static void sdl_exit(running_machine *machine)
{
}


//============================================================
//  sdl_keyboard_poll
//============================================================

static void sdl_keyboard_poll(device_info *devinfo)
{
#if 0
	int keynum;

	// reset the keyboard state and then repopulate
	memset(devinfo->keyboard.state, 0, sizeof(devinfo->keyboard.state));
	
	// iterate over keys
	for (keynum = 0; keynum < ARRAY_LENGTH(sdl_key_trans_table); keynum++)
	{
		int vk = sdl_key_trans_table[keynum][VIRTUAL_KEY];
		if (vk != 0 && (GetAsyncKeyState(vk) & 0x8000) != 0)
		{
			int dik = sdl_key_trans_table[keynum][SDL_KEY];
			
			// conver the VK code to a scancode (DIK code)
			if (dik != 0)
				devinfo->keyboard.state[dik] = 0x80;
				
			// set this flag so that we continue to use win32 until all keys are up
			keyboard_sdl_reported_key_down = TRUE;
		}
	}
#endif
}


//============================================================
//  sdl_lightgun_poll
//============================================================

static void sdl_lightgun_poll(device_info *devinfo)
{
#if 0
	INT32 xpos = 0, ypos = 0;
	POINT mousepos;
	
	// if we are using the shared axis hack, the data is updated via Windows messages only
	if (lightgun_shared_axis_mode)
		return;
	
	// get the cursor position and transform into final results
	GetCursorPos(&mousepos);
	if (sdl_window_list != NULL)
	{
		RECT client_rect;

		// get the position relative to the window		
		GetClientRect(sdl_window_list->hwnd, &client_rect);
		ScreenToClient(sdl_window_list->hwnd, &mousepos);
		
		// convert to absolute coordinates
		xpos = normalize_absolute_axis(mousepos.x, client_rect.left, client_rect.right);
		ypos = normalize_absolute_axis(mousepos.y, client_rect.top, client_rect.bottom);
	}
	
	// update the X/Y positions
	devinfo->mouse.state.lX = xpos;
	devinfo->mouse.state.lY = ypos;
#endif
}

//============================================================
//  default_button_name
//============================================================

static const char *default_button_name(int which)
{
	static char buffer[20];
	snprintf(buffer, sizeof(buffer)-1, "B%d", which);
	return buffer;
}


//============================================================
//  default_pov_name
//============================================================

static const char *default_pov_name(int which)
{
	static char buffer[20];
	snprintf(buffer, strlen(buffer)-1, "POV%d", which);
	return buffer;
}
