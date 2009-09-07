/*********************************************************************

	inputx.c

	Secondary input related functions for MESS specific functionality

*********************************************************************/

#include <ctype.h>
#include <assert.h>
#include <wctype.h>
#include "inputx.h"
#include "inptport.h"
#include "mame.h"

#include "debug/debugcon.h"



/***************************************************************************
    CONSTANTS
***************************************************************************/

#define NUM_SIMUL_KEYS	(UCHAR_SHIFT_END - UCHAR_SHIFT_BEGIN + 1)
#define LOG_INPUTX		0
#define SPACE_COUNT		3
#define INVALID_CHAR	'?'
#define IP_NAME_DEFAULT	NULL



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef const input_port_config *input_port_config_c;
typedef const input_field_config *input_field_config_c;


typedef struct _mess_input_code mess_input_code;
struct _mess_input_code
{
	unicode_char ch;
	input_field_config_c field[NUM_SIMUL_KEYS];
};

typedef struct _key_buffer key_buffer;
struct _key_buffer
{
	int begin_pos;
	int end_pos;
	unsigned int status_keydown : 1;
	unicode_char buffer[4096];
};

typedef struct _char_info char_info;
struct _char_info
{
	unicode_char ch;
	const char *name;
	const char *alternate;	/* alternative string, in UTF-8 */
};



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

static const char_info charinfo[] =
{
	{ 0x0008,					"Backspace",	NULL },		/* Backspace */
	{ 0x0009,					"Tab",			"    " },	/* Tab */
	{ 0x000c,					"Clear",		NULL },		/* Clear */
	{ 0x000d,					"Enter",		NULL },		/* Enter */
	{ 0x001a,					"Esc",			NULL },		/* Esc */
	{ 0x0020,					"Space",		" " },		/* Space */
	{ 0x0061,					NULL,			"A" },		/* a */
	{ 0x0062,					NULL,			"B" },		/* b */
	{ 0x0063,					NULL,			"C" },		/* c */
	{ 0x0064,					NULL,			"D" },		/* d */
	{ 0x0065,					NULL,			"E" },		/* e */
	{ 0x0066,					NULL,			"F" },		/* f */
	{ 0x0067,					NULL,			"G" },		/* g */
	{ 0x0068,					NULL,			"H" },		/* h */
	{ 0x0069,					NULL,			"I" },		/* i */
	{ 0x006a,					NULL,			"J" },		/* j */
	{ 0x006b,					NULL,			"K" },		/* k */
	{ 0x006c,					NULL,			"L" },		/* l */
	{ 0x006d,					NULL,			"M" },		/* m */
	{ 0x006e,					NULL,			"N" },		/* n */
	{ 0x006f,					NULL,			"O" },		/* o */
	{ 0x0070,					NULL,			"P" },		/* p */
	{ 0x0071,					NULL,			"Q" },		/* q */
	{ 0x0072,					NULL,			"R" },		/* r */
	{ 0x0073,					NULL,			"S" },		/* s */
	{ 0x0074,					NULL,			"T" },		/* t */
	{ 0x0075,					NULL,			"U" },		/* u */
	{ 0x0076,					NULL,			"V" },		/* v */
	{ 0x0077,					NULL,			"W" },		/* w */
	{ 0x0078,					NULL,			"X" },		/* x */
	{ 0x0079,					NULL,			"Y" },		/* y */
	{ 0x007a,					NULL,			"Z" },		/* z */
	{ 0x00a0,					NULL,			" " },		/* non breaking space */
	{ 0x00a1,					NULL,			"!" },		/* inverted exclaimation mark */
	{ 0x00a6,					NULL,			"|" },		/* broken bar */
	{ 0x00a9,					NULL,			"(c)" },	/* copyright sign */
	{ 0x00ab,					NULL,			"<<" },		/* left pointing double angle */
	{ 0x00ae,					NULL,			"(r)" },	/* registered sign */
	{ 0x00bb,					NULL,			">>" },		/* right pointing double angle */
	{ 0x00bc,					NULL,			"1/4" },	/* vulgar fraction one quarter */
	{ 0x00bd,					NULL,			"1/2" },	/* vulgar fraction one half */
	{ 0x00be,					NULL,			"3/4" },	/* vulgar fraction three quarters */
	{ 0x00bf,					NULL,			"?" },		/* inverted question mark */
	{ 0x00c0,					NULL,			"A" },		/* 'A' grave */
	{ 0x00c1,					NULL,			"A" },		/* 'A' acute */
	{ 0x00c2,					NULL,			"A" },		/* 'A' circumflex */
	{ 0x00c3,					NULL,			"A" },		/* 'A' tilde */
	{ 0x00c4,					NULL,			"A" },		/* 'A' diaeresis */
	{ 0x00c5,					NULL,			"A" },		/* 'A' ring above */
	{ 0x00c6,					NULL,			"AE" },		/* 'AE' ligature */
	{ 0x00c7,					NULL,			"C" },		/* 'C' cedilla */
	{ 0x00c8,					NULL,			"E" },		/* 'E' grave */
	{ 0x00c9,					NULL,			"E" },		/* 'E' acute */
	{ 0x00ca,					NULL,			"E" },		/* 'E' circumflex */
	{ 0x00cb,					NULL,			"E" },		/* 'E' diaeresis */
	{ 0x00cc,					NULL,			"I" },		/* 'I' grave */
	{ 0x00cd,					NULL,			"I" },		/* 'I' acute */
	{ 0x00ce,					NULL,			"I" },		/* 'I' circumflex */
	{ 0x00cf,					NULL,			"I" },		/* 'I' diaeresis */
	{ 0x00d0,					NULL,			"D" },		/* 'ETH' */
	{ 0x00d1,					NULL,			"N" },		/* 'N' tilde */
	{ 0x00d2,					NULL,			"O" },		/* 'O' grave */
	{ 0x00d3,					NULL,			"O" },		/* 'O' acute */
	{ 0x00d4,					NULL,			"O" },		/* 'O' circumflex */
	{ 0x00d5,					NULL,			"O" },		/* 'O' tilde */
	{ 0x00d6,					NULL,			"O" },		/* 'O' diaeresis */
	{ 0x00d7,					NULL,			"X" },		/* multiplication sign */
	{ 0x00d8,					NULL,			"O" },		/* 'O' stroke */
	{ 0x00d9,					NULL,			"U" },		/* 'U' grave */
	{ 0x00da,					NULL,			"U" },		/* 'U' acute */
	{ 0x00db,					NULL,			"U" },		/* 'U' circumflex */
	{ 0x00dc,					NULL,			"U" },		/* 'U' diaeresis */
	{ 0x00dd,					NULL,			"Y" },		/* 'Y' acute */
	{ 0x00df,					NULL,			"SS" },		/* sharp S */
	{ 0x00e0,					NULL,			"a" },		/* 'a' grave */
	{ 0x00e1,					NULL,			"a" },		/* 'a' acute */
	{ 0x00e2,					NULL,			"a" },		/* 'a' circumflex */
	{ 0x00e3,					NULL,			"a" },		/* 'a' tilde */
	{ 0x00e4,					NULL,			"a" },		/* 'a' diaeresis */
	{ 0x00e5,					NULL,			"a" },		/* 'a' ring above */
	{ 0x00e6,					NULL,			"ae" },		/* 'ae' ligature */
	{ 0x00e7,					NULL,			"c" },		/* 'c' cedilla */
	{ 0x00e8,					NULL,			"e" },		/* 'e' grave */
	{ 0x00e9,					NULL,			"e" },		/* 'e' acute */
	{ 0x00ea,					NULL,			"e" },		/* 'e' circumflex */
	{ 0x00eb,					NULL,			"e" },		/* 'e' diaeresis */
	{ 0x00ec,					NULL,			"i" },		/* 'i' grave */
	{ 0x00ed,					NULL,			"i" },		/* 'i' acute */
	{ 0x00ee,					NULL,			"i" },		/* 'i' circumflex */
	{ 0x00ef,					NULL,			"i" },		/* 'i' diaeresis */
	{ 0x00f0,					NULL,			"d" },		/* 'eth' */
	{ 0x00f1,					NULL,			"n" },		/* 'n' tilde */
	{ 0x00f2,					NULL,			"o" },		/* 'o' grave */
	{ 0x00f3,					NULL,			"o" },		/* 'o' acute */
	{ 0x00f4,					NULL,			"o" },		/* 'o' circumflex */
	{ 0x00f5,					NULL,			"o" },		/* 'o' tilde */
	{ 0x00f6,					NULL,			"o" },		/* 'o' diaeresis */
	{ 0x00f8,					NULL,			"o" },		/* 'o' stroke */
	{ 0x00f9,					NULL,			"u" },		/* 'u' grave */
	{ 0x00fa,					NULL,			"u" },		/* 'u' acute */
	{ 0x00fb,					NULL,			"u" },		/* 'u' circumflex */
	{ 0x00fc,					NULL,			"u" },		/* 'u' diaeresis */
	{ 0x00fd,					NULL,			"y" },		/* 'y' acute */
	{ 0x00ff,					NULL,			"y" },		/* 'y' diaeresis */
	{ 0x2010,					NULL,			"-" },		/* hyphen */
	{ 0x2011,					NULL,			"-" },		/* non-breaking hyphen */
	{ 0x2012,					NULL,			"-" },		/* figure dash */
	{ 0x2013,					NULL,			"-" },		/* en dash */
	{ 0x2014,					NULL,			"-" },		/* em dash */
	{ 0x2015,					NULL,			"-" },		/* horizontal dash */
	{ 0x2018,					NULL,			"\'" },		/* left single quotation mark */
	{ 0x2019,					NULL,			"\'" },		/* right single quotation mark */
	{ 0x201a,					NULL,			"\'" },		/* single low quotation mark */
	{ 0x201b,					NULL,			"\'" },		/* single high reversed quotation mark */
	{ 0x201c,					NULL,			"\"" },		/* left double quotation mark */
	{ 0x201d,					NULL,			"\"" },		/* right double quotation mark */
	{ 0x201e,					NULL,			"\"" },		/* double low quotation mark */
	{ 0x201f,					NULL,			"\"" },		/* double high reversed quotation mark */
	{ 0x2024,					NULL,			"." },		/* one dot leader */
	{ 0x2025,					NULL,			".." },		/* two dot leader */
	{ 0x2026,					NULL,			"..." },	/* horizontal ellipsis */
	{ 0x2047,					NULL,			"??" },		/* double question mark */
	{ 0x2048,					NULL,			"?!" },		/* question exclamation mark */
	{ 0x2049,					NULL,			"!?" },		/* exclamation question mark */
	{ 0xff01,					NULL,			"!" },		/* fullwidth exclamation point */
	{ 0xff02,					NULL,			"\"" },		/* fullwidth quotation mark */
	{ 0xff03,					NULL,			"#" },		/* fullwidth number sign */
	{ 0xff04,					NULL,			"$" },		/* fullwidth dollar sign */
	{ 0xff05,					NULL,			"%" },		/* fullwidth percent sign */
	{ 0xff06,					NULL,			"&" },		/* fullwidth ampersand */
	{ 0xff07,					NULL,			"\'" },		/* fullwidth apostrophe */
	{ 0xff08,					NULL,			"(" },		/* fullwidth left parenthesis */
	{ 0xff09,					NULL,			")" },		/* fullwidth right parenthesis */
	{ 0xff0a,					NULL,			"*" },		/* fullwidth asterisk */
	{ 0xff0b,					NULL,			"+" },		/* fullwidth plus */
	{ 0xff0c,					NULL,			"," },		/* fullwidth comma */
	{ 0xff0d,					NULL,			"-" },		/* fullwidth minus */
	{ 0xff0e,					NULL,			"." },		/* fullwidth period */
	{ 0xff0f,					NULL,			"/" },		/* fullwidth slash */
	{ 0xff10,					NULL,			"0" },		/* fullwidth zero */
	{ 0xff11,					NULL,			"1" },		/* fullwidth one */
	{ 0xff12,					NULL,			"2" },		/* fullwidth two */
	{ 0xff13,					NULL,			"3" },		/* fullwidth three */
	{ 0xff14,					NULL,			"4" },		/* fullwidth four */
	{ 0xff15,					NULL,			"5" },		/* fullwidth five */
	{ 0xff16,					NULL,			"6" },		/* fullwidth six */
	{ 0xff17,					NULL,			"7" },		/* fullwidth seven */
	{ 0xff18,					NULL,			"8" },		/* fullwidth eight */
	{ 0xff19,					NULL,			"9" },		/* fullwidth nine */
	{ 0xff1a,					NULL,			":" },		/* fullwidth colon */
	{ 0xff1b,					NULL,			";" },		/* fullwidth semicolon */
	{ 0xff1c,					NULL,			"<" },		/* fullwidth less than sign */
	{ 0xff1d,					NULL,			"=" },		/* fullwidth equals sign */
	{ 0xff1e,					NULL,			">" },		/* fullwidth greater than sign */
	{ 0xff1f,					NULL,			"?" },		/* fullwidth question mark */
	{ 0xff20,					NULL,			"@" },		/* fullwidth at sign */
	{ 0xff21,					NULL,			"A" },		/* fullwidth 'A' */
	{ 0xff22,					NULL,			"B" },		/* fullwidth 'B' */
	{ 0xff23,					NULL,			"C" },		/* fullwidth 'C' */
	{ 0xff24,					NULL,			"D" },		/* fullwidth 'D' */
	{ 0xff25,					NULL,			"E" },		/* fullwidth 'E' */
	{ 0xff26,					NULL,			"F" },		/* fullwidth 'F' */
	{ 0xff27,					NULL,			"G" },		/* fullwidth 'G' */
	{ 0xff28,					NULL,			"H" },		/* fullwidth 'H' */
	{ 0xff29,					NULL,			"I" },		/* fullwidth 'I' */
	{ 0xff2a,					NULL,			"J" },		/* fullwidth 'J' */
	{ 0xff2b,					NULL,			"K" },		/* fullwidth 'K' */
	{ 0xff2c,					NULL,			"L" },		/* fullwidth 'L' */
	{ 0xff2d,					NULL,			"M" },		/* fullwidth 'M' */
	{ 0xff2e,					NULL,			"N" },		/* fullwidth 'N' */
	{ 0xff2f,					NULL,			"O" },		/* fullwidth 'O' */
	{ 0xff30,					NULL,			"P" },		/* fullwidth 'P' */
	{ 0xff31,					NULL,			"Q" },		/* fullwidth 'Q' */
	{ 0xff32,					NULL,			"R" },		/* fullwidth 'R' */
	{ 0xff33,					NULL,			"S" },		/* fullwidth 'S' */
	{ 0xff34,					NULL,			"T" },		/* fullwidth 'T' */
	{ 0xff35,					NULL,			"U" },		/* fullwidth 'U' */
	{ 0xff36,					NULL,			"V" },		/* fullwidth 'V' */
	{ 0xff37,					NULL,			"W" },		/* fullwidth 'W' */
	{ 0xff38,					NULL,			"X" },		/* fullwidth 'X' */
	{ 0xff39,					NULL,			"Y" },		/* fullwidth 'Y' */
	{ 0xff3a,					NULL,			"Z" },		/* fullwidth 'Z' */
	{ 0xff3b,					NULL,			"[" },		/* fullwidth left bracket */
	{ 0xff3c,					NULL,			"\\" },		/* fullwidth backslash */
	{ 0xff3d,					NULL,			"]"	},		/* fullwidth right bracket */
	{ 0xff3e,					NULL,			"^" },		/* fullwidth caret */
	{ 0xff3f,					NULL,			"_" },		/* fullwidth underscore */
	{ 0xff40,					NULL,			"`" },		/* fullwidth backquote */
	{ 0xff41,					NULL,			"a" },		/* fullwidth 'a' */
	{ 0xff42,					NULL,			"b" },		/* fullwidth 'b' */
	{ 0xff43,					NULL,			"c" },		/* fullwidth 'c' */
	{ 0xff44,					NULL,			"d" },		/* fullwidth 'd' */
	{ 0xff45,					NULL,			"e" },		/* fullwidth 'e' */
	{ 0xff46,					NULL,			"f" },		/* fullwidth 'f' */
	{ 0xff47,					NULL,			"g" },		/* fullwidth 'g' */
	{ 0xff48,					NULL,			"h" },		/* fullwidth 'h' */
	{ 0xff49,					NULL,			"i" },		/* fullwidth 'i' */
	{ 0xff4a,					NULL,			"j" },		/* fullwidth 'j' */
	{ 0xff4b,					NULL,			"k" },		/* fullwidth 'k' */
	{ 0xff4c,					NULL,			"l" },		/* fullwidth 'l' */
	{ 0xff4d,					NULL,			"m" },		/* fullwidth 'm' */
	{ 0xff4e,					NULL,			"n" },		/* fullwidth 'n' */
	{ 0xff4f,					NULL,			"o" },		/* fullwidth 'o' */
	{ 0xff50,					NULL,			"p" },		/* fullwidth 'p' */
	{ 0xff51,					NULL,			"q" },		/* fullwidth 'q' */
	{ 0xff52,					NULL,			"r" },		/* fullwidth 'r' */
	{ 0xff53,					NULL,			"s" },		/* fullwidth 's' */
	{ 0xff54,					NULL,			"t" },		/* fullwidth 't' */
	{ 0xff55,					NULL,			"u" },		/* fullwidth 'u' */
	{ 0xff56,					NULL,			"v" },		/* fullwidth 'v' */
	{ 0xff57,					NULL,			"w" },		/* fullwidth 'w' */
	{ 0xff58,					NULL,			"x" },		/* fullwidth 'x' */
	{ 0xff59,					NULL,			"y" },		/* fullwidth 'y' */
	{ 0xff5a,					NULL,			"z" },		/* fullwidth 'z' */
	{ 0xff5b,					NULL,			"{" },		/* fullwidth left brace */
	{ 0xff5c,					NULL,			"|" },		/* fullwidth vertical bar */
	{ 0xff5d,					NULL,			"}" },		/* fullwidth right brace */
	{ 0xff5e,					NULL,			"~" },		/* fullwidth tilde */
	{ 0xff5f,					NULL,			"((" },		/* fullwidth double left parenthesis */
	{ 0xff60,					NULL,			"))" },		/* fullwidth double right parenthesis */
	{ 0xffe0,					NULL,			"\xC2\xA2" },		/* fullwidth cent sign */
	{ 0xffe1,					NULL,			"\xC2\xA3" },		/* fullwidth pound sign */
	{ 0xffe4,					NULL,			"\xC2\xA4" },		/* fullwidth broken bar */
	{ 0xffe5,					NULL,			"\xC2\xA5" },		/* fullwidth yen sign */
	{ 0xffe6,					NULL,			"\xE2\x82\xA9" },	/* fullwidth won sign */
	{ 0xffe9,					NULL,			"\xE2\x86\x90" },	/* fullwidth left arrow */
	{ 0xffea,					NULL,			"\xE2\x86\x91" },	/* fullwidth up arrow */
	{ 0xffeb,					NULL,			"\xE2\x86\x92" },	/* fullwidth right arrow */
	{ 0xffec,					NULL,			"\xE2\x86\x93" },	/* fullwidth down arrow */
	{ 0xffed,					NULL,			"\xE2\x96\xAA" },	/* fullwidth solid box */
	{ 0xffee,					NULL,			"\xE2\x97\xA6" },	/* fullwidth open circle */
	{ UCHAR_SHIFT_1,			"Shift",		NULL },		/* Shift key */
	{ UCHAR_SHIFT_2,			"Ctrl",			NULL },		/* Ctrl key */
	{ UCHAR_MAMEKEY(F1),		"F1",			NULL },		/* F1 function key */
	{ UCHAR_MAMEKEY(F2),		"F2",			NULL },		/* F2 function key */
	{ UCHAR_MAMEKEY(F3),		"F3",			NULL },		/* F3 function key */
	{ UCHAR_MAMEKEY(F4),		"F4",			NULL },		/* F4 function key */
	{ UCHAR_MAMEKEY(F5),		"F5",			NULL },		/* F5 function key */
	{ UCHAR_MAMEKEY(F6),		"F6",			NULL },		/* F6 function key */
	{ UCHAR_MAMEKEY(F7),		"F7",			NULL },		/* F7 function key */
	{ UCHAR_MAMEKEY(F8),		"F8",			NULL },		/* F8 function key */
	{ UCHAR_MAMEKEY(F9),		"F9",			NULL },		/* F9 function key */
	{ UCHAR_MAMEKEY(F10),		"F10",			NULL },		/* F10 function key */
	{ UCHAR_MAMEKEY(F11),		"F11",			NULL },		/* F11 function key */
	{ UCHAR_MAMEKEY(F12),		"F12",			NULL },		/* F12 function key */
	{ UCHAR_MAMEKEY(F13),		"F13",			NULL },		/* F13 function key */
	{ UCHAR_MAMEKEY(F14),		"F14",			NULL },		/* F14 function key */
	{ UCHAR_MAMEKEY(F15),		"F15",			NULL },		/* F15 function key */
	{ UCHAR_MAMEKEY(ESC),		"Esc",			"\033" },	/* Esc key */
	{ UCHAR_MAMEKEY(INSERT),	"Insert",		NULL },		/* Insert key */
	{ UCHAR_MAMEKEY(DEL),		"Delete",		"\010" },	/* Delete key */
	{ UCHAR_MAMEKEY(HOME),		"Home",			"\014" },	/* Home key */
	{ UCHAR_MAMEKEY(END),		"End",			NULL },		/* End key */
	{ UCHAR_MAMEKEY(PGUP),		"Page Up",		NULL },		/* Page Up key */
	{ UCHAR_MAMEKEY(PGDN),		"Page Down",	NULL },		/* Page Down key */
	{ UCHAR_MAMEKEY(LEFT),		"Cursor Left",	NULL },		/* Cursor Left */
	{ UCHAR_MAMEKEY(RIGHT),		"Cursor Right",	NULL },		/* Cursor Right */
	{ UCHAR_MAMEKEY(UP),		"Cursor Up",	NULL },		/* Cursor Up */
	{ UCHAR_MAMEKEY(DOWN),		"Cursor Down",	NULL },		/* Cursor Down */
	{ UCHAR_MAMEKEY(0_PAD),		"Keypad 0",		NULL },		/* 0 on the numeric keypad */
	{ UCHAR_MAMEKEY(1_PAD),		"Keypad 1",		NULL },		/* 1 on the numeric keypad */
	{ UCHAR_MAMEKEY(2_PAD),		"Keypad 2",		NULL },		/* 2 on the numeric keypad */
	{ UCHAR_MAMEKEY(3_PAD),		"Keypad 3",		NULL },		/* 3 on the numeric keypad */
	{ UCHAR_MAMEKEY(4_PAD),		"Keypad 4",		NULL },		/* 4 on the numeric keypad */
	{ UCHAR_MAMEKEY(5_PAD),		"Keypad 5",		NULL },		/* 5 on the numeric keypad */
	{ UCHAR_MAMEKEY(6_PAD),		"Keypad 6",		NULL },		/* 6 on the numeric keypad */
	{ UCHAR_MAMEKEY(7_PAD),		"Keypad 7",		NULL },		/* 7 on the numeric keypad */
	{ UCHAR_MAMEKEY(8_PAD),		"Keypad 8",		NULL },		/* 8 on the numeric keypad */
	{ UCHAR_MAMEKEY(9_PAD),		"Keypad 9",		NULL },		/* 9 on the numeric keypad */
	{ UCHAR_MAMEKEY(SLASH_PAD),	"Keypad /",		NULL },		/* / on the numeric keypad */
	{ UCHAR_MAMEKEY(ASTERISK),	"Keypad *",		NULL },		/* * on the numeric keypad */
	{ UCHAR_MAMEKEY(MINUS_PAD),	"Keypad -",		NULL },		/* - on the numeric Keypad */
	{ UCHAR_MAMEKEY(PLUS_PAD),	"Keypad +",		NULL },		/* + on the numeric Keypad */
	{ UCHAR_MAMEKEY(DEL_PAD),	"Keypad .",		NULL },		/* . on the numeric keypad */
	{ UCHAR_MAMEKEY(ENTER_PAD),	"Keypad Enter",	NULL },		/* Enter on the numeric keypad */
	{ UCHAR_MAMEKEY(PRTSCR),	"Print Screen",	NULL },		/* Print Screen key */
	{ UCHAR_MAMEKEY(PAUSE),		"Pause",		NULL },		/* Pause key */
	{ UCHAR_MAMEKEY(LSHIFT),	"Left Shift",	NULL },		/* Left Shift key */
	{ UCHAR_MAMEKEY(RSHIFT),	"Right Shift",	NULL },		/* Right Shift key */
	{ UCHAR_MAMEKEY(LCONTROL),	"Left Ctrl",	NULL },		/* Left Control key */
	{ UCHAR_MAMEKEY(RCONTROL),	"Right Ctrl",	NULL },		/* Right Control key */
	{ UCHAR_MAMEKEY(LALT),		"Left Alt",		NULL },		/* Left Alt key */
	{ UCHAR_MAMEKEY(RALT),		"Right Alt",	NULL },		/* Right Alt key */
	{ UCHAR_MAMEKEY(SCRLOCK),	"Scroll Lock",	NULL },		/* Scroll Lock key */
	{ UCHAR_MAMEKEY(NUMLOCK),	"Num Lock",		NULL },		/* Num Lock key */
	{ UCHAR_MAMEKEY(CAPSLOCK),	"Caps Lock",	NULL },		/* Caps Lock key */
	{ UCHAR_MAMEKEY(LWIN),		"Left Win",		NULL },		/* Left Win key */
	{ UCHAR_MAMEKEY(RWIN),		"Right Win",	NULL },		/* Right Win key */
	{ UCHAR_MAMEKEY(MENU),		"Menu",			NULL },		/* Menu key */
	{ UCHAR_MAMEKEY(CANCEL),	"Break",		NULL }		/* Break/Pause key */
};




/***************************************************************************
    MISCELLANEOUS
***************************************************************************/

/*-------------------------------------------------
    find_charinfo - looks up information about a
	particular character
-------------------------------------------------*/

static const char_info *find_charinfo(unicode_char target_char)
{
	int low = 0;
	int high = sizeof(charinfo) / sizeof(charinfo[0]);
	int i;
	unicode_char ch;

	/* perform a simple binary search to find the proper alternate */
	while(high > low)
	{
		i = (high + low) / 2;
		ch = charinfo[i].ch;
		if (ch < target_char)
			low = i + 1;
		else if (ch > target_char)
			high = i;
		else
			return &charinfo[i];
	}
	return NULL;
}



/***************************************************************************
    CODE ASSEMBLING
***************************************************************************/

/*-------------------------------------------------
    code_point_string - obtain a string representation of a
	given code; used for logging and debugging
-------------------------------------------------*/

static const char *code_point_string(running_machine *machine, unicode_char ch)
{
	static char buf[16];
	const char *result = buf;

	switch(ch)
	{
		/* check some magic values */
		case '\0':	strcpy(buf, "\\0");		break;
		case '\r':	strcpy(buf, "\\r");		break;
		case '\n':	strcpy(buf, "\\n");		break;
		case '\t':	strcpy(buf, "\\t");		break;

		default:
			if ((ch >= 32) && (ch < 128))
			{
				/* seven bit ASCII is easy */
				buf[0] = (char) ch;
				buf[1] = '\0';
			}
			else if (ch >= UCHAR_MAMEKEY_BEGIN)
			{
				/* try to obtain a codename with input_code_name(); this can result in an empty string */
				astring *astr = astring_alloc();
				input_code_name(machine, astr, (input_code) ch - UCHAR_MAMEKEY_BEGIN);
				snprintf(buf, ARRAY_LENGTH(buf), "%s", astring_c(astr));
				astring_free(astr);
			}
			else
			{
				/* empty string; resolve later */
				buf[0] = '\0';
			}

			/* did we fail to resolve? if so, we have a last resort */
			if (buf[0] == '\0')
				snprintf(buf, ARRAY_LENGTH(buf), "U+%04X", (unsigned) ch);
			break;
	}
	return result;
}



/*-------------------------------------------------
    get_keyboard_code - accesses a particular
	keyboard code
-------------------------------------------------*/

static unicode_char get_keyboard_code(const input_field_config *field, int i)
{
	unicode_char ch = field->chars[i];

	/* special hack to allow for PORT_CODE('\xA3') */
	if ((ch >= 0xFFFFFF80) && (ch <= 0xFFFFFFFF))
		ch &= 0xFF;
	return ch;
}



/*-------------------------------------------------
    scan_keys - scans through input ports and
	sets up natural keyboard input mapping
-------------------------------------------------*/

static int scan_keys(running_machine *machine, const input_port_config *portconfig, mess_input_code *codes, input_port_config_c *ports, input_field_config_c *shift_ports, int keys, int shift)
{
	int code_count = 0;
	const input_port_config *port;
	const input_field_config *field;
	unicode_char code;

	assert(keys < NUM_SIMUL_KEYS);

	for (port = portconfig; port != NULL; port = port->next)
	{
		for (field = port->fieldlist; field != NULL; field = field->next)
		{
			if (field->type == IPT_KEYBOARD)
			{
				code = get_keyboard_code(field, shift);
				if (code != 0)
				{
					/* is this a shifter key? */
					if ((code >= UCHAR_SHIFT_BEGIN) && (code <= UCHAR_SHIFT_END))
					{
						shift_ports[keys] = field;
						code_count += scan_keys(machine,
							portconfig,
							codes ? &codes[code_count] : NULL,
							ports,
							shift_ports,
							keys+1,
							code - UCHAR_SHIFT_1 + 1);
					}
					else
					{
						/* not a shifter key; record normally */
						if (codes)
						{
							/* if we have a destination, record the codes used here */
							memcpy((void *) codes[code_count].field, shift_ports, sizeof(shift_ports[0]) * keys);
							codes[code_count].ch = code;
							codes[code_count].field[keys] = field;
						}

						/* increment the count */
						code_count++;

						if (LOG_INPUTX)
							logerror("inputx: code=%i (%s) port=%p field->name='%s'\n", (int) code, code_point_string(machine, code), port, field->name);
					}
				}
			}
		}
	}
	return code_count;
}



/*-------------------------------------------------
    build_codes - given an input port table, create
	a input code table useful for mapping unicode
	chars
-------------------------------------------------*/

static mess_input_code *build_codes(running_machine *machine, const input_port_config *portconfig)
{
	mess_input_code *codes = NULL;
	const input_port_config *ports[NUM_SIMUL_KEYS];
	const input_field_config *fields[NUM_SIMUL_KEYS];
	int code_count;

	/* first count the number of codes */
	code_count = scan_keys(machine, portconfig, NULL, ports, fields, 0, 0);
	if (code_count > 0)
	{
		/* allocate the codes */
		codes = auto_alloc_array_clear(machine, mess_input_code, code_count + 1);
	
		/* and populate them */
		scan_keys(machine, portconfig, codes, ports, fields, 0, 0);
	}
	return codes;
}



/***************************************************************************
    VALIDITY CHECKS
***************************************************************************/

/*-------------------------------------------------
    mess_validate_input_ports - validates the
	natural keyboard setup of a set of input ports
-------------------------------------------------*/

int mess_validate_input_ports(int drivnum, const machine_config *config, const input_port_config *portlist)
{
	int error = FALSE;

	if (drivers[drivnum]->flags & GAME_COMPUTER)
	{
		/* unused hook */
	}

	return error;
}



/*-------------------------------------------------
    mess_validate_natural_keyboard_statics - 
	validates natural keyboard static data
-------------------------------------------------*/

int mess_validate_natural_keyboard_statics(void)
{
	int i;
	int error = FALSE;
	unicode_char last_char = 0;
	const char_info *ci;

	/* check to make sure that charinfo is in order */
	for (i = 0; i < sizeof(charinfo) / sizeof(charinfo[0]); i++)
	{
		if (last_char >= charinfo[i].ch)
		{
			mame_printf_error("inputx: charinfo is out of order; 0x%08x should be higher than 0x%08x\n", charinfo[i].ch, last_char);
			error = TRUE;
		}
		last_char = charinfo[i].ch;
	}

	/* check to make sure that I can look up everything on alternate_charmap */
	for (i = 0; i < sizeof(charinfo) / sizeof(charinfo[0]); i++)
	{
		ci = find_charinfo(charinfo[i].ch);
		if (ci != &charinfo[i])
		{
			mame_printf_error("inputx: expected find_charinfo(0x%08x) to work properly\n", charinfo[i].ch);
			error = TRUE;
		}
	}
	return error;
}



/***************************************************************************
    CORE IMPLEMENTATION
***************************************************************************/

static mess_input_code *codes;
static key_buffer *keybuffer;
static emu_timer *inputx_timer;
static int (*queue_chars)(const unicode_char *text, size_t text_len);
static int (*accept_char)(unicode_char ch);
static int (*charqueue_empty)(void);
static attotime current_rate;

static TIMER_CALLBACK(inputx_timerproc);


/*  Debugging commands and handlers. */
static void execute_input(running_machine *machine, int ref, int params, const char *param[]);
static void execute_dumpkbd(running_machine *machine, int ref, int params, const char *param[]);



static void clear_keybuffer(running_machine *machine)
{
	keybuffer = NULL;
	queue_chars = NULL;
	codes = NULL;
}



static void setup_keybuffer(running_machine *machine)
{
	inputx_timer = timer_alloc(machine, inputx_timerproc, NULL);
	keybuffer = auto_alloc_clear(machine, key_buffer);
	add_exit_callback(machine, clear_keybuffer);
}



void inputx_init(running_machine *machine)
{
	codes = NULL;
	inputx_timer = NULL;
	queue_chars = NULL;
	accept_char = NULL;
	charqueue_empty = NULL;
	keybuffer = NULL;

	if (machine->debug_flags & DEBUG_FLAG_ENABLED)
	{
		debug_console_register_command(machine, "input", CMDFLAG_NONE, 0, 1, 1, execute_input);
		debug_console_register_command(machine, "dumpkbd", CMDFLAG_NONE, 0, 0, 1, execute_dumpkbd);
	}

	/* posting keys directly only makes sense for a computer */
	if (machine->gamedrv->flags & GAME_COMPUTER)
	{
		codes = build_codes(machine, machine->portconfig);
		setup_keybuffer(machine);
	}
}



void inputx_setup_natural_keyboard(
	int (*queue_chars_)(const unicode_char *text, size_t text_len),
	int (*accept_char_)(unicode_char ch),
	int (*charqueue_empty_)(void))
{
	queue_chars = queue_chars_;
	accept_char = accept_char_;
	charqueue_empty = charqueue_empty_;
}

int inputx_can_post(running_machine *machine)
{
	return queue_chars || codes;
}

static key_buffer *get_buffer(running_machine *machine)
{
	assert(inputx_can_post(machine));
	return (key_buffer *) keybuffer;
}



static const mess_input_code *find_code(unicode_char ch)
{
	int i;

	assert(codes);
	for (i = 0; codes[i].ch; i++)
	{
		if (codes[i].ch == ch)
			return &codes[i];
	}
	return NULL;
}



static int can_post_key_directly(unicode_char ch)
{
	int rc = FALSE;
	const mess_input_code *code;

	if (queue_chars)
	{
		rc = accept_char ? accept_char(ch) : TRUE;
	}
	else
	{
		code = find_code(ch);
		if (code)
			rc = code->field[0] != NULL;
	}
	return rc;
}



static int can_post_key_alternate(unicode_char ch)
{
	const char *s;
	const char_info *ci;
	unicode_char uchar;
	int rc;

	ci = find_charinfo(ch);
	s = ci ? ci->alternate : NULL;
	if (!s)
		return 0;

	while(*s)
	{
		rc = uchar_from_utf8(&uchar, s, strlen(s));
		if (rc <= 0)
			return 0;
		if (!can_post_key_directly(uchar))
			return 0;
		s += rc;
	}
	return 1;
}



int inputx_can_post_key(running_machine *machine, unicode_char ch)
{
	return inputx_can_post(machine) && (can_post_key_directly(ch) || can_post_key_alternate(ch));
}



static attotime choose_delay(unicode_char ch)
{
	attoseconds_t delay = 0;

	if (attotime_compare(current_rate, attotime_zero) != 0)
		return current_rate;

	if (queue_chars)
	{
		/* systems with queue_chars can afford a much smaller delay */
		delay = DOUBLE_TO_ATTOSECONDS(0.01);
	}
	else
	{
		switch(ch) {
		case '\r':
			delay = DOUBLE_TO_ATTOSECONDS(0.2);
			break;

		default:
			delay = DOUBLE_TO_ATTOSECONDS(0.05);
			break;
		}
	}
	return attotime_make(0, delay);
}



static void internal_post_key(running_machine *machine, unicode_char ch)
{
	key_buffer *keybuf;

	keybuf = get_buffer(machine);

	/* need to start up the timer? */
	if (keybuf->begin_pos == keybuf->end_pos)
	{
		timer_adjust_oneshot(inputx_timer, choose_delay(ch), 0);
		keybuf->status_keydown = 0;
	}

	keybuf->buffer[keybuf->end_pos++] = ch;
	keybuf->end_pos %= sizeof(keybuf->buffer) / sizeof(keybuf->buffer[0]);
}



static int buffer_full(running_machine *machine)
{
	key_buffer *keybuf;
	keybuf = get_buffer(machine);
	return ((keybuf->end_pos + 1) % (sizeof(keybuf->buffer) / sizeof(keybuf->buffer[0]))) == keybuf->begin_pos;
}



void inputx_postn_rate(running_machine *machine, const unicode_char *text, size_t text_len, attotime rate)
{
	int last_cr = 0;
	unicode_char ch;
	const char *s;
	const char_info *ci;
	const mess_input_code *code;

	current_rate = rate;

	if (inputx_can_post(machine))
	{
		while((text_len > 0) && !buffer_full(machine))
		{
			ch = *(text++);
			text_len--;

			/* change all eolns to '\r' */
			if ((ch != '\n') || !last_cr)
			{
				if (ch == '\n')
					ch = '\r';
				else
					last_cr = (ch == '\r');

				if (LOG_INPUTX)
				{
					code = find_code(ch);
					logerror("inputx_postn(): code=%i (%s) field->name='%s'\n", (int) ch, code_point_string(machine, ch), (code && code->field[0]) ? code->field[0]->name : "<null>");
				}

				if (can_post_key_directly(ch))
				{
					/* we can post this key in the queue directly */
					internal_post_key(machine, ch);
				}
				else if (can_post_key_alternate(ch))
				{
					/* we can post this key with an alternate representation */
					ci = find_charinfo(ch);
					assert(ci && ci->alternate);
					s = ci->alternate;
					while(*s)
					{
						s += uchar_from_utf8(&ch, s, strlen(s));
						internal_post_key(machine, ch);
					}
				}
			}
			else
			{
				last_cr = 0;
			}
		}
	}
}



static TIMER_CALLBACK(inputx_timerproc)
{
	key_buffer *keybuf;
	attotime delay;

	keybuf = get_buffer(machine);

	if (queue_chars)
	{
		/* the driver has a queue_chars handler */
		while((keybuf->begin_pos != keybuf->end_pos) && queue_chars(&keybuf->buffer[keybuf->begin_pos], 1))
		{
			keybuf->begin_pos++;
			keybuf->begin_pos %= sizeof(keybuf->buffer) / sizeof(keybuf->buffer[0]);

			if (attotime_compare(current_rate, attotime_zero) != 0)
				break;
		}
	}
	else
	{
		/* the driver does not have a queue_chars handler */
		if (keybuf->status_keydown)
		{
			keybuf->status_keydown = FALSE;
			keybuf->begin_pos++;
			keybuf->begin_pos %= sizeof(keybuf->buffer) / sizeof(keybuf->buffer[0]);
		}
		else
		{
			keybuf->status_keydown = TRUE;
		}
	}

	/* need to make sure timerproc is called again if buffer not empty */
	if (keybuf->begin_pos != keybuf->end_pos)
	{
		delay = choose_delay(keybuf->buffer[keybuf->begin_pos]);
		timer_adjust_oneshot(inputx_timer, delay, 0);
	}
}



/*-------------------------------------------------
    mess_input_port_update_hook - hook function
	called from core to allow for natural keyboard	
-------------------------------------------------*/

void mess_input_port_update_hook(running_machine *machine, const input_port_config *port, input_port_value *digital)
{
	const key_buffer *keybuf;
	const mess_input_code *code;
	unicode_char ch;
	int i;
	UINT32 value;

	if (inputx_can_post(machine))
	{
		keybuf = get_buffer(machine);

		/* is the key down right now? */
		if (keybuf->status_keydown && (keybuf->begin_pos != keybuf->end_pos))
		{
			/* identify the character that is down right now, and its component codes */
			ch = keybuf->buffer[keybuf->begin_pos];
			code = find_code(ch);

			/* loop through this character's component codes */
			if (code != NULL)
			{
				for (i = 0; i < ARRAY_LENGTH(code->field) && (code->field[i] != NULL); i++)
				{
					if (code->field[i]->port == port)
					{
						value = code->field[i]->mask;
						*digital |= value;
					}
				}
			}
		}
	}
}



/*-------------------------------------------------
    mess_get_keyboard_key_name - builds the name of
	a key based on natural keyboard characters
-------------------------------------------------*/

astring *mess_get_keyboard_key_name(const input_field_config *field)
{
	astring *result = astring_alloc();
	int i;
	unicode_char ch;


	/* loop through each character on the field*/
	for (i = 0; i < ARRAY_LENGTH(field->chars) && (field->chars[i] != '\0'); i++)
	{
		ch = get_keyboard_code(field, i);
		astring_printf(result, "%s%-*s ", astring_c(result), MAX(SPACE_COUNT - 1, 0), inputx_key_name(ch));
	}

	/* trim extra spaces */
	astring_trimspace(result);

	/* special case */
	if (astring_len(result) == 0)
		astring_cpyc(result, "Unnamed Key");

	return result;
}



/*-------------------------------------------------
    inputx_key_name - returns the name of a
	specific key
-------------------------------------------------*/

const char *inputx_key_name(unicode_char ch)
{
	static char buf[UTF8_CHAR_MAX + 1];
	const char_info *ci;
	const char *result;
	int pos;

	ci = find_charinfo(ch);
	result = ci ? ci->name : NULL;

	if (ci && ci->name)
	{
		result = ci->name;
	}
	else
	{
		if ((ch > 0x7F) || isprint(ch))
		{
			pos = utf8_from_uchar(buf, ARRAY_LENGTH(buf), ch);
			buf[pos] = '\0';
			result = buf;
		}
		else
			result = "???";
	}
	return result;
}



/* --------------------------------------------------------------------- */

int inputx_is_posting(running_machine *machine)
{
	const key_buffer *keybuf;
	keybuf = get_buffer(machine);
	return (keybuf->begin_pos != keybuf->end_pos) || (charqueue_empty && !charqueue_empty());
}



/***************************************************************************

	Coded input

***************************************************************************/

void inputx_postn_coded_rate(running_machine *machine, const char *text, size_t text_len, attotime rate)
{
	size_t i, j, key_len, increment;
	unicode_char ch;

	static const struct
	{
		const char *key;
		unicode_char code;
	} codes[] =
	{
		{ "BACKSPACE",	8 },
		{ "BS",			8 },
		{ "BKSP",		8 },
		{ "DEL",		UCHAR_MAMEKEY(DEL) },
		{ "DELETE",		UCHAR_MAMEKEY(DEL) },
		{ "END",		UCHAR_MAMEKEY(END) },
		{ "ENTER",		13 },
		{ "ESC",		'\033' },
		{ "HOME",		UCHAR_MAMEKEY(HOME) },
		{ "INS",		UCHAR_MAMEKEY(INSERT) },
		{ "INSERT",		UCHAR_MAMEKEY(INSERT) },
		{ "PGDN",		UCHAR_MAMEKEY(PGDN) },
		{ "PGUP",		UCHAR_MAMEKEY(PGUP) },
		{ "SPACE",		32 },
		{ "TAB",		9 },
		{ "F1",			UCHAR_MAMEKEY(F1) },
		{ "F2",			UCHAR_MAMEKEY(F2) },
		{ "F3",			UCHAR_MAMEKEY(F3) },
		{ "F4",			UCHAR_MAMEKEY(F4) },
		{ "F5",			UCHAR_MAMEKEY(F5) },
		{ "F6",			UCHAR_MAMEKEY(F6) },
		{ "F7",			UCHAR_MAMEKEY(F7) },
		{ "F8",			UCHAR_MAMEKEY(F8) },
		{ "F9",			UCHAR_MAMEKEY(F9) },
		{ "F10",		UCHAR_MAMEKEY(F10) },
		{ "F11",		UCHAR_MAMEKEY(F11) },
		{ "F12",		UCHAR_MAMEKEY(F12) },
		{ "QUOTE",		'\"' }
	};

	i = 0;
	while(i < text_len)
	{
		ch = text[i];
		increment = 1;

		if (ch == '{')
		{
			for (j = 0; j < sizeof(codes) / sizeof(codes[0]); j++)
			{
				key_len = strlen(codes[j].key);
				if (i + key_len + 2 <= text_len)
				{
					if (!memcmp(codes[j].key, &text[i + 1], key_len) && (text[i + key_len + 1] == '}'))
					{
						ch = codes[j].code;
						increment = key_len + 2;
					}
				}
			}
		}

		if (ch)
			inputx_postc_rate(machine, ch, rate);
		i += increment;
	}
}



/***************************************************************************

	Alternative calls

***************************************************************************/

void inputx_postn(running_machine *machine, const unicode_char *text, size_t text_len)
{
	inputx_postn_rate(machine, text, text_len, attotime_make(0, 0));
}



void inputx_post_rate(running_machine *machine, const unicode_char *text, attotime rate)
{
	size_t len = 0;
	while(text[len])
		len++;
	inputx_postn_rate(machine, text, len, rate);
}



void inputx_post(running_machine *machine, const unicode_char *text)
{
	inputx_post_rate(machine, text, attotime_make(0, 0));
}



void inputx_postc_rate(running_machine *machine, unicode_char ch, attotime rate)
{
	inputx_postn_rate(machine, &ch, 1, rate);
}



void inputx_postc(running_machine *machine, unicode_char ch)
{
	inputx_postc_rate(machine, ch, attotime_make(0, 0));
}



void inputx_postn_utf16_rate(running_machine *machine, const utf16_char *text, size_t text_len, attotime rate)
{
	size_t len = 0;
	unicode_char c;
	utf16_char w1, w2;
	unicode_char buf[256];

	while(text_len > 0)
	{
		if (len == (sizeof(buf) / sizeof(buf[0])))
		{
			inputx_postn(machine, buf, len);
			len = 0;
		}

		w1 = *(text++);
		text_len--;

		if ((w1 >= 0xd800) && (w1 <= 0xdfff))
		{
			if (w1 <= 0xDBFF)
			{
				w2 = 0;
				if (text_len > 0)
				{
					w2 = *(text++);
					text_len--;
				}
				if ((w2 >= 0xdc00) && (w2 <= 0xdfff))
				{
					c = w1 & 0x03ff;
					c <<= 10;
					c |= w2 & 0x03ff;
				}
				else
				{
					c = INVALID_CHAR;
				}
			}
			else
			{
				c = INVALID_CHAR;
			}
		}
		else
		{
			c = w1;
		}
		buf[len++] = c;
	}
	inputx_postn_rate(machine, buf, len, rate);
}



void inputx_postn_utf16(running_machine *machine, const utf16_char *text, size_t text_len)
{
	inputx_postn_utf16_rate(machine, text, text_len, attotime_make(0, 0));
}



void inputx_post_utf16_rate(running_machine *machine, const utf16_char *text, attotime rate)
{
	size_t len = 0;
	while(text[len])
		len++;
	inputx_postn_utf16_rate(machine, text, len, rate);
}



void inputx_post_utf16(running_machine *machine, const utf16_char *text)
{
	inputx_post_utf16_rate(machine, text, attotime_make(0, 0));
}



void inputx_postn_utf8_rate(running_machine *machine, const char *text, size_t text_len, attotime rate)
{
	size_t len = 0;
	unicode_char buf[256];
	unicode_char c;
	int rc;

	while(text_len > 0)
	{
		if (len == (sizeof(buf) / sizeof(buf[0])))
		{
			inputx_postn(machine, buf, len);
			len = 0;
		}

		rc = uchar_from_utf8(&c, text, text_len);
		if (rc < 0)
		{
			rc = 1;
			c = INVALID_CHAR;
		}
		text += rc;
		text_len -= rc;
		buf[len++] = c;
	}
	inputx_postn_rate(machine, buf, len, rate);
}



void inputx_postn_utf8(running_machine *machine, const char *text, size_t text_len)
{
	inputx_postn_utf8_rate(machine, text, text_len, attotime_make(0, 0));
}



void inputx_post_utf8_rate(running_machine *machine, const char *text, attotime rate)
{
	inputx_postn_utf8_rate(machine, text, strlen(text), rate);
}



void inputx_post_utf8(running_machine *machine, const char *text)
{
	inputx_post_utf8_rate(machine, text, attotime_make(0, 0));
}



void inputx_post_coded(running_machine *machine, const char *text)
{
	inputx_postn_coded(machine, text, strlen(text));
}



void inputx_post_coded_rate(running_machine *machine, const char *text, attotime rate)
{
	inputx_postn_coded_rate(machine, text, strlen(text), rate);
}



void inputx_postn_coded(running_machine *machine, const char *text, size_t text_len)
{
	inputx_postn_coded_rate(machine, text, text_len, attotime_make(0, 0));
}



/***************************************************************************

	Other stuff

	This stuff is here more out of convienience than anything else
***************************************************************************/

int input_classify_port(const input_field_config *field)
{
	int result;

	if (field->category && (field->type != IPT_CATEGORY))
		return INPUT_CLASS_CATEGORIZED;

	switch(field->type)
	{
		case IPT_JOYSTICK_UP:
		case IPT_JOYSTICK_DOWN:
		case IPT_JOYSTICK_LEFT:
		case IPT_JOYSTICK_RIGHT:
		case IPT_JOYSTICKLEFT_UP:
		case IPT_JOYSTICKLEFT_DOWN:
		case IPT_JOYSTICKLEFT_LEFT:
		case IPT_JOYSTICKLEFT_RIGHT:
		case IPT_JOYSTICKRIGHT_UP:
		case IPT_JOYSTICKRIGHT_DOWN:
		case IPT_JOYSTICKRIGHT_LEFT:
		case IPT_JOYSTICKRIGHT_RIGHT:
		case IPT_BUTTON1:
		case IPT_BUTTON2:
		case IPT_BUTTON3:
		case IPT_BUTTON4:
		case IPT_BUTTON5:
		case IPT_BUTTON6:
		case IPT_BUTTON7:
		case IPT_BUTTON8:
		case IPT_BUTTON9:
		case IPT_BUTTON10:
		case IPT_AD_STICK_X:
		case IPT_AD_STICK_Y:
		case IPT_AD_STICK_Z:
		case IPT_TRACKBALL_X:
		case IPT_TRACKBALL_Y:
		case IPT_LIGHTGUN_X:
		case IPT_LIGHTGUN_Y:
		case IPT_MOUSE_X:
		case IPT_MOUSE_Y:
		case IPT_START:
		case IPT_SELECT:
			result = INPUT_CLASS_CONTROLLER;
			break;

		case IPT_KEYBOARD:
			result = INPUT_CLASS_KEYBOARD;
			break;

		case IPT_CONFIG:
			result = INPUT_CLASS_CONFIG;
			break;

		case IPT_DIPSWITCH:
			result = INPUT_CLASS_DIPSWITCH;
			break;

		case 0:
			if (field->name && (field->name != (const char *) -1))
				result = INPUT_CLASS_MISC;
			else
				result = INPUT_CLASS_INTERNAL;
			break;

		default:
			result = INPUT_CLASS_INTERNAL;
			break;
	}
	return result;
}



int input_player_number(const input_field_config *port)
{
	return port->player;
}



/*-------------------------------------------------
    input_has_input_class - checks to see if a 
	particular input class is present
-------------------------------------------------*/

int input_has_input_class(running_machine *machine, int inputclass)
{
	const input_port_config *port;
	const input_field_config *field;

	for (port = machine->portconfig; port != NULL; port = port->next)
	{
		for (field = port->fieldlist; field != NULL; field = field->next)
		{
			if (input_classify_port(field) == inputclass)
				return TRUE;
		}
	}
	return FALSE;
}



/*-------------------------------------------------
    input_count_players - counts the number of
	active players
-------------------------------------------------*/

int input_count_players(running_machine *machine)
{
	const input_port_config *port;
	const input_field_config *field;
	int joystick_count;

	joystick_count = 0;
	for (port = machine->portconfig; port != NULL; port = port->next)
	{
		for (field = port->fieldlist;  field != NULL; field = field->next)
		{
			if (input_classify_port(field) == INPUT_CLASS_CONTROLLER)
			{
				if (joystick_count <= field->player + 1)
					joystick_count = field->player + 1;
			}
		}
	}
	return joystick_count;
}



/*-------------------------------------------------
    input_category_active - checks to see if a
	specific category is active
-------------------------------------------------*/

int input_category_active(running_machine *machine, int category)
{
	const input_port_config *port;
	const input_field_config *field = NULL;
	const input_setting_config *setting;
	input_field_user_settings settings;

	assert(category >= 1);

	/* loop through the input ports */
	for (port = machine->portconfig; port != NULL; port = port->next)
	{
		for (field = port->fieldlist; field != NULL; field = field->next)
		{
			/* is this field a category? */
			if (field->type == IPT_CATEGORY)
			{
				/* get the settings value */
				input_field_get_user_settings(field, &settings);

				for (setting = field->settinglist; setting != NULL; setting = setting->next)
				{
					/* is this the category we want?  if so, is this settings value correct? */
					if ((setting->category == category) && (settings.value == setting->value))
						return TRUE;
				}
			}
		}
	}
	return FALSE;
}



/***************************************************************************
    DEBUGGER SUPPORT
***************************************************************************/

/*-------------------------------------------------
    execute_input - debugger command to enter
	natural keyboard input
-------------------------------------------------*/

static void execute_input(running_machine *machine, int ref, int params, const char *param[])
{
	inputx_post_coded(machine, param[0]);
}



/*-------------------------------------------------
    execute_dumpkbd - debugger command to natural
	keyboard codes
-------------------------------------------------*/

static void execute_dumpkbd(running_machine *machine, int ref, int params, const char *param[])
{
	const char *filename;
	FILE *file = NULL;
	const mess_input_code *code;
	char buffer[512];
	size_t pos;
	int i, j;
	size_t left_column_width = 24;

	/* was there a file specified? */
	filename = (params > 0) ? param[0] : NULL;
	if (filename != NULL)
	{
		/* if so, open it */
		file = fopen(filename, "w");
		if (file == NULL)
		{
			debug_console_printf(machine, "Cannot open \"%s\"\n", filename);
			return;
		}
	}

	if ((codes != NULL) && (codes[0].ch != 0))
	{
		/* loop through all codes */
		for (i = 0; codes[i].ch; i++)
		{
			code = &codes[i];
			pos = 0;

			/* describe the character code */
			pos += snprintf(&buffer[pos], ARRAY_LENGTH(buffer) - pos, "%08X (%s) ",
				code->ch,
				code_point_string(machine, code->ch));

			/* pad with spaces */
			while(pos < left_column_width)
				buffer[pos++] = ' ';
			buffer[pos] = '\0';

			/* identify the keys used */
			for (j = 0; j < ARRAY_LENGTH(code->field) && (code->field[j] != NULL); j++)
			{
				pos += snprintf(&buffer[pos], ARRAY_LENGTH(buffer) - pos, "%s'%s'",
					(j > 0) ? ", " : "",
					code->field[j]->name);
			}

			/* and output it as appropriate */
			if (file != NULL)
				fprintf(file, "%s\n", buffer);
			else
				debug_console_printf(machine, "%s\n", buffer);
		}
	}
	else
	{
		debug_console_printf(machine, "No natural keyboard support\n");
	}

	/* cleanup */
	if (file != NULL)
		fclose(file);

}
