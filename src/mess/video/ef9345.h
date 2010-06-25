/*********************************************************************

    ef9345.h

    Thomson EF9345 video controller

*********************************************************************/

#ifndef __EF9345_H__
#define __EF9345_H__


/***************************************************************************
    MACROS
***************************************************************************/

DECLARE_LEGACY_DEVICE(EF9345, ef9345);

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _ef9345_config ef9345_config;
struct _ef9345_config
{
	const char* charset;
	UINT16 width;
	UINT16 height;
};

/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MDRV_EF9345_ADD(_tag, _config) \
	MDRV_DEVICE_ADD(_tag, EF9345, 0) \
	MDRV_DEVICE_CONFIG(_config)

/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

void video_update_ef9345(running_device *device, bitmap_t *bitmap, const rectangle *cliprect);
void ef9345_scanline(running_device *device, UINT16 scanline);
READ8_DEVICE_HANDLER( ef9345_r );
WRITE8_DEVICE_HANDLER( ef9345_w );

#endif /* __EF9345_H__ */
