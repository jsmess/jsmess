/***************************************************************************

      Dynax hardware

***************************************************************************/

/*----------- defined in drivers/dynax.c -----------*/

extern UINT8 dynax_blitter_irq,	dynax_blitter2_irq;

void sprtmtch_update_irq(void);
void jantouki_update_irq(void);
void mjelctrn_update_irq(void);
void neruton_update_irq(void);

/*----------- defined in video/dynax.c -----------*/

WRITE8_HANDLER( dynax_blitter_rev2_w );
WRITE8_HANDLER( jantouki_blitter_rev2_w );
WRITE8_HANDLER( jantouki_blitter2_rev2_w );
WRITE8_HANDLER( tenkai_blitter_rev2_w );


WRITE8_HANDLER( dynax_blit_pen_w );
WRITE8_HANDLER( dynax_blit2_pen_w );
WRITE8_HANDLER( dynax_blit_backpen_w );
WRITE8_HANDLER( dynax_blit_dest_w );
WRITE8_HANDLER( dynax_blit2_dest_w );
WRITE8_HANDLER( dynax_blit_palbank_w );
WRITE8_HANDLER( dynax_blit2_palbank_w );
WRITE8_HANDLER( dynax_blit_palette01_w );
WRITE8_HANDLER( dynax_blit_palette23_w );
WRITE8_HANDLER( dynax_blit_palette45_w );
WRITE8_HANDLER( dynax_blit_palette67_w );
WRITE8_HANDLER( dynax_layer_enable_w );
WRITE8_HANDLER( jantouki_layer_enable_w );
WRITE8_HANDLER( dynax_flipscreen_w );
WRITE8_HANDLER( dynax_extra_scrollx_w );
WRITE8_HANDLER( dynax_extra_scrolly_w );
WRITE8_HANDLER( dynax_blit_romregion_w );
WRITE8_HANDLER( dynax_blit2_romregion_w );

WRITE8_HANDLER( hanamai_layer_half_w );
WRITE8_HANDLER( hnoridur_layer_half2_w );

WRITE8_HANDLER( hanamai_priority_w );
WRITE8_HANDLER( tenkai_priority_w );

WRITE8_HANDLER( mjdialq2_blit_dest_w );
WRITE8_HANDLER( tenkai_blit_dest_w );

WRITE8_HANDLER( mjdialq2_layer_enable_w );

WRITE8_HANDLER( tenkai_blit_palette01_w );
WRITE8_HANDLER( tenkai_blit_palette23_w );

VIDEO_START( hanamai );
VIDEO_START( hnoridur );
VIDEO_START( mcnpshnt );
VIDEO_START( sprtmtch );
VIDEO_START( mjdialq2 );
VIDEO_START( jantouki );
VIDEO_START( mjelctrn );
VIDEO_START( neruton );

VIDEO_UPDATE( hanamai );
VIDEO_UPDATE( hnoridur );
VIDEO_UPDATE( sprtmtch );
VIDEO_UPDATE( mjdialq2 );
VIDEO_UPDATE( jantouki );

PALETTE_INIT( sprtmtch );
