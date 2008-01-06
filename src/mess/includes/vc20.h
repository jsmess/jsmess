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

#define JOYSTICK (readinputportbytag( TAG_DEVICES )&0x80)
#define PADDLES (readinputportbytag( TAG_DEVICES )&0x40)
#define LIGHTPEN (readinputportbytag( TAG_DEVICES )&0x20)
#define LIGHTPEN_POINTER (LIGHTPEN&&(readinputportbytag( TAG_DEVICES )&0x10))
#define LIGHTPEN_X_VALUE (readinputportbytag( TAG_LIGHTPENX )&~1)		/* effectiv resolution */
#define LIGHTPEN_Y_VALUE (readinputportbytag( TAG_LIGHTPENY )&~1)		/* effectiv resolution */
#define LIGHTPEN_BUTTON (LIGHTPEN&&(readinputportbytag( TAG_JOYSTICK )&0x80))

#define JOYSTICK_UP (readinputportbytag( TAG_JOYSTICK )&1)
#define JOYSTICK_DOWN (readinputportbytag( TAG_JOYSTICK )&2)
#define JOYSTICK_LEFT (readinputportbytag( TAG_JOYSTICK )&4)
#define JOYSTICK_RIGHT (readinputportbytag( TAG_JOYSTICK )&8)
#define JOYSTICK_BUTTON (readinputportbytag( TAG_JOYSTICK )&0x10)

#define PADDLE1_BUTTON (readinputportbytag( TAG_JOYSTICK )&0x20)
#define PADDLE2_BUTTON (readinputportbytag( TAG_JOYSTICK )&0x40)

#define PADDLE1_VALUE   readinputportbytag( TAG_PADDLE1 )
#define PADDLE2_VALUE	readinputportbytag( TAG_PADDLE2 )


#define DATASSETTE (readinputportbytag( TAG_DEVICES )&0x8)
#define DATASSETTE_TONE (readinputportbytag( TAG_DEVICES )&4)

#define QUICKLOAD		(readinputportbytag( TAG_CASSETTE )&8)

#define DATASSETTE_PLAY		(readinputportbytag( TAG_CASSETTE )&4)
#define DATASSETTE_RECORD	(readinputportbytag( TAG_CASSETTE )&2)
#define DATASSETTE_STOP		(readinputportbytag( TAG_CASSETTE )&1)

/* macros to access the dip switches */
#define EXPANSION (readinputportbytag( TAG_EXPANSION )&7)
#define EXP_3K 1
#define EXP_8K 2
#define EXP_16K 3
#define EXP_32K 4
#define EXP_CUSTOM 5
#define RAMIN0X0400 ((EXPANSION==EXP_3K)\
	||((EXPANSION==EXP_CUSTOM)&&(readinputportbytag( TAG_EXPANSION )&8)) )
#define RAMIN0X2000 ((EXPANSION==EXP_8K)||(EXPANSION==EXP_16K)\
	||(EXPANSION==EXP_32K)\
	||((EXPANSION==EXP_CUSTOM)&&(readinputportbytag( TAG_EXPANSION )&0x10)) )
#define RAMIN0X4000 ((EXPANSION==EXP_16K)||(EXPANSION==EXP_32K)\
	||((EXPANSION==EXP_CUSTOM)&&(readinputportbytag( TAG_EXPANSION )&0x20)) )
#define RAMIN0X6000 ((EXPANSION==EXP_32K)\
	||((EXPANSION==EXP_CUSTOM)&&(readinputportbytag( TAG_EXPANSION )&0x40)) )
#define RAMIN0XA000 ((EXPANSION==EXP_32K)\
	||((EXPANSION==EXP_CUSTOM)&&(readinputportbytag( TAG_EXPANSION )&0x80)) )

#define VC20ADDR2VIC6560ADDR(a) (((a)>0x8000)?((a)&0x1fff):((a)|0x2000))
#define VIC6560ADDR2VC20ADDR(a) (((a)>0x2000)?((a)&0x1fff):((a)|0x8000))


/*----------- defined in machine/vc20.c -----------*/

extern UINT8 *vc20_memory;
extern UINT8 *vc20_memory_9400;

WRITE8_HANDLER ( vc20_write_9400 );

/* split for more performance */
/* VIC reads bits 8 till 11 */
int vic6560_dma_read_color (int offset);

/* VIC reads bits 0 till 7 */
int vic6560_dma_read (int offset);

DEVICE_INIT(vc20_rom);
DEVICE_LOAD(vc20_rom);

DRIVER_INIT( vc20 );
DRIVER_INIT( vic20 );
DRIVER_INIT( vic20i );
DRIVER_INIT( vic1001 );
void vc20_driver_shutdown (void);

MACHINE_RESET( vc20 );
INTERRUPT_GEN( vc20_frame_interrupt );


#endif /* VC20_H_ */
