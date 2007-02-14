/***************************************************************************

    mips3.c
    Core implementation for the portable MIPS III/IV emulator.
    Written by Aaron Giles

    Still not implemented:
       * DMULT needs to be fixed properly

***************************************************************************/

#include <math.h>
#include "driver.h"
#include "debugger.h"
#include "mips3.h"


#define ENABLE_OVERFLOWS	0
#define PRINTF_TLB			0


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

#define RSVAL32		((UINT32)mips3.r[RSREG])
#define RTVAL32		((UINT32)mips3.r[RTREG])
#define RDVAL32		((UINT32)mips3.r[RDREG])

#define RSVAL64		(mips3.r[RSREG])
#define RTVAL64		(mips3.r[RTREG])
#define RDVAL64		(mips3.r[RDREG])

#define FRREG		((op >> 21) & 31)
#define FTREG		((op >> 16) & 31)
#define FSREG		((op >> 11) & 31)
#define FDREG		((op >> 6) & 31)

#define FRVALS_FR0	(((float *)&mips3.cpr[1][0])[FRREG])
#define FTVALS_FR0	(((float *)&mips3.cpr[1][0])[FTREG])
#define FSVALS_FR0	(((float *)&mips3.cpr[1][0])[FSREG])
#define FDVALS_FR0	(((float *)&mips3.cpr[1][0])[FDREG])
#define FSVALW_FR0	(((UINT32 *)&mips3.cpr[1][0])[FSREG])
#define FDVALW_FR0	(((UINT32 *)&mips3.cpr[1][0])[FDREG])

#define FRVALD_FR0	(*(double *)&mips3.cpr[1][FRREG/2])
#define FTVALD_FR0	(*(double *)&mips3.cpr[1][FTREG/2])
#define FSVALD_FR0	(*(double *)&mips3.cpr[1][FSREG/2])
#define FDVALD_FR0	(*(double *)&mips3.cpr[1][FDREG/2])
#define FSVALL_FR0	(((UINT64 *)&mips3.cpr[1][0])[FSREG/2])
#define FDVALL_FR0	(((UINT64 *)&mips3.cpr[1][0])[FDREG/2])

#define FRVALS_FR1	(((float *)&mips3.cpr[1][FRREG])[BYTE_XOR_LE(0)])
#define FTVALS_FR1	(((float *)&mips3.cpr[1][FTREG])[BYTE_XOR_LE(0)])
#define FSVALS_FR1	(((float *)&mips3.cpr[1][FSREG])[BYTE_XOR_LE(0)])
#define FDVALS_FR1	(((float *)&mips3.cpr[1][FDREG])[BYTE_XOR_LE(0)])
#define FSVALW_FR1	(((UINT32 *)&mips3.cpr[1][FSREG])[BYTE_XOR_LE(0)])
#define FDVALW_FR1	(((UINT32 *)&mips3.cpr[1][FDREG])[BYTE_XOR_LE(0)])

#define FRVALD_FR1	(*(double *)&mips3.cpr[1][FRREG])
#define FTVALD_FR1	(*(double *)&mips3.cpr[1][FTREG])
#define FSVALD_FR1	(*(double *)&mips3.cpr[1][FSREG])
#define FDVALD_FR1	(*(double *)&mips3.cpr[1][FDREG])
#define FSVALL_FR1	(*(UINT64 *)&mips3.cpr[1][FSREG])
#define FDVALL_FR1	(*(UINT64 *)&mips3.cpr[1][FDREG])

#define IS_SINGLE(o) (((o) & (1 << 21)) == 0)
#define IS_DOUBLE(o) (((o) & (1 << 21)) != 0)
#define IS_FLOAT(o) (((o) & (1 << 23)) == 0)
#define IS_INTEGRAL(o) (((o) & (1 << 23)) != 0)

#define SIMMVAL		((INT16)op)
#define UIMMVAL		((UINT16)op)
#define LIMMVAL		(op & 0x03ffffff)

#define ADDPC(x)	mips3.nextpc = mips3.pc + ((x) << 2)
#define ADDPCL(x,l)	{ mips3.nextpc = mips3.pc + ((x) << 2); mips3.r[l] = mips3.pc + 4; }
#define ABSPC(x)	mips3.nextpc = (mips3.pc & 0xf0000000) | ((x) << 2)
#define ABSPCL(x,l)	{ mips3.nextpc = (mips3.pc & 0xf0000000) | ((x) << 2); mips3.r[l] = mips3.pc + 4; }
#define SETPC(x)	mips3.nextpc = (x)
#define SETPCL(x,l)	{ mips3.nextpc = (x); mips3.r[l] = mips3.pc + 4; }

#define HIVAL		(UINT32)mips3.hi
#define LOVAL		(UINT32)mips3.lo
#define HIVAL64		mips3.hi
#define LOVAL64		mips3.lo
#define SR			mips3.cpr[0][COP0_Status]
#define CAUSE		mips3.cpr[0][COP0_Cause]

#define GET_FCC(n)	((mips3.ccr[1][31] >> fcc_shift[n]) & 1)
#define SET_FCC(n,v) (mips3.ccr[1][31] = (mips3.ccr[1][31] & ~(1 << fcc_shift[n])) | ((v) << fcc_shift[n]))

#define IS_FR0		(!(SR & SR_FR))
#define IS_FR1		(SR & SR_FR)


/***************************************************************************
    STRUCTURES & TYPEDEFS
***************************************************************************/

/* memory access function table */
typedef struct
{
	UINT8		(*readbyte)(offs_t);
	UINT16		(*readword)(offs_t);
	UINT32		(*readlong)(offs_t);
	UINT64		(*readdouble)(offs_t);
	void		(*writebyte)(offs_t, UINT8);
	void		(*writeword)(offs_t, UINT16);
	void		(*writelong)(offs_t, UINT32);
	void		(*writedouble)(offs_t, UINT64);
} memory_handlers;


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
	UINT8		cf[4];

	/* internal stuff */
	UINT32		ppc;
	UINT32		nextpc;
	UINT32		pcbase;
	int			op;
	int			interrupt_cycles;
	int 		(*irq_callback)(int irqline);
	UINT64		count_zero_time;
	void *		compare_int_timer;
	UINT8		is_mips4;
	UINT32		ll_value;
	UINT64		lld_value;
	UINT32		system_clock;
	UINT32		cpu_clock;

	/* endian-dependent load/store */
	void		(*lwl)(UINT32 op);
	void		(*lwr)(UINT32 op);
	void		(*swl)(UINT32 op);
	void		(*swr)(UINT32 op);
	void		(*ldl)(UINT32 op);
	void		(*ldr)(UINT32 op);
	void		(*sdl)(UINT32 op);
	void		(*sdr)(UINT32 op);

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
} mips3_regs;



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static void recompute_tlb_table(void);

static void lwl_be(UINT32 op);
static void lwr_be(UINT32 op);
static void swl_be(UINT32 op);
static void swr_be(UINT32 op);

static void lwl_le(UINT32 op);
static void lwr_le(UINT32 op);
static void swl_le(UINT32 op);
static void swr_le(UINT32 op);

static void ldl_be(UINT32 op);
static void ldr_be(UINT32 op);
static void sdl_be(UINT32 op);
static void sdr_be(UINT32 op);

static void ldl_le(UINT32 op);
static void ldr_le(UINT32 op);
static void sdl_le(UINT32 op);
static void sdr_le(UINT32 op);

static UINT64 read_qword_32be(offs_t offset);
static UINT64 read_qword_32le(offs_t offset);

static void write_qword_32be(offs_t offset, UINT64 data);
static void write_qword_32le(offs_t offset, UINT64 data);

static UINT8 fcc_shift[8] = { 23, 25, 26, 27, 28, 29, 30, 31 };



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
	program_read_byte_32be,  program_read_word_32be,  program_read_dword_32be, read_qword_32be,
	program_write_byte_32be, program_write_word_32be, program_write_dword_32be, write_qword_32be
};

static const memory_handlers le_memory =
{
	program_read_byte_32le,  program_read_word_32le,  program_read_dword_32le, read_qword_32le,
	program_write_byte_32le, program_write_word_32le, program_write_dword_32le, write_qword_32le
};



/***************************************************************************
    MEMORY ACCESSORS
***************************************************************************/

#define ROPCODE(pc)		cpu_readop32(pc)



/***************************************************************************
    EXECEPTION HANDLING
***************************************************************************/

INLINE void generate_exception(int exception, int backup)
{
/*
    useful for catching exceptions:

    if (exception != 0)
    {
        fprintf(stderr, "Exception: PC=%08X, PPC=%08X\n", mips3.pc, mips3.ppc);
        DEBUGGER_BREAK;
    }
*/

	/* back up if necessary */
	if (backup)
		mips3.pc = mips3.ppc;

	/* set the exception PC */
	mips3.cpr[0][COP0_EPC] = mips3.pc;

	/* put the cause in the low 8 bits and clear the branch delay flag */
	CAUSE = (CAUSE & ~0x800000ff) | (exception << 2);

	/* if we were in a branch delay slot, adjust */
	if (mips3.nextpc != ~0)
	{
		mips3.nextpc = ~0;
		mips3.cpr[0][COP0_EPC] -= 4;
		CAUSE |= 0x80000000;
	}

	/* set the exception level */
	SR |= SR_EXL;

	/* based on the BEV bit, we either go to ROM or RAM */
	mips3.pc = (SR & SR_BEV) ? 0xbfc00200 : 0x80000000;

	/* most exceptions go to offset 0x180, except for TLB stuff */
	if (exception >= EXCEPTION_TLBMOD && exception <= EXCEPTION_TLBSTORE)
	{
		mips3.pc += 0x00;
		mame_printf_debug("TLB miss @ %08X\n", (UINT32)mips3.cpr[0][COP0_BadVAddr]);
	}
	else
		mips3.pc += 0x180;

/*
    useful for tracking interrupts

    if ((CAUSE & 0x7f) == 0)
        logerror("Took interrupt -- Cause = %08X, PC =  %08X\n", (UINT32)CAUSE, mips3.pc);
*/
}


static void generate_tlb_exception(int exception, offs_t address)
{
	mips3.cpr[0][COP0_BadVAddr] = address;
	mips3.cpr[0][COP0_Context] = (mips3.cpr[0][COP0_Context] & 0xff800000) | ((address >> 9) & 0x007ffff0);
	mips3.cpr[0][COP0_EntryHi] = (address & 0xffffe000) | (mips3.cpr[0][COP0_EntryHi] & 0xff);
	generate_exception(exception, 1);
}


INLINE void invalid_instruction(UINT32 op)
{
	generate_exception(EXCEPTION_INVALIDOP, 1);
}



/***************************************************************************
    IRQ HANDLING
***************************************************************************/

static void check_irqs(void)
{
	if ((CAUSE & SR & 0xfc00) && (SR & SR_IE) && !(SR & SR_EXL) && !(SR & SR_ERL))
		generate_exception(EXCEPTION_INTERRUPT, 0);
}


static void set_irq_line(int irqline, int state)
{
	if (state != CLEAR_LINE)
		CAUSE |= 0x400 << irqline;
	else
		CAUSE &= ~(0x400 << irqline);
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

	mips3.irq_callback = irqcallback;
	mips3.compare_int_timer = timer_alloc(compare_int_callback);

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
	{
		mips3.memory = be_memory;
		mips3.lwl = lwl_be;
		mips3.lwr = lwr_be;
		mips3.swl = swl_be;
		mips3.swr = swr_be;
		mips3.ldl = ldl_be;
		mips3.ldr = ldr_be;
		mips3.sdl = sdl_be;
		mips3.sdr = sdr_be;
	}
	else
	{
		mips3.memory = le_memory;
		mips3.lwl = lwl_le;
		mips3.lwr = lwr_le;
		mips3.swl = swl_le;
		mips3.swr = swr_le;
		mips3.ldl = ldl_le;
		mips3.ldr = ldr_le;
		mips3.sdl = sdl_le;
		mips3.sdr = sdr_le;
	}

	/* initialize the state */
	mips3.pc = 0xbfc00000;
	mips3.ppc = ~mips3.pc;
	mips3.nextpc = ~0;
	mips3.cpr[0][COP0_Status] = SR_BEV | SR_ERL;
	mips3.cpr[0][COP0_Compare] = 0xffffffff;
	mips3.cpr[0][COP0_Count] = 0;
	mips3.count_zero_time = activecpu_gettotalcycles64();

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
}


/* kludge for DRC support */
static void mips3drc_set_options(UINT8 cpunum, UINT32 opts)
{
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


static int update_pcbase(void)
{
	UINT32 entry = mips3.tlb_table[mips3.pc >> 12];
	if (entry == 0xffffffff)
	{
		generate_tlb_exception(EXCEPTION_TLBLOAD, mips3.pc);
		return 0;
	}
	mips3.pcbase = entry & ~0xfff;
	change_pc(mips3.pcbase);
	return 1;
}


INLINE int RBYTE(offs_t address, UINT32 *result)
{
	UINT32 tlbval = mips3.tlb_table[address >> 12];
	if (tlbval == 0xffffffff)
	{
		generate_tlb_exception(EXCEPTION_TLBLOAD, address);
		return 0;
	}
	*result = (*mips3.memory.readbyte)((tlbval & ~0xfff) | (address & 0xfff));
	return 1;
}


INLINE int RWORD(offs_t address, UINT32 *result)
{
	UINT32 tlbval = mips3.tlb_table[address >> 12];
	if (tlbval == 0xffffffff)
	{
		generate_tlb_exception(EXCEPTION_TLBLOAD, address);
		return 0;
	}
	*result = (*mips3.memory.readword)((tlbval & ~0xfff) | (address & 0xfff));
	return 1;
}


INLINE int RLONG(offs_t address, UINT32 *result)
{
	UINT32 tlbval = mips3.tlb_table[address >> 12];
	if (tlbval == 0xffffffff)
	{
		generate_tlb_exception(EXCEPTION_TLBLOAD, address);
		return 0;
	}
	*result = (*mips3.memory.readlong)((tlbval & ~0xfff) | (address & 0xfff));
	return 1;
}


INLINE int RDOUBLE(offs_t address, UINT64 *result)
{
	UINT32 tlbval = mips3.tlb_table[address >> 12];
	if (tlbval == 0xffffffff)
	{
		generate_tlb_exception(EXCEPTION_TLBLOAD, address);
		return 0;
	}
	*result = (*mips3.memory.readdouble)((tlbval & ~0xfff) | (address & 0xfff));
	return 1;
}


INLINE void WBYTE(offs_t address, UINT8 data)
{
	UINT32 tlbval = mips3.tlb_table[address >> 12];
	if (tlbval & 1)
		generate_tlb_exception(EXCEPTION_TLBSTORE, address);
	else
		(*mips3.memory.writebyte)(tlbval | (address & 0xfff), data);
}


INLINE void WWORD(offs_t address, UINT16 data)
{
	UINT32 tlbval = mips3.tlb_table[address >> 12];
	if (tlbval & 1)
		generate_tlb_exception(EXCEPTION_TLBSTORE, address);
	else
		(*mips3.memory.writeword)(tlbval | (address & 0xfff), data);
}


INLINE void WLONG(offs_t address, UINT32 data)
{
	UINT32 tlbval = mips3.tlb_table[address >> 12];
	if (tlbval & 1)
		generate_tlb_exception(EXCEPTION_TLBSTORE, address);
	else
		(*mips3.memory.writelong)(tlbval | (address & 0xfff), data);
}


INLINE void WDOUBLE(offs_t address, UINT64 data)
{
	UINT32 tlbval = mips3.tlb_table[address >> 12];
	if (tlbval & 1)
		generate_tlb_exception(EXCEPTION_TLBSTORE, address);
	else
		(*mips3.memory.writedouble)(tlbval | (address & 0xfff), data);
}



/***************************************************************************
    COP0 (SYSTEM) EXECUTION HANDLING
***************************************************************************/

static void update_cycle_counting(void)
{
	/* modify the timer to go off */
	if ((SR & 0x8000) && mips3.cpr[0][COP0_Compare] != 0xffffffff)
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

INLINE UINT64 get_cop0_reg(int idx)
{
	if (idx == COP0_Count)
	{
		/* it doesn't really take 250 cycles to read this register, but it helps speed */
		/* up loops that hammer on it */
		if (mips3_icount >= 250)
			mips3_icount -= 250;
		else
			mips3_icount = 0;
		return (UINT32)((activecpu_gettotalcycles64() - mips3.count_zero_time) / 2);
	}
	else if (idx == COP0_Cause)
	{
		/* it doesn't really take 250 cycles to read this register, but it helps speed */
		/* up loops that hammer on it */
		if (mips3_icount >= 250)
			mips3_icount -= 250;
		else
			mips3_icount = 0;
	}
	else if (idx == COP0_Random)
	{
		int wired = mips3.cpr[0][COP0_Wired] & 0x3f;
		int range = 48 - wired;
		if (range > 0)
			return ((activecpu_gettotalcycles64() - mips3.count_zero_time) % range + wired) & 0x3f;
		else
			return 47;
	}
	return mips3.cpr[0][idx];
}

INLINE void set_cop0_reg(int idx, UINT64 val)
{
	switch (idx)
	{
		case COP0_Cause:
			CAUSE = (CAUSE & 0xfc00) | (val & ~0xfc00);
			if (CAUSE & 0x300)
			{
				/* if we're in a delay slot, propogate the target PC before generating the exception */
				if (mips3.nextpc != ~0)
				{
					mips3.pc = mips3.nextpc;
					mips3.nextpc = ~0;
				}
				generate_exception(EXCEPTION_INTERRUPT, 0);
			}
			break;

		case COP0_Status:
		{
			/* update interrupts and cycle counting */
			UINT32 diff = mips3.cpr[0][idx] ^ val;
//          if (val & 0xe0)
//              fatalerror("System set 64-bit addressing mode, SR=%08X", val);
			mips3.cpr[0][idx] = val;
			if (diff & 0x8000)
				update_cycle_counting();
			check_irqs();
			break;
		}

		case COP0_Count:
			mips3.count_zero_time = activecpu_gettotalcycles64() - ((UINT64)(UINT32)val * 2);
			update_cycle_counting();
			break;

		case COP0_Compare:
			CAUSE &= ~0x8000;
			mips3.cpr[0][idx] = val & 0xffffffff;
			update_cycle_counting();
			break;

		case COP0_PRId:
			break;

		case COP0_Config:
			mips3.cpr[0][idx] = (mips3.cpr[0][idx] & ~7) | (val & 7);
			break;

		case COP0_EntryHi:
			/* if the ASID changes, remap */
			if ((mips3.cpr[0][idx] ^ val) & 0xff)
			{
				mips3.cpr[0][idx] = val;
				unmap_tlb_entries();
				map_tlb_entries();
			}
			mips3.cpr[0][idx] = val;
			break;

		default:
			mips3.cpr[0][idx] = val;
			break;
	}
}

INLINE UINT64 get_cop0_creg(int idx)
{
	return mips3.ccr[0][idx];
}

INLINE void set_cop0_creg(int idx, UINT64 val)
{
	mips3.ccr[0][idx] = val;
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

INLINE void handle_cop0(UINT32 op)
{
	UINT32 index, wired, vpn;

	if ((SR & SR_KSU_MASK) != SR_KSU_KERNEL && !(SR & SR_COP0))
		generate_exception(EXCEPTION_BADCOP, 1);

	switch (RSREG)
	{
		case 0x00:	/* MFCz */		if (RTREG) RTVAL64 = (INT32)get_cop0_reg(RDREG);		break;
		case 0x01:	/* DMFCz */		if (RTREG) RTVAL64 = get_cop0_reg(RDREG);				break;
		case 0x02:	/* CFCz */		if (RTREG) RTVAL64 = (INT32)get_cop0_creg(RDREG);		break;
		case 0x04:	/* MTCz */		set_cop0_reg(RDREG, RTVAL32);							break;
		case 0x05:	/* DMTCz */		set_cop0_reg(RDREG, RTVAL64);							break;
		case 0x06:	/* CTCz */		set_cop0_creg(RDREG, RTVAL32);							break;
		case 0x08:	/* BC */
			switch (RTREG)
			{
				case 0x00:	/* BCzF */	if (!mips3.cf[0]) ADDPC(SIMMVAL);					break;
				case 0x01:	/* BCzF */	if (mips3.cf[0]) ADDPC(SIMMVAL);					break;
				case 0x02:	/* BCzFL */	invalid_instruction(op);							break;
				case 0x03:	/* BCzTL */	invalid_instruction(op);							break;
				default:	invalid_instruction(op);										break;
			}
			break;
		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13:
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x17:
		case 0x18:
		case 0x19:
		case 0x1a:
		case 0x1b:
		case 0x1c:
		case 0x1d:
		case 0x1e:
		case 0x1f:	/* COP */
			switch (op & 0x01ffffff)
			{
				case 0x01:	/* TLBR */
					index = mips3.cpr[0][COP0_Index] & 0x3f;
					if (index < 48)
					{
						mips3.cpr[0][COP0_PageMask] = mips3.tlb[index].page_mask;
						mips3.cpr[0][COP0_EntryHi] = mips3.tlb[index].entry_hi;
						mips3.cpr[0][COP0_EntryLo0] = mips3.tlb[index].entry_lo[0];
						mips3.cpr[0][COP0_EntryLo1] = mips3.tlb[index].entry_lo[1];
					}
					break;

				case 0x02:	/* TLBWI */
					index = mips3.cpr[0][COP0_Index] & 0x3f;
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
					break;

				case 0x06:	/* TLBWR */
					wired = mips3.cpr[0][COP0_Wired] & 0x3f;
					index = 48 - wired;
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
					break;

				case 0x08:	/* TLBP */
//                  DEBUGGER_BREAK;
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
//                          DEBUGGER_BREAK;
#endif
						}
						mips3.cpr[0][COP0_Index] = index;
					}
					else
					{
						if (mips3.tlb_table[vpn & 0xfffff] != 0xffffffff)
						{
#if PRINTF_TLB
							mame_printf_debug("TLBP: Should have found an entry\n");
//                          DEBUGGER_BREAK;
#endif
						}
						mips3.cpr[0][COP0_Index] = 0x80000000;
					}
					break;

				case 0x10:	/* RFE */	invalid_instruction(op);							break;
				case 0x18:	/* ERET */	logerror("ERET\n"); mips3.pc = mips3.cpr[0][COP0_EPC]; SR &= ~SR_EXL; check_irqs(); mips3.lld_value ^= 0xffffffff; mips3.ll_value ^= 0xffffffff;	break;
				default:	invalid_instruction(op);										break;
			}
			break;
		default:	invalid_instruction(op);												break;
	}
}



/***************************************************************************
    COP1 (FPU) EXECUTION HANDLING
***************************************************************************/

INLINE UINT32 get_cop1_reg32(int idx)
{
	if (IS_FR0)
		return ((UINT32 *)&mips3.cpr[1][0])[idx];
	else
		return mips3.cpr[1][idx];
}

INLINE UINT64 get_cop1_reg64(int idx)
{
	if (IS_FR0)
		return ((UINT64 *)&mips3.cpr[1][0])[idx/2];
	else
		return mips3.cpr[1][idx];
}

INLINE void set_cop1_reg32(int idx, UINT32 val)
{
	if (IS_FR0)
		((UINT32 *)&mips3.cpr[1][0])[idx] = val;
	else
		mips3.cpr[1][idx] = val;
}

INLINE void set_cop1_reg64(int idx, UINT64 val)
{
	if (IS_FR0)
		((UINT64 *)&mips3.cpr[1][0])[idx/2] = val;
	else
		mips3.cpr[1][idx] = val;
}

INLINE UINT64 get_cop1_creg(int idx)
{
	return mips3.ccr[1][idx];
}

INLINE void set_cop1_creg(int idx, UINT64 val)
{
	mips3.ccr[1][idx] = val;
}

INLINE void handle_cop1_fr0(UINT32 op)
{
	double dtemp;

	/* note: additional condition codes available on R5000 only */

	if (!(SR & SR_COP1))
		generate_exception(EXCEPTION_BADCOP, 1);

	switch (RSREG)
	{
		case 0x00:	/* MFCz */		if (RTREG) RTVAL64 = (INT32)get_cop1_reg32(RDREG);		break;
		case 0x01:	/* DMFCz */		if (RTREG) RTVAL64 = get_cop1_reg64(RDREG);				break;
		case 0x02:	/* CFCz */		if (RTREG) RTVAL64 = (INT32)get_cop1_creg(RDREG);		break;
		case 0x04:	/* MTCz */		set_cop1_reg32(RDREG, RTVAL32);							break;
		case 0x05:	/* DMTCz */		set_cop1_reg64(RDREG, RTVAL64);							break;
		case 0x06:	/* CTCz */		set_cop1_creg(RDREG, RTVAL32);							break;
		case 0x08:	/* BC */
			switch ((op >> 16) & 3)
			{
				case 0x00:	/* BCzF */	if (!GET_FCC((op >> 18) & 7)) ADDPC(SIMMVAL);	break;
				case 0x01:	/* BCzT */	if (GET_FCC((op >> 18) & 7)) ADDPC(SIMMVAL);	break;
				case 0x02:	/* BCzFL */	if (!GET_FCC((op >> 18) & 7)) ADDPC(SIMMVAL); else mips3.pc += 4;	break;
				case 0x03:	/* BCzTL */	if (GET_FCC((op >> 18) & 7)) ADDPC(SIMMVAL); else mips3.pc += 4;	break;
			}
			break;
		default:
			switch (op & 0x3f)
			{
				case 0x00:
					if (IS_SINGLE(op))	/* ADD.S */
						FDVALS_FR0 = FSVALS_FR0 + FTVALS_FR0;
					else				/* ADD.D */
						FDVALD_FR0 = FSVALD_FR0 + FTVALD_FR0;
					break;

				case 0x01:
					if (IS_SINGLE(op))	/* SUB.S */
						FDVALS_FR0 = FSVALS_FR0 - FTVALS_FR0;
					else				/* SUB.D */
						FDVALD_FR0 = FSVALD_FR0 - FTVALD_FR0;
					break;

				case 0x02:
					if (IS_SINGLE(op))	/* MUL.S */
						FDVALS_FR0 = FSVALS_FR0 * FTVALS_FR0;
					else				/* MUL.D */
						FDVALD_FR0 = FSVALD_FR0 * FTVALD_FR0;
					break;

				case 0x03:
					if (IS_SINGLE(op))	/* DIV.S */
						FDVALS_FR0 = FSVALS_FR0 / FTVALS_FR0;
					else				/* DIV.D */
						FDVALD_FR0 = FSVALD_FR0 / FTVALD_FR0;
					break;

				case 0x04:
					if (IS_SINGLE(op))	/* SQRT.S */
						FDVALS_FR0 = sqrt(FSVALS_FR0);
					else				/* SQRT.D */
						FDVALD_FR0 = sqrt(FSVALD_FR0);
					break;

				case 0x05:
					if (IS_SINGLE(op))	/* ABS.S */
						FDVALS_FR0 = fabs(FSVALS_FR0);
					else				/* ABS.D */
						FDVALD_FR0 = fabs(FSVALD_FR0);
					break;

				case 0x06:
					if (IS_SINGLE(op))	/* MOV.S */
						FDVALS_FR0 = FSVALS_FR0;
					else				/* MOV.D */
						FDVALD_FR0 = FSVALD_FR0;
					break;

				case 0x07:
					if (IS_SINGLE(op))	/* NEG.S */
						FDVALS_FR0 = -FSVALS_FR0;
					else				/* NEG.D */
						FDVALD_FR0 = -FSVALD_FR0;
					break;

				case 0x08:
					if (IS_SINGLE(op))	/* ROUND.L.S */
					{
						double temp = FSVALS_FR0;
						if (temp < 0)
							temp = ceil(temp - 0.5);
						else
							temp = floor(temp + 0.5);
						FDVALL_FR0 = (INT64)temp;
					}
					else				/* ROUND.L.D */
					{
						double temp = FSVALD_FR0;
						if (temp < 0)
							temp = ceil(temp - 0.5);
						else
							temp = floor(temp + 0.5);
						FDVALL_FR0 = (INT64)temp;
					}
					break;

				case 0x09:
					if (IS_SINGLE(op))	/* TRUNC.L.S */
					{
						double temp = FSVALS_FR0;
						if (temp < 0)
							temp = ceil(temp);
						else
							temp = floor(temp);
						FDVALL_FR0 = (INT64)temp;
					}
					else				/* TRUNC.L.D */
					{
						double temp = FSVALD_FR0;
						if (temp < 0)
							temp = ceil(temp);
						else
							temp = floor(temp);
						FDVALL_FR0 = (INT64)temp;
					}
					break;

				case 0x0a:
					if (IS_SINGLE(op))	/* CEIL.L.S */
						dtemp = ceil(FSVALS_FR0);
					else				/* CEIL.L.D */
						dtemp = ceil(FSVALD_FR0);
					FDVALL_FR0 = (INT64)dtemp;
					break;

				case 0x0b:
					if (IS_SINGLE(op))	/* FLOOR.L.S */
						dtemp = floor(FSVALS_FR0);
					else				/* FLOOR.L.D */
						dtemp = floor(FSVALD_FR0);
					FDVALL_FR0 = (INT64)dtemp;
					break;

				case 0x0c:
					if (IS_SINGLE(op))	/* ROUND.W.S */
					{
						dtemp = FSVALS_FR0;
						if (dtemp < 0)
							dtemp = ceil(dtemp - 0.5);
						else
							dtemp = floor(dtemp + 0.5);
						FDVALW_FR0 = (INT32)dtemp;
					}
					else				/* ROUND.W.D */
					{
						dtemp = FSVALD_FR0;
						if (dtemp < 0)
							dtemp = ceil(dtemp - 0.5);
						else
							dtemp = floor(dtemp + 0.5);
						FDVALW_FR0 = (INT32)dtemp;
					}
					break;

				case 0x0d:
					if (IS_SINGLE(op))	/* TRUNC.W.S */
					{
						dtemp = FSVALS_FR0;
						if (dtemp < 0)
							dtemp = ceil(dtemp);
						else
							dtemp = floor(dtemp);
						FDVALW_FR0 = (INT32)dtemp;
					}
					else				/* TRUNC.W.D */
					{
						dtemp = FSVALD_FR0;
						if (dtemp < 0)
							dtemp = ceil(dtemp);
						else
							dtemp = floor(dtemp);
						FDVALW_FR0 = (INT32)dtemp;
					}
					break;

				case 0x0e:
					if (IS_SINGLE(op))	/* CEIL.W.S */
						dtemp = ceil(FSVALS_FR0);
					else				/* CEIL.W.D */
						dtemp = ceil(FSVALD_FR0);
					FDVALW_FR0 = (INT32)dtemp;
					break;

				case 0x0f:
					if (IS_SINGLE(op))	/* FLOOR.W.S */
						dtemp = floor(FSVALS_FR0);
					else				/* FLOOR.W.D */
						dtemp = floor(FSVALD_FR0);
					FDVALW_FR0 = (INT32)dtemp;
					break;

				case 0x11:	/* R5000 */
					if (GET_FCC((op >> 18) & 7) == ((op >> 16) & 1))
					{
						if (IS_SINGLE(op))	/* MOVT/F.S */
							FDVALS_FR0 = FSVALS_FR0;
						else				/* MOVT/F.D */
							FDVALD_FR0 = FSVALD_FR0;
					}
					break;

				case 0x12:	/* R5000 */
					if (RTVAL64 == 0)
					{
						if (IS_SINGLE(op))	/* MOVZ.S */
							FDVALS_FR0 = FSVALS_FR0;
						else				/* MOVZ.D */
							FDVALD_FR0 = FSVALD_FR0;
					}
					break;

				case 0x13:	/* R5000 */
					if (RTVAL64 != 0)
					{
						if (IS_SINGLE(op))	/* MOVN.S */
							FDVALS_FR0 = FSVALS_FR0;
						else				/* MOVN.D */
							FDVALD_FR0 = FSVALD_FR0;
					}
					break;

				case 0x15:	/* R5000 */
					if (IS_SINGLE(op))	/* RECIP.S */
						FDVALS_FR0 = 1.0f / FSVALS_FR0;
					else				/* RECIP.D */
						FDVALD_FR0 = 1.0 / FSVALD_FR0;
					break;

				case 0x16:	/* R5000 */
					if (IS_SINGLE(op))	/* RSQRT.S */
						FDVALS_FR0 = 1.0f / sqrt(FSVALS_FR0);
					else				/* RSQRT.D */
						FDVALD_FR0 = 1.0 / sqrt(FSVALD_FR0);
					break;

				case 0x20:
					if (IS_INTEGRAL(op))
					{
						if (IS_SINGLE(op))	/* CVT.S.W */
							FDVALS_FR0 = (INT32)FSVALW_FR0;
						else				/* CVT.S.L */
							FDVALS_FR0 = (INT64)FSVALL_FR0;
					}
					else					/* CVT.S.D */
						FDVALS_FR0 = FSVALD_FR0;
					break;

				case 0x21:
					if (IS_INTEGRAL(op))
					{
						if (IS_SINGLE(op))	/* CVT.D.W */
							FDVALD_FR0 = (INT32)FSVALW_FR0;
						else				/* CVT.D.L */
							FDVALD_FR0 = (INT64)FSVALL_FR0;
					}
					else					/* CVT.D.S */
						FDVALD_FR0 = FSVALS_FR0;
					break;

				case 0x24:
					if (IS_SINGLE(op))	/* CVT.W.S */
						FDVALW_FR0 = (INT32)FSVALS_FR0;
					else
						FDVALW_FR0 = (INT32)FSVALD_FR0;
					break;

				case 0x25:
					if (IS_SINGLE(op))	/* CVT.L.S */
						FDVALL_FR0 = (INT64)FSVALS_FR0;
					else				/* CVT.L.D */
						FDVALL_FR0 = (INT64)FSVALD_FR0;
					break;

				case 0x30:
				case 0x38:
					if (IS_SINGLE(op))	/* C.F.S */
						SET_FCC((op >> 8) & 7, 0);
					else				/* C.F.D */
						SET_FCC((op >> 8) & 7, 0);
					break;

				case 0x31:
				case 0x39:
					if (IS_SINGLE(op))	/* C.UN.S */
						SET_FCC((op >> 8) & 7, 0);
					else				/* C.UN.D */
						SET_FCC((op >> 8) & 7, 0);
					break;

				case 0x32:
				case 0x3a:
					if (IS_SINGLE(op))	/* C.EQ.S */
						SET_FCC((op >> 8) & 7, (FSVALS_FR0 == FTVALS_FR0));
					else				/* C.EQ.D */
						SET_FCC((op >> 8) & 7, (FSVALD_FR0 == FTVALD_FR0));
					break;

				case 0x33:
				case 0x3b:
					if (IS_SINGLE(op))	/* C.UEQ.S */
						SET_FCC((op >> 8) & 7, (FSVALS_FR0 == FTVALS_FR0));
					else				/* C.UEQ.D */
						SET_FCC((op >> 8) & 7, (FSVALD_FR0 == FTVALD_FR0));
					break;

				case 0x34:
				case 0x3c:
					if (IS_SINGLE(op))	/* C.OLT.S */
						SET_FCC((op >> 8) & 7, (FSVALS_FR0 < FTVALS_FR0));
					else				/* C.OLT.D */
						SET_FCC((op >> 8) & 7, (FSVALD_FR0 < FTVALD_FR0));
					break;

				case 0x35:
				case 0x3d:
					if (IS_SINGLE(op))	/* C.ULT.S */
						SET_FCC((op >> 8) & 7, (FSVALS_FR0 < FTVALS_FR0));
					else				/* C.ULT.D */
						SET_FCC((op >> 8) & 7, (FSVALD_FR0 < FTVALD_FR0));
					break;

				case 0x36:
				case 0x3e:
					if (IS_SINGLE(op))	/* C.OLE.S */
						SET_FCC((op >> 8) & 7, (FSVALS_FR0 <= FTVALS_FR0));
					else				/* C.OLE.D */
						SET_FCC((op >> 8) & 7, (FSVALD_FR0 <= FTVALD_FR0));
					break;

				case 0x37:
				case 0x3f:
					if (IS_SINGLE(op))	/* C.ULE.S */
						SET_FCC((op >> 8) & 7, (FSVALS_FR0 <= FTVALS_FR0));
					else				/* C.ULE.D */
						SET_FCC((op >> 8) & 7, (FSVALD_FR0 <= FTVALD_FR0));
					break;

				default:
					fprintf(stderr, "cop1 %X\n", op);
					break;
			}
			break;
	}
}


INLINE void handle_cop1_fr1(UINT32 op)
{
	double dtemp;

	/* note: additional condition codes available on R5000 only */

	if (!(SR & SR_COP1))
		generate_exception(EXCEPTION_BADCOP, 1);

	switch (RSREG)
	{
		case 0x00:	/* MFCz */		if (RTREG) RTVAL64 = (INT32)get_cop1_reg32(RDREG);		break;
		case 0x01:	/* DMFCz */		if (RTREG) RTVAL64 = get_cop1_reg64(RDREG);				break;
		case 0x02:	/* CFCz */		if (RTREG) RTVAL64 = (INT32)get_cop1_creg(RDREG);		break;
		case 0x04:	/* MTCz */		set_cop1_reg32(RDREG, RTVAL32);							break;
		case 0x05:	/* DMTCz */		set_cop1_reg64(RDREG, RTVAL64);							break;
		case 0x06:	/* CTCz */		set_cop1_creg(RDREG, RTVAL32);							break;
		case 0x08:	/* BC */
			switch ((op >> 16) & 3)
			{
				case 0x00:	/* BCzF */	if (!GET_FCC((op >> 18) & 7)) ADDPC(SIMMVAL);	break;
				case 0x01:	/* BCzT */	if (GET_FCC((op >> 18) & 7)) ADDPC(SIMMVAL);	break;
				case 0x02:	/* BCzFL */	if (!GET_FCC((op >> 18) & 7)) ADDPC(SIMMVAL); else mips3.pc += 4;	break;
				case 0x03:	/* BCzTL */	if (GET_FCC((op >> 18) & 7)) ADDPC(SIMMVAL); else mips3.pc += 4;	break;
			}
			break;
		default:
			switch (op & 0x3f)
			{
				case 0x00:
					if (IS_SINGLE(op))	/* ADD.S */
						FDVALS_FR1 = FSVALS_FR1 + FTVALS_FR1;
					else				/* ADD.D */
						FDVALD_FR1 = FSVALD_FR1 + FTVALD_FR1;
					break;

				case 0x01:
					if (IS_SINGLE(op))	/* SUB.S */
						FDVALS_FR1 = FSVALS_FR1 - FTVALS_FR1;
					else				/* SUB.D */
						FDVALD_FR1 = FSVALD_FR1 - FTVALD_FR1;
					break;

				case 0x02:
					if (IS_SINGLE(op))	/* MUL.S */
						FDVALS_FR1 = FSVALS_FR1 * FTVALS_FR1;
					else				/* MUL.D */
						FDVALD_FR1 = FSVALD_FR1 * FTVALD_FR1;
					break;

				case 0x03:
					if (IS_SINGLE(op))	/* DIV.S */
						FDVALS_FR1 = FSVALS_FR1 / FTVALS_FR1;
					else				/* DIV.D */
						FDVALD_FR1 = FSVALD_FR1 / FTVALD_FR1;
					break;

				case 0x04:
					if (IS_SINGLE(op))	/* SQRT.S */
						FDVALS_FR1 = sqrt(FSVALS_FR1);
					else				/* SQRT.D */
						FDVALD_FR1 = sqrt(FSVALD_FR1);
					break;

				case 0x05:
					if (IS_SINGLE(op))	/* ABS.S */
						FDVALS_FR1 = fabs(FSVALS_FR1);
					else				/* ABS.D */
						FDVALD_FR1 = fabs(FSVALD_FR1);
					break;

				case 0x06:
					if (IS_SINGLE(op))	/* MOV.S */
						FDVALS_FR1 = FSVALS_FR1;
					else				/* MOV.D */
						FDVALD_FR1 = FSVALD_FR1;
					break;

				case 0x07:
					if (IS_SINGLE(op))	/* NEG.S */
						FDVALS_FR1 = -FSVALS_FR1;
					else				/* NEG.D */
						FDVALD_FR1 = -FSVALD_FR1;
					break;

				case 0x08:
					if (IS_SINGLE(op))	/* ROUND.L.S */
					{
						double temp = FSVALS_FR1;
						if (temp < 0)
							temp = ceil(temp - 0.5);
						else
							temp = floor(temp + 0.5);
						FDVALL_FR1 = (INT64)temp;
					}
					else				/* ROUND.L.D */
					{
						double temp = FSVALD_FR1;
						if (temp < 0)
							temp = ceil(temp - 0.5);
						else
							temp = floor(temp + 0.5);
						FDVALL_FR1 = (INT64)temp;
					}
					break;

				case 0x09:
					if (IS_SINGLE(op))	/* TRUNC.L.S */
					{
						double temp = FSVALS_FR1;
						if (temp < 0)
							temp = ceil(temp);
						else
							temp = floor(temp);
						FDVALL_FR1 = (INT64)temp;
					}
					else				/* TRUNC.L.D */
					{
						double temp = FSVALD_FR1;
						if (temp < 0)
							temp = ceil(temp);
						else
							temp = floor(temp);
						FDVALL_FR1 = (INT64)temp;
					}
					break;

				case 0x0a:
					if (IS_SINGLE(op))	/* CEIL.L.S */
						dtemp = ceil(FSVALS_FR1);
					else				/* CEIL.L.D */
						dtemp = ceil(FSVALD_FR1);
					FDVALL_FR1 = (INT64)dtemp;
					break;

				case 0x0b:
					if (IS_SINGLE(op))	/* FLOOR.L.S */
						dtemp = floor(FSVALS_FR1);
					else				/* FLOOR.L.D */
						dtemp = floor(FSVALD_FR1);
					FDVALL_FR1 = (INT64)dtemp;
					break;

				case 0x0c:
					if (IS_SINGLE(op))	/* ROUND.W.S */
					{
						dtemp = FSVALS_FR1;
						if (dtemp < 0)
							dtemp = ceil(dtemp - 0.5);
						else
							dtemp = floor(dtemp + 0.5);
						FDVALW_FR1 = (INT32)dtemp;
					}
					else				/* ROUND.W.D */
					{
						dtemp = FSVALD_FR1;
						if (dtemp < 0)
							dtemp = ceil(dtemp - 0.5);
						else
							dtemp = floor(dtemp + 0.5);
						FDVALW_FR1 = (INT32)dtemp;
					}
					break;

				case 0x0d:
					if (IS_SINGLE(op))	/* TRUNC.W.S */
					{
						dtemp = FSVALS_FR1;
						if (dtemp < 0)
							dtemp = ceil(dtemp);
						else
							dtemp = floor(dtemp);
						FDVALW_FR1 = (INT32)dtemp;
					}
					else				/* TRUNC.W.D */
					{
						dtemp = FSVALD_FR1;
						if (dtemp < 0)
							dtemp = ceil(dtemp);
						else
							dtemp = floor(dtemp);
						FDVALW_FR1 = (INT32)dtemp;
					}
					break;

				case 0x0e:
					if (IS_SINGLE(op))	/* CEIL.W.S */
						dtemp = ceil(FSVALS_FR1);
					else				/* CEIL.W.D */
						dtemp = ceil(FSVALD_FR1);
					FDVALW_FR1 = (INT32)dtemp;
					break;

				case 0x0f:
					if (IS_SINGLE(op))	/* FLOOR.W.S */
						dtemp = floor(FSVALS_FR1);
					else				/* FLOOR.W.D */
						dtemp = floor(FSVALD_FR1);
					FDVALW_FR1 = (INT32)dtemp;
					break;

				case 0x11:	/* R5000 */
					if (GET_FCC((op >> 18) & 7) == ((op >> 16) & 1))
					{
						if (IS_SINGLE(op))	/* MOVT/F.S */
							FDVALS_FR1 = FSVALS_FR1;
						else				/* MOVT/F.D */
							FDVALD_FR1 = FSVALD_FR1;
					}
					break;

				case 0x12:	/* R5000 */
					if (RTVAL64 == 0)
					{
						if (IS_SINGLE(op))	/* MOVZ.S */
							FDVALS_FR1 = FSVALS_FR1;
						else				/* MOVZ.D */
							FDVALD_FR1 = FSVALD_FR1;
					}
					break;

				case 0x13:	/* R5000 */
					if (RTVAL64 != 0)
					{
						if (IS_SINGLE(op))	/* MOVN.S */
							FDVALS_FR1 = FSVALS_FR1;
						else				/* MOVN.D */
							FDVALD_FR1 = FSVALD_FR1;
					}
					break;

				case 0x15:	/* R5000 */
					if (IS_SINGLE(op))	/* RECIP.S */
						FDVALS_FR1 = 1.0f / FSVALS_FR1;
					else				/* RECIP.D */
						FDVALD_FR1 = 1.0 / FSVALD_FR1;
					break;

				case 0x16:	/* R5000 */
					if (IS_SINGLE(op))	/* RSQRT.S */
						FDVALS_FR1 = 1.0f / sqrt(FSVALS_FR1);
					else				/* RSQRT.D */
						FDVALD_FR1 = 1.0 / sqrt(FSVALD_FR1);
					break;

				case 0x20:
					if (IS_INTEGRAL(op))
					{
						if (IS_SINGLE(op))	/* CVT.S.W */
							FDVALS_FR1 = (INT32)FSVALW_FR1;
						else				/* CVT.S.L */
							FDVALS_FR1 = (INT64)FSVALL_FR1;
					}
					else					/* CVT.S.D */
						FDVALS_FR1 = FSVALD_FR1;
					break;

				case 0x21:
					if (IS_INTEGRAL(op))
					{
						if (IS_SINGLE(op))	/* CVT.D.W */
							FDVALD_FR1 = (INT32)FSVALW_FR1;
						else				/* CVT.D.L */
							FDVALD_FR1 = (INT64)FSVALL_FR1;
					}
					else					/* CVT.D.S */
						FDVALD_FR1 = FSVALS_FR1;
					break;

				case 0x24:
					if (IS_SINGLE(op))	/* CVT.W.S */
						FDVALW_FR1 = (INT32)FSVALS_FR1;
					else
						FDVALW_FR1 = (INT32)FSVALD_FR1;
					break;

				case 0x25:
					if (IS_SINGLE(op))	/* CVT.L.S */
						FDVALL_FR1 = (INT64)FSVALS_FR1;
					else				/* CVT.L.D */
						FDVALL_FR1 = (INT64)FSVALD_FR1;
					break;

				case 0x30:
				case 0x38:
					if (IS_SINGLE(op))	/* C.F.S */
						SET_FCC((op >> 8) & 7, 0);
					else				/* C.F.D */
						SET_FCC((op >> 8) & 7, 0);
					break;

				case 0x31:
				case 0x39:
					if (IS_SINGLE(op))	/* C.UN.S */
						SET_FCC((op >> 8) & 7, 0);
					else				/* C.UN.D */
						SET_FCC((op >> 8) & 7, 0);
					break;

				case 0x32:
				case 0x3a:
					if (IS_SINGLE(op))	/* C.EQ.S */
						SET_FCC((op >> 8) & 7, (FSVALS_FR1 == FTVALS_FR1));
					else				/* C.EQ.D */
						SET_FCC((op >> 8) & 7, (FSVALD_FR1 == FTVALD_FR1));
					break;

				case 0x33:
				case 0x3b:
					if (IS_SINGLE(op))	/* C.UEQ.S */
						SET_FCC((op >> 8) & 7, (FSVALS_FR1 == FTVALS_FR1));
					else				/* C.UEQ.D */
						SET_FCC((op >> 8) & 7, (FSVALD_FR1 == FTVALD_FR1));
					break;

				case 0x34:
				case 0x3c:
					if (IS_SINGLE(op))	/* C.OLT.S */
						SET_FCC((op >> 8) & 7, (FSVALS_FR1 < FTVALS_FR1));
					else				/* C.OLT.D */
						SET_FCC((op >> 8) & 7, (FSVALD_FR1 < FTVALD_FR1));
					break;

				case 0x35:
				case 0x3d:
					if (IS_SINGLE(op))	/* C.ULT.S */
						SET_FCC((op >> 8) & 7, (FSVALS_FR1 < FTVALS_FR1));
					else				/* C.ULT.D */
						SET_FCC((op >> 8) & 7, (FSVALD_FR1 < FTVALD_FR1));
					break;

				case 0x36:
				case 0x3e:
					if (IS_SINGLE(op))	/* C.OLE.S */
						SET_FCC((op >> 8) & 7, (FSVALS_FR1 <= FTVALS_FR1));
					else				/* C.OLE.D */
						SET_FCC((op >> 8) & 7, (FSVALD_FR1 <= FTVALD_FR1));
					break;

				case 0x37:
				case 0x3f:
					if (IS_SINGLE(op))	/* C.ULE.S */
						SET_FCC((op >> 8) & 7, (FSVALS_FR1 <= FTVALS_FR1));
					else				/* C.ULE.D */
						SET_FCC((op >> 8) & 7, (FSVALD_FR1 <= FTVALD_FR1));
					break;

				default:
					fprintf(stderr, "cop1 %X\n", op);
					break;
			}
			break;
	}
}



/***************************************************************************
    COP1X (FPU EXTRA) EXECUTION HANDLING
***************************************************************************/

INLINE void handle_cop1x_fr0(UINT32 op)
{
	UINT64 temp64;
	UINT32 temp;

	if (!(SR & SR_COP1))
		generate_exception(EXCEPTION_BADCOP, 1);

	switch (op & 0x3f)
	{
		case 0x00:		/* LWXC1 */
			if (RLONG(RSVAL32 + RTVAL32, &temp)) FDVALW_FR0 = temp;
			break;

		case 0x01:		/* LDXC1 */
			if (RDOUBLE(RSVAL32 + RTVAL32, &temp64)) FDVALL_FR0 = temp64;
			break;

		case 0x08:		/* SWXC1 */
			WLONG(RSVAL32 + RTVAL32, get_cop1_reg32(FDREG));
			break;

		case 0x09:		/* SDXC1 */
			WDOUBLE(RSVAL32 + RTVAL32, get_cop1_reg64(FDREG));
			break;

		case 0x0f:		/* PREFX */
			break;

		case 0x20:		/* MADD.S */
			FDVALS_FR0 = FSVALS_FR0 * FTVALS_FR0 + FRVALS_FR0;
			break;

		case 0x21:		/* MADD.D */
			FDVALD_FR0 = FSVALD_FR0 * FTVALD_FR0 + FRVALD_FR0;
			break;

		case 0x28:		/* MSUB.S */
			FDVALS_FR0 = FSVALS_FR0 * FTVALS_FR0 - FRVALS_FR0;
			break;

		case 0x29:		/* MSUB.D */
			FDVALD_FR0 = FSVALD_FR0 * FTVALD_FR0 - FRVALD_FR0;
			break;

		case 0x30:		/* NMADD.S */
			FDVALS_FR0 = -(FSVALS_FR0 * FTVALS_FR0 + FRVALS_FR0);
			break;

		case 0x31:		/* NMADD.D */
			FDVALD_FR0 = -(FSVALD_FR0 * FTVALD_FR0 + FRVALD_FR0);
			break;

		case 0x38:		/* NMSUB.S */
			FDVALS_FR0 = -(FSVALS_FR0 * FTVALS_FR0 - FRVALS_FR0);
			break;

		case 0x39:		/* NMSUB.D */
			FDVALD_FR0 = -(FSVALD_FR0 * FTVALD_FR0 - FRVALD_FR0);
			break;

		case 0x24:		/* MADD.W */
		case 0x25:		/* MADD.L */
		case 0x2c:		/* MSUB.W */
		case 0x2d:		/* MSUB.L */
		case 0x34:		/* NMADD.W */
		case 0x35:		/* NMADD.L */
		case 0x3c:		/* NMSUB.W */
		case 0x3d:		/* NMSUB.L */
		default:
			fprintf(stderr, "cop1x %X\n", op);
			break;
	}
}


INLINE void handle_cop1x_fr1(UINT32 op)
{
	UINT64 temp64;
	UINT32 temp;

	if (!(SR & SR_COP1))
		generate_exception(EXCEPTION_BADCOP, 1);

	switch (op & 0x3f)
	{
		case 0x00:		/* LWXC1 */
			if (RLONG(RSVAL32 + RTVAL32, &temp)) FDVALW_FR1 = temp;
			break;

		case 0x01:		/* LDXC1 */
			if (RDOUBLE(RSVAL32 + RTVAL32, &temp64)) FDVALL_FR1 = temp64;
			break;

		case 0x08:		/* SWXC1 */
			WLONG(RSVAL32 + RTVAL32, get_cop1_reg32(FDREG));
			break;

		case 0x09:		/* SDXC1 */
			WDOUBLE(RSVAL32 + RTVAL32, get_cop1_reg64(FDREG));
			break;

		case 0x0f:		/* PREFX */
			break;

		case 0x20:		/* MADD.S */
			FDVALS_FR1 = FSVALS_FR1 * FTVALS_FR1 + FRVALS_FR1;
			break;

		case 0x21:		/* MADD.D */
			FDVALD_FR1 = FSVALD_FR1 * FTVALD_FR1 + FRVALD_FR1;
			break;

		case 0x28:		/* MSUB.S */
			FDVALS_FR1 = FSVALS_FR1 * FTVALS_FR1 - FRVALS_FR1;
			break;

		case 0x29:		/* MSUB.D */
			FDVALD_FR1 = FSVALD_FR1 * FTVALD_FR1 - FRVALD_FR1;
			break;

		case 0x30:		/* NMADD.S */
			FDVALS_FR1 = -(FSVALS_FR1 * FTVALS_FR1 + FRVALS_FR1);
			break;

		case 0x31:		/* NMADD.D */
			FDVALD_FR1 = -(FSVALD_FR1 * FTVALD_FR1 + FRVALD_FR1);
			break;

		case 0x38:		/* NMSUB.S */
			FDVALS_FR1 = -(FSVALS_FR1 * FTVALS_FR1 - FRVALS_FR1);
			break;

		case 0x39:		/* NMSUB.D */
			FDVALD_FR1 = -(FSVALD_FR1 * FTVALD_FR1 - FRVALD_FR1);
			break;

		case 0x24:		/* MADD.W */
		case 0x25:		/* MADD.L */
		case 0x2c:		/* MSUB.W */
		case 0x2d:		/* MSUB.L */
		case 0x34:		/* NMADD.W */
		case 0x35:		/* NMADD.L */
		case 0x3c:		/* NMSUB.W */
		case 0x3d:		/* NMSUB.L */
		default:
			fprintf(stderr, "cop1x %X\n", op);
			break;
	}
}



/***************************************************************************
    COP2 (CUSTOM) EXECUTION HANDLING
***************************************************************************/

INLINE UINT64 get_cop2_reg(int idx)
{
	return mips3.cpr[2][idx];
}

INLINE void set_cop2_reg(int idx, UINT64 val)
{
	mips3.cpr[2][idx] = val;
}

INLINE UINT64 get_cop2_creg(int idx)
{
	return mips3.ccr[2][idx];
}

INLINE void set_cop2_creg(int idx, UINT64 val)
{
	mips3.ccr[2][idx] = val;
}

INLINE void handle_cop2(UINT32 op)
{
	if (!(SR & SR_COP2))
		generate_exception(EXCEPTION_BADCOP, 1);

	switch (RSREG)
	{
		case 0x00:	/* MFCz */		if (RTREG) RTVAL64 = (INT32)get_cop2_reg(RDREG);		break;
		case 0x01:	/* DMFCz */		if (RTREG) RTVAL64 = get_cop2_reg(RDREG);				break;
		case 0x02:	/* CFCz */		if (RTREG) RTVAL64 = (INT32)get_cop2_creg(RDREG);		break;
		case 0x04:	/* MTCz */		set_cop2_reg(RDREG, RTVAL32);							break;
		case 0x05:	/* DMTCz */		set_cop2_reg(RDREG, RTVAL64);							break;
		case 0x06:	/* CTCz */		set_cop2_creg(RDREG, RTVAL32);							break;
		case 0x08:	/* BC */
			switch (RTREG)
			{
				case 0x00:	/* BCzF */	if (!mips3.cf[2]) ADDPC(SIMMVAL);					break;
				case 0x01:	/* BCzF */	if (mips3.cf[2]) ADDPC(SIMMVAL);					break;
				case 0x02:	/* BCzFL */	invalid_instruction(op);							break;
				case 0x03:	/* BCzTL */	invalid_instruction(op);							break;
				default:	invalid_instruction(op);										break;
			}
			break;
		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13:
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x17:
		case 0x18:
		case 0x19:
		case 0x1a:
		case 0x1b:
		case 0x1c:
		case 0x1d:
		case 0x1e:
		case 0x1f:	/* COP */		invalid_instruction(op);								break;
		default:	invalid_instruction(op);												break;
	}
}



/***************************************************************************
    CORE EXECUTION LOOP
***************************************************************************/

int mips3_execute(int cycles)
{
	/* count cycles and interrupt cycles */
	mips3_icount = cycles;
	mips3_icount -= mips3.interrupt_cycles;
	mips3.interrupt_cycles = 0;

	/* update timers & such */
	update_cycle_counting();

	/* check for IRQs */
	check_irqs();

	/* core execution loop */
	do
	{
		UINT32 op;
		UINT64 temp64;
		UINT32 temp;

		/* see if we crossed a page boundary */
		if ((mips3.pc ^ mips3.ppc) & 0xfffff000)
			if (!update_pcbase())
				continue;

		/* debugging */
		mips3.ppc = mips3.pc;
		CALL_MAME_DEBUG;

		/* instruction fetch */
		op = ROPCODE(mips3.pcbase | (mips3.pc & 0xfff));

		/* adjust for next PC */
		if (mips3.nextpc != ~0)
		{
			mips3.pc = mips3.nextpc;
			mips3.nextpc = ~0;
		}
		else
			mips3.pc += 4;

		/* parse the instruction */
		switch (op >> 26)
		{
			case 0x00:	/* SPECIAL */
				switch (op & 63)
				{
					case 0x00:	/* SLL */		if (RDREG) RDVAL64 = (INT32)(RTVAL32 << SHIFT);					break;
					case 0x01:	/* MOVF - R5000*/if (RDREG && GET_FCC((op >> 18) & 7) == ((op >> 16) & 1)) RDVAL64 = RSVAL64;	break;
					case 0x02:	/* SRL */		if (RDREG) RDVAL64 = (INT32)(RTVAL32 >> SHIFT);					break;
					case 0x03:	/* SRA */		if (RDREG) RDVAL64 = (INT32)RTVAL32 >> SHIFT;					break;
					case 0x04:	/* SLLV */		if (RDREG) RDVAL64 = (INT32)(RTVAL32 << (RSVAL32 & 31));		break;
					case 0x06:	/* SRLV */		if (RDREG) RDVAL64 = (INT32)(RTVAL32 >> (RSVAL32 & 31));		break;
					case 0x07:	/* SRAV */		if (RDREG) RDVAL64 = (INT32)RTVAL32 >> (RSVAL32 & 31);			break;
					case 0x08:	/* JR */		SETPC(RSVAL32);													break;
					case 0x09:	/* JALR */		SETPCL(RSVAL32,RDREG);											break;
					case 0x0a:	/* MOVZ - R5000 */if (RTVAL64 == 0) { if (RDREG) RDVAL64 = RSVAL64; }			break;
					case 0x0b:	/* MOVN - R5000 */if (RTVAL64 != 0) { if (RDREG) RDVAL64 = RSVAL64; }			break;
					case 0x0c:	/* SYSCALL */	generate_exception(EXCEPTION_SYSCALL, 1);						break;
					case 0x0d:	/* BREAK */		generate_exception(EXCEPTION_BREAK, 1);							break;
					case 0x0f:	/* SYNC */		/* effective no-op */											break;
					case 0x10:	/* MFHI */		if (RDREG) RDVAL64 = HIVAL64;									break;
					case 0x11:	/* MTHI */		HIVAL64 = RSVAL64;												break;
					case 0x12:	/* MFLO */		if (RDREG) RDVAL64 = LOVAL64;									break;
					case 0x13:	/* MTLO */		LOVAL64 = RSVAL64;												break;
					case 0x14:	/* DSLLV */		if (RDREG) RDVAL64 = RTVAL64 << (RSVAL32 & 63);					break;
					case 0x16:	/* DSRLV */		if (RDREG) RDVAL64 = RTVAL64 >> (RSVAL32 & 63);					break;
					case 0x17:	/* DSRAV */		if (RDREG) RDVAL64 = (INT64)RTVAL64 >> (RSVAL32 & 63);			break;
					case 0x18:	/* MULT */
						temp64 = (INT64)(INT32)RSVAL32 * (INT64)(INT32)RTVAL32;
						LOVAL64 = (INT32)temp64;
						HIVAL64 = (INT32)(temp64 >> 32);
						mips3_icount -= 3;
						break;
					case 0x19:	/* MULTU */
						temp64 = (UINT64)RSVAL32 * (UINT64)RTVAL32;
						LOVAL64 = (INT32)temp64;
						HIVAL64 = (INT32)(temp64 >> 32);
						mips3_icount -= 3;
						break;
					case 0x1a:	/* DIV */
						if (RTVAL32)
						{
							LOVAL64 = (INT32)((INT32)RSVAL32 / (INT32)RTVAL32);
							HIVAL64 = (INT32)((INT32)RSVAL32 % (INT32)RTVAL32);
						}
						mips3_icount -= 35;
						break;
					case 0x1b:	/* DIVU */
						if (RTVAL32)
						{
							LOVAL64 = (INT32)(RSVAL32 / RTVAL32);
							HIVAL64 = (INT32)(RSVAL32 % RTVAL32);
						}
						mips3_icount -= 35;
						break;
					case 0x1c:	/* DMULT */
						temp64 = (INT64)RSVAL64 * (INT64)RTVAL64;
						LOVAL64 = temp64;
						HIVAL64 = (INT64)temp64 >> 63;
						mips3_icount -= 7;
						break;
					case 0x1d:	/* DMULTU */
						temp64 = (UINT64)RSVAL64 * (UINT64)RTVAL64;
						LOVAL64 = temp64;
						HIVAL64 = 0;
						mips3_icount -= 7;
						break;
					case 0x1e:	/* DDIV */
						if (RTVAL64)
						{
							LOVAL64 = (INT64)RSVAL64 / (INT64)RTVAL64;
							HIVAL64 = (INT64)RSVAL64 % (INT64)RTVAL64;
						}
						mips3_icount -= 67;
						break;
					case 0x1f:	/* DDIVU */
						if (RTVAL64)
						{
							LOVAL64 = RSVAL64 / RTVAL64;
							HIVAL64 = RSVAL64 % RTVAL64;
						}
						mips3_icount -= 67;
						break;
					case 0x20:	/* ADD */
						if (ENABLE_OVERFLOWS && RSVAL32 > ~RTVAL32) generate_exception(EXCEPTION_OVERFLOW, 1);
						else RDVAL64 = (INT32)(RSVAL32 + RTVAL32);
						break;
					case 0x21:	/* ADDU */		if (RDREG) RDVAL64 = (INT32)(RSVAL32 + RTVAL32);				break;
					case 0x22:	/* SUB */
						if (ENABLE_OVERFLOWS && RSVAL32 < RTVAL32) generate_exception(EXCEPTION_OVERFLOW, 1);
						else RDVAL64 = (INT32)(RSVAL32 - RTVAL32);
						break;
					case 0x23:	/* SUBU */		if (RDREG) RDVAL64 = (INT32)(RSVAL32 - RTVAL32);				break;
					case 0x24:	/* AND */		if (RDREG) RDVAL64 = RSVAL64 & RTVAL64;							break;
					case 0x25:	/* OR */		if (RDREG) RDVAL64 = RSVAL64 | RTVAL64;							break;
					case 0x26:	/* XOR */		if (RDREG) RDVAL64 = RSVAL64 ^ RTVAL64;							break;
					case 0x27:	/* NOR */		if (RDREG) RDVAL64 = ~(RSVAL64 | RTVAL64);						break;
					case 0x2a:	/* SLT */		if (RDREG) RDVAL64 = (INT64)RSVAL64 < (INT64)RTVAL64;			break;
					case 0x2b:	/* SLTU */		if (RDREG) RDVAL64 = (UINT64)RSVAL64 < (UINT64)RTVAL64;			break;
					case 0x2c:	/* DADD */
						if (ENABLE_OVERFLOWS && RSVAL64 > ~RTVAL64) generate_exception(EXCEPTION_OVERFLOW, 1);
						else RDVAL64 = RSVAL64 + RTVAL64;
						break;
					case 0x2d:	/* DADDU */		if (RDREG) RDVAL64 = RSVAL64 + RTVAL64;							break;
					case 0x2e:	/* DSUB */
						if (ENABLE_OVERFLOWS && RSVAL64 < RTVAL64) generate_exception(EXCEPTION_OVERFLOW, 1);
						else RDVAL64 = RSVAL64 - RTVAL64;
						break;
					case 0x2f:	/* DSUBU */		if (RDREG) RDVAL64 = RSVAL64 - RTVAL64;							break;
					case 0x30:	/* TGE */		if ((INT64)RSVAL64 >= (INT64)RTVAL64) generate_exception(EXCEPTION_TRAP, 1); break;
					case 0x31:	/* TGEU */		if (RSVAL64 >= RTVAL64) generate_exception(EXCEPTION_TRAP, 1);	break;
					case 0x32:	/* TLT */		if ((INT64)RSVAL64 < (INT64)RTVAL64) generate_exception(EXCEPTION_TRAP, 1); break;
					case 0x33:	/* TLTU */		if (RSVAL64 < RTVAL64) generate_exception(EXCEPTION_TRAP, 1);	break;
					case 0x34:	/* TEQ */		if (RSVAL64 == RTVAL64) generate_exception(EXCEPTION_TRAP, 1);	break;
					case 0x36:	/* TNE */		if (RSVAL64 != RTVAL64) generate_exception(EXCEPTION_TRAP, 1);	break;
					case 0x38:	/* DSLL */		if (RDREG) RDVAL64 = RTVAL64 << SHIFT;							break;
					case 0x3a:	/* DSRL */		if (RDREG) RDVAL64 = RTVAL64 >> SHIFT;							break;
					case 0x3b:	/* DSRA */		if (RDREG) RDVAL64 = (INT64)RTVAL64 >> SHIFT;					break;
					case 0x3c:	/* DSLL32 */	if (RDREG) RDVAL64 = RTVAL64 << (SHIFT + 32);					break;
					case 0x3e:	/* DSRL32 */	if (RDREG) RDVAL64 = RTVAL64 >> (SHIFT + 32);					break;
					case 0x3f:	/* DSRA32 */	if (RDREG) RDVAL64 = (INT64)RTVAL64 >> (SHIFT + 32);			break;
					default:	/* ??? */		invalid_instruction(op);										break;
				}
				break;

			case 0x01:	/* REGIMM */
				switch (RTREG)
				{
					case 0x00:	/* BLTZ */		if ((INT64)RSVAL64 < 0) ADDPC(SIMMVAL);							break;
					case 0x01:	/* BGEZ */		if ((INT64)RSVAL64 >= 0) ADDPC(SIMMVAL);						break;
					case 0x02:	/* BLTZL */		if ((INT64)RSVAL64 < 0) ADDPC(SIMMVAL);	else mips3.pc += 4;		break;
					case 0x03:	/* BGEZL */		if ((INT64)RSVAL64 >= 0) ADDPC(SIMMVAL); else mips3.pc += 4; 	break;
					case 0x08:	/* TGEI */		if ((INT64)RSVAL64 >= SIMMVAL) generate_exception(EXCEPTION_TRAP, 1);	break;
					case 0x09:	/* TGEIU */		if (RSVAL64 >= SIMMVAL) generate_exception(EXCEPTION_TRAP, 1);	break;
					case 0x0a:	/* TLTI */		if ((INT64)RSVAL64 < SIMMVAL) generate_exception(EXCEPTION_TRAP, 1);	break;
					case 0x0b:	/* TLTIU */		if (RSVAL64 >= SIMMVAL) generate_exception(EXCEPTION_TRAP, 1);	break;
					case 0x0c:	/* TEQI */		if (RSVAL64 == SIMMVAL) generate_exception(EXCEPTION_TRAP, 1);	break;
					case 0x0e:	/* TNEI */		if (RSVAL64 != SIMMVAL) generate_exception(EXCEPTION_TRAP, 1);	break;
					case 0x10:	/* BLTZAL */	if ((INT64)RSVAL64 < 0) ADDPCL(SIMMVAL,31);						break;
					case 0x11:	/* BGEZAL */	if ((INT64)RSVAL64 >= 0) ADDPCL(SIMMVAL,31);					break;
					case 0x12:	/* BLTZALL */	if ((INT64)RSVAL64 < 0) ADDPCL(SIMMVAL,31) else mips3.pc += 4;	break;
					case 0x13:	/* BGEZALL */	if ((INT64)RSVAL64 >= 0) ADDPCL(SIMMVAL,31) else mips3.pc += 4;	break;
					default:	/* ??? */		invalid_instruction(op);										break;
				}
				break;

			case 0x02:	/* J */			ABSPC(LIMMVAL);															break;
			case 0x03:	/* JAL */		ABSPCL(LIMMVAL,31);														break;
			case 0x04:	/* BEQ */		if (RSVAL64 == RTVAL64) ADDPC(SIMMVAL);									break;
			case 0x05:	/* BNE */		if (RSVAL64 != RTVAL64) ADDPC(SIMMVAL);									break;
			case 0x06:	/* BLEZ */		if ((INT64)RSVAL64 <= 0) ADDPC(SIMMVAL);								break;
			case 0x07:	/* BGTZ */		if ((INT64)RSVAL64 > 0) ADDPC(SIMMVAL);									break;
			case 0x08:	/* ADDI */
				if (ENABLE_OVERFLOWS && RSVAL32 > ~SIMMVAL) generate_exception(EXCEPTION_OVERFLOW, 1);
				else if (RTREG) RTVAL64 = (INT32)(RSVAL32 + SIMMVAL);
				break;
			case 0x09:	/* ADDIU */		if (RTREG) RTVAL64 = (INT32)(RSVAL32 + SIMMVAL);						break;
			case 0x0a:	/* SLTI */		if (RTREG) RTVAL64 = (INT64)RSVAL64 < (INT64)SIMMVAL;					break;
			case 0x0b:	/* SLTIU */		if (RTREG) RTVAL64 = (UINT64)RSVAL64 < (UINT64)SIMMVAL;					break;
			case 0x0c:	/* ANDI */		if (RTREG) RTVAL64 = RSVAL64 & UIMMVAL;									break;
			case 0x0d:	/* ORI */		if (RTREG) RTVAL64 = RSVAL64 | UIMMVAL;									break;
			case 0x0e:	/* XORI */		if (RTREG) RTVAL64 = RSVAL64 ^ UIMMVAL;									break;
			case 0x0f:	/* LUI */		if (RTREG) RTVAL64 = (INT32)(UIMMVAL << 16);							break;
			case 0x10:	/* COP0 */		handle_cop0(op);														break;
			case 0x11:	/* COP1 */		if (IS_FR0) handle_cop1_fr0(op); else handle_cop1_fr1(op);				break;
			case 0x12:	/* COP2 */		handle_cop2(op);														break;
			case 0x13:	/* COP1X - R5000 */if (IS_FR0) handle_cop1x_fr0(op); else handle_cop1x_fr1(op);			break;
			case 0x14:	/* BEQL */		if (RSVAL64 == RTVAL64) ADDPC(SIMMVAL); else mips3.pc += 4;				break;
			case 0x15:	/* BNEL */		if (RSVAL64 != RTVAL64) ADDPC(SIMMVAL);	else mips3.pc += 4;				break;
			case 0x16:	/* BLEZL */		if ((INT64)RSVAL64 <= 0) ADDPC(SIMMVAL); else mips3.pc += 4;			break;
			case 0x17:	/* BGTZL */		if ((INT64)RSVAL64 > 0) ADDPC(SIMMVAL); else mips3.pc += 4;				break;
			case 0x18:	/* DADDI */
				if (ENABLE_OVERFLOWS && RSVAL64 > ~SIMMVAL) generate_exception(EXCEPTION_OVERFLOW, 1);
				else if (RTREG) RTVAL64 = RSVAL64 + (INT64)SIMMVAL;
				break;
			case 0x19:	/* DADDIU */	if (RTREG) RTVAL64 = RSVAL64 + (UINT64)SIMMVAL;							break;
			case 0x1a:	/* LDL */		(*mips3.ldl)(op);														break;
			case 0x1b:	/* LDR */		(*mips3.ldr)(op);														break;
			case 0x1c:	/* IDT-specific opcodes: mad/madu/mul on R4640/4650, msub on RC32364 */
				switch (op & 0x1f)
				{
					case 2: /* MUL */
						RDVAL64 = (INT32)((INT32)RSVAL32 * (INT32)RTVAL32);
						mips3_icount -= 3;
						break;
		 			default: invalid_instruction(op);
				}
				break;
			case 0x20:	/* LB */		if (RBYTE(SIMMVAL+RSVAL32, &temp) && RTREG) RTVAL64 = (INT8)temp;		break;
			case 0x21:	/* LH */		if (RWORD(SIMMVAL+RSVAL32, &temp) && RTREG) RTVAL64 = (INT16)temp;		break;
			case 0x22:	/* LWL */		(*mips3.lwl)(op);														break;
			case 0x23:	/* LW */		if (RLONG(SIMMVAL+RSVAL32, &temp) && RTREG) RTVAL64 = (INT32)temp;		break;
			case 0x24:	/* LBU */		if (RBYTE(SIMMVAL+RSVAL32, &temp) && RTREG) RTVAL64 = (UINT8)temp;		break;
			case 0x25:	/* LHU */		if (RWORD(SIMMVAL+RSVAL32, &temp) && RTREG) RTVAL64 = (UINT16)temp;		break;
			case 0x26:	/* LWR */		(*mips3.lwr)(op);														break;
			case 0x27:	/* LWU */		if (RLONG(SIMMVAL+RSVAL32, &temp) && RTREG) RTVAL64 = (UINT32)temp;		break;
			case 0x28:	/* SB */		WBYTE(SIMMVAL+RSVAL32, RTVAL32);										break;
			case 0x29:	/* SH */		WWORD(SIMMVAL+RSVAL32, RTVAL32); 										break;
			case 0x2a:	/* SWL */		(*mips3.swl)(op);														break;
			case 0x2b:	/* SW */		WLONG(SIMMVAL+RSVAL32, RTVAL32);										break;
			case 0x2c:	/* SDL */		(*mips3.sdl)(op);														break;
			case 0x2d:	/* SDR */		(*mips3.sdr)(op);														break;
			case 0x2e:	/* SWR */		(*mips3.swr)(op);														break;
			case 0x2f:	/* CACHE */		/* effective no-op */													break;
			case 0x30:	/* LL */		if (RLONG(SIMMVAL+RSVAL32, &temp) && RTREG) RTVAL64 = (UINT32)temp; mips3.ll_value = RTVAL32;		break;
			case 0x31:	/* LWC1 */		if (RLONG(SIMMVAL+RSVAL32, &temp)) set_cop1_reg32(RTREG, temp);			break;
			case 0x32:	/* LWC2 */		if (RLONG(SIMMVAL+RSVAL32, &temp)) set_cop2_reg(RTREG, temp);			break;
			case 0x33:	/* PREF */		/* effective no-op */													break;
			case 0x34:	/* LLD */		if (RDOUBLE(SIMMVAL+RSVAL32, &temp64) && RTREG) RTVAL64 = temp64; mips3.lld_value = temp64;		break;
			case 0x35:	/* LDC1 */		if (RDOUBLE(SIMMVAL+RSVAL32, &temp64)) set_cop1_reg64(RTREG, temp64);		break;
			case 0x36:	/* LDC2 */		if (RDOUBLE(SIMMVAL+RSVAL32, &temp64)) set_cop2_reg(RTREG, temp64);		break;
			case 0x37:	/* LD */		if (RDOUBLE(SIMMVAL+RSVAL32, &temp64) && RTREG) RTVAL64 = temp64;		break;
			case 0x38:	/* SC */		if (RLONG(SIMMVAL+RSVAL32, &temp) && RTREG)
								{
									if (temp == mips3.ll_value)
									{
										WLONG(SIMMVAL+RSVAL32, RTVAL32);
										RTVAL64 = (UINT32)1;
									}
									else
									{
										RTVAL64 = (UINT32)0;
									}
								}
								break;
			case 0x39:	/* SWC1 */		WLONG(SIMMVAL+RSVAL32, get_cop1_reg32(RTREG));							break;
			case 0x3a:	/* SWC2 */		WLONG(SIMMVAL+RSVAL32, get_cop2_reg(RTREG));							break;
			case 0x3b:	/* SWC3 */		invalid_instruction(op);												break;
			case 0x3c:	/* SCD */		if (RDOUBLE(SIMMVAL+RSVAL32, &temp64) && RTREG)
								{
									if (temp64 == mips3.lld_value)
									{
										WDOUBLE(SIMMVAL+RSVAL32, RTVAL64);
										RTVAL64 = 1;
									}
									else
									{
										RTVAL64 = 0;
									}
								}
								break;
			case 0x3d:	/* SDC1 */		WDOUBLE(SIMMVAL+RSVAL32, get_cop1_reg64(RTREG));							break;
			case 0x3e:	/* SDC2 */		WDOUBLE(SIMMVAL+RSVAL32, get_cop2_reg(RTREG));							break;
			case 0x3f:	/* SD */		WDOUBLE(SIMMVAL+RSVAL32, RTVAL64);										break;
			default:	/* ??? */		invalid_instruction(op);												break;
		}
		mips3_icount--;

	} while (mips3_icount > 0 || mips3.nextpc != ~0);

	mips3_icount -= mips3.interrupt_cycles;
	mips3.interrupt_cycles = 0;
	return cycles - mips3_icount;
}



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



/***************************************************************************
    DOUBLEWORD READS/WRITES
***************************************************************************/

static UINT64 read_qword_32be(offs_t offset)
{
	UINT64 result = (UINT64)program_read_dword_32be(offset) << 32;
	return result | program_read_dword_32be(offset + 4);
}

static UINT64 read_qword_32le(offs_t offset)
{
	UINT64 result = program_read_dword_32le(offset);
	return result | ((UINT64)program_read_dword_32le(offset + 4) << 32);
}

static void write_qword_32be(offs_t offset, UINT64 data)
{
	program_write_dword_32be(offset, data >> 32);
	program_write_dword_32be(offset + 4, data);
}

static void write_qword_32le(offs_t offset, UINT64 data)
{
	program_write_dword_32le(offset, data);
	program_write_dword_32le(offset + 4, data >> 32);
}



/***************************************************************************
    COMPLEX OPCODE IMPLEMENTATIONS
***************************************************************************/

static void lwl_be(UINT32 op)
{
	offs_t offs = SIMMVAL + RSVAL32;
	UINT32 temp;
	if (RLONG(offs & ~3, &temp) && RTREG)
	{
		if (!(offs & 3)) RTVAL64 = (INT32)temp;
		else
		{
			int shift = 8 * (offs & 3);
			RTVAL64 = (INT32)((RTVAL32 & (0x00ffffff >> (24 - shift))) | (temp << shift));
		}
	}
}

static void lwr_be(UINT32 op)
{
	offs_t offs = SIMMVAL + RSVAL32;
	UINT32 temp;
	if (RLONG(offs & ~3, &temp) && RTREG)
	{
		if ((offs & 3) == 3) RTVAL64 = (INT32)temp;
		else
		{
			int shift = 8 * (offs & 3);
			RTVAL64 = (INT32)((RTVAL32 & (0xffffff00 << shift)) | (temp >> (24 - shift)));
		}
	}
}

static void ldl_be(UINT32 op)
{
	offs_t offs = SIMMVAL + RSVAL32;
	UINT64 temp;
	if (RDOUBLE(offs & ~7, &temp) && RTREG)
	{
		if (!(offs & 7)) RTVAL64 = temp;
		else
		{
			UINT64 mask = ~((UINT64)0xff << 56);
			int shift = 8 * (offs & 7);
			RTVAL64 = (RTVAL64 & (mask >> (56 - shift))) | (temp << shift);
		}
	}
}

static void ldr_be(UINT32 op)
{
	offs_t offs = SIMMVAL + RSVAL32;
	UINT64 temp;
	if (RDOUBLE(offs & ~7, &temp) && RTREG)
	{
		if ((offs & 7) == 7) RTVAL64 = temp;
		else
		{
			UINT64 mask = ~((UINT64)0xff);
			int shift = 8 * (offs & 7);
			RTVAL64 = (RTVAL64 & (mask << shift)) | (temp >> (56 - shift));
		}
	}
}

static void swl_be(UINT32 op)
{
	offs_t offs = SIMMVAL + RSVAL32;
	if (!(offs & 3)) WLONG(offs, RTVAL32);
	else
	{
		UINT32 temp;
		int shift = 8 * (offs & 3);
		if (RLONG(offs & ~3, &temp))
			WLONG(offs & ~3, (temp & (0xffffff00 << (24 - shift))) | (RTVAL32 >> shift));
	}
}


static void swr_be(UINT32 op)
{
	offs_t offs = SIMMVAL + RSVAL32;
	if ((offs & 3) == 3) WLONG(offs & ~3, RTVAL32);
	else
	{
		UINT32 temp;
		int shift = 8 * (offs & 3);
		if (RLONG(offs & ~3, &temp))
			WLONG(offs & ~3, (temp & (0x00ffffff >> shift)) | (RTVAL32 << (24 - shift)));
	}
}

static void sdl_be(UINT32 op)
{
	offs_t offs = SIMMVAL + RSVAL32;
	if (!(offs & 7)) WDOUBLE(offs, RTVAL64);
	else
	{
		UINT64 temp;
		UINT64 mask = ~((UINT64)0xff);
		int shift = 8 * (offs & 7);
		if (RDOUBLE(offs & ~7, &temp))
			WDOUBLE(offs & ~7, (temp & (mask << (56 - shift))) | (RTVAL64 >> shift));
	}
}

static void sdr_be(UINT32 op)
{
	offs_t offs = SIMMVAL + RSVAL32;
	if ((offs & 7) == 7) WDOUBLE(offs & ~7, RTVAL64);
	else
	{
		UINT64 temp;
		UINT64 mask = ~((UINT64)0xff << 56);
		int shift = 8 * (offs & 7);
		if (RDOUBLE(offs & ~7, &temp))
			WDOUBLE(offs & ~7, (temp & (mask >> shift)) | (RTVAL64 << (56 - shift)));
	}
}



static void lwl_le(UINT32 op)
{
	offs_t offs = SIMMVAL + RSVAL32;
	UINT32 temp;
	if (RLONG(offs & ~3, &temp) && RTREG)
	{
		if ((offs & 3) == 3) RTVAL64 = (INT32)temp;
		else
		{
			int shift = 8 * (offs & 3);
			RTVAL64 = (INT32)((RTVAL32 & (0x00ffffff >> shift)) | (temp << (24 - shift)));
		}
	}
}

static void lwr_le(UINT32 op)
{
	offs_t offs = SIMMVAL + RSVAL32;
	UINT32 temp;
	if (RLONG(offs & ~3, &temp) && RTREG)
	{
		if (!(offs & 3)) RTVAL64 = (INT32)temp;
		else
		{
			int shift = 8 * (offs & 3);
			RTVAL64 = (INT32)((RTVAL32 & (0xffffff00 << (24 - shift))) | (temp >> shift));
		}
	}
}

static void ldl_le(UINT32 op)
{
	offs_t offs = SIMMVAL + RSVAL32;
	UINT64 temp;
	if (RDOUBLE(offs & ~7, &temp) && RTREG)
	{
		if ((offs & 7) == 7) RTVAL64 = temp;
		else
		{
			UINT64 mask = ~((UINT64)0xff << 56);
			int shift = 8 * (offs & 7);
			RTVAL64 = (RTVAL64 & (mask >> shift)) | (temp << (56 - shift));
		}
	}
}

static void ldr_le(UINT32 op)
{
	offs_t offs = SIMMVAL + RSVAL32;
	UINT64 temp;
	if (RDOUBLE(offs & ~7, &temp) && RTREG)
	{
		if (!(offs & 7)) RTVAL64 = temp;
		else
		{
			UINT64 mask = ~((UINT64)0xff);
			int shift = 8 * (offs & 7);
			RTVAL64 = (RTVAL64 & (mask << (56 - shift))) | (temp >> shift);
		}
	}
}

static void swl_le(UINT32 op)
{
	offs_t offs = SIMMVAL + RSVAL32;
	if ((offs & 3) == 3) WLONG(offs & ~3, RTVAL32);
	else
	{
		UINT32 temp;
		int shift = 8 * (offs & 3);
		if (RLONG(offs & ~3, &temp))
			WLONG(offs & ~3, (temp & (0xffffff00 << shift)) | (RTVAL32 >> (24 - shift)));
	}
}

static void swr_le(UINT32 op)
{
	offs_t offs = SIMMVAL + RSVAL32;
	if (!(offs & 3)) WLONG(offs, RTVAL32);
	else
	{
		UINT32 temp;
		int shift = 8 * (offs & 3);
		if (RLONG(offs & ~3, &temp))
			WLONG(offs & ~3, (temp & (0x00ffffff >> (24 - shift))) | (RTVAL32 << shift));
	}
}

static void sdl_le(UINT32 op)
{
	offs_t offs = SIMMVAL + RSVAL32;
	if ((offs & 7) == 7) WDOUBLE(offs & ~7, RTVAL64);
	else
	{
		UINT64 temp;
		UINT64 mask = ~((UINT64)0xff);
		int shift = 8 * (offs & 7);
		if (RDOUBLE(offs & ~7, &temp))
			WDOUBLE(offs & ~7, (temp & (mask << shift)) | (RTVAL64 >> (56 - shift)));
	}
}

static void sdr_le(UINT32 op)
{
	offs_t offs = SIMMVAL + RSVAL32;
	if (!(offs & 7)) WDOUBLE(offs, RTVAL64);
	else
	{
		UINT64 temp;
		UINT64 mask = ~((UINT64)0xff << 56);
		int shift = 8 * (offs & 7);
		if (RDOUBLE(offs & ~7, &temp))
			WDOUBLE(offs & ~7, (temp & (mask >> (56 - shift))) | (RTVAL64 << shift));
	}
}



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
	}
}



/**************************************************************************
 * Generic get_info
 **************************************************************************/

void mips3_get_info(UINT32 state, cpuinfo *info)
{
	UINT64 temp64;

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

		case CPUINFO_INT_PREVIOUSPC:					info->i = mips3.ppc;					break;

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
		case CPUINFO_INT_REGISTER + MIPS3_WIRED:		info->i = get_cop0_reg(COP0_Wired);		break;
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
		case CPUINFO_STR_REGISTER + MIPS3_COUNT: 		temp64 = ((activecpu_gettotalcycles64() - mips3.count_zero_time) / 2); sprintf(info->s, "Count:%08X%08X", (UINT32)(temp64 >> 32), (UINT32)temp64); break;
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

