/*****************************************************************************
 *
 * includes/mikrosha.h
 *
 ****************************************************************************/

#ifndef MIKROSHA_H_
#define MIKROSHA_H_

#include "machine/8255ppi.h"
#include "machine/8257dma.h"
#include "video/i8275.h"

/*----------- defined in machine/mikrosha.c -----------*/

extern DRIVER_INIT( mikrosha );
extern MACHINE_RESET( mikrosha );

extern const ppi8255_interface mikrosha_ppi8255_interface_1;
extern const ppi8255_interface mikrosha_ppi8255_interface_2;
extern const i8275_interface mikrosha_i8275_interface;
extern const dma8257_interface mikrosha_dma;

/*----------- defined in video/mikrosha.c -----------*/

extern I8275_DISPLAY_PIXELS(mikrosha_display_pixels);
extern UINT8 mikrosha_font_page;
extern VIDEO_UPDATE( mikrosha );
extern PALETTE_INIT( mikrosha );


#endif /* MIKROSHA_H_ */
