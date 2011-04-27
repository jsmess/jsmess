/**********************************************************************

    Commodore 9060/9090 Hard Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __C9060__
#define __C9060__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "imagedev/flopdrv.h"
#include "machine/6522via.h"
#include "machine/6532riot.h"
#include "machine/ieee488.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define C9060_TAG			"c9060"
#define C9090_TAG			"c9090"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_C9060_ADD(_tag, _address) \
    MCFG_DEVICE_ADD(_tag, C9060, 0) \
	c9060_device::static_set_config(*device, _address, TYPE_9060);


#define MCFG_C9090_ADD(_tag, _address) \
    MCFG_DEVICE_ADD(_tag, C9090, 0) \
	c9060_device::static_set_config(*device, _address, TYPE_9090);



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************


// ======================> c9060_device

class c9060_device :  public device_t,
					  public device_ieee488_interface
{

public:
    // construction/destruction
    c9060_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// inline configuration helpers
	static void static_set_config(device_t &device, int address, int variant);

	// optional information overrides
	virtual const rom_entry *device_rom_region() const;
	virtual machine_config_constructor device_mconfig_additions() const;

	// not really public
	DECLARE_READ8_MEMBER( dio_r );
	DECLARE_WRITE8_MEMBER( dio_w );
	DECLARE_READ8_MEMBER( riot1_pa_r );
	DECLARE_WRITE8_MEMBER( riot1_pa_w );
	DECLARE_READ8_MEMBER( riot1_pb_r );
	DECLARE_WRITE8_MEMBER( riot1_pb_w );
	DECLARE_READ8_MEMBER( via_pa_r );
	DECLARE_READ8_MEMBER( via_pb_r );
	DECLARE_WRITE8_MEMBER( via_pa_w );
	DECLARE_WRITE8_MEMBER( via_pb_w );
	enum
	{
		TYPE_9060 = 0,
		TYPE_9090
	};

protected:
    // device-level overrides
    virtual void device_start();
	virtual void device_reset();
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);
    virtual void device_config_complete();

	// device_ieee488_interface overrides
	void ieee488_atn(int state);
	void ieee488_ifc(int state);

private:
	inline void update_ieee_signals();

	required_device<cpu_device> m_maincpu;
	required_device<cpu_device> m_hdccpu;
	required_device<riot6532_device> m_riot0;
	required_device<riot6532_device> m_riot1;
	required_device<via6522_device> m_via;
	required_device<ieee488_device> m_bus;

	// IEEE-488 bus
	int m_rfdo;							// not ready for data output
	int m_daco;							// not data accepted output
	int m_atna;							// attention acknowledge

	int m_address;
	int m_variant;
};


// device type definition
extern const device_type C9060;
extern const device_type C9090;



#endif
