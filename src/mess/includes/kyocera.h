#ifndef __KYOCERA__
#define __KYOCERA__

#define SCREEN_TAG		"screen"

#define I8085_TAG		"m19"
#define I8155_TAG		"m25"
#define UPD1990A_TAG	"m18"
#define IM6402_TAG		"m22"
#define MC14412_TAG		"m31"

//#define I8085_TAG		"m19"
//#define I8155_TAG	"m12"
//#define MC14412_TAG	"m8"
#define RP5C01A_TAG		"m301"
#define TCM5089_TAG		"m11"
#define HD61830_TAG		"m18"
#define MSM8251_TAG		"m20"

#define TANDY200_VIDEORAM_SIZE	0x2000
#define TANDY200_VIDEORAM_MASK	0x1fff

typedef struct _kc85_state kc85_state;
struct _kc85_state
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

	/* peripheral state */
	int iosel;				/* serial interface select */

	const device_config *hd44102[10];
	const device_config *im6042;
	const device_config *upd1990a;
	const device_config *mc14412;
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
	const device_config *im6042;
	const device_config *mc14412;
	const device_config *tcm5089;
	const device_config *centronics;
	const device_config *speaker;
};

/* ---------- defined in video/kyocera.c ---------- */

READ8_HANDLER( kc85_lcd_status_r );
READ8_HANDLER( kc85_lcd_data_r );
WRITE8_HANDLER( kc85_lcd_command_w );
WRITE8_HANDLER( kc85_lcd_data_w );

MACHINE_DRIVER_EXTERN( kc85_video );
MACHINE_DRIVER_EXTERN( tandy200_video );

#endif
