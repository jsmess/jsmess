/***************************************************************************

    Sony PocketStation

    05/2009 Skeleton driver.


    This should be emulated alongside PS1 (and especially its memory cards,
    since Pocket Station games were dowloaded from PS1 games into flash RAM
    after the unit had been inserted in the memory card slot).
    While waiting for full PS1 emulation in MESS, we collect here info on the
    system and its BIOS.

    CPU: ARM7T (32 bit RISC Processor)
    Memory: SRAM 2K bytes, Flash RAM 128K bytes
    Graphics: 32 x 32 dot monochrome LCD
    Sound: Miniature speaker (12 bit PCM) x 1 unit
    Input: 5 input buttons, 1 reset button
    Infrared communication: Bi-directional (supports IrDA based and
        conventional remote control systems)
    Other: 1 LED indicator

    Info available at:
      * http://exophase.devzero.co.uk/ps_info.txt
      * http://members.at.infoseek.co.jp/DrHell/pocket/index.html

****************************************************************************/

#include "driver.h"
#include "cpu/arm7/arm7.h"

#define VERBOSE_LEVEL ( 99 )

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

static ps_ftlb_regs_t ftlb_regs;

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
	}
}

typedef struct
{
	UINT32 hold;
	UINT32 status;
	UINT32 enable;
	UINT32 mask;
} ps_intc_regs_t;

#define PS_INT_BTN_DECISION		0x00000001 // "Decision button" ???
#define PS_INT_BTN_RIGHT		0x00000002 // "Right button"
#define PS_INT_BTN_LEFT			0x00000004 // "Left button"
#define PS_INT_BTN_DOWN			0x00000008 // "Down button"
#define PS_INT_BTN_ON			0x00000010 // "Button on" ???
#define PS_INT_UNKNOWN			0x00000020 // "Unknown"
#define PS_INT_COM				0x00000040 // "COM" ???
#define PS_INT_TIMER0			0x00000080 // "Timer 0"
#define PS_INT_TIMER1			0x00000100 // "Timer 1"
#define PS_INT_RTC				0x00000200 // "RTC"
#define PS_INT_BATTERY			0x00000400 // "Battery Monitor"
#define PS_INT_IOP				0x00000800 // "IOP"
#define PS_INT_IRDA				0x00001000 // "IrDA"
#define PS_INT_TIMER2			0x00002000 // "Timer 2"

static ps_intc_regs_t intc_regs;

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
			verboselog(space->machine, 0, "ps_intc_r: Interrupt Enable = %08x & %08x\n", intc_regs.mask, mem_mask );
			return intc_regs.mask;
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
			break;
		case 0x000c/4:
			verboselog(space->machine, 0, "ps_intc_w: Interrupt Mask = %08x & %08x\n", data, mem_mask );
			COMBINE_DATA(&intc_regs.mask);
			break;
		case 0x0010/4:
			verboselog(space->machine, 0, "ps_intc_w: Interrupt Acknowledge = %08x & %08x\n", data, mem_mask );
			//COMBINE_DATA(&intc_regs.acknowledge);
			break;
		default:
			verboselog(space->machine, 0, "ps_intc_w: Unknown Register %08x & %08x\n", 0x0a000000 + (offset << 2), mem_mask );
			break;
	}
}

typedef struct
{
	UINT32 timer0_period;
	UINT32 timer0_count;
	UINT32 timer0_control;
	UINT32 timer1_period;
	UINT32 timer1_count;
	UINT32 timer1_control;
	UINT32 timer2_period;
	UINT32 timer2_count;
	UINT32 timer2_control;
} ps_timer_regs_t;

static ps_timer_regs_t timer_regs;

static READ32_HANDLER( ps_timer_r )
{
	switch(offset)
	{
		case 0x0000/4:
			verboselog(space->machine, 0, "ps_timer_r: Timer 0 Period = %08x & %08x\n", timer_regs.timer0_period, mem_mask );
			return timer_regs.timer0_period;
		case 0x0004/4:
			verboselog(space->machine, 0, "ps_timer_r: Timer 0 Count = %08x & %08x\n", timer_regs.timer0_count, mem_mask );
			return timer_regs.timer0_count;
		case 0x0008/4:
			verboselog(space->machine, 0, "ps_timer_r: Timer 0 Control = %08x & %08x\n", timer_regs.timer0_control, mem_mask );
			return timer_regs.timer0_control;
		case 0x0010/4:
			verboselog(space->machine, 0, "ps_timer_r: Timer 1 Period = %08x & %08x\n", timer_regs.timer1_period, mem_mask );
			return timer_regs.timer1_period;
		case 0x0014/4:
			verboselog(space->machine, 0, "ps_timer_r: Timer 1 Count = %08x & %08x\n", timer_regs.timer1_count, mem_mask );
			return timer_regs.timer1_count;
		case 0x0018/4:
			verboselog(space->machine, 0, "ps_timer_r: Timer 1 Control = %08x & %08x\n", timer_regs.timer1_control, mem_mask );
			return timer_regs.timer1_control;
		case 0x0020/4:
			verboselog(space->machine, 0, "ps_timer_r: Timer 2 Period = %08x & %08x\n", timer_regs.timer2_period, mem_mask );
			return timer_regs.timer2_period;
		case 0x0024/4:
			verboselog(space->machine, 0, "ps_timer_r: Timer 2 Count = %08x & %08x\n", timer_regs.timer2_count, mem_mask );
			return timer_regs.timer2_count;
		case 0x0028/4:
			verboselog(space->machine, 0, "ps_timer_r: Timer 2 Control = %08x & %08x\n", timer_regs.timer2_control, mem_mask );
			return timer_regs.timer2_control;
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
			verboselog(space->machine, 0, "ps_timer_w: Timer 0 Period = %08x & %08x\n", data, mem_mask );
			COMBINE_DATA(&timer_regs.timer0_period);
			break;
		case 0x0004/4:
			verboselog(space->machine, 0, "ps_timer_w: Timer 0 Count = %08x & %08x\n", data, mem_mask );
			COMBINE_DATA(&timer_regs.timer0_count);
			break;
		case 0x0008/4:
			verboselog(space->machine, 0, "ps_timer_w: Timer 0 Control = %08x & %08x\n", data, mem_mask );
			COMBINE_DATA(&timer_regs.timer0_control);
			break;
		case 0x0010/4:
			verboselog(space->machine, 0, "ps_timer_w: Timer 1 Period = %08x & %08x\n", data, mem_mask );
			COMBINE_DATA(&timer_regs.timer1_period);
			break;
		case 0x0014/4:
			verboselog(space->machine, 0, "ps_timer_w: Timer 1 Count = %08x & %08x\n", data, mem_mask );
			COMBINE_DATA(&timer_regs.timer1_count);
			break;
		case 0x0018/4:
			verboselog(space->machine, 0, "ps_timer_w: Timer 1 Control = %08x & %08x\n", data, mem_mask );
			COMBINE_DATA(&timer_regs.timer1_control);
			break;
		case 0x0020/4:
			verboselog(space->machine, 0, "ps_timer_w: Timer 2 Period = %08x & %08x\n", data, mem_mask );
			COMBINE_DATA(&timer_regs.timer2_period);
			break;
		case 0x0024/4:
			verboselog(space->machine, 0, "ps_timer_w: Timer 2 Count = %08x & %08x\n", data, mem_mask );
			COMBINE_DATA(&timer_regs.timer2_count);
			break;
		case 0x0028/4:
			verboselog(space->machine, 0, "ps_timer_w: Timer 2 Control = %08x & %08x\n", data, mem_mask );
			COMBINE_DATA(&timer_regs.timer2_control);
			break;
		default:
			verboselog(space->machine, 0, "ps_timer_w: Unknown Register %08x & %08x\n", 0x0a800000 + (offset << 2), mem_mask );
			break;
	}
}

static UINT32 ps_unknown1;

static READ32_HANDLER( ps_unk1_r )
{
	verboselog(space->machine, 0, "ps_unk1_r: Unknown 1: %08x & %08x\n", ps_unknown1 | 0x10, mem_mask );
	return ps_unknown1 | 0x10;
}

static WRITE32_HANDLER( ps_unk1_w )
{
	verboselog(space->machine, 0, "ps_unk1_w: Unknown 1: %08x & %08x\n", data, mem_mask );
}

static ADDRESS_MAP_START(pockstat_mem, ADDRESS_SPACE_PROGRAM, 32)
	AM_RANGE(0x00000000, 0x000007ff) AM_RAM
	AM_RANGE(0x04000000, 0x04003fff) AM_ROM AM_REGION("maincpu", 0)
	AM_RANGE(0x06000000, 0x06000307) AM_READWRITE(ps_ftlb_r, ps_ftlb_w)
	AM_RANGE(0x0a000000, 0x0a000013) AM_READWRITE(ps_intc_r, ps_intc_w)
	AM_RANGE(0x0a800000, 0x0a80002b) AM_READWRITE(ps_timer_r, ps_timer_w)
	AM_RANGE(0x0b000000, 0x0b000003) AM_READWRITE(ps_unk1_r, ps_unk1_w)
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
INPUT_PORTS_END


static MACHINE_RESET( pockstat )
{
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), REG_GENPC, 0x4000000);
}

static VIDEO_START( pockstat )
{
}

static VIDEO_UPDATE( pockstat )
{
	return 0;
}

static MACHINE_DRIVER_START( pockstat )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", ARM7, 7900000)	// ??
	MDRV_CPU_PROGRAM_MAP(pockstat_mem)

	MDRV_MACHINE_RESET(pockstat)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", LCD)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(32, 32)
	MDRV_SCREEN_VISIBLE_AREA(0, 32-1, 0, 32-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(pockstat)
	MDRV_VIDEO_UPDATE(pockstat)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( pockstat )
	ROM_REGION( 0x4000, "maincpu", 0 )
	ROM_LOAD( "kernel.bin", 0x0000, 0x4000, CRC(5fb47dd8) SHA1(6ae880493ddde880827d1e9f08e9cb2c38f9f2ec) )
ROM_END

/* Driver */

/*    YEAR  NAME      PARENT  COMPAT  MACHINE    INPUT     INIT  CONFIG     COMPANY                             FULLNAME       FLAGS */
CONS( 1999, pockstat, 0,      0,      pockstat,  pockstat, 0,    0,  "Sony Computer Entertainment Inc.", "Sony PocketStation", GAME_NOT_WORKING)
