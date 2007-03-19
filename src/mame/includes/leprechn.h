/*************************************************************************

    Leprechaun/Pot of Gold

*************************************************************************/

/*----------- defined in machine/leprechn.c -----------*/

DRIVER_INIT( leprechn );
READ8_HANDLER( leprechn_sh_0805_r );


/*----------- defined in video/leprechn.c -----------*/

PALETTE_INIT( leprechn );
VIDEO_START( leprechn );

WRITE8_HANDLER( leprechn_graphics_command_w );
READ8_HANDLER( leprechn_videoram_r );
WRITE8_HANDLER( leprechn_videoram_w );
