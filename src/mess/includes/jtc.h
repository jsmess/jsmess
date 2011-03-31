#pragma once

#ifndef __JTC__
#define __JTC__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z8/z8.h"
#include "imagedev/cassette.h"
#include "machine/ctronics.h"
#include "machine/ram.h"
#include "sound/speaker.h"
#include "sound/wave.h"

#define SCREEN_TAG		"screen"
#define UB8830D_TAG		"ub8830d"
#define CASSETTE_TAG	"cassette"
#define SPEAKER_TAG		"speaker"
#define CENTRONICS_TAG	"centronics"

#define JTC_ES40_VIDEORAM_SIZE	0x2000

class jtc_state : public driver_device
{
public:
	jtc_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, UB8830D_TAG),
		  m_cassette(*this, CASSETTE_TAG),
		  m_speaker(*this, SPEAKER_TAG),
		  m_centronics(*this, CENTRONICS_TAG)
	{ }
	
	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_cassette;
	required_device<device_t> m_speaker;
	required_device<device_t> m_centronics;

	virtual void machine_start();

	virtual void video_start();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);
	
	DECLARE_WRITE8_MEMBER( p2_w );
	DECLARE_READ8_MEMBER( p3_r );
	DECLARE_WRITE8_MEMBER( p3_w );
	
	UINT8 *m_video_ram;
};

class jtces88_state : public jtc_state
{
public:
	jtces88_state(running_machine &machine, const driver_device_config_base &config)
		: jtc_state(machine, config)
	{ }
};

class jtces23_state : public jtc_state
{
public:
	jtces23_state(running_machine &machine, const driver_device_config_base &config)
		: jtc_state(machine, config)
	{ }

	virtual void video_start();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);
};

class jtces40_state : public jtc_state
{
public:
	jtces40_state(running_machine &machine, const driver_device_config_base &config)
		: jtc_state(machine, config)
	{ }

	virtual void video_start();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	DECLARE_READ8_MEMBER( videoram_r );
	DECLARE_WRITE8_MEMBER( videoram_w );
	DECLARE_WRITE8_MEMBER( banksel_w );
	
	UINT8 m_video_bank;
	UINT8 *m_color_ram_r;
	UINT8 *m_color_ram_g;
	UINT8 *m_color_ram_b;
};

#endif
