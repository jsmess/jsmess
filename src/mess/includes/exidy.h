/*****************************************************************************
 *
 * includes/exidy.h
 *
 ****************************************************************************/

#ifndef EXIDY_H_
#define EXIDY_H_

#include "devices/z80bin.h"

/*----------- defined in machine/exidy.c -----------*/

READ8_HANDLER(exidy_fc_r);
READ8_HANDLER(exidy_fd_r);
READ8_HANDLER(exidy_fe_r);
READ8_HANDLER(exidy_ff_r);
WRITE8_HANDLER(exidy_fc_w);
WRITE8_HANDLER(exidy_fd_w);
WRITE8_HANDLER(exidy_fe_w);
WRITE8_HANDLER(exidy_ff_w);
MACHINE_START( exidyd );
MACHINE_START( exidy );
MACHINE_RESET( exidyd );
MACHINE_RESET( exidy );
Z80BIN_EXECUTE( exidy );
void exidy_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);


/*----------- defined in video/exidy.c -----------*/

VIDEO_UPDATE( exidy );


#endif /* EXIDY_H_ */
