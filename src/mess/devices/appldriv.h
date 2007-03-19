/*********************************************************************

	appldriv.h
	
	Apple 5.25" floppy drive emulation (to be interfaced with applefdc.c)

*********************************************************************/

#ifndef APPLDRIV_H
#define APPLDRIV_H

#include "image.h"

enum
{
	DEVINFO_INT_APPLE525_SPINFRACT_DIVIDEND = DEVINFO_INT_DEV_SPECIFIC,
	DEVINFO_INT_APPLE525_SPINFRACT_DIVISOR
};

void apple525_device_getinfo(const device_class *devclass, UINT32 state, union devinfo *info);

void apple525_set_lines(UINT8 lines);
void apple525_set_enable_lines(int enable_mask);
void apple525_set_sel_line(int sel);

UINT8 apple525_read_data(void);
void apple525_write_data(UINT8 data);
int apple525_read_status(void);

#endif /* APPLDRIV_H */

