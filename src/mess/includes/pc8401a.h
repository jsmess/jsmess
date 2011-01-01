#ifndef __PC8401A__
#define __PC8401A__

#define SCREEN_TAG		"screen"
#define CRT_SCREEN_TAG	"screen2"

#define Z80_TAG			"z80"
#define I8255A_TAG		"i8255a"
#define UPD1990A_TAG	"upd1990a"
#define AY8910_TAG		"ay8910"
#define SED1330_TAG		"sed1330"
#define MC6845_TAG		"mc6845"
#define MSM8251_TAG		"msm8251"

#define PC8401A_CRT_VIDEORAM_SIZE	0x2000
#define PC8401A_LCD_VIDEORAM_SIZE	0x2000
#define PC8401A_LCD_VIDEORAM_MASK	0x1fff
#define PC8500_LCD_VIDEORAM_SIZE	0x4000
#define PC8500_LCD_VIDEORAM_MASK	0x3fff

class pc8401a_state : public driver_device
{
public:
	pc8401a_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* keyboard state */
	int key_strobe;			/* key pressed */

	/* clock state */
	int rtc_data;			/* RTC data output */
	int rtc_tp;				/* RTC timing pulse output */

	/* memory state */
	UINT8 mmr;				/* memory mapping register */
	UINT32 io_addr;			/* I/O ROM address counter */

	/* video state */
	UINT8 *video_ram;		/* LCD video RAM */
	UINT8 *crt_ram;			/* CRT video RAM */

	device_t *upd1990a;
	device_t *sed1330;
	device_t *mc6845;
	device_t *lcd;
	UINT8 key_latch;
};

/* ---------- defined in video/pc8401a.c ---------- */

MACHINE_CONFIG_EXTERN( pc8401a_video );
MACHINE_CONFIG_EXTERN( pc8500_video );

#endif
