/*****************************************************************************
 *
 * includes/cbmb.h
 * 
 * Commodore B Series Computer
 *
 * peter.trauner@jk.uni-linz.ac.at
 * 
 ****************************************************************************/

#ifndef CBMB_H_
#define CBMB_H_

#include "video/mc6845.h"


#define C64_DIPS \
	PORT_START_TAG("JOY") \
	PORT_BIT( 0x800, IP_ACTIVE_HIGH, IPT_BUTTON1) \
	PORT_BIT( 0x400, IP_ACTIVE_HIGH, IPT_BUTTON2) \
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_8WAY\
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_8WAY\
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP)  PORT_8WAY\
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_8WAY\
	PORT_BIT( 0x300, 0x000,	 IPT_UNUSED )\
	PORT_BIT( 0x8, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("P2 Button") PORT_CODE(KEYCODE_INSERT) PORT_CODE(JOYCODE_BUTTON1 ) PORT_PLAYER(2)\
	PORT_BIT( 0x4, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("P2 Button 2") PORT_CODE(KEYCODE_PGUP) PORT_CODE(JOYCODE_BUTTON2 ) PORT_PLAYER(2)\
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_NAME("P2 Left") PORT_CODE(KEYCODE_DEL) PORT_CODE(JOYCODE_X_LEFT_SWITCH ) PORT_PLAYER(2) PORT_8WAY\
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_NAME("P2 Right") PORT_CODE(KEYCODE_PGDN) PORT_CODE(JOYCODE_X_RIGHT_SWITCH ) PORT_PLAYER(2) PORT_8WAY\
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_NAME("P2 Up") PORT_CODE(KEYCODE_HOME) PORT_CODE(JOYCODE_Y_UP_SWITCH) PORT_PLAYER(2) PORT_8WAY\
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_NAME("P2 Down") PORT_CODE(KEYCODE_END) PORT_CODE(JOYCODE_Y_DOWN_SWITCH) PORT_PLAYER(2) PORT_8WAY\
	PORT_BIT( 0x3, 0x0,	 IPT_UNUSED )\
	PORT_START_TAG("PAD1") \
	PORT_BIT( 0x100, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Paddle 1 Button") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(JOYCODE_BUTTON1)\
	PORT_BIT(0xff,128,IPT_PADDLE) PORT_SENSITIVITY(30) PORT_KEYDELTA(20) PORT_MINMAX(0,255) PORT_CODE_DEC(KEYCODE_LEFT) PORT_CODE_INC(KEYCODE_RIGHT) PORT_CODE_DEC(JOYCODE_X_LEFT_SWITCH) PORT_CODE_INC(JOYCODE_X_RIGHT_SWITCH) PORT_REVERSE\
	PORT_START_TAG("PAD2") \
	PORT_BIT( 0x100, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("Paddle 2 Button") PORT_CODE(KEYCODE_LALT) PORT_CODE(JOYCODE_BUTTON2)\
	PORT_BIT(0xff,128,IPT_PADDLE) PORT_SENSITIVITY(30) PORT_KEYDELTA(20) PORT_MINMAX(0,255) PORT_CODE_DEC(KEYCODE_DOWN) PORT_CODE_INC(KEYCODE_UP) PORT_CODE_DEC(JOYCODE_Y_UP_SWITCH) PORT_CODE_INC(JOYCODE_Y_DOWN_SWITCH) PORT_PLAYER(2) PORT_REVERSE\
	PORT_START_TAG("PAD3") \
	PORT_BIT( 0x100, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("Paddle 3 Button") PORT_CODE(KEYCODE_INSERT) PORT_CODE(JOYCODE_BUTTON1)\
	PORT_BIT(0xff,128,IPT_PADDLE) PORT_SENSITIVITY(30) PORT_KEYDELTA(20) PORT_MINMAX(0,255) PORT_CODE_DEC(KEYCODE_HOME) PORT_CODE_INC(KEYCODE_PGUP) PORT_PLAYER(3) PORT_REVERSE\
	PORT_START_TAG("PAD4") \
	PORT_BIT( 0x100, IP_ACTIVE_HIGH, IPT_BUTTON4) PORT_NAME("Paddle 4 Button") PORT_CODE(KEYCODE_DEL) PORT_CODE(JOYCODE_BUTTON2)\
	PORT_BIT(0xff,128,IPT_PADDLE) PORT_SENSITIVITY(30) PORT_KEYDELTA(20) PORT_MINMAX(0,255) PORT_CODE_DEC(KEYCODE_END) PORT_CODE_INC(KEYCODE_PGDN) PORT_PLAYER(4) PORT_REVERSE\
	PORT_START_TAG("TRX") \
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Mouse Button Left") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(JOYCODE_BUTTON1)\
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Mouse Button Right") PORT_CODE(KEYCODE_LALT) PORT_CODE(JOYCODE_BUTTON2)\
	PORT_BIT( 0x7e, 0x00, IPT_TRACKBALL_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1)\
	/* PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("Lightpen Signal") PORT_CODE(KEYCODE_LCONTROL) */ \
	/* PORT_BIT(0x1ff,0,IPT_PADDLE) PORT_SENSITIVITY(30) PORT_KEYDELTA(2) PORT_MINMAX(0,320-1) PORT_CODE_DEC(KEYCODE_LEFT) PORT_CODE_INC(KEYCODE_RIGHT) PORT_CODE_DEC(JOYCODE_X_LEFT_SWITCH) PORT_CODE_INC(JOYCODE_X_RIGHT_SWITCH) PORT_PLAYER(1) */ \
	PORT_START_TAG("TRY") \
	PORT_BIT( 0x7e, 0x00, IPT_TRACKBALL_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1) PORT_REVERSE\
	/* PORT_BIT(0xff,0,IPT_PADDLE) PORT_SENSITIVITY(30) PORT_KEYDELTA(2) PORT_MINMAX(0,200-1) PORT_CODE_DEC(KEYCODE_UP) PORT_CODE_INC(KEYCODE_DOWN) PORT_CODE_DEC(JOYCODE_Y_UP_SWITCH) PORT_CODE_INC(JOYCODE_Y_DOWN_SWITCH) PORT_PLAYER(2) */ \
	PORT_START_TAG("DSW0") /* 20 */ \
	PORT_DIPNAME ( 0xe000, 0x2000, "Gameport A")\
	PORT_DIPSETTING(	0x0000, DEF_STR( None ) )\
	PORT_DIPSETTING(	0x2000, "Joystick 1" )\
	PORT_DIPSETTING(	0x4000, "Paddles 1, 2" )\
	PORT_DIPSETTING(	0x6000, "Mouse Joystick Emulation/2 Button Joystick" )\
	PORT_DIPSETTING(	0x8000, "Mouse" )\
	PORT_DIPSETTING(	0xa000, "Lightpen" )\
	PORT_DIPNAME (	0x1000, 0x1000, "Lightpen Draw Pointer")\
	PORT_DIPSETTING(	0x0000, DEF_STR( Off ) )\
	PORT_DIPSETTING(	0x1000, DEF_STR( On ) )\
	PORT_DIPNAME ( 0x0e00, 0x0200, "Gameport B")\
	PORT_DIPSETTING(	0x0000, DEF_STR( None ) )\
	PORT_DIPSETTING(	0x0200, "Joystick 2" )\
	PORT_DIPSETTING(	0x0400, "Paddles 3, 4" )\
	PORT_DIPSETTING(	0x0600, "Mouse Joystick Emulation/2 Button Joystick" )\
	PORT_DIPSETTING(	0x0800, "Mouse" )\
	PORT_DIPNAME( 0x0100, IP_ACTIVE_HIGH, "Swap Gameport 1 and 2") PORT_CODE(KEYCODE_NUMLOCK)\
	PORT_DIPSETTING(	0x0000, DEF_STR( No ) )\
	PORT_DIPSETTING(	0x0100, DEF_STR( Yes ) )

#define LIGHTPEN		(( input_port_read(machine,  "DSW0" ) & 0xe000 ) == 0xa000 )
#define MOUSE1			(( input_port_read(machine,  "DSW0" ) & 0xe000 ) == 0x8000 )
#define JOYSTICK1_2BUTTON	(( input_port_read(machine,  "DSW0" ) & 0xe000 ) == 0x6000 )
#define PADDLES12		(( input_port_read(machine,  "DSW0" ) & 0xe000 ) == 0x4000 )
#define JOYSTICK1		(( input_port_read(machine,  "DSW0" ) & 0xe000 ) == 0x2000 )
#define LIGHTPEN_POINTER	( LIGHTPEN && ( input_port_read(machine,  "DSW0" ) & 0x1000))
#define MOUSE2			(( input_port_read(machine,  "DSW0" ) & 0xe000 ) == 0x800 )
#define JOYSTICK2_2BUTTON	(( input_port_read(machine,  "DSW0" ) & 0xe000 ) == 0x600 )
#define PADDLES34		(( input_port_read(machine,  "DSW0" ) & 0xe000 ) == 0x400 )
#define JOYSTICK2		(( input_port_read(machine,  "DSW0" ) & 0xe000 ) == 0x200 )
#define JOYSTICK_SWAP		( input_port_read(machine,  "DSW0" ) & 0x100 )

#define PADDLE1_BUTTON		(( input_port_read(machine,  "PAD1" ) & 0x100 ))
#define PADDLE1_VALUE		( input_port_read(machine,  "PAD1" ) & 0xff )
#define PADDLE2_BUTTON		(( input_port_read(machine,  "PAD2" ) & 0x100 ))
#define PADDLE2_VALUE		( input_port_read(machine,  "PAD2" ) & 0xff )
#define PADDLE3_BUTTON		(( input_port_read(machine,  "PAD3" ) & 0x100 ))
#define PADDLE3_VALUE		( input_port_read(machine,  "PAD3" ) & 0xff )
#define PADDLE4_BUTTON		(( input_port_read(machine,  "PAD4" ) & 0x100 ))
#define PADDLE4_VALUE		( input_port_read(machine,  "PAD4" ) & 0xff )

#define MOUSE1_BUTTON1		( MOUSE1 && ( input_port_read(machine,  "TRX" ) & 0x8000 ))
#define MOUSE1_BUTTON2		( MOUSE1 && ( input_port_read(machine,  "TRX" ) & 0x4000 ))
#define MOUSE1_X		(( input_port_read(machine,  "TRX" ) & 0x3ff ))
#define MOUSE1_Y		( input_port_read(machine,  "TRY" ))

#define MOUSE2_BUTTON1		( MOUSE1 && ( input_port_read(machine,  "TRX" ) & 0x8000 ))
#define MOUSE2_BUTTON2		( MOUSE1 && ( input_port_read(machine,  "TRX" ) & 0x4000 ))
#define MOUSE2_X		(( input_port_read(machine,  "TRX" ) & 0x3ff ))
#define MOUSE2_Y		( input_port_read(machine,  "TRY" ))

#define LIGHTPEN_BUTTON		( LIGHTPEN && ( input_port_read(machine,  "TRX" ) & 0x8000 ))
#define LIGHTPEN_X_VALUE	(( input_port_read(machine,  "TRX" ) & 0x3ff ) & ~1)	/* effective resolution */
#define LIGHTPEN_Y_VALUE	( input_port_read(machine,  "TRY" ) & ~1)				/* effective resolution */

#define JOYSTICK_1_LEFT		(( input_port_read(machine,  "JOY" ) & 0x8000 ))
#define JOYSTICK_1_RIGHT	(( input_port_read(machine,  "JOY" ) & 0x4000 ))
#define JOYSTICK_1_UP		(( input_port_read(machine,  "JOY" ) & 0x2000 ))
#define JOYSTICK_1_DOWN		(( input_port_read(machine,  "JOY" ) & 0x1000 ))
#define JOYSTICK_1_BUTTON	(( input_port_read(machine,  "JOY" ) & 0x800 ))
#define JOYSTICK_1_BUTTON2	(( input_port_read(machine,  "JOY" ) & 0x400 ))
#define JOYSTICK_2_LEFT		(( input_port_read(machine,  "JOY" ) & 0x80 ))
#define JOYSTICK_2_RIGHT	(( input_port_read(machine,  "JOY" ) & 0x40 ))
#define JOYSTICK_2_UP		(( input_port_read(machine,  "JOY" ) & 0x20 ))
#define JOYSTICK_2_DOWN		(( input_port_read(machine,  "JOY" ) & 0x10 ))
#define JOYSTICK_2_BUTTON	(( input_port_read(machine,  "JOY" ) & 8 ))
#define JOYSTICK_2_BUTTON2	(( input_port_read(machine,  "JOY" ) & 4 ))

#define IEEE8ON	0
#define IEEE9ON	0


/*----------- defined in machine/cbmb.c -----------*/

extern UINT8 *cbmb_basic;
extern UINT8 *cbmb_kernal;
//extern UINT8 *cbmb_chargen;
extern UINT8 *cbmb_memory;
extern UINT8 *cbmb_videoram;
extern UINT8 *cbmb_colorram;

WRITE8_HANDLER ( cbmb_colorram_w );

DRIVER_INIT( cbm500 );
DRIVER_INIT( cbm600 );
DRIVER_INIT( cbm600pal );
DRIVER_INIT( cbm600hu );
DRIVER_INIT( cbm700 );
MACHINE_RESET( cbmb );

void cbmb_rom_load(void);


/*----------- defined in video/cbmb.c -----------*/

VIDEO_START( cbmb_crtc );
VIDEO_UPDATE( cbmb_crtc );
MC6845_UPDATE_ROW( cbm600_update_row );
MC6845_UPDATE_ROW( cbm700_update_row );
MC6845_ON_DE_CHANGED( cbmb_display_enable_changed );

void cbm600_vh_init(void);
void cbm700_vh_init(void);
VIDEO_START( cbm700 );

void cbmb_vh_set_font(int font);


#endif /* CBMB_H_ */
