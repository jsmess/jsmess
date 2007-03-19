/***************************************************************************

Atari Sprint 4 + Ultra Tank driver

***************************************************************************/

#include "sound/discrete.h"

/* discrete sound input nodes */
#define SPRINT4_SKID1_EN        NODE_01
#define SPRINT4_SKID2_EN        NODE_02
#define SPRINT4_SKID3_EN        NODE_03
#define SPRINT4_SKID4_EN        NODE_04
#define SPRINT4_MOTOR1_DATA     NODE_05
#define SPRINT4_MOTOR2_DATA     NODE_06
#define SPRINT4_MOTOR3_DATA     NODE_07
#define SPRINT4_MOTOR4_DATA     NODE_08
#define SPRINT4_CRASH_DATA      NODE_09
#define SPRINT4_ATTRACT_EN      NODE_10

/*----------- defined in audio/sprint4.c -----------*/

extern discrete_sound_block sprint4_discrete_interface[];


/*----------- defined in video/sprint4.c -----------*/

extern UINT8* sprint4_videoram;
extern int sprint4_collision[4];

extern PALETTE_INIT( sprint4 );
extern PALETTE_INIT( ultratnk );

extern VIDEO_EOF( sprint4 );
extern VIDEO_EOF( ultratnk );
extern VIDEO_START( sprint4 );
extern VIDEO_START( ultratnk );
extern VIDEO_UPDATE( sprint4 );
extern VIDEO_UPDATE( ultratnk );

extern WRITE8_HANDLER( sprint4_video_ram_w );
