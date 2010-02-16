/*****************************************************************************
 *
 * machine/upd7002.h
 *
 * uPD7002 Analogue to Digital Converter
 *
 * Driver by Gordon Jefferyes <mess_bbc@gjeffery.dircon.co.uk>
 *
 ****************************************************************************/

#ifndef UPD7002_H_
#define UPD7002_H_

/***************************************************************************
    MACROS
***************************************************************************/

#define UPD7002		DEVICE_GET_INFO_NAME(uPD7002)

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef int (*uPD7002_get_analogue_func)(running_device *device, int channel_number);
#define UPD7002_GET_ANALOGUE(name)	int name(running_device *device, int channel_number )

typedef void (*uPD7002_eoc_func)(running_device *device, int data);
#define UPD7002_EOC(name)	void name(running_device *device, int data )


typedef struct _uPD7002_interface uPD7002_interface;
struct _uPD7002_interface
{
	uPD7002_get_analogue_func get_analogue_func;
	uPD7002_eoc_func		  EOC_func;
};

/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( uPD7002 );

/* Standard handlers */

READ8_DEVICE_HANDLER ( uPD7002_EOC_r );
READ8_DEVICE_HANDLER ( uPD7002_r );
WRITE8_DEVICE_HANDLER ( uPD7002_w );


/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MDRV_UPD7002_ADD(_tag, _intrf) \
	MDRV_DEVICE_ADD(_tag, UPD7002, 0) \
	MDRV_DEVICE_CONFIG(_intrf)


#endif /* UPD7002_H_ */
