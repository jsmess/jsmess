/*****************************************************************************
 *
 * includes/abc80.h
 *
 ****************************************************************************/

#ifndef __ABC80__
#define __ABC80__

#define ABC80_XTAL		11980800.0

#define ABC80_HTOTAL	384
#define ABC80_HBEND		0
#define ABC80_HBSTART	312
#define ABC80_VTOTAL	312
#define ABC80_VBEND		0
#define ABC80_VBSTART	287

#define ABC80_HDSTART	36 // unverified
#define ABC80_VDSTART	23 // unverified

#define ABC80_MODE_TEXT	0x07
#define ABC80_MODE_GFX	0x17

/*----------- defined in video/abc80.c -----------*/

PALETTE_INIT( abc80 );
VIDEO_START( abc80 );
VIDEO_UPDATE( abc80 );


#endif /* ABC80_H_ */
