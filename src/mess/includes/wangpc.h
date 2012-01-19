#pragma once

#ifndef __WANGPC__
#define __WANGPC__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/i86/i86.h"
#include "machine/8237dma.h"
#include "machine/i8255.h"
#include "machine/pic8259.h"
#include "machine/pit8253.h"
#include "machine/ram.h"
#include "machine/upd765.h"
#include "machine/wangpckb.h"

#define I8086_TAG		"i8086"
#define AM9517A_TAG		"am9517a"
#define I8259A_TAG		"i8259"
#define I8255A_TAG		"i8255a"
#define I8253_TAG		"i8253"
#define IM6402_TAG		"im6402"
#define SCN2661_TAG		"scn2661"
#define UPD765_TAG		"upd765"
#define SCREEN_TAG		"screen"

class wangpc_state : public driver_device
{
public:
	// constructor
	wangpc_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		  m_maincpu(*this, I8086_TAG),
		  m_dmac(*this, AM9517A_TAG),
		  m_pic(*this, I8259A_TAG),
		  m_ppi(*this, I8255A_TAG),
		  m_pit(*this, I8253_TAG),
		  m_fdc(*this, UPD765_TAG)
	{ }
	
	required_device<cpu_device> m_maincpu;
	required_device<i8237_device> m_dmac;
	required_device<device_t> m_pic;
	required_device<i8255_device> m_ppi;
	required_device<device_t> m_pit;
	required_device<device_t> m_fdc;

	virtual void machine_start();
	
	UINT32 screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
	{
		bitmap.fill(RGB_BLACK);
		return 0;
	}
};



#endif
