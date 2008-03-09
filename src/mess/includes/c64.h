/*****************************************************************************
 *
 * includes/c64.h
 * 
 * Commodore C64 Home Computer
 * 
 * peter.trauner@jk.uni-linz.ac.at
 * 
 * Documentation: www.funet.fi
 *
 ****************************************************************************/

#ifndef C64_H_
#define C64_H_

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
	PORT_BIT( 8, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("P2 Button") PORT_CODE(KEYCODE_INSERT) PORT_CODE(JOYCODE_BUTTON1 ) PORT_PLAYER(2)\
	PORT_BIT( 4, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("P2 Button 2") PORT_CODE(KEYCODE_PGUP) PORT_CODE(JOYCODE_BUTTON2 ) PORT_PLAYER(2)\
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_NAME("P2 Left") PORT_CODE(KEYCODE_DEL) PORT_CODE(JOYCODE_X_LEFT_SWITCH ) PORT_PLAYER(2) PORT_8WAY\
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_NAME("P2 Right") PORT_CODE(KEYCODE_PGDN) PORT_CODE(JOYCODE_X_RIGHT_SWITCH ) PORT_PLAYER(2) PORT_8WAY\
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_NAME("P2 Up") PORT_CODE(KEYCODE_HOME) PORT_CODE(JOYCODE_Y_UP_SWITCH) PORT_PLAYER(2) PORT_8WAY\
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_NAME("P2 Down") PORT_CODE(KEYCODE_END) PORT_CODE(JOYCODE_Y_DOWN_SWITCH) PORT_PLAYER(2) PORT_8WAY\
	PORT_BIT ( 0x3, 0x0,	 IPT_UNUSED )\
	PORT_START \
	PORT_BIT( 0x100, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Paddle 1 Button") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(JOYCODE_BUTTON1)\
	PORT_BIT(0xff,128,IPT_PADDLE) PORT_SENSITIVITY(30) PORT_KEYDELTA(20) PORT_MINMAX(0,255) PORT_CODE_DEC(KEYCODE_LEFT) PORT_CODE_INC(KEYCODE_RIGHT) PORT_CODE_DEC(JOYCODE_X_LEFT_SWITCH) PORT_CODE_INC(JOYCODE_X_RIGHT_SWITCH) PORT_REVERSE\
	PORT_START \
	PORT_BIT( 0x100, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("Paddle 2 Button") PORT_CODE(KEYCODE_LALT) PORT_CODE(JOYCODE_BUTTON2)\
	PORT_BIT(0xff,128,IPT_PADDLE) PORT_SENSITIVITY(30) PORT_KEYDELTA(20) PORT_MINMAX(0,255) PORT_CODE_DEC(KEYCODE_DOWN) PORT_CODE_INC(KEYCODE_UP) PORT_CODE_DEC(JOYCODE_Y_UP_SWITCH) PORT_CODE_INC(JOYCODE_Y_DOWN_SWITCH) PORT_PLAYER(2) PORT_REVERSE\
	PORT_START \
	PORT_BIT( 0x100, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("Paddle 3 Button") PORT_CODE(KEYCODE_INSERT) PORT_CODE(JOYCODE_BUTTON1)\
	PORT_BIT(0xff,128,IPT_PADDLE) PORT_SENSITIVITY(30) PORT_KEYDELTA(20) PORT_MINMAX(0,255) PORT_CODE_DEC(KEYCODE_HOME) PORT_CODE_INC(KEYCODE_PGUP) PORT_PLAYER(3) PORT_REVERSE\
     PORT_START \
	PORT_BIT( 0x100, IP_ACTIVE_HIGH, IPT_BUTTON4) PORT_NAME("Paddle 4 Button") PORT_CODE(KEYCODE_DEL) PORT_CODE(JOYCODE_BUTTON2)\
	PORT_BIT(0xff,128,IPT_PADDLE) PORT_SENSITIVITY(30) PORT_KEYDELTA(20) PORT_MINMAX(0,255) PORT_CODE_DEC(KEYCODE_END) PORT_CODE_INC(KEYCODE_PGDN) PORT_PLAYER(4) PORT_REVERSE\
	PORT_START \
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Mouse Button Left") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(JOYCODE_BUTTON1)\
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Mouse Button Right") PORT_CODE(KEYCODE_LALT) PORT_CODE(JOYCODE_BUTTON2)\
	PORT_BIT( 0x7e, 0x00, IPT_TRACKBALL_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1)\
	/*PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("Lightpen Signal") PORT_CODE(KEYCODE_LCONTROL) */\
	/*PORT_BIT(0x1ff,0,IPT_PADDLE) PORT_SENSITIVITY(30) PORT_KEYDELTA(2) PORT_MINMAX(0,320-1) PORT_CODE_DEC(KEYCODE_LEFT) PORT_CODE_INC(KEYCODE_RIGHT) PORT_CODE_DEC(JOYCODE_X_LEFT_SWITCH) PORT_CODE_INC(JOYCODE_X_RIGHT_SWITCH) PORT_PLAYER(1)*/\
     PORT_START \
	PORT_BIT( 0x7e, 0x00, IPT_TRACKBALL_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1) PORT_REVERSE\
     /*PORT_BIT(0xff,0,IPT_PADDLE) PORT_SENSITIVITY(30) PORT_KEYDELTA(2) PORT_MINMAX(0,200-1) PORT_CODE_DEC(KEYCODE_UP) PORT_CODE_INC(KEYCODE_DOWN) PORT_CODE_DEC(JOYCODE_Y_UP_SWITCH) PORT_CODE_INC(JOYCODE_Y_DOWN_SWITCH) PORT_PLAYER(2)*/\
	PORT_START \
	PORT_DIPNAME ( 0xe000, 0x2000, "Gameport A")\
	PORT_DIPSETTING(  0, DEF_STR( None ) )\
	PORT_DIPSETTING(	0x2000, "Joystick 1" )\
	PORT_DIPSETTING(	0x4000, "Paddles 1, 2" )\
	PORT_DIPSETTING(	0x6000, "Mouse Joystick Emulation/2 Button Joystick" )\
	PORT_DIPSETTING(	0x8000, "Mouse" )\
	/*PORT_DIPSETTING(  0xa000, "Lightpen" )*/\
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

#define LIGHTPEN				((readinputport(7)&0xe000)==0xa000)
#define MOUSE1					((readinputport(7)&0xe000)==0x8000)
#define JOYSTICK1_2BUTTON		((readinputport(7)&0xe000)==0x6000)
#define PADDLES12				((readinputport(7)&0xe000)==0x4000)
#define JOYSTICK1				((readinputport(7)&0xe000)==0x2000)
#define LIGHTPEN_POINTER		(LIGHTPEN&&(readinputport(7)&0x1000))
#define MOUSE2					((readinputport(7)&0xe00)==0x800)
#define JOYSTICK2_2BUTTON		((readinputport(7)&0xe00)==0x600)
#define PADDLES34				((readinputport(7)&0xe00)==0x400)
#define JOYSTICK2				((readinputport(7)&0xe00)==0x200)
#define JOYSTICK_SWAP			(readinputport(7)&0x100)

#define PADDLE1_BUTTON	((readinputport(1)&0x100))
#define PADDLE1_VALUE	(readinputport(1)&0xff)
#define PADDLE2_BUTTON	((readinputport(2)&0x100))
#define PADDLE2_VALUE	(readinputport(2)&0xff)
#define PADDLE3_BUTTON	((readinputport(3)&0x100))
#define PADDLE3_VALUE	(readinputport(3)&0xff)
#define PADDLE4_BUTTON	((readinputport(4)&0x100))
#define PADDLE4_VALUE	(readinputport(4)&0xff)

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

#define JOYSTICK_1_LEFT	((readinputport(0)&0x8000))
#define JOYSTICK_1_RIGHT	((readinputport(0)&0x4000))
#define JOYSTICK_1_UP		((readinputport(0)&0x2000))
#define JOYSTICK_1_DOWN	((readinputport(0)&0x1000))
#define JOYSTICK_1_BUTTON ((readinputport(0)&0x800))
#define JOYSTICK_1_BUTTON2 ((readinputport(0)&0x400))
#define JOYSTICK_2_LEFT	((readinputport(0)&0x80))
#define JOYSTICK_2_RIGHT	((readinputport(0)&0x40))
#define JOYSTICK_2_UP		((readinputport(0)&0x20))
#define JOYSTICK_2_DOWN	((readinputport(0)&0x10))
#define JOYSTICK_2_BUTTON ((readinputport(0)&8))
#define JOYSTICK_2_BUTTON2 ((readinputport(0)&4))

#define QUICKLOAD		(readinputport(8)&0x8000)
#define DATASSETTE (readinputport(8)&0x4000)
#define DATASSETTE_TONE (readinputport(8)&0x2000)

#define DATASSETTE_PLAY		(readinputport(8)&0x1000)
#define DATASSETTE_RECORD	(readinputport(8)&0x800)
#define DATASSETTE_STOP		(readinputport(8)&0x400)

#define SID8580		((readinputport(8)&0x80) ? MOS8580 : MOS6581)

#define AUTO_MODULE ((readinputport(8)&0x1c)==0)
#define ULTIMAX_MODULE ((readinputport(8)&0x1c)==4)
#define C64_MODULE ((readinputport(8)&0x1c)==8)
#define SUPERGAMES_MODULE ((readinputport(8)&0x1c)==0x10)
#define ROBOCOP2_MODULE ((readinputport(8)&0x1c)==0x14)
#define C128_MODULE ((readinputport(8)&0x1c)==0x18)

#define KEY_ARROW_LEFT (readinputport(9)&0x8000)
#define KEY_1 (readinputport(9)&0x4000)
#define KEY_2 (readinputport(9)&0x2000)
#define KEY_3 (readinputport(9)&0x1000)
#define KEY_4 (readinputport(9)&0x800)
#define KEY_5 (readinputport(9)&0x400)
#define KEY_6 (readinputport(9)&0x200)
#define KEY_7 (readinputport(9)&0x100)
#define KEY_8 (readinputport(9)&0x80)
#define KEY_9 (readinputport(9)&0x40)
#define KEY_0 (readinputport(9)&0x20)
#define KEY_PLUS (readinputport(9)&0x10)
#define KEY_MINUS (readinputport(9)&8)
#define KEY_POUND (readinputport(9)&4)
#define KEY_HOME (readinputport(9)&2)
#define KEY_DEL (readinputport(9)&1)

#define KEY_CTRL (readinputport(10)&0x8000)
#define KEY_Q (readinputport(10)&0x4000)
#define KEY_W (readinputport(10)&0x2000)
#define KEY_E (readinputport(10)&0x1000)
#define KEY_R (readinputport(10)&0x800)
#define KEY_T (readinputport(10)&0x400)
#define KEY_Y (readinputport(10)&0x200)
#define KEY_U (readinputport(10)&0x100)
#define KEY_I (readinputport(10)&0x80)
#define KEY_O (readinputport(10)&0x40)
#define KEY_P (readinputport(10)&0x20)
#define KEY_ATSIGN (readinputport(10)&0x10)
#define KEY_ASTERIX (readinputport(10)&8)
#define KEY_ARROW_UP (readinputport(10)&4)
#define KEY_RESTORE (readinputport(10)&2)
#define KEY_STOP (readinputport(10)&1)

#define KEY_SHIFTLOCK (readinputport(11)&0x8000)
#define KEY_A (readinputport(11)&0x4000)
#define KEY_S (readinputport(11)&0x2000)
#define KEY_D (readinputport(11)&0x1000)
#define KEY_F (readinputport(11)&0x800)
#define KEY_G (readinputport(11)&0x400)
#define KEY_H (readinputport(11)&0x200)
#define KEY_J (readinputport(11)&0x100)
#define KEY_K (readinputport(11)&0x80)
#define KEY_L (readinputport(11)&0x40)
#define KEY_SEMICOLON (readinputport(11)&0x20)
#define KEY_COLON (readinputport(11)&0x10)
#define KEY_EQUALS (readinputport(11)&8)
#define KEY_RETURN (readinputport(11)&4)
#define KEY_CBM (readinputport(11)&2)
#define KEY_LEFT_SHIFT ((readinputport(11)&1)||KEY_SHIFTLOCK)

#define KEY_Z (readinputport(12)&0x8000)
#define KEY_X (readinputport(12)&0x4000)
#define KEY_C (readinputport(12)&0x2000)
#define KEY_V (readinputport(12)&0x1000)
#define KEY_B (readinputport(12)&0x800)
#define KEY_N (readinputport(12)&0x400)
#define KEY_M (readinputport(12)&0x200)
#define KEY_COMMA (readinputport(12)&0x100)
#define KEY_POINT (readinputport(12)&0x80)
#define KEY_SLASH (readinputport(12)&0x40)
#define KEY_RIGHT_SHIFT ((readinputport(12)&0x20)\
			 ||KEY_CURSOR_UP||KEY_CURSOR_LEFT)
#define KEY_CURSOR_DOWN ((readinputport(12)&0x10)||KEY_CURSOR_UP)
#define KEY_CURSOR_RIGHT ((readinputport(12)&8)||KEY_CURSOR_LEFT)
#define KEY_SPACE (readinputport(12)&4)
#define KEY_F1 (readinputport(12)&2)
#define KEY_F3 (readinputport(12)&1)

#define KEY_F5 (readinputport(13)&0x8000)
#define KEY_F7 (readinputport(13)&0x4000)
#define KEY_CURSOR_UP (readinputport(13)&0x2000)
#define KEY_CURSOR_LEFT (readinputport(13)&0x1000)


/*----------- defined in machine/c64.c -----------*/

/* private area */
extern UINT8 c65_keyline;
extern UINT8 c65_6511_port;

extern UINT8 c128_keyline[3];

extern UINT8 *c64_colorram;
extern UINT8 *c64_basic;
extern UINT8 *c64_kernal;
extern UINT8 *c64_chargen;
extern UINT8 *c64_memory;

UINT8 c64_m6510_port_read(UINT8 direction);
void c64_m6510_port_write(UINT8 direction, UINT8 data);

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
READ8_HANDLER(c64_ioarea_r);
WRITE8_HANDLER(c64_ioarea_w);

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


#endif /* C64_H_ */
