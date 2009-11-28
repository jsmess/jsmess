/*****************************************************************************
 *
 * includes/bbc.h
 *
 * BBC Model B
 *
 * Driver by Gordon Jefferyes <mess_bbc@gjeffery.dircon.co.uk>
 *
 ****************************************************************************/

#ifndef BBC_H_
#define BBC_H_

#include "machine/6522via.h"
#include "machine/6850acia.h"
#include "machine/i8271.h"
#include "machine/wd17xx.h"
#include "machine/upd7002.h"


/*----------- defined in machine/bbc.c -----------*/

extern const via6522_interface bbcb_system_via;
extern const via6522_interface bbcb_user_via;
extern const wd17xx_interface bbc_wd17xx_interface;
DRIVER_INIT( bbc );
DRIVER_INIT( bbcm );

MACHINE_START( bbca );
MACHINE_START( bbcb );
MACHINE_START( bbcbp );
MACHINE_START( bbcm );

MACHINE_RESET( bbca );
MACHINE_RESET( bbcb );
MACHINE_RESET( bbcbp );
MACHINE_RESET( bbcm );

INTERRUPT_GEN( bbcb_keyscan );
INTERRUPT_GEN( bbcm_keyscan );

WRITE8_HANDLER ( bbc_memorya1_w );
WRITE8_HANDLER ( bbc_page_selecta_w );

WRITE8_HANDLER ( bbc_memoryb3_w );
WRITE8_HANDLER ( bbc_memoryb4_w );
WRITE8_HANDLER ( bbc_page_selectb_w );


WRITE8_HANDLER ( bbc_memorybp1_w );
//READ8_HANDLER  ( bbc_memorybp2_r );
WRITE8_HANDLER ( bbc_memorybp2_w );
WRITE8_HANDLER ( bbc_memorybp4_w );
WRITE8_HANDLER ( bbc_memorybp4_128_w );
WRITE8_HANDLER ( bbc_memorybp6_128_w );
WRITE8_HANDLER ( bbc_page_selectbp_w );


WRITE8_HANDLER ( bbc_memorybm1_w );
//READ8_HANDLER  ( bbc_memorybm2_r );
WRITE8_HANDLER ( bbc_memorybm2_w );
WRITE8_HANDLER ( bbc_memorybm4_w );
WRITE8_HANDLER ( bbc_memorybm5_w );
WRITE8_HANDLER ( bbc_memorybm7_w );
READ8_HANDLER  ( bbcm_r );
WRITE8_HANDLER ( bbcm_w );
READ8_HANDLER  ( bbcm_ACCCON_read );
WRITE8_HANDLER ( bbcm_ACCCON_write );


//WRITE8_HANDLER ( bbc_bank4_w );

/* disc support */

DEVICE_IMAGE_LOAD ( bbcb_cart );

READ8_HANDLER  ( bbc_disc_r );
WRITE8_HANDLER ( bbc_disc_w );

READ8_HANDLER  ( bbc_wd1770_read );
WRITE8_HANDLER ( bbc_wd1770_write );

READ8_HANDLER  ( bbc_opus_read );
WRITE8_HANDLER ( bbc_opus_write );


READ8_HANDLER  ( bbcm_wd1770l_read );
WRITE8_HANDLER ( bbcm_wd1770l_write );
READ8_HANDLER  ( bbcm_wd1770_read );
WRITE8_HANDLER ( bbcm_wd1770_write );


/* tape support */

WRITE8_HANDLER ( bbc_SerialULA_w );

extern const i8271_interface bbc_i8271_interface;
extern const uPD7002_interface bbc_uPD7002;

/*----------- defined in video/bbc.c -----------*/

extern VIDEO_START( bbca );
extern VIDEO_START( bbcb );
extern VIDEO_START( bbcbp );
extern VIDEO_START( bbcm );
extern VIDEO_UPDATE( bbc );

extern unsigned char bbc_vidmem[0x10000];

void bbc_set_video_memory_lookups(int ramsize);
void bbc_frameclock(void);
void bbc_setscreenstart(int b4,int b5);
void bbcbp_setvideoshadow(running_machine *machine, int vdusel);

WRITE8_HANDLER ( bbc_videoULA_w );

WRITE8_HANDLER ( bbc_6845_w );
READ8_HANDLER ( bbc_6845_r );


#endif /* BBC_H_ */
