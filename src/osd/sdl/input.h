//============================================================
//
//	input.h - SDL implementation of MAME input routines
//
//============================================================

#ifndef __INPUT_H
#define __INPUT_H

//============================================================
//	MACROS
//============================================================

// Define the keyboard indicators.
// (Definitions borrowed from ntddkbd.h)
//

#define IOCTL_KEYBOARD_SET_INDICATORS        
#define IOCTL_KEYBOARD_QUERY_TYPEMATIC       
#define IOCTL_KEYBOARD_QUERY_INDICATORS      


//============================================================
//	PARAMETERS
//============================================================

#define KEYBOARD_CAPS_LOCK_ON     4
#define KEYBOARD_NUM_LOCK_ON      2
#define KEYBOARD_SCROLL_LOCK_ON   1

typedef struct _KEYBOARD_INDICATOR_PARAMETERS {
    unsigned short UnitId;		// Unit identifier.
    unsigned short LedFlags;		// LED indicator state.

} KEYBOARD_INDICATOR_PARAMETERS, *PKEYBOARD_INDICATOR_PARAMETERS;

// table entry indices
#define MAME_KEY		0
#define SDL_KEY			1
#define VIRTUAL_KEY		2
#define ASCII_KEY		3


//============================================================
//	PROTOTYPES
//============================================================

extern const int win_key_trans_table[][4];
extern int win_use_mouse;

#ifdef SDLMAME_WIN32
void win_process_events_buf(void);
#endif

int sdlinput_init(running_machine *machine);
void win_process_events(void);
void start_led(void);
void stop_led(void);
void input_mouse_button_down(int button, int x, int y);
void input_mouse_button_up(int button);

#endif /* ifndef __INPUTD_H */
