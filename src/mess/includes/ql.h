#pragma once

#ifndef __QL__
#define __QL__

#include "machine/ram.h"
#include "machine/wd17xx.h"

#define SCREEN_TAG	"screen"

#define M68008_TAG	"ic18"
#define I8749_TAG	"ic24"
#define I8051_TAG	"i8051"
#define ZX8301_TAG	"ic22"
#define ZX8302_TAG	"ic23"
#define SPEAKER_TAG	"speaker"
#define WD1772_TAG	"wd1772"

#define ROMBANK_TAG	"rombank"
#define RAMBANK_TAG	"rambank"

#define X1 XTAL_15MHz
#define X2 XTAL_32_768kHz
#define X3 XTAL_4_436MHz
#define X4 XTAL_11MHz

#define DRIVE_1_MASK	0x01
#define DRIVE_0_MASK	0x02
#define MOTOR_MASK		0x04
#define SIDE_SHIFT		3
#define SIDE_MASK		(1 << SIDE_SHIFT)

#define CART_ROM_BASE	0x0c000
#define TRUMP_ROM_BASE	0x10000

class ql_state : public driver_device
{
public:
	ql_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, M68008_TAG),
		  m_ipc(*this, I8749_TAG),
		  m_zx8301(*this, ZX8301_TAG),
		  m_zx8302(*this, ZX8302_TAG),
		  m_speaker(*this, SPEAKER_TAG),
		  m_mdv1(*this, MDV_1),
		  m_mdv2(*this, MDV_2),
		  m_ram(*this, RAM_TAG),
		  m_fdc(*this, WD1772_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<cpu_device> m_ipc;
	required_device<zx8301_device> m_zx8301;
	required_device<zx8302_device> m_zx8302;
	required_device<device_t> m_speaker;
	required_device<device_t> m_mdv1;
	required_device<device_t> m_mdv2;
	required_device<device_t> m_ram;
	required_device<device_t> m_fdc;

	virtual void machine_start();
	virtual void machine_reset();
	
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	DECLARE_WRITE8_MEMBER( ipc_w );
	DECLARE_WRITE8_MEMBER( ipc_port1_w );
	DECLARE_WRITE8_MEMBER( ipc_port2_w );
	DECLARE_READ8_MEMBER( ipc_port2_r );
	DECLARE_READ8_MEMBER( ipc_t1_r );
	DECLARE_READ8_MEMBER( ipc_bus_r );
	DECLARE_READ8_MEMBER( ql_ram_r );
	DECLARE_WRITE8_MEMBER( ql_ram_w );
	DECLARE_WRITE_LINE_MEMBER( ql_baudx4_w );
	DECLARE_WRITE_LINE_MEMBER( ql_comdata_w );
	DECLARE_WRITE_LINE_MEMBER( zx8302_mdselck_w );
	DECLARE_WRITE_LINE_MEMBER( zx8302_mdrdw_w );
	DECLARE_WRITE_LINE_MEMBER( zx8302_erase_w );
	DECLARE_WRITE_LINE_MEMBER( zx8302_raw1_w );
	DECLARE_READ_LINE_MEMBER( zx8302_raw1_r );
	DECLARE_WRITE_LINE_MEMBER( zx8302_raw2_w );
	DECLARE_READ_LINE_MEMBER( zx8302_raw2_r );
	
	/* IPC state */
	UINT8 m_keylatch;
	int m_ipl;
	int m_comdata;
	int m_baudx4;

	// Trump card
	DECLARE_READ8_MEMBER( trump_card_r );
	DECLARE_WRITE8_MEMBER( trump_card_w );
	DECLARE_READ8_MEMBER( trump_card_rom_r );
	DECLARE_READ8_MEMBER( cart_rom_r );
	
	void trump_card_set_control(UINT8 data);
};

#endif
