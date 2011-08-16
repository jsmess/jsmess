#pragma once

#ifndef __DP8390X_H__
#define __DP8390X_H__

#include "emu.h"

#define MCFG_DP8390X_ADD(_tag, _clock) \
	MCFG_DEVICE_ADD(_tag, DP8390X, _clock)

#define MCFG_DP8390X_REPLACE(_tag, _clock) \
	MCFG_DEVICE_REPLACE(_tag, DP8390X, _clock)

#define MCFG_DP8390X_REMOVE(_tag) \
	MCFG_DEVICE_REMOVE(_tag)

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> dp8390x_device

class dp8390x_device :
		public device_t
{
public:
        // construction/destruction
        dp8390x_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
		dp8390x_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock);

        DECLARE_READ8_MEMBER(reg_r);
        DECLARE_WRITE8_MEMBER(reg_w);
        
protected:
        // device-level overrides
        virtual void device_start();
        virtual void device_reset();

private:
        UINT8 m_registers[0x10*3];
        int m_regbank;
};


// device type definition
extern const device_type DP8390X;

#endif  /* __DP8390X_H__ */
