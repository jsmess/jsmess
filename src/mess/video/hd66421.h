/*

	Hitachi HD66421 LCD Controller/Driver

	(c) 2001-2007 Tim Schuerewegen

*/

#ifndef _HD66421_H_
#define _HD66421_H_

#include "driver.h"

#define HD66421_WIDTH   160
#define HD66421_HEIGHT  100

UINT8 hd66421_reg_idx_r( void);
void hd66421_reg_idx_w( UINT8 data);

UINT8 hd66421_reg_dat_r( void);
void hd66421_reg_dat_w( UINT8 data);

PALETTE_INIT( hd66421 );

VIDEO_START( hd66421 );
VIDEO_UPDATE( hd66421 );

#endif
