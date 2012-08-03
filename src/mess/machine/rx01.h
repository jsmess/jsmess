/**********************************************************************

	DEC RX01 floppy drive controller

**********************************************************************/

#pragma once

#ifndef __RX01__
#define __RX01__

//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_RX01_ADD(_tag) \
	MCFG_DEVICE_ADD(_tag, RX01, 0)

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> rx01_device

class rx01_device :  public device_t
{
public:
    // construction/destruction
    rx01_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	DECLARE_READ16_MEMBER( read );
	DECLARE_WRITE16_MEMBER( write );

protected:
    // device-level overrides
    virtual void device_start();
	virtual void device_reset();

private:
};

// device type definition
extern const device_type RX01;

#endif
