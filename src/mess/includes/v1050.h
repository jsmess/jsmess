#ifndef __V1050__
#define __V1050__

#define SCREEN_TAG				"screen"

#define Z80_TAG					"u80"
#define UPB8214_TAG				"u38"
#define I8255A_DISP_TAG			"u79"
#define I8255A_MISC_TAG			"u10"
#define I8255A_RTC_TAG			"u36"
#define I8251A_KB_TAG			"u85"
#define I8251A_SIO_TAG			"u8"
#define MSM58321RS_TAG			"u26"
#define MB8877_TAG				"u13"
#define FDC9216_TAG				"u25"
#define M6502_TAG				"u76"
#define I8255A_M6502_TAG		"u101"
#define H46505_TAG				"u75"
#define I8049_TAG				"z5"
#define CENTRONICS_TAG			"centronics"
#define TIMER_KB_TAG			"timer_kb"
#define TIMER_SIO_TAG			"timer_sio"

#define V1050_VIDEORAM_SIZE		0x8000
#define V1050_VIDEORAM_MASK		0x7fff

#define INT_RS_232			0x01
#define INT_WINCHESTER		0x02
#define INT_KEYBOARD		0x04
#define INT_FLOPPY			0x08
#define INT_VSYNC			0x10
#define INT_DISPLAY			0x20
#define INT_EXPANSION_B		0x40
#define INT_EXPANSION_A		0x80

typedef struct _v1050_state v1050_state;
struct _v1050_state
{
	/* interrupt state */
	UINT8 int_mask;				/* interrupt mask */
	UINT8 int_state;			/* interrupt status */
	int f_int_enb;				/* floppy interrupt enable */

	/* keyboard state */
	UINT8 keylatch;				/* keyboard row select */
	UINT8 keydata;
	int keyavail;

	/* serial state */
	int rxrdy;					/* receiver ready */
	int txrdy;					/* transmitter ready */
	int baud_sel;				/* baud select */

	/* memory state */
	UINT8 bank;					/* bank register */

	/* video state */
	UINT8 *video_ram;			/* video RAM */
	UINT8 *attr_ram;			/* attribute RAM */
	UINT8 attr;					/* attribute latch */

	/* devices */
	const device_config *i8214;
	const device_config *msm58321;
	const device_config *i8255a_crt_z80;
	const device_config *i8255a_crt_m6502;
	const device_config *i8251_kb;
	const device_config *i8251_sio;
	const device_config *mb8877;
	const device_config *mc6845;
	const device_config *centronics;
	const device_config *timer_sio;
};

/*----------- defined in drivers/v1050.c -----------*/

void v1050_set_int(running_machine *machine, UINT8 mask, int state);

/*----------- defined in video/v1050.c -----------*/

READ8_HANDLER( v1050_attr_r );
WRITE8_HANDLER( v1050_attr_w );
READ8_HANDLER( v1050_videoram_r );
WRITE8_HANDLER( v1050_videoram_w );

MACHINE_DRIVER_EXTERN( v1050_video );

#endif
