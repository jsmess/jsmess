/*********************************************************************

    tms5501.h

    TMS 5501 code

*********************************************************************/

#ifndef __TMS5501_H__
#define __TMS5501_H__


/***************************************************************************
    MACROS
***************************************************************************/

DECLARE_LEGACY_DEVICE(TMS5501, tms5501);



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/* TMS5501 timer and interrupt controller */
typedef struct _tms5501_interface tms5501_interface;
struct _tms5501_interface
{
	UINT8 (*pio_read_callback)(running_device *device);
	void (*pio_write_callback)(running_device *device, UINT8);
	void (*interrupt_callback)(running_device *device, int intreq, UINT8 vector);
	double clock_rate;
};



/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MDRV_TMS5501_ADD(_tag, _intrf) \
	MDRV_DEVICE_ADD(_tag, TMS5501, 0) \
	MDRV_DEVICE_CONFIG(_intrf)



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

READ8_DEVICE_HANDLER( tms5501_r );
WRITE8_DEVICE_HANDLER( tms5501_w );

void tms5501_set_pio_bit_7 (running_device *device, UINT8 data);
void tms5501_sensor (running_device *device, UINT8 data);

#endif /* __TMS5501_H__ */
