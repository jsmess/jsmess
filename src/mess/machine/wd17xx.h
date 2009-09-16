/*********************************************************************

    wd17xx.h

    Implementations of the Western Digital 17xx and 19xx families of
    floppy disk controllers

*********************************************************************/

#ifndef WD179X_H
#define WD179X_H

#include "devices/flopdrv.h"

/***************************************************************************
    MACROS
***************************************************************************/

#define WD1770		DEVICE_GET_INFO_NAME(wd1770)
#define WD1771		DEVICE_GET_INFO_NAME(wd1771)
#define WD1772		DEVICE_GET_INFO_NAME(wd1772)
#define WD1773		DEVICE_GET_INFO_NAME(wd1773)
#define WD179X		DEVICE_GET_INFO_NAME(wd179x)
#define WD1793		DEVICE_GET_INFO_NAME(wd1793)
#define WD2793		DEVICE_GET_INFO_NAME(wd2793)
#define WD177X		DEVICE_GET_INFO_NAME(wd177x)
#define MB8877		DEVICE_GET_INFO_NAME(mb8877)

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef enum
{
	WD17XX_IRQ_CLR,
	WD17XX_IRQ_SET,
	WD17XX_DRQ_CLR,
	WD17XX_DRQ_SET
} wd17xx_state_t;


typedef void (*wd17xx_callback_func)(const device_config *device, wd17xx_state_t state, void *param);
#define WD17XX_CALLBACK(name)	void name(const device_config *device, wd17xx_state_t state, void *param )

typedef void* (*wd17xx_param_callback_func)(const device_config *device);
#define WD17XX_PARAM_CALLBACK(name)	void* name(const device_config *device)

/* Interface */
typedef struct _wd17xx_interface wd17xx_interface;
struct _wd17xx_interface
{
	wd17xx_callback_func callback;
	wd17xx_param_callback_func callback_param;
	const char *floppy_drive_tags[4];
};

/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* device interface */
extern DEVICE_GET_INFO(wd1770);
extern DEVICE_GET_INFO(wd1771);
extern DEVICE_GET_INFO(wd1772);
extern DEVICE_GET_INFO(wd1773);
extern DEVICE_GET_INFO(wd179x);
extern DEVICE_GET_INFO(wd1793);
extern DEVICE_GET_INFO(wd2793);
extern DEVICE_GET_INFO(wd177x);
extern DEVICE_GET_INFO(mb8877);

void wd17xx_reset(const device_config *device);

/* the following are not strictly part of the wd179x hardware/emulation
but will be put here for now until the flopdrv code has been finalised more */
void wd17xx_set_drive(const device_config *device, UINT8);		/* set drive wd179x is accessing */
void wd17xx_set_side(const device_config *device, UINT8);		/* set side wd179x is accessing */
void wd17xx_set_density(const device_config *device, DENSITY);	/* set density */

READ8_DEVICE_HANDLER( wd17xx_status_r );
READ8_DEVICE_HANDLER( wd17xx_track_r );
READ8_DEVICE_HANDLER( wd17xx_sector_r );
READ8_DEVICE_HANDLER( wd17xx_data_r );

WRITE8_DEVICE_HANDLER( wd17xx_command_w );
WRITE8_DEVICE_HANDLER( wd17xx_track_w );
WRITE8_DEVICE_HANDLER( wd17xx_sector_w );
WRITE8_DEVICE_HANDLER( wd17xx_data_w );

READ8_DEVICE_HANDLER( wd17xx_r );
WRITE8_DEVICE_HANDLER( wd17xx_w );

void wd17xx_set_pause_time(const device_config *device, int usec); /* default is 40 usec if not set */

extern const wd17xx_interface default_wd17xx_interface;
extern const wd17xx_interface default_wd17xx_interface_2_drives;
/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MDRV_WD1770_ADD(_tag, _intrf) \
	MDRV_DEVICE_ADD(_tag, WD1770, 0) \
	MDRV_DEVICE_CONFIG(_intrf)

#define MDRV_WD1771_ADD(_tag, _intrf) \
	MDRV_DEVICE_ADD(_tag, WD1771, 0) \
	MDRV_DEVICE_CONFIG(_intrf)

#define MDRV_WD1772_ADD(_tag, _intrf) \
	MDRV_DEVICE_ADD(_tag, WD1772, 0) \
	MDRV_DEVICE_CONFIG(_intrf)

#define MDRV_WD1773_ADD(_tag, _intrf) \
	MDRV_DEVICE_ADD(_tag, WD1773, 0) \
	MDRV_DEVICE_CONFIG(_intrf)

#define MDRV_WD179X_ADD(_tag, _intrf) \
	MDRV_DEVICE_ADD(_tag, WD179X, 0) \
	MDRV_DEVICE_CONFIG(_intrf)

#define MDRV_WD1793_ADD(_tag, _intrf) \
	MDRV_DEVICE_ADD(_tag, WD1793, 0) \
	MDRV_DEVICE_CONFIG(_intrf)

#define MDRV_WD2793_ADD(_tag, _intrf) \
	MDRV_DEVICE_ADD(_tag, WD2793, 0) \
	MDRV_DEVICE_CONFIG(_intrf)

#define MDRV_WD177X_ADD(_tag, _intrf) \
	MDRV_DEVICE_ADD(_tag, WD177X, 0) \
	MDRV_DEVICE_CONFIG(_intrf)

#define MDRV_MB8877_ADD(_tag, _intrf) \
	MDRV_DEVICE_ADD(_tag, MB8877, 0) \
	MDRV_DEVICE_CONFIG(_intrf)


#endif /* WD179X_H */



