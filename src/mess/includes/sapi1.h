/*****************************************************************************
 *
 * includes/sapi1.h
 *
 ****************************************************************************/

#ifndef SAPI_1_H_
#define SAPI_1_H_

class sapi1_state : public driver_device
{
public:
	sapi1_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8* sapi_video_ram;
	UINT8 keyboard_mask;
	UINT8 refresh_counter;
	UINT8 zps3_25;
};


/*----------- defined in machine/sapi1.c -----------*/

extern DRIVER_INIT( sapi1 );
extern DRIVER_INIT( sapizps3 );
extern MACHINE_START( sapi1 );
extern MACHINE_RESET( sapi1 );
extern MACHINE_RESET( sapizps3 );

extern READ8_HANDLER (sapi1_keyboard_r );
extern WRITE8_HANDLER(sapi1_keyboard_w );

/*----------- defined in video/sapi1.c -----------*/

extern VIDEO_START( sapi1 );
extern VIDEO_UPDATE( sapi1 );
extern VIDEO_START( sapizps3 );
extern VIDEO_UPDATE( sapizps3 );

#endif
