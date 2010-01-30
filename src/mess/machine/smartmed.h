/*
    smartmed.h: header file for smartmed.c
*/

#ifndef __SMARTMEDIA_H__
#define __SMARTMEDIA_H__


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( smartmedia );

int smartmedia_present(running_device *device);
int smartmedia_protected(running_device *device);

UINT8 smartmedia_data_r(running_device *device);
void smartmedia_command_w(running_device *device, UINT8 data);
void smartmedia_address_w(running_device *device, UINT8 data);
void smartmedia_data_w(running_device *device, UINT8 data);

/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define SMARTMEDIA	DEVICE_GET_INFO_NAME(smartmedia)

#define MDRV_SMARTMEDIA_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, SMARTMEDIA, 0)

#endif /* __SMARTMEDIA_H__ */
