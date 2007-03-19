/***************************************************************************

    input.c

    Handle input from the user.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "osdepend.h"
#include "driver.h"
#include "profiler.h"
#include <time.h>
#include <ctype.h>



/***************************************************************************
    CONSTANTS
***************************************************************************/

#define MAX_TOKEN_LEN		64

/* max time between key presses */
#define RECORD_TIME			(CLOCKS_PER_SEC*2/3)



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _input_code_info input_code_info;
struct _input_code_info
{
	UINT8						analogtype;				/* analog type */
	INT32						memory;					/* memory */
	INT32						prev_analog;			/* previous analog value */
	const os_code_info *		osinfo;					/* pointer to the OS code info */
	char						token[MAX_TOKEN_LEN];	/* token string */
};



/***************************************************************************
    STRING <-> CODE MATCHING
***************************************************************************/

#define STANDARD_CODE_STRING(x)	{ x, #x },

static struct
{
	int				code;
	const char *	codename;
} standard_code_strings[] =
{
	STANDARD_CODE_STRING(KEYCODE_A)
 	STANDARD_CODE_STRING(KEYCODE_B)
 	STANDARD_CODE_STRING(KEYCODE_C)
 	STANDARD_CODE_STRING(KEYCODE_D)
 	STANDARD_CODE_STRING(KEYCODE_E)
 	STANDARD_CODE_STRING(KEYCODE_F)
	STANDARD_CODE_STRING(KEYCODE_G)
 	STANDARD_CODE_STRING(KEYCODE_H)
 	STANDARD_CODE_STRING(KEYCODE_I)
 	STANDARD_CODE_STRING(KEYCODE_J)
 	STANDARD_CODE_STRING(KEYCODE_K)
 	STANDARD_CODE_STRING(KEYCODE_L)
	STANDARD_CODE_STRING(KEYCODE_M)
 	STANDARD_CODE_STRING(KEYCODE_N)
 	STANDARD_CODE_STRING(KEYCODE_O)
 	STANDARD_CODE_STRING(KEYCODE_P)
 	STANDARD_CODE_STRING(KEYCODE_Q)
 	STANDARD_CODE_STRING(KEYCODE_R)
	STANDARD_CODE_STRING(KEYCODE_S)
 	STANDARD_CODE_STRING(KEYCODE_T)
 	STANDARD_CODE_STRING(KEYCODE_U)
 	STANDARD_CODE_STRING(KEYCODE_V)
 	STANDARD_CODE_STRING(KEYCODE_W)
 	STANDARD_CODE_STRING(KEYCODE_X)
	STANDARD_CODE_STRING(KEYCODE_Y)
 	STANDARD_CODE_STRING(KEYCODE_Z)
 	STANDARD_CODE_STRING(KEYCODE_0)
 	STANDARD_CODE_STRING(KEYCODE_1)
 	STANDARD_CODE_STRING(KEYCODE_2)
 	STANDARD_CODE_STRING(KEYCODE_3)
	STANDARD_CODE_STRING(KEYCODE_4)
 	STANDARD_CODE_STRING(KEYCODE_5)
 	STANDARD_CODE_STRING(KEYCODE_6)
 	STANDARD_CODE_STRING(KEYCODE_7)
 	STANDARD_CODE_STRING(KEYCODE_8)
 	STANDARD_CODE_STRING(KEYCODE_9)
	STANDARD_CODE_STRING(KEYCODE_F1)
 	STANDARD_CODE_STRING(KEYCODE_F2)
 	STANDARD_CODE_STRING(KEYCODE_F3)
 	STANDARD_CODE_STRING(KEYCODE_F4)
 	STANDARD_CODE_STRING(KEYCODE_F5)
	STANDARD_CODE_STRING(KEYCODE_F6)
 	STANDARD_CODE_STRING(KEYCODE_F7)
 	STANDARD_CODE_STRING(KEYCODE_F8)
 	STANDARD_CODE_STRING(KEYCODE_F9)
 	STANDARD_CODE_STRING(KEYCODE_F10)
	STANDARD_CODE_STRING(KEYCODE_F11)
 	STANDARD_CODE_STRING(KEYCODE_F12)
 	STANDARD_CODE_STRING(KEYCODE_F13)
 	STANDARD_CODE_STRING(KEYCODE_F14)
 	STANDARD_CODE_STRING(KEYCODE_F15)
	STANDARD_CODE_STRING(KEYCODE_ESC)
 	STANDARD_CODE_STRING(KEYCODE_TILDE)
 	STANDARD_CODE_STRING(KEYCODE_MINUS)
 	STANDARD_CODE_STRING(KEYCODE_EQUALS)
 	STANDARD_CODE_STRING(KEYCODE_BACKSPACE)
	STANDARD_CODE_STRING(KEYCODE_TAB)
 	STANDARD_CODE_STRING(KEYCODE_OPENBRACE)
 	STANDARD_CODE_STRING(KEYCODE_CLOSEBRACE)
 	STANDARD_CODE_STRING(KEYCODE_ENTER)
 	STANDARD_CODE_STRING(KEYCODE_COLON)
	STANDARD_CODE_STRING(KEYCODE_QUOTE)
 	STANDARD_CODE_STRING(KEYCODE_BACKSLASH)
 	STANDARD_CODE_STRING(KEYCODE_BACKSLASH2)
 	STANDARD_CODE_STRING(KEYCODE_COMMA)
 	STANDARD_CODE_STRING(KEYCODE_STOP)
	STANDARD_CODE_STRING(KEYCODE_SLASH)
 	STANDARD_CODE_STRING(KEYCODE_SPACE)
 	STANDARD_CODE_STRING(KEYCODE_INSERT)
 	STANDARD_CODE_STRING(KEYCODE_DEL)
	STANDARD_CODE_STRING(KEYCODE_HOME)
 	STANDARD_CODE_STRING(KEYCODE_END)
 	STANDARD_CODE_STRING(KEYCODE_PGUP)
 	STANDARD_CODE_STRING(KEYCODE_PGDN)
	STANDARD_CODE_STRING(KEYCODE_LEFT)
	STANDARD_CODE_STRING(KEYCODE_RIGHT)
 	STANDARD_CODE_STRING(KEYCODE_UP)
 	STANDARD_CODE_STRING(KEYCODE_DOWN)
	STANDARD_CODE_STRING(KEYCODE_0_PAD)
 	STANDARD_CODE_STRING(KEYCODE_1_PAD)
 	STANDARD_CODE_STRING(KEYCODE_2_PAD)
 	STANDARD_CODE_STRING(KEYCODE_3_PAD)
 	STANDARD_CODE_STRING(KEYCODE_4_PAD)
	STANDARD_CODE_STRING(KEYCODE_5_PAD)
 	STANDARD_CODE_STRING(KEYCODE_6_PAD)
 	STANDARD_CODE_STRING(KEYCODE_7_PAD)
 	STANDARD_CODE_STRING(KEYCODE_8_PAD)
 	STANDARD_CODE_STRING(KEYCODE_9_PAD)
	STANDARD_CODE_STRING(KEYCODE_SLASH_PAD)
 	STANDARD_CODE_STRING(KEYCODE_ASTERISK)
 	STANDARD_CODE_STRING(KEYCODE_MINUS_PAD)
 	STANDARD_CODE_STRING(KEYCODE_PLUS_PAD)
	STANDARD_CODE_STRING(KEYCODE_DEL_PAD)
 	STANDARD_CODE_STRING(KEYCODE_ENTER_PAD)
 	STANDARD_CODE_STRING(KEYCODE_PRTSCR)
 	STANDARD_CODE_STRING(KEYCODE_PAUSE)
	STANDARD_CODE_STRING(KEYCODE_LSHIFT)
 	STANDARD_CODE_STRING(KEYCODE_RSHIFT)
 	STANDARD_CODE_STRING(KEYCODE_LCONTROL)
 	STANDARD_CODE_STRING(KEYCODE_RCONTROL)
	STANDARD_CODE_STRING(KEYCODE_LALT)
 	STANDARD_CODE_STRING(KEYCODE_RALT)
 	STANDARD_CODE_STRING(KEYCODE_SCRLOCK)
 	STANDARD_CODE_STRING(KEYCODE_NUMLOCK)
 	STANDARD_CODE_STRING(KEYCODE_CAPSLOCK)
	STANDARD_CODE_STRING(KEYCODE_LWIN)
 	STANDARD_CODE_STRING(KEYCODE_RWIN)
 	STANDARD_CODE_STRING(KEYCODE_MENU)

	STANDARD_CODE_STRING(JOYCODE_1_LEFT)
	STANDARD_CODE_STRING(JOYCODE_1_RIGHT)
	STANDARD_CODE_STRING(JOYCODE_1_UP)
	STANDARD_CODE_STRING(JOYCODE_1_DOWN)
	STANDARD_CODE_STRING(JOYCODE_1_BUTTON1)
	STANDARD_CODE_STRING(JOYCODE_1_BUTTON2)
	STANDARD_CODE_STRING(JOYCODE_1_BUTTON3)
	STANDARD_CODE_STRING(JOYCODE_1_BUTTON4)
	STANDARD_CODE_STRING(JOYCODE_1_BUTTON5)
	STANDARD_CODE_STRING(JOYCODE_1_BUTTON6)
	STANDARD_CODE_STRING(JOYCODE_1_BUTTON7)
	STANDARD_CODE_STRING(JOYCODE_1_BUTTON8)
	STANDARD_CODE_STRING(JOYCODE_1_BUTTON9)
	STANDARD_CODE_STRING(JOYCODE_1_BUTTON10)
	STANDARD_CODE_STRING(JOYCODE_1_BUTTON11)
	STANDARD_CODE_STRING(JOYCODE_1_BUTTON12)
	STANDARD_CODE_STRING(JOYCODE_1_BUTTON13)
	STANDARD_CODE_STRING(JOYCODE_1_BUTTON14)
	STANDARD_CODE_STRING(JOYCODE_1_BUTTON15)
	STANDARD_CODE_STRING(JOYCODE_1_BUTTON16)
 	STANDARD_CODE_STRING(JOYCODE_1_START)
 	STANDARD_CODE_STRING(JOYCODE_1_SELECT)
	STANDARD_CODE_STRING(JOYCODE_2_LEFT)
	STANDARD_CODE_STRING(JOYCODE_2_RIGHT)
	STANDARD_CODE_STRING(JOYCODE_2_UP)
	STANDARD_CODE_STRING(JOYCODE_2_DOWN)
	STANDARD_CODE_STRING(JOYCODE_2_BUTTON1)
	STANDARD_CODE_STRING(JOYCODE_2_BUTTON2)
	STANDARD_CODE_STRING(JOYCODE_2_BUTTON3)
	STANDARD_CODE_STRING(JOYCODE_2_BUTTON4)
	STANDARD_CODE_STRING(JOYCODE_2_BUTTON5)
	STANDARD_CODE_STRING(JOYCODE_2_BUTTON6)
	STANDARD_CODE_STRING(JOYCODE_2_BUTTON7)
	STANDARD_CODE_STRING(JOYCODE_2_BUTTON8)
	STANDARD_CODE_STRING(JOYCODE_2_BUTTON9)
	STANDARD_CODE_STRING(JOYCODE_2_BUTTON10)
	STANDARD_CODE_STRING(JOYCODE_2_BUTTON11)
	STANDARD_CODE_STRING(JOYCODE_2_BUTTON12)
	STANDARD_CODE_STRING(JOYCODE_2_BUTTON13)
	STANDARD_CODE_STRING(JOYCODE_2_BUTTON14)
	STANDARD_CODE_STRING(JOYCODE_2_BUTTON15)
	STANDARD_CODE_STRING(JOYCODE_2_BUTTON16)
 	STANDARD_CODE_STRING(JOYCODE_2_START)
 	STANDARD_CODE_STRING(JOYCODE_2_SELECT)
	STANDARD_CODE_STRING(JOYCODE_3_LEFT)
	STANDARD_CODE_STRING(JOYCODE_3_RIGHT)
	STANDARD_CODE_STRING(JOYCODE_3_UP)
	STANDARD_CODE_STRING(JOYCODE_3_DOWN)
	STANDARD_CODE_STRING(JOYCODE_3_BUTTON1)
	STANDARD_CODE_STRING(JOYCODE_3_BUTTON2)
	STANDARD_CODE_STRING(JOYCODE_3_BUTTON3)
	STANDARD_CODE_STRING(JOYCODE_3_BUTTON4)
	STANDARD_CODE_STRING(JOYCODE_3_BUTTON5)
	STANDARD_CODE_STRING(JOYCODE_3_BUTTON6)
	STANDARD_CODE_STRING(JOYCODE_3_BUTTON7)
	STANDARD_CODE_STRING(JOYCODE_3_BUTTON8)
	STANDARD_CODE_STRING(JOYCODE_3_BUTTON9)
	STANDARD_CODE_STRING(JOYCODE_3_BUTTON10)
	STANDARD_CODE_STRING(JOYCODE_3_BUTTON11)
	STANDARD_CODE_STRING(JOYCODE_3_BUTTON12)
	STANDARD_CODE_STRING(JOYCODE_3_BUTTON13)
	STANDARD_CODE_STRING(JOYCODE_3_BUTTON14)
	STANDARD_CODE_STRING(JOYCODE_3_BUTTON15)
	STANDARD_CODE_STRING(JOYCODE_3_BUTTON16)
 	STANDARD_CODE_STRING(JOYCODE_3_START)
 	STANDARD_CODE_STRING(JOYCODE_3_SELECT)
	STANDARD_CODE_STRING(JOYCODE_4_LEFT)
	STANDARD_CODE_STRING(JOYCODE_4_RIGHT)
	STANDARD_CODE_STRING(JOYCODE_4_UP)
	STANDARD_CODE_STRING(JOYCODE_4_DOWN)
	STANDARD_CODE_STRING(JOYCODE_4_BUTTON1)
	STANDARD_CODE_STRING(JOYCODE_4_BUTTON2)
	STANDARD_CODE_STRING(JOYCODE_4_BUTTON3)
	STANDARD_CODE_STRING(JOYCODE_4_BUTTON4)
	STANDARD_CODE_STRING(JOYCODE_4_BUTTON5)
	STANDARD_CODE_STRING(JOYCODE_4_BUTTON6)
	STANDARD_CODE_STRING(JOYCODE_4_BUTTON7)
	STANDARD_CODE_STRING(JOYCODE_4_BUTTON8)
	STANDARD_CODE_STRING(JOYCODE_4_BUTTON9)
	STANDARD_CODE_STRING(JOYCODE_4_BUTTON10)
	STANDARD_CODE_STRING(JOYCODE_4_BUTTON11)
	STANDARD_CODE_STRING(JOYCODE_4_BUTTON12)
	STANDARD_CODE_STRING(JOYCODE_4_BUTTON13)
	STANDARD_CODE_STRING(JOYCODE_4_BUTTON14)
	STANDARD_CODE_STRING(JOYCODE_4_BUTTON15)
	STANDARD_CODE_STRING(JOYCODE_4_BUTTON16)
 	STANDARD_CODE_STRING(JOYCODE_4_START)
 	STANDARD_CODE_STRING(JOYCODE_4_SELECT)
	STANDARD_CODE_STRING(JOYCODE_5_LEFT)
	STANDARD_CODE_STRING(JOYCODE_5_RIGHT)
	STANDARD_CODE_STRING(JOYCODE_5_UP)
	STANDARD_CODE_STRING(JOYCODE_5_DOWN)
	STANDARD_CODE_STRING(JOYCODE_5_BUTTON1)
	STANDARD_CODE_STRING(JOYCODE_5_BUTTON2)
	STANDARD_CODE_STRING(JOYCODE_5_BUTTON3)
	STANDARD_CODE_STRING(JOYCODE_5_BUTTON4)
	STANDARD_CODE_STRING(JOYCODE_5_BUTTON5)
	STANDARD_CODE_STRING(JOYCODE_5_BUTTON6)
	STANDARD_CODE_STRING(JOYCODE_5_BUTTON7)
	STANDARD_CODE_STRING(JOYCODE_5_BUTTON8)
	STANDARD_CODE_STRING(JOYCODE_5_BUTTON9)
	STANDARD_CODE_STRING(JOYCODE_5_BUTTON10)
	STANDARD_CODE_STRING(JOYCODE_5_BUTTON11)
	STANDARD_CODE_STRING(JOYCODE_5_BUTTON12)
	STANDARD_CODE_STRING(JOYCODE_5_BUTTON13)
	STANDARD_CODE_STRING(JOYCODE_5_BUTTON14)
	STANDARD_CODE_STRING(JOYCODE_5_BUTTON15)
	STANDARD_CODE_STRING(JOYCODE_5_BUTTON16)
 	STANDARD_CODE_STRING(JOYCODE_5_START)
 	STANDARD_CODE_STRING(JOYCODE_5_SELECT)
	STANDARD_CODE_STRING(JOYCODE_6_LEFT)
	STANDARD_CODE_STRING(JOYCODE_6_RIGHT)
	STANDARD_CODE_STRING(JOYCODE_6_UP)
	STANDARD_CODE_STRING(JOYCODE_6_DOWN)
	STANDARD_CODE_STRING(JOYCODE_6_BUTTON1)
	STANDARD_CODE_STRING(JOYCODE_6_BUTTON2)
	STANDARD_CODE_STRING(JOYCODE_6_BUTTON3)
	STANDARD_CODE_STRING(JOYCODE_6_BUTTON4)
	STANDARD_CODE_STRING(JOYCODE_6_BUTTON5)
	STANDARD_CODE_STRING(JOYCODE_6_BUTTON6)
	STANDARD_CODE_STRING(JOYCODE_6_BUTTON7)
	STANDARD_CODE_STRING(JOYCODE_6_BUTTON8)
	STANDARD_CODE_STRING(JOYCODE_6_BUTTON9)
	STANDARD_CODE_STRING(JOYCODE_6_BUTTON10)
	STANDARD_CODE_STRING(JOYCODE_6_BUTTON11)
	STANDARD_CODE_STRING(JOYCODE_6_BUTTON12)
	STANDARD_CODE_STRING(JOYCODE_6_BUTTON13)
	STANDARD_CODE_STRING(JOYCODE_6_BUTTON14)
	STANDARD_CODE_STRING(JOYCODE_6_BUTTON15)
	STANDARD_CODE_STRING(JOYCODE_6_BUTTON16)
 	STANDARD_CODE_STRING(JOYCODE_6_START)
 	STANDARD_CODE_STRING(JOYCODE_6_SELECT)
	STANDARD_CODE_STRING(JOYCODE_7_LEFT)
	STANDARD_CODE_STRING(JOYCODE_7_RIGHT)
	STANDARD_CODE_STRING(JOYCODE_7_UP)
	STANDARD_CODE_STRING(JOYCODE_7_DOWN)
	STANDARD_CODE_STRING(JOYCODE_7_BUTTON1)
	STANDARD_CODE_STRING(JOYCODE_7_BUTTON2)
	STANDARD_CODE_STRING(JOYCODE_7_BUTTON3)
	STANDARD_CODE_STRING(JOYCODE_7_BUTTON4)
	STANDARD_CODE_STRING(JOYCODE_7_BUTTON5)
	STANDARD_CODE_STRING(JOYCODE_7_BUTTON6)
	STANDARD_CODE_STRING(JOYCODE_7_BUTTON7)
	STANDARD_CODE_STRING(JOYCODE_7_BUTTON8)
	STANDARD_CODE_STRING(JOYCODE_7_BUTTON9)
	STANDARD_CODE_STRING(JOYCODE_7_BUTTON10)
	STANDARD_CODE_STRING(JOYCODE_7_BUTTON11)
	STANDARD_CODE_STRING(JOYCODE_7_BUTTON12)
	STANDARD_CODE_STRING(JOYCODE_7_BUTTON13)
	STANDARD_CODE_STRING(JOYCODE_7_BUTTON14)
	STANDARD_CODE_STRING(JOYCODE_7_BUTTON15)
	STANDARD_CODE_STRING(JOYCODE_7_BUTTON16)
 	STANDARD_CODE_STRING(JOYCODE_7_START)
 	STANDARD_CODE_STRING(JOYCODE_7_SELECT)
	STANDARD_CODE_STRING(JOYCODE_8_LEFT)
	STANDARD_CODE_STRING(JOYCODE_8_RIGHT)
	STANDARD_CODE_STRING(JOYCODE_8_UP)
	STANDARD_CODE_STRING(JOYCODE_8_DOWN)
	STANDARD_CODE_STRING(JOYCODE_8_BUTTON1)
	STANDARD_CODE_STRING(JOYCODE_8_BUTTON2)
	STANDARD_CODE_STRING(JOYCODE_8_BUTTON3)
	STANDARD_CODE_STRING(JOYCODE_8_BUTTON4)
	STANDARD_CODE_STRING(JOYCODE_8_BUTTON5)
	STANDARD_CODE_STRING(JOYCODE_8_BUTTON6)
	STANDARD_CODE_STRING(JOYCODE_8_BUTTON7)
	STANDARD_CODE_STRING(JOYCODE_8_BUTTON8)
	STANDARD_CODE_STRING(JOYCODE_8_BUTTON9)
	STANDARD_CODE_STRING(JOYCODE_8_BUTTON10)
	STANDARD_CODE_STRING(JOYCODE_8_BUTTON11)
	STANDARD_CODE_STRING(JOYCODE_8_BUTTON12)
	STANDARD_CODE_STRING(JOYCODE_8_BUTTON13)
	STANDARD_CODE_STRING(JOYCODE_8_BUTTON14)
	STANDARD_CODE_STRING(JOYCODE_8_BUTTON15)
	STANDARD_CODE_STRING(JOYCODE_8_BUTTON16)
 	STANDARD_CODE_STRING(JOYCODE_8_START)
 	STANDARD_CODE_STRING(JOYCODE_8_SELECT)

	STANDARD_CODE_STRING(MOUSECODE_1_BUTTON1)
	STANDARD_CODE_STRING(MOUSECODE_1_BUTTON2)
	STANDARD_CODE_STRING(MOUSECODE_1_BUTTON3)
	STANDARD_CODE_STRING(MOUSECODE_1_BUTTON4)
	STANDARD_CODE_STRING(MOUSECODE_1_BUTTON5)
	STANDARD_CODE_STRING(MOUSECODE_1_BUTTON6)
	STANDARD_CODE_STRING(MOUSECODE_2_BUTTON1)
	STANDARD_CODE_STRING(MOUSECODE_2_BUTTON2)
	STANDARD_CODE_STRING(MOUSECODE_2_BUTTON3)
	STANDARD_CODE_STRING(MOUSECODE_2_BUTTON4)
	STANDARD_CODE_STRING(MOUSECODE_2_BUTTON5)
	STANDARD_CODE_STRING(MOUSECODE_2_BUTTON6)
	STANDARD_CODE_STRING(MOUSECODE_3_BUTTON1)
	STANDARD_CODE_STRING(MOUSECODE_3_BUTTON2)
	STANDARD_CODE_STRING(MOUSECODE_3_BUTTON3)
	STANDARD_CODE_STRING(MOUSECODE_3_BUTTON4)
	STANDARD_CODE_STRING(MOUSECODE_3_BUTTON5)
	STANDARD_CODE_STRING(MOUSECODE_3_BUTTON6)
	STANDARD_CODE_STRING(MOUSECODE_4_BUTTON1)
	STANDARD_CODE_STRING(MOUSECODE_4_BUTTON2)
	STANDARD_CODE_STRING(MOUSECODE_4_BUTTON3)
	STANDARD_CODE_STRING(MOUSECODE_4_BUTTON4)
	STANDARD_CODE_STRING(MOUSECODE_4_BUTTON5)
	STANDARD_CODE_STRING(MOUSECODE_4_BUTTON6)
	STANDARD_CODE_STRING(MOUSECODE_5_BUTTON1)
	STANDARD_CODE_STRING(MOUSECODE_5_BUTTON2)
	STANDARD_CODE_STRING(MOUSECODE_5_BUTTON3)
	STANDARD_CODE_STRING(MOUSECODE_5_BUTTON4)
	STANDARD_CODE_STRING(MOUSECODE_5_BUTTON5)
	STANDARD_CODE_STRING(MOUSECODE_5_BUTTON6)
	STANDARD_CODE_STRING(MOUSECODE_6_BUTTON1)
	STANDARD_CODE_STRING(MOUSECODE_6_BUTTON2)
	STANDARD_CODE_STRING(MOUSECODE_6_BUTTON3)
	STANDARD_CODE_STRING(MOUSECODE_6_BUTTON4)
	STANDARD_CODE_STRING(MOUSECODE_6_BUTTON5)
	STANDARD_CODE_STRING(MOUSECODE_6_BUTTON6)
	STANDARD_CODE_STRING(MOUSECODE_7_BUTTON1)
	STANDARD_CODE_STRING(MOUSECODE_7_BUTTON2)
	STANDARD_CODE_STRING(MOUSECODE_7_BUTTON3)
	STANDARD_CODE_STRING(MOUSECODE_7_BUTTON4)
	STANDARD_CODE_STRING(MOUSECODE_7_BUTTON5)
	STANDARD_CODE_STRING(MOUSECODE_7_BUTTON6)
	STANDARD_CODE_STRING(MOUSECODE_8_BUTTON1)
	STANDARD_CODE_STRING(MOUSECODE_8_BUTTON2)
	STANDARD_CODE_STRING(MOUSECODE_8_BUTTON3)
	STANDARD_CODE_STRING(MOUSECODE_8_BUTTON4)
	STANDARD_CODE_STRING(MOUSECODE_8_BUTTON5)
	STANDARD_CODE_STRING(MOUSECODE_8_BUTTON6)

	STANDARD_CODE_STRING(MOUSECODE_1_X_NEG)
	STANDARD_CODE_STRING(MOUSECODE_1_X_POS)
	STANDARD_CODE_STRING(MOUSECODE_1_Y_NEG)
	STANDARD_CODE_STRING(MOUSECODE_1_Y_POS)
	STANDARD_CODE_STRING(MOUSECODE_1_Z_NEG)
	STANDARD_CODE_STRING(MOUSECODE_1_Z_POS)
	STANDARD_CODE_STRING(MOUSECODE_2_X_NEG)
	STANDARD_CODE_STRING(MOUSECODE_2_X_POS)
	STANDARD_CODE_STRING(MOUSECODE_2_Y_NEG)
	STANDARD_CODE_STRING(MOUSECODE_2_Y_POS)
	STANDARD_CODE_STRING(MOUSECODE_2_Z_NEG)
	STANDARD_CODE_STRING(MOUSECODE_2_Z_POS)
	STANDARD_CODE_STRING(MOUSECODE_3_X_NEG)
	STANDARD_CODE_STRING(MOUSECODE_3_X_POS)
	STANDARD_CODE_STRING(MOUSECODE_3_Y_NEG)
	STANDARD_CODE_STRING(MOUSECODE_3_Y_POS)
	STANDARD_CODE_STRING(MOUSECODE_3_Z_NEG)
	STANDARD_CODE_STRING(MOUSECODE_3_Z_POS)
	STANDARD_CODE_STRING(MOUSECODE_4_X_NEG)
	STANDARD_CODE_STRING(MOUSECODE_4_X_POS)
	STANDARD_CODE_STRING(MOUSECODE_4_Y_NEG)
	STANDARD_CODE_STRING(MOUSECODE_4_Y_POS)
	STANDARD_CODE_STRING(MOUSECODE_4_Z_NEG)
	STANDARD_CODE_STRING(MOUSECODE_4_Z_POS)
	STANDARD_CODE_STRING(MOUSECODE_5_X_NEG)
	STANDARD_CODE_STRING(MOUSECODE_5_X_POS)
	STANDARD_CODE_STRING(MOUSECODE_5_Y_NEG)
	STANDARD_CODE_STRING(MOUSECODE_5_Y_POS)
	STANDARD_CODE_STRING(MOUSECODE_5_Z_NEG)
	STANDARD_CODE_STRING(MOUSECODE_5_Z_POS)
	STANDARD_CODE_STRING(MOUSECODE_6_X_NEG)
	STANDARD_CODE_STRING(MOUSECODE_6_X_POS)
	STANDARD_CODE_STRING(MOUSECODE_6_Y_NEG)
	STANDARD_CODE_STRING(MOUSECODE_6_Y_POS)
	STANDARD_CODE_STRING(MOUSECODE_6_Z_NEG)
	STANDARD_CODE_STRING(MOUSECODE_6_Z_POS)
	STANDARD_CODE_STRING(MOUSECODE_7_X_NEG)
	STANDARD_CODE_STRING(MOUSECODE_7_X_POS)
	STANDARD_CODE_STRING(MOUSECODE_7_Y_NEG)
	STANDARD_CODE_STRING(MOUSECODE_7_Y_POS)
	STANDARD_CODE_STRING(MOUSECODE_7_Z_NEG)
	STANDARD_CODE_STRING(MOUSECODE_7_Z_POS)
	STANDARD_CODE_STRING(MOUSECODE_8_X_NEG)
	STANDARD_CODE_STRING(MOUSECODE_8_X_POS)
	STANDARD_CODE_STRING(MOUSECODE_8_Y_NEG)
	STANDARD_CODE_STRING(MOUSECODE_8_Y_POS)
	STANDARD_CODE_STRING(MOUSECODE_8_Z_NEG)
	STANDARD_CODE_STRING(MOUSECODE_8_Z_POS)

	STANDARD_CODE_STRING(JOYCODE_1_ANALOG_X)
	STANDARD_CODE_STRING(JOYCODE_1_ANALOG_X_NEG)
	STANDARD_CODE_STRING(JOYCODE_1_ANALOG_X_POS)
	STANDARD_CODE_STRING(JOYCODE_1_ANALOG_Y)
	STANDARD_CODE_STRING(JOYCODE_1_ANALOG_Y_NEG)
	STANDARD_CODE_STRING(JOYCODE_1_ANALOG_Y_POS)
	STANDARD_CODE_STRING(JOYCODE_1_ANALOG_Z)
	STANDARD_CODE_STRING(JOYCODE_1_ANALOG_Z_NEG)
	STANDARD_CODE_STRING(JOYCODE_1_ANALOG_Z_POS)
	STANDARD_CODE_STRING(JOYCODE_2_ANALOG_X)
	STANDARD_CODE_STRING(JOYCODE_2_ANALOG_X_NEG)
	STANDARD_CODE_STRING(JOYCODE_2_ANALOG_X_POS)
	STANDARD_CODE_STRING(JOYCODE_2_ANALOG_Y)
	STANDARD_CODE_STRING(JOYCODE_2_ANALOG_Y_NEG)
	STANDARD_CODE_STRING(JOYCODE_2_ANALOG_Y_POS)
	STANDARD_CODE_STRING(JOYCODE_2_ANALOG_Z)
	STANDARD_CODE_STRING(JOYCODE_2_ANALOG_Z_NEG)
	STANDARD_CODE_STRING(JOYCODE_2_ANALOG_Z_POS)
	STANDARD_CODE_STRING(JOYCODE_3_ANALOG_X)
	STANDARD_CODE_STRING(JOYCODE_3_ANALOG_X_NEG)
	STANDARD_CODE_STRING(JOYCODE_3_ANALOG_X_POS)
	STANDARD_CODE_STRING(JOYCODE_3_ANALOG_Y)
	STANDARD_CODE_STRING(JOYCODE_3_ANALOG_Y_NEG)
	STANDARD_CODE_STRING(JOYCODE_3_ANALOG_Y_POS)
	STANDARD_CODE_STRING(JOYCODE_3_ANALOG_Z)
	STANDARD_CODE_STRING(JOYCODE_3_ANALOG_Z_NEG)
	STANDARD_CODE_STRING(JOYCODE_3_ANALOG_Z_POS)
	STANDARD_CODE_STRING(JOYCODE_4_ANALOG_X)
	STANDARD_CODE_STRING(JOYCODE_4_ANALOG_X_NEG)
	STANDARD_CODE_STRING(JOYCODE_4_ANALOG_X_POS)
	STANDARD_CODE_STRING(JOYCODE_4_ANALOG_Y)
	STANDARD_CODE_STRING(JOYCODE_4_ANALOG_Y_NEG)
	STANDARD_CODE_STRING(JOYCODE_4_ANALOG_Y_POS)
	STANDARD_CODE_STRING(JOYCODE_4_ANALOG_Z)
	STANDARD_CODE_STRING(JOYCODE_4_ANALOG_Z_NEG)
	STANDARD_CODE_STRING(JOYCODE_4_ANALOG_Z_POS)
	STANDARD_CODE_STRING(JOYCODE_5_ANALOG_X)
	STANDARD_CODE_STRING(JOYCODE_5_ANALOG_X_NEG)
	STANDARD_CODE_STRING(JOYCODE_5_ANALOG_X_POS)
	STANDARD_CODE_STRING(JOYCODE_5_ANALOG_Y)
	STANDARD_CODE_STRING(JOYCODE_5_ANALOG_Y_NEG)
	STANDARD_CODE_STRING(JOYCODE_5_ANALOG_Y_POS)
	STANDARD_CODE_STRING(JOYCODE_5_ANALOG_Z)
	STANDARD_CODE_STRING(JOYCODE_5_ANALOG_Z_NEG)
	STANDARD_CODE_STRING(JOYCODE_5_ANALOG_Z_POS)
	STANDARD_CODE_STRING(JOYCODE_6_ANALOG_X)
	STANDARD_CODE_STRING(JOYCODE_6_ANALOG_X_NEG)
	STANDARD_CODE_STRING(JOYCODE_6_ANALOG_X_POS)
	STANDARD_CODE_STRING(JOYCODE_6_ANALOG_Y)
	STANDARD_CODE_STRING(JOYCODE_6_ANALOG_Y_NEG)
	STANDARD_CODE_STRING(JOYCODE_6_ANALOG_Y_POS)
	STANDARD_CODE_STRING(JOYCODE_6_ANALOG_Z)
	STANDARD_CODE_STRING(JOYCODE_6_ANALOG_Z_NEG)
	STANDARD_CODE_STRING(JOYCODE_6_ANALOG_Z_POS)
	STANDARD_CODE_STRING(JOYCODE_7_ANALOG_X)
	STANDARD_CODE_STRING(JOYCODE_7_ANALOG_X_NEG)
	STANDARD_CODE_STRING(JOYCODE_7_ANALOG_X_POS)
	STANDARD_CODE_STRING(JOYCODE_7_ANALOG_Y)
	STANDARD_CODE_STRING(JOYCODE_7_ANALOG_Y_NEG)
	STANDARD_CODE_STRING(JOYCODE_7_ANALOG_Y_POS)
	STANDARD_CODE_STRING(JOYCODE_7_ANALOG_Z)
	STANDARD_CODE_STRING(JOYCODE_7_ANALOG_Z_NEG)
	STANDARD_CODE_STRING(JOYCODE_7_ANALOG_Z_POS)
	STANDARD_CODE_STRING(JOYCODE_8_ANALOG_X)
	STANDARD_CODE_STRING(JOYCODE_8_ANALOG_X_NEG)
	STANDARD_CODE_STRING(JOYCODE_8_ANALOG_X_POS)
	STANDARD_CODE_STRING(JOYCODE_8_ANALOG_Y)
	STANDARD_CODE_STRING(JOYCODE_8_ANALOG_Y_NEG)
	STANDARD_CODE_STRING(JOYCODE_8_ANALOG_Y_POS)
	STANDARD_CODE_STRING(JOYCODE_8_ANALOG_Z)
	STANDARD_CODE_STRING(JOYCODE_8_ANALOG_Z_NEG)
	STANDARD_CODE_STRING(JOYCODE_8_ANALOG_Z_POS)
	STANDARD_CODE_STRING(MOUSECODE_1_ANALOG_X)
	STANDARD_CODE_STRING(MOUSECODE_1_ANALOG_Y)
	STANDARD_CODE_STRING(MOUSECODE_1_ANALOG_Z)
	STANDARD_CODE_STRING(MOUSECODE_2_ANALOG_X)
	STANDARD_CODE_STRING(MOUSECODE_2_ANALOG_Y)
	STANDARD_CODE_STRING(MOUSECODE_2_ANALOG_Z)
	STANDARD_CODE_STRING(MOUSECODE_3_ANALOG_X)
	STANDARD_CODE_STRING(MOUSECODE_3_ANALOG_Y)
	STANDARD_CODE_STRING(MOUSECODE_3_ANALOG_Z)
	STANDARD_CODE_STRING(MOUSECODE_4_ANALOG_X)
	STANDARD_CODE_STRING(MOUSECODE_4_ANALOG_Y)
	STANDARD_CODE_STRING(MOUSECODE_4_ANALOG_Z)
	STANDARD_CODE_STRING(MOUSECODE_5_ANALOG_X)
	STANDARD_CODE_STRING(MOUSECODE_5_ANALOG_Y)
	STANDARD_CODE_STRING(MOUSECODE_5_ANALOG_Z)
	STANDARD_CODE_STRING(MOUSECODE_6_ANALOG_X)
	STANDARD_CODE_STRING(MOUSECODE_6_ANALOG_Y)
	STANDARD_CODE_STRING(MOUSECODE_6_ANALOG_Z)
	STANDARD_CODE_STRING(MOUSECODE_7_ANALOG_X)
	STANDARD_CODE_STRING(MOUSECODE_7_ANALOG_Y)
	STANDARD_CODE_STRING(MOUSECODE_7_ANALOG_Z)
	STANDARD_CODE_STRING(MOUSECODE_8_ANALOG_X)
	STANDARD_CODE_STRING(MOUSECODE_8_ANALOG_Y)
	STANDARD_CODE_STRING(MOUSECODE_8_ANALOG_Z)

	STANDARD_CODE_STRING(GUNCODE_1_ANALOG_X)
	STANDARD_CODE_STRING(GUNCODE_1_ANALOG_Y)
	STANDARD_CODE_STRING(GUNCODE_2_ANALOG_X)
	STANDARD_CODE_STRING(GUNCODE_2_ANALOG_Y)
	STANDARD_CODE_STRING(GUNCODE_3_ANALOG_X)
	STANDARD_CODE_STRING(GUNCODE_3_ANALOG_Y)
	STANDARD_CODE_STRING(GUNCODE_4_ANALOG_X)
	STANDARD_CODE_STRING(GUNCODE_4_ANALOG_Y)
	STANDARD_CODE_STRING(GUNCODE_5_ANALOG_X)
	STANDARD_CODE_STRING(GUNCODE_5_ANALOG_Y)
	STANDARD_CODE_STRING(GUNCODE_6_ANALOG_X)
	STANDARD_CODE_STRING(GUNCODE_6_ANALOG_Y)
	STANDARD_CODE_STRING(GUNCODE_7_ANALOG_X)
	STANDARD_CODE_STRING(GUNCODE_7_ANALOG_Y)
	STANDARD_CODE_STRING(GUNCODE_8_ANALOG_X)
	STANDARD_CODE_STRING(GUNCODE_8_ANALOG_Y)
};



/***************************************************************************
    MACROS
***************************************************************************/

#define ANALOG_TYPE(code) (((code) < code_count) ? code_map[code].analogtype : ANALOG_TYPE_NONE)
#define JOYSTICK_TYPE(code) (code >= JOYCODE_1_ANALOG_X && code <= JOYCODE_8_ANALOG_Z_POS)



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

static input_code_info *code_map;
static input_code code_count;

/* Static information used in key/joy recording */
static input_code record_seq[SEQ_MAX];			/* buffer for key recording */
static int record_count;						/* number of key/joy press recorded */
static clock_t record_last;						/* time of last key/joy press */
static UINT8 record_analog;						/* are we recording an analog sequence? */



/***************************************************************************
    CORE IMPLEMENTATION
***************************************************************************/

/*************************************
 *
 *  Code table creation
 *
 *************************************/

void code_init(running_machine *machine)
{
	const os_code_info *codelist = osd_get_code_list();
	const os_code_info *info;
	input_code codenum;
	int extras;

	/* go through and count how many non-standard inputs we have */
	extras = 0;
	for (info = codelist; info->name != NULL; info++)
		if (info->inputcode == CODE_OTHER_DIGITAL || info->inputcode == CODE_OTHER_ANALOG_ABSOLUTE ||
			info->inputcode == CODE_OTHER_ANALOG_RELATIVE || info->inputcode >= __code_max)
		{
			extras++;
		}

	/* allocate the table */
	code_map = (input_code_info *)auto_malloc((__code_max + extras) * sizeof(code_map[0]));
	memset(code_map, 0, (__code_max + extras) * sizeof(code_map[0]));

	/* now go through and match up the OS codes to the standard codes */
	code_count = __code_max;
	for (info = codelist; info->name != NULL; info++)
	{
		if (info->inputcode == CODE_OTHER_DIGITAL || info->inputcode == CODE_OTHER_ANALOG_ABSOLUTE ||
			info->inputcode == CODE_OTHER_ANALOG_RELATIVE || info->inputcode >= __code_max)
		{
			if (info->inputcode == CODE_OTHER_ANALOG_ABSOLUTE)
				code_map[code_count].analogtype = ANALOG_TYPE_ABSOLUTE;
			else if (info->inputcode == CODE_OTHER_ANALOG_RELATIVE)
				code_map[code_count].analogtype = ANALOG_TYPE_RELATIVE;
			else
				code_map[code_count].analogtype = ANALOG_TYPE_NONE;
			code_map[code_count++].osinfo = info;
		}
		else
		{
			if (info->inputcode >= __code_absolute_analog_start && info->inputcode <= __code_absolute_analog_end)
				code_map[info->inputcode].analogtype = ANALOG_TYPE_ABSOLUTE;
			else if (info->inputcode >= __code_relative_analog_start && info->inputcode <= __code_relative_analog_end)
				code_map[info->inputcode].analogtype = ANALOG_TYPE_RELATIVE;
			else
				code_map[info->inputcode].analogtype = ANALOG_TYPE_NONE;
			code_map[info->inputcode].osinfo = info;
		}
	}

	/* finally, go through and make tokens for all the codes */
	for (codenum = 0; codenum < code_count; codenum++)
	{
		int nameindex;

		/* look up the name in the standard table if we can */
		if (codenum < __code_max)
			for (nameindex = 0; nameindex < sizeof(standard_code_strings) / sizeof(standard_code_strings[0]); nameindex++)
				if (standard_code_strings[nameindex].code == codenum)
					strncpy(code_map[codenum].token, standard_code_strings[nameindex].codename, sizeof(code_map[codenum].token) - 1);

		/* otherwise, make one out of the OSD name */
		if (code_map[codenum].token[0] == 0 && code_map[codenum].osinfo != NULL && code_map[codenum].osinfo->name != NULL)
		{
			int charindex;

			/* copy the user-friendly string */
			strncpy(code_map[codenum].token, code_map[codenum].osinfo->name, sizeof(code_map[codenum].token) - 1);

			/* replace spaces with underscores and convert to uppercase */
			for (charindex = 0; code_map[codenum].token[charindex] != 0; charindex++)
			{
				if (code_map[codenum].token[charindex] == ' ')
					code_map[codenum].token[charindex] = '_';
				else
					code_map[codenum].token[charindex] = toupper(code_map[codenum].token[charindex]);
			}
		}
	}
}



/*************************************
 *
 *  Return the analog value of a code.
 *
 *************************************/

INT32 code_analog_value(input_code code)
{
	INT32 value = ANALOG_VALUE_INVALID;

	profiler_mark(PROFILER_INPUT);
	if (code_map[code].osinfo != NULL && ANALOG_TYPE(code) != ANALOG_TYPE_NONE)
		value = osd_get_code_value(code_map[code].osinfo->oscode);
	profiler_mark(PROFILER_END);
	return value;
}



/*************************************
 *
 *  Is a code currently pressed?
 *  (Returns 1 indefinitely while
 *  the code is pressed.)
 *
 *************************************/

int code_pressed(input_code code)
{
	int pressed = 0;

	profiler_mark(PROFILER_INPUT);
	if (code_map[code].osinfo != NULL && ANALOG_TYPE(code) == ANALOG_TYPE_NONE)
		pressed = (osd_get_code_value(code_map[code].osinfo->oscode) != 0);
	profiler_mark(PROFILER_END);
	return pressed;
}



/*************************************
 *
 *  Is a code currently pressed?
 *  (Returns 1 only once for each
 *  press.)
 *
 *************************************/

int code_pressed_memory(input_code code)
{
	int pressed = 0;

	/* determine if the code is still being pressed */
	profiler_mark(PROFILER_INPUT);
	if (code_map[code].osinfo != NULL && ANALOG_TYPE(code) == ANALOG_TYPE_NONE)
		pressed = (osd_get_code_value(code_map[code].osinfo->oscode) != 0);

	/* if so, handle it specially */
	if (pressed)
	{
		/* if this is the first press, leave pressed = 1 */
		if (code_map[code].memory == 0)
			code_map[code].memory = 1;

		/* otherwise, reset pressed = 0 */
		else
			pressed = 0;
	}

	/* if we're not pressed, reset the memory field */
	else
		code_map[code].memory = 0;

	profiler_mark(PROFILER_END);
	return pressed;
}



/*************************************
 *
 *  Is a code currently pressed?
 *  (Returns 1 only once for each
 *  press, plus once for each
 *  autorepeat.)
 *
 *************************************/

int code_pressed_memory_repeat(input_code code, int speed)
{
	static int counter;
	static int keydelay;
	int pressed = 0;

	/* determine if the code is still being pressed */
	profiler_mark(PROFILER_INPUT);
	if (code_map[code].osinfo != NULL && ANALOG_TYPE(code) == ANALOG_TYPE_NONE)
		pressed = (osd_get_code_value(code_map[code].osinfo->oscode) != 0);

	/* if so, handle it specially */
	if (pressed)
	{
		/* if this is the first press, set a 3x delay and leave pressed = 1 */
		if (code_map[code].memory == 0)
		{
			code_map[code].memory = 1;
			keydelay = 3;
			counter = 0;
		}

		/* if this is an autorepeat case, set a 1x delay and leave pressed = 1 */
		else if (++counter > keydelay * speed * SUBSECONDS_TO_HZ(Machine->screen[0].refresh) / 60)
		{
			keydelay = 1;
			counter = 0;
		}

		/* otherwise, reset pressed = 0 */
		else
			pressed = 0;
	}

	/* if we're not pressed, reset the memory field */
	else
		code_map[code].memory = 0;

	profiler_mark(PROFILER_END);
	return pressed;
}



/*************************************
 *
 *  Is a code currently pressed?
 *  (Returns 1 if it is not pressed
 *  and hasn't been tracked by the
 *  code_pressed_memory functions
 *  above.)
 *
 *************************************/

static int code_pressed_not_memorized(input_code code)
{
	int pressed = 0;

	/* determine if the code is still being pressed */
	profiler_mark(PROFILER_INPUT);
	if (code_map[code].osinfo != NULL && ANALOG_TYPE(code) == ANALOG_TYPE_NONE)
		pressed = (osd_get_code_value(code_map[code].osinfo->oscode) != 0);

	/* if so, handle it specially */
	if (pressed)
	{
		if (code_map[code].memory != 0)
			pressed = 0;
	}

	/* if we're not pressed, reset the memory field */
	else
		code_map[code].memory = 0;

	profiler_mark(PROFILER_END);
	return pressed;
}



/*************************************
 *
 *  Return the input code if any
 *  code is pressed; otherwise return
 *  CODE_NONE.
 *
 *************************************/

input_code code_read_async(void)
{
	input_code code;

	profiler_mark(PROFILER_INPUT);

	/* scan all codes for one that is pressed */
	for (code = 0; code < code_count; code++)
		if (code_pressed_memory(code))
			break;

	profiler_mark(PROFILER_END);
	return (code == code_count) ? CODE_NONE : code;
}



/*************************************
 *
 *  Code utilities.
 *
 *************************************/

int code_analog_type(input_code code)
{
	return ANALOG_TYPE(code);
}


const char *code_name(input_code code)
{
	/* in-range codes just return either the OSD name or the standard name */
	if (code < code_count && code_map[code].osinfo)
		return code_map[code].osinfo->name;

	/* a few special other codes */
	switch (code)
	{
		case CODE_NONE : return "None";
		case CODE_NOT : return "not";
		case CODE_OR : return "or";
	}

	/* everything else is n/a */
	return "n/a";
}


input_code token_to_code(const char *token)
{
	input_code code;

	/* look for special cases */
	if (!mame_stricmp(token, "OR"))
		return CODE_OR;
	if (!mame_stricmp(token, "NOT"))
		return CODE_NOT;
	if (!mame_stricmp(token, "NONE"))
		return CODE_NONE;
	if (!mame_stricmp(token, "DEFAULT"))
		return CODE_DEFAULT;

	/* look for a match against any of the codes in the table */
	for (code = 0; code < code_count; code++)
		if (!strcmp(token, code_map[code].token))
			return code;

	return CODE_NONE;
}


void code_to_token(input_code code, char *token)
{
	/* first look in the table if we can */
	if (code < code_count)
	{
		strcpy(token, code_map[code].token);
		return;
	}

	/* some extra names */
	switch (code)
	{
		case CODE_OR:		strcpy(token, "OR");		return;
		case CODE_NOT:		strcpy(token, "NOT");		return;
		case CODE_NONE:		strcpy(token, "NONE");		return;
		case CODE_DEFAULT:	strcpy(token, "DEFAULT");	return;
	}

	/* return an empty token */
	token[0] = 0;
	return;
}



/*************************************
 *
 *  Sequence setting helpers.
 *
 *************************************/

void seq_set_0(input_seq *seq)
{
	int codenum;
	for (codenum = 0; codenum < SEQ_MAX; codenum++)
		seq->code[codenum] = CODE_NONE;
}


void seq_set_1(input_seq *seq, input_code code)
{
	int codenum;
	seq->code[0] = code;
	for (codenum = 1; codenum < SEQ_MAX; codenum++)
		seq->code[codenum] = CODE_NONE;
}


void seq_set_2(input_seq *seq, input_code code1, input_code code2)
{
	int codenum;
	seq->code[0] = code1;
	seq->code[1] = code2;
	for (codenum = 2; codenum < SEQ_MAX; codenum++)
		seq->code[codenum] = CODE_NONE;
}


void seq_set_3(input_seq *seq, input_code code1, input_code code2, input_code code3)
{
	int codenum;
	seq->code[0] = code1;
	seq->code[1] = code2;
	seq->code[2] = code3;
	for (codenum = 3; codenum < SEQ_MAX; codenum++)
		seq->code[codenum] = CODE_NONE;
}


void seq_set_4(input_seq *seq, input_code code1, input_code code2, input_code code3, input_code code4)
{
	int codenum;
	seq->code[0] = code1;
	seq->code[1] = code2;
	seq->code[2] = code3;
	seq->code[3] = code4;
	for (codenum = 4; codenum < SEQ_MAX; codenum++)
		seq->code[codenum] = CODE_NONE;
}


void seq_set_5(input_seq *seq, input_code code1, input_code code2, input_code code3, input_code code4, input_code code5)
{
	int codenum;
	seq->code[0] = code1;
	seq->code[1] = code2;
	seq->code[2] = code3;
	seq->code[3] = code4;
	seq->code[4] = code5;
	for (codenum = 5; codenum < SEQ_MAX; codenum++)
		seq->code[codenum] = CODE_NONE;
}



/*************************************
 *
 *  Copy and compare helpers.
 *
 *************************************/

void seq_copy(input_seq *seqdst, const input_seq *seqsrc)
{
	*seqdst = *seqsrc;
}


int seq_cmp(const input_seq *seqa, const input_seq *seqb)
{
	int codenum;
	for (codenum = 0; codenum < SEQ_MAX; codenum++)
		if (seqa->code[codenum] != seqb->code[codenum])
			return -1;
	return 0;
}



/*************************************
 *
 *  Is a given sequence pressed?
 *
 *************************************/

int seq_pressed(const input_seq *seq)
{
	int codenum;
	int result = 1;
	int invert = 0;
	int count = 0;

	/* iterate over all of the codes */
	for (codenum = 0; codenum < SEQ_MAX && seq->code[codenum] != CODE_NONE; codenum++)
	{
		input_code code = seq->code[codenum];

		switch (code)
		{
			/* OR: if the preceding result was non-zero after processing at least one code, stop there */
			/* otherwise, reset the state and continue */
			case CODE_OR:
				if (result != 0 && count > 0)
					return 1;
				result = 1;
				invert = 0;
				count = 0;
				break;

			/* NOT set the invert flag */
			case CODE_NOT:
				invert = !invert;
				break;

			/* default: check the code if we are still live; otherwise just keep going */
			default:
				if (result)
				{
					int pressed = code_pressed_not_memorized(code);
					if ((pressed != 0) == invert)
						result = 0;
					count++;
				}
				invert = 0;
				break;
		}
	}
	return (result != 0 && count > 0);
}



/*************************************
 *
 *  Determine the analog value of
 *  a sequence.
 *
 *************************************/

INT32 seq_analog_value(const input_seq *seq, int *analogtype)
{
	int codenum, type = ANALOG_TYPE_NONE;
	INT32 result = 0;
	int enable = 1;
	int invert = 0;
	int count = 0;
	int changed = 0;

	/* iterate over all of the codes */
	for (codenum = 0; codenum < SEQ_MAX && seq->code[codenum] != CODE_NONE; codenum++)
	{
		input_code code = seq->code[codenum];

		switch (code)
		{
			/* OR: if the preceding enable was non-zero after processing at least one code, stop there */
			/* otherwise, reset the state and continue */
			case CODE_OR:
				if (enable != 0 && count > 0 && (result != 0 || changed))
				{
					*analogtype = type;
					return result;
				}
				enable = 1;
				invert = 0;
				count = 0;
				break;

			/* NOT set the invert flag */
			case CODE_NOT:
				invert = !invert;
				break;

			/* default: check the code if we are still live; otherwise just keep going */
			default:
				if (enable)
				{
					/* for analog codes, that becomes the result (only one analog code per OR section) */
					if (ANALOG_TYPE(code) != ANALOG_TYPE_NONE)
					{
						INT32 value = code_analog_value(code);
						if (value != ANALOG_VALUE_INVALID)
						{
							result = value;
							type = ANALOG_TYPE(code);
							count++;
							if (type == ANALOG_TYPE_ABSOLUTE && result != code_map[code].prev_analog)
							{
								changed = 1;
								code_map[code].prev_analog = result;
							}
						}
					}

					/* for digital codes, update the enable state */
					else
					{
						int pressed = code_pressed_not_memorized(code);
						if ((pressed != 0) == invert)
							enable = 0;
					}
				}
				invert = 0;
				break;
		}
	}
	if (enable != 0 && count > 0 && (result != 0 || changed))
	{
		*analogtype = type;
		return result;
	}
	*analogtype = ANALOG_TYPE_NONE;
	return 0;
}



/*************************************
 *
 *  Return the friendly name for a
 *  sequence
 *
 *************************************/

char *seq_name(const input_seq *seq, char *buffer, unsigned max)
{
	char *dest = buffer;
	int codenum;

	for (codenum = 0; codenum < SEQ_MAX && seq->code[codenum] != CODE_NONE; codenum++)
	{
		const char* name;

		/* this reduces those pesky "blah or n/a" constructs */
		if (seq->code[codenum] == CODE_OR && (codenum + 1 >= SEQ_MAX || !strcmp(code_name(seq->code[codenum + 1]), "n/a")))
		{
			codenum++;
			continue;
		}

		/* append a space if we are not the first code */
		if (codenum != 0 && 1 + 1 <= max)
		{
			*dest++ = ' ';
			max -= 1;
		}

		/* get the friendly name */
		name = code_name(seq->code[codenum]);
		if (!name)
			break;

		/* append it */
		if (strlen(name) + 1 <= max)
		{
			strcpy(dest, name);
			dest += strlen(name);
			max -= (UINT32)strlen(name);
		}
	}

	/* if we ended up with nothing, say DEF_STR( None ), otherwise NULL-terminate */
	if (dest == buffer && 4 + 1 <= max)
		strcpy(dest, "None");
	else
		*dest = 0;

	return buffer;
}



/*************************************
 *
 *  Sequence validation
 *
 *************************************/

static int is_seq_valid(const input_seq *seq)
{
	int last_code_was_operand = 0;
	int positive_code_count = 0;
	int pending_not = 0;
	int analog_count = 0;
	int seqnum;

	for (seqnum = 0; seqnum < SEQ_MAX && seq->code[seqnum] != CODE_NONE; seqnum++)
	{
		input_code code = seq->code[seqnum];

		switch (code)
		{
			case CODE_OR:
				/* if the last code was't an operand or if there were no positive codes, this is invalid */
				if (!last_code_was_operand || positive_code_count == 0)
					return 0;

				/* reset the state after an OR */
				pending_not = 0;
				positive_code_count = 0;
				last_code_was_operand = 0;
				analog_count = 0;
				break;

			case CODE_NOT:
				/* disallow a double not */
				if (pending_not)
					return 0;

				/* note that there is a pending NOT, and that this was not an operand */
				pending_not = 1;
				last_code_was_operand = 0;
				break;

			default:
				/* if this code wasn't NOT-ed, increment the number of positive codes */
				if (!pending_not)
					positive_code_count++;

				/* some special checks for analog codes */
				if (ANALOG_TYPE(code) != ANALOG_TYPE_NONE)
				{
					/* NOT is invalid before an analog code */
					if (pending_not)
						return 0;

					/* there can only be one per OR section */
					analog_count++;
					if (analog_count > 1)
						return 0;
				}

				/* clear any pending NOTs and note that this was an operand */
				pending_not = 0;
				last_code_was_operand = 1;
				break;
		}
	}

	/* we must end with an operand, and must have at least one positive code */
	return (positive_code_count > 0) && last_code_was_operand;
}



/*************************************
 *
 *  Sequence recording
 *
 *************************************/

void seq_read_async_start(int analog)
{
	input_code codenum;

	/* reset the recording count and the clock */
	record_count = 0;
	record_last = clock();
	record_analog = analog;

	/* reset code memory, otherwise this memory may interferes with the input memory */
	/* for analog codes, get the current value so we can look for changes */
	for (codenum = 0; codenum < code_count; codenum++)
	{
		if (code_map[codenum].analogtype == ANALOG_TYPE_NONE)
			code_map[codenum].memory = 1;
		else
			code_map[codenum].memory = code_analog_value(codenum);
	}
}


int seq_read_async(input_seq *seq, int first)
{
	input_code newcode;

	/* if UI_CANCEL is pressed, return 1 (abort) */
	if (input_ui_pressed(IPT_UI_CANCEL))
		return 1;

	/* if we're at the end, or if the RECORD_TIME has passed, we're done */
	if (record_count == SEQ_MAX || (record_count > 0 && clock() > record_last + RECORD_TIME))
	{
		int seqnum = 0;

		/* if this isn't the first code, append it to the end */
		if (!first)
			while (seqnum < SEQ_MAX && seq->code[seqnum] != CODE_NONE)
				seqnum++;

		/* if no space restart */
		if (seqnum + record_count + (seqnum != 0) > SEQ_MAX)
			seqnum = 0;

		/* insert at the current location */
		if (seqnum + record_count + (seqnum != 0) <= SEQ_MAX)
		{
			int recordnum;

			/* toggle the joystick axis if needed */
			if (JOYSTICK_TYPE(record_seq[0]))
			{
				UINT32 joy_num = (record_seq[0] - JOYCODE_1_ANALOG_X) / 3;
				input_code joy_code = JOYCODE_1_ANALOG_X + joy_num * 3;
				int joy_full_axis = 1;
				int joy_axis_toggled = 0;

				/* set to full axis if first code or last code was a part axis */
				if  (seqnum > 0)
				{
					input_code last_code = seq->code[seqnum - 1];

					if (last_code == joy_code)
					{
						joy_axis_toggled = 1;
						joy_full_axis = 0;
					}
					else if (last_code == (joy_code + 1) || last_code == (joy_code + 2))
						joy_axis_toggled = 1;
				}

				if (joy_full_axis) record_seq[0] = joy_code;

				/* move back to after the code we toggled */
				if (joy_axis_toggled)
				{
					if (seqnum == 1)
						seqnum = 0;
					else
						seqnum -= 2;
				}
			}
			if (seqnum != 0)
				seq->code[seqnum++] = CODE_OR;
			for (recordnum = 0; recordnum < record_count; recordnum++)
				seq->code[seqnum++] = record_seq[recordnum];
		}

		/* fill with CODE_NONE until the end */
		while (seqnum < SEQ_MAX)
			seq->code[seqnum++] = CODE_NONE;

		/* if the final result is invalid, reset to nothing */
		if (!is_seq_valid(seq))
			seq_set_1(seq, CODE_NONE);

		/* return 0 to indicate that we are finished */
		return 0;
	}

	/* digital case: see if we have a new code to process */
	if (!record_analog)
	{
		newcode = code_read_async();
		if (newcode != CODE_NONE)
		{
			/* if code is duplicate negate the code */
			if (record_count > 0 && newcode == record_seq[record_count - 1])
				record_seq[record_count - 1] = CODE_NOT;

			/* append the code and reset the clock */
			record_seq[record_count++] = newcode;
			record_last = clock();
		}
	}

	/* analog case: see if we have an analog change of sufficient amount */
	/* Absolute > 25% --- Relative > 5% */
	else
	{
		/* scan all the analog codes for change */
		for (newcode = 0; newcode < code_count; newcode++)
		{
			int ignore_code = 0;
			/* check to see if it is a joystick axis */
			/* we want to ignore the joystick full axis codes */
			/* we will use the half axis to toggle between full and half */
			if (JOYSTICK_TYPE(newcode))
			{
				UINT32 joy_num = (newcode - JOYCODE_1_ANALOG_X) / 3;
				input_code joy_code = JOYCODE_1_ANALOG_X + joy_num * 3;
				if (newcode == joy_code)
					/* full joystick axis, so ignore it */
					ignore_code = 1;
			}
			if (ANALOG_TYPE(newcode) != ANALOG_TYPE_NONE && !ignore_code)
			{
				INT32 diff = code_analog_value(newcode);
				if (diff != ANALOG_VALUE_INVALID)
				{
					if (ANALOG_TYPE(newcode) == ANALOG_TYPE_ABSOLUTE)
						diff = code_map[newcode].memory - diff;
					if (diff < 0) diff = -diff;
					if (diff > (ANALOG_VALUE_MAX - ANALOG_VALUE_MIN) / ((ANALOG_TYPE(newcode) == ANALOG_TYPE_ABSOLUTE) ? 4 : 20))
						break;
				}
			}
		}
		/* if we got one, add it to the sequence and force an update next time round */
		if (newcode != code_count)
		{
			record_seq[record_count++] = newcode;
			record_last = clock() - RECORD_TIME;
		}
	}

	/* return -1 to indicate that we are still reading */
	return -1;
}



/*************************************
 *
 *  Sequence utilities
 *
 *************************************/

int string_to_seq(const char *string, input_seq *seq)
{
	char token[MAX_TOKEN_LEN + 1];
	int tokenpos, seqnum = 0;

	/* start with a blank sequence */
	seq_set_0(seq);

	/* loop until we're done */
	while (1)
	{
		/* trim any leading spaces */
		while (*string != 0 && isspace(*string))
			string++;

		/* bail if we're done */
		if (*string == 0)
			break;

		/* build up a token */
		tokenpos = 0;
		while (*string != 0 && !isspace(*string) && tokenpos < MAX_TOKEN_LEN)
			token[tokenpos++] = toupper(*string++);
		token[tokenpos] = 0;

		/* translate and add to the sequence */
		seq->code[seqnum++] = token_to_code(token);
	}
	return seqnum;
}


void seq_to_string(const input_seq *seq, char *string, int maxlen)
{
	int seqnum;

	/* reset the output string */
	*string = 0;

	/* loop over each code and translate to a string */
	for (seqnum = 0; seqnum < SEQ_MAX && seq->code[seqnum] != CODE_NONE; seqnum++)
	{
		char token[MAX_TOKEN_LEN];

		/* get the token */
		code_to_token(seq->code[seqnum], token);

		/* if we will fit, append the token to the string */
		if (strlen(string) + strlen(token) + (seqnum != 0) < maxlen)
		{
			if (seqnum != 0)
				strcat(string, " ");
			strcat(string, token);
		}
	}
}
