//============================================================
//
//  input.c - Win32 implementation of MAME input routines
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

// Needed for RAW Input
#define _WIN32_WINNT 0x501

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winioctl.h>
#include <tchar.h>

// undef WINNT for dinput.h to prevent duplicate definition
#undef WINNT
#ifdef DIRECTINPUT_VERSION
#undef DIRECTINPUT_VERSION
#endif
#define DIRECTINPUT_VERSION 0x0700
#include <dinput.h>

// standard C headers
#include <conio.h>
#include <ctype.h>
#include <stddef.h>

// MAME headers
#include "osdepend.h"
#include "driver.h"
#include "ui.h"

// MAMEOS headers
#include "winmain.h"
#include "window.h"
#include "input.h"
#include "debugwin.h"
#include "video.h"
#include "strconv.h"


//============================================================
//  IMPORTS
//============================================================

extern int win_physical_width;
extern int win_physical_height;



//============================================================
//  PARAMETERS
//============================================================

#define MAX_KEYBOARDS		1
#define MAX_MICE			8
#define MAX_JOYSTICKS		8
#define MAX_LIGHTGUNS		8
#define MAX_DX_LIGHTGUNS	2

#define MAX_KEYS			256

#define MAX_JOY				512
#define MAX_AXES			8
#define MAX_LIGHTGUN_AXIS	2
#define MAX_BUTTONS			32
#define MAX_POV				4

#define HISTORY_LENGTH		16

enum
{
	ANALOG_TYPE_PADDLE = 0,
	ANALOG_TYPE_ADSTICK,
	ANALOG_TYPE_LIGHTGUN,
	ANALOG_TYPE_PEDAL,
	ANALOG_TYPE_DIAL,
	ANALOG_TYPE_TRACKBALL,
	ANALOG_TYPE_POSITIONAL,
#ifdef MESS
	ANALOG_TYPE_MOUSE,
#endif // MESS
	ANALOG_TYPE_COUNT
};

enum
{
	SELECT_TYPE_KEYBOARD = 0,
	SELECT_TYPE_MOUSE,
	SELECT_TYPE_JOYSTICK,
	SELECT_TYPE_LIGHTGUN
};

enum
{
	AXIS_TYPE_INVALID = 0,
	AXIS_TYPE_DIGITAL,
	AXIS_TYPE_ANALOG
};



//============================================================
//  MACROS
//============================================================

#define STRUCTSIZE(x)		((dinput_version == 0x0300) ? sizeof(x##_DX3) : sizeof(x))

#define ELEMENTS(x)			(sizeof(x) / sizeof((x)[0]))



//============================================================
//  TYPEDEFS
//============================================================

typedef struct _axis_history axis_history;
struct _axis_history
{
	LONG		value;
	INT32		count;
};

typedef struct _raw_mouse raw_mouse;
struct _raw_mouse
{
	// Identifier for the mouse.
	// WM_INPUT passes the device HANDLE as lparam when registering a mousemove
	HANDLE			device_handle;

	// Current mouse axis and button info
	DIMOUSESTATE2	mouse_state;

	// Used to determine if the HID is using absolute mode or relative mode
	USHORT			flags;
};



//============================================================
//  GLOBAL VARIABLES
//============================================================

int							win_use_mouse;
HANDLE							win_input_handle;



//============================================================
//  LOCAL VARIABLES
//============================================================

// this will be filled in dynamically
static os_code_info	codelist[MAX_KEYS+MAX_JOY];
static int					total_codes;

// DirectInput variables
static LPDIRECTINPUT		dinput;
static int					dinput_version;

// global states
static int					input_paused;
static osd_ticks_t			last_poll;

// Controller override options
static float				a2d_deadzone;
static int					use_joystick;
static int					use_lightgun;
static int					use_lightgun_dual;
static int					use_lightgun_reload;
static int					steadykey;
static UINT8				analog_type[ANALOG_TYPE_COUNT];

// keyboard states
static int					keyboard_count;
static LPDIRECTINPUTDEVICE	keyboard_device[MAX_KEYBOARDS];
static LPDIRECTINPUTDEVICE2	keyboard_device2[MAX_KEYBOARDS];
static DIDEVCAPS			keyboard_caps[MAX_KEYBOARDS];
static BYTE					keyboard_state[MAX_KEYBOARDS][MAX_KEYS];
static UINT8				keyboard_detected_non_di_input;

// additional key data
static INT8					oldkey[MAX_KEYS];
static INT8					currkey[MAX_KEYS];

// mouse states
static int					mouse_active;
static int					mouse_count;
static int					mouse_num_of_buttons;
static LPDIRECTINPUTDEVICE	mouse_device[MAX_MICE+1];
static LPDIRECTINPUTDEVICE2	mouse_device2[MAX_MICE+1];
static raw_mouse			raw_mouse_device[MAX_MICE];
static osd_lock *			raw_mouse_lock;
static DIDEVCAPS			mouse_caps[MAX_MICE+1];
static DIMOUSESTATE2		mouse_state[MAX_MICE];
static DWORD				mouse_state_size;
static char					mouse_name[MAX_MICE+1][MAX_PATH];
static int					lightgun_count;
static POINT				lightgun_dual_player_pos[4];
static int					lightgun_dual_player_state[4];

// joystick states
static int					joystick_count;
static LPDIRECTINPUTDEVICE	joystick_device[MAX_JOYSTICKS];
static LPDIRECTINPUTDEVICE2	joystick_device2[MAX_JOYSTICKS];
static DIDEVCAPS			joystick_caps[MAX_JOYSTICKS];
static DIJOYSTATE			joystick_state[MAX_JOYSTICKS];
static DIPROPRANGE			joystick_range[MAX_JOYSTICKS][MAX_AXES];
static UINT8				joystick_digital[MAX_JOYSTICKS][MAX_AXES];
static char					joystick_name[MAX_JOYSTICKS][MAX_PATH];
static axis_history			joystick_history[MAX_JOYSTICKS][MAX_AXES][HISTORY_LENGTH];
static UINT8				joystick_type[MAX_JOYSTICKS][MAX_AXES];

// gun states
static INT32				gun_axis[MAX_DX_LIGHTGUNS][2];



//============================================================
//  OPTIONS
//============================================================

// prototypes
static void extract_input_config(void);



//============================================================
//  PROTOTYPES
//============================================================

static void wininput_exit(running_machine *machine);
static void updatekeyboard(void);
static void update_joystick_axes(void);
static void init_keycodes(void);
static void init_joycodes(void);
static void poll_lightguns(void);
static BOOL is_rm_rdp_mouse(const TCHAR *device_string);
static void process_raw_input(PRAWINPUT raw);
static BOOL register_raw_mouse(void);
static BOOL init_raw_mouse(void);
static void win_read_raw_mouse(void);



//============================================================
//  Dynamically linked functions from rawinput
//============================================================

typedef WINUSERAPI INT (WINAPI *pGetRawInputDeviceList)(OUT PRAWINPUTDEVICELIST pRawInputDeviceList, IN OUT PINT puiNumDevices, IN UINT cbSize);
typedef WINUSERAPI INT (WINAPI *pGetRawInputData)(IN HRAWINPUT hRawInput, IN UINT uiCommand, OUT LPVOID pData, IN OUT PINT pcbSize, IN UINT cbSizeHeader);
typedef WINUSERAPI INT (WINAPI *pGetRawInputDeviceInfo)(IN HANDLE hDevice, IN UINT uiCommand, OUT LPVOID pData, IN OUT PINT pcbSize);
typedef WINUSERAPI BOOL (WINAPI *pRegisterRawInputDevices)(IN PCRAWINPUTDEVICE pRawInputDevices, IN UINT uiNumDevices, IN UINT cbSize);

static pGetRawInputDeviceList _GetRawInputDeviceList;
static pGetRawInputData _GetRawInputData;
static pGetRawInputDeviceInfo _GetRawInputDeviceInfo;
static pRegisterRawInputDevices _RegisterRawInputDevices;



//============================================================
//  KEYBOARD/JOYSTICK LIST
//============================================================

// macros for building/mapping keyboard codes
#define KEYCODE(dik, vk, ascii)		((dik) | ((vk) << 8) | ((ascii) << 16))
#define DICODE(keycode)				((keycode) & 0xff)
#define VKCODE(keycode)				(((keycode) >> 8) & 0xff)
#define ASCIICODE(keycode)			(((keycode) >> 16) & 0xff)

// macros for building/mapping joystick codes
#define JOYCODE(joy, type, index)	((index) | ((type) << 8) | ((joy) << 12) | 0x80000000)
#define JOYINDEX(joycode)			((joycode) & 0xff)
#define CODETYPE(joycode)			(((joycode) >> 8) & 0xf)
#define JOYNUM(joycode)				(((joycode) >> 12) & 0xf)

// macros for differentiating the two
#define IS_KEYBOARD_CODE(code)		(((code) & 0x80000000) == 0)
#define IS_JOYSTICK_CODE(code)		(((code) & 0x80000000) != 0)

// joystick types
#define CODETYPE_KEYBOARD			0
#define CODETYPE_AXIS_NEG			1
#define CODETYPE_AXIS_POS			2
#define CODETYPE_POV_UP				3
#define CODETYPE_POV_DOWN			4
#define CODETYPE_POV_LEFT			5
#define CODETYPE_POV_RIGHT			6
#define CODETYPE_BUTTON				7
#define CODETYPE_JOYAXIS			8
#define CODETYPE_MOUSEAXIS			9
#define CODETYPE_MOUSEBUTTON		10
#define CODETYPE_GUNAXIS			11

// master keyboard translation table
const int win_key_trans_table[][4] =
{
	// MAME key             dinput key          virtual key     ascii
	{ KEYCODE_ESC, 			DIK_ESCAPE,			VK_ESCAPE,	 	27 },
	{ KEYCODE_1, 			DIK_1,				'1',			'1' },
	{ KEYCODE_2, 			DIK_2,				'2',			'2' },
	{ KEYCODE_3, 			DIK_3,				'3',			'3' },
	{ KEYCODE_4, 			DIK_4,				'4',			'4' },
	{ KEYCODE_5, 			DIK_5,				'5',			'5' },
	{ KEYCODE_6, 			DIK_6,				'6',			'6' },
	{ KEYCODE_7, 			DIK_7,				'7',			'7' },
	{ KEYCODE_8, 			DIK_8,				'8',			'8' },
	{ KEYCODE_9, 			DIK_9,				'9',			'9' },
	{ KEYCODE_0, 			DIK_0,				'0',			'0' },
	{ KEYCODE_MINUS, 		DIK_MINUS, 			0xbd,			'-' },
	{ KEYCODE_EQUALS, 		DIK_EQUALS,		 	0xbb,			'=' },
	{ KEYCODE_BACKSPACE,	DIK_BACK, 			VK_BACK, 		8 },
	{ KEYCODE_TAB, 			DIK_TAB, 			VK_TAB, 		9 },
	{ KEYCODE_Q, 			DIK_Q,				'Q',			'Q' },
	{ KEYCODE_W, 			DIK_W,				'W',			'W' },
	{ KEYCODE_E, 			DIK_E,				'E',			'E' },
	{ KEYCODE_R, 			DIK_R,				'R',			'R' },
	{ KEYCODE_T, 			DIK_T,				'T',			'T' },
	{ KEYCODE_Y, 			DIK_Y,				'Y',			'Y' },
	{ KEYCODE_U, 			DIK_U,				'U',			'U' },
	{ KEYCODE_I, 			DIK_I,				'I',			'I' },
	{ KEYCODE_O, 			DIK_O,				'O',			'O' },
	{ KEYCODE_P, 			DIK_P,				'P',			'P' },
	{ KEYCODE_OPENBRACE,	DIK_LBRACKET, 		0xdb,			'[' },
	{ KEYCODE_CLOSEBRACE,	DIK_RBRACKET, 		0xdd,			']' },
	{ KEYCODE_ENTER, 		DIK_RETURN, 		VK_RETURN, 		13 },
	{ KEYCODE_LCONTROL, 	DIK_LCONTROL, 		VK_LCONTROL, 	0 },
	{ KEYCODE_A, 			DIK_A,				'A',			'A' },
	{ KEYCODE_S, 			DIK_S,				'S',			'S' },
	{ KEYCODE_D, 			DIK_D,				'D',			'D' },
	{ KEYCODE_F, 			DIK_F,				'F',			'F' },
	{ KEYCODE_G, 			DIK_G,				'G',			'G' },
	{ KEYCODE_H, 			DIK_H,				'H',			'H' },
	{ KEYCODE_J, 			DIK_J,				'J',			'J' },
	{ KEYCODE_K, 			DIK_K,				'K',			'K' },
	{ KEYCODE_L, 			DIK_L,				'L',			'L' },
	{ KEYCODE_COLON, 		DIK_SEMICOLON,		0xba,			';' },
	{ KEYCODE_QUOTE, 		DIK_APOSTROPHE,		0xde,			'\'' },
	{ KEYCODE_TILDE, 		DIK_GRAVE, 			0xc0,			'`' },
	{ KEYCODE_LSHIFT, 		DIK_LSHIFT, 		VK_LSHIFT, 		0 },
	{ KEYCODE_BACKSLASH,	DIK_BACKSLASH, 		0xdc,			'\\' },
	{ KEYCODE_Z, 			DIK_Z,				'Z',			'Z' },
	{ KEYCODE_X, 			DIK_X,				'X',			'X' },
	{ KEYCODE_C, 			DIK_C,				'C',			'C' },
	{ KEYCODE_V, 			DIK_V,				'V',			'V' },
	{ KEYCODE_B, 			DIK_B,				'B',			'B' },
	{ KEYCODE_N, 			DIK_N,				'N',			'N' },
	{ KEYCODE_M, 			DIK_M,				'M',			'M' },
	{ KEYCODE_COMMA, 		DIK_COMMA,			0xbc,			',' },
	{ KEYCODE_STOP, 		DIK_PERIOD, 		0xbe,			'.' },
	{ KEYCODE_SLASH, 		DIK_SLASH, 			0xbf,			'/' },
	{ KEYCODE_RSHIFT, 		DIK_RSHIFT, 		VK_RSHIFT, 		0 },
	{ KEYCODE_ASTERISK, 	DIK_MULTIPLY, 		VK_MULTIPLY,	'*' },
	{ KEYCODE_LALT, 		DIK_LMENU, 			VK_LMENU, 		0 },
	{ KEYCODE_SPACE, 		DIK_SPACE, 			VK_SPACE,		' ' },
	{ KEYCODE_CAPSLOCK, 	DIK_CAPITAL, 		VK_CAPITAL, 	0 },
	{ KEYCODE_F1, 			DIK_F1,				VK_F1, 			0 },
	{ KEYCODE_F2, 			DIK_F2,				VK_F2, 			0 },
	{ KEYCODE_F3, 			DIK_F3,				VK_F3, 			0 },
	{ KEYCODE_F4, 			DIK_F4,				VK_F4, 			0 },
	{ KEYCODE_F5, 			DIK_F5,				VK_F5, 			0 },
	{ KEYCODE_F6, 			DIK_F6,				VK_F6, 			0 },
	{ KEYCODE_F7, 			DIK_F7,				VK_F7, 			0 },
	{ KEYCODE_F8, 			DIK_F8,				VK_F8, 			0 },
	{ KEYCODE_F9, 			DIK_F9,				VK_F9, 			0 },
	{ KEYCODE_F10, 			DIK_F10,			VK_F10, 		0 },
	{ KEYCODE_NUMLOCK, 		DIK_NUMLOCK,		VK_NUMLOCK, 	0 },
	{ KEYCODE_SCRLOCK, 		DIK_SCROLL,			VK_SCROLL, 		0 },
	{ KEYCODE_7_PAD, 		DIK_NUMPAD7,		VK_NUMPAD7, 	0 },
	{ KEYCODE_8_PAD, 		DIK_NUMPAD8,		VK_NUMPAD8, 	0 },
	{ KEYCODE_9_PAD, 		DIK_NUMPAD9,		VK_NUMPAD9, 	0 },
	{ KEYCODE_MINUS_PAD,	DIK_SUBTRACT,		VK_SUBTRACT, 	0 },
	{ KEYCODE_4_PAD, 		DIK_NUMPAD4,		VK_NUMPAD4, 	0 },
	{ KEYCODE_5_PAD, 		DIK_NUMPAD5,		VK_NUMPAD5, 	0 },
	{ KEYCODE_6_PAD, 		DIK_NUMPAD6,		VK_NUMPAD6, 	0 },
	{ KEYCODE_PLUS_PAD, 	DIK_ADD,			VK_ADD, 		0 },
	{ KEYCODE_1_PAD, 		DIK_NUMPAD1,		VK_NUMPAD1, 	0 },
	{ KEYCODE_2_PAD, 		DIK_NUMPAD2,		VK_NUMPAD2, 	0 },
	{ KEYCODE_3_PAD, 		DIK_NUMPAD3,		VK_NUMPAD3, 	0 },
	{ KEYCODE_0_PAD, 		DIK_NUMPAD0,		VK_NUMPAD0, 	0 },
	{ KEYCODE_DEL_PAD, 		DIK_DECIMAL,		VK_DECIMAL, 	0 },
	{ KEYCODE_F11, 			DIK_F11,			VK_F11, 		0 },
	{ KEYCODE_F12, 			DIK_F12,			VK_F12, 		0 },
	{ KEYCODE_F13, 			DIK_F13,			VK_F13, 		0 },
	{ KEYCODE_F14, 			DIK_F14,			VK_F14, 		0 },
	{ KEYCODE_F15, 			DIK_F15,			VK_F15, 		0 },
	{ KEYCODE_ENTER_PAD,	DIK_NUMPADENTER,	VK_RETURN, 		0 },
	{ KEYCODE_RCONTROL, 	DIK_RCONTROL,		VK_RCONTROL, 	0 },
	{ KEYCODE_SLASH_PAD,	DIK_DIVIDE,			VK_DIVIDE, 		0 },
	{ KEYCODE_PRTSCR, 		DIK_SYSRQ, 			0, 				0 },
	{ KEYCODE_RALT, 		DIK_RMENU,			VK_RMENU, 		0 },
	{ KEYCODE_HOME, 		DIK_HOME,			VK_HOME, 		0 },
	{ KEYCODE_UP, 			DIK_UP,				VK_UP, 			0 },
	{ KEYCODE_PGUP, 		DIK_PRIOR,			VK_PRIOR, 		0 },
	{ KEYCODE_LEFT, 		DIK_LEFT,			VK_LEFT, 		0 },
	{ KEYCODE_RIGHT, 		DIK_RIGHT,			VK_RIGHT, 		0 },
	{ KEYCODE_END, 			DIK_END,			VK_END, 		0 },
	{ KEYCODE_DOWN, 		DIK_DOWN,			VK_DOWN, 		0 },
	{ KEYCODE_PGDN, 		DIK_NEXT,			VK_NEXT, 		0 },
	{ KEYCODE_INSERT, 		DIK_INSERT,			VK_INSERT, 		0 },
	{ KEYCODE_DEL, 			DIK_DELETE,			VK_DELETE, 		0 },
	{ KEYCODE_LWIN, 		DIK_LWIN,			VK_LWIN, 		0 },
	{ KEYCODE_RWIN, 		DIK_RWIN,			VK_RWIN, 		0 },
	{ KEYCODE_MENU, 		DIK_APPS,			VK_APPS, 		0 },
	{ KEYCODE_PAUSE, 		DIK_PAUSE,			VK_PAUSE,		0 },
	{ KEYCODE_CANCEL,		0,					VK_CANCEL,		0 },

	// New keys introduced in Windows 2000. These have no MAME codes to
	// preserve compatibility with old config files that may refer to them
	// as e.g. FORWARD instead of e.g. KEYCODE_WEBFORWARD. They need table
	// entries anyway because otherwise they aren't recognized when
	// GetAsyncKeyState polling is used (as happens currently when MAME is
	// paused). Some codes are missing because the mapping to vkey codes
	// isn't clear, and MapVirtualKey is no help.

	{ CODE_OTHER_DIGITAL,	DIK_MUTE,			VK_VOLUME_MUTE,			0 },
	{ CODE_OTHER_DIGITAL,	DIK_VOLUMEDOWN,		VK_VOLUME_DOWN,			0 },
	{ CODE_OTHER_DIGITAL,	DIK_VOLUMEUP,		VK_VOLUME_UP,			0 },
	{ CODE_OTHER_DIGITAL,	DIK_WEBHOME,		VK_BROWSER_HOME,		0 },
	{ CODE_OTHER_DIGITAL,	DIK_WEBSEARCH,		VK_BROWSER_SEARCH,		0 },
	{ CODE_OTHER_DIGITAL,	DIK_WEBFAVORITES,	VK_BROWSER_FAVORITES,	0 },
	{ CODE_OTHER_DIGITAL,	DIK_WEBREFRESH,		VK_BROWSER_REFRESH,		0 },
	{ CODE_OTHER_DIGITAL,	DIK_WEBSTOP,		VK_BROWSER_STOP,		0 },
	{ CODE_OTHER_DIGITAL,	DIK_WEBFORWARD,		VK_BROWSER_FORWARD,		0 },
	{ CODE_OTHER_DIGITAL,	DIK_WEBBACK,		VK_BROWSER_BACK,		0 },
	{ CODE_OTHER_DIGITAL,	DIK_MAIL,			VK_LAUNCH_MAIL,			0 },
	{ CODE_OTHER_DIGITAL,	DIK_MEDIASELECT,	VK_LAUNCH_MEDIA_SELECT,	0 },

	{ -1 }
};


// master joystick translation table
static int joy_trans_table[][2] =
{
	// internal code                    MAME code
	{ JOYCODE(0, CODETYPE_AXIS_NEG, 0),	JOYCODE_1_LEFT },
	{ JOYCODE(0, CODETYPE_AXIS_POS, 0),	JOYCODE_1_RIGHT },
	{ JOYCODE(0, CODETYPE_AXIS_NEG, 1),	JOYCODE_1_UP },
	{ JOYCODE(0, CODETYPE_AXIS_POS, 1),	JOYCODE_1_DOWN },
	{ JOYCODE(0, CODETYPE_BUTTON, 0),	JOYCODE_1_BUTTON1 },
	{ JOYCODE(0, CODETYPE_BUTTON, 1),	JOYCODE_1_BUTTON2 },
	{ JOYCODE(0, CODETYPE_BUTTON, 2),	JOYCODE_1_BUTTON3 },
	{ JOYCODE(0, CODETYPE_BUTTON, 3),	JOYCODE_1_BUTTON4 },
	{ JOYCODE(0, CODETYPE_BUTTON, 4),	JOYCODE_1_BUTTON5 },
	{ JOYCODE(0, CODETYPE_BUTTON, 5),	JOYCODE_1_BUTTON6 },
	{ JOYCODE(0, CODETYPE_BUTTON, 6),	JOYCODE_1_BUTTON7 },
	{ JOYCODE(0, CODETYPE_BUTTON, 7),	JOYCODE_1_BUTTON8 },
	{ JOYCODE(0, CODETYPE_BUTTON, 8),	JOYCODE_1_BUTTON9 },
	{ JOYCODE(0, CODETYPE_BUTTON, 9),	JOYCODE_1_BUTTON10 },
	{ JOYCODE(0, CODETYPE_BUTTON, 10),	JOYCODE_1_BUTTON11 },
	{ JOYCODE(0, CODETYPE_BUTTON, 11),	JOYCODE_1_BUTTON12 },
	{ JOYCODE(0, CODETYPE_BUTTON, 12),	JOYCODE_1_BUTTON13 },
	{ JOYCODE(0, CODETYPE_BUTTON, 13),	JOYCODE_1_BUTTON14 },
	{ JOYCODE(0, CODETYPE_BUTTON, 14),	JOYCODE_1_BUTTON15 },
	{ JOYCODE(0, CODETYPE_BUTTON, 15),	JOYCODE_1_BUTTON16 },
	{ JOYCODE(0, CODETYPE_JOYAXIS, 0),	JOYCODE_1_ANALOG_X },
	{ JOYCODE(0, CODETYPE_JOYAXIS, 1),	JOYCODE_1_ANALOG_Y },
	{ JOYCODE(0, CODETYPE_JOYAXIS, 2),	JOYCODE_1_ANALOG_Z },

	{ JOYCODE(1, CODETYPE_AXIS_NEG, 0),	JOYCODE_2_LEFT },
	{ JOYCODE(1, CODETYPE_AXIS_POS, 0),	JOYCODE_2_RIGHT },
	{ JOYCODE(1, CODETYPE_AXIS_NEG, 1),	JOYCODE_2_UP },
	{ JOYCODE(1, CODETYPE_AXIS_POS, 1),	JOYCODE_2_DOWN },
	{ JOYCODE(1, CODETYPE_BUTTON, 0),	JOYCODE_2_BUTTON1 },
	{ JOYCODE(1, CODETYPE_BUTTON, 1),	JOYCODE_2_BUTTON2 },
	{ JOYCODE(1, CODETYPE_BUTTON, 2),	JOYCODE_2_BUTTON3 },
	{ JOYCODE(1, CODETYPE_BUTTON, 3),	JOYCODE_2_BUTTON4 },
	{ JOYCODE(1, CODETYPE_BUTTON, 4),	JOYCODE_2_BUTTON5 },
	{ JOYCODE(1, CODETYPE_BUTTON, 5),	JOYCODE_2_BUTTON6 },
	{ JOYCODE(1, CODETYPE_BUTTON, 6),	JOYCODE_2_BUTTON7 },
	{ JOYCODE(1, CODETYPE_BUTTON, 7),	JOYCODE_2_BUTTON8 },
	{ JOYCODE(1, CODETYPE_BUTTON, 8),	JOYCODE_2_BUTTON9 },
	{ JOYCODE(1, CODETYPE_BUTTON, 9),	JOYCODE_2_BUTTON10 },
	{ JOYCODE(1, CODETYPE_BUTTON, 10),	JOYCODE_2_BUTTON11 },
	{ JOYCODE(1, CODETYPE_BUTTON, 11),	JOYCODE_2_BUTTON12 },
	{ JOYCODE(1, CODETYPE_BUTTON, 12),	JOYCODE_2_BUTTON13 },
	{ JOYCODE(1, CODETYPE_BUTTON, 13),	JOYCODE_2_BUTTON14 },
	{ JOYCODE(1, CODETYPE_BUTTON, 14),	JOYCODE_2_BUTTON15 },
	{ JOYCODE(1, CODETYPE_BUTTON, 15),	JOYCODE_2_BUTTON16 },
	{ JOYCODE(1, CODETYPE_JOYAXIS, 0),	JOYCODE_2_ANALOG_X },
	{ JOYCODE(1, CODETYPE_JOYAXIS, 1),	JOYCODE_2_ANALOG_Y },
	{ JOYCODE(1, CODETYPE_JOYAXIS, 2),	JOYCODE_2_ANALOG_Z },

	{ JOYCODE(2, CODETYPE_AXIS_NEG, 0),	JOYCODE_3_LEFT },
	{ JOYCODE(2, CODETYPE_AXIS_POS, 0),	JOYCODE_3_RIGHT },
	{ JOYCODE(2, CODETYPE_AXIS_NEG, 1),	JOYCODE_3_UP },
	{ JOYCODE(2, CODETYPE_AXIS_POS, 1),	JOYCODE_3_DOWN },
	{ JOYCODE(2, CODETYPE_BUTTON, 0),	JOYCODE_3_BUTTON1 },
	{ JOYCODE(2, CODETYPE_BUTTON, 1),	JOYCODE_3_BUTTON2 },
	{ JOYCODE(2, CODETYPE_BUTTON, 2),	JOYCODE_3_BUTTON3 },
	{ JOYCODE(2, CODETYPE_BUTTON, 3),	JOYCODE_3_BUTTON4 },
	{ JOYCODE(2, CODETYPE_BUTTON, 4),	JOYCODE_3_BUTTON5 },
	{ JOYCODE(2, CODETYPE_BUTTON, 5),	JOYCODE_3_BUTTON6 },
	{ JOYCODE(2, CODETYPE_BUTTON, 6),	JOYCODE_3_BUTTON7 },
	{ JOYCODE(2, CODETYPE_BUTTON, 7),	JOYCODE_3_BUTTON8 },
	{ JOYCODE(2, CODETYPE_BUTTON, 8),	JOYCODE_3_BUTTON9 },
	{ JOYCODE(2, CODETYPE_BUTTON, 9),	JOYCODE_3_BUTTON10 },
	{ JOYCODE(2, CODETYPE_BUTTON, 10),	JOYCODE_3_BUTTON11 },
	{ JOYCODE(2, CODETYPE_BUTTON, 11),	JOYCODE_3_BUTTON12 },
	{ JOYCODE(2, CODETYPE_BUTTON, 12),	JOYCODE_3_BUTTON13 },
	{ JOYCODE(2, CODETYPE_BUTTON, 13),	JOYCODE_3_BUTTON14 },
	{ JOYCODE(2, CODETYPE_BUTTON, 14),	JOYCODE_3_BUTTON15 },
	{ JOYCODE(2, CODETYPE_BUTTON, 15),	JOYCODE_3_BUTTON16 },
	{ JOYCODE(2, CODETYPE_JOYAXIS, 0),	JOYCODE_3_ANALOG_X },
	{ JOYCODE(2, CODETYPE_JOYAXIS, 1),	JOYCODE_3_ANALOG_Y },
	{ JOYCODE(2, CODETYPE_JOYAXIS, 2),	JOYCODE_3_ANALOG_Z },

	{ JOYCODE(3, CODETYPE_AXIS_NEG, 0),	JOYCODE_4_LEFT },
	{ JOYCODE(3, CODETYPE_AXIS_POS, 0),	JOYCODE_4_RIGHT },
	{ JOYCODE(3, CODETYPE_AXIS_NEG, 1),	JOYCODE_4_UP },
	{ JOYCODE(3, CODETYPE_AXIS_POS, 1),	JOYCODE_4_DOWN },
	{ JOYCODE(3, CODETYPE_BUTTON, 0),	JOYCODE_4_BUTTON1 },
	{ JOYCODE(3, CODETYPE_BUTTON, 1),	JOYCODE_4_BUTTON2 },
	{ JOYCODE(3, CODETYPE_BUTTON, 2),	JOYCODE_4_BUTTON3 },
	{ JOYCODE(3, CODETYPE_BUTTON, 3),	JOYCODE_4_BUTTON4 },
	{ JOYCODE(3, CODETYPE_BUTTON, 4),	JOYCODE_4_BUTTON5 },
	{ JOYCODE(3, CODETYPE_BUTTON, 5),	JOYCODE_4_BUTTON6 },
	{ JOYCODE(3, CODETYPE_BUTTON, 6),	JOYCODE_4_BUTTON7 },
	{ JOYCODE(3, CODETYPE_BUTTON, 7),	JOYCODE_4_BUTTON8 },
	{ JOYCODE(3, CODETYPE_BUTTON, 8),	JOYCODE_4_BUTTON9 },
	{ JOYCODE(3, CODETYPE_BUTTON, 9),	JOYCODE_4_BUTTON10 },
	{ JOYCODE(3, CODETYPE_BUTTON, 10),	JOYCODE_4_BUTTON11 },
	{ JOYCODE(3, CODETYPE_BUTTON, 11),	JOYCODE_4_BUTTON12 },
	{ JOYCODE(3, CODETYPE_BUTTON, 12),	JOYCODE_4_BUTTON13 },
	{ JOYCODE(3, CODETYPE_BUTTON, 13),	JOYCODE_4_BUTTON14 },
	{ JOYCODE(3, CODETYPE_BUTTON, 14),	JOYCODE_4_BUTTON15 },
	{ JOYCODE(3, CODETYPE_BUTTON, 15),	JOYCODE_4_BUTTON16 },
	{ JOYCODE(3, CODETYPE_JOYAXIS, 0),	JOYCODE_4_ANALOG_X },
	{ JOYCODE(3, CODETYPE_JOYAXIS, 1),	JOYCODE_4_ANALOG_Y },
	{ JOYCODE(3, CODETYPE_JOYAXIS, 2),	JOYCODE_4_ANALOG_Z },

	{ JOYCODE(4, CODETYPE_AXIS_NEG, 0),	JOYCODE_5_LEFT },
	{ JOYCODE(4, CODETYPE_AXIS_POS, 0),	JOYCODE_5_RIGHT },
	{ JOYCODE(4, CODETYPE_AXIS_NEG, 1),	JOYCODE_5_UP },
	{ JOYCODE(4, CODETYPE_AXIS_POS, 1),	JOYCODE_5_DOWN },
	{ JOYCODE(4, CODETYPE_BUTTON, 0),	JOYCODE_5_BUTTON1 },
	{ JOYCODE(4, CODETYPE_BUTTON, 1),	JOYCODE_5_BUTTON2 },
	{ JOYCODE(4, CODETYPE_BUTTON, 2),	JOYCODE_5_BUTTON3 },
	{ JOYCODE(4, CODETYPE_BUTTON, 3),	JOYCODE_5_BUTTON4 },
	{ JOYCODE(4, CODETYPE_BUTTON, 4),	JOYCODE_5_BUTTON5 },
	{ JOYCODE(4, CODETYPE_BUTTON, 5),	JOYCODE_5_BUTTON6 },
	{ JOYCODE(4, CODETYPE_BUTTON, 6),	JOYCODE_5_BUTTON7 },
	{ JOYCODE(4, CODETYPE_BUTTON, 7),	JOYCODE_5_BUTTON8 },
	{ JOYCODE(4, CODETYPE_BUTTON, 8),	JOYCODE_5_BUTTON9 },
	{ JOYCODE(4, CODETYPE_BUTTON, 9),	JOYCODE_5_BUTTON10 },
	{ JOYCODE(4, CODETYPE_BUTTON, 10),	JOYCODE_5_BUTTON11 },
	{ JOYCODE(4, CODETYPE_BUTTON, 11),	JOYCODE_5_BUTTON12 },
	{ JOYCODE(4, CODETYPE_BUTTON, 12),	JOYCODE_5_BUTTON13 },
	{ JOYCODE(4, CODETYPE_BUTTON, 13),	JOYCODE_5_BUTTON14 },
	{ JOYCODE(4, CODETYPE_BUTTON, 14),	JOYCODE_5_BUTTON15 },
	{ JOYCODE(4, CODETYPE_BUTTON, 15),	JOYCODE_5_BUTTON16 },
	{ JOYCODE(4, CODETYPE_JOYAXIS, 0),	JOYCODE_5_ANALOG_X },
	{ JOYCODE(4, CODETYPE_JOYAXIS, 1), 	JOYCODE_5_ANALOG_Y },
	{ JOYCODE(4, CODETYPE_JOYAXIS, 2),	JOYCODE_5_ANALOG_Z },

	{ JOYCODE(5, CODETYPE_AXIS_NEG, 0),	JOYCODE_6_LEFT },
	{ JOYCODE(5, CODETYPE_AXIS_POS, 0),	JOYCODE_6_RIGHT },
	{ JOYCODE(5, CODETYPE_AXIS_NEG, 1),	JOYCODE_6_UP },
	{ JOYCODE(5, CODETYPE_AXIS_POS, 1),	JOYCODE_6_DOWN },
	{ JOYCODE(5, CODETYPE_BUTTON, 0),	JOYCODE_6_BUTTON1 },
	{ JOYCODE(5, CODETYPE_BUTTON, 1),	JOYCODE_6_BUTTON2 },
	{ JOYCODE(5, CODETYPE_BUTTON, 2),	JOYCODE_6_BUTTON3 },
	{ JOYCODE(5, CODETYPE_BUTTON, 3),	JOYCODE_6_BUTTON4 },
	{ JOYCODE(5, CODETYPE_BUTTON, 4),	JOYCODE_6_BUTTON5 },
	{ JOYCODE(5, CODETYPE_BUTTON, 5),	JOYCODE_6_BUTTON6 },
	{ JOYCODE(5, CODETYPE_BUTTON, 6),	JOYCODE_6_BUTTON7 },
	{ JOYCODE(5, CODETYPE_BUTTON, 7),	JOYCODE_6_BUTTON8 },
	{ JOYCODE(5, CODETYPE_BUTTON, 8),	JOYCODE_6_BUTTON9 },
	{ JOYCODE(5, CODETYPE_BUTTON, 9),	JOYCODE_6_BUTTON10 },
	{ JOYCODE(5, CODETYPE_BUTTON, 10),	JOYCODE_6_BUTTON11 },
	{ JOYCODE(5, CODETYPE_BUTTON, 11),	JOYCODE_6_BUTTON12 },
	{ JOYCODE(5, CODETYPE_BUTTON, 12),	JOYCODE_6_BUTTON13 },
	{ JOYCODE(5, CODETYPE_BUTTON, 13),	JOYCODE_6_BUTTON14 },
	{ JOYCODE(5, CODETYPE_BUTTON, 14),	JOYCODE_6_BUTTON15 },
	{ JOYCODE(5, CODETYPE_BUTTON, 15),	JOYCODE_6_BUTTON16 },
	{ JOYCODE(5, CODETYPE_JOYAXIS, 0),	JOYCODE_6_ANALOG_X },
	{ JOYCODE(5, CODETYPE_JOYAXIS, 1),	JOYCODE_6_ANALOG_Y },
	{ JOYCODE(5, CODETYPE_JOYAXIS, 2),	JOYCODE_6_ANALOG_Z },

	{ JOYCODE(6, CODETYPE_AXIS_NEG, 0),	JOYCODE_7_LEFT },
	{ JOYCODE(6, CODETYPE_AXIS_POS, 0),	JOYCODE_7_RIGHT },
	{ JOYCODE(6, CODETYPE_AXIS_NEG, 1),	JOYCODE_7_UP },
	{ JOYCODE(6, CODETYPE_AXIS_POS, 1),	JOYCODE_7_DOWN },
	{ JOYCODE(6, CODETYPE_BUTTON, 0),	JOYCODE_7_BUTTON1 },
	{ JOYCODE(6, CODETYPE_BUTTON, 1),	JOYCODE_7_BUTTON2 },
	{ JOYCODE(6, CODETYPE_BUTTON, 2),	JOYCODE_7_BUTTON3 },
	{ JOYCODE(6, CODETYPE_BUTTON, 3),	JOYCODE_7_BUTTON4 },
	{ JOYCODE(6, CODETYPE_BUTTON, 4),	JOYCODE_7_BUTTON5 },
	{ JOYCODE(6, CODETYPE_BUTTON, 5),	JOYCODE_7_BUTTON6 },
	{ JOYCODE(6, CODETYPE_BUTTON, 6),	JOYCODE_7_BUTTON7 },
	{ JOYCODE(6, CODETYPE_BUTTON, 7),	JOYCODE_7_BUTTON8 },
	{ JOYCODE(6, CODETYPE_BUTTON, 8),	JOYCODE_7_BUTTON9 },
	{ JOYCODE(6, CODETYPE_BUTTON, 9),	JOYCODE_7_BUTTON10 },
	{ JOYCODE(6, CODETYPE_BUTTON, 10),	JOYCODE_7_BUTTON11 },
	{ JOYCODE(6, CODETYPE_BUTTON, 11),	JOYCODE_7_BUTTON12 },
	{ JOYCODE(6, CODETYPE_BUTTON, 12),	JOYCODE_7_BUTTON13 },
	{ JOYCODE(6, CODETYPE_BUTTON, 13),	JOYCODE_7_BUTTON14 },
	{ JOYCODE(6, CODETYPE_BUTTON, 14),	JOYCODE_7_BUTTON15 },
	{ JOYCODE(6, CODETYPE_BUTTON, 15),	JOYCODE_7_BUTTON16 },
	{ JOYCODE(6, CODETYPE_JOYAXIS, 0),	JOYCODE_7_ANALOG_X },
	{ JOYCODE(6, CODETYPE_JOYAXIS, 1),	JOYCODE_7_ANALOG_Y },
	{ JOYCODE(6, CODETYPE_JOYAXIS, 2),	JOYCODE_7_ANALOG_Z },

	{ JOYCODE(7, CODETYPE_AXIS_NEG, 0),	JOYCODE_8_LEFT },
	{ JOYCODE(7, CODETYPE_AXIS_POS, 0),	JOYCODE_8_RIGHT },
	{ JOYCODE(7, CODETYPE_AXIS_NEG, 1),	JOYCODE_8_UP },
	{ JOYCODE(7, CODETYPE_AXIS_POS, 1),	JOYCODE_8_DOWN },
	{ JOYCODE(7, CODETYPE_BUTTON, 0),	JOYCODE_8_BUTTON1 },
	{ JOYCODE(7, CODETYPE_BUTTON, 1),	JOYCODE_8_BUTTON2 },
	{ JOYCODE(7, CODETYPE_BUTTON, 2),	JOYCODE_8_BUTTON3 },
	{ JOYCODE(7, CODETYPE_BUTTON, 3),	JOYCODE_8_BUTTON4 },
	{ JOYCODE(7, CODETYPE_BUTTON, 4),	JOYCODE_8_BUTTON5 },
	{ JOYCODE(7, CODETYPE_BUTTON, 5),	JOYCODE_8_BUTTON6 },
	{ JOYCODE(7, CODETYPE_BUTTON, 6),	JOYCODE_8_BUTTON7 },
	{ JOYCODE(7, CODETYPE_BUTTON, 7),	JOYCODE_8_BUTTON8 },
	{ JOYCODE(7, CODETYPE_BUTTON, 8),	JOYCODE_8_BUTTON9 },
	{ JOYCODE(7, CODETYPE_BUTTON, 9),	JOYCODE_8_BUTTON10 },
	{ JOYCODE(7, CODETYPE_BUTTON, 10),	JOYCODE_8_BUTTON11 },
	{ JOYCODE(7, CODETYPE_BUTTON, 11),	JOYCODE_8_BUTTON12 },
	{ JOYCODE(7, CODETYPE_BUTTON, 12),	JOYCODE_8_BUTTON13 },
	{ JOYCODE(7, CODETYPE_BUTTON, 13),	JOYCODE_8_BUTTON14 },
	{ JOYCODE(7, CODETYPE_BUTTON, 14),	JOYCODE_8_BUTTON15 },
	{ JOYCODE(7, CODETYPE_BUTTON, 15),	JOYCODE_8_BUTTON16 },
	{ JOYCODE(7, CODETYPE_JOYAXIS, 0),	JOYCODE_8_ANALOG_X },
	{ JOYCODE(7, CODETYPE_JOYAXIS, 1),	JOYCODE_8_ANALOG_Y },
	{ JOYCODE(7, CODETYPE_JOYAXIS, 2),	JOYCODE_8_ANALOG_Z },

	{ JOYCODE(0, CODETYPE_MOUSEBUTTON, 0), 	MOUSECODE_1_BUTTON1 },
	{ JOYCODE(0, CODETYPE_MOUSEBUTTON, 1), 	MOUSECODE_1_BUTTON2 },
	{ JOYCODE(0, CODETYPE_MOUSEBUTTON, 2), 	MOUSECODE_1_BUTTON3 },
	{ JOYCODE(0, CODETYPE_MOUSEBUTTON, 3), 	MOUSECODE_1_BUTTON4 },
	{ JOYCODE(0, CODETYPE_MOUSEBUTTON, 4), 	MOUSECODE_1_BUTTON5 },
	{ JOYCODE(0, CODETYPE_MOUSEAXIS, 0),	MOUSECODE_1_ANALOG_X },
	{ JOYCODE(0, CODETYPE_MOUSEAXIS, 1),	MOUSECODE_1_ANALOG_Y },
	{ JOYCODE(0, CODETYPE_MOUSEAXIS, 2),	MOUSECODE_1_ANALOG_Z },

	{ JOYCODE(1, CODETYPE_MOUSEBUTTON, 0), 	MOUSECODE_2_BUTTON1 },
	{ JOYCODE(1, CODETYPE_MOUSEBUTTON, 1), 	MOUSECODE_2_BUTTON2 },
	{ JOYCODE(1, CODETYPE_MOUSEBUTTON, 2), 	MOUSECODE_2_BUTTON3 },
	{ JOYCODE(1, CODETYPE_MOUSEBUTTON, 3), 	MOUSECODE_2_BUTTON4 },
	{ JOYCODE(1, CODETYPE_MOUSEBUTTON, 4), 	MOUSECODE_2_BUTTON5 },
	{ JOYCODE(1, CODETYPE_MOUSEAXIS, 0),	MOUSECODE_2_ANALOG_X },
	{ JOYCODE(1, CODETYPE_MOUSEAXIS, 1),	MOUSECODE_2_ANALOG_Y },
	{ JOYCODE(1, CODETYPE_MOUSEAXIS, 2),	MOUSECODE_2_ANALOG_Z },

	{ JOYCODE(2, CODETYPE_MOUSEBUTTON, 0), 	MOUSECODE_3_BUTTON1 },
	{ JOYCODE(2, CODETYPE_MOUSEBUTTON, 1), 	MOUSECODE_3_BUTTON2 },
	{ JOYCODE(2, CODETYPE_MOUSEBUTTON, 2), 	MOUSECODE_3_BUTTON3 },
	{ JOYCODE(2, CODETYPE_MOUSEBUTTON, 3), 	MOUSECODE_3_BUTTON4 },
	{ JOYCODE(2, CODETYPE_MOUSEBUTTON, 4), 	MOUSECODE_3_BUTTON5 },
	{ JOYCODE(2, CODETYPE_MOUSEAXIS, 0),	MOUSECODE_3_ANALOG_X },
	{ JOYCODE(2, CODETYPE_MOUSEAXIS, 1),	MOUSECODE_3_ANALOG_Y },
	{ JOYCODE(2, CODETYPE_MOUSEAXIS, 2),	MOUSECODE_3_ANALOG_Z },

	{ JOYCODE(3, CODETYPE_MOUSEBUTTON, 0), 	MOUSECODE_4_BUTTON1 },
	{ JOYCODE(3, CODETYPE_MOUSEBUTTON, 1), 	MOUSECODE_4_BUTTON2 },
	{ JOYCODE(3, CODETYPE_MOUSEBUTTON, 2), 	MOUSECODE_4_BUTTON3 },
	{ JOYCODE(3, CODETYPE_MOUSEBUTTON, 3), 	MOUSECODE_4_BUTTON4 },
	{ JOYCODE(3, CODETYPE_MOUSEBUTTON, 4), 	MOUSECODE_4_BUTTON5 },
	{ JOYCODE(3, CODETYPE_MOUSEAXIS, 0),	MOUSECODE_4_ANALOG_X },
	{ JOYCODE(3, CODETYPE_MOUSEAXIS, 1),	MOUSECODE_4_ANALOG_Y },
	{ JOYCODE(3, CODETYPE_MOUSEAXIS, 2),	MOUSECODE_4_ANALOG_Z },

	{ JOYCODE(4, CODETYPE_MOUSEBUTTON, 0), 	MOUSECODE_5_BUTTON1 },
	{ JOYCODE(4, CODETYPE_MOUSEBUTTON, 1), 	MOUSECODE_5_BUTTON2 },
	{ JOYCODE(4, CODETYPE_MOUSEBUTTON, 2), 	MOUSECODE_5_BUTTON3 },
	{ JOYCODE(4, CODETYPE_MOUSEBUTTON, 3), 	MOUSECODE_5_BUTTON4 },
	{ JOYCODE(4, CODETYPE_MOUSEBUTTON, 4), 	MOUSECODE_5_BUTTON5 },
	{ JOYCODE(4, CODETYPE_MOUSEAXIS, 0),	MOUSECODE_5_ANALOG_X },
	{ JOYCODE(4, CODETYPE_MOUSEAXIS, 1),	MOUSECODE_5_ANALOG_Y },
	{ JOYCODE(4, CODETYPE_MOUSEAXIS, 2),	MOUSECODE_5_ANALOG_Z },

	{ JOYCODE(5, CODETYPE_MOUSEBUTTON, 0), 	MOUSECODE_6_BUTTON1 },
	{ JOYCODE(5, CODETYPE_MOUSEBUTTON, 1), 	MOUSECODE_6_BUTTON2 },
	{ JOYCODE(5, CODETYPE_MOUSEBUTTON, 2), 	MOUSECODE_6_BUTTON3 },
	{ JOYCODE(5, CODETYPE_MOUSEBUTTON, 3), 	MOUSECODE_6_BUTTON4 },
	{ JOYCODE(5, CODETYPE_MOUSEBUTTON, 4), 	MOUSECODE_6_BUTTON5 },
	{ JOYCODE(5, CODETYPE_MOUSEAXIS, 0),	MOUSECODE_6_ANALOG_X },
	{ JOYCODE(5, CODETYPE_MOUSEAXIS, 1),	MOUSECODE_6_ANALOG_Y },
	{ JOYCODE(5, CODETYPE_MOUSEAXIS, 2),	MOUSECODE_6_ANALOG_Z },

	{ JOYCODE(6, CODETYPE_MOUSEBUTTON, 0), 	MOUSECODE_7_BUTTON1 },
	{ JOYCODE(6, CODETYPE_MOUSEBUTTON, 1), 	MOUSECODE_7_BUTTON2 },
	{ JOYCODE(6, CODETYPE_MOUSEBUTTON, 2), 	MOUSECODE_7_BUTTON3 },
	{ JOYCODE(6, CODETYPE_MOUSEBUTTON, 3), 	MOUSECODE_7_BUTTON4 },
	{ JOYCODE(6, CODETYPE_MOUSEBUTTON, 4), 	MOUSECODE_7_BUTTON5 },
	{ JOYCODE(6, CODETYPE_MOUSEAXIS, 0),	MOUSECODE_7_ANALOG_X },
	{ JOYCODE(6, CODETYPE_MOUSEAXIS, 1),	MOUSECODE_7_ANALOG_Y },
	{ JOYCODE(6, CODETYPE_MOUSEAXIS, 2),	MOUSECODE_7_ANALOG_Z },

	{ JOYCODE(7, CODETYPE_MOUSEBUTTON, 0), 	MOUSECODE_8_BUTTON1 },
	{ JOYCODE(7, CODETYPE_MOUSEBUTTON, 1), 	MOUSECODE_8_BUTTON2 },
	{ JOYCODE(7, CODETYPE_MOUSEBUTTON, 2), 	MOUSECODE_8_BUTTON3 },
	{ JOYCODE(7, CODETYPE_MOUSEBUTTON, 3), 	MOUSECODE_8_BUTTON4 },
	{ JOYCODE(7, CODETYPE_MOUSEBUTTON, 4), 	MOUSECODE_8_BUTTON5 },
	{ JOYCODE(7, CODETYPE_MOUSEAXIS, 0),	MOUSECODE_8_ANALOG_X },
	{ JOYCODE(7, CODETYPE_MOUSEAXIS, 1),	MOUSECODE_8_ANALOG_Y },
	{ JOYCODE(7, CODETYPE_MOUSEAXIS, 2),	MOUSECODE_8_ANALOG_Z },

	{ JOYCODE(0, CODETYPE_GUNAXIS, 0),		GUNCODE_1_ANALOG_X },
	{ JOYCODE(0, CODETYPE_GUNAXIS, 1),		GUNCODE_1_ANALOG_Y },

	{ JOYCODE(1, CODETYPE_GUNAXIS, 0),		GUNCODE_2_ANALOG_X },
	{ JOYCODE(1, CODETYPE_GUNAXIS, 1),		GUNCODE_2_ANALOG_Y },

	{ JOYCODE(2, CODETYPE_GUNAXIS, 0),		GUNCODE_3_ANALOG_X },
	{ JOYCODE(2, CODETYPE_GUNAXIS, 1),		GUNCODE_3_ANALOG_Y },

	{ JOYCODE(3, CODETYPE_GUNAXIS, 0),		GUNCODE_4_ANALOG_X },
	{ JOYCODE(3, CODETYPE_GUNAXIS, 1),		GUNCODE_4_ANALOG_Y },

	{ JOYCODE(4, CODETYPE_GUNAXIS, 0),		GUNCODE_5_ANALOG_X },
	{ JOYCODE(4, CODETYPE_GUNAXIS, 1),		GUNCODE_5_ANALOG_Y },

	{ JOYCODE(5, CODETYPE_GUNAXIS, 0),		GUNCODE_6_ANALOG_X },
	{ JOYCODE(5, CODETYPE_GUNAXIS, 1),		GUNCODE_6_ANALOG_Y },

	{ JOYCODE(6, CODETYPE_GUNAXIS, 0),		GUNCODE_7_ANALOG_X },
	{ JOYCODE(6, CODETYPE_GUNAXIS, 1),		GUNCODE_7_ANALOG_Y },

	{ JOYCODE(7, CODETYPE_GUNAXIS, 0),		GUNCODE_8_ANALOG_X },
	{ JOYCODE(7, CODETYPE_GUNAXIS, 1),		GUNCODE_8_ANALOG_Y },
};



//============================================================
//  autoselect_analog_devices
//============================================================

static void autoselect_analog_devices(const input_port_entry *inp, int type1, int type2, int type3, int anatype, const char *ananame)
{
	// loop over input ports
	for ( ; inp->type != IPT_END; inp++)

		// if this port type is in use, apply the autoselect criteria
		if ((type1 != 0 && inp->type == type1) ||
			(type2 != 0 && inp->type == type2) ||
			(type3 != 0 && inp->type == type3))
		{
			// autoenable mouse devices
			if (analog_type[anatype] == SELECT_TYPE_MOUSE && !win_use_mouse)
			{
				win_use_mouse = 1;
				verbose_printf("Input: Autoenabling mice due to presence of a %s\n", ananame);
			}

			// autoenable joystick devices
			if (analog_type[anatype] == SELECT_TYPE_JOYSTICK && !use_joystick)
			{
				use_joystick = 1;
				verbose_printf("Input: Autoenabling joysticks due to presence of a %s\n", ananame);
			}

			// autoenable lightgun devices
			if (analog_type[anatype] == SELECT_TYPE_LIGHTGUN && !use_lightgun)
			{
				use_lightgun = 1;
				verbose_printf("Input: Autoenabling lightguns due to presence of a %s\n", ananame);
			}

			// all done
			break;
		}
}



//============================================================
//  enum_keyboard_callback
//============================================================

static BOOL CALLBACK enum_keyboard_callback(LPCDIDEVICEINSTANCE instance, LPVOID ref)
{
	HRESULT result;

	// if we're not out of mice, log this one
	if (keyboard_count >= MAX_KEYBOARDS)
		goto out_of_keyboards;

	// attempt to create a device
	result = IDirectInput_CreateDevice(dinput, &instance->guidInstance, &keyboard_device[keyboard_count], NULL);
	if (result != DI_OK)
		goto cant_create_device;

	// try to get a version 2 device for it
	result = IDirectInputDevice_QueryInterface(keyboard_device[keyboard_count], &IID_IDirectInputDevice2, (void **)&keyboard_device2[keyboard_count]);
	if (result != DI_OK)
		keyboard_device2[keyboard_count] = NULL;

	// get the caps
	keyboard_caps[keyboard_count].dwSize = STRUCTSIZE(DIDEVCAPS);
	result = IDirectInputDevice_GetCapabilities(keyboard_device[keyboard_count], &keyboard_caps[keyboard_count]);
	if (result != DI_OK)
		goto cant_get_caps;

	// attempt to set the data format
	result = IDirectInputDevice_SetDataFormat(keyboard_device[keyboard_count], &c_dfDIKeyboard);
	if (result != DI_OK)
		goto cant_set_format;

	// set the cooperative level
	result = IDirectInputDevice_SetCooperativeLevel(keyboard_device[keyboard_count], win_window_list->hwnd,
					DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	if (result != DI_OK)
		goto cant_set_coop_level;

	// increment the count
	keyboard_count++;
	return DIENUM_CONTINUE;

cant_set_coop_level:
cant_set_format:
cant_get_caps:
	if (keyboard_device2[keyboard_count])
		IDirectInputDevice_Release(keyboard_device2[keyboard_count]);
	IDirectInputDevice_Release(keyboard_device[keyboard_count]);
cant_create_device:
out_of_keyboards:
	return DIENUM_CONTINUE;
}



//============================================================
//  enum_mouse_callback
//============================================================

static BOOL CALLBACK enum_mouse_callback(LPCDIDEVICEINSTANCE instance, LPVOID ref)
{
	DIPROPDWORD value;
	HRESULT result;
	char *utf8_instance_name;

	// if we're not out of mice, log this one
	if (mouse_count > MAX_MICE)
		goto out_of_mice;

	// attempt to create a device
	result = IDirectInput_CreateDevice(dinput, &instance->guidInstance, &mouse_device[mouse_count], NULL);
	if (result != DI_OK)
		goto cant_create_device;

	// try to get a version 2 device for it
	result = IDirectInputDevice_QueryInterface(mouse_device[mouse_count], &IID_IDirectInputDevice2, (void **)&mouse_device2[mouse_count]);
	if (result != DI_OK)
		mouse_device2[mouse_count] = NULL;

	// remember the name
	utf8_instance_name = utf8_from_tstring(instance->tszInstanceName);
	if (utf8_instance_name == NULL)
		goto cant_alloc_memory;
	strcpy(mouse_name[mouse_count], utf8_instance_name);
	free(utf8_instance_name);

	// get the caps
	mouse_caps[mouse_count].dwSize = STRUCTSIZE(DIDEVCAPS);
	result = IDirectInputDevice_GetCapabilities(mouse_device[mouse_count], &mouse_caps[mouse_count]);
	if (result != DI_OK)
		goto cant_get_caps;

	// set relative mode
	value.diph.dwSize = sizeof(DIPROPDWORD);
	value.diph.dwHeaderSize = sizeof(value.diph);
	value.diph.dwObj = 0;
	value.diph.dwHow = DIPH_DEVICE;
	value.dwData = DIPROPAXISMODE_REL;
	result = IDirectInputDevice_SetProperty(mouse_device[mouse_count], DIPROP_AXISMODE, &value.diph);
	if (result != DI_OK)
		goto cant_set_axis_mode;

	// attempt to set the data format
	result = IDirectInputDevice_SetDataFormat(mouse_device[mouse_count], &c_dfDIMouse2);
	if (result != DI_OK)
	{
		result = IDirectInputDevice_SetDataFormat(mouse_device[mouse_count], &c_dfDIMouse);
		if (result != DI_OK)
			goto cant_set_format;
		mouse_num_of_buttons = 4;
		mouse_state_size = sizeof(DIMOUSESTATE);
	}

	// set the cooperative level
	if (use_lightgun)
		result = IDirectInputDevice_SetCooperativeLevel(mouse_device[mouse_count], win_window_list->hwnd,
					DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	else
		result = IDirectInputDevice_SetCooperativeLevel(mouse_device[mouse_count], win_window_list->hwnd,
					DISCL_FOREGROUND | DISCL_EXCLUSIVE);

	if (result != DI_OK)
		goto cant_set_coop_level;

	// increment the count
	if (use_lightgun)
		lightgun_count++;
	mouse_count++;
	return DIENUM_CONTINUE;

cant_set_coop_level:
cant_set_format:
cant_set_axis_mode:
cant_get_caps:
cant_alloc_memory:
	if (mouse_device2[mouse_count])
		IDirectInputDevice_Release(mouse_device2[mouse_count]);
	IDirectInputDevice_Release(mouse_device[mouse_count]);
cant_create_device:
out_of_mice:
	return DIENUM_CONTINUE;
}



//============================================================
//  remove_dx_system_mouse
//============================================================

static void remove_dx_system_mouse(void)
{
	int i;
	LPDIRECTINPUTDEVICE  sys_mouse_device;
	LPDIRECTINPUTDEVICE2 sys_mouse_device2;
	DIDEVCAPS sys_mouse_caps;
	char sys_mouse_name[MAX_PATH];

	// store system mouse info so it does not get lost
	sys_mouse_device  = mouse_device[0];
	sys_mouse_device2 = mouse_device2[0];
	sys_mouse_caps = mouse_caps[0];

	if (mouse_count < 2) goto setup_sys_mouse;

	mouse_count--;

	// shift mouse list
	for (i = 0; i < mouse_count; i++)
	{
		if (mouse_device2[i+1])
			mouse_device2[i] = mouse_device2[i+1];
		mouse_device[i] = mouse_device[i+1];
		mouse_caps[i] = mouse_caps[i+1];
		strcpy(mouse_name[i], mouse_name[i+1]);
	}

setup_sys_mouse:
	// system mouse will be stored at the end of the list
	mouse_device[MAX_MICE]  = sys_mouse_device;
	mouse_device2[MAX_MICE] = sys_mouse_device2;
	mouse_caps[MAX_MICE] = sys_mouse_caps;
	strcpy(mouse_name[MAX_MICE], sys_mouse_name);

	return;
}



//============================================================
//  enum_joystick_callback
//============================================================

static BOOL CALLBACK enum_joystick_callback(LPCDIDEVICEINSTANCE instance, LPVOID ref)
{
	DIPROPDWORD value;
	HRESULT result = DI_OK;
	DWORD flags;
	char *utf8_instance_name;

	// if we're not out of mice, log this one
	if (joystick_count >= MAX_JOYSTICKS)
		goto out_of_joysticks;

	// attempt to create a device
	result = IDirectInput_CreateDevice(dinput, &instance->guidInstance, &joystick_device[joystick_count], NULL);
	if (result != DI_OK)
		goto cant_create_device;

	// try to get a version 2 device for it
	result = IDirectInputDevice_QueryInterface(joystick_device[joystick_count], &IID_IDirectInputDevice2, (void **)&joystick_device2[joystick_count]);
	if (result != DI_OK)
		joystick_device2[joystick_count] = NULL;

	// remember the name
	utf8_instance_name = utf8_from_tstring(instance->tszInstanceName);
	if (utf8_instance_name == NULL)
		goto cant_alloc_memory;
	strcpy(joystick_name[joystick_count], utf8_instance_name);
	free(utf8_instance_name);

	// get the caps
	joystick_caps[joystick_count].dwSize = STRUCTSIZE(DIDEVCAPS);
	result = IDirectInputDevice_GetCapabilities(joystick_device[joystick_count], &joystick_caps[joystick_count]);
	if (result != DI_OK)
		goto cant_get_caps;

	// set absolute mode
	value.diph.dwSize = sizeof(DIPROPDWORD);
	value.diph.dwHeaderSize = sizeof(value.diph);
	value.diph.dwObj = 0;
	value.diph.dwHow = DIPH_DEVICE;
	value.dwData = DIPROPAXISMODE_ABS;
	result = IDirectInputDevice_SetProperty(joystick_device[joystick_count], DIPROP_AXISMODE, &value.diph);
 	if (result != DI_OK)
		goto cant_set_axis_mode;

	// attempt to set the data format
	result = IDirectInputDevice_SetDataFormat(joystick_device[joystick_count], &c_dfDIJoystick);
	if (result != DI_OK)
		goto cant_set_format;

	// set the cooperative level
#if HAS_WINDOW_MENU
	flags = DISCL_BACKGROUND | DISCL_EXCLUSIVE;
#else
	flags = DISCL_FOREGROUND | DISCL_EXCLUSIVE;
#endif
	result = IDirectInputDevice_SetCooperativeLevel(joystick_device[joystick_count], win_window_list->hwnd, flags);
	if (result != DI_OK)
		goto cant_set_coop_level;

	// increment the count
	joystick_count++;
	return DIENUM_CONTINUE;

cant_set_coop_level:
cant_set_format:
cant_set_axis_mode:
cant_get_caps:
cant_alloc_memory:
	if (joystick_device2[joystick_count])
		IDirectInputDevice_Release(joystick_device2[joystick_count]);
	IDirectInputDevice_Release(joystick_device[joystick_count]);
cant_create_device:
out_of_joysticks:
	return DIENUM_CONTINUE;
}



//============================================================
//  wininput_init
//============================================================

int wininput_init(running_machine *machine)
{
	const input_port_entry *inp;
	HRESULT result;

	win_input_handle = GetStdHandle(STD_INPUT_HANDLE);
	add_pause_callback(machine, win_pause_input);
	add_exit_callback(machine, wininput_exit);

	// allocate a lock
	raw_mouse_lock = osd_lock_alloc();

	// decode the options
	extract_input_config();

	// first attempt to initialize DirectInput
	dinput_version = DIRECTINPUT_VERSION;
	result = DirectInputCreate(GetModuleHandle(NULL), dinput_version, &dinput, NULL);
	if (result != DI_OK)
	{
		dinput_version = 0x0500;
		result = DirectInputCreate(GetModuleHandle(NULL), dinput_version, &dinput, NULL);
		if (result != DI_OK)
		{
			dinput_version = 0x0300;
			result = DirectInputCreate(GetModuleHandle(NULL), dinput_version, &dinput, NULL);
			if (result != DI_OK)
				goto cant_create_dinput;
		}
	}
	verbose_printf("DirectInput: Using DirectInput %d\n", dinput_version >> 8);

	// enable devices based on autoselect
	if (Machine != NULL && Machine->gamedrv != NULL)
	{
		begin_resource_tracking();
		inp = input_port_allocate(Machine->gamedrv->ipt, NULL);
		autoselect_analog_devices(inp, IPT_PADDLE,     IPT_PADDLE_V,    0,             ANALOG_TYPE_PADDLE,    "paddle");
		autoselect_analog_devices(inp, IPT_AD_STICK_X, IPT_AD_STICK_Y,  IPT_AD_STICK_Z,ANALOG_TYPE_ADSTICK,   "analog joystick");
		autoselect_analog_devices(inp, IPT_LIGHTGUN_X, IPT_LIGHTGUN_Y,  0,             ANALOG_TYPE_LIGHTGUN,  "lightgun");
		autoselect_analog_devices(inp, IPT_PEDAL,      IPT_PEDAL2,      IPT_PEDAL3,    ANALOG_TYPE_PEDAL,     "pedal");
		autoselect_analog_devices(inp, IPT_DIAL,       IPT_DIAL_V,      0,             ANALOG_TYPE_DIAL,      "dial");
		autoselect_analog_devices(inp, IPT_TRACKBALL_X,IPT_TRACKBALL_Y, 0,             ANALOG_TYPE_TRACKBALL, "trackball");
		autoselect_analog_devices(inp, IPT_POSITIONAL, IPT_POSITIONAL_V,0,             ANALOG_TYPE_POSITIONAL,"positional");
#ifdef MESS
		autoselect_analog_devices(inp, IPT_MOUSE_X,    IPT_MOUSE_Y,    0,             ANALOG_TYPE_MOUSE,    "mouse");
#endif // MESS
		end_resource_tracking();
	}

	// initialize keyboard devices
	keyboard_count = 0;
	result = IDirectInput_EnumDevices(dinput, DIDEVTYPE_KEYBOARD, enum_keyboard_callback, 0, DIEDFL_ATTACHEDONLY);
	if (result != DI_OK)
		goto cant_init_keyboard;

	// initialize mouse devices
	lightgun_count = 0;
	mouse_count = 0;
	mouse_num_of_buttons = 8;
	mouse_state_size = sizeof(DIMOUSESTATE2);
	if (win_use_mouse || use_lightgun)
	{
		lightgun_dual_player_state[0] = lightgun_dual_player_state[1] = 0;
		lightgun_dual_player_state[2] = lightgun_dual_player_state[3] = 0;
		win_use_raw_mouse = init_raw_mouse();
		if (!win_use_raw_mouse)
		{
			result = IDirectInput_EnumDevices(dinput, DIDEVTYPE_MOUSE, enum_mouse_callback, 0, DIEDFL_ATTACHEDONLY);
			if (result != DI_OK)
				goto cant_init_mouse;
			// remove system mouse on multi-mouse systems
			remove_dx_system_mouse();
		}

		// if we have at least one mouse, and the "Dual" option is selected,
		//  then the lightgun_count is 2 (The two guns are read as a single
		//  4-button mouse).
		if (mouse_count && use_lightgun_dual && lightgun_count < 2)
			lightgun_count = 2;
	}

	// initialize joystick devices
	joystick_count = 0;
	if (use_joystick)
	{
		result = IDirectInput_EnumDevices(dinput, DIDEVTYPE_JOYSTICK, enum_joystick_callback, 0, DIEDFL_ATTACHEDONLY);
		if (result != DI_OK)
			goto cant_init_joystick;
	}

	total_codes = 0;

	// init the keyboard list
	init_keycodes();

	// init the joystick list
	init_joycodes();

	// terminate array
	memset(&codelist[total_codes], 0, sizeof(codelist[total_codes]));

	// print the results
	verbose_printf("Input: Keyboards=%d  Mice=%d  Joysticks=%d  Lightguns=%d\n", keyboard_count, mouse_count, joystick_count, lightgun_count);
	if (options.controller)
		verbose_printf("Input: \"%s\" controller support enabled\n", options.controller);
	return 0;

cant_init_joystick:
cant_init_mouse:
cant_init_keyboard:
	IDirectInput_Release(dinput);
cant_create_dinput:
	dinput = NULL;
	return 1;
}



//============================================================
//  wininput_exit
//============================================================

static void wininput_exit(running_machine *machine)
{
	int i;

	// release all our keyboards
	for (i = 0; i < keyboard_count; i++)
	{
		IDirectInputDevice_Release(keyboard_device[i]);
		if (keyboard_device2[i])
			IDirectInputDevice_Release(keyboard_device2[i]);
		keyboard_device2[i]=0;
	}

	// release all our joysticks
	for (i = 0; i < joystick_count; i++)
	{
		IDirectInputDevice_Release(joystick_device[i]);
		if (joystick_device2[i])
			IDirectInputDevice_Release(joystick_device2[i]);
		joystick_device2[i]=0;
	}

	// release all our mice
	if (!win_use_raw_mouse)
	{
		if (mouse_count > 1)
		{
			IDirectInputDevice_Release(mouse_device[MAX_MICE]);
			if (mouse_device2[MAX_MICE])
				IDirectInputDevice_Release(mouse_device2[MAX_MICE]);
		}
		for (i = 0; i < mouse_count; i++)
		{
			IDirectInputDevice_Release(mouse_device[i]);
			if (mouse_device2[i])
				IDirectInputDevice_Release(mouse_device2[i]);
			mouse_device2[i]=0;
		}
	}

	// free allocated strings
	for (i = 0; i < total_codes; i++)
	{
		free(codelist[i].name);
		codelist[i].name = NULL;
	}

	// release DirectInput
	if (dinput)
		IDirectInput_Release(dinput);
	dinput = NULL;

	// release lock
	if (raw_mouse_lock != NULL)
		osd_lock_free(raw_mouse_lock);
}



//============================================================
//  win_pause_input
//============================================================

void win_pause_input(running_machine *machine, int paused)
{
	int i;

	// if paused, unacquire all devices
	if (paused)
	{
		// unacquire all keyboards
		for (i = 0; i < keyboard_count; i++)
			IDirectInputDevice_Unacquire(keyboard_device[i]);

		// unacquire all our mice
		if (!win_use_raw_mouse)
		{
			if (mouse_count > 1)
				IDirectInputDevice_Unacquire(mouse_device[MAX_MICE]);
			for (i = 0; i < mouse_count; i++)
				IDirectInputDevice_Unacquire(mouse_device[i]);
		}
	}

	// otherwise, reacquire all devices
	else
	{
		// acquire all keyboards
		for (i = 0; i < keyboard_count; i++)
			IDirectInputDevice_Acquire(keyboard_device[i]);

		// acquire all our mice if active
		if (!win_use_raw_mouse)
		{
			if (mouse_count > 1)
			IDirectInputDevice_Acquire(mouse_device[MAX_MICE]);
			if (mouse_active && !win_has_menu(win_window_list))
				for (i = 0; i < mouse_count && (win_use_mouse || use_lightgun); i++)
					IDirectInputDevice_Acquire(mouse_device[i]);
		}
	}

	// set the paused state
	input_paused = paused;
	winwindow_update_cursor_state();
}



//============================================================
//  wininput_poll
//============================================================

void wininput_poll(void)
{
	HWND focus = GetFocus();
	HRESULT result = 1;
	int i, j;

	// remember when this happened
	last_poll = osd_ticks();

	// periodically process events, in case they're not coming through
	winwindow_process_events_periodic();

	// if we don't have focus, turn off all keys
	if (!focus)
	{
		memset(&keyboard_state[0][0], 0, sizeof(keyboard_state[i]));
		updatekeyboard();
		return;
	}

	// poll all keyboards
	if (keyboard_detected_non_di_input)
		result = DIERR_NOTACQUIRED;
	else
		for (i = 0; i < keyboard_count; i++)
		{
			// first poll the device
			if (keyboard_device2[i])
				IDirectInputDevice2_Poll(keyboard_device2[i]);

			// get the state
			result = IDirectInputDevice_GetDeviceState(keyboard_device[i], sizeof(keyboard_state[i]), &keyboard_state[i][0]);

			// handle lost inputs here
			if ((result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED) && !input_paused)
			{
				result = IDirectInputDevice_Acquire(keyboard_device[i]);
				if (result == DI_OK)
					result = IDirectInputDevice_GetDeviceState(keyboard_device[i], sizeof(keyboard_state[i]), &keyboard_state[i][0]);
			}

			// convert to 0 or 1
			if (result == DI_OK)
				for (j = 0; j < sizeof(keyboard_state[i]); j++)
					keyboard_state[i][j] >>= 7;
		}

	keyboard_detected_non_di_input = FALSE;

	// if we couldn't poll the keyboard that way, poll it via GetAsyncKeyState
	if (result != DI_OK)
		for (i = 0; codelist[i].oscode; i++)
			if (IS_KEYBOARD_CODE(codelist[i].oscode))
			{
				int dik = DICODE(codelist[i].oscode);
				int vk = VKCODE(codelist[i].oscode);

				// if we have a non-zero VK, query it
				if (vk)
				{
					keyboard_state[0][dik] = (GetAsyncKeyState(vk) >> 15) & 1;
					if (keyboard_state[0][dik])
						keyboard_detected_non_di_input = TRUE;
				}
			}

	// update the lagged keyboard
	updatekeyboard();

	// poll all joysticks
	for (i = 0; i < joystick_count; i++)
	{
		// first poll the device
		if (joystick_device2[i])
			IDirectInputDevice2_Poll(joystick_device2[i]);

		// get the state
		result = IDirectInputDevice_GetDeviceState(joystick_device[i], sizeof(joystick_state[i]), &joystick_state[i]);

		// handle lost inputs here
		if (result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED)
		{
			result = IDirectInputDevice_Acquire(joystick_device[i]);
			if (result == DI_OK)
				result = IDirectInputDevice_GetDeviceState(joystick_device[i], sizeof(joystick_state[i]), &joystick_state[i]);
		}
	}

	// update joystick axis history
	update_joystick_axes();

	// poll all our mice if active
	if (win_use_raw_mouse)
		win_read_raw_mouse();
	else
	{
		if (mouse_active && !win_has_menu(win_window_list))
			for (i = 0; i < mouse_count && (win_use_mouse||use_lightgun); i++)
			{
				// first poll the device
				if (mouse_device2[i])
					IDirectInputDevice2_Poll(mouse_device2[i]);

				// get the state
				result = IDirectInputDevice_GetDeviceState(mouse_device[i], mouse_state_size, &mouse_state[i]);

				// handle lost inputs here
				if ((result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED) && !input_paused)
				{
					result = IDirectInputDevice_Acquire(mouse_device[i]);
					if (result == DI_OK)
						result = IDirectInputDevice_GetDeviceState(mouse_device[i], mouse_state_size, &mouse_state[i]);
				}
			}

		// poll the lightguns
		poll_lightguns();
	}
}



//============================================================
//  win_is_mouse_captured
//============================================================

int win_is_mouse_captured(void)
{
	return (!input_paused && mouse_active && mouse_count > 0 && win_use_mouse && !win_has_menu(win_window_list));
}



//============================================================
//  parse_analog_select
//============================================================

static void parse_analog_select(int type, const char *option)
{
	const char *stemp = options_get_string(option);

	if (strcmp(stemp, "keyboard") == 0)
		analog_type[type] = SELECT_TYPE_KEYBOARD;
	else if (strcmp(stemp, "mouse") == 0)
		analog_type[type] = SELECT_TYPE_MOUSE;
	else if (strcmp(stemp, "joystick") == 0)
		analog_type[type] = SELECT_TYPE_JOYSTICK;
	else if (strcmp(stemp, "lightgun") == 0)
		analog_type[type] = SELECT_TYPE_LIGHTGUN;
	else
	{
		fprintf(stderr, "Invalid %s value %s; reverting to keyboard\n", option, stemp);
		analog_type[type] = SELECT_TYPE_KEYBOARD;
	}
}


//============================================================
//  parse_digital
//============================================================

static void parse_digital(const char *option)
{
	const char *soriginal = options_get_string(option);
	const char *stemp = soriginal;

	if (strcmp(stemp, "none") == 0)
		memset(joystick_digital, 0, sizeof(joystick_digital));
	else if (strcmp(stemp, "all") == 0)
		memset(joystick_digital, 1, sizeof(joystick_digital));
	else
	{
		/* scan the string */
		while (1)
		{
			int joynum = 0;
			int axisnum = 0;

			/* stop if we hit the end */
			if (stemp[0] == 0)
				break;

			/* we require the next bits to be j<N> */
			if (tolower(stemp[0]) != 'j' || sscanf(&stemp[1], "%d", &joynum) != 1)
				goto usage;
			stemp++;
			while (stemp[0] != 0 && isdigit(stemp[0]))
				stemp++;

			/* if we are followed by a comma or an end, mark all the axes digital */
			if (stemp[0] == 0 || stemp[0] == ',')
			{
				if (joynum != 0 && joynum - 1 < MAX_JOYSTICKS)
					memset(&joystick_digital[joynum - 1], 1, sizeof(joystick_digital[joynum - 1]));
				if (stemp[0] == 0)
					break;
				stemp++;
				continue;
			}

			/* loop over axes */
			while (1)
			{
				/* stop if we hit the end */
				if (stemp[0] == 0)
					break;

				/* if we hit a comma, skip it and break out */
				if (stemp[0] == ',')
				{
					stemp++;
					break;
				}

				/* we require the next bits to be a<N> */
				if (tolower(stemp[0]) != 'a' || sscanf(&stemp[1], "%d", &axisnum) != 1)
					goto usage;
				stemp++;
				while (stemp[0] != 0 && isdigit(stemp[0]))
					stemp++;

				/* set that axis to digital */
				if (joynum != 0 && joynum - 1 < MAX_JOYSTICKS && axisnum < MAX_AXES)
					joystick_digital[joynum - 1][axisnum] = 1;
			}
		}
	}
	return;

usage:
	fprintf(stderr, "Invalid %s value %s; reverting to all -- valid values are:\n", option, soriginal);
	fprintf(stderr, "         none -- no axes on any joysticks are digital\n");
	fprintf(stderr, "         all -- all axes on all joysticks are digital\n");
	fprintf(stderr, "         j<N> -- all axes on joystick <N> are digital\n");
	fprintf(stderr, "         j<N>a<M> -- axis <M> on joystick <N> is digital\n");
	fprintf(stderr, "    Multiple axes can be specified for one joystick:\n");
	fprintf(stderr, "         j1a5a6 -- axes 5 and 6 on joystick 1 are digital\n");
	fprintf(stderr, "    Multiple joysticks can be specified separated by commas:\n");
	fprintf(stderr, "         j1,j2a2 -- all joystick 1 axes and axis 2 on joystick 2 are digital\n");
}

//============================================================
//  extract_input_config
//============================================================

static void extract_input_config(void)
{
	// extract boolean options
	win_use_mouse = options_get_bool("mouse");
	use_joystick = options_get_bool("joystick");
	use_lightgun = options_get_bool("lightgun");
	use_lightgun_dual = options_get_bool("dual_lightgun");
	use_lightgun_reload = options_get_bool("offscreen_reload");
	steadykey = options_get_bool("steadykey");
	a2d_deadzone = options_get_float("a2d_deadzone");
	options.controller = options_get_string("ctrlr");
	parse_analog_select(ANALOG_TYPE_PADDLE, "paddle_device");
	parse_analog_select(ANALOG_TYPE_ADSTICK, "adstick_device");
	parse_analog_select(ANALOG_TYPE_PEDAL, "pedal_device");
	parse_analog_select(ANALOG_TYPE_DIAL, "dial_device");
	parse_analog_select(ANALOG_TYPE_TRACKBALL, "trackball_device");
	parse_analog_select(ANALOG_TYPE_LIGHTGUN, "lightgun_device");
	parse_analog_select(ANALOG_TYPE_POSITIONAL, "positional_device");
#ifdef MESS
	parse_analog_select(ANALOG_TYPE_MOUSE, "mouse_device");
#endif
	parse_digital("digital");
}



//============================================================
//  updatekeyboard
//============================================================

// since the keyboard controller is slow, it is not capable of reporting multiple
// key presses fast enough. We have to delay them in order not to lose special moves
// tied to simultaneous button presses.

static void updatekeyboard(void)
{
	int i, changed = 0;

	// see if any keys have changed state
	for (i = 0; i < MAX_KEYS; i++)
		if (keyboard_state[0][i] != oldkey[i])
		{
			changed = 1;

			// keypress was missed, turn it on for one frame
			if (keyboard_state[0][i] == 0 && currkey[i] == 0)
				currkey[i] = -1;
		}

	// if keyboard state is stable, copy it over
	if (!changed)
		memcpy(currkey, &keyboard_state[0][0], sizeof(currkey));

	// remember the previous state
	memcpy(oldkey, &keyboard_state[0][0], sizeof(oldkey));
}



//============================================================
//  is_key_pressed
//============================================================

static int is_key_pressed(os_code keycode)
{
	int dik = DICODE(keycode);

	// make sure we've polled recently
	if (osd_ticks() > last_poll + osd_ticks_per_second()/4)
		wininput_poll();

	// if the video window isn't visible, we have to get our events from the console
	if (!win_window_list->hwnd || !IsWindowVisible(win_window_list->hwnd))
	{
		// warning: this code relies on the assumption that when you're polling for
		// keyboard events before the system is initialized, they are all of the
		// "press any key" to continue variety
		DWORD result = 0;
		INPUT_RECORD record;
		if (GetNumberOfConsoleInputEvents(win_input_handle, &result) && result > 0)
			ReadConsoleInput(win_input_handle, &record, 1, &result);
		return result;
	}

#ifdef MAME_DEBUG
	// if the debugger is visible and we don't have focus, the key is not pressed
	if (debugwin_is_debugger_visible() && GetFocus() != win_window_list->hwnd)
		return 0;
#endif

	// otherwise, just return the current keystate
	if (steadykey)
		return currkey[dik];
	else
		return keyboard_state[0][dik];
}



//============================================================
//  init_keycodes
//============================================================

static void init_keycodes(void)
{
	const int iswin9x = (GetVersion() >> 31) & 1;

	int key;

	// iterate over all possible keys
	for (key = 0; key < MAX_KEYS; key++)
	{
		DIDEVICEOBJECTINSTANCE instance = { 0 };
		HRESULT result;

		// attempt to get the object info
		instance.dwSize = STRUCTSIZE(DIDEVICEOBJECTINSTANCE);
		result = IDirectInputDevice_GetObjectInfo(keyboard_device[0], &instance, key, DIPH_BYOFFSET);
		if (result == DI_OK)
		{
			// if it worked, assume we have a valid key

			// copy the name
			char *namecopy = utf8_from_tstring(instance.tszName);
			if (namecopy != NULL)
			{
				input_code standardcode;
				os_code code;
				int entry;

				// find the table entry, if there is one
				for (entry = 0; win_key_trans_table[entry][0] >= 0; entry++)
					if (win_key_trans_table[entry][DI_KEY] == key)
						break;

				// compute the code, which encodes DirectInput, virtual, and ASCII codes
				code = KEYCODE(key, 0, 0);
				standardcode = CODE_OTHER_DIGITAL;
				if (win_key_trans_table[entry][0] >= 0)
				{
					int vkey = win_key_trans_table[entry][VIRTUAL_KEY];
					if (iswin9x)
						switch (vkey)
						{
							case VK_LSHIFT:   case VK_RSHIFT:   vkey = VK_SHIFT;   break;
							case VK_LCONTROL: case VK_RCONTROL: vkey = VK_CONTROL; break;
							case VK_LMENU:    case VK_RMENU:    vkey = VK_MENU;    break;
						}

					code = KEYCODE(key, vkey, win_key_trans_table[entry][ASCII_KEY]);
					standardcode = win_key_trans_table[entry][MAME_KEY];
				}

				// fill in the key description
				codelist[total_codes].name = namecopy;
				codelist[total_codes].oscode = code;
				codelist[total_codes].inputcode = standardcode;
				total_codes++;
			}
		}
	}
}



//============================================================
//  update_joystick_axes
//============================================================

static void update_joystick_axes(void)
{
	int joynum, axis;

	for (joynum = 0; joynum < joystick_count; joynum++)
		for (axis = 0; axis < MAX_AXES; axis++)
		{
			axis_history *history = &joystick_history[joynum][axis][0];
			LONG curval = ((LONG *)&joystick_state[joynum].lX)[axis];
			int newtype;

			/* if same as last time (within a small tolerance), update the count */
			if (history[0].count > 0 && (history[0].value - curval) > -4 && (history[0].value - curval) < 4)
				history[0].count++;

			/* otherwise, update the history */
			else
			{
				memmove(&history[1], &history[0], sizeof(history[0]) * (HISTORY_LENGTH - 1));
				history[0].count = 1;
				history[0].value = curval;
			}

			/* if we've only ever seen one value here, or if we've been stuck at the same value for a long */
			/* time (1 minute), mark the axis as dead or invalid */
			if (history[1].count == 0 || history[0].count > Machine->screen[0].refresh * 60)
				newtype = AXIS_TYPE_INVALID;

			/* scan the history and count unique values; if we get more than 3, it's analog */
			else
			{
				int bucketsize = (joystick_range[joynum][axis].lMax - joystick_range[joynum][axis].lMin) / 3;
				LONG uniqueval[3] = { 1234567890, 1234567890, 1234567890 };
				int histnum;

				/* assume digital unless we figure out otherwise */
				newtype = AXIS_TYPE_DIGITAL;

				/* loop over the whole history, bucketing the values */
				for (histnum = 0; histnum < HISTORY_LENGTH; histnum++)
					if (history[histnum].count > 0)
					{
						int bucket = (history[histnum].value - joystick_range[joynum][axis].lMin) / bucketsize;

						/* if we already have an entry in this bucket, we're analog */
						if (uniqueval[bucket] != 1234567890 && uniqueval[bucket] != history[histnum].value)
						{
							newtype = AXIS_TYPE_ANALOG;
							break;
						}

						/* remember this value */
						uniqueval[bucket] = history[histnum].value;
					}
			}

			/* if the type doesn't match, switch it */
			if (joystick_type[joynum][axis] != newtype)
			{
				static const char *axistypes[] = { "invalid", "digital", "analog" };
				joystick_type[joynum][axis] = newtype;
				verbose_printf("Input: Joystick %d axis %d is now %s\n", joynum, axis, axistypes[newtype]);
			}
		}
}



//============================================================
//  add_joylist_entry
//============================================================

static void add_joylist_entry(const char *name, os_code code, input_code standardcode)
{
	// copy the name
	char *namecopy = malloc(strlen(name) + 1);
	if (namecopy)
	{
		int entry;

		// find the table entry, if there is one
		for (entry = 0; entry < ELEMENTS(joy_trans_table); entry++)
			if (joy_trans_table[entry][0] == code)
				break;

		// fill in the joy description
		codelist[total_codes].name = strcpy(namecopy, name);
		codelist[total_codes].oscode = code;
		if (entry < ELEMENTS(joy_trans_table))
			standardcode = joy_trans_table[entry][1];
		codelist[total_codes].inputcode = standardcode;
		total_codes++;
	}
}



//============================================================
//  init_joycodes
//============================================================

static void init_joycodes(void)
{
	int mouse, gun, stick, axis, button, pov;
	char tempname[MAX_PATH];
	char mousename[30];

	// map mice first
	for (mouse = 0; mouse < mouse_count; mouse++)
	{
		if (mouse_count > 1)
			sprintf(mousename, "Mouse %d ", mouse + 1);
		else
			sprintf(mousename, "Mouse ");

		// log the info
		verbose_printf("Input: %s: %s\n", mousename, mouse_name[mouse]);

		// add analog axes (fix me -- should enumerate these)
		sprintf(tempname, "%sX", mousename);
		add_joylist_entry(tempname, JOYCODE(mouse, CODETYPE_MOUSEAXIS, 0), CODE_OTHER_ANALOG_RELATIVE);
		sprintf(tempname, "%sY", mousename);
		add_joylist_entry(tempname, JOYCODE(mouse, CODETYPE_MOUSEAXIS, 1), CODE_OTHER_ANALOG_RELATIVE);
		sprintf(tempname, "%sZ", mousename);
		add_joylist_entry(tempname, JOYCODE(mouse, CODETYPE_MOUSEAXIS, 2), CODE_OTHER_ANALOG_RELATIVE);

		// add mouse buttons
		for (button = 0; button < mouse_num_of_buttons; button++)
		{
			if (win_use_raw_mouse)
			{
				sprintf(tempname, "%sButton %d", mousename, button);
				add_joylist_entry(tempname, JOYCODE(mouse, CODETYPE_MOUSEBUTTON, button), CODE_OTHER_DIGITAL);
			}
			else
			{
				DIDEVICEOBJECTINSTANCE instance = { 0 };
				HRESULT result;

				// attempt to get the object info
				instance.dwSize = STRUCTSIZE(DIDEVICEOBJECTINSTANCE);
				result = IDirectInputDevice_GetObjectInfo(mouse_device[mouse], &instance, offsetof(DIMOUSESTATE, rgbButtons[button]), DIPH_BYOFFSET);
				if (result == DI_OK)
				{
					char *utf8_name = utf8_from_tstring(instance.tszName);
					if (utf8_name != NULL)
					{
						sprintf(tempname, "%s%s", mousename, utf8_name);
						free(utf8_name);
						add_joylist_entry(tempname, JOYCODE(mouse, CODETYPE_MOUSEBUTTON, button), CODE_OTHER_DIGITAL);
					}
				}
			}
		}
	}

	// map lightguns second
	for (gun = 0; gun < lightgun_count; gun++)
	{
		if (win_use_raw_mouse)
			sprintf(mousename, "Lightgun on Mouse %d ", gun + 1);
		else
			sprintf(mousename, "Lightgun %d ", gun + 1);
		// add lightgun axes (fix me -- should enumerate these)
		sprintf(tempname, "%sX", mousename);
		add_joylist_entry(tempname, JOYCODE(gun, CODETYPE_GUNAXIS, 0), CODE_OTHER_ANALOG_ABSOLUTE);
		sprintf(tempname, "%sY", mousename);
		add_joylist_entry(tempname, JOYCODE(gun, CODETYPE_GUNAXIS, 1), CODE_OTHER_ANALOG_ABSOLUTE);
	}

	// now map joysticks
	for (stick = 0; stick < joystick_count; stick++)
	{
		// log the info
		verbose_printf("Input: Joystick %d: %s (%d axes, %d buttons, %d POVs)\n", stick + 1, joystick_name[stick], (int)joystick_caps[stick].dwAxes, (int)joystick_caps[stick].dwButtons, (int)joystick_caps[stick].dwPOVs);

		// loop over all axes
		for (axis = 0; axis < MAX_AXES; axis++)
		{
			DIDEVICEOBJECTINSTANCE instance = { 0 };
			HRESULT result;

			// reset the type
			joystick_type[stick][axis] = AXIS_TYPE_INVALID;

			// attempt to get the object info
			instance.dwSize = STRUCTSIZE(DIDEVICEOBJECTINSTANCE);
			result = IDirectInputDevice_GetObjectInfo(joystick_device[stick], &instance, offsetof(DIJOYSTATE, lX) + axis * sizeof(LONG), DIPH_BYOFFSET);
			if (result == DI_OK)
			{
				char *utf8_name = utf8_from_tstring(instance.tszName);
				if (utf8_name != NULL)
				{
					verbose_printf("Input:  Axis %d (%s)%s\n", axis, utf8_name, joystick_digital[stick][axis] ? " - digital" : "");

					// add analog axis
					if (!joystick_digital[stick][axis])
					{
						sprintf(tempname, "J%d %s", stick + 1, utf8_name);
						add_joylist_entry(tempname, JOYCODE(stick, CODETYPE_JOYAXIS, axis), CODE_OTHER_ANALOG_ABSOLUTE);
					}

					// add negative value
					sprintf(tempname, "J%d %s -", stick + 1, utf8_name);
					add_joylist_entry(tempname, JOYCODE(stick, CODETYPE_AXIS_NEG, axis), CODE_OTHER_DIGITAL);

					// add positive value
					sprintf(tempname, "J%d %s +", stick + 1, utf8_name);
					add_joylist_entry(tempname, JOYCODE(stick, CODETYPE_AXIS_POS, axis), CODE_OTHER_DIGITAL);

					// get the axis range while we're here
					joystick_range[stick][axis].diph.dwSize = sizeof(DIPROPRANGE);
					joystick_range[stick][axis].diph.dwHeaderSize = sizeof(joystick_range[stick][axis].diph);
					joystick_range[stick][axis].diph.dwObj = offsetof(DIJOYSTATE, lX) + axis * sizeof(LONG);
					joystick_range[stick][axis].diph.dwHow = DIPH_BYOFFSET;
					result = IDirectInputDevice_GetProperty(joystick_device[stick], DIPROP_RANGE, &joystick_range[stick][axis].diph);
					free(utf8_name);
				}
			}
		}

		// loop over all buttons
		for (button = 0; button < MAX_BUTTONS; button++)
		{
			DIDEVICEOBJECTINSTANCE instance = { 0 };
			HRESULT result;

			// attempt to get the object info
			instance.dwSize = STRUCTSIZE(DIDEVICEOBJECTINSTANCE);
			result = IDirectInputDevice_GetObjectInfo(joystick_device[stick], &instance, offsetof(DIJOYSTATE, rgbButtons[button]), DIPH_BYOFFSET);
			if (result == DI_OK)
			{
				// make the name for this item
				char *utf8_name = utf8_from_tstring(instance.tszName);
				if (utf8_name != NULL)
				{
					sprintf(tempname, "J%d %s", stick + 1, utf8_name);
					free(utf8_name);
					add_joylist_entry(tempname, JOYCODE(stick, CODETYPE_BUTTON, button), CODE_OTHER_DIGITAL);
				}
			}
		}

		// check POV hats
		for (pov = 0; pov < MAX_POV; pov++)
		{
			DIDEVICEOBJECTINSTANCE instance = { 0 };
			HRESULT result;

			// attempt to get the object info
			instance.dwSize = STRUCTSIZE(DIDEVICEOBJECTINSTANCE);
			result = IDirectInputDevice_GetObjectInfo(joystick_device[stick], &instance, offsetof(DIJOYSTATE, rgdwPOV[pov]), DIPH_BYOFFSET);
			if (result == DI_OK)
			{
				char *utf8_name = utf8_from_tstring(instance.tszName);
				if (utf8_name != NULL)
				{
					// add up direction
					sprintf(tempname, "J%d %s U", stick + 1, utf8_name);
					add_joylist_entry(tempname, JOYCODE(stick, CODETYPE_POV_UP, pov), CODE_OTHER_DIGITAL);

					// add down direction
					sprintf(tempname, "J%d %s D", stick + 1, utf8_name);
					add_joylist_entry(tempname, JOYCODE(stick, CODETYPE_POV_DOWN, pov), CODE_OTHER_DIGITAL);

					// add left direction
					sprintf(tempname, "J%d %s L", stick + 1, utf8_name);
					add_joylist_entry(tempname, JOYCODE(stick, CODETYPE_POV_LEFT, pov), CODE_OTHER_DIGITAL);

					// add right direction
					sprintf(tempname, "J%d %s R", stick + 1, utf8_name);
					add_joylist_entry(tempname, JOYCODE(stick, CODETYPE_POV_RIGHT, pov), CODE_OTHER_DIGITAL);

					free(utf8_name);
				}
			}
		}
	}
}



//============================================================
//  get_joycode_value
//============================================================

static INT32 get_joycode_value(os_code joycode)
{
	int joyindex = JOYINDEX(joycode);
	int codetype = CODETYPE(joycode);
	int joynum = JOYNUM(joycode);
	DWORD pov;

	// switch off the type
	switch (codetype)
	{
		case CODETYPE_MOUSEBUTTON:
			if (!win_use_raw_mouse) {
				/* ActLabs lightgun - remap button 2 (shot off-screen) as button 1 */
				if (use_lightgun_dual && joyindex<4) {
					if (use_lightgun_reload && joynum==0) {
						if (joyindex==0 && lightgun_dual_player_state[1])
							return 1;
						if (joyindex==1 && lightgun_dual_player_state[1])
							return 0;
						if (joyindex==2 && lightgun_dual_player_state[3])
							return 1;
						if (joyindex==3 && lightgun_dual_player_state[3])
							return 0;
					}
					return lightgun_dual_player_state[joyindex];
				}

				if (use_lightgun) {
					if (use_lightgun_reload && joynum==0) {
						if (joyindex==0 && (mouse_state[0].rgbButtons[1]&0x80))
							return 1;
						if (joyindex==1 && (mouse_state[0].rgbButtons[1]&0x80))
							return 0;
					}
				}
			}
			return mouse_state[joynum].rgbButtons[joyindex] >> 7;

		case CODETYPE_BUTTON:
			return joystick_state[joynum].rgbButtons[joyindex] >> 7;

		case CODETYPE_AXIS_POS:
		case CODETYPE_AXIS_NEG:
		{
			LONG val = ((LONG *)&joystick_state[joynum].lX)[joyindex];
			LONG top = joystick_range[joynum][joyindex].lMax;
			LONG bottom = joystick_range[joynum][joyindex].lMin;
			LONG middle = (top + bottom) / 2;

			// watch for movement greater "a2d_deadzone" along either axis
			// FIXME in the two-axis joystick case, we need to find out
			// the angle. Anything else is unprecise.
			if (codetype == CODETYPE_AXIS_POS)
				return (val > middle + ((top - middle) * a2d_deadzone));
			else
				return (val < middle - ((middle - bottom) * a2d_deadzone));
		}

		// anywhere from 0-45 (315) deg to 0+45 (45) deg
		case CODETYPE_POV_UP:
			pov = joystick_state[joynum].rgdwPOV[joyindex];
			return ((pov & 0xffff) != 0xffff && (pov >= 31500 || pov <= 4500));

		// anywhere from 90-45 (45) deg to 90+45 (135) deg
		case CODETYPE_POV_RIGHT:
			pov = joystick_state[joynum].rgdwPOV[joyindex];
			return ((pov & 0xffff) != 0xffff && (pov >= 4500 && pov <= 13500));

		// anywhere from 180-45 (135) deg to 180+45 (225) deg
		case CODETYPE_POV_DOWN:
			pov = joystick_state[joynum].rgdwPOV[joyindex];
			return ((pov & 0xffff) != 0xffff && (pov >= 13500 && pov <= 22500));

		// anywhere from 270-45 (225) deg to 270+45 (315) deg
		case CODETYPE_POV_LEFT:
			pov = joystick_state[joynum].rgdwPOV[joyindex];
			return ((pov & 0xffff) != 0xffff && (pov >= 22500 && pov <= 31500));

		// analog joystick axis
		case CODETYPE_JOYAXIS:
		{
			if (joystick_type[joynum][joyindex] != AXIS_TYPE_ANALOG)
				return ANALOG_VALUE_INVALID;
			else
			{
				LONG val = ((LONG *)&joystick_state[joynum].lX)[joyindex];
				LONG top = joystick_range[joynum][joyindex].lMax;
				LONG bottom = joystick_range[joynum][joyindex].lMin;

				if (!use_joystick)
					return 0;
				val = (INT64)(val - bottom) * (INT64)(ANALOG_VALUE_MAX - ANALOG_VALUE_MIN) / (INT64)(top - bottom) + ANALOG_VALUE_MIN;
				if (val < ANALOG_VALUE_MIN) val = ANALOG_VALUE_MIN;
				if (val > ANALOG_VALUE_MAX) val = ANALOG_VALUE_MAX;
				return val;
			}
		}

		// analog mouse axis
		case CODETYPE_MOUSEAXIS:
			// if the mouse isn't yet active, make it so
			if (!mouse_active && win_use_mouse && !win_has_menu(win_window_list))
			{
				mouse_active = 1;
				win_pause_input(Machine, 0);
			}

			if (win_use_raw_mouse && (raw_mouse_device[joynum].flags != MOUSE_MOVE_RELATIVE))
				return 0;

			// return the latest mouse info
			if (joyindex == 0)
				return mouse_state[joynum].lX * 512;
			if (joyindex == 1)
				return mouse_state[joynum].lY * 512;
			// Z axis on most mice is incremeted 120 for each change.
			// But some are only 30 so we will scale for  +/- 1 increments on those
			// 25% scaling will give  +/- 1 increments for most mice.
			if (joyindex == 2)
				return (mouse_state[joynum].lZ / 30) * 512;
			return 0;

		// analog gun axis
		case CODETYPE_GUNAXIS:
			if (joyindex >= MAX_LIGHTGUN_AXIS)
				return 0;

			if (win_use_raw_mouse) {
				if (raw_mouse_device[joynum].flags == MOUSE_MOVE_RELATIVE)
					return 0;
				// convert absolute mouse data to the range we need
				return (INT64)((joyindex ? mouse_state[joynum].lY : mouse_state[joynum].lX) + 1) * (INT64)(ANALOG_VALUE_MAX - ANALOG_VALUE_MIN + 1) / 65536 - 1 + ANALOG_VALUE_MIN;
			}

			// return the latest gun info
			if (joynum >= MAX_DX_LIGHTGUNS)
				return 0;
			return gun_axis[joynum][joyindex];
	}

	// keep the compiler happy
	return 0;
}



//============================================================
//  osd_get_code_value
//============================================================

INT32 osd_get_code_value(os_code code)
{
	if (IS_KEYBOARD_CODE(code))
		return is_key_pressed(code);
	else
		return get_joycode_value(code);
}



//============================================================
//  osd_get_code_list
//============================================================

const os_code_info *osd_get_code_list(void)
{
	return codelist;
}



//============================================================
//  input_mouse_button_down
//============================================================

void input_mouse_button_down(int button, int x, int y)
{
	if (!use_lightgun_dual)
		return;

	lightgun_dual_player_state[button]=1;
	lightgun_dual_player_pos[button].x=x;
	lightgun_dual_player_pos[button].y=y;

	//logerror("mouse %d at %d %d\n",button,x,y);
}

//============================================================
//  input_mouse_button_up
//============================================================

void input_mouse_button_up(int button)
{
	if (!use_lightgun_dual)
		return;

	lightgun_dual_player_state[button]=0;
}

//============================================================
//  poll_lightguns
//============================================================

static void poll_lightguns(void)
{
	POINT point;
	int player;

	// if the mouse isn't yet active, make it so
	if (!mouse_active && (win_use_mouse || use_lightgun) && !win_has_menu(win_window_list))
	{
		mouse_active = 1;
		win_pause_input(Machine, 0);
	}

	// if out of range, skip it
	if (!use_lightgun || !win_physical_width || !win_physical_height)
		return;

	// Warning message to users - design wise this probably isn't the best function to put this in...
	if (video_config.windowed)
		popmessage("Lightgun not supported in windowed mode");

	// loop over players
	for (player = 0; player < MAX_DX_LIGHTGUNS; player++)
	{
		// Hack - if button 2 is pressed on lightgun, then return 0,ANALOG_VALUE_MAX (off-screen) to simulate reload
		if (use_lightgun_reload)
		{
			int return_offscreen=0;

			// In dualmode we need to use the buttons returned from Windows messages
			if (use_lightgun_dual)
			{
				if (player==0 && lightgun_dual_player_state[1])
					return_offscreen=1;

				if (player==1 && lightgun_dual_player_state[3])
					return_offscreen=1;
			}
			else
			{
				if (mouse_state[0].rgbButtons[1]&0x80)
					return_offscreen=1;
			}

			if (return_offscreen)
			{
				gun_axis[player][0] = ANALOG_VALUE_MIN;
				gun_axis[player][1] = ANALOG_VALUE_MAX;
				continue;
			}
		}

		// Act-Labs dual lightgun - _only_ works with Windows messages for input location
		if (use_lightgun_dual)
		{
			if (player==0)
			{
				point.x=lightgun_dual_player_pos[0].x; // Button 0 is player 1
				point.y=lightgun_dual_player_pos[0].y; // Button 0 is player 1
			}
			else if (player==1)
			{
				point.x=lightgun_dual_player_pos[2].x; // Button 2 is player 2
				point.y=lightgun_dual_player_pos[2].y; // Button 2 is player 2
			}
			else
			{
				point.x=point.y=0;
			}
		}
		else
		{
			// I would much prefer to use DirectInput to read the gun values but there seem to be
			// some problems...  DirectInput (8.0 tested) on Win98 returns garbage for both buffered
			// and immediate, absolute and relative axis modes.  Win2k (DX 8.1) returns good data
			// for buffered absolute reads, but WinXP (8.1) returns garbage on all modes.  DX9 betas
			// seem to exhibit the same behaviour.  I have no idea of the cause of this, the only
			// consistent way to read the location seems to be the Windows system call GetCursorPos
			// which requires the application have non-exclusive access to the mouse device
			//
			GetCursorPos(&point);
		}

		// Map absolute pixel values into ANALOG_VALUE_MIN -> ANALOG_VALUE_MAX range
		gun_axis[player][0] = (point.x * (ANALOG_VALUE_MAX - ANALOG_VALUE_MIN) + win_physical_width/2) / (win_physical_width-1) + ANALOG_VALUE_MIN;
		gun_axis[player][1] = (point.y * (ANALOG_VALUE_MAX - ANALOG_VALUE_MIN) + win_physical_height/2) / (win_physical_height-1) + ANALOG_VALUE_MIN;

		if (gun_axis[player][0] < ANALOG_VALUE_MIN) gun_axis[player][0] = ANALOG_VALUE_MIN;
		if (gun_axis[player][0] > ANALOG_VALUE_MAX) gun_axis[player][0] = ANALOG_VALUE_MAX;
		if (gun_axis[player][1] < ANALOG_VALUE_MIN) gun_axis[player][1] = ANALOG_VALUE_MIN;
		if (gun_axis[player][1] > ANALOG_VALUE_MAX) gun_axis[player][1] = ANALOG_VALUE_MAX;
	}
}



//============================================================
//  osd_joystick_needs_calibration
//============================================================

int osd_joystick_needs_calibration(void)
{
	return 0;
}



//============================================================
//  osd_joystick_start_calibration
//============================================================

void osd_joystick_start_calibration(void)
{
}



//============================================================
//  osd_joystick_calibrate_next
//============================================================

const char *osd_joystick_calibrate_next(void)
{
	return 0;
}



//============================================================
//  osd_joystick_calibrate
//============================================================

void osd_joystick_calibrate(void)
{
}



//============================================================
//  osd_joystick_end_calibration
//============================================================

void osd_joystick_end_calibration(void)
{
}



//============================================================
//  osd_customize_inputport_list
//============================================================

void osd_customize_inputport_list(input_port_default_entry *defaults)
{
	static input_seq no_alt_tab_seq = SEQ_DEF_5(KEYCODE_TAB, CODE_NOT, KEYCODE_LALT, CODE_NOT, KEYCODE_RALT);
	input_port_default_entry *idef = defaults;

	// loop over all the defaults
	while (idef->type != IPT_END)
	{
		// map in some OSD-specific keys
		switch (idef->type)
		{
			// alt-enter for fullscreen
			case IPT_OSD_1:
				idef->token = "TOGGLE_FULLSCREEN";
				idef->name = "Toggle Fullscreen";
				seq_set_2(&idef->defaultseq, KEYCODE_LALT, KEYCODE_ENTER);
				break;

#ifdef MESS
			case IPT_OSD_2:
				if (options.disable_normal_ui)
				{
					idef->token = "TOGGLE_MENUBAR";
					idef->name = "Toggle Menubar";
					seq_set_1 (&idef->defaultseq, KEYCODE_SCRLOCK);
				}
				break;
#endif /* MESS */

			// insert for fastforward
			case IPT_OSD_3:
				idef->token = "FAST_FORWARD";
				idef->name = "Fast Forward";
				seq_set_1(&idef->defaultseq, KEYCODE_INSERT);
				break;
		}

		// disable the config menu if the ALT key is down
		// (allows ALT-TAB to switch between windows apps)
		if (idef->type == IPT_UI_CONFIGURE)
			seq_copy(&idef->defaultseq, &no_alt_tab_seq);

#ifdef MESS
		if (idef->type == IPT_UI_THROTTLE)
			seq_set_0(&idef->defaultseq);
#endif /* MESS */

		// Dual lightguns - remap default buttons to suit
		if (use_lightgun && use_lightgun_dual)
		{
			static input_seq p1b2 = SEQ_DEF_3(KEYCODE_LALT, CODE_OR, JOYCODE_1_BUTTON2);
			static input_seq p1b3 = SEQ_DEF_3(KEYCODE_SPACE, CODE_OR, JOYCODE_1_BUTTON3);
			static input_seq p2b1 = SEQ_DEF_5(KEYCODE_A, CODE_OR, JOYCODE_2_BUTTON1, CODE_OR, MOUSECODE_1_BUTTON3);
			static input_seq p2b2 = SEQ_DEF_3(KEYCODE_S, CODE_OR, JOYCODE_2_BUTTON2);

			if (idef->type == IPT_BUTTON2 && idef->player == 1)
				seq_copy(&idef->defaultseq, &p1b2);
			if (idef->type == IPT_BUTTON3 && idef->player == 1)
				seq_copy(&idef->defaultseq, &p1b3);
			if (idef->type == IPT_BUTTON1 && idef->player == 2)
				seq_copy(&idef->defaultseq, &p2b1);
			if (idef->type == IPT_BUTTON2 && idef->player == 2)
				seq_copy(&idef->defaultseq, &p2b2);
		}

		// find the next one
		idef++;
	}
}



//============================================================
//  set_rawmouse_device_name
//============================================================
static void set_rawmouse_device_name(const TCHAR *raw_string, unsigned int mouse_num)
{
	// This routine is used to get the mouse name.  This will give
	// us the same name that DX will report on non-XP systems.
	// It is a total mess to find the real name of a USB device from
	// the RAW name.  The mouse is named as the HID device, but
	// the actual physical name that the mouse reports is in the USB
	// device.
	// If the mouse is not HID, we use the name pointed to by the raw_string key.
	// If the mouse is HID, we try to get the LocationInformation value
	// from the parent USB device.  If this is not available we default
	// to the name pointed to by the raw_string key.  Fun, eh?
	// The raw_string as passed is formatted as:
	//   \??\type-id#hardware-id#instance-id#{DeviceClasses-id}

	TCHAR reg_string[MAX_PATH] = TEXT("SYSTEM\\CurrentControlSet\\Enum\\");
	TCHAR parent_id_prefix[MAX_PATH];		// the id we are looking for
	TCHAR test_parent_id_prefix[MAX_PATH];	// the id we are testing
	TCHAR *test_pos;						// general purpose test pointer for positioning
	DWORD name_length = MAX_PATH;
	DWORD instance, hardware;
	HKEY hardware_key, reg_key, sub_key = NULL;
	int key_pos, hardware_key_pos;		// used to keep track of where the keys are added.
	LONG hardware_result, instance_result;

	// too many mice?
	if (mouse_num > MAX_MICE) return;
	mouse_name[mouse_num][0] = 0;

	// is string formated in RAW format?
	if (_tcsncmp(raw_string, TEXT("\\??\\"), 4)) return;
	key_pos = _tcslen(reg_string);

	// remove \??\ from start and add this onto the end of the key string
	_tcscat(reg_string, raw_string + 4);
	// then remove the final DeviceClasses string
	*(_tcsrchr(reg_string, '#')) = 0;

	// convert the remaining 2 '#' to '\'
	test_pos = _tcschr(reg_string, '#');
	*test_pos = '\\';
	*(_tcschr(test_pos, '#')) = '\\';
	// finialize registry key

	// Open the key.  If we can't and we're HID then try USB
	instance_result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
							reg_string,
							0, KEY_READ, &reg_key);
	if (instance_result == ERROR_SUCCESS)
	{
		// first check for the name in LocationInformation (slim chance)
		// if it is not found then default to the name in DeviceDesc
		instance_result = RegQueryValueEx(reg_key,
								TEXT("LocationInformation"),
								NULL, NULL,
								mouse_name[mouse_num],
								&name_length)
				&& RegQueryValueEx(reg_key,
								TEXT("DeviceDesc"),
								NULL, NULL,
								mouse_name[mouse_num],
								&name_length);
		RegCloseKey(reg_key);
	}

	// give up if not HID
	if (_tcsncmp(raw_string + 4, TEXT("HID"), 3)) return;

	// Try and track down the parent USB device.
	// The raw name we are passed contains the HID Hardware ID, and USB Parent ID.
	// Some times the HID Hardware ID is the same as the USB Hardware ID.  But not always.
	// We need to look in the device instances of the each USB hardware instance for the parent-id.
	_tcsncpy(reg_string + key_pos, TEXT("USB"), 3);
	test_pos = _tcsrchr(reg_string, '\\');
	_tcscpy(parent_id_prefix, test_pos + 1);
	key_pos += 4;
	*(reg_string + key_pos) = 0;
	hardware_key_pos = _tcslen(reg_string);
	// reg_string now contains the key name for the USB hardware.
	// parent_id_prefix is the value we will be looking to match.
	hardware_result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
						reg_string,
						0, KEY_READ, &hardware_key);
	if (hardware_result != ERROR_SUCCESS) return;

	// Start checking each peice of USB hardware
	for (hardware = 0; hardware_result == ERROR_SUCCESS; hardware++)
	{
		name_length = MAX_PATH - hardware_key_pos;
		// get hardware key name
		hardware_result = RegEnumKeyEx(hardware_key, hardware,
								reg_string + hardware_key_pos,
								&name_length, NULL, NULL, NULL, NULL);
		if (hardware_result == ERROR_SUCCESS)
		{
			_tcscat(reg_string, TEXT("\\"));
			key_pos = _tcslen(reg_string);
			// open hardware key
			hardware_result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
											reg_string,
											0, KEY_READ, &reg_key);
			if (hardware_result == ERROR_SUCCESS)
			{
				// We now have to enumerate the keys in Hardware key.  You may have
				// more then one of the same device.
				instance_result = ERROR_SUCCESS;
				for (instance = 0; instance_result == ERROR_SUCCESS; instance++)
				{
					name_length = MAX_PATH - key_pos;
					// get sub key name
					instance_result = RegEnumKeyEx(reg_key, instance,
											reg_string + key_pos,
											&name_length, NULL, NULL, NULL, NULL);
					if (instance_result == ERROR_SUCCESS)
					{
						// open sub_key
						instance_result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
												reg_string,
												0, KEY_READ, &sub_key);
						if (instance_result == ERROR_SUCCESS)
						{
							// get the ParentIdPrefix of this instance of the hardware
							name_length = sizeof(test_parent_id_prefix);
							if (RegQueryValueEx(sub_key,
													TEXT("ParentIdPrefix"),
													NULL, NULL,
													(LPBYTE) test_parent_id_prefix,
													&name_length) == ERROR_SUCCESS)
							{
								// is this the ParentIdPrefix that we are looking for?
								if (!_tcsncmp(test_parent_id_prefix, parent_id_prefix, _tcslen(test_parent_id_prefix)))
									break;
							} // keep looking
							RegCloseKey(sub_key);
						} // next - can't open sub-key
					} // next - can't enumerate or we are out of instances of the hardware
				} // check next hardware instance
				RegCloseKey(reg_key);
				if (instance_result == ERROR_SUCCESS) break;
			} // done - can't open Hardware key
		} // done - can't enumerate or we are out of USB devices
	} // check next USB instance

	RegCloseKey(hardware_key);

	if (instance_result == ERROR_SUCCESS)
	{
		// We should now be at the USB parent device.  Get the real mouse name.
		name_length = MAX_PATH;
		instance_result = RegQueryValueEx(sub_key,
								TEXT("LocationInformation"),
								NULL, NULL,
								mouse_name[mouse_num],
								&name_length);
		RegCloseKey(sub_key);
		RegCloseKey(reg_key);
	}

	return;
}



//============================================================
//  is_rm_rdp_mouse
//============================================================
// returns TRUE if it is a remote desktop mouse

// A weak solution to weeding out the Terminal Services virtual mouse from the
// list of devices. I read somewhere that you shouldn't trust the device name's format.
// In other words, you shouldn't do what I'm doing.
// Here's the quote from http://www.jsiinc.com/SUBJ/tip4600/rh4628.htm:
//   "Note that you should not make any programmatic assumption about how an instance ID
//    is formatted. To determine root devices, you can check device status bits."
//  So tell me, what are these (how you say) "status bits" and where can I get some?

static BOOL is_rm_rdp_mouse(const TCHAR *device_string)
{
	return !(_tcsncmp(device_string, TEXT("\\??\\Root#RDP_MOU#0000#"), 22));
}



//============================================================
//  register_raw_mouse
//============================================================
// returns TRUE if registration succeeds

static BOOL register_raw_mouse(void)
{
	// This function registers to receive the WM_INPUT messages
	RAWINPUTDEVICE rid[1]; // Register only for mouse messages from wm_input.

	//register to get wm_input messages
	rid[0].usUsagePage = 0x01;
	rid[0].usUsage = 0x02;
	rid[0].dwFlags = 0;// RIDEV_NOLEGACY;   // adds HID mouse and also ignores legacy mouse messages
	rid[0].hwndTarget = NULL;

	// Register to receive the WM_INPUT message for any change in mouse
	// (buttons, wheel, and movement will all generate the same message)
	return ((*_RegisterRawInputDevices)(rid, 1, sizeof (rid[0])));
}



//============================================================
//  init_raw_mouse
//============================================================
// returns TRUE if initialization succeeds

static BOOL init_raw_mouse(void)
{
	PRAWINPUTDEVICELIST raw_input_device_list;
	TCHAR *ps_name = NULL;
	HMODULE user32;
	int input_devices, size, i;

	/* Check to see if OS is raw input capable */
	user32 = LoadLibrary(TEXT("user32.dll"));
	if (!user32) goto cant_use_raw_input;
	_RegisterRawInputDevices = (pRegisterRawInputDevices)GetProcAddress(user32,"RegisterRawInputDevices");
	if (!_RegisterRawInputDevices) goto cant_use_raw_input;
	_GetRawInputDeviceList = (pGetRawInputDeviceList)GetProcAddress(user32,"GetRawInputDeviceList");
	if (!_GetRawInputDeviceList) goto cant_use_raw_input;
#ifdef UNICODE
	_GetRawInputDeviceInfo = (pGetRawInputDeviceInfo)GetProcAddress(user32,"GetRawInputDeviceInfoW");
#else
	_GetRawInputDeviceInfo = (pGetRawInputDeviceInfo)GetProcAddress(user32,"GetRawInputDeviceInfoA");
#endif
	if (!_GetRawInputDeviceInfo) goto cant_use_raw_input;
	_GetRawInputData = (pGetRawInputData)GetProcAddress(user32,"GetRawInputData");
	if (!_GetRawInputData) goto cant_use_raw_input;

	// 1st call to GetRawInputDeviceList: Pass NULL to get the number of devices.
	if ((*_GetRawInputDeviceList)(NULL, &input_devices, sizeof(RAWINPUTDEVICELIST)) != 0)
		goto cant_use_raw_input;

	// Allocate the array to hold the DeviceList
	if ((raw_input_device_list = malloc(sizeof(RAWINPUTDEVICELIST) * input_devices)) == NULL)
		goto cant_use_raw_input;

	// 2nd call to GetRawInputDeviceList: Pass the pointer to our DeviceList and GetRawInputDeviceList() will fill the array
	if ((*_GetRawInputDeviceList)(raw_input_device_list, &input_devices, sizeof(RAWINPUTDEVICELIST)) == -1)
		goto cant_create_raw_input;

	// Loop through all devices and setup the mice.
	// RAWMOUSE reports the list last mouse to first,
	//  so we will read from the end of the list first.
	// Otherwise every new mouse plugged in becomes mouse 1.
	for (i = input_devices - 1; (i >= 0) && (mouse_count < MAX_MICE); i--)
	{
		if (raw_input_device_list[i].dwType == RIM_TYPEMOUSE)
		{
			/* Get the device name and use it to determine if it's the RDP Terminal Services virtual device. */

			// 1st call to GetRawInputDeviceInfo: Pass NULL to get the size of the device name
			if ((*_GetRawInputDeviceInfo)(raw_input_device_list[i].hDevice, RIDI_DEVICENAME, NULL, &size) != 0)
				goto cant_create_raw_input;

			// Allocate the array to hold the name
			if ((ps_name = (TCHAR *)malloc(sizeof(TCHAR) * size)) == NULL)
				goto cant_create_raw_input;

			// 2nd call to GetRawInputDeviceInfo: Pass our pointer to get the device name
			if ((int)(*_GetRawInputDeviceInfo)(raw_input_device_list[i].hDevice, RIDI_DEVICENAME, ps_name, &size) < 0)
				goto cant_create_raw_input;

			// Use this mouse if it's not an RDP mouse
			if (!is_rm_rdp_mouse(ps_name))
			{
				raw_mouse_device[mouse_count].device_handle = raw_input_device_list[i].hDevice;
				set_rawmouse_device_name(ps_name, mouse_count);
				mouse_count++;
			}

			free(ps_name);
		}
	}

	free(raw_input_device_list);

	// only use raw mouse support for multiple mice
	if (mouse_count < 2)
	{
		mouse_count = 0;
		goto dont_use_raw_input;
	}

	// finally, register to recieve raw input WM_INPUT messages
	if (!register_raw_mouse())
		goto cant_init_raw_input;

	verbose_printf("Input: Using RAWMOUSE for Mouse input\n");
	mouse_num_of_buttons = 5;

	// override lightgun settings.  Not needed with RAWinput.
	use_lightgun = 0;
	use_lightgun_dual = 0;
	// There is no way to know if a mouse is a lightgun or mouse at init.
	// Treat every mouse as a possible lightgun.
	// When the data is received from the device,
	//   it will update the mouse data if relative,
	//   else it will update the lightgun data if absolute.
	// The only way I can think of to tell if a mouse is a lightgun is to
	//  use the device id.  And keep a list of known lightguns.
	lightgun_count = mouse_count;
	return 1;

cant_create_raw_input:
	free(raw_input_device_list);
	free(ps_name);
dont_use_raw_input:
cant_use_raw_input:
cant_init_raw_input:
	return 0;
}



//============================================================
//  process_raw_input
//============================================================

static void process_raw_input(PRAWINPUT raw)
{
	int i;
	USHORT button_flags;
	BYTE *buttons;

	osd_lock_acquire(raw_mouse_lock);
	for ( i=0; i < mouse_count; i++)
	{
		if (raw_mouse_device[i].device_handle == raw->header.hDevice)
		{
			// Update the axis values for the specified mouse
			if (raw_mouse_device[i].flags == MOUSE_MOVE_RELATIVE)
			{
				raw_mouse_device[i].mouse_state.lX += raw->data.mouse.lLastX;
				raw_mouse_device[i].mouse_state.lY += raw->data.mouse.lLastY;
				// The Z axis uses a signed SHORT value stored in a unsigned USHORT.
				raw_mouse_device[i].mouse_state.lZ += (LONG)((SHORT)raw->data.mouse.usButtonData);
			}
			else
			{
				raw_mouse_device[i].mouse_state.lX = raw->data.mouse.lLastX;
				raw_mouse_device[i].mouse_state.lY = raw->data.mouse.lLastY;
				// We will assume the Z axis is also absolute but it might be relative
				raw_mouse_device[i].mouse_state.lZ = (LONG)((SHORT)raw->data.mouse.usButtonData);
			}

			// The following 2 lines are just to avoid a lot of pointer re-direction
			button_flags = raw->data.mouse.usButtonFlags;
			buttons = raw_mouse_device[i].mouse_state.rgbButtons;

			// Update the button values for the specified mouse
			if (button_flags & RI_MOUSE_BUTTON_1_DOWN) buttons[0] = 0x80;
			if (button_flags & RI_MOUSE_BUTTON_1_UP)   buttons[0] = 0;
			if (button_flags & RI_MOUSE_BUTTON_2_DOWN) buttons[1] = 0x80;
			if (button_flags & RI_MOUSE_BUTTON_2_UP)   buttons[1] = 0;
			if (button_flags & RI_MOUSE_BUTTON_3_DOWN) buttons[2] = 0x80;
			if (button_flags & RI_MOUSE_BUTTON_3_UP)   buttons[2] = 0;
			if (button_flags & RI_MOUSE_BUTTON_4_DOWN) buttons[3] = 0x80;
			if (button_flags & RI_MOUSE_BUTTON_4_UP)   buttons[3] = 0;
			if (button_flags & RI_MOUSE_BUTTON_5_DOWN) buttons[4] = 0x80;
			if (button_flags & RI_MOUSE_BUTTON_5_UP)   buttons[4] = 0;

			raw_mouse_device[i].flags = raw->data.mouse.usFlags;
		}
	}
	osd_lock_release(raw_mouse_lock);
}



//============================================================
//  win_raw_mouse_update
//============================================================
// returns TRUE if raw mouse info successfully updated

BOOL win_raw_mouse_update(HANDLE in_device_handle)
{
	//  When the WM_INPUT message is received, the lparam must be passed to this
	//  function to keep a running tally of all mouse moves.

	LPBYTE data;
	int size;

	if ((*_GetRawInputData)((HRAWINPUT)in_device_handle, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER)) == -1)
		goto cant_find_raw_data;

	data = (LPBYTE)malloc(sizeof(LPBYTE) * size);
	if (data == NULL)
		goto cant_find_raw_data;

	if ((*_GetRawInputData)((HRAWINPUT)in_device_handle, RID_INPUT, data, &size, sizeof(RAWINPUTHEADER)) != size )
		goto cant_read_raw_data;

	process_raw_input((RAWINPUT*)data);

	free(data);
	return 1;

cant_read_raw_data:
	free(data);
cant_find_raw_data:
	return 0;
}



//============================================================
//  win_read_raw_mouse
//============================================================
// use this to poll and reset the current mouse info

static void win_read_raw_mouse(void)
{
	int i;

	osd_lock_acquire(raw_mouse_lock);
	for ( i = 0; i < mouse_count; i++)
	{
		mouse_state[i] = raw_mouse_device[i].mouse_state;

		// set X,Y to MIN values if offscreen reload is used and fire
		if (use_lightgun_reload && mouse_state[i].rgbButtons[1] & 0x80)
		{
			mouse_state[i].lX = ANALOG_VALUE_MIN;
			mouse_state[i].lY = ANALOG_VALUE_MAX;
			mouse_state[i].rgbButtons[0] = 0x80;
		}

		// do not clear if absolute
		if (raw_mouse_device[i].flags == MOUSE_MOVE_RELATIVE)
		{
			raw_mouse_device[i].mouse_state.lX = 0;
			raw_mouse_device[i].mouse_state.lY = 0;
			raw_mouse_device[i].mouse_state.lZ = 0;
		}
	}
	osd_lock_release(raw_mouse_lock);
}
