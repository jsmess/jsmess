/*****************************************************************************
 *
 * includes/c64.h
 *
 * Commodore C64 Home Computer
 *
 * peter.trauner@jk.uni-linz.ac.at
 *
 * Documentation: www.funet.fi
 *
 ****************************************************************************/

#ifndef C64_H_
#define C64_H_

#include "machine/6526cia.h"
#include "devices/cartslot.h"

/*----------- defined in machine/c64.c -----------*/

/* private area */
extern UINT8 *c64_colorram;
extern UINT8 *c64_basic;
extern UINT8 *c64_kernal;
extern UINT8 *c64_chargen;
extern UINT8 *c64_memory;

UINT8 c64_m6510_port_read(const device_config *device, UINT8 direction);
void c64_m6510_port_write(const device_config *device, UINT8 direction, UINT8 data);

READ8_HANDLER ( c64_colorram_read );
WRITE8_HANDLER ( c64_colorram_write );

DRIVER_INIT( c64 );
DRIVER_INIT( c64pal );
DRIVER_INIT( ultimax );
DRIVER_INIT( c64gs );
DRIVER_INIT( sx64 );

MACHINE_START( c64 );
MACHINE_RESET( c64 );
INTERRUPT_GEN( c64_frame_interrupt );
TIMER_CALLBACK( c64_tape_timer );

/* private area */
READ8_HANDLER(c64_ioarea_r);
WRITE8_HANDLER(c64_ioarea_w);

WRITE8_HANDLER ( c64_write_io );
READ8_HANDLER ( c64_read_io );
int c64_paddle_read (const device_config *device, int which);
void c64_vic_interrupt (running_machine *machine, int level);

extern int c64_pal;
extern int c64_tape_on;
extern UINT8 *c64_roml;
extern UINT8 *c64_romh;
extern UINT8 *c64_vicaddr, *c128_vicaddr;
extern UINT8 c64_game, c64_exrom;
extern const mos6526_interface c64_ntsc_cia0, c64_pal_cia0;
extern const mos6526_interface c64_ntsc_cia1, c64_pal_cia1;

MACHINE_DRIVER_EXTERN( c64_cartslot );
MACHINE_DRIVER_EXTERN( ultimax_cartslot );

#endif /* C64_H_ */
