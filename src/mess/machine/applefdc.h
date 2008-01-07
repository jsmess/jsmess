/*********************************************************************

	applefdc.h

	Implementation of various Apple Floppy Disk Controllers, including
	the classic Apple controller and the IWM (Integrated Woz Machine)
	chip

	Nate Woods
	Raphael Nabet

*********************************************************************/

#ifndef APPLEFDC_H
#define APPLEFDC_H

#include "driver.h"

#define APPLEFDC_PH0	0x01
#define APPLEFDC_PH1	0x02
#define APPLEFDC_PH2	0x04
#define APPLEFDC_PH3	0x08

typedef enum
{
	APPLEFDC_APPLE2,	/* classic Apple II disk controller (pre-IWM) */
	APPLEFDC_IWM,		/* Integrated Woz Machine */
	APPLEFDC_SWIM		/* Steve Woz Integrated Machine (NYI) */
} applefdc_t;


struct applefdc_interface
{
	applefdc_t type;
	void (*set_lines)(UINT8 lines);
	void (*set_enable_lines)(int enable_mask);

	UINT8 (*read_data)(void);
	void (*write_data)(UINT8 data);
	int (*read_status)(void);
};


void applefdc_init(const struct applefdc_interface *intf);
READ8_HANDLER( applefdc_r );
WRITE8_HANDLER( applefdc_w );
UINT8 applefdc_get_lines(void);


#endif /* APPLEFDC_H */
