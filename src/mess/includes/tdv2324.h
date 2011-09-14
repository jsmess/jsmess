#pragma once

#ifndef __TDV2324__
#define __TDV2324__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "cpu/m6800/m6800.h"
#include "formats/basicdsk.h"
#include "imagedev/flopdrv.h"
#include "imagedev/harddriv.h"
#include "machine/pit8253.h"
#include "machine/pic8259.h"
#include "machine/s1410.h"
#include "machine/wd17xx.h"
#include "video/tms9927.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define P8085AH_0_TAG		"17f"
#define MK3887N4_TAG		"15d"
#define P8259A_TAG			"17d"
#define P8253_5_0_TAG		"17c"
#define P8253_5_1_TAG		"18c"
#define ER3400_TAG			"12a"

#define P8085AH_1_TAG		"6c"
#define TMS9937NL_TAG		"7e"

#define MC68B02P_TAG		"12b"
#define FD1797PL02_TAG		"fd1797"

#define SCREEN_TAG			"screen"



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

class tdv2324_state : public driver_device
{
public:
	tdv2324_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		  m_maincpu(*this, P8085AH_0_TAG),
		  m_subcpu(*this, P8085AH_1_TAG),
		  m_fdccpu(*this, MC68B02P_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<cpu_device> m_subcpu;
	required_device<cpu_device> m_fdccpu;
	
	virtual void video_start();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	DECLARE_READ8_MEMBER( tdv2324_main_io_e6 );
};


#endif
