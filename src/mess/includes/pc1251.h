/*****************************************************************************
 *
 * includes/pc1251.h
 *
 * Pocket Computer 1251
 *
 ****************************************************************************/

#ifndef PC1251_H_
#define PC1251_H_


#define PC1251_SWITCH_MODE (input_port_read_indexed(machine, 0)&7)

#define PC1251_KEY_DEF input_port_read_indexed(machine, 0)&0x100
#define PC1251_KEY_SHIFT input_port_read_indexed(machine, 0)&0x80
#define PC1251_KEY_DOWN input_port_read_indexed(machine, 0)&0x40
#define PC1251_KEY_UP input_port_read_indexed(machine, 0)&0x20
#define PC1251_KEY_LEFT input_port_read_indexed(machine, 0)&0x10
#define PC1251_KEY_RIGHT input_port_read_indexed(machine, 0)&8

#define PC1251_KEY_BRK input_port_read_indexed(machine, 1)&0x80
#define PC1251_KEY_Q input_port_read_indexed(machine, 1)&0x40
#define PC1251_KEY_W input_port_read_indexed(machine, 1)&0x20
#define PC1251_KEY_E input_port_read_indexed(machine, 1)&0x10
#define PC1251_KEY_R input_port_read_indexed(machine, 1)&8
#define PC1251_KEY_T input_port_read_indexed(machine, 1)&4
#define PC1251_KEY_Y input_port_read_indexed(machine, 1)&2
#define PC1251_KEY_U input_port_read_indexed(machine, 1)&1

#define PC1251_KEY_I input_port_read_indexed(machine, 2)&0x80
#define PC1251_KEY_O input_port_read_indexed(machine, 2)&0x40
#define PC1251_KEY_P input_port_read_indexed(machine, 2)&0x20
#define PC1251_KEY_A input_port_read_indexed(machine, 2)&0x10
#define PC1251_KEY_S input_port_read_indexed(machine, 2)&8
#define PC1251_KEY_D input_port_read_indexed(machine, 2)&4
#define PC1251_KEY_F input_port_read_indexed(machine, 2)&2
#define PC1251_KEY_G input_port_read_indexed(machine, 2)&1

#define PC1251_KEY_H input_port_read_indexed(machine, 3)&0x80
#define PC1251_KEY_J input_port_read_indexed(machine, 3)&0x40
#define PC1251_KEY_K input_port_read_indexed(machine, 3)&0x20
#define PC1251_KEY_L input_port_read_indexed(machine, 3)&0x10
#define PC1251_KEY_EQUALS input_port_read_indexed(machine, 3)&8
#define PC1251_KEY_Z input_port_read_indexed(machine, 3)&4
#define PC1251_KEY_X input_port_read_indexed(machine, 3)&2
#define PC1251_KEY_C input_port_read_indexed(machine, 3)&1

#define PC1251_KEY_V input_port_read_indexed(machine, 4)&0x80
#define PC1251_KEY_B input_port_read_indexed(machine, 4)&0x40
#define PC1251_KEY_N input_port_read_indexed(machine, 4)&0x20
#define PC1251_KEY_M input_port_read_indexed(machine, 4)&0x10
#define PC1251_KEY_SPACE input_port_read_indexed(machine, 4)&8
#define PC1251_KEY_ENTER input_port_read_indexed(machine, 4)&4
#define PC1251_KEY_7 input_port_read_indexed(machine, 4)&2
#define PC1251_KEY_8 input_port_read_indexed(machine, 4)&1

#define PC1251_KEY_9 input_port_read_indexed(machine, 5)&0x80
#define PC1251_KEY_CL input_port_read_indexed(machine, 5)&0x40
#define PC1251_KEY_4 input_port_read_indexed(machine, 5)&0x20
#define PC1251_KEY_5 input_port_read_indexed(machine, 5)&0x10
#define PC1251_KEY_6 input_port_read_indexed(machine, 5)&8
#define PC1251_KEY_SLASH input_port_read_indexed(machine, 5)&4
#define PC1251_KEY_1 input_port_read_indexed(machine, 5)&2
#define PC1251_KEY_2 input_port_read_indexed(machine, 5)&1

#define PC1251_KEY_3 input_port_read_indexed(machine, 6)&0x80
#define PC1251_KEY_ASTERIX input_port_read_indexed(machine, 6)&0x40
#define PC1251_KEY_0 input_port_read_indexed(machine, 6)&0x20
#define PC1251_KEY_POINT input_port_read_indexed(machine, 6)&0x10
#define PC1251_KEY_PLUS input_port_read_indexed(machine, 6)&8
#define PC1251_KEY_MINUS input_port_read_indexed(machine, 6)&4
#define PC1251_KEY_RESET input_port_read_indexed(machine, 6)&1

#define PC1251_RAM4K (input_port_read_indexed(machine, 7)&0xc0)==0x40
#define PC1251_RAM6K (input_port_read_indexed(machine, 7)&0xc0)==0x80
#define PC1251_RAM11K (input_port_read_indexed(machine, 7)&0xc0)==0xc0

#define PC1251_CONTRAST (input_port_read_indexed(machine, 7)&7)


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
