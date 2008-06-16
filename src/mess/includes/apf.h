/*****************************************************************************
 *
 * includes/apf.h
 *
 ****************************************************************************/

#ifndef APF_H_
#define APF_H_


/*----------- defined in video/apf.c -----------*/

extern unsigned char *apf_video_ram;

extern unsigned char apf_ints;

VIDEO_START(apf);
extern UINT8 apf_m6847_attr;


/*----------- defined in drivers/apf.c -----------*/

void apf_update_ints(running_machine *machine);

/*----------- defined in machine/apf.c -----------*/

DEVICE_IMAGE_LOAD( apfimag_floppy );


#endif /* APF_H_ */
