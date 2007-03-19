/***************************************************************************

	machine/mpc105.h

	Motorola MPC105 PCI bridge

***************************************************************************/

#include "mpc105.h"
#include "machine/pci.h"

#define LOG_MPC105		1

struct mpc105_info
{
	int bank_base;
	UINT8 bank_enable;
	UINT32 bank_registers[8];
};


static struct mpc105_info *mpc105;


static void mpc105_update_memory(void)
{
	int cpunum, bank;
	offs_t begin, end;

	if (LOG_MPC105)
		logerror("mpc105_update_memory(): Updating memory (bank enable=0x%02X)\n", mpc105->bank_enable);

	if (mpc105->bank_base > 0)
	{
		for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
		{
			/* first clear everything out */
			memory_install_read64_handler(cpunum, ADDRESS_SPACE_PROGRAM, 0x00000000, 0x3FFFFFFF, 0, 0, MRA64_NOP);
			memory_install_write64_handler(cpunum, ADDRESS_SPACE_PROGRAM, 0x00000000, 0x3FFFFFFF, 0, 0, MWA64_NOP);
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

			end = MIN(end, begin + mess_ram_size - 1);

			if ((begin + 0x100000) <= end)
			{
				if (LOG_MPC105)
					logerror("\tbank #%d [%02d]: 0x%08X - 0x%08X [%p-%p]\n", bank, bank + mpc105->bank_base, begin, end, mess_ram, mess_ram + (end - begin));

				if (mpc105->bank_base > 0)
				{
					for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
					{
						memory_install_read64_handler(cpunum, ADDRESS_SPACE_PROGRAM, begin, end,
							0, 0, (read64_handler) (bank + mpc105->bank_base));
						memory_install_write64_handler(cpunum, ADDRESS_SPACE_PROGRAM, begin, end,
							0, 0, (write64_handler) (bank + mpc105->bank_base));
					}
					memory_set_bankptr(bank + mpc105->bank_base, mess_ram);
				}
			}
		}
	}
}



static UINT32 mpc105_pci_read(int function, int offset, UINT32 mem_mask)
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
			switch(cpu_getactivecpu())
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



static void mpc105_pci_write(int function, int offset, UINT32 data, UINT32 mem_mask)
{
	int i;

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
				mpc105_update_memory();
			}
			break;

		case 0xA0:	/* memory enable */
			if (mpc105->bank_enable != (UINT8) data)
			{
				mpc105->bank_enable = (UINT8) data;
				mpc105_update_memory();
			}
			break;

		case 0xF0:	/* memory control configuration 1 */
		case 0xF4:	/* memory control configuration 2 */
		case 0xF8:	/* memory control configuration 3 */
		case 0xFC:	/* memory control configuration 4 */
			break;

		case 0xA8:	/* processor interface configuration 1 */
			//fatalerror("mpc105_pci_write(): Unexpected PCI write 0x%02X <-- 0x%08X\n", offset, data);
			break;
	}
}



static const struct pci_device_info mpc105_callbacks =
{
	mpc105_pci_read,
	mpc105_pci_write
};



void mpc105_init(int bank_base)
{
	/* setup PCI */
	pci_init();
	pci_add_device(0, 0, &mpc105_callbacks);

	mpc105 = (struct mpc105_info *) auto_malloc(sizeof(*mpc105));
	memset(mpc105, '\0', sizeof(*mpc105));
	mpc105->bank_base = bank_base;
}
