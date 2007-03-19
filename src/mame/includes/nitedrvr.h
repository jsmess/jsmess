/*************************************************************************

    Atari Night Driver hardware

*************************************************************************/

#include "sound/discrete.h"

/* Discrete Sound Input Nodes */
#define NITEDRVR_BANG_DATA	NODE_01
#define NITEDRVR_SKID1_EN	NODE_02
#define NITEDRVR_SKID2_EN	NODE_03
#define NITEDRVR_MOTOR_DATA	NODE_04
#define NITEDRVR_CRASH_EN	NODE_05
#define NITEDRVR_ATTRACT_EN	NODE_06


/*----------- defined in machine/nitedrvr.c -----------*/

READ8_HANDLER( nitedrvr_in0_r );
READ8_HANDLER( nitedrvr_in1_r );
READ8_HANDLER( nitedrvr_steering_reset_r );
WRITE8_HANDLER( nitedrvr_steering_reset_w );
WRITE8_HANDLER( nitedrvr_out0_w );
WRITE8_HANDLER( nitedrvr_out1_w );

void nitedrvr_crash_toggle(int dummy);


/*----------- defined in audio/nitedrvr.c -----------*/

extern discrete_sound_block nitedrvr_discrete_interface[];


/*----------- defined in video/nitedrvr.c -----------*/

extern unsigned char *nitedrvr_hvc;
WRITE8_HANDLER( nitedrvr_hvc_w );
VIDEO_UPDATE( nitedrvr );
