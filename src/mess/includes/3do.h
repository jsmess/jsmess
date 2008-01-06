/*****************************************************************************
 *
 * includes/3do.h
 *
 ****************************************************************************/

#ifndef _3DO_H_
#define _3DO_H_


/*----------- defined in machine/3do.c -----------*/

READ32_HANDLER( nvarea_r );
WRITE32_HANDLER( nvarea_w );

READ32_HANDLER( unk_318_r );
WRITE32_HANDLER( unk_318_w );

READ32_HANDLER( vram_sport_r );
WRITE32_HANDLER( vram_sport_w );

READ32_HANDLER( madam_r );
WRITE32_HANDLER( madam_w );
void madam_init( void );

READ32_HANDLER( clio_r );
WRITE32_HANDLER( clio_w );
void clio_init( void );


#endif /* _3DO_H_ */
