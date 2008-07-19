/*****************************************************************************
 *
 * includes/partner.h
 *
 ****************************************************************************/

#ifndef partner_H_
#define partner_H_

#include "machine/8255ppi.h"
#include "machine/8257dma.h"

/*----------- defined in machine/partner.c -----------*/

extern DRIVER_INIT( partner );
extern MACHINE_RESET( partner );
extern MACHINE_START( partner );

extern DEVICE_IMAGE_LOAD( partner_floppy );

extern const dma8257_interface partner_dma;

extern WRITE8_HANDLER (partner_mem_page_w );
extern WRITE8_HANDLER (partner_win_memory_page_w);
#endif /* partner_H_ */
