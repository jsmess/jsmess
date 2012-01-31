/**********************************************************************

    Commodore VIC-1010 Expansion Module emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "vic1010.h"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type VIC1010 = &device_creator<vic1010_device>;


//-------------------------------------------------
//  VIC20_EXPANSION_INTERFACE( expansion_intf )
//-------------------------------------------------

WRITE_LINE_MEMBER( vic1010_device::irq_w )
{
	m_slot->irq_w(state);
}

WRITE_LINE_MEMBER( vic1010_device::nmi_w )
{
	m_slot->nmi_w(state);
}

WRITE_LINE_MEMBER( vic1010_device::res_w )
{
	m_slot->res_w(state);
}

static VIC20_EXPANSION_INTERFACE( expansion_intf )
{
	DEVCB_LINE_MEMBER(vic1010_device, irq_w),
	DEVCB_LINE_MEMBER(vic1010_device, nmi_w),
	DEVCB_LINE_MEMBER(vic1010_device, res_w)
};


//-------------------------------------------------
//  MACHINE_DRIVER( vic1010 )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( vic1010 )
	MCFG_VIC20_EXPANSION_SLOT_ADD("slot1", expansion_intf, vic20_expansion_cards, NULL, NULL)
	MCFG_VIC20_EXPANSION_SLOT_ADD("slot2", expansion_intf, vic20_expansion_cards, NULL, NULL)
	MCFG_VIC20_EXPANSION_SLOT_ADD("slot3", expansion_intf, vic20_expansion_cards, NULL, NULL)
	MCFG_VIC20_EXPANSION_SLOT_ADD("slot4", expansion_intf, vic20_expansion_cards, NULL, NULL)
	MCFG_VIC20_EXPANSION_SLOT_ADD("slot5", expansion_intf, vic20_expansion_cards, NULL, NULL)
	MCFG_VIC20_EXPANSION_SLOT_ADD("slot6", expansion_intf, vic20_expansion_cards, NULL, NULL)
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor vic1010_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( vic1010 );
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  vic1010_device - constructor
//-------------------------------------------------

vic1010_device::vic1010_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, VIC1010, "VIC1010", tag, owner, clock),
	  device_vic20_expansion_card_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void vic1010_device::device_start()
{
	// find devices
	m_expansion_slot[0] = dynamic_cast<vic20_expansion_slot_device *>(subdevice("slot1"));
	m_expansion_slot[1] = dynamic_cast<vic20_expansion_slot_device *>(subdevice("slot2"));
	m_expansion_slot[2] = dynamic_cast<vic20_expansion_slot_device *>(subdevice("slot3"));
	m_expansion_slot[3] = dynamic_cast<vic20_expansion_slot_device *>(subdevice("slot4"));
	m_expansion_slot[4] = dynamic_cast<vic20_expansion_slot_device *>(subdevice("slot5"));
	m_expansion_slot[5] = dynamic_cast<vic20_expansion_slot_device *>(subdevice("slot6"));
}


//-------------------------------------------------
//  vic20_ram1_r - RAM 1 read
//-------------------------------------------------

UINT8 vic1010_device::vic20_ram1_r(address_space &space, offs_t offset)
{
	UINT8 data = 0;

	for (int i = 0; i < MAX_SLOTS; i++)
	{
		data |= m_expansion_slot[i]->ram1_r(space, offset);
	}

	return data;
}


//-------------------------------------------------
//  vic20_ram1_w - RAM 1 write
//-------------------------------------------------

void vic1010_device::vic20_ram1_w(address_space &space, offs_t offset, UINT8 data)
{
	for (int i = 0; i < MAX_SLOTS; i++)
	{
		m_expansion_slot[i]->ram1_w(space, offset, data);
	}
}


//-------------------------------------------------
//  vic20_ram2_r - RAM 2 read
//-------------------------------------------------

UINT8 vic1010_device::vic20_ram2_r(address_space &space, offs_t offset)
{
	UINT8 data = 0;

	for (int i = 0; i < MAX_SLOTS; i++)
	{
		data |= m_expansion_slot[i]->ram2_r(space, offset);
	}

	return data;
}


//-------------------------------------------------
//  vic20_ram2_w - RAM 2 write
//-------------------------------------------------

void vic1010_device::vic20_ram2_w(address_space &space, offs_t offset, UINT8 data)
{
	for (int i = 0; i < MAX_SLOTS; i++)
	{
		m_expansion_slot[i]->ram2_w(space, offset, data);
	}
}


//-------------------------------------------------
//  vic20_ram3_r - RAM 3 read
//-------------------------------------------------

UINT8 vic1010_device::vic20_ram3_r(address_space &space, offs_t offset)
{
	UINT8 data = 0;

	for (int i = 0; i < MAX_SLOTS; i++)
	{
		data |= m_expansion_slot[i]->ram3_r(space, offset);
	}

	return data;
}


//-------------------------------------------------
//  vic20_ram3_w - RAM 3 write
//-------------------------------------------------

void vic1010_device::vic20_ram3_w(address_space &space, offs_t offset, UINT8 data)
{
	for (int i = 0; i < MAX_SLOTS; i++)
	{
		m_expansion_slot[i]->ram3_w(space, offset, data);
	}
}


//-------------------------------------------------
//  vic20_blk1_r - block 1 read
//-------------------------------------------------

UINT8 vic1010_device::vic20_blk1_r(address_space &space, offs_t offset)
{
	UINT8 data = 0;

	for (int i = 0; i < MAX_SLOTS; i++)
	{
		data |= m_expansion_slot[i]->blk1_r(space, offset);
	}

	return data;
}


//-------------------------------------------------
//  vic20_blk1_w - block 1 write
//-------------------------------------------------

void vic1010_device::vic20_blk1_w(address_space &space, offs_t offset, UINT8 data)
{
	for (int i = 0; i < MAX_SLOTS; i++)
	{
		m_expansion_slot[i]->blk1_w(space, offset, data);
	}
}


//-------------------------------------------------
//  vic20_blk2_r - block 2 read
//-------------------------------------------------

UINT8 vic1010_device::vic20_blk2_r(address_space &space, offs_t offset)
{
	UINT8 data = 0;

	for (int i = 0; i < MAX_SLOTS; i++)
	{
		data |= m_expansion_slot[i]->blk2_r(space, offset);
	}

	return data;
}


//-------------------------------------------------
//  vic20_blk2_w - block 2 write
//-------------------------------------------------

void vic1010_device::vic20_blk2_w(address_space &space, offs_t offset, UINT8 data)
{
	for (int i = 0; i < MAX_SLOTS; i++)
	{
		m_expansion_slot[i]->blk2_w(space, offset, data);
	}
}


//-------------------------------------------------
//  vic20_blk3_r - block 3 read
//-------------------------------------------------

UINT8 vic1010_device::vic20_blk3_r(address_space &space, offs_t offset)
{
	UINT8 data = 0;

	for (int i = 0; i < MAX_SLOTS; i++)
	{
		data |= m_expansion_slot[i]->blk3_r(space, offset);
	}

	return data;
}


//-------------------------------------------------
//  vic20_blk3_w - block 3 write
//-------------------------------------------------

void vic1010_device::vic20_blk3_w(address_space &space, offs_t offset, UINT8 data)
{
	for (int i = 0; i < MAX_SLOTS; i++)
	{
		m_expansion_slot[i]->blk3_w(space, offset, data);
	}
}


//-------------------------------------------------
//  vic20_blk5_r - block 5 read
//-------------------------------------------------

UINT8 vic1010_device::vic20_blk5_r(address_space &space, offs_t offset)
{
	UINT8 data = 0;

	for (int i = 0; i < MAX_SLOTS; i++)
	{
		data |= m_expansion_slot[i]->blk5_r(space, offset);
	}

	return data;
}


//-------------------------------------------------
//  vic20_blk5_w - block 5 write
//-------------------------------------------------

void vic1010_device::vic20_blk5_w(address_space &space, offs_t offset, UINT8 data)
{
	for (int i = 0; i < MAX_SLOTS; i++)
	{
		m_expansion_slot[i]->blk5_w(space, offset, data);
	}
}
