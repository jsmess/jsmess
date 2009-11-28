/*****************************************************************************
 *
 * includes/3do.h
 *
 ****************************************************************************/

#ifndef _3DO_H_
#define _3DO_H_


/*----------- defined in machine/3do.c -----------*/

READ32_HANDLER( _3do_nvarea_r );
WRITE32_HANDLER( _3do_nvarea_w );

READ32_HANDLER( _3do_unk_318_r );
WRITE32_HANDLER( _3do_unk_318_w );

READ32_HANDLER( _3do_vram_sport_r );
WRITE32_HANDLER( _3do_vram_sport_w );

READ32_HANDLER( _3do_madam_r );
WRITE32_HANDLER( _3do_madam_w );
void _3do_madam_init( void );

READ32_HANDLER( _3do_clio_r );
WRITE32_HANDLER( _3do_clio_w );
void _3do_clio_init( void );


#endif /* _3DO_H_ */
