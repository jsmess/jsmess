/**********************************************************************

    RCA VP550 - VIP Super Sound System emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __VP550__
#define __VP550__

#include "emu.h"
#include "cpu/cosmac/cosmac.h"
#include "sound/cdp1863.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define VP550_TAG	"vp550"
#define VP551_TAG	"vp551"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_VP550_ADD(_clock) \
	MCFG_DEVICE_ADD(VP550_TAG, VP550, _clock) \
	vp550_device::static_set_config(*device, vp550_device::TYPE_VP550);


#define MCFG_VP551_ADD(_clock) \
	MCFG_DEVICE_ADD(VP551_TAG, VP551, _clock) \
	vp550_device::static_set_config(*device, vp550_device::TYPE_VP551);



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> vp550_device

class vp550_device :	public device_t
{
public:
	enum
	{
		TYPE_VP550 = 0,
		TYPE_VP551
	};

    // construction/destruction
    vp550_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	DECLARE_WRITE8_MEMBER( octave_w );
	DECLARE_WRITE8_MEMBER( vlmna_w );
	DECLARE_WRITE8_MEMBER( vlmnb_w );
	DECLARE_WRITE8_MEMBER( sync_w );
	DECLARE_WRITE_LINE_MEMBER( q_w );
	DECLARE_WRITE_LINE_MEMBER( sc1_w );

	void install_write_handlers(address_space *space, bool enabled);

	// inline configuration helpers
	static void static_set_config(device_t &device, int variant);

	// optional information overrides
	virtual machine_config_constructor device_mconfig_additions() const;

protected:
    // device-level overrides
    virtual void device_start();
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);
private:
	int m_variant;

	required_device<cdp1863_device> m_pfg_a;
	required_device<cdp1863_device> m_pfg_b;
	optional_device<cdp1863_device> m_pfg_c;
	optional_device<cdp1863_device> m_pfg_d;

	// timers
	emu_timer *m_sync_timer;
};


// device type definition
extern const device_type VP550;
extern const device_type VP551;



#endif
