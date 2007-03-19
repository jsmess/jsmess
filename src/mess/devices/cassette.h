/*********************************************************************

	cassette.h

	MESS interface to the cassette image abstraction code

*********************************************************************/

#ifndef CASSETTE_H
#define CASSETTE_H

#include "formats/cassimg.h"
#include "image.h"

enum
{
	DEVINFO_INT_CASSETTE_DEFAULT_STATE = DEVINFO_INT_DEV_SPECIFIC,

	DEVINFO_PTR_CASSETTE_FORMATS = DEVINFO_PTR_DEV_SPECIFIC,
	DEVINFO_PTR_CASSETTE_OPTIONS
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
cassette_state cassette_get_state(mess_image *cassette);
void cassette_set_state(mess_image *cassette, cassette_state state);
void cassette_change_state(mess_image *cassette, cassette_state state, cassette_state mask);

double cassette_input(mess_image *cassette);
void cassette_output(mess_image *cassette, double value);

cassette_image *cassette_get_image(mess_image *cassette);
double cassette_get_position(mess_image *cassette);
double cassette_get_length(mess_image *cassette);
void cassette_seek(mess_image *cassette, double time, int origin);

/* device specification */
void cassette_device_getinfo(const device_class *devclass, UINT32 state, union devinfo *info);

#endif /* CASSETTE_H */
