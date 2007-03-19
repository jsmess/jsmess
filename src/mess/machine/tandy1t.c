#include "driver.h"

#include "machine/pckeybrd.h"

#include "machine/pcshare.h"
#include "includes/tandy1t.h"


/* tandy 1000 eeprom
  hx and later
  clock, and data out lines at 0x37c
  data in at 0x62 bit 4 (0x10)

  8x read 16 bit word at x
  30 cx 30 4x 16bit 00 write 16bit at x
*/
static struct {
	int state;
	int clock;
	UINT8 oper;
	UINT16 data;
	struct {
		UINT8 low, high;
	} ee[0x40]; /* only 0 to 4 used in hx, addressing seems to allow this */
} eeprom={0};

NVRAM_HANDLER( tandy1000 )
{
	if (file==NULL) {
		// init only 
	} else if (read_or_write) {
		mame_fwrite(file, eeprom.ee, sizeof(eeprom.ee));
	} else {
		mame_fread(file, eeprom.ee, sizeof(eeprom.ee));
	}
}

static int tandy1000_read_eeprom(void)
{
	if ((eeprom.state>=100)&&(eeprom.state<=199))
		return eeprom.data&0x8000;
	return 1;
}

static void tandy1000_write_eeprom(UINT8 data)
{
	if (!eeprom.clock && (data&4) ) {
//				logerror("!!!tandy1000 eeprom %.2x %.2x\n",eeprom.state, data);
		switch (eeprom.state) {
		case 0:
			if ((data&3)==0) eeprom.state++;
			break;
		case 1:
			if ((data&3)==2) eeprom.state++;
			break;
		case 2:
			if ((data&3)==3) eeprom.state++;
			break;
		case 3:
			eeprom.oper=data&1;
			eeprom.state++;
			break;
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
			eeprom.oper=(eeprom.oper<<1)|(data&1);
			eeprom.state++;
			break;
		case 10:
			eeprom.oper=(eeprom.oper<<1)|(data&1);
			logerror("!!!tandy1000 eeprom %.2x\n",eeprom.oper);
			if ((eeprom.oper&0xc0)==0x80) {
				eeprom.state=100;
				eeprom.data=eeprom.ee[eeprom.oper&0x3f].low
					|(eeprom.ee[eeprom.oper&0x3f].high<<8);
				logerror("!!!tandy1000 eeprom read %.2x,%.4x\n",eeprom.oper,eeprom.data);
			} else if ((eeprom.oper&0xc0)==0x40) {
				eeprom.state=200;
			} else eeprom.state=0;
			break;
			
			/* read 16 bit */
		case 100:
			eeprom.state++;
			break;
		case 101:
		case 102:
		case 103:
		case 104:
		case 105:
		case 106:
		case 107:
		case 108:
		case 109:
		case 110:
		case 111:
		case 112:
		case 113:
		case 114:
		case 115:
			eeprom.data<<=1;
			eeprom.state++;
			break;
		case 116:
			eeprom.data<<=1;
			eeprom.state=0;
			break;
			
			/* write 16 bit */
		case 200:
		case 201:
		case 202:
		case 203:
		case 204:
		case 205:
		case 206:
		case 207:
		case 208:
		case 209:
		case 210:
		case 211:
		case 212:
		case 213:
		case 214:
			eeprom.data=(eeprom.data<<1)|(data&1);
			eeprom.state++;
			break;
		case 215:
			eeprom.data=(eeprom.data<<1)|(data&1);
			logerror("tandy1000 %.2x %.4x written\n",eeprom.oper,eeprom.data);
			eeprom.ee[eeprom.oper&0x3f].low=eeprom.data&0xff;
			eeprom.ee[eeprom.oper&0x3f].high=eeprom.data>>8;
			eeprom.state=0;
			break;
		}
	}
	eeprom.clock=data&4;
}

static struct {
	UINT8 data[8];
} tandy={ {0}};

WRITE8_HANDLER ( pc_t1t_p37x_w )
{
//	DBG_LOG(2,"T1T_p37x_w",("%.5x #%d $%02x\n", activecpu_get_pc(),offset, data));
	if (offset!=4)
		logerror("T1T_p37x_w %.5x #%d $%02x\n", activecpu_get_pc(),offset, data);
	tandy.data[offset]=data;
	switch( offset )
	{
		case 4:
			tandy1000_write_eeprom(data);
			break;
	}
}

 READ8_HANDLER ( pc_t1t_p37x_r )
{
	int data = tandy.data[offset];
//	DBG_LOG(1,"T1T_p37x_r",("%.5x #%d $%02x\n", activecpu_get_pc(), offset, data));
    return data;
}

/* this is for tandy1000hx
   hopefully this works for all x models
   must not be a ppi8255 chip
   (think custom chip)
   port c:
   bit 4 input eeprom data in
   bit 3 output turbo mode
*/
static struct {
	UINT8 portb, portc;
} tandy_ppi= { 0 };

WRITE8_HANDLER ( tandy1000_pio_w )
{
	switch (offset) {
	case 1:
		tandy_ppi.portb = data;
		pc_sh_speaker(data&3);
		pc_keyb_set_clock(data&0x40);
		break;
	case 2:
		tandy_ppi.portc = data;
		if (data&8) cpunum_set_clockscale(0, 1);
		else cpunum_set_clockscale(0, 4.77/8);
		break;
	}
}

 READ8_HANDLER(tandy1000_pio_r)
{
	int data=0xff;
	switch (offset) {
	case 0:
		data = pc_keyb_read();
		break;
	case 1:
		data=tandy_ppi.portb;
		break;
	case 2:
//		if (tandy1000hx) {
//		data=tandy_ppi.portc; // causes problems (setuphx)
		if (!tandy1000_read_eeprom()) data&=~0x10;
//	}
		break;
	}
	return data;
}



#define T1000_HELPER(bit,text,key1,key2) \
	PORT_BIT( bit, 0x0000, IPT_KEYBOARD) PORT_NAME(text) PORT_CODE(key1) PORT_CODE(key2)

INPUT_PORTS_START( t1000_keyboard )
	PORT_START	/* IN4 */
	PORT_BIT ( 0x0001, 0x0000, IPT_UNUSED ) 	/* unused scancode 0 */
	T1000_HELPER( 0x0002, "Esc",          KEYCODE_ESC,        CODE_NONE ) /* Esc                         01  81 */
	T1000_HELPER( 0x0004, "1 !",          KEYCODE_1,          CODE_NONE ) /* 1                           02  82 */
	T1000_HELPER( 0x0008, "2 @",          KEYCODE_2,          CODE_NONE ) /* 2                           03  83 */
	T1000_HELPER( 0x0010, "3 #",          KEYCODE_3,          CODE_NONE ) /* 3                           04  84 */
	T1000_HELPER( 0x0020, "4 $",          KEYCODE_4,          CODE_NONE ) /* 4                           05  85 */
	T1000_HELPER( 0x0040, "5 %",          KEYCODE_5,          CODE_NONE ) /* 5                           06  86 */
	T1000_HELPER( 0x0080, "6 ^",          KEYCODE_6,          CODE_NONE ) /* 6                           07  87 */
	T1000_HELPER( 0x0100, "7 &",          KEYCODE_7,          CODE_NONE ) /* 7                           08  88 */
	T1000_HELPER( 0x0200, "8 *",          KEYCODE_8,          CODE_NONE ) /* 8                           09  89 */
	T1000_HELPER( 0x0400, "9 (",          KEYCODE_9,          CODE_NONE ) /* 9                           0A  8A */
	T1000_HELPER( 0x0800, "0 )",          KEYCODE_0,          CODE_NONE ) /* 0                           0B  8B */
	T1000_HELPER( 0x1000, "- _",          KEYCODE_MINUS,      CODE_NONE ) /* -                           0C  8C */
	T1000_HELPER( 0x2000, "= +",          KEYCODE_EQUALS,     CODE_NONE ) /* =                           0D  8D */
	T1000_HELPER( 0x4000, "<--",          KEYCODE_BACKSPACE,  CODE_NONE ) /* Backspace                   0E  8E */
	T1000_HELPER( 0x8000, "Tab",          KEYCODE_TAB,        CODE_NONE ) /* Tab                         0F  8F */
		
	PORT_START	/* IN5 */
	T1000_HELPER( 0x0001, "Q",            KEYCODE_Q,          CODE_NONE ) /* Q                           10  90 */
	T1000_HELPER( 0x0002, "W",            KEYCODE_W,          CODE_NONE ) /* W                           11  91 */
	T1000_HELPER( 0x0004, "E",            KEYCODE_E,          CODE_NONE ) /* E                           12  92 */
	T1000_HELPER( 0x0008, "R",            KEYCODE_R,          CODE_NONE ) /* R                           13  93 */
	T1000_HELPER( 0x0010, "T",            KEYCODE_T,          CODE_NONE ) /* T                           14  94 */
	T1000_HELPER( 0x0020, "Y",            KEYCODE_Y,          CODE_NONE ) /* Y                           15  95 */
	T1000_HELPER( 0x0040, "U",            KEYCODE_U,          CODE_NONE ) /* U                           16  96 */
	T1000_HELPER( 0x0080, "I",            KEYCODE_I,          CODE_NONE ) /* I                           17  97 */
	T1000_HELPER( 0x0100, "O",            KEYCODE_O,          CODE_NONE ) /* O                           18  98 */
	T1000_HELPER( 0x0200, "P",            KEYCODE_P,          CODE_NONE ) /* P                           19  99 */
	T1000_HELPER( 0x0400, "[ {",          KEYCODE_OPENBRACE,  CODE_NONE ) /* [                           1A  9A */
	T1000_HELPER( 0x0800, "] }",          KEYCODE_CLOSEBRACE, CODE_NONE ) /* ]                           1B  9B */
	T1000_HELPER( 0x1000, "Enter",        KEYCODE_ENTER,      CODE_NONE ) /* Enter                       1C  9C */
	T1000_HELPER( 0x2000, "L-Ctrl",       KEYCODE_LCONTROL,   CODE_NONE ) /* Left Ctrl                   1D  9D */
	T1000_HELPER( 0x4000, "A",            KEYCODE_A,          CODE_NONE ) /* A                           1E  9E */
	T1000_HELPER( 0x8000, "S",            KEYCODE_S,          CODE_NONE ) /* S                           1F  9F */
		
	PORT_START	/* IN6 */
	T1000_HELPER( 0x0001, "D",            KEYCODE_D,          CODE_NONE ) /* D                           20  A0 */
	T1000_HELPER( 0x0002, "F",            KEYCODE_F,          CODE_NONE ) /* F                           21  A1 */
	T1000_HELPER( 0x0004, "G",            KEYCODE_G,          CODE_NONE ) /* G                           22  A2 */
	T1000_HELPER( 0x0008, "H",            KEYCODE_H,          CODE_NONE ) /* H                           23  A3 */
	T1000_HELPER( 0x0010, "J",            KEYCODE_J,          CODE_NONE ) /* J                           24  A4 */
	T1000_HELPER( 0x0020, "K",            KEYCODE_K,          CODE_NONE ) /* K                           25  A5 */
	T1000_HELPER( 0x0040, "L",            KEYCODE_L,          CODE_NONE ) /* L                           26  A6 */
	T1000_HELPER( 0x0080, "; :",          KEYCODE_COLON,      CODE_NONE ) /* ;                           27  A7 */
	T1000_HELPER( 0x0100, "' \"",         KEYCODE_QUOTE,      CODE_NONE ) /* '                           28  A8 */
	T1000_HELPER( 0x0200, "Cursor Up",    KEYCODE_UP,		  CODE_NONE ) /*                             29  A9 */
	T1000_HELPER( 0x0400, "L-Shift",      KEYCODE_LSHIFT,     CODE_NONE ) /* Left Shift                  2A  AA */
	T1000_HELPER( 0x0800, "Cursor Left",  KEYCODE_LEFT,		  CODE_NONE ) /*                             2B  AB */
	T1000_HELPER( 0x1000, "Z",            KEYCODE_Z,          CODE_NONE ) /* Z                           2C  AC */
	T1000_HELPER( 0x2000, "X",            KEYCODE_X,          CODE_NONE ) /* X                           2D  AD */
	T1000_HELPER( 0x4000, "C",            KEYCODE_C,          CODE_NONE ) /* C                           2E  AE */
	T1000_HELPER( 0x8000, "V",            KEYCODE_V,          CODE_NONE ) /* V                           2F  AF */
		
	PORT_START	/* IN7 */
	T1000_HELPER( 0x0001, "B",            KEYCODE_B,          CODE_NONE ) /* B                           30  B0 */
	T1000_HELPER( 0x0002, "N",            KEYCODE_N,          CODE_NONE ) /* N                           31  B1 */
	T1000_HELPER( 0x0004, "M",            KEYCODE_M,          CODE_NONE ) /* M                           32  B2 */
	T1000_HELPER( 0x0008, ", <",          KEYCODE_COMMA,      CODE_NONE ) /* ,                           33  B3 */
	T1000_HELPER( 0x0010, ". >",          KEYCODE_STOP,       CODE_NONE ) /* .                           34  B4 */
	T1000_HELPER( 0x0020, "/ ?",          KEYCODE_SLASH,      CODE_NONE ) /* /                           35  B5 */
	T1000_HELPER( 0x0040, "R-Shift",      KEYCODE_RSHIFT,     CODE_NONE ) /* Right Shift                 36  B6 */
	T1000_HELPER( 0x0080, "Print",		  CODE_NONE,		  CODE_NONE ) /*                             37  B7 */
	T1000_HELPER( 0x0100, "Alt",          KEYCODE_LALT,       CODE_NONE ) /* Left Alt                    38  B8 */
	T1000_HELPER( 0x0200, "Space",        KEYCODE_SPACE,      CODE_NONE ) /* Space                       39  B9 */
	T1000_HELPER( 0x0400, "Caps",         KEYCODE_CAPSLOCK,   CODE_NONE ) /* Caps Lock                   3A  BA */
	T1000_HELPER( 0x0800, "F1",           KEYCODE_F1,         CODE_NONE ) /* F1                          3B  BB */
	T1000_HELPER( 0x1000, "F2",           KEYCODE_F2,         CODE_NONE ) /* F2                          3C  BC */
	T1000_HELPER( 0x2000, "F3",           KEYCODE_F3,         CODE_NONE ) /* F3                          3D  BD */
	T1000_HELPER( 0x4000, "F4",           KEYCODE_F4,         CODE_NONE ) /* F4                          3E  BE */
	T1000_HELPER( 0x8000, "F5",           KEYCODE_F5,         CODE_NONE ) /* F5                          3F  BF */
		
	PORT_START	/* IN8 */
	T1000_HELPER( 0x0001, "F6",           KEYCODE_F6,         CODE_NONE ) /* F6                          40  C0 */
	T1000_HELPER( 0x0002, "F7",           KEYCODE_F7,         CODE_NONE ) /* F7                          41  C1 */
	T1000_HELPER( 0x0004, "F8",           KEYCODE_F8,         CODE_NONE ) /* F8                          42  C2 */
	T1000_HELPER( 0x0008, "F9",           KEYCODE_F9,         CODE_NONE ) /* F9                          43  C3 */
	T1000_HELPER( 0x0010, "F10",          KEYCODE_F10,        CODE_NONE ) /* F10                         44  C4 */
	T1000_HELPER( 0x0020, "NumLock",      KEYCODE_NUMLOCK,    CODE_NONE ) /* Num Lock                    45  C5 */
	T1000_HELPER( 0x0040, "Hold",		  KEYCODE_SCRLOCK,    CODE_NONE ) /*		                     46  C6 */
	T1000_HELPER( 0x0080, "KP 7 /",		  KEYCODE_7_PAD,      CODE_NONE ) /* Keypad 7                    47  C7 */
	T1000_HELPER( 0x0100, "KP 8 ~",		  KEYCODE_8_PAD,      CODE_NONE ) /* Keypad 8                    48  C8 */
	T1000_HELPER( 0x0200, "KP 9 (PgUp)",  KEYCODE_9_PAD,      CODE_NONE ) /* Keypad 9  (PgUp)            49  C9 */
	T1000_HELPER( 0x0400, "Cursor Down",  KEYCODE_DOWN,		  CODE_NONE ) /*                             4A  CA */
	T1000_HELPER( 0x0800, "KP 4 |",		  KEYCODE_4_PAD,	  CODE_NONE ) /* Keypad 4                    4B  CB */
	T1000_HELPER( 0x1000, "KP 5",         KEYCODE_5_PAD,      CODE_NONE ) /* Keypad 5                    4C  CC */
	T1000_HELPER( 0x2000, "KP 6",		  KEYCODE_6_PAD,	  CODE_NONE ) /* Keypad 6                    4D  CD */
	T1000_HELPER( 0x4000, "Cursor Right", KEYCODE_RIGHT,	  CODE_NONE ) /*                             4E  CE */
	T1000_HELPER( 0x8000, "KP 1 (End)",   KEYCODE_1_PAD,      CODE_NONE ) /* Keypad 1  (End)             4F  CF */
		
	PORT_START	/* IN9 */
	T1000_HELPER( 0x0001, "KP 2 `",		  KEYCODE_2_PAD,	  CODE_NONE ) /* Keypad 2                    50  D0 */
	T1000_HELPER( 0x0002, "KP 3 (PgDn)",  KEYCODE_3_PAD,      CODE_NONE ) /* Keypad 3  (PgDn)            51  D1 */
	T1000_HELPER( 0x0004, "KP 0",		  KEYCODE_0_PAD,      CODE_NONE ) /* Keypad 0                    52  D2 */
	T1000_HELPER( 0x0008, "KP - (Del)",   KEYCODE_MINUS_PAD,  CODE_NONE ) /* - Delete                    53  D3 */
	T1000_HELPER( 0x0010, "Break",		  KEYCODE_STOP,       CODE_NONE ) /* Break                       54  D4 */
	T1000_HELPER( 0x0020, "+ Insert",	  KEYCODE_PLUS_PAD,	  CODE_NONE ) /* + Insert                    55  D5 */
	T1000_HELPER( 0x0040, ".",			  KEYCODE_DEL_PAD,    CODE_NONE ) /* .                           56  D6 */
	T1000_HELPER( 0x0080, "Enter",		  KEYCODE_ENTER_PAD,  CODE_NONE ) /* Enter                       57  D7 */
	T1000_HELPER( 0x0100, "Home",		  KEYCODE_HOME,       CODE_NONE ) /* HOME                        58  D8 */
	T1000_HELPER( 0x0200, "F11",		  KEYCODE_F11,        CODE_NONE ) /* F11                         59  D9 */
	T1000_HELPER( 0x0400, "F12",		  KEYCODE_F12,        CODE_NONE ) /* F12                         5a  Da */
		
	PORT_START	/* IN10 */
	PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )
		
	PORT_START	/* IN11 */
	PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )
INPUT_PORTS_END
