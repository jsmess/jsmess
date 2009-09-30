/*********************************************************************

    bitbngr.h

    TRS style "bitbanger" serial port

*********************************************************************/

#ifndef __BITBNGR_H__
#define __BITBNGR_H__

#include "image.h"



/***************************************************************************
    CONSTANTS
***************************************************************************/

#define BITBANGER	DEVICE_GET_INFO_NAME(bitbanger)

#define MDRV_BITBANGER_ADD(_tag, _intrf) \
	MDRV_DEVICE_ADD(_tag, BITBANGER, 0) \
	MDRV_DEVICE_CONFIG(_intrf)


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _bitbanger_config bitbanger_config;
struct _bitbanger_config
{
	/* filter function; returns non-zero if input accepted */
	int (*filter)(const device_config *device, const int *pulses, int total_pulses, int total_duration);
	double pulse_threshhold;			/* the maximum duration pulse that we will consider */
	double pulse_tolerance;				/* deviation tolerance for pulses */
	int minimum_pulses;					/* the minimum amount of pulses before we start analyzing */
	int maximum_pulses;					/* the maximum amount of pulses that we will track */
	int begin_pulse_value;				/* the begining value of a pulse */
	int initial_value;					/* the initial value of the bitbanger line */
};



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* outputs data to a bitbanger port */
void bitbanger_output(const device_config *device, int value);

/* device getinfo function */
DEVICE_GET_INFO(bitbanger);


#endif /* __BITBNGR_H__ */
