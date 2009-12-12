#ifndef _CPSCHNGHR_H_
#define _CPSCHNGHR_H_


/*----------- defined in machine/kabuki.c -----------*/


void wof_decode(running_machine *machine);


/*----------- defined in video/cpschngr.c -----------*/


extern UINT16 *cps1_gfxram;     /* Video RAM */
extern UINT16 *cps1_cps_a_regs;
extern UINT16 *cps1_cps_b_regs;
extern size_t cps1_gfxram_size;

WRITE16_HANDLER( cps1_cps_a_w );
WRITE16_HANDLER( cps1_cps_b_w );
READ16_HANDLER( cps1_cps_b_r );
WRITE16_HANDLER( cps1_gfxram_w );

DRIVER_INIT( cps1 );

VIDEO_START( cps1 );
VIDEO_UPDATE( cps1 );
VIDEO_EOF( cps1 );

#endif
