/**************************************************************************
 *
 * gp32.c - Game Park GP32
 * Skeleton by R. Belmont
 *
 * CPU: Samsung S3C2400X01 SoC
 * S3C2400X01 consists of:
 *    ARM920T CPU core + MMU
 *    LCD controller
 *    DMA controller
 *    Interrupt controller
 *    USB controller
 *    and more.
 *
 **************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "cpu/arm7/arm7.h"
#include "cpu/arm7/arm7core.h"
#include "machine/smartmed.h"
#include "includes/gp32.h"

static UINT32 *s3c240x_ram;

// LCD CONTROLLER

static UINT32 s3c240x_vidregs[0x400/4];

#define BPPMODE_TFT_01	0x08
#define BPPMODE_TFT_08	0x0B
#define BPPMODE_TFT_16	0x0C

//             76543210 76543210 76543210 76543210
// 5551 16-bit 00000000 00000000 RRRRRGGG GGBBBBB0
// 5551 32-bit 00000000 RRRRRI00 GGGGGI00 BBBBBI00
// 565  16-bit 00000000 00000000 RRRRRGGG GGBBBBB0
// 565  32-bit 00000000 RRRRR000 GGGGG000 BBBBB000

static VIDEO_UPDATE( gp32 )
{
	if (s3c240x_vidregs[0] & 1)	// display enabled?
	{
		int x, y, bppmode;
//		int hwswp, tpal, offsize;
		UINT32 vramaddr;

//		logerror("vramaddr 1 = %08X\n", s3c240x_vidregs[5] << 1);
//		logerror("vramaddr 2 = %08X\n", ((s3c240x_vidregs[5] & 0xFFE00000) | s3c240x_vidregs[6]) << 1);
		vramaddr = s3c240x_vidregs[5] << 1;

		bppmode = (s3c240x_vidregs[0] >> 1) & 0xF;

//		hwswp = (s3c240x_vidregs[4] >> 0) & 0x1;
//		tpal = s3c240x_vidregs[20] & 0x0001FFFF;
//		offsize = (s3c240x_vidregs[7] >> 11) & 0x7FF;
//		logerror("bppmode %d hwswp %d tpal %05X offsize %03X\n", bppmode, hwswp, tpal, offsize);

		switch (bppmode)
		{
			case BPPMODE_TFT_08 :
			{
				UINT8 *vram;
				vram = (UINT8 *)&s3c240x_ram[(vramaddr-0x0c000000)/4];
				for (y = 0; y < 320; y++)
				{
					UINT32 *scanline = BITMAP_ADDR32( bitmap, y, 0);
					for (x = 0; x < 240; x++)
					{
						*scanline++ = palette_get_color( screen->machine, *vram++);
					}
				}
			}
			break;
			case BPPMODE_TFT_16 :
			{
				UINT16 *vram;
				vram = (UINT16 *)&s3c240x_ram[(vramaddr-0x0c000000)/4];
				for (y = 0; y < 320; y++)
				{
					UINT32 *scanline = BITMAP_ADDR32( bitmap, y, 0);
					for (x = 0; x < 240; x++)
					{
						UINT16 data;
						UINT8 r, g, b;
						data = *vram++;
						r = ((data >> 11) & 0x1F) << 3;
						g = ((data >>  6) & 0x1F) << 3;
						b = ((data >>  1) & 0x1F) << 3;
						*scanline++ = MAKE_RGB( r, g, b);
					}
				}
			}
			break;
			default :
			{
				printf( "bppmode %d not supported\n", bppmode);
			}
			break;
		}
	}
	return 0;
}

static READ32_HANDLER( s3c240x_vidregs_r )
{
	UINT32 data = s3c240x_vidregs[offset];
	switch (offset)
	{
		// LCDCON1
		case 0x00 / 4 :
		{
			// make sure line counter is going
			data = (data & ~0xFFFC0000) | ((240 - video_screen_get_vpos( space->machine->primary_screen)) << 18);
		}
		break;
	}
	return data;
}

static WRITE32_HANDLER( s3c240x_vidregs_w )
{
//	logerror( "(LCD) %08X <- %08X\n", 0x14A00000 + (offset << 2), data);
	COMBINE_DATA(&s3c240x_vidregs[offset]);
}

static WRITE32_HANDLER( s3c240x_palette_w )
{
	UINT8 r, g, b;
	if (mem_mask != 0xffffffff)
	{
		logerror("s3c240x_palette_w: unknown mask %08x\n", mem_mask);
	}
	r = ((data >> 11) & 0x1F) << 3;
	g = ((data >>  6) & 0x1F) << 3;
	b = ((data >>  1) & 0x1F) << 3;
	palette_set_color_rgb(space->machine, offset, r, g, b);
}

// CLOCK & POWER MANAGEMENT

static UINT32 s3c240x_clkpow_regs[0x18/4];

#define MPLLCON  1
#define UPLLCON  2

static UINT32 s3c240x_get_fclk( int reg)
{
	UINT32 data, mdiv, pdiv, sdiv;
	data = s3c240x_clkpow_regs[reg]; // MPLLCON or UPLLCON
	mdiv = (data >> 12) & 0xFF;
	pdiv = (data >>  4) & 0x3F;
	sdiv = (data >>  0) & 0x03;
	return (UINT32)((double)((mdiv + 8) * 12000000) / (double)((pdiv + 2) * (1 << sdiv)));
}

/*
static UINT32 s3c240x_get_hclk( int reg)
{
	switch (s3c240x_clkpow_regs[5] & 0x3) // CLKDIVN
	{
		case 0 : return s3c240x_get_fclk( reg) / 1;
		case 1 : return s3c240x_get_fclk( reg) / 1;
		case 2 : return s3c240x_get_fclk( reg) / 2;
		case 3 : return s3c240x_get_fclk( reg) / 2;
	}
	return 0;
}
*/

static UINT32 s3c240x_get_pclk( int reg)
{
	switch (s3c240x_clkpow_regs[5] & 0x3) // CLKDIVN
	{
		case 0 : return s3c240x_get_fclk( reg) / 1;
		case 1 : return s3c240x_get_fclk( reg) / 2;
		case 2 : return s3c240x_get_fclk( reg) / 2;
		case 3 : return s3c240x_get_fclk( reg) / 4;
	}
	return 0;
}

static READ32_HANDLER( s3c240x_clkpow_r )
{
	return s3c240x_clkpow_regs[offset];
}

static WRITE32_HANDLER( s3c240x_clkpow_w )
{
//	logerror( "(CLKPOW) %08X <- %08X\n", 0x14800000 + (offset << 2), data);
	COMBINE_DATA(&s3c240x_clkpow_regs[offset]);
	switch (offset)
	{
		// MPLLCON
		case 0x04 / 4 :
		{
			cputag_set_clock( space->machine, "maincpu", s3c240x_get_fclk( MPLLCON));
		}
		break;
	}
}

// INTERRUPT CONTROLLER

static UINT32 s3c240x_irq_regs[0x18/4];

static READ32_HANDLER( s3c240x_irq_r )
{
	UINT32 data = s3c240x_irq_regs[offset];
//	logerror( "(IRQ) %08X -> %08X\n", 0x14400000 + (offset << 2), data);
	return data;
}

static WRITE32_HANDLER( s3c240x_irq_w )
{
//	logerror( "(IRQ) %08X <- %08X\n", 0x14400000 + (offset << 2), data);
	COMBINE_DATA(&s3c240x_irq_regs[offset]);
}

static void s3c240x_request_irq( running_machine *machine, UINT32 int_type)
{
//	logerror( "request irq %d\n", int_type);
	s3c240x_irq_regs[5] = int_type; // INTOFFSET
	cpu_set_input_line( cputag_get_cpu( machine, "maincpu"), ARM7_IRQ_LINE, ASSERT_LINE);
	cpu_set_input_line( cputag_get_cpu( machine, "maincpu"), ARM7_IRQ_LINE, CLEAR_LINE);
}

// PWM TIMER

#if 0
static const char *timer_reg_names[] =
{
	"Timer config 0",
	"Timer config 1",
	"Timer control",
	"Timer count buffer 0",
	"Timer compare buffer 0",
	"Timer count observation 0",
	"Timer count buffer 1",
	"Timer compare buffer 1",
	"Timer count observation 1",
	"Timer count buffer 2",
	"Timer compare buffer 2",
	"Timer count observation 2",
	"Timer count buffer 3",
	"Timer compare buffer 3",
	"Timer count observation 3",
	"Timer count buffer 4",
	"Timer compare buffer 4",
	"Timer count observation 4",
};
#endif

static emu_timer *s3c240x_timers[5];
static UINT32 s3c240x_timer_regs[0x44/4];

static READ32_HANDLER( s3c240x_timer_r )
{
	UINT32 data = s3c240x_timer_regs[offset];
//	logerror( "(TIMER) %08X -> %08X\n", 0x15100000 + (offset << 2), data);
	return data;
}

static void s3c240x_timer_recalc( running_machine *machine, int timer, UINT32 ctrl_bits, UINT32 count_reg)
{
	const int mux_table[] = { 2, 4, 8, 16};
	const int prescaler_shift[] = { 0, 0, 8, 8, 8};
	const int mux_shift[] = { 0, 4, 8, 12, 16};
	if (ctrl_bits & 1)	// timer start?
	{
		UINT32 prescaler, mux, cnt, cmp;
		double freq, hz;
		logerror( "starting timer %d\n", timer);
		prescaler = (s3c240x_timer_regs[0] >> prescaler_shift[timer]) & 0xFF;
		mux = (s3c240x_timer_regs[1] >> mux_shift[timer]) & 0x0F;
		freq = s3c240x_get_pclk( MPLLCON) / (prescaler + 1) / mux_table[mux];
		cnt = s3c240x_timer_regs[count_reg+0] & 0xFFFF;
		if (timer != 4)
			cmp = s3c240x_timer_regs[count_reg+1] & 0xFFFF;
		else
			cmp = 0;
		//hz = (double)((prescaler + 1) * mux_table[mux]) / (cnt - cmp + 1);
		hz = freq / (cnt - cmp + 1);
//		logerror( "TIMER %d - FCLK=%d HCLK=%d PCLK=%d prescaler=%d div=%d freq=%f cnt=%d cmp=%d hz=%f\n", timer, s3c240x_get_fclk( MPLLCON), s3c240x_get_hclk( MPLLCON), s3c240x_get_pclk( MPLLCON), prescaler, mux_table[mux], freq, cnt, cmp, hz);
		if (ctrl_bits & 8) // auto reload
		{
			timer_adjust_periodic( s3c240x_timers[timer], ATTOTIME_IN_HZ( hz), timer, ATTOTIME_IN_HZ( hz));
		}
		else
		{
			timer_adjust_oneshot( s3c240x_timers[timer], ATTOTIME_IN_HZ( hz), timer);
		}
	}
	else	// stopping is easy
	{
		logerror( "stopping timer %d\n", timer);
		timer_adjust_oneshot( s3c240x_timers[timer], attotime_never, 0);
	}
}

static WRITE32_HANDLER( s3c240x_timer_w )
{
	UINT32 old_value = s3c240x_timer_regs[offset];
//	logerror( "(TIMER) %08X <- %08X\n", 0x15100000 + (offset << 2), data);
	COMBINE_DATA(&s3c240x_timer_regs[offset]);
	if (offset == 2)	// TCON
	{
		if ((data & 1) != (old_value & 1))	// timer 0 status change
		{
			s3c240x_timer_recalc(space->machine, 0, data&0xf, 0xc / 4);
		}
		if ((data & 0x100) != (old_value & 0x100))	// timer 1 status change
		{
			s3c240x_timer_recalc(space->machine, 1, (data>>8)&0xf, 0x18 / 4);
		}
		if ((data & 0x1000) != (old_value & 0x1000))	// timer 2 status change
		{
			s3c240x_timer_recalc(space->machine, 2, (data>>12)&0xf, 0x24 / 4);
		}
		if ((data & 0x10000) != (old_value & 0x10000))	// timer 3 status change
		{
			s3c240x_timer_recalc(space->machine, 3, (data>>16)&0xf, 0x30 / 4);
		}
		if ((data & 0x100000) != (old_value & 0x100000))	// timer 4 status change
		{
			s3c240x_timer_recalc(space->machine, 4, (data>>20)&0xf, 0x3c / 4);
		}
	}
}

static TIMER_CALLBACK( s3c240x_timer_exp )
{
	int ch = param;
	static UINT32 ch_int[5] = { INT_TIMER0, INT_TIMER1, INT_TIMER2, INT_TIMER3, INT_TIMER4 };
//	logerror( "timer %d callback\n", ch);
//	timer_adjust_oneshot(s3c240x_timers[ch], attotime_never, 0);
	s3c240x_request_irq( machine, ch_int[ch]);
}

// DMA

static UINT32 s3c240x_dma_regs[0x7c/4];

static void s3c240x_dma_start( running_machine *machine, int dma)
{
	const address_space *space = cpu_get_address_space( cputag_get_cpu( machine, "maincpu"), ADDRESS_SPACE_PROGRAM);
	UINT32 size, addr_src, addr_dst, dsz, tsz, tc, reg;
	int i, inc_src, inc_dst;
	reg = dma << 3;
	inc_src = (s3c240x_dma_regs[reg+0] >> 29) & 1;
	addr_src = s3c240x_dma_regs[reg+0] & 0x1FFFFFFF;
	inc_dst = (s3c240x_dma_regs[reg+1] >> 29) & 1;
	addr_dst = s3c240x_dma_regs[reg+1] & 0x1FFFFFFF;
	tsz = (s3c240x_dma_regs[reg+2] >> 27) & 1;
	dsz = (s3c240x_dma_regs[reg+2] >> 20) & 2;
	tc = (s3c240x_dma_regs[reg+2] >> 0) & 0x000FFFFF;
	size = tc << dsz;
//	logerror( "DMA %d - addr_src %08X inc_src %d addr_dst %08X inc_dst %d tsz %d dsz %d tc %d size %d\n", dma, addr_src, inc_src, addr_dst, inc_dst, tsz, dsz, tc, size);
//	logerror( "DMA %d - copy 0x%08X bytes from 0x%08X (%d) to 0x%08X (%d)\n", dma, size, addr_src, inc_src, addr_dst, inc_dst);
	for (i=0;i<size;i++)
	{
		memory_write_byte( space, addr_dst, memory_read_byte( space, addr_src));
		if (!inc_src) addr_src++;
		if (!inc_dst) addr_dst++;
	}
}

static READ32_HANDLER( s3c240x_dma_r )
{
	UINT32 data = s3c240x_dma_regs[offset];
//	logerror( "(DMA) %08X -> %08X\n", 0x14600000 + (offset << 2), data);
	return data;
}

static WRITE32_HANDLER( s3c240x_dma_w )
{
//	logerror( "(DMA) %08X <- %08X\n", 0x14600000 + (offset << 2), data);
	COMBINE_DATA(&s3c240x_dma_regs[offset]);
	switch (offset)
	{
		// DMASKTRIG0
		case 0x18 / 4 :
		{
			if (data & 2) s3c240x_dma_start( space->machine, 0);
		}
		break;
		// DMASKTRIG1
		case 0x38 / 4 :
		{
			if (data & 2) s3c240x_dma_start( space->machine, 1);
		}
		break;
		// DMASKTRIG2
		case 0x58 / 4 :
		{
			if (data & 2) s3c240x_dma_start( space->machine, 2);
		}
		break;
		// DMASKTRIG3
		case 0x78 / 4 :
		{
			if (data & 2) s3c240x_dma_start( space->machine, 3);
		}
		break;
	}
}

// SMARTMEDIA

static struct {
	int add_latch;
	int chip;
	int cmd_latch;
	int do_read;
	int do_write;
	int read;
	int wp;
	int busy;
	UINT8 datarx;
	UINT8 datatx;
} smc;

void smc_reset(void)
{
//	logerror( "smc_reset\n");
	smc.add_latch = 0;
	smc.chip = 0;
	smc.cmd_latch = 0;
	smc.do_read = 0;
	smc.do_write = 0;
	smc.read = 0;
	smc.wp = 0;
	smc.busy = 0;
}

static UINT8 smc_read( running_machine *machine)
{
	const device_config *smartmedia = devtag_get_device( machine, "smartmedia");
	UINT8 data;
	data = smartmedia_data_r( smartmedia);
//	logerror( "smc_read %08X\n", data);
	return data;
}

static void smc_write( running_machine *machine, UINT8 data)
{
//	logerror( "smc_write %08X\n", data);
	if ((smc.chip) && (!smc.read))
	{
		const device_config *smartmedia = devtag_get_device( machine, "smartmedia");
		if (smc.cmd_latch)
		{
//			logerror( "smartmedia_command_w %08X\n", data);
			smartmedia_command_w( smartmedia, data);
		}
		else if (smc.add_latch)
		{
//			logerror( "smartmedia_address_w %08X\n", data);
			smartmedia_address_w( smartmedia, data);
		}
		else
		{
//			logerror( "smartmedia_data_w %08X\n", data);
 			smartmedia_data_w( smartmedia, data);
		}
	}
}

static void smc_update( running_machine *machine)
{
	if (!smc.chip)
	{
		smc_reset();
	}
	else
	{
		if ((smc.do_write) && (!smc.read))
		{
			smc_write( machine, smc.datatx);
		}
		else if ((!smc.do_write) && (smc.do_read) && (smc.read) && (!smc.cmd_latch) && (!smc.add_latch))
		{
			smc.datarx = smc_read( machine);
		}
	}
}

// I/O PORT

static UINT32 s3c240x_gpio[0x60/4];

static READ32_HANDLER( s3c240x_gpio_r )
{
	UINT32 data = s3c240x_gpio[offset];
//	logerror("read GPIO @ 0x156000%02x (PC %x)\n", offset*4, cpu_get_pc(space->cpu));
	switch (offset)
	{
		// PBCON
		case 0x08 / 4 :
		{
			// smartmedia
			data = (data & ~0x00000001);
			if (!smc.read) data = data | 0x00000001;
		}
		break;
		// PBDAT
		case 0x0C / 4 :
		{
			// smartmedia
			data = (data & ~0x000000FF) | (smc.datarx & 0xFF);
			// buttons
			data = (data & ~0x0000FF00) | (input_port_read( space->machine, "IN0") & 0x0000FF00);
		}
		break;
		// PDDAT
		case 0x24 / 4 :
		{
			const device_config *smartmedia = devtag_get_device( space->machine, "smartmedia");
			// smartmedia
			data = (data & ~0x000003C0);
			if (!smc.busy) data = data | 0x00000200;
			if (!smc.do_read) data = data | 0x00000100;
			if (!smc.chip) data = data | 0x00000080;
			if (!smartmedia_protected( smartmedia)) data = data | 0x00000040;
		}
		break;
		// PEDAT
		case 0x30 / 4 :
		{
			const device_config *smartmedia = devtag_get_device( space->machine, "smartmedia");
			// smartmedia
			data = (data & ~0x0000003C);
			if (smc.cmd_latch) data = data | 0x00000020;
			if (smc.add_latch) data = data | 0x00000010;
			if (!smc.do_write) data = data | 0x00000008;
			if (!smartmedia_present( smartmedia)) data = data | 0x00000004;
			// buttons
			data = (data & ~0x000000C0) | (input_port_read( space->machine, "IN1") & 0x000000C0);
		}
		break;
	}
	return data;
}

static WRITE32_HANDLER( s3c240x_gpio_w )
{
//	UINT32 old_value = s3c240x_timer_regs[offset];
	COMBINE_DATA(&s3c240x_gpio[offset]);
//	logerror("%08x to GPIO @ 0x156000%02x (PC %x)\n", data, offset*4, cpu_get_pc(space->cpu));
//	if (offset == 0x30/4) logerror("[%08x] %s %s\n", data, (data & 0x200) ? "SCL" : "scl", (data & 0x400) ? "SDL" : "sdl");
	switch (offset)
	{
		// PBCON
		case 0x08 / 4 :
		{
			// smartmedia
			//if ((old_value & 0x00000001) != (data & 0x00000001))
			{
				smc.read = ((data & 0x00000001) == 0);
				smc_update( space->machine);
			}
		}
		break;
		// PBDAT
		case 0x0C / 4 :
		{
			// smartmedia
			smc.datatx = data & 0xFF;
		}
		break;
		// PDDAT
		case 0x24 / 4 :
		{
			// smartmedia
			//if ((old_value & 0x000001C0) != (data & 0x000001C0))
			{
				smc.do_read = ((data & 0x00000100) == 0);
				smc.chip = ((data & 0x00000080) == 0);
				smc.wp = ((data & 0x00000040) == 0);
				smc_update( space->machine);
			}
		}
		break;
		// PEDAT
		case 0x30 / 4 :
		{
			// smartmedia
			//if ((old_value & 0x00000038) != (data & 0x00000038))
			{
				smc.cmd_latch = ((data & 0x00000020) != 0);
				smc.add_latch = ((data & 0x00000010) != 0);
				smc.do_write  = ((data & 0x00000008) == 0);
				smc_update( space->machine);
			}
		}
		break;
	}
}

// MEMORY CONTROLLER

static UINT32 s3c240x_memcon_regs[0x34/4];

static READ32_HANDLER( s3c240x_memcon_r )
{
	return s3c240x_memcon_regs[offset];
}

static WRITE32_HANDLER( s3c240x_memcon_w )
{
//	logerror( "(MEMCON) %08X <- %08X\n", 0x14000000 + (offset << 2), data);
	COMBINE_DATA(&s3c240x_memcon_regs[offset]);
}

// USB HOST CONTROLLER

static UINT32 s3c240x_usb_host_regs[0x5C/4];

static READ32_HANDLER( s3c240x_usb_host_r )
{
	return s3c240x_usb_host_regs[offset];
}

static WRITE32_HANDLER( s3c240x_usb_host_w )
{
//	logerror( "(USB H) %08X <- %08X\n", 0x14200000 + (offset << 2), data);
	COMBINE_DATA(&s3c240x_usb_host_regs[offset]);
}

// UART 0

static UINT32 s3c240x_uart_0_regs[0x2C/4];

static READ32_HANDLER( s3c240x_uart_0_r )
{
	UINT32 data = s3c240x_uart_0_regs[offset];
	switch (offset)
	{
		// UTRSTAT0
		case 0x10 / 4 :
		{
			data = (data & ~0x00000006) | 0x00000004 | 0x00000002;
		}
		break;
	}
	return data;
}

static WRITE32_HANDLER( s3c240x_uart_0_w )
{
//	logerror( "(UART 0) %08X <- %08X\n", 0x15000000 + (offset << 2), data);
	COMBINE_DATA(&s3c240x_uart_0_regs[offset]);
}

// UART 1

static UINT32 s3c240x_uart_1_regs[0x2C/4];

static READ32_HANDLER( s3c240x_uart_1_r )
{
	return s3c240x_uart_1_regs[offset];
}

static WRITE32_HANDLER( s3c240x_uart_1_w )
{
//	logerror( "(UART 1) %08X <- %08X\n", 0x15004000 + (offset << 2), data);
	COMBINE_DATA(&s3c240x_uart_1_regs[offset]);
}

// USB DEVICE

static UINT32 s3c240x_usb_device_regs[0xBC/4];

static READ32_HANDLER( s3c240x_usb_device_r )
{
	return s3c240x_usb_device_regs[offset];
}

static WRITE32_HANDLER( s3c240x_usb_device_w )
{
//	logerror( "(USB D) %08X <- %08X\n", 0x15200140 + (offset << 2), data);
	COMBINE_DATA(&s3c240x_usb_device_regs[offset]);
}

// WATCHDOG TIMER

static UINT32 s3c240x_watchdog_regs[0x0C/4];

static READ32_HANDLER( s3c240x_watchdog_r )
{
	return s3c240x_watchdog_regs[offset];
}

static WRITE32_HANDLER( s3c240x_watchdog_w )
{
//	logerror( "(WDOG) %08X <- %08X\n", 0x15300000 + (offset << 2), data);
	COMBINE_DATA(&s3c240x_watchdog_regs[offset]);
}

// IIC

static UINT32 s3c240x_iic_regs[0x10/4];

static READ32_HANDLER( s3c240x_iic_r )
{
	return s3c240x_iic_regs[offset];
}

static WRITE32_HANDLER( s3c240x_iic_w )
{
//	logerror( "(IIC) %08X <- %08X\n", 0x15400000 + (offset << 2), data);
	COMBINE_DATA(&s3c240x_iic_regs[offset]);
}

// IIS

static UINT32 s3c240x_iis_regs[0x14/4];

static READ32_HANDLER( s3c240x_iis_r )
{
	UINT32 data = s3c240x_iis_regs[offset];
	//if (offset == 0) data = data & (~0x01); // mp3
	return data;
}

static WRITE32_HANDLER( s3c240x_iis_w )
{
//	logerror( "(IIS) %08X <- %08X\n", 0x15508000 + (offset << 2), data);
	COMBINE_DATA(&s3c240x_iis_regs[offset]);
}

// RTC

static UINT32 s3c240x_rtc_regs[0x4C/4];

static READ32_HANDLER( s3c240x_rtc_r )
{
	return s3c240x_rtc_regs[offset];
}

static WRITE32_HANDLER( s3c240x_rtc_w )
{
//	logerror( "(RTC) %08X <- %08X\n", 0x15700040 + (offset << 2), data);
	COMBINE_DATA(&s3c240x_rtc_regs[offset]);
}

// A/D CONVERTER

static UINT32 s3c240x_adc_regs[0x08/4];

static READ32_HANDLER( s3c240x_adc_r )
{
	return s3c240x_adc_regs[offset];
}

static WRITE32_HANDLER( s3c240x_adc_w )
{
//	logerror( "(ADC) %08X <- %08X\n", 0x15800000 + (offset << 2), data);
	COMBINE_DATA(&s3c240x_adc_regs[offset]);
}

// SPI

static UINT32 s3c240x_spi_regs[0x18/4];

static READ32_HANDLER( s3c240x_spi_r )
{
	return s3c240x_spi_regs[offset];
}

static WRITE32_HANDLER( s3c240x_spi_w )
{
//	logerror( "(SPI) %08X <- %08X\n", 0x15900000 + (offset << 2), data);
	COMBINE_DATA(&s3c240x_spi_regs[offset]);
}

// MMC INTERFACE

static UINT32 s3c240x_mmc_regs[0x40/4];

static READ32_HANDLER( s3c240x_mmc_r )
{
	return s3c240x_mmc_regs[offset];
}

static WRITE32_HANDLER( s3c240x_mmc_w )
{
	COMBINE_DATA(&s3c240x_mmc_regs[offset]);
}

// ...

static void s3c240x_machine_start(running_machine *machine)
{
	s3c240x_timers[0] = timer_alloc(machine, s3c240x_timer_exp, (void *)(FPTR)0);
	s3c240x_timers[1] = timer_alloc(machine, s3c240x_timer_exp, (void *)(FPTR)1);
	s3c240x_timers[2] = timer_alloc(machine, s3c240x_timer_exp, (void *)(FPTR)2);
	s3c240x_timers[3] = timer_alloc(machine, s3c240x_timer_exp, (void *)(FPTR)3);
	s3c240x_timers[4] = timer_alloc(machine, s3c240x_timer_exp, (void *)(FPTR)4);
	smc_reset();
}

static void s3c240x_machine_reset(running_machine *machine)
{
	smc_reset();
}

static ADDRESS_MAP_START( gp32_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x0007ffff) AM_ROM
	AM_RANGE(0x0c000000, 0x0c7fffff) AM_RAM AM_BASE(&s3c240x_ram)
	AM_RANGE(0x14000000, 0x1400003b) AM_READWRITE(s3c240x_memcon_r, s3c240x_memcon_w)
	AM_RANGE(0x14200000, 0x1420005b) AM_READWRITE(s3c240x_usb_host_r, s3c240x_usb_host_w)
	AM_RANGE(0x14400000, 0x14400017) AM_READWRITE(s3c240x_irq_r, s3c240x_irq_w)
	AM_RANGE(0x14600000, 0x1460007b) AM_READWRITE(s3c240x_dma_r, s3c240x_dma_w)
	AM_RANGE(0x14800000, 0x14800017) AM_READWRITE(s3c240x_clkpow_r, s3c240x_clkpow_w)
	AM_RANGE(0x14a00000, 0x14a003ff) AM_READWRITE(s3c240x_vidregs_r, s3c240x_vidregs_w)
	AM_RANGE(0x14a00400, 0x14a007ff) AM_RAM AM_BASE_GENERIC(paletteram) AM_WRITE(s3c240x_palette_w)
	AM_RANGE(0x15000000, 0x1500002b) AM_READWRITE(s3c240x_uart_0_r, s3c240x_uart_0_w)
	AM_RANGE(0x15004000, 0x1500402b) AM_READWRITE(s3c240x_uart_1_r, s3c240x_uart_1_w)
	AM_RANGE(0x15100000, 0x15100043) AM_READWRITE(s3c240x_timer_r, s3c240x_timer_w)
	AM_RANGE(0x15200140, 0x152001fb) AM_READWRITE(s3c240x_usb_device_r, s3c240x_usb_device_w)
	AM_RANGE(0x15300000, 0x1530000b) AM_READWRITE(s3c240x_watchdog_r, s3c240x_watchdog_w)
	AM_RANGE(0x15400000, 0x1540000f) AM_READWRITE(s3c240x_iic_r, s3c240x_iic_w)
	AM_RANGE(0x15508000, 0x15508013) AM_READWRITE(s3c240x_iis_r, s3c240x_iis_w)
	AM_RANGE(0x15600000, 0x1560005b) AM_READWRITE(s3c240x_gpio_r, s3c240x_gpio_w)
	AM_RANGE(0x15700040, 0x1570008b) AM_READWRITE(s3c240x_rtc_r, s3c240x_rtc_w)
	AM_RANGE(0x15800000, 0x15800007) AM_READWRITE(s3c240x_adc_r, s3c240x_adc_w)
	AM_RANGE(0x15900000, 0x15900017) AM_READWRITE(s3c240x_spi_r, s3c240x_spi_w)
	AM_RANGE(0x15a00000, 0x15a0003f) AM_READWRITE(s3c240x_mmc_r, s3c240x_mmc_w)
ADDRESS_MAP_END

static INPUT_PORTS_START( gp32 )
	PORT_START("IN0")
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("R") PORT_PLAYER(1)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("L") PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("B") PORT_PLAYER(1)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("A") PORT_PLAYER(1)
	PORT_START("IN1")
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_SELECT ) PORT_NAME("SELECT") PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_START ) PORT_NAME("START") PORT_PLAYER(1)
INPUT_PORTS_END

static MACHINE_START( gp32 )
{
	s3c240x_machine_start(machine);
}

static MACHINE_RESET( gp32 )
{
	s3c240x_machine_reset(machine);
}

static MACHINE_DRIVER_START( gp32 )
	MDRV_CPU_ADD("maincpu", ARM9, 40000000)
	MDRV_CPU_PROGRAM_MAP(gp32_map)

	MDRV_PALETTE_LENGTH(32768)

	MDRV_SCREEN_ADD("screen", LCD)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_SIZE(240, 320)
	MDRV_SCREEN_VISIBLE_AREA(0, 239, 0, 319)
	/* 320x240 is 4:3 but ROT270 causes an aspect ratio of 3:4 by default */
	MDRV_DEFAULT_LAYOUT(layout_lcd_rot)

	MDRV_VIDEO_UPDATE(gp32)

	MDRV_MACHINE_START(gp32)
	MDRV_MACHINE_RESET(gp32)

	MDRV_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")

	MDRV_SMARTMEDIA_ADD("smartmedia")
MACHINE_DRIVER_END

ROM_START( gp32 )
	ROM_REGION( 0x80000, "maincpu", 0 )
	ROM_SYSTEM_BIOS( 0, "157e", "GP32 Firmware 1.5.7 (English)" )
	ROMX_LOAD( "gp32157e.bin", 0x000000, 0x080000, CRC(b1e35643) SHA1(1566bc2a27980602e9eb501cf8b2d62939bfd1e5), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "100k", "GP32 Firmware 1.0.0 (Korean)" )
	ROMX_LOAD( "gp32100k.bin", 0x000000, 0x080000, CRC(d9925ac9) SHA1(3604d0d7210ed72eddd3e3e0c108f1102508423c), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "156k", "GP32 Firmware 1.5.6 (Korean)" )
	ROMX_LOAD( "gp32156k.bin", 0x000000, 0x080000, CRC(667fb1c8) SHA1(d179ab8e96411272b6a1d683e59da752067f9da8), ROM_BIOS(3) )
	ROM_SYSTEM_BIOS( 3, "166m", "GP32 Firmware 1.6.6 (European)" )
	ROMX_LOAD( "gp32166m.bin", 0x000000, 0x080000, CRC(4548a840) SHA1(1ad0cab0af28fb45c182e5e8c87ead2aaa4fffe1), ROM_BIOS(4) )
//	ROM_SYSTEM_BIOS( 4, "test", "test" )
//	ROMX_LOAD( "test.bin", 0x000000, 0x080000, CRC(00000000) SHA1(0000000000000000000000000000000000000000), ROM_BIOS(5) )
ROM_END

CONS(2001, gp32, 0, 0, gp32, gp32, 0, 0, "Game Park", "GP32", ROT270|GAME_NOT_WORKING|GAME_NO_SOUND)
