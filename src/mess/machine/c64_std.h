/**********************************************************************

    Commodore 64 Standard 8K/16K cartridge emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __C64_STANDARD_CARTRIDGE__
#define __C64_STANDARD_CARTRIDGE__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "imagedev/cartslot.h"
#include "machine/c64exp.h"



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> c64_standard_cartridge_device

class c64_standard_cartridge_device : public device_t,
									  public device_c64_expansion_card_interface
{
public:
	// construction/destruction
	c64_standard_cartridge_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// optional information overrides
	virtual const rom_entry *device_rom_region() const;

protected:
	// device-level overrides
	virtual void device_start();
	virtual void device_reset();
    virtual void device_config_complete() { m_shortname = "c64_std"; }

	// device_c64_expansion_card_interface overrides
	virtual UINT8 c64_cd_r(offs_t offset, int roml, int romh, int io1, int io2);
	virtual int c64_game_r() { return m_game; }
	virtual int c64_exrom_r() { return m_exrom; }

	virtual UINT8* get_cart_base() { return m_rom; }

private:
	c64_expansion_slot_device *m_slot;

	UINT8 *m_rom;
	int m_game;
	int m_exrom;
};


// device type definition
extern const device_type C64_STD;


#endif
