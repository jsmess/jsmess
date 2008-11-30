/*********************************************************************

	tc8521.h

	Toshiba TC8251 Real Time Clock code

*********************************************************************/

#ifndef __TC8521_H__
#define __TC8525_H__


/***************************************************************************
    MACROS
***************************************************************************/

#define TC8521			DEVICE_GET_INFO_NAME(tc8521)



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _tc8521_interface tc8521_interface;
struct _tc8521_interface
{
	/* output of alarm */
	void (*alarm_output_callback)(const device_config *device, int);
};



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

DEVICE_GET_INFO(tc8521);

READ8_DEVICE_HANDLER(tc8521_r);
WRITE8_DEVICE_HANDLER(tc8521_w);

void tc8521_load_stream(const device_config *device, mame_file *file);
void tc8521_save_stream(const device_config *device, mame_file *file);


#endif /* __TC8521_H__ */
