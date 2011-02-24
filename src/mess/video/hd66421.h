/*

    Hitachi HD66421 LCD Controller/Driver

    (c) 2001-2007 Tim Schuerewegen

*/

#ifndef HD66421_H_
#define HD66421_H_


#define HD66421_WIDTH   160
#define HD66421_HEIGHT  100


/*----------- defined in video/hd66421.c -----------*/

UINT8 hd66421_reg_idx_r(void);
void hd66421_reg_idx_w(UINT8 data);

UINT8 hd66421_reg_dat_r(void);
void hd66421_reg_dat_w(UINT8 data);

PALETTE_INIT( hd66421 );

VIDEO_START( hd66421 );
SCREEN_UPDATE( hd66421 );


#endif /* HD66421_H_ */
