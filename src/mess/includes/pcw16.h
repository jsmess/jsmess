/*****************************************************************************
 *
 * includes/pcw16.h
 *
 ****************************************************************************/

#ifndef PCW16_H_
#define PCW16_H_


#define PCW16_BORDER_HEIGHT 8
#define PCW16_BORDER_WIDTH 8
#define PCW16_NUM_COLOURS 32
#define PCW16_DISPLAY_WIDTH 640
#define PCW16_DISPLAY_HEIGHT 480

#define PCW16_SCREEN_WIDTH	(PCW16_DISPLAY_WIDTH + (PCW16_BORDER_WIDTH<<1))
#define PCW16_SCREEN_HEIGHT	(PCW16_DISPLAY_HEIGHT  + (PCW16_BORDER_HEIGHT<<1))


/*----------- defined in video/pcw16.c -----------*/

extern PALETTE_INIT( pcw16 );
extern VIDEO_START( pcw16 );
extern VIDEO_UPDATE( pcw16 );


#endif /* PCW16_H_ */
