#pragma once

#ifndef __NORTHBRIDGE_H__
#define __NORTHBRIDGE_H__

#include "emu.h"

#include "machine/ram.h"

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> northbridge_device

class northbridge_device :
  	  public device_t
{
public:
		// construction/destruction
        northbridge_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

protected:
        // device-level overrides
        virtual void device_start();
        virtual void device_reset();
public:
		required_device<cpu_device> m_maincpu;
		required_device<ram_device> m_ram;

};


// device type definition
extern const device_type NORTHBRIDGE;

#endif  /* __NORTHBRIDGE_H__ */