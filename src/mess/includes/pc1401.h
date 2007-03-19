/*********************************************************************

	pc1401.h

	Pocket Computer 1401

*********************************************************************/

#ifndef PC1401_H
#define PC1401_H

extern UINT8 pc1401_portc;
bool pc1401_reset(void);
bool pc1401_brk(void);
void pc1401_outa(int data);
void pc1401_outb(int data);
void pc1401_outc(int data);
int pc1401_ina(void);
int pc1401_inb(void);

DRIVER_INIT( pc1401 );
NVRAM_HANDLER( pc1401 );

/* in vidhrdw/pocketc.c */
READ8_HANDLER(pc1401_lcd_read);
WRITE8_HANDLER(pc1401_lcd_write);
VIDEO_UPDATE( pc1401 );

/* in systems/pocketc.c */
#define KEY_OFF input_port_0_r(0)&0x80
#define KEY_CAL input_port_0_r(0)&0x40
#define KEY_BASIC input_port_0_r(0)&0x20
#define KEY_BRK input_port_0_r(0)&0x10
#define KEY_DEF input_port_0_r(0)&8
#define KEY_DOWN input_port_0_r(0)&4
#define KEY_UP input_port_0_r(0)&2
#define KEY_LEFT input_port_0_r(0)&1

#define KEY_RIGHT input_port_1_r(0)&0x80
// pc1403 sml
#define KEY_SHIFT input_port_1_r(0)&0x20
#define KEY_Q input_port_1_r(0)&0x10
#define KEY_W input_port_1_r(0)&8
#define KEY_E input_port_1_r(0)&4
#define KEY_R input_port_1_r(0)&2
#define KEY_T input_port_1_r(0)&1

#define KEY_Y input_port_2_r(0)&0x80
#define KEY_U input_port_2_r(0)&0x40
#define KEY_I input_port_2_r(0)&0x20
#define KEY_O input_port_2_r(0)&0x10
#define KEY_P input_port_2_r(0)&8
#define KEY_A input_port_2_r(0)&4
#define KEY_S input_port_2_r(0)&2
#define KEY_D input_port_2_r(0)&1

#define KEY_F input_port_3_r(0)&0x80
#define KEY_G input_port_3_r(0)&0x40
#define KEY_H input_port_3_r(0)&0x20
#define KEY_J input_port_3_r(0)&0x10
#define KEY_K input_port_3_r(0)&8
#define KEY_L input_port_3_r(0)&4
#define KEY_COMMA input_port_3_r(0)&2
#define KEY_Z input_port_3_r(0)&1

#define KEY_X input_port_4_r(0)&0x80
#define KEY_C input_port_4_r(0)&0x40
#define KEY_V input_port_4_r(0)&0x20
#define KEY_B input_port_4_r(0)&0x10
#define KEY_N input_port_4_r(0)&8
#define KEY_M input_port_4_r(0)&4
#define KEY_SPC input_port_4_r(0)&2
#define KEY_ENTER input_port_4_r(0)&1

#define KEY_HYP input_port_5_r(0)&0x80
#define KEY_SIN input_port_5_r(0)&0x40
#define KEY_COS input_port_5_r(0)&0x20
#define KEY_TAN input_port_5_r(0)&0x10
#define KEY_FE input_port_5_r(0)&8
#define KEY_CCE input_port_5_r(0)&4
#define KEY_HEX input_port_5_r(0)&2
#define KEY_DEG input_port_5_r(0)&1

#define KEY_LN input_port_6_r(0)&0x80
#define KEY_LOG input_port_6_r(0)&0x40
#define KEY_1X input_port_6_r(0)&0x20
#define KEY_STAT input_port_6_r(0)&0x10
#define KEY_EXP input_port_6_r(0)&8
#define KEY_POT input_port_6_r(0)&4
#define KEY_ROOT input_port_6_r(0)&2
#define KEY_SQUARE input_port_6_r(0)&1

#define KEY_BRACE_LEFT input_port_7_r(0)&0x80
#define KEY_BRACE_RIGHT input_port_7_r(0)&0x40
#define KEY_7 input_port_7_r(0)&0x20
#define KEY_8 input_port_7_r(0)&0x10
#define KEY_9 input_port_7_r(0)&8
#define KEY_DIV input_port_7_r(0)&4
#define KEY_XM input_port_7_r(0)&2
#define KEY_4 input_port_7_r(0)&1

#define KEY_5 input_port_8_r(0)&0x80
#define KEY_6 input_port_8_r(0)&0x40
#define KEY_MUL input_port_8_r(0)&0x20
#define KEY_RM input_port_8_r(0)&0x10
#define KEY_1 input_port_8_r(0)&8
#define KEY_2 input_port_8_r(0)&4
#define KEY_3 input_port_8_r(0)&2
#define KEY_MINUS input_port_8_r(0)&1

#define KEY_MPLUS input_port_9_r(0)&0x80
#define KEY_0 input_port_9_r(0)&0x40
#define KEY_SIGN input_port_9_r(0)&0x20
#define KEY_POINT input_port_9_r(0)&0x10
#define KEY_PLUS input_port_9_r(0)&8
#define KEY_EQUALS input_port_9_r(0)&4
#define KEY_RESET input_port_9_r(0)&2

#define RAM4K (input_port_10_r(0)&0xc0)==0x40
#define RAM10K (input_port_10_r(0)&0xc0)==0x80
#define CONTRAST (input_port_10_r(0)&7)

#endif /* PC1401_H */
