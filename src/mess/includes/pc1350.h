/*********************************************************************

	pc1350.h

	Pocket Computer 1350

*********************************************************************/

#ifndef PC1350_H
#define PC1350_H

void pc1350_outa(int data);
void pc1350_outb(int data);
void pc1350_outc(int data);

bool pc1350_brk(void);
int pc1350_ina(void);
int pc1350_inb(void);

MACHINE_START( pc1350 );
NVRAM_HANDLER( pc1350 );

/* in vidhrdw/pocketc.c */
READ8_HANDLER(pc1350_lcd_read);
WRITE8_HANDLER(pc1350_lcd_write);
VIDEO_UPDATE( pc1350 );

int pc1350_keyboard_line_r(void);

/* in systems/pocketc.c */
#define PC1350_KEY_OFF input_port_0_r(0)&0x80
#define PC1350_KEY_DOWN input_port_0_r(0)&0x40
#define PC1350_KEY_UP input_port_0_r(0)&0x20
#define PC1350_KEY_MODE input_port_0_r(0)&0x10
#define PC1350_KEY_CLS input_port_0_r(0)&8
#define PC1350_KEY_LEFT input_port_0_r(0)&4
#define PC1350_KEY_RIGHT input_port_0_r(0)&2
#define PC1350_KEY_DEL input_port_0_r(0)&1

#define PC1350_KEY_INS input_port_1_r(0)&0x80
#define PC1350_KEY_BRK input_port_1_r(0)&0x40
#define PC1350_KEY_RSHIFT input_port_1_r(0)&0x20
#define PC1350_KEY_7 input_port_1_r(0)&0x10
#define PC1350_KEY_8 input_port_1_r(0)&8
#define PC1350_KEY_9 input_port_1_r(0)&4
#define PC1350_KEY_BRACE_OPEN input_port_1_r(0)&2
#define PC1350_KEY_BRACE_CLOSE input_port_1_r(0)&1

#define PC1350_KEY_4 input_port_2_r(0)&0x80
#define PC1350_KEY_5 input_port_2_r(0)&0x40
#define PC1350_KEY_6 input_port_2_r(0)&0x20
#define PC1350_KEY_SLASH input_port_2_r(0)&0x10
#define PC1350_KEY_COLON input_port_2_r(0)&8
#define PC1350_KEY_1 input_port_2_r(0)&4
#define PC1350_KEY_2 input_port_2_r(0)&2
#define PC1350_KEY_3 input_port_2_r(0)&1

#define PC1350_KEY_ASTERIX input_port_3_r(0)&0x80
#define PC1350_KEY_SEMICOLON input_port_3_r(0)&0x40
#define PC1350_KEY_0 input_port_3_r(0)&0x20
#define PC1350_KEY_POINT input_port_3_r(0)&0x10
#define PC1350_KEY_PLUS input_port_3_r(0)&8
#define PC1350_KEY_MINUS input_port_3_r(0)&4
#define PC1350_KEY_COMMA input_port_3_r(0)&2
#define PC1350_KEY_LSHIFT input_port_3_r(0)&1

#define PC1350_KEY_Q input_port_4_r(0)&0x80
#define PC1350_KEY_W input_port_4_r(0)&0x40
#define PC1350_KEY_E input_port_4_r(0)&0x20
#define PC1350_KEY_R input_port_4_r(0)&0x10
#define PC1350_KEY_T input_port_4_r(0)&8
#define PC1350_KEY_Y input_port_4_r(0)&4
#define PC1350_KEY_U input_port_4_r(0)&2
#define PC1350_KEY_I input_port_4_r(0)&1

#define PC1350_KEY_O input_port_5_r(0)&0x80
#define PC1350_KEY_P input_port_5_r(0)&0x40
#define PC1350_KEY_DEF input_port_5_r(0)&0x20
#define PC1350_KEY_A input_port_5_r(0)&0x10
#define PC1350_KEY_S input_port_5_r(0)&8
#define PC1350_KEY_D input_port_5_r(0)&4
#define PC1350_KEY_F input_port_5_r(0)&2
#define PC1350_KEY_G input_port_5_r(0)&1

#define PC1350_KEY_H input_port_6_r(0)&0x80
#define PC1350_KEY_J input_port_6_r(0)&0x40
#define PC1350_KEY_K input_port_6_r(0)&0x20
#define PC1350_KEY_L input_port_6_r(0)&0x10
#define PC1350_KEY_EQUALS input_port_6_r(0)&8
#define PC1350_KEY_SML input_port_6_r(0)&4
#define PC1350_KEY_Z input_port_6_r(0)&2
#define PC1350_KEY_X input_port_6_r(0)&1

#define PC1350_KEY_C input_port_7_r(0)&0x80
#define PC1350_KEY_V input_port_7_r(0)&0x40
#define PC1350_KEY_B input_port_7_r(0)&0x20
#define PC1350_KEY_N input_port_7_r(0)&0x10
#define PC1350_KEY_M input_port_7_r(0)&8
#define PC1350_KEY_SPACE input_port_7_r(0)&4
#define PC1350_KEY_ENTER input_port_7_r(0)&2

#define PC1350_CONTRAST (input_port_8_r(0)&7)

#endif /* PC1350_H */
