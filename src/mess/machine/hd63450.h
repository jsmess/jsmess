/*
	Hitachi HD63450 DMA Controller
*/

#include "driver.h"

struct hd63450_interface
{
	int cpu;
	double clock[4];
	double burst_clock[4];
	void (*dma_end)(int channel,int irq);  // called when the DMA transfer ends
	void (*dma_error)(int channel, int irq);  // called when a DMA transfer error occurs
	int (*dma_read[4])(int addr);  // special read / write handlers for each channel
	void (*dma_write[4])(int addr,int data);  
};

struct hd63450
{
	struct
	{  // offsets in bytes
		unsigned char csr;  // [00] Channel status register (R/W)
		unsigned char cer;  // [01] Channel error register (R)
		unsigned char dcr;  // [04] Device control register (R/W)
		unsigned char ocr;  // [05] Operation control register (R/W)
		unsigned char scr;  // [06] Sequence control register (R/W)
		unsigned char ccr;  // [07] Channel control register (R/W)
		unsigned short mtc;  // [0a,0b]  Memory Transfer Counter (R/W)
		unsigned long mar;  // [0c-0f]  Memory Address Register (R/W)
		unsigned long dar;  // [14-17]  Device Address Register (R/W)
		unsigned short btc;  // [1a,1b]  Base Transfer Counter (R/W)
		unsigned long bar;  // [1c-1f]  Base Address Register (R/W)
		unsigned char niv;  // [25]  Normal Interrupt Vector (R/W)
		unsigned char eiv;  // [27]  Error Interrupt Vector (R/W)
		unsigned char mfc;  // [29]  Memory Function Code (R/W)
		unsigned char cpr;  // [2d]  Channel Priority Register (R/W)
		unsigned char dfc;  // [31]  Device Function Code (R/W)
		unsigned char bfc;  // [39]  Base Function Code (R/W)
		unsigned char gcr;  // [3f]  General Control Register (R/W)
	} reg[4];
	mame_timer* timer[4];  // for timing data reading/writing each channel
	int in_progress[4];  // if a channel is in use
	int transfer_size[4];
	int halted[4];  // non-zero if a channel has been halted, and can be continued later.
	const struct hd63450_interface* intf;
};  

void hd63450_init(struct hd63450_interface* intf);
int hd63450_read(int offset, UINT16 mem_mask);
void hd63450_write(int offset, int data, UINT16 mem_mask);
void hd63450_single_transfer(int x);

void dma_transfer_start(int channel, int dir);

int hd63450_get_vector(int channel);
int hd63450_get_error_vector(int channel);

READ16_HANDLER( hd63450_r );
WRITE16_HANDLER( hd63450_w );
