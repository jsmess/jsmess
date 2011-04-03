#pragma once

#ifndef __COLECO__
#define __COLECO__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"
#include "sound/sn76496.h"
#include "video/tms9928a.h"
#include "machine/coleco.h"
#include "imagedev/cartslot.h"

#define Z80_TAG		"z80"

class coleco_state : public driver_device
{
public:
	coleco_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, Z80_TAG)
	{ }

	required_device<cpu_device> m_maincpu;

	virtual void machine_start();
	virtual void machine_reset();

	DECLARE_READ8_MEMBER( paddle_1_r );
	DECLARE_READ8_MEMBER( paddle_2_r );
	DECLARE_WRITE8_MEMBER( paddle_off_w );
	DECLARE_WRITE8_MEMBER( paddle_on_w );

	int m_joy_mode;
	int m_joy_status[2];
	int m_last_state;
};

#endif
