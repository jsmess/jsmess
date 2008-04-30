/*
    Hitachi HD63450 DMA Controller

	Largely based on documentation of the Sharp X68000
*/

#include "machine/hd63450.h"


static TIMER_CALLBACK(dma_transfer_timer);
static void dma_transfer_abort(const device_config* device, int channel);
static void dma_transfer_halt(const device_config* device, int channel);
static void dma_transfer_continue(const device_config* device, int channel);

DEVICE_START(hd63450)
{
	hd63450_t* dmac = device->token;
	int x;

	assert(device->machine != NULL);
	dmac->intf = device->static_config;

	// Initialise timers and registers
	for(x=0;x<4;x++)
	{
		dmac->timer[x] = timer_alloc(dma_transfer_timer, (void*)device);
		dmac->reg[x].niv = 0x0f;  // defaults?
		dmac->reg[x].eiv = 0x0f;
	}
}

int hd63450_read(const device_config* device, int offset, UINT16 mem_mask)
{
	int channel,reg;
	hd63450_t* dmac = device->token;

	channel = (offset & 0x60) >> 5;
	reg = offset & 0x1f;

	switch(reg)
	{
	case 0x00:  // CSR / CER
		return (dmac->reg[channel].csr << 8) | dmac->reg[channel].cer;
	case 0x02:  // DCR / OCR
		return (dmac->reg[channel].dcr << 8) | dmac->reg[channel].ocr;
	case 0x03:  // SCR / CCR
		return (dmac->reg[channel].scr << 8) | dmac->reg[channel].ccr;
	case 0x05:  // MTC
		return dmac->reg[channel].mtc;
	case 0x06:  // MAR (high)
		return (dmac->reg[channel].mar & 0xffff0000) >> 16;
	case 0x07:  // MAR (low)
		return (dmac->reg[channel].mar & 0x0000ffff);
	case 0x0a:  // DAR (high)
		return (dmac->reg[channel].dar & 0xffff0000) >> 16;
	case 0x0b:  // DAR (low)
		return (dmac->reg[channel].dar & 0x0000ffff);
	case 0x0d:  // BTC
		return dmac->reg[channel].btc;
	case 0x0e:  // BAR (high)
		return (dmac->reg[channel].bar & 0xffff0000) >> 16;
	case 0x0f:  // BAR (low)
		return (dmac->reg[channel].bar & 0x0000ffff);
	case 0x12:  // NIV
		return dmac->reg[channel].niv;
	case 0x13:  // EIV
		return dmac->reg[channel].eiv;
	case 0x14:  // MFC
		return dmac->reg[channel].mfc;
	case 0x16:  // CPR
		return dmac->reg[channel].cpr;
	case 0x18:  // DFC
		return dmac->reg[channel].dfc;
	case 0x1c:  // BFC
		return dmac->reg[channel].bfc;
	case 0x1f:  // GCR
		return dmac->reg[channel].gcr;
	}
	return 0xff;
}

void hd63450_write(const device_config* device, int offset, int data, UINT16 mem_mask)
{
	int channel,reg;

	hd63450_t* dmac = device->token;

	channel = (offset & 0x60) >> 5;
	reg = offset & 0x1f;
	switch(reg)
	{
	case 0x00:  // CSR / CER
		if(ACCESSING_BITS_8_15)
		{
//			dmac->reg[channel].csr = (data & 0xff00) >> 8;
//			logerror("DMA#%i: Channel status write : %02x\n",channel,dmac.reg[channel].csr);
		}
		// CER is read-only, so no action needed there.
		break;
	case 0x02:  // DCR / OCR
		if(ACCESSING_BITS_8_15)
		{
			dmac->reg[channel].dcr = (data & 0xff00) >> 8;
			logerror("DMA#%i: Device Control write : %02x\n",channel,dmac->reg[channel].dcr);
		}
		if(ACCESSING_BITS_0_7)
		{
			dmac->reg[channel].ocr = data & 0x00ff;
			logerror("DMA#%i: Operation Control write : %02x\n",channel,dmac->reg[channel].ocr);
		}
		break;
	case 0x03:  // SCR / CCR
		if(ACCESSING_BITS_8_15)
		{
			dmac->reg[channel].scr = (data & 0xff00) >> 8;
			logerror("DMA#%i: Sequence Control write : %02x\n",channel,dmac->reg[channel].scr);
		}
		if(ACCESSING_BITS_0_7)
		{
			dmac->reg[channel].ccr = data & 0x00ff;
			if((data & 0x0080))// && !dmac->intf->dma_read[channel] && !dmac->intf->dma_write[channel])
				dma_transfer_start(device, channel,0);
			if(data & 0x0010)  // software abort
				dma_transfer_abort(device,channel);
			if(data & 0x0020)  // halt operation
				dma_transfer_halt(device,channel);
			if(data & 0x0040)  // continure operation
				dma_transfer_continue(device,channel);
			logerror("DMA#%i: Channel Control write : %02x\n",channel,dmac->reg[channel].ccr);
		}
		break;
	case 0x05:  // MTC
		dmac->reg[channel].mtc = data;
		logerror("DMA#%i:  Memory Transfer Counter write : %04x\n",channel,dmac->reg[channel].mtc);
		break;
	case 0x06:  // MAR (high)
		dmac->reg[channel].mar = (dmac->reg[channel].mar & 0x0000ffff) | (data << 16);
		logerror("DMA#%i:  Memory Address write : %08lx\n",channel,dmac->reg[channel].mar);
		break;
	case 0x07:  // MAR (low)
		dmac->reg[channel].mar = (dmac->reg[channel].mar & 0xffff0000) | (data & 0x0000ffff);
		logerror("DMA#%i:  Memory Address write : %08lx\n",channel,dmac->reg[channel].mar);
		break;
	case 0x0a:  // DAR (high)
		dmac->reg[channel].dar = (dmac->reg[channel].dar & 0x0000ffff) | (data << 16);
		logerror("DMA#%i:  Device Address write : %08lx\n",channel,dmac->reg[channel].dar);
		break;
	case 0x0b:  // DAR (low)
		dmac->reg[channel].dar = (dmac->reg[channel].dar & 0xffff0000) | (data & 0x0000ffff);
		logerror("DMA#%i:  Device Address write : %08lx\n",channel,dmac->reg[channel].dar);
		break;
	case 0x0d:  // BTC
		dmac->reg[channel].btc = data;
		logerror("DMA#%i:  Base Transfer Counter write : %04x\n",channel,dmac->reg[channel].btc);
		break;
	case 0x0e:  // BAR (high)
		dmac->reg[channel].bar = (dmac->reg[channel].bar & 0x0000ffff) | (data << 16);
		logerror("DMA#%i:  Base Address write : %08lx\n",channel,dmac->reg[channel].bar);
		break;
	case 0x0f:  // BAR (low)
		dmac->reg[channel].bar = (dmac->reg[channel].bar & 0xffff0000) | (data & 0x0000ffff);
		logerror("DMA#%i:  Base Address write : %08lx\n",channel,dmac->reg[channel].bar);
		break;
	case 0x12:  // NIV
		dmac->reg[channel].niv = data & 0xff;
		logerror("DMA#%i:  Normal IRQ Vector write : %02x\n",channel,dmac->reg[channel].niv);
		break;
	case 0x13:  // EIV
		dmac->reg[channel].eiv = data & 0xff;
		logerror("DMA#%i:  Error IRQ Vector write : %02x\n",channel,dmac->reg[channel].eiv);
		break;
	case 0x14:  // MFC
		dmac->reg[channel].mfc = data & 0xff;
		logerror("DMA#%i:  Memory Function Code write : %02x\n",channel,dmac->reg[channel].mfc);
		break;
	case 0x16:  // CPR
		dmac->reg[channel].cpr = data & 0xff;
		logerror("DMA#%i:  Channel Priority write : %02x\n",channel,dmac->reg[channel].cpr);
		break;
	case 0x18:  // DFC
		dmac->reg[channel].dfc = data & 0xff;
		logerror("DMA#%i:  Device Function Code write : %02x\n",channel,dmac->reg[channel].dfc);
		break;
	case 0x1c:  // BFC
		dmac->reg[channel].bfc = data & 0xff;
		logerror("DMA#%i:  Base Function Code write : %02x\n",channel,dmac->reg[channel].bfc);
		break;
	case 0x1f:
		dmac->reg[channel].gcr = data & 0xff;
		logerror("DMA#%i:  General Control write : %02x\n",channel,dmac->reg[channel].gcr);
		break;
	}
}

void dma_transfer_start(const device_config* device, int channel, int dir)
{
	hd63450_t* dmac = device->token;
	dmac->in_progress[channel] = 1;
	dmac->reg[channel].csr &= ~0xe0;
	dmac->reg[channel].csr |= 0x08;  // Channel active
	dmac->reg[channel].csr &= ~0x30;  // Reset Error and Normal termination bits
	if(dmac->reg[channel].btc > 0)
	{
		dmac->reg[channel].mar = program_read_word(dmac->reg[channel].bar) << 8;
		dmac->reg[channel].mar |= program_read_word(dmac->reg[channel].bar+2);
		dmac->reg[channel].mtc = program_read_word(dmac->reg[channel].bar+4);
		dmac->reg[channel].btc--;
	}

	// Burst transfers will halt the CPU until the transfer is complete
	if((dmac->reg[channel].dcr & 0xc0) == 0x00)  // Burst transfer
	{
		cpunum_set_input_line(device->machine, dmac->intf->cpu,INPUT_LINE_HALT,ASSERT_LINE);
		timer_adjust_periodic(dmac->timer[channel], attotime_zero, channel, dmac->intf->burst_clock[channel]);
	}
	else
		timer_adjust_periodic(dmac->timer[channel], ATTOTIME_IN_USEC(500), channel, dmac->intf->clock[channel]);


	dmac->transfer_size[channel] = dmac->reg[channel].mtc;

	logerror("DMA: Transfer begins: size=0x%08x\n",dmac->transfer_size[channel]);
}

static TIMER_CALLBACK(dma_transfer_timer)
{
	hd63450_single_transfer(ptr, param);
}

static void dma_transfer_abort(const device_config* device, int channel)
{
	hd63450_t* dmac = device->token;

	logerror("DMA#%i: Transfer aborted\n",channel);
	timer_adjust_oneshot(dmac->timer[channel], attotime_zero, 0);
	dmac->in_progress[channel] = 0;
	dmac->reg[channel].mtc = dmac->transfer_size[channel];
	dmac->reg[channel].csr |= 0xe0;  // channel operation complete, block transfer complete
	dmac->reg[channel].csr &= ~0x08;  // channel no longer active
}

static void dma_transfer_halt(const device_config* device, int channel)
{
	hd63450_t* dmac = device->token;

	dmac->halted[channel] = 1;
	timer_adjust_oneshot(dmac->timer[channel], attotime_zero, 0);
}

static void dma_transfer_continue(const device_config* device, int channel)
{
	hd63450_t* dmac = device->token;

	if(dmac->halted[channel] != 0)
	{
		dmac->halted[channel] = 0;
		timer_adjust_periodic(dmac->timer[channel], attotime_zero, channel, dmac->intf->clock[channel]);
	}
}

void hd63450_single_transfer(const device_config* device, int x)
{
	int data;
	int datasize = 1;
	hd63450_t* dmac = device->token;

		if(dmac->in_progress[x] != 0)  // DMA in progress in channel x
		{
			if(dmac->reg[x].ocr & 0x80)  // direction: 1 = device -> memory
			{
				if(dmac->intf->dma_read[x])
				{
					data = dmac->intf->dma_read[x](dmac->reg[x].mar);
					if(data == -1)
						return;  // not ready to recieve data
					program_write_byte(dmac->reg[x].mar,data);
					datasize = 1;
				}
				else
				{
					switch(dmac->reg[x].ocr & 0x30)  // operation size
					{
					case 0x00:  // 8 bit
						data = program_read_byte(dmac->reg[x].dar);  // read from device address
						program_write_byte(dmac->reg[x].mar, data);  // write to memory address
						datasize = 1;
						break;
					case 0x10:  // 16 bit
						data = program_read_word(dmac->reg[x].dar);  // read from device address
						program_write_word(dmac->reg[x].mar, data);  // write to memory address
						datasize = 2;
						break;
					case 0x20:  // 32 bit
						data = program_read_word(dmac->reg[x].dar) << 16;  // read from device address
						data |= program_read_word(dmac->reg[x].dar+2);
						program_write_word(dmac->reg[x].mar, (data & 0xffff0000) >> 16);  // write to memory address
						program_write_word(dmac->reg[x].mar+2, data & 0x0000ffff);
						datasize = 4;
						break;
					case 0x30:  // 8 bit packed (?)
						data = program_read_byte(dmac->reg[x].dar);  // read from device address
						program_write_byte(dmac->reg[x].mar, data);  // write to memory address
						datasize = 1;
						break;
					}
				}
//				logerror("DMA#%i: byte transfer %08lx -> %08lx  (byte = %02x)\n",x,dmac.reg[x].dar,dmac.reg[x].mar,data);
			}
			else  // memory -> device
			{
				if(dmac->intf->dma_write[x])
				{
					data = program_read_byte(dmac->reg[x].mar);
					dmac->intf->dma_write[x](dmac->reg[x].mar,data);
					datasize = 1;
				}
				else
				{
					switch(dmac->reg[x].ocr & 0x30)  // operation size
					{
					case 0x00:  // 8 bit
						data = program_read_byte(dmac->reg[x].mar);  // read from memory address
						program_write_byte(dmac->reg[x].dar, data);  // write to device address
						datasize = 1;
						break;
					case 0x10:  // 16 bit
						data = program_read_word(dmac->reg[x].mar);  // read from memory address
						program_write_word(dmac->reg[x].dar, data);  // write to device address
						datasize = 2;
						break;
					case 0x20:  // 32 bit
						data = program_read_word(dmac->reg[x].mar) << 16;  // read from memory address
						data |= program_read_word(dmac->reg[x].mar+2);  // read from memory address
						program_write_word(dmac->reg[x].dar, (data & 0xffff0000) >> 16);  // write to device address
						program_write_word(dmac->reg[x].dar+2, data & 0x0000ffff);  // write to device address
						datasize = 4;
						break;
					case 0x30:  // 8 bit packed (?)
						data = program_read_byte(dmac->reg[x].mar);  // read from memory address
						//program_write_byte(dmac->reg[x].dar, data);  // write to device address  -- disabled for now
						datasize = 1;
						break;
					}
				}
//				logerror("DMA#%i: byte transfer %08lx -> %08lx\n",x,dmac->reg[x].mar,dmac->reg[x].dar);
			}


			// decrease memory transfer counter
			if(dmac->reg[x].mtc > 0)
				dmac->reg[x].mtc--;

			// handle change of memory and device addresses
			if((dmac->reg[x].scr & 0x03) == 0x01)
				dmac->reg[x].dar+=datasize;
			else if((dmac->reg[x].scr & 0x03) == 0x02)
				dmac->reg[x].dar-=datasize;

			if((dmac->reg[x].scr & 0x0c) == 0x04)
				dmac->reg[x].mar+=datasize;
			else if((dmac->reg[x].scr & 0x0c) == 0x08)
				dmac->reg[x].mar-=datasize;

			if(dmac->reg[x].mtc <= 0)
			{
				// End of transfer
				logerror("DMA#%i: End of transfer\n",x);
				if(dmac->reg[x].btc > 0)
				{
					dmac->reg[x].btc--;
					dmac->reg[x].bar+=6;
					dmac->reg[x].mar = program_read_word(dmac->reg[x].bar) << 8;
					dmac->reg[x].mar |= program_read_word(dmac->reg[x].bar+2);
					dmac->reg[x].mtc = program_read_word(dmac->reg[x].bar+4);
					return;
				}
				timer_adjust_oneshot(dmac->timer[x], attotime_zero, 0);
				dmac->in_progress[x] = 0;
				dmac->reg[x].mtc = dmac->transfer_size[x];
				dmac->reg[x].csr |= 0xe0;  // channel operation complete, block transfer complete
				dmac->reg[x].csr &= ~0x08;  // channel no longer active
				if((dmac->reg[x].dcr & 0xc0) == 0x00)  // Burst transfer
					cpunum_set_input_line(device->machine, dmac->intf->cpu,INPUT_LINE_HALT,CLEAR_LINE);

				if(dmac->intf->dma_end)
					dmac->intf->dma_end(x,dmac->reg[x].ccr & 0x08);
			}
		}
}

int hd63450_get_vector(const device_config* device, int channel)
{
	hd63450_t* dmac = device->token;
	return dmac->reg[channel].niv;
}

int hd63450_get_error_vector(const device_config* device, int channel)
{
	hd63450_t* dmac = device->token;
	return dmac->reg[channel].eiv;
}

static DEVICE_SET_INFO( hd63450 )
{
	switch (state)
	{
		/* no parameters to set */
	}
}

DEVICE_GET_INFO(hd63450)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_OTHER;				break;
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(hd63450_t);				break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_SET_INFO:						info->set_info = DEVICE_SET_INFO_NAME(hd63450); break;
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(hd63450);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							/*info->reset = DEVICE_RESET_NAME(hd63450);*/	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							info->s = "Hitachi HD63450";				break;
		case DEVINFO_STR_FAMILY:						info->s = "DMA Controller";					break;
		case DEVINFO_STR_VERSION:						info->s = "1.0";							break;
		case DEVINFO_STR_SOURCE_FILE:					info->s = __FILE__;							break;
		case DEVINFO_STR_CREDITS:						info->s = "Copyright the MESS Team"; 		break;
	}
}

READ16_DEVICE_HANDLER(hd63450_r) { return hd63450_read(device,offset,mem_mask); }
WRITE16_DEVICE_HANDLER(hd63450_w) { hd63450_write(device,offset,data,mem_mask); }

