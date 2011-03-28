/***************************************************************************

    video/cirrus.h

    Cirrus SVGA card emulation (preliminary)

***************************************************************************/

#ifndef CIRRUS_H
#define CIRRUS_H

#include "machine/pci.h"

#define MCFG_CIRRUS_ADD(_tag) \
    MCFG_DEVICE_ADD(_tag, CIRRUS, 0) \
	
// ======================> cirrus_device_config

class cirrus_device_config :  public device_config
{
    friend class cirrus_device;

    // construction/destruction
    cirrus_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);

public:
    // allocators
    static device_config *static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
    virtual device_t *alloc_device(running_machine &machine) const;
	
protected:
    // internal state goes here
};



// ======================> cirrus_device

class cirrus_device : public device_t
{
    friend class cirrus_device_config;

    // construction/destruction
    cirrus_device(running_machine &_machine, const cirrus_device_config &config);

public:
	UINT32 pci_read(device_t *busdevice, int function, int offset, UINT32 mem_mask);
	void   pci_write(device_t *busdevice, int function, int offset, UINT32 data, UINT32 mem_mask);

protected:
    // device-level overrides
    virtual void device_start();
    virtual void device_reset();

	// internal state
    const cirrus_device_config &m_config;

private:	
};


// device type definition
extern const device_type CIRRUS;

UINT32 cirrus5430_pci_read(device_t *busdevice, device_t *device, int function, int offset, UINT32 mem_mask);
void cirrus5430_pci_write(device_t *busdevice, device_t *device, int function, int offset, UINT32 data, UINT32 mem_mask);

WRITE8_DEVICE_HANDLER( cirrus_42E8_w );

#endif /* CIRRUS_H */
