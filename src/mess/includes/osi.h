#pragma once

#ifndef __OSI__
#define __OSI__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "formats/basicdsk.h"
#include "imagedev/cassette.h"
#include "imagedev/flopdrv.h"
#include "machine/6850acia.h"
#include "machine/6821pia.h"
#include "machine/ram.h"
#include "sound/discrete.h"
#include "sound/beep.h"

#define SCREEN_TAG		"screen"
#define M6502_TAG		"m6502"
#define DISCRETE_TAG	"discrete"
#define BEEP_TAG		"beep"
#define CASSETTE_TAG	"cassette"

#define X1			3932160
#define UK101_X1	XTAL_8MHz

#define OSI600_VIDEORAM_SIZE	0x400
#define OSI630_COLORRAM_SIZE	0x400

class sb2m600_state : public driver_device
{
public:
	sb2m600_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, M6502_TAG),
		  m_cassette(*this, CASSETTE_TAG),
		  m_discrete(*this, DISCRETE_TAG),
		  m_ram(*this, RAM_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_cassette;
	optional_device<device_t> m_discrete;
	required_device<device_t> m_ram;

	virtual void machine_start();

	virtual void video_start();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	DECLARE_READ8_MEMBER( keyboard_r );
	DECLARE_WRITE8_MEMBER( keyboard_w );
	DECLARE_WRITE8_MEMBER( ctrl_w );
	DECLARE_READ_LINE_MEMBER( cassette_rx );
	DECLARE_WRITE_LINE_MEMBER( cassette_tx );

	/* keyboard state */
	UINT8 m_keylatch;

	/* video state */
	int m_32;
	int m_coloren;
	UINT8 *m_video_ram;
	UINT8 *m_color_ram;

	/* floppy state */
	int m_fdc_index;
};

class c1p_state : public sb2m600_state
{
public:
	c1p_state(running_machine &machine, const driver_device_config_base &config)
		: sb2m600_state(machine, config),
		  m_beep(*this, BEEP_TAG)
	{ }

	required_device<device_t> m_beep;

	virtual void machine_start();

	DECLARE_WRITE8_MEMBER( osi630_ctrl_w );
	DECLARE_WRITE8_MEMBER( osi630_sound_w );
};

class c1pmf_state : public c1p_state
{
public:
	c1pmf_state(running_machine &machine, const driver_device_config_base &config)
		: c1p_state(machine, config),
		  m_floppy(*this, FLOPPY_0)
	{ }

	required_device<device_t> m_floppy;

	virtual void machine_start();

	DECLARE_READ8_MEMBER( osi470_pia_pa_r );
	DECLARE_WRITE8_MEMBER( osi470_pia_pa_w );
	DECLARE_WRITE8_MEMBER( osi470_pia_pb_w );
	DECLARE_WRITE_LINE_MEMBER( osi470_pia_cb2_w );
};

class uk101_state : public sb2m600_state
{
public:
	uk101_state(running_machine &machine, const driver_device_config_base &config)
		: sb2m600_state(machine, config)
	{ }

	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	DECLARE_WRITE8_MEMBER( keyboard_w );
};

/* ---------- defined in video/osi.c ---------- */

MACHINE_CONFIG_EXTERN( osi600_video );
MACHINE_CONFIG_EXTERN( uk101_video );
MACHINE_CONFIG_EXTERN( osi630_video );

#endif
