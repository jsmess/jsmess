/*************************************************************************

    Jaleco Exerion

*************************************************************************/

/*----------- defined in video/exerion.c -----------*/

PALETTE_INIT( exerion );
VIDEO_START( exerion );
VIDEO_UPDATE( exerion );

WRITE8_HANDLER( exerion_videoreg_w );
WRITE8_HANDLER( exerion_video_latch_w );
READ8_HANDLER( exerion_video_timing_r );

extern UINT8 exerion_cocktail_flip;
