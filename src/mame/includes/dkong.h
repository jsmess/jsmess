/*----------- defined in video/dkong.c -----------*/

WRITE8_HANDLER( radarscp_grid_enable_w );
WRITE8_HANDLER( radarscp_grid_color_w );
WRITE8_HANDLER( dkong_flipscreen_w );
WRITE8_HANDLER( dkongjr_gfxbank_w );
WRITE8_HANDLER( dkong3_gfxbank_w );
WRITE8_HANDLER( dkong_palettebank_w );

WRITE8_HANDLER( dkong_videoram_w );

PALETTE_INIT( dkong );
PALETTE_INIT( dkong3 );
VIDEO_START( dkong );
VIDEO_UPDATE( radarscp );
VIDEO_UPDATE( dkong );
VIDEO_UPDATE( pestplce );
VIDEO_UPDATE( spclforc );


/*----------- defined in machine/strtheat.c -----------*/

DRIVER_INIT( strtheat );


/*----------- defined in machine/drakton.c -----------*/

DRIVER_INIT( drakton );


/*----------- defined in audio/dkong.c -----------*/

WRITE8_HANDLER( dkong_sh_w );
WRITE8_HANDLER( dkongjr_sh_death_w );
WRITE8_HANDLER( dkongjr_sh_drop_w );
WRITE8_HANDLER( dkongjr_sh_roar_w );
WRITE8_HANDLER( dkongjr_sh_jump_w );
WRITE8_HANDLER( dkongjr_sh_walk_w );
WRITE8_HANDLER( dkongjr_sh_climb_w );
WRITE8_HANDLER( dkongjr_sh_land_w );
WRITE8_HANDLER( dkongjr_sh_snapjaw_w );

SOUND_START( dkong );
WRITE8_HANDLER( dkong_sh1_w );
