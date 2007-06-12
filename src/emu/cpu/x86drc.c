/***************************************************************************

    x86drc.c

    x86 Dynamic recompiler support routines.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "cpuintrf.h"
#include "x86drc.h"
#include "debugger.h"

#define LOG_DISPATCHES		0




const UINT8 scale_lookup[] = { 0,0,1,0,2,0,0,0,3 };

static UINT16 fp_control[4] = { 0x023f, 0x063f, 0x0a3f, 0x0e3f };
static UINT32 sse_control[4] = { 0x9fc0, 0xbfc0, 0xdfc0, 0xffc0 };


static void append_entry_point(drc_core *drc);
static void append_recompile(drc_core *drc);
static void append_flush(drc_core *drc);
static void append_out_of_cycles(drc_core *drc);

#if LOG_DISPATCHES
static void log_dispatch(drc_core *drc);
#endif


/***************************************************************************
    EXTERNAL INTERFACES
***************************************************************************/

/*------------------------------------------------------------------
    drc_init
------------------------------------------------------------------*/

drc_core *drc_init(UINT8 cpunum, drc_config *config)
{
	int address_bits = config->address_bits;
	int effective_address_bits = address_bits - config->lsbs_to_ignore;
	drc_core *drc;

	/* allocate memory */
	drc = malloc(sizeof(*drc));
	if (!drc)
		return NULL;
	memset(drc, 0, sizeof(*drc));

	/* copy in relevant data from the config */
	drc->pcptr        = config->pcptr;
	drc->icountptr    = config->icountptr;
	drc->esiptr       = config->esiptr;
	drc->cb_reset     = config->cb_reset;
	drc->cb_recompile = config->cb_recompile;
	drc->cb_entrygen  = config->cb_entrygen;
	drc->uses_fp      = config->uses_fp;
	drc->uses_sse     = config->uses_sse;
	drc->pc_in_memory = config->pc_in_memory;
	drc->icount_in_memory = config->icount_in_memory;
	drc->fpcw_curr    = fp_control[0];
	drc->mxcsr_curr   = sse_control[0];

	/* allocate cache */
	drc->cache_size = config->cache_size;
	drc->cache_base = osd_alloc_executable(drc->cache_size);
	if (!drc->cache_base)
		return NULL;
	drc->cache_end = drc->cache_base + drc->cache_size;
	drc->cache_danger = drc->cache_end - 65536;

	/* compute shifts and masks */
	drc->l1bits = effective_address_bits/2;
	drc->l2bits = effective_address_bits - drc->l1bits;
	drc->l1shift = config->lsbs_to_ignore + drc->l2bits;
	drc->l2mask = ((1 << drc->l2bits) - 1) << config->lsbs_to_ignore;
	drc->l2scale = 4 >> config->lsbs_to_ignore;

	/* allocate lookup tables */
	drc->lookup_l1 = malloc(sizeof(*drc->lookup_l1) * (1 << drc->l1bits));
	drc->lookup_l2_recompile = malloc(sizeof(*drc->lookup_l2_recompile) * (1 << drc->l2bits));
	if (!drc->lookup_l1 || !drc->lookup_l2_recompile)
		return NULL;
	memset(drc->lookup_l1, 0, sizeof(*drc->lookup_l1) * (1 << drc->l1bits));
	memset(drc->lookup_l2_recompile, 0, sizeof(*drc->lookup_l2_recompile) * (1 << drc->l2bits));

	/* allocate the sequence and tentative lists */
	drc->sequence_count_max = config->max_instructions;
	drc->sequence_list = malloc(drc->sequence_count_max * sizeof(*drc->sequence_list));
	if (!drc->sequence_list)
		return NULL;
	drc->tentative_count_max = config->max_instructions;
	if (drc->tentative_count_max)
	{
		drc->tentative_list = malloc(drc->tentative_count_max * sizeof(*drc->tentative_list));
		if (!drc->tentative_list)
			return NULL;
	}

	/* seed the cache */
	drc_cache_reset(drc);
	return drc;
}


/*------------------------------------------------------------------
    drc_cache_reset
------------------------------------------------------------------*/

void drc_cache_reset(drc_core *drc)
{
	int i;

	/* reset the cache and add the basics */
	drc->cache_top = drc->cache_base;

	/* append the core entry points to the fresh cache */
	drc->entry_point = (void (*)(void))(UINT32)drc->cache_top;
	append_entry_point(drc);
	drc->out_of_cycles = drc->cache_top;
	append_out_of_cycles(drc);
	drc->recompile = drc->cache_top;
	append_recompile(drc);
	drc->dispatch = drc->cache_top;
	drc_append_dispatcher(drc);
	drc->flush = drc->cache_top;
	append_flush(drc);

	/* populate the recompile table */
	for (i = 0; i < (1 << drc->l2bits); i++)
		drc->lookup_l2_recompile[i] = drc->recompile;

	/* reset all the l1 tables */
	for (i = 0; i < (1 << drc->l1bits); i++)
	{
		/* point NULL entries to the generic recompile table */
		if (drc->lookup_l1[i] == NULL)
			drc->lookup_l1[i] = drc->lookup_l2_recompile;

		/* reset allocated tables to point all entries back to the recompiler */
		else if (drc->lookup_l1[i] != drc->lookup_l2_recompile)
			memcpy(drc->lookup_l1[i], drc->lookup_l2_recompile, sizeof(*drc->lookup_l2_recompile) * (1 << drc->l2bits));
	}

	/* call back to the host */
	if (drc->cb_reset)
		(*drc->cb_reset)(drc);
}


/*------------------------------------------------------------------
    drc_execute
------------------------------------------------------------------*/

void drc_execute(drc_core *drc)
{
	(*drc->entry_point)();
}


/*------------------------------------------------------------------
    drc_exit
------------------------------------------------------------------*/

void drc_exit(drc_core *drc)
{
	int i;

	/* free the cache */
	if (drc->cache_base)
	  osd_free_executable(drc->cache_base, drc->cache_size);

	/* free all the l2 tables allocated */
	for (i = 0; i < (1 << drc->l1bits); i++)
		if (drc->lookup_l1[i] != drc->lookup_l2_recompile)
			free(drc->lookup_l1[i]);

	/* free the l1 table */
	if (drc->lookup_l1)
		free(drc->lookup_l1);

	/* free the default l2 table */
	if (drc->lookup_l2_recompile)
		free(drc->lookup_l2_recompile);

	/* free the lists */
	if (drc->sequence_list)
		free(drc->sequence_list);
	if (drc->tentative_list)
		free(drc->tentative_list);

	/* and the drc itself */
	free(drc);
}


/*------------------------------------------------------------------
    drc_begin_sequence
------------------------------------------------------------------*/

void drc_begin_sequence(drc_core *drc, UINT32 pc)
{
	UINT32 l1index = pc >> drc->l1shift;
	UINT32 l2index = ((pc & drc->l2mask) * drc->l2scale) / 4;

	/* reset the sequence and tentative counts */
	drc->sequence_count = 0;
	drc->tentative_count = 0;

	/* allocate memory if necessary */
	if (drc->lookup_l1[l1index] == drc->lookup_l2_recompile)
	{
		/* create a new copy of the recompile table */
		drc->lookup_l1[l1index] = malloc_or_die(sizeof(*drc->lookup_l2_recompile) * (1 << drc->l2bits));

		memcpy(drc->lookup_l1[l1index], drc->lookup_l2_recompile, sizeof(*drc->lookup_l2_recompile) * (1 << drc->l2bits));
	}

	/* nuke any previous link to this instruction */
	if (drc->lookup_l1[l1index][l2index] != drc->recompile)
	{
		UINT8 *cache_save = drc->cache_top;
		drc->cache_top = drc->lookup_l1[l1index][l2index];
		_jmp(drc->dispatch);
		drc->cache_top = cache_save;
	}

	/* note the current location for this instruction */
	drc->lookup_l1[l1index][l2index] = drc->cache_top;
}


/*------------------------------------------------------------------
    drc_end_sequence
------------------------------------------------------------------*/

void drc_end_sequence(drc_core *drc)
{
	int i, j;

	/* fix up any internal links */
	for (i = 0; i < drc->tentative_count; i++)
		for (j = 0; j < drc->sequence_count; j++)
			if (drc->tentative_list[i].pc == drc->sequence_list[j].pc)
			{
				UINT8 *cache_save = drc->cache_top;
				drc->cache_top = drc->tentative_list[i].target;
				_jmp(drc->sequence_list[j].target);
				drc->cache_top = cache_save;
				break;
			}
}


/*------------------------------------------------------------------
    drc_register_code_at_cache_top
------------------------------------------------------------------*/

void drc_register_code_at_cache_top(drc_core *drc, UINT32 pc)
{
	pc_ptr_pair *pair = &drc->sequence_list[drc->sequence_count++];
	assert_always(drc->sequence_count <= drc->sequence_count_max, "drc_register_code_at_cache_top: too many instructions!");

	pair->target = drc->cache_top;
	pair->pc = pc;
}


/*------------------------------------------------------------------
    drc_get_code_at_pc
------------------------------------------------------------------*/

void *drc_get_code_at_pc(drc_core *drc, UINT32 pc)
{
	UINT32 l1index = pc >> drc->l1shift;
	UINT32 l2index = ((pc & drc->l2mask) * drc->l2scale) / 4;
	return (drc->lookup_l1[l1index][l2index] != drc->recompile) ? drc->lookup_l1[l1index][l2index] : NULL;
}


/*------------------------------------------------------------------
    drc_append_verify_code
------------------------------------------------------------------*/

void drc_append_verify_code(drc_core *drc, void *code, UINT8 length)
{
	if (length > 8)
	{
		UINT32 *codeptr = code, sum = 0;
		void *target;
		int i;

		for (i = 0; i < length / 4; i++)
		{
			sum = (sum >> 1) | (sum << 31);
			sum += *codeptr++;
		}

		_xor_r32_r32(REG_EAX, REG_EAX);								// xor  eax,eax
		_mov_r32_imm(REG_EBX, code);								// mov  ebx,code
		_mov_r32_imm(REG_ECX, length / 4);							// mov  ecx,length / 4
		target = drc->cache_top;									// target:
		_ror_r32_imm(REG_EAX, 1);									// ror  eax,1
		_add_r32_m32bd(REG_EAX, REG_EBX, 0);						// add  eax,[ebx]
		_sub_or_dec_r32_imm(REG_ECX, 1);							// sub  ecx,1
		_lea_r32_m32bd(REG_EBX, REG_EBX, 4);						// lea  ebx,[ebx+4]
		_jcc(COND_NZ, target);										// jnz  target
		_cmp_r32_imm(REG_EAX, sum);									// cmp  eax,sum
		_jcc(COND_NE, drc->recompile);								// jne  recompile
	}
	else if (length >= 12)
	{
		_cmp_m32abs_imm(code, *(UINT32 *)code);						// cmp  [pc],opcode
		_jcc(COND_NE, drc->recompile);								// jne  recompile
		_cmp_m32abs_imm((UINT8 *)code + 4, ((UINT32 *)code)[1]);	// cmp  [pc+4],opcode+4
		_jcc(COND_NE, drc->recompile);								// jne  recompile
		_cmp_m32abs_imm((UINT8 *)code + 8, ((UINT32 *)code)[2]);	// cmp  [pc+8],opcode+8
		_jcc(COND_NE, drc->recompile);								// jne  recompile
	}
	else if (length >= 8)
	{
		_cmp_m32abs_imm(code, *(UINT32 *)code);						// cmp  [pc],opcode
		_jcc(COND_NE, drc->recompile);								// jne  recompile
		_cmp_m32abs_imm((UINT8 *)code + 4, ((UINT32 *)code)[1]);	// cmp  [pc+4],opcode+4
		_jcc(COND_NE, drc->recompile);								// jne  recompile
	}
	else if (length >= 4)
	{
		_cmp_m32abs_imm(code, *(UINT32 *)code);						// cmp  [pc],opcode
		_jcc(COND_NE, drc->recompile);								// jne  recompile
	}
	else if (length >= 2)
	{
		_cmp_m16abs_imm(code, *(UINT16 *)code);						// cmp  [pc],opcode
		_jcc(COND_NE, drc->recompile);								// jne  recompile
	}
	else
	{
		_cmp_m8abs_imm(code, *(UINT8 *)code);						// cmp  [pc],opcode
		_jcc(COND_NE, drc->recompile);								// jne  recompile
	}
}


/*------------------------------------------------------------------
    drc_append_call_debugger
------------------------------------------------------------------*/

void drc_append_call_debugger(drc_core *drc)
{
#ifdef MAME_DEBUG
	if (Machine->debug_mode)
	{
		link_info link;
		_cmp_m32abs_imm(&Machine->debug_mode, 0);						// cmp  [Machine->debug_mode],0
		_jcc_short_link(COND_E, &link);									// je   skip
		_sub_r32_imm(REG_ESP, 12);										// align stack
		drc_append_save_call_restore(drc, (genf *)mame_debug_hook, 12);	// save volatiles
		_resolve_link(&link);
	}
#endif
}


/*------------------------------------------------------------------
    drc_append_save_volatiles
------------------------------------------------------------------*/

void drc_append_save_volatiles(drc_core *drc)
{
	if (drc->icountptr && !drc->icount_in_memory)
		_mov_m32abs_r32(drc->icountptr, REG_EBP);
	if (drc->pcptr && !drc->pc_in_memory)
		_mov_m32abs_r32(drc->pcptr, REG_EDI);
	if (drc->esiptr)
		_mov_m32abs_r32(drc->esiptr, REG_ESI);
}


/*------------------------------------------------------------------
    drc_append_restore_volatiles
------------------------------------------------------------------*/

void drc_append_restore_volatiles(drc_core *drc)
{
	if (drc->icountptr && !drc->icount_in_memory)
		_mov_r32_m32abs(REG_EBP, drc->icountptr);
	if (drc->pcptr && !drc->pc_in_memory)
		_mov_r32_m32abs(REG_EDI, drc->pcptr);
	if (drc->esiptr)
		_mov_r32_m32abs(REG_ESI, drc->esiptr);
}


/*------------------------------------------------------------------
    drc_append_save_call_restore
------------------------------------------------------------------*/

void drc_append_save_call_restore(drc_core *drc, genf *target, UINT32 stackadj)
{
	drc_append_save_volatiles(drc);									// save volatiles
	_call(target);													// call target
	drc_append_restore_volatiles(drc);								// restore volatiles
	if (stackadj)
		_add_r32_imm(REG_ESP, stackadj);							// adjust stack
}


/*------------------------------------------------------------------
    drc_append_standard_epilogue
------------------------------------------------------------------*/

void drc_append_standard_epilogue(drc_core *drc, INT32 cycles, INT32 pcdelta, int allow_exit)
{
	if (pcdelta != 0 && drc->pc_in_memory)
		_add_m32abs_imm(drc->pcptr, pcdelta);						// add  [pc],pcdelta
	if (cycles != 0)
	{
		if (drc->icount_in_memory)
			_sub_m32abs_imm(drc->icountptr, cycles);				// sub  [icount],cycles
		else
			_sub_or_dec_r32_imm(REG_EBP, cycles);					// sub  ebp,cycles
	}
	if (pcdelta != 0 && !drc->pc_in_memory)
		_lea_r32_m32bd(REG_EDI, REG_EDI, pcdelta);					// lea  edi,[edi+pcdelta]
	if (allow_exit && cycles != 0)
		_jcc(COND_S, drc->out_of_cycles);							// js   out_of_cycles
}


/*------------------------------------------------------------------
    drc_append_dispatcher
------------------------------------------------------------------*/

void drc_append_dispatcher(drc_core *drc)
{
#if LOG_DISPATCHES
	_sub_r32_imm(REG_ESP, 8);										// align stack
	_push_imm(drc);													// push drc
	drc_append_save_call_restore(drc, (void *)log_dispatch, 12);	// call log_dispatch
#endif
	if (drc->pc_in_memory)
		_mov_r32_m32abs(REG_EDI, drc->pcptr);						// mov  edi,[pc]
	_mov_r32_r32(REG_EAX, REG_EDI);									// mov  eax,edi
	_shr_r32_imm(REG_EAX, drc->l1shift);							// shr  eax,l1shift
	_mov_r32_r32(REG_EDX, REG_EDI);									// mov  edx,edi
	_mov_r32_m32isd(REG_EAX, REG_EAX, 4, drc->lookup_l1);			// mov  eax,[eax*4 + l1lookup]
	_and_r32_imm(REG_EDX, drc->l2mask);								// and  edx,l2mask
	_jmp_m32bisd(REG_EAX, REG_EDX, drc->l2scale, 0);				// jmp  [eax+edx*l2scale]
}


/*------------------------------------------------------------------
    drc_append_fixed_dispatcher
------------------------------------------------------------------*/

void drc_append_fixed_dispatcher(drc_core *drc, UINT32 newpc)
{
	void **base = drc->lookup_l1[newpc >> drc->l1shift];
	if (base == drc->lookup_l2_recompile)
	{
		_mov_r32_m32abs(REG_EAX, &drc->lookup_l1[newpc >> drc->l1shift]);// mov eax,[(newpc >> l1shift)*4 + l1lookup]
		_jmp_m32bd(REG_EAX, (newpc & drc->l2mask) * drc->l2scale);		// jmp  [eax+(newpc & l2mask)*l2scale]
	}
	else
		_jmp_m32abs((UINT8 *)base + (newpc & drc->l2mask) * drc->l2scale);	// jmp  [eax+(newpc & l2mask)*l2scale]
}


/*------------------------------------------------------------------
    drc_append_tentative_fixed_dispatcher
------------------------------------------------------------------*/

void drc_append_tentative_fixed_dispatcher(drc_core *drc, UINT32 newpc)
{
	pc_ptr_pair *pair = &drc->tentative_list[drc->tentative_count++];
	assert_always(drc->tentative_count <= drc->tentative_count_max, "drc_append_tentative_fixed_dispatcher: too many tentative branches!");

	pair->target = drc->cache_top;
	pair->pc = newpc;
	drc_append_fixed_dispatcher(drc, newpc);
}


/*------------------------------------------------------------------
    drc_append_set_fp_rounding
------------------------------------------------------------------*/

void drc_append_set_fp_rounding(drc_core *drc, UINT8 regindex)
{
	_fldcw_m16isd(regindex, 2, &fp_control[0]);						// fldcw [fp_control + reg*2]
	_fnstcw_m16abs(&drc->fpcw_curr);								// fnstcw [fpcw_curr]
}



/*------------------------------------------------------------------
    drc_append_set_temp_fp_rounding
------------------------------------------------------------------*/

void drc_append_set_temp_fp_rounding(drc_core *drc, UINT8 rounding)
{
	_fldcw_m16abs(&fp_control[rounding]);							// fldcw [fp_control]
}



/*------------------------------------------------------------------
    drc_append_restore_fp_rounding
------------------------------------------------------------------*/

void drc_append_restore_fp_rounding(drc_core *drc)
{
	_fldcw_m16abs(&drc->fpcw_curr);									// fldcw [fpcw_curr]
}



/*------------------------------------------------------------------
    drc_append_set_sse_rounding
------------------------------------------------------------------*/

void drc_append_set_sse_rounding(drc_core *drc, UINT8 regindex)
{
	_ldmxcsr_m32isd(regindex, 4, &sse_control[0]);					// ldmxcsr [sse_control + reg*2]
	_stmxcsr_m32abs(&drc->mxcsr_curr);								// stmxcsr [mxcsr_curr]
}



/*------------------------------------------------------------------
    drc_append_set_temp_sse_rounding
------------------------------------------------------------------*/

void drc_append_set_temp_sse_rounding(drc_core *drc, UINT8 rounding)
{
	_ldmxcsr_m32abs(&sse_control[rounding]);						// ldmxcsr [sse_control]
}



/*------------------------------------------------------------------
    drc_append_restore_sse_rounding
------------------------------------------------------------------*/

void drc_append_restore_sse_rounding(drc_core *drc)
{
	_ldmxcsr_m32abs(&drc->mxcsr_curr);								// ldmxcsr [mxcsr_curr]
}



/*------------------------------------------------------------------
    drc_dasm

    An attempt to make a disassembler for DRC code; currently limited
    by the functionality of DasmI386
------------------------------------------------------------------*/

void drc_dasm(FILE *f, const void *begin, const void *end)
{
	extern int i386_dasm_one(char *buffer, UINT32 eip, UINT8 *oprom, int mode);

	char buffer[256];
	const UINT8 *begin_ptr = (const UINT8 *) begin;
	const UINT8 *end_ptr = (const UINT8 *) end;
	UINT32 pc = (UINT32) begin;
	int length;

	while(begin_ptr < end_ptr)
	{
#if defined(MAME_DEBUG) && HAS_I386
		length = i386_dasm_one(buffer, pc, (UINT8 *) begin_ptr, 32) & DASMFLAG_LENGTHMASK;
#else
		sprintf(buffer, "%02X", *begin_ptr);
		length = 1;
#endif

		fprintf(f, "%08X:\t%s\n", (unsigned) pc, buffer);
		begin_ptr += length;
		pc += length;
	}
}




/***************************************************************************
    INTERNAL CODEGEN
***************************************************************************/

/*------------------------------------------------------------------
    append_entry_point
------------------------------------------------------------------*/

static void append_entry_point(drc_core *drc)
{
	_pushad();														// pushad
	if (drc->uses_fp)
	{
		_fnstcw_m16abs(&drc->fpcw_save);							// fstcw [fpcw_save]
		_fldcw_m16abs(&drc->fpcw_curr);								// fldcw [fpcw_curr]
	}
	if (drc->uses_sse)
	{
		_stmxcsr_m32abs(&drc->mxcsr_save);							// stmxcsr [mxcsr_save]
		_ldmxcsr_m32abs(&drc->mxcsr_curr);							// ldmxcsr [mxcsr_curr]
	}
	drc_append_restore_volatiles(drc);								// load volatiles
	if (drc->cb_entrygen)
		(*drc->cb_entrygen)(drc);									// additional entry point duties
	drc_append_dispatcher(drc);										// dispatch
}


/*------------------------------------------------------------------
    recompile_code
------------------------------------------------------------------*/

static void recompile_code(drc_core *drc)
{
	if (drc->cache_top >= drc->cache_danger)
		drc_cache_reset(drc);
	(*drc->cb_recompile)(drc);
}


/*------------------------------------------------------------------
    append_recompile
------------------------------------------------------------------*/

static void append_recompile(drc_core *drc)
{
	_sub_r32_imm(REG_ESP, 8);										// align stack
	_push_imm(drc);													// push drc
	drc_append_save_call_restore(drc, (genf *)recompile_code, 12);	// call recompile_code
	drc_append_dispatcher(drc);										// dispatch
}


/*------------------------------------------------------------------
    append_flush
------------------------------------------------------------------*/

static void append_flush(drc_core *drc)
{
	_sub_r32_imm(REG_ESP, 8);										// align stack
	_push_imm(drc);													// push drc
	drc_append_save_call_restore(drc, (genf *)drc_cache_reset, 12);	// call drc_cache_reset
	drc_append_dispatcher(drc);										// dispatch
}


/*------------------------------------------------------------------
    append_out_of_cycles
------------------------------------------------------------------*/

static void append_out_of_cycles(drc_core *drc)
{
	drc_append_save_volatiles(drc);									// save volatiles
	if (drc->uses_fp)
	{
		_fnclex();													// fnclex
		_fldcw_m16abs(&drc->fpcw_save);								// fldcw [fpcw_save]
	}
	if (drc->uses_sse)
		_ldmxcsr_m32abs(&drc->mxcsr_save);							// ldmxcsr [mxcsr_save]
	_popad();														// popad
	_ret();															// ret
}



/*------------------------------------------------------------------
    drc_x86_get_features()
------------------------------------------------------------------*/
UINT32 drc_x86_get_features(void)
{
	UINT32 features = 0;
#ifdef _MSC_VER
	__asm
	{
		mov eax, 1
		xor ebx, ebx
		xor ecx, ecx
		xor edx, edx
		__asm _emit 0Fh __asm _emit 0A2h	// cpuid
		mov features, edx
	}
#else /* !_MSC_VER */
	__asm__
	(
	        "pushl %%ebx         ; "
		"movl $1,%%eax       ; "
		"xorl %%ebx,%%ebx    ; "
		"xorl %%ecx,%%ecx    ; "
		"xorl %%edx,%%edx    ; "
		"cpuid               ; "
		"movl %%edx,%0       ; "
                "popl %%ebx          ; "
	: "=&a" (features)		/* result has to go in eax */
	: 				/* no inputs */
	: "%ecx", "%edx"	/* clobbers ebx, ecx and edx */
	);
#endif /* MSC_VER */
	return features;
}



/*------------------------------------------------------------------
    log_dispatch
------------------------------------------------------------------*/

#if LOG_DISPATCHES
static void log_dispatch(drc_core *drc)
{
	if (code_pressed(KEYCODE_D))
		logerror("Disp:%08X\n", *drc->pcptr);
}
#endif
