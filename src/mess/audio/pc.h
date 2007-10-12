/***************************************************************************

	audio/pc.h

	Functions to emulate a PC PIC timer 2 used for simple tone generation

	TODO:
	Add support for the SN76496 used in the Tandy1000/PCjr
	It most probably is on port 0xc0, but I'm not sure...

****************************************************************************/

#ifndef __AUDIO_PC_H__
#define __AUDIO_PC_H__

#include "driver.h"

extern const struct CustomSound_interface pc_sound_interface;
void pc_sh_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int samples);
void pc_sh_speaker(int mode);

void pc_sh_speaker_change_clock(double pc_clock);

#endif /* __AUDIO_PC_H__ */
