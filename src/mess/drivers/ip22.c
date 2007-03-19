/*********************************************************************\
*
*   SGI IP22 Indigo2/Indy workstation
*
*   Todo: Fix tod clock set problem
*         Fix NVRAM saving
*         Fix SCSI DMA to handle chains properly
*         Probably many more things
*
*  NOTE: this driver does not work with the MIPS DRC!
*  You must use the interpreter and take the speed hit :(
*
*  Memory map:
*
*  18000000 - 1effffff      RESERVED - Unused
*  1f000000 - 1f3fffff      GIO - GFX
*  1f400000 - 1f5fffff      GIO - EXP0
*  1f600000 - 1f9fffff      GIO - EXP1 - Unused
*  1fa00000 - 1fa02047      Memory Controller
*  1fb00000 - 1fb1a7ff      HPC3 CHIP1
*  1fb80000 - 1fb9a7ff      HPC3 CHIP0
*  1fc00000 - 1fc7ffff      BIOS
*
*  References used:
*    MipsLinux: http://www.mips-linux.org/
*      linux-2.6.6/include/newport.h
*      linux-2.6.6/include/asm-mips/sgi/gio.h
*      linux-2.6.6/include/asm-mips/sgi/mc.h
*      linux-2.6.6/include/asm-mips/sgi/hpc3.h
*    NetBSD: http://www.netbsd.org/
*    gxemul: http://gavare.se/gxemul/
* 
* Gentoo LiveCD r5 boot instructions:
*     mess -cdrom gentoor5.chd ip225015
*     enter the command interpreter and type "sashARCS".  press enter and 
*     it'll autoboot.
*
* IRIX boot instructions:
*     mess -cdrom irix656inst1.chd ip225015
*     at the menu, choose either "run diagnostics" or "install system software"
*
\*********************************************************************/

#include "driver.h"
#include "cpu/mips/mips3.h"
#include "machine/sgi.h"
#include "machine/pckeybrd.h"
#include "includes/pc_mouse.h"
#include "includes/at.h"
#include "machine/8042kbdc.h"
#include "machine/pit8253.h"
#include "includes/ps2.h"
#include "machine/pcshare.h"
#include "includes/pclpt.h"
#include "video/newport.h"
#include "machine/wd33c93.h"
#include "devices/harddriv.h"
#include "devices/chd_cd.h"
#include "sound/dmadac.h"

#define VERBOSE_LEVEL ( 0 )

static UINT32 *ip22_mainram;

INLINE void verboselog( int n_level, const char *s_fmt, ... )
{
	if( VERBOSE_LEVEL >= n_level )
	{
		va_list v;
		char buf[ 32768 ];
		va_start( v, s_fmt );
		vsprintf( buf, s_fmt, v );
		va_end( v );
		logerror( "%08x: %s", safe_activecpu_get_pc(), buf );
	}
}

UINT8 nRTC_Regs[0x80];
UINT8 nRTC_UserRAM[0x200];
UINT8 nRTC_RAM[0x800];

static UINT32 nHPC_SCSI0Descriptor, nHPC_SCSI0DMACtrl;

static const struct pit8253_config ip22_pit8254_config =
{
	TYPE8254,
	{
		{
			1000000,				/* Timer 0: 1MHz */
			NULL,
			NULL
		}, {
			1000000,				/* Timer 1: 1MHz */
			NULL,
			NULL
		}, {
			1000000,				/* Timer 2: 1MHz */
			NULL,
			NULL
		}
	}
};

static NVRAM_HANDLER( ip22 )
{
	if (read_or_write)
	{
		mame_fwrite(file, nRTC_UserRAM, 0x200);
		mame_fwrite(file, nRTC_RAM, 0x200);
	}
	else
	{
		if (file)
		{
			mame_fread(file, nRTC_UserRAM, 0x200);
			mame_fread(file, nRTC_RAM, 0x200);
		}
	}
}

#define RTC_DAY		nRTC_RAM[0x09]
#define RTC_HOUR	nRTC_RAM[0x08]
#define RTC_MINUTE	nRTC_RAM[0x07]
#define RTC_SECOND	nRTC_RAM[0x06]
#define RTC_HUNDREDTH	nRTC_RAM[0x05]

// interrupt sources handled by INT3
#define INT3_LOCAL0_FIFO	(0x01)
#define INT3_LOCAL0_SCSI0	(0x02)
#define INT3_LOCAL0_SCSI1	(0x04)
#define INT3_LOCAL0_ETHERNET	(0x08)
#define INT3_LOCAL0_MC_DMA	(0x10)
#define INT3_LOCAL0_PARALLEL	(0x20)
#define INT3_LOCAL0_GRAPHICS	(0x40)
#define INT3_LOCAL0_MAPPABLE0	(0x80)

#define INT3_LOCAL1_GP0		(0x01)
#define INT3_LOCAL1_PANEL	(0x02)
#define INT3_LOCAL1_GP2		(0x04)
#define INT3_LOCAL1_MAPPABLE1   (0x08)
#define INT3_LOCAL1_HPC_DMA    	(0x10)
#define INT3_LOCAL1_AC_FAIL     (0x20)
#define INT3_LOCAL1_VSYNC 	(0x40)
#define INT3_LOCAL1_RETRACE	(0x80)

static UINT32 int3_regs[64];
/*static UINT8 nIOC_ParCntl;*/
static UINT32 nIOC_ParReadCnt;

// raise a local0 interrupt
static void int3_raise_local0_irq(UINT8 source_mask)
{
	// signal the interrupt is pending
	int3_regs[0] |= source_mask;

	// if it's not masked, also assert it now at the CPU
	if (int3_regs[1] & source_mask)
	{
		cpunum_set_input_line(0, MIPS3_IRQ0, ASSERT_LINE);
	}
}

// lower a local0 interrupt
static void int3_lower_local0_irq(UINT8 source_mask)
{
	int3_regs[0] &= ~source_mask;
}

// raise a local1 interrupt
static void int3_raise_local1_irq(UINT8 source_mask)
{
	// signal the interrupt is pending
	int3_regs[2] |= source_mask;

	// if it's not masked, also assert it now at the CPU
	if (int3_regs[2] & source_mask)
	{
		cpunum_set_input_line(0, MIPS3_IRQ1, ASSERT_LINE);
	}
}

// lower a local1 interrupt
static void int3_lower_local1_irq(UINT8 source_mask)
{
	int3_regs[2] &= ~source_mask;
}

static READ32_HANDLER( hpc3_pbus6_r )
{
	UINT8 ret8;
	switch( offset )
	{
	case 0x004/4:
		ret8 = pc_parallelport0_r(2) ^ 0x0d;
		verboselog( 0, "Parallel Control Read: %02x\n", ret8 );
		return ret8;
		break;
	case 0x008/4:
//		if( nIOC_ParReadCnt > 7 )
//		{
//			ret8 = 0x40;
//		}
//		else
//		{
			ret8 = pc_parallelport0_r(1) ^ 0x80;
//			nIOC_ParReadCnt++;
//		}
		verboselog( 0, "Parallel Status Read: %02x\n", ret8 );
		return ret8;
		break;
	case 0x030/4:
		verboselog( 2, "Serial 1 Command Transfer Read, 0x1fbd9830: %02x\n", 0x04 );
		switch(activecpu_get_pc())
		{
			case 0x9fc1d9e4:	// interpreter (ip244415)
			case 0x9fc1d9e0:	// DRC (ip244415)
			case 0x9fc1f8e0:	// interpreter (ip224613)
			case 0x9fc1f8dc:	// DRC (ip224613)
			case 0x9fc204c8:	// interpreter (ip225015)
			case 0x9fc204c4:	// DRC (ip225015)
				return 0x00000005;
		}
		return 0x00000004;
		break;
	case 0x038/4:
		verboselog( 2, "Serial 2 Command Transfer Read, 0x1fbd9838: %02x\n", 0x04 );
		return 0x00000004;
		break;
	case 0x40/4:
		return kbdc8042_8_r(0);
		break;
	case 0x44/4:
		return kbdc8042_8_r(4);
		break;
	case 0x58/4:
		return 0x20;	// chip rev 1, board rev 0, "Guinness" (Indy) => 0x01 for "Full House" (Indigo2)
		break;
	case 0x80/4:
	case 0x84/4:
	case 0x88/4:
	case 0x8c/4:
	case 0x90/4:
	case 0x94/4:
	case 0x98/4:
	case 0x9c/4:
	case 0xa0/4:
	case 0xa4/4:
	case 0xa8/4:
	case 0xac/4:
//		mame_printf_info("INT3: r @ %x mask %08x (PC=%x)\n", offset*4, mem_mask, activecpu_get_pc());
		return int3_regs[offset-0x80/4];
		break;
	case 0xb0/4:
		ret8 = pit8253_0_r(0);
		verboselog( 0, "HPC PBUS6 IOC4 Timer Counter 0 Register Read: 0x%02x (%08x)\n", ret8, mem_mask );
		return ret8;
		break;
	case 0xb4/4:
		ret8 = pit8253_0_r(1);
		verboselog( 0, "HPC PBUS6 IOC4 Timer Counter 1 Register Read: 0x%02x (%08x)\n", ret8, mem_mask );
		return ret8;
		break;
	case 0xb8/4:
		ret8 = pit8253_0_r(2);
		verboselog( 0, "HPC PBUS6 IOC4 Timer Counter 2 Register Read: 0x%02x (%08x)\n", ret8, mem_mask );
		return ret8;
		break;
	case 0xbc/4:
		ret8 = pit8253_0_r(3);
		verboselog( 0, "HPC PBUS6 IOC4 Timer Control Word Register Read: 0x%02x (%08x)\n", ret8, mem_mask );
		return ret8;
		break;
	default:
		verboselog( 0, "Unknown HPC PBUS6 Read: 0x%08x (%08x)\n", 0x1fbd9800 + ( offset << 2 ), mem_mask );
		return 0;
		break;
	}
	return 0;
}

static WRITE32_HANDLER( hpc3_pbus6_w )
{
	char cChar;
	switch( offset )
	{
	case 0x004/4:
		verboselog( 0, "Parallel Control Write: %08x\n", data );
		pc_parallelport0_w(2, data ^ 0x0d);
//		nIOC_ParCntl = data;
		break;
	case 0x030/4:
		if( ( data & 0x000000ff ) >= 0x20 )
		{
			verboselog( 2, "Serial 1 Command Transfer Write: %02x: %c\n", data & 0x000000ff, data & 0x000000ff );
		}
		else
		{
			verboselog( 2, "Serial 1 Command Transfer Write: %02x\n", data & 0x000000ff );
		}
		cChar = data & 0x000000ff;
		if( cChar >= 0x20 || cChar == 0x0d || cChar == 0x0a )
		{
//			mame_printf_info( "%c", cChar );
		}
		break;
	case 0x034/4:
		if( ( data & 0x000000ff ) >= 0x20 )
		{
			verboselog( 2, "Serial 1 Data Transfer Write: %02x: %c\n", data & 0x000000ff, data & 0x000000ff );
		}
		else
		{
			verboselog( 2, "Serial 1 Data Transfer Write: %02x\n", data & 0x000000ff );
		}
		cChar = data & 0x000000ff;
		if( cChar >= 0x20 || cChar == 0x0d || cChar == 0x0a )
		{
//			mame_printf_info( "%c", cChar );
		}
		break;
	case 0x40/4:
		kbdc8042_8_w(0, data);
		break;
	case 0x44/4:
		kbdc8042_8_w(4, data);
		break;
	case 0x80/4:
	case 0x84/4:
	case 0x88/4:
	case 0x8c/4:
	case 0x90/4:
	case 0x94/4:
	case 0x98/4:
	case 0x9c/4:
	case 0xa0/4:
	case 0xa4/4:
//		mame_printf_info("INT3: w %x to %x (reg %d) mask %08x (PC=%x)\n", data, offset*4, offset-0x80/4, mem_mask, activecpu_get_pc());
		int3_regs[offset-0x80/4] = data;

		// if no local0 interrupts now, clear the input to the CPU
		if ((int3_regs[0] & int3_regs[1]) == 0)
		{
			cpunum_set_input_line(0, MIPS3_IRQ0, CLEAR_LINE);
		}

		// if no local1 interrupts now, clear the input to the CPU
		if ((int3_regs[2] & int3_regs[3]) == 0)
		{
			cpunum_set_input_line(0, MIPS3_IRQ1, CLEAR_LINE);
		}
		break;
	case 0xb0/4:
		verboselog( 0, "HPC PBUS6 IOC4 Timer Counter 0 Register Write: 0x%08x (%08x)\n", data, mem_mask );
		pit8253_0_w(0, data & 0x000000ff);
		return;
		break;
	case 0xb4/4:
		verboselog( 0, "HPC PBUS6 IOC4 Timer Counter 1 Register Write: 0x%08x (%08x)\n", data, mem_mask );
		pit8253_0_w(1, data & 0x000000ff);
		return;
		break;
	case 0xb8/4:
		verboselog( 0, "HPC PBUS6 IOC4 Timer Counter 2 Register Write: 0x%08x (%08x)\n", data, mem_mask );
		pit8253_0_w(2, data & 0x000000ff);
		return;
		break;
	case 0xbc/4:
		verboselog( 0, "HPC PBUS6 IOC4 Timer Control Word Register Write: 0x%08x (%08x)\n", data, mem_mask );
		pit8253_0_w(3, data & 0x000000ff);
		return;
		break;
	default:
		verboselog( 0, "Unknown HPC PBUS6 Write: 0x%08x: 0x%08x (%08x)\n", 0x1fbd9800 + ( offset << 2 ), data, mem_mask );
		break;
	}
}

UINT32 nHPC3_enetr_nbdp;
UINT32 nHPC3_enetr_cbp;
UINT32 nHPC3_hd0_register;
UINT32 nHPC3_hd0_regs[0x20];
UINT32 nHPC3_hd1_regs[0x20];

static READ32_HANDLER( hpc3_hd_enet_r )
{
	switch( offset )
	{
	case 0x0004/4:
		return nHPC_SCSI0Descriptor;
		break;

	case 0x1004/4:
		return nHPC_SCSI0DMACtrl;
		break;

	case 0x4000/4:
		verboselog( 2, "HPC3 ENETR CBP Read: %08x (%08x): %08x\n", 0x1fb90000 + ( offset << 2), mem_mask, nHPC3_enetr_nbdp );
		return nHPC3_enetr_cbp;
		break;
	case 0x4004/4:
		verboselog( 2, "HPC3 ENETR NBDP Read: %08x (%08x): %08x\n", 0x1fb90000 + ( offset << 2), mem_mask, nHPC3_enetr_nbdp );
		return nHPC3_enetr_nbdp;
		break;
	default:
		verboselog( 0, "Unknown HPC3 ENET/HDx Read: %08x (%08x)\n", 0x1fb90000 + ( offset << 2 ), mem_mask );
		return 0;
		break;
	}
	return 0;
}

static WRITE32_HANDLER( hpc3_hd_enet_w )
{
	switch( offset )
	{
	case 0x0004/4:
		nHPC_SCSI0Descriptor = data;
		break;

	case 0x1004/4:
		nHPC_SCSI0DMACtrl = data;
		break;

	case 0x4000/4:
		verboselog( 2, "HPC3 ENETR CBP Write: %08x\n", data );
		nHPC3_enetr_cbp = data;
		break;
	case 0x4004/4:
		verboselog( 2, "HPC3 ENETR NBDP Write: %08x\n", data );
		nHPC3_enetr_nbdp = data;
		break;
	default:
		verboselog( 0, "Unknown HPC3 ENET/HDx write: %08x (%08x): %08x\n", 0x1fb90000 + ( offset << 2 ), mem_mask, data );
		break;
	}
}

static READ32_HANDLER( hpc3_hd0_r )
{
	switch( offset )
	{
	case 0x0000/4:
	case 0x4000/4:
//		verboselog( 2, "HPC3 HD0 Status Read: %08x (%08x): %08x\n", 0x1fb90000 + ( offset << 2), mem_mask, nHPC3_hd0_regs[0x17] );
		if (!(mem_mask & 0x000000ff))
		{
			return wd33c93_r( 0 );
		}
		else
		{
			return 0;
		}
		break;
	case 0x0004/4:
	case 0x4004/4:
//		verboselog( 2, "HPC3 HD0 Register Read: %08x (%08x): %08x\n", 0x1fb90000 + ( offset << 2), mem_mask, nHPC3_hd0_regs[nHPC3_hd0_register] );
		if (!(mem_mask & 0x000000ff))
		{
			return wd33c93_r( 1 );
		}
		else
		{
			return 0;
		}
		break;
	default:
		verboselog( 0, "Unknown HPC3 HD0 Read: %08x (%08x) [%x] PC=%x\n", 0x1fbc0000 + ( offset << 2 ), mem_mask, offset, activecpu_get_pc() );
		return 0;
		break;
	}
	return 0;
}

static WRITE32_HANDLER( hpc3_hd0_w )
{
	switch( offset )
	{
	case 0x0000/4:
	case 0x4000/4:
//		verboselog( 2, "HPC3 HD0 Register Select Write: %08x\n", data );
		if (!(mem_mask & 0x000000ff))
		{
			wd33c93_w( 0, data & 0x000000ff );
		}
		break;
	case 0x0004/4:
	case 0x4004/4:
//		verboselog( 2, "HPC3 HD0 Register %d Write: %08x\n", nHPC3_hd0_register, data );
		if (!(mem_mask & 0x000000ff))
		{
			wd33c93_w( 1,  data & 0x000000ff );
		}
		break;
	default:
		verboselog( 0, "Unknown HPC3 HD0 Write: %08x (%08x): %08x\n", 0x1fbc0000 + ( offset << 2 ), mem_mask, data );
		break;
	}
}

UINT32 nHPC3_unk0;
UINT32 nHPC3_unk1;
UINT32 nHPC3_IC_Unk0;

static READ32_HANDLER( hpc3_pbus4_r )
{
	switch( offset )
	{
	case 0x0004/4:
		verboselog( 2, "HPC3 PBUS4 Unknown 0 Read: (%08x): %08x\n", mem_mask, nHPC3_unk0 );
		return nHPC3_unk0;
		break;
	case 0x000c/4:
		verboselog( 2, "Interrupt Controller(?) Read: (%08x): %08x\n", mem_mask, nHPC3_IC_Unk0 );
		return nHPC3_IC_Unk0;
		break;
	case 0x0014/4:
		verboselog( 2, "HPC3 PBUS4 Unknown 1 Read: (%08x): %08x\n", mem_mask, nHPC3_unk1 );
		return nHPC3_unk1;
		break;
	default:
		verboselog( 0, "Unknown HPC3 PBUS4 Read: %08x (%08x)\n", 0x1fbd9000 + ( offset << 2 ), mem_mask );
		return 0;
		break;
	}
	return 0;
}

static WRITE32_HANDLER( hpc3_pbus4_w )
{
	switch( offset )
	{
	case 0x0004/4:
		verboselog( 2, "HPC3 PBUS4 Unknown 0 Write: %08x (%08x)\n", data, mem_mask );
		nHPC3_unk0 = data;
		break;
	case 0x000c/4:
		verboselog( 2, "Interrupt Controller(?) Write: (%08x): %08x\n", mem_mask, data );
		nHPC3_IC_Unk0 = data;
		break;
	case 0x0014/4:
		verboselog( 2, "HPC3 PBUS4 Unknown 1 Write: %08x (%08x)\n", data, mem_mask );
		nHPC3_unk1 = data;
		break;
	default:
		verboselog( 0, "Unknown HPC3 PBUS4 Write: %08x (%08x): %08x\n", 0x1fbd9000 + ( offset << 2 ), mem_mask, data );
		break;
	}
}

#define RTC_SECONDS	nRTC_Regs[0x00]
#define RTC_SECONDS_A	nRTC_Regs[0x01]
#define RTC_MINUTES	nRTC_Regs[0x02]
#define RTC_MINUTES_A	nRTC_Regs[0x03]
#define RTC_HOURS	nRTC_Regs[0x04]
#define RTC_HOURS_A	nRTC_Regs[0x05]
#define RTC_DAYOFWEEK	nRTC_Regs[0x06]
#define RTC_DAYOFMONTH	nRTC_Regs[0x07]
#define RTC_MONTH	nRTC_Regs[0x08]
#define RTC_YEAR	nRTC_Regs[0x09]
#define RTC_REGISTERA	nRTC_Regs[0x0a]
#define RTC_REGISTERB	nRTC_Regs[0x0b]
#define RTC_REGISTERC	nRTC_Regs[0x0c]
#define RTC_REGISTERD	nRTC_Regs[0x0d]
#define RTC_MODELBYTE	nRTC_Regs[0x40]
#define RTC_SERBYTE0	nRTC_Regs[0x41]
#define RTC_SERBYTE1	nRTC_Regs[0x42]
#define RTC_SERBYTE2	nRTC_Regs[0x43]
#define RTC_SERBYTE3	nRTC_Regs[0x44]
#define RTC_SERBYTE4	nRTC_Regs[0x45]
#define RTC_SERBYTE5	nRTC_Regs[0x46]
#define RTC_CRC		nRTC_Regs[0x47]
#define RTC_CENTURY	nRTC_Regs[0x48]
#define RTC_DAYOFMONTH_A nRTC_Regs[0x49]
#define RTC_EXTCTRL0	nRTC_Regs[0x4a]
#define RTC_EXTCTRL1	nRTC_Regs[0x4b]
#define RTC_RTCADDR2	nRTC_Regs[0x4e]
#define RTC_RTCADDR3	nRTC_Regs[0x4f]
#define RTC_RAMLSB	nRTC_Regs[0x50]
#define RTC_RAMMSB	nRTC_Regs[0x51]
#define RTC_WRITECNT	nRTC_Regs[0x5e]

static READ32_HANDLER( rtc_r )
{
//	mame_printf_info("RTC_R: offset %x = %x (PC=%x)\n", offset, nRTC_Regs[offset], activecpu_get_pc());

	if( offset <= 0x0d )
	{
		switch( offset )
		{
		case 0x0000:
//			verboselog( 2, "RTC Seconds Read: %d \n", RTC_SECONDS );
			return RTC_SECONDS;
			break;
		case 0x0001:
//			verboselog( 2, "RTC Seconds Alarm Read: %d \n", RTC_SECONDS_A );
			return RTC_SECONDS_A;
			break;
		case 0x0002:
			verboselog( 3, "RTC Minutes Read: %d \n", RTC_MINUTES );
			return RTC_MINUTES;
			break;
		case 0x0003:
			verboselog( 3, "RTC Minutes Alarm Read: %d \n", RTC_MINUTES_A );
			return RTC_MINUTES_A;
			break;
		case 0x0004:
			verboselog( 3, "RTC Hours Read: %d \n", RTC_HOURS );
			return RTC_HOURS;
			break;
		case 0x0005:
			verboselog( 3, "RTC Hours Alarm Read: %d \n", RTC_HOURS_A );
			return RTC_HOURS_A;
			break;
		case 0x0006:
			verboselog( 3, "RTC Day of Week Read: %d \n", RTC_DAYOFWEEK );
			return RTC_DAYOFWEEK;
			break;
		case 0x0007:
			verboselog( 3, "RTC Day of Month Read: %d \n", RTC_DAYOFMONTH );
			return RTC_DAYOFMONTH;
			break;
		case 0x0008:
			verboselog( 3, "RTC Month Read: %d \n", RTC_MONTH );
			return RTC_MONTH;
			break;
		case 0x0009:
			verboselog( 3, "RTC Year Read: %d \n", RTC_YEAR );
			return RTC_YEAR;
			break;
		case 0x000a:
			verboselog( 3, "RTC Register A Read: %02x \n", RTC_REGISTERA );
			return RTC_REGISTERA;
			break;
		case 0x000b:
			verboselog( 3, "RTC Register B Read: %02x \n", RTC_REGISTERB );
			return RTC_REGISTERB;
			break;
		case 0x000c:
			verboselog( 3, "RTC Register C Read: %02x \n", RTC_REGISTERC );
			return RTC_REGISTERC;
			break;
		case 0x000d:
			verboselog( 3, "RTC Register D Read: %02x \n", RTC_REGISTERD );
			return RTC_REGISTERD;
			break;
		default:
			verboselog( 3, "Unknown RTC Read: %08x (%08x)\n", 0x1fbe0000 + ( offset << 2 ), mem_mask );
			return 0;
			break;
		}
	}
	if( offset >= 0x0e && offset < 0x40 )
	{
		return nRTC_Regs[offset];
	}
	if( offset >= 0x40 && offset < 0x80 && !( RTC_REGISTERA & 0x10 ) )
	{
		return nRTC_UserRAM[offset - 0x40];
	}
	if( offset >= 0x40 && offset < 0x80 && ( RTC_REGISTERA & 0x10 ) )
	{
		switch( offset )
		{
		case 0x0040:
			verboselog( 3, "RTC Model Byte Read: %02x\n", RTC_MODELBYTE );
			return RTC_MODELBYTE;
			break;
		case 0x0041:
			verboselog( 3, "RTC Serial Byte 0 Read: %02x\n", RTC_SERBYTE0 );
			return RTC_SERBYTE0;
			break;
		case 0x0042:
			verboselog( 3, "RTC Serial Byte 1 Read: %02x\n", RTC_SERBYTE1 );
			return RTC_SERBYTE1;
			break;
		case 0x0043:
			verboselog( 3, "RTC Serial Byte 2 Read: %02x\n", RTC_SERBYTE2 );
			return RTC_SERBYTE2;
			break;
		case 0x0044:
			verboselog( 3, "RTC Serial Byte 3 Read: %02x\n", RTC_SERBYTE3 );
			return RTC_SERBYTE3;
			break;
		case 0x0045:
			verboselog( 3, "RTC Serial Byte 4 Read: %02x\n", RTC_SERBYTE4 );
			return RTC_SERBYTE4;
			break;
		case 0x0046:
			verboselog( 3, "RTC Serial Byte 5 Read: %02x\n", RTC_SERBYTE5 );
			return RTC_SERBYTE5;
			break;
		case 0x0047:
			verboselog( 3, "RTC CRC Read: %02x\n", RTC_CRC );
			return RTC_CRC;
			break;
		case 0x0048:
			verboselog( 3, "RTC Century Read: %02x\n", RTC_CENTURY );
			return RTC_CENTURY;
			break;
		case 0x0049:
			verboselog( 3, "RTC Day of Month Alarm Read: %02x \n", RTC_DAYOFMONTH_A );
			return RTC_DAYOFMONTH_A;
			break;
		case 0x004a:
			verboselog( 3, "RTC Extended Control 0 Read: %02x \n", RTC_EXTCTRL0 );
			return RTC_EXTCTRL0;
			break;
		case 0x004b:
			verboselog( 3, "RTC Extended Control 1 Read: %02x \n", RTC_EXTCTRL1 );
			return RTC_EXTCTRL1;
			break;
		case 0x004e:
			verboselog( 3, "RTC SMI Recovery Address 2 Read: %02x \n", RTC_RTCADDR2 );
			return RTC_RTCADDR2;
			break;
		case 0x004f:
			verboselog( 3, "RTC SMI Recovery Address 3 Read: %02x \n", RTC_RTCADDR3 );
			return RTC_RTCADDR3;
			break;
		case 0x0050:
			verboselog( 3, "RTC RAM LSB Read: %02x \n", RTC_RAMLSB );
			return RTC_RAMLSB;
			break;
		case 0x0051:
			verboselog( 3, "RTC RAM MSB Read: %02x \n", RTC_RAMMSB );
			return RTC_RAMMSB;
			break;
		case 0x0053:
			return nRTC_RAM[ ( RTC_RAMMSB << 8 ) | RTC_RAMLSB ];
			break;
		case 0x005e:
			return RTC_WRITECNT;
			break;
		default:
			verboselog( 3, "Unknown RTC Ext. Reg. Read: %02x\n", offset );
			return 0;
			break;
		}
	}
	if( offset >= 0x80 )
	{
		return nRTC_UserRAM[ offset - 0x80 ];
	}
	return 0;
}

static WRITE32_HANDLER( rtc_w )
{
	RTC_WRITECNT++;

//	mame_printf_info("RTC_W: offset %x => %x (PC=%x)\n", data, offset, activecpu_get_pc());

	if( offset <= 0x0d )
	{
		switch( offset )
		{
		case 0x0000:
			verboselog( 3, "RTC Seconds Write: %02x \n", data );
			RTC_SECONDS = data;
			break;
		case 0x0001:
			verboselog( 3, "RTC Seconds Alarm Write: %02x \n", data );
			RTC_SECONDS_A = data;
			break;
		case 0x0002:
			verboselog( 3, "RTC Minutes Write: %02x \n", data );
			RTC_MINUTES = data;
			break;
		case 0x0003:
			verboselog( 3, "RTC Minutes Alarm Write: %02x \n", data );
			RTC_MINUTES_A = data;
			break;
		case 0x0004:
			verboselog( 3, "RTC Hours Write: %02x \n", data );
			RTC_HOURS = data;
			break;
		case 0x0005:
			verboselog( 3, "RTC Hours Alarm Write: %02x \n", data );
			RTC_HOURS_A = data;
			break;
		case 0x0006:
			verboselog( 3, "RTC Day of Week Write: %02x \n", data );
			RTC_DAYOFWEEK = data;
			break;
		case 0x0007:
			verboselog( 3, "RTC Day of Month Write: %02x \n", data );
			RTC_DAYOFMONTH = data;
			break;
		case 0x0008:
			verboselog( 3, "RTC Month Write: %02x \n", data );
			RTC_MONTH = data;
			break;
		case 0x0009:
			verboselog( 3, "RTC Year Write: %02x \n", data );
			RTC_YEAR = data;
			break;
		case 0x000a:
			verboselog( 3, "RTC Register A Write (Bit 7 Ignored): %02x \n", data );
			RTC_REGISTERA = data & 0x0000007f;
			break;
		case 0x000b:
			verboselog( 3, "RTC Register B Write: %02x \n", data );
			RTC_REGISTERB = data;
			break;
		case 0x000c:
			verboselog( 3, "RTC Register C Write (Ignored): %02x \n", data );
			break;
		case 0x000d:
			verboselog( 3, "RTC Register D Write (Ignored): %02x \n", data );
			break;
		default:
			verboselog( 3, "Unknown RTC Write: %08x (%08x): %08x\n", 0x1fbe0000 + ( offset << 2 ), mem_mask, data );
			break;
		}
	}
	if( offset >= 0x0e && offset < 0x40 )
	{
		nRTC_Regs[offset] = data;
		return;
	}
	if( offset >= 0x40 && offset < 0x80 && !( RTC_REGISTERA & 0x10 ) )
	{
		nRTC_UserRAM[offset - 0x40] = data;
		return;
	}
	if( offset >= 0x40 && offset < 0x80 && ( RTC_REGISTERA & 0x10 ) )
	{
		switch( offset )
		{
		case 0x0040:
		case 0x0041:
		case 0x0042:
		case 0x0043:
		case 0x0044:
		case 0x0045:
		case 0x0046:
			verboselog( 3, "Invalid write to RTC serial number byte %d: %02x\n", offset - 0x0040, data );
			break;
		case 0x0047:
			verboselog( 3, "RTC Century Write: %02x \n", data );
			RTC_CENTURY = data;
			break;
		case 0x0048:
			verboselog( 3, "RTC Century Write: %02x \n", data );
			RTC_CENTURY = data;
			break;
		case 0x0049:
			verboselog( 3, "RTC Day of Month Alarm Write: %02x \n", data );
			RTC_DAYOFMONTH_A = data;
			break;
		case 0x004a:
			verboselog( 3, "RTC Extended Control 0 Write: %02x \n", data );
			RTC_EXTCTRL0 = data;
			break;
		case 0x004b:
			verboselog( 3, "RTC Extended Control 1 Write: %02x \n", data );
			RTC_EXTCTRL1 = data;
			break;
		case 0x004e:
			verboselog( 3, "RTC SMI Recovery Address 2 Write: %02x \n", data );
			RTC_RTCADDR2 = data;
			break;
		case 0x004f:
			verboselog( 3, "RTC SMI Recovery Address 3 Write: %02x \n", data );
			RTC_RTCADDR3 = data;
			break;
		case 0x0050:
			verboselog( 3, "RTC RAM LSB Write: %02x \n", data );
			RTC_RAMLSB = data;
			break;
		case 0x0051:
			verboselog( 3, "RTC RAM MSB Write: %02x \n", data );
			RTC_RAMMSB = data;
			break;
		case 0x0053:
			nRTC_RAM[ ( RTC_RAMMSB << 8 ) | RTC_RAMLSB ] = data;
			break;
		default:
			verboselog( 3, "Unknown RTC Ext. Reg. Write: %02x: %02x\n", offset, data );
			break;
		}
	}
	if( offset >= 0x80 )
	{
		nRTC_UserRAM[ offset - 0x80 ] = data;
	}
}

static READ32_HANDLER( unk_r )
{
//	mame_printf_info("UNK_R: PC=%x\n", activecpu_get_pc());
	return 0x12345678;
}

static READ32_HANDLER( deadbeef_r )
{
	return 0xdeadbeef;
}

static READ32_HANDLER( ffffffff_r )
{
	return 0xffffffff;
}

// a bit hackish, but makes the memory detection work properly and allows a big cleanup of the mapping
static WRITE32_HANDLER( ip22_write_ram )
{
	// if banks 2 or 3 are enabled, do nothing, we don't support that much memory
	if (mc_r(0xc8/4, 0xffffffff) & 0x10001000)
	{
		// a random perturbation so the memory test fails
		data ^= 0xffffffff;
	}

	// if banks 0 or 1 have 2 subbanks, also kill it, we only want 128 MB
	if (mc_r(0xc0/4, 0xffffffff) & 0x40004000)
	{
		// a random perturbation so the memory test fails
		data ^= 0xffffffff;
	}
	COMBINE_DATA(&ip22_mainram[offset]);
}

UINT32 nHAL2_IAR;
UINT32 nHAL2_IDR[4];

#define H2_IAR_TYPE			0xf000
#define H2_IAR_NUM			0x0f00
#define H2_IAR_ACCESS_SEL	0x0080
#define H2_IAR_PARAM		0x000c
#define H2_IAR_RB_INDEX		0x0003

#define H2_ISR_TSTATUS		0x01
#define H2_ISR_USTATUS		0x02
#define H2_ISR_QUAD_MODE	0x04
#define H2_ISR_GLOBAL_RESET	0x08
#define H2_ISR_CODEC_RESET	0x10

static READ32_HANDLER( hal2_r )
{
	switch( offset )
	{
	case 0x0010/4:
		verboselog( 0, "HAL2 Status read: 0x0004\n", 0x0004 );
		return 0x0004;
		break;
	case 0x0020/4:
		verboselog( 0, "HAL2 Revision read: 0x4011\n", 0x4011 );
		return 0x4011;
		break;
	}
	verboselog( 0, "Unknown HAL2 read: 0x%08x (%08x)\n", 0x1fbd8000 + offset*4, mem_mask );
	return 0;
}

static WRITE32_HANDLER( hal2_w )
{
	switch( offset )
	{
	case 0x0010/4:
		verboselog( 0, "HAL2 Status Write: 0x%08x (%08x)\n", data, mem_mask );
		if( data & H2_ISR_GLOBAL_RESET )
		{
			verboselog( 0, "    HAL2 Global Reset\n" );
		}
		if( data & H2_ISR_CODEC_RESET )
		{
			verboselog( 0, "    HAL2 Codec Reset\n" );
		}
		break;
	case 0x0030/4:
		verboselog( 0, "HAL2 Indirect Address Register Write: 0x%08x (%08x)\n", data, mem_mask );
		nHAL2_IAR = data;
		switch( data & H2_IAR_TYPE )
		{
		case 0x1000:
			verboselog( 0, "    DMA Port\n" );
			switch( data & H2_IAR_NUM )
			{
			case 0x0100:
				verboselog( 0, "        Synth In\n" );
				break;
			case 0x0200:
				verboselog( 0, "        AES In\n" );
				break;
			case 0x0300:
				verboselog( 0, "        AES Out\n" );
				break;
			case 0x0400:
				verboselog( 0, "        DAC Out\n" );
				break;
			case 0x0500:
				verboselog( 0, "        ADC Out\n" );
				break;
			case 0x0600:
				verboselog( 0, "        Synth Control\n" );
				break;
			}
			break;
		case 0x2000:
			verboselog( 0, "    Bresenham\n" );
			switch( data & H2_IAR_NUM )
			{
			case 0x0100:
				verboselog( 0, "        Bresenham Clock Gen 1\n" );
				break;
			case 0x0200:
				verboselog( 0, "        Bresenham Clock Gen 2\n" );
				break;
			case 0x0300:
				verboselog( 0, "        Bresenham Clock Gen 3\n" );
				break;
			}
			break;
		case 0x3000:
			verboselog( 0, "    Unix Timer\n" );
			switch( data & H2_IAR_NUM )
			{
			case 0x0100:
				verboselog( 0, "        Unix Timer\n" );
				break;
			}
			break;
		case 0x9000:
			verboselog( 0, "    Global DMA Control\n" );
			switch( data & H2_IAR_NUM )
			{
			case 0x0100:
				verboselog( 0, "        DMA Control\n" );
				break;
			}
			break;
		}
		switch( data & H2_IAR_ACCESS_SEL )
		{
		case 0x0000:
			verboselog( 0, "    Write\n" );
			break;
		case 0x0080:
			verboselog( 0, "    Read\n" );
			break;
		}
		verboselog( 0, "    Parameter: %01x\n", ( data & H2_IAR_PARAM ) >> 2 );
		return;
		verboselog( 0, "    Read Back Index: %01x\n", ( data & H2_IAR_RB_INDEX ) );
		break;
	case 0x0040/4:
		verboselog( 0, "HAL2 Indirect Data Register 0 Write: 0x%08x (%08x)\n", data, mem_mask );
		nHAL2_IDR[0] = data;
		return;
		break;
	case 0x0050/4:
		verboselog( 0, "HAL2 Indirect Data Register 1 Write: 0x%08x (%08x)\n", data, mem_mask );
		nHAL2_IDR[1] = data;
		return;
		break;
	case 0x0060/4:
		verboselog( 0, "HAL2 Indirect Data Register 2 Write: 0x%08x (%08x)\n", data, mem_mask );
		nHAL2_IDR[2] = data;
		return;
		break;
	case 0x0070/4:
		verboselog( 0, "HAL2 Indirect Data Register 3 Write: 0x%08x (%08x)\n", data, mem_mask );
		nHAL2_IDR[3] = data;
		return;
		break;
	}
	verboselog( 0, "Unknown HAL2 write: 0x%08x: 0x%08x (%08x)\n", 0x1fbd8000 + offset*4, data, mem_mask );
}

#define PBUS_CTRL_ENDIAN		0x00000002
#define PBUS_CTRL_RECV			0x00000004
#define PBUS_CTRL_FLUSH			0x00000008
#define PBUS_CTRL_DMASTART		0x00000010
#define PBUS_CTRL_LOAD_EN		0x00000020
#define PBUS_CTRL_REALTIME		0x00000040
#define PBUS_CTRL_HIGHWATER		0x0000ff00
#define PBUS_CTRL_FIFO_BEG		0x003f0000
#define PBUS_CTRL_FIFO_END		0x3f000000

#define PBUS_DMADESC_EOX		0x80000000
#define PBUS_DMADESC_EOXP		0x40000000
#define PBUS_DMADESC_XIE		0x20000000
#define PBUS_DMADESC_IPG		0x00ff0000
#define PBUS_DMADESC_TXD		0x00008000
#define PBUS_DMADESC_BC			0x00003fff

UINT8 nPBUS_DMA_Active;
UINT32 nPBUS_DMA_CurPtr;
UINT32 nPBUS_DMA_DescPtr;
UINT32 nPBUS_DMA_NextPtr;
UINT32 nPBUS_DMA_WordsLeft;

static void ip22_dma(int refcon)
{
	if( nPBUS_DMA_Active )
	{
		INT16 temp16;
//		mame_printf_info( "nPBUS_DMA_CurPtr - 0x08000000/4 = %08x\n", (nPBUS_DMA_CurPtr - 0x08000000)/4 );
		verboselog( 0, "nPBUS_DMA_CurPtr - 0x08000000/4 = %08x\n", (nPBUS_DMA_CurPtr - 0x08000000)/4 );
		temp16 = ( ip22_mainram[(nPBUS_DMA_CurPtr - 0x08000000)/4] & 0xffff0000 ) >> 16;
		temp16 = ( ( temp16 & 0xff00 ) >> 8 ) | ( ( temp16 & 0x00ff ) << 8 );
		dmadac_transfer(0, 1, 1, 1, 1, &temp16);
		nPBUS_DMA_CurPtr += 4;
		nPBUS_DMA_WordsLeft -= 4;

		if( nPBUS_DMA_WordsLeft == 0 )
		{
			if( nPBUS_DMA_NextPtr != 0 )
			{
				nPBUS_DMA_DescPtr = nPBUS_DMA_NextPtr;
				nPBUS_DMA_CurPtr = ip22_mainram[(nPBUS_DMA_DescPtr - 0x08000000)/4];
				nPBUS_DMA_WordsLeft = ip22_mainram[(nPBUS_DMA_DescPtr - 0x08000000)/4+1];
				nPBUS_DMA_NextPtr = ip22_mainram[(nPBUS_DMA_DescPtr - 0x08000000)/4+2];
			}
			else
			{
				nPBUS_DMA_Active = 0;
				return;
			}
		}
		timer_set(TIME_IN_HZ(44100), 0, ip22_dma);
	}
}

static READ32_HANDLER( hpc3_pbusdma_r )
{
	UINT32 channel = offset / (0x2000/4);
	verboselog( 0, "PBUS DMA Channel %d Read: 0x%08x (%08x)\n", channel, 0x1fb80000 + offset*4, mem_mask );
	return 0;
}

static WRITE32_HANDLER( hpc3_pbusdma_w )
{
	UINT32 channel = offset / (0x2000/4);
	switch( offset & 0x07ff )
	{
	case 0x0000/4:
		verboselog( 0, "PBUS DMA Channel %d Buffer Pointer Write: 0x%08x\n", channel, data );
		return;
		break;
	case 0x0004/4:
		verboselog( 0, "PBUS DMA Channel %d Descriptor Pointer Write: 0x%08x\n", channel, data );
		if( channel == 1 )
		{
			nPBUS_DMA_DescPtr = data;
			nPBUS_DMA_CurPtr = ip22_mainram[(nPBUS_DMA_DescPtr - 0x08000000)/4];
			nPBUS_DMA_WordsLeft = ip22_mainram[(nPBUS_DMA_DescPtr - 0x08000000)/4+1];
			nPBUS_DMA_NextPtr = ip22_mainram[(nPBUS_DMA_DescPtr - 0x08000000)/4+2];
			verboselog( 0, "nPBUS_DMA_DescPtr = %08x\n", nPBUS_DMA_DescPtr );
			verboselog( 0, "nPBUS_DMA_CurPtr = %08x\n", nPBUS_DMA_CurPtr );
			verboselog( 0, "nPBUS_DMA_WordsLeft = %08x\n", nPBUS_DMA_WordsLeft );
			verboselog( 0, "nPBUS_DMA_NextPtr = %08x\n", nPBUS_DMA_NextPtr );
		}
		return;
		break;
	case 0x1000/4:
		verboselog( 0, "PBUS DMA Channel %d Control Register Write: 0x%08x\n", channel, data );
		if( data & PBUS_CTRL_ENDIAN )
		{
			verboselog( 0, "    Little Endian\n" );
		}
		else
		{
			verboselog( 0, "    Big Endian\n" );
		}
		if( data & PBUS_CTRL_RECV )
		{
			verboselog( 0, "    RX DMA\n" );
		}
		else
		{
			verboselog( 0, "    TX DMA\n" );
		}
		if( data & PBUS_CTRL_FLUSH )
		{
			verboselog( 0, "    Flush for RX\n" );
		}
		if( data & PBUS_CTRL_DMASTART )
		{
			verboselog( 0, "    Start DMA\n" );
		}
		if( data & PBUS_CTRL_LOAD_EN )
		{
			verboselog( 0, "    Load Enable\n" );
		}
		verboselog( 0, "    High Water Mark: %04x bytes\n", ( data & PBUS_CTRL_HIGHWATER ) >> 8 );
		verboselog( 0, "    FIFO Begin: Row %04x\n", ( data & PBUS_CTRL_FIFO_BEG ) >> 16 );
		verboselog( 0, "    FIFO End: Rowe %04x\n", ( data & PBUS_CTRL_FIFO_END ) >> 24 );
		if( ( data & PBUS_CTRL_DMASTART ) || ( data & PBUS_CTRL_LOAD_EN ) )
		{
			timer_set(TIME_IN_HZ(44100), 0, ip22_dma);
			nPBUS_DMA_Active = 1;
		}
		return;
		break;
	}
	verboselog( 0, "Unknown PBUS DMA Channel %d Write: 0x%08x: 0x%08x (%08x)\n", channel, 0x1fb80000 + offset*4, data, mem_mask );
}

static ADDRESS_MAP_START( ip225015_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE( 0x00000000, 0x0007ffff ) AM_READWRITE( MRA32_BANK1, MWA32_BANK1 )	/* mirror of first 512k of main RAM */
	AM_RANGE( 0x08000000, 0x0fffffff ) AM_RAM AM_SHARE(1) AM_BASE( &ip22_mainram ) AM_WRITE(ip22_write_ram)		/* 128 MB of main RAM */
	AM_RANGE( 0x1f0f0000, 0x1f0f1fff ) AM_READWRITE( newport_rex3_r, newport_rex3_w )
	AM_RANGE( 0x1fa00000, 0x1fa1ffff ) AM_READWRITE( mc_r, mc_w )
	AM_RANGE( 0x1fb90000, 0x1fb9ffff ) AM_READWRITE( hpc3_hd_enet_r, hpc3_hd_enet_w )
	AM_RANGE( 0x1fbb0000, 0x1fbb0003 ) AM_RAM 	/* unknown, but read a lot and discarded */
	AM_RANGE( 0x1fbc0000, 0x1fbc7fff ) AM_READWRITE( hpc3_hd0_r, hpc3_hd0_w )
	AM_RANGE( 0x1fb80000, 0x1fb8ffff ) AM_READWRITE( hpc3_pbusdma_r, hpc3_pbusdma_w )
	AM_RANGE( 0x1fbd8000, 0x1fbd83ff ) AM_READWRITE( hal2_r, hal2_w )
	AM_RANGE( 0x1fbd9000, 0x1fbd93ff ) AM_READWRITE( hpc3_pbus4_r, hpc3_pbus4_w )
	AM_RANGE( 0x1fbd9800, 0x1fbd9bff ) AM_READWRITE( hpc3_pbus6_r, hpc3_pbus6_w )
	AM_RANGE( 0x1fbe0000, 0x1fbe04ff ) AM_READWRITE( rtc_r, rtc_w )
	AM_RANGE( 0x1fc00000, 0x1fc7ffff ) AM_ROM AM_REGION( REGION_USER1, 0 )
	AM_RANGE( 0x20000000, 0x27ffffff ) AM_RAM AM_SHARE(1) AM_WRITE(ip22_write_ram)
ADDRESS_MAP_END

UINT32 nIntCounter;

// mc_update wants once every millisecond (1/1000th of a second)
static void ip22_timer(int refcon)
{
	mc_update(0);

	timer_set(TIME_IN_MSEC(1), 0, ip22_timer);
}

static MACHINE_RESET( ip225015 )
{
	mc_init();
	nHPC3_enetr_nbdp = 0x80000000;
	nHPC3_enetr_cbp = 0x80000000;
	nIntCounter = 0;
	RTC_REGISTERB = 0x08;
	RTC_REGISTERD = 0x80;

	timer_set(TIME_IN_MSEC(1), 0, ip22_timer);

	// set up low RAM mirror
	memory_set_bankptr(1, ip22_mainram);

	nPBUS_DMA_Active = 0;

	dmadac_set_frequency(0, 1, 44100);
	dmadac_enable(0, 1, 1);
}

static void dump_chain(UINT32 ch_base)
{
//	mame_printf_info("node: %08x %08x %08x (len = %x)\n", program_read_dword(ch_base), program_read_dword(ch_base+4), program_read_dword(ch_base+8), program_read_dword(ch_base+4) & 0x3fff);

	if ((program_read_dword(ch_base+8) != 0) && !(program_read_dword(ch_base+4) & 0x80000000))
	{
		dump_chain(program_read_dword(ch_base+8));
	}
}

// HPC3 SCSI DMA control register bits
#define HPC3_DMACTRL_IRQ	(0x01)
#define HPC3_DMACTRL_ENDIAN	(0x02)
#define HPC3_DMACTRL_DIR	(0x04)
#define HPC3_DMACTRL_ENABLE	(0x10)

static UINT8 dma_buffer[4096];

static void scsi_irq(int state)
{
	if (state)
	{
		if (wd33c93_get_dma_count())
		{
			if (nHPC_SCSI0DMACtrl & HPC3_DMACTRL_ENABLE)
			{
				if (nHPC_SCSI0DMACtrl & HPC3_DMACTRL_IRQ) logerror("IP22: Unhandled SCSI DMA IRQ\n");
			}

			// HPC3 DMA: host to device
			if ((nHPC_SCSI0DMACtrl & HPC3_DMACTRL_ENABLE) && (nHPC_SCSI0DMACtrl & HPC3_DMACTRL_DIR))
			{
				UINT32 rptr, dptr, tmpword;
				int length;

				rptr = program_read_dword(nHPC_SCSI0Descriptor);
				length = program_read_dword(nHPC_SCSI0Descriptor+4) & 0x3fff;

/*				mame_printf_info("DMA to device: length %x\n", length);
				mame_printf_info("first words: %08x %08x %08x %08x\n",
					program_read_dword(rptr),
					program_read_dword(rptr+4),
					program_read_dword(rptr+8),
					program_read_dword(rptr+12));*/

				if (length <= 4096)
				{
					dptr = 0;
					while (length > 0)
					{
						tmpword = program_read_dword(rptr);
						if (nHPC_SCSI0DMACtrl & HPC3_DMACTRL_ENDIAN)
						{
							dma_buffer[dptr+3] = (tmpword>>24)&0xff;
							dma_buffer[dptr+2] = (tmpword>>16)&0xff;
							dma_buffer[dptr+1] = (tmpword>>8)&0xff;
							dma_buffer[dptr] = tmpword&0xff;
						}
						else
						{
							dma_buffer[dptr] = (tmpword>>24)&0xff;
							dma_buffer[dptr+1] = (tmpword>>16)&0xff;
							dma_buffer[dptr+2] = (tmpword>>8)&0xff;
							dma_buffer[dptr+3] = tmpword&0xff;
						}

						dptr += 4;
						rptr += 4;
						length -= 4;
					}

					length = program_read_dword(nHPC_SCSI0Descriptor+4) & 0x3fff;
					wd33c93_write_data(length, dma_buffer);

					// clear DMA on the controller too
					wd33c93_clear_dma();
				}
				else
				{
					logerror("IP22: overly large host to device transfer, can't handle!\n");
				}
			}

			// HPC3 DMA: device to host
			if ((nHPC_SCSI0DMACtrl & HPC3_DMACTRL_ENABLE) && !(nHPC_SCSI0DMACtrl & HPC3_DMACTRL_DIR))
			{
				UINT32 wptr, tmpword;
				int words, sptr, twords;
		
				words = wd33c93_get_dma_count();
				words /= 4;

				wptr = program_read_dword(nHPC_SCSI0Descriptor);
				sptr = 0;

//				mame_printf_info("DMA from device: %d words @ %x\n", words, wptr);

				dump_chain(nHPC_SCSI0Descriptor);

				if (words <= (1024/4))
				{
					// one-shot
					wd33c93_read_data(wd33c93_get_dma_count(), dma_buffer);

					while (words)
					{
						if (nHPC_SCSI0DMACtrl & HPC3_DMACTRL_ENDIAN)
						{
							tmpword = dma_buffer[sptr+3]<<24 | dma_buffer[sptr+2]<<16 | dma_buffer[sptr+1]<<8 | dma_buffer[sptr];
						}
						else
						{
							tmpword = dma_buffer[sptr]<<24 | dma_buffer[sptr+1]<<16 | dma_buffer[sptr+2]<<8 | dma_buffer[sptr+3];
						}
						program_write_dword(wptr, tmpword);
						wptr += 4;
						sptr += 4;
						words--;
					}
				}
				else
				{
					while (words)
					{
						wd33c93_read_data(512, dma_buffer);
						twords = 512/4;
						sptr = 0;

						while (twords)
						{
							if (nHPC_SCSI0DMACtrl & HPC3_DMACTRL_ENDIAN)
							{
								tmpword = dma_buffer[sptr+3]<<24 | dma_buffer[sptr+2]<<16 | dma_buffer[sptr+1]<<8 | dma_buffer[sptr];
							}
							else
							{
								tmpword = dma_buffer[sptr]<<24 | dma_buffer[sptr+1]<<16 | dma_buffer[sptr+2]<<8 | dma_buffer[sptr+3];
							}
							program_write_dword(wptr, tmpword);

							wptr += 4;
							sptr += 4;
							twords--;
						}

						words -= (512/4);
					}
				}

				// clear DMA on the controller too
				wd33c93_clear_dma();
			}
		}

		// clear HPC3 DMA active flag
		nHPC_SCSI0DMACtrl &= ~HPC3_DMACTRL_ENABLE;

		// set the interrupt
		int3_raise_local0_irq(INT3_LOCAL0_SCSI0);
	}
	else
	{
		int3_lower_local0_irq(INT3_LOCAL0_SCSI0);
	}
}

static SCSIConfigTable dev_table =
{
        2,                                      /* 1 SCSI device */
        { { SCSI_ID_4, 0, SCSI_DEVICE_CDROM },  /* SCSI ID 4, using CD 0, and it's a CD-ROM */ 
	  { SCSI_ID_2, 0, SCSI_DEVICE_CDROM } } /* SCSI ID 2, using HD 0, and it's a CD-ROM */ 
};

static struct WD33C93interface scsi_intf =
{
	&dev_table,		/* SCSI device table */
	&scsi_irq,		/* command completion IRQ */
};

static DRIVER_INIT( ip225015 )
{
	struct kbdc8042_interface at8042 =
	{
		KBDC8042_STANDARD, NULL, NULL
	};

	// IP22 uses 2 pieces of PC-compatible hardware: the 8042 PS/2 keyboard/mouse
	// interface and the 8254 PIT.  Both are licensed cores embedded in the IOC custom chip.
	init_pc_common(PCCOMMON_KEYBOARD_AT | PCCOMMON_TIMER_NONE );
	kbdc8042_init(&at8042);
	pit8253_init(1, &ip22_pit8254_config);

	// SCSI init
	wd33c93_init(&scsi_intf);

	nIOC_ParReadCnt = 0;
}

INPUT_PORTS_START( ip225015 )
	PORT_START	// unused IN0
	PORT_START	// unused IN1
	PORT_START	// unused IN2
	PORT_START	// unused IN3
	PORT_INCLUDE( at_keyboard )		/* IN4 - IN11 */
	PORT_INCLUDE( pc_mouse_microsoft )	/* IN12 - IN14 */
INPUT_PORTS_END

static void rtc_update(void)
{
	RTC_SECONDS++;

	switch( RTC_REGISTERB & 0x04 )
	{
	case 0x00:	/* Non-BCD */
		if( RTC_SECONDS == 60 )
		{
			RTC_SECONDS = 0;
			RTC_MINUTES++;
		}
		if( RTC_MINUTES == 60 )
		{
			RTC_MINUTES = 0;
			RTC_HOURS++;
		}
		if( RTC_HOURS == 24 )
		{
			RTC_HOURS = 0;
			RTC_DAYOFMONTH++;
		}
		RTC_SECONDS_A = RTC_SECONDS;
		RTC_MINUTES_A = RTC_MINUTES;
		RTC_HOURS_A = RTC_HOURS;
		break;
	case 0x04:	/* BCD */
		if( ( RTC_SECONDS & 0x0f ) == 0x0a )
		{
			RTC_SECONDS -= 0x0a;
			RTC_SECONDS += 0x10;
		}
		if( ( RTC_SECONDS & 0xf0 ) == 0x60 )
		{
			RTC_SECONDS -= 0x60;
			RTC_MINUTES++;
		}
		if( ( RTC_MINUTES & 0x0f ) == 0x0a )
		{
			RTC_MINUTES -= 0x0a;
			RTC_MINUTES += 0x10;
		}
		if( ( RTC_MINUTES & 0xf0 ) == 0x60 )
		{
			RTC_MINUTES -= 0x60;
			RTC_HOURS++;
		}
		if( ( RTC_HOURS & 0x0f ) == 0x0a )
		{
			RTC_HOURS -= 0x0a;
			RTC_HOURS += 0x10;
		}
		if( RTC_HOURS == 0x24 )
		{
			RTC_HOURS = 0;
			RTC_DAYOFMONTH++;
		}
		RTC_SECONDS_A = RTC_SECONDS;
		RTC_MINUTES_A = RTC_MINUTES;
		RTC_HOURS_A = RTC_HOURS;
		break;
	}
}

static INTERRUPT_GEN( ip22_vbl )
{
	nIntCounter++;
//	if( nIntCounter == 60 )
	{
		nIntCounter = 0;
		rtc_update();
	}
}

static struct mips3_config config =
{
	32768,	/* code cache size */
	32768	/* data cache size */
};

static void ip22_chdcd_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* CHD CD-ROM */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 4; break;

		default: cdrom_device_getinfo(devclass, state, info); break;
	}
}

static void ip22_harddisk_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* harddisk */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 2; break;

		default: harddisk_device_getinfo(devclass, state, info); break;
	}
}

MACHINE_DRIVER_START( ip225015 )
	MDRV_CPU_ADD_TAG( "main", R5000BE, 50000000*3 )
	MDRV_CPU_CONFIG( config )
	MDRV_CPU_PROGRAM_MAP( ip225015_map, 0 )
	MDRV_CPU_VBLANK_INT( ip22_vbl, 1 )

	MDRV_SCREEN_REFRESH_RATE( 60 )
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET( ip225015 )
	MDRV_NVRAM_HANDLER( ip22 )

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB15)
	MDRV_SCREEN_SIZE(1280+64, 1024+64)
	MDRV_SCREEN_VISIBLE_AREA(0, 1279, 0, 1023)
	MDRV_PALETTE_LENGTH(65536)

	MDRV_VIDEO_START( newport )
	MDRV_VIDEO_UPDATE( newport )

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(DMADAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
	MDRV_SOUND_ADD( CDDA, 0 )
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( ip224613 )
	MDRV_IMPORT_FROM( ip225015 )
	MDRV_CPU_REPLACE( "main", R4600BE, 133333333 )
MACHINE_DRIVER_END

MACHINE_DRIVER_START( ip244415 )
	MDRV_IMPORT_FROM( ip225015 )
	MDRV_CPU_REPLACE( "main", R4600BE, 150000000 )
MACHINE_DRIVER_END

ROM_START( ip225015 )
	ROM_REGION( 0x80000, REGION_USER1, 0 )
	ROM_LOAD( "ip225015.bin", 0x000000, 0x080000, CRC(aee5502e) SHA1(9243fef0a3508790651e0d6d2705c887629b1280) )
ROM_END

ROM_START( ip224613 )
	ROM_REGION( 0x80000, REGION_USER1, 0 )
	ROM_LOAD( "ip224613.bin", 0x000000, 0x080000, CRC(f1868b5b) SHA1(0dcbbd776e671785b9b65f3c6dbd609794a40157) )
ROM_END

ROM_START( ip244415 )
	ROM_REGION( 0x80000, REGION_USER1, 0 )
	ROM_LOAD( "ip244415.bin", 0x000000, 0x080000, CRC(2f37825a) SHA1(0d48c573b53a307478820b85aacb57b868297ca3) )
ROM_END

SYSTEM_CONFIG_START( ip225015 )
	CONFIG_DEVICE(ip22_chdcd_getinfo)
	CONFIG_DEVICE(ip22_harddisk_getinfo)
SYSTEM_CONFIG_END

/*     YEAR  NAME      PARENT    COMPAT    MACHINE   INPUT     INIT      CONFIG    COMPANY   FULLNAME */
COMP( 1993, ip225015, 0,        0,        ip225015, ip225015, ip225015, ip225015, "Silicon Graphics, Inc", "Indy (R5000, 150MHz)", GAME_NOT_WORKING )
COMP( 1993, ip224613, 0,        0,        ip224613, ip225015, ip225015, ip225015, "Silicon Graphics, Inc", "Indy (R4600, 133MHz)", GAME_NOT_WORKING )
COMP( 1994, ip244415, 0,        0,        ip244415, ip225015, ip225015, ip225015, "Silicon Graphics, Inc", "Indigo2 (R4400, 150MHz)", GAME_NOT_WORKING )
