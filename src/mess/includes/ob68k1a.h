#pragma once

#ifndef __OB68K1A__
#define __OB68K1A__

#include "machine/ram.h"

#define MC68000L10_TAG	"u50"
#define MC6821_0_TAG	"u32"
#define MC6821_1_TAG	"u33"
#define MC6840_TAG		"u35"
#define MC6850_0_TAG	"u34"
#define MC6850_1_TAG	"u26"
#define COM8116_TAG		"u56"

class ob68k1a_state : public driver_device
{
public:
	ob68k1a_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, MC68000L10_TAG),
		  m_dbrg(*this, COM8116_TAG),
		  m_acia0(*this, MC6850_0_TAG),
		  m_pia0(*this, MC6821_0_TAG),
		  m_pia1(*this, MC6821_1_TAG),
		  m_terminal(*this, TERMINAL_TAG),
		  m_ram(*this, RAM_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_dbrg;
	required_device<device_t> m_acia0;
	required_device<device_t> m_pia0;
	required_device<device_t> m_pia1;
	required_device<device_t> m_terminal;
	required_device<device_t> m_ram;

	virtual void machine_start();
	virtual void machine_reset();

	DECLARE_WRITE8_MEMBER( com8116_w );
	DECLARE_READ8_MEMBER( pia_r );
	DECLARE_WRITE8_MEMBER( pia_w );
};

#endif
