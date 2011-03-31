#pragma once

#ifndef __PC8001__
#define __PC8001__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"
#include "imagedev/cassette.h"
#include "machine/ctronics.h"
#include "machine/i8257.h"
#include "machine/i8214.h"
#include "machine/i8255a.h"
#include "machine/msm8251.h"
#include "machine/ram.h"
#include "machine/upd1990a.h"
#include "sound/speaker.h"
#include "video/upd3301.h"

#define Z80_TAG			"z80"
#define I8251_TAG		"i8251"
#define I8255A_TAG		"i8255"
#define I8257_TAG		"i8257"
#define UPD1990A_TAG	"upd1990a"
#define UPD3301_TAG		"upd3301"
#define CASSETTE_TAG	"cassette"
#define CENTRONICS_TAG	"centronics"
#define SCREEN_TAG		"screen"
#define SPEAKER_TAG		"speaker"

class pc8001_state : public driver_device
{
public:
	pc8001_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, Z80_TAG),
		  m_rtc(*this, UPD1990A_TAG),
		  m_dma(*this, I8257_TAG),
		  m_crtc(*this, UPD3301_TAG),
		  m_cassette(*this, CASSETTE_TAG),
		  m_centronics(*this, CENTRONICS_TAG),
		  m_speaker(*this, SPEAKER_TAG),
		  m_ram(*this, RAM_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<upd1990a_device> m_rtc;
	required_device<device_t> m_dma;
	required_device<device_t> m_crtc;
	required_device<device_t> m_cassette;
	required_device<device_t> m_centronics;
	required_device<device_t> m_speaker;
	required_device<device_t> m_ram;

	virtual void machine_start();

	virtual void video_start();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	DECLARE_WRITE8_MEMBER( port30_w );
	DECLARE_WRITE_LINE_MEMBER( hrq_w );
	
	/* video state */
	UINT8 *m_char_rom;
	int m_width80;
	int m_color;
};

class pc8001mk2_state : public pc8001_state
{
public:
	pc8001mk2_state(running_machine &machine, const driver_device_config_base &config)
		: pc8001_state(machine, config)
	{ }

	DECLARE_WRITE8_MEMBER( port31_w );
};

#endif
