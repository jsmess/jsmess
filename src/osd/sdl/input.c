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

#define KTT_ENTRY0(MAME, SDL, VK, AS, UI) { ITEM_ID_ ## MAME, SDLK_ ## SDL, "ITEM_ID_" #MAME, (char *) UI }
#define KTT_ENTRY1(MAME, SDL) KTT_ENTRY0(MAME, SDL, MAME, MAME, #MAME)
// only for reference ...
#define KTT_ENTRY2(MAME, SDL) KTT_ENTRY0(MAME, SDL, 0, 0, #MAME)

// master keyboard translation table
typedef struct _kt_table kt_table;
struct _kt_table {
	INT32			mame_key;
	INT32			sdl_key;
	//const char *	vkey;
	//const char *	ascii;
	const char 	*	mame_key_name;
	char 		*	ui_name;
};

static kt_table sdl_key_trans_table[] =
{
	// MAME key			SDL key			vkey	ascii
	KTT_ENTRY0(  ESC,			ESCAPE,			0x1b,	0x1b,		"ESC"  ),
	KTT_ENTRY1(  1,	 			1 ),
	KTT_ENTRY1(  2,	 			2 ),
	KTT_ENTRY1(  3,	 			3 ),
	KTT_ENTRY1(  4,	 			4 ),
	KTT_ENTRY1(  5,	 			5 ),
	KTT_ENTRY1(  6,		 		6 ),
	KTT_ENTRY1(  7,		 		7 ),
	KTT_ENTRY1(  8,	 			8 ),
	KTT_ENTRY1(  9,	 			9 ),
	KTT_ENTRY1(  0,		 		0 ),
	KTT_ENTRY0(  MINUS,			MINUS,			0xbd,	'-',	"MINUS" ),
	KTT_ENTRY0(  EQUALS,		EQUALS,			0xbb,	'=',	"EQUALS" ),
	KTT_ENTRY0(  BACKSPACE,		BACKSPACE,		0x08,	0x08,	"BACKSPACE" ),
	KTT_ENTRY0(  TAB,			TAB,			0x09,	0x09,	"TAB" ),
	KTT_ENTRY1(  Q,				q ),
	KTT_ENTRY1(  W,				w ),
	KTT_ENTRY1(  E,				e ),
	KTT_ENTRY1(  R,				r ),
	KTT_ENTRY1(  T,				t ),
	KTT_ENTRY1(  Y,				y ),
	KTT_ENTRY1(  U,				u ),
	KTT_ENTRY1(  I,				i ),
	KTT_ENTRY1(  O,				o ),
	KTT_ENTRY1(  P,				p ),
	KTT_ENTRY0(  OPENBRACE,	LEFTBRACKET,		0xdb, 	'[',	"OPENBRACE" ),
	KTT_ENTRY0(  CLOSEBRACE,RIGHTBRACKET, 		0xdd,	']',	"CLOSEBRACE" ),
	KTT_ENTRY0(  ENTER,		RETURN, 			0x0d, 	0x0d,	"RETURN" ),
	KTT_ENTRY2(  LCONTROL,	LCTRL ),
	KTT_ENTRY1(  A,				a ),
	KTT_ENTRY1(  S, 			s ),
	KTT_ENTRY1(  D, 			d ),
	KTT_ENTRY1(  F, 			f ),
	KTT_ENTRY1(  G, 			g ),
	KTT_ENTRY1(  H, 			h ),
	KTT_ENTRY1(  J, 			j ),
	KTT_ENTRY1(  K, 			k ),
	KTT_ENTRY1(  L, 			l ),
	KTT_ENTRY0(  COLON, 		SEMICOLON,		0xba,	';',	"COLON" ),
	KTT_ENTRY0(  QUOTE, 		QUOTE,			0xde,	'\'',	"QUOTE" ),
	KTT_ENTRY2(  LSHIFT, 		LSHIFT ),
	KTT_ENTRY0(  BACKSLASH,		BACKSLASH, 		0xdc,	'\\',	"BACKSLASH" ),
	KTT_ENTRY1(  Z, 			z ),
	KTT_ENTRY1(  X, 			x ),
	KTT_ENTRY1(  C, 			c ),
	KTT_ENTRY1(  V, 			v ),
	KTT_ENTRY1(  B, 			b ),
	KTT_ENTRY1(  N, 			n ),
	KTT_ENTRY1(  M, 			m ),
	KTT_ENTRY0(  COMMA, 		COMMA,	     	0xbc,	',',	"COMMA" ),
	KTT_ENTRY0(  STOP,	 		PERIOD, 		0xbe,	'.',	"STOP"  ),
	KTT_ENTRY0(  SLASH, 		SLASH, 	     	0xbf,	'/',	"SLASH" ),
	KTT_ENTRY2(  RSHIFT, 		RSHIFT ),
	KTT_ENTRY0(  ASTERISK, 		KP_MULTIPLY,    '*',	'*',	"ASTERIX" ), 
	KTT_ENTRY2(  LALT, 			LALT ),
	KTT_ENTRY0(  SPACE, 		SPACE, 			' ',	' ',	"SPACE" ),
	KTT_ENTRY2(  CAPSLOCK, 		CAPSLOCK ),
	KTT_ENTRY2(  F1, 			F1 ),
	KTT_ENTRY2(  F2, 			F2 ),
	KTT_ENTRY2(  F3, 			F3 ),
	KTT_ENTRY2(  F4, 			F4 ),
	KTT_ENTRY2(  F5, 			F5 ),
	KTT_ENTRY2(  F6, 			F6 ),
	KTT_ENTRY2(  F7, 			F7 ),
	KTT_ENTRY2(  F8, 			F8 ),
	KTT_ENTRY2(  F9, 			F9 ),
	KTT_ENTRY2(  F10, 			F10 ),
	KTT_ENTRY2(  NUMLOCK, 		NUMLOCK ),
	KTT_ENTRY2(  SCRLOCK,	 	SCROLLOCK ),
	KTT_ENTRY2(  7_PAD, 		KP7 ),
	KTT_ENTRY2(  8_PAD, 		KP8 ),
	KTT_ENTRY2(  9_PAD, 		KP9 ),
	KTT_ENTRY2(  MINUS_PAD,		KP_MINUS ),
	KTT_ENTRY2(  4_PAD, 		KP4 ),
	KTT_ENTRY2(  5_PAD, 		KP5 ),
	KTT_ENTRY2(  6_PAD, 		KP6 ),
	KTT_ENTRY2(  PLUS_PAD, 		KP_PLUS ),
	KTT_ENTRY2(  1_PAD, 		KP1 ),
	KTT_ENTRY2(  2_PAD, 		KP2 ),
	KTT_ENTRY2(  3_PAD, 		KP3 ),
	KTT_ENTRY2(  0_PAD, 		KP0 ),
	KTT_ENTRY2(  DEL_PAD, 		KP_PERIOD ),
	KTT_ENTRY2(  F11, 			F11 ),
	KTT_ENTRY2(  F12, 			F12 ),
	KTT_ENTRY2(  F13, 			F13 ),
	KTT_ENTRY2(  F14, 			F14 ),
	KTT_ENTRY2(  F15, 			F15 ),
	KTT_ENTRY2(  ENTER_PAD,		KP_ENTER  ),
	KTT_ENTRY2(  RCONTROL, 		RCTRL ),
	KTT_ENTRY2(  SLASH_PAD,		KP_DIVIDE ),
	KTT_ENTRY2(  PRTSCR,	 	PRINT ),
	KTT_ENTRY2(  RALT, 			RALT ),
	KTT_ENTRY2(  HOME, 			HOME ),
	KTT_ENTRY2(  UP, 			UP ),
	KTT_ENTRY2(  PGUP, 			PAGEUP ),
	KTT_ENTRY2(  LEFT, 			LEFT ),
	KTT_ENTRY2(  RIGHT, 		RIGHT ),
	KTT_ENTRY2(  END, 			END ),
	KTT_ENTRY2(  DOWN, 			DOWN ),
	KTT_ENTRY2(  PGDN, 			PAGEDOWN ),
	KTT_ENTRY2(  INSERT, 		INSERT ),
	{ ITEM_ID_DEL, SDLK_DELETE,  "ITEM_ID_DEL", (char *)"DELETE" },
	KTT_ENTRY2(  LWIN, 			LSUPER ),
	KTT_ENTRY2(  RWIN, 			RSUPER ),
	KTT_ENTRY2(  MENU,	 		MENU ),
#if (SDL_VERSIONNUM(SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL) < 1300)
	KTT_ENTRY0(  TILDE, 		BACKQUOTE,  	0xc0,	'`',	"TILDE" ),
	KTT_ENTRY0(  BACKSLASH2,	HASH,     0xdc,   '\\', "BACKSLASH2" ),
#else
	KTT_ENTRY0(  TILDE, 		GRAVE,  	0xc0,	'`',	"TILDE" ),
	KTT_ENTRY0(  BACKSLASH2,	NONUSBACKSLASH,     0xdc,   '\\', "BACKSLASH2" ),
#endif
	{ -1 }
};

typedef struct _key_lookup_table key_lookup_table;

struct _key_lookup_table 
{
	int code;
	const char *name;
};

#define KE(x) { SDLK_ ## x, "SDLK_" #x },
#define KE8(A, B, C, D, E, F, G, H) KE(A) KE(B) KE(C) KE(D) KE(E) KE(F) KE(G) KE(H) 

static key_lookup_table sdl_lookup_table[] =
{
	KE8(UNKNOWN,	FIRST,		BACKSPACE,		TAB,		CLEAR,		RETURN,		PAUSE,		ESCAPE		)
	KE8(SPACE,		EXCLAIM,	QUOTEDBL,		HASH,		DOLLAR,		AMPERSAND,	QUOTE,		LEFTPAREN	)
	KE8(RIGHTPAREN,	ASTERISK,	PLUS,			COMMA,		MINUS,		PERIOD,		SLASH,		0			)
	KE8(1,			2,			3,				4,			5,			6,			7,			8			)
	KE8(9,			COLON,		SEMICOLON,		LESS,		EQUALS,		GREATER,	QUESTION,	AT			)
	KE8(LEFTBRACKET,BACKSLASH,	RIGHTBRACKET,	CARET,		UNDERSCORE,	BACKQUOTE,	a,			b			)
	KE8(c,			d,			e,				f,			g,			h,			i,			j			)
	KE8(k,			l,			m,				n,			o,			p,			q,			r			)
	KE8(s,			t,			u,				v,			w,			x,			y,			z			)
	{ SDLK_DELETE, "SDLK_DELETE" }, 
	//FIXME:Proper check for version
#if (SDL_VERSIONNUM(SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL) < 1300)
	KE8(WORLD_0,	WORLD_1,	WORLD_2,	WORLD_3,	WORLD_4,	WORLD_5,	WORLD_6,	WORLD_7		)
	KE8(WORLD_8,	WORLD_9,	WORLD_10,	WORLD_11,	WORLD_12,	WORLD_13,	WORLD_14,	WORLD_15	)
	KE8(WORLD_16,	WORLD_17,	WORLD_18,	WORLD_19,	WORLD_20,	WORLD_21,	WORLD_22,	WORLD_23	)
	KE8(WORLD_24,	WORLD_25,	WORLD_26,	WORLD_27,	WORLD_28,	WORLD_29,	WORLD_30,	WORLD_31	)
	KE8(WORLD_32,	WORLD_33,	WORLD_34,	WORLD_35,	WORLD_36,	WORLD_37,	WORLD_38,	WORLD_39	)
	KE8(WORLD_40,	WORLD_41,	WORLD_42,	WORLD_43,	WORLD_44,	WORLD_45,	WORLD_46,	WORLD_47	)
	KE8(WORLD_48,	WORLD_49,	WORLD_50,	WORLD_51,	WORLD_52,	WORLD_53,	WORLD_54,	WORLD_55	)
	KE8(WORLD_56,	WORLD_57,	WORLD_58,	WORLD_59,	WORLD_60,	WORLD_61,	WORLD_62,	WORLD_63	)
	KE8(WORLD_64,	WORLD_65,	WORLD_66,	WORLD_67,	WORLD_68,	WORLD_69,	WORLD_70,	WORLD_71	)
	KE8(WORLD_72,	WORLD_73,	WORLD_74,	WORLD_75,	WORLD_76,	WORLD_77,	WORLD_78,	WORLD_79	)
	KE8(WORLD_80,	WORLD_81,	WORLD_82,	WORLD_83,	WORLD_84,	WORLD_85,	WORLD_86,	WORLD_87	)
	KE8(WORLD_88,	WORLD_89,	WORLD_90,	WORLD_91,	WORLD_92,	WORLD_93,	WORLD_94,	WORLD_95	)
#endif
	KE8(KP0,		KP1,		KP2,		KP3,		KP4,		KP5,		KP6,		KP7			)
	KE8(KP8,		KP9,		KP_PERIOD,	KP_DIVIDE,	KP_MULTIPLY,KP_MINUS,	KP_PLUS, 	KP_ENTER	)
	KE8(UP,			DOWN,		RIGHT,		LEFT,		INSERT,		HOME,		KP_EQUALS,	END			)
	KE8(PAGEUP,		PAGEDOWN,	F1,			F2,			F3,			F4,			F5,			F6			)
	KE8(F7,			F8,			F9,			F10,		F11,		F12,		F13,		F14			)
	KE8(F15,		NUMLOCK,	CAPSLOCK,	SCROLLOCK,	RSHIFT,		LSHIFT,		RCTRL,		LCTRL		)
	KE8(RALT,		LALT,		RMETA,		LMETA,		LSUPER,		RSUPER,		MODE,		COMPOSE		)
	KE8(HELP,		PRINT,		SYSREQ,		BREAK,		MENU,		POWER,		EURO,		UNDO		)
	KE(LAST)		
	{-1, ""}
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
#if 0
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
#endif
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

static char *alloc_name(const char *instr)
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
		mame_printf_verbose("Joystick:   ...  Physical id %d mapped to logical id %d\n", physical_stick, stick);

		// loop over all axes
		for (axis = 0; axis < SDL_JoystickNumAxes(joy); axis++)
		{
			if (axis < 6)
			{
				sprintf(tempname, "J%d A%d %s", stick + 1, axis, joy_name);
				input_device_item_add(devinfo->device, tempname, &devinfo->joystick.axes[axis], ITEM_ID_XAXIS + axis, generic_axis_get_state);
			}
			else
			{
				sprintf(tempname, "J%d A%d %s", stick + 1, axis, joy_name);
				input_device_item_add(devinfo->device, tempname, &devinfo->joystick.axes[axis], ITEM_ID_OTHER_AXIS_ABSOLUTE, generic_axis_get_state);
			}
		}

		// loop over all buttons
		for (button = 0; button < SDL_JoystickNumButtons(joy); button++)
		{
			if (button < 16)
			{
				sprintf(tempname, "J%d button %d", stick + 1, button);
				input_device_item_add(devinfo->device, tempname, &devinfo->joystick.buttons[button], ITEM_ID_BUTTON1 + button, generic_button_get_state);
			}
			else
			{
				sprintf(tempname, "J%d button %d", stick + 1, button);
				input_device_item_add(devinfo->device, tempname, &devinfo->joystick.buttons[button], ITEM_ID_OTHER_SWITCH, generic_button_get_state);
			}
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
//  lookup_sdl_code
//============================================================

static int lookup_sdl_code(const char *scode)
{
	int i=0;

	while (sdl_lookup_table[i].code>=0)
	{
		if (!strcmp(scode, sdl_lookup_table[i].name))
			return sdl_lookup_table[i].code;
		i++;
	}
	return -1;
}

//============================================================
//  lookup_mame_code
//============================================================

static int lookup_mame_index(const char *scode)
{
	int index, i;
	
	index=-1;
	i=0;
	while (sdl_key_trans_table[i].mame_key >= 0)
	{
		if (!strcmp(scode, sdl_key_trans_table[i].mame_key_name))
		{
			index=i;
			break;
		}
		i++;
	}
	return index;
}

static int lookup_mame_code(const char *scode)
{
	int index;
	index = lookup_mame_index(scode);
	if (index >= 0)
		return sdl_key_trans_table[index].mame_key; 		
	else
		return -1;
}

//============================================================
//  sdlinput_init
//============================================================

void sdlinput_init(running_machine *machine)
{
	device_info *devinfo;
	int keynum, button;
	char defname[20];
	kt_table *key_trans_table;
	kt_table *ktt_alloc = NULL;

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
			key_trans_table = sdl_key_trans_table;
		}
		else
		{
			int index,i, sk, vk, ak;
			char buf[256];
			char mks[21];
			char sks[21];
			char kns[21];
			
			ktt_alloc = malloc_or_die(sizeof(sdl_key_trans_table));
			key_trans_table = ktt_alloc;
			memcpy((void *) key_trans_table, sdl_key_trans_table, sizeof(sdl_key_trans_table));
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
					
					index=lookup_mame_index(mks);
					sk = lookup_sdl_code(sks);

					if ( sk >= 0 && index >=0) 
					{
						key_trans_table[index].sdl_key = sk;
						// vk and ak are not really needed
						//key_trans_table[index][VIRTUAL_KEY] = vk;
						//key_trans_table[index][ASCII_KEY] = ak;
						key_trans_table[index].ui_name = auto_malloc(strlen(kns)+1);
						strcpy(key_trans_table[index].ui_name, kns);
						mame_printf_verbose("Keymap: Mapped <%s> to <%s> with ui-text <%s>\n", sks, mks, kns);
					}
					else
						mame_printf_warning("Keymap: Error on line %d - %s key not found: %s\n", line, (sk<0) ? "sdl" : "mame", buf);
				}
				line++;
			}
			fclose(keymap_file);
			mame_printf_verbose("Keymap: Processed %d lines\n", line);
		}
	}
	else
	{
		key_trans_table = sdl_key_trans_table;
	}

	// allocate a lock for input synchronizations
	input_lock = osd_lock_alloc();
	assert_always(input_lock != NULL, "Failed to allocate input_lock");

	// SDL 1.2 only has 1 keyboard (1.3+ will have multiple, this must be revisited then)
	// add it now
	devinfo = generic_device_alloc(&keyboard_list, alloc_name((const char *)"System keyboard"));
	devinfo->device = input_device_add(DEVICE_CLASS_KEYBOARD, devinfo->name, devinfo);

	// populate it
	for (keynum = 0; sdl_key_trans_table[keynum].mame_key >= 0; keynum++)
	{
		input_item_id itemid;
		
		itemid = key_trans_table[keynum].mame_key;

		// generate the default / modified name
		snprintf(defname, sizeof(defname)-1, "%s", key_trans_table[keynum].ui_name);
		
		// add the item to the device
		input_device_item_add(devinfo->device, defname, &devinfo->keyboard.state[key_trans_table[keynum].sdl_key-SDLK_FIRST], itemid, generic_button_get_state);
	}

	// SDL 1.2 has only 1 mouse - 1.3+ will also change that, so revisit this then
	devinfo = generic_device_alloc(&mouse_list, alloc_name((const char *)"System mouse"));
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
	
	// clean up
	if (ktt_alloc)
		free(ktt_alloc);
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
			devinfo->keyboard.state[event.key.keysym.sym-SDLK_FIRST] = 0x80;
			break;
		case SDL_KEYUP:
			devinfo = keyboard_list;
			devinfo->keyboard.state[event.key.keysym.sym-SDLK_FIRST] = 0;
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
	input_port_default_entry *idef = defaults;
	#ifdef MESS
	int mameid_code ,ui_code;
	#endif

	// loop over all the defaults
	while (idef->type != IPT_END)
	{
		// map in some OSD-specific keys
		switch (idef->type)
		{
			#ifdef MESS
			// configurable UI mode switch
			case IPT_UI_TOGGLE_UI:
				mameid_code = lookup_mame_code((const char *)options_get_string(mame_options(), SDLOPTION_UIMODEKEY));
				ui_code = INPUT_CODE(DEVICE_CLASS_KEYBOARD, 0, ITEM_CLASS_SWITCH, ITEM_MODIFIER_NONE, mameid_code);
				input_seq_set_1(&idef->defaultseq, ui_code);
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
				input_seq_set_3(&idef->defaultseq, KEYCODE_ENTER, SEQCODE_NOT, KEYCODE_LALT);
				break;*/

			// page down for fastforward (must be OSD_3 as per src/emu/ui.c)
			case IPT_UI_FAST_FORWARD:
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
			case IPT_UI_SOFT_RESET:
				input_seq_set_5(&idef->defaultseq, KEYCODE_F3, SEQCODE_NOT, KEYCODE_LCONTROL, SEQCODE_NOT, KEYCODE_LSHIFT);
				break;
			
			// LCTRL-F4 to toggle keep aspect
			case IPT_OSD_4:
				idef->token = "TOGGLE_KEEP_ASPECT";
				idef->name = "Toggle Keepaspect";
				input_seq_set_2(&idef->defaultseq, KEYCODE_F4, KEYCODE_LCONTROL);
				break;
			// add a Not lcrtl condition to the show gfx key
			case IPT_UI_SHOW_GFX:
				input_seq_set_3(&idef->defaultseq, KEYCODE_F4, SEQCODE_NOT, KEYCODE_LCONTROL);
				break;
			
			// LCTRL-F5 to toggle OpenGL filtering
			case IPT_OSD_5:
				idef->token = "TOGGLE_FILTER";
				idef->name = "Toggle Filter";
				input_seq_set_2(&idef->defaultseq, KEYCODE_F5, KEYCODE_LCONTROL);
				break;
			// add a Not lcrtl condition to the toggle debug key
			case IPT_UI_TOGGLE_DEBUG:
				input_seq_set_3(&idef->defaultseq, KEYCODE_F5, SEQCODE_NOT, KEYCODE_LCONTROL);
				break;
			
			// LCTRL-F6 to decrease OpenGL prescaling
			case IPT_OSD_6:
				idef->token = "DECREASE_PRESCALE";
				idef->name = "Decrease Prescaling";
				input_seq_set_2(&idef->defaultseq, KEYCODE_F6, KEYCODE_LCONTROL);
				break;
			// add a Not lcrtl condition to the toggle cheat key
			case IPT_UI_TOGGLE_CHEAT:
				input_seq_set_3(&idef->defaultseq, KEYCODE_F6, SEQCODE_NOT, KEYCODE_LCONTROL);
				break;
			
			// LCTRL-F7 to increase OpenGL prescaling
			case IPT_OSD_7:
				idef->token = "INCREASE_PRESCALE";
				idef->name = "Increase Prescaling";
				input_seq_set_2(&idef->defaultseq, KEYCODE_F7, KEYCODE_LCONTROL);
				break;
			// add a Not lcrtl condition to the load state key
			case IPT_UI_LOAD_STATE:
				input_seq_set_5(&idef->defaultseq, KEYCODE_F7, SEQCODE_NOT, KEYCODE_LCONTROL, SEQCODE_NOT, KEYCODE_LSHIFT);
				break;
			
			// LCTRL-F8 to decrease prescaling effect #
			case IPT_OSD_8:
				if (getenv("SDLMAME_UNSUPPORTED")) {
					idef->token = "DECREASE_EFFECT";
					idef->name = "Decrease Effect";
					input_seq_set_2(&idef->defaultseq, KEYCODE_F8, KEYCODE_LCONTROL);
				}
				break;
			// add a Not lcrtl condition to frameskip decrease
			case IPT_UI_FRAMESKIP_DEC:
				if (getenv("SDLMAME_UNSUPPORTED"))
					input_seq_set_3(&idef->defaultseq, KEYCODE_F8, SEQCODE_NOT, KEYCODE_LCONTROL);
				break;
			
			// LCTRL-F9 to increase prescaling effect #
			case IPT_OSD_9:
				if (getenv("SDLMAME_UNSUPPORTED")) {
					idef->token = "INCREASE_EFFECT";
					idef->name = "Increase Effect";
					input_seq_set_2(&idef->defaultseq, KEYCODE_F9, KEYCODE_LCONTROL);
				}
				break;
			// add a Not lcrtl condition to frameskip increase
			case IPT_UI_FRAMESKIP_INC:
				if (getenv("SDLMAME_UNSUPPORTED"))
					input_seq_set_3(&idef->defaultseq, KEYCODE_F9, SEQCODE_NOT, KEYCODE_LCONTROL);
				break;
			
			// LCTRL-F10 to toggle the renderer (software vs opengl)
			case IPT_OSD_10:
				idef->token = "TOGGLE_RENDERER";
				idef->name = "Toggle OpenGL/software rendering";
				input_seq_set_2(&idef->defaultseq, KEYCODE_F10, KEYCODE_LCONTROL);
				break;
			// add a Not lcrtl condition to the throttle key
			case IPT_UI_THROTTLE:
				input_seq_set_3(&idef->defaultseq, KEYCODE_F10, SEQCODE_NOT, KEYCODE_LCONTROL);
				break;
			
			// disable the config menu if the ALT key is down
			// (allows ALT-TAB to switch between apps)
			case IPT_UI_CONFIGURE:
				input_seq_set_5(&idef->defaultseq, KEYCODE_TAB, SEQCODE_NOT, KEYCODE_LALT, SEQCODE_NOT, KEYCODE_RALT);
				break;
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
	INT32 *itemdata = item_internal;
	
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
