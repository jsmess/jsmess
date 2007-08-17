/******************************************************************************
 * 
 *	Acorn Archimedes 310
 *
 *	Skeleton: Juergen Buchmueller <pullmoll@t-online.de>, Jul 2000
 *	Enhanced: R. Belmont, June 2007
 *
 *      Memory map (from http://b-em.bbcmicro.com/arculator/archdocs.txt)
 *
 *	0000000 - 1FFFFFF - logical RAM (32 meg)
 *	2000000 - 2FFFFFF - physical RAM (supervisor only - max 16MB - requires quad MEMCs)
 *	3000000 - 33FFFFF - IOC (IO controllers - supervisor only)
 *	3310000 - FDC - WD1772
 *	33A0000 - Econet - 6854
 *	33B0000 - Serial - 6551
 *	3240000 - 33FFFFF - internal expansion cards
 *	32D0000 - hard disc controller (not IDE) - HD63463
 *	3350010 - printer
 *	3350018 - latch A
 *	3350040 - latch B
 *	3270000 - external expansion cards
 *
 *	3400000 - 3FFFFFF - ROM (read - 12 meg - Arthur and RiscOS 2 512k, RiscOS 3 2MB)
 *	3400000 - 37FFFFF - Low ROM  (4 meg, I think this is expansion ROMs)
 *	3800000 - 3FFFFFF - High ROM (main OS ROM)
 *
 *	3400000 - 35FFFFF - VICD10 (write - supervisor only)
 *	3600000 - 3FFFFFF - MEMC (write - supervisor only)
 *
 *****************************************************************************/

#include "driver.h"
#include "machine/wd17xx.h"
#include "video/generic.h"
#include "devices/basicdsk.h"
#include "cpu/arm/arm.h"
#include "sound/dac.h"
#include "image.h"

#ifndef VERBOSE
#define VERBOSE 0
#endif

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)	/* x */
#endif

// interrupt register stuff
#define A310_IRQA_PRINTER_BUSY (0x01)
#define A310_IRQA_SERIAL_RING  (0x02)
#define A310_IRQA_PRINTER_ACK  (0x04)
#define A310_IRQA_VBL	       (0x08)
#define A310_IRQA_RESET        (0x10)
#define A310_IRQA_TIMER0       (0x20)
#define A310_IRQA_TIMER1       (0x40)
#define A310_IRQA_ALWAYS       (0x80)

#define A310_IRQB_PODULE_FIQ   (0x01)
#define A310_IRQB_SOUND_EMPTY  (0x02)
#define A310_IRQB_SERIAL       (0x04)
#define A310_IRQB_HDD	       (0x08)
#define A310_IRQB_DISC_CHANGE  (0x10)
#define A310_IRQB_PODULE_IRQ   (0x20)
#define A310_IRQB_KBD_XMIT_EMPTY  (0x40)
#define A310_IRQB_KBD_RECV_FULL   (0x80)

#define A310_FIQ_FLOPPY_DRQ    (0x01)
#define A310_FIQ_FLOPPY        (0x02)
#define A310_FIQ_ECONET        (0x04)
#define A310_FIQ_PODULE        (0x40)
#define A310_FIQ_FORCE         (0x80)

static int page_sizes[4] = { 4096, 8192, 16384, 32768 };

static UINT32 *a310_physmem;
static UINT32 a310_pagesize;
static int a310_latchrom;
static INT16 a310_pages[(32*1024*1024)/(4096)];	// the logical RAM area is 32 megs, and the smallest page size is 4k
static UINT32 a310_vidregs[256];
static UINT8 a310_iocregs[0x80/4];
static UINT32 a310_timercnt[4];
static UINT32 a310_sndstart, a310_sndend, a310_sndcur;

static mame_timer *vbl_timer, *timer[4], *snd_timer;

VIDEO_START( a310 )
{
}

VIDEO_UPDATE( a310 )
{
	return 0;
}

static void a310_request_irq_a(int mask)
{
	a310_iocregs[4] |= mask;

	if (a310_iocregs[6] & mask)
	{
		cpunum_set_input_line(0, ARM_IRQ_LINE, ASSERT_LINE);
	}
}

static void a310_request_irq_b(int mask)
{
	a310_iocregs[8] |= mask;

	if (a310_iocregs[10] & mask)
	{
		cpunum_set_input_line(0, ARM_IRQ_LINE, PULSE_LINE);
	}
}

static void a310_request_fiq(int mask)
{
	a310_iocregs[12] |= mask;

	if (a310_iocregs[14] & mask)
	{
		cpunum_set_input_line(0, ARM_FIRQ_LINE, PULSE_LINE);
	}
}

static TIMER_CALLBACK( a310_audio_tick )
{
	a310_sndcur++;

	if (a310_sndcur >= a310_sndend)
	{
		a310_request_irq_b(A310_IRQB_SOUND_EMPTY);
	}	
}

static TIMER_CALLBACK( a310_vblank )
{
	a310_request_irq_a(A310_IRQA_VBL);

	// set up for next vbl
	mame_timer_adjust(vbl_timer, video_screen_get_time_until_pos(0, a310_vidregs[0xb4], 0), 0, time_never);
}

static void a310_set_timer(int tmr)
{
	double freq = 2000000.0 / (double)a310_timercnt[tmr];

//	logerror("IOC: starting timer %d, %d ticks, freq %f Hz\n", tmr, a310_timercnt[tmr], freq);

	mame_timer_adjust(timer[tmr], MAME_TIME_IN_HZ(freq), tmr, time_never);
}

// param
static TIMER_CALLBACK( a310_timer )
{
	// all timers always run
	a310_set_timer(param);

	// but only timers 0 and 1 generate IRQs	
	switch (param)
	{
		case 0:
			a310_request_irq_a(A310_IRQA_TIMER0);
			break;

		case 1:
			a310_request_irq_a(A310_IRQA_TIMER1);
			break;
	}	
}

static void a310_memc_reset(void)
{
	int i;

	a310_latchrom = 1;			// map in the boot ROM

	// kill all memc mappings
	for (i = 0; i < (32*1024*1024)/(4096); i++)
	{
		a310_pages[i] = -1;		// indicate unmapped
	}
}

static MACHINE_RESET( a310 )
{
	a310_memc_reset();
}

static void a310_wd177x_callback(wd17xx_state_t event, void *param)
{
	switch (event)
	{
		case WD17XX_IRQ_CLR:
			a310_iocregs[12] &= ~A310_FIQ_FLOPPY;
			break;

		case WD17XX_IRQ_SET:
			a310_request_fiq(A310_FIQ_FLOPPY);
			break;
			 
		case WD17XX_DRQ_CLR:
			a310_iocregs[12] &= ~A310_FIQ_FLOPPY_DRQ;
			break;

		case WD17XX_DRQ_SET:
			a310_request_fiq(A310_FIQ_FLOPPY_DRQ);
			break;
	}
}


static MACHINE_START( a310 )
{
	a310_pagesize = 0;
	wd17xx_init(WD_TYPE_1772, a310_wd177x_callback, NULL);

	vbl_timer = mame_timer_alloc(a310_vblank);
	mame_timer_adjust(vbl_timer, time_never, 0, time_never);

	timer[0] = mame_timer_alloc(a310_timer);
	timer[1] = mame_timer_alloc(a310_timer);
	timer[2] = mame_timer_alloc(a310_timer);
	timer[3] = mame_timer_alloc(a310_timer);
	mame_timer_adjust(timer[0], time_never, 0, time_never);
	mame_timer_adjust(timer[1], time_never, 0, time_never);
	mame_timer_adjust(timer[2], time_never, 0, time_never);
	mame_timer_adjust(timer[3], time_never, 0, time_never);

	snd_timer = mame_timer_alloc(a310_audio_tick);
	mame_timer_adjust(snd_timer, time_never, 0, time_never);

	// reset the DAC to centerline
	DAC_signed_data_w(0, 0x80);
}

static READ32_HANDLER(logical_r)
{
	UINT32 page, poffs;
	
	// are we mapping in the boot ROM?
	if (a310_latchrom)
	{
		UINT32 *rom;
		
		rom = (UINT32 *)memory_region(REGION_CPU1);
		
		return rom[offset & 0x1fffff];		
	}
	else
	{
		// figure out the page number and offset in the page
		page = (offset<<2) / page_sizes[a310_pagesize]; 
		poffs = (offset<<2) % page_sizes[a310_pagesize];

//		printf("Reading offset %x (addr %x): page %x (size %d %d) offset %x ==> %x %x\n", offset, offset<<2, page, a310_pagesize, page_sizes[a310_pagesize], poffs, a310_pages[page], a310_pages[page]*page_sizes[a310_pagesize]);

		if (a310_pages[page] != -1)
		{
			return a310_physmem[((a310_pages[page] * page_sizes[a310_pagesize]) + poffs)>>2];
		}
		else
		{
			printf("A310_MEMC: Reading unmapped page, what do we do?\n");
		}
	}

	return 0;
}

static WRITE32_HANDLER(logical_w)
{
	UINT32 page, poffs;

	// if the boot ROM is mapped, ignore writes
	if (a310_latchrom)
	{
		return;
	}
	else
	{
		// figure out the page number and offset in the page
		page = (offset<<2) / page_sizes[a310_pagesize]; 
		poffs = (offset<<2) % page_sizes[a310_pagesize];

//		printf("Writing offset %x (addr %x): page %x (size %d %d) offset %x ==> %x %x\n", offset, offset<<2, page, a310_pagesize, page_sizes[a310_pagesize], poffs, a310_pages[page], a310_pages[page]*page_sizes[a310_pagesize]);

		if (a310_pages[page] != -1)
		{
			COMBINE_DATA(&a310_physmem[((a310_pages[page] * page_sizes[a310_pagesize]) + poffs)>>2]);
		}
		else
		{
			printf("A310_MEMC: Writing unmapped page, what do we do?\n");
		}
	}
}

static OPBASE_HANDLER( a310_setopbase )
{
	// if we're not in logical memory, MAME can do the right thing
	if (address > 0x1ffffff)
	{
		return address;
	}

	// if the boot ROM is mapped in, do some trickery to make it show up
	if (a310_latchrom)
	{
		opcode_mask = 0x1fffff;
		opcode_memory_min = 0;
		opcode_memory_max = 0x1fffff;
		opcode_base = opcode_arg_base = memory_region(REGION_CPU1);		
	}
	else	// executing from logical memory
	{	
		UINT32 page = address / page_sizes[a310_pagesize];  

		opcode_mask = page_sizes[a310_pagesize]-1;
		opcode_memory_min = page * page_sizes[a310_pagesize];
		opcode_memory_max = opcode_memory_min + opcode_mask;
		opcode_base = opcode_arg_base = (UINT8 *)&a310_physmem[(a310_pages[page] * page_sizes[a310_pagesize])>>2];
	}

	return ~0;
}

static DRIVER_INIT(a310)
{
	memory_set_opbase_handler(0, a310_setopbase);
}

static const char *ioc_regnames[] = 
{
	"(rw) Control",					// 0
	"(read) Keyboard receive (write) keyboard send",	// 1
	"?",
	"?",
	"(read) IRQ status A",			   	// 4
	"(read) IRQ request A (write) IRQ clear",	// 5
	"(rw) IRQ mask A",				// 6
	"?",
	"(read) IRQ status B",		// 8
	"(read) IRQ request B",		// 9
	"(rw) IRQ mask B",		// 10
	"?",
	"(read) FIQ status",		// 12
	"(read) FIQ request",		// 13
	"(rw) FIQ mask",		// 14
	"?",
	"(read) Timer 0 count low (write) Timer 0 latch low",		// 16
	"(read) Timer 0 count high (write) Timer 0 latch high",		// 17
	"(write) Timer 0 go command",					// 18
	"(write) Timer 0 latch command",				// 19
	"(read) Timer 1 count low (write) Timer 1 latch low",		// 20
	"(read) Timer 1 count high (write) Timer 1 latch high",		// 21
	"(write) Timer 1 go command",					// 22
	"(write) Timer 1 latch command",				// 23
	"(read) Timer 2 count low (write) Timer 2 latch low",		// 24
	"(read) Timer 2 count high (write) Timer 2 latch high",		// 25
	"(write) Timer 2 go command",					// 26
	"(write) Timer 2 latch command",				// 27
	"(read) Timer 3 count low (write) Timer 3 latch low",		// 28
	"(read) Timer 3 count high (write) Timer 3 latch high",		// 29
	"(write) Timer 3 go command",					// 30
	"(write) Timer 3 latch command"					// 31
};

static void update_timer_cnt(int tmr)
{
	double time;
	UINT32 cnt;

	time = mame_time_to_double(mame_timer_timeelapsed(timer[tmr]));
	time *= 2000000.0;	// find out how many 2 MHz ticks have gone by
	cnt = a310_timercnt[tmr] - (UINT32)time;
	a310_iocregs[16+(4*tmr)] = cnt&0xff;
	a310_iocregs[17+(4*tmr)] = (cnt>>8)&0xff;
}

static READ32_HANDLER(ioc_r)
{
	if (offset >= 0x80000 && offset < 0xc0000)
	{
		switch (offset & 0x1f)
		{
			case 1:	// keyboard read
				a310_request_irq_b(A310_IRQB_KBD_XMIT_EMPTY);
				break;

			case 16:	// timer 0 read
			case 17:
				update_timer_cnt(0);
				break;
			case 20:	// timer 1 read
			case 21:
				update_timer_cnt(1);
				break;
			case 24:	// timer 2 read
			case 25:
				update_timer_cnt(2);
				break;
			case 28:	// timer 3 read
			case 29:
				update_timer_cnt(3);
				break;
		}

		logerror("IOC: R %s = %02x (PC=%x)\n", ioc_regnames[offset&0x1f], a310_iocregs[offset&0x1f], activecpu_get_pc());
		return a310_iocregs[offset&0x1f];
	}
	else if (offset >= 0xc4000 && offset <= 0xc4010)
	{
		logerror("17XX: R @ addr %x mask %08x\n", offset*4, mem_mask);
		return wd17xx_data_r(offset&0xf);
	}
	else
	{
		logerror("I/O: R @ %x (mask %08x)\n", (offset*4)+0x3000000, mem_mask);
	}


	return 0;
}

static WRITE32_HANDLER(ioc_w)
{
	if (offset >= 0x80000 && offset < 0xc0000)
	{
		logerror("IOC: W %02x @ reg %s (PC=%x)\n", data&0xff, ioc_regnames[offset&0x1f], activecpu_get_pc());

		switch (offset&0x1f)
		{
			case 5: 	// IRQ clear A
				a310_iocregs[4] &= ~(data&0xff);

				// if that did it, clear the IRQ
				if (a310_iocregs[4] == 0)
				{
					cpunum_set_input_line(0, ARM_IRQ_LINE, CLEAR_LINE);
				}
				break;

			case 16:
			case 17:
				a310_iocregs[offset&0x1f] = data & 0xff;
				a310_timercnt[0] = a310_iocregs[17]<<8 | a310_iocregs[16];
				break;

			case 20:
			case 21:
				a310_iocregs[offset&0x1f] = data & 0xff;
				a310_timercnt[1] = a310_iocregs[21]<<8 | a310_iocregs[20];
				break;

			case 24:
			case 25:
				a310_iocregs[offset&0x1f] = data & 0xff;
				a310_timercnt[2] = a310_iocregs[25]<<8 | a310_iocregs[24];
				break;

			case 28:
			case 29:
				a310_iocregs[offset&0x1f] = data & 0xff;
				a310_timercnt[3] = a310_iocregs[29]<<8 | a310_iocregs[28];
				break;

			case 19:	// Timer 0 latch
				a310_timercnt[0] = a310_iocregs[17]<<8 | a310_iocregs[16];
				break;

			case 23:	// Timer 1 latch
				a310_timercnt[1] = a310_iocregs[21]<<8 | a310_iocregs[20];
				break;

			case 27:	// Timer 2 latch
				a310_timercnt[2] = a310_iocregs[25]<<8 | a310_iocregs[24];
				break;

			case 31:	// Timer 3 latch
				a310_timercnt[3] = a310_iocregs[29]<<8 | a310_iocregs[28];
				break;

			case 18:	// Timer 0 start
				a310_set_timer(0);
				break;

			case 22:	// Timer 1 start
				a310_set_timer(1);
				break;

			case 26:	// Timer 2 start
				a310_set_timer(2);
				break;

			case 30:	// Timer 3 start
				a310_set_timer(3);
				break;

			default:
				a310_iocregs[offset&0x1f] = data & 0xff;
				break;
		}
	}
	else if (offset >= 0xc4000 && offset <= 0xc4010)
	{
		logerror("17XX: %x to addr %x mask %08x\n", data, offset*4, mem_mask);
		wd17xx_data_w(offset&0xf, data&0xff);
	}
	else if (offset == 0xd40006)
	{
		// latch A
		if (data & 1)
		{
			wd17xx_set_drive(0);
		}
		if (data & 2)
		{
			wd17xx_set_drive(1);
		}
		if (data & 4)
		{
			wd17xx_set_drive(2);
		}
		if (data & 8)
		{
			wd17xx_set_drive(3);
		}

		wd17xx_set_side((data & 0x10)>>4);

	}
	else if (offset == 0xd40010)
	{
		// latch B
		wd17xx_set_density((data & 2) ? DEN_MFM_LO : DEN_MFM_HI);
	}
	else
	{
		logerror("I/O: W %x @ %x (mask %08x)\n", data, (offset*4)+0x3000000, mem_mask);
	}
}

static READ32_HANDLER(vidc_r)
{
	return 0;
}

static WRITE32_HANDLER(vidc_w)
{
	UINT32 reg = data>>24;
	UINT32 val = data & 0xffffff;
	static const char *vrnames[] = 
	{
		"horizontal total",
		"horizontal sync width",
		"horizontal border start",
		"horizontal display start",
		"horizontal display end",
		"horizontal border end",
		"horizontal cursor start",
		"horizontal interlace",
		"vertical total",
		"vertical sync width",
		"vertical border start",
		"vertical display start",
		"vertical display end",
		"vertical border end",
		"vertical cursor start",
		"vertical cursor end",
	};

	if (reg >= 0x80 && reg <= 0xbc)
	{
		logerror("VIDC: %s = %d\n", vrnames[(reg-0x80)/4], val>>12);

		if ((reg == 0xb0) & ((val>>12) != 0))
		{
			rectangle visarea;

			visarea.min_x = 0;
			visarea.min_y = 0;
			visarea.max_x = a310_vidregs[0x94] - a310_vidregs[0x88];
			visarea.max_y = a310_vidregs[0xb4] - a310_vidregs[0xa8];

			logerror("Configuring: htotal %d vtotal %d vis %d,%d\n",
				a310_vidregs[0x80], a310_vidregs[0xa0],
				visarea.max_x, visarea.max_y);

			video_screen_configure(0, a310_vidregs[0x80], a310_vidregs[0xa0], &visarea, Machine->screen[0].refresh);

			// slightly hacky: fire off a VBL right now.  the BIOS doesn't wait long enough otherwise.
			mame_timer_adjust(vbl_timer, time_zero, 0, time_never);
		}
	
		a310_vidregs[reg] = val>>12;
	}
	else
	{
		logerror("VIDC: %x to register %x\n", val, reg);
		a310_vidregs[reg] = val&0xffff;
	}
}

static READ32_HANDLER(memc_r)
{
	return 0;
}

static WRITE32_HANDLER(memc_w)
{
	// is it a register?
	if ((data & 0x0fe00000) == 0x03600000)
	{
		switch ((data >> 17) & 7)
		{
			case 4:	/* sound start */
				a310_sndstart = ((data>>2)&0x7fff)*16;
				break;

			case 5: /* sound end */
				a310_sndend = ((data>>2)&0x7fff)*16;
				break;

			case 7:	/* Control */
				a310_pagesize = ((data>>2) & 3);

				logerror("MEMC: %x to Control (page size %d, %s, %s)\n", data & 0x1ffc, page_sizes[a310_pagesize], ((data>>10)&1) ? "Video DMA on" : "Video DMA off", ((data>>11)&1) ? "Sound DMA on" : "Sound DMA off");

				if ((data>>11)&1)
				{
					double sndhz;

					sndhz = 250000.0 / (double)((a310_vidregs[0xc0]&0xff)+2);

					logerror("MEMC: Starting audio DMA at %f Hz, buffer from %x to %x\n", sndhz, a310_sndstart, a310_sndend);

					a310_sndcur = a310_sndstart;

					mame_timer_adjust(snd_timer, MAME_TIME_IN_HZ(sndhz), 0, MAME_TIME_IN_HZ(sndhz));
				}
				else
				{
					mame_timer_adjust(snd_timer, time_never, 0, time_never);
					DAC_signed_data_w(0, 0x80);
				}
				break;

			default:
				logerror("MEMC: %x to Unk reg %d\n", data&0x1ffff, (data >> 17) & 7);
				break;
		}
	}
	else
	{
		logerror("MEMC non-reg: W %x @ %x (mask %08x)\n", data, offset, mem_mask);
	}	
}

/*
	  22 2222 1111 1111 1100 0000 0000
          54 3210 9876 5432 1098 7654 3210
4k  page: 11 1LLL LLLL LLLL LLAA MPPP PPPP
8k  page: 11 1LLL LLLL LLLM LLAA MPPP PPPP
16k page: 11 1LLL LLLL LLxM LLAA MPPP PPPP
32k page: 11 1LLL LLLL LxxM LLAA MPPP PPPP
	   3   8    2   9    0    f    f

L - logical page
P - physical page
A - access permissions
M - MEMC number (for machines with multiple MEMCs)

The logical page is encoded with bits 11+10 being the most significant bits
(in that order), and the rest being bit 22 down.

The physical page is encoded differently depending on the page size :

4k  page:   bits 6-0 being bits 6-0
8k  page:   bits 6-1 being bits 5-0, bit 0 being bit 6
16k page:   bits 6-2 being bits 4-0, bits 1-0 being bits 6-5
32k page:   bits 6-3 being bits 4-0, bit 0 being bit 4, bit 2 being bit 5, bit
            1 being bit 6
*/

static WRITE32_HANDLER(memc_page_w)
{
	UINT32 log, phys, memc, perms;

	perms = (data & 0x300)>>8;
	log = phys = memc = 0;

	switch (a310_pagesize)
	{
		case 0:
			phys = data & 0x7f;
			log = (data & 0xc00)>>10;
			log <<= 23;
			log |= (data & 0x7ff000);
			memc = (data & 0x80) ? 1 : 0;
			break;

		case 1:
			phys = ((data & 0x7f) >> 1) | (data & 1) ? 0x40 : 0; 
			log = (data & 0xc00)>>10;
			log <<= 23;
			log |= (data & 0x7fe000);
			memc = ((data & 0x80) ? 1 : 0) | ((data & 0x1000) ? 2 : 0);
			break;

		case 2:
			phys = ((data & 0x7f) >> 2) | ((data & 3) << 5);
			log = (data & 0xc00)>>10;
			log <<= 23;
			log |= (data & 0x7fc000);
			memc = ((data & 0x80) ? 1 : 0) | ((data & 0x1000) ? 2 : 0);
			break;

		case 3:
			phys = ((data & 0x7f) >> 3) | (data & 1)<<4 | (data & 2) << 5 | (data & 4)<<3;
			log = (data & 0xc00)>>10;
			log <<= 23;
			log |= (data & 0x7f8000);
			memc = ((data & 0x80) ? 1 : 0) | ((data & 0x1000) ? 2 : 0);
			break;
	}

	log >>= (12 + a310_pagesize);

	// always make sure ROM mode is disconnected when this occurs
	a310_latchrom = 0;

	// now go ahead and set the mapping in the page table
	a310_pages[log] = phys * memc;

//	printf("MEMC_PAGE(%d): W %08x: log %x to phys %x, MEMC %d, perms %d\n", a310_pagesize, data, log, phys, memc, perms);
}

static ADDRESS_MAP_START( a310_mem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x01ffffff) AM_READWRITE(logical_r, logical_w)
	AM_RANGE(0x02000000, 0x02ffffff) AM_RAM AM_BASE(&a310_physmem) /* physical RAM - 16 MB for now, should be 512k for the A310 */
	AM_RANGE(0x03000000, 0x033fffff) AM_READWRITE(ioc_r, ioc_w)
	AM_RANGE(0x03400000, 0x035fffff) AM_READWRITE(vidc_r, vidc_w)
	AM_RANGE(0x03600000, 0x037fffff) AM_READWRITE(memc_r, memc_w)
	AM_RANGE(0x03800000, 0x03ffffff) AM_ROM AM_REGION(REGION_CPU1, 0) AM_WRITE(memc_page_w)
ADDRESS_MAP_END


INPUT_PORTS_START( a310 )
	PORT_START /* DIP switches */
	PORT_BIT(0xfd, 0xfd, IPT_UNUSED)

	PORT_START /* KEY ROW 0 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC)
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("1  !") PORT_CODE(KEYCODE_1)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("2  \"") PORT_CODE(KEYCODE_2)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("3  #") PORT_CODE(KEYCODE_3)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("4  $") PORT_CODE(KEYCODE_4)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("5  %") PORT_CODE(KEYCODE_5)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("6  &") PORT_CODE(KEYCODE_6)
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("7  '") PORT_CODE(KEYCODE_7)

	PORT_START /* KEY ROW 1 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("8  *") PORT_CODE(KEYCODE_8)
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("9  (") PORT_CODE(KEYCODE_9)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("0  )") PORT_CODE(KEYCODE_0)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("-  _") PORT_CODE(KEYCODE_MINUS)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("=  +") PORT_CODE(KEYCODE_EQUALS)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("`  ~") PORT_CODE(KEYCODE_TILDE)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("BACK SPACE") PORT_CODE(KEYCODE_BACKSPACE)
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("TAB") PORT_CODE(KEYCODE_TAB)

	PORT_START /* KEY ROW 2 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("q  Q") PORT_CODE(KEYCODE_Q)
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("w  W") PORT_CODE(KEYCODE_W)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("e  E") PORT_CODE(KEYCODE_E)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("r  R") PORT_CODE(KEYCODE_R)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("t  T") PORT_CODE(KEYCODE_T)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("y  Y") PORT_CODE(KEYCODE_Y)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("u  U") PORT_CODE(KEYCODE_U)
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("i  I") PORT_CODE(KEYCODE_I)

	PORT_START /* KEY ROW 3 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("o  O") PORT_CODE(KEYCODE_O)
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("p  P") PORT_CODE(KEYCODE_P)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("[  {") PORT_CODE(KEYCODE_OPENBRACE)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("]  }") PORT_CODE(KEYCODE_CLOSEBRACE)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("DEL") PORT_CODE(KEYCODE_DEL)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL)
	PORT_BIT(0x80, 0x80, IPT_KEYBOARD) PORT_NAME("CAPS LOCK") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE

	PORT_START /* KEY ROW 4 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("a  A") PORT_CODE(KEYCODE_A)
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("s  S") PORT_CODE(KEYCODE_S)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("d  D") PORT_CODE(KEYCODE_D)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("f  F") PORT_CODE(KEYCODE_F)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("g  G") PORT_CODE(KEYCODE_G)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("h  H") PORT_CODE(KEYCODE_H)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("j  J") PORT_CODE(KEYCODE_J)
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("k  K") PORT_CODE(KEYCODE_K)

	PORT_START /* KEY ROW 5 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("l  L") PORT_CODE(KEYCODE_L)
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME(";  :") PORT_CODE(KEYCODE_COLON)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("'  \"") PORT_CODE(KEYCODE_QUOTE)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("\\  |") PORT_CODE(KEYCODE_ASTERISK)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("SHIFT (L)") PORT_CODE(KEYCODE_LSHIFT)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("z  Z") PORT_CODE(KEYCODE_Z)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("x  X") PORT_CODE(KEYCODE_X)
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("c  C") PORT_CODE(KEYCODE_C)

	PORT_START /* KEY ROW 6 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("v  V") PORT_CODE(KEYCODE_V)
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("b  B") PORT_CODE(KEYCODE_B)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("n  N") PORT_CODE(KEYCODE_N)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("m  M") PORT_CODE(KEYCODE_M)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME(",  <") PORT_CODE(KEYCODE_COMMA)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME(".  >") PORT_CODE(KEYCODE_STOP)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("/  ?") PORT_CODE(KEYCODE_SLASH)
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("SHIFT (R)") PORT_CODE(KEYCODE_RSHIFT)

	PORT_START /* KEY ROW 7 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("LINE FEED")
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("- (KP)") PORT_CODE(KEYCODE_MINUS_PAD)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME(", (KP)") PORT_CODE(KEYCODE_PLUS_PAD)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("ENTER (KP)") PORT_CODE(KEYCODE_ENTER_PAD)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME(". (KP)") PORT_CODE(KEYCODE_DEL_PAD)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("0 (KP)") PORT_CODE(KEYCODE_0_PAD)
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("1 (KP)") PORT_CODE(KEYCODE_1_PAD)

	PORT_START /* KEY ROW 8 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("2 (KP)") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("3 (KP)") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("4 (KP)") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("5 (KP)") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("6 (KP)") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("7 (KP)") PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("8 (KP)") PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("9 (KP)") PORT_CODE(KEYCODE_9_PAD)

	PORT_START /* VIA #1 PORT A */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1					 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2					 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_4WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_4WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_4WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_4WAY

	PORT_START /* tape control */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("TAPE STOP") PORT_CODE(KEYCODE_F5)
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("TAPE PLAY") PORT_CODE(KEYCODE_F6)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("TAPE REW") PORT_CODE(KEYCODE_F7)
	PORT_BIT (0xf8, 0x80, IPT_UNUSED)
INPUT_PORTS_END


static MACHINE_DRIVER_START( a310 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", ARM, 8000000)        /* 8 MHz */
	MDRV_CPU_PROGRAM_MAP(a310_mem, 0)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET( a310 )
	MDRV_MACHINE_START( a310 )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(32*8, 16*16)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 32*8 - 1, 0*16, 16*16 - 1)
	MDRV_PALETTE_LENGTH(32768)

	MDRV_VIDEO_START(a310)
	MDRV_VIDEO_UPDATE(a310)

	MDRV_SPEAKER_STANDARD_MONO("a310")
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(0, "a310", 1.00)
MACHINE_DRIVER_END


ROM_START(a310)
	ROM_REGION(0x800000, REGION_CPU1, 0)
	ROM_LOAD("ic24.rom", 0x000000, 0x80000, CRC(c1adde84) SHA1(12d060e0401dd0523d44453f947bdc55dd2c3240))
	ROM_LOAD("ic25.rom", 0x080000, 0x80000, CRC(15d89664) SHA1(78f5d0e6f1e8ee603317807f53ff8fe65a3b3518))
	ROM_LOAD("ic26.rom", 0x100000, 0x80000, CRC(a81ceb7c) SHA1(46b870876bc1f68f242726415f0c49fef7be0c72))
	ROM_LOAD("ic27.rom", 0x180000, 0x80000, CRC(707b0c6c) SHA1(345199a33fed23996374b9db8170a52ab63f0380))

	ROM_REGION(0x00800, REGION_GFX1, 0)
ROM_END

/*    YEAR  NAME  PARENT COMPAT	 MACHINE  INPUT	 INIT  CONFIG  COMPANY	FULLNAME */
COMP( 1988, a310, 0,     0,      a310,    a310,  a310, NULL,   "Acorn", "Archimedes 310", GAME_NOT_WORKING)
