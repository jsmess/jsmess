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

int smartmedia_present(const device_config *device);
int smartmedia_protected(const device_config *device);

UINT8 smartmedia_data_r(const device_config *device);
void smartmedia_command_w(const device_config *device, UINT8 data);
void smartmedia_address_w(const device_config *device, UINT8 data);
void smartmedia_data_w(const device_config *device, UINT8 data);

/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define SMARTMEDIA	DEVICE_GET_INFO_NAME(smartmedia)

#define MDRV_SMARTMEDIA_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, SMARTMEDIA, 0)

#endif /* __SMARTMEDIA_H__ */
