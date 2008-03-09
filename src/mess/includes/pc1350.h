/*****************************************************************************
 *
 * includes/pc1350.h
 *
 * Pocket Computer 1350
 *
 ****************************************************************************/

#ifndef PC1350_H_
#define PC1350_H_


#define PC1350_KEY_OFF readinputport(0)&0x80
#define PC1350_KEY_DOWN readinputport(0)&0x40
#define PC1350_KEY_UP readinputport(0)&0x20
#define PC1350_KEY_MODE readinputport(0)&0x10
#define PC1350_KEY_CLS readinputport(0)&8
#define PC1350_KEY_LEFT readinputport(0)&4
#define PC1350_KEY_RIGHT readinputport(0)&2
#define PC1350_KEY_DEL readinputport(0)&1

#define PC1350_KEY_INS readinputport(1)&0x80
#define PC1350_KEY_BRK readinputport(1)&0x40
#define PC1350_KEY_RSHIFT readinputport(1)&0x20
#define PC1350_KEY_7 readinputport(1)&0x10
#define PC1350_KEY_8 readinputport(1)&8
#define PC1350_KEY_9 readinputport(1)&4
#define PC1350_KEY_BRACE_OPEN readinputport(1)&2
#define PC1350_KEY_BRACE_CLOSE readinputport(1)&1

#define PC1350_KEY_4 readinputport(2)&0x80
#define PC1350_KEY_5 readinputport(2)&0x40
#define PC1350_KEY_6 readinputport(2)&0x20
#define PC1350_KEY_SLASH readinputport(2)&0x10
#define PC1350_KEY_COLON readinputport(2)&8
#define PC1350_KEY_1 readinputport(2)&4
#define PC1350_KEY_2 readinputport(2)&2
#define PC1350_KEY_3 readinputport(2)&1

#define PC1350_KEY_ASTERIX readinputport(3)&0x80
#define PC1350_KEY_SEMICOLON readinputport(3)&0x40
#define PC1350_KEY_0 readinputport(3)&0x20
#define PC1350_KEY_POINT readinputport(3)&0x10
#define PC1350_KEY_PLUS readinputport(3)&8
#define PC1350_KEY_MINUS readinputport(3)&4
#define PC1350_KEY_COMMA readinputport(3)&2
#define PC1350_KEY_LSHIFT readinputport(3)&1

#define PC1350_KEY_Q readinputport(4)&0x80
#define PC1350_KEY_W readinputport(4)&0x40
#define PC1350_KEY_E readinputport(4)&0x20
#define PC1350_KEY_R readinputport(4)&0x10
#define PC1350_KEY_T readinputport(4)&8
#define PC1350_KEY_Y readinputport(4)&4
#define PC1350_KEY_U readinputport(4)&2
#define PC1350_KEY_I readinputport(4)&1

#define PC1350_KEY_O readinputport(5)&0x80
#define PC1350_KEY_P readinputport(5)&0x40
#define PC1350_KEY_DEF readinputport(5)&0x20
#define PC1350_KEY_A readinputport(5)&0x10
#define PC1350_KEY_S readinputport(5)&8
#define PC1350_KEY_D readinputport(5)&4
#define PC1350_KEY_F readinputport(5)&2
#define PC1350_KEY_G readinputport(5)&1

#define PC1350_KEY_H readinputport(6)&0x80
#define PC1350_KEY_J readinputport(6)&0x40
#define PC1350_KEY_K readinputport(6)&0x20
#define PC1350_KEY_L readinputport(6)&0x10
#define PC1350_KEY_EQUALS readinputport(6)&8
#define PC1350_KEY_SML readinputport(6)&4
#define PC1350_KEY_Z readinputport(6)&2
#define PC1350_KEY_X readinputport(6)&1

#define PC1350_KEY_C readinputport(7)&0x80
#define PC1350_KEY_V readinputport(7)&0x40
#define PC1350_KEY_B readinputport(7)&0x20
#define PC1350_KEY_N readinputport(7)&0x10
#define PC1350_KEY_M readinputport(7)&8
#define PC1350_KEY_SPACE readinputport(7)&4
#define PC1350_KEY_ENTER readinputport(7)&2

#define PC1350_CONTRAST (readinputport(8)&7)


/*----------- defined in machine/pc1350.c -----------*/

void pc1350_outa(int data);
void pc1350_outb(int data);
void pc1350_outc(int data);

int pc1350_brk(void);
int pc1350_ina(void);
int pc1350_inb(void);

MACHINE_START( pc1350 );
NVRAM_HANDLER( pc1350 );


/*----------- defined in video/pc1350.c -----------*/

READ8_HANDLER(pc1350_lcd_read);
WRITE8_HANDLER(pc1350_lcd_write);
VIDEO_UPDATE( pc1350 );

int pc1350_keyboard_line_r(void);


#endif /* PC1350_H_ */
