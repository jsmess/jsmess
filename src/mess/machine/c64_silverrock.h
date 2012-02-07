/**********************************************************************

    SilverRock Productions cartridge emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __SILVERROCK__
#define __SILVERROCK__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "machine/c64exp.h"



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> c64_silverrock_cartridge_device

class c64_silverrock_cartridge_device : public device_t,
										public device_c64_expansion_card_interface
{
public:
	// construction/destruction
	c64_silverrock_cartridge_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

protected:
	// device-level overrides
	virtual void device_start();
	virtual void device_reset();

	// device_c64_expansion_card_interface overrides
	virtual UINT8 c64_cd_r(offs_t offset, int roml, int romh, int io1, int io2);
	virtual void c64_cd_w(offs_t offset, UINT8 data, int roml, int romh, int io1, int io2);
	virtual int c64_game_r() { return 0; }
	virtual int c64_exrom_r() { return 0; }
	virtual UINT8* c64_roml_pointer(size_t size);

private:
	UINT8 *m_roml;
	UINT8 m_bank;
};


// device type definition
extern const device_type C64_SILVERROCK;


#endif
