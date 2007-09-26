/***************************************************************************

    mdrc64.c

    x64 MIPS III recompiler.

    Copyright (c) 2007, Aaron Giles
    Released for general use under the MAME license
    Visit http://mamedev.org for licensing and usage restrictions.

****************************************************************************

    Conventions/rules:

        * Instructions are grouped into sequences by mips3fe; each
          sequence of instructions is guaranteed to not contain any
          branches to any instructions within the sequence apart from
          the first.

        * Because of the above rule, assumptions are made during the
          execution of a sequence in order to optimize the generated
          code.

        * Because some state may be live and not flushed to memory at
          the time an exception is generated, all exception-generating
          instructions must jump to stub code that is appended to each
          group of instructions which cleans up live registers before
          running the exception code.

        * Page faults are trickier because they are detected in a
          subroutine. In order to handle this, special markers are added
          to the code consisting of one qword EXCEPTION_ADDRESS_COOKIE
          followed by the return address of the call. When a page fault
          needs to be generated, the common code scans forward from
          the return address for this cookie/address pair and jumps to
          the code immediately following.

        * Convention: jumps to non-constant addresses have their target
          PC stored at [sp+SPOFFS_NEXTPC]

****************************************************************************

    Future improvements:
        * spec says if registers aren't sign-extended, then 32-bit ops have
            undefined results; this can simplify our code
        * switch to a model where the compiler ensures all MIPS registers
            are live in x64 registers at the start of the instruction
            and flushed at the end

****************************************************************************

    Temporary stack usage:
        [esp+32].b  = storage for CL in LWL/LWR/SWL/SWR
        [esp+36].l  = storage for new target PC in JR/JALR
        [esp+40].l  = storage for parameter 1 when invalidating code
        [esp+48].l  = storage for parameter 2 when invalidating code
        [esp+56].l  = storage for parameter 3 when invalidating code

***************************************************************************/

#include "mips3fe.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define COMPILE_BACKWARDS_BYTES			128
#define COMPILE_FORWARDS_BYTES			512
#define COMPILE_MAX_INSTRUCTIONS		((COMPILE_BACKWARDS_BYTES/4) + (COMPILE_FORWARDS_BYTES/4))

#define COND_NONE						16
#define COND_NO_JUMP					17

#define EXCEPTION_ADDRESS_COOKIE		U64(0xcc90cc90cc90cc90)

/* append_readwrite flags */
#define ARW_READ						0x0000
#define ARW_WRITE						0x0001
#define ARW_UNSIGNED					0x0000
#define ARW_SIGNED						0x0002
#define ARW_MASKED						0x0004
#define ARW_CACHED						0x0008
#define ARW_UNCACHED					0x0010

#if COMPARE_AGAINST_C
#undef  MIPS3_COUNT_READ_CYCLES
#undef  MIPS3_CAUSE_READ_CYCLES
#define MIPS3_COUNT_READ_CYCLES			0
#define MIPS3_CAUSE_READ_CYCLES			0
#endif



/***************************************************************************
    MACROS
***************************************************************************/

#define SPOFFS_SAVECL			32
#define SPOFFS_NEXTPC			36
#define SPOFFS_SAVEP1			40
#define SPOFFS_SAVEP2			48
#define SPOFFS_SAVEP3			56

#define REGADDR(reg)			MDRC(&mips3.core->r[reg])
#define HIADDR					MDRC(&mips3.core->hi)
#define LOADDR					MDRC(&mips3.core->lo)
#define CPR0ADDR(reg)			MDRC(&mips3.core->cpr[0][reg])
#define CCR0ADDR(reg)			MDRC(&mips3.core->ccr[0][reg])
#define CPR1ADDR(reg)			MDRC(&mips3.core->cpr[1][reg])
#define CCR1ADDR(reg)			MDRC(&mips3.core->ccr[1][reg])
#define CPR2ADDR(reg)			MDRC(&mips3.core->cpr[2][reg])
#define CCR2ADDR(reg)			MDRC(&mips3.core->ccr[2][reg])
#define FPR32(reg)				(IS_FR0 ? &((float *)&mips3.core->cpr[1][0])[reg] : (float *)&mips3.core->cpr[1][reg])
#define FPR32ADDR(reg)			MDRC(FPR32(reg))
#define FPR64(reg)				(IS_FR0 ? (double *)&mips3.core->cpr[1][(reg)/2] : (double *)&mips3.core->cpr[1][reg])
#define FPR64ADDR(reg)			MDRC(FPR64(reg))
#define CF1ADDR(which)			MDRC(&mips3.core->cf[1][(mips3.core->flavor < MIPS3_TYPE_MIPS_IV) ? 0 : (which)])
#define PCADDR					MDRC(&mips3.core->pc)
#define ICOUNTADDR				MDRC(&mips3.core->icount)
#define COREADDR				MDRC(mips3.core)



/***************************************************************************
    TYPEDEFS
***************************************************************************/

typedef struct _readwrite_handlers readwrite_handlers;
struct _readwrite_handlers
{
	x86code *		read_byte_signed;
	x86code *		read_byte_unsigned;
	x86code *		read_word_signed;
	x86code *		read_word_unsigned;
	x86code *		read_long_signed;
	x86code *		read_long_unsigned;
	x86code *		read_long_masked;
	x86code *		read_double;
	x86code *		read_double_masked;
	x86code *		write_byte;
	x86code *		write_word;
	x86code *		write_long;
	x86code *		write_long_masked;
	x86code *		write_double;
	x86code *		write_double_masked;
};


typedef struct _compiler_state compiler_state;
struct _compiler_state
{
	UINT32			cycles;					/* accumulated cycles */
};


typedef struct _oob_handler oob_handler;

typedef void (*oob_callback)(drc_core *drc, oob_handler *oob, void *param);

struct _oob_handler
{
	oob_callback	callback;				/* pointer to callback function */
	void *			param;					/* callback parameter */
	const mips3_opcode_desc *desc;			/* pointer to description of relevant instruction */
	emit_link		link;					/* link to source of the branch */
	compiler_state	compiler;				/* state of the compiler at this time */
};


struct _mips3drc_data
{
	/* misc data */
	UINT64			abs64mask[2];
	UINT32			abs32mask[4];

	/* C functions */
	x86code *		activecpu_gettotalcycles64;
	x86code *		mips3com_update_cycle_counting;
	x86code *		mips3com_tlbr;
	x86code *		mips3com_tlbwi;
	x86code *		mips3com_tlbwr;
	x86code *		mips3com_tlbp;

	/* internal functions */
	x86code *		generate_interrupt_exception;
	x86code *		generate_cop_exception;
	x86code *		generate_overflow_exception;
	x86code *		generate_invalidop_exception;
	x86code *		generate_syscall_exception;
	x86code *		generate_break_exception;
	x86code *		generate_trap_exception;
	x86code *		generate_tlbload_exception;
	x86code *		generate_tlbstore_exception;
	x86code *		handle_pc_tlb_mismatch;
	x86code *		find_exception_handler;
	x86code *		explode_ccr31;
	x86code *		recover_ccr31;

	/* read/write handlers */
	readwrite_handlers general;
	readwrite_handlers cached;
	readwrite_handlers uncached;

	/* out of bounds handlers */
	oob_handler		ooblist[COMPILE_MAX_INSTRUCTIONS*2];
	int				oobcount;

#if COMPARE_AGAINST_C
	/* C functions */
	x86code *		execute_c_version;
#endif
};



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static void drc_reset_callback(drc_core *drc);
static void drc_recompile_callback(drc_core *drc);
static void drc_entrygen_callback(drc_core *drc);

static void compile_one(drc_core *drc, compiler_state *compiler, mips3_opcode_desc *desc);

static void oob_exception_cleanup(drc_core *drc, oob_handler *oob, void *param);
static void oob_interrupt_cleanup(drc_core *drc, oob_handler *oob, void *param);

static void generate_read_write_handlers(drc_core *drc, readwrite_handlers *handlers, const char *type, UINT8 flags);
static void append_generate_exception(drc_core *drc, UINT8 exception);
static void append_readwrite_and_translate(drc_core *drc, int size, UINT8 flags, UINT32 ptroffs);
static void append_handle_pc_tlb_mismatch(drc_core *drc);
static void append_find_exception_handler(drc_core *drc);
static void append_explode_ccr31(drc_core *drc);
static void append_recover_ccr31(drc_core *drc);
static void append_check_interrupts(drc_core *drc, compiler_state *compiler, const mips3_opcode_desc *desc);
static void append_check_sw_interrupts(drc_core *drc, int inline_generate);

static int recompile_instruction(drc_core *drc, compiler_state *compiler, mips3_opcode_desc *desc);
static int recompile_special(drc_core *drc, compiler_state *compiler, mips3_opcode_desc *desc);
static int recompile_regimm(drc_core *drc, compiler_state *compiler, mips3_opcode_desc *desc);
static int recompile_cop0(drc_core *drc, compiler_state *compiler, mips3_opcode_desc *desc);
static int recompile_cop1(drc_core *drc, compiler_state *compiler, mips3_opcode_desc *desc);
static int recompile_cop1x(drc_core *drc, compiler_state *compiler, mips3_opcode_desc *desc);



/***************************************************************************
    PRIVATE GLOBAL VARIABLES
***************************************************************************/

static UINT64 dmult_temp1;
static UINT64 dmult_temp2;
static UINT32 jr_temp;

static compiler_state compiler_save;



/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    oob_request_callback - request an out-of-
    bounds callback for later
-------------------------------------------------*/

INLINE void oob_request_callback(drc_core *drc, UINT8 condition, oob_callback callback, const compiler_state *compiler, const mips3_opcode_desc *desc, void *param)
{
	oob_handler *oob = &mips3.drcdata->ooblist[mips3.drcdata->oobcount++];

	/* fill in the next handler */
	oob->callback = callback;
	oob->desc = desc;
	oob->compiler = *compiler;
	oob->param = param;

	/* emit an appropriate jump */
	if (condition == COND_NO_JUMP)
	{
		oob->link.size = 0;
		oob->link.target = drc->cache_top;
	}
	else if (condition == COND_NONE)
		emit_jmp_near_link(DRCTOP, &oob->link);												// jmp  <target>
	else
		emit_jcc_near_link(DRCTOP, condition, &oob->link);									// jcc  <target>
}



/***************************************************************************
    CORE RECOMPILER SYSTEMS
***************************************************************************/

/*-------------------------------------------------
    mips3drc_init - initialize the drc-specific
    state
-------------------------------------------------*/

static void mips3drc_init(void)
{
	drc_config drconfig;

	/* fill in the config */
	memset(&drconfig, 0, sizeof(drconfig));
	drconfig.cache_base       = mips3.cache;
	drconfig.cache_size       = CACHE_SIZE;
	drconfig.max_instructions = MAX_INSTRUCTIONS;
	drconfig.address_bits     = 32;
	drconfig.lsbs_to_ignore   = 2;
	drconfig.baseptr	      = &mips3.core->r[17];
	drconfig.pcptr            = (UINT32 *)&mips3.core->pc;
	drconfig.cb_reset         = drc_reset_callback;
	drconfig.cb_recompile     = drc_recompile_callback;
	drconfig.cb_entrygen      = drc_entrygen_callback;

	/* initialize the compiler */
	mips3.drc = drc_init(cpu_getactivecpu(), &drconfig);
	mips3.drcoptions = MIPS3DRC_FASTEST_OPTIONS;

	/* allocate our data out of the cache */
	mips3.drcdata = drc_alloc(mips3.drc, sizeof(*mips3.drcdata) + 16);
	mips3.drcdata = (mips3drc_data *)((((FPTR)mips3.drcdata + 15) >> 4) << 4);
	memset(mips3.drcdata, 0, sizeof(*mips3.drcdata));

	/* set up constants */
	mips3.drcdata->abs64mask[0] = U64(0x7fffffffffffffff);
	mips3.drcdata->abs32mask[0] = 0x7fffffff;

	/* get pointers to C functions */
	mips3.drcdata->activecpu_gettotalcycles64 = (x86code *)activecpu_gettotalcycles64;
	mips3.drcdata->mips3com_update_cycle_counting = (x86code *)mips3com_update_cycle_counting;
	mips3.drcdata->mips3com_tlbr = (x86code *)mips3com_tlbr;
	mips3.drcdata->mips3com_tlbwi = (x86code *)mips3com_tlbwi;
	mips3.drcdata->mips3com_tlbwr = (x86code *)mips3com_tlbwr;
	mips3.drcdata->mips3com_tlbp = (x86code *)mips3com_tlbp;

#if COMPARE_AGAINST_C
	mips3.drcdata->execute_c_version = (x86code *)execute_c_version;
#endif
}


/*-------------------------------------------------
    mips3drc_exit - clean up the drc-specific
    state
-------------------------------------------------*/

static void mips3drc_exit(void)
{
	mips3fe_exit();
}



/***************************************************************************
    RECOMPILER CALLBACKS
***************************************************************************/

/*------------------------------------------------------------------
    drc_reset_callback
------------------------------------------------------------------*/

static void drc_reset_callback(drc_core *drc)
{
	code_log_reset();

	code_log("entry_point:", (x86code *)drc->entry_point, drc->exit_point);
	code_log("exit_point:", drc->exit_point, drc->recompile);
	code_log("recompile:", drc->recompile, drc->dispatch);
	code_log("dispatch:", drc->dispatch, drc->flush);
	code_log("flush:", drc->flush, drc->cache_top);

	mips3.drcdata->generate_interrupt_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_INTERRUPT);
	code_log("generate_interrupt_exception:", mips3.drcdata->generate_interrupt_exception, drc->cache_top);

	mips3.drcdata->generate_cop_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_BADCOP);
	code_log("generate_cop_exception:", mips3.drcdata->generate_cop_exception, drc->cache_top);

	mips3.drcdata->generate_overflow_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_OVERFLOW);
	code_log("generate_overflow_exception:", mips3.drcdata->generate_overflow_exception, drc->cache_top);

	mips3.drcdata->generate_invalidop_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_INVALIDOP);
	code_log("generate_invalidop_exception:", mips3.drcdata->generate_invalidop_exception, drc->cache_top);

	mips3.drcdata->generate_syscall_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_SYSCALL);
	code_log("generate_syscall_exception:", mips3.drcdata->generate_syscall_exception, drc->cache_top);

	mips3.drcdata->generate_break_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_BREAK);
	code_log("generate_break_exception:", mips3.drcdata->generate_break_exception, drc->cache_top);

	mips3.drcdata->generate_trap_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_TRAP);
	code_log("generate_trap_exception:", mips3.drcdata->generate_trap_exception, drc->cache_top);

	mips3.drcdata->generate_tlbload_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_TLBLOAD);
	code_log("generate_tlbload_exception:", mips3.drcdata->generate_tlbload_exception, drc->cache_top);

	mips3.drcdata->generate_tlbstore_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_TLBSTORE);
	code_log("generate_tlbstore_exception:", mips3.drcdata->generate_tlbstore_exception, drc->cache_top);

	mips3.drcdata->handle_pc_tlb_mismatch = drc->cache_top;
	append_handle_pc_tlb_mismatch(drc);
	code_log("handle_pc_tlb_mismatch:", mips3.drcdata->handle_pc_tlb_mismatch, drc->cache_top);

	mips3.drcdata->find_exception_handler = drc->cache_top;
	append_find_exception_handler(drc);
	code_log("find_exception_handler:", mips3.drcdata->find_exception_handler, drc->cache_top);

	mips3.drcdata->explode_ccr31 = drc->cache_top;
	append_explode_ccr31(drc);
	code_log("explode_ccr31:", mips3.drcdata->explode_ccr31, drc->cache_top);

	mips3.drcdata->recover_ccr31 = drc->cache_top;
	append_recover_ccr31(drc);
	code_log("recover_ccr31:", mips3.drcdata->recover_ccr31, drc->cache_top);

	generate_read_write_handlers(drc, &mips3.drcdata->general, "general", 0);
	generate_read_write_handlers(drc, &mips3.drcdata->cached, "cached", ARW_CACHED);
	generate_read_write_handlers(drc, &mips3.drcdata->uncached, "uncached", ARW_UNCACHED);
}


/*------------------------------------------------------------------
    drc_recompile_callback
------------------------------------------------------------------*/

static void drc_recompile_callback(drc_core *drc)
{
	mips3_opcode_desc *seqhead, *seqlast;
	mips3_opcode_desc *desclist;
	void *start = drc->cache_top;
	int override = FALSE;
	int oobnum;

	(void)start;

	/* begin the sequence */
	drc_begin_sequence(drc, mips3.core->pc);
	code_log_reset();
	mips3.drcdata->oobcount = 0;

	/* get a description of this sequence */
	desclist = mips3fe_describe_sequence(mips3.core, mips3.core->pc, mips3.core->pc - 128, mips3.core->pc + 512);
	desc_log(desclist, 0);

	/* loop until we get through all instruction sequences */
	for (seqhead = desclist; seqhead != NULL; seqhead = seqlast->next)
	{
		int instructions_remaining;
		mips3_opcode_desc *curdesc;
		compiler_state compiler;

		/* determine the last instruction in this sequence */
		instructions_remaining = Machine->debug_mode ? 1 : 32;
		for (seqlast = seqhead; seqlast != NULL; seqlast = seqlast->next)
		{
			mips3_opcode_desc *next = seqlast->next;

			/* if this instruction ends a block, stop */
			if (seqlast->flags & IDESC_END_SEQUENCE)
				break;

			/* if we're out of instructions, stop */
			if (--instructions_remaining == 0)
				break;
		}
		assert(seqlast != NULL);

		/* add this as an entry point */
		if (drc_add_entry_point(drc, seqhead->pc, override) && !override)
		{
			/* if this is the first sequence, it is a recompile request; allow overrides */
			if (seqhead == desclist)
			{
				override = TRUE;
				drc_add_entry_point(drc, seqhead->pc, override);
			}

			/* otherwise, just emit a jump to existing code */
			else
			{
				drc_append_fixed_dispatcher(drc, seqhead->pc, TRUE);
				continue;
			}
		}

		/* add a code log entry */
		code_log_add_entry_string(drc->cache_top, "[Validation for %08X]", seqhead->pc);

		/* validate this code block */
		emit_mov_r64_imm(DRCTOP, REG_RAX, (UINT64)seqhead->opptr);							// mov  rax,seqhead->opptr
		emit_mov_r32_imm(DRCTOP, REG_P1, seqhead->pc);										// mov  p1,pc
		for (curdesc = seqhead; curdesc != seqlast; curdesc = curdesc->next)
		{
			emit_cmp_m32_imm(DRCTOP, MBD(REG_RAX, (UINT8 *)curdesc->opptr - (UINT8 *)seqhead->opptr), *curdesc->opptr);
																							// cmp  [code],val
			emit_jcc(DRCTOP, COND_NE, mips3.drc->recompile);								// jne  recompile
		}
		emit_cmp_m32_imm(DRCTOP, MBD(REG_RAX, (UINT8 *)curdesc->opptr - (UINT8 *)seqhead->opptr), *curdesc->opptr);
																							// cmp  [code],val
		emit_jcc(DRCTOP, COND_NE, mips3.drc->recompile);									// jne  recompile

		/* initialize the compiler state */
		compiler.cycles = 0;

		/* iterate over instructions in the sequence and compile them */
		for (curdesc = seqhead; curdesc != seqlast; curdesc = curdesc->next)
			compile_one(drc, &compiler, curdesc);
		compile_one(drc, &compiler, curdesc);

		/* at the end of the sequence; update the PC in memory and check cycle counts */
		if (!(curdesc->flags & (IDESC_IS_UNCONDITIONAL_BRANCH | IDESC_WILL_CAUSE_EXCEPTION)))
		{
			UINT32 nextpc = curdesc->pc + ((curdesc->flags & IDESC_IS_LIKELY_BRANCH) ? 8 : 4);
			if (compiler.cycles != 0)
				emit_sub_m32_imm(DRCTOP, ICOUNTADDR, compiler.cycles);						// sub  [icount],cycles
			emit_mov_r32_imm(DRCTOP, REG_P1, nextpc);										// mov  p1,nextpc
			if (compiler.cycles != 0)
				emit_jcc(DRCTOP, COND_S, mips3.drc->exit_point);							// js   exit_point
		}

		/* if this is a likely branch, the fall through case needs to skip the next instruction */
		if ((curdesc->flags & (IDESC_IS_CONDITIONAL_BRANCH | IDESC_IS_LIKELY_BRANCH)) == (IDESC_IS_CONDITIONAL_BRANCH | IDESC_IS_LIKELY_BRANCH))
			if (curdesc->next != NULL && curdesc->next->pc != curdesc->pc + 8)
				drc_append_tentative_fixed_dispatcher(drc, curdesc->pc + 8, TRUE);			// jmp  <pc+8>

		/* if we need a redispatch, do it now */
		if (curdesc->flags & IDESC_REDISPATCH)
		{
			UINT32 nextpc = curdesc->pc + ((curdesc->flags & IDESC_IS_LIKELY_BRANCH) ? 8 : 4);
			drc_append_tentative_fixed_dispatcher(drc, nextpc, TRUE);						// jmp  <nextpc>
		}

		/* if we need to return to the start, do it */
		if (curdesc->flags & IDESC_RETURN_TO_START)
			drc_append_tentative_fixed_dispatcher(drc, mips3.core->pc, FALSE);				// jmp  <startpc>
	}

	/* free the list */
	mips3fe_release_descriptions(desclist);

	/* end the sequence */
	drc_end_sequence(drc);

	/* run the out-of-bounds handlers */
	for (oobnum = 0; oobnum < mips3.drcdata->oobcount; oobnum++)
	{
		oob_handler *oob = &mips3.drcdata->ooblist[oobnum];
		(*oob->callback)(drc, oob, oob->param);
	}

#if LOG_CODE
{
	char label[40];
	sprintf(label, "Code @ %08X (%08X physical)", desclist->pc, desclist->physpc);
	code_log(label, start, drc->cache_top);
}
#endif
}


/*------------------------------------------------------------------
    drc_entrygen_callback
------------------------------------------------------------------*/

static void drc_entrygen_callback(drc_core *drc)
{
	append_check_interrupts(drc, NULL, NULL);
}



/***************************************************************************
    RECOMPILER CORE
***************************************************************************/

/*------------------------------------------------------------------
    compile_one
------------------------------------------------------------------*/

static void compile_one(drc_core *drc, compiler_state *compiler, mips3_opcode_desc *desc)
{
	int hotnum;

	/* register this instruction */
	if (!(desc->flags & IDESC_IN_DELAY_SLOT))
		drc_register_code_at_cache_top(drc, desc->pc);

	/* add an entry for the log */
	code_log_add_entry(drc->cache_top, desc->pc, *desc->opptr);

	/* if we are debugging, call the debugger */
	if (Machine->debug_mode)
	{
		emit_mov_m32_imm(DRCTOP, PCADDR, desc->pc);											// mov  [pc],desc->pc
#if COMPARE_AGAINST_C
		if (!(desc->flags & IDESC_IN_DELAY_SLOT))
			emit_call_m64(DRCTOP, MDRC(&mips3.drcdata->execute_c_version));
#endif
		drc_append_call_debugger(drc);														// <call debugger>
	}

	/* validate our TLB entry at this PC; if we fail, we need to recompile */
	if (desc->flags & IDESC_VALIDATE_TLB)
	{
		emit_cmp_m32_imm(DRCTOP, MDRC(&mips3.core->tlb_table[desc->pc >> 12]), mips3.core->tlb_table[desc->pc >> 12]);
																							// cmp  tlb_table[pc>>12],<original value>
		oob_request_callback(drc, COND_NE, oob_exception_cleanup, compiler, desc, mips3.drcdata->handle_pc_tlb_mismatch);
																							// jne  handle_pc_tlb_mismatch
	}

	/* accumulate total cycles */
	compiler->cycles += desc->cycles;

	/* is this a hotspot? */
	for (hotnum = 0; hotnum < MIPS3_MAX_HOTSPOTS; hotnum++)
		if (desc->pc == mips3.hotspot[hotnum].pc && *desc->opptr == mips3.hotspot[hotnum].opcode)
		{
			compiler->cycles += mips3.hotspot[hotnum].cycles;
			break;
		}

	/* if this is an invalid opcode, generate the exception now */
	if (desc->flags & IDESC_INVALID_OPCODE)
		oob_request_callback(drc, COND_NONE, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_invalidop_exception);

	/* otherwise, it's a regular instruction */
	else
	{
		UINT32 nextpc = desc->pc + ((desc->flags & IDESC_IS_LIKELY_BRANCH) ? 8 : 4);

		/* compile the instruction */
		if (!recompile_instruction(drc, compiler, desc))
			fatalerror("Unimplemented op %08X (%02X,%02X)", *desc->opptr, *desc->opptr >> 26, *desc->opptr & 0x3f);

		/* if there was a read or a write, we need to update cycle counts and flush */
		if (!(desc->flags & IDESC_IN_DELAY_SLOT) && (desc->flags & (IDESC_READS_MEMORY | IDESC_WRITES_MEMORY)))
		{
			emit_sub_m32_imm(DRCTOP, ICOUNTADDR, compiler->cycles);
			compiler->cycles = 0;
			oob_request_callback(drc, COND_S, oob_interrupt_cleanup, compiler, desc, mips3.drc->exit_point);
		}
	}
}



/***************************************************************************
    OUT-OF-BOUNDS CODEGEN
***************************************************************************/

/*-------------------------------------------------
    oob_exception_cleanup - out-of-bounds code
    generator for cleaning up and generating an
    exception with the EPC pointing to the
    current instruction
-------------------------------------------------*/

static void oob_exception_cleanup(drc_core *drc, oob_handler *oob, void *param)
{
	x86code *start = drc->cache_top;

	/* if this is non-jumping error, emit a unique signature */
	if (oob->link.size == 0)
	{
		while (((UINT64)drc->cache_top & 7) != 0)
			emit_int_3(DRCTOP);
		code_log_add_entry_string(drc->cache_top, "[Exception path for %08X]", oob->desc->pc);
		emit_qword(DRCTOP, EXCEPTION_ADDRESS_COOKIE);
		emit_qword(DRCTOP, (UINT64)oob->link.target);
	}

	/* otherwise, just resolve the link */
	else
	{
		resolve_link(DRCTOP, &oob->link);
		code_log_add_entry_string(drc->cache_top, "[Exception path for %08X]", oob->desc->pc);
	}

	/* adjust for cycles */
	if (oob->compiler.cycles != 0)
		emit_sub_m32_imm(DRCTOP, ICOUNTADDR, oob->compiler.cycles);							// sub  [icount],cycles

	/* update the PC with the instruction PC */
	if (!(oob->desc->flags & IDESC_IN_DELAY_SLOT))
		emit_mov_r32_imm(DRCTOP, REG_P1, oob->desc->pc);									// mov  p1,pc
	else
		emit_mov_r32_imm(DRCTOP, REG_P1, oob->desc->pc | 1);								// mov  p1,pc | 1

	/* eventually: flush any dirty registers */

	/* jump to the exception generator */
	emit_jmp(DRCTOP, param);																// jmp  <generate exception>
}


/*-------------------------------------------------
    oob_interrupt_cleanup - out-of-bounds code
    generator for cleaning up and generating an
    interrupt with the EPC pointing to the
    following instruction
-------------------------------------------------*/

static void oob_interrupt_cleanup(drc_core *drc, oob_handler *oob, void *param)
{
	x86code *start = drc->cache_top;

	/* resolve the link */
	resolve_link(DRCTOP, &oob->link);

	/* add a code log entry */
	code_log_add_entry_string(drc->cache_top, "[Interrupt path for %08X]", oob->desc->pc);

	/* adjust for cycles */
	if (oob->compiler.cycles != 0)
		emit_sub_m32_imm(DRCTOP, ICOUNTADDR, oob->compiler.cycles);							// sub  [icount],cycles

	/* update the PC with the following instruction PC */
	if (!(oob->desc->flags & IDESC_IN_DELAY_SLOT))
	{
		assert(!(oob->desc->flags & IDESC_IS_BRANCH));
		emit_mov_r32_imm(DRCTOP, REG_P1, oob->desc->pc + 4);								// mov  p1,pc + 4
	}
	else
	{
		assert(oob->desc->branch != NULL);
		if (oob->desc->branch->targetpc != ~0)
			emit_mov_r32_imm(DRCTOP, REG_P1, oob->desc->branch->targetpc);					// mov  p1,<targetpc>
		else
			emit_mov_r32_m32(DRCTOP, REG_P1, MBD(REG_RSP, SPOFFS_NEXTPC));					// mov  p1,[rsp + nextpc]
	}

	/* eventually: flush any dirty registers */

	/* jump to the exception generator */
	emit_jmp(DRCTOP, param);																// jmp  <generate exception>
}



/***************************************************************************
    COMMON ROUTINES
***************************************************************************/

/*------------------------------------------------------------------
    generate_read_write_handlers
------------------------------------------------------------------*/

static void generate_read_write_handlers(drc_core *drc, readwrite_handlers *handlers, const char *type, UINT8 flags)
{
	char buffer[100];

	handlers->read_byte_signed = drc->cache_top;
	append_readwrite_and_translate(drc, 1, ARW_READ | ARW_SIGNED | flags, offsetof(readwrite_handlers, read_byte_signed));
	sprintf(buffer, "%s.read_byte_signed:", type);
	code_log(buffer, handlers->read_byte_signed, drc->cache_top);

	handlers->read_byte_unsigned = drc->cache_top;
	append_readwrite_and_translate(drc, 1, ARW_READ | ARW_UNSIGNED | flags, offsetof(readwrite_handlers, read_byte_unsigned));
	sprintf(buffer, "%s.read_byte_unsigned:", type);
	code_log(buffer, handlers->read_byte_unsigned, drc->cache_top);

	handlers->read_word_signed = drc->cache_top;
	append_readwrite_and_translate(drc, 2, ARW_READ | ARW_SIGNED | flags, offsetof(readwrite_handlers, read_word_signed));
	sprintf(buffer, "%s.read_word_signed:", type);
	code_log(buffer, handlers->read_word_signed, drc->cache_top);

	handlers->read_word_unsigned = drc->cache_top;
	append_readwrite_and_translate(drc, 2, ARW_READ | ARW_UNSIGNED | flags, offsetof(readwrite_handlers, read_word_unsigned));
	sprintf(buffer, "%s.read_word_unsigned:", type);
	code_log(buffer, handlers->read_word_unsigned, drc->cache_top);

	handlers->read_long_signed = drc->cache_top;
	append_readwrite_and_translate(drc, 4, ARW_READ | ARW_SIGNED | flags, offsetof(readwrite_handlers, read_long_signed));
	sprintf(buffer, "%s.read_long_signed:", type);
	code_log(buffer, handlers->read_long_signed, drc->cache_top);

	handlers->read_long_unsigned = drc->cache_top;
	append_readwrite_and_translate(drc, 4, ARW_READ | ARW_UNSIGNED | flags, offsetof(readwrite_handlers, read_long_unsigned));
	sprintf(buffer, "%s.read_long_unsigned:", type);
	code_log(buffer, handlers->read_long_unsigned, drc->cache_top);

	handlers->read_long_masked = drc->cache_top;
	append_readwrite_and_translate(drc, 4, ARW_READ | ARW_UNSIGNED | ARW_MASKED | flags, offsetof(readwrite_handlers, read_long_masked));
	sprintf(buffer, "%s.read_long_masked:", type);
	code_log(buffer, handlers->read_long_masked, drc->cache_top);

	handlers->read_double = drc->cache_top;
	append_readwrite_and_translate(drc, 8, ARW_READ | flags, offsetof(readwrite_handlers, read_double));
	sprintf(buffer, "%s.read_double:", type);
	code_log(buffer, handlers->read_double, drc->cache_top);

	handlers->read_double_masked = drc->cache_top;
	append_readwrite_and_translate(drc, 8, ARW_READ | ARW_MASKED | flags, offsetof(readwrite_handlers, read_double_masked));
	sprintf(buffer, "%s.read_double_masked:", type);
	code_log(buffer, handlers->read_double_masked, drc->cache_top);

	handlers->write_byte = drc->cache_top;
	append_readwrite_and_translate(drc, 1, ARW_WRITE | flags, offsetof(readwrite_handlers, write_byte));
	sprintf(buffer, "%s.write_byte:", type);
	code_log(buffer, handlers->write_byte, drc->cache_top);

	handlers->write_word = drc->cache_top;
	append_readwrite_and_translate(drc, 2, ARW_WRITE | flags, offsetof(readwrite_handlers, write_word));
	sprintf(buffer, "%s.write_word:", type);
	code_log(buffer, handlers->write_word, drc->cache_top);

	handlers->write_long = drc->cache_top;
	append_readwrite_and_translate(drc, 4, ARW_WRITE | flags, offsetof(readwrite_handlers, write_long));
	sprintf(buffer, "%s.write_long:", type);
	code_log(buffer, handlers->write_long, drc->cache_top);

	handlers->write_long_masked = drc->cache_top;
	append_readwrite_and_translate(drc, 4, ARW_WRITE | ARW_MASKED | flags, offsetof(readwrite_handlers, write_long_masked));
	sprintf(buffer, "%s.write_long_masked:", type);
	code_log(buffer, handlers->write_long_masked, drc->cache_top);

	handlers->write_double = drc->cache_top;
	append_readwrite_and_translate(drc, 8, ARW_WRITE | flags, offsetof(readwrite_handlers, write_double));
	sprintf(buffer, "%s.write_double:", type);
	code_log(buffer, handlers->write_double, drc->cache_top);

	handlers->write_double_masked = drc->cache_top;
	append_readwrite_and_translate(drc, 8, ARW_WRITE | ARW_MASKED | flags, offsetof(readwrite_handlers, write_double_masked));
	sprintf(buffer, "%s.write_double_masked:", type);
	code_log(buffer, handlers->write_double_masked, drc->cache_top);
}


/*------------------------------------------------------------------
    append_delay_slot_and_branch
------------------------------------------------------------------*/

static void append_delay_slot_and_branch(drc_core *drc, const compiler_state *compiler, const mips3_opcode_desc *desc)
{
	compiler_state compiler_temp = *compiler;

	/* compile the delay slot using temporary compiler state */
	assert(desc->delay != NULL);
	compile_one(drc, &compiler_temp, desc->delay);											// <next instruction>

	/* if we have cycles, subtract them now */
	if (compiler_temp.cycles != 0)
		emit_sub_m32_imm(DRCTOP, ICOUNTADDR, compiler_temp.cycles);							// sub  [icount],cycles

	/* load the target PC into P1 */
	if (desc->targetpc != ~0)
		emit_mov_r32_imm(DRCTOP, REG_P1, desc->targetpc);									// mov  p1,desc->targetpc
	else
		emit_mov_r32_m32(DRCTOP, REG_P1, MBD(REG_RSP, SPOFFS_NEXTPC));						// mov  p1,[esp+nextpc]

	/* if we subtracted cycles and went negative, time to exit */
	if (compiler_temp.cycles != 0)
		emit_jcc(DRCTOP, COND_S, mips3.drc->exit_point);									// js   exit_point

	/* otherwise, append a dispatcher */
	if (desc->targetpc != ~0)
		drc_append_tentative_fixed_dispatcher(drc, desc->targetpc, FALSE);					// <redispatch>
	else
		drc_append_dispatcher(drc);															// <redispatch>
}


/*------------------------------------------------------------------
    append_generate_exception
------------------------------------------------------------------*/

static void append_generate_exception(drc_core *drc, UINT8 exception)
{
	UINT32 offset = (exception >= EXCEPTION_TLBMOD && exception <= EXCEPTION_TLBSTORE) ? 0x00 : 0x180;
	emit_link link1, link2;

	if (exception == EXCEPTION_TLBLOAD || exception == EXCEPTION_TLBSTORE)
	{
		/* assumes failed address is in P2 */
		emit_mov_m32_r32(DRCTOP, CPR0ADDR(COP0_BadVAddr), REG_P2);							// mov  [BadVAddr],p2
		emit_and_m32_imm(DRCTOP, CPR0ADDR(COP0_EntryHi), 0x000000ff);						// and  [EntryHi],0x000000ff
		emit_and_r32_imm(DRCTOP, REG_P2, 0xffffe000);										// and  p2,0xffffe000
		emit_or_m32_r32(DRCTOP, CPR0ADDR(COP0_EntryHi), REG_P2);							// or   [EntryHi],p2
		emit_and_m32_imm(DRCTOP, CPR0ADDR(COP0_Context), 0xff800000);						// and  [Context],0xff800000
		emit_shr_r32_imm(DRCTOP, REG_P2, 9);												// shr  p2,9
		emit_or_m32_r32(DRCTOP, CPR0ADDR(COP0_Context), REG_P2);							// or   [Context],p2
	}

	/* on entry, exception PC must be in REG_P1, and low bit set if in a delay slot */
	emit_mov_r32_m32(DRCTOP, REG_EAX, CPR0ADDR(COP0_Cause));								// mov  eax,[Cause]
	emit_and_r32_imm(DRCTOP, REG_EAX, ~0x800000ff);											// and  eax,~0x800000ff
	emit_test_r32_imm(DRCTOP, REG_P1, 1);													// test p1,1
	emit_jcc_short_link(DRCTOP, COND_Z, &link1);											// jz   skip
	emit_or_r32_imm(DRCTOP, REG_EAX, 0x80000000);											// or   eax,0x80000000
	emit_sub_r32_imm(DRCTOP, REG_P1, 5);													// sub  p1,5

	resolve_link(DRCTOP, &link1);														// skip:
	emit_mov_m32_r32(DRCTOP, CPR0ADDR(COP0_EPC), REG_P1);									// mov  [EPC],p1
	if (exception)
		emit_or_r32_imm(DRCTOP, REG_EAX, exception << 2);									// or   eax,exception << 2
	emit_mov_m32_r32(DRCTOP, CPR0ADDR(COP0_Cause), REG_EAX);								// mov  [Cause],eax
	emit_mov_r32_m32(DRCTOP, REG_EAX, CPR0ADDR(COP0_Status));								// mov  eax,[SR]
	emit_or_r32_imm(DRCTOP, REG_EAX, SR_EXL);												// or   eax,SR_EXL
	emit_test_r32_imm(DRCTOP, REG_EAX, SR_BEV);												// test eax,SR_BEV
	emit_mov_m32_r32(DRCTOP, CPR0ADDR(COP0_Status), REG_EAX);								// mov  [SR],eax
	emit_mov_r32_imm(DRCTOP, REG_P1, 0xbfc00200 + offset);									// mov  p1,0xbfc00200+offset
	emit_jcc_short_link(DRCTOP, COND_NZ, &link2);											// jnz  skip2
	emit_mov_r32_imm(DRCTOP, REG_P1, 0x80000000 + offset);									// mov  p1,0x80000000+offset

	resolve_link(DRCTOP, &link2);														// skip2:
	drc_append_dispatcher(drc);																// dispatch
}


/*------------------------------------------------------------------
    append_handle_pc_tlb_mismatch
------------------------------------------------------------------*/

static void append_handle_pc_tlb_mismatch(drc_core *drc)
{
#if 0
	emit_mov_r32_r32(DRCTOP, REG_EAX, REG_R15D);											// mov  eax,edi
	emit_shr_r32_imm(DRCTOP, REG_EAX, 12);													// shr  eax,12
	emit_mov_r32_m32(DRCTOP, REG_EDX, MISD(REG_EAX, 4, &mips3.core->tlb_table));			// mov  edx,tlb_table[eax*4]
	emit_test_r32_imm(DRCTOP, REG_EDX, 2);													// test edx,2
	emit_mov_r32_r32(DRCTOP, REG_EAX, REG_R15D);											// mov  eax,edi
	emit_jcc(DRCTOP, COND_NZ, mips3.drcdata->generate_tlbload_exception);					// jnz  generate_tlbload_exception
	emit_mov_m32bd_r32(DRCTOP, drc->pcptr, REG_R15D);										// mov  [pcptr],edi
	emit_jmp(DRCTOP, drc->recompile);														// call recompile
#endif
}


/*------------------------------------------------------------------
    append_find_exception_handler
------------------------------------------------------------------*/

static void append_find_exception_handler(drc_core *drc)
{
	x86code *loop;

	emit_mov_r64_m64(DRCTOP, REG_RAX, MBD(REG_RSP, 0));										// mov  rax,[rsp]
	emit_mov_r64_imm(DRCTOP, REG_V5, EXCEPTION_ADDRESS_COOKIE);								// mov  v5,EXCEPTION_ADDRESS_COOKIE
	emit_mov_r64_r64(DRCTOP, REG_V6, REG_RAX);												// mov  v6,rax
	emit_and_r64_imm(DRCTOP, REG_RAX, ~7);													// and  rax,~7
	emit_add_r64_imm(DRCTOP, REG_RSP, 8);													// add  rsp,8
	emit_add_r64_imm(DRCTOP, REG_RAX, 8);													// add  rax,8
	loop = drc->cache_top;																// loop:
	emit_cmp_m64_r64(DRCTOP, MBD(REG_RAX, 0), REG_V5);										// cmp  [rax],v5
	emit_lea_r64_m64(DRCTOP, REG_RAX, MBD(REG_RAX, 8));										// lea  rax,[rax+8]
	emit_jcc(DRCTOP, COND_NE, loop);														// jne  loop
	emit_cmp_m64_r64(DRCTOP, MBD(REG_RAX, 0), REG_V6);										// cmp  [rax],v6
	emit_lea_r64_m64(DRCTOP, REG_RAX, MBD(REG_RAX, 8));										// lea  rax,[rax+8]
	emit_jcc(DRCTOP, COND_NE, loop);														// jne  loop
	emit_jmp_r64(DRCTOP, REG_RAX);															// jmp  rax
}


/*------------------------------------------------------------------
    append_direct_ram_read
------------------------------------------------------------------*/

static void append_direct_ram_read(drc_core *drc, int size, UINT8 flags, void *rambase, offs_t ramstart)
{
	void *adjbase = (UINT8 *)rambase - ramstart;
	INT64 adjdisp = (UINT8 *)adjbase - (UINT8 *)drc->baseptr;
	int raxrel = ((INT32)adjdisp != adjdisp);

	/* if we can't fit the delta to adjbase within an INT32, load RAX with the full address here */
	if (raxrel)
		emit_mov_r64_imm(DRCTOP, REG_RAX, (UINT64)adjbase);

	/* fast RAM byte read */
	if (size == 1)
	{
		/* adjust for endianness */
		if (mips3.core->bigendian)
			emit_xor_r32_imm(DRCTOP, REG_P1, 3);											// xor   p1,3

		/* read rax-relative */
		if (raxrel)
		{
			if (flags & ARW_SIGNED)
				emit_movsx_r64_m8(DRCTOP, REG_RAX, MBISD(REG_RAX, REG_P1, 1, 0));			// movsx rax,byte ptr [p1 + rax]
			else
				emit_movzx_r64_m8(DRCTOP, REG_RAX, MBISD(REG_RAX, REG_P1, 1, 0));			// movzx rax,byte ptr [p1 + rax]
		}

		/* read drc-relative */
		else
		{
			if (flags & ARW_SIGNED)
				emit_movsx_r64_m8(DRCTOP, REG_RAX, MDRCISD(adjbase, REG_P1, 1, 0));			// movsx rax,byte ptr [p1 + adjbase]
			else
				emit_movzx_r64_m8(DRCTOP, REG_RAX, MDRCISD(adjbase, REG_P1, 1, 0));			// movzx rax,byte ptr [p1 + adjbase]
		}
	}

	/* fast RAM word read */
	else if (size == 2)
	{
		/* adjust for endianness */
		if (mips3.core->bigendian)
			emit_xor_r32_imm(DRCTOP, REG_P1, 2);											// xor   p1,2

		/* read rax-relative */
		if (raxrel)
		{
			if (flags & ARW_SIGNED)
				emit_movsx_r64_m16(DRCTOP, REG_RAX, MBISD(REG_RAX, REG_P1, 1, 0));			// movsx rax,word ptr [p1 + rax]
			else
				emit_movzx_r64_m16(DRCTOP, REG_EAX, MBISD(REG_RAX, REG_P1, 1, 0));			// movzx rax,word ptr [p1 + rax]
		}

		/* read drc-relative */
		else
		{
			if (flags & ARW_SIGNED)
				emit_movsx_r64_m16(DRCTOP, REG_RAX, MDRCISD(adjbase, REG_P1, 1, 0));		// movsx rax,word ptr [p1 + adjbase]
			else
				emit_movzx_r64_m16(DRCTOP, REG_EAX, MDRCISD(adjbase, REG_P1, 1, 0));		// movzx rax,word ptr [p1 + adjbase]
		}
	}

	/* fast RAM dword read */
	else if (size == 4)
	{
		/* read rax-relative */
		if (raxrel)
		{
			if (flags & ARW_SIGNED)
				emit_movsxd_r64_m32(DRCTOP, REG_RAX, MBISD(REG_RAX, REG_P1, 1, 0));			// movsxd rax,dword ptr [p1 + rax]
			else
				emit_mov_r32_m32(DRCTOP, REG_EAX, MBISD(REG_RAX, REG_P1, 1, 0));			// mov   eax,word ptr [p1 + rax]
		}

		/* read drc-relative */
		else
		{
			if (flags & ARW_SIGNED)
				emit_movsxd_r64_m32(DRCTOP, REG_RAX, MDRCISD(adjbase, REG_P1, 1, 0));		// movsxd rax,dword ptr [p1 + adjbase]
			else
				emit_mov_r32_m32(DRCTOP, REG_EAX, MDRCISD(adjbase, REG_P1, 1, 0));			// mov   eax,word ptr [p1 + adjbase]
		}
	}

	/* fast RAM qword read */
	else if (size == 8)
	{
		/* read rax-relative */
		if (raxrel)
			emit_mov_r64_m64(DRCTOP, REG_RAX, MBISD(REG_RAX, REG_P1, 1, 0));				// mov   rax,[p1 + rax]

		/* read drc-relative */
		else
			emit_mov_r64_m64(DRCTOP, REG_RAX, MDRCISD(adjbase, REG_P1, 1, 0));				// mov   rax,[p1 + adjbase]

		/* adjust for endianness */
		if (mips3.core->bigendian)
			emit_ror_r64_imm(DRCTOP, REG_RAX, 32);											// ror   rax,32
	}
}


/*------------------------------------------------------------------
    append_direct_ram_write
------------------------------------------------------------------*/

static void append_direct_ram_write(drc_core *drc, int size, UINT8 flags, void *rambase, offs_t ramstart)
{
	void *adjbase = (UINT8 *)rambase - ramstart;
	INT64 adjdisp = (UINT8 *)adjbase - (UINT8 *)drc->baseptr;
	int raxrel = ((INT32)adjdisp != adjdisp);

	/* if we can't fit the delta to adjbase within an INT32, load RAX with the full address here */
	if (raxrel)
		emit_mov_r64_imm(DRCTOP, REG_RAX, (UINT64)adjbase);

	/* fast RAM byte write */
	if (size == 1)
	{
		/* adjust for endianness */
		if (mips3.core->bigendian)
			emit_xor_r32_imm(DRCTOP, REG_P1, 3);											// xor   p1,3

		/* write either rax-relative or drc-relative */
		if (raxrel)
			emit_mov_m8_r8(DRCTOP, MBISD(REG_RAX, REG_P1, 1, 0), REG_P2);					// mov   [p1 + rax],p2
		else
			emit_mov_m8_r8(DRCTOP, MDRCISD(adjbase, REG_P1, 1, 0), REG_P2);					// mov   [p1 + adjbase],p2
	}

	/* fast RAM word write */
	else if (size == 2)
	{
		/* adjust for endianness */
		if (mips3.core->bigendian)
			emit_xor_r32_imm(DRCTOP, REG_P1, 2);											// xor   p1,2

		/* write either rax-relative or drc-relative */
		if (raxrel)
			emit_mov_m16_r16(DRCTOP, MBISD(REG_RAX, REG_P1, 1, 0), REG_P2);					// mov   [p1 + rax],p2
		else
			emit_mov_m16_r16(DRCTOP, MDRCISD(adjbase, REG_P1, 1, 0), REG_P2);				// mov   [p1 + adjbase],p2
	}

	/* fast RAM dword write */
	else if (size == 4)
	{
		/* unmasked */
		if (!(flags & ARW_MASKED))
		{
			if (raxrel)
				emit_mov_m32_r32(DRCTOP, MBISD(REG_RAX, REG_P1, 1, 0), REG_P2);				// mov   [p1 + rax],p2
			else
				emit_mov_m32_r32(DRCTOP, MDRCISD(adjbase, REG_P1, 1, 0), REG_P2);			// mov   [p1 + adjbase],p2
		}

		/* masked */
		else
		{
			emit_mov_r32_r32(DRCTOP, REG_P4, REG_P3);										// mov   p4,p3
			if (raxrel)
				emit_and_r32_m32(DRCTOP, REG_P3, MBISD(REG_RAX, REG_P1, 1, 0));				// and   p3,[p1 + rax]
			else
				emit_and_r32_m32(DRCTOP, REG_P3, MDRCISD(adjbase, REG_P1, 1, 0));			// and   p3,[p1 + adjbase]
			emit_not_r32(DRCTOP, REG_P4);													// not   p4
			emit_and_r32_r32(DRCTOP, REG_P4, REG_P2);										// and   p4,p2
			emit_or_r32_r32(DRCTOP, REG_P4, REG_P3);										// or    p4,p3
			if (raxrel)
				emit_mov_m32_r32(DRCTOP, MBISD(REG_RAX, REG_P1, 1, 0), REG_P4);				// mov   [p1 + rax],p4
			else
				emit_mov_m32_r32(DRCTOP, MDRCISD(adjbase, REG_P1, 1, 0), REG_P4);			// mov   [p1 + adjbase],p4
		}
	}

	/* fast RAM qword write */
	else if (size == 8)
	{
		/* adjust for endianness */
		if (mips3.core->bigendian)
			emit_ror_r64_imm(DRCTOP, REG_P2, 32);											// ror  p2,32

		/* unmasked */
		if (!(flags & ARW_MASKED))
		{
			if (raxrel)
				emit_mov_m64_r64(DRCTOP, MBISD(REG_RAX, REG_P1, 1, 0), REG_P2);				// mov   [p1 + rax],p2
			else
				emit_mov_m64_r64(DRCTOP, MDRCISD(adjbase, REG_P1, 1, 0), REG_P2);			// mov   [p1 + adjbase],p2
		}

		/* masked */
		else
		{
			if (mips3.core->bigendian)
				emit_ror_r64_imm(DRCTOP, REG_P3, 32);										// ror   p3,32
			emit_mov_r64_r64(DRCTOP, REG_P4, REG_P3);										// mov   p4,p3
			if (raxrel)
				emit_and_r64_m64(DRCTOP, REG_P3, MBISD(REG_RAX, REG_P1, 1, 0));				// and   p3,[p1 + rax]
			else
				emit_and_r64_m64(DRCTOP, REG_P3, MDRCISD(adjbase, REG_P1, 1, 0));			// and   p3,[p1 + adjbase]
			emit_not_r64(DRCTOP, REG_P4);													// not   p4
			emit_and_r64_r64(DRCTOP, REG_P4, REG_P2);										// and   p4,p2
			emit_or_r64_r64(DRCTOP, REG_P4, REG_P3);										// or    p4,p3
			if (raxrel)
				emit_mov_m64_r64(DRCTOP, MBISD(REG_RAX, REG_P1, 1, 0), REG_P4);				// mov   [p1 + rax],p4
			else
				emit_mov_m64_r64(DRCTOP, MDRCISD(adjbase, REG_P1, 1, 0), REG_P4);			// mov   [p1 + adjbase],p4
		}
	}
}


/*------------------------------------------------------------------
    append_readwrite_and_translate
------------------------------------------------------------------*/

static void append_readwrite_and_translate(drc_core *drc, int size, UINT8 flags, UINT32 ptroffs)
{
	UINT32 base = ((flags & ARW_CACHED) ? 0x80000000 : (flags & ARW_UNCACHED) ? 0xa0000000 : 0);
	emit_link page_fault = { 0 }, handle_kernel = { 0 }, wrong_handler = { 0 };
	x86code *continue_mapped = NULL;
	int ramnum;

	/* if the base is 0, this is a general case which may be paged; perform the translation first */
	/* the result of the translation produces the physical address inline in P1 */
	/* if a page fault occurs, we have to keep an original copy of the address in EAX */
	/* note that we use RAX and P4 as scratch registers */
	if (base == 0)
	{
		emit_test_r32_r32(DRCTOP, REG_P1, REG_P1);											// test p1,p1
		emit_mov_r32_r32(DRCTOP, REG_P4, REG_P1);											// mov  p4,p1d
		emit_jcc_near_link(DRCTOP, COND_S, &handle_kernel);									// js   handle_kernel
		continue_mapped = drc->cache_top;												// continue_mapped:
		emit_mov_r32_r32(DRCTOP, REG_V5, REG_P1);											// mov  v5,p1
		emit_shr_r32_imm(DRCTOP, REG_P1, 12);												// shr  p1,12
		emit_mov_r32_m32(DRCTOP, REG_P1, MDRCISD(mips3.core->tlb_table, REG_P1, 4, 0));		// mov  p1,tlb_table[p1*4]
		emit_and_r32_imm(DRCTOP, REG_P4, 0xfff);											// and  p4,0xfff
		emit_shr_r32_imm(DRCTOP, REG_P1, (flags & ARW_WRITE) ? 1 : 2);						// shr  p1,2/1 (read/write)
		emit_lea_r32_m32(DRCTOP, REG_P1, MBISD(REG_P4, REG_P1, (flags & ARW_WRITE) ? 2 : 4, 0));
																							// lea  p1,[p4 + p1 * 4/2 (read/write)]
		emit_jcc_near_link(DRCTOP, COND_C, &page_fault);									// jc   page_fault
	}

	/* if the base is non-zero, this is a cached/uncached direct-mapped case; no translation necessary */
	/* just subtract the base from the address to get the physcial address */
	else
		emit_sub_r32_imm(DRCTOP, REG_P1, base);												// sub  p1,base

	/* iterate over all fast RAM ranges looking for matches that can be handled directly */
	for (ramnum = 0; ramnum < MIPS3_MAX_FASTRAM; ramnum++)
	{
		const fast_ram_info *raminfo = &mips3.fastram[ramnum];
		if (!Machine->debug_mode && raminfo->base != NULL && (!(flags & ARW_WRITE) || !raminfo->readonly))
		{
			emit_link notram1 = { 0 }, notram2 = { 0 };

			/* if we have an end, check it first */
			if (raminfo->end != 0xffffffff)
			{
				emit_cmp_r32_imm(DRCTOP, REG_P1, raminfo->end);								// cmp  p1,fastram_end
				emit_jcc_short_link(DRCTOP, COND_A, &notram1);								// ja   notram1
			}

			/* if we have a non-0 start, check it next */
			if (raminfo->start != 0x00000000)
			{
				emit_cmp_r32_imm(DRCTOP, REG_P1, raminfo->start);							// cmp  p1,fastram_start
				emit_jcc_short_link(DRCTOP, COND_B, &notram2);								// jb   notram2
			}

			/* handle a fast RAM read */
			if (!(flags & ARW_WRITE))
			{
				append_direct_ram_read(drc, size, flags, raminfo->base, raminfo->start);	// <direct read>
				emit_ret(DRCTOP);															// ret
			}

			/* handle a fast RAM write */
			else
			{
				append_direct_ram_write(drc, size, flags, raminfo->base, raminfo->start);	// <direct write>
				emit_ret(DRCTOP);															// ret
			}

			/* resolve links that skipped over us */
			if (raminfo->end != 0xffffffff)
				resolve_link(DRCTOP, &notram1);											// notram1:
			if (raminfo->start != 0x00000000)
				resolve_link(DRCTOP, &notram2);											// notram2:
		}
	}

	/* failed to find it in fast RAM; make sure we are within our expected range */
	if (base != 0)
	{
		emit_cmp_r32_imm(DRCTOP, REG_P1, 0x20000000);										// cmp  p1,0x20000000
		emit_jcc_short_link(DRCTOP, COND_AE, &wrong_handler);								// jae  wrong_handler
	}

	/* handle a write */
	if (flags & ARW_WRITE)
	{
		if (size == 1)
			emit_jmp_m64(DRCTOP, MDRC(&mips3.core->memory.writebyte));						// jmp  writebyte
		else if (size == 2)
			emit_jmp_m64(DRCTOP, MDRC(&mips3.core->memory.writeword));						// jmp  writeword
		else if (size == 4)
		{
			if (!(flags & ARW_MASKED))
				emit_jmp_m64(DRCTOP, MDRC(&mips3.core->memory.writelong));					// jmp  writelong
			else
				emit_jmp_m64(DRCTOP, MDRC(&mips3.core->memory.writelong_masked));			// jmp  writelong_masked
		}
		else
		{
			if (!(flags & ARW_MASKED))
				emit_jmp_m64(DRCTOP, MDRC(&mips3.core->memory.writedouble));				// jmp  writedouble
			else
				emit_jmp_m64(DRCTOP, MDRC(&mips3.core->memory.writedouble_masked));			// jmp  writedouble_masked
		}
	}

	/* handle a read */
	else
	{
		if (size == 1)
		{
			emit_sub_r64_imm(DRCTOP, REG_RSP, 0x28);										// sub   rsp,0x28
			emit_call_m64(DRCTOP, MDRC(&mips3.core->memory.readbyte));						// call  readbyte
			if (flags & ARW_SIGNED)
				emit_movsx_r64_r8(DRCTOP, REG_RAX, REG_AL);									// movsx rax,al
			else
				emit_movzx_r64_r8(DRCTOP, REG_RAX, REG_AL);									// movzx rax,al
			emit_add_r64_imm(DRCTOP, REG_RSP, 0x28);										// add   rsp,0x28
			emit_ret(DRCTOP);																// ret
		}
		else if (size == 2)
		{
			emit_sub_r64_imm(DRCTOP, REG_RSP, 0x28);										// sub   rsp,0x28
			emit_call_m64(DRCTOP, MDRC(&mips3.core->memory.readword));						// call  readword
			if (flags & ARW_SIGNED)
				emit_movsx_r64_r16(DRCTOP, REG_RAX, REG_AX);								// movsx rax,ax
			else
				emit_movzx_r64_r16(DRCTOP, REG_RAX, REG_AX);								// movzx rax,ax
			emit_add_r64_imm(DRCTOP, REG_RSP, 0x28);										// add   rsp,0x28
			emit_ret(DRCTOP);																// ret
		}
		else if (size == 4)
		{
			if (!(flags & ARW_MASKED))
			{
				if (!(flags & ARW_SIGNED))
					emit_jmp_m64(DRCTOP, MDRC(&mips3.core->memory.readlong));				// jmp  readlong
				else
				{
					emit_sub_r64_imm(DRCTOP, REG_RSP, 0x28);								// sub  rsp,0x28
					emit_call_m64(DRCTOP, MDRC(&mips3.core->memory.readlong));				// call readlong
					emit_movsxd_r64_r32(DRCTOP, REG_RAX, REG_EAX);							// movsxd rax,eax
					emit_add_r64_imm(DRCTOP, REG_RSP, 0x28);								// add  rsp,0x28
					emit_ret(DRCTOP);														// ret
				}
			}
			else
				emit_jmp_m64(DRCTOP, MDRC(&mips3.core->memory.readlong_masked));			// jmp  readlong_masked
		}
		else
		{
			if (!(flags & ARW_MASKED))
				emit_jmp_m64(DRCTOP, MDRC(&mips3.core->memory.readdouble));					// jmp  readdouble
			else
				emit_jmp_m64(DRCTOP, MDRC(&mips3.core->memory.readdouble_masked));			// jmp  readdouble_masked
		}
	}

	/* handle wrong handlers */
	if (base == 0)
	{
		resolve_link(DRCTOP, &handle_kernel);											// handle_kernel:
		emit_cmp_r32_imm(DRCTOP, REG_P1, 0xc0000000);										// cmp  p1,0xc0000000
		emit_jcc(DRCTOP, COND_AE, continue_mapped);											// jae  continue_mapped
		emit_test_r32_imm(DRCTOP, REG_P1, 0x20000000);										// test p1,0x20000000
		emit_cmovcc_r64_m64(DRCTOP, COND_Z, REG_RAX, MDRCD(&mips3.drcdata->cached, ptroffs));
																							// mov  rax,<cached version>
		emit_cmovcc_r64_m64(DRCTOP, COND_NZ, REG_RAX, MDRCD(&mips3.drcdata->uncached, ptroffs));
																							// mov  rax,<uncached version>
		emit_mov_r64_m64(DRCTOP, REG_P4, MBD(REG_RSP, 0));									// mov  p4,[rsp]
		emit_push_r64(DRCTOP, REG_RAX);														// push rax
		emit_sub_r64_r64(DRCTOP, REG_RAX, REG_P4);											// sub  rax,p4
		emit_mov_m32_r32(DRCTOP, MBD(REG_P4, -4), REG_EAX);									// mov  [p4-4],eax
		emit_ret(DRCTOP);																	// ret
	}
	else
	{
		resolve_link(DRCTOP, &wrong_handler);											// wrong_handler:
		emit_mov_r64_m64(DRCTOP, REG_RAX, MDRCD(&mips3.drcdata->general, ptroffs));
																							// mov  rax,<general version>
		emit_mov_r64_m64(DRCTOP, REG_P4, MBD(REG_RSP, 0));									// mov  p4,[rsp]
		emit_push_r64(DRCTOP, REG_RAX);														// push rax
		emit_sub_r64_r64(DRCTOP, REG_RAX, REG_P4);											// sub  rax,p4
		emit_mov_m32_r32(DRCTOP, MBD(REG_P4, -4), REG_EAX);									// mov  [p4-4],eax
		emit_add_r32_imm(DRCTOP, REG_P1, base);												// add  p1,base
		emit_ret(DRCTOP);																	// ret
	}

	/* handle page faults */
	if (base == 0)
	{
		resolve_link(DRCTOP, &page_fault);												// page_fault:
		emit_mov_r32_r32(DRCTOP, REG_P2, REG_V5);											// mov  p2,v5
		emit_jmp(DRCTOP, mips3.drcdata->find_exception_handler);							// jmp  find_exception_handler
	}
}


/*------------------------------------------------------------------
    append_explode_ccr31
------------------------------------------------------------------*/

static void append_explode_ccr31(drc_core *drc)
{
	emit_link link1;
	int i;

	/* new CCR31 value in EAX */
	emit_test_r32_imm(DRCTOP, REG_EAX, 1 << 23);											// test eax,1 << 23
	emit_mov_m32_r32(DRCTOP, CCR1ADDR(31), REG_EAX);										// mov  ccr[31],eax
	emit_setcc_m8(DRCTOP, COND_NZ, CF1ADDR(0));												// setnz fcc[0]
	if (mips3.core->flavor >= MIPS3_TYPE_MIPS_IV)
		for (i = 1; i <= 7; i++)
		{
			emit_test_r32_imm(DRCTOP, REG_EAX, 1 << (24 + i));								// test eax,1 << (24+i)
			emit_setcc_m8(DRCTOP, COND_NZ, CF1ADDR(i));										// setnz fcc[i]
		}
	emit_and_r32_imm(DRCTOP, REG_EAX, 3);													// and  eax,3
	emit_test_r32_imm(DRCTOP, REG_EAX, 1);													// test eax,1
	emit_jcc_near_link(DRCTOP, COND_Z, &link1);												// jz   skip
	emit_xor_r32_imm(DRCTOP, REG_EAX, 2);													// xor  eax,2
	resolve_link(DRCTOP, &link1);														// skip:
	drc_append_set_sse_rounding(drc, REG_EAX);												// set_rounding(EAX)
	emit_ret(DRCTOP);																		// ret
}


/*------------------------------------------------------------------
    append_recover_ccr31
------------------------------------------------------------------*/

static void append_recover_ccr31(drc_core *drc)
{
	int i;

	emit_movsxd_r64_m32(DRCTOP, REG_RAX, CCR1ADDR(31));										// movsxd rax,ccr1[rdreg]
	emit_and_r64_imm(DRCTOP, REG_RAX, ~0xfe800000);											// and  rax,~0xfe800000
	emit_xor_r32_r32(DRCTOP, REG_EDX, REG_EDX);												// xor  edx,edx
	emit_cmp_m8_imm(DRCTOP, CF1ADDR(0), 0);													// cmp  fcc[0],0
	emit_setcc_r8(DRCTOP, COND_NZ, REG_DL);													// setnz dl
	emit_shl_r32_imm(DRCTOP, REG_EDX, 23);													// shl  edx,23
	emit_or_r64_r64(DRCTOP, REG_RAX, REG_RDX);												// or   rax,rdx
	if (mips3.core->flavor >= MIPS3_TYPE_MIPS_IV)
		for (i = 1; i <= 7; i++)
		{
			emit_cmp_m8_imm(DRCTOP, CF1ADDR(i), 0);											// cmp  fcc[i],0
			emit_setcc_r8(DRCTOP, COND_NZ, REG_DL);											// setnz dl
			emit_shl_r32_imm(DRCTOP, REG_EDX, 24+i);										// shl  edx,24+i
			emit_or_r64_r64(DRCTOP, REG_RAX, REG_RDX);										// or   rax,rdx
		}
	emit_ret(DRCTOP);																		// ret
}


/*------------------------------------------------------------------
    append_check_interrupts
------------------------------------------------------------------*/

static void append_check_interrupts(drc_core *drc, compiler_state *compiler, const mips3_opcode_desc *desc)
{
	emit_link link1, link2;

	emit_mov_r32_m32(DRCTOP, REG_EAX, CPR0ADDR(COP0_Cause));								// mov  eax,[Cause]
	emit_and_r32_m32(DRCTOP, REG_EAX, CPR0ADDR(COP0_Status));								// and  eax,[Status]
	emit_and_r32_imm(DRCTOP, REG_EAX, 0xfc00);												// and  eax,0xfc00
	emit_jcc_short_link(DRCTOP, COND_Z, &link1);											// jz   skip
	emit_test_m32_imm(DRCTOP, CPR0ADDR(COP0_Status), SR_IE);								// test [Status],SR_IE
	emit_jcc_short_link(DRCTOP, COND_Z, &link2);											// jz   skip
	emit_test_m32_imm(DRCTOP, CPR0ADDR(COP0_Status), SR_EXL | SR_ERL);						// test [Status],SR_EXL | SR_ERL
	if (desc == NULL)
	{
		emit_mov_r32_m32(DRCTOP, REG_P1, MDRC(drc->pcptr));									// mov  p1,[pc]
		emit_jcc(DRCTOP, COND_Z, mips3.drcdata->generate_interrupt_exception);				// jz   generate_interrupt_exception
	}
	else
		oob_request_callback(drc, COND_Z, oob_interrupt_cleanup, compiler, desc, mips3.drcdata->generate_interrupt_exception);
	resolve_link(DRCTOP, &link1);														// skip:
	resolve_link(DRCTOP, &link2);
}



/***************************************************************************
    CORE RECOMPILATION
***************************************************************************/

/*------------------------------------------------------------------
    recompile_instruction
------------------------------------------------------------------*/

static int recompile_instruction(drc_core *drc, compiler_state *compiler, mips3_opcode_desc *desc)
{
	UINT32 op = *desc->opptr;
	emit_link link1;

	switch (op >> 26)
	{
		case 0x00:	/* SPECIAL */
			return recompile_special(drc, compiler, desc);

		case 0x01:	/* REGIMM */
			return recompile_regimm(drc, compiler, desc);

		case 0x02:	/* J */
			append_delay_slot_and_branch(drc, compiler, desc);								// <next instruction + redispatch>
			return TRUE;

		case 0x03:	/* JAL */
			assert(desc->delay != NULL);
			emit_mov_m64_imm(DRCTOP, REGADDR(31), (INT32)(desc->pc + 8));					// mov  [r31],pc + 8
			append_delay_slot_and_branch(drc, compiler, desc);								// <next instruction + redispatch>
			return TRUE;

		case 0x04:	/* BEQ */
		case 0x14:	/* BEQL */
			assert(desc->delay != NULL);
			if (RSREG != RTREG)
			{
				if (RSREG == 0)
				{
					emit_cmp_m64_imm(DRCTOP, REGADDR(RTREG), 0);							// cmp  [rtreg],0
					emit_jcc_near_link(DRCTOP, COND_NE, &link1);							// jne  skip
				}
				else if (RTREG == 0)
				{
					emit_cmp_m64_imm(DRCTOP, REGADDR(RSREG), 0);							// cmp  [rsreg],0
					emit_jcc_near_link(DRCTOP, COND_NE, &link1);							// jne  skip
				}
				else
				{
					emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));						// mov  rax,[rsreg]
					emit_cmp_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));						// cmp  rax,[rtreg]
					emit_jcc_near_link(DRCTOP, COND_NE, &link1);							// jne  skip
				}
			}
			append_delay_slot_and_branch(drc, compiler, desc);								// <next instruction + redispatch>
			if (RSREG != RTREG)
				resolve_link(DRCTOP, &link1);											// skip:
			return TRUE;

		case 0x05:	/* BNE */
		case 0x15:	/* BNEL */
			assert(desc->delay != NULL);
			if (RSREG == RTREG)
				return TRUE;
			else if (RSREG == 0)
			{
				emit_cmp_m64_imm(DRCTOP, REGADDR(RTREG), 0);								// cmp  [rtreg],0
				emit_jcc_near_link(DRCTOP, COND_E, &link1);									// je  skip
			}
			else if (RTREG == 0)
			{
				emit_cmp_m64_imm(DRCTOP, REGADDR(RSREG), 0);								// cmp  [rsreg],0
				emit_jcc_near_link(DRCTOP, COND_E, &link1);									// je  skip
			}
			else
			{
				emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));							// mov  rax,[rsreg]
				emit_cmp_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));							// cmp  rax,[rtreg]
				emit_jcc_near_link(DRCTOP, COND_E, &link1);									// je  skip
			}
			append_delay_slot_and_branch(drc, compiler, desc);								// <next instruction + redispatch>
			resolve_link(DRCTOP, &link1);												// skip:
			return TRUE;

		case 0x06:	/* BLEZ */
		case 0x16:	/* BLEZL */
			assert(desc->delay != NULL);
			if (RSREG != 0)
			{
				emit_cmp_m64_imm(DRCTOP, REGADDR(RSREG), 0);								// cmp  [rsreg],0
				emit_jcc_near_link(DRCTOP, COND_G, &link1);									// jg  skip
			}
			append_delay_slot_and_branch(drc, compiler, desc);								// <next instruction + redispatch>
			if (RSREG != 0)
				resolve_link(DRCTOP, &link1);											// skip:
			return TRUE;

		case 0x07:	/* BGTZ */
		case 0x17:	/* BGTZL */
			if (RSREG == 0)
				return TRUE;
			else
			{
				emit_cmp_m64_imm(DRCTOP, REGADDR(RSREG), 0);								// cmp  [rsreg],0
				emit_jcc_near_link(DRCTOP, COND_LE, &link1);								// jle  skip
			}
			append_delay_slot_and_branch(drc, compiler, desc);								// <next instruction + redispatch>
			resolve_link(DRCTOP, &link1);												// skip:
			return TRUE;

		case 0x08:	/* ADDI */
			if (RSREG != 0)
			{
				emit_mov_r32_m32(DRCTOP, REG_EAX, REGADDR(RSREG));							// mov  eax,[rsreg]
				emit_add_r32_imm(DRCTOP, REG_EAX, SIMMVAL);									// add  eax,SIMMVAL
				oob_request_callback(drc, COND_O, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_overflow_exception);
																							// jo   generate_overflow_exception
				if (RTREG != 0)
				{
					emit_movsxd_r64_r32(DRCTOP, REG_RAX, REG_EAX);							// movsxd rax,eax
					emit_mov_m64_r64(DRCTOP, REGADDR(RTREG), REG_RAX);						// mov  [rtreg],rax
				}
			}
			else if (RTREG != 0)
				emit_mov_m64_imm(DRCTOP, REGADDR(RTREG), SIMMVAL);							// mov  [rtreg],const
			return TRUE;

		case 0x09:	/* ADDIU */
			if (RTREG != 0)
			{
				if (RSREG != 0)
				{
					emit_mov_r32_m32(DRCTOP, REG_EAX, REGADDR(RSREG));						// mov  eax,[rsreg]
					emit_add_r32_imm(DRCTOP, REG_EAX, SIMMVAL);								// add  eax,SIMMVAL
					emit_movsxd_r64_r32(DRCTOP, REG_RAX, REG_EAX);							// movsxd rax,eax
					emit_mov_m64_r64(DRCTOP, REGADDR(RTREG), REG_RAX);						// mov  [rtreg],rax
				}
				else
					emit_mov_m64_imm(DRCTOP, REGADDR(RTREG), SIMMVAL);						// mov  [rtreg],const
			}
			return TRUE;

		case 0x0a:	/* SLTI */
			if (RTREG != 0)
			{
				if (RSREG != 0)
				{
					emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));						// mov  rax,[rsreg]
					emit_sub_r64_imm(DRCTOP, REG_RAX, SIMMVAL);								// sub  rax,SIMMVAL
					emit_shr_r64_imm(DRCTOP, REG_RAX, 63);									// shr  rax,63
					emit_mov_m64_r64(DRCTOP, REGADDR(RTREG), REG_RAX);						// mov  [rtreg],rax
				}
				else
					emit_mov_m64_imm(DRCTOP, REGADDR(RTREG), (0 < SIMMVAL));				// mov  [rtreg],(0 < SIMMVAL)
			}
			return TRUE;

		case 0x0b:	/* SLTIU */
			if (RTREG != 0)
			{
				if (RSREG != 0)
				{
					emit_cmp_m64_imm(DRCTOP, REGADDR(RSREG), SIMMVAL);						// cmp   [rsreg],SIMMVAL
					emit_setcc_r8(DRCTOP, COND_B, REG_AL);									// setb  al
					emit_movsx_r64_r8(DRCTOP, REG_RAX, REG_AL);								// movsx rax,al
					emit_mov_m64_r64(DRCTOP, REGADDR(RTREG), REG_RAX);						// mov  [rtreg],rax
				}
				else
					emit_mov_m64_imm(DRCTOP, REGADDR(RTREG), (0 < (UINT64)SIMMVAL));
																							// mov  [rtreg],(0 < SIMMVAL)
			}
			return TRUE;

		case 0x0c:	/* ANDI */
			if (RTREG != 0)
			{
				if (RSREG == RTREG)
					emit_and_m64_imm(DRCTOP, REGADDR(RTREG), UIMMVAL);						// and  [rtreg],UIMMVAL
				else if (RSREG != 0)
				{
					emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));						// mov  rax,[rsreg]
					emit_and_r64_imm(DRCTOP, REG_EAX, UIMMVAL);								// and  rax,UIMMVAL
					emit_mov_m64_r64(DRCTOP, REGADDR(RTREG), REG_RAX);						// mov  [rtreg],rax
				}
				else
					emit_mov_m64_imm(DRCTOP, REGADDR(RTREG), 0);							// mov  [rtreg],0
			}
			return TRUE;

		case 0x0d:	/* ORI */
			if (RTREG != 0)
			{
				if (RSREG == RTREG)
					emit_or_m64_imm(DRCTOP, REGADDR(RTREG), UIMMVAL);						// or   [rtreg],UIMMVAL
				else if (RSREG != 0)
				{
					emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));						// mov  rax,[rsreg]
					emit_or_r64_imm(DRCTOP, REG_EAX, UIMMVAL);								// or   rax,UIMMVAL
					emit_mov_m64_r64(DRCTOP, REGADDR(RTREG), REG_RAX);						// mov  [rtreg],rax
				}
				else
					emit_mov_m64_imm(DRCTOP, REGADDR(RTREG), UIMMVAL);						// mov  [rtreg],UIMMVAL
			}
			return TRUE;

		case 0x0e:	/* XORI */
			if (RTREG != 0)
			{
				if (RSREG == RTREG)
					emit_xor_m64_imm(DRCTOP, REGADDR(RTREG), UIMMVAL);						// xor  [rtreg],UIMMVAL
				else if (RSREG != 0)
				{
					emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));						// mov  rax,[rsreg]
					emit_xor_r64_imm(DRCTOP, REG_EAX, UIMMVAL);								// xor  rax,UIMMVAL
					emit_mov_m64_r64(DRCTOP, REGADDR(RTREG), REG_RAX);						// mov  [rtreg],rax
				}
				else
					emit_mov_m64_imm(DRCTOP, REGADDR(RTREG), UIMMVAL);						// mov  [rtreg],UIMMVAL
			}
			return TRUE;

		case 0x0f:	/* LUI */
			if (RTREG != 0)
				emit_mov_m64_imm(DRCTOP, REGADDR(RTREG), SIMMVAL << 16);					// mov  [rtreg],const << 16
			return TRUE;

		case 0x10:	/* COP0 */
			return recompile_cop0(drc, compiler, desc);

		case 0x11:	/* COP1 */
			return recompile_cop1(drc, compiler, desc);

		case 0x12:	/* COP2 */
			oob_request_callback(drc, COND_NONE, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_invalidop_exception);
																							// jmp  generate_invalidop_exception
			return TRUE;

		case 0x13:	/* COP1X - R5000 */
			return recompile_cop1x(drc, compiler, desc);

		case 0x18:	/* DADDI */
			if (RSREG != 0)
			{
				emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));							// mov  rax,[rsreg]
				emit_add_r64_imm(DRCTOP, REG_RAX, SIMMVAL);									// add  rax,SIMMVAL
				oob_request_callback(drc, COND_O, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_overflow_exception);
																							// jo   generate_overflow_exception
				if (RTREG != 0)
					emit_mov_m64_r64(DRCTOP, REGADDR(RTREG), REG_RAX);						// mov  [rtreg],rax
			}
			else if (RTREG != 0)
				emit_mov_m64_imm(DRCTOP, REGADDR(RTREG), SIMMVAL);							// mov  [rtreg],const
			return TRUE;

		case 0x19:	/* DADDIU */
			if (RTREG != 0)
			{
				if (RSREG == RTREG)
					emit_add_m64_imm(DRCTOP, REGADDR(RTREG), SIMMVAL);						// add  [rtreg],SIMMVAL
				else if (RSREG != 0)
				{
					emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));						// mov  rax,[rsreg]
					emit_add_r64_imm(DRCTOP, REG_RAX, SIMMVAL);								// add  rax,SIMMVAL
					emit_mov_m64_r64(DRCTOP, REGADDR(RTREG), REG_RAX);						// mov  [rtreg],rax
				}
				else
					emit_mov_m64_imm(DRCTOP, REGADDR(RTREG), SIMMVAL);						// mov  [rtreg],const
			}
			return TRUE;

		case 0x1a:	/* LDL */
			if (RSREG != 0)
			{
				emit_mov_r32_m32(DRCTOP, REG_EAX, REGADDR(RSREG));							// mov  eax,[rsreg]
				if (SIMMVAL != 0)
					emit_add_r32_imm(DRCTOP, REG_EAX, SIMMVAL);								// add  eax,SIMMVAL
			}
			else
				emit_mov_r32_imm(DRCTOP, REG_EAX, SIMMVAL);									// mov  eax,SIMMVAL
			emit_lea_r32_m32(DRCTOP, REG_ECX, MISD(REG_RAX, 8, 0));							// lea  ecx,[rax*8]
			emit_or_r64_imm(DRCTOP, REG_P2, -1);											// or   p2,-1
			if (!mips3.core->bigendian)
				emit_xor_r32_imm(DRCTOP, REG_ECX, 0x38);									// xor  ecx,0x38
			emit_and_r32_imm(DRCTOP, REG_EAX, ~7);											// and  eax,~7
			emit_shr_r64_cl(DRCTOP, REG_P2);												// shr  p2,cl
			emit_mov_m8_r8(DRCTOP, MBD(REG_RSP, SPOFFS_SAVECL), REG_CL);					// mov  [sp+savecl],cl
			emit_not_r64(DRCTOP, REG_P2);													// not  p2
			emit_mov_r32_r32(DRCTOP, REG_P1, REG_EAX);										// mov  p1,eax
			emit_call(DRCTOP, mips3.drcdata->general.read_double_masked);					// call general.read_double_masked
			oob_request_callback(drc, COND_NO_JUMP, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_tlbload_exception);
			if (RTREG != 0)
			{
				emit_mov_r8_m8(DRCTOP, REG_CL, MBD(REG_RSP, SPOFFS_SAVECL));				// mov  cl,[sp+savecl]
				emit_or_r64_imm(DRCTOP, REG_P2, -1);										// or   p2,-1
				emit_shl_r64_cl(DRCTOP, REG_P2);											// shl  p2,cl
				emit_not_r64(DRCTOP, REG_P2);												// not  p2
				emit_shl_r64_cl(DRCTOP, REG_RAX);											// shl  rax,cl
				emit_and_r64_m64(DRCTOP, REG_P2, REGADDR(RTREG));							// and  p2,[rtreg]
				emit_or_r64_r64(DRCTOP, REG_RAX, REG_P2);									// or   rax,p2
				emit_mov_m64_r64(DRCTOP, REGADDR(RTREG), REG_RAX);							// mov  [rtreg],rax
			}
			return TRUE;

		case 0x1b:	/* LDR */
			if (RSREG != 0)
			{
				emit_mov_r32_m32(DRCTOP, REG_EAX, REGADDR(RSREG));							// mov  eax,[rsreg]
				if (SIMMVAL != 0)
					emit_add_r32_imm(DRCTOP, REG_EAX, SIMMVAL);								// add  eax,SIMMVAL
			}
			else
				emit_mov_r32_imm(DRCTOP, REG_EAX, SIMMVAL);									// mov  eax,SIMMVAL
			emit_lea_r32_m32(DRCTOP, REG_ECX, MISD(REG_RAX, 8, 0));							// lea  ecx,[rax*8]
			emit_or_r64_imm(DRCTOP, REG_P2, -1);											// or   p2,-1
			if (mips3.core->bigendian)
				emit_xor_r32_imm(DRCTOP, REG_ECX, 0x38);									// xor  ecx,0x38
			emit_and_r32_imm(DRCTOP, REG_EAX, ~7);											// and  eax,~7
			emit_shl_r64_cl(DRCTOP, REG_P2);												// shl  p2,cl
			emit_mov_m8_r8(DRCTOP, MBD(REG_RSP, SPOFFS_SAVECL), REG_CL);					// mov  [sp+savecl],cl
			emit_not_r64(DRCTOP, REG_P2);													// not  p2
			emit_mov_r32_r32(DRCTOP, REG_P1, REG_EAX);										// mov  p1,eax
			emit_call(DRCTOP, mips3.drcdata->general.read_double_masked);					// call general.read_double_masked
			oob_request_callback(drc, COND_NO_JUMP, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_tlbload_exception);
			if (RTREG != 0)
			{
				emit_mov_r8_m8(DRCTOP, REG_CL, MBD(REG_RSP, SPOFFS_SAVECL));				// mov  cl,[sp+savecl]
				emit_or_r64_imm(DRCTOP, REG_P2, -1);										// or   p2,-1
				emit_shr_r64_cl(DRCTOP, REG_P2);											// shr  p2,cl
				emit_not_r64(DRCTOP, REG_P2);												// not  p2
				emit_shr_r64_cl(DRCTOP, REG_RAX);											// shr  rax,cl
				emit_and_r64_m64(DRCTOP, REG_P2, REGADDR(RTREG));							// and  p2,[rtreg]
				emit_or_r64_r64(DRCTOP, REG_RAX, REG_P2);									// or   rax,p2
				emit_mov_m64_r64(DRCTOP, REGADDR(RTREG), REG_RAX);							// mov  [rtreg],rax
			}
			return TRUE;

		case 0x1c:	/* IDT-specific opcodes: mad/madu/mul on R4640/4650, msub on RC32364 */
#if 0
			switch (op & 0x1f)
			{
				case 0: /* MAD */
					if (RSREG != 0 && RTREG != 0)
					{
						emit_mov_r32_m32(REG_EAX, REGADDR(RSREG));							// mov  eax,[rsreg]
						emit_mov_r32_m32(REG_EDX, REGADDR(RTREG));							// mov  edx,[rtreg]
						emit_imul_r32(DRCTOP, REG_EDX);										// imul edx
						emit_add_r32_m32(DRCTOP, LOADDR);									// add  eax,[lo]
						emit_adc_r32_m32(DRCTOP, HIADDR);									// adc  edx,[hi]
						emit_mov_r32_r32(DRCTOP, REG_EBX, REG_EDX);							// mov  ebx,edx
						emit_cdq(DRCTOP);													// cdq
						emit_mov_m64_r64(DRCTOP, LOADDR, REG_EAX);							// mov  [lo],edx:eax
						emit_mov_r32_r32(DRCTOP, REG_EAX, REG_EBX);							// mov  eax,ebx
						emit_cdq(DRCTOP);													// cdq
						emit_mov_m64_r64(DRCTOP, HIADDR, REG_EAX);							// mov  [hi],edx:eax
					}
					return RECOMPILE_SUCCESSFUL_CP(3,4);

				case 1: /* MADU */
					if (RSREG != 0 && RTREG != 0)
					{
						emit_mov_r32_m32(REG_EAX, REGADDR(RSREG));							// mov  eax,[rsreg]
						emit_mov_r32_m32(REG_EDX, REGADDR(RTREG));							// mov  edx,[rtreg]
						emit_mul_r32(DRCTOP, REG_EDX);										// mul  edx
						emit_add_r32_m32(DRCTOP, LOADDR);									// add  eax,[lo]
						emit_adc_r32_m32(DRCTOP, HIADDR);									// adc  edx,[hi]
						emit_mov_r32_r32(DRCTOP, REG_EBX, REG_EDX);							// mov  ebx,edx
						emit_cdq(DRCTOP);													// cdq
						emit_mov_m64_r64(DRCTOP, LOADDR, REG_EDX, REG_EAX);					// mov  [lo],edx:eax
						emit_mov_r32_r32(DRCTOP, REG_EAX, REG_EBX);							// mov  eax,ebx
						emit_cdq(DRCTOP);													// cdq
						emit_mov_m64_r64(DRCTOP, HIADDR, REG_EDX, REG_EAX);					// mov  [hi],edx:eax
					}
					return RECOMPILE_SUCCESSFUL_CP(3,4);

				case 2: /* MUL */
					if (RDREG != 0)
					{
						if (RSREG != 0 && RTREG != 0)
						{
							emit_mov_r32_m32(REG_EAX, REGADDR(RSREG));						// mov  eax,[rsreg]
							emit_imul_r32_m32(REG_EAX, REGADDR(RTREG));						// imul eax,[rtreg]
							emit_cdq(DRCTOP);												// cdq
							emit_mov_m64_r64(REG_ESI, REGADDR(RDREG), REG_EDX, REG_EAX);	// mov  [rdreg],edx:eax
						}
						else
							emit_zero_m64(REG_ESI, REGADDR(RDREG));
					}
					return RECOMPILE_SUCCESSFUL_CP(3,4);
			}
#endif
			return FALSE;

		case 0x20:	/* LB */
			if (RSREG != 0)
			{
				emit_mov_r32_m32(DRCTOP, REG_P1, REGADDR(RSREG));							// mov  p1,[rsreg]
				if (SIMMVAL != 0)
					emit_add_r32_imm(DRCTOP, REG_P1, SIMMVAL);								// add  p1,SIMMVAL
			}
			else
				emit_mov_r32_imm(DRCTOP, REG_P1, SIMMVAL);									// mov  p1,SIMMVAL
			emit_call(DRCTOP, mips3.drcdata->general.read_byte_signed);						// call general.read_byte_signed
			oob_request_callback(drc, COND_NO_JUMP, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_tlbload_exception);
			if (RTREG != 0)
				emit_mov_m64_r64(DRCTOP, REGADDR(RTREG), REG_RAX);							// mov  [rtreg],rax
			return TRUE;

		case 0x21:	/* LH */
			if (RSREG != 0)
			{
				emit_mov_r32_m32(DRCTOP, REG_P1, REGADDR(RSREG));							// mov  p1,[rsreg]
				if (SIMMVAL != 0)
					emit_add_r32_imm(DRCTOP, REG_P1, SIMMVAL);								// add  p1,SIMMVAL
			}
			else
				emit_mov_r32_imm(DRCTOP, REG_P1, SIMMVAL);									// mov  p1,SIMMVAL
			emit_call(DRCTOP, mips3.drcdata->general.read_word_signed);						// call general.read_word_signed
			oob_request_callback(drc, COND_NO_JUMP, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_tlbload_exception);
			if (RTREG != 0)
				emit_mov_m64_r64(DRCTOP, REGADDR(RTREG), REG_RAX);							// mov  [rtreg],rax
			return TRUE;

		case 0x22:	/* LWL */
			if (RSREG != 0)
			{
				emit_mov_r32_m32(DRCTOP, REG_EAX, REGADDR(RSREG));							// mov  eax,[rsreg]
				if (SIMMVAL != 0)
					emit_add_r32_imm(DRCTOP, REG_EAX, SIMMVAL);								// add  eax,SIMMVAL
			}
			else
				emit_mov_r32_imm(DRCTOP, REG_EAX, SIMMVAL);									// mov  eax,SIMMVAL
			emit_lea_r32_m32(DRCTOP, REG_ECX, MISD(REG_RAX, 8, 0));							// lea  ecx,[rax*8]
			emit_or_r32_imm(DRCTOP, REG_P2, -1);											// or   p2,-1
			if (!mips3.core->bigendian)
				emit_xor_r32_imm(DRCTOP, REG_ECX, 0x18);									// xor  ecx,0x18
			emit_and_r32_imm(DRCTOP, REG_EAX, ~3);											// and  eax,~3
			emit_shr_r32_cl(DRCTOP, REG_P2);												// shr  p2,cl
			emit_mov_m8_r8(DRCTOP, MBD(REG_RSP, SPOFFS_SAVECL), REG_CL);					// mov  [sp+savecl],cl
			emit_not_r32(DRCTOP, REG_P2);													// not  p2
			emit_mov_r32_r32(DRCTOP, REG_P1, REG_EAX);										// mov  p1,eax
			emit_call(DRCTOP, mips3.drcdata->general.read_long_masked);						// call general.read_long_masked
			oob_request_callback(drc, COND_NO_JUMP, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_tlbload_exception);
			if (RTREG != 0)
			{
				emit_mov_r8_m8(DRCTOP, REG_CL, MBD(REG_RSP, SPOFFS_SAVECL));				// mov  cl,[sp+savecl]
				emit_or_r32_imm(DRCTOP, REG_P2, -1);										// or   p2,-1
				emit_shl_r32_cl(DRCTOP, REG_P2);											// shl  p2,cl
				emit_not_r32(DRCTOP, REG_P2);												// not  p2
				emit_shl_r32_cl(DRCTOP, REG_EAX);											// shl  eax,cl
				emit_and_r32_m32(DRCTOP, REG_P2, REGADDR(RTREG));							// and  p2,[rtreg]
				emit_or_r32_r32(DRCTOP, REG_EAX, REG_P2);									// or   eax,p2
				emit_movsxd_r64_r32(DRCTOP, REG_RAX, REG_EAX);								// movsxd rax,eax
				emit_mov_m64_r64(DRCTOP, REGADDR(RTREG), REG_RAX);							// mov  [rtreg],rax
			}
			return TRUE;

		case 0x23:	/* LW */
			if (RSREG != 0)
			{
				emit_mov_r32_m32(DRCTOP, REG_P1, REGADDR(RSREG));							// mov  p1,[rsreg]
				if (SIMMVAL != 0)
					emit_add_r32_imm(DRCTOP, REG_P1, SIMMVAL);								// add  p1,SIMMVAL
			}
			else
				emit_mov_r32_imm(DRCTOP, REG_P1, SIMMVAL);									// mov  p1,SIMMVAL
			emit_call(DRCTOP, mips3.drcdata->general.read_long_signed);						// call general.read_long_signed
			oob_request_callback(drc, COND_NO_JUMP, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_tlbload_exception);
			if (RTREG != 0)
				emit_mov_m64_r64(DRCTOP, REGADDR(RTREG), REG_RAX);							// mov  [rtreg],rax
			return TRUE;

		case 0x24:	/* LBU */
			if (RSREG != 0)
			{
				emit_mov_r32_m32(DRCTOP, REG_P1, REGADDR(RSREG));							// mov  p1,[rsreg]
				if (SIMMVAL != 0)
					emit_add_r32_imm(DRCTOP, REG_P1, SIMMVAL);								// add  p1,SIMMVAL
			}
			else
				emit_mov_r32_imm(DRCTOP, REG_P1, SIMMVAL);									// mov  p1,SIMMVAL
			emit_call(DRCTOP, mips3.drcdata->general.read_byte_unsigned);					// call general.read_byte_unsigned
			oob_request_callback(drc, COND_NO_JUMP, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_tlbload_exception);
			if (RTREG != 0)
				emit_mov_m64_r64(DRCTOP, REGADDR(RTREG), REG_RAX);							// mov  [rtreg],rax
			return TRUE;

		case 0x25:	/* LHU */
			if (RSREG != 0)
			{
				emit_mov_r32_m32(DRCTOP, REG_P1, REGADDR(RSREG));							// mov  p1,[rsreg]
				if (SIMMVAL != 0)
					emit_add_r32_imm(DRCTOP, REG_P1, SIMMVAL);								// add  p1,SIMMVAL
			}
			else
				emit_mov_r32_imm(DRCTOP, REG_P1, SIMMVAL);									// mov  p1,SIMMVAL
			emit_call(DRCTOP, mips3.drcdata->general.read_word_unsigned);					// call general.read_word_unsigned
			oob_request_callback(drc, COND_NO_JUMP, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_tlbload_exception);
			if (RTREG != 0)
				emit_mov_m64_r64(DRCTOP, REGADDR(RTREG), REG_RAX);							// mov  [rtreg],rax
			return TRUE;

		case 0x26:	/* LWR */
			if (RSREG != 0)
			{
				emit_mov_r32_m32(DRCTOP, REG_EAX, REGADDR(RSREG));							// mov  eax,[rsreg]
				if (SIMMVAL != 0)
					emit_add_r32_imm(DRCTOP, REG_EAX, SIMMVAL);								// add  eax,SIMMVAL
			}
			else
				emit_mov_r32_imm(DRCTOP, REG_EAX, SIMMVAL);									// mov  eax,SIMMVAL
			emit_lea_r32_m32(DRCTOP, REG_ECX, MISD(REG_RAX, 8, 0));							// lea  ecx,[rax*8]
			emit_or_r32_imm(DRCTOP, REG_P2, -1);											// or   p2,-1
			if (mips3.core->bigendian)
				emit_xor_r32_imm(DRCTOP, REG_ECX, 0x18);									// xor  ecx,0x18
			emit_and_r32_imm(DRCTOP, REG_EAX, ~3);											// and  eax,~3
			emit_shl_r32_cl(DRCTOP, REG_P2);												// shl  p2,cl
			emit_mov_m8_r8(DRCTOP, MBD(REG_RSP, SPOFFS_SAVECL), REG_CL);					// mov  [sp+32],savecl
			emit_not_r32(DRCTOP, REG_P2);													// not  p2
			emit_mov_r32_r32(DRCTOP, REG_P1, REG_EAX);										// mov  p1,eax
			emit_call(DRCTOP, mips3.drcdata->general.read_long_masked);						// call general.read_long_masked
			oob_request_callback(drc, COND_NO_JUMP, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_tlbload_exception);
			if (RTREG != 0)
			{
				emit_mov_r8_m8(DRCTOP, REG_CL, MBD(REG_RSP, SPOFFS_SAVECL));				// mov  cl,[sp+savecl]
				emit_or_r32_imm(DRCTOP, REG_P2, -1);										// or   p2,-1
				emit_shr_r32_cl(DRCTOP, REG_P2);											// shr  p2,cl
				emit_not_r32(DRCTOP, REG_P2);												// not  p2
				emit_shr_r32_cl(DRCTOP, REG_EAX);											// shr  eax,cl
				emit_and_r32_m32(DRCTOP, REG_P2, REGADDR(RTREG));							// and  p2,[rtreg]
				emit_or_r32_r32(DRCTOP, REG_EAX, REG_P2);									// or   eax,p2
				emit_movsxd_r64_r32(DRCTOP, REG_RAX, REG_EAX);								// movsxd rax,eax
				emit_mov_m64_r64(DRCTOP, REGADDR(RTREG), REG_RAX);							// mov  [rtreg],rax
			}
			return TRUE;

		case 0x27:	/* LWU */
			if (RSREG != 0)
			{
				emit_mov_r32_m32(DRCTOP, REG_P1, REGADDR(RSREG));							// mov  p1,[rsreg]
				if (SIMMVAL != 0)
					emit_add_r32_imm(DRCTOP, REG_P1, SIMMVAL);								// add  p1,SIMMVAL
			}
			else
				emit_mov_r32_imm(DRCTOP, REG_P1, SIMMVAL);									// mov  p1,SIMMVAL
			emit_call(DRCTOP, mips3.drcdata->general.read_long_unsigned);					// call general.read_long_unsigned
			oob_request_callback(drc, COND_NO_JUMP, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_tlbload_exception);
			if (RTREG != 0)
				emit_mov_m64_r64(DRCTOP, REGADDR(RTREG), REG_RAX);							// mov  [rtreg],rax
			return TRUE;

		case 0x28:	/* SB */
			if (RSREG != 0)
			{
				emit_mov_r32_m32(DRCTOP, REG_P1, REGADDR(RSREG));							// mov  p1,[rsreg]
				if (SIMMVAL != 0)
					emit_add_r32_imm(DRCTOP, REG_P1, SIMMVAL);								// add  p1,SIMMVAL
			}
			else
				emit_mov_r32_imm(DRCTOP, REG_P1, SIMMVAL);									// mov  p1,SIMMVAL
			if (RTREG != 0)
				emit_movzx_r64_m8(DRCTOP, REG_P2, REGADDR(RTREG));							// movzx p2,byte ptr [rtreg]
			else
				emit_xor_r32_r32(DRCTOP, REG_P2, REG_P2);									// xor  p2,p2
			emit_call(DRCTOP, mips3.drcdata->general.write_byte);							// call general.write_byte
			oob_request_callback(drc, COND_NO_JUMP, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_tlbstore_exception);
			return TRUE;

		case 0x29:	/* SH */
			if (RSREG != 0)
			{
				emit_mov_r32_m32(DRCTOP, REG_P1, REGADDR(RSREG));							// mov  p1,[rsreg]
				if (SIMMVAL != 0)
					emit_add_r32_imm(DRCTOP, REG_P1, SIMMVAL);								// add  p1,SIMMVAL
			}
			else
				emit_mov_r32_imm(DRCTOP, REG_P1, SIMMVAL);									// mov  p1,SIMMVAL
			if (RTREG != 0)
				emit_movzx_r64_m16(DRCTOP, REG_P2, REGADDR(RTREG));							// movzx p2,word ptr [rtreg]
			else
				emit_xor_r32_r32(DRCTOP, REG_P2, REG_P2);									// xor  p2,p2
			emit_call(DRCTOP, mips3.drcdata->general.write_word);							// call general.write_word
			oob_request_callback(drc, COND_NO_JUMP, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_tlbstore_exception);
			return TRUE;

		case 0x2a:	/* SWL */
			if (RSREG != 0)
			{
				emit_mov_r32_m32(DRCTOP, REG_EAX, REGADDR(RSREG));							// mov  eax,[rsreg]
				if (SIMMVAL != 0)
					emit_add_r32_imm(DRCTOP, REG_EAX, SIMMVAL);								// add  eax,SIMMVAL
			}
			else
				emit_mov_r32_imm(DRCTOP, REG_EAX, SIMMVAL);									// mov  eax,SIMMVAL
			emit_or_r32_imm(DRCTOP, REG_P3, -1);											// or   p3,-1
			emit_lea_r32_m32(DRCTOP, REG_ECX, MISD(REG_RAX, 8, 0));							// lea  ecx,[rax*8]
			if (RTREG != 0)
				emit_mov_r32_m32(DRCTOP, REG_P2, REGADDR(RTREG));							// mov  p2,[rtreg]
			else
				emit_xor_r32_r32(DRCTOP, REG_P2, REG_P2);									// xor  p2,p2
			if (!mips3.core->bigendian)
				emit_xor_r32_imm(DRCTOP, REG_ECX, 0x18);									// xor  ecx,0x18
			emit_and_r32_imm(DRCTOP, REG_EAX, ~3);											// and  eax,~3
			emit_shr_r32_cl(DRCTOP, REG_P3);												// shr  p3,cl
			emit_shr_r32_cl(DRCTOP, REG_P2);												// shr  p2,cl
			emit_mov_r32_r32(DRCTOP, REG_P1, REG_EAX);										// mov  p1,eax
			emit_not_r32(DRCTOP, REG_P3);													// not  p3
			emit_call(DRCTOP, mips3.drcdata->general.write_long_masked);					// call general.write_long_masked
			oob_request_callback(drc, COND_NO_JUMP, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_tlbstore_exception);
			return TRUE;

		case 0x2b:	/* SW */
			if (RSREG != 0)
			{
				emit_mov_r32_m32(DRCTOP, REG_P1, REGADDR(RSREG));							// mov  p1,[rsreg]
				if (SIMMVAL != 0)
					emit_add_r32_imm(DRCTOP, REG_P1, SIMMVAL);								// add  p1,SIMMVAL
			}
			else
				emit_mov_r32_imm(DRCTOP, REG_P1, SIMMVAL);									// mov  p1,SIMMVAL
			if (RTREG != 0)
				emit_mov_r32_m32(DRCTOP, REG_P2, REGADDR(RTREG));							// mov  p2,[rtreg]
			else
				emit_xor_r32_r32(DRCTOP, REG_P2, REG_P2);									// xor  p2,p2
			emit_call(DRCTOP, mips3.drcdata->general.write_long);							// call general.write_long
			oob_request_callback(drc, COND_NO_JUMP, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_tlbstore_exception);
			return TRUE;

		case 0x2c:	/* SDL */
			if (RSREG != 0)
			{
				emit_mov_r32_m32(DRCTOP, REG_EAX, REGADDR(RSREG));							// mov  eax,[rsreg]
				if (SIMMVAL != 0)
					emit_add_r32_imm(DRCTOP, REG_EAX, SIMMVAL);								// add  eax,SIMMVAL
			}
			else
				emit_mov_r32_imm(DRCTOP, REG_EAX, SIMMVAL);									// mov  eax,SIMMVAL
			emit_or_r64_imm(DRCTOP, REG_P3, -1);											// or   p3,-1
			emit_lea_r32_m32(DRCTOP, REG_ECX, MISD(REG_RAX, 8, 0));							// lea  ecx,[rax*8]
			if (RTREG != 0)
				emit_mov_r64_m64(DRCTOP, REG_P2, REGADDR(RTREG));							// mov  p2,[rtreg]
			else
				emit_xor_r32_r32(DRCTOP, REG_P2, REG_P2);									// xor  p2,p2
			if (!mips3.core->bigendian)
				emit_xor_r32_imm(DRCTOP, REG_ECX, 0x38);									// xor  ecx,0x38
			emit_and_r32_imm(DRCTOP, REG_EAX, ~7);											// and  eax,~7
			emit_shr_r64_cl(DRCTOP, REG_P3);												// shr  p3,cl
			emit_shr_r64_cl(DRCTOP, REG_P2);												// shr  p2,cl
			emit_mov_r32_r32(DRCTOP, REG_P1, REG_EAX);										// mov  p1,eax
			emit_not_r64(DRCTOP, REG_P3);													// not  p3
			emit_call(DRCTOP, mips3.drcdata->general.write_double_masked);					// call general.write_double_masked
			oob_request_callback(drc, COND_NO_JUMP, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_tlbstore_exception);
			return TRUE;

		case 0x2d:	/* SDR */
			if (RSREG != 0)
			{
				emit_mov_r32_m32(DRCTOP, REG_EAX, REGADDR(RSREG));							// mov  eax,[rsreg]
				if (SIMMVAL != 0)
					emit_add_r32_imm(DRCTOP, REG_EAX, SIMMVAL);								// add  eax,SIMMVAL
			}
			else
				emit_mov_r32_imm(DRCTOP, REG_EAX, SIMMVAL);									// mov  eax,SIMMVAL
			emit_or_r64_imm(DRCTOP, REG_P3, -1);											// or   p3,-1
			emit_lea_r32_m32(DRCTOP, REG_ECX, MISD(REG_RAX, 8, 0));							// lea  ecx,[rax*8]
			if (RTREG != 0)
				emit_mov_r64_m64(DRCTOP, REG_P2, REGADDR(RTREG));							// mov  p2,[rtreg]
			else
				emit_xor_r32_r32(DRCTOP, REG_P2, REG_P2);									// xor  p2,p2
			if (mips3.core->bigendian)
				emit_xor_r32_imm(DRCTOP, REG_ECX, 0x38);									// xor  ecx,0x38
			emit_and_r32_imm(DRCTOP, REG_EAX, ~7);											// and  eax,~7
			emit_shl_r64_cl(DRCTOP, REG_P3);												// shl  p3,cl
			emit_shl_r64_cl(DRCTOP, REG_P2);												// shl  p2,cl
			emit_mov_r32_r32(DRCTOP, REG_P1, REG_EAX);										// mov  p1,eax
			emit_not_r64(DRCTOP, REG_P3);													// not  p3
			emit_call(DRCTOP, mips3.drcdata->general.write_double_masked);					// call general.write_double_masked
			oob_request_callback(drc, COND_NO_JUMP, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_tlbstore_exception);
			return TRUE;

		case 0x2e:	/* SWR */
			if (RSREG != 0)
			{
				emit_mov_r32_m32(DRCTOP, REG_EAX, REGADDR(RSREG));							// mov  eax,[rsreg]
				if (SIMMVAL != 0)
					emit_add_r32_imm(DRCTOP, REG_EAX, SIMMVAL);								// add  eax,SIMMVAL
			}
			else
				emit_mov_r32_imm(DRCTOP, REG_EAX, SIMMVAL);									// mov  eax,SIMMVAL
			emit_or_r32_imm(DRCTOP, REG_P3, -1);											// or   p3,-1
			emit_lea_r32_m32(DRCTOP, REG_ECX, MISD(REG_RAX, 8, 0));							// lea  ecx,[rax*8]
			if (RTREG != 0)
				emit_mov_r32_m32(DRCTOP, REG_P2, REGADDR(RTREG));							// mov  p2,[rtreg]
			else
				emit_xor_r32_r32(DRCTOP, REG_P2, REG_P2);									// xor  p2,p2
			if (mips3.core->bigendian)
				emit_xor_r32_imm(DRCTOP, REG_ECX, 0x18);									// xor  ecx,0x18
			emit_and_r32_imm(DRCTOP, REG_EAX, ~3);											// and  eax,~3
			emit_shl_r32_cl(DRCTOP, REG_P3);												// shl  p3,cl
			emit_shl_r32_cl(DRCTOP, REG_P2);												// shl  p2,cl
			emit_mov_r32_r32(DRCTOP, REG_P1, REG_EAX);										// mov  p1,eax
			emit_not_r32(DRCTOP, REG_P3);													// not  p3
			emit_call(DRCTOP, mips3.drcdata->general.write_long_masked);					// call general.write_long_masked
			oob_request_callback(drc, COND_NO_JUMP, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_tlbstore_exception);
			return TRUE;

		case 0x2f:	/* CACHE */
			return TRUE;

//      case 0x30:  /* LL */        logerror("mips3 Unhandled op: LL\n");                                   break;

		case 0x31:	/* LWC1 */
			if (RSREG != 0)
			{
				emit_mov_r32_m32(DRCTOP, REG_P1, REGADDR(RSREG));							// mov  p1,[rsreg]
				if (SIMMVAL != 0)
					emit_add_r32_imm(DRCTOP, REG_P1, SIMMVAL);								// add  p1,SIMMVAL
			}
			else
				emit_mov_r32_imm(DRCTOP, REG_P1, SIMMVAL);									// mov  p1,SIMMVAL
			emit_call(DRCTOP, mips3.drcdata->general.read_long_unsigned);					// call general.read_long_unsigned
			oob_request_callback(drc, COND_NO_JUMP, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_tlbload_exception);
			emit_mov_m32_r32(DRCTOP, FPR32ADDR(RTREG), REG_EAX);							// mov  [rtreg],eax
			return TRUE;

		case 0x32:	/* LWC2 */
			if (RSREG != 0)
			{
				emit_mov_r32_m32(DRCTOP, REG_P1, REGADDR(RSREG));							// mov  p1,[rsreg]
				if (SIMMVAL != 0)
					emit_add_r32_imm(DRCTOP, REG_P1, SIMMVAL);								// add  p1,SIMMVAL
			}
			else
				emit_mov_r32_imm(DRCTOP, REG_P1, SIMMVAL);									// mov  p1,SIMMVAL
			emit_call(DRCTOP, mips3.drcdata->general.read_long_unsigned);					// call general.read_long_unsigned
			oob_request_callback(drc, COND_NO_JUMP, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_tlbload_exception);
			emit_mov_m32_r32(DRCTOP, CPR2ADDR(RTREG), REG_EAX);								// mov  [rtreg],eax
			return TRUE;

		case 0x33:	/* PREF */
			return TRUE;

//      case 0x34:  /* LLD */       logerror("mips3 Unhandled op: LLD\n");                                  break;

		case 0x35:	/* LDC1 */
			if (RSREG != 0)
			{
				emit_mov_r32_m32(DRCTOP, REG_P1, REGADDR(RSREG));							// mov  p1,[rsreg]
				if (SIMMVAL != 0)
					emit_add_r32_imm(DRCTOP, REG_P1, SIMMVAL);								// add  p1,SIMMVAL
			}
			else
				emit_mov_r32_imm(DRCTOP, REG_P1, SIMMVAL);									// mov  p1,SIMMVAL
			emit_call(DRCTOP, mips3.drcdata->general.read_double);							// call general.read_double
			oob_request_callback(drc, COND_NO_JUMP, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_tlbload_exception);
			emit_mov_m64_r64(DRCTOP, FPR64ADDR(RTREG), REG_RAX);							// mov  [rtreg],rax
			return TRUE;

		case 0x36:	/* LDC2 */
			if (RSREG != 0)
			{
				emit_mov_r32_m32(DRCTOP, REG_P1, REGADDR(RSREG));							// mov  p1,[rsreg]
				if (SIMMVAL != 0)
					emit_add_r32_imm(DRCTOP, REG_P1, SIMMVAL);								// add  p1,SIMMVAL
			}
			else
				emit_mov_r32_imm(DRCTOP, REG_P1, SIMMVAL);									// mov  p1,SIMMVAL
			emit_call(DRCTOP, mips3.drcdata->general.read_double);							// call general.read_double
			oob_request_callback(drc, COND_NO_JUMP, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_tlbload_exception);
			emit_mov_m64_r64(DRCTOP, CPR2ADDR(RTREG), REG_RAX);								// mov  [rtreg],rax
			return TRUE;

		case 0x37:	/* LD */
			if (RSREG != 0)
			{
				emit_mov_r32_m32(DRCTOP, REG_P1, REGADDR(RSREG));							// mov  p1,[rsreg]
				if (SIMMVAL != 0)
					emit_add_r32_imm(DRCTOP, REG_P1, SIMMVAL);								// add  p1,SIMMVAL
			}
			else
				emit_mov_r32_imm(DRCTOP, REG_P1, SIMMVAL);									// mov  p1,SIMMVAL
			emit_call(DRCTOP, mips3.drcdata->general.read_double);							// call general.read_double
			oob_request_callback(drc, COND_NO_JUMP, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_tlbload_exception);
			if (RTREG != 0)
				emit_mov_m64_r64(DRCTOP, REGADDR(RTREG), REG_RAX);							// mov  [rtreg],rax
			return TRUE;

//      case 0x38:  /* SC */        logerror("mips3 Unhandled op: SC\n");                                   break;

		case 0x39:	/* SWC1 */
			if (RSREG != 0)
			{
				emit_mov_r32_m32(DRCTOP, REG_P1, REGADDR(RSREG));							// mov  p1,[rsreg]
				if (SIMMVAL != 0)
					emit_add_r32_imm(DRCTOP, REG_P1, SIMMVAL);								// add  p1,SIMMVAL
			}
			else
				emit_mov_r32_imm(DRCTOP, REG_P1, SIMMVAL);									// mov  p1,SIMMVAL
			emit_mov_r32_m32(DRCTOP, REG_P2, FPR32ADDR(RTREG));								// mov  p2,[rtreg]
			emit_call(DRCTOP, mips3.drcdata->general.write_long);							// call general.write_long
			oob_request_callback(drc, COND_NO_JUMP, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_tlbstore_exception);
			return TRUE;

		case 0x3a:	/* SWC2 */
			if (RSREG != 0)
			{
				emit_mov_r32_m32(DRCTOP, REG_P1, REGADDR(RSREG));							// mov  p1,[rsreg]
				if (SIMMVAL != 0)
					emit_add_r32_imm(DRCTOP, REG_P1, SIMMVAL);								// add  p1,SIMMVAL
			}
			else
				emit_mov_r32_imm(DRCTOP, REG_P1, SIMMVAL);									// mov  p1,SIMMVAL
			emit_mov_r32_m32(DRCTOP, REG_P2, CPR2ADDR(RTREG));								// mov  p2,[rtreg]
			emit_call(DRCTOP, mips3.drcdata->general.write_long);							// call general.write_long
			oob_request_callback(drc, COND_NO_JUMP, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_tlbstore_exception);
			return TRUE;

//      case 0x3b:  /* SWC3 */      invalid_instruction(op);                                                break;
//      case 0x3c:  /* SCD */       logerror("mips3 Unhandled op: SCD\n");                                  break;

		case 0x3d:	/* SDC1 */
			if (RSREG != 0)
			{
				emit_mov_r32_m32(DRCTOP, REG_P1, REGADDR(RSREG));							// mov  p1,[rsreg]
				if (SIMMVAL != 0)
					emit_add_r32_imm(DRCTOP, REG_P1, SIMMVAL);								// add  p1,SIMMVAL
			}
			else
				emit_mov_r32_imm(DRCTOP, REG_P1, SIMMVAL);									// mov  p1,SIMMVAL
			emit_mov_r64_m64(DRCTOP, REG_P2, FPR64ADDR(RTREG));								// mov  p2,[rtreg]
			emit_call(DRCTOP, mips3.drcdata->general.write_double);							// call general.write_double
			oob_request_callback(drc, COND_NO_JUMP, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_tlbstore_exception);
			return TRUE;

		case 0x3e:	/* SDC2 */
			if (RSREG != 0)
			{
				emit_mov_r32_m32(DRCTOP, REG_P1, REGADDR(RSREG));							// mov  p1,[rsreg]
				if (SIMMVAL != 0)
					emit_add_r32_imm(DRCTOP, REG_P1, SIMMVAL);								// add  p1,SIMMVAL
			}
			else
				emit_mov_r32_imm(DRCTOP, REG_P1, SIMMVAL);									// mov  p1,SIMMVAL
			emit_mov_r64_m64(DRCTOP, REG_P2, CPR2ADDR(RTREG));								// mov  p2,[rtreg]
			emit_call(DRCTOP, mips3.drcdata->general.write_double);							// call general.write_double
			oob_request_callback(drc, COND_NO_JUMP, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_tlbstore_exception);
			return TRUE;

		case 0x3f:	/* SD */
			if (RSREG != 0)
			{
				emit_mov_r32_m32(DRCTOP, REG_P1, REGADDR(RSREG));							// mov  p1,[rsreg]
				if (SIMMVAL != 0)
					emit_add_r32_imm(DRCTOP, REG_P1, SIMMVAL);								// add  p1,SIMMVAL
			}
			else
				emit_mov_r32_imm(DRCTOP, REG_P1, SIMMVAL);									// mov  p1,SIMMVAL
			if (RTREG != 0)
				emit_mov_r64_m64(DRCTOP, REG_P2, REGADDR(RTREG));							// mov  p2,[rtreg]
			else
				emit_xor_r32_r32(DRCTOP, REG_P2, REG_P2);									// xor  p2,p2
			emit_call(DRCTOP, mips3.drcdata->general.write_double);							// call general.write_double
			oob_request_callback(drc, COND_NO_JUMP, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_tlbstore_exception);
			return TRUE;

//      default:    /* ??? */       invalid_instruction(op);                                                break;
	}

	return FALSE;
}


/*------------------------------------------------------------------
    recompile_special
------------------------------------------------------------------*/

static int recompile_special1(drc_core *drc, compiler_state *compiler, mips3_opcode_desc *desc)
{
	UINT32 op = *desc->opptr;
	emit_link link1;

	switch (op & 63)
	{
		case 0x00:	/* SLL - MIPS I */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					emit_mov_r32_m32(DRCTOP, REG_EAX, REGADDR(RTREG));						// mov  eax,[rtreg]
					if (SHIFT != 0)
						emit_shl_r32_imm(DRCTOP, REG_EAX, SHIFT);							// shl  eax,SHIFT
					emit_movsxd_r64_r32(DRCTOP, REG_RAX, REG_EAX);							// movsxd rax,eax
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else
					emit_mov_m64_imm(DRCTOP, REGADDR(RDREG), 0);							// mov  [rdreg],0
			}
			return TRUE;

		case 0x01:	/* MOVF - MIPS IV */
			emit_cmp_m8_imm(DRCTOP, CF1ADDR((op >> 18) & 7), 0);							// cmp  fcc[x],0
			emit_jcc_short_link(DRCTOP, ((op >> 16) & 1) ? COND_Z : COND_NZ, &link1);		// jz/nz skip
			emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));								// mov  rax,[rsreg]
			emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);								// mov  [rdreg],rax
			resolve_link(DRCTOP, &link1);												// skip:
			return TRUE;

		case 0x02:	/* SRL - MIPS I */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					emit_mov_r32_m32(DRCTOP, REG_EAX, REGADDR(RTREG));						// mov  eax,[rtreg]
					if (SHIFT != 0)
						emit_shr_r32_imm(DRCTOP, REG_EAX, SHIFT);							// shr  eax,SHIFT
					emit_movsxd_r64_r32(DRCTOP, REG_RAX, REG_EAX);							// movsxd rax,eax
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else
					emit_mov_m64_imm(DRCTOP, REGADDR(RDREG), 0);							// mov  [rdreg],0
			}
			return TRUE;

		case 0x03:	/* SRA - MIPS I */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					emit_mov_r32_m32(DRCTOP, REG_EAX, REGADDR(RTREG));						// mov  eax,[rtreg]
					if (SHIFT != 0)
						emit_sar_r32_imm(DRCTOP, REG_EAX, SHIFT);							// sar  eax,SHIFT
					emit_movsxd_r64_r32(DRCTOP, REG_RAX, REG_EAX);							// movsxd rax,eax
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else
					emit_mov_m64_imm(DRCTOP, REGADDR(RDREG), 0);							// mov  [rdreg],0
			}
			return TRUE;

		case 0x04:	/* SLLV - MIPS I */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					emit_mov_r32_m32(DRCTOP, REG_EAX, REGADDR(RTREG));						// mov  eax,[rtreg]
					if (RSREG != 0)
					{
						emit_mov_r8_m8(DRCTOP, REG_CL, REGADDR(RSREG));						// mov  cl,[rsreg]
						emit_shl_r32_cl(DRCTOP, REG_EAX);									// shl  eax,cl
					}
					emit_movsxd_r64_r32(DRCTOP, REG_RAX, REG_EAX);							// movsxd rax,eax
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else
					emit_mov_m64_imm(DRCTOP, REGADDR(RDREG), 0);							// mov  [rdreg],0
			}
			return TRUE;

		case 0x06:	/* SRLV - MIPS I */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					emit_mov_r32_m32(DRCTOP, REG_EAX, REGADDR(RTREG));						// mov  eax,[rtreg]
					if (RSREG != 0)
					{
						emit_mov_r8_m8(DRCTOP, REG_CL, REGADDR(RSREG));						// mov  cl,[rsreg]
						emit_shr_r32_cl(DRCTOP, REG_EAX);									// shr  eax,cl
					}
					emit_movsxd_r64_r32(DRCTOP, REG_RAX, REG_EAX);							// movsxd rax,eax
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else
					emit_mov_m64_imm(DRCTOP, REGADDR(RDREG), 0);							// mov  [rdreg],0
			}
			return TRUE;

		case 0x07:	/* SRAV - MIPS I */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					emit_mov_r32_m32(DRCTOP, REG_EAX, REGADDR(RTREG));						// mov  eax,[rtreg]
					if (RSREG != 0)
					{
						emit_mov_r8_m8(DRCTOP, REG_CL, REGADDR(RSREG));						// mov  cl,[rsreg]
						emit_sar_r32_cl(DRCTOP, REG_EAX);									// sar  eax,cl
					}
					emit_movsxd_r64_r32(DRCTOP, REG_RAX, REG_EAX);							// movsxd rax,eax
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else
					emit_mov_m64_imm(DRCTOP, REGADDR(RDREG), 0);							// mov  [rdreg],0
			}
			return TRUE;

		case 0x08:	/* JR - MIPS I */
			assert(desc->delay != NULL);
			emit_mov_r32_m32(DRCTOP, REG_EAX, REGADDR(RSREG));								// mov  eax,[rsreg]
			emit_mov_m32_r32(DRCTOP, MBD(REG_RSP, SPOFFS_NEXTPC), REG_EAX);					// mov  [esp+nextpc],eax
			append_delay_slot_and_branch(drc, compiler, desc);								// <next instruction + redispatch>
			return TRUE;

		case 0x09:	/* JALR - MIPS I */
			assert(desc->delay != NULL);
			emit_mov_r32_m32(DRCTOP, REG_EAX, REGADDR(RSREG));								// mov  eax,[rsreg]
			emit_mov_m32_r32(DRCTOP, MBD(REG_RSP, SPOFFS_NEXTPC), REG_EAX);					// mov  [esp+nextpc],eax
			if (RDREG != 0)
				emit_mov_m64_imm(DRCTOP, REGADDR(RDREG), (INT32)(desc->pc + 8));			// mov  [rdreg],pc + 8
			append_delay_slot_and_branch(drc, compiler, desc);								// <next instruction + redispatch>
			return TRUE;

		case 0x0a:	/* MOVZ - MIPS IV */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					emit_cmp_m64_imm(DRCTOP, REGADDR(RTREG), 0);							// cmp  [rtreg],0
					emit_jcc_short_link(DRCTOP, COND_NE, &link1);							// jne  skip
				}
				emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));							// mov  rax,[rsreg]
				emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);							// mov  [rdreg],rax
				if (RTREG != 0)
					resolve_link(DRCTOP, &link1);										// skip:
			}
			return TRUE;

		case 0x0b:	/* MOVN - MIPS IV */
			if (RDREG != 0 && RTREG != 0)
			{
				emit_cmp_m64_imm(DRCTOP, REGADDR(RTREG), 0);								// cmp  [rtreg],0
				emit_jcc_short_link(DRCTOP, COND_E, &link1);								// je   skip
				emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));							// mov  rax,[rsreg]
				emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);							// mov  [rdreg],rax
				resolve_link(DRCTOP, &link1);											// skip:
			}
			return TRUE;

		case 0x0c:	/* SYSCALL - MIPS I */
			oob_request_callback(drc, COND_NONE, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_syscall_exception);
																							// jmp  generate_syscall_exception
			return TRUE;

		case 0x0d:	/* BREAK - MIPS I */
			oob_request_callback(drc, COND_NONE, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_break_exception);
																							// jmp  generate_break_exception
			return TRUE;

		case 0x0f:	/* SYNC - MIPS II */
			return TRUE;

		case 0x10:	/* MFHI - MIPS I */
			if (RDREG != 0)
			{
				emit_mov_r64_m64(DRCTOP, REG_RAX, HIADDR);									// mov  rax,[hi]
				emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);							// mov  [rdreg],rax
			}
			return TRUE;

		case 0x11:	/* MTHI - MIPS I */
			if (RSREG != 0)
			{
				emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));							// mov  rax,[rsreg]
				emit_mov_m64_r64(DRCTOP, HIADDR, REG_RAX);									// mov  [hi],rax
			}
			else
				emit_mov_m64_imm(DRCTOP, HIADDR, 0);										// mov  [hi],0
			return TRUE;

		case 0x12:	/* MFLO - MIPS I */
			if (RDREG != 0)
			{
				emit_mov_r64_m64(DRCTOP, REG_RAX, LOADDR);									// mov  rax,[lo]
				emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);							// mov  [rdreg],rax
			}
			return TRUE;

		case 0x13:	/* MTLO - MIPS I */
			if (RSREG != 0)
			{
				emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));							// mov  rax,[rsreg]
				emit_mov_m64_r64(DRCTOP, LOADDR, REG_RAX);									// mov  [lo],rax
			}
			else
				emit_mov_m64_imm(DRCTOP, LOADDR, 0);										// mov  [lo],0
			return TRUE;

		case 0x14:	/* DSLLV - MIPS III */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					if (RSREG != 0)
					{
						emit_mov_r8_m8(DRCTOP, REG_CL, REGADDR(RSREG));						// mov  cl,[rsreg]
						if (RTREG == RDREG)
							emit_shl_m64_cl(DRCTOP, REGADDR(RDREG));						// shl  [rdreg],cl
						else
						{
							emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));				// mov  rax,[rtreg]
							emit_shl_r64_cl(DRCTOP, REG_RAX);								// shl  rax,cl
							emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);				// mov  [rdreg],rax
						}
					}
					else if (RTREG != RDREG)
					{
						emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));					// mov  rax,[rtreg]
						emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);					// mov  [rdreg],rax
					}
				}
				else
					emit_mov_m64_imm(DRCTOP, REGADDR(RDREG), 0);							// mov  [rdreg],0
			}
			return TRUE;

		case 0x16:	/* DSRLV - MIPS III */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					if (RSREG != 0)
					{
						emit_mov_r8_m8(DRCTOP, REG_CL, REGADDR(RSREG));						// mov  cl,[rsreg]
						if (RTREG == RDREG)
							emit_shr_m64_cl(DRCTOP, REGADDR(RDREG));						// shr  [rdreg],cl
						else
						{
							emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));				// mov  rax,[rtreg]
							emit_shr_r64_cl(DRCTOP, REG_RAX);								// shr  rax,cl
							emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);				// mov  [rdreg],rax
						}
					}
					else if (RTREG != RDREG)
					{
						emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));					// mov  rax,[rtreg]
						emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);					// mov  [rdreg],rax
					}
				}
				else
					emit_mov_m64_imm(DRCTOP, REGADDR(RDREG), 0);							// mov  [rdreg],0
			}
			return TRUE;

		case 0x17:	/* DSRAV - MIPS III */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					if (RSREG != 0)
					{
						emit_mov_r8_m8(DRCTOP, REG_CL, REGADDR(RSREG));						// mov  cl,[rsreg]
						if (RTREG == RDREG)
							emit_sar_m64_cl(DRCTOP, REGADDR(RDREG));						// sar  [rdreg],cl
						else
						{
							emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));				// mov  rax,[rtreg]
							emit_sar_r64_cl(DRCTOP, REG_RAX);								// sar  rax,cl
							emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);				// mov  [rdreg],rax
						}
					}
					else if (RTREG != RDREG)
					{
						emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));					// mov  rax,[rtreg]
						emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);					// mov  [rdreg],rax
					}
				}
				else
					emit_mov_m64_imm(DRCTOP, REGADDR(RDREG), 0);							// mov  [rdreg],0
			}
			return TRUE;

		case 0x18:	/* MULT - MIPS I */
			if (RSREG == 0 || RTREG == 0)
			{
				emit_mov_m64_imm(DRCTOP, HIADDR, 0);										// mov  [hi],0
				emit_mov_m64_imm(DRCTOP, LOADDR, 0);										// mov  [lo],0
			}
			else
			{
				emit_mov_r32_m32(DRCTOP, REG_EAX, REGADDR(RTREG));							// mov  eax,[rtreg]
				emit_imul_m32(DRCTOP, REGADDR(RSREG));										// imul [rsreg]
				emit_movsxd_r64_r32(DRCTOP, REG_RAX, REG_EAX);								// movsxd rax,eax
				emit_movsxd_r64_r32(DRCTOP, REG_RDX, REG_EDX);								// movsxd rdx,edx
				emit_mov_m64_r64(DRCTOP, LOADDR, REG_RAX);									// mov  [lo],rax
				emit_mov_m64_r64(DRCTOP, HIADDR, REG_RDX);									// mov  [hi],rdx
			}
			return TRUE;

		case 0x19:	/* MULTU - MIPS I */
			if (RSREG == 0 || RTREG == 0)
			{
				emit_mov_m64_imm(DRCTOP, HIADDR, 0);										// mov  [hi],0
				emit_mov_m64_imm(DRCTOP, LOADDR, 0);										// mov  [lo],0
			}
			else
			{
				emit_mov_r32_m32(DRCTOP, REG_EAX, REGADDR(RTREG));							// mov  eax,[rtreg]
				emit_mul_m32(DRCTOP, REGADDR(RSREG));										// mul  [rsreg]
				emit_movsxd_r64_r32(DRCTOP, REG_RAX, REG_EAX);								// movsxd rax,eax
				emit_movsxd_r64_r32(DRCTOP, REG_RDX, REG_EDX);								// movsxd rdx,edx
				emit_mov_m64_r64(DRCTOP, LOADDR, REG_RAX);									// mov  [lo],rax
				emit_mov_m64_r64(DRCTOP, HIADDR, REG_RDX);									// mov  [hi],rdx
			}
			return TRUE;

		case 0x1a:	/* DIV - MIPS I */
			if (RTREG != 0)
			{
				emit_cmp_m32_imm(DRCTOP, REGADDR(RTREG), 0);								// cmp  [rtreg],0
				emit_mov_r32_m32(DRCTOP, REG_EAX, REGADDR(RSREG));							// mov  eax,[rsreg]
				emit_jcc_short_link(DRCTOP, COND_E, &link1);								// je   skip
				emit_cdq(DRCTOP);															// cdq
				emit_idiv_m32(DRCTOP, REGADDR(RTREG));										// idiv [rtreg]
				emit_movsxd_r64_r32(DRCTOP, REG_RAX, REG_EAX);								// movsxd rax,eax
				emit_movsxd_r64_r32(DRCTOP, REG_RDX, REG_EDX);								// movsxd rdx,edx
				emit_mov_m64_r64(DRCTOP, LOADDR, REG_RAX);									// mov  [lo],rax
				emit_mov_m64_r64(DRCTOP, HIADDR, REG_RDX);									// mov  [hi],rdx
				resolve_link(DRCTOP, &link1);											// skip:
			}
			return TRUE;

		case 0x1b:	/* DIVU - MIPS I */
			if (RTREG != 0)
			{
				emit_cmp_m32_imm(DRCTOP, REGADDR(RTREG), 0);								// cmp  [rtreg],0
				emit_mov_r32_m32(DRCTOP, REG_EAX, REGADDR(RSREG));							// mov  eax,[rsreg]
				emit_jcc_short_link(DRCTOP, COND_E, &link1);								// je   skip
				emit_xor_r32_r32(DRCTOP, REG_EDX, REG_EDX);									// xor  edx,edx
				emit_div_m32(DRCTOP, REGADDR(RTREG));										// div  [rtreg]
				emit_movsxd_r64_r32(DRCTOP, REG_RAX, REG_EAX);								// movsxd rax,eax
				emit_movsxd_r64_r32(DRCTOP, REG_RDX, REG_EDX);								// movsxd rdx,edx
				emit_mov_m64_r64(DRCTOP, LOADDR, REG_RAX);									// mov  [lo],rax
				emit_mov_m64_r64(DRCTOP, HIADDR, REG_RDX);									// mov  [hi],rdx
				resolve_link(DRCTOP, &link1);											// skip:
			}
			return TRUE;

		case 0x1c:	/* DMULT - MIPS III */
			if (RSREG == 0 || RTREG == 0)
			{
				emit_mov_m64_imm(DRCTOP, HIADDR, 0);										// mov  [hi],0
				emit_mov_m64_imm(DRCTOP, LOADDR, 0);										// mov  [lo],0
			}
			else
			{
				emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));							// mov  rax,[rtreg]
				emit_imul_m64(DRCTOP, REGADDR(RSREG));										// imul [rsreg]
				emit_mov_m64_r64(DRCTOP, LOADDR, REG_RAX);									// mov  [lo],rax
				emit_mov_m64_r64(DRCTOP, HIADDR, REG_RDX);									// mov  [hi],rdx
			}
			return TRUE;

		case 0x1d:	/* DMULTU - MIPS III */
			if (RSREG == 0 || RTREG == 0)
			{
				emit_mov_m64_imm(DRCTOP, HIADDR, 0);										// mov  [hi],0
				emit_mov_m64_imm(DRCTOP, LOADDR, 0);										// mov  [lo],0
			}
			else
			{
				emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));							// mov  rax,[rtreg]
				emit_mul_m64(DRCTOP, REGADDR(RSREG));										// mul  [rsreg]
				emit_mov_m64_r64(DRCTOP, LOADDR, REG_RAX);									// mov  [lo],rax
				emit_mov_m64_r64(DRCTOP, HIADDR, REG_RDX);									// mov  [hi],rdx
			}
			return TRUE;

		case 0x1e:	/* DDIV - MIPS III */
			if (RTREG != 0)
			{
				emit_cmp_m64_imm(DRCTOP, REGADDR(RTREG), 0);								// cmp  [rtreg],0
				emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));							// mov  rax,[rsreg]
				emit_jcc_short_link(DRCTOP, COND_E, &link1);								// je   skip
				emit_cqo(DRCTOP);															// cqo
				emit_idiv_m64(DRCTOP, REGADDR(RTREG));										// idiv [rtreg]
				emit_mov_m64_r64(DRCTOP, LOADDR, REG_RAX);									// mov  [lo],rax
				emit_mov_m64_r64(DRCTOP, HIADDR, REG_RDX);									// mov  [hi],rdx
				resolve_link(DRCTOP, &link1);											// skip:
			}
			return TRUE;

		case 0x1f:	/* DDIVU - MIPS III */
			if (RTREG != 0)
			{
				emit_cmp_m64_imm(DRCTOP, REGADDR(RTREG), 0);								// cmp  [rtreg],0
				emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));							// mov  rax,[rsreg]
				emit_jcc_short_link(DRCTOP, COND_E, &link1);								// je   skip
				emit_xor_r32_r32(DRCTOP, REG_RDX, REG_RDX);									// xor  rdx,rdx
				emit_div_m64(DRCTOP, REGADDR(RTREG));										// div  [rtreg]
				emit_mov_m64_r64(DRCTOP, LOADDR, REG_RAX);									// mov  [lo],rax
				emit_mov_m64_r64(DRCTOP, HIADDR, REG_RDX);									// mov  [hi],rdx
				resolve_link(DRCTOP, &link1);											// skip:
			}
			return TRUE;
	}
	return FALSE;
}

static int recompile_special2(drc_core *drc, compiler_state *compiler, mips3_opcode_desc *desc)
{
	UINT32 op = *desc->opptr;

	switch (op & 63)
	{
		case 0x20:	/* ADD - MIPS I */
			if (RSREG != 0 && RTREG != 0)
			{
				emit_mov_r32_m32(DRCTOP, REG_EAX, REGADDR(RSREG));							// mov  eax,[rsreg]
				emit_add_r32_m32(DRCTOP, REG_EAX, REGADDR(RTREG));							// add  eax,[rtreg]
				oob_request_callback(drc, COND_O, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_overflow_exception);
																							// jo   generate_overflow_exception
				if (RDREG != 0)
				{
					emit_movsxd_r64_r32(DRCTOP, REG_RAX, REG_EAX);							// movsxd rax,eax
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
			}
			else if (RDREG != 0)
			{
				if (RSREG != 0)
				{
					emit_movsxd_r64_m32(DRCTOP, REG_RAX, REGADDR(RSREG));					// movsxd rax,[rsreg]
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else if (RTREG != 0)
				{
					emit_movsxd_r64_m32(DRCTOP, REG_RAX, REGADDR(RTREG));					// movsxd rax,[rtreg]
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else
					emit_mov_m64_imm(DRCTOP, REGADDR(RDREG), 0);							// mov  [rdreg],0
			}
			return TRUE;

		case 0x21:	/* ADDU - MIPS I */
			if (RDREG != 0)
			{
				if (RSREG != 0 && RTREG != 0)
				{
					emit_mov_r32_m32(DRCTOP, REG_EAX, REGADDR(RSREG));						// mov  eax,[rsreg]
					emit_add_r32_m32(DRCTOP, REG_EAX, REGADDR(RTREG));						// add  eax,[rtreg]
					emit_movsxd_r64_r32(DRCTOP, REG_RAX, REG_EAX);							// movsxd rax,eax
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else if (RSREG != 0)
				{
					emit_movsxd_r64_m32(DRCTOP, REG_RAX, REGADDR(RSREG));					// movsxd rax,[rsreg]
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else if (RTREG != 0)
				{
					emit_movsxd_r64_m32(DRCTOP, REG_RAX, REGADDR(RTREG));					// movsxd rax,[rtreg]
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else
					emit_mov_m64_imm(DRCTOP, REGADDR(RDREG), 0);							// mov  [rdreg],0
			}
			return TRUE;

		case 0x22:	/* SUB - MIPS I */
			if (RSREG != 0 && RTREG != 0)
			{
				emit_mov_r32_m32(DRCTOP, REG_EAX, REGADDR(RSREG));							// mov  eax,[rsreg]
				emit_sub_r32_m32(DRCTOP, REG_EAX, REGADDR(RTREG));							// sub  eax,[rtreg]
				oob_request_callback(drc, COND_O, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_overflow_exception);
																							// jo   generate_overflow_exception
				if (RDREG != 0)
				{
					emit_movsxd_r64_r32(DRCTOP, REG_RAX, REG_EAX);							// movsxd rax,eax
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
			}
			else if (RDREG != 0)
			{
				if (RSREG != 0)
				{
					emit_movsxd_r64_m32(DRCTOP, REG_RAX, REGADDR(RSREG));					// movsxd rax,[rsreg]
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else if (RTREG != 0)
				{
					emit_movsxd_r64_m32(DRCTOP, REG_RAX, REGADDR(RTREG));					// movsxd rax,[rtreg]
					emit_neg_r64(DRCTOP, REG_RAX);											// neg  rax
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else
					emit_mov_m64_imm(DRCTOP, REGADDR(RDREG), 0);							// mov  [rdreg],0
			}
			return TRUE;

		case 0x23:	/* SUBU - MIPS I */
			if (RDREG != 0)
			{
				if (RSREG != 0 && RTREG != 0)
				{
					emit_mov_r32_m32(DRCTOP, REG_EAX, REGADDR(RSREG));						// mov  eax,[rsreg]
					emit_sub_r32_m32(DRCTOP, REG_EAX, REGADDR(RTREG));						// sub  eax,[rtreg]
					emit_movsxd_r64_r32(DRCTOP, REG_RAX, REG_EAX);							// movsxd rax,eax
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else if (RSREG != 0)
				{
					emit_movsxd_r64_m32(DRCTOP, REG_RAX, REGADDR(RSREG));					// movsxd rax,[rsreg]
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else if (RTREG != 0)
				{
					emit_movsxd_r64_m32(DRCTOP, REG_RAX, REGADDR(RTREG));					// movsxd rax,[rtreg]
					emit_neg_r64(DRCTOP, REG_RAX);											// neg  rax
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else
					emit_mov_m64_imm(DRCTOP, REGADDR(RDREG), 0);							// mov  [rdreg],0
			}
			return TRUE;

		case 0x24:	/* AND - MIPS I */
			if (RDREG != 0)
			{
				if (RSREG != 0 && RTREG != 0)
				{
					emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));						// mov  rax,[rsreg]
					emit_and_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));						// and  rax,[rtreg]
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else
					emit_mov_m64_imm(DRCTOP, REGADDR(RDREG), 0);							// mov  [rdreg],0
			}
			return TRUE;

		case 0x25:	/* OR - MIPS I */
			if (RDREG != 0)
			{
				if (RSREG != 0 && RTREG != 0)
				{
					emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));						// mov  rax,[rsreg]
					emit_or_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));						// or   rax,[rtreg]
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else if (RSREG != 0)
				{
					emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));						// mov  rax,[rsreg]
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else if (RTREG != 0)
				{
					emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));						// mov  rax,[rtreg]
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else
					emit_mov_m64_imm(DRCTOP, REGADDR(RDREG), 0);							// mov  [rdreg],0
			}
			return TRUE;

		case 0x26:	/* XOR - MIPS I */
			if (RDREG != 0)
			{
				if (RSREG != 0 && RTREG != 0)
				{
					emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));						// mov  rax,[rsreg]
					emit_xor_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));						// xor  rax,[rtreg]
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else if (RSREG != 0)
				{
					emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));						// mov  rax,[rsreg]
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else if (RTREG != 0)
				{
					emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));						// mov  rax,[rtreg]
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else
					emit_mov_m64_imm(DRCTOP, REGADDR(RDREG), 0);							// mov  [rdreg],0
			}
			return TRUE;

		case 0x27:	/* NOR - MIPS I */
			if (RDREG != 0)
			{
				if (RSREG != 0 && RTREG != 0)
				{
					emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));						// mov  rax,[rsreg]
					emit_or_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));						// or   rax,[rtreg]
					emit_not_r64(DRCTOP, REG_RAX);											// not  rax
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else if (RSREG != 0)
				{
					emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));						// mov  rax,[rsreg]
					emit_not_r64(DRCTOP, REG_RAX);											// not  rax
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else if (RTREG != 0)
				{
					emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));						// mov  rax,[rtreg]
					emit_not_r64(DRCTOP, REG_RAX);											// not  rax
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else
					emit_mov_m64_imm(DRCTOP, REGADDR(RDREG), ~0);							// mov  [rdreg],~0
			}
			return TRUE;

		case 0x2a:	/* SLT - MIPS I */
			if (RDREG != 0)
			{
				if (RSREG != 0)
					emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));						// mov  rax,[rsreg]
				else
					emit_xor_r32_r32(DRCTOP, REG_RAX, REG_RAX);								// xor  rax,rax
				if (RTREG != 0)
					emit_sub_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));						// sub  rax,[rtreg]
				emit_shr_r64_imm(DRCTOP, REG_RAX, 63);										// shr  rax,63
				emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);							// mov  [rdreg],rax
			}
			return TRUE;

		case 0x2b:	/* SLTU - MIPS I */
			if (RDREG != 0)
			{
				if (RSREG != 0)
					emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));						// mov  rax,[rsreg]
				else
					emit_xor_r32_r32(DRCTOP, REG_RAX, REG_RAX);								// xor  rax,rax
				if (RTREG != 0)
					emit_cmp_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));						// cmp  rax,[rtreg]
				else
					emit_cmp_r64_imm(DRCTOP, REG_RAX, 0);									// cmp  rax,0
				emit_setcc_r8(DRCTOP, COND_B, REG_AL);										// setb  al
				emit_movsx_r64_r8(DRCTOP, REG_RAX, REG_AL);									// movsx rax,al
				emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);							// mov  [rdreg],rax
			}
			return TRUE;

		case 0x2c:	/* DADD - MIPS III */
			if (RSREG != 0 && RTREG != 0)
			{
				emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));							// mov  rax,[rsreg]
				emit_add_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));							// add  rax,[rtreg]
				oob_request_callback(drc, COND_O, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_overflow_exception);
																							// jo   generate_overflow_exception
				if (RDREG != 0)
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
			}
			else if (RDREG != 0)
			{
				if (RSREG != 0)
				{
					emit_movsxd_r64_m32(DRCTOP, REG_RAX, REGADDR(RSREG));					// movsxd rax,[rsreg]
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else if (RTREG != 0)
				{
					emit_movsxd_r64_m32(DRCTOP, REG_RAX, REGADDR(RTREG));					// movsxd rax,[rtreg]
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else
					emit_mov_m64_imm(DRCTOP, REGADDR(RDREG), 0);							// mov  [rdreg],0
			}
			return TRUE;

		case 0x2d:	/* DADDU - MIPS III */
			if (RDREG != 0)
			{
				if (RSREG != 0 && RTREG != 0)
				{
					emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));						// mov  rax,[rsreg]
					emit_add_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));						// add  rax,[rtreg]
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else if (RSREG != 0)
				{
					emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));						// mov  rax,[rsreg]
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else if (RTREG != 0)
				{
					emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));						// mov  rax,[rtreg]
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else
					emit_mov_m64_imm(DRCTOP, REGADDR(RDREG), 0);							// mov  [rdreg],0
			}
			return TRUE;

		case 0x2e:	/* DSUB - MIPS III */
			if (RSREG != 0 && RTREG != 0)
			{
				emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));							// mov  rax,[rsreg]
				emit_sub_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));							// sub  rax,[rtreg]
				oob_request_callback(drc, COND_O, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_overflow_exception);
																							// jo   generate_overflow_exception
				if (RDREG != 0)
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
			}
			else if (RDREG != 0)
			{
				if (RSREG != 0)
				{
					emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));						// mov  rax,[rsreg]
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else if (RTREG != 0)
				{
					emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));						// mov  rax,[rtreg]
					emit_neg_r64(DRCTOP, REG_RAX);											// neg  rax
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else
					emit_mov_m64_imm(DRCTOP, REGADDR(RDREG), 0);							// mov  [rdreg],0
			}
			return TRUE;

		case 0x2f:	/* DSUBU - MIPS III */
			if (RDREG != 0)
			{
				if (RSREG != 0 && RTREG != 0)
				{
					emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));						// mov  rax,[rsreg]
					emit_sub_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));						// sub  rax,[rtreg]
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else if (RSREG != 0)
				{
					emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));						// mov  rax,[rsreg]
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else if (RTREG != 0)
				{
					emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));						// mov  rax,[rtreg]
					emit_neg_r64(DRCTOP, REG_RAX);											// neg  rax
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else
					emit_mov_m64_imm(DRCTOP, REGADDR(RDREG), 0);							// mov  [rdreg],0
			}
			return TRUE;

		case 0x30:	/* TGE - MIPS II */
			if (RSREG != 0)
				emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));							// mov  rax,[rsreg]
			else
				emit_xor_r32_r32(DRCTOP, REG_RAX, REG_RAX);									// xor  rax,rax
			if (RTREG != 0)
				emit_cmp_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));							// cmp  rax,[rtreg]
			else
				emit_cmp_r64_imm(DRCTOP, REG_RAX, 0);										// cmp  rax,0
			oob_request_callback(drc, COND_GE, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_trap_exception);
																							// jge  generate_overflow_exception
			return TRUE;

		case 0x31:	/* TGEU - MIPS II */
			if (RSREG != 0)
				emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));							// mov  rax,[rsreg]
			else
				emit_xor_r32_r32(DRCTOP, REG_RAX, REG_RAX);									// xor  rax,rax
			if (RTREG != 0)
				emit_cmp_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));							// cmp  rax,[rtreg]
			else
				emit_cmp_r64_imm(DRCTOP, REG_RAX, 0);										// cmp  rax,0
			oob_request_callback(drc, COND_AE, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_trap_exception);
																							// jae  generate_overflow_exception
			return TRUE;

		case 0x32:	/* TLT - MIPS II */
			if (RSREG != 0)
				emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));							// mov  rax,[rsreg]
			else
				emit_xor_r32_r32(DRCTOP, REG_RAX, REG_RAX);									// xor  rax,rax
			if (RTREG != 0)
				emit_cmp_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));							// cmp  rax,[rtreg]
			else
				emit_cmp_r64_imm(DRCTOP, REG_RAX, 0);										// cmp  rax,0
			oob_request_callback(drc, COND_L, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_trap_exception);
																							// jl   generate_overflow_exception
			return TRUE;

		case 0x33:	/* TLTU - MIPS II */
			if (RSREG != 0)
				emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));							// mov  rax,[rsreg]
			else
				emit_xor_r32_r32(DRCTOP, REG_RAX, REG_RAX);									// xor  rax,rax
			if (RTREG != 0)
				emit_cmp_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));							// cmp  rax,[rtreg]
			else
				emit_cmp_r64_imm(DRCTOP, REG_RAX, 0);										// cmp  rax,0
			oob_request_callback(drc, COND_B, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_trap_exception);
																							// jb  generate_overflow_exception
			return TRUE;

		case 0x34:	/* TEQ - MIPS II */
			if (RSREG != 0)
				emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));							// mov  rax,[rsreg]
			else
				emit_xor_r32_r32(DRCTOP, REG_RAX, REG_RAX);									// xor  rax,rax
			if (RTREG != 0)
				emit_cmp_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));							// cmp  rax,[rtreg]
			else
				emit_cmp_r64_imm(DRCTOP, REG_RAX, 0);										// cmp  rax,0
			oob_request_callback(drc, COND_E, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_trap_exception);
																							// je   generate_overflow_exception
			return TRUE;

		case 0x36:	/* TNE - MIPS II */
			if (RSREG != 0)
				emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RSREG));							// mov  rax,[rsreg]
			else
				emit_xor_r32_r32(DRCTOP, REG_RAX, REG_RAX);									// xor  rax,rax
			if (RTREG != 0)
				emit_cmp_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));							// cmp  rax,[rtreg]
			else
				emit_cmp_r64_imm(DRCTOP, REG_RAX, 0);										// cmp  rax,0
			oob_request_callback(drc, COND_NE, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_trap_exception);
																							// jne  generate_overflow_exception
			return TRUE;

		case 0x38:	/* DSLL - MIPS III */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));						// mov  rax,[rtreg]
					if (SHIFT != 0)
						emit_shl_r64_imm(DRCTOP, REG_EAX, SHIFT);							// shl  rax,SHIFT
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else
					emit_mov_m64_imm(DRCTOP, REGADDR(RDREG), 0);							// mov  [rdreg],0
			}
			return TRUE;

		case 0x3a:	/* DSRL - MIPS III */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));						// mov  rax,[rtreg]
					if (SHIFT != 0)
						emit_shr_r64_imm(DRCTOP, REG_EAX, SHIFT);							// shr  rax,SHIFT
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else
					emit_mov_m64_imm(DRCTOP, REGADDR(RDREG), 0);							// mov  [rdreg],0
			}
			return TRUE;

		case 0x3b:	/* DSRA - MIPS III */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));						// mov  rax,[rtreg]
					if (SHIFT != 0)
						emit_sar_r64_imm(DRCTOP, REG_EAX, SHIFT);							// sar  rax,SHIFT
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else
					emit_mov_m64_imm(DRCTOP, REGADDR(RDREG), 0);							// mov  [rdreg],0
			}
			return TRUE;

		case 0x3c:	/* DSLL32 - MIPS III */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));						// mov  rax,[rtreg]
					emit_shl_r64_imm(DRCTOP, REG_EAX, SHIFT + 32);							// shl  rax,SHIFT+32
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else
					emit_mov_m64_imm(DRCTOP, REGADDR(RDREG), 0);							// mov  [rdreg],0
			}
			return TRUE;

		case 0x3e:	/* DSRL32 - MIPS III */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));						// mov  rax,[rtreg]
					emit_shr_r64_imm(DRCTOP, REG_EAX, SHIFT + 32);							// shr  rax,SHIFT+32
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else
					emit_mov_m64_imm(DRCTOP, REGADDR(RDREG), 0);							// mov  [rdreg],0
			}
			return TRUE;

		case 0x3f:	/* DSRA32 - MIPS III */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));						// mov  rax,[rtreg]
					emit_sar_r64_imm(DRCTOP, REG_EAX, SHIFT + 32);							// sar  rax,SHIFT+32
					emit_mov_m64_r64(DRCTOP, REGADDR(RDREG), REG_RAX);						// mov  [rdreg],rax
				}
				else
					emit_mov_m64_imm(DRCTOP, REGADDR(RDREG), 0);							// mov  [rdreg],0
			}
			return TRUE;
	}
	return FALSE;
}


static int recompile_special(drc_core *drc, compiler_state *compiler, mips3_opcode_desc *desc)
{
	if (!(*desc->opptr & 32))
		return recompile_special1(drc, compiler, desc);
	else
		return recompile_special2(drc, compiler, desc);
}


/*------------------------------------------------------------------
    recompile_regimm
------------------------------------------------------------------*/

static int recompile_regimm(drc_core *drc, compiler_state *compiler, mips3_opcode_desc *desc)
{
	UINT32 op = *desc->opptr;
	emit_link link1;

	switch (RTREG)
	{
		case 0x00:	/* BLTZ */
		case 0x02:	/* BLTZL */
			assert(desc->delay != NULL);
			if (RSREG == 0)
				return TRUE;
			else
			{
				emit_cmp_m64_imm(DRCTOP, REGADDR(RSREG), 0);								// cmp  [rsreg],0
				emit_jcc_near_link(DRCTOP, COND_GE, &link1);								// jge  skip
			}
			append_delay_slot_and_branch(drc, compiler, desc);								// <next instruction + redispatch>
			resolve_link(DRCTOP, &link1);												// skip:
			return TRUE;

		case 0x01:	/* BGEZ */
		case 0x03:	/* BGEZL */
			if (RSREG != 0)
			{
				emit_cmp_m64_imm(DRCTOP, REGADDR(RSREG), 0);								// cmp  [rsreg],0
				emit_jcc_near_link(DRCTOP, COND_L, &link1);									// jl   skip
			}
			append_delay_slot_and_branch(drc, compiler, desc);								// <next instruction + redispatch>
			if (RSREG != 0)
				resolve_link(DRCTOP, &link1);											// skip:
			return TRUE;

		case 0x08:	/* TGEI */
			if (RSREG != 0)
			{
				emit_cmp_m64_imm(DRCTOP, REGADDR(RSREG), SIMMVAL);							// cmp  [rsreg],SIMMVAL
				oob_request_callback(drc, COND_GE, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_trap_exception);
																							// jge  generate_trap_exception
			}
			else
			{
				if (0 >= SIMMVAL)
					oob_request_callback(drc, COND_NONE, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_trap_exception);
																							// jmp  generate_trap_exception
			}
			return TRUE;

		case 0x09:	/* TGEIU */
			if (RSREG != 0)
			{
				emit_cmp_m64_imm(DRCTOP, REGADDR(RSREG), SIMMVAL);							// cmp  [rsreg],SIMMVAL
				oob_request_callback(drc, COND_AE, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_trap_exception);
																							// jae  generate_trap_exception
			}
			else
			{
				if (0 >= (UINT64)SIMMVAL)
					oob_request_callback(drc, COND_NONE, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_trap_exception);
																							// jmp  generate_trap_exception
			}
			return TRUE;

		case 0x0a:	/* TLTI */
			if (RSREG != 0)
			{
				emit_cmp_m64_imm(DRCTOP, REGADDR(RSREG), SIMMVAL);							// cmp  [rsreg],SIMMVAL
				oob_request_callback(drc, COND_L, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_trap_exception);
																							// jl   generate_trap_exception
			}
			else
			{
				if (0 < SIMMVAL)
					oob_request_callback(drc, COND_NONE, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_trap_exception);
																							// jmp  generate_trap_exception
			}
			return TRUE;

		case 0x0b:	/* TLTIU */
			if (RSREG != 0)
			{
				emit_cmp_m64_imm(DRCTOP, REGADDR(RSREG), SIMMVAL);							// cmp  [rsreg],SIMMVAL
				oob_request_callback(drc, COND_B, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_trap_exception);
																							// jb   generate_trap_exception
			}
			else
			{
				if (0 < (UINT64)SIMMVAL)
					oob_request_callback(drc, COND_NONE, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_trap_exception);
																							// jmp  generate_trap_exception
			}
			return TRUE;

		case 0x0c:	/* TEQI */
			if (RSREG != 0)
			{
				emit_cmp_m64_imm(DRCTOP, REGADDR(RSREG), SIMMVAL);							// cmp  [rsreg],SIMMVAL
				oob_request_callback(drc, COND_E, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_trap_exception);
																							// je   generate_trap_exception
			}
			else
			{
				if (0 == SIMMVAL)
					oob_request_callback(drc, COND_NONE, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_trap_exception);
																							// jmp  generate_trap_exception
			}
			return TRUE;

		case 0x0e:	/* TNEI */
			if (RSREG != 0)
			{
				emit_cmp_m64_imm(DRCTOP, REGADDR(RSREG), SIMMVAL);							// cmp  [rsreg],SIMMVAL
				oob_request_callback(drc, COND_NE, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_trap_exception);
																							// jne  generate_trap_exception
			}
			else
			{
				if (0 != SIMMVAL)
					oob_request_callback(drc, COND_NONE, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_trap_exception);
																							// jmp  generate_trap_exception
			}
			return TRUE;

		case 0x10:	/* BLTZAL */
		case 0x12:	/* BLTZALL */
			assert(desc->delay != NULL);
			if (RSREG == 0)
				return TRUE;
			else
			{
				emit_cmp_m64_imm(DRCTOP, REGADDR(RSREG), 0);								// cmp  [rsreg],0
				emit_jcc_near_link(DRCTOP, COND_GE, &link1);								// jge  skip
			}
			emit_mov_m64_imm(DRCTOP, REGADDR(31), (INT32)(desc->pc + 8));					// mov  [r31],pc + 8
			append_delay_slot_and_branch(drc, compiler, desc);								// <next instruction + redispatch>
			resolve_link(DRCTOP, &link1);												// skip:
			return TRUE;

		case 0x11:	/* BGEZAL */
		case 0x13:	/* BGEZALL */
			if (RSREG != 0)
			{
				emit_cmp_m64_imm(DRCTOP, REGADDR(RSREG), 0);								// cmp  [rsreg],0
				emit_jcc_near_link(DRCTOP, COND_L, &link1);									// jl   skip
			}
			emit_mov_m64_imm(DRCTOP, REGADDR(31), (INT32)(desc->pc + 8));					// mov  [r31],pc + 8
			append_delay_slot_and_branch(drc, compiler, desc);								// <next instruction + redispatch>
			if (RSREG != 0)
				resolve_link(DRCTOP, &link1);											// skip:
			return TRUE;
	}
	return FALSE;
}



/***************************************************************************
    COP0 RECOMPILATION
***************************************************************************/

/*------------------------------------------------------------------
    recompile_set_cop0_reg
------------------------------------------------------------------*/

static int recompile_set_cop0_reg(drc_core *drc, compiler_state *compiler, const mips3_opcode_desc *desc, UINT8 reg)
{
	emit_link link1;

	switch (reg)
	{
		case COP0_Cause:
			emit_mov_r32_m32(DRCTOP, REG_EDX, CPR0ADDR(COP0_Cause));						// mov  edx,[Cause]
			emit_and_r32_imm(DRCTOP, REG_EAX, ~0xfc00);										// and  eax,~0xfc00
			emit_and_r32_imm(DRCTOP, REG_EDX, 0xfc00);										// and  edx,0xfc00
			emit_or_r32_r32(DRCTOP, REG_EAX, REG_EDX);										// or   eax,edx
			emit_mov_m32_r32(DRCTOP, CPR0ADDR(COP0_Cause), REG_EAX);						// mov  [Cause],eax
			emit_and_r32_m32(DRCTOP, REG_EAX, CPR0ADDR(COP0_Status));						// and  eax,[SR]
			emit_test_r32_imm(DRCTOP, REG_EAX, 0x300);										// test eax,0x300
			oob_request_callback(drc, COND_NZ, oob_interrupt_cleanup, compiler, desc, mips3.drcdata->generate_interrupt_exception);
																							// jnz  generate_interrupt_exception
			return TRUE;

		case COP0_Status:
			emit_mov_r32_m32(DRCTOP, REG_EDX, CPR0ADDR(COP0_Status));						// mov  edx,[Status]
			emit_mov_m32_r32(DRCTOP, CPR0ADDR(COP0_Status), REG_EAX);						// mov  [Status],eax
			emit_xor_r32_r32(DRCTOP, REG_EDX, REG_EAX);										// xor  edx,eax
			emit_test_r32_imm(DRCTOP, REG_EDX, 0x8000);										// test edx,0x8000
			emit_jcc_short_link(DRCTOP, COND_Z, &link1);									// jz   skip
			emit_lea_r64_m64(DRCTOP, REG_P1, COREADDR);										// lea  p1,[mips3.core]
			emit_call_m64(DRCTOP, MDRC(&mips3.drcdata->mips3com_update_cycle_counting));
																							// call mips3com_update_cycle_counting
			resolve_link(DRCTOP, &link1);												// skip:
			append_check_interrupts(drc, compiler, desc);									// <check interrupts>
			return TRUE;

		case COP0_Count:
			emit_mov_m32_r32(DRCTOP, CPR0ADDR(COP0_Count), REG_EAX);						// mov  [Count],eax
			emit_call_m64(DRCTOP, MDRC(&mips3.drcdata->activecpu_gettotalcycles64));		// call activecpu_gettotalcycles64
			emit_mov_r32_m32(DRCTOP, REG_EDX, CPR0ADDR(COP0_Count));						// mov  edx,[Count]
			emit_sub_r64_r64(DRCTOP, REG_RAX, REG_RDX);										// sub  rax,rdx
			emit_sub_r64_r64(DRCTOP, REG_RAX, REG_RDX);										// sub  rax,rdx
			emit_mov_m64_r64(DRCTOP, MDRC(&mips3.core->count_zero_time), REG_RAX);
																							// mov  [count_zero_time],rax
			emit_lea_r64_m64(DRCTOP, REG_P1, COREADDR);										// lea  p1,[mips3.core]
			emit_call_m64(DRCTOP, MDRC(&mips3.drcdata->mips3com_update_cycle_counting));
																							// call mips3com_update_cycle_counting
			return TRUE;

		case COP0_Compare:
			emit_mov_m32_r32(DRCTOP, CPR0ADDR(COP0_Compare), REG_EAX);						// mov  [Compare],eax
			emit_and_m32_imm(DRCTOP, CPR0ADDR(COP0_Cause), ~0x8000);						// and  [Cause],~0x8000
			emit_lea_r64_m64(DRCTOP, REG_P1, COREADDR);										// lea  p1,[mips3.core]
			emit_call_m64(DRCTOP, MDRC(&mips3.drcdata->mips3com_update_cycle_counting));
																							// call mips3com_update_cycle_counting
			return TRUE;

		case COP0_PRId:
			return TRUE;

		case COP0_Config:
			emit_mov_r32_m32(DRCTOP, REG_EDX, CPR0ADDR(COP0_Config));						// mov  edx,[Config]
			emit_and_r32_imm(DRCTOP, REG_EAX, 0x0007);										// and  eax,0x0007
			emit_and_r32_imm(DRCTOP, REG_EDX, ~0x0007);										// and  edx,~0x0007
			emit_or_r32_r32(DRCTOP, REG_EAX, REG_EDX);										// or   eax,edx
			emit_mov_m32_r32(DRCTOP, CPR0ADDR(COP0_Config), REG_EAX);						// mov  [Config],eax
			return TRUE;

		default:
			emit_mov_m32_r32(DRCTOP, CPR0ADDR(reg), REG_EAX);								// mov  cpr0[reg],eax
			return TRUE;
	}
	return FALSE;
}


/*------------------------------------------------------------------
    recompile_get_cop0_reg
------------------------------------------------------------------*/

static int recompile_get_cop0_reg(drc_core *drc, compiler_state *compiler, const mips3_opcode_desc *desc, UINT8 reg)
{
	emit_link link1, link2;

	switch (reg)
	{
		case COP0_Count:
#if (!COMPARE_AGAINST_C)
			emit_sub_m32_imm(DRCTOP, ICOUNTADDR, compiler->cycles + MIPS3_COUNT_READ_CYCLES);
																							// sub  [icount],<lots>
			compiler->cycles = 0;
			emit_jcc_short_link(DRCTOP, COND_NS, &link1);									// jns  notneg
			emit_mov_m32_imm(DRCTOP, ICOUNTADDR, 0);										// mov  [icount],0
			resolve_link(DRCTOP, &link1);												// notneg:
#endif
			emit_call_m64(DRCTOP, MDRC(&mips3.drcdata->activecpu_gettotalcycles64));
																							// call activecpu_gettotalcycles64
			emit_sub_r64_m64(DRCTOP, REG_RAX, MDRC(&mips3.core->count_zero_time));			// sub  rax,[count_zero_time]
			emit_shr_r64_imm(DRCTOP, REG_RAX, 1);											// shr  rax,1
			emit_movsxd_r64_r32(DRCTOP, REG_RAX, REG_EAX);									// movsxd rax,eax
			return TRUE;

		case COP0_Cause:
#if (!COMPARE_AGAINST_C)
			emit_sub_m32_imm(DRCTOP, ICOUNTADDR, compiler->cycles + MIPS3_CAUSE_READ_CYCLES);
																							// sub  [icount],<lots>
			compiler->cycles = 0;
			emit_jcc_short_link(DRCTOP, COND_NS, &link1);									// jns  notneg
			emit_mov_m32_imm(DRCTOP, ICOUNTADDR, 0);										// mov  [icount],0
			resolve_link(DRCTOP, &link1);												// notneg:
#endif
			emit_movsxd_r64_m32(DRCTOP, REG_RAX, CPR0ADDR(COP0_Cause));						// movsxd rax,[Cause]
			return TRUE;

		case COP0_Random:
			emit_call_m64(DRCTOP, MDRC(&mips3.drcdata->activecpu_gettotalcycles64));
																							// call activecpu_gettotalcycles64
			emit_sub_r64_m64(DRCTOP, REG_RAX, MDRC(&mips3.core->count_zero_time));			// sub  rax,[count_zero_time]
			emit_mov_r32_m32(DRCTOP, REG_ECX, CPR0ADDR(COP0_Wired));						// mov  ecx,[Wired]
			emit_mov_r32_imm(DRCTOP, REG_R8D, 48);											// mov  r8d,48
			emit_and_r32_imm(DRCTOP, REG_ECX, 0x3f);										// and  ecx,0x3f
			emit_sub_r32_r32(DRCTOP, REG_R8D, REG_ECX);										// sub  r8d,ecx
			emit_mov_r64_r64(DRCTOP, REG_RDX, REG_RAX);										// mov  rdx,rax
			emit_jcc_short_link(DRCTOP, COND_BE, &link1);									// jbe  link1
			emit_shr_r64_imm(DRCTOP, REG_RDX, 32);											// shr  rdx,32
			emit_idiv_r32(DRCTOP, REG_R8D);													// idiv r8d
			emit_mov_r32_r32(DRCTOP, REG_EAX, REG_EDX);										// mov  eax,edx
			emit_add_r32_r32(DRCTOP, REG_EAX, REG_ECX);										// add  eax,ecx
			emit_and_r32_imm(DRCTOP, REG_EAX, 0x3f);										// and  eax,0x3f
			emit_jmp_short_link(DRCTOP, &link2);											// jmp  link2
			resolve_link(DRCTOP, &link1);												// link1:
			emit_mov_r32_imm(DRCTOP, REG_EAX, 47);											// mov  eax,47
			resolve_link(DRCTOP, &link2);												// link2:
			return TRUE;

		default:
			emit_movsxd_r64_m32(DRCTOP, REG_RAX, CPR0ADDR(reg));							// movsxd rax,cpr0[reg]
			return TRUE;
	}
	return FALSE;
}


/*------------------------------------------------------------------
    recompile_cop0
------------------------------------------------------------------*/

static int recompile_cop0(drc_core *drc, compiler_state *compiler, mips3_opcode_desc *desc)
{
	UINT32 op = *desc->opptr;

	/* generate an exception if COP0 is disabled or we are not in kernel mode */
	if (mips3.drcoptions & MIPS3DRC_STRICT_COP0)
	{
		emit_link checklink;

		emit_test_m32_imm(DRCTOP, CPR0ADDR(COP0_Status), SR_KSU_MASK);						// test [SR],SR_KSU_MASK
		emit_jcc_short_link(DRCTOP, COND_Z, &checklink);									// jz   okay
		emit_test_m32_imm(DRCTOP, CPR0ADDR(COP0_Status), SR_COP0);							// test [SR],SR_COP0
		oob_request_callback(drc, COND_Z, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_cop_exception);
																							// jz   generate_cop_exception
		resolve_link(DRCTOP, &checklink);												// okay:
	}

	switch (RSREG)
	{
		case 0x00:	/* MFCz */
		case 0x01:	/* DMFCz */
			if (RTREG != 0)
			{
				recompile_get_cop0_reg(drc, compiler, desc, RDREG);							// mov  eax,cpr0[rdreg])
				emit_mov_m64_r64(DRCTOP, REGADDR(RTREG), REG_RAX);							// mov  [rtreg],rax
			}
			return TRUE;

		case 0x02:	/* CFCz */
			if (RTREG != 0)
			{
				emit_movsxd_r64_m32(DRCTOP, REG_RAX, CCR0ADDR(RDREG));						// movsxd rax,ccr0[rdreg]
				emit_mov_m64_r64(DRCTOP, REGADDR(RTREG), REG_RAX);							// mov  [rtreg],rax
			}
			return TRUE;

		case 0x04:	/* MTCz */
		case 0x05:	/* DMTCz */
			if (RTREG != 0)
				emit_mov_r32_m32(DRCTOP, REG_EAX, REGADDR(RTREG));							// mov  eax,[rtreg]
			else
				emit_xor_r32_r32(DRCTOP, REG_RAX, REG_RAX);									// xor  rax,rax
			recompile_set_cop0_reg(drc, compiler, desc, RDREG);								// mov  cpr0[rdreg],rax
			return TRUE;

		case 0x06:	/* CTCz */
			if (RTREG != 0)
			{
				emit_mov_r32_m32(DRCTOP, REG_EAX, REGADDR(RTREG));							// mov  eax,[rtreg]
				emit_mov_m32_r32(DRCTOP, CCR0ADDR(RDREG), REG_EAX);							// mov  ccr0[rdreg],eax
			}
			else
				emit_mov_m32_imm(DRCTOP, CCR0ADDR(RDREG), 0);								// mov  ccr0[rdreg],0
			return TRUE;

//      case 0x08:  /* BC */
//          switch (RTREG)
//          {
//              case 0x00:  /* BCzF */  if (!mips3.core->fcc[0][0]) ADDPC(SIMMVAL);                break;
//              case 0x01:  /* BCzF */  if (mips3.core->fcc[0][0]) ADDPC(SIMMVAL);                 break;
//              case 0x02:  /* BCzFL */ invalid_instruction(op);                            break;
//              case 0x03:  /* BCzTL */ invalid_instruction(op);                            break;
//              default:    invalid_instruction(op);                                        break;
//          }
//          break;

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
					emit_lea_r64_m64(DRCTOP, REG_P1, COREADDR);								// lea  p1,[mips3.core]
					emit_call_m64(DRCTOP, MDRC(&mips3.drcdata->mips3com_tlbr));				// call mips3com_tlbr
					return TRUE;

				case 0x02:	/* TLBWI */
					emit_lea_r64_m64(DRCTOP, REG_P1, COREADDR);								// lea  p1,[mips3.core]
					emit_call_m64(DRCTOP, MDRC(&mips3.drcdata->mips3com_tlbwi));			// call mips3com_tlbwi
					return TRUE;

				case 0x06:	/* TLBWR */
					emit_lea_r64_m64(DRCTOP, REG_P1, COREADDR);								// lea  p1,[mips3.core]
					emit_call_m64(DRCTOP, MDRC(&mips3.drcdata->mips3com_tlbwr));			// call mips3com_tlbwr
					return TRUE;

				case 0x08:	/* TLBP */
					emit_lea_r64_m64(DRCTOP, REG_P1, COREADDR);								// lea  p1,[mips3.core]
					emit_call_m64(DRCTOP, MDRC(&mips3.drcdata->mips3com_tlbp));				// call mips3com_tlbp
					return TRUE;

				case 0x18:	/* ERET */
					emit_mov_r32_m32(DRCTOP, REG_P1, CPR0ADDR(COP0_EPC));					// mov  p1,[EPC]
					emit_and_m32_imm(DRCTOP, CPR0ADDR(COP0_Status), ~SR_EXL);				// and  [SR],~SR_EXL
					drc_append_dispatcher(drc);												// <dispatch>
					return TRUE;

				case 0x20:	/* WAIT */
					return TRUE;
			}
			break;
	}

	return FALSE;
}



/***************************************************************************
    COP1 RECOMPILATION
***************************************************************************/

/*------------------------------------------------------------------
    recompile_cop1
------------------------------------------------------------------*/

static int recompile_cop1(drc_core *drc, compiler_state *compiler, mips3_opcode_desc *desc)
{
	UINT32 op = *desc->opptr;
	emit_link link1;

	/* generate an exception if COP1 is disabled */
	if (mips3.drcoptions & MIPS3DRC_STRICT_COP1)
	{
		emit_test_m32_imm(DRCTOP, CPR0ADDR(COP0_Status), SR_COP1);							// test [mips3.core->cpr[0][COP0_Status]],SR_COP1
		oob_request_callback(drc, COND_Z, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_cop_exception);
																							// jz   generate_cop_exception
	}

	switch (RSREG)
	{
		case 0x00:	/* MFCz */
			if (RTREG != 0)
			{
				emit_movsxd_r64_m32(DRCTOP, REG_RAX, FPR32ADDR(RDREG));						// movsxd rax,cpr[rdreg]
				emit_mov_m64_r64(DRCTOP, REGADDR(RTREG), REG_RAX);							// mov  [rtreg],rax
			}
			return TRUE;

		case 0x01:	/* DMFCz */
			if (RTREG != 0)
			{
				emit_mov_r64_m64(DRCTOP, REG_RAX, FPR32ADDR(RDREG));						// mov  rax,cpr[rdreg]
				emit_mov_m64_r64(DRCTOP, REGADDR(RTREG), REG_RAX);							// mov  [rtreg],rax
			}
			return TRUE;

		case 0x02:	/* CFCz */
			if (RTREG != 0)
			{
				if (RDREG != 31)
					emit_movsxd_r64_m32(DRCTOP, REG_RAX, CCR1ADDR(RDREG));					// movsxd rax,ccr1[rdreg]
				else
					emit_call(DRCTOP, mips3.drcdata->recover_ccr31);						// call recover_ccr31
				emit_mov_m64_r64(DRCTOP, REGADDR(RTREG), REG_RAX);							// mov  [rtreg],rax
			}
			return TRUE;

		case 0x04:	/* MTCz */
			if (RTREG != 0)
			{
				emit_mov_r32_m32(DRCTOP, REG_EAX, REGADDR(RTREG));							// mov  eax,[mips3.core->r[RTREG]]
				emit_mov_m32_r32(DRCTOP, FPR32ADDR(RDREG), REG_EAX);						// mov  cpr1[rdreg],eax
			}
			else
				emit_mov_m32_imm(DRCTOP, FPR32ADDR(RDREG), 0);								// mov  cpr1[rdreg],0
			return TRUE;

		case 0x05:	/* DMTCz */
			if (RTREG != 0)
			{
				emit_mov_r64_m64(DRCTOP, REG_RAX, REGADDR(RTREG));							// mov  rax,[rtreg]
				emit_mov_m64_r64(DRCTOP, FPR64ADDR(RDREG), REG_RAX);						// mov  cpr1[rdreg],rax
			}
			else
				emit_mov_m64_imm(DRCTOP, FPR64ADDR(RDREG), 0);								// mov  cpr1[rdreg],0
			return TRUE;

		case 0x06:	/* CTCz */
			if (RTREG != 0)
				emit_mov_r32_m32(DRCTOP, REG_EAX, REGADDR(RTREG));							// mov  eax,[mips3.core->r[RTREG]]
			else
				emit_xor_r32_r32(DRCTOP, REG_EAX, REG_EAX);									// xor  eax,eax
			if (RDREG != 31)
				emit_mov_m32_r32(DRCTOP, CCR1ADDR(RDREG), REG_EAX);							// mov  ccr1[rdreg],eax
			else
				emit_call(DRCTOP, mips3.drcdata->explode_ccr31);							// call explode_ccr31
			return TRUE;

		case 0x08:	/* BC */
			switch ((op >> 16) & 3)
			{
				case 0x00:	/* BCzF */
				case 0x02:	/* BCzFL */
					emit_cmp_m8_imm(DRCTOP, CF1ADDR((op >> 18) & 7), 0);					// cmp  fcc[which],0
					emit_jcc_near_link(DRCTOP, COND_NZ, &link1);							// jnz  link1
					append_delay_slot_and_branch(drc, compiler, desc);						// <next instruction + redispatch>
					resolve_link(DRCTOP, &link1);										// skip:
					return TRUE;

				case 0x01:	/* BCzT */
				case 0x03:	/* BCzTL */
					emit_cmp_m8_imm(DRCTOP, CF1ADDR((op >> 18) & 7), 0);					// cmp  fcc[which],0
					emit_jcc_near_link(DRCTOP, COND_Z, &link1);								// jz  link1
					append_delay_slot_and_branch(drc, compiler, desc);						// <next instruction + redispatch>
					resolve_link(DRCTOP, &link1);										// skip:
					return TRUE;
			}
			break;

		default:
			switch (op & 0x3f)
			{
				case 0x00:
					if (IS_SINGLE(op))	/* ADD.S */
					{
						emit_movss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FSREG));			// movss xmm0,[fsreg]
						emit_addss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FTREG));			// addss xmm0,[ftreg]
						emit_movss_m32_r128(DRCTOP, FPR32ADDR(FDREG), REG_XMM0);			// movss [fdreg],xmm0
					}
					else				/* ADD.D */
					{
						emit_movsd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FSREG));			// movsd xmm0,[fsreg]
						emit_addsd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FTREG));			// addsd xmm0,[ftreg]
						emit_movsd_m64_r128(DRCTOP, FPR64ADDR(FDREG), REG_XMM0);			// movsd [fdreg],xmm0
					}
					return TRUE;

				case 0x01:
					if (IS_SINGLE(op))	/* SUB.S */
					{
						emit_movss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FSREG));			// movss xmm0,[fsreg]
						emit_subss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FTREG));			// subss xmm0,[ftreg]
						emit_movss_m32_r128(DRCTOP, FPR32ADDR(FDREG), REG_XMM0);			// movss [fdreg],xmm0
					}
					else				/* SUB.D */
					{
						emit_movsd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FSREG));			// movsd xmm0,[fsreg]
						emit_subsd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FTREG));			// subsd xmm0,[ftreg]
						emit_movsd_m64_r128(DRCTOP, FPR64ADDR(FDREG), REG_XMM0);			// movsd [fdreg],xmm0
					}
					return TRUE;

				case 0x02:
					if (IS_SINGLE(op))	/* MUL.S */
					{
						emit_movss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FSREG));			// movss xmm0,[fsreg]
						emit_mulss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FTREG));			// mulss xmm0,[ftreg]
						emit_movss_m32_r128(DRCTOP, FPR32ADDR(FDREG), REG_XMM0);			// movss [fdreg],xmm0
					}
					else				/* MUL.D */
					{
						emit_movsd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FSREG));			// movsd xmm0,[fsreg]
						emit_mulsd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FTREG));			// mulsd xmm0,[ftreg]
						emit_movsd_m64_r128(DRCTOP, FPR64ADDR(FDREG), REG_XMM0);			// movsd [fdreg],xmm0
					}
					return TRUE;

				case 0x03:
					if (IS_SINGLE(op))	/* DIV.S */
					{
						emit_movss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FSREG));			// movss xmm0,[fsreg]
						emit_divss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FTREG));			// divss xmm0,[ftreg]
						emit_movss_m32_r128(DRCTOP, FPR32ADDR(FDREG), REG_XMM0);			// movss [fdreg],xmm0
					}
					else				/* DIV.D */
					{
						emit_movsd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FSREG));			// movsd xmm0,[fsreg]
						emit_divsd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FTREG));			// divsd xmm0,[ftreg]
						emit_movsd_m64_r128(DRCTOP, FPR64ADDR(FDREG), REG_XMM0);			// movsd [fdreg],xmm0
					}
					return TRUE;

				case 0x04:
					if (IS_SINGLE(op))	/* SQRT.S */
					{
						emit_sqrtss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FSREG));			// sqrtss xmm0,[fsreg]
						emit_movss_m32_r128(DRCTOP, FPR32ADDR(FDREG), REG_XMM0);			// movss [fdreg],xmm0
					}
					else				/* SQRT.D */
					{
						emit_sqrtsd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FSREG));			// sqrtsd xmm0,[fsreg]
						emit_movsd_m64_r128(DRCTOP, FPR64ADDR(FDREG), REG_XMM0);			// movsd [fdreg],xmm0
					}
					return TRUE;

				case 0x05:
					if (IS_SINGLE(op))	/* ABS.S */
					{
						emit_movss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FSREG));			// movss xmm0,[fsreg]
						emit_andps_r128_m128(DRCTOP, REG_XMM0, MDRC(mips3.drcdata->abs32mask));
																							// andps xmm0,[abs32mask]
						emit_movss_m32_r128(DRCTOP, FPR32ADDR(FDREG), REG_XMM0);			// movss [fdreg],xmm0
					}
					else				/* ABS.D */
					{
						emit_movsd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FSREG));			// movsd xmm0,[fsreg]
						emit_andps_r128_m128(DRCTOP, REG_XMM0, MDRC(mips3.drcdata->abs64mask));
																							// andpd xmm0,[abs64mask]
						emit_movsd_m64_r128(DRCTOP, FPR64ADDR(FDREG), REG_XMM0);			// movsd [fdreg],xmm0
					}
					return TRUE;

				case 0x06:
					if (IS_SINGLE(op))	/* MOV.S */
					{
						emit_movss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FSREG));			// movss xmm0,[fsreg]
						emit_movss_m32_r128(DRCTOP, FPR32ADDR(FDREG), REG_XMM0);			// movss [fdreg],xmm0
					}
					else				/* MOV.D */
					{
						emit_movsd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FSREG));			// movsd xmm0,[fsreg]
						emit_movsd_m64_r128(DRCTOP, FPR64ADDR(FDREG), REG_XMM0);			// movsd [fdreg],xmm0
					}
					return TRUE;

				case 0x07:
					if (IS_SINGLE(op))	/* NEG.S */
					{
						emit_xorps_r128_r128(DRCTOP, REG_XMM0, REG_XMM0);					// xorps xmm0,xmm0
						emit_subss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FSREG));			// subss xmm0,[fsreg]
						emit_movss_m32_r128(DRCTOP, FPR32ADDR(FDREG), REG_XMM0);			// movss [fdreg],xmm0
					}
					else				/* NEG.D */
					{
						emit_xorps_r128_r128(DRCTOP, REG_XMM0, REG_XMM0);					// xorps xmm0,xmm0
						emit_subsd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FSREG));			// subsd xmm0,[fsreg]
						emit_movsd_m64_r128(DRCTOP, FPR64ADDR(FDREG), REG_XMM0);			// movsd [fdreg],xmm0
					}
					return TRUE;

				case 0x08:
					/* FUTURE: On Penryn, we can use ROUNDPS/ROUNDPD */
					drc_append_set_temp_sse_rounding(drc, FPRND_NEAR);						// ldmxcsr [round]
					if (IS_SINGLE(op))	/* ROUND.L.S */
						emit_cvtss2si_r64_m32(DRCTOP, REG_RAX, FPR32ADDR(FSREG));			// cvtss2si rax,[fsreg]
					else				/* ROUND.L.D */
						emit_cvtsd2si_r64_m64(DRCTOP, REG_RAX, FPR64ADDR(FSREG));			// cvtsd2si rax,[fsreg]
					emit_mov_m64_r64(DRCTOP, FPR64ADDR(FDREG), REG_RAX);					// mov  [fdreg],rax
					drc_append_restore_sse_rounding(drc);									// ldmxcsr [mode]
					return TRUE;

				case 0x09:
					if (IS_SINGLE(op))	/* TRUNC.L.S */
						emit_cvttss2si_r64_m32(DRCTOP, REG_RAX, FPR32ADDR(FSREG));			// cvttss2si rax,[fsreg]
					else				/* TRUNC.L.D */
						emit_cvttsd2si_r64_m64(DRCTOP, REG_RAX, FPR64ADDR(FSREG));			// cvttsd2si rax,[fsreg]
					emit_mov_m64_r64(DRCTOP, FPR64ADDR(FDREG), REG_RAX);					// mov  [fdreg],rax
					return TRUE;

				case 0x0a:
					/* FUTURE: On Penryn, we can use ROUNDPS/ROUNDPD */
					drc_append_set_temp_sse_rounding(drc, FPRND_UP);						// ldmxcsr [round]
					if (IS_SINGLE(op))	/* CEIL.L.S */
						emit_cvtss2si_r64_m32(DRCTOP, REG_RAX, FPR32ADDR(FSREG));			// cvtss2si rax,[fsreg]
					else				/* CEIL.L.D */
						emit_cvtsd2si_r64_m64(DRCTOP, REG_RAX, FPR64ADDR(FSREG));			// cvtsd2si rax,[fsreg]
					emit_mov_m64_r64(DRCTOP, FPR64ADDR(FDREG), REG_RAX);					// mov  [fdreg],rax
					drc_append_restore_sse_rounding(drc);									// ldmxcsr [mode]
					return TRUE;

				case 0x0b:
					/* FUTURE: On Penryn, we can use ROUNDPS/ROUNDPD */
					drc_append_set_temp_sse_rounding(drc, FPRND_DOWN);						// ldmxcsr [round]
					if (IS_SINGLE(op))	/* FLOOR.L.S */
						emit_cvtss2si_r64_m32(DRCTOP, REG_RAX, FPR32ADDR(FSREG));			// cvtss2si rax,[fsreg]
					else				/* FLOOR.L.D */
						emit_cvtsd2si_r64_m64(DRCTOP, REG_RAX, FPR64ADDR(FSREG));			// cvtsd2si rax,[fsreg]
					emit_mov_m64_r64(DRCTOP, FPR64ADDR(FDREG), REG_RAX);					// mov  [fdreg],rax
					drc_append_restore_sse_rounding(drc);									// ldmxcsr [mode]
					return TRUE;

				case 0x0c:
					/* FUTURE: On Penryn, we can use ROUNDPS/ROUNDPD */
					drc_append_set_temp_sse_rounding(drc, FPRND_NEAR);						// ldmxcsr [round]
					if (IS_SINGLE(op))	/* ROUND.W.S */
						emit_cvtss2si_r32_m32(DRCTOP, REG_EAX, FPR32ADDR(FSREG));			// cvtss2si eax,[fsreg]
					else				/* ROUND.W.D */
						emit_cvtsd2si_r32_m64(DRCTOP, REG_EAX, FPR64ADDR(FSREG));			// cvtsd2si eax,[fsreg]
					emit_mov_m32_r32(DRCTOP, FPR32ADDR(FDREG), REG_EAX);					// mov  [fdreg],eax
					drc_append_restore_sse_rounding(drc);									// ldmxcsr [mode]
					return TRUE;

				case 0x0d:
					if (IS_SINGLE(op))	/* TRUNC.W.S */
						emit_cvttss2si_r32_m32(DRCTOP, REG_EAX, FPR32ADDR(FSREG));			// cvttss2si eax,[fsreg]
					else				/* TRUNC.W.D */
						emit_cvttsd2si_r32_m64(DRCTOP, REG_EAX, FPR64ADDR(FSREG));			// cvttsd2si eax,[fsreg]
					emit_mov_m32_r32(DRCTOP, FPR32ADDR(FDREG), REG_EAX);					// mov  [fdreg],eax
					return TRUE;

				case 0x0e:
					/* FUTURE: On Penryn, we can use ROUNDPS/ROUNDPD */
					drc_append_set_temp_sse_rounding(drc, FPRND_UP);						// ldmxcsr [round]
					if (IS_SINGLE(op))	/* CEIL.W.S */
						emit_cvtss2si_r32_m32(DRCTOP, REG_EAX, FPR32ADDR(FSREG));			// cvtss2si eax,[fsreg]
					else				/* CEIL.W.D */
						emit_cvtsd2si_r32_m64(DRCTOP, REG_EAX, FPR64ADDR(FSREG));			// cvtsd2si eax,[fsreg]
					emit_mov_m32_r32(DRCTOP, FPR32ADDR(FDREG), REG_EAX);					// mov  [fdreg],eax
					drc_append_restore_sse_rounding(drc);									// ldmxcsr [mode]
					return TRUE;

				case 0x0f:
					/* FUTURE: On Penryn, we can use ROUNDPS/ROUNDPD */
					drc_append_set_temp_sse_rounding(drc, FPRND_DOWN);						// ldmxcsr [round]
					if (IS_SINGLE(op))	/* FLOOR.W.S */
						emit_cvtss2si_r32_m32(DRCTOP, REG_EAX, FPR32ADDR(FSREG));			// cvtss2si eax,[fsreg]
					else				/* FLOOR.W.D */
						emit_cvtsd2si_r32_m64(DRCTOP, REG_EAX, FPR64ADDR(FSREG));			// cvtsd2si eax,[fsreg]
					emit_mov_m32_r32(DRCTOP, FPR32ADDR(FDREG), REG_EAX);					// mov  [fdreg],eax
					drc_append_restore_sse_rounding(drc);									// ldmxcsr [mode]
					return TRUE;

				case 0x11:	/* R5000 */
					emit_cmp_m8_imm(DRCTOP, CF1ADDR((op >> 18) & 7), 0);					// cmp  fcc[which],0
					emit_jcc_short_link(DRCTOP, ((op >> 16) & 1) ? COND_Z : COND_NZ, &link1);
																							// jz/nz skip
					if (IS_SINGLE(op))	/* MOVT/F.S */
					{
						emit_movss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FSREG));			// movss xmm0,[fsreg]
						emit_movss_m32_r128(DRCTOP, FPR32ADDR(FDREG), REG_XMM0);			// movss [fdreg],xmm0
					}
					else				/* MOVT/F.D */
					{
						emit_movsd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FSREG));			// movsd xmm0,[fsreg]
						emit_movsd_m64_r128(DRCTOP, FPR64ADDR(FDREG), REG_XMM0);			// movsd [fdreg],xmm0
					}
					resolve_link(DRCTOP, &link1);										// skip:
					return TRUE;

				case 0x12:	/* R5000 */
					emit_cmp_m64_imm(DRCTOP, REGADDR(RTREG), 0);							// cmp  [rtreg],0
					emit_jcc_short_link(DRCTOP, COND_NZ, &link1);							// jnz  skip
					if (IS_SINGLE(op))	/* MOVZ.S */
					{
						emit_movss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FSREG));			// movss xmm0,[fsreg]
						emit_movss_m32_r128(DRCTOP, FPR32ADDR(FDREG), REG_XMM0);			// movss [fdreg],xmm0
					}
					else				/* MOVZ.D */
					{
						emit_movsd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FSREG));			// movsd xmm0,[fsreg]
						emit_movsd_m64_r128(DRCTOP, FPR64ADDR(FDREG), REG_XMM0);			// movsd [fdreg],xmm0
					}
					resolve_link(DRCTOP, &link1);										// skip:
					return TRUE;

				case 0x13:	/* R5000 */
					emit_cmp_m64_imm(DRCTOP, REGADDR(RTREG), 0);							// cmp  [rtreg],0
					emit_jcc_short_link(DRCTOP, COND_Z, &link1);							// jz  skip
					if (IS_SINGLE(op))	/* MOVN.S */
					{
						emit_movss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FSREG));			// movss xmm0,[fsreg]
						emit_movss_m32_r128(DRCTOP, FPR32ADDR(FDREG), REG_XMM0);			// movss [fdreg],xmm0
					}
					else				/* MOVN.D */
					{
						emit_movsd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FSREG));			// movsd xmm0,[fsreg]
						emit_movsd_m64_r128(DRCTOP, FPR64ADDR(FDREG), REG_XMM0);			// movsd [fdreg],xmm0
					}
					resolve_link(DRCTOP, &link1);										// skip:
					return TRUE;

				case 0x15:	/* R5000 */
					if (IS_SINGLE(op))	/* RECIP.S */
					{
						emit_rcpss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FSREG));			// rcpss xmm0,[fsreg]
						emit_movss_m32_r128(DRCTOP, FPR32ADDR(FDREG), REG_XMM0);			// movss [fdreg],xmm0
					}
					else				/* RECIP.D */
					{
						emit_cvtsd2ss_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FSREG));			// cvtsd2ss xmm0,[fsreg]
						emit_rcpss_r128_r128(DRCTOP, REG_XMM0, REG_XMM0);					// rcpss xmm0,xmm0
						emit_cvtss2sd_r128_r128(DRCTOP, REG_XMM0, REG_XMM0);				// cvtss2sd xmm0,xmm0
						emit_movsd_m64_r128(DRCTOP, FPR64ADDR(FDREG), REG_XMM0);			// movsd [fdreg],xmm0
					}
					return TRUE;

				case 0x16:	/* R5000 */
					if (IS_SINGLE(op))	/* RSQRT.S */
					{
						emit_rsqrtss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FSREG));			// rsqrtss xmm0,[fsreg]
						emit_movss_m32_r128(DRCTOP, FPR32ADDR(FDREG), REG_XMM0);			// movss [fdreg],xmm0
					}
					else				/* RSQRT.D */
					{
						emit_cvtsd2ss_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FSREG));			// cvtss2sd xmm0,[fsreg]
						emit_rsqrtss_r128_r128(DRCTOP, REG_XMM0, REG_XMM0);					// rsqrtss xmm0,xmm0
						emit_cvtss2sd_r128_r128(DRCTOP, REG_XMM0, REG_XMM0);				// cvtsd2ss xmm0,xmm0
						emit_movsd_m64_r128(DRCTOP, FPR64ADDR(FDREG), REG_XMM0);			// movsd [fdreg],xmm0
					}
					return TRUE;

				case 0x20:
					if (IS_INTEGRAL(op))
					{
						if (IS_SINGLE(op))	/* CVT.S.W */
							emit_cvtsi2ss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FSREG));		// cvtsi2ss xmm0,[fsreg]
						else				/* CVT.S.L */
							emit_cvtsi2ss_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FSREG));		// cvtsi2ss xmm0,[fsreg]
					}
					else					/* CVT.S.D */
						emit_cvtsd2ss_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FSREG));			// cvtsd2ss xmm0,[fsreg]
					emit_movss_m32_r128(DRCTOP, FPR32ADDR(FDREG), REG_XMM0);				// movss [fdreg],xmm0
					return TRUE;

				case 0x21:
					if (IS_INTEGRAL(op))
					{
						if (IS_SINGLE(op))	/* CVT.D.W */
							emit_cvtsi2sd_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FSREG));		// cvtsi2sd xmm0,[fsreg]
						else				/* CVT.D.L */
							emit_cvtsi2sd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FSREG));		// cvtsi2sd xmm0,[fsreg]
					}
					else					/* CVT.D.S */
						emit_cvtss2sd_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FSREG));			// cvtss2sd xmm0,[fsreg]
					emit_movsd_m64_r128(DRCTOP, FPR64ADDR(FDREG), REG_XMM0);				// movsd [fdreg],xmm0
					return TRUE;

				case 0x24:
					if (IS_SINGLE(op))	/* CVT.W.S */
						emit_cvtss2si_r32_m32(DRCTOP, REG_EAX, FPR32ADDR(FSREG));			// cvtss2si eax,[fsreg]
					else				/* CVT.W.D */
						emit_cvtsd2si_r32_m64(DRCTOP, REG_EAX, FPR64ADDR(FSREG));			// cvtsd2si eax,[fsreg]
					emit_mov_m32_r32(DRCTOP, FPR32ADDR(FDREG), REG_EAX);					// mov  [fdreg],eax
					return TRUE;

				case 0x25:
					if (IS_SINGLE(op))	/* CVT.L.S */
						emit_cvtss2si_r64_m32(DRCTOP, REG_RAX, FPR32ADDR(FSREG));			// cvtss2si rax,[fsreg]
					else				/* CVT.L.D */
						emit_cvtsd2si_r64_m64(DRCTOP, REG_RAX, FPR64ADDR(FSREG));			// cvtsd2si rax,[fsreg]
					emit_mov_m64_r64(DRCTOP, FPR32ADDR(FDREG), REG_RAX);					// mov  [fdreg],rax
					return TRUE;

				case 0x30:
				case 0x38:
					emit_mov_m8_imm(DRCTOP, CF1ADDR((op >> 8) & 7), 0);						/* C.F.S/D */
					return TRUE;

				case 0x31:
				case 0x39:
					emit_mov_m8_imm(DRCTOP, CF1ADDR((op >> 8) & 7), 0);						/* C.F.S/D */
					return TRUE;

				case 0x32:
				case 0x3a:
					if (IS_SINGLE(op))	/* C.EQ.S */
					{
						emit_movss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FSREG));			// movss xmm0,[fsreg]
						emit_comiss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FTREG));			// comiss xmm0,[ftreg]
					}
					else				/* C.EQ.D */
					{
						emit_movsd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FSREG));			// movsd xmm0,[fsreg]
						emit_comisd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FTREG));			// comisd xmm0,[ftreg]
					}
					emit_setcc_m8(DRCTOP, COND_E, CF1ADDR((op >> 8) & 7));					// sete fcc[x]
					return TRUE;

				case 0x33:
				case 0x3b:
					if (IS_SINGLE(op))	/* C.UEQ.S */
					{
						emit_movss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FSREG));			// movss xmm0,[fsreg]
						emit_ucomiss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FTREG));			// ucomiss xmm0,[ftreg]
					}
					else				/* C.UEQ.D */
					{
						emit_movsd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FSREG));			// movsd xmm0,[fsreg]
						emit_ucomisd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FTREG));			// ucomisd xmm0,[ftreg]
					}
					emit_setcc_m8(DRCTOP, COND_E, CF1ADDR((op >> 8) & 7));					// sete fcc[x]
					return TRUE;

				case 0x34:
				case 0x3c:
					if (IS_SINGLE(op))	/* C.OLT.S */
					{
						emit_movss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FSREG));			// movss xmm0,[fsreg]
						emit_comiss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FTREG));			// comiss xmm0,[ftreg]
					}
					else				/* C.OLT.D */
					{
						emit_movsd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FSREG));			// movsd xmm0,[fsreg]
						emit_comisd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FTREG));			// comisd xmm0,[ftreg]
					}
					emit_setcc_m8(DRCTOP, COND_B, CF1ADDR((op >> 8) & 7));					// setb fcc[x]
					return TRUE;

				case 0x35:
				case 0x3d:
					if (IS_SINGLE(op))	/* C.ULT.S */
					{
						emit_movss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FSREG));			// movss xmm0,[fsreg]
						emit_ucomiss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FTREG));			// ucomiss xmm0,[ftreg]
					}
					else				/* C.ULT.D */
					{
						emit_movsd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FSREG));			// movsd xmm0,[fsreg]
						emit_ucomisd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FTREG));			// ucomisd xmm0,[ftreg]
					}
					emit_setcc_m8(DRCTOP, COND_B, CF1ADDR((op >> 8) & 7));					// setb fcc[x]
					return TRUE;

				case 0x36:
				case 0x3e:
					if (IS_SINGLE(op))	/* C.OLE.S */
					{
						emit_movss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FSREG));			// movss xmm0,[fsreg]
						emit_comiss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FTREG));			// comiss xmm0,[ftreg]
					}
					else				/* C.OLE.D */
					{
						emit_movsd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FSREG));			// movsd xmm0,[fsreg]
						emit_comisd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FTREG));			// comisd xmm0,[ftreg]
					}
					emit_setcc_m8(DRCTOP, COND_BE, CF1ADDR((op >> 8) & 7));					// setbe fcc[x]
					return TRUE;

				case 0x37:
				case 0x3f:
					if (IS_SINGLE(op))	/* C.OLE.S */
					{
						emit_movss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FSREG));			// movss xmm0,[fsreg]
						emit_ucomiss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FTREG));			// ucomiss xmm0,[ftreg]
					}
					else				/* C.OLE.D */
					{
						emit_movsd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FSREG));			// movsd xmm0,[fsreg]
						emit_ucomisd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FTREG));			// ucomisd xmm0,[ftreg]
					}
					emit_setcc_m8(DRCTOP, COND_BE, CF1ADDR((op >> 8) & 7));					// setbe fcc[x]
					return TRUE;
			}
			break;
	}
	return FALSE;
}



/***************************************************************************
    COP1X RECOMPILATION
***************************************************************************/

/*------------------------------------------------------------------
    recompile_cop1x
------------------------------------------------------------------*/

static int recompile_cop1x(drc_core *drc, compiler_state *compiler, mips3_opcode_desc *desc)
{
	UINT32 op = *desc->opptr;

	if (mips3.drcoptions & MIPS3DRC_STRICT_COP1)
	{
		emit_test_m32_imm(DRCTOP, CPR0ADDR(COP0_Status), SR_COP1);							// test [mips3.core->cpr[0][COP0_Status]],SR_COP1
		oob_request_callback(drc, COND_Z, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_cop_exception);
																							// jz   generate_cop_exception
	}

	switch (op & 0x3f)
	{
		case 0x00:		/* LWXC1 */
			emit_mov_r32_m32(DRCTOP, REG_P1, REGADDR(RSREG));								// mov  p1,[rsreg]
			emit_add_r32_m32(DRCTOP, REG_P1, REGADDR(RTREG));								// add  p1,[rtreg]
			emit_call(DRCTOP, mips3.drcdata->general.read_long_unsigned);					// call general.read_long_unsigned
			oob_request_callback(drc, COND_NO_JUMP, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_tlbload_exception);
			emit_mov_m32_r32(DRCTOP, FPR32ADDR(FDREG), REG_EAX);							// mov  [fdreg],eax
			return TRUE;

		case 0x01:		/* LDXC1 */
			emit_mov_r32_m32(DRCTOP, REG_P1, REGADDR(RSREG));								// mov  p1,[rsreg]
			emit_add_r32_m32(DRCTOP, REG_P1, REGADDR(RTREG));								// add  p1,[rtreg]
			emit_call(DRCTOP, mips3.drcdata->general.read_double);							// call general.read_double
			oob_request_callback(drc, COND_NO_JUMP, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_tlbload_exception);
			emit_mov_m64_r64(DRCTOP, FPR64ADDR(FDREG), REG_RAX);							// mov  [fdreg],rax
			return TRUE;

		case 0x08:		/* SWXC1 */
			emit_mov_r32_m32(DRCTOP, REG_P1, REGADDR(RSREG));								// mov  p1,[rsreg]
			emit_mov_r32_m32(DRCTOP, REG_P2, FPR32ADDR(FSREG));								// mov  p2,[fsreg]
			emit_add_r32_m32(DRCTOP, REG_P1, REGADDR(RTREG));								// add  p1,[rtreg]
			emit_call(DRCTOP, mips3.drcdata->general.write_long);							// call general.write_long
			oob_request_callback(drc, COND_NO_JUMP, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_tlbstore_exception);
			return TRUE;

		case 0x09:		/* SDXC1 */
			emit_mov_r32_m32(DRCTOP, REG_P1, REGADDR(RSREG));								// mov  p1,[rsreg]
			emit_mov_r64_m64(DRCTOP, REG_P2, FPR64ADDR(FSREG));								// mov  p2,[fsreg]
			emit_add_r32_m32(DRCTOP, REG_P1, REGADDR(RTREG));								// add  p1,[rtreg]
			emit_call(DRCTOP, mips3.drcdata->general.write_long);							// call general.write_long
			oob_request_callback(drc, COND_NO_JUMP, oob_exception_cleanup, compiler, desc, mips3.drcdata->generate_tlbstore_exception);
			return TRUE;

		case 0x0f:		/* PREFX */
			return TRUE;

		case 0x20:		/* MADD.S */
			emit_movss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FSREG));						// movss xmm0,[fsreg]
			emit_mulss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FTREG));						// mulss xmm0,[ftreg]
			emit_addss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FRREG));						// addss xmm0,[frreg]
			emit_movss_m32_r128(DRCTOP, FPR32ADDR(FDREG), REG_XMM0);						// movss [fdreg],xmm0
			return TRUE;

		case 0x21:		/* MADD.D */
			emit_movsd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FSREG));						// movsd xmm0,[fsreg]
			emit_mulsd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FTREG));						// mulsd xmm0,[ftreg]
			emit_addsd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FRREG));						// addsd xmm0,[frreg]
			emit_movsd_m64_r128(DRCTOP, FPR64ADDR(FDREG), REG_XMM0);						// movsd [fdreg],xmm0
			return TRUE;

		case 0x28:		/* MSUB.S */
			emit_movss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FSREG));						// movss xmm0,[fsreg]
			emit_mulss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FTREG));						// mulss xmm0,[ftreg]
			emit_subss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FRREG));						// subss xmm0,[frreg]
			emit_movss_m32_r128(DRCTOP, FPR32ADDR(FDREG), REG_XMM0);						// movss [fdreg],xmm0
			return TRUE;

		case 0x29:		/* MSUB.D */
			emit_movsd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FSREG));						// movsd xmm0,[fsreg]
			emit_mulsd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FTREG));						// mulsd xmm0,[ftreg]
			emit_subsd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FRREG));						// subsd xmm0,[frreg]
			emit_movsd_m64_r128(DRCTOP, FPR64ADDR(FDREG), REG_XMM0);						// movsd [fdreg],xmm0
			return TRUE;

		case 0x30:		/* NMADD.S */
			emit_xorps_r128_r128(DRCTOP, REG_XMM1, REG_XMM1);								// xorps xmm1,xmm1
			emit_movss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FSREG));						// movss xmm0,[fsreg]
			emit_mulss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FTREG));						// mulss xmm0,[ftreg]
			emit_addss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FRREG));						// addss xmm0,[frreg]
			emit_subss_r128_r128(DRCTOP, REG_XMM1, REG_XMM0);								// subss xmm1,xmm0
			emit_movss_m32_r128(DRCTOP, FPR32ADDR(FDREG), REG_XMM1);						// movss [fdreg],xmm1
			return TRUE;

		case 0x31:		/* NMADD.D */
			emit_xorps_r128_r128(DRCTOP, REG_XMM1, REG_XMM1);								// xorps xmm1,xmm1
			emit_movsd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FSREG));						// movsd xmm0,[fsreg]
			emit_mulsd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FTREG));						// mulsd xmm0,[ftreg]
			emit_addsd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FRREG));						// addsd xmm0,[frreg]
			emit_subsd_r128_r128(DRCTOP, REG_XMM1, REG_XMM0);								// subsd xmm1,xmm0
			emit_movsd_m64_r128(DRCTOP, FPR64ADDR(FDREG), REG_XMM1);						// movsd [fdreg],xmm1
			return TRUE;

		case 0x38:		/* NMSUB.S */
			emit_movss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FSREG));						// movss xmm0,[fsreg]
			emit_mulss_r128_m32(DRCTOP, REG_XMM0, FPR32ADDR(FTREG));						// mulss xmm0,[ftreg]
			emit_movss_r128_m32(DRCTOP, REG_XMM1, FPR32ADDR(FRREG));						// movss xmm1,[frreg]
			emit_subss_r128_r128(DRCTOP, REG_XMM1, REG_XMM0);								// subss xmm1,xmm0
			emit_movss_m32_r128(DRCTOP, FPR32ADDR(FDREG), REG_XMM1);						// movss [fdreg],xmm1
			return TRUE;

		case 0x39:		/* NMSUB.D */
			emit_movsd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FSREG));						// movsd xmm0,[fsreg]
			emit_mulsd_r128_m64(DRCTOP, REG_XMM0, FPR64ADDR(FTREG));						// mulsd xmm0,[ftreg]
			emit_movsd_r128_m64(DRCTOP, REG_XMM1, FPR64ADDR(FRREG));						// movsd xmm1,[frreg]
			emit_subsd_r128_r128(DRCTOP, REG_XMM1, REG_XMM0);								// subsd xmm1,xmm0
			emit_movsd_m64_r128(DRCTOP, FPR64ADDR(FDREG), REG_XMM1);						// movsd [fdreg],xmm1
			return TRUE;

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
	return FALSE;
}
