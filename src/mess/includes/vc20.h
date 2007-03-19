
#ifndef __VC20_H_
#define __VC20_H_

#include "driver.h"

#include "cbmserb.h"

#define KEY_ARROW_LEFT (readinputport(5)&0x8000)
#define KEY_1 (readinputport(5)&0x4000)
#define KEY_2 (readinputport(5)&0x2000)
#define KEY_3 (readinputport(5)&0x1000)
#define KEY_4 (readinputport(5)&0x0800)
#define KEY_5 (readinputport(5)&0x0400)
#define KEY_6 (readinputport(5)&0x0200)
#define KEY_7 (readinputport(5)&0x0100)
#define KEY_8 (readinputport(5)&0x0080)
#define KEY_9 (readinputport(5)&0x0040)
#define KEY_0 (readinputport(5)&0x0020)
#define KEY_PLUS (readinputport(5)&0x0010)
#define KEY_MINUS (readinputport(5)&0x0008)
#define KEY_POUND (readinputport(5)&0x0004)
#define KEY_HOME (readinputport(5)&0x0002)
#define KEY_DEL (readinputport(5)&0x0001)

#define KEY_CTRL (readinputport(6)&0x8000)
#define KEY_Q (readinputport(6)&0x4000)
#define KEY_W (readinputport(6)&0x2000)
#define KEY_E (readinputport(6)&0x1000)
#define KEY_R (readinputport(6)&0x0800)
#define KEY_T (readinputport(6)&0x0400)
#define KEY_Y (readinputport(6)&0x0200)
#define KEY_U (readinputport(6)&0x0100)
#define KEY_I (readinputport(6)&0x0080)
#define KEY_O (readinputport(6)&0x0040)
#define KEY_P (readinputport(6)&0x0020)
#define KEY_AT (readinputport(6)&0x0010)
#define KEY_ASTERIX (readinputport(6)&0x0008)
#define KEY_ARROW_UP (readinputport(6)&0x0004)
#define KEY_RESTORE (readinputport(6)&0x0002)

#define KEY_STOP (readinputport(7)&0x8000)
#define KEY_SHIFT_LOCK (readinputport(7)&0x4000)
#define KEY_A (readinputport(7)&0x2000)
#define KEY_S (readinputport(7)&0x1000)
#define KEY_D (readinputport(7)&0x0800)
#define KEY_F (readinputport(7)&0x0400)
#define KEY_G (readinputport(7)&0x0200)
#define KEY_H (readinputport(7)&0x0100)
#define KEY_J (readinputport(7)&0x0080)
#define KEY_K (readinputport(7)&0x0040)
#define KEY_L (readinputport(7)&0x0020)
#define KEY_COLON (readinputport(7)&0x0010)
#define KEY_SEMICOLON (readinputport(7)&0x0008)
#define KEY_EQUALS (readinputport(7)&0x0004)
#define KEY_RETURN (readinputport(7)&0x0002)

#define KEY_SPACE (readinputport(9)&0x8000)
#define KEY_F1 (readinputport(9)&0x4000)
#define KEY_F3 (readinputport(9)&0x2000)
#define KEY_F5 (readinputport(9)&0x1000)
#define KEY_F7 (readinputport(9)&0x0800)
#define KEY_UP (readinputport(9)&0x0400)
#define KEY_LEFT (readinputport(9)&0x0200)

#define KEY_CBM (readinputport(8)&0x8000)
#define KEY_LEFT_SHIFT ((readinputport(8)&0x4000)||KEY_SHIFT_LOCK)
#define KEY_Z (readinputport(8)&0x2000)
#define KEY_X (readinputport(8)&0x1000)
#define KEY_C (readinputport(8)&0x0800)
#define KEY_V (readinputport(8)&0x0400)
#define KEY_B (readinputport(8)&0x0200)
#define KEY_N (readinputport(8)&0x0100)
#define KEY_M (readinputport(8)&0x0080)
#define KEY_COMMA (readinputport(8)&0x0040)
#define KEY_POINT (readinputport(8)&0x0020)
#define KEY_SLASH (readinputport(8)&0x0010)
#define KEY_RIGHT_SHIFT ((readinputport(8)&0x0008)||KEY_UP||KEY_LEFT)
#define KEY_DOWN ((readinputport(8)&0x0004)||KEY_UP)
#define KEY_RIGHT ((readinputport(8)&0x0002)||KEY_LEFT)

/**
  joystick (4 directions switches, 1 button),
  paddles (2) button, 270 degree wheel,
  and lightpen are all connected to the gameport connector
  lightpen to joystick button
  paddle 1 button to joystick left
  paddle 2 button to joystick right
 */

#define JOYSTICK (readinputport(11)&0x80)
#define PADDLES (readinputport(11)&0x40)
#define LIGHTPEN (readinputport(11)&0x20)
#define LIGHTPEN_POINTER (LIGHTPEN&&(readinputport(11)&0x10))
#define LIGHTPEN_X_VALUE (readinputport(3)&~1)		/* effectiv resolution */
#define LIGHTPEN_Y_VALUE (readinputport(4)&~1)		/* effectiv resolution */
#define LIGHTPEN_BUTTON (LIGHTPEN&&(readinputport(0)&0x80))

#define JOYSTICK_UP (readinputport(0)&1)
#define JOYSTICK_DOWN (readinputport(0)&2)
#define JOYSTICK_LEFT (readinputport(0)&4)
#define JOYSTICK_RIGHT (readinputport(0)&8)
#define JOYSTICK_BUTTON (readinputport(0)&0x10)

#define PADDLE1_BUTTON (readinputport(0)&0x20)
#define PADDLE2_BUTTON (readinputport(0)&0x40)

#define PADDLE1_VALUE   readinputport(1)
#define PADDLE2_VALUE	readinputport(2)


#define DATASSETTE (readinputport(11)&0x8)
#define DATASSETTE_TONE (readinputport(11)&4)

#define QUICKLOAD		(readinputport(9)&8)

#define DATASSETTE_PLAY		(readinputport(9)&4)
#define DATASSETTE_RECORD	(readinputport(9)&2)
#define DATASSETTE_STOP		(readinputport(9)&1)

/* macros to access the dip switches */
#define EXPANSION (readinputport(10)&7)
#define EXP_3K 1
#define EXP_8K 2
#define EXP_16K 3
#define EXP_32K 4
#define EXP_CUSTOM 5
#define RAMIN0X0400 ((EXPANSION==EXP_3K)\
	||((EXPANSION==EXP_CUSTOM)&&(readinputport(10)&8)) )
#define RAMIN0X2000 ((EXPANSION==EXP_8K)||(EXPANSION==EXP_16K)\
	||(EXPANSION==EXP_32K)\
	||((EXPANSION==EXP_CUSTOM)&&(readinputport(10)&0x10)) )
#define RAMIN0X4000 ((EXPANSION==EXP_16K)||(EXPANSION==EXP_32K)\
	||((EXPANSION==EXP_CUSTOM)&&(readinputport(10)&0x20)) )
#define RAMIN0X6000 ((EXPANSION==EXP_32K)\
	||((EXPANSION==EXP_CUSTOM)&&(readinputport(10)&0x40)) )
#define RAMIN0XA000 ((EXPANSION==EXP_32K)\
	||((EXPANSION==EXP_CUSTOM)&&(readinputport(10)&0x80)) )

#define VC20ADDR2VIC6560ADDR(a) (((a)>0x8000)?((a)&0x1fff):((a)|0x2000))
#define VIC6560ADDR2VC20ADDR(a) (((a)>0x2000)?((a)&0x1fff):((a)|0x8000))

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

#endif

