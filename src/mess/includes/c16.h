/*****************************************************************************
 *
 * includes/c16.h
 *
 ****************************************************************************/

#ifndef C16_H_
#define C16_H_


#define JOYSTICK1_PORT (readinputport(7)&0x80)
#define JOYSTICK2_PORT (readinputport(7)&0x40)

#define JOYSTICK_2_LEFT	((readinputport(1)&0x80))
#define JOYSTICK_2_RIGHT	((readinputport(1)&0x40))
#define JOYSTICK_2_UP		((readinputport(1)&0x20))
#define JOYSTICK_2_DOWN	((readinputport(1)&0x10))
#define JOYSTICK_2_BUTTON ((readinputport(1)&8))

#define JOYSTICK_1_LEFT	((readinputport(0)&0x80))
#define JOYSTICK_1_RIGHT	((readinputport(0)&0x40))
#define JOYSTICK_1_UP		((readinputport(0)&0x20))
#define JOYSTICK_1_DOWN	((readinputport(0)&0x10))
#define JOYSTICK_1_BUTTON ((readinputport(0)&8))

#define KEY_ESC (readinputport(2)&0x8000)
#define KEY_1 (readinputport(2)&0x4000)
#define KEY_2 (readinputport(2)&0x2000)
#define KEY_3 (readinputport(2)&0x1000)
#define KEY_4 (readinputport(2)&0x800)
#define KEY_5 (readinputport(2)&0x400)
#define KEY_6 (readinputport(2)&0x200)
#define KEY_7 (readinputport(2)&0x100)
#define KEY_8 (readinputport(2)&0x80)
#define KEY_9 (readinputport(2)&0x40)
#define KEY_0 (readinputport(2)&0x20)
#define KEY_LEFT (readinputport(2)&0x10)
#define KEY_RIGHT (readinputport(2)&8)
#define KEY_UP (readinputport(2)&4)
#define KEY_DOWN (readinputport(2)&2)
#define KEY_DEL (readinputport(2)&1)

#define KEY_CTRL (readinputport(3)&0x8000)
#define KEY_Q (readinputport(3)&0x4000)
#define KEY_W (readinputport(3)&0x2000)
#define KEY_E (readinputport(3)&0x1000)
#define KEY_R (readinputport(3)&0x800)
#define KEY_T (readinputport(3)&0x400)
#define KEY_Y (readinputport(3)&0x200)
#define KEY_U (readinputport(3)&0x100)
#define KEY_I (readinputport(3)&0x80)
#define KEY_O (readinputport(3)&0x40)
#define KEY_P (readinputport(3)&0x20)
#define KEY_ATSIGN (readinputport(3)&0x10)
#define KEY_PLUS (readinputport(3)&8)
#define KEY_MINUS (readinputport(3)&4)
#define KEY_HOME (readinputport(3)&2)
#define KEY_STOP (readinputport(3)&1)

#define KEY_SHIFTLOCK (readinputport(4)&0x8000)
#define KEY_A (readinputport(4)&0x4000)
#define KEY_S (readinputport(4)&0x2000)
#define KEY_D (readinputport(4)&0x1000)
#define KEY_F (readinputport(4)&0x800)
#define KEY_G (readinputport(4)&0x400)
#define KEY_H (readinputport(4)&0x200)
#define KEY_J (readinputport(4)&0x100)
#define KEY_K (readinputport(4)&0x80)
#define KEY_L (readinputport(4)&0x40)
#define KEY_SEMICOLON (readinputport(4)&0x20)
#define KEY_COLON (readinputport(4)&0x10)
#define KEY_ASTERIX (readinputport(4)&8)
#define KEY_RETURN (readinputport(4)&4)
#define KEY_CBM (readinputport(4)&2)
#define KEY_LEFT_SHIFT (readinputport(4)&1)


#define KEY_Z (readinputport(5)&0x8000)
#define KEY_X (readinputport(5)&0x4000)
#define KEY_C (readinputport(5)&0x2000)
#define KEY_V (readinputport(5)&0x1000)
#define KEY_B (readinputport(5)&0x800)
#define KEY_N (readinputport(5)&0x400)
#define KEY_M (readinputport(5)&0x200)
#define KEY_COMMA (readinputport(5)&0x100)
#define KEY_POINT (readinputport(5)&0x80)
#define KEY_SLASH (readinputport(5)&0x40)
#define KEY_RIGHT_SHIFT (readinputport(5)&0x20)
#define KEY_POUND (readinputport(5)&0x10)
#define KEY_EQUALS (readinputport(5)&8)
#define KEY_SPACE (readinputport(5)&4)
#define KEY_F1 (readinputport(5)&2)
#define KEY_F2 (readinputport(5)&1)

#define KEY_F3 (readinputport(6)&0x8000)
#define KEY_HELP (readinputport(6)&0x4000)

#define JOYSTICK_SWAP (readinputport(6)&0x2000)

#define DATASSETTE_PLAY		(readinputport(6)&4)
#define DATASSETTE_RECORD	(readinputport(6)&2)
#define DATASSETTE_STOP		(readinputport(6)&1)

#define QUICKLOAD		(readinputport(6)&8)

#define KEY_SHIFT (KEY_LEFT_SHIFT||KEY_RIGHT_SHIFT||KEY_SHIFTLOCK)

#define DATASSETTE (readinputport(7)&0x20)
#define DATASSETTE_TONE (readinputport(7)&0x10)

#define NO_REAL_FLOPPY ((readinputport(8)&0xc0)==0)
#define REAL_C1551 ((readinputport(8)&0xc0)==0x40)
#define REAL_VC1541 ((readinputport(8)&0xc0)==0x80)

#define IEC8ON ((readinputport(8)&0x38)==8)
#define IEC8ON ((readinputport(8)&0x38)==8)
#define IEC9ON ((readinputport(8)&7)==1)

#define SIDCARD ((readinputport(9)&0x80))
// a lot of c64 software has been converted to c16
// these oftenly still produce the commands for the sid chip at 0xd400
// with following hack you can hear these sounds
#define SIDCARD_HACK ((readinputport(9)&0x40))

#define C16_PAL ((readinputport(9)&0x10)==0)

#define TYPE_C16 ((readinputport(9)&0xc)==0)
#define TYPE_PLUS4 ((readinputport(9)&0xc)==4)
#define TYPE_364 ((readinputport(9)&0xc)==8)


/*----------- defined in machine/c16.c -----------*/

UINT8 c16_m7501_port_read(void);
void c16_m7501_port_write(UINT8 data);

extern WRITE8_HANDLER(c16_6551_port_w);
extern  READ8_HANDLER(c16_6551_port_r);

extern  READ8_HANDLER(c16_fd1x_r);
extern WRITE8_HANDLER(plus4_6529_port_w);
extern  READ8_HANDLER(plus4_6529_port_r);

extern WRITE8_HANDLER(c16_6529_port_w);
extern  READ8_HANDLER(c16_6529_port_r);

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
extern void c16_interrupt (running_machine *machine, int);

extern void c16_driver_init(running_machine *machine);
extern MACHINE_RESET( c16 );
extern INTERRUPT_GEN( c16_frame_interrupt );

extern DEVICE_LOAD(c16_rom);


/*----------- defined in audio/t6721.c -----------*/

extern WRITE8_HANDLER(c364_speech_w);
extern  READ8_HANDLER(c364_speech_r);

extern void c364_speech_init(void);


#endif /* C16_H_ */
