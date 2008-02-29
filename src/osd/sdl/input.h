//============================================================
//
//  input.h - SDL implementation of MAME input routines
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//  SDLMAME by Olivier Galibert and R. Belmont
//
//============================================================

#ifndef __INPUT_H
#define __INPUT_H


//============================================================
//  PROTOTYPES
//============================================================

void sdlinput_init(running_machine *machine);
void sdlinput_poll(running_machine *machine);
int  sdlinput_should_hide_mouse(void);

#ifdef SDLMAME_WIN32
void  sdlinput_process_events_buf(void);
#endif

#endif /* __INPUT_H */
