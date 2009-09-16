/*****************************************************************************
 *
 * machine/i8271.h
 *
 ****************************************************************************/

#ifndef I8271_H_
#define I8271_H_

/***************************************************************************
    MACROS
***************************************************************************/

#define I8271		DEVICE_GET_INFO_NAME(i8271)

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct i8271_interface
{
	void (*interrupt)(const device_config *device, int state);
	void (*dma_request)(const device_config *device, int state, int read_);
	const char *floppy_drive_tags[2];
} i8271_interface;

/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( i8271 );

READ8_DEVICE_HANDLER (i8271_r);
WRITE8_DEVICE_HANDLER(i8271_w);

READ8_DEVICE_HANDLER (i8271_dack_r);
WRITE8_DEVICE_HANDLER(i8271_dack_w);

READ8_DEVICE_HANDLER (i8271_data_r);
WRITE8_DEVICE_HANDLER(i8271_data_w);


/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MDRV_I8271_ADD(_tag, _intrf) \
	MDRV_DEVICE_ADD(_tag, I8271, 0) \
	MDRV_DEVICE_CONFIG(_intrf)

#endif /* I8271_H_ */
