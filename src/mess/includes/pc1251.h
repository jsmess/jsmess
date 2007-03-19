/*********************************************************************

	pc1251.h

	Pocket Computer 1251

*********************************************************************/

#ifndef PC1251_H
#define PC1251_H

void pc1251_outa(int data);
void pc1251_outb(int data);
void pc1251_outc(int data);

bool pc1251_reset(void);
bool pc1251_brk(void);
int pc1251_ina(void);
int pc1251_inb(void);

DRIVER_INIT( pc1251 );
NVRAM_HANDLER( pc1251 );

/* in vidhrdw/pocketc.c */
READ8_HANDLER(pc1251_lcd_read);
WRITE8_HANDLER(pc1251_lcd_write);
VIDEO_UPDATE( pc1251 );

/* in systems/pocketc.c */
#define PC1251_SWITCH_MODE (input_port_0_r(0)&7)

#define PC1251_KEY_DEF input_port_0_r(0)&0x100
#define PC1251_KEY_SHIFT input_port_0_r(0)&0x80
#define PC1251_KEY_DOWN input_port_0_r(0)&0x40
#define PC1251_KEY_UP input_port_0_r(0)&0x20
#define PC1251_KEY_LEFT input_port_0_r(0)&0x10
#define PC1251_KEY_RIGHT input_port_0_r(0)&8

#define PC1251_KEY_BRK input_port_1_r(0)&0x80
#define PC1251_KEY_Q input_port_1_r(0)&0x40
#define PC1251_KEY_W input_port_1_r(0)&0x20
#define PC1251_KEY_E input_port_1_r(0)&0x10
#define PC1251_KEY_R input_port_1_r(0)&8
#define PC1251_KEY_T input_port_1_r(0)&4
#define PC1251_KEY_Y input_port_1_r(0)&2
#define PC1251_KEY_U input_port_1_r(0)&1

#define PC1251_KEY_I input_port_2_r(0)&0x80
#define PC1251_KEY_O input_port_2_r(0)&0x40
#define PC1251_KEY_P input_port_2_r(0)&0x20
#define PC1251_KEY_A input_port_2_r(0)&0x10
#define PC1251_KEY_S input_port_2_r(0)&8
#define PC1251_KEY_D input_port_2_r(0)&4
#define PC1251_KEY_F input_port_2_r(0)&2
#define PC1251_KEY_G input_port_2_r(0)&1

#define PC1251_KEY_H input_port_3_r(0)&0x80
#define PC1251_KEY_J input_port_3_r(0)&0x40
#define PC1251_KEY_K input_port_3_r(0)&0x20
#define PC1251_KEY_L input_port_3_r(0)&0x10
#define PC1251_KEY_EQUALS input_port_3_r(0)&8
#define PC1251_KEY_Z input_port_3_r(0)&4
#define PC1251_KEY_X input_port_3_r(0)&2
#define PC1251_KEY_C input_port_3_r(0)&1

#define PC1251_KEY_V input_port_4_r(0)&0x80
#define PC1251_KEY_B input_port_4_r(0)&0x40
#define PC1251_KEY_N input_port_4_r(0)&0x20
#define PC1251_KEY_M input_port_4_r(0)&0x10
#define PC1251_KEY_SPACE input_port_4_r(0)&8
#define PC1251_KEY_ENTER input_port_4_r(0)&4
#define PC1251_KEY_7 input_port_4_r(0)&2
#define PC1251_KEY_8 input_port_4_r(0)&1

#define PC1251_KEY_9 input_port_5_r(0)&0x80
#define PC1251_KEY_CL input_port_5_r(0)&0x40
#define PC1251_KEY_4 input_port_5_r(0)&0x20
#define PC1251_KEY_5 input_port_5_r(0)&0x10
#define PC1251_KEY_6 input_port_5_r(0)&8
#define PC1251_KEY_SLASH input_port_5_r(0)&4
#define PC1251_KEY_1 input_port_5_r(0)&2
#define PC1251_KEY_2 input_port_5_r(0)&1

#define PC1251_KEY_3 input_port_6_r(0)&0x80
#define PC1251_KEY_ASTERIX input_port_6_r(0)&0x40
#define PC1251_KEY_0 input_port_6_r(0)&0x20
#define PC1251_KEY_POINT input_port_6_r(0)&0x10
#define PC1251_KEY_PLUS input_port_6_r(0)&8
#define PC1251_KEY_MINUS input_port_6_r(0)&4
#define PC1251_KEY_RESET input_port_6_r(0)&1

#define PC1251_RAM4K (input_port_7_r(0)&0xc0)==0x40
#define PC1251_RAM6K (input_port_7_r(0)&0xc0)==0x80
#define PC1251_RAM11K (input_port_7_r(0)&0xc0)==0xc0

#define PC1251_CONTRAST (input_port_7_r(0)&7)

#endif /* PC1251_H */
