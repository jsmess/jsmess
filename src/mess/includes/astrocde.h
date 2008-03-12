/*****************************************************************************
 *
 * includes/astrocde.h
 *
 ****************************************************************************/

#ifndef ASTROCDE_H_
#define ASTROCDE_H_


/*----------- defined in machine/astrocde.c -----------*/

WRITE8_HANDLER( astrocade_interrupt_enable_w );
WRITE8_HANDLER( astrocade_interrupt_w );
WRITE8_HANDLER ( astrocade_interrupt_vector_w );
MACHINE_RESET( astrocde );
DRIVER_INIT( astrocde );


/*----------- defined in video/astrocde.c -----------*/

extern unsigned char *astrocade_videoram;

extern PALETTE_INIT( astrocade );
extern  READ8_HANDLER( astrocade_intercept_r );
extern WRITE8_HANDLER( astrocade_videoram_w );
extern WRITE8_HANDLER( astrocade_magic_expand_color_w );
extern WRITE8_HANDLER( astrocade_magic_control_w );
extern WRITE8_HANDLER( astrocade_magicram_w );

extern  READ8_HANDLER( astrocade_video_retrace_r );
extern WRITE8_HANDLER( astrocade_vertical_blank_w );

WRITE8_HANDLER ( astrocade_mode_w );

WRITE8_HANDLER ( astrocade_colour_register_w );
WRITE8_HANDLER ( astrocade_colour_block_w );
WRITE8_HANDLER ( astrocade_colour_split_w );

VIDEO_UPDATE( astrocde );


#endif /* ASTROCDE_H_ */
