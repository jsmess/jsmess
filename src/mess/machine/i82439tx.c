/***************************************************************************

    machine/i82439tx.c

    Intel 82439TX PCI Bridge

***************************************************************************/

#include "driver.h"
#include "machine/i82439tx.h"
#include "machine/pci.h"


struct intel82439tx_info
{
	UINT32 regs[8];
	UINT32 bios_ram[0x40000 / 4];
};

static struct intel82439tx_info *i82439tx;


UINT32 intel82439tx_pci_read(const device_config *busdevice, const device_config *device, int function, int offset, UINT32 mem_mask)
{
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
			fatalerror("intel82439tx_pci_read(): Unexpected PCI read 0x%02X\n", offset);
			break;
	}
	return result;
}



static void intel82439tx_configure_memory(running_machine *machine, UINT8 val, offs_t begin, offs_t end)
{
	const address_space *space = cpu_get_address_space(machine->firstcpu, ADDRESS_SPACE_PROGRAM);

	char read_bank[10];
	sprintf(read_bank, "%05x_r", begin);

	memory_install_read_bank(space, begin, end, 0, 0, read_bank);

	if (BIT(val, 0))
		memory_set_bankptr(machine, read_bank, i82439tx->bios_ram + (begin - 0xc0000) / 4);
	else
		memory_set_bankptr(machine, read_bank, memory_region(machine, "user1") + (begin - 0xc0000));

	/* write enabled? */
	if (BIT(val, 1))
	{
		char write_bank[10];
		sprintf(write_bank, "%05x_w", begin);

		memory_install_write_bank(space, begin, end, 0, 0, write_bank);
		memory_set_bankptr(machine, write_bank, i82439tx->bios_ram + (begin - 0xc0000) / 4);
	}
	else
		memory_nop_write(space, begin, end, 0, 0);
}



void intel82439tx_pci_write(const device_config *busdevice, const device_config *device, int function, int offset, UINT32 data, UINT32 mem_mask)
{
	running_machine *machine = busdevice->machine;

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
						intel82439tx_configure_memory(machine, data >> 12, 0xF0000, 0xFFFFF);
					if ((mem_mask & 0x000f0000))
						intel82439tx_configure_memory(machine, data >> 16, 0xC0000, 0xC3FFF);
					if ((mem_mask & 0x00f00000))
						intel82439tx_configure_memory(machine, data >> 20, 0xC4000, 0xC7FFF);
					if ((mem_mask & 0x0f000000))
						intel82439tx_configure_memory(machine, data >> 24, 0xC8000, 0xCCFFF);
					if ((mem_mask & 0xf0000000))
						intel82439tx_configure_memory(machine, data >> 28, 0xCC000, 0xCFFFF);
					break;

				case 0x5C:
					if ((mem_mask & 0x0000000f))
						intel82439tx_configure_memory(machine, data >>  0, 0xD0000, 0xD3FFF);
					if ((mem_mask & 0x000000f0))
						intel82439tx_configure_memory(machine, data >>  4, 0xD4000, 0xD7FFF);
					if ((mem_mask & 0x00000f00))
						intel82439tx_configure_memory(machine, data >>  8, 0xD8000, 0xDBFFF);
					if ((mem_mask & 0x0000f000))
						intel82439tx_configure_memory(machine, data >> 12, 0xDC000, 0xDFFFF);
					if ((mem_mask & 0x000f0000))
						intel82439tx_configure_memory(machine, data >> 16, 0xE0000, 0xE3FFF);
					if ((mem_mask & 0x00f00000))
						intel82439tx_configure_memory(machine, data >> 20, 0xE4000, 0xE7FFF);
					if ((mem_mask & 0x0f000000))
						intel82439tx_configure_memory(machine, data >> 24, 0xE8000, 0xECFFF);
					if ((mem_mask & 0xf0000000))
						intel82439tx_configure_memory(machine, data >> 28, 0xEC000, 0xEFFFF);
					break;
			}

			COMBINE_DATA(&i82439tx->regs[(offset - 0x50) / 4]);
			break;

		default:
			fatalerror("intel82439tx_pci_write(): Unexpected PCI write 0x%02X <-- 0x%08X\n", offset, data);
			break;
	}
}


void intel82439tx_init(running_machine *machine)
{
	i82439tx = auto_alloc(machine, struct intel82439tx_info);
}

void intel82439tx_reset(running_machine *machine)
{
	/* setup PCI */
	memset(i82439tx, 0, sizeof(*i82439tx));
	i82439tx->regs[0x00] = 0x14020000;
	i82439tx->regs[0x01] = 0x01520000;
	i82439tx->regs[0x02] = 0x00000000;
	i82439tx->regs[0x04] = 0x02020202;
	i82439tx->regs[0x05] = 0x00000002;

	intel82439tx_configure_memory(machine, 0, 0xF0000, 0xFFFFF);
	intel82439tx_configure_memory(machine, 0, 0xC0000, 0xC3FFF);
	intel82439tx_configure_memory(machine, 0, 0xC4000, 0xC7FFF);
	intel82439tx_configure_memory(machine, 0, 0xC8000, 0xCCFFF);
	intel82439tx_configure_memory(machine, 0, 0xCC000, 0xCFFFF);
	intel82439tx_configure_memory(machine, 0, 0xD0000, 0xD3FFF);
	intel82439tx_configure_memory(machine, 0, 0xD4000, 0xD7FFF);
	intel82439tx_configure_memory(machine, 0, 0xD8000, 0xDBFFF);
	intel82439tx_configure_memory(machine, 0, 0xDC000, 0xDFFFF);
	intel82439tx_configure_memory(machine, 0, 0xE0000, 0xE3FFF);
	intel82439tx_configure_memory(machine, 0, 0xE4000, 0xE7FFF);
	intel82439tx_configure_memory(machine, 0, 0xE8000, 0xECFFF);
	intel82439tx_configure_memory(machine, 0, 0xEC000, 0xEFFFF);
}
