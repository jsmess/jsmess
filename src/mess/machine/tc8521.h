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

#define MDRV_TC8521_ADD(_tag, _intrf) \
	MDRV_DEVICE_ADD(_tag, TC8521, 0) \
	MDRV_DEVICE_CONFIG(_intrf)

#define MDRV_TC8521_REMOVE(_tag) \
	MDRV_DEVICE_REMOVE(_tag)



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _tc8521_interface tc8521_interface;
struct _tc8521_interface
{
	/* output of alarm */
	void (*alarm_output_callback)(running_device *device, int);
};



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

extern const tc8521_interface default_tc8521_interface;

DEVICE_GET_INFO(tc8521);

READ8_DEVICE_HANDLER(tc8521_r);
WRITE8_DEVICE_HANDLER(tc8521_w);

void tc8521_load_stream(running_device *device, mame_file *file);
void tc8521_save_stream(running_device *device, mame_file *file);


#endif /* __TC8521_H__ */
