/*****************************************************************************
 *
 * includes/c65.h
 *
 ****************************************************************************/

#ifndef C65_H_
#define C65_H_


#define C65_KEY_TAB (readinputport(10)&0x8000)
#define C65_KEY_CTRL (readinputport(10)&1)

#define C65_KEY_RIGHT_SHIFT ((readinputport(12)&0x20)\
			 ||C65_KEY_CURSOR_UP||C65_KEY_CURSOR_LEFT)
#define C65_KEY_CURSOR_UP (readinputport(12)&0x10)
#define C65_KEY_SPACE (readinputport(12)&8)
#define C65_KEY_CURSOR_LEFT (readinputport(12)&4)
#define C65_KEY_CURSOR_DOWN ((readinputport(12)&2)||C65_KEY_CURSOR_UP)
#define C65_KEY_CURSOR_RIGHT ((readinputport(12)&1)||C65_KEY_CURSOR_LEFT)

#define C65_KEY_STOP (readinputport(13)&0x8000)
#define C65_KEY_ESCAPE (readinputport(13)&0x4000)
#define C65_KEY_ALT (readinputport(13)&0x2000)
#define C65_KEY_DIN (readinputport(13)&0x1000)
#define C65_KEY_NOSCRL (readinputport(13)&0x0800)
#define C65_KEY_F1 (readinputport(13)&0x0400)
#define C65_KEY_F3 (readinputport(13)&0x0200)
#define C65_KEY_F5 (readinputport(13)&0x0100)
#define C65_KEY_F7 (readinputport(13)&0x0080)
#define C65_KEY_F9 (readinputport(13)&0x0040)
#define C65_KEY_F11 (readinputport(13)&0x0020)
#define C65_KEY_F13 (readinputport(13)&0x0010)
#define C65_KEY_HELP (readinputport(13)&0x0008)


/*----------- defined in machine/c65.c -----------*/

/*extern UINT8 *c65_memory; */
/*extern UINT8 *c65_basic; */
/*extern UINT8 *c65_kernal; */
extern UINT8 *c65_chargen;
/*extern UINT8 *c65_dos; */
/*extern UINT8 *c65_monitor; */
extern UINT8 *c65_interface;
/*extern UINT8 *c65_graphics; */

void c65_bankswitch (void);
void c65_colorram_write (int offset, int value);

void c65_driver_init (void);
void c65_driver_alpha1_init (void);
void c65pal_driver_init (void);
MACHINE_START( c65 );


#endif /* C65_H_ */
