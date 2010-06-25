/****************************************************************************

    printer.h

    Code for handling printer devices

****************************************************************************/

#ifndef __PRINTER_H__
#define __PRINTER_H__

#include "image.h"


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef void (*online_func)(running_device *device, int state);

typedef struct _printer_config printer_config;
struct _printer_config
{
	online_func online;
};


DECLARE_LEGACY_IMAGE_DEVICE(PRINTER, printer);


#define MDRV_PRINTER_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, PRINTER, 0) \

#define MDRV_PRINTER_ONLINE(_online) \
	MDRV_DEVICE_CONFIG_DATAPTR(printer_config, online, _online)


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* checks to see if a printer is ready */
int printer_is_ready(running_device *printer);

/* outputs data to a printer */
void printer_output(running_device *printer, UINT8 data);

#endif /* __PRINTER_H__ */
