/*************************************************************************

    Atari Skydiver hardware

*************************************************************************/

#include "sound/discrete.h"

/* Discrete Sound Input Nodes */
#define ULTRATNK_FIRE1_EN			NODE_01
#define ULTRATNK_FIRE2_EN			NODE_02
#define ULTRATNK_MOTOR1_DATA		NODE_03
#define ULTRATNK_MOTOR2_DATA		NODE_04
#define ULTRATNK_EXPLOSION_DATA		NODE_05
#define ULTRATNK_ATTRACT_EN			NODE_06


/*----------- defined in sndhrdw/skydiver.c -----------*/

extern discrete_sound_block ultratnk_discrete_interface[];


/*----------- defined in video/ultratnk.c -----------*/

int ultratnk_collision[4];

PALETTE_INIT( ultratnk );
VIDEO_START( ultratnk );
VIDEO_UPDATE( ultratnk );
VIDEO_EOF( ultratnk );

WRITE8_HANDLER( ultratnk_video_ram_w );
