/*****************************************************************************
 *
 * includes/c16.h
 *
 ****************************************************************************/

#ifndef C16_H_
#define C16_H_


#define JOYSTICK1_PORT		( input_port_read(Machine, "CFG0") & 0x80 )
#define JOYSTICK2_PORT		( input_port_read(Machine, "CFG0") & 0x40 )

#define JOYSTICK_2_LEFT		( input_port_read(Machine, "JOY1") & 0x80 )
#define JOYSTICK_2_RIGHT	( input_port_read(Machine, "JOY1") & 0x40 )
#define JOYSTICK_2_UP		( input_port_read(Machine, "JOY1") & 0x20 )
#define JOYSTICK_2_DOWN		( input_port_read(Machine, "JOY1") & 0x10 )
#define JOYSTICK_2_BUTTON	( input_port_read(Machine, "JOY1") & 0x08 )

#define JOYSTICK_1_LEFT		( input_port_read(Machine, "JOY0") & 0x80 )
#define JOYSTICK_1_RIGHT	( input_port_read(Machine, "JOY0") & 0x40 )
#define JOYSTICK_1_UP		( input_port_read(Machine, "JOY0") & 0x20 )
#define JOYSTICK_1_DOWN		( input_port_read(Machine, "JOY0") & 0x10 )
#define JOYSTICK_1_BUTTON	( input_port_read(Machine, "JOY0") & 0x08 )

#define JOYSTICK_SWAP		( input_port_read(Machine, "Special") & 0x80 )

#define DATASSETTE_PLAY		( input_port_read(Machine, "CFG0") & 0x04 )
#define DATASSETTE_RECORD	( input_port_read(Machine, "CFG0") & 0x02 )
#define DATASSETTE_STOP		( input_port_read(Machine, "CFG0") & 0x01 )


#define DATASSETTE			( input_port_read(Machine, "CFG0") & 0x20 )
#define DATASSETTE_TONE		( input_port_read(Machine, "CFG0") & 0x10 )

#define NO_REAL_FLOPPY		(( input_port_read(Machine, "CFG1") & 0xc0 ) == 0x00 )
#define REAL_C1551			(( input_port_read(Machine, "CFG1") & 0xc0 ) == 0x40 )
#define REAL_VC1541			(( input_port_read(Machine, "CFG1") & 0xc0 ) == 0x80 )

#define IEC8ON				(( input_port_read(Machine, "CFG1") & 0x38 ) ==8 )
#define IEC8ON				(( input_port_read(Machine, "CFG1") & 0x38 ) ==8 )
#define IEC9ON				(( input_port_read(Machine, "CFG1") & 0x07 ) ==1 )

#define SIDCARD				( input_port_read(Machine, "CFG2") & 0x80 )
// a lot of c64 software has been converted to c16
// these oftenly still produce the commands for the sid chip at 0xd400
// with following hack you can hear these sounds
#define SIDCARD_HACK		( input_port_read(Machine, "CFG2") & 0x40 )

#define C16_PAL				(( input_port_read(Machine, "CFG2") & 0x10 ) == 0x00 )

#define TYPE_C16			(( input_port_read(Machine, "CFG2") & 0x0c ) == 0x00 )
#define TYPE_PLUS4			(( input_port_read(Machine, "CFG2") & 0x0c ) == 0x04 )
#define TYPE_364			(( input_port_read(Machine, "CFG2") & 0x0c ) == 0x08 )


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

extern DEVICE_IMAGE_LOAD(c16_rom);


/*----------- defined in audio/t6721.c -----------*/

extern WRITE8_HANDLER(c364_speech_w);
extern  READ8_HANDLER(c364_speech_r);

extern void c364_speech_init(void);


#endif /* C16_H_ */
