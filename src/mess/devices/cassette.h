/*********************************************************************

	cassette.h

	MESS interface to the cassette image abstraction code

*********************************************************************/

#ifndef CASSETTE_H
#define CASSETTE_H

#include "device.h"
#include "image.h"
#include "formats/cassimg.h"


enum
{
	MESS_DEVINFO_INT_CASSETTE_DEFAULT_STATE = MESS_DEVINFO_INT_DEV_SPECIFIC,

	MESS_DEVINFO_PTR_CASSETTE_FORMATS = MESS_DEVINFO_PTR_DEV_SPECIFIC,
	MESS_DEVINFO_PTR_CASSETTE_OPTIONS
};

typedef enum
{
	/* this part of the state is controlled by the UI */
	CASSETTE_STOPPED			= 0,
	CASSETTE_PLAY				= 1,
	CASSETTE_RECORD				= 2,

	/* this part of the state is controlled by drivers */
	CASSETTE_MOTOR_ENABLED		= 0,
	CASSETTE_MOTOR_DISABLED		= 4,
	CASSETTE_SPEAKER_ENABLED	= 0,
	CASSETTE_SPEAKER_MUTED		= 8,

	/* masks */
	CASSETTE_MASK_UISTATE		= 3,
	CASSETTE_MASK_MOTOR			= 4,
	CASSETTE_MASK_SPEAKER		= 8,
	CASSETTE_MASK_DRVSTATE		= 12
} cassette_state;

/* cassette prototypes */
cassette_state cassette_get_state(const device_config *cassette);
void cassette_set_state(const device_config *cassette, cassette_state state);
void cassette_change_state(const device_config *cassette, cassette_state state, cassette_state mask);

double cassette_input(const device_config *cassette);
void cassette_output(const device_config *cassette, double value);

cassette_image *cassette_get_image(const device_config *cassette);
double cassette_get_position(const device_config *cassette);
double cassette_get_length(const device_config *cassette);
void cassette_seek(const device_config *cassette, double time, int origin);

/* device specification */
void cassette_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);


#endif /* CASSETTE_H */
