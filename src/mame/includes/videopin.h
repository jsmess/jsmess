/*************************************************************************

    Atari Video Pinball hardware

*************************************************************************/

#include "sound/discrete.h"

/* Discrete Sound Input Nodes */
#define VIDEOPIN_OCTAVE_DATA	NODE_01
#define VIDEOPIN_NOTE_DATA		NODE_02
#define VIDEOPIN_BELL_EN		NODE_03
#define VIDEOPIN_BONG_EN		NODE_04
#define VIDEOPIN_ATTRACT_EN		NODE_05
#define VIDEOPIN_VOL_DATA		NODE_06


/*----------- defined in audio/videopin.c -----------*/

extern discrete_sound_block videopin_discrete_interface[];


/*----------- defined in video/videopin.c -----------*/

extern UINT8* videopin_video_ram;

WRITE8_HANDLER( videopin_video_ram_w );
WRITE8_HANDLER( videopin_ball_w );

VIDEO_START( videopin );
VIDEO_UPDATE( videopin );

