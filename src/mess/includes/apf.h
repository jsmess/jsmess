/*****************************************************************************
 *
 * includes/apf.h
 *
 ****************************************************************************/

#ifndef APF_H_
#define APF_H_


/*----------- defined in drivers/apf.c -----------*/

extern unsigned char apf_ints;

void apf_update_ints(running_machine *machine);

#endif /* APF_H_ */
