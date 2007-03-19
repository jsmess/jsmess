//============================================================
//
//  input.h - Win32 implementation of MAME input routines
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

#ifndef __INPUT_H
#define __INPUT_H

//============================================================
//  PARAMETERS
//============================================================

#define KEYBOARD_CAPS_LOCK_ON     4
#define KEYBOARD_NUM_LOCK_ON      2
#define KEYBOARD_SCROLL_LOCK_ON   1

// table entry indices
#define MAME_KEY		0
#define DI_KEY			1
#define VIRTUAL_KEY		2
#define ASCII_KEY		3


//============================================================
//  PROTOTYPES
//============================================================

extern const int win_key_trans_table[][4];
extern int win_use_mouse;

int wininput_init(running_machine *machine);

void win_pause_input(running_machine *machine, int pause);
void wininput_poll(void);

BOOL win_raw_mouse_update(HANDLE in_device_handle);

void input_mouse_button_down(int button, int x, int y);
void input_mouse_button_up(int button);

int win_is_mouse_captured(void);

#endif /* __INPUT_H */
