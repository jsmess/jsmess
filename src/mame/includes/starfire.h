/*----------- defined in drivers/starfire.c -----------*/

extern UINT8 *starfire_videoram;
extern UINT8 *starfire_colorram;


/*----------- defined in video/starfire.c -----------*/

VIDEO_UPDATE( starfire );
VIDEO_START( starfire );
void starfire_video_update(int scanline, int count);

WRITE8_HANDLER( starfire_videoram_w );
READ8_HANDLER( starfire_videoram_r );
WRITE8_HANDLER( starfire_colorram_w );
READ8_HANDLER( starfire_colorram_r );
WRITE8_HANDLER( starfire_vidctrl_w );
WRITE8_HANDLER( starfire_vidctrl1_w );

