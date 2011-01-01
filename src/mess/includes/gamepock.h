#ifndef _GAMEPOCK_H_
#define _GAMEPOCK_H_

typedef struct {
	UINT8	enabled;
	UINT8	start_page;
	UINT8	address;
	UINT8	y_inc;
	UINT8	ram[256];	/* There are actually 50 x 4 x 8 bits. This just makes addressing easier. */
} HD44102CH;

class gamepock_state : public driver_device
{
public:
	gamepock_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 port_a;
	UINT8 port_b;
	HD44102CH hd44102ch[3];
};


/*----------- defined in machine/gamepock.c -----------*/

MACHINE_RESET( gamepock );

VIDEO_UPDATE( gamepock );

WRITE8_HANDLER( gamepock_port_a_w );
WRITE8_HANDLER( gamepock_port_b_w );

READ8_HANDLER( gamepock_port_b_r );
READ8_HANDLER( gamepock_port_c_r );

int gamepock_io_callback( device_t *device, int ioline, int state );

#endif
