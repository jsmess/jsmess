/*****************************************************************************
 *
 * includes/atom.h
 *
 ****************************************************************************/

#ifndef ATOM_H_
#define ATOM_H_

#include "devices/snapquik.h"
#include "machine/6522via.h"
#include "machine/i8255a.h"
#include "machine/i8271.h"

/* Motherboard crystals

Source: http://acorn.chriswhy.co.uk/docs/Acorn/Manuals/Acorn_AtomTechnicalManual.pdf

*/

#define X1	XTAL_3_579545MHz	// MC6847 Clock
#define X2	XTAL_4MHz		// CPU Clock - a divider reduces it to 1MHz

/*----------- defined in machine/atom.c -----------*/

extern UINT8 atom_8255_porta;
extern UINT8 atom_8255_portc;

extern const i8255a_interface atom_8255_int;
extern const via6522_interface atom_6522_interface;
extern const i8271_interface atom_8271_interface;

MACHINE_RESET( atom );
QUICKLOAD_LOAD( atom );
READ8_DEVICE_HANDLER (atom_8255_porta_r);
READ8_DEVICE_HANDLER (atom_8255_portb_r);
READ8_DEVICE_HANDLER (atom_8255_portc_r);
WRITE8_DEVICE_HANDLER (atom_8255_porta_w );
WRITE8_DEVICE_HANDLER (atom_8255_portb_w );
WRITE8_DEVICE_HANDLER (atom_8255_portc_w );

READ8_HANDLER(atom_eprom_box_r);
WRITE8_HANDLER(atom_eprom_box_w);
void atom_eprom_box_init(running_machine *machine);

MACHINE_RESET( atomeb );

READ8_DEVICE_HANDLER( atom_mc6847_videoram_r );
VIDEO_UPDATE( atom );


#endif /* ATOM_H_ */
