/**********************************************************************

    Commodore IEEE-488 cartridge emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "c64_ieee488.h"



//**************************************************************************
//  MACROS/CONSTANTS
//**************************************************************************

#define MOS6525_TAG		"u3"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type C64_IEEE488 = &device_creator<c64_ieee488_device>;


//-------------------------------------------------
//  ROM( c64_ieee488 )
//-------------------------------------------------

ROM_START( c64_ieee488 )
	ROM_REGION( 0x1000, "roml", 0 )
	ROM_LOAD( "commodore ieee-488 cartridge.u3", 0x0000, 0x1000, CRC(e894e448) SHA1(c62b88b73d79b485ebb250d56065dfa54b8baf4c) )
ROM_END


//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *c64_ieee488_device::device_rom_region() const
{
	return ROM_NAME( c64_ieee488 );
}


//-------------------------------------------------
//  tpi6525_interface tpi_intf
//-------------------------------------------------

READ8_MEMBER( c64_ieee488_device::tpi_pa_r )
{
	/*

        bit     description

        PA0		
        PA1
        PA2		REN
        PA3		ATN
        PA4		DAV
        PA5		EOI
        PA6		NDAC
        PA7		NRFD

    */

	UINT8 data = 0;
	
	data |= m_bus->ren_r() << 2;
	data |= m_bus->atn_r() << 3;
	data |= m_bus->dav_r() << 4;
	data |= m_bus->eoi_r() << 5;
	data |= m_bus->ndac_r() << 6;
	data |= m_bus->nrfd_r() << 7;

	return data;
}

WRITE8_MEMBER( c64_ieee488_device::tpi_pa_w )
{
	/*

        bit     description

        PA0		U4 DC
        PA1		U4/U5 TE
        PA2		REN
        PA3		ATN
        PA4		DAV
        PA5		EOI
        PA6		NDAC
        PA7		NRFD

    */
	
	m_bus->ren_w(BIT(data, 2));
	m_bus->atn_w(BIT(data, 3));
	m_bus->dav_w(BIT(data, 4));
	m_bus->eoi_w(BIT(data, 5));
	m_bus->ndac_w(BIT(data, 6));
	m_bus->nrfd_w(BIT(data, 7));
}

READ8_MEMBER( c64_ieee488_device::tpi_pc_r )
{
	/*

        bit     description

        PC0		IFC
        PC1		SRQ
        PC2
        PC3
        PC4
        PC5
        PC6
        PC7		slot _EXROM

    */

	UINT8 data = 0;
	
	data |= m_bus->ifc_r();
	data |= m_bus->srq_r() << 1;
	
	data |= m_exp->exrom_r() << 7;

	return data;
}

WRITE8_MEMBER( c64_ieee488_device::tpi_pc_w )
{
	/*

        bit     description

        PC0		IFC
        PC1		SRQ
        PC2
        PC3		C64 _EXROM
        PC4		_ROML select
        PC5
        PC6
        PC7

    */

	m_bus->ifc_w(BIT(data, 0));
	m_bus->srq_w(BIT(data, 1));
	
	m_exrom = !BIT(data, 3);
	
	m_roml_sel = BIT(data, 4);
}

static const tpi6525_interface tpi_intf =
{
	DEVCB_NULL,
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, c64_ieee488_device, tpi_pa_r),
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, c64_ieee488_device, tpi_pa_w),
	DEVCB_DEVICE_MEMBER(IEEE488_TAG, ieee488_device, dio_r),
	DEVCB_DEVICE_MEMBER(IEEE488_TAG, ieee488_device, dio_w),
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, c64_ieee488_device, tpi_pc_r),
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, c64_ieee488_device, tpi_pc_w),
	DEVCB_NULL,
	DEVCB_NULL
};


//-------------------------------------------------
//  IEEE488_INTERFACE( ieee488_intf )
//-------------------------------------------------

static IEEE488_INTERFACE( ieee488_intf )
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};


//-------------------------------------------------
//  C64_EXPANSION_INTERFACE( expansion_intf )
//-------------------------------------------------

WRITE_LINE_MEMBER( c64_ieee488_device::irq_w )
{
	m_slot->irq_w(state);
}

WRITE_LINE_MEMBER( c64_ieee488_device::nmi_w )
{
	m_slot->nmi_w(state);
}

WRITE_LINE_MEMBER( c64_ieee488_device::dma_w )
{
	m_slot->dma_w(state);
}

WRITE_LINE_MEMBER( c64_ieee488_device::reset_w )
{
	m_slot->reset_w(state);
}

static C64_EXPANSION_INTERFACE( expansion_intf )
{
	DEVCB_LINE_MEMBER(c64_ieee488_device, irq_w),
	DEVCB_LINE_MEMBER(c64_ieee488_device, nmi_w),
	DEVCB_LINE_MEMBER(c64_ieee488_device, dma_w),
	DEVCB_LINE_MEMBER(c64_ieee488_device, reset_w)
};


//-------------------------------------------------
//  MACHINE_CONFIG_FRAGMENT( c64_ieee488 )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( c64_ieee488 )
	MCFG_TPI6525_ADD(MOS6525_TAG, tpi_intf)
	MCFG_CBM_IEEE488_ADD(ieee488_intf, NULL)
	MCFG_C64_EXPANSION_SLOT_ADD(C64_EXPANSION_SLOT_TAG, expansion_intf, c64_expansion_cards, NULL, NULL)
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor c64_ieee488_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( c64_ieee488 );
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  c64_ieee488_device - constructor
//-------------------------------------------------

c64_ieee488_device::c64_ieee488_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, C64_IEEE488, "IEEE-488", tag, owner, clock),
	device_c64_expansion_card_interface(mconfig, *this),
	m_tpi(*this, MOS6525_TAG),
    m_bus(*this, IEEE488_TAG),
    m_exp(*this, C64_EXPANSION_SLOT_TAG),
	m_roml_sel(1)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void c64_ieee488_device::device_start()
{
	m_roml = subregion("roml")->base();
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void c64_ieee488_device::device_reset()
{
	m_exrom = 0;
}


//-------------------------------------------------
//  c64_cd_r - cartridge data read
//-------------------------------------------------

UINT8 c64_ieee488_device::c64_cd_r(address_space &space, offs_t offset, int roml, int romh, int io1, int io2)
{
	UINT8 data = 0;

	if (!roml)
	{
		if (m_roml_sel)
		{
			data = m_roml[offset & 0xfff];
		}
		else
		{
			data = m_exp->roml_r(space, offset);
		}
	}
	else if (!romh)
	{
		data = m_exp->romh_r(space, offset);
	}
	else if (!io1)
	{
		data = m_exp->io1_r(space, offset);
	}
	else if (!io2)
	{
		data = m_exp->io2_r(space, offset);
		
		data |= tpi6525_r(m_tpi, offset & 0x07);
	}
	else
	{
		data = m_exp->read(space, offset);
	}

	return data;
}


//-------------------------------------------------
//  c64_cd_w - cartridge data write
//-------------------------------------------------

void c64_ieee488_device::c64_cd_w(address_space &space, offs_t offset, UINT8 data, int roml, int romh, int io1, int io2)
{
	if (!io1)
	{
		m_exp->io1_w(space, offset, data);
	}
	else if (!io2)
	{
		m_exp->io2_w(space, offset, data);
		
		tpi6525_w(m_tpi, offset & 0x07, data);
	}
	else
	{
		m_exp->write(space, offset, data);
	}
}


//-------------------------------------------------
//  c64_game_r - GAME read
//-------------------------------------------------

int c64_ieee488_device::c64_game_r(offs_t offset, int ba, int rw)
{
	return m_exp->game_r(offset, ba, rw);
}
