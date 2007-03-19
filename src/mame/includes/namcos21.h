/**
 * @file namcos21.h
 */

#define NAMCOS21_POLY_FRAME_WIDTH 496
#define NAMCOS21_POLY_FRAME_HEIGHT 480

/*----------- defined in drivers/namcos21.c -----------*/

extern UINT16 *namcos21_dspram16;

/*----------- defined in video/namcos21.c -----------*/

extern void namcos21_ClearPolyFrameBuffer( void );
extern void namcos21_DrawQuad( int sx[4], int sy[4], int zcode[4], int color );

VIDEO_START( namcos21 );
VIDEO_UPDATE( namcos21_default );
VIDEO_UPDATE( namcos21_winrun );

