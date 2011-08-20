/***************************************************************************

  National Semiconductor DP8390x (DP8390, DP83901, DP83902) Ethernet family

***************************************************************************/

#include "emu.h"
#include "machine/dp8390x.h"

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type DP8390X = &device_creator<dp8390x_device>;

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  dp8390x_device - constructor
//-------------------------------------------------

dp8390x_device::dp8390x_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, DP8390X, "National Semicondutor DP8390x", tag, owner, clock)
{
}

dp8390x_device::dp8390x_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, type, name, tag, owner, clock)
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void dp8390x_device::device_start()
{
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void dp8390x_device::device_reset()
{
	m_regbank = 0;
	memset(m_registers, 0, sizeof(m_registers));
}

WRITE8_MEMBER( dp8390x_device::reg_w )
{
	// register 0 is identical in all 3 pages
	if (offset == 0)
	{
		m_registers[offset] = data;
		m_regbank = (data>>6)&3;
//      printf("dp8390x: %02x to control, regbank = %d (PC=%x)\n", data, m_regbank, cpu_get_pc(&space.device()));
	}
	else
	{
		m_registers[(m_regbank*0x10) + offset] = data;
//      printf("dp8390x: bank %d reg %x = %02x (PC=%x)\n", m_regbank, offset, data, cpu_get_pc(&space.device()));
	}
}

READ8_MEMBER( dp8390x_device::reg_r )
{
//  printf("dp8390x_r: @ %x (regbank %d, PC=%x)\n", offset, m_regbank, cpu_get_pc(&space.device()));

	if (offset == 0)
	{
		return m_registers[0];
	}
	else
	{
		return m_registers[(m_regbank*0x10) + offset];
	}
	return 0;
}

/*

loopback:

write 00000000 to 2 mem_mask ff000000
write 00000000 to 9 mem_mask ff000000
write 00000000 to a mem_mask ff000000
write 22000000 to f mem_mask ff000000
write 02000000 to 2 mem_mask ff000000
write 011f0000 to 8 mem_mask ffff0000
write 00400000 to 1 mem_mask ffff0000

dp8390x: bank 0 reg f = 22 (PC=6cbe2e)
dp8390x: bank 0 reg 5 = 00 (PC=6cbce4)
dp8390x: bank 0 reg 4 = 00 (PC=6cbcea)
dp8390x: bank 0 reg f = 21 (PC=6cbcf2)

waits for bit 7 of current remote DMA

write 41000000 to 1 mem_mask ff000000
write 1f000000 to 3 mem_mask ff000000
write 02000000 to 2 mem_mask ff000000
write 06000000 to e mem_mask ff000000
write ff000000 to d mem_mask ff000000
write fe000000 to c mem_mask ff000000
write ff000000 to 8 mem_mask ff000000
write 00000000 to 0 mem_mask ff000000
write 61000000 to f mem_mask ff000000
write 06000000 to 8 mem_mask ff000000
write 00000000 to e mem_mask ff000000
write 00000000 to d mem_mask ff000000
write 00000000 to c mem_mask ff000000
write 00000000 to b mem_mask ff000000
write 00000000 to a mem_mask ff000000
write 00000000 to 9 mem_mask ff000000
write 22000000 to f mem_mask ff000000
write 00000000 to 2 mem_mask ff000000
write 00000000 to 2 mem_mask ff000000
write 00000000 to 5 mem_mask ff000000
write 00000000 to 4 mem_mask ff000000
write 21000000 to f mem_mask ff000000

dp8390x_r: @ 8 (regbank 0, PC=6cbd08)

waits for bit 7 of current remote DMA

dp8390x: bank 0 reg 1 = 43 (PC=6cbeac)
dp8390x: bank 0 reg 3 = 1f (PC=6cbeb4)
dp8390x: bank 0 reg 2 = 06 (PC=6cbeba)
dp8390x: bank 0 reg 8 = ff (PC=6cbec2)
dp8390x: bank 0 reg f = 22 (PC=6cbece)
dp8390x: bank 0 reg b = 00 (PC=6cbed4)
dp8390x: bank 0 reg a = 78 (PC=6cbeda)
dp8390x: bank 0 reg 9 = 00 (PC=6cbee2)
dp8390x: bank 0 reg f = 26 (PC=6cbeea)
dp8390x_r: @ f (regbank 0, PC=6cbef4)

wait for 22 (counter overflow + packet transmit) (or ???)

dp8390x: bank 0 reg 8 = 01 (PC=6cbf7e)
dp8390x: bank 0 reg 2 = 00 (PC=6cbe9e)
dp8390x: bank 0 reg 5 = 00 (PC=6cbce4)
dp8390x: bank 0 reg 4 = 00 (PC=6cbcea)
dp8390x: bank 0 reg f = 21 (PC=6cbcf2)

waits for bit 7 of current remote DMA

dp8390x: bank 0 reg 1 = 43 (PC=6cbeac)
dp8390x: bank 0 reg 3 = 1f (PC=6cbeb4)
dp8390x: bank 0 reg 2 = 06 (PC=6cbeba)
dp8390x: bank 0 reg 8 = ff (PC=6cbec2)
dp8390x: bank 0 reg f = 22 (PC=6cbece)
dp8390x: bank 0 reg b = 00 (PC=6cbed4)
dp8390x: bank 0 reg a = 78 (PC=6cbeda)
dp8390x: bank 0 reg 9 = 00 (PC=6cbee2)
dp8390x: bank 0 reg f = 26 (PC=6cbeea)
dp8390x_r: @ f (regbank 0, PC=6cbef4)

wait for 22 (counter overflow + packet transmit) (or ???)

*/
