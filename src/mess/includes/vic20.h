#pragma once

#ifndef __VIC20__
#define __VIC20__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "includes/cbm.h"
#include "formats/cbm_snqk.h"
#include "cpu/m6502/m6502.h"
#include "imagedev/cartslot.h"
#include "machine/6522via.h"
#include "machine/cbmiec.h"
#include "machine/cbmipt.h"
#include "machine/ieee488.h"
#include "machine/ram.h"
#include "machine/vic20exp.h"
#include "machine/vic1112.h"
#include "sound/dac.h"
#include "sound/mos6560.h"

#define M6502_TAG		"ue10"
#define M6522_0_TAG		"uab3"
#define M6522_1_TAG		"uab1"
#define M6560_TAG		"ub7"
#define M6561_TAG		"ub7"
#define IEC_TAG			"iec"
#define TIMER_C1530_TAG	"c1530"
#define SCREEN_TAG		"screen"

class vic20_state : public driver_device
{
public:
	vic20_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		  m_maincpu(*this, M6502_TAG),
		  m_via0(*this, M6522_0_TAG),
		  m_via1(*this, M6522_1_TAG),
		  m_vic(*this, M6560_TAG),
		  m_iec(*this, CBM_IEC_TAG),
		  m_cassette(*this, CASSETTE_TAG),
		  m_ram(*this, RAM_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<via6522_device> m_via0;
	required_device<via6522_device> m_via1;
	required_device<device_t> m_vic;
	required_device<cbm_iec_device> m_iec;
	required_device<cassette_image_device> m_cassette;
	required_device<ram_device> m_ram;
	
	virtual void machine_start();

	DECLARE_READ8_MEMBER( via0_pa_r );
	DECLARE_WRITE8_MEMBER( via0_pa_w );
	DECLARE_READ8_MEMBER( via0_pb_r );
	DECLARE_WRITE8_MEMBER( via0_pb_w );
	DECLARE_WRITE_LINE_MEMBER( via0_ca2_w );
	DECLARE_READ8_MEMBER( via1_pa_r );
	DECLARE_READ8_MEMBER( via1_pb_r );
	DECLARE_WRITE8_MEMBER( via1_pb_w );
	DECLARE_WRITE_LINE_MEMBER( via1_ca2_w );
	DECLARE_WRITE_LINE_MEMBER( via1_cb2_w );

	// keyboard state
	int m_key_col;

	// timers
	timer_device *m_cassette_timer;
};

#endif
