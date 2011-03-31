#pragma once

#ifndef __TRS80M2__
#define __TRS80M2__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "cpu/mcs48/mcs48.h"
#include "cpu/m68000/m68000.h"
#include "imagedev/flopdrv.h"
#include "machine/ram.h"
#include "machine/ctronics.h"
#include "machine/wd17xx.h"
#include "machine/z80ctc.h"
#include "machine/z80dma.h"
#include "machine/z80pio.h"
#include "machine/z80dart.h"
#include "video/mc6845.h"

#define SCREEN_TAG		"screen"
#define Z80_TAG			"u12"
#define M68000_TAG		"m68000"
#define I8021_TAG		"z4"
#define Z80CTC_TAG		"u19"
#define Z80DMA_TAG		"u20"
#define Z80PIO_TAG		"u22"
#define Z80SIO_TAG		"u18"
#define FD1791_TAG		"u6"
#define MC6845_TAG		"u11"
#define CENTRONICS_TAG	"j2"

class trs80m2_state : public driver_device
{
public:
	trs80m2_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, Z80_TAG),
		  m_ctc(*this, Z80CTC_TAG),
		  m_pio(*this, Z80PIO_TAG),
		  m_crtc(*this, MC6845_TAG),
		  m_fdc(*this, FD1791_TAG),
		  m_centronics(*this, CENTRONICS_TAG),
		  m_floppy(*this, FLOPPY_0),
		  m_ram(*this, RAM_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t>m_ctc;
	required_device<device_t> m_pio;
	required_device<device_t> m_crtc;
	required_device<device_t> m_fdc;
	required_device<device_t> m_centronics;
	required_device<device_t> m_floppy;
	required_device<device_t> m_ram;
	
	virtual void machine_start();
	virtual void machine_reset();

	virtual void video_start();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	DECLARE_WRITE8_MEMBER( drvslt_w );
	DECLARE_WRITE8_MEMBER( rom_enable_w );
	DECLARE_READ8_MEMBER( keyboard_r );
	DECLARE_READ8_MEMBER( rtc_r );
	DECLARE_READ8_MEMBER( nmi_r );
	DECLARE_WRITE8_MEMBER( nmi_w );
	DECLARE_READ8_MEMBER( keyboard_busy_r );
	DECLARE_READ8_MEMBER( keyboard_data_r );
	DECLARE_WRITE8_MEMBER( keyboard_ctrl_w );
	DECLARE_WRITE8_MEMBER( keyboard_latch_w );
	DECLARE_WRITE_LINE_MEMBER( de_w );
	DECLARE_WRITE_LINE_MEMBER( vsync_w );
	DECLARE_READ8_MEMBER( pio_pa_r );
	DECLARE_WRITE8_MEMBER( pio_pa_w );
	DECLARE_WRITE_LINE_MEMBER( strobe_w );
	DECLARE_WRITE_LINE_MEMBER( fdc_intrq_w );
	
	void scan_keyboard();
	void bankswitch();
	
	/* memory state */
	int m_boot_rom;
	int m_bank;
	int m_msel;

	/* keyboard state */
	UINT8 m_key_latch;
	UINT8 m_key_data;
	int m_key_bit;
	int m_kbclk;
	int m_kbdata;
	int m_kbirq;

	/* video state */
	UINT8 *m_video_ram;
	UINT8 *m_char_rom;
	int m_blnkvid;
	int m_80_40_char_en;
	int m_de;
	int m_rtc_int;
	int m_enable_rtc_int;
};

#endif
