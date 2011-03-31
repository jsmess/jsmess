#pragma once

#ifndef __BETA__
#define __BETA__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "imagedev/cartslot.h"
#include "machine/6532riot.h"
#include "sound/speaker.h"
#include "machine/ram.h"

#define SCREEN_TAG		"screen"
#define M6502_TAG		"m6502"
#define M6532_TAG		"m6532"
#define EPROM_TAG		"eprom"
#define SPEAKER_TAG		"b237"

class beta_state : public driver_device
{
public:
	beta_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, M6502_TAG),
		  m_speaker(*this, SPEAKER_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_speaker;
	
	virtual void machine_start();

	DECLARE_READ8_MEMBER( riot_pa_r );
	DECLARE_WRITE8_MEMBER( riot_pa_w );
	DECLARE_READ8_MEMBER( riot_pb_r );
	DECLARE_WRITE8_MEMBER( riot_pb_w );

	/* EPROM state */
	int m_eprom_oe;
	int m_eprom_ce;
	UINT16 m_eprom_addr;
	UINT8 m_eprom_data;
	UINT8 m_old_data;

	/* display state */
	UINT8 m_ls145_p;
	UINT8 m_segment;

	emu_timer *m_led_refresh_timer;
};

#endif
