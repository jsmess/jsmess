/*************************************************************************

    8080bw.h

*************************************************************************/

#include "sound/discrete.h"


/*----------- defined in audio/8080bw.c -----------*/

MACHINE_RESET( sstrangr );
MACHINE_RESET( spcewars );
MACHINE_RESET( lrescue );
MACHINE_RESET( ballbomb );
MACHINE_RESET( schaser );
MACHINE_RESET( polaris );
MACHINE_RESET( indianbt );
MACHINE_RESET( invrvnge );
MACHINE_RESET( lupin3 );
MACHINE_RESET( rollingc );
MACHINE_RESET( schasrcv );
MACHINE_RESET( shuttlei );
MACHINE_RESET( yosakdon );

WRITE8_HANDLER( invadpt2_sh_port_1_w );
WRITE8_HANDLER( invadpt2_sh_port_2_w );

extern struct Samplesinterface lrescue_samples_interface;
extern discrete_sound_block polaris_discrete_interface[];
extern discrete_sound_block indianbt_discrete_interface[];
extern discrete_sound_block schaser_discrete_interface[];
void schaser_effect_555_cb(int effect);
extern int schaser_sx10;
extern mame_timer *schaser_effect_555_timer;
extern struct SN76477interface schaser_sn76477_interface;


/*----------- defined in video/8080bw.c -----------*/

DRIVER_INIT( 8080bw );
DRIVER_INIT( invadpt2 );
DRIVER_INIT( cosmo );
DRIVER_INIT( sstrngr2 );
DRIVER_INIT( invaddlx );
DRIVER_INIT( schaser );
DRIVER_INIT( schasrcv );
DRIVER_INIT( rollingc );
DRIVER_INIT( polaris );
DRIVER_INIT( lupin3 );
DRIVER_INIT( indianbt );
DRIVER_INIT( shuttlei );

void c8080bw_flip_screen_w(int data);
void c8080bw_screen_red_w(int data);

INTERRUPT_GEN( polaris_interrupt );

WRITE8_HANDLER( c8080bw_videoram_w );

VIDEO_UPDATE( 8080bw );

PALETTE_INIT( invadpt2 );
PALETTE_INIT( sflush );
PALETTE_INIT( cosmo );
PALETTE_INIT( indianbt );
