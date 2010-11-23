/*****************************************************************************
 *
 * includes/aquarius.h
 *
 ****************************************************************************/

#ifndef __AQUARIUS__
#define __AQUARIUS__

class aquarius_state : public driver_device
{
public:
	aquarius_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *videoram;
	UINT8 scrambler;
	UINT8 *colorram;
	tilemap_t *tilemap;
};


/*----------- defined in video/aquarius.c -----------*/

WRITE8_HANDLER( aquarius_videoram_w );
WRITE8_HANDLER( aquarius_colorram_w );

PALETTE_INIT( aquarius );
VIDEO_START( aquarius );
VIDEO_UPDATE( aquarius );

#endif /* AQUARIUS_H_ */
