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
#include "sound/dac.h"

#define CLOCK_MULTIPLIER 1

#define BIT(x,n) (((x)>>(n))&1)
#define BITS(x,m,n) (((x)>>(n))&((1<<((m)-(n)+1))-1))

static UINT32 *s3c240x_ram;
static UINT8 *eeprom_data;

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

		bppmode = BITS( s3c240x_vidregs[0], 4, 1);

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
				logerror( "bppmode %d not supported\n", bppmode);
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
//	logerror( "(LCD) %08X -> %08X (PC %08X)\n", 0x14A00000 + (offset << 2), data, cpu_get_pc( space->cpu));
	return data;
}

static WRITE32_HANDLER( s3c240x_vidregs_w )
{
//	logerror( "(LCD) %08X <- %08X (PC %08X)\n", 0x14A00000 + (offset << 2), data, cpu_get_pc( space->cpu));
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
	mdiv = BITS( data, 19, 12);
	pdiv = BITS( data, 9, 4);
	sdiv = BITS( data, 1, 0);
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
//	logerror( "(CLKPOW) %08X <- %08X (PC %08X)\n", 0x14800000 + (offset << 2), data, cpu_get_pc( space->cpu));
	COMBINE_DATA(&s3c240x_clkpow_regs[offset]);
	switch (offset)
	{
		// MPLLCON
		case 0x04 / 4 :
		{
			cputag_set_clock( space->machine, "maincpu", s3c240x_get_fclk( MPLLCON) * CLOCK_MULTIPLIER);
		}
		break;
	}
}

// INTERRUPT CONTROLLER

static UINT32 s3c240x_irq_regs[0x18/4];

static void s3c240x_check_pending_irq( running_machine *machine)
{
	if (s3c240x_irq_regs[0] != 0)
	{
		UINT32 int_type = 0, temp;
		temp = s3c240x_irq_regs[0];
		while (!(temp & 1))
		{
			int_type++;
			temp = temp >> 1;
		}
		s3c240x_irq_regs[4] |= (1 << int_type); // INTPND
		s3c240x_irq_regs[5] = int_type; // INTOFFSET
		cpu_set_input_line( cputag_get_cpu( machine, "maincpu"), ARM7_IRQ_LINE, ASSERT_LINE);
	}
	else
	{
		cpu_set_input_line( cputag_get_cpu( machine, "maincpu"), ARM7_IRQ_LINE, CLEAR_LINE);
	}
}

static void s3c240x_request_irq( running_machine *machine, UINT32 int_type)
{
//	logerror( "request irq %d\n", int_type);
//	logerror( "(1) %08X %08X %08X %08X %08X %08X\n", s3c240x_irq_regs[0], s3c240x_irq_regs[1], s3c240x_irq_regs[2], s3c240x_irq_regs[3], s3c240x_irq_regs[4], s3c240x_irq_regs[5]);
	if (s3c240x_irq_regs[0] == 0)
	{
		s3c240x_irq_regs[0] |= (1 << int_type); // SRCPND
		s3c240x_irq_regs[4] |= (1 << int_type); // INTPND
		s3c240x_irq_regs[5] = int_type; // INTOFFSET
		cpu_set_input_line( cputag_get_cpu( machine, "maincpu"), ARM7_IRQ_LINE, ASSERT_LINE);
	}
	else
	{
		s3c240x_irq_regs[0] |= (1 << int_type); // SRCPND
		s3c240x_check_pending_irq( machine);
	}
}


static READ32_HANDLER( s3c240x_irq_r )
{
	UINT32 data = s3c240x_irq_regs[offset];
//	logerror( "(IRQ) %08X -> %08X (PC %08X)\n", 0x14400000 + (offset << 2), data, cpu_get_pc( space->cpu));
	return data;
}

static WRITE32_HANDLER( s3c240x_irq_w )
{
	UINT32 old_value = s3c240x_irq_regs[offset];
//	logerror( "(IRQ) %08X <- %08X (PC %08X)\n", 0x14400000 + (offset << 2), data, cpu_get_pc( space->cpu));
	COMBINE_DATA(&s3c240x_irq_regs[offset]);
	switch (offset)
	{
		// SRCPND
		case 0x00 / 4 :
		{
			s3c240x_irq_regs[0] = (old_value & ~data); // clear only the bit positions of SRCPND corresponding to those set to one in the data
//			logerror( "(2) %08X %08X %08X %08X %08X %08X\n", s3c240x_irq_regs[0], s3c240x_irq_regs[1], s3c240x_irq_regs[2], s3c240x_irq_regs[3], s3c240x_irq_regs[4], s3c240x_irq_regs[5]);
			s3c240x_check_pending_irq( space->machine);
		}
		break;
		// INTPND
		case 0x10 / 4 :
		{
			s3c240x_irq_regs[4] = (old_value & ~data); // clear only the bit positions of INTPND corresponding to those set to one in the data
//			logerror( "(3) %08X %08X %08X %08X %08X %08X\n", s3c240x_irq_regs[0], s3c240x_irq_regs[1], s3c240x_irq_regs[2], s3c240x_irq_regs[3], s3c240x_irq_regs[4], s3c240x_irq_regs[5]);
		}
		break;
	}
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

static emu_timer *s3c240x_pwm_timer[5];
static UINT32 s3c240x_pwm_regs[0x44/4];

static READ32_HANDLER( s3c240x_pwm_r )
{
	UINT32 data = s3c240x_pwm_regs[offset];
//	logerror( "(PWM) %08X -> %08X (PC %08X)\n", 0x15100000 + (offset << 2), data, cpu_get_pc( space->cpu));
	return data;
}

static void s3c240x_pwm_start( running_machine *machine, int timer)
{
	const int mux_table[] = { 2, 4, 8, 16};
	const int prescaler_shift[] = { 0, 0, 8, 8, 8};
	const int mux_shift[] = { 0, 4, 8, 12, 16};
	const int tcon_shift[] = { 0, 8, 12, 16, 20};
	const UINT32 *regs = &s3c240x_pwm_regs[3+timer*3];
	UINT32 prescaler, mux, cnt, cmp, auto_reload;
	double freq, hz;
//		logerror( "PWM %d start\n", timer);
	prescaler = (s3c240x_pwm_regs[0] >> prescaler_shift[timer]) & 0xFF;
	mux = (s3c240x_pwm_regs[1] >> mux_shift[timer]) & 0x0F;
	freq = s3c240x_get_pclk( MPLLCON) / (prescaler + 1) / mux_table[mux];
	cnt = BITS( regs[0], 15, 0);
	if (timer != 4)
	{
		cmp = BITS( regs[1], 15, 0);
		auto_reload = BIT( s3c240x_pwm_regs[2], tcon_shift[timer] + 3);
	}
	else
	{
		cmp = 0;
		auto_reload = BIT( s3c240x_pwm_regs[2], tcon_shift[timer] + 2);
	}
	hz = freq / (cnt - cmp + 1);
//	logerror( "PWM %d - FCLK=%d HCLK=%d PCLK=%d prescaler=%d div=%d freq=%f cnt=%d cmp=%d auto_reload=%d hz=%f\n", timer, s3c240x_get_fclk( MPLLCON), s3c240x_get_hclk( MPLLCON), s3c240x_get_pclk( MPLLCON), prescaler, mux_table[mux], freq, cnt, cmp, auto_reload, hz);
	if (auto_reload)
	{
		timer_adjust_periodic( s3c240x_pwm_timer[timer], ATTOTIME_IN_HZ( hz), timer, ATTOTIME_IN_HZ( hz));
	}
	else
	{
		timer_adjust_oneshot( s3c240x_pwm_timer[timer], ATTOTIME_IN_HZ( hz), timer);
	}
}

static void s3c240x_pwm_stop( running_machine *machine, int timer)
{
//	logerror( "PWM %d stop\n", timer);
	timer_adjust_oneshot( s3c240x_pwm_timer[timer], attotime_never, 0);
}

static void s3c240x_pwm_recalc( running_machine *machine, int timer)
{
	const int tcon_shift[] = { 0, 8, 12, 16, 20};
	if (s3c240x_pwm_regs[2] & (1 << tcon_shift[timer]))
	{
		s3c240x_pwm_start( machine, timer);
	}
	else
	{
		s3c240x_pwm_stop( machine, timer);
	}
}

static WRITE32_HANDLER( s3c240x_pwm_w )
{
	UINT32 old_value = s3c240x_pwm_regs[offset];
//	logerror( "(PWM) %08X <- %08X (PC %08X)\n", 0x15100000 + (offset << 2), data, cpu_get_pc( space->cpu));
	COMBINE_DATA(&s3c240x_pwm_regs[offset]);
	switch (offset)
	{
		// TCON
		case 0x08 / 4 :
		{
			if ((data & 1) != (old_value & 1))
			{
				s3c240x_pwm_recalc( space->machine, 0);
			}
			if ((data & 0x100) != (old_value & 0x100))
			{
				s3c240x_pwm_recalc( space->machine, 1);
			}
			if ((data & 0x1000) != (old_value & 0x1000))
			{
				s3c240x_pwm_recalc( space->machine, 2);
			}
			if ((data & 0x10000) != (old_value & 0x10000))
			{
				s3c240x_pwm_recalc( space->machine, 3);
			}
			if ((data & 0x100000) != (old_value & 0x100000))
			{
				s3c240x_pwm_recalc( space->machine, 4);
			}
		}
	}
}

static TIMER_CALLBACK( s3c240x_pwm_timer_exp )
{
	int ch = param;
	const int ch_int[] = { INT_TIMER0, INT_TIMER1, INT_TIMER2, INT_TIMER3, INT_TIMER4 };
//	logerror( "PWM %d timer callback\n", ch);
	s3c240x_request_irq( machine, ch_int[ch]);
}

// DMA

static emu_timer *s3c240x_dma_timer[4];
static UINT32 s3c240x_dma_regs[0x7c/4];

static void s3c240x_dma_trigger( running_machine *machine, int dma)
{
	UINT32 *regs = &s3c240x_dma_regs[dma<<3];
	UINT32 curr_tc, curr_src, curr_dst;
  const address_space *space = cputag_get_address_space( machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	int dsz, inc;
	static UINT32 ch_int[] = { INT_DMA0, INT_DMA1, INT_DMA2, INT_DMA3};
//	logerror( "DMA %d trigger\n", dma);
	curr_tc = BITS( regs[3], 19, 0);
	curr_src = BITS( regs[4], 28, 0);
	curr_dst = BITS( regs[5], 28, 0);
	dsz = BITS( regs[2], 21, 20);
//	logerror( "DMA %d - curr_src %08X curr_dst %08X curr_tc %04X dsz %d\n", dma, curr_src, curr_dst, curr_tc, dsz);
	switch (dsz)
	{
		case 0 : memory_write_byte( space, curr_dst, memory_read_byte( space, curr_src)); break;
		case 1 : memory_write_word( space, curr_dst, memory_read_word( space, curr_src)); break;
		case 2 : memory_write_dword( space, curr_dst, memory_read_dword( space, curr_src)); break;
	}
	// update curr_src
	inc = BIT( regs[0], 29);
	if (!inc)
	{
		curr_src += (1 << dsz);
		regs[4] = (regs[4] & ~0x1FFFFFFF) | curr_src;
	}
	// update curr_dst
	inc = BIT( regs[1], 29);
	if (!inc)
	{
		curr_dst += (1 << dsz);
		regs[5] = (regs[5] & ~0x1FFFFFFF) | curr_dst;
	}
	// update curr_tc
	curr_tc--;
	regs[3] = (regs[3] & ~0x000FFFFF) | curr_tc;
	// ...
	if (curr_tc == 0)
	{
		int _int, reload;
		reload = BIT( regs[2], 22);
		if (!reload)
		{
			regs[3] = (regs[3] & ~0x000FFFFF) | BITS( regs[2], 19, 0);
			regs[4] = (regs[4] & ~0x1FFFFFFF) | BITS( regs[0], 28, 0);
			regs[5] = (regs[5] & ~0x1FFFFFFF) | BITS( regs[1], 28, 0);
		}
		else
		{
			regs[6] &= ~(1 << 1); // clear on/off
		}
		_int = BIT( regs[2], 28);
		if (_int)
		{
//			logerror( "DMA %d request irq\n", dma);
			s3c240x_request_irq( machine, ch_int[dma]);
		}
	}
}

static void s3c240x_dma_start( running_machine *machine, int dma)
{
	UINT32 addr_src, addr_dst, tc;
	UINT32 *regs = &s3c240x_dma_regs[dma<<3];
//	UINT32 dsz, tsz, reload;
//	int inc_src, inc_dst, _int, servmode, swhw_sel, hwsrcsel;
//	logerror( "DMA %d start\n", dma);
	addr_src = BITS( regs[0], 28, 0);
	addr_dst = BITS( regs[1], 28, 0);
	tc = BITS( regs[2], 19, 0);
//	inc_src = BIT( regs[0], 29);
//	inc_dst = BIT( regs[1], 29);
//	tsz = BIT( regs[2], 27);
//	_int = BIT( regs[2], 28);
//	servmode = BIT( regs[2], 26);
//	hwsrcsel = BITS( regs[2], 25, 24);
//	swhw_sel = BIT( regs[2], 23);
//	reload = BIT( regs[2], 22);
//	dsz = BITS( regs[2], 21, 20);
//	logerror( "DMA %d - addr_src %08X inc_src %d addr_dst %08X inc_dst %d int %d tsz %d servmode %d hwsrcsel %d swhw_sel %d reload %d dsz %d tc %d\n", dma, addr_src, inc_src, addr_dst, inc_dst, _int, tsz, servmode, hwsrcsel, swhw_sel, reload, dsz, tc);
//	logerror( "DMA %d - copy %08X bytes from %08X (%s) to %08X (%s)\n", dma, tc << dsz, addr_src, inc_src ? "fix" : "inc", addr_dst, inc_dst ? "fix" : "inc");
	regs[3] = (regs[3] & ~0x000FFFFF) | tc;
	regs[4] = (regs[4] & ~0x1FFFFFFF) | addr_src;
	regs[5] = (regs[5] & ~0x1FFFFFFF) | addr_dst;
}

static void s3c240x_dma_stop( running_machine *machine, int dma)
{
//	logerror( "DMA %d stop\n", dma);
}

static void s3c240x_dma_recalc( running_machine *machine, int dma)
{
	if (s3c240x_dma_regs[(dma<<3)+6] & 2)
	{
		s3c240x_dma_start( machine, dma);
	}
	else
	{
		s3c240x_dma_stop( machine, dma);
	}
}

static READ32_HANDLER( s3c240x_dma_r )
{
	UINT32 data = s3c240x_dma_regs[offset];
//	logerror( "(DMA) %08X -> %08X (PC %08X)\n", 0x14600000 + (offset << 2), data, cpu_get_pc( space->cpu));
	return data;
}

static WRITE32_HANDLER( s3c240x_dma_w )
{
	UINT32 old_value = s3c240x_dma_regs[offset];
//	logerror( "(DMA) %08X <- %08X (PC %08X)\n", 0x14600000 + (offset << 2), data, cpu_get_pc( space->cpu));
	COMBINE_DATA(&s3c240x_dma_regs[offset]);
	switch (offset)
	{
		// DCON0
		case 0x08 / 4 :
		{
			if (((data >> 22) & 1) != 0) // reload
			{
				s3c240x_dma_regs[0x18/4] &= ~(1 << 1); // clear on/off
			}
		}
		break;
		// DMASKTRIG0
		case 0x18 / 4 :
		{
			if ((old_value & 2) != (data & 2)) s3c240x_dma_recalc( space->machine, 0);
		}
		break;
		// DCON1
		case 0x28 / 4 :
		{
			if (((data >> 22) & 1) != 0) // reload
			{
				s3c240x_dma_regs[0x38/4] &= ~(1 << 1); // clear on/off
			}
		}
		break;
		// DMASKTRIG1
		case 0x38 / 4 :
		{
			if ((old_value & 2) != (data & 2)) s3c240x_dma_recalc( space->machine, 1);
		}
		break;
		// DCON2
		case 0x48 / 4 :
		{
			if (((data >> 22) & 1) != 0) // reload
			{
				s3c240x_dma_regs[0x58/4] &= ~(1 << 1); // clear on/off
			}
		}
		break;
		// DMASKTRIG2
		case 0x58 / 4 :
		{
			if ((old_value & 2) != (data & 2)) s3c240x_dma_recalc( space->machine, 2);
		}
		break;
		// DCON3
		case 0x68 / 4 :
		{
			if (((data >> 22) & 1) != 0) // reload
			{
				s3c240x_dma_regs[0x78/4] &= ~(1 << 1); // clear on/off
			}
		}
		break;
		// DMASKTRIG3
		case 0x78 / 4 :
		{
			if ((old_value & 2) != (data & 2)) s3c240x_dma_recalc( space->machine, 3);
		}
		break;
	}
}

static TIMER_CALLBACK( s3c240x_dma_timer_exp )
{
//	int ch = param;
//	logerror( "DMA %d timer callback\n", ch);
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

static void smc_reset( running_machine *machine)
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

static void smc_init( running_machine *machine)
{
//	logerror( "smc_init\n");
	smc_reset( machine);
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
		smc_reset( machine);
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

// I2S

#define I2S_L3C ( 1 )
#define I2S_L3M ( 2 )
#define I2S_L3D ( 3 )

static struct {
	int l3d;
	int l3m;
	int l3c;
} i2s;

static void i2s_reset( running_machine *machine)
{
//	logerror( "i2s_reset\n");
	i2s.l3d = 0;
	i2s.l3m = 0;
	i2s.l3c = 0;
}

static void i2s_init( running_machine *machine)
{
//	logerror( "i2s_init\n");
	i2s_reset( machine);
}

static void i2s_write( running_machine *machine, int line, int data)
{
	switch (line)
	{
		case I2S_L3C :
		{
			if (data != i2s.l3c)
			{
//				logerror( "I2S L3C %d\n", data);
				i2s.l3c = data;
			}
		}
		break;
		case I2S_L3M :
		{
			if (data != i2s.l3m)
			{
//				logerror( "I2S L3M %d\n", data);
				i2s.l3m = data;
			}
		}
		break;
		case I2S_L3D :
		{
			if (data != i2s.l3d)
			{
//				logerror( "I2S L3D %d\n", data);
				i2s.l3d = data;
			}
		}
		break;
	}
}

// I/O PORT

static UINT32 s3c240x_gpio[0x60/4];

static READ32_HANDLER( s3c240x_gpio_r )
{
	UINT32 data = s3c240x_gpio[offset];
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
//	logerror( "(GPIO) %08X -> %08X (PC %08X)\n", 0x15600000 + (offset << 2), data, cpu_get_pc( space->cpu));
	return data;
}

static WRITE32_HANDLER( s3c240x_gpio_w )
{
//	UINT32 old_value = s3c240x_gpio_regs[offset];
	COMBINE_DATA(&s3c240x_gpio[offset]);
//	logerror( "(GPIO) %08X <- %08X (PC %08X)\n", 0x15600000 + (offset << 2), data, cpu_get_pc( space->cpu));
	switch (offset)
	{
		// PBCON
		case 0x08 / 4 :
		{
			// smartmedia
			smc.read = ((data & 0x00000001) == 0);
			smc_update( space->machine);
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
			smc.do_read = ((data & 0x00000100) == 0);
			smc.chip = ((data & 0x00000080) == 0);
			smc.wp = ((data & 0x00000040) == 0);
			smc_update( space->machine);
		}
		break;
		// PEDAT
		case 0x30 / 4 :
		{
			// smartmedia
			smc.cmd_latch = ((data & 0x00000020) != 0);
			smc.add_latch = ((data & 0x00000010) != 0);
			smc.do_write  = ((data & 0x00000008) == 0);
			smc_update( space->machine);
			// sound
			i2s_write( space->machine, I2S_L3D, (data & 0x00000800) ? 1 : 0);
			i2s_write( space->machine, I2S_L3M, (data & 0x00000400) ? 1 : 0);
			i2s_write( space->machine, I2S_L3C, (data & 0x00000200) ? 1 : 0);
		}
		break;
/*
		// PGDAT
		case 0x48 / 4 :
		{
			int i2ssdo;
			i2ssdo = BIT( data, 3);
		}
		break;
*/
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
//	logerror( "(MEMCON) %08X <- %08X (PC %08X)\n", 0x14000000 + (offset << 2), data, cpu_get_pc( space->cpu));
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
//	logerror( "(USB H) %08X <- %08X (PC %08X)\n", 0x14200000 + (offset << 2), data, cpu_get_pc( space->cpu));
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
			data = (data & ~0x00000006) | 0x00000004 | 0x00000002; // [bit 2] Transmitter empty / [bit 1] Transmit buffer empty
		}
		break;
	}
//	logerror( "(UART 0) %08X -> %08X (PC %08X)\n", 0x15000000 + (offset << 2), data, cpu_get_pc( space->cpu));
	return data;
}

static WRITE32_HANDLER( s3c240x_uart_0_w )
{
//	logerror( "(UART 0) %08X <- %08X (PC %08X)\n", 0x15000000 + (offset << 2), data, cpu_get_pc( space->cpu));
	COMBINE_DATA(&s3c240x_uart_0_regs[offset]);
}

// UART 1

static UINT32 s3c240x_uart_1_regs[0x2C/4];

static READ32_HANDLER( s3c240x_uart_1_r )
{
	UINT32 data = s3c240x_uart_1_regs[offset];
	switch (offset)
	{
		// UTRSTAT1
		case 0x10 / 4 :
		{
			data = (data & ~0x00000006) | 0x00000004 | 0x00000002; // [bit 2] Transmitter empty / [bit 1] Transmit buffer empty
		}
		break;
	}
//	logerror( "(UART 1) %08X -> %08X (PC %08X)\n", 0x15004000 + (offset << 2), data, cpu_get_pc( space->cpu));
	return data;
}

static WRITE32_HANDLER( s3c240x_uart_1_w )
{
//	logerror( "(UART 1) %08X <- %08X (PC %08X)\n", 0x15004000 + (offset << 2), data, cpu_get_pc( space->cpu));
	COMBINE_DATA(&s3c240x_uart_1_regs[offset]);
}

// USB DEVICE

static UINT32 s3c240x_usb_device_regs[0xBC/4];

static READ32_HANDLER( s3c240x_usb_device_r )
{
	UINT32 data = s3c240x_usb_device_regs[offset];
//	logerror( "(USB D) %08X -> %08X (PC %08X)\n", 0x15200140 + (offset << 2), data, cpu_get_pc( space->cpu));
	return data;
}

static WRITE32_HANDLER( s3c240x_usb_device_w )
{
//	logerror( "(USB D) %08X <- %08X (PC %08X)\n", 0x15200140 + (offset << 2), data, cpu_get_pc( space->cpu));
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
//	logerror( "(WDOG) %08X <- %08X (PC %08X)\n", 0x15300000 + (offset << 2), data, cpu_get_pc( space->cpu));
	COMBINE_DATA(&s3c240x_watchdog_regs[offset]);
}

// IIC

static emu_timer *s3c240x_iic_timer;
static UINT32 s3c240x_iic_regs[0x10/4];
static UINT8 s3c240x_iic_data[3];
static int s3c240x_iic_data_index = 0;
static UINT16 s3c240x_iic_addr = 0;

/*
static UINT8 i2cmem_read_byte( running_machine *machine, int last)
{
	UINT8 data = 0;
	int i;
	i2cmem_write( machine, 0, I2CMEM_SDA, 1);
	for (i=0;i<8;i++)
	{
		i2cmem_write( machine, 0, I2CMEM_SCL, 1);
    data = (data << 1) + (i2cmem_read( machine, 0, I2CMEM_SDA) ? 1 : 0);
		i2cmem_write( machine, 0, I2CMEM_SCL, 0);
	}
	i2cmem_write( machine, 0, I2CMEM_SDA, last);
	i2cmem_write( machine, 0, I2CMEM_SCL, 1);
	i2cmem_write( machine, 0, I2CMEM_SCL, 0);
	return data;
}
*/

/*
static void i2cmem_write_byte( running_machine *machine, UINT8 data)
{
	int i;
	for (i=0;i<8;i++)
	{
		i2cmem_write( machine, 0, I2CMEM_SDA, (data & 0x80) ? 1 : 0);
		data = data << 1;
		i2cmem_write( machine, 0, I2CMEM_SCL, 1);
		i2cmem_write( machine, 0, I2CMEM_SCL, 0);
	}
	i2cmem_write( machine, 0, I2CMEM_SDA, 1); // ack bit
	i2cmem_write( machine, 0, I2CMEM_SCL, 1);
	i2cmem_write( machine, 0, I2CMEM_SCL, 0);
}
*/

/*
static void i2cmem_start( running_machine *machine)
{
	i2cmem_write( machine, 0, I2CMEM_SDA, 1);
	i2cmem_write( machine, 0, I2CMEM_SCL, 1);
	i2cmem_write( machine, 0, I2CMEM_SDA, 0);
	i2cmem_write( machine, 0, I2CMEM_SCL, 0);
}
*/

/*
static void i2cmem_stop( running_machine *machine)
{
	i2cmem_write( machine, 0, I2CMEM_SDA, 0);
	i2cmem_write( machine, 0, I2CMEM_SCL, 1);
	i2cmem_write( machine, 0, I2CMEM_SDA, 1);
	i2cmem_write( machine, 0, I2CMEM_SCL, 0);
}
*/

static void iic_start( running_machine *machine)
{
//	logerror( "IIC start\n");
	s3c240x_iic_data_index = 0;
	timer_adjust_oneshot( s3c240x_iic_timer, ATTOTIME_IN_MSEC( 1), 0);
}

static void iic_stop( running_machine *machine)
{
//	logerror( "IIC stop\n");
	timer_adjust_oneshot( s3c240x_iic_timer, attotime_never, 0);
}

static void iic_resume( running_machine *machine)
{
//	logerror( "IIC resume\n");
	timer_adjust_oneshot( s3c240x_iic_timer, ATTOTIME_IN_MSEC( 1), 0);
}

static READ32_HANDLER( s3c240x_iic_r )
{
	UINT32 data = s3c240x_iic_regs[offset];
	switch (offset)
	{
		// IICSTAT
		case 0x04 / 4 :
		{
			data = data & ~0x0000000F;
		}
		break;
	}
//	logerror( "(IIC) %08X -> %08X (PC %08X)\n", 0x15400000 + (offset << 2), data, cpu_get_pc( space->cpu));
	return data;
}

static WRITE32_HANDLER( s3c240x_iic_w )
{
//	logerror( "(IIC) %08X <- %08X (PC %08X)\n", 0x15400000 + (offset << 2), data, cpu_get_pc( space->cpu));
	COMBINE_DATA(&s3c240x_iic_regs[offset]);
	switch (offset)
	{
		// ADDR_IICCON
		case 0x00 / 4 :
		{
			int interrupt_pending_flag;
//			const int div_table[] = { 16, 512};
//			int enable_interrupt, transmit_clock_value, tx_clock_source_selection
//			double clock;
//			transmit_clock_value = (data >> 0) & 0xF;
//			tx_clock_source_selection = (data >> 6) & 1;
//			enable_interrupt = (data >> 5) & 1;
//			clock = (double)(s3c240x_get_pclk( MPLLCON) / div_table[tx_clock_source_selection] / (transmit_clock_value + 1));
			interrupt_pending_flag = BIT( data, 4);
			if (interrupt_pending_flag == 0)
			{
				int start_stop_condition;
				start_stop_condition = BIT( s3c240x_iic_regs[1], 5);
				if (start_stop_condition != 0)
				{
					iic_resume( space->machine);
				}
			}
		}
		break;
		// IICSTAT
		case 0x04 / 4 :
		{
			int start_stop_condition;
			start_stop_condition = BIT( data, 5);
			if (start_stop_condition != 0)
			{
				iic_start( space->machine);
			}
			else
			{
				iic_stop( space->machine);
			}
		}
		break;
	}
}

static TIMER_CALLBACK( s3c240x_iic_timer_exp )
{
	int enable_interrupt, mode_selection;
//	logerror( "IIC timer callback\n");
	mode_selection = BITS( s3c240x_iic_regs[1], 7, 6);
	switch (mode_selection)
	{
		// master receive mode
		case 2 :
		{
			if (s3c240x_iic_data_index == 0)
			{
//				UINT8 data_shift = s3c240x_iic_regs[3] & 0xFF;
//				logerror( "IIC write %02X\n", data_shift);
			}
			else
			{
				UINT8 data_shift = eeprom_data[s3c240x_iic_addr];
//				logerror( "IIC read %02X [%04X]\n", data_shift, s3c240x_iic_addr);
				s3c240x_iic_regs[3] = (s3c240x_iic_regs[3] & ~0xFF) | data_shift;
			}
			s3c240x_iic_data_index++;
		}
		break;
		// master transmit mode
		case 3 :
		{
			UINT8 data_shift = s3c240x_iic_regs[3] & 0xFF;
//			logerror( "IIC write %02X\n", data_shift);
			s3c240x_iic_data[s3c240x_iic_data_index++] = data_shift;
			if (s3c240x_iic_data_index == 3)
			{
				s3c240x_iic_addr = (s3c240x_iic_data[1] << 8) | s3c240x_iic_data[2];
			}
		}
		break;
	}
	enable_interrupt = BIT( s3c240x_iic_regs[0], 5);
	if (enable_interrupt)
	{
//		logerror( "IIC request irq\n");
		s3c240x_request_irq( machine, INT_IIC);
	}
}

// IIS

static emu_timer *s3c240x_iis_timer;
static UINT32 s3c240x_iis_regs[0x14/4];
static UINT16 s3c240x_iis_fifo[16/2];
static int s3c240x_iis_fifo_index = 0;

static void s3c240x_iis_start( running_machine *machine)
{
	const UINT32 codeclk_table[] = { 256, 384};
	double freq;
	int prescaler_enable, prescaler_control_a, prescaler_control_b, codeclk;
//	logerror( "IIS start\n");
	prescaler_enable = BIT( s3c240x_iis_regs[0], 1);
	prescaler_control_a = BITS( s3c240x_iis_regs[2], 9, 5);
	prescaler_control_b = BITS( s3c240x_iis_regs[2], 4, 0);
	codeclk = BIT( s3c240x_iis_regs[1], 2);
	freq = (double)(s3c240x_get_pclk( MPLLCON) / (prescaler_control_a + 1) / codeclk_table[codeclk]) * 2; // why do I have to multiply by two?
//	logerror( "IIS - pclk %d psc_enable %d psc_a %d psc_b %d codeclk %d freq %f\n", s3c240x_get_pclk( MPLLCON), prescaler_enable, prescaler_control_a, prescaler_control_b, codeclk_table[codeclk], freq);
	timer_adjust_periodic( s3c240x_iis_timer, ATTOTIME_IN_HZ( freq), 0, ATTOTIME_IN_HZ( freq));
}

static void s3c240x_iis_stop( running_machine *machine)
{
//	logerror( "IIS stop\n");
	timer_adjust_oneshot( s3c240x_iis_timer, attotime_never, 0);
}

static void s3c240x_iis_recalc( running_machine *machine)
{
	if (s3c240x_iis_regs[0] & 1)
	{
		s3c240x_iis_start( machine);
	}
	else
	{
		s3c240x_iis_stop( machine);
	}
}

static READ32_HANDLER( s3c240x_iis_r )
{
	UINT32 data = s3c240x_iis_regs[offset];
//	logerror( "(IIS) %08X -> %08X (PC %08X)\n", 0x15508000 + (offset << 2), data, cpu_get_pc( space->cpu));
	return data;
}

static WRITE32_HANDLER( s3c240x_iis_w )
{
	UINT32 old_value = s3c240x_iis_regs[offset];
//	logerror( "(IIS) %08X <- %08X (PC %08X)\n", 0x15508000 + (offset << 2), data, cpu_get_pc( space->cpu));
	COMBINE_DATA(&s3c240x_iis_regs[offset]);
	switch (offset)
	{
		// IISCON
		case 0x00 / 4 :
		{
			if ((old_value & 1) != (data & 1)) s3c240x_iis_recalc( space->machine);
		}
		break;
		// IISFIF
		case 0x10 / 4 :
		{
			if (ACCESSING_BITS_16_31)
			{
				s3c240x_iis_fifo[s3c240x_iis_fifo_index++] = (data >> 16) & 0xFFFF;
			}
			if (ACCESSING_BITS_0_15)
			{
				s3c240x_iis_fifo[s3c240x_iis_fifo_index++] = data & 0xFFFF;
			}
			if (s3c240x_iis_fifo_index == 2)
			{
				const device_config *dac[2];
				dac[0] = devtag_get_device( space->machine, "dac1");
				dac[1] = devtag_get_device( space->machine, "dac2");
				s3c240x_iis_fifo_index = 0;
    		dac_signed_data_16_w( dac[0], s3c240x_iis_fifo[0] + 0x8000);
    		dac_signed_data_16_w( dac[1], s3c240x_iis_fifo[1] + 0x8000);
			}
		}
		break;
	}
}

static TIMER_CALLBACK( s3c240x_iis_timer_exp )
{
	UINT32 dcon;
	int hwsrcsel, swhwsel;
//	logerror( "IIS timer callback\n");
	dcon = s3c240x_dma_regs[0x48/4];
	hwsrcsel = BITS( dcon, 25, 24);
	swhwsel = BIT( dcon, 23);
	if ((swhwsel == 1) && (hwsrcsel == 0))
	{
		UINT32 dmasktrig;
		int on_off;
		dmasktrig = s3c240x_dma_regs[0x58/4];
		on_off = BIT( dmasktrig, 1);
		if (on_off)
		{
			s3c240x_dma_trigger( machine, 2);
		}
	}
}

// RTC

static UINT32 s3c240x_rtc_regs[0x4C/4];

static READ32_HANDLER( s3c240x_rtc_r )
{
	return s3c240x_rtc_regs[offset];
}

static WRITE32_HANDLER( s3c240x_rtc_w )
{
//	logerror( "(RTC) %08X <- %08X (PC %08X)\n", 0x15700040 + (offset << 2), data, cpu_get_pc( space->cpu));
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
//	logerror( "(ADC) %08X <- %08X (PC %08X)\n", 0x15800000 + (offset << 2), data, cpu_get_pc( space->cpu));
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
//	logerror( "(SPI) %08X <- %08X (PC %08X)\n", 0x15900000 + (offset << 2), data, cpu_get_pc( space->cpu));
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
	s3c240x_pwm_timer[0] = timer_alloc(machine, s3c240x_pwm_timer_exp, (void *)(FPTR)0);
	s3c240x_pwm_timer[1] = timer_alloc(machine, s3c240x_pwm_timer_exp, (void *)(FPTR)1);
	s3c240x_pwm_timer[2] = timer_alloc(machine, s3c240x_pwm_timer_exp, (void *)(FPTR)2);
	s3c240x_pwm_timer[3] = timer_alloc(machine, s3c240x_pwm_timer_exp, (void *)(FPTR)3);
	s3c240x_pwm_timer[4] = timer_alloc(machine, s3c240x_pwm_timer_exp, (void *)(FPTR)4);
	s3c240x_dma_timer[0] = timer_alloc( machine, s3c240x_dma_timer_exp, (void *)(FPTR)0);
	s3c240x_dma_timer[1] = timer_alloc( machine, s3c240x_dma_timer_exp, (void *)(FPTR)1);
	s3c240x_dma_timer[2] = timer_alloc( machine, s3c240x_dma_timer_exp, (void *)(FPTR)2);
	s3c240x_dma_timer[3] = timer_alloc( machine, s3c240x_dma_timer_exp, (void *)(FPTR)3);
	s3c240x_iic_timer = timer_alloc(machine, s3c240x_iic_timer_exp, (void *)(FPTR)0);
	s3c240x_iis_timer = timer_alloc(machine, s3c240x_iis_timer_exp, (void *)(FPTR)0);
	eeprom_data = auto_alloc_array( machine, UINT8, 0x2000);
	smc_init( machine);
	i2s_init( machine);
}

static void s3c240x_machine_reset(running_machine *machine)
{
	smc_reset( machine);
	i2s_reset( machine);
}

static NVRAM_HANDLER( gp32 )
{
	if (read_or_write)
	{
		if (file)
		{
			mame_fwrite( file, eeprom_data, 0x2000);
		}
	}
	else
	{
		if (file)
		{
			mame_fread( file, eeprom_data, 0x2000);
		}
		else
		{
			memset( eeprom_data, 0, 0x2000);
		}
	}
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
	AM_RANGE(0x15100000, 0x15100043) AM_READWRITE(s3c240x_pwm_r, s3c240x_pwm_w)
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
	MDRV_SOUND_ADD("dac1", DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "lspeaker", 1.0)
	MDRV_SOUND_ADD("dac2", DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "rspeaker", 1.0)

	MDRV_NVRAM_HANDLER(gp32)

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
