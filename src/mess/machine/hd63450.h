/*
    Hitachi HD63450 DMA Controller
*/

#include "emu.h"

typedef struct _hd63450_interface hd63450_intf;
struct _hd63450_interface
{
	const char *cpu_tag;
	attotime clock[4];
	attotime burst_clock[4];
	void (*dma_end)(running_machine *machine,int channel,int irq);  // called when the DMA transfer ends
	void (*dma_error)(running_machine *machine,int channel, int irq);  // called when a DMA transfer error occurs
	int (*dma_read[4])(running_machine *machine,int addr);  // special read / write handlers for each channel
	void (*dma_write[4])(running_machine *machine,int addr,int data);
};

int hd63450_read(running_device* device, int offset, UINT16 mem_mask);
void hd63450_write(running_device* device,int offset, int data, UINT16 mem_mask);
void hd63450_single_transfer(running_device* device, int x);
void hd63450_set_timer(running_device* device, int channel, attotime tm);

int hd63450_get_vector(running_device* device, int channel);
int hd63450_get_error_vector(running_device* device, int channel);

DECLARE_LEGACY_DEVICE(HD63450, hd63450);

#define MDRV_HD63450_ADD(_tag, _config) \
	MDRV_DEVICE_ADD(_tag, HD63450, 0) \
	MDRV_DEVICE_CONFIG(_config)

READ16_DEVICE_HANDLER( hd63450_r );
WRITE16_DEVICE_HANDLER( hd63450_w );
