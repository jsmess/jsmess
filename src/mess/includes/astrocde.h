/*****************************************************************************
 *
 * includes/astrocde.h
 *
 ****************************************************************************/

#ifndef ASTROCDE_H_
#define ASTROCDE_H_


#define XTAL_Y1  XTAL_14_31818MHz


/*----------- defined in machine/astrocde.c -----------*/

WRITE8_HANDLER( astrocade_interrupt_enable_w );
WRITE8_HANDLER( astrocade_interrupt_w );
WRITE8_HANDLER ( astrocade_interrupt_vector_w );
MACHINE_RESET( astrocde );
DRIVER_INIT( astrocde );


/*----------- defined in video/astrocde.c -----------*/

PALETTE_INIT( astrocade );
READ8_HANDLER( astrocade_intercept_r );
WRITE8_HANDLER( astrocade_magic_expand_color_w );
WRITE8_HANDLER( astrocade_magic_control_w );
WRITE8_HANDLER( astrocade_magicram_w );

READ8_HANDLER( astrocade_video_retrace_r );
WRITE8_HANDLER( astrocade_vertical_blank_w );

WRITE8_HANDLER( astrocade_mode_w );

WRITE8_HANDLER( astrocade_colour_register_w );
WRITE8_HANDLER( astrocade_colour_block_w );
WRITE8_HANDLER( astrocade_colour_split_w );

VIDEO_UPDATE( astrocde );


#endif /* ASTROCDE_H_ */
