/***************************************************************************

    Intel 82439TX System Controller (MTXC)

    Part of the Intel 430TX chipset

***************************************************************************/

#ifndef __I82439TX_H__
#define __I82439TX_H__


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _i82439tx_config i82439tx_config;
struct _i82439tx_config
{
	const char *cputag;
	const char *rom_region;
};


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

DEVICE_GET_INFO( i82439tx );

UINT32 i82439tx_pci_read(running_device *busdevice, running_device *device, int function, int offset, UINT32 mem_mask);
void i82439tx_pci_write(running_device *busdevice, running_device *device, int function, int offset, UINT32 data, UINT32 mem_mask);


/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define I82439TX DEVICE_GET_INFO_NAME(i82439tx)

#define MDRV_I82439TX_ADD(_tag, _cputag, _rom_region) \
	MDRV_DEVICE_ADD(_tag, I82439TX, 0) \
	MDRV_DEVICE_CONFIG_DATAPTR(i82439tx_config, cputag, _cputag) \
	MDRV_DEVICE_CONFIG_DATAPTR(i82439tx_config, rom_region, _rom_region)


#endif /* __I82439TX_H__ */
