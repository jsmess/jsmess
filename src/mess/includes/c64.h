/***************************************************************************
	commodore c64 home computer

    peter.trauner@jk.uni-linz.ac.at
    documentation
     www.funet.fi
***************************************************************************/
#ifndef __C64_H_
#define __C64_H_

#include "driver.h"
#include "machine/6526cia.h"

#define C64_DIPS \
     PORT_START \
     PORT_BIT( 0x800, IP_ACTIVE_HIGH, IPT_BUTTON1) \
     PORT_BIT( 0x400, IP_ACTIVE_HIGH, IPT_BUTTON2) \
     PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_8WAY\
     PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_8WAY\
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP)  PORT_8WAY\
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_8WAY\
	PORT_BIT ( 0x300, 0x0,	 IPT_UNUSED )\
	PORT_BIT( 8, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("P2 Button") PORT_CODE(KEYCODE_INSERT) PORT_CODE(JOYCODE_2_BUTTON1 ) PORT_PLAYER(2)\
	PORT_BIT( 4, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("P2 Button 2") PORT_CODE(KEYCODE_PGUP) PORT_CODE(JOYCODE_2_BUTTON2 ) PORT_PLAYER(2)\
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_NAME("P2 Left") PORT_CODE(KEYCODE_DEL) PORT_CODE(JOYCODE_2_LEFT ) PORT_PLAYER(2) PORT_8WAY\
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_NAME("P2 Right") PORT_CODE(KEYCODE_PGDN) PORT_CODE(JOYCODE_2_RIGHT ) PORT_PLAYER(2) PORT_8WAY\
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_NAME("P2 Up") PORT_CODE(KEYCODE_HOME) PORT_CODE(JOYCODE_2_UP) PORT_PLAYER(2) PORT_8WAY\
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_NAME("P2 Down") PORT_CODE(KEYCODE_END) PORT_CODE(JOYCODE_2_DOWN) PORT_PLAYER(2) PORT_8WAY\
	PORT_BIT ( 0x3, 0x0,	 IPT_UNUSED )\
	PORT_START \
	PORT_BIT( 0x100, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Paddle 1 Button") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(JOYCODE_1_BUTTON1)\
	PORT_BIT(0xff,128,IPT_PADDLE) PORT_SENSITIVITY(30) PORT_KEYDELTA(20) PORT_MINMAX(0,255) PORT_CODE_DEC(KEYCODE_LEFT) PORT_CODE_INC(KEYCODE_RIGHT) PORT_CODE_DEC(JOYCODE_1_LEFT) PORT_CODE_INC(JOYCODE_1_RIGHT) PORT_REVERSE\
	PORT_START \
	PORT_BIT( 0x100, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("Paddle 2 Button") PORT_CODE(KEYCODE_LALT) PORT_CODE(JOYCODE_1_BUTTON2)\
	PORT_BIT(0xff,128,IPT_PADDLE) PORT_SENSITIVITY(30) PORT_KEYDELTA(20) PORT_MINMAX(0,255) PORT_CODE_DEC(KEYCODE_DOWN) PORT_CODE_INC(KEYCODE_UP) PORT_CODE_DEC(JOYCODE_1_UP) PORT_CODE_INC(JOYCODE_1_DOWN) PORT_PLAYER(2) PORT_REVERSE\
	PORT_START \
	PORT_BIT( 0x100, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("Paddle 3 Button") PORT_CODE(KEYCODE_INSERT) PORT_CODE(JOYCODE_2_BUTTON1)\
	PORT_BIT(0xff,128,IPT_PADDLE) PORT_SENSITIVITY(30) PORT_KEYDELTA(20) PORT_MINMAX(0,255) PORT_CODE_DEC(KEYCODE_HOME) PORT_CODE_INC(KEYCODE_PGUP) PORT_PLAYER(3) PORT_REVERSE\
     PORT_START \
	PORT_BIT( 0x100, IP_ACTIVE_HIGH, IPT_BUTTON4) PORT_NAME("Paddle 4 Button") PORT_CODE(KEYCODE_DEL) PORT_CODE(JOYCODE_2_BUTTON2)\
	PORT_BIT(0xff,128,IPT_PADDLE) PORT_SENSITIVITY(30) PORT_KEYDELTA(20) PORT_MINMAX(0,255) PORT_CODE_DEC(KEYCODE_END) PORT_CODE_INC(KEYCODE_PGDN) PORT_PLAYER(4) PORT_REVERSE\
	PORT_START \
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Mouse Button Left") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(JOYCODE_1_BUTTON1)\
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Mouse Button Right") PORT_CODE(KEYCODE_LALT) PORT_CODE(JOYCODE_1_BUTTON2)\
	PORT_BIT( 0x7e, 0x00, IPT_TRACKBALL_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_MINMAX(0,0)  PORT_PLAYER(1)\
	/*PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("Lightpen Signal") PORT_CODE(KEYCODE_LCONTROL) */\
	/*PORT_BIT(0x1ff,0,IPT_PADDLE) PORT_SENSITIVITY(30) PORT_KEYDELTA(2) PORT_MINMAX(0,320-1) PORT_CODE_DEC(KEYCODE_LEFT) PORT_CODE_INC(KEYCODE_RIGHT) PORT_CODE_DEC(JOYCODE_1_LEFT) PORT_CODE_INC(JOYCODE_1_RIGHT) PORT_PLAYER(1)*/\
     PORT_START \
	PORT_BIT( 0x7e, 0x00, IPT_TRACKBALL_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_MINMAX(0,0)  PORT_PLAYER(1) PORT_REVERSE\
     /*PORT_BIT(0xff,0,IPT_PADDLE) PORT_SENSITIVITY(30) PORT_KEYDELTA(2) PORT_MINMAX(0,200-1) PORT_CODE_DEC(KEYCODE_UP) PORT_CODE_INC(KEYCODE_DOWN) PORT_CODE_DEC(JOYCODE_1_UP) PORT_CODE_INC(JOYCODE_1_DOWN) PORT_PLAYER(2)*/\
	PORT_START \
	PORT_DIPNAME ( 0xe000, 0x2000, "Gameport A")\
	PORT_DIPSETTING(  0, DEF_STR( None ) )\
	PORT_DIPSETTING(	0x2000, "Joystick 1" )\
	PORT_DIPSETTING(	0x4000, "Paddles 1, 2" )\
	PORT_DIPSETTING(	0x6000, "Mouse Joystick Emulation/2 Button Joystick" )\
	PORT_DIPSETTING(	0x8000, "Mouse" )\
	/*PORT_DIPSETTING(	0xa000, "Lightpen" )*/\
	/*PORT_DIPNAME ( 0x1000, 0x1000, "Lightpen Draw Pointer")*/\
	PORT_DIPSETTING(  0, DEF_STR( Off ) )\
	PORT_DIPSETTING(  0x1000, DEF_STR( On ) )\
	PORT_DIPNAME ( 0xe00, 0x200, "Gameport B")\
	PORT_DIPSETTING(  0, DEF_STR( None ) )\
	PORT_DIPSETTING(	0x0200, "Joystick 2" )\
	PORT_DIPSETTING(	0x0400, "Paddles 3, 4" )\
	PORT_DIPSETTING(	0x0600, "Mouse Joystick Emulation/2 Button Joystick" )\
	PORT_DIPSETTING(	0x0800, "Mouse" )\
	PORT_DIPNAME( 0x100, IP_ACTIVE_HIGH, "Swap Gameport 1 and 2") PORT_CODE(KEYCODE_NUMLOCK)\
	PORT_DIPSETTING(  0, DEF_STR( No ) )\
	PORT_DIPSETTING(	0x100, DEF_STR( Yes ) )

#define LIGHTPEN				((input_port_7_word_r(0,0)&0xe000)==0xa000)
#define MOUSE1					((input_port_7_word_r(0,0)&0xe000)==0x8000)
#define JOYSTICK1_2BUTTON		((input_port_7_word_r(0,0)&0xe000)==0x6000)
#define PADDLES12				((input_port_7_word_r(0,0)&0xe000)==0x4000)
#define JOYSTICK1				((input_port_7_word_r(0,0)&0xe000)==0x2000)
#define LIGHTPEN_POINTER		(LIGHTPEN&&(input_port_7_word_r(0,0)&0x1000))
#define MOUSE2					((input_port_7_word_r(0,0)&0xe00)==0x800)
#define JOYSTICK2_2BUTTON		((input_port_7_word_r(0,0)&0xe00)==0x600)
#define PADDLES34				((input_port_7_word_r(0,0)&0xe00)==0x400)
#define JOYSTICK2				((input_port_7_word_r(0,0)&0xe00)==0x200)
#define JOYSTICK_SWAP			(input_port_7_word_r(0,0)&0x100)

#define PADDLE1_BUTTON	((input_port_1_word_r(0,0)&0x100))
#define PADDLE1_VALUE	(input_port_1_word_r(0,0)&0xff)
#define PADDLE2_BUTTON	((input_port_2_word_r(0,0)&0x100))
#define PADDLE2_VALUE	(input_port_2_word_r(0,0)&0xff)
#define PADDLE3_BUTTON	((input_port_3_word_r(0,0)&0x100))
#define PADDLE3_VALUE	(input_port_3_word_r(0,0)&0xff)
#define PADDLE4_BUTTON	((input_port_4_word_r(0,0)&0x100))
#define PADDLE4_VALUE	(input_port_4_word_r(0,0)&0xff)

#define MOUSE1_BUTTON1 (MOUSE1&&(readinputport(5)&0x8000))
#define MOUSE1_BUTTON2 (MOUSE1&&(readinputport(5)&0x4000))
#define MOUSE1_X ((readinputport(5)&0x3ff))
#define MOUSE1_Y (readinputport(6))

#define MOUSE2_BUTTON1 (MOUSE1&&(readinputport(5)&0x8000))
#define MOUSE2_BUTTON2 (MOUSE1&&(readinputport(5)&0x4000))
#define MOUSE2_X ((readinputport(5)&0x3ff))
#define MOUSE2_Y (readinputport(6))

#define LIGHTPEN_BUTTON (LIGHTPEN&&(readinputport(5)&0x8000))
#define LIGHTPEN_X_VALUE ((readinputport(5)&0x3ff)&~1)	/* effectiv resolution */
#define LIGHTPEN_Y_VALUE (readinputport(6)&~1)	/* effectiv resolution */

#define JOYSTICK_1_LEFT	((input_port_0_word_r(0,0)&0x8000))
#define JOYSTICK_1_RIGHT	((input_port_0_word_r(0,0)&0x4000))
#define JOYSTICK_1_UP		((input_port_0_word_r(0,0)&0x2000))
#define JOYSTICK_1_DOWN	((input_port_0_word_r(0,0)&0x1000))
#define JOYSTICK_1_BUTTON ((input_port_0_word_r(0,0)&0x800))
#define JOYSTICK_1_BUTTON2 ((input_port_0_word_r(0,0)&0x400))
#define JOYSTICK_2_LEFT	((input_port_0_word_r(0,0)&0x80))
#define JOYSTICK_2_RIGHT	((input_port_0_word_r(0,0)&0x40))
#define JOYSTICK_2_UP		((input_port_0_word_r(0,0)&0x20))
#define JOYSTICK_2_DOWN	((input_port_0_word_r(0,0)&0x10))
#define JOYSTICK_2_BUTTON ((input_port_0_word_r(0,0)&8))
#define JOYSTICK_2_BUTTON2 ((input_port_0_word_r(0,0)&4))

#define QUICKLOAD		(input_port_8_word_r(0,0)&0x8000)
#define DATASSETTE (input_port_8_word_r(0,0)&0x4000)
#define DATASSETTE_TONE (input_port_8_word_r(0,0)&0x2000)

#define DATASSETTE_PLAY		(input_port_8_word_r(0,0)&0x1000)
#define DATASSETTE_RECORD	(input_port_8_word_r(0,0)&0x800)
#define DATASSETTE_STOP		(input_port_8_word_r(0,0)&0x400)

#define SID8580		((input_port_8_r(0)&0x80) ? MOS8580 : MOS6581)

#define AUTO_MODULE ((input_port_8_r(0)&0x1c)==0)
#define ULTIMAX_MODULE ((input_port_8_r(0)&0x1c)==4)
#define C64_MODULE ((input_port_8_r(0)&0x1c)==8)
#define SUPERGAMES_MODULE ((input_port_8_r(0)&0x1c)==0x10)
#define ROBOCOP2_MODULE ((input_port_8_r(0)&0x1c)==0x14)
#define C128_MODULE ((input_port_8_r(0)&0x1c)==0x18)

#define KEY_ARROW_LEFT (input_port_9_word_r(0,0)&0x8000)
#define KEY_1 (input_port_9_word_r(0,0)&0x4000)
#define KEY_2 (input_port_9_word_r(0,0)&0x2000)
#define KEY_3 (input_port_9_word_r(0,0)&0x1000)
#define KEY_4 (input_port_9_word_r(0,0)&0x800)
#define KEY_5 (input_port_9_word_r(0,0)&0x400)
#define KEY_6 (input_port_9_word_r(0,0)&0x200)
#define KEY_7 (input_port_9_word_r(0,0)&0x100)
#define KEY_8 (input_port_9_word_r(0,0)&0x80)
#define KEY_9 (input_port_9_word_r(0,0)&0x40)
#define KEY_0 (input_port_9_word_r(0,0)&0x20)
#define KEY_PLUS (input_port_9_word_r(0,0)&0x10)
#define KEY_MINUS (input_port_9_word_r(0,0)&8)
#define KEY_POUND (input_port_9_word_r(0,0)&4)
#define KEY_HOME (input_port_9_word_r(0,0)&2)
#define KEY_DEL (input_port_9_word_r(0,0)&1)

#define KEY_CTRL (input_port_10_word_r(0,0)&0x8000)
#define KEY_Q (input_port_10_word_r(0,0)&0x4000)
#define KEY_W (input_port_10_word_r(0,0)&0x2000)
#define KEY_E (input_port_10_word_r(0,0)&0x1000)
#define KEY_R (input_port_10_word_r(0,0)&0x800)
#define KEY_T (input_port_10_word_r(0,0)&0x400)
#define KEY_Y (input_port_10_word_r(0,0)&0x200)
#define KEY_U (input_port_10_word_r(0,0)&0x100)
#define KEY_I (input_port_10_word_r(0,0)&0x80)
#define KEY_O (input_port_10_word_r(0,0)&0x40)
#define KEY_P (input_port_10_word_r(0,0)&0x20)
#define KEY_ATSIGN (input_port_10_word_r(0,0)&0x10)
#define KEY_ASTERIX (input_port_10_word_r(0,0)&8)
#define KEY_ARROW_UP (input_port_10_word_r(0,0)&4)
#define KEY_RESTORE (input_port_10_word_r(0,0)&2)
#define KEY_STOP (input_port_10_word_r(0,0)&1)

#define KEY_SHIFTLOCK (input_port_11_word_r(0,0)&0x8000)
#define KEY_A (input_port_11_word_r(0,0)&0x4000)
#define KEY_S (input_port_11_word_r(0,0)&0x2000)
#define KEY_D (input_port_11_word_r(0,0)&0x1000)
#define KEY_F (input_port_11_word_r(0,0)&0x800)
#define KEY_G (input_port_11_word_r(0,0)&0x400)
#define KEY_H (input_port_11_word_r(0,0)&0x200)
#define KEY_J (input_port_11_word_r(0,0)&0x100)
#define KEY_K (input_port_11_word_r(0,0)&0x80)
#define KEY_L (input_port_11_word_r(0,0)&0x40)
#define KEY_SEMICOLON (input_port_11_word_r(0,0)&0x20)
#define KEY_COLON (input_port_11_word_r(0,0)&0x10)
#define KEY_EQUALS (input_port_11_word_r(0,0)&8)
#define KEY_RETURN (input_port_11_word_r(0,0)&4)
#define KEY_CBM (input_port_11_word_r(0,0)&2)
#define KEY_LEFT_SHIFT ((input_port_11_word_r(0,0)&1)||KEY_SHIFTLOCK)

#define KEY_Z (input_port_12_word_r(0,0)&0x8000)
#define KEY_X (input_port_12_word_r(0,0)&0x4000)
#define KEY_C (input_port_12_word_r(0,0)&0x2000)
#define KEY_V (input_port_12_word_r(0,0)&0x1000)
#define KEY_B (input_port_12_word_r(0,0)&0x800)
#define KEY_N (input_port_12_word_r(0,0)&0x400)
#define KEY_M (input_port_12_word_r(0,0)&0x200)
#define KEY_COMMA (input_port_12_word_r(0,0)&0x100)
#define KEY_POINT (input_port_12_word_r(0,0)&0x80)
#define KEY_SLASH (input_port_12_word_r(0,0)&0x40)
#define KEY_RIGHT_SHIFT ((input_port_12_word_r(0,0)&0x20)\
			 ||KEY_CURSOR_UP||KEY_CURSOR_LEFT)
#define KEY_CURSOR_DOWN ((input_port_12_word_r(0,0)&0x10)||KEY_CURSOR_UP)
#define KEY_CURSOR_RIGHT ((input_port_12_word_r(0,0)&8)||KEY_CURSOR_LEFT)
#define KEY_SPACE (input_port_12_word_r(0,0)&4)
#define KEY_F1 (input_port_12_word_r(0,0)&2)
#define KEY_F3 (input_port_12_word_r(0,0)&1)

#define KEY_F5 (input_port_13_word_r(0,0)&0x8000)
#define KEY_F7 (input_port_13_word_r(0,0)&0x4000)
#define KEY_CURSOR_UP (input_port_13_word_r(0,0)&0x2000)
#define KEY_CURSOR_LEFT (input_port_13_word_r(0,0)&0x1000)

extern UINT8 *c64_colorram;
extern UINT8 *c64_basic;
extern UINT8 *c64_kernal;
extern UINT8 *c64_chargen;
extern UINT8 *c64_memory;

UINT8 c64_m6510_port_read(void);
void c64_m6510_port_write(UINT8 data);

READ8_HANDLER ( c64_colorram_read );
WRITE8_HANDLER ( c64_colorram_write );

DRIVER_INIT( c64 );
DRIVER_INIT( c64pal );
DRIVER_INIT( ultimax );
DRIVER_INIT( c64gs );
DRIVER_INIT( sx64 );
void c64_common_init_machine (void);

MACHINE_START( c64 );
INTERRUPT_GEN( c64_frame_interrupt );

void c64_rom_load(void);
void c64_rom_recognition (void);

/* private area */

WRITE8_HANDLER ( c64_write_io );
READ8_HANDLER ( c64_read_io );
WRITE8_HANDLER ( c64_tape_read );
int c64_paddle_read (int which);
void c64_vic_interrupt (int level);

extern int c64_pal;
extern int c64_tape_on;
extern UINT8 *c64_roml;
extern UINT8 *c64_romh;
extern UINT8 c64_keyline[10];
extern int c128_va1617;
extern UINT8 *c64_vicaddr, *c128_vicaddr;
extern UINT8 c64_game, c64_exrom;
extern const cia6526_interface c64_cia0, c64_cia1;

#endif
