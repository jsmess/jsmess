/*********************************************************************

	applefdc.h

	Implementation of various Apple Floppy Disk Controllers, including
	the classic Apple controller and the IWM (Integrated Woz Machine)
	chip

	Nate Woods
	Raphael Nabet

*********************************************************************/

#ifndef __APPLEFDC_H__
#define __APPLEFDC_H__

#include "driver.h"



/***************************************************************************
    CONSTANTS
***************************************************************************/

#define APPLEFDC_PH0	0x01
#define APPLEFDC_PH1	0x02
#define APPLEFDC_PH2	0x04
#define APPLEFDC_PH3	0x08

#define APPLEFDC	DEVICE_GET_INFO_NAME( applefdc )
#define IWM			DEVICE_GET_INFO_NAME( iwm )
#define SWIM		DEVICE_GET_INFO_NAME( swim )



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _applefdc_interface applefdc_interface;
struct _applefdc_interface
{
	void (*set_lines)(UINT8 lines);
	void (*set_enable_lines)(int enable_mask);

	UINT8 (*read_data)(void);
	void (*write_data)(UINT8 data);
	int (*read_status)(void);
};



/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* classic Apple II disk controller (pre-IWM) */
DEVICE_GET_INFO(applefdc);	

/* Integrated Woz Machine */
DEVICE_GET_INFO(iwm);

/* Steve Woz Integrated Machine (NYI) */
DEVICE_GET_INFO(swim);	

/* read/write handlers */
READ8_DEVICE_HANDLER(applefdc_r);
WRITE8_DEVICE_HANDLER(applefdc_w);

/* accessor */
UINT8 applefdc_get_lines(const device_config *device);


#endif /* __APPLEFDC_H__ */
