#include "devices/snapquik.h"

/* machine/atom.c */
MACHINE_RESET( atom );
QUICKLOAD_LOAD( atom );
 READ8_HANDLER (atom_8255_porta_r);
 READ8_HANDLER (atom_8255_portb_r);
 READ8_HANDLER (atom_8255_portc_r);
WRITE8_HANDLER (atom_8255_porta_w );
WRITE8_HANDLER (atom_8255_portb_w );
WRITE8_HANDLER (atom_8255_portc_w );

/* machine/vidhrdw.c */
VIDEO_START( atom );

/* for floppy disc interface */
 READ8_HANDLER (atom_8271_r);
WRITE8_HANDLER (atom_8271_w);

DEVICE_LOAD( atom_floppy );

 READ8_HANDLER(atom_eprom_box_r);
WRITE8_HANDLER(atom_eprom_box_w);
void atom_eprom_box_init(void);

MACHINE_RESET( atomeb );
