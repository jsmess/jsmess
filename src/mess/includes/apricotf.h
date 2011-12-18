#pragma once

#ifndef __APRICOTF__
#define __APRICOTF__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/i86/i86.h"
#include "cpu/z80/z80daisy.h"
#include "formats/basicdsk.h"
#include "imagedev/flopdrv.h"
#include "machine/ctronics.h"
#include "machine/wd17xx.h"
#include "machine/z80ctc.h"
#include "machine/z80dart.h"
#include "rendlay.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define SCREEN_TAG		"screen"
#define I8086_TAG		"10d"
#define MC6845_TAG		"b12"
#define Z80CTC_TAG		"13d"
#define Z80SIO2_TAG		"15d"
#define WD2797_TAG		"5f"
#define UPD7507C_TAG	"ic2"
#define CENTRONICS_TAG	"centronics"



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> f1_state

class f1_state : public driver_device
{
public:
	f1_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		  m_maincpu(*this, I8086_TAG),
		  m_ctc(*this, Z80CTC_TAG),
		  m_sio(*this, Z80SIO2_TAG),
		  m_fdc(*this, WD2797_TAG),
		  m_floppy0(*this, FLOPPY_0),
		  m_centronics(*this, CENTRONICS_TAG),
		  m_ctc_int(CLEAR_LINE),
		  m_sio_int(CLEAR_LINE)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<z80ctc_device> m_ctc;
	required_device<z80dart_device> m_sio;
	required_device<device_t> m_fdc;
	required_device<device_t> m_floppy0;
	required_device<device_t> m_centronics;

	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);
	
	DECLARE_READ16_MEMBER( palette_r );
	DECLARE_WRITE16_MEMBER( palette_w );
	DECLARE_WRITE8_MEMBER( system_w );
	DECLARE_WRITE_LINE_MEMBER( sio_int_w );
	DECLARE_WRITE_LINE_MEMBER( ctc_int_w );
	DECLARE_WRITE_LINE_MEMBER( ctc_z1_w );
	DECLARE_WRITE_LINE_MEMBER( ctc_z2_w );
	
	UINT8 read_keyboard();
	READ8_MEMBER( kb_lo_r );
	READ8_MEMBER( kb_hi_r );
	READ8_MEMBER( kb_p6_r );
	WRITE8_MEMBER( kb_p3_w );
	WRITE8_MEMBER( kb_y0_w );
	WRITE8_MEMBER( kb_y4_w );
	WRITE8_MEMBER( kb_y8_w );
	WRITE8_MEMBER( kb_yc_w );

	int m_ctc_int;
	int m_sio_int;
	
	UINT16 *m_p_paletteram;
	UINT16 *m_p_scrollram;
	int m_40_80;
	int m_200_256;
	
	UINT16 m_kb_y;
};

class f1p_state : public f1_state
{
public:
	f1p_state(const machine_config &mconfig, device_type type, const char *tag)
		: f1_state(mconfig, type, tag)
	{ }

	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);
};

#endif
