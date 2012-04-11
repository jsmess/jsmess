/**********************************************************************

    Currah Speech 64 cartridge emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

PCB Layout
----------

|===========================|
|=|                         |
|=|         VLSI            |
|=|                         |
|=|                         |
|=|         ROM             |
|=|                         |
|=|                         |
|=|         SP0256          |
|===========================|

Notes:
    All IC's shown.

	VLSI   - General Instruments LA05-164 custom
	ROM    - General Instruments R09864CS-2030 8Kx8 ROM "778R01"
	SP0256 - General Instruments SP0256A-AL2 Speech Synthesizer

*/

#include "c64_currah_speech.h"



//**************************************************************************
//  MACROS/CONSTANTS
//**************************************************************************

#define SP0256_TAG		"sp0256"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type C64_CURRAH_SPEECH = &device_creator<c64_currah_speech_cartridge_device>;


//-------------------------------------------------
//  ROM( c64_currah_speech )
//-------------------------------------------------

ROM_START( c64_currah_speech )
	ROM_REGION( 0x800, SP0256_TAG, 0 )
	ROM_LOAD( "sp0256a-al2", 0x000, 0x800, CRC(df8de0b0) SHA1(86fb6d9fef955ac0bc76e0c45c66585946d278a1) )
ROM_END


//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *c64_currah_speech_cartridge_device::device_rom_region() const
{
	return ROM_NAME( c64_currah_speech );
}


//-------------------------------------------------
//  sp0256_interface sp0256_intf
//-------------------------------------------------

static sp0256_interface sp0256_intf =
{
	DEVCB_NULL,
	DEVCB_NULL
};


//-------------------------------------------------
//  MACHINE_CONFIG_FRAGMENT( c64_currah_speech )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( c64_currah_speech )
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(SP0256_TAG, SP0256, 1000000) // ???
	MCFG_SOUND_CONFIG(sp0256_intf)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor c64_currah_speech_cartridge_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( c64_currah_speech );
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  c64_currah_speech_cartridge_device - constructor
//-------------------------------------------------

c64_currah_speech_cartridge_device::c64_currah_speech_cartridge_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, C64_CURRAH_SPEECH, "C64 Currah Speech", tag, owner, clock),
	device_c64_expansion_card_interface(mconfig, *this),
	m_nsp(*this, SP0256_TAG)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void c64_currah_speech_cartridge_device::device_start()
{
	// find memory regions
	m_rom = subregion(SP0256_TAG)->base();
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void c64_currah_speech_cartridge_device::device_reset()
{
}


//-------------------------------------------------
//  c64_cd_r - cartridge data read
//-------------------------------------------------

UINT8 c64_currah_speech_cartridge_device::c64_cd_r(address_space &space, offs_t offset, int roml, int romh, int io1, int io2)
{
	UINT8 data = 0;

	if (!io1)
	{
		/*
		
		    bit     description
		
		    0       
		    1       
		    2       
		    3       
		    4       
		    5       
		    6       
		    7       LRQ
		
		*/

		data = sp0256_lrq_r(m_nsp) << 7;
	}

	return data;
}


//-------------------------------------------------
//  c64_cd_w - cartridge data write
//-------------------------------------------------

void c64_currah_speech_cartridge_device::c64_cd_w(address_space &space, offs_t offset, UINT8 data, int roml, int romh, int io1, int io2)
{
	if (!io1)
	{
		// TODO offset bit 0 = high/low voice
		sp0256_ALD_w(m_nsp, 0, data);
	}
}


//-------------------------------------------------
//  c64_game_r - GAME read
//-------------------------------------------------

int c64_currah_speech_cartridge_device::c64_game_r(offs_t offset, int ba, int rw, int hiram)
{
	return 1; // TODO
}


//-------------------------------------------------
//  c64_exrom_r - EXROM read
//-------------------------------------------------

int c64_currah_speech_cartridge_device::c64_exrom_r()
{
	return 1; // TODO
}
