/*****************************************************************************
 *
 * includes/apf.h
 *
 ****************************************************************************/

#ifndef APF_H_
#define APF_H_


/*----------- defined in video/apf.c -----------*/

extern unsigned char *apf_video_ram;

VIDEO_START(apf);
extern UINT8 apf_m6847_attr;


/*----------- defined in drivers/apf.c -----------*/

extern unsigned char apf_ints;

void apf_update_ints(running_machine *machine);

#endif /* APF_H_ */
