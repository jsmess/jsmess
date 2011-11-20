/*****************************************************************************
 *
 * includes/abc80.h
 *
 ****************************************************************************/

#ifndef __ABC80__
#define __ABC80__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "imagedev/flopdrv.h"
#include "imagedev/printer.h"
#include "imagedev/cassette.h"
#include "machine/abcbus.h"
#include "machine/abc830.h"
#include "machine/abc_fd2.h"
#include "machine/abc_sio.h"
#include "machine/lux10828.h"
#include "machine/ram.h"
#include "machine/z80pio.h"
#include "sound/sn76477.h"

#define ABC80_XTAL		11980800.0

#define ABC80_HTOTAL	384
#define ABC80_HBEND		35
#define ABC80_HBSTART	384
#define ABC80_VTOTAL	312
#define ABC80_VBEND		15
#define ABC80_VBSTART	312

#define ABC80_K5_HSYNC			0x01
#define ABC80_K5_DH				0x02
#define ABC80_K5_LINE_END		0x04
#define ABC80_K5_ROW_START		0x08

#define ABC80_K2_VSYNC			0x01
#define ABC80_K2_DV				0x02
#define ABC80_K2_FRAME_END		0x04
#define ABC80_K2_FRAME_RESET	0x08

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

class abc80_state : public driver_device
{
public:
	abc80_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		  m_maincpu(*this, Z80_TAG),
		  m_pio(*this, Z80PIO_TAG),
		  m_cassette(*this, CASSETTE_TAG),
		  m_bus(*this, ABCBUS_TAG),
		  m_ram(*this, RAM_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_pio;
	required_device<cassette_image_device> m_cassette;
	required_device<abcbus_slot_device> m_bus;
	required_device<device_t> m_ram;

	virtual void machine_start();

	virtual void video_start();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	void update_screen(bitmap_t *bitmap, const rectangle *cliprect);

	DECLARE_READ8_MEMBER( mmu_r );
	DECLARE_WRITE8_MEMBER( mmu_w );

	DECLARE_READ8_MEMBER( pio_pa_r );
	DECLARE_READ8_MEMBER( pio_pb_r );
	DECLARE_WRITE8_MEMBER( pio_pb_w );

	// keyboard state
	int m_key_data;
	int m_key_strobe;
	int m_pio_astb;

	// video state
	UINT8 *m_video_ram;
	UINT8 m_latch;
	int m_blink;

	// memory regions
	const UINT8 *m_mmu_rom;			// memory mapping ROM
	const UINT8 *m_char_rom;		// character generator ROM
	const UINT8 *m_hsync_prom;		// horizontal sync PROM
	const UINT8 *m_vsync_prom;		// horizontal sync PROM
	const UINT8 *m_line_prom;		// line address PROM
	const UINT8 *m_attr_prom;		// character attribute PROM
};

//----------- defined in video/abc80.c -----------

MACHINE_CONFIG_EXTERN( abc80_video );

#endif
