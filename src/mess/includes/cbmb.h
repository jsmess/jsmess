/***************************************************************************
	commodore b series computer

    peter.trauner@jk.uni-linz.ac.at
***************************************************************************/
#ifndef __CBMB_H_
#define __CBMB_H_

#include "driver.h"

#define C64_DIPS \
     PORT_START /* 13 */ \
     PORT_BIT( 0x800, IP_ACTIVE_HIGH, IPT_BUTTON1) \
     PORT_BIT( 0x400, IP_ACTIVE_HIGH, IPT_BUTTON2) \
     PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_8WAY\
     PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_8WAY\
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP)  PORT_8WAY\
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_8WAY\
	PORT_BIT( 8, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("P2 Button") PORT_CODE(KEYCODE_LALT) PORT_CODE(JOYCODE_BUTTON1 ) PORT_PLAYER(2)\
	PORT_BIT( 4, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("P2 Button 2") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(JOYCODE_BUTTON2 ) PORT_PLAYER(2)\
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_NAME("P2 Left") PORT_CODE(KEYCODE_DEL) PORT_CODE(JOYCODE_X_LEFT_SWITCH) PORT_PLAYER(2) PORT_8WAY\
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_NAME("P2 Right") PORT_CODE(KEYCODE_PGDN) PORT_CODE(JOYCODE_X_RIGHT_SWITCH) PORT_PLAYER(2) PORT_8WAY\
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_NAME("P2 Up") PORT_CODE(KEYCODE_HOME) PORT_CODE(JOYCODE_Y_UP_SWITCH) PORT_PLAYER(2) PORT_8WAY\
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_NAME("P2 Down") PORT_CODE(KEYCODE_END) PORT_CODE(JOYCODE_Y_DOWN_SWITCH) PORT_PLAYER(2) PORT_8WAY\
	PORT_BIT ( 0x7, 0x0,	 IPT_UNUSED )\
	PORT_START /* 14 */ \
	PORT_BIT( 0x100, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Paddle 1 Button") PORT_CODE(KEYCODE_LCONTROL) \
	PORT_BIT(0xff,128,IPT_PADDLE, 30) PORT_SENSITIVITY(20) PORT_KEYDELTA(0) PORT_MINMAX(0,255) PORT_CODE_DEC(KEYCODE_LEFT) PORT_CODE_INC(KEYCODE_RIGHT) PORT_CODE_DEC(JOYCODE_X_LEFT_SWITCH) PORT_CODE_INC(JOYCODE_X_RIGHT_SWITCH) PORT_REVERSE\
	PORT_START /* 15 */ \
	PORT_BIT( 0x100, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("Paddle 2 Button") PORT_CODE(KEYCODE_LALT) \
	PORT_BIT(0xff,128,IPT_PADDLE, 30) PORT_SENSITIVITY(20) PORT_KEYDELTA(0) PORT_MINMAX(0,255) PORT_CODE_DEC(KEYCODE_DOWN) PORT_CODE_INC(KEYCODE_UP) PORT_CODE_DEC(0) PORT_CODE_INC(0) PORT_PLAYER(2) PORT_REVERSE\
	PORT_START /* 16 */ \
	PORT_BIT( 0x100, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("Paddle 3 Button") PORT_CODE(KEYCODE_INSERT) \
	PORT_BIT(0xff,128,IPT_PADDLE, 30) PORT_SENSITIVITY(20) PORT_KEYDELTA(0) PORT_MINMAX(0,255) PORT_CODE_DEC(KEYCODE_HOME) PORT_CODE_INC(KEYCODE_PGUP) PORT_CODE_DEC(0) PORT_CODE_INC(0) PORT_PLAYER(3) PORT_REVERSE\
	PORT_START /* 17 */ \
	PORT_BIT( 0x100, IP_ACTIVE_HIGH, IPT_BUTTON4) PORT_NAME("Paddle 4 Button") PORT_CODE(KEYCODE_DEL) \
	PORT_BIT(0xff,128,IPT_PADDLE, 30) PORT_SENSITIVITY(20) PORT_KEYDELTA(0) PORT_MINMAX(0,255) PORT_CODE_DEC(KEYCODE_END) PORT_CODE_INC(KEYCODE_PGDN) PORT_CODE_DEC(JOYCODE_Y_UP_SWITCH) PORT_CODE_INC(JOYCODE_Y_DOWN_SWITCH) PORT_PLAYER(4) PORT_REVERSE\
	PORT_START /* 18 */ \
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("Lightpen Signal") PORT_CODE(KEYCODE_LCONTROL) \
	PORT_BIT(0x1ff,0,IPT_PADDLE, 30) PORT_SENSITIVITY(2) PORT_KEYDELTA(0) PORT_MINMAX(0,320-1) PORT_CODE_DEC(KEYCODE_LEFT) PORT_CODE_INC(KEYCODE_RIGHT) PORT_CODE_DEC(JOYCODE_X_LEFT_SWITCH) PORT_CODE_INC(JOYCODE_X_RIGHT_SWITCH) PORT_PLAYER(1)\
	PORT_START /* 19 */ \
	PORT_BIT(0xff,0,IPT_PADDLE, 30) PORT_SENSITIVITY(2) PORT_KEYDELTA(0) PORT_MINMAX(0,200-1) PORT_CODE_DEC(KEYCODE_UP) PORT_CODE_INC(KEYCODE_DOWN) PORT_CODE_DEC(JOYCODE_Y_UP_SWITCH) PORT_CODE_INC(JOYCODE_Y_DOWN_SWITCH) PORT_PLAYER(2)\
	PORT_START /* 20 */ \
	PORT_DIPNAME ( 0xe000, 0x2000, "Gameport A")\
	PORT_DIPSETTING(  0, "None" )\
	PORT_DIPSETTING(	0x2000, "Joystick 1" )\
	PORT_DIPSETTING(	0x4000, "Paddles 1, 2" )\
	PORT_DIPSETTING(	0x6000, "Mouse Joystick Emulation/2 Button Joystick" )\
	PORT_DIPSETTING(	0x8000, "Mouse" )\
	PORT_DIPSETTING(	0xa000, "Lightpen" )\
	PORT_DIPNAME ( 0x1000, 0x1000, "Lightpen Draw Pointer")\
	PORT_DIPSETTING(  0, DEF_STR( Off ) )\
	PORT_DIPSETTING(  0x1000, DEF_STR( On ) )\
	PORT_DIPNAME ( 0xe00, 0x200, "Gameport B")\
	PORT_DIPSETTING(  0, "None" )\
	PORT_DIPSETTING(	0x0200, "Joystick 2" )\
	PORT_DIPSETTING(	0x0400, "Paddles 3, 4" )\
	PORT_DIPSETTING(	0x0600, "Mouse Joystick Emulation/2 Button Joystick" )\
	PORT_DIPSETTING(	0x0800, "Mouse" )\
	PORT_BIT( 0x100, IP_ACTIVE_HIGH, 0) PORT_NAME("Swap Gameport 1 and 2") PORT_CODE(KEYCODE_NUMLOCK) PORT_TOGGLE

#define LIGHTPEN ((input_port_20_word_r(0,0)&0xe000)==0xa000)
#define MOUSE1 ((input_port_20_word_r(0,0)&0xe000)==0x8000)
#define JOYSTICK1_2BUTTON ((input_port_20_word_r(0,0)&0xe000)==0x6000)
#define PADDLES12 ((input_port_20_word_r(0,0)&0xe000)==0x4000)
#define JOYSTICK1 ((input_port_20_word_r(0,0)&0xe000)==0x2000)
#define LIGHTPEN_POINTER (LIGHTPEN&&(input_port_20_word_r(0,0)&0x1000))
#define MOUSE2 ((input_port_20_word_r(0,0)&0xe00)==0x800)
#define JOYSTICK2_2BUTTON ((input_port_20_word_r(0,0)&0xe00)==0x600)
#define PADDLES34 ((input_port_20_word_r(0,0)&0xe00)==0x400)
#define JOYSTICK2 ((input_port_20_word_r(0,0)&0xe00)==0x200)
#define JOYSTICK_SWAP		(input_port_20_word_r(0,0)&0x100)

#define PADDLE1_BUTTON	((input_port_14_word_r(0,0)&0x100))
#define PADDLE1_VALUE	(input_port_14_word_r(0,0)&0xff)
#define PADDLE2_BUTTON	((input_port_15_word_r(0,0)&0x100))
#define PADDLE2_VALUE	(input_port_15_word_r(0,0)&0xff)
#define PADDLE3_BUTTON	((input_port_16_word_r(0,0)&0x100))
#define PADDLE3_VALUE	(input_port_16_word_r(0,0)&0xff)
#define PADDLE4_BUTTON	((input_port_17_word_r(0,0)&0x100))
#define PADDLE4_VALUE	(input_port_17_word_r(0,0)&0xff)

#define LIGHTPEN_BUTTON (LIGHTPEN&&(readinputport(18)&0x8000))
#define LIGHTPEN_X_VALUE ((readinputport(18)&0x3ff)&~1)	/* effectiv resolution */
#define LIGHTPEN_Y_VALUE (readinputport(19)&~1)	/* effectiv resolution */

#define JOYSTICK_1_LEFT	((input_port_13_word_r(0,0)&0x8000))
#define JOYSTICK_1_RIGHT	((input_port_13_word_r(0,0)&0x4000))
#define JOYSTICK_1_UP		((input_port_13_word_r(0,0)&0x2000))
#define JOYSTICK_1_DOWN	((input_port_13_word_r(0,0)&0x1000))
#define JOYSTICK_1_BUTTON ((input_port_13_word_r(0,0)&0x800))
#define JOYSTICK_1_BUTTON2 ((input_port_13_word_r(0,0)&0x400))
#define JOYSTICK_2_LEFT	((input_port_13_word_r(0,0)&0x80))
#define JOYSTICK_2_RIGHT	((input_port_13_word_r(0,0)&0x40))
#define JOYSTICK_2_UP		((input_port_13_word_r(0,0)&0x20))
#define JOYSTICK_2_DOWN	((input_port_13_word_r(0,0)&0x10))
#define JOYSTICK_2_BUTTON ((input_port_13_word_r(0,0)&8))
#define JOYSTICK_2_BUTTON2 ((input_port_13_word_r(0,0)&4))

#define IEEE8ON	0
#define IEEE9ON	0

extern UINT8 *cbmb_basic;
extern UINT8 *cbmb_kernal;
//extern UINT8 *cbmb_chargen;
extern UINT8 *cbmb_memory;
extern UINT8 *cbmb_videoram;
extern UINT8 *cbmb_colorram;

WRITE8_HANDLER ( cbmb_colorram_w );

void cbm500_driver_init (void);
void cbm600_driver_init (void);
void cbm600pal_driver_init (void);
void cbm600hu_driver_init (void);
void cbm700_driver_init (void);
void cbmb_common_init_machine (void);
void cbm600_update_row(mame_bitmap *bitmap, const rectangle *cliprect, UINT16 ma,
					   UINT8 ra, UINT16 y, UINT8 x_count, void *param);
void cbm700_update_row(mame_bitmap *bitmap, const rectangle *cliprect, UINT16 ma,
					   UINT8 ra, UINT16 y, UINT8 x_count, void *param);
MACHINE_RESET( cbmb );

void cbmb_rom_load(void);

void cbm600_vh_init(void);
void cbm700_vh_init(void);
extern VIDEO_START( cbm700 );

void cbmb_vh_set_font(int font);

void cbmb_state(void);

#endif
