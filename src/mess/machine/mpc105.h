/***************************************************************************

    machine/mpc105.h

    Motorola MPC105 PCI bridge

***************************************************************************/

#ifndef MPC105_H
#define MPC105_H

#define MPC105_MEMORYBANK_COUNT		8

void mpc105_init(running_machine *machine, int bank_base);
UINT32 mpc105_pci_read(device_t *busdevice, device_t *device, int function, int offset, UINT32 mem_mask);
void mpc105_pci_write(device_t *busdevice, device_t *device, int function, int offset, UINT32 data, UINT32 mem_mask);

#endif /* MPC105_H */
