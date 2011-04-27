/***************************************************************************

    video/cirrus.h

    Cirrus SVGA card emulation (preliminary)

***************************************************************************/

#ifndef CIRRUS_H
#define CIRRUS_H

#include "machine/pci.h"

#define MCFG_CIRRUS_ADD(_tag) \
    MCFG_DEVICE_ADD(_tag, CIRRUS, 0) \

// ======================> cirrus_device

class cirrus_device : public device_t
{
public:
		// construction/destruction
    cirrus_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	UINT32 pci_read(device_t *busdevice, int function, int offset, UINT32 mem_mask);
	void   pci_write(device_t *busdevice, int function, int offset, UINT32 data, UINT32 mem_mask);

protected:
    // device-level overrides
    virtual void device_start();
    virtual void device_reset();

private:
};


// device type definition
extern const device_type CIRRUS;

UINT32 cirrus5430_pci_read(device_t *busdevice, device_t *device, int function, int offset, UINT32 mem_mask);
void cirrus5430_pci_write(device_t *busdevice, device_t *device, int function, int offset, UINT32 data, UINT32 mem_mask);

WRITE8_DEVICE_HANDLER( cirrus_42E8_w );

#endif /* CIRRUS_H */
