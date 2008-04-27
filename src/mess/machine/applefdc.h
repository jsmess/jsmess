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



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef enum
{
	APPLEFDC_APPLE2,	/* classic Apple II disk controller (pre-IWM) */
	APPLEFDC_IWM,		/* Integrated Woz Machine */
	APPLEFDC_SWIM		/* Steve Woz Integrated Machine (NYI) */
} applefdc_t;


typedef struct _applefdc_interface applefdc_interface;
struct _applefdc_interface
{
	applefdc_t type;
	void (*set_lines)(UINT8 lines);
	void (*set_enable_lines)(int enable_mask);

	UINT8 (*read_data)(void);
	void (*write_data)(UINT8 data);
	int (*read_status)(void);
};



/***************************************************************************
    PROTOTYPES
***************************************************************************/

void applefdc_init(const applefdc_interface *intf);
READ8_HANDLER( applefdc_r );
WRITE8_HANDLER( applefdc_w );
UINT8 applefdc_get_lines(void);


#endif /* __APPLEFDC_H__ */
