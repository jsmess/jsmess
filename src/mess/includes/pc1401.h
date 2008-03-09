/*****************************************************************************
 *
 * includes/pc1401.h
 *
 * Pocket Computer 1401
 *
 ****************************************************************************/

#ifndef PC1401_H_
#define PC1401_H_


#define KEY_OFF readinputport(0)&0x80
#define KEY_CAL readinputport(0)&0x40
#define KEY_BASIC readinputport(0)&0x20
#define KEY_BRK readinputport(0)&0x10
#define KEY_DEF readinputport(0)&8
#define KEY_DOWN readinputport(0)&4
#define KEY_UP readinputport(0)&2
#define KEY_LEFT readinputport(0)&1

#define KEY_RIGHT readinputport(1)&0x80
// pc1403 sml
#define KEY_SHIFT readinputport(1)&0x20
#define KEY_Q readinputport(1)&0x10
#define KEY_W readinputport(1)&8
#define KEY_E readinputport(1)&4
#define KEY_R readinputport(1)&2
#define KEY_T readinputport(1)&1

#define KEY_Y readinputport(2)&0x80
#define KEY_U readinputport(2)&0x40
#define KEY_I readinputport(2)&0x20
#define KEY_O readinputport(2)&0x10
#define KEY_P readinputport(2)&8
#define KEY_A readinputport(2)&4
#define KEY_S readinputport(2)&2
#define KEY_D readinputport(2)&1

#define KEY_F readinputport(3)&0x80
#define KEY_G readinputport(3)&0x40
#define KEY_H readinputport(3)&0x20
#define KEY_J readinputport(3)&0x10
#define KEY_K readinputport(3)&8
#define KEY_L readinputport(3)&4
#define KEY_COMMA readinputport(3)&2
#define KEY_Z readinputport(3)&1

#define KEY_X readinputport(4)&0x80
#define KEY_C readinputport(4)&0x40
#define KEY_V readinputport(4)&0x20
#define KEY_B readinputport(4)&0x10
#define KEY_N readinputport(4)&8
#define KEY_M readinputport(4)&4
#define KEY_SPC readinputport(4)&2
#define KEY_ENTER readinputport(4)&1

#define KEY_HYP readinputport(5)&0x80
#define KEY_SIN readinputport(5)&0x40
#define KEY_COS readinputport(5)&0x20
#define KEY_TAN readinputport(5)&0x10
#define KEY_FE readinputport(5)&8
#define KEY_CCE readinputport(5)&4
#define KEY_HEX readinputport(5)&2
#define KEY_DEG readinputport(5)&1

#define KEY_LN readinputport(6)&0x80
#define KEY_LOG readinputport(6)&0x40
#define KEY_1X readinputport(6)&0x20
#define KEY_STAT readinputport(6)&0x10
#define KEY_EXP readinputport(6)&8
#define KEY_POT readinputport(6)&4
#define KEY_ROOT readinputport(6)&2
#define KEY_SQUARE readinputport(6)&1

#define KEY_BRACE_LEFT readinputport(7)&0x80
#define KEY_BRACE_RIGHT readinputport(7)&0x40
#define KEY_7 readinputport(7)&0x20
#define KEY_8 readinputport(7)&0x10
#define KEY_9 readinputport(7)&8
#define KEY_DIV readinputport(7)&4
#define KEY_XM readinputport(7)&2
#define KEY_4 readinputport(7)&1

#define KEY_5 readinputport(8)&0x80
#define KEY_6 readinputport(8)&0x40
#define KEY_MUL readinputport(8)&0x20
#define KEY_RM readinputport(8)&0x10
#define KEY_1 readinputport(8)&8
#define KEY_2 readinputport(8)&4
#define KEY_3 readinputport(8)&2
#define KEY_MINUS readinputport(8)&1

#define KEY_MPLUS readinputport(9)&0x80
#define KEY_0 readinputport(9)&0x40
#define KEY_SIGN readinputport(9)&0x20
#define KEY_POINT readinputport(9)&0x10
#define KEY_PLUS readinputport(9)&8
#define KEY_EQUALS readinputport(9)&4
#define KEY_RESET readinputport(9)&2

#define RAM4K (readinputport(10)&0xc0)==0x40
#define RAM10K (readinputport(10)&0xc0)==0x80
#define CONTRAST (readinputport(10)&7)


/*----------- defined in machine/pc1401.c -----------*/

extern UINT8 pc1401_portc;
int pc1401_reset(void);
int pc1401_brk(void);
void pc1401_outa(int data);
void pc1401_outb(int data);
void pc1401_outc(int data);
int pc1401_ina(void);
int pc1401_inb(void);

DRIVER_INIT( pc1401 );
NVRAM_HANDLER( pc1401 );


/*----------- defined in video/pc1401.c -----------*/

READ8_HANDLER(pc1401_lcd_read);
WRITE8_HANDLER(pc1401_lcd_write);
VIDEO_UPDATE( pc1401 );


#endif /* PC1401_H_ */
