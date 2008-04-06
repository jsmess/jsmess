/*****************************************************************************
 *
 * includes/c16.h
 *
 ****************************************************************************/

#ifndef C16_H_
#define C16_H_


#define JOYSTICK1_PORT		( readinputportbytag("CFG0") & 0x80 )
#define JOYSTICK2_PORT		( readinputportbytag("CFG0") & 0x40 )

#define JOYSTICK_2_LEFT		( readinputportbytag("JOY1") & 0x80 )
#define JOYSTICK_2_RIGHT	( readinputportbytag("JOY1") & 0x40 )
#define JOYSTICK_2_UP		( readinputportbytag("JOY1") & 0x20 )
#define JOYSTICK_2_DOWN		( readinputportbytag("JOY1") & 0x10 )
#define JOYSTICK_2_BUTTON	( readinputportbytag("JOY1") & 0x08 )

#define JOYSTICK_1_LEFT		( readinputportbytag("JOY0") & 0x80 )
#define JOYSTICK_1_RIGHT	( readinputportbytag("JOY0") & 0x40 )
#define JOYSTICK_1_UP		( readinputportbytag("JOY0") & 0x20 )
#define JOYSTICK_1_DOWN		( readinputportbytag("JOY0") & 0x10 )
#define JOYSTICK_1_BUTTON	( readinputportbytag("JOY0") & 0x08 )

#define JOYSTICK_SWAP		( readinputportbytag("Special") & 0x80 )

#define DATASSETTE_PLAY		( readinputportbytag("CFG0") & 0x04 )
#define DATASSETTE_RECORD	( readinputportbytag("CFG0") & 0x02 )
#define DATASSETTE_STOP		( readinputportbytag("CFG0") & 0x01 )

#define QUICKLOAD			( readinputportbytag("CFG0") & 0x08 )


#define DATASSETTE			( readinputportbytag("CFG0") & 0x20 )
#define DATASSETTE_TONE		( readinputportbytag("CFG0") & 0x10 )

#define NO_REAL_FLOPPY		(( readinputportbytag("CFG1") & 0xc0 ) == 0x00 )
#define REAL_C1551			(( readinputportbytag("CFG1") & 0xc0 ) == 0x40 )
#define REAL_VC1541			(( readinputportbytag("CFG1") & 0xc0 ) == 0x80 )

#define IEC8ON				(( readinputportbytag("CFG1") & 0x38 ) ==8 )
#define IEC8ON				(( readinputportbytag("CFG1") & 0x38 ) ==8 )
#define IEC9ON				(( readinputportbytag("CFG1") & 0x07 ) ==1 )

#define SIDCARD				( readinputportbytag("CFG2") & 0x80 )
// a lot of c64 software has been converted to c16
// these oftenly still produce the commands for the sid chip at 0xd400
// with following hack you can hear these sounds
#define SIDCARD_HACK		( readinputportbytag("CFG2") & 0x40 )

#define C16_PAL				(( readinputportbytag("CFG2") & 0x10 ) == 0x00 )

#define TYPE_C16			(( readinputportbytag("CFG2") & 0x0c ) == 0x00 )
#define TYPE_PLUS4			(( readinputportbytag("CFG2") & 0x0c ) == 0x04 )
#define TYPE_364			(( readinputportbytag("CFG2") & 0x0c ) == 0x08 )


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
