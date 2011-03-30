#pragma once

#ifndef __PC8401A__
#define __PC8401A__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"
#include "imagedev/cartslot.h"
#include "machine/i8255a.h"
#include "machine/msm8251.h"
#include "machine/ram.h"
#include "machine/upd1990a.h"
#include "video/mc6845.h"
#include "video/sed1330.h"

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
		: driver_device(machine, config),
		  m_maincpu(*this, Z80_TAG),
		  m_rtc(*this, UPD1990A_TAG),
		  m_lcdc(*this, SED1330_TAG),
		  m_crtc(*this, MC6845_TAG),
		  m_screen_lcd(*this, SCREEN_TAG),
		  m_ram(*this, RAM_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<upd1990a_device> m_rtc;
	required_device<device_t> m_lcdc;
	required_device<device_t> m_crtc;
	required_device<device_t> m_screen_lcd;
	required_device<device_t> m_ram;

	virtual void machine_start();

	virtual void video_start();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	DECLARE_WRITE8_MEMBER( mmr_w );
	DECLARE_READ8_MEMBER( mmr_r );
	DECLARE_READ8_MEMBER( rtc_r );
	DECLARE_WRITE8_MEMBER( rtc_cmd_w );
	DECLARE_WRITE8_MEMBER( rtc_ctrl_w );
	DECLARE_READ8_MEMBER( io_rom_data_r );
	DECLARE_WRITE8_MEMBER( io_rom_addr_w );
	DECLARE_READ8_MEMBER( port70_r );
	DECLARE_READ8_MEMBER( port71_r );
	DECLARE_WRITE8_MEMBER( port70_w );
	DECLARE_WRITE8_MEMBER( port71_w );
	DECLARE_READ8_MEMBER( ppi_pc_r );
	DECLARE_WRITE8_MEMBER( ppi_pc_w );
	
	void bankswitch(UINT8 data);

	// keyboard state
	int m_key_strobe;			// key pressed

	// memory state
	UINT8 m_mmr;				// memory mapping register
	UINT32 m_io_addr;			// I/O ROM address counter

	// video state
	UINT8 *m_video_ram;			// LCD video RAM
	UINT8 *m_crt_ram;			// CRT video RAM

	UINT8 m_key_latch;
};

class pc8500_state : public pc8401a_state
{
public:
	pc8500_state(running_machine &machine, const driver_device_config_base &config)
		: pc8401a_state(machine, config)
	{ }

	virtual void video_start();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	DECLARE_READ8_MEMBER( vd_r );
	DECLARE_WRITE8_MEMBER( vd_w );
};

// ---------- defined in video/pc8401a.c ----------

MACHINE_CONFIG_EXTERN( pc8401a_video );
MACHINE_CONFIG_EXTERN( pc8500_video );

#endif
