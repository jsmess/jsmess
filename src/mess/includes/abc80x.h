/*****************************************************************************
 *
 * includes/abc80x.h
 *
 ****************************************************************************/

#ifndef ABC80X_H_
#define ABC80X_H_


#define ABC800_X01 12000000.0


/*----------- defined in video/abc80x.c -----------*/

WRITE8_HANDLER( abc800_videoram_w );
PALETTE_INIT( abc800m );
PALETTE_INIT( abc800c );
VIDEO_START( abc800m );
VIDEO_START( abc800c );
VIDEO_UPDATE( abc800 );
VIDEO_START( abc802 );
VIDEO_UPDATE( abc802 );

void abc802_set_columns(int columns);


#endif /* ABC80X_H_ */
