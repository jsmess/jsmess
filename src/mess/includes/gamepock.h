#ifndef _GAMEPOCK_H_
#define _GAMEPOCK_H_

/*----------- defined in machine/gamepock.c -----------*/

MACHINE_RESET( gamepock );

VIDEO_UPDATE( gamepock );

WRITE8_HANDLER( gamepock_port_a_w );
WRITE8_HANDLER( gamepock_port_b_w );

READ8_HANDLER( gamepock_port_b_r );
READ8_HANDLER( gamepock_port_c_r );

int gamepock_io_callback( running_device *device, int ioline, int state );

#endif
