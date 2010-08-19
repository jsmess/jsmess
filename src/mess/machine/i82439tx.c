/***************************************************************************

    Intel 82439TX System Controller (MTXC)

***************************************************************************/

#include "emu.h"
#include "i82439tx.h"


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _i82439tx_state i82439tx_state;
struct _i82439tx_state
{
	address_space *space;
	UINT8 *rom;

	UINT32 regs[8];
	UINT32 bios_ram[0x40000 / 4];
};


/*****************************************************************************
    INLINE FUNCTIONS
*****************************************************************************/

INLINE i82439tx_state *get_safe_token(running_device *device)
{
	assert(device != NULL);
	assert(device->type() == I82439TX);

	return (i82439tx_state *)downcast<legacy_device_base *>(device)->token();
}


/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

static void i82439tx_configure_memory(running_device *device, UINT8 val, offs_t begin, offs_t end)
{
	i82439tx_state *i82439tx = get_safe_token(device);

	switch (val & 0x03)
	{
	case 0:
		memory_install_rom(i82439tx->space, begin, end, 0, 0, i82439tx->rom + (begin - 0xc0000));
		memory_nop_write(i82439tx->space, begin, end, 0, 0);
		break;
	case 1:
		memory_install_rom(i82439tx->space, begin, end, 0, 0, i82439tx->bios_ram + (begin - 0xc0000) / 4);
		memory_nop_write(i82439tx->space, begin, end, 0, 0);
		break;
	case 2:
		memory_install_rom(i82439tx->space, begin, end, 0, 0, i82439tx->rom + (begin - 0xc0000));
		memory_install_writeonly(i82439tx->space, begin, end, 0, 0, i82439tx->bios_ram + (begin - 0xc0000) / 4);
		break;
	case 3:
		memory_install_ram(i82439tx->space, begin, end, 0, 0, i82439tx->bios_ram + (begin - 0xc0000) / 4);
		break;
	}
}


/***************************************************************************
    PCI INTERFACE
***************************************************************************/

UINT32 i82439tx_pci_read(running_device *busdevice, running_device *device, int function, int offset, UINT32 mem_mask)
{
	i82439tx_state *i82439tx = get_safe_token(device);
	UINT32 result = 0;

	if (function != 0)
		return 0;

	switch(offset)
	{
		case 0x00:	/* vendor/device ID */
			result = 0x71008086;
			break;

		case 0x08:	/* revision identification register and class code register*/
			result = 0x00000001;
			break;

		case 0x04:	/* PCI command register */
		case 0x0C:
		case 0x10:	/* reserved */
		case 0x14:	/* reserved */
		case 0x18:	/* reserved */
		case 0x1C:	/* reserved */
		case 0x20:	/* reserved */
		case 0x24:	/* reserved */
		case 0x28:	/* reserved */
		case 0x2C:	/* reserved */
		case 0x30:	/* reserved */
		case 0x34:	/* reserved */
		case 0x38:	/* reserved */
		case 0x3C:	/* reserved */
		case 0x4C:	/* reserved */
		case 0x50:
		case 0x54:
		case 0x58:
		case 0x5C:
		case 0x60:
		case 0x64:
		case 0x68:
		case 0x78:
		case 0xC0:
		case 0xE0:
			result = i82439tx->regs[(offset - 0x50) / 4];
			break;

		default:
			fatalerror("i82439tx_pci_read(): Unexpected PCI read 0x%02X", offset);
			break;
	}
	return result;
}

void i82439tx_pci_write(running_device *busdevice, running_device *device, int function, int offset, UINT32 data, UINT32 mem_mask)
{
	i82439tx_state *i82439tx = get_safe_token(device);

	if (function != 0)
		return;

	switch(offset)
	{
		case 0x00:	/* vendor/device ID */
		case 0x10:	/* reserved */
		case 0x14:	/* reserved */
		case 0x18:	/* reserved */
		case 0x1C:	/* reserved */
		case 0x20:	/* reserved */
		case 0x24:	/* reserved */
		case 0x28:	/* reserved */
		case 0x2C:	/* reserved */
		case 0x30:	/* reserved */
		case 0x3C:	/* reserved */
		case 0x4C:	/* reserved */
			/* read only */
			break;

		case 0x04:	/* PCI command register */
		case 0x0C:
		case 0x50:
		case 0x54:
		case 0x58:
		case 0x5C:
		case 0x60:
		case 0x64:
		case 0x68:
		case 0x70:
		case 0x74:
		case 0x78:
		case 0xC0:
		case 0xE0:
			switch(offset)
			{
				case 0x58:
					if ((mem_mask & 0x0000f000))
						i82439tx_configure_memory(device, data >> 12, 0xf0000, 0xfffff);
					if ((mem_mask & 0x000f0000))
						i82439tx_configure_memory(device, data >> 16, 0xc0000, 0xc3fff);
					if ((mem_mask & 0x00f00000))
						i82439tx_configure_memory(device, data >> 20, 0xc4000, 0xc7fff);
					if ((mem_mask & 0x0f000000))
						i82439tx_configure_memory(device, data >> 24, 0xc8000, 0xccfff);
					if ((mem_mask & 0xf0000000))
						i82439tx_configure_memory(device, data >> 28, 0xcc000, 0xcffff);
					break;

				case 0x5C:
					if ((mem_mask & 0x0000000f))
						i82439tx_configure_memory(device, data >>  0, 0xd0000, 0xd3fff);
					if ((mem_mask & 0x000000f0))
						i82439tx_configure_memory(device, data >>  4, 0xd4000, 0xd7fff);
					if ((mem_mask & 0x00000f00))
						i82439tx_configure_memory(device, data >>  8, 0xd8000, 0xdbfff);
					if ((mem_mask & 0x0000f000))
						i82439tx_configure_memory(device, data >> 12, 0xdc000, 0xdffff);
					if ((mem_mask & 0x000f0000))
						i82439tx_configure_memory(device, data >> 16, 0xe0000, 0xe3fff);
					if ((mem_mask & 0x00f00000))
						i82439tx_configure_memory(device, data >> 20, 0xe4000, 0xe7fff);
					if ((mem_mask & 0x0f000000))
						i82439tx_configure_memory(device, data >> 24, 0xe8000, 0xecfff);
					if ((mem_mask & 0xf0000000))
						i82439tx_configure_memory(device, data >> 28, 0xec000, 0xeffff);
					break;
			}

			COMBINE_DATA(&i82439tx->regs[(offset - 0x50) / 4]);
			break;

		default:
			fatalerror("i82439tx_pci_write(): Unexpected PCI write 0x%02X <-- 0x%08X", offset, data);
			break;
	}
}


/*****************************************************************************
    DEVICE INTERFACE
*****************************************************************************/

static DEVICE_START( i82439tx )
{
	i82439tx_state *i82439tx = get_safe_token(device);
	i82439tx_config *config = (i82439tx_config *)downcast<const legacy_device_config_base &>(device->baseconfig()).inline_config();

	/* get address space we are working on */
	running_device *cpu = device->machine->device(config->cputag);
	assert(cpu != NULL);

	i82439tx->space = cpu_get_address_space(cpu, ADDRESS_SPACE_PROGRAM);

	/* get rom region */
	i82439tx->rom = memory_region(device->machine, config->rom_region);

	/* setup save states */
	state_save_register_device_item_array(device, 0, i82439tx->regs);
	state_save_register_device_item_array(device, 0, i82439tx->bios_ram);
}

static DEVICE_RESET( i82439tx )
{
	i82439tx_state *i82439tx = get_safe_token(device);

	/* setup initial values */
	i82439tx->regs[0x00] = 0x14020000;
	i82439tx->regs[0x01] = 0x01520000;
	i82439tx->regs[0x02] = 0x00000000;
	i82439tx->regs[0x03] = 0x00000000;
	i82439tx->regs[0x04] = 0x02020202;
	i82439tx->regs[0x05] = 0x00000002;
	i82439tx->regs[0x06] = 0x00000000;
	i82439tx->regs[0x07] = 0x00000000;

	/* configure initial memory state */
	i82439tx_configure_memory(device, 0, 0xf0000, 0xfffff);
	i82439tx_configure_memory(device, 0, 0xc0000, 0xc3fff);
	i82439tx_configure_memory(device, 0, 0xc4000, 0xc7fff);
	i82439tx_configure_memory(device, 0, 0xc8000, 0xccfff);
	i82439tx_configure_memory(device, 0, 0xcc000, 0xcffff);
	i82439tx_configure_memory(device, 0, 0xd0000, 0xd3fff);
	i82439tx_configure_memory(device, 0, 0xd4000, 0xd7fff);
	i82439tx_configure_memory(device, 0, 0xd8000, 0xdbfff);
	i82439tx_configure_memory(device, 0, 0xdc000, 0xdffff);
	i82439tx_configure_memory(device, 0, 0xe0000, 0xe3fff);
	i82439tx_configure_memory(device, 0, 0xe4000, 0xe7fff);
	i82439tx_configure_memory(device, 0, 0xe8000, 0xecfff);
	i82439tx_configure_memory(device, 0, 0xec000, 0xeffff);
}


/***************************************************************************
    DEVICE GETINFO
***************************************************************************/

static const char DEVTEMPLATE_SOURCE[] = __FILE__;

#define DEVTEMPLATE_ID(p,s)				p##i82439tx##s
#define DEVTEMPLATE_FEATURES			DT_HAS_START | DT_HAS_RESET | DT_HAS_INLINE_CONFIG
#define DEVTEMPLATE_NAME				"Intel 82439TX"
#define DEVTEMPLATE_FAMILY				"North Bridge"
#define DEVTEMPLATE_CLASS				DEVICE_CLASS_OTHER
#define DEVTEMPLATE_VERSION				"1.0"
#define DEVTEMPLATE_CREDITS				"Copyright MESS Team"
#include "devtempl.h"

DEFINE_LEGACY_DEVICE(I82439TX, i82439tx);
