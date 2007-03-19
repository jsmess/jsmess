/***************************************************************************

	machine/i82439tx.c

	Intel 82439TX PCI Bridge

***************************************************************************/

#include "machine/i82439tx.h"
#include "machine/pci.h"

#define BANK_C0000_R	12
#define BANK_C4000_R	13
#define BANK_C8000_R	14
#define BANK_CC000_R	15
#define BANK_D0000_R	16
#define BANK_D4000_R	17
#define BANK_D8000_R	18
#define BANK_DC000_R	19
#define BANK_E0000_R	20
#define BANK_E4000_R	21
#define BANK_E8000_R	22
#define BANK_EC000_R	23
#define BANK_F0000_R	24

struct intel82439tx_info
{
	UINT32 regs[8];
	UINT32 bios_ram[0x40000 / 4];
};


static struct intel82439tx_info *i82439tx;


static WRITE32_HANDLER(bank_c0000_w)	{ COMBINE_DATA(&i82439tx->bios_ram[offset + 0x00000 / 4]); }
static WRITE32_HANDLER(bank_c4000_w)	{ COMBINE_DATA(&i82439tx->bios_ram[offset + 0x04000 / 4]); }
static WRITE32_HANDLER(bank_c8000_w)	{ COMBINE_DATA(&i82439tx->bios_ram[offset + 0x08000 / 4]); }
static WRITE32_HANDLER(bank_cc000_w)	{ COMBINE_DATA(&i82439tx->bios_ram[offset + 0x0c000 / 4]); }
static WRITE32_HANDLER(bank_d0000_w)	{ COMBINE_DATA(&i82439tx->bios_ram[offset + 0x10000 / 4]); }
static WRITE32_HANDLER(bank_d4000_w)	{ COMBINE_DATA(&i82439tx->bios_ram[offset + 0x14000 / 4]); }
static WRITE32_HANDLER(bank_d8000_w)	{ COMBINE_DATA(&i82439tx->bios_ram[offset + 0x18000 / 4]); }
static WRITE32_HANDLER(bank_dc000_w)	{ COMBINE_DATA(&i82439tx->bios_ram[offset + 0x1c000 / 4]); }
static WRITE32_HANDLER(bank_e0000_w)	{ COMBINE_DATA(&i82439tx->bios_ram[offset + 0x20000 / 4]); }
static WRITE32_HANDLER(bank_e4000_w)	{ COMBINE_DATA(&i82439tx->bios_ram[offset + 0x24000 / 4]); }
static WRITE32_HANDLER(bank_e8000_w)	{ COMBINE_DATA(&i82439tx->bios_ram[offset + 0x28000 / 4]); }
static WRITE32_HANDLER(bank_ec000_w)	{ COMBINE_DATA(&i82439tx->bios_ram[offset + 0x2c000 / 4]); }
static WRITE32_HANDLER(bank_f0000_w)	{ COMBINE_DATA(&i82439tx->bios_ram[offset + 0x30000 / 4]); }

static UINT32 intel82439tx_pci_read(int function, int offset, UINT32 mem_mask)
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
		case 0xE0:
			result = i82439tx->regs[(offset - 0x50) / 4];
			break;

		default:
			fatalerror("intel82439tx_pci_read(): Unexpected PCI read 0x%02X\n", offset);
			break;
	}
	return result;
}



static void intel82439tx_configure_memory(UINT8 val, offs_t begin, offs_t end, int read_bank, write32_handler wh)
{
	memory_install_read_handler(0, ADDRESS_SPACE_PROGRAM, begin, end, 0, 0, read_bank);
	if (val & 0x01)
		memory_set_bankptr(read_bank, i82439tx->bios_ram + (begin - 0xC0000) / 4);
	else
		memory_set_bankptr(read_bank, memory_region(REGION_USER1) + (begin - 0xC0000));

	if (val & 0x02)
		memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, begin, end, 0, 0, wh);
	else
		memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, begin, end, 0, 0, MWA32_ROM);
}



static void intel82439tx_pci_write(int function, int offset, UINT32 data, UINT32 mem_mask)
{
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
		case 0xE0:
			switch(offset)
			{
				case 0x58:
					if (!(mem_mask & 0x0000f000))
						intel82439tx_configure_memory(data >> 12, 0xF0000, 0xFFFFF, BANK_F0000_R, bank_f0000_w);
					if (!(mem_mask & 0x000f0000))
						intel82439tx_configure_memory(data >> 16, 0xC0000, 0xC3FFF, BANK_C0000_R, bank_c0000_w);
					if (!(mem_mask & 0x00f00000))
						intel82439tx_configure_memory(data >> 20, 0xC4000, 0xC7FFF, BANK_C4000_R, bank_c4000_w);
					if (!(mem_mask & 0x0f000000))
						intel82439tx_configure_memory(data >> 24, 0xC8000, 0xCCFFF, BANK_C8000_R, bank_c8000_w);
					if (!(mem_mask & 0xf0000000))
						intel82439tx_configure_memory(data >> 28, 0xCC000, 0xCFFFF, BANK_CC000_R, bank_cc000_w);
					break;

				case 0x5C:
					if (!(mem_mask & 0x0000000f))
						intel82439tx_configure_memory(data >>  0, 0xD0000, 0xD3FFF, BANK_D0000_R, bank_d0000_w);
					if (!(mem_mask & 0x000000f0))
						intel82439tx_configure_memory(data >>  4, 0xD4000, 0xD7FFF, BANK_D4000_R, bank_d4000_w);
					if (!(mem_mask & 0x00000f00))
						intel82439tx_configure_memory(data >>  8, 0xD8000, 0xDBFFF, BANK_D8000_R, bank_d8000_w);
					if (!(mem_mask & 0x0000f000))
						intel82439tx_configure_memory(data >> 12, 0xDC000, 0xDFFFF, BANK_DC000_R, bank_dc000_w);
					if (!(mem_mask & 0x000f0000))
						intel82439tx_configure_memory(data >> 16, 0xE0000, 0xE3FFF, BANK_E0000_R, bank_e0000_w);
					if (!(mem_mask & 0x00f00000))
						intel82439tx_configure_memory(data >> 20, 0xE4000, 0xE7FFF, BANK_E4000_R, bank_e4000_w);
					if (!(mem_mask & 0x0f000000))
						intel82439tx_configure_memory(data >> 24, 0xE8000, 0xECFFF, BANK_E8000_R, bank_e8000_w);
					if (!(mem_mask & 0xf0000000))
						intel82439tx_configure_memory(data >> 28, 0xEC000, 0xEFFFF, BANK_EC000_R, bank_ec000_w);
					break;
			}

			COMBINE_DATA(&i82439tx->regs[(offset - 0x50) / 4]);
			break;

		default:
			fatalerror("intel82439tx_pci_write(): Unexpected PCI write 0x%02X <-- 0x%08X\n", offset, data);
			break;
	}

	/* hack to compensate for weird issue (maybe the Pentium core needs to support caching? */
	if ((cpunum_get_reg(0, REG_PC) == 0xFCB01) && !strcmp(Machine->gamedrv->name, "at586"))
	{
		memory_region(REGION_USER1)[0x3CB01] = 0xF3;
		memory_region(REGION_USER1)[0x3CB02] = 0xA4;
		memory_region(REGION_USER1)[0x3CB03] = 0x58;
		memory_region(REGION_USER1)[0x3CB04] = 0xEE;
	}
}



static const struct pci_device_info intel82439tx_callbacks =
{
	intel82439tx_pci_read,
	intel82439tx_pci_write
};



void intel82439tx_init(void)
{
	/* setup PCI */
	pci_init();
	pci_add_device(0, 0, &intel82439tx_callbacks);

	i82439tx = auto_malloc(sizeof(*i82439tx));
	memset(i82439tx, 0, sizeof(*i82439tx));
	i82439tx->regs[0x00] = 0x14020000;
	i82439tx->regs[0x01] = 0x01520000;
	i82439tx->regs[0x02] = 0x00000000;
	i82439tx->regs[0x04] = 0x02020202;
	i82439tx->regs[0x05] = 0x00000002;

	intel82439tx_configure_memory(0, 0xF0000, 0xFFFFF, BANK_F0000_R, bank_f0000_w);
	intel82439tx_configure_memory(0, 0xC0000, 0xC3FFF, BANK_C0000_R, bank_c0000_w);
	intel82439tx_configure_memory(0, 0xC4000, 0xC7FFF, BANK_C4000_R, bank_c4000_w);
	intel82439tx_configure_memory(0, 0xC8000, 0xCCFFF, BANK_C8000_R, bank_c8000_w);
	intel82439tx_configure_memory(0, 0xCC000, 0xCFFFF, BANK_CC000_R, bank_cc000_w);
	intel82439tx_configure_memory(0, 0xD0000, 0xD3FFF, BANK_D0000_R, bank_d0000_w);
	intel82439tx_configure_memory(0, 0xD4000, 0xD7FFF, BANK_D4000_R, bank_d4000_w);
	intel82439tx_configure_memory(0, 0xD8000, 0xDBFFF, BANK_D8000_R, bank_d8000_w);
	intel82439tx_configure_memory(0, 0xDC000, 0xDFFFF, BANK_DC000_R, bank_dc000_w);
	intel82439tx_configure_memory(0, 0xE0000, 0xE3FFF, BANK_E0000_R, bank_e0000_w);
	intel82439tx_configure_memory(0, 0xE4000, 0xE7FFF, BANK_E4000_R, bank_e4000_w);
	intel82439tx_configure_memory(0, 0xE8000, 0xECFFF, BANK_E8000_R, bank_e8000_w);
	intel82439tx_configure_memory(0, 0xEC000, 0xEFFFF, BANK_EC000_R, bank_ec000_w);
}

