/***************************************************************************

    machine/i82439tx.h

    Intel 82439TX PCI Bridge

***************************************************************************/

#ifndef I82439TX_H
#define I82439TX_H

void intel82439tx_init(running_machine *machine);
UINT32 intel82439tx_pci_read(const device_config *busdevice, const device_config *device, int function, int offset, UINT32 mem_mask);
void intel82439tx_pci_write(const device_config *busdevice, const device_config *device, int function, int offset, UINT32 data, UINT32 mem_mask);

#endif /* I82439TX_H */
