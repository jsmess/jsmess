#ifndef __V1050__
#define __V1050__

#define SCREEN_TAG				"screen"

#define Z80_TAG					"u80"
#define I8255_RTC_TAG			"u36"
#define UPB8214_TAG				"u38"
#define I8251_KB_TAG			"u85"
#define MSM58321_TAG			"u26"
#define I8251_SIO_TAG			"u8"
#define I8255_MISC_TAG			"u10"
#define MB8877_TAG				"u13"
#define FDC9216_TAG				"u25"
#define I8255_DISP_TAG			"u79"
#define M6502_TAG				"u76"
#define I8255_M6502_TAG			"u101"
#define HD6845_TAG				"u75"
#define I8049_TAG				"z5"
#define CENTRONICS_TAG			"centronics"

#define V1050_VIDEORAM_SIZE		0x6000
#define V1050_VIDEORAM_MASK		0x5fff

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
	/* memory state */
	UINT8 bank;
	UINT8 int_mask;
	UINT8 int_state;
	int rtc_busy;

	/* floppy state */
	int f_int_enb;

	/* video state */
	UINT8 *video_ram;
	UINT8 *attr_ram;
	UINT8 attr;
	int ppi8255_z80_pc;
	int ppi8255_m6502_pc;

	/* devices */
	const device_config *i8214;
	const device_config *msm58321;
	const device_config *i8255a_crt_z80;
	const device_config *i8255a_crt_m6502;
	const device_config *mb8877;
	const device_config *mc6845;
	const device_config *centronics;
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
