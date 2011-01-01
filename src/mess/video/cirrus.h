/***************************************************************************

    video/cirrus.h

    Cirrus SVGA card emulation (preliminary)

***************************************************************************/

#ifndef CIRRUS_H
#define CIRRUS_H

#include "pc_vga_mess.h"
#include "machine/pci.h"


extern const struct pc_svga_interface cirrus_svga_interface;

UINT32 cirrus5430_pci_read(device_t *busdevice, device_t *device, int function, int offset, UINT32 mem_mask);
void cirrus5430_pci_write(device_t *busdevice, device_t *device, int function, int offset, UINT32 data, UINT32 mem_mask);

WRITE8_HANDLER( cirrus_42E8_w );
WRITE64_HANDLER( cirrus_64be_42E8_w );

#endif /* CIRRUS_H */
