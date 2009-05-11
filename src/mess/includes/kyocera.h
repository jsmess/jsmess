#ifndef __KYOCERA__
#define __KYOCERA__

#define SCREEN_TAG		"screen"
#define I8085_TAG		"i8085"
#define PIO8155_TAG		"pio8155"
#define UPD1990A_TAG	"upd1990a"

/* ---------- defined in video/kyocera.c ---------- */

void kyo85_set_lcd_bank(UINT16 data);

READ8_HANDLER( kyo85_lcd_status_r );
READ8_HANDLER( kyo85_lcd_data_r );
WRITE8_HANDLER( kyo85_lcd_command_w );
WRITE8_HANDLER( kyo85_lcd_data_w );

PALETTE_INIT( kyo85 );
VIDEO_START( kyo85 );
VIDEO_UPDATE( kyo85 );

#endif
