/******************************************************************************
 *
 *	kaypro.h
 *
 *	interface for Kaypro 2x
 *
 *	Juergen Buchmueller, July 1998
 *
 ******************************************************************************/

#include "machine/wd17xx.h"
#include "machine/cpm_bios.h"

DRIVER_INIT( kaypro );
MACHINE_RESET( kaypro );

INTERRUPT_GEN( kaypro_interrupt );

#define KAYPRO_FONT_W 	8
#define KAYPRO_FONT_H 	16
#define KAYPRO_SCREEN_W	80
#define KAYPRO_SCREEN_H   25

VIDEO_START( kaypro );
VIDEO_UPDATE( kaypro );

READ8_HANDLER ( kaypro_const_r );
WRITE8_HANDLER ( kaypro_const_w );
READ8_HANDLER ( kaypro_conin_r );
WRITE8_HANDLER ( kaypro_conin_w );
READ8_HANDLER ( kaypro_conout_r );
WRITE8_HANDLER ( kaypro_conout_w );

void kaypro_bell(void);
void kaypro_click(void);
