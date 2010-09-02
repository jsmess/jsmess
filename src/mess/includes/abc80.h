/*****************************************************************************
 *
 * includes/abc80.h
 *
 ****************************************************************************/

#ifndef __ABC80__
#define __ABC80__

#define ABC80_XTAL		11980800.0

#define ABC80_HTOTAL	384
#define ABC80_HBEND		0
#define ABC80_HBSTART	312
#define ABC80_VTOTAL	312
#define ABC80_VBEND		0
#define ABC80_VBSTART	287

#define ABC80_HDSTART	36 // unverified
#define ABC80_VDSTART	23 // unverified

#define ABC80_MODE_TEXT	0x07
#define ABC80_MODE_GFX	0x17

#define ABC80_K5_HSYNC			0x01
#define ABC80_K5_DH				0x02
#define ABC80_K5_LINE_END		0x04
#define ABC80_K5_ROW_START		0x08

#define ABC80_K2_VSYNC			0x01
#define ABC80_K2_DV				0x02
#define ABC80_K2_FRAME_END		0x04
#define ABC80_K2_FRAME_START	0x08

#define ABC80_J3_BLANK			0x01
#define ABC80_J3_TEXT			0x02
#define ABC80_J3_GRAPHICS		0x04
#define ABC80_J3_VERSAL			0x08

#define ABC80_E7_VIDEO_RAM		0x01
#define ABC80_E7_INT_RAM		0x02
#define ABC80_E7_31K_EXT_RAM	0x04
#define ABC80_E7_16K_INT_RAM	0x08

#define ABC80_CHAR_CURSOR		0x80

#define SCREEN_TAG		"screen"
#define Z80_TAG			"ab67"
#define Z80PIO_TAG		"cd67"
#define SN76477_TAG		"g8"
#define ABCBUS_TAG		"abcbus"
#define CASSETTE_TAG	"cassette"

class abc80_state : public driver_device
{
public:
	abc80_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* keyboard state */
	int key_data;
	int key_strobe;
	int z80pio_astb;

	/* video state */
	UINT8 *video_ram;
	UINT8 *video_80_ram;
	tilemap_t *tx_tilemap;
	int blink;
	int char_bank;
	int char_row;

	/* memory regions */
	const UINT8 *char_rom;		/* character generator ROM */
	const UINT8 *hsync_prom;	/* horizontal sync PROM */
	const UINT8 *vsync_prom;	/* horizontal sync PROM */
	const UINT8 *line_prom;		/* line address PROM */
	const UINT8 *attr_prom;		/* character attribute PROM */

	/* devices */
	running_device *z80pio;
};

/*----------- defined in video/abc80.c -----------*/

MACHINE_CONFIG_EXTERN( abc80_video );

#endif /* ABC80_H_ */
