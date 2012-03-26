/**********************************************************************

    Commodore 64 CP/M cartridge emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "c64_cpm.h"



//**************************************************************************
//  MACROS/CONSTANTS
//**************************************************************************

#define Z80_TAG		"z80"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type C64_CPM = &device_creator<c64_cpm_cartridge_device>;


//-------------------------------------------------
//  ADDRESS_MAP( z80_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( z80_mem, AS_PROGRAM, 8, c64_cpm_cartridge_device )
	AM_RANGE(0x0000, 0xffff) AM_READWRITE(dma_r, dma_w)
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( z80_io )
//-------------------------------------------------

static ADDRESS_MAP_START( z80_io, AS_IO, 8, c64_cpm_cartridge_device )
	AM_RANGE(0x0000, 0xffff) AM_READWRITE(dma_r, dma_w)
ADDRESS_MAP_END


//-------------------------------------------------
//  MACHINE_CONFIG_FRAGMENT( c64_cpm )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( c64_cpm )
	MCFG_CPU_ADD(Z80_TAG, Z80, 3000000)
	MCFG_CPU_PROGRAM_MAP(z80_mem)
	MCFG_CPU_IO_MAP(z80_io)
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor c64_cpm_cartridge_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( c64_cpm );
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  c64_cpm_cartridge_device - constructor
//-------------------------------------------------

c64_cpm_cartridge_device::c64_cpm_cartridge_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, C64_CPM, "C64 CP/M cartridge", tag, owner, clock),
	device_c64_expansion_card_interface(mconfig, *this),
	m_maincpu(*this, Z80_TAG)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void c64_cpm_cartridge_device::device_start()
{
	// TODO stop the Z80 here before it gets any ideas
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void c64_cpm_cartridge_device::device_reset()
{
	m_slot->dma_w(CLEAR_LINE);
	m_maincpu->set_input_line(INPUT_LINE_HALT, ASSERT_LINE);
}


//-------------------------------------------------
//  c64_cd_w - cartridge data write
//-------------------------------------------------

void c64_cpm_cartridge_device::c64_cd_w(address_space &space, offs_t offset, UINT8 data, int roml, int romh, int io1, int io2)
{
	if (!io1)
	{
		// HACK
		m_slot->dma_w(BIT(data, 0) ? ASSERT_LINE : CLEAR_LINE);
		m_maincpu->set_input_line(INPUT_LINE_HALT, BIT(data, 0) ? CLEAR_LINE : ASSERT_LINE);
	}
}


//-------------------------------------------------
//	dma_r -
//-------------------------------------------------

READ8_MEMBER( c64_cpm_cartridge_device::dma_r )
{
	return m_slot->dma_cd_r(offset | 0x1000);
}


//-------------------------------------------------
//  dma_w -
//-------------------------------------------------

WRITE8_MEMBER( c64_cpm_cartridge_device::dma_w )
{
	m_slot->dma_cd_w(offset | 0x1000, data);
}
