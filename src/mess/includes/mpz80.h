#pragma once

#ifndef __MPZ80__
#define __MPZ80__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"
#include "formats/basicdsk.h"
#include "imagedev/flopdrv.h"
#include "machine/ins8250.h"
#include "machine/pic8259.h"
#include "machine/ram.h"
#include "machine/terminal.h"
#include "machine/upd1990a.h"

#define Z80_TAG			"17a"
#define AM9512_TAG		"17d"
#define I8259A_TAG		"13b"
#define INS8250_1_TAG	"6d"
#define INS8250_2_TAG	"5d"
#define INS8250_3_TAG	"4d"
#define UPD1990C_TAG	"12a"
#define TERMINAL_TAG	"terminal"

class mpz80_state : public driver_device
{
public:
	mpz80_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		  m_maincpu(*this, Z80_TAG),
		  m_pic(*this, I8259A_TAG),
		  m_ace1(*this, INS8250_1_TAG),
		  m_ace2(*this, INS8250_2_TAG),
		  m_ace3(*this, INS8250_3_TAG),
		  m_ram(*this, RAM_TAG),
		  m_terminal(*this, TERMINAL_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_pic;
	required_device<device_t> m_ace1;
	required_device<device_t> m_ace2;
	required_device<device_t> m_ace3;
	required_device<device_t> m_ram;
	required_device<device_t> m_terminal;

	virtual void machine_start();
	virtual void machine_reset();

	DECLARE_READ8_MEMBER( trap_addr_r );
	DECLARE_READ8_MEMBER( keyboard_r );
	DECLARE_READ8_MEMBER( switch_r );
	DECLARE_READ8_MEMBER( status_r );
	DECLARE_WRITE8_MEMBER( disp_seg_w );
	DECLARE_WRITE8_MEMBER( disp_col_w );
	DECLARE_WRITE8_MEMBER( task_w );
	DECLARE_WRITE8_MEMBER( mask_w );
	DECLARE_READ8_MEMBER( wunderbus_r );
	DECLARE_WRITE8_MEMBER( wunderbus_w );
	DECLARE_WRITE8_MEMBER( terminal_w );
	
	// memory state
	UINT8 *m_map_ram;
};

#endif
