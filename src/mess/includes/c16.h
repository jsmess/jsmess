#ifndef __C16_H_
#define __C16_H_

#include "driver.h"
#include "cbmserb.h"

#define JOYSTICK1_PORT (input_port_7_r(0)&0x80)
#define JOYSTICK2_PORT (input_port_7_r(0)&0x40)

#define JOYSTICK_2_LEFT	((input_port_1_r(0)&0x80))
#define JOYSTICK_2_RIGHT	((input_port_1_r(0)&0x40))
#define JOYSTICK_2_UP		((input_port_1_r(0)&0x20))
#define JOYSTICK_2_DOWN	((input_port_1_r(0)&0x10))
#define JOYSTICK_2_BUTTON ((input_port_1_r(0)&8))

#define JOYSTICK_1_LEFT	((input_port_0_r(0)&0x80))
#define JOYSTICK_1_RIGHT	((input_port_0_r(0)&0x40))
#define JOYSTICK_1_UP		((input_port_0_r(0)&0x20))
#define JOYSTICK_1_DOWN	((input_port_0_r(0)&0x10))
#define JOYSTICK_1_BUTTON ((input_port_0_r(0)&8))

#define KEY_ESC (input_port_2_word_r(0,0)&0x8000)
#define KEY_1 (input_port_2_word_r(0,0)&0x4000)
#define KEY_2 (input_port_2_word_r(0,0)&0x2000)
#define KEY_3 (input_port_2_word_r(0,0)&0x1000)
#define KEY_4 (input_port_2_word_r(0,0)&0x800)
#define KEY_5 (input_port_2_word_r(0,0)&0x400)
#define KEY_6 (input_port_2_word_r(0,0)&0x200)
#define KEY_7 (input_port_2_word_r(0,0)&0x100)
#define KEY_8 (input_port_2_word_r(0,0)&0x80)
#define KEY_9 (input_port_2_word_r(0,0)&0x40)
#define KEY_0 (input_port_2_word_r(0,0)&0x20)
#define KEY_LEFT (input_port_2_word_r(0,0)&0x10)
#define KEY_RIGHT (input_port_2_word_r(0,0)&8)
#define KEY_UP (input_port_2_word_r(0,0)&4)
#define KEY_DOWN (input_port_2_word_r(0,0)&2)
#define KEY_DEL (input_port_2_word_r(0,0)&1)

#define KEY_CTRL (input_port_3_word_r(0,0)&0x8000)
#define KEY_Q (input_port_3_word_r(0,0)&0x4000)
#define KEY_W (input_port_3_word_r(0,0)&0x2000)
#define KEY_E (input_port_3_word_r(0,0)&0x1000)
#define KEY_R (input_port_3_word_r(0,0)&0x800)
#define KEY_T (input_port_3_word_r(0,0)&0x400)
#define KEY_Y (input_port_3_word_r(0,0)&0x200)
#define KEY_U (input_port_3_word_r(0,0)&0x100)
#define KEY_I (input_port_3_word_r(0,0)&0x80)
#define KEY_O (input_port_3_word_r(0,0)&0x40)
#define KEY_P (input_port_3_word_r(0,0)&0x20)
#define KEY_ATSIGN (input_port_3_word_r(0,0)&0x10)
#define KEY_PLUS (input_port_3_word_r(0,0)&8)
#define KEY_MINUS (input_port_3_word_r(0,0)&4)
#define KEY_HOME (input_port_3_word_r(0,0)&2)
#define KEY_STOP (input_port_3_word_r(0,0)&1)

#define KEY_SHIFTLOCK (input_port_4_word_r(0,0)&0x8000)
#define KEY_A (input_port_4_word_r(0,0)&0x4000)
#define KEY_S (input_port_4_word_r(0,0)&0x2000)
#define KEY_D (input_port_4_word_r(0,0)&0x1000)
#define KEY_F (input_port_4_word_r(0,0)&0x800)
#define KEY_G (input_port_4_word_r(0,0)&0x400)
#define KEY_H (input_port_4_word_r(0,0)&0x200)
#define KEY_J (input_port_4_word_r(0,0)&0x100)
#define KEY_K (input_port_4_word_r(0,0)&0x80)
#define KEY_L (input_port_4_word_r(0,0)&0x40)
#define KEY_SEMICOLON (input_port_4_word_r(0,0)&0x20)
#define KEY_COLON (input_port_4_word_r(0,0)&0x10)
#define KEY_ASTERIX (input_port_4_word_r(0,0)&8)
#define KEY_RETURN (input_port_4_word_r(0,0)&4)
#define KEY_CBM (input_port_4_word_r(0,0)&2)
#define KEY_LEFT_SHIFT (input_port_4_word_r(0,0)&1)


#define KEY_Z (input_port_5_word_r(0,0)&0x8000)
#define KEY_X (input_port_5_word_r(0,0)&0x4000)
#define KEY_C (input_port_5_word_r(0,0)&0x2000)
#define KEY_V (input_port_5_word_r(0,0)&0x1000)
#define KEY_B (input_port_5_word_r(0,0)&0x800)
#define KEY_N (input_port_5_word_r(0,0)&0x400)
#define KEY_M (input_port_5_word_r(0,0)&0x200)
#define KEY_COMMA (input_port_5_word_r(0,0)&0x100)
#define KEY_POINT (input_port_5_word_r(0,0)&0x80)
#define KEY_SLASH (input_port_5_word_r(0,0)&0x40)
#define KEY_RIGHT_SHIFT (input_port_5_word_r(0,0)&0x20)
#define KEY_POUND (input_port_5_word_r(0,0)&0x10)
#define KEY_EQUALS (input_port_5_word_r(0,0)&8)
#define KEY_SPACE (input_port_5_word_r(0,0)&4)
#define KEY_F1 (input_port_5_word_r(0,0)&2)
#define KEY_F2 (input_port_5_word_r(0,0)&1)

#define KEY_F3 (input_port_6_word_r(0,0)&0x8000)
#define KEY_HELP (input_port_6_word_r(0,0)&0x4000)

#define JOYSTICK_SWAP (input_port_6_word_r(0,0)&0x2000)

#define DATASSETTE_PLAY		(input_port_6_word_r(0,0)&4)
#define DATASSETTE_RECORD	(input_port_6_word_r(0,0)&2)
#define DATASSETTE_STOP		(input_port_6_word_r(0,0)&1)

#define QUICKLOAD		(input_port_6_word_r(0,0)&8)

#define KEY_SHIFT (KEY_LEFT_SHIFT||KEY_RIGHT_SHIFT||KEY_SHIFTLOCK)

#define DATASSETTE (input_port_7_r(0)&0x20)
#define DATASSETTE_TONE (input_port_7_r(0)&0x10)

#define NO_REAL_FLOPPY ((input_port_8_r(0)&0xc0)==0)
#define REAL_C1551 ((input_port_8_r(0)&0xc0)==0x40)
#define REAL_VC1541 ((input_port_8_r(0)&0xc0)==0x80)

#define IEC8ON ((input_port_8_r(0)&0x38)==8)
#define IEC8ON ((input_port_8_r(0)&0x38)==8)
#define IEC9ON ((input_port_8_r(0)&7)==1)

#define SIDCARD ((input_port_9_r(0)&0x80))
// a lot of c64 software has been converted to c16
// these oftenly still produce the commands for the sid chip at 0xd400
// with following hack you can hear these sounds
#define SIDCARD_HACK ((input_port_9_r(0)&0x40))

#define C16_PAL ((input_port_9_r(0)&0x10)==0)

#define TYPE_C16 ((input_port_9_r(0)&0xc)==0)
#define TYPE_PLUS4 ((input_port_9_r(0)&0xc)==4)
#define TYPE_364 ((input_port_9_r(0)&0xc)==8)

extern UINT8 *c16_memory;

UINT8 c16_m7501_port_read(void);
void c16_m7501_port_write(UINT8 data);

extern WRITE8_HANDLER(c16_6551_port_w);
extern  READ8_HANDLER(c16_6551_port_r);

extern  READ8_HANDLER(c16_fd1x_r);
extern WRITE8_HANDLER(plus4_6529_port_w);
extern  READ8_HANDLER(plus4_6529_port_r);

extern WRITE8_HANDLER(c16_6529_port_w);
extern  READ8_HANDLER(c16_6529_port_r);

extern WRITE8_HANDLER(c364_speech_w);
extern  READ8_HANDLER(c364_speech_r);

extern void c364_speech_init(void);

#if 0
extern WRITE8_HANDLER(c16_iec9_port_w);
extern  READ8_HANDLER(c16_iec9_port_r);

extern WRITE8_HANDLER(c16_iec8_port_w);
extern  READ8_HANDLER(c16_iec8_port_r);

#endif

extern WRITE8_HANDLER(c16_select_roms);
extern WRITE8_HANDLER(c16_switch_to_rom);
extern WRITE8_HANDLER(c16_switch_to_ram);

/* ted reads */
extern int c16_read_keyboard (int databus);
extern void c16_interrupt (int);

extern void c16_driver_init(void);
extern MACHINE_RESET( c16 );
extern INTERRUPT_GEN( c16_frame_interrupt );

extern DEVICE_LOAD(c16_rom);

#endif
