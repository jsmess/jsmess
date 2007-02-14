/***************************************************************************

    mips3drc.c
    x86 Dynamic recompiler for MIPS III/IV emulator.
    Written by Aaron Giles

    Philosophy: this is intended to be a very basic implementation of a
    dynamic compiler in order to keep things simple. There are certainly
    more optimizations that could be added but for now, we keep it
    strictly to NOP stripping and LUI optimizations.

***************************************************************************/

#include <stddef.h>
#include "driver.h"
#include "debugger.h"
#include "mips3.h"
#include "cpu/x86drc.h"


//In GCC, ECX/EDX are volatile
//EBX/EBP/ESI/EDI are non-volatile



/***************************************************************************
    CONFIGURATION
***************************************************************************/

#define LOG_CODE			(0)
#define PRINTF_TLB			(0)

#define CACHE_SIZE			(16 * 1024 * 1024)
#define MAX_INSTRUCTIONS	512



/***************************************************************************
    CONSTANTS
***************************************************************************/

/* COP0 registers */
#define COP0_Index			0
#define COP0_Random			1
#define COP0_EntryLo		2
#define COP0_EntryLo0		2
#define COP0_EntryLo1		3
#define COP0_Context		4
#define COP0_PageMask		5
#define COP0_Wired			6
#define COP0_BadVAddr		8
#define COP0_Count			9
#define COP0_EntryHi		10
#define COP0_Compare		11
#define COP0_Status			12
#define COP0_Cause			13
#define COP0_EPC			14
#define COP0_PRId			15
#define COP0_Config			16
#define COP0_XContext		20

/* Status register bits */
#define SR_IE				0x00000001
#define SR_EXL				0x00000002
#define SR_ERL				0x00000004
#define SR_KSU_MASK			0x00000018
#define SR_KSU_KERNEL		0x00000000
#define SR_KSU_SUPERVISOR	0x00000008
#define SR_KSU_USER			0x00000010
#define SR_IMSW0			0x00000100
#define SR_IMSW1			0x00000200
#define SR_IMEX0			0x00000400
#define SR_IMEX1			0x00000800
#define SR_IMEX2			0x00001000
#define SR_IMEX3			0x00002000
#define SR_IMEX4			0x00004000
#define SR_IMEX5			0x00008000
#define SR_DE				0x00010000
#define SR_CE				0x00020000
#define SR_CH				0x00040000
#define SR_SR				0x00100000
#define SR_TS				0x00200000
#define SR_BEV				0x00400000
#define SR_RE				0x02000000
#define SR_FR				0x04000000
#define SR_RP				0x08000000
#define SR_COP0				0x10000000
#define SR_COP1				0x20000000
#define SR_COP2				0x40000000
#define SR_COP3				0x80000000

/* exception types */
#define EXCEPTION_INTERRUPT	0
#define EXCEPTION_TLBMOD	1
#define EXCEPTION_TLBLOAD	2
#define EXCEPTION_TLBSTORE	3
#define EXCEPTION_ADDRLOAD	4
#define EXCEPTION_ADDRSTORE	5
#define EXCEPTION_BUSINST	6
#define EXCEPTION_BUSDATA	7
#define EXCEPTION_SYSCALL	8
#define EXCEPTION_BREAK		9
#define EXCEPTION_INVALIDOP	10
#define EXCEPTION_BADCOP	11
#define EXCEPTION_OVERFLOW	12
#define EXCEPTION_TRAP		13



/***************************************************************************
    HELPER MACROS
***************************************************************************/

#define RSREG		((op >> 21) & 31)
#define RTREG		((op >> 16) & 31)
#define RDREG		((op >> 11) & 31)
#define SHIFT		((op >> 6) & 31)

#define FRREG		((op >> 21) & 31)
#define FTREG		((op >> 16) & 31)
#define FSREG		((op >> 11) & 31)
#define FDREG		((op >> 6) & 31)

#define IS_SINGLE(o) (((o) & (1 << 21)) == 0)
#define IS_DOUBLE(o) (((o) & (1 << 21)) != 0)
#define IS_FLOAT(o) (((o) & (1 << 23)) == 0)
#define IS_INTEGRAL(o) (((o) & (1 << 23)) != 0)

#define SIMMVAL		((INT16)op)
#define UIMMVAL		((UINT16)op)
#define LIMMVAL		(op & 0x03ffffff)

#define IS_FR0		(!(mips3.cpr[0][COP0_Status] & SR_FR))
#define IS_FR1		(mips3.cpr[0][COP0_Status] & SR_FR)



/***************************************************************************
    STRUCTURES & TYPEDEFS
***************************************************************************/

/* memory access function table */
typedef struct
{
	UINT8		(*readbyte)(offs_t);
	UINT16		(*readword)(offs_t);
	UINT32		(*readlong)(offs_t);
	void		(*writebyte)(offs_t, UINT8);
	void		(*writeword)(offs_t, UINT16);
	void		(*writelong)(offs_t, UINT32);
} memory_handlers;


/* code logging info */
struct _code_log_entry
{
	UINT32		pc;
	UINT32		op;
	void *		base;
};
typedef struct _code_log_entry code_log_entry;


/* MIPS3 Registers */
typedef struct
{
	/* core registers */
	UINT32		pc;
	UINT64		hi;
	UINT64		lo;
	UINT64		r[32];

	/* COP registers */
	UINT64		cpr[4][32];
	UINT64		ccr[4][32];
	UINT8		cf[4][8];

	/* internal stuff */
	drc_core *	drc;
	UINT32		drcoptions;
	UINT32		nextpc;
	int 		(*irq_callback)(int irqline);
	UINT64		count_zero_time;
	void *		compare_int_timer;
	UINT8		is_mips4;
	UINT8		flush_pending;
	UINT32		system_clock;
	UINT32		cpu_clock;

	/* memory accesses */
	UINT8		bigendian;
	memory_handlers memory;

	/* cache memory */
	UINT32 *	icache;
	UINT32 *	dcache;
	size_t		icache_size;
	size_t		dcache_size;

	/* MMU */
	struct
	{
		UINT64	page_mask;
		UINT64	entry_hi;
		UINT64	entry_lo[2];
	} tlb[48];
	UINT32 *	tlb_table;

	/* fast RAM */
	UINT32		fastram_select;
	struct
	{
		offs_t	start;
		offs_t	end;
		int		readonly;
		void *	base;
	} fastram[MIPS3_MAX_FASTRAM];

	/* hotspots */
	UINT32		hotspot_select;
	struct
	{
		offs_t		pc;
		UINT32		opcode;
		UINT32		cycles;
	} hotspot[MIPS3_MAX_HOTSPOTS];

	/* callbacks */
	void *		generate_interrupt_exception;
	void *		generate_cop_exception;
	void *		generate_overflow_exception;
	void *		generate_invalidop_exception;
	void *		generate_syscall_exception;
	void *		generate_break_exception;
	void *		generate_trap_exception;
	void *		generate_tlbload_exception;
	void *		generate_tlbstore_exception;
	void *		handle_pc_tlb_mismatch;
	void *		read_and_translate_byte_signed;
	void *		read_and_translate_byte_unsigned;
	void *		read_and_translate_word_signed;
	void *		read_and_translate_word_unsigned;
	void *		read_and_translate_long;
	void *		read_and_translate_double;
	void *		write_and_translate_byte;
	void *		write_and_translate_word;
	void *		write_and_translate_long;
	void *		write_and_translate_double;
	void *		write_back_long;
	void *		write_back_double;
} mips3_regs;



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static void mips3drc_init(void);
static void mips3drc_reset(drc_core *drc);
static void mips3drc_recompile(drc_core *drc);
static void mips3drc_entrygen(drc_core *drc);

static void recompute_tlb_table(void);
static void update_cycle_counting(void);

#ifdef MAME_DEBUG
static offs_t mips3_dasm(char *buffer, offs_t pc, const UINT8 *oprom, const UINT8 *opram);
#endif /* MAME_DEBUG */



/***************************************************************************
    PUBLIC GLOBAL VARIABLES
***************************************************************************/

static int	mips3_icount;



/***************************************************************************
    PRIVATE GLOBAL VARIABLES
***************************************************************************/

static mips3_regs mips3;


static const memory_handlers be_memory =
{
	program_read_byte_32be,  program_read_word_32be,  program_read_dword_32be,
	program_write_byte_32be, program_write_word_32be, program_write_dword_32be
};

static const memory_handlers le_memory =
{
	program_read_byte_32le,  program_read_word_32le,  program_read_dword_32le,
	program_write_byte_32le, program_write_word_32le, program_write_dword_32le
};



/***************************************************************************
    IRQ HANDLING
***************************************************************************/

static void set_irq_line(int irqline, int state)
{
	if (state != CLEAR_LINE)
		mips3.cpr[0][COP0_Cause] |= 0x400 << irqline;
	else
		mips3.cpr[0][COP0_Cause] &= ~(0x400 << irqline);
}



/***************************************************************************
    CONTEXT SWITCHING
***************************************************************************/

static void mips3_get_context(void *dst)
{
	/* copy the context */
	if (dst)
		*(mips3_regs *)dst = mips3;
}


static void mips3_set_context(void *src)
{
	/* copy the context */
	if (src)
		mips3 = *(mips3_regs *)src;
}



/***************************************************************************
    INITIALIZATION AND SHUTDOWN
***************************************************************************/

static void compare_int_callback(int cpu)
{
	cpunum_set_input_line(cpu, 5, ASSERT_LINE);
}


static void mips3_init(int index, int clock, const void *_config, int (*irqcallback)(int))
{
	const struct mips3_config *config = _config;
	mips3drc_init();

	mips3.compare_int_timer = timer_alloc(compare_int_callback);
	mips3.irq_callback = irqcallback;

	/* allocate memory */
	mips3.icache = auto_malloc(config->icache);
	mips3.dcache = auto_malloc(config->dcache);
	mips3.tlb_table = auto_malloc(sizeof(mips3.tlb_table[0]) * (1 << (32 - 12)));

	/* initialize the rest of the config */
	mips3.icache_size = config->icache;
	mips3.dcache_size = config->dcache;
	mips3.system_clock = config->system_clock;
	mips3.cpu_clock = clock;
}


static void mips3_reset(int bigendian, int mips4, UINT32 prid)
{
	UINT32 configreg;
	int divisor;

	/* set up the endianness */
	mips3.bigendian = bigendian;
	if (mips3.bigendian)
		mips3.memory = be_memory;
	else
		mips3.memory = le_memory;

	/* initialize the state */
	mips3.pc = 0xbfc00000;
	mips3.nextpc = ~0;
	mips3.cpr[0][COP0_Status] = SR_BEV | SR_ERL;
	mips3.cpr[0][COP0_Compare] = 0xffffffff;
	mips3.cpr[0][COP0_Count] = 0;
	mips3.count_zero_time = activecpu_gettotalcycles64();

	/* reset the DRC */
	drc_cache_reset(mips3.drc);

	/* config register: set the cache line size to 32 bytes */
	configreg = 0x00026030;

	/* config register: set the data cache size */
	     if (mips3.icache_size <= 0x01000) configreg |= 0 << 6;
	else if (mips3.icache_size <= 0x02000) configreg |= 1 << 6;
	else if (mips3.icache_size <= 0x04000) configreg |= 2 << 6;
	else if (mips3.icache_size <= 0x08000) configreg |= 3 << 6;
	else if (mips3.icache_size <= 0x10000) configreg |= 4 << 6;
	else if (mips3.icache_size <= 0x20000) configreg |= 5 << 6;
	else if (mips3.icache_size <= 0x40000) configreg |= 6 << 6;
	else                                   configreg |= 7 << 6;

	/* config register: set the instruction cache size */
	     if (mips3.icache_size <= 0x01000) configreg |= 0 << 9;
	else if (mips3.icache_size <= 0x02000) configreg |= 1 << 9;
	else if (mips3.icache_size <= 0x04000) configreg |= 2 << 9;
	else if (mips3.icache_size <= 0x08000) configreg |= 3 << 9;
	else if (mips3.icache_size <= 0x10000) configreg |= 4 << 9;
	else if (mips3.icache_size <= 0x20000) configreg |= 5 << 9;
	else if (mips3.icache_size <= 0x40000) configreg |= 6 << 9;
	else                                   configreg |= 7 << 9;

	/* config register: set the endianness bit */
	if (bigendian) configreg |= 0x00008000;

	/* config register: set the system clock divider */
	divisor = 2;
	if (mips3.system_clock != 0)
	{
		divisor = mips3.cpu_clock / mips3.system_clock;
		if (mips3.system_clock * divisor != mips3.cpu_clock)
		{
			configreg |= 0x80000000;
			divisor = mips3.cpu_clock * 2 / mips3.system_clock;
		}
	}
	configreg |= (((divisor < 2) ? 2 : (divisor > 8) ? 8 : divisor) - 2) << 28;

	/* set up the architecture */
	mips3.is_mips4 = mips4;
	mips3.cpr[0][COP0_PRId] = prid;
	mips3.cpr[0][COP0_Config] = configreg;

	/* recompute the TLB table */
	recompute_tlb_table();
}


/*

    R6000A = 0x0600
    R10000 = 0x0900
    R4300  = 0x0b00
    VR41XX = 0x0c00
    R12000 = 0x0e00
    R8000  = 0x1000
    R4600  = 0x2000
    R4650  = 0x2000
    R5000  = 0x2300
    R5432  = 0x5400
    RM7000 = 0x2700
*/

#if (HAS_R4600)
static void r4600be_reset(void)
{
	mips3_reset(1, 0, 0x2000);
}

static void r4600le_reset(void)
{
	mips3_reset(0, 0, 0x2000);
}
#endif


#if (HAS_R4700)
static void r4700be_reset(void)
{
	mips3_reset(1, 0, 0x2100);
}

static void r4700le_reset(void)
{
	mips3_reset(0, 0, 0x2100);
}
#endif


#if (HAS_R5000)
static void r5000be_reset(void)
{
	mips3_reset(1, 1, 0x2300);
}

static void r5000le_reset(void)
{
	mips3_reset(0, 1, 0x2300);
}
#endif


#if (HAS_QED5271)
static void qed5271be_reset(void)
{
	mips3_reset(1, 1, 0x2300);
}

static void qed5271le_reset(void)
{
	mips3_reset(1, 0, 0x2300);
}
#endif


#if (HAS_RM7000)
static void rm7000be_reset(void)
{
	mips3_reset(1, 1, 0x2700);
}

static void rm7000le_reset(void)
{
	mips3_reset(0, 1, 0x2700);
}
#endif


static void mips3_exit(void)
{
	drc_exit(mips3.drc);
}


static int mips3_execute(int cycles)
{
	/* update the cycle timing */
	update_cycle_counting();

	/* count cycles and interrupt cycles */
	mips3_icount = cycles;
	drc_execute(mips3.drc);
	return cycles - mips3_icount;
}



/***************************************************************************
    TLB HANDLING
***************************************************************************/

static void map_tlb_entries(void)
{
	int valid_asid = mips3.cpr[0][COP0_EntryHi] & 0xff;
	int index;

	/* iterate over all TLB entries */
	for (index = 0; index < 48; index++)
	{
		UINT64 hi = mips3.tlb[index].entry_hi;

		/* only process if the ASID matches or if BOTH entries are marked global */
		if (valid_asid == (hi & 0xff) || ((mips3.tlb[index].entry_lo[0] & 1) && (mips3.tlb[index].entry_lo[1] & 1)))
		{
			UINT32 count = (mips3.tlb[index].page_mask >> 13) & 0x00fff;
			UINT32 vpn = ((hi >> 13) & 0x07ffffff) << 1;
			int which, i;

			/* ignore if the virtual address is beyond 32 bits */
			if (vpn < (1 << (32 - 12)))

				/* loop over both the even and odd pages */
				for (which = 0; which < 2; which++)
				{
					UINT64 lo = mips3.tlb[index].entry_lo[which];

					/* only map if the TLB entry is valid */
					if (lo & 2)
					{
						UINT32 pfn = (lo >> 6) & 0x00ffffff;
						UINT32 wp = (~lo >> 2) & 1;

						for (i = 0; i <= count; i++, vpn++, pfn++)
							if (vpn < 0x80000 || vpn >= 0xc0000)
								mips3.tlb_table[vpn] = (pfn << 12) | wp;
					}

					/* otherwise, advance by the number of pages we would have mapped */
					else
						vpn += count + 1;
				}
		}
	}
}


static void unmap_tlb_entries(void)
{
	int index;

	/* iterate over all TLB entries */
	for (index = 0; index < 48; index++)
	{
		UINT64 hi = mips3.tlb[index].entry_hi;
		UINT32 count = (mips3.tlb[index].page_mask >> 13) & 0x00fff;
		UINT32 vpn = ((hi >> 13) & 0x07ffffff) << 1;
		int which, i;

		/* ignore if the virtual address is beyond 32 bits */
		if (vpn < (1 << (32 - 12)))

			/* loop over both the even and odd pages */
			for (which = 0; which < 2; which++)
				for (i = 0; i <= count; i++, vpn++)
					if (vpn < 0x80000 || vpn >= 0xc0000)
						mips3.tlb_table[vpn] = 0xffffffff;
	}
}


static void recompute_tlb_table(void)
{
	UINT32 addr;

	/* map in the hard-coded spaces */
	for (addr = 0x80000000; addr < 0xc0000000; addr += 1 << 12)
		mips3.tlb_table[addr >> 12] = addr & 0x1ffff000;

	/* reset everything else to unmapped */
	memset(&mips3.tlb_table[0x00000000 >> 12], 0xff, sizeof(mips3.tlb_table[0]) * (0x80000000 >> 12));
	memset(&mips3.tlb_table[0xc0000000 >> 12], 0xff, sizeof(mips3.tlb_table[0]) * (0x40000000 >> 12));

	/* remap all the entries in the TLB */
	map_tlb_entries();
}


static int translate_address(int space, offs_t *address)
{
	if (space == ADDRESS_SPACE_PROGRAM)
	{
		UINT32 result = mips3.tlb_table[*address >> 12];
		if (result == 0xffffffff)
			return 0;
		*address = (result & ~0xfff) | (*address & 0xfff);
	}
	return 1;
}


INLINE void logonetlbentry(int index, int which)
{
#if PRINTF_TLB
	UINT64 hi = mips3.cpr[0][COP0_EntryHi];
	UINT64 lo = mips3.cpr[0][COP0_EntryLo0 + which];
	UINT32 vpn = (((hi >> 13) & 0x07ffffff) << 1);
	UINT32 asid = hi & 0xff;
	UINT32 r = (hi >> 62) & 3;
	UINT32 pfn = (lo >> 6) & 0x00ffffff;
	UINT32 c = (lo >> 3) & 7;
	UINT32 pagesize = (((mips3.cpr[0][COP0_PageMask] >> 13) & 0xfff) + 1) << 12;
	UINT64 vaddr = (UINT64)vpn * 0x1000;
	UINT64 paddr = (UINT64)pfn * 0x1000;

	vaddr += pagesize * which;

	mame_printf_debug("index=%08X  pagesize=%08X  vaddr=%08X%08X  paddr=%08X%08X  asid=%02X  r=%X  c=%X  dvg=%c%c%c\n",
			index, pagesize, (UINT32)(vaddr >> 32), (UINT32)vaddr, (UINT32)(paddr >> 32), (UINT32)paddr,
			asid, r, c, (lo & 4) ? 'd' : '.', (lo & 2) ? 'v' : '.', (lo & 1) ? 'g' : '.');
#endif
}


static void logtlbentry(int index)
{
	logonetlbentry(index, 0);
	logonetlbentry(index, 1);
}


static void tlbr(void)
{
	UINT32 index = mips3.cpr[0][COP0_Index] & 0x3f;
	if (index < 48)
	{
		mips3.cpr[0][COP0_PageMask] = mips3.tlb[index].page_mask;
		mips3.cpr[0][COP0_EntryHi] = mips3.tlb[index].entry_hi;
		mips3.cpr[0][COP0_EntryLo0] = mips3.tlb[index].entry_lo[0];
		mips3.cpr[0][COP0_EntryLo1] = mips3.tlb[index].entry_lo[1];
	}
}


static void tlbwi(void)
{
	UINT32 index = mips3.cpr[0][COP0_Index] & 0x3f;
	logtlbentry(index);
	if (index < 48)
	{
		unmap_tlb_entries();
		mips3.tlb[index].page_mask = mips3.cpr[0][COP0_PageMask];
		mips3.tlb[index].entry_hi = mips3.cpr[0][COP0_EntryHi] & ~(mips3.tlb[index].page_mask & U64(0x0000000001ffe000));
		mips3.tlb[index].entry_lo[0] = mips3.cpr[0][COP0_EntryLo0];
		mips3.tlb[index].entry_lo[1] = mips3.cpr[0][COP0_EntryLo1];
		map_tlb_entries();
	}
}


static void tlbwr(void)
{
	UINT32 wired = mips3.cpr[0][COP0_Wired] & 0x3f;
	UINT32 index = 48 - wired;
	if (index > 0)
		index = ((activecpu_gettotalcycles64() - mips3.count_zero_time) % index + wired) & 0x3f;
	else
		index = 47;
	logtlbentry(index);
	{
		unmap_tlb_entries();
		mips3.tlb[index].page_mask = mips3.cpr[0][COP0_PageMask];
		mips3.tlb[index].entry_hi = mips3.cpr[0][COP0_EntryHi] & ~(mips3.tlb[index].page_mask & U64(0x0000000001ffe000));
		mips3.tlb[index].entry_lo[0] = mips3.cpr[0][COP0_EntryLo0];
		mips3.tlb[index].entry_lo[1] = mips3.cpr[0][COP0_EntryLo1];
		map_tlb_entries();
	}
}


static void tlbp(void)
{
	UINT32 index;
	UINT64 vpn;

//  DEBUGGER_BREAK;
	for (index = 0; index < 48; index++)
	{
		UINT64 mask = ~(mips3.tlb[index].page_mask & U64(0x0000000001ffe000)) & ~U64(0x1fff);
#if PRINTF_TLB
mame_printf_debug("Mask = %08X%08X  TLB = %08X%08X  MATCH = %08X%08X\n",
	(UINT32)(mask >> 32), (UINT32)mask,
	(UINT32)(mips3.tlb[index].entry_hi >> 32), (UINT32)mips3.tlb[index].entry_hi,
	(UINT32)(mips3.cpr[0][COP0_EntryHi] >> 32), (UINT32)mips3.cpr[0][COP0_EntryHi]);
#endif
		if ((mips3.tlb[index].entry_hi & mask) == (mips3.cpr[0][COP0_EntryHi] & mask))
		{
			if (((mips3.tlb[index].entry_lo[0] & 1) && (mips3.tlb[index].entry_lo[1] & 1)) ||
				(mips3.tlb[index].entry_hi & 0xff) == (mips3.cpr[0][COP0_EntryHi] & 0xff))
				break;
		}
	}
	vpn = ((mips3.cpr[0][COP0_EntryHi] >> 13) & 0x07ffffff) << 1;
	if (index != 48)
	{
		if (mips3.tlb_table[vpn & 0xfffff] == 0xffffffff)
		{
#if PRINTF_TLB
			mame_printf_debug("TLBP: Should have not found an entry\n");
//          DEBUGGER_BREAK;
#endif
		}
		mips3.cpr[0][COP0_Index] = index;
	}
	else
	{
		if (mips3.tlb_table[vpn & 0xfffff] != 0xffffffff)
		{
#if PRINTF_TLB
			mame_printf_debug("TLBP: Should havefound an entry\n");
//          DEBUGGER_BREAK;
#endif
		}
		mips3.cpr[0][COP0_Index] = 0x80000000;
	}
}



/***************************************************************************
    RECOMPILER CALLBACKS
***************************************************************************/

/*------------------------------------------------------------------
    update_cycle_counting
------------------------------------------------------------------*/

static void update_cycle_counting(void)
{
	/* modify the timer to go off */
	if ((mips3.cpr[0][COP0_Status] & 0x8000) && mips3.cpr[0][COP0_Compare] != 0xffffffff)
	{
		UINT32 count = (activecpu_gettotalcycles64() - mips3.count_zero_time) / 2;
		UINT32 compare = mips3.cpr[0][COP0_Compare];
		UINT32 cyclesleft = compare - count;
		double newtime = TIME_IN_CYCLES(((INT64)cyclesleft * 2), cpu_getactivecpu());

		/* due to accuracy issues, don't bother setting timers unless they're for less than 100msec */
		if (newtime < TIME_IN_MSEC(100))
			timer_adjust(mips3.compare_int_timer, newtime, cpu_getactivecpu(), 0);
	}
	else
		timer_adjust(mips3.compare_int_timer, TIME_NEVER, cpu_getactivecpu(), 0);
}




/***************************************************************************
    CODE LOGGING
***************************************************************************/

#if LOG_CODE

static code_log_entry code_log_buffer[MAX_INSTRUCTIONS*2];
static int code_log_index;

INLINE void code_log_reset(void)
{
	code_log_index = 0;
}

INLINE void code_log_add_entry(UINT32 pc, UINT32 op, void *base)
{
	code_log_buffer[code_log_index].pc = pc;
	code_log_buffer[code_log_index].op = op;
	code_log_buffer[code_log_index].base = base;
	code_log_index++;
}

static void code_log(drc_core *drc, const char *label, void *start)
{
	extern int i386_dasm_one(char *buffer, UINT32 eip, UINT8 *oprom, int addr_size, int op_size);
	extern unsigned dasmmips3(char *buffer, unsigned pc, UINT32 op);
	UINT8 *cur = start;
	static FILE *f;

	/* open the file, creating it if necessary */
	if (!f)
		f = fopen("mips3drc.asm", "w");
	if (!f)
		return;
	fprintf(f, "\n%s\n", label);

	/* loop from the start until the cache top */
	while (cur < drc->cache_top)
	{
		char buffer[100];
		int bytes;
		int op;

		/* disassemble this instruction */
		bytes = i386_dasm_one(buffer, (UINT32)cur, cur, 1, 1) & DASMFLAG_LENGTHMASK;

		/* look for a match in the registered opcodes */
		for (op = 0; op < code_log_index; op++)
			if (code_log_buffer[op].base == (void *)cur)
				break;

		/* if no match, just output the current instruction */
		if (op == code_log_index)
			fprintf(f, "%08X: %s\n", (UINT32)cur, buffer);

		/* otherwise, output with the original instruction to the right */
		else
		{
			char buffer2[100];
			dasmmips3(buffer2, code_log_buffer[op].pc, code_log_buffer[op].op);
			fprintf(f, "%08X: %-50s %08X: %s\n", (UINT32)cur, buffer, code_log_buffer[op].pc, buffer2);
		}

		/* advance past this instruction */
		cur += bytes;
	}

	/* flush the file */
	fflush(f);
}

#else

#define code_log_reset()
#define code_log_add_entry(a,b,c)
#define code_log(a,b,c)

#endif



/***************************************************************************
    RECOMPILER CORE
***************************************************************************/


#include "mdrcold.c"
//#include "mdrc32.c"



/***************************************************************************
    DISASSEMBLY HOOK
***************************************************************************/

#ifdef MAME_DEBUG
static offs_t mips3_dasm(char *buffer, offs_t pc, const UINT8 *oprom, const UINT8 *opram)
{
	extern unsigned dasmmips3(char *, unsigned, UINT32);
	UINT32 op = *(UINT32 *)oprom;
	if (mips3.bigendian)
		op = BIG_ENDIANIZE_INT32(op);
	else
		op = LITTLE_ENDIANIZE_INT32(op);
	return dasmmips3(buffer, pc, op);
}
#endif /* MAME_DEBUG */


/**************************************************************************
 * Generic set_info
 **************************************************************************/

static void mips3_set_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are set as 64-bit signed integers --- */
		case CPUINFO_INT_INPUT_STATE + MIPS3_IRQ0:		set_irq_line(MIPS3_IRQ0, info->i);		break;
		case CPUINFO_INT_INPUT_STATE + MIPS3_IRQ1:		set_irq_line(MIPS3_IRQ1, info->i);		break;
		case CPUINFO_INT_INPUT_STATE + MIPS3_IRQ2:		set_irq_line(MIPS3_IRQ2, info->i);		break;
		case CPUINFO_INT_INPUT_STATE + MIPS3_IRQ3:		set_irq_line(MIPS3_IRQ3, info->i);		break;
		case CPUINFO_INT_INPUT_STATE + MIPS3_IRQ4:		set_irq_line(MIPS3_IRQ4, info->i);		break;
		case CPUINFO_INT_INPUT_STATE + MIPS3_IRQ5:		set_irq_line(MIPS3_IRQ5, info->i);		break;

		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + MIPS3_PC:			mips3.pc = info->i;						break;
		case CPUINFO_INT_REGISTER + MIPS3_SR:			mips3.cpr[0][COP0_Status] = info->i; 	break;
		case CPUINFO_INT_REGISTER + MIPS3_EPC:			mips3.cpr[0][COP0_EPC] = info->i; 		break;
		case CPUINFO_INT_REGISTER + MIPS3_CAUSE:		mips3.cpr[0][COP0_Cause] = info->i;		break;
		case CPUINFO_INT_REGISTER + MIPS3_COUNT:		mips3.cpr[0][COP0_Count] = info->i; 	break;
		case CPUINFO_INT_REGISTER + MIPS3_COMPARE:		mips3.cpr[0][COP0_Compare] = info->i; 	break;
		case CPUINFO_INT_REGISTER + MIPS3_INDEX:		mips3.cpr[0][COP0_Index] = info->i; 	break;
		case CPUINFO_INT_REGISTER + MIPS3_RANDOM:		mips3.cpr[0][COP0_Random] = info->i; 	break;
		case CPUINFO_INT_REGISTER + MIPS3_ENTRYHI:		mips3.cpr[0][COP0_EntryHi] = info->i; 	break;
		case CPUINFO_INT_REGISTER + MIPS3_ENTRYLO0:		mips3.cpr[0][COP0_EntryLo0] = info->i; 	break;
		case CPUINFO_INT_REGISTER + MIPS3_ENTRYLO1:		mips3.cpr[0][COP0_EntryLo1] = info->i; 	break;
		case CPUINFO_INT_REGISTER + MIPS3_PAGEMASK:		mips3.cpr[0][COP0_PageMask] = info->i; 	break;
		case CPUINFO_INT_REGISTER + MIPS3_WIRED:		mips3.cpr[0][COP0_Wired] = info->i; 	break;
		case CPUINFO_INT_REGISTER + MIPS3_BADVADDR:		mips3.cpr[0][COP0_BadVAddr] = info->i; 	break;

		case CPUINFO_INT_REGISTER + MIPS3_R0:			mips3.r[0] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R1:			mips3.r[1] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R2:			mips3.r[2] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R3:			mips3.r[3] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R4:			mips3.r[4] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R5:			mips3.r[5] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R6:			mips3.r[6] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R7:			mips3.r[7] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R8:			mips3.r[8] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R9:			mips3.r[9] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R10:			mips3.r[10] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R11:			mips3.r[11] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R12:			mips3.r[12] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R13:			mips3.r[13] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R14:			mips3.r[14] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R15:			mips3.r[15] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R16:			mips3.r[16] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R17:			mips3.r[17] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R18:			mips3.r[18] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R19:			mips3.r[19] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R20:			mips3.r[20] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R21:			mips3.r[21] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R22:			mips3.r[22] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R23:			mips3.r[23] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R24:			mips3.r[24] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R25:			mips3.r[25] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R26:			mips3.r[26] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R27:			mips3.r[27] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R28:			mips3.r[28] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R29:			mips3.r[29] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R30:			mips3.r[30] = info->i;					break;
		case CPUINFO_INT_SP:
		case CPUINFO_INT_REGISTER + MIPS3_R31:			mips3.r[31] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_HI:			mips3.hi = info->i;						break;
		case CPUINFO_INT_REGISTER + MIPS3_LO:			mips3.lo = info->i;						break;

		case CPUINFO_INT_MIPS3_DRC_OPTIONS:				mips3.drcoptions = info->i;				break;

		case CPUINFO_INT_MIPS3_FASTRAM_SELECT:			if (info->i >= 0 && info->i < MIPS3_MAX_FASTRAM) mips3.fastram_select = info->i; break;
		case CPUINFO_INT_MIPS3_FASTRAM_START:			mips3.fastram[mips3.fastram_select].start = info->i; break;
		case CPUINFO_INT_MIPS3_FASTRAM_END:				mips3.fastram[mips3.fastram_select].end = info->i; break;
		case CPUINFO_INT_MIPS3_FASTRAM_READONLY:		mips3.fastram[mips3.fastram_select].readonly = info->i; break;

		case CPUINFO_INT_MIPS3_HOTSPOT_SELECT:			if (info->i >= 0 && info->i < MIPS3_MAX_HOTSPOTS) mips3.hotspot_select = info->i; break;
		case CPUINFO_INT_MIPS3_HOTSPOT_PC:				mips3.hotspot[mips3.hotspot_select].pc = info->i; break;
		case CPUINFO_INT_MIPS3_HOTSPOT_OPCODE:			mips3.hotspot[mips3.hotspot_select].opcode = info->i; break;
		case CPUINFO_INT_MIPS3_HOTSPOT_CYCLES:			mips3.hotspot[mips3.hotspot_select].cycles = info->i; break;

		/* --- the following bits of info are set as pointers to data or functions --- */
		case CPUINFO_PTR_MIPS3_FASTRAM_BASE:			mips3.fastram[mips3.fastram_select].base = info->p;	break;
	}
}



/**************************************************************************
 * Generic get_info
 **************************************************************************/

void mips3_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof(mips3);				break;
		case CPUINFO_INT_INPUT_LINES:					info->i = 6;							break;
		case CPUINFO_INT_DEFAULT_IRQ_VECTOR:			info->i = 0;							break;
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;
		case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 1;							break;
		case CPUINFO_INT_MIN_INSTRUCTION_BYTES:			info->i = 4;							break;
		case CPUINFO_INT_MAX_INSTRUCTION_BYTES:			info->i = 4;							break;
		case CPUINFO_INT_MIN_CYCLES:					info->i = 1;							break;
		case CPUINFO_INT_MAX_CYCLES:					info->i = 40;							break;

		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 32;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 32;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM: info->i = 0;					break;
		case CPUINFO_INT_LOGADDR_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 32;					break;
		case CPUINFO_INT_PAGE_SHIFT + ADDRESS_SPACE_PROGRAM: 	info->i = 12;					break;

		case CPUINFO_INT_INPUT_STATE + MIPS3_IRQ0:		info->i = (mips3.cpr[0][COP0_Cause] & 0x400) ? ASSERT_LINE : CLEAR_LINE;	break;
		case CPUINFO_INT_INPUT_STATE + MIPS3_IRQ1:		info->i = (mips3.cpr[0][COP0_Cause] & 0x800) ? ASSERT_LINE : CLEAR_LINE;	break;
		case CPUINFO_INT_INPUT_STATE + MIPS3_IRQ2:		info->i = (mips3.cpr[0][COP0_Cause] & 0x1000) ? ASSERT_LINE : CLEAR_LINE;	break;
		case CPUINFO_INT_INPUT_STATE + MIPS3_IRQ3:		info->i = (mips3.cpr[0][COP0_Cause] & 0x2000) ? ASSERT_LINE : CLEAR_LINE;	break;
		case CPUINFO_INT_INPUT_STATE + MIPS3_IRQ4:		info->i = (mips3.cpr[0][COP0_Cause] & 0x4000) ? ASSERT_LINE : CLEAR_LINE;	break;
		case CPUINFO_INT_INPUT_STATE + MIPS3_IRQ5:		info->i = (mips3.cpr[0][COP0_Cause] & 0x8000) ? ASSERT_LINE : CLEAR_LINE;	break;

		case CPUINFO_INT_PREVIOUSPC:					/* not implemented */					break;

		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + MIPS3_PC:			info->i = mips3.pc;						break;
		case CPUINFO_INT_REGISTER + MIPS3_SR:			info->i = mips3.cpr[0][COP0_Status];	break;
		case CPUINFO_INT_REGISTER + MIPS3_EPC:			info->i = mips3.cpr[0][COP0_EPC];		break;
		case CPUINFO_INT_REGISTER + MIPS3_CAUSE:		info->i = mips3.cpr[0][COP0_Cause];		break;
		case CPUINFO_INT_REGISTER + MIPS3_COUNT:		info->i = ((activecpu_gettotalcycles64() - mips3.count_zero_time) / 2); break;
		case CPUINFO_INT_REGISTER + MIPS3_COMPARE:		info->i = mips3.cpr[0][COP0_Compare];	break;
		case CPUINFO_INT_REGISTER + MIPS3_INDEX:		info->i = mips3.cpr[0][COP0_Index];		break;
		case CPUINFO_INT_REGISTER + MIPS3_RANDOM:		info->i = mips3.cpr[0][COP0_Random];	break;
		case CPUINFO_INT_REGISTER + MIPS3_ENTRYHI:		info->i = mips3.cpr[0][COP0_EntryHi];	break;
		case CPUINFO_INT_REGISTER + MIPS3_ENTRYLO0:		info->i = mips3.cpr[0][COP0_EntryLo0];	break;
		case CPUINFO_INT_REGISTER + MIPS3_ENTRYLO1:		info->i = mips3.cpr[0][COP0_EntryLo1];	break;
		case CPUINFO_INT_REGISTER + MIPS3_PAGEMASK:		info->i = mips3.cpr[0][COP0_PageMask];	break;
		case CPUINFO_INT_REGISTER + MIPS3_WIRED:		info->i = mips3.cpr[0][COP0_Wired];		break;
		case CPUINFO_INT_REGISTER + MIPS3_BADVADDR:		info->i = mips3.cpr[0][COP0_BadVAddr];	break;

		case CPUINFO_INT_REGISTER + MIPS3_R0:			info->i = mips3.r[0];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R1:			info->i = mips3.r[1];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R2:			info->i = mips3.r[2];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R3:			info->i = mips3.r[3];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R4:			info->i = mips3.r[4];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R5:			info->i = mips3.r[5];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R6:			info->i = mips3.r[6];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R7:			info->i = mips3.r[7];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R8:			info->i = mips3.r[8];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R9:			info->i = mips3.r[9];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R10:			info->i = mips3.r[10];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R11:			info->i = mips3.r[11];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R12:			info->i = mips3.r[12];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R13:			info->i = mips3.r[13];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R14:			info->i = mips3.r[14];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R15:			info->i = mips3.r[15];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R16:			info->i = mips3.r[16];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R17:			info->i = mips3.r[17];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R18:			info->i = mips3.r[18];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R19:			info->i = mips3.r[19];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R20:			info->i = mips3.r[20];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R21:			info->i = mips3.r[21];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R22:			info->i = mips3.r[22];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R23:			info->i = mips3.r[23];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R24:			info->i = mips3.r[24];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R25:			info->i = mips3.r[25];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R26:			info->i = mips3.r[26];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R27:			info->i = mips3.r[27];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R28:			info->i = mips3.r[28];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R29:			info->i = mips3.r[29];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R30:			info->i = mips3.r[30];					break;
		case CPUINFO_INT_SP:
		case CPUINFO_INT_REGISTER + MIPS3_R31:			info->i = mips3.r[31];					break;
		case CPUINFO_INT_REGISTER + MIPS3_HI:			info->i = mips3.hi;						break;
		case CPUINFO_INT_REGISTER + MIPS3_LO:			info->i = mips3.lo;						break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_SET_INFO:						info->setinfo = mips3_set_info;			break;
		case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = mips3_get_context;	break;
		case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = mips3_set_context;	break;
		case CPUINFO_PTR_INIT:							info->init = mips3_init;				break;
		case CPUINFO_PTR_RESET:							/* provided per-CPU */					break;
		case CPUINFO_PTR_EXIT:							info->exit = mips3_exit;				break;
		case CPUINFO_PTR_EXECUTE:						info->execute = mips3_execute;			break;
		case CPUINFO_PTR_BURN:							info->burn = NULL;						break;
#ifdef MAME_DEBUG
		case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = mips3_dasm;			break;
#endif /* MAME_DEBUG */
		case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &mips3_icount;			break;
		case CPUINFO_PTR_TRANSLATE:						info->translate = translate_address;	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "MIPS III");			break;
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s, "MIPS III");			break;
		case CPUINFO_STR_CORE_VERSION:					strcpy(info->s, "1.0");					break;
		case CPUINFO_STR_CORE_FILE:						strcpy(info->s, __FILE__);				break;
		case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s, "Copyright (C) Aaron Giles 2000-2004"); break;

		case CPUINFO_STR_FLAGS:							strcpy(info->s, " ");					break;

		case CPUINFO_STR_REGISTER + MIPS3_PC:			sprintf(info->s, "PC: %08X", mips3.pc); break;
		case CPUINFO_STR_REGISTER + MIPS3_SR:			sprintf(info->s, "SR: %08X", (UINT32)mips3.cpr[0][COP0_Status]); break;
		case CPUINFO_STR_REGISTER + MIPS3_EPC:			sprintf(info->s, "EPC:%08X", (UINT32)mips3.cpr[0][COP0_EPC]); break;
		case CPUINFO_STR_REGISTER + MIPS3_CAUSE: 		sprintf(info->s, "Cause:%08X", (UINT32)mips3.cpr[0][COP0_Cause]); break;
		case CPUINFO_STR_REGISTER + MIPS3_COUNT: 		sprintf(info->s, "Count:%08X", (UINT32)((activecpu_gettotalcycles64() - mips3.count_zero_time) / 2)); break;
		case CPUINFO_STR_REGISTER + MIPS3_COMPARE:		sprintf(info->s, "Compare:%08X", (UINT32)mips3.cpr[0][COP0_Compare]); break;
		case CPUINFO_STR_REGISTER + MIPS3_INDEX:		sprintf(info->s, "Index:%08X", (UINT32)mips3.cpr[0][COP0_Index]); break;
		case CPUINFO_STR_REGISTER + MIPS3_RANDOM:		sprintf(info->s, "Random:%08X", (UINT32)mips3.cpr[0][COP0_Random]); break;
		case CPUINFO_STR_REGISTER + MIPS3_ENTRYHI:		sprintf(info->s, "EntryHi:%08X%08X", (UINT32)(mips3.cpr[0][COP0_EntryHi] >> 32), (UINT32)mips3.cpr[0][COP0_EntryHi]); break;
		case CPUINFO_STR_REGISTER + MIPS3_ENTRYLO0:		sprintf(info->s, "EntryLo0:%08X%08X", (UINT32)(mips3.cpr[0][COP0_EntryLo0] >> 32), (UINT32)mips3.cpr[0][COP0_EntryLo0]); break;
		case CPUINFO_STR_REGISTER + MIPS3_ENTRYLO1:		sprintf(info->s, "EntryLo1:%08X%08X", (UINT32)(mips3.cpr[0][COP0_EntryLo1] >> 32), (UINT32)mips3.cpr[0][COP0_EntryLo1]); break;
		case CPUINFO_STR_REGISTER + MIPS3_PAGEMASK:		sprintf(info->s, "PageMask:%08X%08X", (UINT32)(mips3.cpr[0][COP0_PageMask] >> 32), (UINT32)mips3.cpr[0][COP0_PageMask]); break;
		case CPUINFO_STR_REGISTER + MIPS3_WIRED:		sprintf(info->s, "Wired:%08X", (UINT32)mips3.cpr[0][COP0_Wired]); break;
		case CPUINFO_STR_REGISTER + MIPS3_BADVADDR:		sprintf(info->s, "BadVAddr:%08X", (UINT32)mips3.cpr[0][COP0_BadVAddr]); break;

		case CPUINFO_STR_REGISTER + MIPS3_R0:			sprintf(info->s, "R0: %08X%08X", (UINT32)(mips3.r[0] >> 32), (UINT32)mips3.r[0]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R1:			sprintf(info->s, "R1: %08X%08X", (UINT32)(mips3.r[1] >> 32), (UINT32)mips3.r[1]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R2:			sprintf(info->s, "R2: %08X%08X", (UINT32)(mips3.r[2] >> 32), (UINT32)mips3.r[2]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R3:			sprintf(info->s, "R3: %08X%08X", (UINT32)(mips3.r[3] >> 32), (UINT32)mips3.r[3]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R4:			sprintf(info->s, "R4: %08X%08X", (UINT32)(mips3.r[4] >> 32), (UINT32)mips3.r[4]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R5:			sprintf(info->s, "R5: %08X%08X", (UINT32)(mips3.r[5] >> 32), (UINT32)mips3.r[5]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R6:			sprintf(info->s, "R6: %08X%08X", (UINT32)(mips3.r[6] >> 32), (UINT32)mips3.r[6]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R7:			sprintf(info->s, "R7: %08X%08X", (UINT32)(mips3.r[7] >> 32), (UINT32)mips3.r[7]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R8:			sprintf(info->s, "R8: %08X%08X", (UINT32)(mips3.r[8] >> 32), (UINT32)mips3.r[8]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R9:			sprintf(info->s, "R9: %08X%08X", (UINT32)(mips3.r[9] >> 32), (UINT32)mips3.r[9]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R10:			sprintf(info->s, "R10:%08X%08X", (UINT32)(mips3.r[10] >> 32), (UINT32)mips3.r[10]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R11:			sprintf(info->s, "R11:%08X%08X", (UINT32)(mips3.r[11] >> 32), (UINT32)mips3.r[11]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R12:			sprintf(info->s, "R12:%08X%08X", (UINT32)(mips3.r[12] >> 32), (UINT32)mips3.r[12]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R13:			sprintf(info->s, "R13:%08X%08X", (UINT32)(mips3.r[13] >> 32), (UINT32)mips3.r[13]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R14:			sprintf(info->s, "R14:%08X%08X", (UINT32)(mips3.r[14] >> 32), (UINT32)mips3.r[14]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R15:			sprintf(info->s, "R15:%08X%08X", (UINT32)(mips3.r[15] >> 32), (UINT32)mips3.r[15]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R16:			sprintf(info->s, "R16:%08X%08X", (UINT32)(mips3.r[16] >> 32), (UINT32)mips3.r[16]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R17:			sprintf(info->s, "R17:%08X%08X", (UINT32)(mips3.r[17] >> 32), (UINT32)mips3.r[17]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R18:			sprintf(info->s, "R18:%08X%08X", (UINT32)(mips3.r[18] >> 32), (UINT32)mips3.r[18]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R19:			sprintf(info->s, "R19:%08X%08X", (UINT32)(mips3.r[19] >> 32), (UINT32)mips3.r[19]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R20:			sprintf(info->s, "R20:%08X%08X", (UINT32)(mips3.r[20] >> 32), (UINT32)mips3.r[20]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R21:			sprintf(info->s, "R21:%08X%08X", (UINT32)(mips3.r[21] >> 32), (UINT32)mips3.r[21]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R22:			sprintf(info->s, "R22:%08X%08X", (UINT32)(mips3.r[22] >> 32), (UINT32)mips3.r[22]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R23:			sprintf(info->s, "R23:%08X%08X", (UINT32)(mips3.r[23] >> 32), (UINT32)mips3.r[23]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R24:			sprintf(info->s, "R24:%08X%08X", (UINT32)(mips3.r[24] >> 32), (UINT32)mips3.r[24]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R25:			sprintf(info->s, "R25:%08X%08X", (UINT32)(mips3.r[25] >> 32), (UINT32)mips3.r[25]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R26:			sprintf(info->s, "R26:%08X%08X", (UINT32)(mips3.r[26] >> 32), (UINT32)mips3.r[26]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R27:			sprintf(info->s, "R27:%08X%08X", (UINT32)(mips3.r[27] >> 32), (UINT32)mips3.r[27]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R28:			sprintf(info->s, "R28:%08X%08X", (UINT32)(mips3.r[28] >> 32), (UINT32)mips3.r[28]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R29:			sprintf(info->s, "R29:%08X%08X", (UINT32)(mips3.r[29] >> 32), (UINT32)mips3.r[29]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R30:			sprintf(info->s, "R30:%08X%08X", (UINT32)(mips3.r[30] >> 32), (UINT32)mips3.r[30]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R31:			sprintf(info->s, "R31:%08X%08X", (UINT32)(mips3.r[31] >> 32), (UINT32)mips3.r[31]); break;
		case CPUINFO_STR_REGISTER + MIPS3_HI:			sprintf(info->s, "HI: %08X%08X", (UINT32)(mips3.hi >> 32), (UINT32)mips3.hi); break;
		case CPUINFO_STR_REGISTER + MIPS3_LO:			sprintf(info->s, "LO: %08X%08X", (UINT32)(mips3.lo >> 32), (UINT32)mips3.lo); break;
	}
}


#if (HAS_R4600)
/**************************************************************************
 * CPU-specific set_info
 **************************************************************************/

void r4600be_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_RESET:							info->reset = r4600be_reset;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "R4600 (big)");			break;

		default:										mips3_get_info(state, info);			break;
	}
}


void r4600le_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_RESET:							info->reset = r4600le_reset;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "R4600 (little)");		break;

		default:										mips3_get_info(state, info);			break;
	}
}
#endif

#if (HAS_R4650)
/**************************************************************************
 * CPU-specific set_info
 **************************************************************************/

void r4650be_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_RESET:							info->reset = r4600be_reset;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "IDT R4650 (big)");		break;

		default:										mips3_get_info(state, info);			break;
	}
}


void r4650le_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_RESET:							info->reset = r4600le_reset;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "IDT R4650 (little)");	break;

		default:										mips3_get_info(state, info);			break;
	}
}
#endif

#if (HAS_R4700)
/**************************************************************************
 * CPU-specific set_info
 **************************************************************************/

void r4700be_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_RESET:							info->reset = r4700be_reset;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "R4700 (big)");			break;

		default:										mips3_get_info(state, info);			break;
	}
}


void r4700le_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_RESET:							info->reset = r4700le_reset;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "R4700 (little)");		break;

		default:										mips3_get_info(state, info);			break;
	}
}
#endif


#if (HAS_R5000)
/**************************************************************************
 * CPU-specific set_info
 **************************************************************************/

void r5000be_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_RESET:							info->reset = r5000be_reset;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "R5000 (big)");			break;

		default:										mips3_get_info(state, info);			break;
	}
}


void r5000le_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_RESET:							info->reset = r5000le_reset;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "R5000 (little)");		break;

		default:										mips3_get_info(state, info);			break;
	}
}
#endif


#if (HAS_QED5271)
/**************************************************************************
 * CPU-specific set_info
 **************************************************************************/

void qed5271be_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_RESET:							info->reset = qed5271be_reset;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "QED5271 (big)");		break;

		default:										mips3_get_info(state, info);			break;
	}
}


void qed5271le_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_RESET:							info->reset = qed5271le_reset;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "QED5271 (little)");	break;

		default:										mips3_get_info(state, info);			break;
	}
}
#endif


#if (HAS_RM7000)
/**************************************************************************
 * CPU-specific set_info
 **************************************************************************/

void rm7000be_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_RESET:							info->reset = rm7000be_reset;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "RM7000 (big)");		break;

		default:										mips3_get_info(state, info);			break;
	}
}


void rm7000le_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_RESET:							info->reset = rm7000le_reset;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "RM7000 (little)");		break;

		default:										mips3_get_info(state, info);			break;
	}
}
#endif




#if !defined(MAME_DEBUG) && (LOG_CODE)
#include "mips3dsm.c"
#include "cpu/i386/i386dasm.c"
#endif
