/*****************************************************************************
 *
 * includes/pc1251.h
 *
 * Pocket Computer 1251
 *
 ****************************************************************************/

#ifndef PC1251_H_
#define PC1251_H_


#define PC1251_SWITCH_MODE (readinputport(0)&7)

#define PC1251_KEY_DEF readinputport(0)&0x100
#define PC1251_KEY_SHIFT readinputport(0)&0x80
#define PC1251_KEY_DOWN readinputport(0)&0x40
#define PC1251_KEY_UP readinputport(0)&0x20
#define PC1251_KEY_LEFT readinputport(0)&0x10
#define PC1251_KEY_RIGHT readinputport(0)&8

#define PC1251_KEY_BRK readinputport(1)&0x80
#define PC1251_KEY_Q readinputport(1)&0x40
#define PC1251_KEY_W readinputport(1)&0x20
#define PC1251_KEY_E readinputport(1)&0x10
#define PC1251_KEY_R readinputport(1)&8
#define PC1251_KEY_T readinputport(1)&4
#define PC1251_KEY_Y readinputport(1)&2
#define PC1251_KEY_U readinputport(1)&1

#define PC1251_KEY_I readinputport(2)&0x80
#define PC1251_KEY_O readinputport(2)&0x40
#define PC1251_KEY_P readinputport(2)&0x20
#define PC1251_KEY_A readinputport(2)&0x10
#define PC1251_KEY_S readinputport(2)&8
#define PC1251_KEY_D readinputport(2)&4
#define PC1251_KEY_F readinputport(2)&2
#define PC1251_KEY_G readinputport(2)&1

#define PC1251_KEY_H readinputport(3)&0x80
#define PC1251_KEY_J readinputport(3)&0x40
#define PC1251_KEY_K readinputport(3)&0x20
#define PC1251_KEY_L readinputport(3)&0x10
#define PC1251_KEY_EQUALS readinputport(3)&8
#define PC1251_KEY_Z readinputport(3)&4
#define PC1251_KEY_X readinputport(3)&2
#define PC1251_KEY_C readinputport(3)&1

#define PC1251_KEY_V readinputport(4)&0x80
#define PC1251_KEY_B readinputport(4)&0x40
#define PC1251_KEY_N readinputport(4)&0x20
#define PC1251_KEY_M readinputport(4)&0x10
#define PC1251_KEY_SPACE readinputport(4)&8
#define PC1251_KEY_ENTER readinputport(4)&4
#define PC1251_KEY_7 readinputport(4)&2
#define PC1251_KEY_8 readinputport(4)&1

#define PC1251_KEY_9 readinputport(5)&0x80
#define PC1251_KEY_CL readinputport(5)&0x40
#define PC1251_KEY_4 readinputport(5)&0x20
#define PC1251_KEY_5 readinputport(5)&0x10
#define PC1251_KEY_6 readinputport(5)&8
#define PC1251_KEY_SLASH readinputport(5)&4
#define PC1251_KEY_1 readinputport(5)&2
#define PC1251_KEY_2 readinputport(5)&1

#define PC1251_KEY_3 readinputport(6)&0x80
#define PC1251_KEY_ASTERIX readinputport(6)&0x40
#define PC1251_KEY_0 readinputport(6)&0x20
#define PC1251_KEY_POINT readinputport(6)&0x10
#define PC1251_KEY_PLUS readinputport(6)&8
#define PC1251_KEY_MINUS readinputport(6)&4
#define PC1251_KEY_RESET readinputport(6)&1

#define PC1251_RAM4K (readinputport(7)&0xc0)==0x40
#define PC1251_RAM6K (readinputport(7)&0xc0)==0x80
#define PC1251_RAM11K (readinputport(7)&0xc0)==0xc0

#define PC1251_CONTRAST (readinputport(7)&7)


/*----------- defined in machine/pc1251.c -----------*/

void pc1251_outa(int data);
void pc1251_outb(int data);
void pc1251_outc(int data);

int pc1251_reset(void);
int pc1251_brk(void);
int pc1251_ina(void);
int pc1251_inb(void);

DRIVER_INIT( pc1251 );
NVRAM_HANDLER( pc1251 );


/*----------- defined in video/pc1251.c -----------*/

READ8_HANDLER(pc1251_lcd_read);
WRITE8_HANDLER(pc1251_lcd_write);
VIDEO_UPDATE( pc1251 );


#endif /* PC1251_H_ */
