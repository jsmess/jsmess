/***************************************************************************

	vidhrdw/cirrus.h

	Cirrus SVGA card emulation (preliminary)

***************************************************************************/

#ifndef CIRRUS_H
#define CIRRUS_H

#include "pc_vga.h"
#include "machine/pci.h"


extern const struct pc_svga_interface cirrus_svga_interface;
extern const struct pci_device_info cirrus5430_callbacks;

WRITE8_HANDLER( cirrus_42E8_w );
WRITE64_HANDLER( cirrus_64be_42E8_w );

#endif /* CIRRUS_H */
