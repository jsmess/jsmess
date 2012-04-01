/**********************************************************************

    Commodore VIC-1010 Expansion Module emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __VIC1010__
#define __VIC1010__


#include "emu.h"
#include "machine/cbmipt.h"
#include "machine/vic20exp.h"



//**************************************************************************
//  MACROS/CONSTANTS
//**************************************************************************

#define MAX_SLOTS 6



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> vic1010_device

class vic1010_device :  public device_t,
						public device_vic20_expansion_card_interface
{
public:
    // construction/destruction
    vic1010_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// not really public
	DECLARE_WRITE_LINE_MEMBER( irq_w );
	DECLARE_WRITE_LINE_MEMBER( nmi_w );
	DECLARE_WRITE_LINE_MEMBER( res_w );

protected:
    // device-level overrides
    virtual void device_start();
	virtual void device_config_complete() { m_shortname = "vic1010"; }

	// optional information overrides
	virtual machine_config_constructor device_mconfig_additions() const;

	// device_vic20_expansion_card_interface overrides
	virtual UINT8 vic20_ram1_r(address_space &space, offs_t offset);
	virtual void vic20_ram1_w(address_space &space, offs_t offset, UINT8 data);
	virtual UINT8 vic20_ram2_r(address_space &space, offs_t offset);
	virtual void vic20_ram2_w(address_space &space, offs_t offset, UINT8 data);
	virtual UINT8 vic20_ram3_r(address_space &space, offs_t offset);
	virtual void vic20_ram3_w(address_space &space, offs_t offset, UINT8 data);
	virtual UINT8 vic20_blk1_r(address_space &space, offs_t offset);
	virtual void vic20_blk1_w(address_space &space, offs_t offset, UINT8 data);
	virtual UINT8 vic20_blk2_r(address_space &space, offs_t offset);
	virtual void vic20_blk2_w(address_space &space, offs_t offset, UINT8 data);
	virtual UINT8 vic20_blk3_r(address_space &space, offs_t offset);
	virtual void vic20_blk3_w(address_space &space, offs_t offset, UINT8 data);
	virtual UINT8 vic20_blk5_r(address_space &space, offs_t offset);
	virtual void vic20_blk5_w(address_space &space, offs_t offset, UINT8 data);

private:
	vic20_expansion_slot_device *m_expansion_slot[MAX_SLOTS];
};


// device type definition
extern const device_type VIC1010;



#endif
