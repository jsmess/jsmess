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


#define C128_MAIN_MEMORY	( readinputportbytag("Config") & 0x300 )
#define RAM128KB			( 0x00 )
#define RAM256KB			( 0x100 )
#define RAM1MB				( 0x200 )

#define VDC_RAM_64KBYTE		( readinputportbytag("Config") & 0x40 ) /* else 16KB */
#define MONITOR_TV			( readinputportbytag("Config") & 0x20 ) /* else RGBI */

#define KEY_DIN				( readinputportbytag( "Special" ) & 0x20 )
#define KEY_4080			( readinputportbytag( "Special" ) & 0x10 )


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

void c128_bankswitch_64 (running_machine *machine, int reset);


#endif /* C128_H_ */
