#ifndef __KYOCERA__
#define __KYOCERA__

#define SCREEN_TAG		"screen"
#define I8085_TAG		"i8085"
#define PIO8155_TAG		"pio8155"
#define UPD1990A_TAG	"upd1990a"
#define RP5C01A_TAG		"rp5c01a"
#define TCM5089_TAG		"tcm5089"
#define HD61830_TAG		"hd61830b"

#define TRSM200_VIDEORAM_SIZE	0x2000
#define TRSM200_VIDEORAM_MASK	0x1fff

typedef struct _kyocera_state kyocera_state;
struct _kyocera_state
{
	/* memory state */
	UINT8 bank;				/* memory bank selection */

	/* clock state */
	int upd1990a_data;		/* RTC data output */

	/* keyboard state */
	UINT16 keylatch;

	/* video state */
	int lcd_cs2[10];		/* LCD driver chip select */

	/* sound state */
	int buzzer;				/* buzzer select */
	int bell;				/* bell output */

	const device_config *hd44102[10];
	const device_config *upd1990a;
	const device_config *centronics;
	const device_config *speaker;
};

typedef struct _tandy200_state tandy200_state;
struct _tandy200_state
{
	/* memory state */
	UINT8 bank;				/* memory bank selection */

	/* keyboard state */
	UINT16 keylatch;
	int tp;					/* timing pulse */

	/* video state */
	UINT8 *video_ram;		/* video RAM */

	/* sound state */
	int buzzer;				/* buzzer select */
	int bell;				/* bell output */

	const device_config *hd61830;
	const device_config *centronics;
	const device_config *speaker;
};

/* ---------- defined in video/kyocera.c ---------- */

READ8_HANDLER( kyo85_lcd_status_r );
READ8_HANDLER( kyo85_lcd_data_r );
WRITE8_HANDLER( kyo85_lcd_command_w );
WRITE8_HANDLER( kyo85_lcd_data_w );

MACHINE_DRIVER_EXTERN( kyo85_video );
MACHINE_DRIVER_EXTERN( tandy200_video );

#endif
