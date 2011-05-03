#pragma once

#ifndef __NEXT__
#define __NEXT__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "imagedev/flopdrv.h"
#include "machine/mccs1850.h"

#define MCCS1850_TAG	"u3"

class next_state : public driver_device
{
public:
	next_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		  m_maincpu(*this, "maincpu"),
		  m_rtc(*this, MCCS1850_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<mccs1850_device> m_rtc;

	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	READ8_MEMBER( io_r );
	WRITE8_MEMBER( io_w );
	READ32_MEMBER( rom_map_r );
	READ32_MEMBER( scr2_r );
	WRITE32_MEMBER( scr2_w );
	READ32_MEMBER( scr1_r );
	READ32_MEMBER( irq_status_r );
	WRITE32_MEMBER( irq_status_w );
	READ32_MEMBER( irq_mask_r );
	WRITE32_MEMBER( irq_mask_w );
	READ32_MEMBER( modisk_r );
	READ32_MEMBER( network_r );
	READ32_MEMBER( unk_r );

	UINT32 m_scr2;
	UINT32 m_irq_status;
	UINT32 m_irq_mask;
	UINT32 *m_vram;
};

#endif
