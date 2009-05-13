#ifndef __KYOCERA__
#define __KYOCERA__

#define SCREEN_TAG		"screen"
#define I8085_TAG		"i8085"
#define PIO8155_TAG		"pio8155"
#define UPD1990A_TAG	"upd1990a"
#define RP5C01A_TAG		"rp5c01a"
#define TCM5089_TAG		"tcm5089"
#define HD61830_TAG		"hd61830b"

typedef struct _kyocera_state kyocera_state;
struct _kyocera_state
{
	/* memory state */
	UINT8 bank;				/* memory bank selection */

	/* clock state */
	int upd1990a_data;		/* RTC data output */

	/* video state */
	UINT16 lcd_cs2;			/* LCD driver chip select */

	const device_config *hd44102[10];
	const device_config *upd1990a;
	const device_config *centronics;
};

typedef struct _trsm200_state trsm200_state;
struct _trsm200_state
{
	/* memory state */
	UINT8 bank;				/* memory bank selection */

	/* keyboard state */
	UINT16 keylatch;
	int tp;					/* timing pulse */

	/* video state */
	UINT8 *video_ram;		/* video RAM */

	const device_config *hd61830;
	const device_config *centronics;
};

/* ---------- defined in video/kyocera.c ---------- */

void kyo85_set_lcd_bank(running_machine *machine, UINT16 data);

READ8_HANDLER( kyo85_lcd_status_r );
READ8_HANDLER( kyo85_lcd_data_r );
WRITE8_HANDLER( kyo85_lcd_command_w );
WRITE8_HANDLER( kyo85_lcd_data_w );

MACHINE_DRIVER_EXTERN( kyo85_video );
MACHINE_DRIVER_EXTERN( trsm200_video );

#endif
