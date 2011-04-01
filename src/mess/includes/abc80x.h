#pragma once

#ifndef __ABC800__
#define __ABC800__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "cpu/mcs48/mcs48.h"
#include "imagedev/cassette.h"
#include "imagedev/printer.h"
#include "machine/abc77.h"
#include "machine/abc99.h"
#include "machine/abc830.h"
#include "machine/abcbus.h"
#include "machine/e0516.h"
#include "machine/lux21046.h"
#include "machine/z80ctc.h"
#include "machine/z80dart.h"
#include "machine/ram.h"
#include "sound/discrete.h"
#include "video/mc6845.h"
#include "video/saa5050.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define ABC800_X01	XTAL_12MHz
#define ABC806_X02	XTAL_32_768kHz

#define ABC802_AT0	0x01
#define ABC802_AT1	0x02
#define ABC802_ATD	0x40
#define ABC802_ATE	0x80
#define ABC802_INV	0x80

#define ABC800C_CHAR_RAM_SIZE	0x400
#define ABC800M_CHAR_RAM_SIZE	0x800
#define ABC800_VIDEO_RAM_SIZE	0x4000
#define ABC802_CHAR_RAM_SIZE	0x800
#define ABC806_CHAR_RAM_SIZE	0x800
#define ABC806_ATTR_RAM_SIZE	0x800
#define ABC806_VIDEO_RAM_SIZE	0x20000

#define ABC800_CHAR_WIDTH	6
#define ABC800_CCLK			ABC800_X01/ABC800_CHAR_WIDTH

#define SCREEN_TAG		"screen"
#define Z80_TAG			"z80"
#define I8048_TAG		"i8048"
#define E0516_TAG		"j13"
#define MC6845_TAG		"b12"
#define SAA5052_TAG		"5c"
#define Z80CTC_TAG		"z80ctc"
#define Z80SIO_TAG		"z80sio"
#define Z80DART_TAG		"z80dart"
#define ABCBUS_TAG		"abcbus"
#define CASSETTE_TAG	"cassette"



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> abc800_state

class abc800_state : public driver_device
{
public:
	abc800_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, Z80_TAG),
		  m_ctc(*this, Z80CTC_TAG),
		  m_dart(*this, Z80DART_TAG),
		  m_sio(*this, Z80SIO_TAG),
		  m_discrete(*this, "discrete"),
		  m_ram(*this, RAM_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_ctc;
	required_device<z80dart_device> m_dart;
	required_device<device_t> m_sio;
	optional_device<device_t> m_discrete;
	required_device<device_t> m_ram;

	virtual void machine_start();
	virtual void machine_reset();

	virtual void video_start();

	void bankswitch();

	DECLARE_READ8_MEMBER( pling_r );
	DECLARE_READ_LINE_MEMBER( keyboard_txd_r );
	DECLARE_READ8_MEMBER( keyboard_col_r );
	DECLARE_WRITE8_MEMBER( keyboard_row_w );
	DECLARE_WRITE8_MEMBER( keyboard_ctrl_w );
	DECLARE_READ8_MEMBER( keyboard_t1_r );
	DECLARE_WRITE8_MEMBER( hrs_w );
	DECLARE_WRITE8_MEMBER( hrc_w );
	DECLARE_WRITE_LINE_MEMBER( ctc_z0_w );
	DECLARE_WRITE_LINE_MEMBER( ctc_z1_w );
	DECLARE_WRITE_LINE_MEMBER( ctc_z2_w );

	// cpu state
	int m_fetch_charram;			// opcode fetched from character RAM region (0x7800-0x7fff)

	// video state
	UINT8 *m_char_ram;				// character RAM
	UINT8 *m_video_ram;				// HR video RAM
	const UINT8 *m_char_rom;		// character generator ROM
	const UINT8 *m_fgctl_prom;		// foreground control PROM
	UINT8 m_hrs;					// HR picture start scanline
	UINT8 m_fgctl;					// HR foreground control

	// keyboard state
	int m_kb_row;
	int m_kb_txd;
	int m_kb_clk;
	int m_kb_stb;

	// sound state
	int m_pling;					// pling

	// serial state
	UINT8 m_sb;
	int m_sio_rxcb;
	int m_sio_txcb;
};

class abc800m_state : public abc800_state
{
public:
	abc800m_state(running_machine &machine, const driver_device_config_base &config)
		: abc800_state(machine, config),
		  m_crtc(*this, MC6845_TAG)
	{ }

	required_device<device_t> m_crtc;

	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	void hr_update(bitmap_t *bitmap, const rectangle *cliprect);
};

class abc800c_state : public abc800_state
{
public:
	abc800c_state(running_machine &machine, const driver_device_config_base &config)
		: abc800_state(machine, config),
		  m_trom(*this, SAA5052_TAG)
	{ }

	required_device<device_t> m_trom;

	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	void hr_update(bitmap_t *bitmap, const rectangle *cliprect);
};

// ======================> abc802_state

class abc802_state : public abc800_state
{
public:
	abc802_state(running_machine &machine, const driver_device_config_base &config)
		: abc800_state(machine, config),
		  m_crtc(*this, MC6845_TAG),
		  m_abc77(*this, ABC77_TAG)
	{ }

	required_device<device_t> m_crtc;
	optional_device<device_t> m_abc77;

	virtual void machine_start();
	virtual void machine_reset();

	virtual void video_start();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	void bankswitch();

	DECLARE_READ8_MEMBER( pling_r );
	DECLARE_WRITE_LINE_MEMBER( lrs_w );
	DECLARE_WRITE_LINE_MEMBER( mux80_40_w );
	DECLARE_WRITE_LINE_MEMBER( vs_w );

	// cpu state
	int m_lrs;					// low RAM select

	// video state
	const UINT8 *m_char_rom;	// character generator ROM

	int m_flshclk_ctr;			// flash clock counter
	int m_flshclk;				// flash clock
	int m_80_40_mux;			// 40/80 column mode

	// fake keyboard state
	int m_keylatch;
};


// ======================> abc806_state

class abc806_state : public abc800_state
{
public:
	abc806_state(running_machine &machine, const driver_device_config_base &config)
		: abc800_state(machine, config),
		  m_crtc(*this, MC6845_TAG),
		  m_rtc(*this, E0516_TAG),
		  m_abc77(*this, ABC77_TAG)
	{ }

	required_device<device_t> m_crtc;
	required_device<e0516_device> m_rtc;
	optional_device<device_t> m_abc77;

	virtual void machine_start();
	virtual void machine_reset();

	virtual void video_start();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	void bankswitch();
	void hr_update(bitmap_t *bitmap, const rectangle *cliprect);

	DECLARE_READ8_MEMBER( mai_r );
	DECLARE_WRITE8_MEMBER( mao_w );
	DECLARE_WRITE8_MEMBER( hrs_w );
	DECLARE_WRITE8_MEMBER( hrc_w );
	DECLARE_READ8_MEMBER( charram_r );
	DECLARE_WRITE8_MEMBER( charram_w );
	DECLARE_READ8_MEMBER( ami_r );
	DECLARE_WRITE8_MEMBER( amo_w );
	DECLARE_READ8_MEMBER( cli_r );
	DECLARE_WRITE8_MEMBER( sso_w );
	DECLARE_READ8_MEMBER( sti_r );
	DECLARE_WRITE8_MEMBER( sto_w );
	DECLARE_WRITE_LINE_MEMBER( keydtr_w );
	DECLARE_WRITE_LINE_MEMBER( hs_w );
	DECLARE_WRITE_LINE_MEMBER( vs_w );

	// memory state
	int m_keydtr;				// keyboard DTR
	int m_eme;					// extended memory enable
	int m_fetch_charram;		// opcode fetched from character RAM region (0x7800-0x7fff)
	UINT8 m_map[16];			// memory page register

	// video state
	UINT8 *m_color_ram;			// attribute RAM
	const UINT8 *m_rad_prom;	// line address PROM
	const UINT8 *m_hru2_prom;	// HR palette PROM
	const UINT8 *m_char_rom;	// character generator ROM

	int m_txoff;				// text display enable
	int m_40;					// 40/80 column mode
	int m_flshclk_ctr;			// flash clock counter
	int m_flshclk;				// flash clock
	UINT8 m_attr_data;			// attribute data latch
	UINT8 m_hrs;				// HR memory mapping
	UINT8 m_hrc[16];			// HR palette
	UINT8 m_sync;				// line synchronization delay
	UINT8 m_v50_addr;			// vertical sync PROM address
	int m_hru2_a8;				// HRU II PROM address line 8
	UINT32 m_vsync_shift;		// vertical sync shift register
	int m_vsync;				// vertical sync
	int m_d_vsync;				// delayed vertical sync

	// fake keyboard state
	int m_keylatch;
};



//**************************************************************************
//  MACHINE CONFIGURATION
//**************************************************************************

/*----------- defined in video/abc800.c -----------*/

MACHINE_CONFIG_EXTERN(abc800m_video);
MACHINE_CONFIG_EXTERN(abc800c_video);

/*----------- defined in video/abc802.c -----------*/

MACHINE_CONFIG_EXTERN(abc802_video);

/*----------- defined in video/abc806.c -----------*/

MACHINE_CONFIG_EXTERN(abc806_video);

#endif
