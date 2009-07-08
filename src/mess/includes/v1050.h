#ifndef __V1050__
#define __V1050__

#define SCREEN_TAG				"screen"

#define Z80_TAG					"u80"
#define RTC_PPI8255_TAG			"u36"
#define UPB8214_TAG				"u38"
#define KB_MSM8251_TAG			"u85"
#define RTC58321_TAG			"u26"
#define SIO_MSM8251_TAG			"u8"
#define FDC_PPI8255_TAG			"u10"
#define MB8877_TAG				"u13"
#define FDC9216_TAG				"u25"
#define CRT_Z80_PPI8255_TAG		"u79"

#define M6502_TAG				"u76"
#define CRT_M6502_PPI8255_TAG	"u101"
#define HD6845_TAG				"u75"

#define I8049_TAG				"z5"

typedef struct _v1050_state v1050_state;
struct _v1050_state
{
	/* keyboard state */
	int keylatch;
	int keyclk;

	/* video state */
	UINT8 *video_ram;
	UINT8 *attr_ram;
	UINT8 attr;

	/* devices */
	const device_config *upb8214;
	const device_config *mc6845;
};

/*----------- defined in video/v1050.c -----------*/

MACHINE_DRIVER_EXTERN( v1050_video );

#endif
