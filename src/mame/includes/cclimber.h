/*----------- defined in machine/cclimber.c -----------*/

DRIVER_INIT( cclimber );
DRIVER_INIT( cclimbrj );
DRIVER_INIT( mshuttle );
DRIVER_INIT( cannonb );
DRIVER_INIT( ckongb );

/*----------- defined in video/cclimber.c -----------*/

extern unsigned char *cclimber_bsvideoram;
extern size_t cclimber_bsvideoram_size;
extern unsigned char *cclimber_bigspriteram;
extern unsigned char *cclimber_column_scroll;

extern UINT8 *toprollr_videoram2;
extern UINT8 *toprollr_videoram3;
extern UINT8 *toprollr_videoram4;

WRITE8_HANDLER( cclimber_colorram_w );
WRITE8_HANDLER( cclimber_bigsprite_videoram_w );
PALETTE_INIT( cclimber );
VIDEO_UPDATE( cclimber );

VIDEO_UPDATE( cannonb );

VIDEO_UPDATE( yamato );
VIDEO_START( toprollr );
VIDEO_UPDATE( toprollr );

WRITE8_HANDLER( swimmer_bgcolor_w );
WRITE8_HANDLER( swimmer_palettebank_w );
PALETTE_INIT( swimmer );
VIDEO_UPDATE( swimmer );
WRITE8_HANDLER( swimmer_sidepanel_enable_w );


/*----------- defined in audio/cclimber.c -----------*/

extern struct AY8910interface cclimber_ay8910_interface;
extern struct Samplesinterface cclimber_custom_interface;
WRITE8_HANDLER( cclimber_sample_trigger_w );
WRITE8_HANDLER( cclimber_sample_rate_w );
WRITE8_HANDLER( cclimber_sample_volume_w );
