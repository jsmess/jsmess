/*
 * cpc_ssa1.c  --  Amstrad SSA-1 Speech Synthesiser, dk'Tronics Speech Synthesiser
 *
 *  Created on: 16/07/2011
 *
 */

#include "emu.h"
#include "cpc_ssa1.h"

//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type CPC_SSA1 = &device_creator<cpc_ssa1_device>;
const device_type CPC_DKSPEECH = &device_creator<cpc_dkspeech_device>;

//-------------------------------------------------
//  device I/O handlers
//-------------------------------------------------

static READ8_DEVICE_HANDLER(ssa1_r)
{
	UINT8 ret = 0xff;
	cpc_ssa1_device* ssa1 = device->machine().device<cpc_ssa1_device>("exp:ssa1");

	if(ssa1->get_sby() == 0)
		ret &= ~0x80;

	if(ssa1->get_lrq() != 0)
		ret &= ~0x40;

	return ret;
}

static WRITE8_DEVICE_HANDLER(ssa1_w)
{
	sp0256_ALD_w(device,0,data);
}

static READ8_DEVICE_HANDLER(dkspeech_r)
{
	UINT8 ret = 0xff;
	cpc_dkspeech_device* dkspeech = device->machine().device<cpc_dkspeech_device>("exp:dkspeech");

	// SBY is not connected

	if(dkspeech->get_lrq() != 0)
		ret &= ~0x80;

	return ret;
}

static WRITE8_DEVICE_HANDLER(dkspeech_w)
{
	sp0256_ALD_w(device,0,data & 0x3f);
}

static WRITE_LINE_DEVICE_HANDLER(ssa1_lrq_cb)
{
	cpc_ssa1_device* ssa1 = device->machine().device<cpc_ssa1_device>("exp:ssa1");
	ssa1->set_lrq(state);
}

static WRITE_LINE_DEVICE_HANDLER(ssa1_sby_cb)
{
	cpc_ssa1_device* ssa1 = device->machine().device<cpc_ssa1_device>("exp:ssa1");
	ssa1->set_sby(state);
}

static WRITE_LINE_DEVICE_HANDLER(dk_lrq_cb)
{
	cpc_dkspeech_device* dkspeech = device->machine().device<cpc_dkspeech_device>("exp:dkspeech");
	dkspeech->set_lrq(state);
}

static WRITE_LINE_DEVICE_HANDLER(dk_sby_cb)
{
	cpc_dkspeech_device* dkspeech = device->machine().device<cpc_dkspeech_device>("exp:dkspeech");
	dkspeech->set_sby(state);
}

static sp0256_interface sp0256_intf =
{
		DEVCB_LINE(ssa1_lrq_cb),
		DEVCB_LINE(ssa1_sby_cb)
};

static sp0256_interface sp0256_dk_intf =
{
		DEVCB_LINE(dk_lrq_cb),
		DEVCB_LINE(dk_sby_cb)
};

//-------------------------------------------------
//  Device ROM definition
//-------------------------------------------------

// Has no actual ROM, just that internal to the SP0256
ROM_START( cpc_ssa1 )
	ROM_REGION( 0x10000, "sp0256", 0 )
	ROM_LOAD( "sp0256-al2.bin",   0x1000, 0x0800, CRC(b504ac15) SHA1(e60fcb5fa16ff3f3b69d36c7a6e955744d3feafc) )
ROM_END

ROM_START( cpc_dkspeech )
	ROM_REGION( 0x10000, "sp0256", 0 )
	ROM_LOAD( "sp0256-al2.bin",   0x1000, 0x0800, CRC(b504ac15) SHA1(e60fcb5fa16ff3f3b69d36c7a6e955744d3feafc) )
	// TODO: Add expansion ROM
ROM_END

//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *cpc_ssa1_device::device_rom_region() const
{
	return ROM_NAME( cpc_ssa1 );
}

const rom_entry *cpc_dkspeech_device::device_rom_region() const
{
	return ROM_NAME( cpc_dkspeech );
}

// device machine config
static MACHINE_CONFIG_FRAGMENT( cpc_ssa1 )
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("sp0256",SP0256,XTAL_3_12MHz)
	MCFG_SOUND_CONFIG(sp0256_intf)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_CONFIG_END

static MACHINE_CONFIG_FRAGMENT( cpc_dkspeech )
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("sp0256",SP0256,XTAL_4MHz)  // uses the CPC's clock from pin 50 of the expansion port
	MCFG_SOUND_CONFIG(sp0256_dk_intf)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_CONFIG_END

machine_config_constructor cpc_ssa1_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( cpc_ssa1 );
}

machine_config_constructor cpc_dkspeech_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( cpc_dkspeech );
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

cpc_ssa1_device::cpc_ssa1_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, CPC_SSA1, "SSA-1", tag, owner, clock),
	device_cpc_expansion_card_interface(mconfig, *this),
	device_slot_card_interface(mconfig, *this),
	m_lrq(1)
{
}

cpc_dkspeech_device::cpc_dkspeech_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, CPC_DKSPEECH, "DK'T Speech", tag, owner, clock),
	device_cpc_expansion_card_interface(mconfig, *this),
	device_slot_card_interface(mconfig, *this),
	m_lrq(1)
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void cpc_ssa1_device::device_start()
{
	device_t* cpu = machine().device("maincpu");
	address_space* space = cpu->memory().space(AS_IO);
	m_slot = dynamic_cast<cpc_expansion_slot_device *>(owner());

	m_rom = subregion("sp0256")->base();

	m_sp0256_device = subdevice("sp0256");

	space->install_legacy_readwrite_handler(*m_sp0256_device,0xfaee,0xfaee,0,0,FUNC(ssa1_r),FUNC(ssa1_w));
	space->install_legacy_readwrite_handler(*m_sp0256_device,0xfbee,0xfbee,0,0,FUNC(ssa1_r),FUNC(ssa1_w));
}

void cpc_dkspeech_device::device_start()
{
	device_t* cpu = machine().device("maincpu");
	address_space* space = cpu->memory().space(AS_IO);
	m_slot = dynamic_cast<cpc_expansion_slot_device *>(owner());

	m_rom = subregion("sp0256")->base();

	m_sp0256_device = subdevice("sp0256");

	space->install_legacy_readwrite_handler(*m_sp0256_device,0xfbfe,0xfbfe,0,0,FUNC(dkspeech_r),FUNC(dkspeech_w));
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void cpc_ssa1_device::device_reset()
{
	m_sp0256_device->reset();
}

void cpc_dkspeech_device::device_reset()
{
	m_sp0256_device->reset();
}

