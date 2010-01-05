/***************************************************************************

    Intel 82371AB PCI IDE ISA Xcelerator (PIIX4)

    Part of the Intel 430TX chipset

    - Integrated IDE Controller
    - Enhanced DMA Controller based on two 82C37
    - Interrupt Controller based on two 82C59
    - Timers based on 82C54
    - USB
    - SMBus
    - Real Time Clock based on MC146818

***************************************************************************/

#include "driver.h"
#include "i82371ab.h"


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _i82371ab_state i82371ab_state;
struct _i82371ab_state
{
	UINT32 dummy;
};


/*****************************************************************************
    INLINE FUNCTIONS
*****************************************************************************/

INLINE i82371ab_state *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == I82371AB);

	return (i82371ab_state *)device->token;
}


/***************************************************************************
    PCI INTERFACE
***************************************************************************/

UINT32 i82371ab_pci_read(const device_config *busdevice, const device_config *device, int function, int offset, UINT32 mem_mask)
{
	UINT32 result = 0;

	logerror("i82371ab_pci r function = %d, offset = %02x\n", function, offset);

	if (function != 0)
		return 0;

	switch (offset)
	{
	case 0x00:
		result = 0x71108086;
		break;
	case 0x04:
		result = 0x00000000;
		break;
	case 0x08:
		result = 0x06010000;
		break;
	case 0x0c:
		result = 0x00800000;
		break;
	}

	return result;
}

void i82371ab_pci_write(const device_config *busdevice, const device_config *device, int function, int offset, UINT32 data, UINT32 mem_mask)
{
	logerror("i82371ab_pci w function = %d, offset = %02x\n", function, offset);
}


/*****************************************************************************
    DEVICE INTERFACE
*****************************************************************************/

static DEVICE_START( i82371ab )
{
	i82371ab_state *i82371ab = get_safe_token(device);

	i82371ab->dummy = 0;

	/* setup save states */
}

static DEVICE_RESET( i82371ab )
{
	i82371ab_state *i82371ab = get_safe_token(device);

	i82371ab->dummy = 0;
}


/***************************************************************************
    DEVICE GETINFO
***************************************************************************/

static const char DEVTEMPLATE_SOURCE[] = __FILE__;

#define DEVTEMPLATE_ID(p,s)				p##i82371ab##s
#define DEVTEMPLATE_FEATURES			DT_HAS_START | DT_HAS_RESET
#define DEVTEMPLATE_NAME				"Intel 82371AB"
#define DEVTEMPLATE_FAMILY				"South Bridge"
#define DEVTEMPLATE_CLASS				DEVICE_CLASS_OTHER
#define DEVTEMPLATE_VERSION				"1.0"
#define DEVTEMPLATE_CREDITS				"Copyright MESS Team"
#include "devtempl.h"
