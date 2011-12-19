#pragma once

#ifndef __APRICOTP__
#define __APRICOTP__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/i86/i86.h"
#include "cpu/m6800/m6800.h"
#include "formats/basicdsk.h"
#include "imagedev/flopdrv.h"
#include "machine/8237dma.h"
#include "machine/apricotkb.h"
#include "machine/ctronics.h"
#include "machine/pic8259.h"
#include "machine/wd17xx.h"
#include "machine/z80dart.h"
#include "video/mc6845.h"
#include "rendlay.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define SCREEN_TAG		"screen"
#define I8086_TAG		"ic7"
#define I8284_TAG		"ic30"
#define I8237_TAG		"ic17"
#define I8259A_TAG		"ic51"
#define TMS4500_TAG		"ic42"
#define MC6845_TAG		"ic69"
#define HD63B01V1_TAG	"ic29"
#define AD7574_TAG		"ic34"
#define AD1408_TAG		"ic37"
#define Z80SIO0_TAG		"ic6"
#define WD2797_TAG		"ic5"
#define UPD7507C_TAG	"ic2"
#define CENTRONICS_TAG	"centronics"



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> fp_state

class fp_state : public driver_device
{
public:
	fp_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		  m_maincpu(*this, I8086_TAG),
		  m_soundcpu(*this, HD63B01V1_TAG),
		  m_dmac(*this, I8237_TAG),
		  m_pic(*this, I8259A_TAG),
		  m_sio(*this, Z80SIO0_TAG),
		  m_fdc(*this, WD2797_TAG),
		  m_crtc(*this, MC6845_TAG),
		  m_floppy0(*this, FLOPPY_0),
		  m_centronics(*this, CENTRONICS_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<cpu_device> m_soundcpu;
	required_device<i8237_device> m_dmac;
	required_device<device_t> m_pic;
	required_device<z80dart_device> m_sio;
	required_device<device_t> m_fdc;
	required_device<mc6845_device> m_crtc;
	required_device<device_t> m_floppy0;
	required_device<device_t> m_centronics;

	virtual void machine_start();

	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);
};



#endif
