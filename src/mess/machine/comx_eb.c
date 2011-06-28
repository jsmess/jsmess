/*

COMX-35E Expansion Box
(c) 1984 Comx World Operations

PCB Layout
----------

F-001-EB-REV0

     |--------------------------------------|
     |  40174       4073    4049    4075    |
     |                                      |
     |  ROM         40175   4073    4075    |
     |                                      |
|----|      -       -       -       -       |
|           |       |       |       | 7805  |
|           |       |       |       |       |
|           |       |       |       |       |
|C          C       C       C       C       |
|N          N       N       N       N       |
|5          1       2       3       4       |
|           |       |       |       |       |
|           |       |       |       |       |
|           |       |       |       |       |
|           -       -       - LD1   -       |
|-------------------------------------------|

Notes:
    All IC's shown.

    ROM     - NEC D2732D-4 4Kx8 EPROM, unlabeled
    CN1     - COMX-35 bus connector slot 1
    CN2     - COMX-35 bus connector slot 2
    CN3     - COMX-35 bus connector slot 3
    CN4     - COMX-35 bus connector slot 4
    CN5     - COMX-35 bus PCB edge connector
    LD1     - LED

*/

#include "comx_eb.h"



//**************************************************************************
//  MACROS/CONSTANTS
//**************************************************************************

#define SLOT1_TAG			"slot1"
#define SLOT2_TAG			"slot2"
#define SLOT3_TAG			"slot3"
#define SLOT4_TAG			"slot4"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type COMX_EB = &device_creator<comx_eb_device>;


//-------------------------------------------------
//  ROM( comx_eb )
//-------------------------------------------------

ROM_START( comx_eb )
	ROM_REGION( 0x1000, "e000", 0 )
	ROM_LOAD( "expansion.e5",			0x0000, 0x1000, CRC(52cb44e2) SHA1(3f9a3d9940b36d4fee5eca9f1359c99d7ed545b9) )

	ROM_REGION( 0x1000, "fm31", 0 )
	ROM_LOAD( "f&m.expansion.3.1.e5",	0x0000, 0x1000, CRC(818ca2ef) SHA1(ea000097622e7fd472d53e7899e3c83773433045) )

	ROM_REGION( 0x1000, "fm32", 0 )
	ROM_LOAD( "f&m.expansion.3.2.e5",	0x0000, 0x1000, CRC(0f0fc960) SHA1(eb6b6e7bc9e761d13554482025d8cb5e260c0619) )
ROM_END


//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *comx_eb_device::device_rom_region() const
{
	return ROM_NAME( comx_eb );
}


//-------------------------------------------------
//  COMX_EXPANSION_INTERFACE( expansion_intf )
//-------------------------------------------------

static SLOT_INTERFACE_START( comx_expansion_cards )
	SLOT_INTERFACE("fd", COMX_FD)
	SLOT_INTERFACE("clm", COMX_CLM)
	SLOT_INTERFACE("ram", COMX_RAM)
	SLOT_INTERFACE("joy", COMX_JOY)
	SLOT_INTERFACE("prn", COMX_PRN)
	SLOT_INTERFACE("thm", COMX_THM)
SLOT_INTERFACE_END

static COMX_EXPANSION_INTERFACE( expansion_intf )
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};


//-------------------------------------------------
//  MACHINE_CONFIG_FRAGMENT( comx_eb )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( comx_eb )
	MCFG_COMX_EXPANSION_SLOT_ADD(SLOT1_TAG, comx_expansion_cards, NULL)
	MCFG_COMX_EXPANSION_SLOT_ADD(SLOT2_TAG, comx_expansion_cards, NULL)
	MCFG_COMX_EXPANSION_SLOT_ADD(SLOT3_TAG, comx_expansion_cards, NULL)
	MCFG_COMX_EXPANSION_SLOT_ADD(SLOT4_TAG, comx_expansion_cards, NULL)
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor comx_eb_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( comx_eb );
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  comx_eb_device - constructor
//-------------------------------------------------

comx_eb_device::comx_eb_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, COMX_EB, "COMX-35E Expansion Box", tag, owner, clock),
	device_comx_expansion_card_interface(mconfig, *this),
	device_slot_card_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void comx_eb_device::device_start()
{
	m_bus = machine().device<comx_expansion_bus_device>(COMX_EXPANSION_BUS_TAG);

	m_slot[0] = dynamic_cast<device_comx_expansion_card_interface *>(subdevice(SLOT1_TAG));
	m_slot[1] = dynamic_cast<device_comx_expansion_card_interface *>(subdevice(SLOT2_TAG));
	m_slot[2] = dynamic_cast<device_comx_expansion_card_interface *>(subdevice(SLOT3_TAG));
	m_slot[3] = dynamic_cast<device_comx_expansion_card_interface *>(subdevice(SLOT4_TAG));
	m_owner = dynamic_cast<comx_expansion_slot_device *>(owner());
	
	m_rom = subregion("e000")->base();
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void comx_eb_device::device_reset()
{
}


//-------------------------------------------------
//  comx_mrd_r - memory read
//-------------------------------------------------

UINT8 comx_eb_device::comx_mrd_r(offs_t offset)
{
	UINT8 data = 0;
	
	if (offset >= 0x1000 && offset < 0x1800)
	{
		data = m_rom[offset & 0x7ff];
	}
	else if (offset >= 0xe000 && offset < 0xf000)
	{
		data = m_rom[offset & 0xfff];
	}
	else
	{
		for (int slot = 0; slot < MAX_EB_SLOTS; slot++)
		{
			if (BIT(m_select, slot) && m_slot[slot] != NULL)
			{
				data |= m_slot[slot]->comx_mrd_r(offset);
			}
		}
	}
	
	return data;
}


//-------------------------------------------------
//  comx_q_w - Q write
//-------------------------------------------------

void comx_eb_device::comx_q_w(int state)
{
	for (int slot = 0; slot < MAX_EB_SLOTS; slot++)
	{
		if (BIT(m_select, slot) && m_slot[slot] != NULL)
		{
			m_slot[slot]->comx_q_w(state);
		}
	}
}


//-------------------------------------------------
//  comx_mwr_w - memory write
//-------------------------------------------------

void comx_eb_device::comx_mwr_w(offs_t offset, UINT8 data)
{
	for (int slot = 0; slot < MAX_EB_SLOTS; slot++)
	{
		if (BIT(m_select, slot) && m_slot[slot] != NULL)
		{
			m_slot[slot]->comx_mwr_w(offset, data);
		}
	}
}


//-------------------------------------------------
//  comx_io_r - I/O read
//-------------------------------------------------

UINT8 comx_eb_device::comx_io_r(offs_t offset)
{
	UINT8 data = 0;
	
	for (int slot = 0; slot < MAX_EB_SLOTS; slot++)
	{
		if (BIT(m_select, slot) && m_slot[slot] != NULL)
		{
			data |= m_slot[slot]->comx_io_r(offset);
		}
	}

	return data;
}


//-------------------------------------------------
//  comx_io_w - I/O write
//-------------------------------------------------

void comx_eb_device::comx_io_w(offs_t offset, UINT8 data)
{
	if (offset == 1)
	{
		m_select = data;
	}
	
	for (int slot = 0; slot < MAX_EB_SLOTS; slot++)
	{
		if (BIT(m_select, slot) && m_slot[slot] != NULL)
		{
			m_slot[slot]->comx_io_w(offset, data);
		}
	}
}


//-------------------------------------------------
//  comx_screen_update - screen update
//-------------------------------------------------

bool comx_eb_device::comx_screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)
{
	return false;
}
