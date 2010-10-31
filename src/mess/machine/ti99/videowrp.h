/*
    TI-99/4(A) and /8 Video subsystem
	This device actually wraps the naked video chip implementation 
    Michael Zapf, October 2010
*/

#ifndef __TIVIDEO__
#define __TIVIDEO__

#include "video/tms9928a.h"
#include "video/v9938.h"

#define READ8Z_DEVICE_HANDLER(name)		void name(ATTR_UNUSED device_t *device, ATTR_UNUSED offs_t offset, UINT8 *value)

#define TI_TMS991X 1
#define TI_V9938 2

typedef struct _ti99_video_config
{
	TMS9928a_interface			*tmsparam;
	int							chip;
	void 						(*callback)(running_machine *, int);
	
} ti99_video_config;

READ16_DEVICE_HANDLER( ti_tms991x_r16 );
READ8Z_DEVICE_HANDLER( ti8_tms991x_rz );
WRITE16_DEVICE_HANDLER( ti_tms991x_w16 );
WRITE8_DEVICE_HANDLER( ti8_tms991x_w );
READ16_DEVICE_HANDLER( ti_video_rnop );
WRITE16_DEVICE_HANDLER( ti_video_wnop );
READ16_DEVICE_HANDLER( ti_v9938_r16 );
WRITE16_DEVICE_HANDLER( ti_v9938_w16 );
READ8Z_DEVICE_HANDLER( gen_v9938_rz );
WRITE8_DEVICE_HANDLER( gen_v9938_w );

void video_update_mouse( running_device *device, int delta_x, int delta_y, int buttons);

DECLARE_LEGACY_DEVICE( TIVIDEO, ti99_video );

#define MDRV_TI_TMS991x_ADD(_tag, _chip, _rate, _screen, _blank, _tmsparam)		\
	MDRV_DEVICE_ADD(_tag, TIVIDEO, 0)								\
	MDRV_DEVICE_CONFIG_DATAPTR(ti99_video_config, tmsparam, _tmsparam)	\
	MDRV_DEVICE_CONFIG_DATA32(ti99_video_config, chip, TI_TMS991X)		\
	MDRV_FRAGMENT_ADD(_chip)										\
	MDRV_SCREEN_MODIFY(_screen)										\
	MDRV_SCREEN_REFRESH_RATE(_rate)									\
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(_blank))
	
#define MDRV_TI_V9938_ADD(_tag, _rate, _screen, _blank, _x, _y, _int)		\
	MDRV_DEVICE_ADD(_tag, TIVIDEO, 0)								\
	MDRV_DEVICE_CONFIG_DATAPTR(ti99_video_config, tmsparam, NULL)	\
	MDRV_DEVICE_CONFIG_DATAPTR(ti99_video_config, callback, _int)	\
	MDRV_DEVICE_CONFIG_DATA32(ti99_video_config, chip, TI_V9938)		\
	MDRV_SCREEN_ADD(_screen, RASTER)	\
	MDRV_SCREEN_REFRESH_RATE(_rate)									\
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)	\
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(_blank))	\
	MDRV_SCREEN_SIZE(_x, _y)	\
	MDRV_SCREEN_VISIBLE_AREA(0, _x - 1, 0, _y - 1)	\
	MDRV_PALETTE_LENGTH(512)	\
	MDRV_PALETTE_INIT(v9938)	\
	MDRV_VIDEO_UPDATE(generic_bitmapped)
	
#endif /* __TIVIDEO__ */

