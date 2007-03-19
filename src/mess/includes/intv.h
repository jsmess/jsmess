#ifndef __INTV_H
#define __INTV_H

/* in vidhrdw/intv.c */
extern VIDEO_START( intv );
extern VIDEO_UPDATE( intv );
extern VIDEO_START( intvkbd );
extern VIDEO_UPDATE( intvkbd );

/* in machine/intv.c */

/*  for the console alone... */
extern UINT8 intv_gramdirty;
extern UINT8 intv_gram[];
extern UINT8 intv_gramdirtybytes[];
extern UINT16 intv_ram16[];

DEVICE_INIT( intv_cart );
DEVICE_LOAD( intv_cart );

extern MACHINE_RESET( intv );
extern INTERRUPT_GEN( intv_interrupt );

READ16_HANDLER( intv_gram_r );
WRITE16_HANDLER( intv_gram_w );
READ16_HANDLER( intv_empty_r );
READ16_HANDLER( intv_ram8_r );
WRITE16_HANDLER( intv_ram8_w );
READ16_HANDLER( intv_ram16_r );
WRITE16_HANDLER( intv_ram16_w );

void stic_screenrefresh(void);

READ8_HANDLER( intv_right_control_r );
READ8_HANDLER( intv_left_control_r );

/* for the console + keyboard component... */
extern int intvkbd_text_blanked;

DEVICE_LOAD( intvkbd_cart );

extern UINT16 *intvkbd_dualport_ram;
WRITE16_HANDLER ( intvkbd_dualport16_w );
READ8_HANDLER ( intvkbd_dualport8_lsb_r );
WRITE8_HANDLER ( intvkbd_dualport8_lsb_w );
READ8_HANDLER ( intvkbd_dualport8_msb_r );
WRITE8_HANDLER ( intvkbd_dualport8_msb_w );

READ8_HANDLER ( intvkbd_tms9927_r );
WRITE8_HANDLER ( intvkbd_tms9927_w );

/* in sndhrdw/intv.c */
READ16_HANDLER( AY8914_directread_port_0_lsb_r );
WRITE16_HANDLER( AY8914_directwrite_port_0_lsb_w );

#endif

