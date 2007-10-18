/***************************************************************************
	commodore pet series computer

    peter.trauner@jk.uni-linz.ac.at
***************************************************************************/
#ifndef __PET_H_
#define __PET_H_

#include "driver.h"

/* call to init videodriver */
extern void pet_vh_init (void);
extern void pet80_vh_init (void);
extern void superpet_vh_init (void);
extern VIDEO_UPDATE( pet );
void pet40_update_row(mame_bitmap *bitmap, const rectangle *cliprect, UINT16 ma,
					  UINT8 ra, UINT16 y, UINT8 x_count, void *param);
void pet80_update_row(mame_bitmap *bitmap, const rectangle *cliprect, UINT16 ma,
					  UINT8 ra, UINT16 y, UINT8 x_count, void *param);

extern int pet_font;

#define QUICKLOAD		(input_port_11_word_r(0,0)&0x8000)
#define DATASSETTE (input_port_11_word_r(0,0)&0x4000)
#define DATASSETTE_TONE (input_port_11_word_r(0,0)&0x2000)

#define DATASSETTE_PLAY		(input_port_11_word_r(0,0)&0x1000)
#define DATASSETTE_RECORD	(input_port_11_word_r(0,0)&0x800)
#define DATASSETTE_STOP		(input_port_11_word_r(0,0)&0x400)

#define BUSINESS_KEYBOARD (input_port_11_word_r(0,0)&0x200)

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
DRIVER_INIT( pet1 );
DRIVER_INIT( pet40 );
DRIVER_INIT( cbm80 );
DRIVER_INIT( superpet );
MACHINE_RESET( pet );
INTERRUPT_GEN( pet_frame_interrupt );

void pet_rom_load(void);

#endif
