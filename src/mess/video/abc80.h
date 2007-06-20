#ifndef __ABC80_VIDEO__
#define __ABC80_VIDEO__

#include "driver.h"
#include "video/generic.h"

#define ABC80_XTAL		11980800.0

#define ABC80_HTOTAL	384
#define ABC80_HBEND		0
#define ABC80_HBSTART	312
#define ABC80_VTOTAL	312
#define ABC80_VBEND		0
#define ABC80_VBSTART	287

#define ABC80_HDSTART	36 // unverified
#define ABC80_VDSTART	23 // unverified

#define ABC80_MODE_TEXT	135
#define ABC80_MODE_GFX	151

PALETTE_INIT( abc80 );
VIDEO_START( abc80 );
VIDEO_UPDATE( abc80 );

#endif
