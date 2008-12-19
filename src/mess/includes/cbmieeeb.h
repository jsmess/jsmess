/*****************************************************************************
 *
 * includes/cbmieeeb.h
 *
 ****************************************************************************/

#ifndef CBMIEEEB_H_
#define CBMIEEEB_H_


/*----------- defined in machine/cbmieeeb.c -----------*/

void cbm_ieee_open(void);

void cbm_ieee_dav_w(running_machine *machine, int device, int data);
void cbm_ieee_nrfd_w(running_machine *machine, int device, int data);
void cbm_ieee_ndac_w(running_machine *machine, int device, int data);
void cbm_ieee_atn_w(running_machine *machine, int device, int data);
void cbm_ieee_eoi_w(running_machine *machine, int device, int data);
void cbm_ieee_data_w(running_machine *machine, int device, int data);

int cbm_ieee_srq_r(running_machine *machine);
int cbm_ieee_dav_r(running_machine *machine);
int cbm_ieee_nrfd_r(running_machine *machine);
int cbm_ieee_ndac_r(running_machine *machine);
int cbm_ieee_atn_r(running_machine *machine);
int cbm_ieee_eoi_r(running_machine *machine);
int cbm_ieee_data_r(running_machine *machine);

/* for debugging  */
READ8_HANDLER(cbm_ieee_state);


#endif /* CBMIEEEB_H_ */
