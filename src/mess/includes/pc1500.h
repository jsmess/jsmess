#ifndef __PC1500_H_
#define __PC1500_H_

#include "driver.h"
#include "osdepend.h"

UINT8 pc1500_in(void);
WRITE8_HANDLER(lh5811_w);
 READ8_HANDLER(lh5811_r);


/* in mess/vidhrdw/ */
extern unsigned char pc1500_palette[242][3];
extern unsigned short pc1500_colortable[1][2];


void pc1500_init_colors (unsigned char *sys_palette,
						 unsigned short *sys_colortable,
						 const unsigned char *color_prom);
int pc1500_vh_start(void);
void pc1500_vh_stop(void);
void pc1500_vh_screenrefresh (mame_bitmap *bitmap, int full_refresh);
void pc1500a_vh_screenrefresh (mame_bitmap *bitmap, int full_refresh);
void trs80pc2_vh_screenrefresh (mame_bitmap *bitmap, int full_refresh);
//void pc1600_vh_screenrefresh (mame_bitmap *bitmap, int full_refresh);


// on key directly connected to bf1 and +5v
#define KEY_DEF (readinputport(0)&1)
#define KEY_CALLSIGN (readinputport(0)&2)
#define KEY_QUOTE (readinputport(0)&4)
#define KEY_NUMBER (readinputport(0)&8)
#define KEY_STRING (readinputport(0)&0x10)
#define KEY_PERCENT (readinputport(0)&0x20)
#define KEY_AMBERSAND (readinputport(0)&0x40)
#define KEY_SHIFT (readinputport(0)&0x80)
#define KEY_OFF (readinputport(0)&0x100)
#define KEY_ON (readinputport(0)&0x200)

#define KEY_Q (readinputport(1)&1)
#define KEY_W (readinputport(1)&2)
#define KEY_E (readinputport(1)&4)
#define KEY_R (readinputport(1)&8)
#define KEY_T (readinputport(1)&0x10)
#define KEY_Y (readinputport(1)&0x20)
#define KEY_U (readinputport(1)&0x40)
#define KEY_I (readinputport(1)&0x80)
#define KEY_O (readinputport(1)&0x100)
#define KEY_P (readinputport(1)&0x200)
#define KEY_7 (readinputport(1)&0x400)
#define KEY_8 (readinputport(1)&0x800)
#define KEY_9 (readinputport(1)&0x1000)
#define KEY_SLASH (readinputport(1)&0x2000)
#define KEY_CL (readinputport(1)&0x4000)

#define KEY_A (readinputport(2)&0x0001)
#define KEY_S (readinputport(2)&0x0002)
#define KEY_D (readinputport(2)&0x0004)
#define KEY_F (readinputport(2)&0x0008)
#define KEY_G (readinputport(2)&0x0010)
#define KEY_H (readinputport(2)&0x0020)
#define KEY_J (readinputport(2)&0x0040)
#define KEY_K (readinputport(2)&0x0080)
#define KEY_L (readinputport(2)&0x0100)
#define KEY_4 (readinputport(2)&0x0200)
#define KEY_5 (readinputport(2)&0x0400)
#define KEY_6 (readinputport(2)&0x0800)
#define KEY_ASTERIX (readinputport(2)&0x1000)
#define KEY_MODE (readinputport(2)&0x2000)

#define KEY_Z (readinputport(3)&0x0001)
#define KEY_X (readinputport(3)&0x0002)
#define KEY_C (readinputport(3)&0x0004)
#define KEY_V (readinputport(3)&0x0008)
#define KEY_B (readinputport(3)&0x0010)
#define KEY_N (readinputport(3)&0x0020)
#define KEY_M (readinputport(3)&0x0040)
#define KEY_OPENBRACE (readinputport(3)&0x0080)
#define KEY_CLOSEBRACE (readinputport(3)&0x0100)
#define KEY_1 (readinputport(3)&0x0200)
#define KEY_2 (readinputport(3)&0x0400)
#define KEY_3 (readinputport(3)&0x0800)
#define KEY_MINUS (readinputport(3)&0x1000)
#define KEY_LEFT (readinputport(3)&0x2000)

#define KEY_SML (readinputport(4)&0x0001)
#define KEY_RESERVE (readinputport(4)&0x0002)
#define KEY_RCL (readinputport(4)&0x0004)
#define KEY_SPACE (readinputport(4)&0x0008)
#define KEY_DOWN (readinputport(4)&0x0010)
#define KEY_UP (readinputport(4)&0x0020)
#define KEY_ENTER (readinputport(4)&0x0040)
#define KEY_0 (readinputport(4)&0x0080)
#define KEY_POINT (readinputport(4)&0x0100)
#define KEY_EQUALS (readinputport(4)&0x0200)
#define KEY_PLUS (readinputport(4)&0x0400)
#define KEY_RIGHT (readinputport(4)&0x0800)

#endif
