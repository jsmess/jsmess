/*********************************************************************

	tms5501.h

	TMS 5501 code

*********************************************************************/

#ifndef __TMS5501_H__
#define __TMS5501_H__


/***************************************************************************
    MACROS
***************************************************************************/

#define TMS5501			DEVICE_GET_INFO_NAME(tms5501)



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/* TMS5501 timer and interrupt controler */
typedef struct _tms5501_interface tms5501_interface;
struct _tms5501_interface
{
	UINT8 (*pio_read_callback)(const device_config *device);
	void (*pio_write_callback)(const device_config *device, UINT8);
	void (*interrupt_callback)(const device_config *device, int intreq, UINT8 vector);
	double clock_rate;
};



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

DEVICE_GET_INFO(tms5501);

READ8_DEVICE_HANDLER( tms5501_r );
WRITE8_DEVICE_HANDLER( tms5501_w );

void tms5501_set_pio_bit_7 (const device_config *device, UINT8 data);
void tms5501_sensor (const device_config *device, UINT8 data);

#endif /* __TMS5501_H__ */
