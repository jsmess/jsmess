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

DEVICE_GET_INFO( i82371ab );

UINT32 i82371ab_pci_read(running_device *busdevice, running_device *device, int function, int offset, UINT32 mem_mask);
void i82371ab_pci_write(running_device *busdevice, running_device *device, int function, int offset, UINT32 data, UINT32 mem_mask);


/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define I82371AB DEVICE_GET_INFO_NAME(i82371ab)

#define MDRV_I82371AB_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, I82371AB, 0)


#endif /* __I82371AB_H__ */
