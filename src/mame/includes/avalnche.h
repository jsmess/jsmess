/*************************************************************************

    Atari Avalanche hardware

*************************************************************************/

#include "sound/discrete.h"


/*----------- defined in audio/avalnche.c -----------*/

extern discrete_sound_block avalnche_discrete_interface[];
WRITE8_HANDLER( avalnche_noise_amplitude_w );
WRITE8_HANDLER( avalnche_attract_enable_w );
WRITE8_HANDLER( avalnche_audio_w );
