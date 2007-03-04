#include "driver.h"

#include "machine/pit8253.h"
#include "machine/pcshare.h"
#include "includes/amstr_pc.h"
#include "includes/pclpt.h"
#include "video/pc_vga.h"

/* pc20 (v2)
   fc078
   fc102 color/mono selection
   fc166
   fc1b4
   fd841 (output something)
   ff17c (output something, read monitor type inputs)
   fc212
   fc26c
   fc2df
   fc3fe
   fc0f4
   fc432
   fc49f
   fc514
   fc566
   fc5db
   fc622 in 3de

port 0379 read
port 03de write/read
 */

/* pc1512 (v1)
   fc1b5
   fc1f1
   fc264
   fc310
   fc319
   fc385
   fc436
   fc459
   fc4cb
   fc557
   fc591
   fc624
   fc768
   fc818
   fc87d display amstrad ..
    fca17 keyboard check
	 fca69
	 fd680
      fd7f9
	 fca7b !keyboard interrupt routine for this check 
 */


/* pc1640 (v3)
   bios power up self test
   important addresses

   fc0c9
   fc0f2
   fc12e
   fc193
   fc20e
   fc2c1
   fc2d4


   fc375
   fc3ba
   fc3e1
   fc412
   fc43c
   fc47d
   fc48f
   fc51f
   fc5a2
   fc5dd mouse

   fc1c0
   fc5fa
    the following when language selection not 0 (test for presence of 0x80000..0x9ffff ram)
    fc60e
	fc667
   fc678
   fc6e5
   fc72e
   fc78f
   fc7cb ; coprocessor related
   fc834
    feda6 no problem with disk inserted

   fca2a

   cmos ram 28 0 amstrad pc1512 integrated cga
   cmos ram 23 dipswitches?
*/

static struct {
	struct {
		UINT8 x,y; //byte clipping needed
	} mouse;

	// 64 system status register?
	UINT8 port60;
	UINT8 port61;
	UINT8 port62;
	UINT8 port65;

	int dipstate;
} pc1640={{0}, 0};

/* test sequence in bios
 write 00 to 65
 write 30 to 61
 read 62 and (0x10) 
 write 34 to 61
 read 62 and (0x0f)
 return or of the 2 62 reads 

 allows set of the original ibm pc "dipswitches"!!!!

 66 write gives reset?
*/

/* mouse x counter at 0x78 (read- writable)
   mouse y counter at 0x7a (read- writable) 

   mouse button 1,2 keys
   joystick (4 directions, 2 buttons) keys
   these get value from cmos ram
   74 15 00 enter
   70 17 00 forward del
   77 1b 00 joystick button 1
   78 19 00 joystick button 2


   79 00 4d right
   7a 00 4b left
   7b 00 50 down
   7c 00 48 up

   7e 00 01 mouse button left
   7d 01 01 mouse button right
*/

WRITE8_HANDLER( pc1640_port60_w )
{
	switch (offset) {
	case 1:
		pc1640.port61=data;
		if (data==0x30) pc1640.port62=(pc1640.port65&0x10)>>4;
		else if (data==0x34) pc1640.port62=pc1640.port65&0xf;
		pc_sh_speaker(data&3);
		pc_keyb_set_clock(data&0x40);
		break;
	case 4:
		if (data&0x80) {
			pc1640.port60=data^0x8d;
		} else {
			pc1640.port60=data;
		}
		break;
	case 5: 
		// stores the configuration data for port 62 configuration dipswitch emulation
		pc1640.port65=data; 
		break;
	}
	
	logerror("pc1640 write %.2x %.2x\n",offset,data);
}


 READ8_HANDLER( pc1640_port60_r )
{
	int data=0;
	switch (offset) {
	case 0:
		if (pc1640.port61&0x80)
			data=pc1640.port60;
		else 
			data = pc_keyb_read();
		break;

	case 1:
		data = pc1640.port61;
		break;

	case 2: 
		data = pc1640.port62;
		if (pit8253_get_output(0, 2))
			data |= 0x20;
		break;
	}
	return data;
}

 READ8_HANDLER( pc200_port378_r )
{
	int data=pc_parallelport1_r(offset);
	if (offset==1) data=(data&~7)|(input_port_1_r(0)&7);
	if (offset==2) data=(data&~0xe0)|(input_port_1_r(0)&0xc0);
	return data;
}


 READ8_HANDLER( pc1640_port378_r )
{
	int data=pc_parallelport1_r(offset);
	if (offset==1) data=(data&~7)|(input_port_1_r(0)&7);
	if (offset==2) {
		switch (pc1640.dipstate) {
		case 0:
			data=(data&~0xe0)|(input_port_1_r(0)&0xe0);
			break;
		case 1:
			data=(data&~0xe0)|((input_port_1_r(0)&0xe000)>>8);
			break;
		case 2:
			data=(data&~0xe0)|((input_port_1_r(0)&0xe00)>>4);
			break;

		}
	}
	return data;
}

 READ8_HANDLER( pc1640_port3d0_r )
{
	if (offset==0xa) pc1640.dipstate=0;
	return vga_port_03d0_r(offset);
}

 READ8_HANDLER( pc1640_port4278_r )
{
	if (offset==2) pc1640.dipstate=1;
	// read parallelport
	return 0;
}

 READ8_HANDLER( pc1640_port278_r )
{
	if ((offset==2)||(offset==0)) pc1640.dipstate=2;
	// read parallelport
	return 0;
}

 READ8_HANDLER( pc1640_mouse_x_r )
{
	return pc1640.mouse.x-input_port_13_r(0);
}

 READ8_HANDLER( pc1640_mouse_y_r )
{
	return pc1640.mouse.y-input_port_14_r(0);
}

WRITE8_HANDLER( pc1640_mouse_x_w )
{
	pc1640.mouse.x=data+input_port_13_r(0);
}

WRITE8_HANDLER( pc1640_mouse_y_w )
{
	pc1640.mouse.y=data+input_port_14_r(0);
}

#define AMSTRAD_HELPER(bit, text, key1, key2) \
	PORT_BIT( bit, 0x0000, IPT_KEYBOARD) PORT_NAME(text) PORT_CODE(key1) PORT_CODE(key2)

INPUT_PORTS_START( amstrad_keyboard )

	PORT_START_TAG("pc_keyboard_0")
	PORT_BIT ( 0x0001, 0x0000, IPT_UNUSED ) 	/* unused scancode 0 */
	AMSTRAD_HELPER( 0x0002, "Esc",          KEYCODE_ESC,        CODE_NONE )
	AMSTRAD_HELPER( 0x0004, "1 !",          KEYCODE_1,          CODE_NONE )
	AMSTRAD_HELPER( 0x0008, "2 @",          KEYCODE_2,          CODE_NONE )
	AMSTRAD_HELPER( 0x0010, "3 #",          KEYCODE_3,          CODE_NONE )
	AMSTRAD_HELPER( 0x0020, "4 $",          KEYCODE_4,          CODE_NONE )
	AMSTRAD_HELPER( 0x0040, "5 %",          KEYCODE_5,          CODE_NONE )
	AMSTRAD_HELPER( 0x0080, "6 ^",          KEYCODE_6,          CODE_NONE )
	AMSTRAD_HELPER( 0x0100, "7 &",          KEYCODE_7,          CODE_NONE )
	AMSTRAD_HELPER( 0x0200, "8 *",          KEYCODE_8,          CODE_NONE )
	AMSTRAD_HELPER( 0x0400, "9 (",          KEYCODE_9,          CODE_NONE )
	AMSTRAD_HELPER( 0x0800, "0 )",          KEYCODE_0,          CODE_NONE )
	AMSTRAD_HELPER( 0x1000, "- _",          KEYCODE_MINUS,      CODE_NONE )
	AMSTRAD_HELPER( 0x2000, "= +",          KEYCODE_EQUALS,     CODE_NONE )
	AMSTRAD_HELPER( 0x4000, "<--",          KEYCODE_BACKSPACE,  CODE_NONE )
	AMSTRAD_HELPER( 0x8000, "Tab",          KEYCODE_TAB,        CODE_NONE )

	PORT_START_TAG("pc_keyboard_1")
	AMSTRAD_HELPER( 0x0001, "Q",            KEYCODE_Q,          CODE_NONE )
	AMSTRAD_HELPER( 0x0002, "W",            KEYCODE_W,          CODE_NONE )
	AMSTRAD_HELPER( 0x0004, "E",            KEYCODE_E,          CODE_NONE )
	AMSTRAD_HELPER( 0x0008, "R",            KEYCODE_R,          CODE_NONE )
	AMSTRAD_HELPER( 0x0010, "T",            KEYCODE_T,          CODE_NONE )
	AMSTRAD_HELPER( 0x0020, "Y",            KEYCODE_Y,          CODE_NONE )
	AMSTRAD_HELPER( 0x0040, "U",            KEYCODE_U,          CODE_NONE )
	AMSTRAD_HELPER( 0x0080, "I",            KEYCODE_I,          CODE_NONE )
	AMSTRAD_HELPER( 0x0100, "O",            KEYCODE_O,          CODE_NONE )
	AMSTRAD_HELPER( 0x0200, "P",            KEYCODE_P,          CODE_NONE )
	AMSTRAD_HELPER( 0x0400, "[ {",          KEYCODE_OPENBRACE,  CODE_NONE )
	AMSTRAD_HELPER( 0x0800, "] }",          KEYCODE_CLOSEBRACE, CODE_NONE )
	AMSTRAD_HELPER( 0x1000, "Enter",        KEYCODE_ENTER,      CODE_NONE )
	AMSTRAD_HELPER( 0x2000, "Ctrl",			KEYCODE_LCONTROL,   CODE_NONE )
	AMSTRAD_HELPER( 0x4000, "A",            KEYCODE_A,          CODE_NONE )
	AMSTRAD_HELPER( 0x8000, "S",            KEYCODE_S,          CODE_NONE )

	PORT_START_TAG("pc_keyboard_2")
	AMSTRAD_HELPER( 0x0001, "D",            KEYCODE_D,          CODE_NONE )
	AMSTRAD_HELPER( 0x0002, "F",            KEYCODE_F,          CODE_NONE )
	AMSTRAD_HELPER( 0x0004, "G",            KEYCODE_G,          CODE_NONE )
	AMSTRAD_HELPER( 0x0008, "H",            KEYCODE_H,          CODE_NONE )
	AMSTRAD_HELPER( 0x0010, "J",            KEYCODE_J,          CODE_NONE )
	AMSTRAD_HELPER( 0x0020, "K",            KEYCODE_K,          CODE_NONE )
	AMSTRAD_HELPER( 0x0040, "L",            KEYCODE_L,          CODE_NONE )
	AMSTRAD_HELPER( 0x0080, "; :",          KEYCODE_COLON,      CODE_NONE )
	AMSTRAD_HELPER( 0x0100, "' \"",         KEYCODE_QUOTE,      CODE_NONE )
	AMSTRAD_HELPER( 0x0200, "` ~",          KEYCODE_TILDE,      CODE_NONE )
	AMSTRAD_HELPER( 0x0400, "L-Shift",      KEYCODE_LSHIFT,     CODE_NONE )
	AMSTRAD_HELPER( 0x0800, "\\ |",         KEYCODE_BACKSLASH,  CODE_NONE )
	AMSTRAD_HELPER( 0x1000, "Z",            KEYCODE_Z,          CODE_NONE )
	AMSTRAD_HELPER( 0x2000, "X",            KEYCODE_X,          CODE_NONE )
	AMSTRAD_HELPER( 0x4000, "C",            KEYCODE_C,          CODE_NONE )
	AMSTRAD_HELPER( 0x8000, "V",            KEYCODE_V,          CODE_NONE )

	PORT_START_TAG("pc_keyboard_3")
	AMSTRAD_HELPER( 0x0001, "B",            KEYCODE_B,          CODE_NONE )
	AMSTRAD_HELPER( 0x0002, "N",            KEYCODE_N,          CODE_NONE )
	AMSTRAD_HELPER( 0x0004, "M",            KEYCODE_M,          CODE_NONE )
	AMSTRAD_HELPER( 0x0008, ", <",          KEYCODE_COMMA,      CODE_NONE )
	AMSTRAD_HELPER( 0x0010, ". >",          KEYCODE_STOP,       CODE_NONE )
	AMSTRAD_HELPER( 0x0020, "/ ?",          KEYCODE_SLASH,      CODE_NONE )
	AMSTRAD_HELPER( 0x0040, "R-Shift",      KEYCODE_RSHIFT,     CODE_NONE )
	AMSTRAD_HELPER( 0x0080, "KP * (PrtScr)",KEYCODE_ASTERISK,   CODE_NONE )
	AMSTRAD_HELPER( 0x0100, "Alt",          KEYCODE_LALT,       CODE_NONE )
	AMSTRAD_HELPER( 0x0200, "Space",        KEYCODE_SPACE,      CODE_NONE )
	AMSTRAD_HELPER( 0x0400, "Caps",         KEYCODE_CAPSLOCK,   CODE_NONE )
	AMSTRAD_HELPER( 0x0800, "F1",           KEYCODE_F1,         CODE_NONE )
	AMSTRAD_HELPER( 0x1000, "F2",           KEYCODE_F2,         CODE_NONE )
	AMSTRAD_HELPER( 0x2000, "F3",           KEYCODE_F3,         CODE_NONE )
	AMSTRAD_HELPER( 0x4000, "F4",           KEYCODE_F4,         CODE_NONE )
	AMSTRAD_HELPER( 0x8000, "F5",           KEYCODE_F5,         CODE_NONE )

	PORT_START_TAG("pc_keyboard_4")
	AMSTRAD_HELPER( 0x0001, "F6",           KEYCODE_F6,         CODE_NONE )
	AMSTRAD_HELPER( 0x0002, "F7",           KEYCODE_F7,         CODE_NONE )
	AMSTRAD_HELPER( 0x0004, "F8",           KEYCODE_F8,         CODE_NONE )
	AMSTRAD_HELPER( 0x0008, "F9",           KEYCODE_F9,         CODE_NONE )
	AMSTRAD_HELPER( 0x0010, "F10",          KEYCODE_F10,        CODE_NONE )
	AMSTRAD_HELPER( 0x0020, "NumLock",      KEYCODE_NUMLOCK,    CODE_NONE )
	AMSTRAD_HELPER( 0x0040, "ScrLock",      KEYCODE_SCRLOCK,    CODE_NONE )
	AMSTRAD_HELPER( 0x0080, "KP 7 (Home)",  KEYCODE_7_PAD,      KEYCODE_HOME )
	AMSTRAD_HELPER( 0x0100, "KP 8 (Up)",    KEYCODE_8_PAD,      CODE_NONE )
	AMSTRAD_HELPER( 0x0200, "KP 9 (PgUp)",  KEYCODE_9_PAD,      KEYCODE_PGUP)
	AMSTRAD_HELPER( 0x0400, "KP -",         KEYCODE_MINUS_PAD,  CODE_NONE )
	AMSTRAD_HELPER( 0x0800, "KP 4 (Left)",  KEYCODE_4_PAD,      CODE_NONE )
	AMSTRAD_HELPER( 0x1000, "KP 5",         KEYCODE_5_PAD,      CODE_NONE )
	AMSTRAD_HELPER( 0x2000, "KP 6 (Right)", KEYCODE_6_PAD,      CODE_NONE )
	AMSTRAD_HELPER( 0x4000, "KP +",         KEYCODE_PLUS_PAD,   CODE_NONE )
	AMSTRAD_HELPER( 0x8000, "KP 1 (End)",   KEYCODE_1_PAD,      KEYCODE_END )

	PORT_START_TAG("pc_keyboard_5")
	AMSTRAD_HELPER( 0x0001, "KP 2 (Down)",  KEYCODE_2_PAD,      CODE_NONE )
	AMSTRAD_HELPER( 0x0002, "KP 3 (PgDn)",  KEYCODE_3_PAD,      KEYCODE_PGDN )
	AMSTRAD_HELPER( 0x0004, "KP 0 (Ins)",   KEYCODE_0_PAD,      KEYCODE_INSERT )
	AMSTRAD_HELPER( 0x0008, "KP . (Del)",   KEYCODE_DEL_PAD,    KEYCODE_DEL )
	PORT_BIT ( 0x0030, 0x0000, IPT_UNUSED )
	AMSTRAD_HELPER( 0x0040, "?(84/102)\\",	KEYCODE_BACKSLASH2,	CODE_NONE )
	PORT_BIT ( 0xff80, 0x0000, IPT_UNUSED )

	PORT_START_TAG("pc_keyboard_6")
	PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )

	PORT_START_TAG("pc_keyboard_7")
	PORT_BIT ( 0x806e, 0x0000, IPT_UNUSED )
	AMSTRAD_HELPER( 0x0001, "-->",			CODE_DEFAULT,    CODE_NONE )
	AMSTRAD_HELPER( 0x0010,	"?Enter",		CODE_DEFAULT,    CODE_NONE )
	AMSTRAD_HELPER( 0x0100,	"Amstrad Joystick Button 1",KEYCODE_RALT, CODE_NONE )
	AMSTRAD_HELPER( 0x0080,	"Amstrad Joystick Button 2",KEYCODE_RCONTROL, CODE_NONE )
	AMSTRAD_HELPER( 0x0200,	"Amstrad Joystick Right",	KEYCODE_RIGHT, CODE_NONE )
	AMSTRAD_HELPER( 0x0400,	"Amstrad Joystick Left",	KEYCODE_LEFT, CODE_NONE )
	AMSTRAD_HELPER( 0x0800,	"Amstrad Joystick Down",	KEYCODE_DOWN, CODE_NONE )
	AMSTRAD_HELPER( 0x1000,	"Amstrad Joystick Up",		KEYCODE_UP, CODE_NONE )
	AMSTRAD_HELPER( 0x4000,	"Amstrad Mouse Button left",	KEYCODE_F11, CODE_NONE )
	AMSTRAD_HELPER( 0x2000,	"Amstrad Mouse Button right",	KEYCODE_F12, CODE_NONE )

	PORT_START_TAG("pc_mouse_misc")
	PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )

	PORT_START_TAG( "pc_mouse_x" ) /* Mouse - X AXIS */  
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_MINMAX(0,0) PORT_PLAYER(1) PORT_REVERSE

	PORT_START_TAG( "pc_mouse_y" ) /* Mouse - Y AXIS */
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_MINMAX(0,0) PORT_PLAYER(1)

INPUT_PORTS_END
