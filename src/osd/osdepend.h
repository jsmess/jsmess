/***************************************************************************

    osdepend.h

    OS-dependent code interface.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

****************************************************************************

    The prototypes in this file describe the interfaces that the MAME core
    relies upon to interact with the outside world. They are broken out into
    several categories.

    The general flow for an OSD port of MAME is as follows:

        - parse the command line or display the frontend
        - call run_game (mame.c) with the index in the driver list of
            the game selected
            - osd_init() is called shortly afterwards; at this time, you are
                expected to set up the display system and create render_targets
            - the input system will call osd_get_code_list()
            - the input port system will call osd_customize_inputport_list()
            - the sound system will call osd_start_audio_stream()
            - while the game runs, osd_update() will be called periodically
            - when the game exits, we return from run_game()
        - the OSD layer is now in control again

    This process is expected to be in flux over the next several versions
    (this was written during 0.109u2 development) as some of the OSD
    responsibilities are pushed into the core.

*******************************************************************c********/

#pragma once

#ifndef __OSDEPEND_H__
#define __OSDEPEND_H__

#include "mamecore.h"
#include "osdcore.h"
#include "timer.h"


/*-----------------------------------------------------------------------------
    osd_init: initialize the OSD system.

    Parameters:

        machine - pointer to a structure that contains parameters for the
            current "machine"

    Return value:

        0 if no errors occurred, or non-zero in case of an error.

    Notes:

        This function has several responsibilities.

        The most important responsibility of this function is to create the
        render_targets that will be used by the MAME core to provide graphics
        data to the system. Although it is possible to do this later, the
        assumption in the MAME core is that the user interface will be
        visible starting at osd_init() time, so you will have some work to
        do to avoid these assumptions.

        A secondary responsibility of this function is to initialize any
        other OSD systems that require information about the current
        running_machine.

        This callback is also the last opportunity to adjust the options
        before they are consumed by the rest of the core.

        Note that there is no corresponding osd_exit(). Rather, like most
        systems in MAME, you can register an exit callback via the
        add_exit_callback() function in mame.c.

    Future work/changes:

        The return value may be removed in the future. It is recommended that
        if you encounter an error at this time, that you call the core
        function fatalerror() with a message to the user rather than relying
        on this return value.

        Audio and input initialization may eventually move into here as well,
        instead of relying on independent callbacks from each system.
-----------------------------------------------------------------------------*/
int osd_init(running_machine *machine);


void osd_wait_for_debugger(void);



/******************************************************************************

    Display

******************************************************************************/

void osd_update(int skip_redraw);




/******************************************************************************

    Sound

******************************************************************************/

void osd_update_audio_stream(INT16 *buffer, int samples_this_frame);

/*
  control master volume. attenuation is the attenuation in dB (a negative
  number). To convert from dB to a linear volume scale do the following:
    volume = MAX_VOLUME;
    while (attenuation++ < 0)
        volume /= 1.122018454;      //  = (10 ^ (1/20)) = 1dB
*/
void osd_set_mastervolume(int attenuation);



/******************************************************************************

    Controls

******************************************************************************/

typedef UINT32 os_code;

typedef struct _os_code_info os_code_info;
struct _os_code_info
{
	const char *name;			/* OS dependant name; 0 terminates the list */
	os_code		oscode;			/* OS dependant code */
	input_code	inputcode;		/* CODE_xxx equivalent from input.h, or one of CODE_OTHER_* if n/a */
};

/*
  return a list of all available inputs (see input.h)
*/
const os_code_info *osd_get_code_list(void);

/*
  return the value of the specified input. digital inputs return 0 or 1. analog
  inputs should return a value between -65536 and +65536. oscode is the OS dependent
  code specified in the list returned by osd_get_code_list().
*/
INT32 osd_get_code_value(os_code oscode);

/*
  inptport.c defines some general purpose defaults for key and joystick bindings.
  They may be further adjusted by the OS dependent code to better match the
  available keyboard, e.g. one could map pause to the Pause key instead of P, or
  snapshot to PrtScr instead of F12. Of course the user can further change the
  settings to anything he/she likes.
  This function is called on startup, before reading the configuration from disk.
  Scan the list, and change the keys/joysticks you want.
*/
void osd_customize_inputport_list(input_port_default_entry *defaults);



#endif	/* __OSDEPEND_H__ */
