/*****************************************************************************
 *
 * includes/mekd2.h
 *
 ****************************************************************************/

#ifndef MEKD2_H_
#define MEKD2_H_


class mekd2_state : public driver_device
{
public:
	mekd2_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *videoram;
};


/*----------- defined in machine/mekd2.c -----------*/

DRIVER_INIT( mekd2 );
DEVICE_IMAGE_LOAD( mekd2_cart );


/*----------- defined in video/mekd2.c -----------*/

PALETTE_INIT( mekd2 );
VIDEO_START( mekd2 );
VIDEO_UPDATE( mekd2 );


#endif /* MEKD2_H_ */
