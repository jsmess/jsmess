/*
    smartmed.h: header file for smartmed.c
*/

#ifndef __SMARTMEDIA_H__
#define __SMARTMEDIA_H__


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

int smartmedia_present(device_t *device);
int smartmedia_protected(device_t *device);

UINT8 smartmedia_data_r(device_t *device);
void smartmedia_command_w(device_t *device, UINT8 data);
void smartmedia_address_w(device_t *device, UINT8 data);
void smartmedia_data_w(device_t *device, UINT8 data);

/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

DECLARE_LEGACY_IMAGE_DEVICE(SMARTMEDIA, smartmedia);

#define MCFG_SMARTMEDIA_ADD(_tag) \
	MCFG_DEVICE_ADD(_tag, SMARTMEDIA, 0)

#endif /* __SMARTMEDIA_H__ */
