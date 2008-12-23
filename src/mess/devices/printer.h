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

#define PRINTER	DEVICE_GET_INFO_NAME(printer)


#define MDRV_PRINTER_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, PRINTER, 0) \


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* checks to see if a printer is ready */
int printer_is_ready(const device_config *printer);

/* outputs data to a printer */
void printer_output(const device_config *printer, UINT8 data);

/* device getinfo function */
DEVICE_GET_INFO(printer);

#endif /* __PRINTER_H__ */
