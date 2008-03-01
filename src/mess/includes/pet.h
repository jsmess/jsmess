/***************************************************************************
    commodore pet series computer

    peter.trauner@jk.uni-linz.ac.at
***************************************************************************/

#ifndef PET_H_
#define PET_H_

#include "video/mc6845.h"

READ8_HANDLER( pet_mc6845_register_r );
WRITE8_HANDLER( pet_mc6845_register_w );
WRITE8_HANDLER( pet_mc6845_address_w );

/*----------- defined in video/pet.c -----------*/

/* call to init videodriver */
void pet_vh_init (void);
void pet80_vh_init (void);
void superpet_vh_init (void);
VIDEO_UPDATE( pet );
MC6845_UPDATE_ROW( pet40_update_row );
MC6845_UPDATE_ROW( pet80_update_row );
void pet_display_enable_changed(running_machine *machine, mc6845_t *mc6845, int display_enabled);

extern int pet_font;


/*----------- defined in machine/pet.c -----------*/

#define CBM8096_MEMORY (input_port_11_r(0)&8)
#define M6809_SELECT (input_port_11_r(0)&4)
#define IEEE8ON (input_port_11_r(0)&2)
#define IEEE9ON (input_port_11_r(0)&1)

extern UINT8 *pet_memory;
extern UINT8 *pet_videoram;
extern UINT8 *superpet_memory;

WRITE8_HANDLER(cbm8096_w);
extern READ8_HANDLER(superpet_r);
extern WRITE8_HANDLER(superpet_w);

DRIVER_INIT( pet );
DRIVER_INIT( petb );
DRIVER_INIT( pet1 );
DRIVER_INIT( pet40 );
DRIVER_INIT( cbm80 );
DRIVER_INIT( superpet );
MACHINE_RESET( pet );
INTERRUPT_GEN( pet_frame_interrupt );

void pet_rom_load(void);


#endif /* PET_H_ */
