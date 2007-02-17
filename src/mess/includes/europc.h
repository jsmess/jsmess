WRITE8_HANDLER( europc_pio_w );
 READ8_HANDLER( europc_pio_r );

extern WRITE8_HANDLER ( europc_jim_w );
extern  READ8_HANDLER ( europc_jim_r );
extern  READ8_HANDLER ( europc_jim2_r );

extern  READ8_HANDLER( europc_rtc_r );
extern WRITE8_HANDLER( europc_rtc_w );
extern NVRAM_HANDLER( europc_rtc );

void europc_rtc_set_time(void);
void europc_rtc_init(void);

#define EUROPC_HELPER(bit,text,key1,key2) \
	PORT_BIT( bit, 0x0000, IPT_KEYBOARD) PORT_NAME(text) PORT_CODE(key1) PORT_CODE(key2)

/*
layout of an uk europc

ESC, [SPACE], F1,F2,F3,F4,[SPACE],F5,F6,F7,F8,[SPACE],F9,F0,F11,F12
[SPACE]
\|, 1,2,3,4,5,6,7,8,9,0 -,+, BACKSPACE,[SPACE], NUM LOCK, SCROLL LOCK, PRINT SCREEN, KEYPAD - 
TAB,Q,W,E,R,T,Y,U,I,O,P,[,], RETURN, [SPACE], KEYPAD 7, KEYPAD 8, KEYPAD 9, KEYPAD + 
CTRL, A,S,D,F,G,H,J,K,L,;,@,~, RETURN, [SPACE],KEYPAD 4,KEYPAD 5,KEYPAD 6, KEYPAD +
LEFT SHIFT, Z,X,C,V,B,N,M,<,>,?,RIGHT SHIFT,[SPACE],KEYPAD 1, KEYPAD 2, KEYPAD 3, KEYPAD ENTER
ALT,[SPACE], SPACE BAR,[SPACE],CAPS LOCK,[SPACE], KEYPAD 0, KEYPAD ., KEYPAD ENTER

\ and ~ had to be swapped
i am not sure if keypad enter delivers the mf2 keycode
 */
#define EUROPC_KEYBOARD \
    PORT_START_TAG("pc_keyboard_0")  /* IN4 */\
	PORT_BIT ( 0x0001, 0x0000, IPT_UNUSED ) 	/* unused scancode 0 */\
	EUROPC_HELPER( 0x0002, "Esc",          KEYCODE_ESC,        CODE_NONE ) /* Esc                         01  81 */\
	EUROPC_HELPER( 0x0004, "1 !",          KEYCODE_1,          CODE_NONE ) /* 1                           02  82 */\
	EUROPC_HELPER( 0x0008, "2 @",          KEYCODE_2,          CODE_NONE ) /* 2                           03  83 */\
	EUROPC_HELPER( 0x0010, "3 #",          KEYCODE_3,          CODE_NONE ) /* 3                           04  84 */\
	EUROPC_HELPER( 0x0020, "4 $",          KEYCODE_4,          CODE_NONE ) /* 4                           05  85 */\
	EUROPC_HELPER( 0x0040, "5 %",          KEYCODE_5,          CODE_NONE ) /* 5                           06  86 */\
	EUROPC_HELPER( 0x0080, "6 ^",          KEYCODE_6,          CODE_NONE ) /* 6                           07  87 */\
	EUROPC_HELPER( 0x0100, "7 &",          KEYCODE_7,          CODE_NONE ) /* 7                           08  88 */\
	EUROPC_HELPER( 0x0200, "8 *",          KEYCODE_8,          CODE_NONE ) /* 8                           09  89 */\
	EUROPC_HELPER( 0x0400, "9 (",          KEYCODE_9,          CODE_NONE ) /* 9                           0A  8A */\
	EUROPC_HELPER( 0x0800, "0 )",          KEYCODE_0,          CODE_NONE ) /* 0                           0B  8B */\
	EUROPC_HELPER( 0x1000, "- _",          KEYCODE_MINUS,      CODE_NONE ) /* -                           0C  8C */\
	EUROPC_HELPER( 0x2000, "= +",          KEYCODE_EQUALS,     CODE_NONE ) /* =                           0D  8D */\
	EUROPC_HELPER( 0x4000, "<--",          KEYCODE_BACKSPACE,  CODE_NONE ) /* Backspace                   0E  8E */\
	EUROPC_HELPER( 0x8000, "Tab",          KEYCODE_TAB,        CODE_NONE ) /* Tab                         0F  8F */\
		\
	PORT_START_TAG("pc_keyboard_1")	/* IN5 */\
	EUROPC_HELPER( 0x0001, "Q",            KEYCODE_Q,          CODE_NONE ) /* Q                           10  90 */\
	EUROPC_HELPER( 0x0002, "W",            KEYCODE_W,          CODE_NONE ) /* W                           11  91 */\
	EUROPC_HELPER( 0x0004, "E",            KEYCODE_E,          CODE_NONE ) /* E                           12  92 */\
	EUROPC_HELPER( 0x0008, "R",            KEYCODE_R,          CODE_NONE ) /* R                           13  93 */\
	EUROPC_HELPER( 0x0010, "T",            KEYCODE_T,          CODE_NONE ) /* T                           14  94 */\
	EUROPC_HELPER( 0x0020, "Y",            KEYCODE_Y,          CODE_NONE ) /* Y                           15  95 */\
	EUROPC_HELPER( 0x0040, "U",            KEYCODE_U,          CODE_NONE ) /* U                           16  96 */\
	EUROPC_HELPER( 0x0080, "I",            KEYCODE_I,          CODE_NONE ) /* I                           17  97 */\
	EUROPC_HELPER( 0x0100, "O",            KEYCODE_O,          CODE_NONE ) /* O                           18  98 */\
	EUROPC_HELPER( 0x0200, "P",            KEYCODE_P,          CODE_NONE ) /* P                           19  99 */\
	EUROPC_HELPER( 0x0400, "[ {",          KEYCODE_OPENBRACE,  CODE_NONE ) /* [                           1A  9A */\
	EUROPC_HELPER( 0x0800, "] }",          KEYCODE_CLOSEBRACE, CODE_NONE ) /* ]                           1B  9B */\
	EUROPC_HELPER( 0x1000, "Enter",        KEYCODE_ENTER,      CODE_NONE ) /* Enter                       1C  9C */\
	EUROPC_HELPER( 0x2000, "Ctrl",         KEYCODE_LCONTROL,   KEYCODE_RCONTROL ) /* Ctrl                   1D  9D */\
	EUROPC_HELPER( 0x4000, "A",            KEYCODE_A,          CODE_NONE ) /* A                           1E  9E */\
	EUROPC_HELPER( 0x8000, "S",            KEYCODE_S,          CODE_NONE ) /* S                           1F  9F */\
		\
	PORT_START_TAG("pc_keyboard_2")	/* IN6 */\
	EUROPC_HELPER( 0x0001, "D",            KEYCODE_D,          CODE_NONE ) /* D                           20  A0 */\
	EUROPC_HELPER( 0x0002, "F",            KEYCODE_F,          CODE_NONE ) /* F                           21  A1 */\
	EUROPC_HELPER( 0x0004, "G",            KEYCODE_G,          CODE_NONE ) /* G                           22  A2 */\
	EUROPC_HELPER( 0x0008, "H",            KEYCODE_H,          CODE_NONE ) /* H                           23  A3 */\
	EUROPC_HELPER( 0x0010, "J",            KEYCODE_J,          CODE_NONE ) /* J                           24  A4 */\
	EUROPC_HELPER( 0x0020, "K",            KEYCODE_K,          CODE_NONE ) /* K                           25  A5 */\
	EUROPC_HELPER( 0x0040, "L",            KEYCODE_L,          CODE_NONE ) /* L                           26  A6 */\
	EUROPC_HELPER( 0x0080, "; :",          KEYCODE_COLON,      CODE_NONE ) /* ;                           27  A7 */\
	EUROPC_HELPER( 0x0100, "' \"",         KEYCODE_QUOTE,      CODE_NONE ) /* '                           28  A8 */\
	EUROPC_HELPER( 0x0200, "` ~",          KEYCODE_BACKSLASH,  CODE_NONE ) /* `                           29  A9 */\
	EUROPC_HELPER( 0x0400, "L-Shift",      KEYCODE_LSHIFT,     CODE_NONE ) /* Left Shift                  2A  AA */\
	EUROPC_HELPER( 0x0800, "\\ |",         KEYCODE_TILDE,      CODE_NONE ) /* \                           2B  AB */\
	EUROPC_HELPER( 0x1000, "Z",            KEYCODE_Z,          CODE_NONE ) /* Z                           2C  AC */\
	EUROPC_HELPER( 0x2000, "X",            KEYCODE_X,          CODE_NONE ) /* X                           2D  AD */\
	EUROPC_HELPER( 0x4000, "C",            KEYCODE_C,          CODE_NONE ) /* C                           2E  AE */\
	EUROPC_HELPER( 0x8000, "V",            KEYCODE_V,          CODE_NONE ) /* V                           2F  AF */\
		\
	PORT_START_TAG("pc_keyboard_3")	/* IN7 */\
	EUROPC_HELPER( 0x0001, "B",            KEYCODE_B,          CODE_NONE ) /* B                           30  B0 */\
	EUROPC_HELPER( 0x0002, "N",            KEYCODE_N,          CODE_NONE ) /* N                           31  B1 */\
	EUROPC_HELPER( 0x0004, "M",            KEYCODE_M,          CODE_NONE ) /* M                           32  B2 */\
	EUROPC_HELPER( 0x0008, ", <",          KEYCODE_COMMA,      CODE_NONE ) /* ,                           33  B3 */\
	EUROPC_HELPER( 0x0010, ". >",          KEYCODE_STOP,       CODE_NONE ) /* .                           34  B4 */\
	EUROPC_HELPER( 0x0020, "/ ?",          KEYCODE_SLASH,      CODE_NONE ) /* /                           35  B5 */\
	EUROPC_HELPER( 0x0040, "R-Shift",      KEYCODE_RSHIFT,     CODE_NONE ) /* Right Shift                 36  B6 */\
	EUROPC_HELPER( 0x0080, "PrtScr",	   KEYCODE_PRTSCR,     CODE_NONE ) /* Keypad *  (PrtSc)           37  B7 */\
	EUROPC_HELPER( 0x0100, "Alt",          KEYCODE_LALT,       KEYCODE_RALT ) /* Alt                    38  B8 */\
	EUROPC_HELPER( 0x0200, "Space",        KEYCODE_SPACE,      CODE_NONE ) /* Space                       39  B9 */\
	EUROPC_HELPER( 0x0400, "Caps",         KEYCODE_CAPSLOCK,   CODE_NONE ) /* Caps Lock                   3A  BA */\
	EUROPC_HELPER( 0x0800, "F1",           KEYCODE_F1,         CODE_NONE ) /* F1                          3B  BB */\
	EUROPC_HELPER( 0x1000, "F2",           KEYCODE_F2,         CODE_NONE ) /* F2                          3C  BC */\
	EUROPC_HELPER( 0x2000, "F3",           KEYCODE_F3,         CODE_NONE ) /* F3                          3D  BD */\
	EUROPC_HELPER( 0x4000, "F4",           KEYCODE_F4,         CODE_NONE ) /* F4                          3E  BE */\
	EUROPC_HELPER( 0x8000, "F5",           KEYCODE_F5,         CODE_NONE ) /* F5                          3F  BF */\
		\
	PORT_START_TAG("pc_keyboard_4")	/* IN8 */\
	EUROPC_HELPER( 0x0001, "F6",           KEYCODE_F6,         CODE_NONE )     /* F6                          40  C0 */\
	EUROPC_HELPER( 0x0002, "F7",           KEYCODE_F7,         CODE_NONE )     /* F7                          41  C1 */\
	EUROPC_HELPER( 0x0004, "F8",           KEYCODE_F8,         CODE_NONE )     /* F8                          42  C2 */\
	EUROPC_HELPER( 0x0008, "F9",           KEYCODE_F9,         CODE_NONE )     /* F9                          43  C3 */\
	EUROPC_HELPER( 0x0010, "F10",          KEYCODE_F10,        CODE_NONE )     /* F10                         44  C4 */\
	EUROPC_HELPER( 0x0020, "NumLock",      KEYCODE_NUMLOCK,    CODE_NONE )     /* Num Lock                    45  C5 */\
	EUROPC_HELPER( 0x0040, "ScrLock",      KEYCODE_SCRLOCK,    CODE_NONE )     /* Scroll Lock                 46  C6 */\
	EUROPC_HELPER( 0x0080, "KP 7 (Home)",  KEYCODE_7_PAD,      KEYCODE_HOME )  /* Keypad 7  (Home)            47  C7 */\
	EUROPC_HELPER( 0x0100, "KP 8 (Up)",    KEYCODE_8_PAD,      KEYCODE_UP )    /* Keypad 8  (Up arrow)        48  C8 */\
	EUROPC_HELPER( 0x0200, "KP 9 (PgUp)",  KEYCODE_9_PAD,      KEYCODE_PGUP)   /* Keypad 9  (PgUp)            49  C9 */\
	EUROPC_HELPER( 0x0400, "KP -",         KEYCODE_MINUS_PAD,  CODE_NONE )     /* Keypad -                    4A  CA */\
	EUROPC_HELPER( 0x0800, "KP 4 (Left)",  KEYCODE_4_PAD,      KEYCODE_LEFT )  /* Keypad 4  (Left arrow)      4B  CB */\
	EUROPC_HELPER( 0x1000, "KP 5",         KEYCODE_5_PAD,      CODE_NONE )     /* Keypad 5                    4C  CC */\
	EUROPC_HELPER( 0x2000, "KP 6 (Right)", KEYCODE_6_PAD,      KEYCODE_RIGHT ) /* Keypad 6  (Right arrow)     4D  CD */\
	EUROPC_HELPER( 0x4000, "KP +",         KEYCODE_PLUS_PAD,   CODE_NONE )     /* Keypad +                    4E  CE */\
	EUROPC_HELPER( 0x8000, "KP 1 (End)",   KEYCODE_1_PAD,      KEYCODE_END )   /* Keypad 1  (End)             4F  CF */\
		\
	PORT_START_TAG("pc_keyboard_5")	/* IN9 */\
	EUROPC_HELPER( 0x0001, "KP 2 (Down)",  KEYCODE_2_PAD,      KEYCODE_DOWN )   /* Keypad 2  (Down arrow)      50  D0 */\
	EUROPC_HELPER( 0x0002, "KP 3 (PgDn)",  KEYCODE_3_PAD,      KEYCODE_PGDN )   /* Keypad 3  (PgDn)            51  D1 */\
	EUROPC_HELPER( 0x0004, "KP 0 (Ins)",   KEYCODE_0_PAD,      KEYCODE_INSERT ) /* Keypad 0  (Ins)             52  D2 */\
	EUROPC_HELPER( 0x0008, "KP . (Del)",   KEYCODE_DEL_PAD,    KEYCODE_DEL )    /* Keypad .  (Del)             53  D3 */\
	PORT_BIT ( 0x0070, 0x0000, IPT_UNUSED )\
	/* 0x40 non us backslash 2 not available */\
	EUROPC_HELPER( 0x0080, "F11",		   KEYCODE_F11,        CODE_NONE )		/* F11                         57  D7 */\
	EUROPC_HELPER( 0x0100, "F12",	       KEYCODE_F12,        CODE_NONE )		/* F12                         58  D8 */\
	PORT_BIT ( 0xfe00, 0x0000, IPT_UNUSED )\
		\
	PORT_START_TAG("pc_keyboard_6")	/* IN10 */\
	EUROPC_HELPER( 0x0001, "KP Enter",	   KEYCODE_ENTER_PAD,  CODE_NONE )		/* PAD Enter                   60  e0 */\
	PORT_BIT ( 0xfffe, 0x0000, IPT_UNUSED )\
		\
	PORT_START_TAG("pc_keyboard_7")	/* IN11 */\
	PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )

