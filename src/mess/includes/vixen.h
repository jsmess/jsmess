#pragma once

#ifndef __VIXEN__
#define __VIXEN__

#include "machine/ram.h"

#define Z8400A_TAG		"5f"
#define FDC1797_TAG		"5n"
#define P8155H_TAG		"2n"
#define P8155H_IO_TAG	"c7"
#define P8251A_TAG		"c3"
#define DISCRETE_TAG	"discrete"
#define IEEE488_TAG		"j9"
#define SCREEN_TAG		"screen"

class vixen_state : public driver_device
{
public:
	vixen_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, Z8400A_TAG),
		  m_fdc(*this, FDC1797_TAG),
		  m_io_i8155(*this, P8155H_IO_TAG),
		  m_usart(*this, P8251A_TAG),
		  m_discrete(*this, DISCRETE_TAG),
		  m_ieee488(*this, IEEE488_TAG),
		  m_ram(*this, RAM_TAG),
		  m_floppy0(*this, FLOPPY_0),
		  m_floppy1(*this, FLOPPY_1),
		  m_fdint(0),
		  m_vsync(0),
		  m_srq(1),
		  m_atn(1),
		  m_rxrdy(0),
		  m_txrdy(0)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_fdc;
	required_device<device_t> m_io_i8155;
	required_device<device_t> m_usart;
	required_device<device_t> m_discrete;
	required_device<device_t> m_ieee488;
	required_device<device_t> m_ram;
	required_device<device_t> m_floppy0;
	required_device<device_t> m_floppy1;

	virtual void machine_start();
	virtual void machine_reset();

	virtual void video_start();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	void update_interrupt();

	DECLARE_WRITE8_MEMBER( ctl_w );
	DECLARE_READ8_MEMBER( status_r );
	DECLARE_WRITE8_MEMBER( cmd_w );
	DECLARE_READ8_MEMBER( port3_r );
	DECLARE_READ8_MEMBER( i8155_pa_r );
	DECLARE_WRITE8_MEMBER( i8155_pb_w );
	DECLARE_WRITE8_MEMBER( i8155_pc_w );
	DECLARE_WRITE8_MEMBER( io_i8155_pa_w );
	DECLARE_WRITE8_MEMBER( io_i8155_pb_w );
	DECLARE_WRITE8_MEMBER( io_i8155_pc_w );
	DECLARE_WRITE_LINE_MEMBER( io_i8155_to_w );
	DECLARE_WRITE_LINE_MEMBER( srq_w );
	DECLARE_WRITE_LINE_MEMBER( atn_w );
	DECLARE_WRITE_LINE_MEMBER( rxrdy_w );
	DECLARE_WRITE_LINE_MEMBER( txrdy_w );
	DECLARE_WRITE_LINE_MEMBER( fdint_w );

	// memory state
	int m_reset;

	// keyboard state
	UINT8 m_col;

	// interrupt state
	int m_cmd_d0;
	int m_cmd_d1;

	int m_fdint;
	int m_vsync;

	int m_srq;
	int m_atn;
	int m_enb_srq_int;
	int m_enb_atn_int;

	int m_rxrdy;
	int m_txrdy;
	int m_int_clk;
	int m_enb_xmt_int;
	int m_enb_rcv_int;
	int m_enb_ring_int;

	// video state
	int m_alt;
	int m_256;
	UINT8 *m_video_ram;
	const UINT8 *m_sync_rom;
	const UINT8 *m_char_rom;
};

#endif
