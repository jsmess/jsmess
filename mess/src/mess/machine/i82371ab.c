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

#include "emu.h"
#include "i82371ab.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _i82371ab_state i82371ab_state;
struct _i82371ab_state
{
	UINT32 regs[4][0x100/4];
};


/*****************************************************************************
    INLINE FUNCTIONS
*****************************************************************************/

INLINE i82371ab_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == I82371AB);

	return (i82371ab_state *)downcast<legacy_device_base *>(device)->token();
}


/***************************************************************************
    PCI INTERFACE
***************************************************************************/

static UINT32 i82371ab_pci_isa_r(device_t *busdevice, device_t *device, int offset, UINT32 mem_mask)
{
	i82371ab_state *i82371ab = get_safe_token(device);
	UINT32 result = i82371ab->regs[0][offset];

	logerror("i82371ab_pci_isa_r, offset = %02x, mem_mask = %08x\n", offset, mem_mask);

	return result;
}

static void i82371ab_pci_isa_w(device_t *busdevice, device_t *device, int offset, UINT32 data, UINT32 mem_mask)
{
	i82371ab_state *i82371ab = get_safe_token(device);

	logerror("i82371ab_pci_isa_w, offset = %02x, data = %08x, mem_mask = %08x\n", offset, data, mem_mask);

	switch (offset)
	{
	case 0x04:
		COMBINE_DATA(&i82371ab->regs[0][offset]);

		/* clear reserved bits */
		i82371ab->regs[0][offset] &= 0x00000005;

		/* set new status */
		i82371ab->regs[0][offset] |= 0x02800000;

		break;
	}
}

static UINT32 i82371ab_pci_ide_r(device_t *busdevice, device_t *device, int offset, UINT32 mem_mask)
{
	i82371ab_state *i82371ab = get_safe_token(device);
	UINT32 result = i82371ab->regs[1][offset];

	logerror("i82371ab_pci_ide_r, offset = %02x, mem_mask = %08x\n", offset, mem_mask);

	return result;
}

static void i82371ab_pci_ide_w(device_t *busdevice, device_t *device, int offset, UINT32 data, UINT32 mem_mask)
{
	i82371ab_state *i82371ab = get_safe_token(device);

	logerror("i82371ab_pci_ide_w, offset = %02x, data = %08x, mem_mask = %08x\n", offset, data, mem_mask);

	switch (offset)
	{
	case 0x04:
		COMBINE_DATA(&i82371ab->regs[1][offset]);

		/* clear reserved bits */
		i82371ab->regs[1][offset] &= 0x00000005;

		/* set new status */
		i82371ab->regs[1][offset] |= 0x02800000;

		break;
	}
}

static UINT32 i82371ab_pci_usb_r(device_t *busdevice, device_t *device, int offset, UINT32 mem_mask)
{
	i82371ab_state *i82371ab = get_safe_token(device);
	UINT32 result = i82371ab->regs[2][offset];

	logerror("i82371ab_pci_usb_r, offset = %02x, mem_mask = %08x\n", offset, mem_mask);

	return result;
}

static void i82371ab_pci_usb_w(device_t *busdevice, device_t *device, int offset, UINT32 data, UINT32 mem_mask)
{
	i82371ab_state *i82371ab = get_safe_token(device);

	logerror("i82371ab_pci_usb_w, offset = %02x, data = %08x, mem_mask = %08x\n", offset, data, mem_mask);

	switch (offset)
	{
	case 0x04:
		COMBINE_DATA(&i82371ab->regs[2][offset]);

		/* clear reserved bits */
		i82371ab->regs[2][offset] &= 0x00000005;

		/* set new status */
		i82371ab->regs[2][offset] |= 0x02800000;

		break;
	}
}

static UINT32 i82371ab_pci_acpi_r(device_t *busdevice, device_t *device, int offset, UINT32 mem_mask)
{
	i82371ab_state *i82371ab = get_safe_token(device);
	UINT32 result = i82371ab->regs[3][offset];

	logerror("i82371ab_pci_acpi_r, offset = %02x, mem_mask = %08x\n", offset, mem_mask);

	return result;
}

static void i82371ab_pci_acpi_w(device_t *busdevice, device_t *device, int offset, UINT32 data, UINT32 mem_mask)
{
	i82371ab_state *i82371ab = get_safe_token(device);

	logerror("i82371ab_pci_acpi_w, offset = %02x, data = %08x, mem_mask = %08x\n", offset, data, mem_mask);

	switch (offset)
	{
	case 0x04:
		COMBINE_DATA(&i82371ab->regs[3][offset]);

		/* clear reserved bits */
		i82371ab->regs[3][offset] &= 0x00000005;

		/* set new status */
		i82371ab->regs[3][offset] |= 0x02800000;

		break;
	}
}

UINT32 i82371ab_pci_read(device_t *busdevice, device_t *device, int function, int offset, UINT32 mem_mask)
{
	switch (function)
	{
	case 0: return i82371ab_pci_isa_r(busdevice, device, offset, mem_mask);
	case 1: return i82371ab_pci_ide_r(busdevice, device, offset, mem_mask);
	case 2: return i82371ab_pci_usb_r(busdevice, device, offset, mem_mask);
	case 3: return i82371ab_pci_acpi_r(busdevice, device, offset, mem_mask);
	}

	logerror("i82371ab_pci_read: read from undefined function %d\n", function);

	return 0;
}

void i82371ab_pci_write(device_t *busdevice, device_t *device, int function, int offset, UINT32 data, UINT32 mem_mask)
{
	switch (function)
	{
	case 0: i82371ab_pci_isa_w(busdevice, device, offset, data, mem_mask); break;
	case 1: i82371ab_pci_ide_w(busdevice, device, offset, data, mem_mask); break;
	case 2: i82371ab_pci_usb_w(busdevice, device, offset, data, mem_mask); break;
	case 3: i82371ab_pci_acpi_w(busdevice, device, offset, data, mem_mask); break;
	}
}


/*****************************************************************************
    DEVICE INTERFACE
*****************************************************************************/

static DEVICE_START( i82371ab )
{
	i82371ab_state *i82371ab = get_safe_token(device);

	/* setup save states */
	device->save_item(NAME(i82371ab->regs));
}

static DEVICE_RESET( i82371ab )
{
	i82371ab_state *i82371ab = get_safe_token(device);

	/* isa */
	i82371ab->regs[0][0x00] = 0x71108086;
	i82371ab->regs[0][0x04] = 0x00000000;
	i82371ab->regs[0][0x08] = 0x06010000;
	i82371ab->regs[0][0x0c] = 0x00800000;

	/* ide */
	i82371ab->regs[1][0x00] = 0x71118086;
	i82371ab->regs[1][0x04] = 0x02800000;
	i82371ab->regs[1][0x08] = 0x01018000;
	i82371ab->regs[1][0x0c] = 0x00000000;

	/* usb */
	i82371ab->regs[2][0x00] = 0x71128086;
	i82371ab->regs[2][0x04] = 0x02800000;
	i82371ab->regs[2][0x08] = 0x0c030000;
	i82371ab->regs[2][0x0c] = 0x00000000;

	/* acpi */
	i82371ab->regs[3][0x00] = 0x71138086;
	i82371ab->regs[3][0x04] = 0x02800000;
	i82371ab->regs[3][0x08] = 0x06800000;
	i82371ab->regs[3][0x0c] = 0x02800000;
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

DEFINE_LEGACY_DEVICE(I82371AB, i82371ab);
