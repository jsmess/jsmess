/*****************************************************************************
 *
 * video/zx8301.h
 *
 ****************************************************************************/

#ifndef ZX8301_H_
#define ZX8301_H_

#include "mame.h"
#include "driver.h"


/*----------- defined in video/zx8301.c -----------*/

WRITE8_HANDLER( zx8301_control_w );

PALETTE_INIT( zx8301 );
VIDEO_START( zx8301 );
VIDEO_UPDATE( zx8301 );


#endif /* ZX8301_H_ */
