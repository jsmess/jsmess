#pragma once

#ifndef __NEXT__
#define __NEXT__

#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "imagedev/flopdrv.h"
#include "machine/mccs1850.h"
#include "machine/am8530h.h"
#include "machine/nextkbd.h"
#include "machine/n82077aa.h"

class next_state : public driver_device
{
public:
	next_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		  maincpu(*this, "maincpu"),
		  rtc(*this, "rtc"),
		  scc(*this, "scc"),
		  keyboard(*this, "keyboard"),
		  fdc(*this, "fdc")
	{ }

	required_device<cpu_device> maincpu;
	required_device<mccs1850_device> rtc;
	required_device<am8530h_device> scc;
	required_device<nextkbd_device> keyboard;
	required_device<n82077aa_device> fdc;

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
	READ32_MEMBER( event_counter_r );
	READ32_MEMBER( dsp_r );

	UINT32 scr2;
	UINT32 irq_status;
	UINT32 irq_mask;
	UINT32 *vram;

	void scc_int();
	void keyboard_int();

protected:
	virtual void machine_start();
	virtual void machine_reset();
};

#endif
