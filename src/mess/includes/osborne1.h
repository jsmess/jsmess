/*****************************************************************************
 *
 * includes/osborne1.h
 *
 ****************************************************************************/

#ifndef OSBORNE1_H_
#define OSBORNE1_H_

#include "machine/6821pia.h"

/*----------- defined in machine/osborne1.c -----------*/

extern const pia6821_interface osborne1_ieee_pia_config;
extern const pia6821_interface osborne1_video_pia_config;

WRITE8_HANDLER( osborne1_0000_w );
WRITE8_HANDLER( osborne1_1000_w );
READ8_HANDLER( osborne1_2000_r );
WRITE8_HANDLER( osborne1_2000_w );
WRITE8_HANDLER( osborne1_3000_w );
WRITE8_HANDLER( osborne1_videoram_w );

WRITE8_HANDLER( osborne1_bankswitch_w );

DRIVER_INIT( osborne1 );
MACHINE_RESET( osborne1 );

/* Osborne1 specific daisy chain interface */
#define OSBORNE1_DAISY	DEVICE_GET_INFO_NAME( osborne1_daisy )
DEVICE_GET_INFO( osborne1_daisy );

#endif /* OSBORNE1_H_ */
