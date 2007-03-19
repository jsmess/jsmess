#ifndef __C65_H_
#define __C65_H_

#include "driver.h"

#include "c64.h"

#define C65_KEY_TAB (input_port_10_word_r(0,0)&0x8000)
#define C65_KEY_CTRL (input_port_10_word_r(0,0)&1)

#define C65_KEY_RIGHT_SHIFT ((input_port_12_r(0)&0x20)\
			 ||C65_KEY_CURSOR_UP||C65_KEY_CURSOR_LEFT)
#define C65_KEY_CURSOR_UP (input_port_12_r(0)&0x10)
#define C65_KEY_SPACE (input_port_12_r(0)&8)
#define C65_KEY_CURSOR_LEFT (input_port_12_r(0)&4)
#define C65_KEY_CURSOR_DOWN ((input_port_12_r(0)&2)||C65_KEY_CURSOR_UP)
#define C65_KEY_CURSOR_RIGHT ((input_port_12_r(0)&1)||C65_KEY_CURSOR_LEFT)

#define C65_KEY_STOP (input_port_13_word_r(0,0)&0x8000)
#define C65_KEY_ESCAPE (input_port_13_word_r(0,0)&0x4000)
#define C65_KEY_ALT (input_port_13_word_r(0,0)&0x2000)
#define C65_KEY_DIN (input_port_13_word_r(0,0)&0x1000)
#define C65_KEY_NOSCRL (input_port_13_word_r(0,0)&0x0800)
#define C65_KEY_F1 (input_port_13_word_r(0,0)&0x0400)
#define C65_KEY_F3 (input_port_13_word_r(0,0)&0x0200)
#define C65_KEY_F5 (input_port_13_word_r(0,0)&0x0100)
#define C65_KEY_F7 (input_port_13_word_r(0,0)&0x0080)
#define C65_KEY_F9 (input_port_13_word_r(0,0)&0x0040)
#define C65_KEY_F11 (input_port_13_word_r(0,0)&0x0020)
#define C65_KEY_F13 (input_port_13_word_r(0,0)&0x0010)
#define C65_KEY_HELP (input_port_13_word_r(0,0)&0x0008)

/*extern UINT8 *c65_memory; */
/*extern UINT8 *c65_basic; */
/*extern UINT8 *c65_kernal; */
extern UINT8 *c65_chargen;
/*extern UINT8 *c65_dos; */
/*extern UINT8 *c65_monitor; */
extern UINT8 *c65_interface;
/*extern UINT8 *c65_graphics; */

void c65_map(int a, int x, int y, int z);
void c65_bankswitch (void);
void c65_colorram_write (int offset, int value);

void c65_driver_init (void);
void c65_driver_alpha1_init (void);
void c65pal_driver_init (void);
MACHINE_START( c65 );

extern UINT8 c65_keyline;
extern UINT8 c65_6511_port;

#endif
