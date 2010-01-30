/**********************************************************************

    Motorola 6883 SAM interface and emulation

    This function emulates all the functionality of one M6883
    synchronous address multiplexer.

    Note that the real SAM chip was intimately involved in things like
    memory and video addressing, which are things that the MAME core
    largely handles.  Thus, this code only takes care of a small part
    of the SAM's actual functionality; it simply tracks the SAM
    registers and handles things like save states.  It then delegates
    the bulk of the responsibilities back to the host.

**********************************************************************/

#ifndef __6833SAM_H__
#define __6833SAM_H__

#include "emu.h"
#include "devcb.h"


/***************************************************************************
    MACROS
***************************************************************************/

#define SAM6883			DEVICE_GET_INFO_NAME(sam6883)
#define SAM6883_GIME	DEVICE_GET_INFO_NAME(sam6883_gime)

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/
typedef UINT8 *(*sam6883_get_rambase_func)(running_device *device);
#define SAM6883_GET_RAMBASE(name)	UINT8 *name(running_device *device )

typedef void (*sam6883_set_pageonemode_func)(running_device *device, int val);
#define SAM6883_SET_PAGE_ONE_MODE(name)	void name(running_device *device, int val )

typedef void (*sam6883_set_mpurate_func)(running_device *device, int val);
#define SAM6883_SET_MPU_RATE(name)	void name(running_device *device, int val )

typedef void (*sam6883_set_memorysize_func)(running_device *device, int val);
#define SAM6883_SET_MEMORY_SIZE(name)	void name(running_device *device, int val )

typedef void (*sam6883_set_map_type_func)(running_device *device, int val);
#define SAM6883_SET_MAP_TYPE(name)	void name(running_device *device, int val )


typedef struct _sam6883_interface sam6883_interface;
struct _sam6883_interface
{
	sam6883_get_rambase_func get_rambase;
	sam6883_set_pageonemode_func set_pageonemode;
	sam6883_set_mpurate_func set_mpurate;
	sam6883_set_memorysize_func set_memorysize;
	sam6883_set_map_type_func set_maptype;
};

/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( sam6883 );
DEVICE_GET_INFO( sam6883_gime );

/* Standard handlers */

/* write to the SAM */
WRITE8_DEVICE_HANDLER(sam6883_w);

/* set the state of the SAM */
void sam6883_set_state(running_device *device,UINT16 state, UINT16 mask);

/* used by video/m6847.c to read the position of the SAM */
const UINT8 *sam6883_videoram(running_device *device,int scanline);

/* used to get memory size and pagemode independent of callbacks */
UINT8 sam6883_memorysize(running_device *device);
UINT8 sam6883_pagemode(running_device *device);
UINT8 sam6883_maptype(running_device *device);

#if 0
WRITE_LINE_DEVICE_HANDLER( sam6883_hs_w );
#endif


/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MDRV_SAM6883_ADD(_tag, _intrf) \
	MDRV_DEVICE_ADD(_tag, SAM6883, 0) \
	MDRV_DEVICE_CONFIG(_intrf)

#define MDRV_SAM6883_GIME_ADD(_tag, _intrf) \
	MDRV_DEVICE_ADD(_tag, SAM6883_GIME, 0) \
	MDRV_DEVICE_CONFIG(_intrf)


#endif /* __6833SAM_H__ */
