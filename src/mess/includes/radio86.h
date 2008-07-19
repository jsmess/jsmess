/*****************************************************************************
 *
 * includes/radio86.h
 *
 ****************************************************************************/

#ifndef radio86_H_
#define radio86_H_

#include "machine/8255ppi.h"
#include "machine/8257dma.h"
#include "video/i8275.h"

/*----------- defined in machine/radio86.c -----------*/

extern DRIVER_INIT( radio86 );
extern DRIVER_INIT( radioram );
extern MACHINE_RESET( radio86 );

extern const ppi8255_interface radio86_ppi8255_interface_1;
extern const ppi8255_interface radio86_ppi8255_interface_2;
extern const ppi8255_interface rk7007_ppi8255_interface;

extern const ppi8255_interface mikrosha_ppi8255_interface_1;
extern const ppi8255_interface mikrosha_ppi8255_interface_2;

extern const i8275_interface radio86_i8275_interface;
extern const i8275_interface partner_i8275_interface;
extern const i8275_interface mikrosha_i8275_interface;
extern const i8275_interface apogee_i8275_interface;

extern WRITE8_HANDLER ( radio86_pagesel );
extern READ8_HANDLER(radio86_dma_read_byte);
extern WRITE8_HANDLER ( radio86_write_video );
extern const dma8257_interface radio86_dma;

extern UINT8 radio86_tape_value;

extern void radio86_init_keyboard(void);
/*----------- defined in video/radio86.c -----------*/
extern UINT8 mikrosha_font_page;

extern I8275_DISPLAY_PIXELS(radio86_display_pixels);
extern I8275_DISPLAY_PIXELS(partner_display_pixels);
extern I8275_DISPLAY_PIXELS(mikrosha_display_pixels);
extern I8275_DISPLAY_PIXELS(apogee_display_pixels);

extern VIDEO_UPDATE( radio86 );
extern PALETTE_INIT( radio86 );

#endif /* radio86_H_ */
