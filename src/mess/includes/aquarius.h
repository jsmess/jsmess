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
	aquarius_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

	UINT8 *m_videoram;
	UINT8 m_scrambler;
	UINT8 *m_colorram;
	tilemap_t *m_tilemap;
};


/*----------- defined in video/aquarius.c -----------*/

WRITE8_HANDLER( aquarius_videoram_w );
WRITE8_HANDLER( aquarius_colorram_w );

PALETTE_INIT( aquarius );
VIDEO_START( aquarius );
SCREEN_UPDATE_IND16( aquarius );

#endif /* AQUARIUS_H_ */
