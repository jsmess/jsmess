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

int osd_update(mame_time emutime);


/*
  Returns a pointer to the text to display when the FPS display is toggled.
  This normally includes information about the frameskip, FPS, and percentage
  of full game speed.
*/
const char *osd_get_fps_text(const performance_info *performance);



/******************************************************************************

    Sound

******************************************************************************/

/*
  osd_start_audio_stream() is called at the start of the emulation to initialize
  the output stream, then osd_update_audio_stream() is called every frame to
  feed new data. osd_stop_audio_stream() is called when the emulation is stopped.

  The sample rate is fixed at Machine->sample_rate. Samples are 16-bit, signed.
  When the stream is stereo, left and right samples are alternated in the
  stream.

  osd_start_audio_stream() and osd_update_audio_stream() must return the number
  of samples (or couples of samples, when using stereo) required for next frame.
  This will be around Machine->sample_rate / Machine->screen[0].refresh,
  the code may adjust it by SMALL AMOUNTS to keep timing accurate and to
  maintain audio and video in sync when using vsync. Note that sound emulation,
  especially when DACs are involved, greatly depends on the number of samples
  per frame to be roughly constant, so the returned value must always stay close
  to the reference value of Machine->sample_rate / Machine->screen[0].refresh.
  Of course that value is not necessarily an integer so at least a +/- 1
  adjustment is necessary to avoid drifting over time.
*/
int osd_start_audio_stream(int stereo);
int osd_update_audio_stream(INT16 *buffer);
void osd_stop_audio_stream(void);

/*
  control master volume. attenuation is the attenuation in dB (a negative
  number). To convert from dB to a linear volume scale do the following:
    volume = MAX_VOLUME;
    while (attenuation++ < 0)
        volume /= 1.122018454;      //  = (10 ^ (1/20)) = 1dB
*/
void osd_set_mastervolume(int attenuation);
int osd_get_mastervolume(void);

void osd_sound_enable(int enable);



/******************************************************************************

    Controls

******************************************************************************/

typedef UINT32 os_code;

typedef struct _os_code_info os_code_info;
struct _os_code_info
{
	char *		name;			/* OS dependant name; 0 terminates the list */
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


/* Joystick calibration routines BW 19981216 */
/* Do we need to calibrate the joystick at all? */
int osd_joystick_needs_calibration(void);
/* Preprocessing for joystick calibration. Returns 0 on success */
void osd_joystick_start_calibration(void);
/* Prepare the next calibration step. Return a description of this step. */
/* (e.g. "move to upper left") */
const char *osd_joystick_calibrate_next(void);
/* Get the actual joystick calibration data for the current position */
void osd_joystick_calibrate(void);
/* Postprocessing (e.g. saving joystick data to config) */
void osd_joystick_end_calibration(void);



#endif	/* __OSDEPEND_H__ */
