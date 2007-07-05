//============================================================
//
//	input.h - SDL implementation of MAME input routines
//
//============================================================

#ifndef __INPUT_H
#define __INPUT_H


// table entry indices
#define MAME_KEY		0
#define SDL_KEY			1
#define VIRTUAL_KEY		2
#define ASCII_KEY		3


//============================================================
//	PROTOTYPES
//============================================================

#ifdef SDLMAME_WIN32
void sdlinput_process_events_buf(void);
#endif

int sdlinput_init(running_machine *machine);
int sdl_is_mouse_captured(void);
void sdlinput_process_events(void);

#endif /* ifndef __INPUTD_H */
