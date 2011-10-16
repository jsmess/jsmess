#pragma once

#ifndef __LUXOR_4105__
#define __LUXOR_4105__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "abc1600_bus.h"
#include "machine/scsibus.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define LUXOR_4105_TAG		"luxor_4105"


#define ADDRESS				0x25



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> luxor_4105_device

class luxor_4105_device :  public device_t,
						   public device_abc1600bus_card_interface,
						   public device_slot_card_interface
{
public:
    // construction/destruction
    luxor_4105_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// optional information overrides
	virtual machine_config_constructor device_mconfig_additions() const;
	virtual ioport_constructor device_input_ports() const;

protected:
    // device-level overrides
    virtual void device_start();
	virtual void device_reset();

	// device_abc1600bus_interface overrides
	virtual void abc1600bus_cs(UINT8 data);
	virtual int abc1600bus_csb();
	virtual void abc1600bus_brst();
	virtual UINT8 abc1600bus_inp();
	virtual void abc1600bus_out(UINT8 data);
	virtual UINT8 abc1600bus_stat();
	virtual void abc1600bus_c1(UINT8 data);
	virtual void abc1600bus_c3(UINT8 data);
	virtual void abc1600bus_c4(UINT8 data);
	
private:
	required_device<device_t> m_sasibus;

	int m_cs;
	UINT8 m_data;
	UINT8 m_dma;
};


// device type definition
extern const device_type LUXOR_4105;



#endif
