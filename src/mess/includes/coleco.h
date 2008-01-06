/*****************************************************************************
 *
 * includes/coleco.h
 *
 ****************************************************************************/

#ifndef COLECO_H_
#define COLECO_H_


/*----------- defined in machine/coleco.c -----------*/

int coleco_cart_verify(const UINT8 *buf, size_t size);

 READ8_HANDLER  ( coleco_paddle_r );
WRITE8_HANDLER ( coleco_paddle_toggle_off );
WRITE8_HANDLER ( coleco_paddle_toggle_on );


/*----------- defined in drivers/coleco.c -----------*/

 READ8_HANDLER ( coleco_video_r );
WRITE8_HANDLER ( coleco_video_w );


#endif /* COLECO_H_ */
