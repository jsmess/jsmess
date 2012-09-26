/***************************************************************************

    Intel 82371AB PCI IDE ISA Xcelerator (PIIX4)

    Part of the Intel 430TX chipset

***************************************************************************/

#ifndef __I82371AB_H__
#define __I82371AB_H__


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _i82371ab_config i82371ab_config;
struct _i82371ab_config
{
	int dummy;
};


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/
UINT32 i82371ab_pci_read(device_t *busdevice, device_t *device, int function, int offset, UINT32 mem_mask);
void i82371ab_pci_write(device_t *busdevice, device_t *device, int function, int offset, UINT32 data, UINT32 mem_mask);


/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

DECLARE_LEGACY_DEVICE(I82371AB, i82371ab);

#define MCFG_I82371AB_ADD(_tag) \
	MCFG_DEVICE_ADD(_tag, I82371AB, 0)


#endif /* __I82371AB_H__ */
