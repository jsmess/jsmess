/***************************************************************************

    Sony PocketStation

    05/2009 Skeleton driver written.
	11/2009 Initial bring-up commenced by Harmony.

    PocketStation games were dowloaded from PS1 games into flash RAM after
    the unit had been inserted in the memory card slot, and so this should
    be emulated alongside the PS1.  However, as many flash dumps exist, it
    is possible to emulate the PocketStation in the meantime.

    CPU: ARM7T (32 bit RISC Processor)
    Memory: 2Kbytes of SRAM, 128Kbytes of FlashROM
    Graphics: 32x32 monochrome LCD
    Sound: 1 12-bit PCM channel
    Input: 5 input buttons, 1 reset button
    Infrared communication: Bi-directional and uni-directional comms
    Other: 1 LED indicator

    Info available at:
      * http://exophase.devzero.co.uk/ps_info.txt
      * http://members.at.infoseek.co.jp/DrHell/pocket/index.html

	The current reason why the driver fails to run is:
		- The BIOS is displeased by the RTC data that it is getting.

****************************************************************************/

#include "driver.h"
#include "cpu/arm7/arm7.h"
#include "cpu/arm7/arm7core.h"

#define DEFAULT_CLOCK	2000000

static const int CPU_FREQ[16] =
{
	0x00f800,
	0x01f000,
	0x03e000,
	0x07c000,
	0x0f8000,
	0x1e8000,
	0x3d0000,
	0x7a0000,
	0x7a0000,
	0x7a0000,
	0x7a0000,
	0x7a0000,
	0x7a0000,
	0x7a0000,
	0x7a0000,
	0x7a0000
};

#define VERBOSE_LEVEL ( 0 )

#define ENABLE_VERBOSE_LOG (0)

#if ENABLE_VERBOSE_LOG
INLINE void ATTR_PRINTF(3,4) verboselog( running_machine *machine, int n_level, const char *s_fmt, ... )
{
	if( VERBOSE_LEVEL >= n_level )
	{
		va_list v;
		char buf[ 32768 ];
		va_start( v, s_fmt );
		vsprintf( buf, s_fmt, v );
		va_end( v );
		logerror( "%s: %s", cpuexec_describe_context(machine), buf );
	}
}
#else
#define verboselog(x,y,z,...)
#endif

static UINT32 *lcd_buffer;

// Flash TLB
static READ32_HANDLER( ps_ftlb_r );
static WRITE32_HANDLER( ps_ftlb_w );

// Interrupt Controller
static UINT32 ps_intc_get_interrupt_line(running_machine *machine, UINT32 line);
static void ps_intc_set_interrupt_line(running_machine *machine, UINT32 line, int state);
static READ32_HANDLER( ps_intc_r );
static WRITE32_HANDLER( ps_intc_w );

// Timers
static TIMER_CALLBACK( timer_tick );
static void ps_timer_start(running_machine *machine, int index);
static READ32_HANDLER( ps_timer_r );
static WRITE32_HANDLER( ps_timer_w );

// Clock
static READ32_HANDLER( ps_clock_r );
static WRITE32_HANDLER( ps_clock_w );

// RTC
static TIMER_CALLBACK( rtc_tick );
static READ32_HANDLER( ps_rtc_r );
static WRITE32_HANDLER( ps_rtc_w );

typedef struct
{
	UINT32 control;
	UINT32 stat;
	UINT32 valid;
	UINT32 wait1;
	UINT32 wait2;
	UINT32 entry[16];
	UINT32 serial;
} ps_ftlb_regs_t;

typedef struct
{
	UINT32 hold;
	UINT32 status;
	UINT32 enable;
	UINT32 mask;
} ps_intc_regs_t;

#define PS_INT_BTN_ACTION		0x00000001 // "Action button"
#define PS_INT_BTN_RIGHT		0x00000002 // "Right button"
#define PS_INT_BTN_LEFT			0x00000004 // "Left button"
#define PS_INT_BTN_DOWN			0x00000008 // "Down button"
#define PS_INT_BTN_UP			0x00000010 // "Up button"
#define PS_INT_UNKNOWN			0x00000020 // "Unknown"
#define PS_INT_COM				0x00000040 // "COM" ???
#define PS_INT_TIMER0			0x00000080 // "Timer 0"
#define PS_INT_TIMER1			0x00000100 // "Timer 1"
#define PS_INT_RTC				0x00000200 // "RTC"
#define PS_INT_BATTERY			0x00000400 // "Battery Monitor"
#define PS_INT_IOP				0x00000800 // "IOP"
#define PS_INT_IRDA				0x00001000 // "IrDA"
#define PS_INT_TIMER2			0x00002000 // "Timer 2"
#define PS_INT_IRQ_MASK			0x00001fbf
#define PS_INT_FIQ_MASK			0x00002040
#define PS_INT_STATUS_MASK		0x0000021f

#define MAX_PS_TIMERS	3

typedef struct
{
	UINT32 period;
	UINT32 count;
	UINT32 control;
	emu_timer *timer;
	emu_timer *int_reset_timer; // Probably not right
} ps_timer_t;

typedef struct
{
	ps_timer_t timer[MAX_PS_TIMERS];
} ps_timer_regs_t;

typedef struct
{
	UINT32 mode;
	UINT32 control;
} ps_clock_regs_t;

#define PS_CLOCK_STEADY		0x10

typedef struct
{
	UINT32 mode;
	UINT32 control;
	UINT32 time;
	UINT32 date;
	emu_timer *timer;
} ps_rtc_regs_t;

static ps_ftlb_regs_t ftlb_regs;
static ps_intc_regs_t intc_regs;
static ps_timer_regs_t timer_regs;
static ps_clock_regs_t clock_regs;
static ps_rtc_regs_t rtc_regs;

static READ32_HANDLER( ps_ftlb_r )
{
	switch(offset)
	{
		case 0x0000/4:
			verboselog(space->machine, 0, "ps_ftlb_r: FlashROM TLB Control = %08x & %08x\n", ftlb_regs.control, mem_mask );
			return ftlb_regs.control;
		case 0x0004/4:
			verboselog(space->machine, 0, "ps_ftlb_r: Unknown (F_STAT) = %08x & %08x\n", ftlb_regs.stat, mem_mask );
			return ftlb_regs.stat;
		case 0x0008/4:
			verboselog(space->machine, 0, "ps_ftlb_r: FlashROM TLB Valid Tag = %08x & %08x\n", ftlb_regs.valid, mem_mask );
			return ftlb_regs.valid;
		case 0x000c/4:
			verboselog(space->machine, 0, "ps_ftlb_r: Unknown (F_WAIT1) = %08x & %08x\n", ftlb_regs.wait1, mem_mask );
			return ftlb_regs.wait1;
		case 0x0010/4:
			verboselog(space->machine, 0, "ps_ftlb_r: Unknown (F_WAIT2) = %08x & %08x\n", ftlb_regs.wait2, mem_mask );
			return ftlb_regs.wait2;
		case 0x0100/4:
		case 0x0104/4:
		case 0x0108/4:
		case 0x010c/4:
		case 0x0110/4:
		case 0x0114/4:
		case 0x0118/4:
		case 0x011c/4:
		case 0x0120/4:
		case 0x0124/4:
		case 0x0128/4:
		case 0x012c/4:
		case 0x0130/4:
		case 0x0134/4:
		case 0x0138/4:
		case 0x013c/4:
			verboselog(space->machine, 0, "ps_ftlb_r: FlashROM TLB Entry %d = %08x & %08x\n", offset - 0x100/4, ftlb_regs.entry[offset - 0x100/4], mem_mask );
			return ftlb_regs.entry[offset - 0x100/4];
		case 0x0300/4:
			verboselog(space->machine, 0, "ps_ftlb_r: Unknown (F_SN) = %08x & %08x\n", ftlb_regs.serial, mem_mask );
			return ftlb_regs.serial;
		default:
			verboselog(space->machine, 0, "ps_ftlb_r: Unknown Register %08x & %08x\n", 0x06000000 + (offset << 2), mem_mask );
			break;
	}
	return 0;
}

static WRITE32_HANDLER( ps_ftlb_w )
{
	switch(offset)
	{
		case 0x0000/4:
			verboselog(space->machine, 0, "ps_ftlb_w: FlashROM TLB Control = %08x & %08x\n", data, mem_mask );
			COMBINE_DATA(&ftlb_regs.control);
			break;
		case 0x0004/4:
			verboselog(space->machine, 0, "ps_ftlb_w: Unknown (F_STAT) = %08x & %08x\n", data, mem_mask );
			COMBINE_DATA(&ftlb_regs.stat);
			break;
		case 0x0008/4:
			verboselog(space->machine, 0, "ps_ftlb_w: FlashROM TLB Valid Tag = %08x & %08x\n", data, mem_mask );
			COMBINE_DATA(&ftlb_regs.valid);
			break;
		case 0x000c/4:
			verboselog(space->machine, 0, "ps_ftlb_w: Unknown (F_WAIT1) = %08x & %08x\n", data, mem_mask );
			COMBINE_DATA(&ftlb_regs.wait1);
			break;
		case 0x0010/4:
			verboselog(space->machine, 0, "ps_ftlb_w: Unknown (F_WAIT2) = %08x & %08x\n", data, mem_mask );
			COMBINE_DATA(&ftlb_regs.wait2);
			break;
		case 0x0100/4:
		case 0x0104/4:
		case 0x0108/4:
		case 0x010c/4:
		case 0x0110/4:
		case 0x0114/4:
		case 0x0118/4:
		case 0x011c/4:
		case 0x0120/4:
		case 0x0124/4:
		case 0x0128/4:
		case 0x012c/4:
		case 0x0130/4:
		case 0x0134/4:
		case 0x0138/4:
		case 0x013c/4:
			verboselog(space->machine, 0, "ps_ftlb_w: FlashROM TLB Entry %d = %08x & %08x\n", offset - 0x100/4, data, mem_mask );
			COMBINE_DATA(&ftlb_regs.entry[offset - 0x100/4]);
			break;
		case 0x0300/4:
			verboselog(space->machine, 0, "ps_ftlb_w: Unknown (F_SN) = %08x & %08x\n", data, mem_mask );
			COMBINE_DATA(&ftlb_regs.serial);
			break;
		default:
			verboselog(space->machine, 0, "ps_ftlb_w: Unknown Register %08x = %08x & %08x\n", 0x06000000 + (offset << 2), data, mem_mask );
			break;
	}
}

static UINT32 ps_intc_get_interrupt_line(running_machine *machine, UINT32 line)
{
	return intc_regs.hold & line;
}

static void ps_intc_set_interrupt_line(running_machine *machine, UINT32 line, int state)
{
	if(line)
	{
		if(state)
		{
			intc_regs.status |= line & PS_INT_STATUS_MASK;
			line &= intc_regs.enable;
			line &= ~intc_regs.mask;
			intc_regs.hold |= line;
		}
		else
		{
			intc_regs.status &= ~line;
		}
	}
	if(intc_regs.hold & PS_INT_IRQ_MASK)
	{
		cpu_set_input_line(cputag_get_cpu(machine, "maincpu"), ARM7_IRQ_LINE, ASSERT_LINE);
	}
	else
	{
		cpu_set_input_line(cputag_get_cpu(machine, "maincpu"), ARM7_IRQ_LINE, CLEAR_LINE);
	}
	if(intc_regs.hold & PS_INT_FIQ_MASK)
	{
		cpu_set_input_line(cputag_get_cpu(machine, "maincpu"), ARM7_FIRQ_LINE, ASSERT_LINE);
	}
	else
	{
		cpu_set_input_line(cputag_get_cpu(machine, "maincpu"), ARM7_FIRQ_LINE, CLEAR_LINE);
	}
}

static READ32_HANDLER( ps_intc_r )
{
	switch(offset)
	{
		case 0x0000/4:
			verboselog(space->machine, 0, "ps_intc_r: Held Interrupt = %08x & %08x\n", intc_regs.hold, mem_mask );
			return intc_regs.hold;
		case 0x0004/4:
			verboselog(space->machine, 0, "ps_intc_r: Interrupt Status = %08x & %08x\n", intc_regs.status, mem_mask );
			return intc_regs.status;
		case 0x0008/4:
			verboselog(space->machine, 0, "ps_intc_r: Interrupt Enable = %08x & %08x\n", intc_regs.enable, mem_mask );
			return intc_regs.enable;
		case 0x000c/4:
			verboselog(space->machine, 0, "ps_intc_r: Interrupt Mask (Invalid Read) = %08x & %08x\n", 0, mem_mask );
			return 0;
		case 0x0010/4:
			verboselog(space->machine, 0, "ps_intc_r: Interrupt Acknowledge (Invalid Read) = %08x & %08x\n", 0, mem_mask );
			return 0;
		default:
			verboselog(space->machine, 0, "ps_intc_r: Unknown Register %08x & %08x\n", 0x0a000000 + (offset << 2), mem_mask );
			break;
	}
	return 0;
}

static WRITE32_HANDLER( ps_intc_w )
{
	switch(offset)
	{
		case 0x0000/4:
			verboselog(space->machine, 0, "ps_intc_w: Held Interrupt (Invalid Write) = %08x & %08x\n", data, mem_mask );
			break;
		case 0x0004/4:
			verboselog(space->machine, 0, "ps_intc_w: Interrupt Status (Invalid Write) = %08x & %08x\n", data, mem_mask );
			break;
		case 0x0008/4:
			verboselog(space->machine, 0, "ps_intc_w: Interrupt Enable = %08x & %08x\n", data, mem_mask );
			COMBINE_DATA(&intc_regs.enable);
			intc_regs.mask &= ~intc_regs.enable;
			break;
		case 0x000c/4:
			verboselog(space->machine, 0, "ps_intc_w: Interrupt Mask = %08x & %08x\n", data, mem_mask );
			COMBINE_DATA(&intc_regs.mask);
			intc_regs.enable &= ~intc_regs.mask;
			break;
		case 0x0010/4:
			verboselog(space->machine, 0, "ps_intc_w: Interrupt Acknowledge = %08x & %08x\n", data, mem_mask );
			intc_regs.hold &= ~data;
			intc_regs.status &= ~data;
			ps_intc_set_interrupt_line(space->machine, 0, 0);
			//COMBINE_DATA(&intc_regs.acknowledge);
			break;
		default:
			verboselog(space->machine, 0, "ps_intc_w: Unknown Register %08x = %08x & %08x\n", 0x0a000000 + (offset << 2), data, mem_mask );
			break;
	}
}

static TIMER_CALLBACK( timer_tick )
{
	ps_intc_set_interrupt_line(machine, param == 2 ? PS_INT_TIMER2 : (param == 1 ? PS_INT_TIMER1 : PS_INT_TIMER0), 1);
	//printf( "Timer %d is calling back\n", param );
	timer_regs.timer[param].count = timer_regs.timer[param].period;
	ps_timer_start(machine, param);
}

static void ps_timer_start(running_machine *machine, int index)
{
	int divisor = 1;
	attotime period;
	switch(timer_regs.timer[index].control & 3)
	{
		case 0:
		case 3:
			divisor = 1;
			break;
		case 1:
			divisor = 16;
			break;
		case 2:
			divisor = 256;
			break;
	}
	period = attotime_mul(ATTOTIME_IN_HZ(CPU_FREQ[clock_regs.mode & 0x0f] / 2), divisor);
	period = attotime_mul(period, timer_regs.timer[index].count);
	timer_adjust_oneshot(timer_regs.timer[index].timer, period, index);
}

static READ32_HANDLER( ps_timer_r )
{
	switch(offset)
	{
		case 0x0000/4:
		case 0x0010/4:
		case 0x0020/4:
			verboselog(space->machine, 0, "ps_timer_r: Timer %d Period = %08x & %08x\n", offset / (0x10/4), timer_regs.timer[offset / (0x10/4)].period, mem_mask );
			return timer_regs.timer[offset / (0x10/4)].period;
		case 0x0004/4:
		case 0x0014/4:
		case 0x0024/4:
 			verboselog(space->machine, 0, "ps_timer_r: Timer %d Count = %08x & %08x\n", offset / (0x10/4), timer_regs.timer[offset / (0x10/4)].count, mem_mask );
 			if(timer_regs.timer[offset / (0x10/4)].control & 4)
 			{
				return --timer_regs.timer[offset / (0x10/4)].count;
			}
			return timer_regs.timer[offset / (0x10/4)].count;
		case 0x0008/4:
		case 0x0018/4:
		case 0x0028/4:
			verboselog(space->machine, 0, "ps_timer_r: Timer %d Control = %08x & %08x\n", offset / (0x10/4), timer_regs.timer[offset / (0x10/4)].control, mem_mask );
			return timer_regs.timer[offset / (0x10/4)].control;
		default:
			verboselog(space->machine, 0, "ps_timer_r: Unknown Register %08x & %08x\n", 0x0a800000 + (offset << 2), mem_mask );
			break;
	}
	return 0;
}

static WRITE32_HANDLER( ps_timer_w )
{
	switch(offset)
	{
		case 0x0000/4:
		case 0x0010/4:
		case 0x0020/4:
			verboselog(space->machine, 0, "ps_timer_w: Timer %d Period = %08x & %08x\n", offset / (0x10/4), data, mem_mask );
			COMBINE_DATA(&timer_regs.timer[offset / (0x10/4)].period);
			break;
		case 0x0004/4:
		case 0x0014/4:
		case 0x0024/4:
			verboselog(space->machine, 0, "ps_timer_w: Timer %d Count = %08x & %08x\n", offset / (0x10/4), data, mem_mask );
			COMBINE_DATA(&timer_regs.timer[offset / (0x10/4)].count);
			break;
		case 0x0008/4:
		case 0x0018/4:
		case 0x0028/4:
			verboselog(space->machine, 0, "ps_timer_w: Timer %d Control = %08x & %08x\n", offset / (0x10/4), data, mem_mask );
			COMBINE_DATA(&timer_regs.timer[offset / (0x10/4)].control);
			if(timer_regs.timer[offset / (0x10/4)].control & 4)
			{
				ps_timer_start(space->machine, offset / (0x10/4));
			}
			else
			{
				timer_adjust_oneshot(timer_regs.timer[offset / (0x10/4)].timer, attotime_never, offset / (0x10/4));
			}
			break;
		default:
			verboselog(space->machine, 0, "ps_timer_w: Unknown Register %08x = %08x & %08x\n", 0x0a800000 + (offset << 2), data, mem_mask );
			break;
	}
}

static READ32_HANDLER( ps_clock_r )
{
	switch(offset)
	{
		case 0x0000/4:
			verboselog(space->machine, 0, "ps_clock_r: Clock Mode = %08x & %08x\n", clock_regs.mode | 0x10, mem_mask );
			return clock_regs.mode | PS_CLOCK_STEADY;
		case 0x0004/4:
			verboselog(space->machine, 0, "ps_clock_r: Clock Control = %08x & %08x\n", clock_regs.control, mem_mask );
			return clock_regs.control;
		default:
			verboselog(space->machine, 0, "ps_clock_r: Unknown Register %08x & %08x\n", 0x0b000000 + (offset << 2), mem_mask );
			break;
	}
	return 0;
}

static WRITE32_HANDLER( ps_clock_w )
{
	switch(offset)
	{
		case 0x0000/4:
			verboselog(space->machine, 0, "ps_clock_w: Clock Mode = %08x & %08x\n", data, mem_mask );
			COMBINE_DATA(&clock_regs.mode);
			cputag_set_clock(space->machine, "maincpu", CPU_FREQ[clock_regs.mode & 0x0f]);
			break;
		case 0x0004/4:
			verboselog(space->machine, 0, "ps_clock_w: Clock Control = %08x & %08x\n", data, mem_mask );
			COMBINE_DATA(&clock_regs.control);
			break;
		default:
			verboselog(space->machine, 0, "ps_clock_w: Unknown Register %08x = %08x & %08x\n", 0x0b000000 + (offset << 2), data, mem_mask );
			break;
	}
}

static TIMER_CALLBACK( rtc_tick )
{
	//printf( "RTC is calling back\n" );
	ps_intc_set_interrupt_line(machine, PS_INT_RTC, ps_intc_get_interrupt_line(machine, PS_INT_RTC) ? 0 : 1);
	if(!(rtc_regs.mode & 1))
	{
		rtc_regs.time++;
		if((rtc_regs.time & 0x0000000f) == 0x0000000a)
		{
			rtc_regs.time &= 0xfffffff0;
			rtc_regs.time += 0x00000010;
			if((rtc_regs.time & 0x000000ff) == 0x00000060)
			{
				rtc_regs.time &= 0xffffff00;
				rtc_regs.time += 0x00000100;
				if((rtc_regs.time & 0x00000f00) == 0x00000a00)
				{
					rtc_regs.time &= 0xfffff0ff;
					rtc_regs.time += 0x00001000;
					if((rtc_regs.time & 0x0000ff00) == 0x00006000)
					{
						rtc_regs.time &= 0xffff00ff;
						rtc_regs.time += 0x00010000;
						if((rtc_regs.time & 0x00ff0000) == 0x00240000)
						{
							rtc_regs.time &= 0xff00ffff;
							rtc_regs.time += 0x01000000;
							if((rtc_regs.time & 0x0f000000) == 0x08000000)
							{
								rtc_regs.time &= 0xf0ffffff;
								rtc_regs.time |= 0x01000000;
							}
						}
						else if((rtc_regs.time & 0x000f0000) == 0x000a0000)
						{
							rtc_regs.time &= 0xfff0ffff;
							rtc_regs.time += 0x00100000;
						}
					}
				}
			}
		}
	}
	timer_adjust_oneshot(rtc_regs.timer, ATTOTIME_IN_HZ(1), 0);
}

static READ32_HANDLER( ps_rtc_r )
{
	switch(offset)
	{
		case 0x0000/4:
			verboselog(space->machine, 0, "ps_rtc_r: RTC Mode = %08x & %08x\n", rtc_regs.mode, mem_mask );
			return rtc_regs.mode;
		case 0x0004/4:
			verboselog(space->machine, 0, "ps_rtc_r: RTC Control = %08x & %08x\n", rtc_regs.control, mem_mask );
			return rtc_regs.control;
		case 0x0008/4:
			verboselog(space->machine, 0, "ps_rtc_r: RTC Time = %08x & %08x\n", rtc_regs.time, mem_mask );
			return rtc_regs.time;
		case 0x000c/4:
			verboselog(space->machine, 0, "ps_rtc_r: RTC Date = %08x & %08x\n", rtc_regs.date, mem_mask );
			return rtc_regs.date;
		default:
			verboselog(space->machine, 0, "ps_rtc_r: Unknown Register %08x & %08x\n", 0x0b800000 + (offset << 2), mem_mask );
			break;
	}
	return 0;
}

static WRITE32_HANDLER( ps_rtc_w )
{
	switch(offset)
	{
		case 0x0000/4:
			verboselog(space->machine, 0, "ps_rtc_w: RTC Mode = %08x & %08x\n", data, mem_mask );
			COMBINE_DATA(&rtc_regs.mode);
			break;
		case 0x0004/4:
			verboselog(space->machine, 0, "ps_rtc_w: RTC Control = %08x & %08x\n", data, mem_mask );
			COMBINE_DATA(&rtc_regs.control);
			break;
		default:
			verboselog(space->machine, 0, "ps_rtc_w: Unknown Register %08x = %08x & %08x\n", 0x0b800000 + (offset << 2), data, mem_mask );
			break;
	}
}

static UINT32 lcd_control;

static READ32_HANDLER( ps_lcd_r )
{
	switch(offset)
	{
		case 0x0000/4:
			verboselog(space->machine, 0, "ps_lcd_r: LCD Control = %08x & %08x\n", lcd_control | 0x100, mem_mask );
			return lcd_control;
		default:
			verboselog(space->machine, 0, "ps_lcd_r: Unknown Register %08x & %08x\n", 0x0d000000 + (offset << 2), mem_mask );
			break;
	}
	return 0;
}

static WRITE32_HANDLER( ps_lcd_w )
{
	switch(offset)
	{
		case 0x0000/4:
			verboselog(space->machine, 0, "ps_lcd_w: LCD Control = %08x & %08x\n", data, mem_mask );
			COMBINE_DATA(&lcd_control);
			break;
		default:
			verboselog(space->machine, 0, "ps_lcd_w: Unknown Register %08x = %08x & %08x\n", 0x0d000000 + (offset << 2), data, mem_mask );
			break;
	}
}

static INPUT_CHANGED( input_update )
{
	UINT32 buttons = input_port_read(field->port->machine, "BUTTONS");

	ps_intc_set_interrupt_line(field->port->machine, PS_INT_BTN_ACTION, (buttons &  1) ? 1 : 0);
	ps_intc_set_interrupt_line(field->port->machine, PS_INT_BTN_RIGHT, 	(buttons &  2) ? 1 : 0);
	ps_intc_set_interrupt_line(field->port->machine, PS_INT_BTN_LEFT, 	(buttons &  4) ? 1 : 0);
	ps_intc_set_interrupt_line(field->port->machine, PS_INT_BTN_DOWN, 	(buttons &  8) ? 1 : 0);
	ps_intc_set_interrupt_line(field->port->machine, PS_INT_BTN_UP, 	(buttons & 16) ? 1 : 0);
}

static ADDRESS_MAP_START(pockstat_mem, ADDRESS_SPACE_PROGRAM, 32)
	AM_RANGE(0x00000000, 0x000007ff) AM_RAM
	AM_RANGE(0x04000000, 0x04003fff) AM_ROM AM_REGION("maincpu", 0)
	AM_RANGE(0x06000000, 0x06000307) AM_READWRITE(ps_ftlb_r, ps_ftlb_w)
	AM_RANGE(0x0a000000, 0x0a000013) AM_READWRITE(ps_intc_r, ps_intc_w)
	AM_RANGE(0x0a800000, 0x0a80002b) AM_READWRITE(ps_timer_r, ps_timer_w)
	AM_RANGE(0x0b000000, 0x0b000007) AM_READWRITE(ps_clock_r, ps_clock_w)
	AM_RANGE(0x0b800000, 0x0b80000f) AM_READWRITE(ps_rtc_r, ps_rtc_w)
	AM_RANGE(0x0d000000, 0x0d000003) AM_READWRITE(ps_lcd_r, ps_lcd_w)
	AM_RANGE(0x0d000100, 0x0d00017f) AM_RAM AM_BASE(&lcd_buffer)
/*  AM_RANGE(0x02000000, 0x0201ffff) Flash ROM
    AM_RANGE(0x08000000, 0x0801ffff) Same as 0x02 above. Mirrors every 128KB.
    AM_RANGE(0x0b000000, 0x0b000003) Internal CPU Clock control
    AM_RANGE(0x0b800000, 0x0b800003) RTC control word
    AM_RANGE(0x0b800004, 0x0b800007) RTC Modify value (write only)
    AM_RANGE(0x0b800008, 0x0b80000b) RTC Time of day + day of week (read only)
    AM_RANGE(0x0b80000c, 0x0b80000f) RTC Date (read only)
    AM_RANGE(0x0d000000, 0x0b000003) LCD control word
    AM_RANGE(0x0d000100, 0x0b00017f) LCD buffer - 1 word per scanline   */
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( pockstat )
	PORT_START("BUTTONS")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1) 		PORT_NAME("Action Button")	PORT_CHANGED(input_update, 0)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_NAME("Right")			PORT_CHANGED(input_update, 0)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) 	PORT_NAME("Left")			PORT_CHANGED(input_update, 0)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) 	PORT_NAME("Down")			PORT_CHANGED(input_update, 0)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) 	PORT_NAME("Up")				PORT_CHANGED(input_update, 0)
	PORT_BIT( 0xe0, IP_ACTIVE_HIGH, IPT_UNUSED)
INPUT_PORTS_END

static MACHINE_START( pockstat )
{
	int index = 0;
	for(index = 0; index < 3; index++)
	{
		timer_regs.timer[index].timer = timer_alloc(machine, timer_tick, 0);
		timer_adjust_oneshot(timer_regs.timer[index].timer, attotime_never, index);
	}

	rtc_regs.time = 0x01000000;
	rtc_regs.date = 0x19990101;

	rtc_regs.timer = timer_alloc(machine, rtc_tick, 0);
	timer_adjust_oneshot(rtc_regs.timer, ATTOTIME_IN_HZ(1), index);
}

static MACHINE_RESET( pockstat )
{
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), REG_GENPC, 0x4000000);
}

static VIDEO_UPDATE( pockstat )
{
	int x = 0;
	int y = 0;
	for(y = 0; y < 32; y++)
	{
		UINT32 *scanline = BITMAP_ADDR32(bitmap, y, 0);
		for(x = 0; x < 32; x++)
		{
			if(lcd_buffer[y] & (1 << x))
			{
				scanline[x] = 0x00000000;
			}
			else
			{
				scanline[x] = 0x00ffffff;
			}
		}
	}
	return 0;
}

static MACHINE_DRIVER_START( pockstat )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", ARM7, DEFAULT_CLOCK)
	MDRV_CPU_PROGRAM_MAP(pockstat_mem)

	MDRV_MACHINE_RESET(pockstat)
	MDRV_MACHINE_START(pockstat)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", LCD)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_SIZE(32, 32)
	MDRV_SCREEN_VISIBLE_AREA(0, 32-1, 0, 32-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(pockstat)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( pockstat )
	ROM_REGION( 0x4000, "maincpu", 0 )
	ROM_LOAD( "kernel.bin", 0x0000, 0x4000, CRC(5fb47dd8) SHA1(6ae880493ddde880827d1e9f08e9cb2c38f9f2ec) )
ROM_END

/* Driver */

/*    YEAR  NAME      PARENT  COMPAT  MACHINE    INPUT     INIT  CONFIG     COMPANY                             FULLNAME       FLAGS */
CONS( 1999, pockstat, 0,      0,      pockstat,  pockstat, 0,    0,  "Sony Computer Entertainment Inc.", "Sony PocketStation", GAME_NOT_WORKING )
