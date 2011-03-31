#pragma once

#ifndef __E01__
#define __E01__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "imagedev/flopdrv.h"
#include "machine/ram.h"
#include "machine/ctronics.h"
#include "machine/6522via.h"
#include "machine/mc146818.h"
#include "machine/mc6854.h"
#include "machine/wd17xx.h"

#define R65C102_TAG		"r65c102"
#define R6522_TAG		"ic21"
#define WD2793_TAG		"ic20"
#define MC6854_TAG		"mc6854"
#define HD146818_TAG	"hd146818"
#define CENTRONICS_TAG	"centronics"

class e01_state : public driver_device
{
public:
	e01_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, R65C102_TAG),
		  m_fdc(*this, WD2793_TAG),
		  m_rtc(*this, HD146818_TAG),
		  m_ram(*this, RAM_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_fdc;
	required_device<mc146818_device> m_rtc;
	required_device<device_t> m_ram;

	virtual void machine_start();
	virtual void machine_reset();
	
	void update_interrupts();
	void network_irq_enable(int enabled);
	void hdc_irq_enable(int enabled);
	DECLARE_READ8_MEMBER( ram_select_r );
	DECLARE_WRITE8_MEMBER( floppy_w );
	DECLARE_READ8_MEMBER( network_irq_disable_r );
	DECLARE_WRITE8_MEMBER( network_irq_disable_w );
	DECLARE_READ8_MEMBER( network_irq_enable_r );
	DECLARE_WRITE8_MEMBER( network_irq_enable_w );
	DECLARE_WRITE8_MEMBER( hdc_irq_enable_w );
	DECLARE_READ8_MEMBER( rtc_address_r );
	DECLARE_WRITE8_MEMBER( rtc_address_w );
	DECLARE_READ8_MEMBER( rtc_data_r );
	DECLARE_WRITE8_MEMBER( rtc_data_w );
	DECLARE_WRITE_LINE_MEMBER( rtc_irq_w );
	DECLARE_WRITE_LINE_MEMBER( adlc_irq_w );
	DECLARE_WRITE_LINE_MEMBER( via_irq_w );
	DECLARE_WRITE_LINE_MEMBER( fdc_drq_w );
	
	/* interrupt state */
	int m_adlc_ie;
	int m_hdc_ie;

	int m_rtc_irq;
	int m_via_irq;
	int m_hdc_irq;
	int m_fdc_drq;
	int m_adlc_irq;
};

#endif
