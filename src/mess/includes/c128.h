/*****************************************************************************
 *
 * includes/c128.h
 * 
 * Commodore C128 Home Computer
 * 
 * peter.trauner@jk.uni-linz.ac.at
 *
 * Documentation: iDOC (http://www.softwolves.pp.se/idoc)
 *   Christian Janoff <mepk@c64.org>
 * 
 ****************************************************************************/

#ifndef C128_H_
#define C128_H_


#define C128_MAIN_MEMORY		(readinputport(8)&0x300)
#define RAM128KB (0)
#define RAM256KB (0x100)
#define RAM1MB (0x200)

#define VDC_RAM_64KBYTE		(readinputport(8)&0x40) /* else 16KB */
#define MONITOR_TV		(readinputport(8)&0x20) /* else RGBI */

#define C128_KEY_RIGHT_SHIFT ((readinputport(12)&0x20)\
			 ||C128_KEY_CURSOR_UP||C128_KEY_CURSOR_LEFT)

#define C128_KEY_CURSOR_DOWN ((readinputport(12)&0x10)||C128_KEY_CURSOR_UP)
#define C128_KEY_CURSOR_RIGHT ((readinputport(12)&8)||C128_KEY_CURSOR_LEFT)

#define KEY_ESCAPE (readinputport(13)&0x8000)
#define KEY_TAB (readinputport(13)&0x4000)
#define KEY_ALT (readinputport(13)&0x2000)
#define KEY_DIN (readinputport(13)&0x1000)

#define KEY_HELP (readinputport(13)&0x0800)
#define KEY_LINEFEED (readinputport(13)&0x0400)
#define KEY_4080 (readinputport(13)&0x0200)
#define KEY_NOSCRL (readinputport(13)&0x0100)

#define KEY_UP (readinputport(13)&0x0080)
#define KEY_DOWN (readinputport(13)&0x0040)
#define KEY_LEFT (readinputport(13)&0x0020)
#define KEY_RIGHT (readinputport(13)&0x0010)

#define C128_KEY_F1 (readinputport(13)&0x0008)
#define C128_KEY_F3 (readinputport(13)&0x0004)
#define C128_KEY_F5 (readinputport(13)&0x0002)
#define C128_KEY_F7 (readinputport(13)&0x0001)

#define KEY_NUM7 (readinputport(14)&0x8000)
#define KEY_NUM8 (readinputport(14)&0x4000)
#define KEY_NUM9 (readinputport(14)&0x2000)
#define KEY_NUMPLUS (readinputport(14)&0x1000)
#define KEY_NUM4 (readinputport(14)&0x800)
#define KEY_NUM5 (readinputport(14)&0x400)
#define KEY_NUM6 (readinputport(14)&0x200)
#define KEY_NUMMINUS (readinputport(14)&0x100)
#define KEY_NUM1 (readinputport(14)&0x80)
#define KEY_NUM2 (readinputport(14)&0x40)
#define KEY_NUM3 (readinputport(14)&0x20)
#define KEY_NUM0 (readinputport(14)&0x10)
#define KEY_NUMPOINT (readinputport(14)&8)
#define KEY_NUMENTER (readinputport(14)&4)
#define C128_KEY_CURSOR_UP (readinputport(14)&2)
#define C128_KEY_CURSOR_LEFT (readinputport(14)&1)


/*----------- defined in machine/c128.c -----------*/

extern UINT8 *c128_basic;
extern UINT8 *c128_kernal;
extern UINT8 *c128_chargen;
extern UINT8 *c128_z80;
extern UINT8 *c128_editor;
extern UINT8 *c128_internal_function;
extern UINT8 *c128_external_function;
extern UINT8 *c128_vdcram;

READ8_HANDLER(c128_m6510_port_r);
WRITE8_HANDLER(c128_mmu8722_port_w);
READ8_HANDLER(c128_mmu8722_port_r);
WRITE8_HANDLER(c128_mmu8722_ff00_w);
READ8_HANDLER(c128_mmu8722_ff00_r);
int c128_capslock_r (void);
WRITE8_HANDLER(c128_write_0000);
WRITE8_HANDLER(c128_write_1000);
WRITE8_HANDLER(c128_write_4000);
WRITE8_HANDLER(c128_write_8000);
WRITE8_HANDLER(c128_write_a000);
WRITE8_HANDLER(c128_write_c000);
WRITE8_HANDLER(c128_write_d000);
WRITE8_HANDLER(c128_write_e000);
WRITE8_HANDLER(c128_write_ff00);
WRITE8_HANDLER(c128_write_ff05);

extern DRIVER_INIT( c128 );
extern DRIVER_INIT( c128pal );
extern MACHINE_RESET( c128 );

extern VIDEO_START( c128 );
extern VIDEO_UPDATE( c128 );

void c128_bankswitch_64 (int reset);


#endif /* C128_H_ */
