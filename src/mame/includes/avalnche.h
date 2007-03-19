/*************************************************************************

    Atari Avalanche hardware

*************************************************************************/

#include "sound/discrete.h"

/* Discrete Sound Input Nodes */
#define AVALNCHE_AUD0_EN			NODE_01
#define AVALNCHE_AUD1_EN			NODE_02
#define AVALNCHE_AUD2_EN			NODE_03
#define AVALNCHE_SOUNDLVL_DATA		NODE_04
#define AVALNCHE_ATTRACT_EN			NODE_05


/*----------- defined in machine/avalnche.c -----------*/

READ8_HANDLER( avalnche_input_r );
WRITE8_HANDLER( avalnche_output_w );
WRITE8_HANDLER( avalnche_noise_amplitude_w );
INTERRUPT_GEN( avalnche_interrupt );


/*----------- defined in audio/avalnche.c -----------*/

extern discrete_sound_block avalnche_discrete_interface[];


/*----------- defined in video/avalnche.c -----------*/

WRITE8_HANDLER( avalnche_videoram_w );
VIDEO_UPDATE( avalnche );
