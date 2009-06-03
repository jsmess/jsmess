/*----- super80 ----------*/


PALETTE_INIT( super80m );
VIDEO_UPDATE( super80 );
VIDEO_UPDATE( super80d );
VIDEO_UPDATE( super80e );
VIDEO_UPDATE( super80m );
VIDEO_START( super80 );
VIDEO_EOF( super80m );

/*----- Super80V ---------*/

READ8_HANDLER( super80v_low_r );
READ8_HANDLER( super80v_high_r );
WRITE8_HANDLER( super80v_low_w );
WRITE8_HANDLER( super80v_high_w );
WRITE8_HANDLER( super80v_10_w );
WRITE8_HANDLER( super80v_11_w );
WRITE8_HANDLER( super80_f1_w );
VIDEO_START( super80v );
VIDEO_UPDATE( super80v );
MC6845_UPDATE_ROW( super80v_update_row );

extern UINT8 *pcgram;
extern UINT8 super80v_vid_col;
extern UINT8 super80v_rom_pcg;
extern UINT8 super80_mhz;
