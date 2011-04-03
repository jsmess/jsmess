#pragma once

#ifndef __COMX35__
#define __COMX35__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/cosmac/cosmac.h"
#include "sound/cdp1869.h"
#include "sound/wave.h"
#include "imagedev/flopdrv.h"
#include "formats/comx35_dsk.h"
#include "formats/comx35_comx.h"
#include "imagedev/cassette.h"
#include "imagedev/printer.h"
#include "imagedev/snapquik.h"
#include "machine/cdp1871.h"
#include "machine/comxpl80.h"
#include "machine/wd17xx.h"
#include "machine/ram.h"
#include "machine/rescap.h"
#include "video/mc6845.h"

#define SCREEN_TAG			"screen"
#define MC6845_SCREEN_TAG	"screen80"

#define CDP1870_TAG			"u1"
#define CDP1869_TAG			"u2"
#define CDP1802_TAG			"u3"
#define CDP1871_TAG			"u4"
#define MC6845_TAG			"mc6845"
#define WD1770_TAG			"wd1770"

#define CASSETTE_TAG		"cassette"

#define COMX35_CHARRAM_SIZE 0x800
#define COMX35_VIDEORAM_SIZE 0x800

#define COMX35_CHARRAM_MASK 0x7ff
#define COMX35_VIDEORAM_MASK 0x7ff

enum
{
	BANK_NONE = 0,
	BANK_FLOPPY,
	BANK_PRINTER_PARALLEL,
	BANK_PRINTER_PARALLEL_FM,
	BANK_PRINTER_SERIAL,
	BANK_PRINTER_THERMAL,
	BANK_JOYCARD,
	BANK_80_COLUMNS,
	BANK_RAMCARD
};

class comx35_state : public driver_device
{
public:
	comx35_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, CDP1802_TAG),
		  m_vis(*this, CDP1869_TAG),
		  m_crtc(*this, MC6845_TAG),
		  m_fdc(*this, WD1770_TAG),
		  m_kbe(*this, CDP1871_TAG),
		  m_cassette(*this, CASSETTE_TAG),
		  m_ram(*this, RAM_TAG)
	{ }

	required_device<cosmac_device> m_maincpu;
	required_device<cdp1869_device> m_vis;
	required_device<device_t> m_crtc;
	required_device<device_t> m_fdc;
	required_device<cdp1871_device> m_kbe;
	required_device<device_t> m_cassette;
	required_device<device_t> m_ram;

	virtual void machine_start();
	virtual void machine_reset();

	virtual void video_start();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	DECLARE_WRITE8_MEMBER( cdp1869_w );
	DECLARE_READ8_MEMBER( io_r );
	DECLARE_READ8_MEMBER( io2_r );
	DECLARE_WRITE8_MEMBER( bank_select_w );
	DECLARE_WRITE8_MEMBER( io_w );
	DECLARE_READ_LINE_MEMBER( clear_r );
	DECLARE_READ_LINE_MEMBER( ef2_r );
	DECLARE_READ_LINE_MEMBER( ef4_r );
	DECLARE_WRITE_LINE_MEMBER( q_w );
	DECLARE_READ_LINE_MEMBER( shift_r );
	DECLARE_READ_LINE_MEMBER( control_r );
	DECLARE_WRITE_LINE_MEMBER( fdc_intrq_w );
	DECLARE_WRITE_LINE_MEMBER( fdc_drq_w );

	UINT8 read_expansion();
	bool is_expansion_box_installed();
	bool is_dos_card_active();
	UINT8 fdc_r();
	void fdc_w(UINT8 data);
	UINT8 printer_r();
	void printer_w(UINT8 data);
	void get_active_bank(UINT8 data);
	void set_active_bank();

	// processor state
	int m_reset;				// CPU mode
	int m_cdp1802_q;			// Q flag
	int m_cdp1802_ef4;			// EF4 flag
	int m_iden;					// interrupt/DMA enable
	int m_slot;					// selected slot
	int m_bank;					// selected device bank
	int m_rambank;				// selected RAM bank
	int m_dma;					// memory refresh DMA

	// video state
	UINT8 *m_charram;			// character memory
	UINT8 *m_videoram;			// 80 column video memory

	// floppy state
	int m_fdc_addr;				// FDC address
	int m_fdc_irq;				// interrupt request
	int m_fdc_drq_enable;		// EF4 enabled

	// timers
	emu_timer *m_reset_timer;	// power on reset timer
};

// ---------- defined in machine/comx35.c ----------

extern const wd17xx_interface comx35_wd17xx_interface;

INPUT_CHANGED( comx35_reset );

// ---------- defined in video/comx35.c ----------

MACHINE_CONFIG_EXTERN( comx35_pal_video );
MACHINE_CONFIG_EXTERN( comx35_ntsc_video );

#endif
