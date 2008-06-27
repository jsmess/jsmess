/*****************************************************************************
 *
 * includes/partner.h
 *
 ****************************************************************************/

#ifndef partner_H_
#define partner_H_

#include "machine/8255ppi.h"
#include "machine/8257dma.h"
#include "video/i8275.h"

/*----------- defined in machine/partner.c -----------*/

extern DRIVER_INIT( partner );
extern MACHINE_RESET( partner );

extern const ppi8255_interface partner_ppi8255_interface_1;
extern const i8275_interface partner_i8275_interface;
extern const dma8257_interface partner_dma;

extern WRITE8_HANDLER (partner_mem_page_w );
extern WRITE8_HANDLER (partner_win_memory_page_w);

/*----------- defined in video/partner.c -----------*/

extern I8275_DISPLAY_PIXELS(partner_display_pixels);

extern VIDEO_UPDATE( partner );
extern PALETTE_INIT( partner );


#endif /* partner_H_ */
