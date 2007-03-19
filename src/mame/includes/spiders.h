/*************************************************************************

    Sigma Spiders hardware

*************************************************************************/

#include "sound/discrete.h"

/* Discrete Sound Input Nodes */
#define SPIDERS_WEB_SOUND_DATA      NODE_01
#define SPIDER_WEB_SOUND_MOD_DATA   NODE_02
#define SPIDERS_FIRE_EN             NODE_03
#define SPIDERS_EXP_EN              NODE_04
#define SPIDERS_SUPER_WEB_EN        NODE_05
#define SPIDERS_SUPER_WEB_EXPL_EN   NODE_06
#define SPIDERS_X_EN                NODE_07


/*----------- defined in audio/spiders.c -----------*/

extern discrete_sound_block spiders_discrete_interface[];


/*----------- defined in drivers/spiders.c -----------*/

extern UINT8 *spiders_ram;

/*----------- defined in machine/spiders.c -----------*/

extern int spiders_video_flip;
MACHINE_START( spiders );
MACHINE_RESET( spiders );
INTERRUPT_GEN( spiders_timed_irq );

/*----------- defined in video/spiders.c -----------*/

VIDEO_START( spiders );
VIDEO_UPDATE( spiders );
