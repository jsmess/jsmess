#ifndef COLECO_H
#define COLECO_H

#include "driver.h"

/* machine/coleco.c */
int coleco_cart_verify(const UINT8 *buf, size_t size);

 READ8_HANDLER  ( coleco_paddle_r );
WRITE8_HANDLER ( coleco_paddle_toggle_off );
WRITE8_HANDLER ( coleco_paddle_toggle_on );
 READ8_HANDLER ( coleco_mem_r );
WRITE8_HANDLER ( coleco_mem_w );
 READ8_HANDLER ( coleco_video_r );
WRITE8_HANDLER ( coleco_video_w );

#endif /* COLECO_H */
