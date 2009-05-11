#ifndef __KYOCERA__
#define __KYOCERA__

#define SCREEN_TAG		"screen"
#define I8085_TAG		"i8085"
#define PIO8155_TAG		"pio8155"
#define UPD1990A_TAG	"upd1990a"

typedef struct _kyocera_state kyocera_state;
struct _kyocera_state
{
	/* video state */
	UINT16 lcd_cs2;			/* LCD driver chip select */

	const device_config *hd44102[10];
};

/* ---------- defined in video/kyocera.c ---------- */

void kyo85_set_lcd_bank(running_machine *machine, UINT16 data);

READ8_HANDLER( kyo85_lcd_status_r );
READ8_HANDLER( kyo85_lcd_data_r );
WRITE8_HANDLER( kyo85_lcd_command_w );
WRITE8_HANDLER( kyo85_lcd_data_w );

MACHINE_DRIVER_EXTERN( kyo85_video );

#endif
