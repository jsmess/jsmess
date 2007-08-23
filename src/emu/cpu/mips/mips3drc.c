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



/***************************************************************************
    DEBUGGING
***************************************************************************/

#define LOG_CODE			(0)



/***************************************************************************
    CONFIGURATION
***************************************************************************/

#define MAX_INSTRUCTIONS	512



/***************************************************************************
    MACROS
***************************************************************************/

#define SR			mips3.core.cpr[0][COP0_Status]
#define IS_FR0		(!(SR & SR_FR))
#define IS_FR1		(SR & SR_FR)



/***************************************************************************
    STRUCTURES & TYPEDEFS
***************************************************************************/

/* code logging info */
typedef struct _code_log_entry code_log_entry;
struct _code_log_entry
{
	UINT32		pc;
	UINT32		op;
	void *		base;
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
	mips3_state		core;
	mips3drc_data *	drc;
	UINT32			drcoptions;

	/* internal stuff */
	UINT32			nextpc;

	/* fast RAM */
	UINT32			fastram_select;
	fast_ram_info	fastram[MIPS3_MAX_FASTRAM];

	/* hotspots */
	UINT32			hotspot_select;
	hotspot_info	hotspot[MIPS3_MAX_HOTSPOTS];
};



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static void mips3drc_init(void);
static void mips3drc_reset(void);
static void mips3drc_execute(void);
static void mips3drc_exit(void);



/***************************************************************************
    PRIVATE GLOBAL VARIABLES
***************************************************************************/

static mips3_regs mips3;



/***************************************************************************
    CORE CALLBACKS
***************************************************************************/

/*-------------------------------------------------
    mips3_reset - reset the processor
-------------------------------------------------*/

static void mips3_reset(void)
{
	mips3com_reset(&mips3.core);
	mips3drc_reset();
}


/*-------------------------------------------------
    mips3_execute - execute the CPU for the
    specified number of cycles
-------------------------------------------------*/

static int mips3_execute(int cycles)
{
	/* update the cycle timing */
	mips3com_update_cycle_counting(&mips3.core);

	/* execute */
	mips3_icount = cycles;
	mips3drc_execute();
	return cycles - mips3_icount;
}


/*-------------------------------------------------
    mips3_exit - cleanup from execution
-------------------------------------------------*/

static void mips3_exit(void)
{
	mips3drc_exit();
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
	return mips3com_translate_address(&mips3.core, space, address);
}


/*-------------------------------------------------
    mips3_dasm - disassemble an instruction
-------------------------------------------------*/

#ifdef MAME_DEBUG
static offs_t mips3_dasm(char *buffer, offs_t pc, const UINT8 *oprom, const UINT8 *opram)
{
	return mips3com_dasm(&mips3.core, buffer, pc, oprom, opram);
}
#endif /* MAME_DEBUG */



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

static void code_log(const char *label, void *start, void *stop)
{
	extern int i386_dasm_one(char *buffer, UINT32 eip, UINT8 *oprom, int mode);
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
	while (cur < (UINT8 *)stop)
	{
		char buffer[100];
		int bytes;
		int op;

		/* disassemble this instruction */
#ifdef PTR64
		bytes = i386_dasm_one(buffer, (UINT32)cur, cur, 64) & DASMFLAG_LENGTHMASK;
#else
		bytes = i386_dasm_one(buffer, (UINT32)cur, cur, 32) & DASMFLAG_LENGTHMASK;
#endif

		/* look for a match in the registered opcodes */
		for (op = 0; op < code_log_index; op++)
			if (code_log_buffer[op].base == (void *)cur)
				break;

		/* if no match, just output the current instruction */
		if (op == code_log_index)
			fprintf(f, "%p: %s\n", cur, buffer);

		/* otherwise, output with the original instruction to the right */
		else
		{
			char buffer2[100];
			dasmmips3(buffer2, code_log_buffer[op].pc, code_log_buffer[op].op);
			fprintf(f, "%p: %-50s %08X: %s\n", cur, buffer, code_log_buffer[op].pc, buffer2);
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

		/* --- everything else is handled generically --- */
		default:										mips3com_set_info(&mips3.core, state, info);	break;
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
		default:										mips3com_get_info(&mips3.core, state, info); break;
	}
}



/***************************************************************************
    R4600 VARIANTS
***************************************************************************/

#if (HAS_R4600)
static void r4600be_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	mips3com_init(&mips3.core, MIPS3_TYPE_R4600, TRUE, index, clock, config, irqcallback);
	mips3drc_init();
}

static void r4600le_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	mips3com_init(&mips3.core, MIPS3_TYPE_R4600, FALSE, index, clock, config, irqcallback);
	mips3drc_init();
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
	mips3com_init(&mips3.core, MIPS3_TYPE_R4650, TRUE, index, clock, config, irqcallback);
	mips3drc_init();
}

static void r4650le_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	mips3com_init(&mips3.core, MIPS3_TYPE_R4650, FALSE, index, clock, config, irqcallback);
	mips3drc_init();
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
	mips3com_init(&mips3.core, MIPS3_TYPE_R4700, TRUE, index, clock, config, irqcallback);
	mips3drc_init();
}

static void r4700le_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	mips3com_init(&mips3.core, MIPS3_TYPE_R4700, FALSE, index, clock, config, irqcallback);
	mips3drc_init();
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
	mips3com_init(&mips3.core, MIPS3_TYPE_R5000, TRUE, index, clock, config, irqcallback);
	mips3drc_init();
}

static void r5000le_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	mips3com_init(&mips3.core, MIPS3_TYPE_R5000, FALSE, index, clock, config, irqcallback);
	mips3drc_init();
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
	mips3com_init(&mips3.core, MIPS3_TYPE_QED5271, TRUE, index, clock, config, irqcallback);
	mips3drc_init();
}

static void qed5271le_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	mips3com_init(&mips3.core, MIPS3_TYPE_QED5271, FALSE, index, clock, config, irqcallback);
	mips3drc_init();
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
	mips3com_init(&mips3.core, MIPS3_TYPE_RM7000, TRUE, index, clock, config, irqcallback);
	mips3drc_init();
}

static void rm7000le_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	mips3com_init(&mips3.core, MIPS3_TYPE_RM7000, FALSE, index, clock, config, irqcallback);
	mips3drc_init();
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
