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

protected:
	// device-level overrides
    virtual void device_config_complete() { m_shortname = "c64_std"; }
	virtual void device_start();

	// device_c64_expansion_card_interface overrides
	virtual UINT8 c64_cd_r(offs_t offset, int roml, int romh, int io1, int io2);
	virtual UINT8* c64_roml_pointer();
	virtual UINT8* c64_romh_pointer();

private:
	UINT8 *m_roml;
	UINT8 *m_romh;
};


// device type definition
extern const device_type C64_STD;


#endif
