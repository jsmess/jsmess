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

#include "window.h"

//============================================================
//  PROTOTYPES
//============================================================

void sdlinput_init(running_machine *machine);
void sdlinput_poll(running_machine *machine);
int  sdlinput_should_hide_mouse(running_machine *machine);

sdl_window_info *sdlinput_get_focus_window(running_machine *machine);

void  sdlinput_process_events_buf(running_machine *machine);

#endif /* __INPUT_H */
