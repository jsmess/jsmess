/***************************************************************************

  MIOT 6530 emulation

***************************************************************************/

#ifndef __MIOT6530_H__
#define __MIOT6530_H__


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef UINT8 (*miot_read_func)(const device_config *device, UINT8 olddata);
typedef void (*miot_write_func)(const device_config *device, UINT8 newdata, UINT8 olddata);


typedef struct _miot6530_interface miot6530_interface;
struct _miot6530_interface
{
	miot_read_func		in_a_func;
	miot_read_func		in_b_func;
	miot_write_func		out_a_func;
	miot_write_func		out_b_func;
};



/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MDRV_MIOT6530_ADD(_tag, _clock, _intrf) \
	MDRV_DEVICE_ADD(_tag, MIOT6530, _clock) \
	MDRV_DEVICE_CONFIG(_intrf)



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

READ8_DEVICE_HANDLER( miot6530_r );
WRITE8_DEVICE_HANDLER( miot6530_w );

void miot6530_porta_in_set(const device_config *device, UINT8 data, UINT8 mask);
void miot6530_portb_in_set(const device_config *device, UINT8 data, UINT8 mask);

UINT8 miot6530_porta_in_get(const device_config *device);
UINT8 miot6530_portb_in_get(const device_config *device);

UINT8 miot6530_porta_out_get(const device_config *device);
UINT8 miot6530_portb_out_get(const device_config *device);


/* ----- device interface ----- */

#define MIOT6530 DEVICE_GET_INFO_NAME(miot6530)
DEVICE_GET_INFO( miot6530 );

#endif
