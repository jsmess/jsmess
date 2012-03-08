/***************************************************************************

    Intel 82371SB PCI IDE ISA Xcelerator (PIIX3)

    Part of the Intel 430TX chipset

***************************************************************************/

#ifndef __I82371SB_H__
#define __I82371SB_H__


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _i82371sb_config i82371sb_config;
struct _i82371sb_config
{
	int dummy;
};


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/
UINT32 i82371sb_pci_read(device_t *busdevice, device_t *device, int function, int offset, UINT32 mem_mask);
void i82371sb_pci_write(device_t *busdevice, device_t *device, int function, int offset, UINT32 data, UINT32 mem_mask);


/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

DECLARE_LEGACY_DEVICE(I82371SB, i82371sb);

#define MCFG_I82371SB_ADD(_tag) \
	MCFG_DEVICE_ADD(_tag, I82371SB, 0)


#endif /* __I82371SB_H__ */
