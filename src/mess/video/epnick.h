/*****************************************************************************
 *
 * video/epnick.h
 *
 * Nick Graphics Chip - found in Enterprise
 *
 ****************************************************************************/

#ifndef __EPNICK_H__
#define __EPNICK_H__


/* there are 64us per line, although in reality
   about 50 are visible. */
#define ENTERPRISE_SCREEN_WIDTH (50*16)

/* there are 312 lines per screen, although in reality
   about 35*8 are visible */
#define ENTERPRISE_SCREEN_HEIGHT	(35*8)


#define NICK_PALETTE_SIZE	256


/*----------- defined in video/epnick.c -----------*/

PALETTE_INIT( nick );
VIDEO_START( nick );
VIDEO_UPDATE( nick );

WRITE8_HANDLER( Nick_reg_w );


#endif /* __EPNICK_H__ */
