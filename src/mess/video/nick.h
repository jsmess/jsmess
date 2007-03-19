#ifndef __NICK_GRAPHICS_CHIP_MESS_INCLUDED__
#define __NICK_GRAPHICS_CHIP_MESS_INCLUDED__

/**************************************
NICK GRAPHICS CHIP
***************************************/

#include "driver.h"

extern int Nick_vh_start(void);
extern void Nick_DoScreen(mame_bitmap *bm);

extern int Nick_reg_r(int);
extern void Nick_reg_w(int, int);

#endif
