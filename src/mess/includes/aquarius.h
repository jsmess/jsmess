/*****************************************************************************
 *
 * includes/aquarius.h
 *
 ****************************************************************************/

#ifndef __AQUARIUS__
#define __AQUARIUS__

/*----------- defined in video/aquarius.c -----------*/

extern UINT8 *aquarius_colorram;
WRITE8_HANDLER( aquarius_videoram_w );
WRITE8_HANDLER( aquarius_colorram_w );

PALETTE_INIT( aquarius );
VIDEO_START( aquarius );
VIDEO_UPDATE( aquarius );

#endif /* AQUARIUS_H_ */
