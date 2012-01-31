/**********************************************************************

    Commodore VIC-1210 3K RAM Expansion Cartridge emulation
    Commodore VIC-1211A Super Expander with 3K RAM Cartridge emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __VIC1210__
#define __VIC1210__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "machine/vic20exp.h"



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> vic1210_device

class vic1210_device :  public device_t,
						public device_vic20_expansion_card_interface
{
public:
    // construction/destruction
	vic1210_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock);
    vic1210_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

protected:
    // device-level overrides
    virtual void device_start();

	// device_vic20_expansion_card_interface overrides
	virtual UINT8 vic20_ram1_r(address_space &space, offs_t offset);
	virtual void vic20_ram1_w(address_space &space, offs_t offset, UINT8 data);
	virtual UINT8 vic20_ram2_r(address_space &space, offs_t offset);
	virtual void vic20_ram2_w(address_space &space, offs_t offset, UINT8 data);
	virtual UINT8 vic20_ram3_r(address_space &space, offs_t offset);
	virtual void vic20_ram3_w(address_space &space, offs_t offset, UINT8 data);

	UINT8 *m_ram;
};


// ======================> vic1211a_device

class vic1211a_device :  public vic1210_device
{
public:
    // construction/destruction
    vic1211a_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

protected:
    // device-level overrides
    virtual void device_start();
	virtual void device_config_complete() { m_shortname = "vic1211a"; }

	// optional information overrides
	virtual const rom_entry *device_rom_region() const;

	// device_vic20_expansion_card_interface overrides
	virtual UINT8 vic20_blk5_r(address_space &space, offs_t offset);

private:
	UINT8 *m_rom;
};


// device type definition
extern const device_type VIC1210;
extern const device_type VIC1211A;



#endif
