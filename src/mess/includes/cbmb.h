/*****************************************************************************
 *
 * includes/cbmb.h
 * 
 * Commodore B Series Computer
 *
 * peter.trauner@jk.uni-linz.ac.at
 * 
 ****************************************************************************/

#ifndef CBMB_H_
#define CBMB_H_

#include "video/mc6845.h"


/*----------- defined in machine/cbmb.c -----------*/

extern UINT8 *cbmb_basic;
extern UINT8 *cbmb_kernal;
//extern UINT8 *cbmb_chargen;
extern UINT8 *cbmb_memory;
extern UINT8 *cbmb_videoram;
extern UINT8 *cbmb_colorram;

WRITE8_HANDLER ( cbmb_colorram_w );

DRIVER_INIT( cbm500 );
DRIVER_INIT( cbm600 );
DRIVER_INIT( cbm600pal );
DRIVER_INIT( cbm600hu );
DRIVER_INIT( cbm700 );
MACHINE_RESET( cbmb );

void cbmb_cartslot_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);


/*----------- defined in video/cbmb.c -----------*/

VIDEO_START( cbmb_crtc );
VIDEO_UPDATE( cbmb_crtc );
MC6845_UPDATE_ROW( cbm600_update_row );
MC6845_UPDATE_ROW( cbm700_update_row );
MC6845_ON_DE_CHANGED( cbmb_display_enable_changed );

void cbm600_vh_init(running_machine *machine);
void cbm700_vh_init(running_machine *machine);
VIDEO_START( cbm700 );

void cbmb_vh_set_font(int font);


#endif /* CBMB_H_ */
