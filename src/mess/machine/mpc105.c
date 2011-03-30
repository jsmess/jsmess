/***************************************************************************

    machine/mpc105.h

    Motorola MPC105 PCI bridge

***************************************************************************/

#include "emu.h"
#include "mpc105.h"
#include "machine/pci.h"
#include "machine/ram.h"

#define LOG_MPC105		0

//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type MPC105 = mpc105_device_config::static_alloc_device_config;

//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  mpc105_device_config - constructor
//-------------------------------------------------

mpc105_device_config::mpc105_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
    : device_config(mconfig, static_alloc_device_config, "MPC105", tag, owner, clock)
{
}


//-------------------------------------------------
//  static_alloc_device_config - allocate a new
//  configuration object
//-------------------------------------------------

device_config *mpc105_device_config::static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
{
    return global_alloc(mpc105_device_config(mconfig, tag, owner, clock));
}


//-------------------------------------------------
//  alloc_device - allocate a new device object
//-------------------------------------------------

device_t *mpc105_device_config::alloc_device(running_machine &machine) const
{
    return auto_alloc(machine, mpc105_device(machine, *this));
}


//-------------------------------------------------
//  static_set_bank_base - configuration helper
//  to set the bank base
//-------------------------------------------------

void mpc105_device_config::static_set_bank_base(device_config *device, int bank_base)
{
	mpc105_device_config *mpc105 = downcast<mpc105_device_config *>(device);
	mpc105->m_bank_base = bank_base;
}

//-------------------------------------------------
//  static_set_cputag - configuration helper
//  to set the cpu tag
//-------------------------------------------------

void mpc105_device_config::static_set_cputag(device_config *device, const char *tag)
{
	mpc105_device_config *mpc105 = downcast<mpc105_device_config *>(device);
	mpc105->m_cputag = tag;
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  mpc105_device - constructor
//-------------------------------------------------

mpc105_device::mpc105_device(running_machine &_machine, const mpc105_device_config &config)
    : device_t(_machine, config),
      m_config(config),
	  m_maincpu(*owner(), config.m_cputag)	  
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void mpc105_device::device_start()
{
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void mpc105_device::device_reset()
{
	m_bank_base = m_config.m_bank_base;
	m_bank_enable = 0;
	memset(m_bank_registers,0,sizeof(m_bank_registers));
}


//-------------------------------------------------
//  update_memory - MMU update 
//-------------------------------------------------

void mpc105_device::update_memory()
{
	int bank;
	offs_t begin, end;
	char bank_str[10];

	if (LOG_MPC105)
		logerror("mpc105_update_memory(machine): Updating memory (bank enable=0x%02X)\n", m_bank_enable);

	if (m_bank_base > 0)
	{
		address_space *space = m_maincpu->memory().space(AS_PROGRAM);

		/* first clear everything out */
		space->nop_read(0x00000000, 0x3FFFFFFF);
		space->nop_read(0x00000000, 0x3FFFFFFF);
	}

	for (bank = 0; bank < MPC105_MEMORYBANK_COUNT; bank++)
	{
		if (m_bank_enable & (1 << bank))
		{
			begin = (((m_bank_registers[(bank / 4) + 0] >> (bank % 4) * 8)) & 0xFF) << 20
				|	(((m_bank_registers[(bank / 4) + 2] >> (bank % 4) * 8)) & 0x03) << 28;

			end   = (((m_bank_registers[(bank / 4) + 4] >> (bank % 4) * 8)) & 0xFF) << 20
				|	(((m_bank_registers[(bank / 4) + 6] >> (bank % 4) * 8)) & 0x03) << 28
				| 0x000FFFFF;

			end = MIN(end, begin + ram_get_size(m_machine.device(RAM_TAG)) - 1);

			if ((begin + 0x100000) <= end)
			{
				if (LOG_MPC105)
					logerror("\tbank #%d [%02d]: 0x%08X - 0x%08X [%p-%p]\n", bank, bank + m_bank_base, begin, end, ram_get_ptr(m_machine.device(RAM_TAG)), ram_get_ptr(m_machine.device(RAM_TAG)) + (end - begin));

				if (m_bank_base > 0)
				{
					address_space *space = m_maincpu->memory().space(AS_PROGRAM);

					space->install_legacy_read_handler(begin, end,
						0, 0, FUNC((read64_space_func)(FPTR)(bank + m_bank_base)));
					space->install_legacy_write_handler(begin, end,
						0, 0, FUNC((write64_space_func)(FPTR)(bank + m_bank_base)));
					sprintf(bank_str,"bank%d",bank + m_bank_base);
					memory_set_bankptr(m_machine, bank_str, ram_get_ptr(m_machine.device(RAM_TAG)));
				}
			}
		}
	}
}

//-------------------------------------------------
//  pci_read - implementation of PCI read
//-------------------------------------------------

UINT32 mpc105_device::pci_read(device_t *busdevice, int function, int offset, UINT32 mem_mask)
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
			result = m_bank_registers[(offset - 0x80) / 4];
			break;

		case 0xA0:	/* memory enable */
			result = m_bank_enable;
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


//-------------------------------------------------
//  pci_write - implementation of PCI write
//-------------------------------------------------

void mpc105_device::pci_write(device_t *busdevice, int function, int offset, UINT32 data, UINT32 mem_mask)
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
			if (m_bank_registers[i] != data)
			{
				m_bank_registers[i] = data;
				update_memory();
			}
			break;

		case 0xA0:	/* memory enable */
			if (m_bank_enable != (UINT8) data)
			{
				m_bank_enable = (UINT8) data;
				update_memory();
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


UINT32 mpc105_pci_read(device_t *busdevice, device_t *device, int function, int offset, UINT32 mem_mask)
{
	mpc105_device *mpc105 = dynamic_cast<mpc105_device *>(device);
	return mpc105->pci_read(busdevice, function, offset, mem_mask);
}

void mpc105_pci_write(device_t *busdevice, device_t *device, int function, int offset, UINT32 data, UINT32 mem_mask)
{
	mpc105_device *mpc105 = dynamic_cast<mpc105_device *>(device);
	mpc105->pci_write(busdevice, function, offset, data, mem_mask);
}
