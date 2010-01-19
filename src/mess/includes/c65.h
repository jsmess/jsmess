/*****************************************************************************
 *
 * includes/c65.h
 *
 ****************************************************************************/

#ifndef C65_H_
#define C65_H_

#include "machine/6526cia.h"

/*----------- defined in machine/c65.c -----------*/

/*extern UINT8 *c65_memory; */
/*extern UINT8 *c65_basic; */
/*extern UINT8 *c65_kernal; */
extern UINT8 *c65_chargen;
/*extern UINT8 *c65_dos; */
/*extern UINT8 *c65_monitor; */
extern UINT8 *c65_interface;
/*extern UINT8 *c65_graphics; */

void c65_bankswitch (running_machine *machine);
void c65_colorram_write (int offset, int value);

DRIVER_INIT( c65 );
DRIVER_INIT( c65pal );
MACHINE_START( c65 );
INTERRUPT_GEN( c65_frame_interrupt );

extern const mos6526_interface c65_ntsc_cia0, c65_pal_cia0;
extern const mos6526_interface c65_ntsc_cia1, c65_pal_cia1;

#endif /* C65_H_ */
