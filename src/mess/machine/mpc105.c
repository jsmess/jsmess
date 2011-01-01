/***************************************************************************

    machine/mpc105.h

    Motorola MPC105 PCI bridge

***************************************************************************/

#include "emu.h"
#include "mpc105.h"
#include "machine/pci.h"
#include "devices/messram.h"

#define LOG_MPC105		1

struct mpc105_info
{
	int bank_base;
	UINT8 bank_enable;
	UINT32 bank_registers[8];
};


static struct mpc105_info *mpc105;


static void mpc105_update_memory(running_machine *machine)
{
	const cpu_device *cpu = NULL;
	int bank;
	offs_t begin, end;
	char bank_str[10];

	if (LOG_MPC105)
		logerror("mpc105_update_memory(machine): Updating memory (bank enable=0x%02X)\n", mpc105->bank_enable);

	if (mpc105->bank_base > 0)
	{
		/* TODO: Fix me properly! changing all cpus???? */
		for (bool gotone = machine->config->m_devicelist.first(cpu); gotone; gotone = cpu->typenext())
		{
			address_space *space = cpu_get_address_space( (cpu_device *)cpu, ADDRESS_SPACE_PROGRAM );

			/* first clear everything out */
			memory_nop_read(space, 0x00000000, 0x3FFFFFFF, 0, 0);
			memory_nop_read(space, 0x00000000, 0x3FFFFFFF, 0, 0);
		}
	}

	for (bank = 0; bank < MPC105_MEMORYBANK_COUNT; bank++)
	{
		if (mpc105->bank_enable & (1 << bank))
		{
			begin = (((mpc105->bank_registers[(bank / 4) + 0] >> (bank % 4) * 8)) & 0xFF) << 20
				|	(((mpc105->bank_registers[(bank / 4) + 2] >> (bank % 4) * 8)) & 0x03) << 28;

			end   = (((mpc105->bank_registers[(bank / 4) + 4] >> (bank % 4) * 8)) & 0xFF) << 20
				|	(((mpc105->bank_registers[(bank / 4) + 6] >> (bank % 4) * 8)) & 0x03) << 28
				| 0x000FFFFF;

			end = MIN(end, begin + messram_get_size(machine->device("messram")) - 1);

			if ((begin + 0x100000) <= end)
			{
				if (LOG_MPC105)
					logerror("\tbank #%d [%02d]: 0x%08X - 0x%08X [%p-%p]\n", bank, bank + mpc105->bank_base, begin, end, messram_get_ptr(machine->device("messram")), messram_get_ptr(machine->device("messram")) + (end - begin));

				if (mpc105->bank_base > 0)
				{
					/* TODO: Fix me properly! changing all cpus??? */
					for (bool gotone = machine->config->m_devicelist.first(cpu); gotone; gotone = cpu->typenext())
					{
						address_space *space = cpu_get_address_space( (cpu_device *)cpu, ADDRESS_SPACE_PROGRAM );

						memory_install_read64_handler(space, begin, end,
							0, 0, (read64_space_func) (FPTR)(bank + mpc105->bank_base));
						memory_install_write64_handler(space, begin, end,
							0, 0, (write64_space_func) (FPTR)(bank + mpc105->bank_base));
					}
					sprintf(bank_str,"bank%d",bank + mpc105->bank_base);
					memory_set_bankptr(machine, bank_str, messram_get_ptr(machine->device("messram")));
				}
			}
		}
	}
}



UINT32 mpc105_pci_read(device_t *busdevice, device_t *device, int function, int offset, UINT32 mem_mask)
{
	UINT32 result;

	if (function != 0)
		return 0;

	switch(offset)
	{
		case 0x00:	/* vendor/device ID */
			result = 0x00011057;
			break;

		case 0x08:
			result = 0x06000000;
			break;

		case 0x80:	/* memory starting address 1 */
		case 0x84:	/* memory starting address 2 */
		case 0x88:	/* extended memory starting address 1 */
		case 0x8C:	/* extended memory starting address 2 */
		case 0x90:	/* memory ending address 1 */
		case 0x94:	/* memory ending address 2 */
		case 0x98:	/* extended memory ending address 1 */
		case 0x9C:	/* extended memory ending address 2 */
			result = mpc105->bank_registers[(offset - 0x80) / 4];
			break;

		case 0xA0:	/* memory enable */
			result = mpc105->bank_enable;
			break;

		case 0xA8:	/* processor interface configuration 1 */
			/* TODO: Fix me! */
			switch(/*cpu_getactivecpu()*/0)
			{
				case 0:
					result = 0xFF000010;
					break;

				case 1:
					result = 0xFF008010;
					break;

				default:
					fatalerror("Unknown CPU");
					break;
			}
			break;

		case 0xAC:	/* processor interface configuration 1 */
			result = 0x000C060C;
			break;

		case 0xF0:	/* memory control configuration 1 */
			result = 0xFF020000;
			break;
		case 0xF4:	/* memory control configuration 2 */
			result = 0x00000003;
			break;
		case 0xF8:	/* memory control configuration 3 */
			result = 0x00000000;
			break;
		case 0xFC:	/* memory control configuration 4 */
			result = 0x00100000;
			break;

		default:
			result = 0;
			break;
	}
	return result;
}



void mpc105_pci_write(device_t *busdevice, device_t *device, int function, int offset, UINT32 data, UINT32 mem_mask)
{
	int i;
	running_machine *machine = busdevice->machine;

	if (function != 0)
		return;

	switch(offset)
	{
		case 0x80:	/* memory starting address 1 */
		case 0x84:	/* memory starting address 2 */
		case 0x88:	/* extended memory starting address 1 */
		case 0x8C:	/* extended memory starting address 2 */
		case 0x90:	/* memory ending address 1 */
		case 0x94:	/* memory ending address 2 */
		case 0x98:	/* extended memory ending address 1 */
		case 0x9C:	/* extended memory ending address 2 */
			i = (offset - 0x80) / 4;
			if (mpc105->bank_registers[i] != data)
			{
				mpc105->bank_registers[i] = data;
				mpc105_update_memory(machine);
			}
			break;

		case 0xA0:	/* memory enable */
			if (mpc105->bank_enable != (UINT8) data)
			{
				mpc105->bank_enable = (UINT8) data;
				mpc105_update_memory(machine);
			}
			break;

		case 0xF0:	/* memory control configuration 1 */
		case 0xF4:	/* memory control configuration 2 */
		case 0xF8:	/* memory control configuration 3 */
		case 0xFC:	/* memory control configuration 4 */
			break;

		case 0xA8:	/* processor interface configuration 1 */
			//fatalerror("mpc105_pci_write(): Unexpected PCI write 0x%02X <-- 0x%08X", offset, data);
			break;
	}
}



void mpc105_init(running_machine *machine, int bank_base)
{
	/* setup PCI */
	mpc105 = auto_alloc(machine, struct mpc105_info);
	memset(mpc105, '\0', sizeof(*mpc105));
	mpc105->bank_base = bank_base;
}
