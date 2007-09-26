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
#include "cpuintrf.h"
#include "debugger.h"
#include "mips3com.h"

#ifdef PTR64
#include "cpu/x64drc.h"
#include "mips3fe.h"
#else
#include "cpu/x86drc.h"
#endif



/***************************************************************************
    DEBUGGING
***************************************************************************/

#define LOG_CODE				(0)

#ifdef MAME_DEBUG
#define COMPARE_AGAINST_C		(0)
#else
#define COMPARE_AGAINST_C		(0)
#endif



/***************************************************************************
    CONFIGURATION
***************************************************************************/

#define MAX_INSTRUCTIONS		512
#define CACHE_SIZE				(32 * 1024 * 1024)



/***************************************************************************
    MACROS
***************************************************************************/

#define SR						mips3.core->cpr[0][COP0_Status]
#define IS_FR0					(!(SR & SR_FR))
#define IS_FR1					(SR & SR_FR)



/***************************************************************************
    STRUCTURES & TYPEDEFS
***************************************************************************/

/* code logging info */
typedef struct _code_log_entry code_log_entry;
struct _code_log_entry
{
	UINT32		pc;
	UINT32		op;
	x86code *	base;
	const char *string;
};


/* fast RAM info */
typedef struct _fast_ram_info fast_ram_info;
struct _fast_ram_info
{
	offs_t		start;
	offs_t		end;
	UINT8		readonly;
	void *		base;
};


/* hotspot info */
typedef struct _hotspot_info hotspot_info;
struct _hotspot_info
{
	offs_t		pc;
	UINT32		opcode;
	UINT32		cycles;
};


/* opaque drc-specific info */
typedef struct _mips3drc_data mips3drc_data;


/* MIPS3 registers */
typedef struct _mips3_regs mips3_regs;
struct _mips3_regs
{
	/* core state */
	UINT8 *			cache;						/* base of the cache */
	mips3_state *	core;						/* pointer to the core MIPS3 state */
	drc_core *		drc;						/* pointer to the DRC core */
	mips3drc_data *	drcdata;					/* pointer to the DRC-specific data */
	UINT32			drcoptions;					/* configurable DRC options */

	/* internal stuff */
	UINT32			nextpc;
	UINT8			cache_dirty;

	/* fast RAM */
	UINT32			fastram_select;
	fast_ram_info	fastram[MIPS3_MAX_FASTRAM];

	/* hotspots */
	UINT32			hotspot_select;
	hotspot_info	hotspot[MIPS3_MAX_HOTSPOTS];

#if COMPARE_AGAINST_C
	/* memory state */
	UINT8			access_size;
	UINT8			access_type;
	offs_t			access_addr;
	UINT64			access_data;
	UINT64			access_mask;
	memory_handlers	orighandler;
#endif
};



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static void mips3drc_init(void);
static void mips3drc_exit(void);

#if COMPARE_AGAINST_C
static void execute_c_version(void);
static void compare_c_version(void);
static void mips3c_init(mips3_flavor flavor, int bigendian, int index, int clock, const struct mips3_config *config, int (*irqcallback)(int));
static void mips3c_reset(void);
#endif



/***************************************************************************
    PRIVATE GLOBAL VARIABLES
***************************************************************************/

static mips3_regs mips3;



/***************************************************************************
    CORE CALLBACKS
***************************************************************************/

/*-------------------------------------------------
    mips3_init - initialize the processor
-------------------------------------------------*/

static void mips3_init(mips3_state *mips, mips3_flavor flavor, int bigendian, int index, int clock, const struct mips3_config *config, int (*irqcallback)(int))
{
	/* allocate a cache and memory for the core data in a single block */
	mips3.core = osd_alloc_executable(CACHE_SIZE + sizeof(*mips3.core));
	if (mips3.core == NULL)
		fatalerror("Unable to allocate cache of size %d\n", CACHE_SIZE);
	mips3.cache = (UINT8 *)(mips3.core + 1);

	/* initialize the core */
	mips3com_init(mips3.core, flavor, bigendian, index, clock, config, irqcallback);
#if COMPARE_AGAINST_C
	mips3c_init(flavor, bigendian, index, clock, config, irqcallback);
#endif

	/* initialize the DRC to use the cache */
	mips3drc_init();
}


/*-------------------------------------------------
    mips3_reset - reset the processor
-------------------------------------------------*/

static void mips3_reset(void)
{
	/* reset the common code and flush the cache */
	mips3com_reset(mips3.core);
	drc_cache_reset(mips3.drc);

#if COMPARE_AGAINST_C
	mips3c_reset();
#endif
}


/*-------------------------------------------------
    mips3_execute - execute the CPU for the
    specified number of cycles
-------------------------------------------------*/

static int mips3_execute(int cycles)
{
	/* reset the cache if dirty */
	if (mips3.cache_dirty)
		drc_cache_reset(mips3.drc);
	mips3.cache_dirty = FALSE;

	/* update the cycle timing */
	mips3com_update_cycle_counting(mips3.core);

	/* execute */
	mips3.core->icount = cycles;
	drc_execute(mips3.drc);
	return cycles - mips3.core->icount;
}


/*-------------------------------------------------
    mips3_exit - cleanup from execution
-------------------------------------------------*/

static void mips3_exit(void)
{
	/* clean up the DRC */
	mips3drc_exit();
	drc_exit(mips3.drc);

	/* free the cache */
	osd_free_executable(mips3.core, CACHE_SIZE + sizeof(*mips3.core));
}


/*-------------------------------------------------
    mips3_get_context - return a copy of the
    current context
-------------------------------------------------*/

static void mips3_get_context(void *dst)
{
	if (dst != NULL)
		*(mips3_regs *)dst = mips3;
}


/*-------------------------------------------------
    mips3_set_context - copy the current context
    into the global state
-------------------------------------------------*/

static void mips3_set_context(void *src)
{
	if (src != NULL)
		mips3 = *(mips3_regs *)src;
}


/*-------------------------------------------------
    mips3_translate - perform virtual-to-physical
    address translation
-------------------------------------------------*/

static int mips3_translate(int space, offs_t *address)
{
	return mips3com_translate_address(mips3.core, space, address);
}


/*-------------------------------------------------
    mips3_dasm - disassemble an instruction
-------------------------------------------------*/

#ifdef MAME_DEBUG
static offs_t mips3_dasm(char *buffer, offs_t pc, const UINT8 *oprom, const UINT8 *opram)
{
	return mips3com_dasm(mips3.core, buffer, pc, oprom, opram);
}
#endif /* MAME_DEBUG */




/***************************************************************************
    CODE LOGGING
***************************************************************************/

#if LOG_CODE

static code_log_entry code_log_buffer[MAX_INSTRUCTIONS*2];
static int code_log_index;
static FILE *logfile;


/*-------------------------------------------------
    open_logfile - open the log file if it is
    not already opened
-------------------------------------------------*/

INLINE int open_logfile(void)
{
	if (logfile == NULL)
		logfile = fopen("mips3drc.asm", "w");
	return (logfile != NULL);
}


/*-------------------------------------------------
    code_log_reset - reset the logging index
-------------------------------------------------*/

INLINE void code_log_reset(void)
{
	code_log_index = 0;
}


/*-------------------------------------------------
    code_log_add_entry - add an entry to the log
-------------------------------------------------*/

INLINE void code_log_add_entry(x86code *base, UINT32 pc, UINT32 op)
{
	code_log_buffer[code_log_index].base = base;
	code_log_buffer[code_log_index].pc = pc;
	code_log_buffer[code_log_index].op = op;
	code_log_buffer[code_log_index].string = NULL;
	code_log_index++;
}


/*-------------------------------------------------
    code_log_add_entry_string - add a string
    entry to the log
-------------------------------------------------*/

INLINE void code_log_add_entry_string(x86code *base, const char *string, UINT32 param)
{
	code_log_buffer[code_log_index].base = base;
	code_log_buffer[code_log_index].string = string;
	code_log_buffer[code_log_index].op = param;
	code_log_buffer[code_log_index].pc = 0;
	code_log_index++;
}


/*-------------------------------------------------
    code_log - actually log some code
-------------------------------------------------*/

static void print_log_entry(code_log_entry *entry, x86code *cur, const char *x86buffer)
{
	extern unsigned dasmmips3(char *buffer, unsigned pc, UINT32 op);

	/* if we have a match and a string, print it */
	if (entry->string != NULL)
	{
		fprintf(logfile, "%p: %-50s ", cur, x86buffer);
		fprintf(logfile, entry->string, entry->op);
		fprintf(logfile, "\n");
	}

	/* otherwise, output with the original instruction to the right */
	else
	{
		char buffer2[100];
		dasmmips3(buffer2, entry->pc, entry->op);
		fprintf(logfile, "%p: %-50s %08X: %s\n", cur, x86buffer, entry->pc, buffer2);
	}
}

static void code_log(const char *label, x86code *start, x86code *stop)
{
	extern int i386_dasm_one(char *buffer, UINT32 eip, const UINT8 *oprom, int mode);
	x86code *cur = start;
	int data_remaining = 0;

	/* open the file, creating it if necessary */
	if (!open_logfile())
		return;
	fprintf(logfile, "\n%s\n", label);

	/* loop from the start until the cache top */
	while (cur < stop)
	{
		char buffer[100];
		int op, lastop;
		int bytes;

		/* skip filler opcodes */
		if (*cur == 0xcc)
		{
			cur++;
			continue;
		}

		/* disassemble this instruction */
#ifdef PTR64
		/* look for special signatures and output as data */
		if (cur[0] == 0x90 && cur[1] == 0xcc && cur[2] == 0x90 && cur[3] == 0xcc &&
			cur[4] == 0x90 && cur[5] == 0xcc && cur[6] == 0x90 && cur[7] == 0xcc)
			data_remaining = 2;

		/* handle data cases first */
		if (data_remaining > 0)
		{
			UINT64 data = *(UINT64 *)cur;
			data_remaining--;
			sprintf(buffer, "dq      %08X%08X", (UINT32)(data >> 32), (UINT32)data);
			bytes = 8;
		}
		else
			bytes = i386_dasm_one(buffer, (UINT32)(FPTR)cur, cur, 64) & DASMFLAG_LENGTHMASK;
#else
		bytes = i386_dasm_one(buffer, (UINT32)cur, cur, 32) & DASMFLAG_LENGTHMASK;
#endif

		/* look for a match in the registered opcodes */
		lastop = -1;
		for (op = 0; op < code_log_index; op++)
			if (code_log_buffer[op].base == cur)
			{
				if (lastop != -1)
					print_log_entry(&code_log_buffer[lastop], cur, "");
				lastop = op;
			}

		/* if no match, just output the current instruction */
		if (lastop == -1)
			fprintf(logfile, "%p: %s\n", cur, buffer);

		/* if we have a match and a string, print it */
		else
			print_log_entry(&code_log_buffer[lastop], cur, buffer);

		/* advance past this instruction */
		cur += bytes;
	}

	/* flush the file */
	fflush(logfile);
}


#ifdef PTR64

/*-------------------------------------------------
    desc_flags_log - generate a string
    representing the instruction description
    flags
-------------------------------------------------*/

static const char *desc_flags_log(UINT32 flags)
{
	static char tempbuf[30];
	char *dest = tempbuf;

	/* branches */
	if (flags & IDESC_IS_UNCONDITIONAL_BRANCH)
		*dest++ = 'U';
	else if (flags & IDESC_IS_CONDITIONAL_BRANCH)
		*dest++ = 'C';
	else
		*dest++ = '.';

	/* intrablock branches */
	*dest++ = (flags & IDESC_INTRABLOCK_BRANCH) ? 'i' : '.';

	/* branch targets */
	*dest++ = (flags & IDESC_IS_BRANCH_TARGET) ? 'B' : '.';

	/* delay slots */
	*dest++ = (flags & IDESC_IN_DELAY_SLOT) ? 'D' : '.';

	/* exceptions */
	if (flags & IDESC_WILL_CAUSE_EXCEPTION)
		*dest++ = 'E';
	else if (flags & IDESC_CAN_CAUSE_EXCEPTION)
		*dest++ = 'e';
	else
		*dest++ = '.';

	/* read/write */
	if (flags & IDESC_READS_MEMORY)
		*dest++ = 'R';
	else if (flags & IDESC_WRITES_MEMORY)
		*dest++ = 'W';
	else
		*dest++ = '.';

	/* TLB validation */
	*dest++ = (flags & IDESC_VALIDATE_TLB) ? 'V' : '.';

	/* TLB modification */
	*dest++ = (flags & IDESC_MODIFIES_TRANSLATION) ? 'T' : '.';

	/* redispatch */
	*dest++ = (flags & IDESC_REDISPATCH) ? 'R' : '.';
	return tempbuf;
}


/*-------------------------------------------------
    desc_log - log a list of descriptions
-------------------------------------------------*/

static void desc_log(const mips3_opcode_desc *desclist, int indent)
{
	/* open the file, creating it if necessary */
	if (!open_logfile())
		return;
	if (indent == 0)
		fprintf(logfile, "\nDescriptor list @ %08X\n", desclist->pc);

	/* output each descriptor */
	while (desclist != NULL)
	{
		char buffer[100];
		dasmmips3(buffer, desclist->pc, *desclist->opptr);
		fprintf(logfile, "%08X [%08X] t:%08X f:%s: %-30s\n", desclist->pc, desclist->physpc, desclist->targetpc, desc_flags_log(desclist->flags), buffer);
		if (desclist->delay)
			desc_log(desclist->delay, indent + 1);
		if (desclist->flags & IDESC_END_SEQUENCE)
			fprintf(logfile, "-----\n");
		desclist = desclist->next;
	}
}
#endif

#else

#define code_log_reset()
#define code_log_add_entry(a,b,c)
#define code_log_add_entry_string(a,b,c)
#define code_log(a,b,c)
#define desc_log(a,b)

#endif



/***************************************************************************
    RECOMPILER CORE
***************************************************************************/

#ifdef PTR64
#include "mdrc64.c"
#else
#include "mdrcold.c"
#endif



/***************************************************************************
    GENERIC GET/SET INFO
***************************************************************************/

static void mips3_set_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are set as 64-bit signed integers --- */
		case CPUINFO_INT_MIPS3_DRC_OPTIONS:				mips3.drcoptions = info->i;				break;

		case CPUINFO_INT_MIPS3_FASTRAM_SELECT:			if (info->i >= 0 && info->i < MIPS3_MAX_FASTRAM) mips3.fastram_select = info->i; mips3.cache_dirty = TRUE; break;
		case CPUINFO_INT_MIPS3_FASTRAM_START:			mips3.fastram[mips3.fastram_select].start = info->i; mips3.cache_dirty = TRUE; break;
		case CPUINFO_INT_MIPS3_FASTRAM_END:				mips3.fastram[mips3.fastram_select].end = info->i; mips3.cache_dirty = TRUE; break;
		case CPUINFO_INT_MIPS3_FASTRAM_READONLY:		mips3.fastram[mips3.fastram_select].readonly = info->i; mips3.cache_dirty = TRUE; break;

		case CPUINFO_INT_MIPS3_HOTSPOT_SELECT:			if (info->i >= 0 && info->i < MIPS3_MAX_HOTSPOTS) mips3.hotspot_select = info->i; mips3.cache_dirty = TRUE; break;
		case CPUINFO_INT_MIPS3_HOTSPOT_PC:				mips3.hotspot[mips3.hotspot_select].pc = info->i; mips3.cache_dirty = TRUE; break;
		case CPUINFO_INT_MIPS3_HOTSPOT_OPCODE:			mips3.hotspot[mips3.hotspot_select].opcode = info->i; mips3.cache_dirty = TRUE; break;
		case CPUINFO_INT_MIPS3_HOTSPOT_CYCLES:			mips3.hotspot[mips3.hotspot_select].cycles = info->i; mips3.cache_dirty = TRUE; break;

		/* --- the following bits of info are set as pointers to data or functions --- */
		case CPUINFO_PTR_MIPS3_FASTRAM_BASE:			mips3.fastram[mips3.fastram_select].base = info->p;	break;

		/* --- everything else is handled generically --- */
		default:										mips3com_set_info(mips3.core, state, info);	break;
	}
}


void mips3_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof(mips3);				break;
		case CPUINFO_INT_PREVIOUSPC:					/* not implemented */					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_SET_INFO:						info->setinfo = mips3_set_info;			break;
		case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = mips3_get_context;	break;
		case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = mips3_set_context;	break;
		case CPUINFO_PTR_INIT:							/* provided per-CPU */					break;
		case CPUINFO_PTR_RESET:							info->reset = mips3_reset;				break;
		case CPUINFO_PTR_EXIT:							info->exit = mips3_exit;				break;
		case CPUINFO_PTR_EXECUTE:						info->execute = mips3_execute;			break;
#ifdef MAME_DEBUG
		case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = mips3_dasm;			break;
#endif /* MAME_DEBUG */
		case CPUINFO_PTR_TRANSLATE:						info->translate = mips3_translate;		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_CORE_FILE:						strcpy(info->s, __FILE__);				break;

		/* --- everything else is handled generically --- */
		default:										mips3com_get_info(mips3.core, state, info); break;
	}
}



/***************************************************************************
    R4600 VARIANTS
***************************************************************************/

#if (HAS_R4600)
static void r4600be_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	mips3_init(mips3.core, MIPS3_TYPE_R4600, TRUE, index, clock, config, irqcallback);
}

static void r4600le_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	mips3_init(mips3.core, MIPS3_TYPE_R4600, FALSE, index, clock, config, irqcallback);
}

void r4600be_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = r4600be_init;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "R4600 (big)");			break;

		/* --- everything else is handled generically --- */
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
		case CPUINFO_PTR_INIT:							info->init = r4600le_init;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "R4600 (little)");		break;

		/* --- everything else is handled generically --- */
		default:										mips3_get_info(state, info);			break;
	}
}
#endif



/***************************************************************************
    R4650 VARIANTS
***************************************************************************/

#if (HAS_R4650)
static void r4650be_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	mips3_init(mips3.core, MIPS3_TYPE_R4650, TRUE, index, clock, config, irqcallback);
}

static void r4650le_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	mips3_init(mips3.core, MIPS3_TYPE_R4650, FALSE, index, clock, config, irqcallback);
}

void r4650be_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = r4650be_init;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "IDT R4650 (big)");		break;

		/* --- everything else is handled generically --- */
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
		case CPUINFO_PTR_INIT:							info->init = r4650le_init;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "IDT R4650 (little)");	break;

		/* --- everything else is handled generically --- */
		default:										mips3_get_info(state, info);			break;
	}
}
#endif



/***************************************************************************
    R4700 VARIANTS
***************************************************************************/

#if (HAS_R4700)
static void r4700be_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	mips3_init(mips3.core, MIPS3_TYPE_R4700, TRUE, index, clock, config, irqcallback);
}

static void r4700le_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	mips3_init(mips3.core, MIPS3_TYPE_R4700, FALSE, index, clock, config, irqcallback);
}

void r4700be_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = r4700be_init;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "R4700 (big)");			break;

		/* --- everything else is handled generically --- */
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
		case CPUINFO_PTR_INIT:							info->init = r4700le_init;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "R4700 (little)");		break;

		/* --- everything else is handled generically --- */
		default:										mips3_get_info(state, info);			break;
	}
}
#endif



/***************************************************************************
    R5000 VARIANTS
***************************************************************************/

#if (HAS_R5000)
static void r5000be_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	mips3_init(mips3.core, MIPS3_TYPE_R5000, TRUE, index, clock, config, irqcallback);
}

static void r5000le_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	mips3_init(mips3.core, MIPS3_TYPE_R5000, FALSE, index, clock, config, irqcallback);
}

void r5000be_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = r5000be_init;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "R5000 (big)");			break;

		/* --- everything else is handled generically --- */
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
		case CPUINFO_PTR_INIT:							info->init = r5000le_init;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "R5000 (little)");		break;

		/* --- everything else is handled generically --- */
		default:										mips3_get_info(state, info);			break;
	}
}
#endif



/***************************************************************************
    QED5271 VARIANTS
***************************************************************************/

#if (HAS_QED5271)
static void qed5271be_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	mips3_init(mips3.core, MIPS3_TYPE_QED5271, TRUE, index, clock, config, irqcallback);
}

static void qed5271le_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	mips3_init(mips3.core, MIPS3_TYPE_QED5271, FALSE, index, clock, config, irqcallback);
}

void qed5271be_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = qed5271be_init;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "QED5271 (big)");		break;

		/* --- everything else is handled generically --- */
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
		case CPUINFO_PTR_INIT:							info->init = qed5271le_init;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "QED5271 (little)");	break;

		/* --- everything else is handled generically --- */
		default:										mips3_get_info(state, info);			break;
	}
}
#endif



/***************************************************************************
    RM7000 VARIANTS
***************************************************************************/

#if (HAS_RM7000)
static void rm7000be_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	mips3_init(mips3.core, MIPS3_TYPE_RM7000, TRUE, index, clock, config, irqcallback);
}

static void rm7000le_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	mips3_init(mips3.core, MIPS3_TYPE_RM7000, FALSE, index, clock, config, irqcallback);
}

void rm7000be_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = rm7000be_init;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "RM7000 (big)");		break;

		/* --- everything else is handled generically --- */
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
		case CPUINFO_PTR_INIT:							info->init = rm7000le_init;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "RM7000 (little)");		break;

		/* --- everything else is handled generically --- */
		default:										mips3_get_info(state, info);			break;
	}
}
#endif



/***************************************************************************
    DISASSEMBLERS
***************************************************************************/

#if !defined(MAME_DEBUG) && (LOG_CODE)
#include "mips3dsm.c"
#include "cpu/i386/i386dasm.c"
#endif



/***************************************************************************
    C CORE FOR COMPARISON
***************************************************************************/

#if COMPARE_AGAINST_C

static UINT8 dummy_read_byte_32le(offs_t offs)
{
	if (mips3.access_type == 0)
	{
		mips3.access_type = 1;
		mips3.access_size = 1;
		mips3.access_addr = offs;
		mips3.access_mask = 0;
		mips3.access_data = program_read_byte_32le(offs);
	}
	else
	{
		assert(mips3.access_type == 1);
		assert(mips3.access_size == 1);
		assert(mips3.access_addr == offs);
		assert(mips3.access_mask == 0);
		mips3.access_type = 0;
	}
	return mips3.access_data;
}

static UINT16 dummy_read_word_32le(offs_t offs)
{
	if (mips3.access_type == 0)
	{
		mips3.access_type = 1;
		mips3.access_size = 2;
		mips3.access_addr = offs;
		mips3.access_mask = 0;
		mips3.access_data = program_read_word_32le(offs);
	}
	else
	{
		assert(mips3.access_type == 1);
		assert(mips3.access_size == 2);
		assert(mips3.access_addr == offs);
		assert(mips3.access_mask == 0);
		mips3.access_type = 0;
	}
	return mips3.access_data;
}

static UINT32 dummy_read_dword_32le(offs_t offs)
{
	if (mips3.access_type == 0)
	{
		mips3.access_type = 1;
		mips3.access_size = 4;
		mips3.access_addr = offs;
		mips3.access_mask = 0;
		mips3.access_data = program_read_dword_32le(offs);
	}
	else
	{
		assert(mips3.access_type == 1);
		assert(mips3.access_size == 4);
		assert(mips3.access_addr == offs);
		assert(mips3.access_mask == 0);
		mips3.access_type = 0;
	}
	return mips3.access_data;
}

static UINT32 dummy_read_masked_32le(offs_t offs, UINT32 mask)
{
	if (mips3.access_type == 0)
	{
		mips3.access_type = 1;
		mips3.access_size = 4;
		mips3.access_addr = offs;
		mips3.access_mask = mask;
		mips3.access_data = program_read_masked_32le(offs, mask);
	}
	else
	{
		assert(mips3.access_type == 1);
		assert(mips3.access_size == 4);
		assert(mips3.access_addr == offs);
		assert(mips3.access_mask == mask);
		mips3.access_type = 0;
	}
	return mips3.access_data;
}

static UINT64 dummy_read_qword_32le(offs_t offs)
{
	if (mips3.access_type == 0)
	{
		mips3.access_type = 1;
		mips3.access_size = 8;
		mips3.access_addr = offs;
		mips3.access_mask = 0;
		mips3.access_data = (*mips3.orighandler.readdouble)(offs);
	}
	else
	{
		assert(mips3.access_type == 1);
		assert(mips3.access_size == 8);
		assert(mips3.access_addr == offs);
		assert(mips3.access_mask == 0);
		mips3.access_type = 0;
	}
	return mips3.access_data;
}

static UINT64 dummy_read_qword_masked_32le(offs_t offs, UINT64 mask)
{
	if (mips3.access_type == 0)
	{
		mips3.access_type = 1;
		mips3.access_size = 8;
		mips3.access_addr = offs;
		mips3.access_mask = mask;
		mips3.access_data = (*mips3.orighandler.readdouble_masked)(offs, mask);
	}
	else
	{
		assert(mips3.access_type == 1);
		assert(mips3.access_size == 8);
		assert(mips3.access_addr == offs);
		assert(mips3.access_mask == mask);
		mips3.access_type = 0;
	}
	return mips3.access_data;
}

static void dummy_write_byte_32le(offs_t offs, UINT8 data)
{
	if (mips3.access_type == 0)
	{
		mips3.access_type = 2;
		mips3.access_size = 1;
		mips3.access_addr = offs;
		mips3.access_data = data;
		mips3.access_mask = 0;
	}
	else
	{
		assert(mips3.access_type == 2);
		assert(mips3.access_size == 1);
		assert(mips3.access_addr == offs);
		assert(mips3.access_data == data);
		assert(mips3.access_mask == 0);
		mips3.access_type = 0;
		program_write_byte_32le(offs, data);
	}
}

static void dummy_write_word_32le(offs_t offs, UINT16 data)
{
	if (mips3.access_type == 0)
	{
		mips3.access_type = 2;
		mips3.access_size = 2;
		mips3.access_addr = offs;
		mips3.access_data = data;
		mips3.access_mask = 0;
	}
	else
	{
		assert(mips3.access_type == 2);
		assert(mips3.access_size == 2);
		assert(mips3.access_addr == offs);
		assert(mips3.access_data == data);
		assert(mips3.access_mask == 0);
		mips3.access_type = 0;
		program_write_word_32le(offs, data);
	}
}

static void dummy_write_dword_32le(offs_t offs, UINT32 data)
{
	if (mips3.access_type == 0)
	{
		mips3.access_type = 2;
		mips3.access_size = 4;
		mips3.access_addr = offs;
		mips3.access_data = data;
		mips3.access_mask = 0;
	}
	else
	{
		assert(mips3.access_type == 2);
		assert(mips3.access_size == 4);
		assert(mips3.access_addr == offs);
		assert(mips3.access_data == data);
		assert(mips3.access_mask == 0);
		mips3.access_type = 0;
		program_write_dword_32le(offs, data);
	}
}

static void dummy_write_masked_32le(offs_t offs, UINT32 data, UINT32 mask)
{
	if (mips3.access_type == 0)
	{
		mips3.access_type = 2;
		mips3.access_size = 4;
		mips3.access_addr = offs;
		mips3.access_data = data;
		mips3.access_mask = mask;
	}
	else
	{
		assert(mips3.access_type == 2);
		assert(mips3.access_size == 4);
		assert(mips3.access_addr == offs);
		assert(mips3.access_data == data);
		assert(mips3.access_mask == mask);
		mips3.access_type = 0;
		program_write_masked_32le(offs, data, mask);
	}
}

static void dummy_write_qword_32le(offs_t offs, UINT64 data)
{
	if (mips3.access_type == 0)
	{
		mips3.access_type = 2;
		mips3.access_size = 8;
		mips3.access_addr = offs;
		mips3.access_data = data;
		mips3.access_mask = 0;
	}
	else
	{
		assert(mips3.access_type == 2);
		assert(mips3.access_size == 8);
		assert(mips3.access_addr == offs);
		assert(mips3.access_data == data);
		assert(mips3.access_mask == 0);
		mips3.access_type = 0;
		(*mips3.orighandler.writedouble)(offs, data);
	}
}

static void dummy_write_qword_masked_32le(offs_t offs, UINT64 data, UINT64 mask)
{
	if (mips3.access_type == 0)
	{
		mips3.access_type = 2;
		mips3.access_size = 8;
		mips3.access_addr = offs;
		mips3.access_data = data;
		mips3.access_mask = mask;
	}
	else
	{
		assert(mips3.access_type == 2);
		assert(mips3.access_size == 8);
		assert(mips3.access_addr == offs);
		assert(mips3.access_data == data);
		assert(mips3.access_mask == mask);
		mips3.access_type = 0;
		(*mips3.orighandler.writedouble_masked)(offs, data, mask);
	}
}

static const memory_handlers override_memory =
{
	dummy_read_byte_32le,
	dummy_read_word_32le,
	dummy_read_dword_32le,
	dummy_read_masked_32le,
	dummy_read_qword_32le,
	dummy_read_qword_masked_32le,

	dummy_write_byte_32le,
	dummy_write_word_32le,
	dummy_write_dword_32le,
	dummy_write_masked_32le,
	dummy_write_qword_32le,
	dummy_write_qword_masked_32le
};

#undef SR

#define mips3				c_mips3
#define mips3_regs			c_mips3_regs

#define mips3_get_context 	c_mips3_get_context
#define mips3_set_context 	c_mips3_set_context
#define mips3_reset 		c_mips3_reset
#define mips3_translate 	c_mips3_translate
#define mips3_dasm 			c_mips3_dasm
#define mips3_execute 		c_mips3_execute
#define mips3_set_info 		c_mips3_set_info
#define mips3_get_info 		c_mips3_get_info

#define r4600be_init		c_r4600be_init
#define r4600le_init		c_r4600le_init
#define r4600be_get_info	c_r4600be_get_info
#define r4600le_get_info	c_r4600le_get_info

#define r4650be_init		c_r4650be_init
#define r4650le_init		c_r4650le_init
#define r4650be_get_info	c_r4650be_get_info
#define r4650le_get_info	c_r4650le_get_info

#define r4700be_init		c_r4700be_init
#define r4700le_init		c_r4700le_init
#define r4700be_get_info	c_r4700be_get_info
#define r4700le_get_info	c_r4700le_get_info

#define r5000be_init		c_r5000be_init
#define r5000le_init		c_r5000le_init
#define r5000be_get_info	c_r5000be_get_info
#define r5000le_get_info	c_r5000le_get_info

#define qed5271be_init		c_qed5271be_init
#define qed5271le_init		c_qed5271le_init
#define qed5271be_get_info	c_qed5271be_get_info
#define qed5271le_get_info	c_qed5271le_get_info

#define rm7000be_init		c_rm7000be_init
#define rm7000le_init		c_rm7000le_init
#define rm7000be_get_info	c_rm7000be_get_info
#define rm7000le_get_info	c_rm7000le_get_info

#include "mips3.c"

#undef mips3

static void mips3c_init(mips3_flavor flavor, int bigendian, int index, int clock, const struct mips3_config *config, int (*irqcallback)(int))
{
	mips3com_init(&c_mips3.core, flavor, bigendian, index, clock, config, irqcallback);
	mips3.orighandler = mips3.core->memory;
	mips3.core->memory = override_memory;
	c_mips3.core.memory = override_memory;
}

static void mips3c_reset(void)
{
	c_mips3_reset();
}

static void execute_c_version(void)
{
	int save_debug_mode = Machine->debug_mode;
	int i;

	assert(mips3.access_type == 0);

	c_mips3.core.cpr[0][COP0_Cause] = (c_mips3.core.cpr[0][COP0_Cause] & ~0xfc00) | (mips3.core->cpr[0][COP0_Cause] & 0xfc00);
	check_irqs();

	assert(c_mips3.core.pc == mips3.core->pc);
	assert(c_mips3.core.hi == mips3.core->hi);
	assert(c_mips3.core.lo == mips3.core->lo);
	for (i = 0; i < 32; i++)
	{
		assert(c_mips3.core.r[i] == mips3.core->r[i]);
		assert(c_mips3.core.cpr[0][i] == mips3.core->cpr[0][i]);
		assert(c_mips3.core.ccr[0][i] == mips3.core->ccr[0][i]);
		assert(c_mips3.core.cpr[1][i] == mips3.core->cpr[1][i]);
		assert(c_mips3.core.ccr[1][i] == mips3.core->ccr[1][i]);
	}
	for (i = 0; i < 8; i++)
		assert(c_mips3.core.cf[1][i] == mips3.core->cf[1][i]);
	assert(c_mips3.core.count_zero_time == mips3.core->count_zero_time);

	mips3.access_type = 0;

	Machine->debug_mode = 0;
	c_mips3_execute(0);
	Machine->debug_mode = save_debug_mode;
}

static void compare_c_version(void)
{
}

#endif
