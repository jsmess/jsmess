/***************************************************************************

    OKI MSM6255 Dot Matrix LCD Controller implementation

    Copyright MESS team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#ifndef __MSM6255_VIDEO__
#define __MSM6255_VIDEO__

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define MSM6255		DEVICE_GET_INFO_NAME(msm6255)

#define MDRV_MSM6255_ADD(_tag, _clock, _config) \
	MDRV_DEVICE_ADD(_tag, MSM6255, _clock) \
	MDRV_DEVICE_CONFIG(_config)

#define MDRV_MSM6255_REMOVE(_tag) \
	MDRV_DEVICE_REMOVE(_tag)

#define MSM6255_INTERFACE(_name) \
	const msm6255_interface (_name) =

#define MSM6255_CHAR_RAM_READ(_name) \
	UINT8 _name(running_device *device, UINT16 ma, UINT8 ra)

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef UINT8 (*msm6255_char_ram_read_func)(running_device *device, UINT16 ma, UINT8 ra);

typedef struct _msm6255_interface msm6255_interface;
struct _msm6255_interface
{
	const char *screen_tag;		/* screen we are acting on */

	int character_clock;		/* the character clock of the chip */

	/* ROM/RAM data read function */
	msm6255_char_ram_read_func		char_ram_r;
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( msm6255 );

/* character memory access */
READ8_DEVICE_HANDLER ( msm6255_register_r );
WRITE8_DEVICE_HANDLER ( msm6255_register_w );

/* updates the screen */
void msm6255_update(running_device *device, bitmap_t *bitmap, const rectangle *cliprect);

#endif
