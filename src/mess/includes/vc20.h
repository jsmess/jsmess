/*****************************************************************************
 *
 * includes/vc20.h
 *
 ****************************************************************************/

#ifndef VC20_H_
#define VC20_H_


#define TAG_JOYSTICK		"joystick"
#define TAG_PADDLE1			"paddle1"
#define TAG_PADDLE2			"paddle2"
#define TAG_LIGHTPENX		"lightpenX"
#define TAG_LIGHTPENY		"lightpenY"
#define TAG_EXPANSION		"expansion"
#define TAG_DEVICES			"devices"
#define TAG_KEYBOARD_ROW0	"keyboard0"
#define TAG_KEYBOARD_ROW1	"keyboard1"
#define TAG_KEYBOARD_ROW2	"keyboard2"
#define TAG_KEYBOARD_ROW3	"keyboard3"
#define TAG_KEYBOARD_ROW4	"keyboard4"
#define TAG_KEYBOARD_ROW5	"keyboard5"
#define TAG_KEYBOARD_ROW6	"keyboard6"
#define TAG_KEYBOARD_ROW7	"keyboard7"
#define TAG_KEYBOARD_EXTRA	"keyboardextra"
#define TAG_CASSETTE		"cassette"

/**
  joystick (4 directions switches, 1 button),
  paddles (2) button, 270 degree wheel,
  and lightpen are all connected to the gameport connector
  lightpen to joystick button
  paddle 1 button to joystick left
  paddle 2 button to joystick right
 */

#define JOYSTICK (input_port_read(machine,  TAG_DEVICES )&0x80)
#define PADDLES (input_port_read(machine,  TAG_DEVICES )&0x40)
#define LIGHTPEN (input_port_read(machine,  TAG_DEVICES )&0x20)
#define LIGHTPEN_POINTER (LIGHTPEN&&(input_port_read(machine,  TAG_DEVICES )&0x10))
#define LIGHTPEN_X_VALUE (input_port_read(machine,  TAG_LIGHTPENX )&~1)		/* effectiv resolution */
#define LIGHTPEN_Y_VALUE (input_port_read(machine,  TAG_LIGHTPENY )&~1)		/* effectiv resolution */
#define LIGHTPEN_BUTTON (LIGHTPEN&&(input_port_read(machine,  TAG_JOYSTICK )&0x80))

#define JOYSTICK_UP (input_port_read(machine,  TAG_JOYSTICK )&1)
#define JOYSTICK_DOWN (input_port_read(machine,  TAG_JOYSTICK )&2)
#define JOYSTICK_LEFT (input_port_read(machine,  TAG_JOYSTICK )&4)
#define JOYSTICK_RIGHT (input_port_read(machine,  TAG_JOYSTICK )&8)
#define JOYSTICK_BUTTON (input_port_read(machine,  TAG_JOYSTICK )&0x10)

#define PADDLE1_BUTTON (input_port_read(machine,  TAG_JOYSTICK )&0x20)
#define PADDLE2_BUTTON (input_port_read(machine,  TAG_JOYSTICK )&0x40)

#define PADDLE1_VALUE   input_port_read(machine,  TAG_PADDLE1 )
#define PADDLE2_VALUE	input_port_read(machine,  TAG_PADDLE2 )


#define DATASSETTE (input_port_read(machine,  TAG_DEVICES )&0x8)
#define DATASSETTE_TONE (input_port_read(machine,  TAG_DEVICES )&4)

#define DATASSETTE_PLAY		(input_port_read(machine,  TAG_CASSETTE )&4)
#define DATASSETTE_RECORD	(input_port_read(machine,  TAG_CASSETTE )&2)
#define DATASSETTE_STOP		(input_port_read(machine,  TAG_CASSETTE )&1)

#define VC20ADDR2VIC6560ADDR(a) (((a)>0x8000)?((a)&0x1fff):((a)|0x2000))
#define VIC6560ADDR2VC20ADDR(a) (((a)>0x2000)?((a)&0x1fff):((a)|0x8000))


/*----------- defined in machine/vc20.c -----------*/

extern UINT8 *vc20_memory_9400;

WRITE8_HANDLER ( vc20_write_9400 );

WRITE8_HANDLER( vc20_0400_w );
WRITE8_HANDLER( vc20_2000_w );
WRITE8_HANDLER( vc20_4000_w );
WRITE8_HANDLER( vc20_6000_w );

/* split for more performance */
/* VIC reads bits 8 till 11 */
int vic6560_dma_read_color (int offset);

/* VIC reads bits 0 till 7 */
int vic6560_dma_read (int offset);

DEVICE_START(vc20_rom);
DEVICE_IMAGE_LOAD(vc20_rom);

DRIVER_INIT( vc20 );
DRIVER_INIT( vic20 );
DRIVER_INIT( vic20i );
DRIVER_INIT( vic1001 );
void vc20_driver_shutdown (void);

MACHINE_RESET( vc20 );
INTERRUPT_GEN( vc20_frame_interrupt );


#endif /* VC20_H_ */
