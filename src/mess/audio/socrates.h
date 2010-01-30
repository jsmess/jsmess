#pragma once

#ifndef __SOCR_SND_H__
#define __SOCR_SND_H__

void socrates_snd_reg0_w(running_device *device, int data);
void socrates_snd_reg1_w(running_device *device, int data);
void socrates_snd_reg2_w(running_device *device, int data);
void socrates_snd_reg3_w(running_device *device, int data);
void socrates_snd_reg4_w(running_device *device, int data);

DEVICE_GET_INFO( socrates_snd );
#define SOUND_SOCRATES DEVICE_GET_INFO_NAME( socrates_snd )

#endif /* __SOCR_SND_H__ */

