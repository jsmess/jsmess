/***************************************************************************

        ISA bus device


    8 bit ISA bus connector:

    A1    O  I/O CH CHK    B1       GND
    A2   IO  D7            B2   I   RESET
    A3   IO  D6            B3       +5V
    A4   IO  D5            B4    O  IRQ2
    A5   IO  D4            B5       -5V
    A6   IO  D3            B6    O  DRQ2
    A7   IO  D2            B7       -12V
    A8   IO  D1            B8    O  /NOWS
    A9   IO  D0            B9       +12V
    A10   O  I/O CH RDY    B10      GND
    A11  I   AEN           B11  I   /SMEMW
    A12  IO  A19           B12  I   /SMEMR
    A13  IO  A18           B13  I   /IOW
    A14  IO  A17           B14  I   /IOR
    A15  IO  A16           B15  I   /DACK3
    A16  IO  A15           B16   O  DRQ3
    A17  IO  A14           B17  I   /DACK1
    A18  IO  A13           B18   O  DRQ1
    A19  IO  A12           B19  IO  /REFRESH
    A20  IO  A11           B20  I   CLOCK
    A21  IO  A10           B21   O  IRQ7
    A22  IO  A9            B22   O  IRQ6
    A23  IO  A8            B23   O  IRQ5
    A24  IO  A7            B24   O  IRQ4
    A25  IO  A6            B25   O  IRQ3
    A26  IO  A5            B26  I   /DACK2
    A27  IO  A4            B27  I   T/C
    A28  IO  A3            B28  I   ALE
    A29  IO  A2            B29      +5V
    A30  IO  A1            B30  I   OSC
    A31  IO  A0            B31      GND

***************************************************************************/

#pragma once

#ifndef __ISA_H__
#define __ISA_H__

#include "emu.h"


//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_ISA8_BUS_ADD(_tag, _cputag, _config) \
    MCFG_DEVICE_ADD(_tag, ISA8, 0) \
    MCFG_DEVICE_CONFIG(_config) \
    isa8_device::static_set_cputag(*device, _cputag); \

#define MCFG_ISA8_BUS_DEVICE(_isatag, _num, _tag, _dev_type) \
    MCFG_DEVICE_ADD(_tag, _dev_type, 0) \
	device_isa8_card_interface::static_set_isa8_tag(*device, _isatag); \
	device_isa8_card_interface::static_set_isa8_num(*device, _num); \


//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************


// ======================> isabus_interface

struct isabus_interface
{
    devcb_write_line	m_out_irq2_cb;
    devcb_write_line	m_out_irq3_cb;
    devcb_write_line	m_out_irq4_cb;
    devcb_write_line	m_out_irq5_cb;
    devcb_write_line	m_out_irq6_cb;
    devcb_write_line	m_out_irq7_cb;
    devcb_write_line	m_out_drq1_cb;
    devcb_write_line	m_out_drq2_cb;
    devcb_write_line	m_out_drq3_cb;
};


// ======================> isa8_device
class device_isa8_card_interface;
class isa8_device : public device_t,
                    public isabus_interface
{
public:
	// construction/destruction
	isa8_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	// inline configuration
	static void static_set_cputag(device_t &device, const char *tag);
	
	void add_isa_card(device_isa8_card_interface *card,int pos);
	void install_device(device_t *dev, offs_t start, offs_t end, offs_t mask, offs_t mirror, read8_device_func rhandler, const char* rhandler_name, write8_device_func whandler, const char *whandler_name);
	void install_bank(offs_t start, offs_t end, offs_t mask, offs_t mirror, const char *tag, UINT8 *data);
	void install_rom(device_t *dev, offs_t start, offs_t end, offs_t mask, offs_t mirror, const char *tag, const char *region);

	DECLARE_WRITE_LINE_MEMBER( irq2_w );
	DECLARE_WRITE_LINE_MEMBER( irq3_w );
	DECLARE_WRITE_LINE_MEMBER( irq4_w );
	DECLARE_WRITE_LINE_MEMBER( irq5_w );
	DECLARE_WRITE_LINE_MEMBER( irq6_w );
	DECLARE_WRITE_LINE_MEMBER( irq7_w );

	DECLARE_WRITE_LINE_MEMBER( drq1_w );
	DECLARE_WRITE_LINE_MEMBER( drq2_w );
	DECLARE_WRITE_LINE_MEMBER( drq3_w );

	UINT8 dack_r(int line);
	void dack_w(int line,UINT8 data);
	void eop_w(int state);

protected:
	// device-level overrides
	virtual void device_start();
	virtual void device_reset();
	virtual void device_config_complete();

private:
	// internal state

	device_t   *m_maincpu;

	devcb_resolved_write_line	m_out_irq2_func;
	devcb_resolved_write_line	m_out_irq3_func;
	devcb_resolved_write_line	m_out_irq4_func;
	devcb_resolved_write_line	m_out_irq5_func;
	devcb_resolved_write_line	m_out_irq6_func;
	devcb_resolved_write_line	m_out_irq7_func;

	devcb_resolved_write_line	m_out_drq1_func;
	devcb_resolved_write_line	m_out_drq2_func;
	devcb_resolved_write_line	m_out_drq3_func;

	device_isa8_card_interface *m_isa_device[8];
	const char *m_cputag;		
};


// device type definition
extern const device_type ISA8;

// ======================> device_isa8_card_interface

// class representing interface-specific live isa8 card
class device_isa8_card_interface : public device_interface
{
	friend class isa8_device;
public:
	// construction/destruction
	device_isa8_card_interface(const machine_config &mconfig, device_t &device);
	virtual ~device_isa8_card_interface();

    // inline configuration
    static void static_set_isa8_tag(device_t &device, const char *tag);
    static void static_set_isa8_num(device_t &device, int num);

	// configuration access
	virtual UINT8 dack_r(int line);
	virtual void dack_w(int line,UINT8 data);
	virtual void eop_w(int state);
	virtual bool have_dack(int line);
protected:
	// configuration
	const char *m_isa_tag;
	int m_isa_num;
};

#endif  /* __ISA_H__ */
