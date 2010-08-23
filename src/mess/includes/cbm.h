/*****************************************************************************
 *
 * includes/cbm.h
 *
 ****************************************************************************/

#ifndef CBM_H_
#define CBM_H_

#include "devices/cassette.h"


/* global header file for c16, c64, c65, c128, vc20 */

/*----------- defined in machine/cbm.c -----------*/

/* keyboard lines */
extern UINT8 c64_keyline[10];
void cbm_common_interrupt( running_device *device );

UINT8 cbm_common_cia0_port_a_r( running_device *device, UINT8 output_b );
UINT8 cbm_common_cia0_port_b_r( running_device *device, UINT8 output_a );

/***********************************************

    CBM Datasette Tapes

***********************************************/

extern const cassette_config cbm_cassette_config;


#endif /* CBM_H_ */
