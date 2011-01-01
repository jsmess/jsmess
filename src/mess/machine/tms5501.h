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
	UINT8 (*pio_read_callback)(device_t *device);
	void (*pio_write_callback)(device_t *device, UINT8);
	void (*interrupt_callback)(device_t *device, int intreq, UINT8 vector);
	double clock_rate;
};



/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MCFG_TMS5501_ADD(_tag, _intrf) \
	MCFG_DEVICE_ADD(_tag, TMS5501, 0) \
	MCFG_DEVICE_CONFIG(_intrf)



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

READ8_DEVICE_HANDLER( tms5501_r );
WRITE8_DEVICE_HANDLER( tms5501_w );

void tms5501_set_pio_bit_7 (device_t *device, UINT8 data);
void tms5501_sensor (device_t *device, UINT8 data);

#endif /* __TMS5501_H__ */
