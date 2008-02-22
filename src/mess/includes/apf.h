/*****************************************************************************
 *
 * includes/apf.h
 *
 ****************************************************************************/

#ifndef APF_H_
#define APF_H_


/*----------- defined in video/apf.c -----------*/

VIDEO_START(apf);
extern UINT8 apf_m6847_attr;


/*----------- defined in drivers/apf.c -----------*/

void apf_update_ints(void);

/*----------- defined in machine/apf.c -----------*/

DEVICE_LOAD( apfimag_floppy );


#endif /* APF_H_ */
