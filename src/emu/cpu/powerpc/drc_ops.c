/* PowerPC common opcodes */

// it really seems like this should be elsewhere - like maybe the floating point checks can hang out someplace else
#include <math.h>

#define USE_SSE2			0
#define COMPILE_FPU			0

#define PPCDRC_STRICT_VERIFY		0x0001			/* verify all instructions */

#define PPCDRC_COMPATIBLE_OPTIONS	(PPCDRC_STRICT_VERIFY)
#define PPCDRC_FASTEST_OPTIONS		0

/* recompiler flags */
#define RECOMPILE_UNIMPLEMENTED			0x0000
#define RECOMPILE_SUCCESSFUL			0x0001
#define RECOMPILE_SUCCESSFUL_CP(c,p)	(RECOMPILE_SUCCESSFUL | (((c) & 0xff) << 16) | (((p) & 0xff) << 24))
#define RECOMPILE_END_OF_STRING			0x0002
#define RECOMPILE_ADD_DISPATCH			0x0004

static UINT32 compile_one(drc_core *drc, UINT32 pc);

static void append_generate_exception(drc_core *drc, UINT8 exception);
static void append_check_interrupts(drc_core *drc, int inline_generate);
static UINT32 recompile_instruction(drc_core *drc, UINT32 pc, UINT32 *opptr);

static UINT32 temp_ppc_pc;

static void ppcdrc_init(void)
{
	drc_config drconfig;

	/* fill in the config */
	memset(&drconfig, 0, sizeof(drconfig));
	drconfig.cache_size       = CACHE_SIZE;
	drconfig.max_instructions = MAX_INSTRUCTIONS;
	drconfig.address_bits     = 32;
	drconfig.lsbs_to_ignore   = 2;
	drconfig.uses_fp          = 1;
	drconfig.uses_sse         = USE_SSE2;
	drconfig.pc_in_memory     = 0;
	drconfig.icount_in_memory = 0;
	drconfig.pcptr            = (UINT32 *)&ppc.pc;
	drconfig.icountptr        = (UINT32 *)&ppc_icount;
	drconfig.esiptr           = NULL;
	drconfig.cb_reset         = ppcdrc_reset;
	drconfig.cb_recompile     = ppcdrc_recompile;
	drconfig.cb_entrygen      = ppcdrc_entrygen;

	/* initialize the compiler */
	ppc.drc = drc_init(cpu_getactivecpu(), &drconfig);
	ppc.drcoptions = PPCDRC_FASTEST_OPTIONS;
}

static void ppcdrc_reset(drc_core *drc)
{
	ppc.invoke_exception_handler = drc->cache_top;
	drc_append_restore_volatiles(drc);
	_mov_r32_m32bd(REG_EAX, REG_ESP, 4);
	_mov_r32_m32abs(REG_ESP, &ppc.host_esp);
	_mov_m32abs_r32(&SRR0, REG_EDI);		/* save return address */
	_jmp_r32(REG_EAX);

	ppc.generate_interrupt_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_IRQ);

	ppc.generate_syscall_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_SYSTEM_CALL);

	ppc.generate_decrementer_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_DECREMENTER);

	ppc.generate_trap_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_TRAP);

	ppc.generate_dsi_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_DSI);

	ppc.generate_isi_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_ISI);

	if (!ppc.is603 && !ppc.is602)
	{
		ppc.generate_fit_exception = drc->cache_top;
		append_generate_exception(drc, EXCEPTION_FIXED_INTERVAL_TIMER);
	}
}

static UINT32 *ppcdrc_getopptr(UINT32 address)
{
	UINT32 *result;
	UINT32 offset = 0;

	if (ppc.is603 || ppc.is602)
	{
		if (MSR & MSR_IR)
		{
			if (!ppc_translate_address(&address, PPC_TRANSLATE_CODE | PPC_TRANSLATE_READ | PPC_TRANSLATE_NOEXCEPTION))
				return NULL;
		}
		address = DWORD_XOR_BE(address);
		offset = (address & 0x07) / sizeof(*result);
		address &= ~0x07;
	}

	result = (UINT32 *) memory_get_op_ptr(cpu_getactivecpu(), address, 0);
	if (result)
		result += offset;
	return result;
}

static void ppcdrc_recompile(drc_core *drc)
{
	int remaining = MAX_INSTRUCTIONS;
	UINT32 pc = ppc.pc;
	UINT32 *opptr;

	/* begin the sequence */
	drc_begin_sequence(drc, pc);

	/* loose verification case: one verification here only */
	if (!(ppc.drcoptions & PPCDRC_STRICT_VERIFY))
	{
		opptr = ppcdrc_getopptr(pc);
		if (opptr)
			drc_append_verify_code(drc, opptr, 4);
	}

	/* loop until we hit an unconditional branch */
	while (--remaining != 0)
	{
		UINT32 result;

		/* compile one instruction */
		result = compile_one(drc, pc);
		pc += (INT8)(result >> 24);
		if (result & RECOMPILE_END_OF_STRING)
			break;

		/* do not recompile across MMU page boundaries */
		if ((pc & 0x0FFF) == 0)
		{
			remaining = 0;
			break;
		}
	}

	/* add dispatcher just in case */
	if (remaining == 0)
		drc_append_dispatcher(drc);

	/* end the sequence */
	drc_end_sequence(drc);
	log_code(drc);
}

static void update_counters(drc_core *drc)
{
	link_info link1;

	/* decrementer */
	if (ppc.is603 || ppc.is602)
	{
		_cmp_r32_m32abs(REG_EBP, &ppc_dec_trigger_cycle);
		_jcc_short_link(COND_NZ, &link1);
		_or_m32abs_imm(&ppc.exception_pending, 0x2);
		_resolve_link(&link1);
	}

	/* FIT */
	if (!ppc.is603 && !ppc.is602)
	{
		link_info link2;
		_cmp_r32_m32abs(REG_EBP, &ppc_fit_trigger_cycle);
		_jcc_short_link(COND_NZ, &link1);
		_cmp_r32_m32abs(REG_EBP, &ppc.fit_int_enable);
		_jcc_short_link(COND_Z, &link2);
		_or_m32abs_imm(&ppc.exception_pending, 0x4);
		_resolve_link(&link1);
		_resolve_link(&link2);
	}
}

static void ppcdrc_entrygen(drc_core *drc)
{
	_mov_m32abs_r32(&ppc.host_esp, REG_ESP);
	append_check_interrupts(drc, 0);
}

static UINT32 compile_one(drc_core *drc, UINT32 pc)
{
	int pcdelta, cycles;
	UINT32 *opptr;
	UINT32 result;

	/* register this instruction */
	drc_register_code_at_cache_top(drc, pc);

	/* get a pointer to the current instruction */
	opptr = ppcdrc_getopptr(pc);

	//log_symbol(drc, ~2);
	//log_symbol(drc, pc);

	/* emit debugging call */
	drc_append_call_debugger(drc);

	/* null opptr?  if legit, we need to generate an ISI exception */
	if (!opptr)
	{
		/* first check to see if the code is up to date; if not, recompile */
		_push_imm(pc);
		_call(ppcdrc_getopptr);
		_add_r32_imm(REG_ESP, 4);
		_cmp_r32_imm(REG_EAX, 0);
		_jcc(COND_NZ, drc->recompile);

		/* code is up to date; do the exception */
		_mov_m32abs_r32(&SRR0, REG_EDI);		/* save return address */
		_mov_r32_m32abs(REG_EAX, &ppc.generate_isi_exception);
		_jmp_r32(REG_EAX);
		return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;
	}

	/* emit self-modifying code checks */
	if (ppc.drcoptions & PPCDRC_STRICT_VERIFY)
		drc_append_verify_code(drc, opptr, 4);

	/* compile the instruction */
	result = recompile_instruction(drc, pc, opptr);

	/* handle the results */
	if (!(result & RECOMPILE_SUCCESSFUL))
		fatalerror("Unimplemented op %08X", *opptr);

	pcdelta = (INT8)(result >> 24);
	cycles = (INT8)(result >> 16);

	/* epilogue */
	update_counters(drc);
	drc_append_standard_epilogue(drc, cycles, pcdelta, 1);

	if (result & RECOMPILE_ADD_DISPATCH)
		drc_append_dispatcher(drc);

	return (result & 0xffff) | ((UINT8)cycles << 16) | ((UINT8)pcdelta << 24);
}

static UINT32 recompile_instruction(drc_core *drc, UINT32 pc, UINT32 *opptr)
{
	UINT32 opcode;
	temp_ppc_pc = pc;

	opcode = *opptr;

	if (opcode != 0) {		// this is a little workaround for VF3
		switch(opcode >> 26)
		{
			case 19:	return optable19[(opcode >> 1) & 0x3ff](drc, opcode);
			case 31:	return optable31[(opcode >> 1) & 0x3ff](drc, opcode);
			case 59:	return optable59[(opcode >> 1) & 0x3ff](drc, opcode);
			case 63:	return optable63[(opcode >> 1) & 0x3ff](drc, opcode);
			default:	return optable[opcode >> 26](drc, opcode);
		}
	}
	return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;
}


static const UINT32 exception_vector[32] =
{
	0x0000, 0x0500, 0x0900, 0x0700, 0x0c00, 0x1400, 0x0300, 0x0400,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x1000, 0x1010, 0x1020
};

static void append_generate_exception(drc_core *drc, UINT8 exception)
{
	link_info link1, link2, link3;

	_mov_r32_m32abs(REG_EAX, &ppc.msr);
	_and_r32_imm(REG_EAX, 0xff73);
	_mov_m32abs_r32(&SRR1, REG_EAX);
	_mov_r32_m32abs(REG_EAX, &ppc.msr);

	// Clear POW, EE, PR, FP, FE0, SE, BE, FE1, IR, DR, RI
	_and_r32_imm(REG_EAX, ~(MSR_POW | MSR_EE | MSR_PR | MSR_FP | MSR_FE0 | MSR_SE | MSR_BE | MSR_FE1 | MSR_IR | MSR_DR | MSR_RI));
	// Set LE to ILE
	_and_r32_imm(REG_EAX, ~MSR_LE);		// clear LE first
	_test_r32_imm(REG_EAX, MSR_ILE);
	_jcc_short_link(COND_Z, &link1);	// if Z == 0, bit == 1
	_or_r32_imm(REG_EAX, MSR_LE);		// set LE
	_resolve_link(&link1);
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_r32(REG_EAX);
	_call((genf *)ppc_set_msr);
	_pop_r32(REG_EDX);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	if (ppc.is603)
	{
		_mov_r32_imm(REG_EDI, exception_vector[exception]);		// first move the exception handler offset
		_test_r32_imm(REG_EDX, MSR_IP);							// test if the base should be 0xfff0 or EVPR
		_jcc_short_link(COND_Z, &link2);						// if Z == 1, bit == 0 means base == 0x00000000
		_or_r32_imm(REG_EDI, 0xfff00000);						// else base == 0xfff00000
		_resolve_link(&link2);
	}
	else if (ppc.is602)
	{
		_mov_r32_imm(REG_EDI, exception_vector[exception]);		// first move the exception handler offset
		_test_r32_imm(REG_EDX, MSR_IP);							// test if the base should be 0xfff0 or IBR
		_jcc_short_link(COND_NZ, &link2);						// if Z == 0, bit == 1 means base == 0xfff00000
		_or_r32_m32abs(REG_EDI, &ppc.ibr);						// else base == IBR
		_jmp_short_link(&link3);
		_resolve_link(&link2);
		_or_r32_imm(REG_EDI, 0xfff00000);
		_resolve_link(&link3);
	}
	else
	{
		_mov_r32_imm(REG_EDI, exception_vector[exception]);		// first move the exception handler offset
		_test_r32_imm(REG_EDX, MSR_IP);							// test if the base should be 0xfff0 or EVPR
		_jcc_short_link(COND_NZ, &link2);						// if Z == 0, bit == 1 means base == 0xfff00000
		_or_r32_m32abs(REG_EDI, &EVPR);							// else base == EVPR
		_jmp_short_link(&link3);
		_resolve_link(&link2);
		_or_r32_imm(REG_EDI, 0xfff00000);
		_resolve_link(&link3);
	}

	if (exception == EXCEPTION_IRQ)
	{
		_and_m32abs_imm(&ppc.exception_pending, ~0x1);		// clear pending irq
	}
	if (exception == EXCEPTION_DECREMENTER)
	{
		_and_m32abs_imm(&ppc.exception_pending, ~0x2);		// clear pending decrementer exception
	}
	if (exception == EXCEPTION_FIXED_INTERVAL_TIMER)
	{
		_and_m32abs_imm(&ppc.exception_pending, ~0x4);		// clear pending fit exception
	}

	drc_append_dispatcher(drc);
}

static void append_check_interrupts(drc_core *drc, int inline_generate)
{
	if (ppc.is602 || ppc.is603)
	{
		link_info link1, link2, link3, link4;
		_test_m32abs_imm(&ppc.msr, MSR_EE);		/* no interrupt if external interrupts are not enabled */
		_jcc_short_link(COND_Z, &link1);		/* ZF = 1 if bit == 0 */

		/* else check if any interrupt are pending */
		_mov_r32_m32abs(REG_EAX, &ppc.exception_pending);
		_cmp_r32_imm(REG_EAX, 0);
		_jcc_short_link(COND_Z, &link2);		/* reg == 0, no exceptions are pending */

		/* else handle the first pending exception */
		_test_r32_imm(REG_EAX, 0x1);			/* is it a IRQ? */
		_jcc_short_link(COND_Z, &link3);

		_mov_m32abs_r32(&SRR0, REG_EDI);		/* save return address */
		_mov_r32_m32abs(REG_EAX, &ppc.generate_interrupt_exception);
		_jmp_r32(REG_EAX);
		_resolve_link(&link3);

		_test_r32_imm(REG_EAX, 0x2);			/* is it a decrementer exception */
		_jcc_short_link(COND_Z, &link4);
		_mov_m32abs_r32(&SRR0, REG_EDI);		/* save return address */
		_mov_r32_m32abs(REG_EAX, &ppc.generate_decrementer_exception);
		_jmp_r32(REG_EAX);
		_resolve_link(&link4);

		_resolve_link(&link1);
		_resolve_link(&link2);
	}
	else
	{
		link_info link1, link2, link3, link4, link5, link6;
		_test_m32abs_imm(&ppc.msr, MSR_EE);		/* no interrupt if external interrupts are not enabled */
		_jcc_short_link(COND_Z, &link1);		/* ZF = 1 if bit == 0 */

		/* else check if any interrupt are pending */
		_mov_r32_m32abs(REG_EAX, &ppc.exception_pending);
		_cmp_r32_imm(REG_EAX, 0);
		_jcc_short_link(COND_Z, &link2);		/* reg == 0, no exceptions are pending */

		/* else handle the first pending exception */
		_test_r32_imm(REG_EAX, 0x1);			/* is it a IRQ? */
		_jcc_short_link(COND_Z, &link3);

		_mov_r32_m32abs(REG_EAX, &ppc.exisr);
		_and_r32_m32abs(REG_EAX, &ppc.exier);
		_cmp_r32_imm(REG_EAX, 0);
		_jcc_short_link(COND_Z, &link4);

		_mov_m32abs_r32(&SRR0, REG_EDI);		/* save return address */
		_mov_r32_m32abs(REG_EAX, &ppc.generate_interrupt_exception);
		_jmp_r32(REG_EAX);

		/* check if it's FIT exception */
		_resolve_link(&link3);
		_test_r32_imm(REG_EAX, 0x4);
		_jcc_short_link(COND_Z, &link5);

		// check if FIT interrupts are enabled
		_test_m32abs_imm(&ppc.fit_int_enable, 0x1);
		_jcc_short_link(COND_Z, &link6);

		// calculate the next trigger cycle for FIT
		_mov_r32_m32abs(REG_EAX, &ppc_fit_trigger_cycle);
		_sub_r32_m32abs(REG_EAX, &ppc.fit_bit);
		_mov_m32abs_r32(&ppc_fit_trigger_cycle, REG_EAX);

		_mov_m32abs_r32(&SRR0, REG_EDI);		/* save return address */
		_mov_r32_m32abs(REG_EAX, &ppc.generate_fit_exception);
		_jmp_r32(REG_EAX);

		_resolve_link(&link1);
		_resolve_link(&link2);
		_resolve_link(&link4);
		_resolve_link(&link5);
		_resolve_link(&link6);
	}
}

static void append_branch_or_dispatch(drc_core *drc, UINT32 newpc, int cycles)
{
	void *code = drc_get_code_at_pc(drc, newpc);
	_mov_r32_imm(REG_EDI, newpc);

	update_counters(drc);
	append_check_interrupts(drc, 0);

	drc_append_standard_epilogue(drc, cycles, 0, 1);


	if (code)
		_jmp(code);
	else
		drc_append_tentative_fixed_dispatcher(drc, newpc);
}

/*
// this table translates x86 SF and ZF flags to PPC CR values
static UINT8 condition_table[4] =
{
    0x4,    // x86 SF == 0, ZF == 0   -->   PPC GT (positive)
    0x2,    // x86 SF == 0, ZF == 1   -->   PPC EQ (zero)
    0x8,    // x86 SF == 1, ZF == 0   -->   PPC LT (negative)
    0x0,    // x86 SF == 1, ZF == 1 (impossible)
};
*/

// expects the result value in EDX!!!
static void append_set_cr0(drc_core *drc)
{
	_xor_r32_r32(REG_EBX, REG_EBX);
	_xor_r32_r32(REG_EAX, REG_EAX);
	_cmp_r32_imm(REG_EDX, 0);
	/*_lahf();

    _shr_r32_imm(REG_EAX, 14);

    _add_r32_imm(REG_EAX, &condition_table);
    _mov_r8_m8bd(REG_BL, REG_EAX, 0);
    */

	_setcc_r8(COND_Z, REG_AL);
	_setcc_r8(COND_L, REG_AH);
	_setcc_r8(COND_G, REG_BL);
	_shl_r8_imm(REG_AL, 1);
	_shl_r8_imm(REG_AH, 3);
	_shl_r8_imm(REG_BL, 2);
	_or_r8_r8(REG_BL, REG_AH);
	_or_r8_r8(REG_BL, REG_AL);

	_bt_m32abs_imm(&XER, 31);		// set XER SO bit to carry
	_adc_r32_imm(REG_EBX, 0);		// effectively sets bit 0 to carry

	_mov_m8abs_r8(&ppc.cr[0], REG_BL);
}

static void append_set_cr1(drc_core *drc)
{
	_mov_r32_m32abs(REG_EAX, &ppc.fpscr);
	_shr_r32_imm(REG_EAX, 28);
	_and_r32_imm(REG_EAX, 0xf);
	_mov_m8abs_r8(&ppc.cr[1], REG_AL);
}

static UINT32 recompile_addx(drc_core *drc, UINT32 op)
{
	_mov_r32_m32abs(REG_EDX, &REG(RA));
	_add_r32_m32abs(REG_EDX, &REG(RB));
	_mov_m32abs_r32(&REG(RT), REG_EDX);

	if (OEBIT) {
		mame_printf_debug("recompile_addx: OE bit set !\n");
		return RECOMPILE_UNIMPLEMENTED;
	}
	if (RCBIT) {
		append_set_cr0(drc);
	}

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_addcx(drc_core *drc, UINT32 op)
{
	_xor_r32_r32(REG_EAX, REG_EAX);
	_mov_r32_m32abs(REG_EBX, &XER);
	_and_r32_imm(REG_EBX, ~0x20000000);		// clear carry
	_mov_r32_m32abs(REG_EDX, &REG(RA));
	_add_r32_m32abs(REG_EDX, &REG(RB));
	_mov_m32abs_r32(&REG(RT), REG_EDX);

	_setcc_r8(COND_C, REG_AL);		// carry to AL
	_shl_r32_imm(REG_EAX, 29);		// shift to carry bit
	_or_r32_r32(REG_EBX, REG_EAX);
	_mov_m32abs_r32(&XER, REG_EBX);

	if (OEBIT) {
		mame_printf_debug("recompile_addcx: OE bit set !\n");
		return RECOMPILE_UNIMPLEMENTED;
	}
	if (RCBIT) {
		append_set_cr0(drc);
	}

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_addex(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_addex);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_addi(drc_core *drc, UINT32 op)
{
	if (RA == 0)
	{
		_mov_m32abs_imm(&REG(RT), SIMM16);
	}
	else
	{
		_mov_r32_m32abs(REG_EAX, &REG(RA));
		_add_r32_imm(REG_EAX, SIMM16);
		_mov_m32abs_r32(&REG(RT), REG_EAX);
	}

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_addic(drc_core *drc, UINT32 op)
{
	_xor_r32_r32(REG_EAX, REG_EAX);
	_mov_r32_m32abs(REG_EBX, &XER);
	_and_r32_imm(REG_EBX, ~0x20000000);	// clear carry bit
	_mov_r32_m32abs(REG_EDX, &REG(RA));
	_add_r32_imm(REG_EDX, SIMM16);
	_mov_m32abs_r32(&REG(RT), REG_EDX);

	_setcc_r8(COND_C, REG_AL);			// carry to AL
	_shl_r32_imm(REG_EAX, 29);			// shift to carry bit
	_or_r32_r32(REG_EBX, REG_EAX);
	_mov_m32abs_r32(&XER, REG_EBX);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_addic_rc(drc_core *drc, UINT32 op)
{
	_xor_r32_r32(REG_EAX, REG_EAX);
	_mov_r32_m32abs(REG_EBX, &XER);
	_and_r32_imm(REG_EBX, ~0x20000000);	// clear carry bit
	_mov_r32_m32abs(REG_EDX, &REG(RA));
	_add_r32_imm(REG_EDX, SIMM16);
	_mov_m32abs_r32(&REG(RT), REG_EDX);

	_setcc_r8(COND_C, REG_AL);			// carry to AL
	_shl_r32_imm(REG_EAX, 29);			// shift to carry bit
	_or_r32_r32(REG_EBX, REG_EAX);
	_mov_m32abs_r32(&XER, REG_EBX);

	append_set_cr0(drc);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_addis(drc_core *drc, UINT32 op)
{
	if (RA == 0)
	{
		_mov_m32abs_imm(&REG(RT), UIMM16 << 16);
	}
	else
	{
		_mov_r32_m32abs(REG_EAX, &REG(RA));
		_add_r32_imm(REG_EAX, UIMM16 << 16);
		_mov_m32abs_r32(&REG(RT), REG_EAX);
	}

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_addmex(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_addmex);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_addzex(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_addzex);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_andx(drc_core *drc, UINT32 op)
{
	_mov_r32_m32abs(REG_EDX, &REG(RS));
	_and_r32_m32abs(REG_EDX, &REG(RB));
	_mov_m32abs_r32(&REG(RA), REG_EDX);

	if (RCBIT) {
		append_set_cr0(drc);
	}

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_andcx(drc_core *drc, UINT32 op)
{
	_mov_r32_m32abs(REG_EDX, &REG(RS));
	_mov_r32_m32abs(REG_EAX, &REG(RB));
	_not_r32(REG_EAX);
	_and_r32_r32(REG_EDX, REG_EAX);
	_mov_m32abs_r32(&REG(RA), REG_EDX);

	if (RCBIT) {
		append_set_cr0(drc);
	}

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_andi_rc(drc_core *drc, UINT32 op)
{
	_mov_r32_m32abs(REG_EDX, &REG(RS));
	_and_r32_imm(REG_EDX, UIMM16);
	_mov_m32abs_r32(&REG(RA), REG_EDX);

	append_set_cr0(drc);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_andis_rc(drc_core *drc, UINT32 op)
{
	_mov_r32_m32abs(REG_EDX, &REG(RS));
	_and_r32_imm(REG_EDX, UIMM16 << 16);
	_mov_m32abs_r32(&REG(RA), REG_EDX);

	append_set_cr0(drc);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_bx(drc_core *drc, UINT32 op)
{
	UINT32 newpc;
	INT32 li = op & 0x3fffffc;
	if( li & 0x2000000 )
		li |= 0xfc000000;

	if( AABIT ) {
		newpc = li;
	} else {
		newpc = temp_ppc_pc + li;
	}

	if( LKBIT ) {
		_mov_m32abs_imm(&LR, temp_ppc_pc + 4);
	}

	append_branch_or_dispatch(drc, newpc, 1);

	return RECOMPILE_SUCCESSFUL_CP(0,0) | RECOMPILE_END_OF_STRING;
}

static UINT32 recompile_bcx(drc_core *drc, UINT32 op)
{
	link_info link1 = {0}, link2 = {0};
	int do_link1 = 0, do_link2 = 0;
	UINT32 newpc;

	if( AABIT ) {
		newpc = SIMM16 & ~0x3;
	} else {
		newpc = temp_ppc_pc + (SIMM16 & ~0x3);
	}

	if( LKBIT ) {
		_mov_m32abs_imm(&LR, temp_ppc_pc + 4);
	}

	if (BO == 20)		/* condition is always true, so the basic block ends here */
	{
		append_branch_or_dispatch(drc, newpc, 1);

		return RECOMPILE_SUCCESSFUL_CP(0,0) | RECOMPILE_END_OF_STRING;
	}
	else
	{
		// if BO[2] == 0, update CTR and check CTR condition
		if ((BO & 0x4) == 0)
		{
			do_link1 = 1;
			//_dec_m32abs(&CTR);
			_sub_m32abs_imm(&CTR, 1);

			// if BO[1] == 0, branch if CTR != 0
			if ((BO & 0x2) == 0)
			{
				_jcc_near_link(COND_Z, &link1);
			}
			else
			{
				_jcc_near_link(COND_NZ, &link1);
			}
		}

		// if BO[0] == 0, check condition
		if ((BO & 0x10) == 0)
		{
			do_link2 = 1;
			_movzx_r32_m8abs(REG_EAX, &ppc.cr[(BI)/4]);
			_test_r32_imm(REG_EAX, 1 << (3 - ((BI) & 0x3)));	// test if condition register bit is set

			// if BO[3] == 0, branch if condition == FALSE (bit zero)
			if ((BO & 0x8) == 0)
			{
				_jcc_near_link(COND_NZ, &link2);			// bit not zero, skip branch
			}
			// if BO[3] == 1, branch if condition == TRUE (bit not zero)
			else
			{
				_jcc_near_link(COND_Z, &link2);				// bit zero, skip branch
			}
		}

		// take the branch
		append_branch_or_dispatch(drc, newpc, 1);

		// skip the branch
		if (do_link1) {
			_resolve_link(&link1);
		}
		if (do_link2) {
			_resolve_link(&link2);
		}

		return RECOMPILE_SUCCESSFUL_CP(1,4);
	}
}

static UINT32 recompile_bcctrx(drc_core *drc, UINT32 op)
{
	link_info link1 = {0} ,link2 = {0};
	int do_link1 = 0, do_link2 = 0;

	if (BO == 20)		/* condition is always true, so the basic block ends here */
	{
		_mov_r32_m32abs(REG_EDI, &CTR);					// mov  edi, CTR

		if( LKBIT ) {
			_mov_m32abs_imm(&LR, temp_ppc_pc + 4);
		}
		return RECOMPILE_SUCCESSFUL_CP(1,0) | RECOMPILE_END_OF_STRING | RECOMPILE_ADD_DISPATCH;
	}
	else
	{
		// if BO[2] == 0, update CTR and check CTR condition
		if ((BO & 0x4) == 0)
		{
			do_link1 = 1;
			//_dec_m32abs(&CTR);
			_sub_m32abs_imm(&CTR, 1);

			// if BO[1] == 0, branch if CTR != 0
			if ((BO & 0x2) == 0)
			{
				_jcc_near_link(COND_Z, &link1);
			}
			else
			{
				_jcc_near_link(COND_NZ, &link1);
			}
		}

		// if BO[0] == 0, check condition
		if ((BO & 0x10) == 0)
		{
			do_link2 = 1;
			_movzx_r32_m8abs(REG_EAX, &ppc.cr[(BI)/4]);
			_test_r32_imm(REG_EAX, 1 << (3 - ((BI) & 0x3)));	// test if condition register bit is set

			// if BO[3] == 0, branch if condition == FALSE (bit zero)
			if ((BO & 0x8) == 0)
			{
				_jcc_near_link(COND_NZ, &link2);			// bit not zero, skip branch
			}
			// if BO[3] == 1, branch if condition == TRUE (bit not zero)
			else
			{
				_jcc_near_link(COND_Z, &link2);				// bit zero, skip branch
			}
		}

		// take the branch
		_mov_r32_m32abs(REG_EDI, &CTR);		// mov  edi, CTR
		if( LKBIT ) {
			_mov_m32abs_imm(&LR, temp_ppc_pc + 4);
		}
		append_check_interrupts(drc, 0);
		drc_append_standard_epilogue(drc, 1, 0, 1);
		drc_append_dispatcher(drc);

		// skip the branch
		if (do_link1) {
			_resolve_link(&link1);
		}
		if (do_link2) {
			_resolve_link(&link2);
		}
		if( LKBIT ) {
			_mov_m32abs_imm(&LR, temp_ppc_pc + 4);
		}

		return RECOMPILE_SUCCESSFUL_CP(1,4);
	}
}

static UINT32 recompile_bclrx(drc_core *drc, UINT32 op)
{
	link_info link1 = {0}, link2 = {0};
	int do_link1 = 0, do_link2 = 0;

	if (BO == 20)		/* condition is always true, so the basic block ends here */
	{
		_mov_r32_m32abs(REG_EDI, &LR);					// mov  edi, LR

		if( LKBIT ) {
			_mov_m32abs_imm(&LR, temp_ppc_pc + 4);
		}
		return RECOMPILE_SUCCESSFUL_CP(1,0) | RECOMPILE_END_OF_STRING | RECOMPILE_ADD_DISPATCH;
	}
	else
	{
		// if BO[2] == 0, update CTR and check CTR condition
		if ((BO & 0x4) == 0)
		{
			do_link1 = 1;
			//_dec_m32abs(&CTR);
			_sub_m32abs_imm(&CTR, 1);

			// if BO[1] == 0, branch if CTR != 0
			if ((BO & 0x2) == 0)
			{
				_jcc_near_link(COND_Z, &link1);
			}
			else
			{
				_jcc_near_link(COND_NZ, &link1);
			}
		}

		// if BO[0] == 0, check condition
		if ((BO & 0x10) == 0)
		{
			do_link2 = 1;
			_movzx_r32_m8abs(REG_EAX, &ppc.cr[(BI)/4]);
			_test_r32_imm(REG_EAX, 1 << (3 - ((BI) & 0x3)));	// test if condition register bit is set

			// if BO[3] == 0, branch if condition == FALSE (bit zero)
			if ((BO & 0x8) == 0)
			{
				_jcc_near_link(COND_NZ, &link2);			// bit not zero, skip branch
			}
			// if BO[3] == 1, branch if condition == TRUE (bit not zero)
			else
			{
				_jcc_near_link(COND_Z, &link2);				// bit zero, skip branch
			}
		}

		// take the branch
		_mov_r32_m32abs(REG_EDI, &LR);		// mov  edi, LR
		if( LKBIT ) {
			_mov_m32abs_imm(&LR, temp_ppc_pc + 4);
		}
		append_check_interrupts(drc, 0);
		drc_append_standard_epilogue(drc, 1, 0, 1);
		drc_append_dispatcher(drc);

		// skip the branch
		if (do_link1) {
			_resolve_link(&link1);
		}
		if (do_link2) {
			_resolve_link(&link2);
		}
		if( LKBIT ) {
			_mov_m32abs_imm(&LR, temp_ppc_pc + 4);
		}

		return RECOMPILE_SUCCESSFUL_CP(1,4);
	}
}

static UINT32 recompile_cmp(drc_core *drc, UINT32 op)
{
	_xor_r32_r32(REG_EAX, REG_EAX);
	_xor_r32_r32(REG_EBX, REG_EBX);
	_mov_r32_m32abs(REG_ECX, &REG(RA));
	_cmp_r32_m32abs(REG_ECX, &REG(RB));

	_setcc_r8(COND_Z, REG_AL);
	_setcc_r8(COND_L, REG_AH);
	_setcc_r8(COND_G, REG_BL);
	_shl_r8_imm(REG_AL, 1);
	_shl_r8_imm(REG_AH, 3);
	_shl_r8_imm(REG_BL, 2);
	_or_r8_r8(REG_BL, REG_AH);
	_or_r8_r8(REG_BL, REG_AL);

	_bt_m32abs_imm(&XER, 31);		// set XER SO bit to carry
	_adc_r32_imm(REG_EBX, 0);		// effectively sets bit 0 to carry
	_mov_m8abs_r8(&ppc.cr[CRFD], REG_BL);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_cmpi(drc_core *drc, UINT32 op)
{
	_xor_r32_r32(REG_EAX, REG_EAX);
	_xor_r32_r32(REG_EBX, REG_EBX);
	_mov_r32_m32abs(REG_ECX, &REG(RA));
	_cmp_r32_imm(REG_ECX, SIMM16);

	_setcc_r8(COND_Z, REG_AL);
	_setcc_r8(COND_L, REG_AH);
	_setcc_r8(COND_G, REG_BL);
	_shl_r8_imm(REG_AL, 1);
	_shl_r8_imm(REG_AH, 3);
	_shl_r8_imm(REG_BL, 2);
	_or_r8_r8(REG_BL, REG_AH);
	_or_r8_r8(REG_BL, REG_AL);

	_bt_m32abs_imm(&XER, 31);		// set XER SO bit to carry
	_adc_r32_imm(REG_EBX, 0);		// effectively sets bit 0 to carry
	_mov_m8abs_r8(&ppc.cr[CRFD], REG_BL);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_cmpl(drc_core *drc, UINT32 op)
{
	_xor_r32_r32(REG_EAX, REG_EAX);
	_xor_r32_r32(REG_EBX, REG_EBX);
	_mov_r32_m32abs(REG_ECX, &REG(RA));
	_cmp_r32_m32abs(REG_ECX, &REG(RB));

	_setcc_r8(COND_Z, REG_AL);
	_setcc_r8(COND_B, REG_AH);
	_setcc_r8(COND_A, REG_BL);
	_shl_r8_imm(REG_AL, 1);
	_shl_r8_imm(REG_AH, 3);
	_shl_r8_imm(REG_BL, 2);
	_or_r8_r8(REG_BL, REG_AH);
	_or_r8_r8(REG_BL, REG_AL);

	_bt_m32abs_imm(&XER, 31);		// set XER SO bit to carry
	_adc_r32_imm(REG_EBX, 0);		// effectively sets bit 0 to carry
	_mov_m8abs_r8(&ppc.cr[CRFD], REG_BL);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_cmpli(drc_core *drc, UINT32 op)
{
	_xor_r32_r32(REG_EAX, REG_EAX);
	_xor_r32_r32(REG_EBX, REG_EBX);
	_mov_r32_m32abs(REG_ECX, &REG(RA));
	_cmp_r32_imm(REG_ECX, UIMM16);

	_setcc_r8(COND_Z, REG_AL);
	_setcc_r8(COND_B, REG_AH);
	_setcc_r8(COND_A, REG_BL);
	_shl_r8_imm(REG_AL, 1);
	_shl_r8_imm(REG_AH, 3);
	_shl_r8_imm(REG_BL, 2);
	_or_r8_r8(REG_BL, REG_AH);
	_or_r8_r8(REG_BL, REG_AL);

	_bt_m32abs_imm(&XER, 31);		// set XER SO bit to carry
	_adc_r32_imm(REG_EBX, 0);		// effectively sets bit 0 to carry
	_mov_m8abs_r8(&ppc.cr[CRFD], REG_BL);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_cntlzw(drc_core *drc, UINT32 op)
{
	_xor_r32_r32(REG_EBX, REG_EBX);
	_mov_r32_imm(REG_EDX, 31);
	_mov_r32_m32abs(REG_EAX, &REG(RT));
	_bsr_r32_r32(REG_EAX, REG_EAX);
	_setcc_r8(COND_Z, REG_BL);			// if all zeros, set BL to 1, so result becomes 32
	_sub_r32_r32(REG_EDX, REG_EAX);
	_add_r32_r32(REG_EDX, REG_EBX);
	_mov_m32abs_r32(&REG(RA), REG_EDX);

	if (RCBIT) {
		append_set_cr0(drc);
	}

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_crand(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_crand);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_crandc(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_crandc);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_creqv(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_creqv);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_crnand(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_crnand);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_crnor(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_crnor);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_cror(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_cror);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_crorc(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_crorc);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_crxor(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_crxor);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_dcbf(drc_core *drc, UINT32 op)
{
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_dcbi(drc_core *drc, UINT32 op)
{
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_dcbst(drc_core *drc, UINT32 op)
{
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_dcbt(drc_core *drc, UINT32 op)
{
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_dcbtst(drc_core *drc, UINT32 op)
{
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_dcbz(drc_core *drc, UINT32 op)
{
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_divwx(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_divwx);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_divwux(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_divwux);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_eieio(drc_core *drc, UINT32 op)
{
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_eqvx(drc_core *drc, UINT32 op)
{
	_mov_r32_m32abs(REG_EDX, &REG(RS));
	_xor_r32_m32abs(REG_EDX, &REG(RB));
	_not_r32(REG_EDX);
	_mov_m32abs_r32(&REG(RA), REG_EDX);

	if (RCBIT) {
		append_set_cr0(drc);
	}

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_extsbx(drc_core *drc, UINT32 op)
{
	_mov_r32_m32abs(REG_EDX, &REG(RS));
	_movsx_r32_r8(REG_EDX, REG_DL);
	_mov_m32abs_r32(&REG(RA), REG_EDX);

	if (RCBIT) {
		append_set_cr0(drc);
	}

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_extshx(drc_core *drc, UINT32 op)
{
	_mov_r32_m32abs(REG_EDX, &REG(RS));
	_movsx_r32_r16(REG_EDX, REG_DX);
	_mov_m32abs_r32(&REG(RA), REG_EDX);

	if (RCBIT) {
		append_set_cr0(drc);
	}

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_icbi(drc_core *drc, UINT32 op)
{
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_isync(drc_core *drc, UINT32 op)
{
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_lbz(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	if (RA == 0)
	{
		_push_imm(SIMM16);
	}
	else
	{
		_mov_r32_m32abs(REG_EAX, &REG(RA));
		_add_r32_imm(REG_EAX, SIMM16);
		_push_r32(REG_EAX);
	}
	_call((genf *)READ8);
	_add_r32_imm(REG_ESP, 4);
	_movzx_r32_r8(REG_EAX, REG_AL);
	_mov_m32abs_r32(&REG(RT), REG_EAX);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_lbzu(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_mov_r32_m32abs(REG_EDX, &REG(RA));
	_add_r32_imm(REG_EDX, SIMM16);
	_mov_m32abs_r32(&REG(RA), REG_EDX);
	_push_r32(REG_EDX);
	_call((genf *)READ8);
	_add_r32_imm(REG_ESP, 4);
	_movzx_r32_r8(REG_EAX, REG_AL);
	_mov_m32abs_r32(&REG(RT), REG_EAX);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_lbzux(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_mov_r32_m32abs(REG_EDX, &REG(RA));
	_add_r32_m32abs(REG_EDX, &REG(RB));
	_mov_m32abs_r32(&REG(RA), REG_EDX);
	_push_r32(REG_EDX);
	_call((genf *)READ8);
	_add_r32_imm(REG_ESP, 4);
	_movzx_r32_r8(REG_EAX, REG_AL);
	_mov_m32abs_r32(&REG(RT), REG_EAX);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_lbzx(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_mov_r32_m32abs(REG_EAX, &REG(RB));
	if (RA != 0)
	{
		_add_r32_m32abs(REG_EAX, &REG(RA));
	}
	_push_r32(REG_EAX);
	_call((genf *)READ8);
	_add_r32_imm(REG_ESP, 4);
	_movzx_r32_r8(REG_EAX, REG_AL);
	_mov_m32abs_r32(&REG(RT), REG_EAX);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_lha(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	if (RA == 0)
	{
		_push_imm(SIMM16);
	}
	else
	{
		_mov_r32_m32abs(REG_EAX, &REG(RA));
		_add_r32_imm(REG_EAX, SIMM16);
		_push_r32(REG_EAX);
	}
	_call((genf *)READ16);
	_add_r32_imm(REG_ESP, 4);
	_movsx_r32_r16(REG_EAX, REG_AX);
	_mov_m32abs_r32(&REG(RT), REG_EAX);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_lhau(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_mov_r32_m32abs(REG_EDX, &REG(RA));
	_add_r32_imm(REG_EDX, SIMM16);
	_mov_m32abs_r32(&REG(RA), REG_EDX);
	_push_r32(REG_EDX);
	_call((genf *)READ16);
	_add_r32_imm(REG_ESP, 4);
	_movsx_r32_r16(REG_EAX, REG_AX);
	_mov_m32abs_r32(&REG(RT), REG_EAX);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_lhaux(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_mov_r32_m32abs(REG_EDX, &REG(RA));
	_add_r32_m32abs(REG_EDX, &REG(RB));
	_mov_m32abs_r32(&REG(RA), REG_EDX);
	_push_r32(REG_EDX);
	_call((genf *)READ16);
	_add_r32_imm(REG_ESP, 4);
	_movsx_r32_r16(REG_EAX, REG_AX);
	_mov_m32abs_r32(&REG(RT), REG_EAX);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_lhax(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_mov_r32_m32abs(REG_EAX, &REG(RB));
	if (RA != 0)
	{
		_add_r32_m32abs(REG_EAX, &REG(RA));
	}
	_push_r32(REG_EAX);
	_call((genf *)READ16);
	_add_r32_imm(REG_ESP, 4);
	_movsx_r32_r16(REG_EAX, REG_AX);
	_mov_m32abs_r32(&REG(RT), REG_EAX);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_lhbrx(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_mov_r32_m32abs(REG_EAX, &REG(RB));
	if (RA != 0)
	{
		_add_r32_m32abs(REG_EAX, &REG(RA));
	}
	_push_r32(REG_EAX);
	_call((genf *)READ16);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_imm(REG_ECX, 8);
	_rol_r16_cl(REG_AX);
	_mov_m32abs_r32(&REG(RT), REG_EAX);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_lhz(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	if (RA == 0)
	{
		_push_imm(SIMM16);
	}
	else
	{
		_mov_r32_m32abs(REG_EAX, &REG(RA));
		_add_r32_imm(REG_EAX, SIMM16);
		_push_r32(REG_EAX);
	}
	_call((genf *)READ16);
	_add_r32_imm(REG_ESP, 4);
	_mov_m32abs_r32(&REG(RT), REG_EAX);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_lhzu(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_mov_r32_m32abs(REG_EDX, &REG(RA));
	_add_r32_imm(REG_EDX, SIMM16);
	_mov_m32abs_r32(&REG(RA), REG_EDX);
	_push_r32(REG_EDX);
	_call((genf *)READ16);
	_add_r32_imm(REG_ESP, 4);
	_mov_m32abs_r32(&REG(RT), REG_EAX);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_lhzux(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_mov_r32_m32abs(REG_EDX, &REG(RA));
	_add_r32_m32abs(REG_EDX, &REG(RB));
	_mov_m32abs_r32(&REG(RA), REG_EDX);
	_push_r32(REG_EDX);
	_call((genf *)READ16);
	_add_r32_imm(REG_ESP, 4);
	_mov_m32abs_r32(&REG(RT), REG_EAX);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_lhzx(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_mov_r32_m32abs(REG_EAX, &REG(RB));
	if (RA != 0)
	{
		_add_r32_m32abs(REG_EAX, &REG(RA));
	}
	_push_r32(REG_EAX);
	_call((genf *)READ16);
	_add_r32_imm(REG_ESP, 4);
	_mov_m32abs_r32(&REG(RT), REG_EAX);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_lmw(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_lmw);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_lswi(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_lswi);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_lswx(drc_core *drc, UINT32 op)
{
	mame_printf_debug("PPCDRC: recompile lswx\n");
	return RECOMPILE_UNIMPLEMENTED;
}

static UINT32 recompile_lwarx(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_lwarx);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_lwbrx(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_mov_r32_m32abs(REG_EAX, &REG(RB));
	if (RA != 0)
	{
		_add_r32_m32abs(REG_EAX, &REG(RA));
	}
	_push_r32(REG_EAX);
	_call((genf *)READ32);
	_add_r32_imm(REG_ESP, 4);
	_bswap_r32(REG_EAX);
	_mov_m32abs_r32(&REG(RT), REG_EAX);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_lwz(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	if (RA == 0)
	{
		_push_imm(SIMM16);
	}
	else
	{
		_mov_r32_m32abs(REG_EAX, &REG(RA));
		_add_r32_imm(REG_EAX, SIMM16);
		_push_r32(REG_EAX);
	}
	_call((genf *)READ32);
	_add_r32_imm(REG_ESP, 4);
	_mov_m32abs_r32(&REG(RT), REG_EAX);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_lwzu(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_mov_r32_m32abs(REG_EDX, &REG(RA));
	_add_r32_imm(REG_EDX, SIMM16);
	_mov_m32abs_r32(&REG(RA), REG_EDX);
	_push_r32(REG_EDX);
	_call((genf *)READ32);
	_add_r32_imm(REG_ESP, 4);
	_mov_m32abs_r32(&REG(RT), REG_EAX);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_lwzux(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_mov_r32_m32abs(REG_EDX, &REG(RA));
	_add_r32_m32abs(REG_EDX, &REG(RB));
	_mov_m32abs_r32(&REG(RA), REG_EDX);
	_push_r32(REG_EDX);
	_call((genf *)READ32);
	_add_r32_imm(REG_ESP, 4);
	_mov_m32abs_r32(&REG(RT), REG_EAX);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_lwzx(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_mov_r32_m32abs(REG_EAX, &REG(RB));
	if (RA != 0)
	{
		_add_r32_m32abs(REG_EAX, &REG(RA));
	}
	_push_r32(REG_EAX);
	_call((genf *)READ32);
	_add_r32_imm(REG_ESP, 4);
	_mov_m32abs_r32(&REG(RT), REG_EAX);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_mcrf(drc_core *drc, UINT32 op)
{
	_mov_r8_m8abs(REG_AL, &ppc.cr[RA >> 2]);
	_mov_m8abs_r8(&ppc.cr[RT >> 2], REG_AL);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_mcrxr(drc_core *drc, UINT32 op)
{
	mame_printf_debug("PPCDRC: recompile mcrxr\n");
	return RECOMPILE_UNIMPLEMENTED;
}

static UINT32 recompile_mfcr(drc_core *drc, UINT32 op)
{
	int i;
	_xor_r32_r32(REG_EAX, REG_EAX);

	// generate code for each condition register
	for (i=0; i < 8; i++)
	{
		_xor_r32_r32(REG_EDX, REG_EDX);
		_mov_r8_m8abs(REG_DL, &ppc.cr[i]);
		_shl_r32_imm(REG_EDX, ((7-i) * 4));
		_or_r32_r32(REG_EAX, REG_EDX);
	}
	_mov_m32abs_r32(&REG(RT), REG_EAX);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_mfmsr(drc_core *drc, UINT32 op)
{
	_mov_r32_m32abs(REG_EAX, &ppc.msr);
	_mov_m32abs_r32(&REG(RT), REG_EAX);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_mfspr(drc_core *drc, UINT32 op)
{
	if (SPR == SPR_LR)			// optimized case, LR
	{
		_mov_r32_m32abs(REG_EAX, &LR);
	}
	else if(SPR == SPR_CTR)		// optimized case, CTR
	{
		_mov_r32_m32abs(REG_EAX, &CTR);
	}
	else
	{
		_mov_m32abs_r32(&ppc_icount, REG_EBP);
		_push_imm(SPR);
		_call((genf *)ppc_get_spr);
		_add_r32_imm(REG_ESP, 4);
		_mov_r32_m32abs(REG_EBP, &ppc_icount);
	}
	_mov_m32abs_r32(&REG(RT), REG_EAX);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_mtcrf(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_mtcrf);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_mtmsr(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_mov_r32_m32abs(REG_EAX, &REG(RS));
	_push_r32(REG_EAX);
	_call((genf *)ppc_set_msr);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_mtspr(drc_core *drc, UINT32 op)
{
	_mov_r32_m32abs(REG_EAX, &REG(RS));
	if (SPR == SPR_LR)			// optimized case, LR
	{
		_mov_m32abs_r32(&LR, REG_EAX);
	}
	else if(SPR == SPR_CTR)		// optimized case, CTR
	{
		_mov_m32abs_r32(&CTR, REG_EAX);
	}
	else
	{
		_mov_m32abs_r32(&ppc_icount, REG_EBP);
		_push_r32(REG_EAX);
		_push_imm(SPR);
		_call((genf *)ppc_set_spr);
		_add_r32_imm(REG_ESP, 8);
		_mov_r32_m32abs(REG_EBP, &ppc_icount);
	}

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_mulhwx(drc_core *drc, UINT32 op)
{
	_mov_r32_m32abs(REG_EAX, &REG(RA));
	_mov_r32_m32abs(REG_EBX, &REG(RB));
	_imul_r32(REG_EBX);
	_mov_m32abs_r32(&REG(RT), REG_EDX);

	if (RCBIT) {
		append_set_cr0(drc);
	}

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_mulhwux(drc_core *drc, UINT32 op)
{
	_mov_r32_m32abs(REG_EAX, &REG(RA));
	_mov_r32_m32abs(REG_EBX, &REG(RB));
	_mul_r32(REG_EBX);
	_mov_m32abs_r32(&REG(RT), REG_EDX);

	if (RCBIT) {
		append_set_cr0(drc);
	}

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_mulli(drc_core *drc, UINT32 op)
{
	_mov_r32_m32abs(REG_EAX, &REG(RA));
	_mov_r32_imm(REG_EBX, SIMM16);
	_imul_r32(REG_EBX);
	_mov_m32abs_r32(&REG(RT), REG_EAX);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_mullwx(drc_core *drc, UINT32 op)
{
	_mov_r32_m32abs(REG_EAX, &REG(RA));
	_mov_r32_m32abs(REG_EBX, &REG(RB));
	_mul_r32(REG_EBX);
	_mov_r32_r32(REG_EDX, REG_EAX);
	_mov_m32abs_r32(&REG(RT), REG_EDX);

	if (OEBIT) {
		mame_printf_debug("recompile_mullwx: OEBIT set!\n");
		return RECOMPILE_UNIMPLEMENTED;
	}

	if (RCBIT) {
		append_set_cr0(drc);
	}

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_nandx(drc_core *drc, UINT32 op)
{
	_mov_r32_m32abs(REG_EDX, &REG(RS));
	_and_r32_m32abs(REG_EDX, &REG(RB));
	_not_r32(REG_EDX);
	_mov_m32abs_r32(&REG(RA), REG_EDX);

	if (RCBIT) {
		append_set_cr0(drc);
	}

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_negx(drc_core *drc, UINT32 op)
{
	_mov_r32_m32abs(REG_EDX, &REG(RA));
	_neg_r32(REG_EDX);
	_mov_m32abs_r32(&REG(RT), REG_EDX);

	if (RCBIT) {
		append_set_cr0(drc);
	}

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_norx(drc_core *drc, UINT32 op)
{
	_mov_r32_m32abs(REG_EDX, &REG(RS));
	_or_r32_m32abs(REG_EDX, &REG(RB));
	_not_r32(REG_EDX);
	_mov_m32abs_r32(&REG(RA), REG_EDX);

	if (RCBIT) {
		append_set_cr0(drc);
	}

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_orx(drc_core *drc, UINT32 op)
{
	_mov_r32_m32abs(REG_EDX, &REG(RS));
	_or_r32_m32abs(REG_EDX, &REG(RB));
	_mov_m32abs_r32(&REG(RA), REG_EDX);

	if (RCBIT) {
		append_set_cr0(drc);
	}

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_orcx(drc_core *drc, UINT32 op)
{
	_mov_r32_m32abs(REG_EDX, &REG(RB));
	_not_r32(REG_EDX);
	_or_r32_m32abs(REG_EDX, &REG(RS));
	_mov_m32abs_r32(&REG(RA), REG_EDX);

	if (RCBIT) {
		append_set_cr0(drc);
	}

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_ori(drc_core *drc, UINT32 op)
{

	_mov_r32_m32abs(REG_EAX, &REG(RS));
	_or_r32_imm(REG_EAX, UIMM16);
	_mov_m32abs_r32(&REG(RA), REG_EAX);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_oris(drc_core *drc, UINT32 op)
{
	_mov_r32_m32abs(REG_EAX, &REG(RS));
	_or_r32_imm(REG_EAX, UIMM16 << 16);
	_mov_m32abs_r32(&REG(RA), REG_EAX);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_rfi(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_mov_r32_m32abs(REG_EDI, &ppc.srr0);	/* get saved PC from SRR0 */
	_mov_r32_m32abs(REG_EAX, &ppc.srr1);	/* get saved MSR from SRR1 */

	_push_r32(REG_EAX);
	_call((genf *)ppc_set_msr);		/* set MSR */
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,0) | RECOMPILE_END_OF_STRING | RECOMPILE_ADD_DISPATCH;
}

static UINT32 recompile_rlwimix(drc_core *drc, UINT32 op)
{
	UINT32 mask = GET_ROTATE_MASK(MB, ME);

	_mov_r32_m32abs(REG_EDX, &REG(RS));
	_mov_r32_m32abs(REG_EAX, &REG(RA));
	_rol_r32_imm(REG_EDX, (SH));
	_and_r32_imm(REG_EDX, mask);
	_and_r32_imm(REG_EAX, ~mask);
	_or_r32_r32(REG_EDX, REG_EAX);
	_mov_m32abs_r32(&REG(RA), REG_EDX);

	if (RCBIT) {
		append_set_cr0(drc);
	}

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_rlwinmx(drc_core *drc, UINT32 op)
{
	UINT32 mask = GET_ROTATE_MASK(MB, ME);

	_mov_r32_m32abs(REG_EDX, &REG(RS));
	_rol_r32_imm(REG_EDX, (SH));
	_and_r32_imm(REG_EDX, mask);
	_mov_m32abs_r32(&REG(RA), REG_EDX);

	if (RCBIT) {
		append_set_cr0(drc);
	}

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_rlwnmx(drc_core *drc, UINT32 op)
{
	UINT32 mask = GET_ROTATE_MASK(MB, ME);

	_mov_r32_m32abs(REG_ECX, &REG(RB));		// x86 rotate instruction use only 5 bits, so no need to mask this
	_mov_r32_m32abs(REG_EDX, &REG(RS));
	_rol_r32_cl(REG_EDX);
	_and_r32_imm(REG_EDX, mask);
	_mov_m32abs_r32(&REG(RA), REG_EDX);

	if (RCBIT) {
		append_set_cr0(drc);
	}

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_sc(drc_core *drc, UINT32 op)
{
	_mov_m32abs_imm(&SRR0, temp_ppc_pc + 4);
	_jmp(ppc.generate_syscall_exception);
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_slwx(drc_core *drc, UINT32 op)
{
#if USE_SSE2
	_mov_r32_m32abs(REG_ECX, &REG(RB));
	_and_r32_imm(REG_ECX, 0x3f);
	_movd_r128_m32abs(REG_XMM0, &REG(RS));
	_movd_r128_r32(REG_XMM1, REG_ECX);
	_psllq_r128_r128(REG_XMM0, REG_XMM1);
	_movd_r32_r128(REG_EDX, REG_XMM0);
	_mov_m32abs_r32(&REG(RA), REG_EDX);

	if (RCBIT) {
		append_set_cr0(drc);
	}
#else
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_slwx);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);
#endif

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_srawx(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_srawx);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_srawix(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_srawix);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_srwx(drc_core *drc, UINT32 op)
{
#if USE_SSE2
	_mov_r32_m32abs(REG_ECX, &REG(RB));
	_and_r32_imm(REG_ECX, 0x3f);
	_movd_r128_m32abs(REG_XMM0, &REG(RS));
	_movd_r128_r32(REG_XMM1, REG_ECX);
	_psrlq_r128_r128(REG_XMM0, REG_XMM1);
	_movd_r32_r128(REG_EDX, REG_XMM0);
	_mov_m32abs_r32(&REG(RA), REG_EDX);

	if (RCBIT) {
		append_set_cr0(drc);
	}
#else
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_srwx);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);
#endif

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_stb(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_mov_r32_m32abs(REG_EAX, &REG(RS));
	_movzx_r32_r8(REG_EAX, REG_AL);
	_push_r32(REG_EAX);

	if (RA == 0)
	{
		_push_imm(SIMM16);
	}
	else
	{
		_mov_r32_m32abs(REG_EAX, &REG(RA));
		_add_r32_imm(REG_EAX, SIMM16);
		_push_r32(REG_EAX);
	}
	_call((genf *)WRITE8);
	_add_r32_imm(REG_ESP, 8);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_stbu(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_mov_r32_m32abs(REG_EAX, &REG(RS));
	_movzx_r32_r8(REG_EAX, REG_AL);
	_push_r32(REG_EAX);

	_mov_r32_m32abs(REG_EDX, &REG(RA));
	_add_r32_imm(REG_EDX, SIMM16);
	_mov_m32abs_r32(&REG(RA), REG_EDX);
	_push_r32(REG_EDX);
	_call((genf *)WRITE8);
	_add_r32_imm(REG_ESP, 8);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_stbux(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_mov_r32_m32abs(REG_EAX, &REG(RS));
	_movzx_r32_r8(REG_EAX, REG_AL);
	_push_r32(REG_EAX);

	_mov_r32_m32abs(REG_EDX, &REG(RA));
	_add_r32_m32abs(REG_EDX, &REG(RB));
	_mov_m32abs_r32(&REG(RA), REG_EDX);
	_push_r32(REG_EDX);
	_call((genf *)WRITE8);
	_add_r32_imm(REG_ESP, 8);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_stbx(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_mov_r32_m32abs(REG_EAX, &REG(RS));
	_movzx_r32_r8(REG_EAX, REG_AL);
	_push_r32(REG_EAX);

	_mov_r32_m32abs(REG_EDX, &REG(RB));
	if (RA != 0)
	{
		_add_r32_m32abs(REG_EDX, &REG(RA));
	}
	_push_r32(REG_EDX);
	_call((genf *)WRITE8);
	_add_r32_imm(REG_ESP, 8);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_sth(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_mov_r32_m32abs(REG_EAX, &REG(RS));
	_movzx_r32_r16(REG_EAX, REG_AX);
	_push_r32(REG_EAX);

	if (RA == 0)
	{
		_push_imm(SIMM16);
	}
	else
	{
		_mov_r32_m32abs(REG_EDX, &REG(RA));
		_add_r32_imm(REG_EDX, SIMM16);
		_push_r32(REG_EDX);
	}
	_call((genf *)WRITE16);
	_add_r32_imm(REG_ESP, 8);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_sthbrx(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_mov_r32_m32abs(REG_EAX, &REG(RS));
	_movzx_r32_r16(REG_EAX, REG_AX);
	_mov_r32_imm(REG_ECX, 8);
	_rol_r16_cl(REG_AX);
	_push_r32(REG_EAX);

	_mov_r32_m32abs(REG_EDX, &REG(RB));
	if (RA != 0)
	{
		_add_r32_m32abs(REG_EDX, &REG(RA));
	}
	_push_r32(REG_EDX);
	_call((genf *)WRITE16);
	_add_r32_imm(REG_ESP, 8);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_sthu(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_mov_r32_m32abs(REG_EAX, &REG(RS));
	_movzx_r32_r16(REG_EAX, REG_AX);
	_push_r32(REG_EAX);

	_mov_r32_m32abs(REG_EDX, &REG(RA));
	_add_r32_imm(REG_EDX, SIMM16);
	_mov_m32abs_r32(&REG(RA), REG_EDX);
	_push_r32(REG_EDX);
	_call((genf *)WRITE16);
	_add_r32_imm(REG_ESP, 8);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_sthux(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_mov_r32_m32abs(REG_EAX, &REG(RS));
	_movzx_r32_r16(REG_EAX, REG_AX);
	_push_r32(REG_EAX);

	_mov_r32_m32abs(REG_EDX, &REG(RA));
	_add_r32_m32abs(REG_EDX, &REG(RB));
	_mov_m32abs_r32(&REG(RA), REG_EDX);
	_push_r32(REG_EDX);
	_call((genf *)WRITE16);
	_add_r32_imm(REG_ESP, 8);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_sthx(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_mov_r32_m32abs(REG_EAX, &REG(RS));
	_movzx_r32_r16(REG_EAX, REG_AX);
	_push_r32(REG_EAX);

	_mov_r32_m32abs(REG_EDX, &REG(RB));
	if (RA != 0)
	{
		_add_r32_m32abs(REG_EDX, &REG(RA));
	}
	_push_r32(REG_EDX);
	_call((genf *)WRITE16);
	_add_r32_imm(REG_ESP, 8);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_stmw(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_stmw);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_stswi(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_stswi);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_stswx(drc_core *drc, UINT32 op)
{
	mame_printf_debug("PPCDRC: recompile stswx\n");
	return RECOMPILE_UNIMPLEMENTED;
}

static UINT32 recompile_stw(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_m32abs(&REG(RS));
	if (RA == 0)
	{
		_push_imm(SIMM16);
	}
	else
	{
		_mov_r32_m32abs(REG_EAX, &REG(RA));
		_add_r32_imm(REG_EAX, SIMM16);
		_push_r32(REG_EAX);
	}
	_call((genf *)WRITE32);
	_add_r32_imm(REG_ESP, 8);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_stwbrx(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_mov_r32_m32abs(REG_EAX, &REG(RS));
	_bswap_r32(REG_EAX);
	_push_r32(REG_EAX);

	_mov_r32_m32abs(REG_EDX, &REG(RB));
	if (RA != 0)
	{
		_add_r32_m32abs(REG_EDX, &REG(RA));
	}
	_push_r32(REG_EDX);
	_call((genf *)WRITE32);
	_add_r32_imm(REG_ESP, 8);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_stwcx_rc(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_stwcx_rc);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_stwu(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_m32abs(&REG(RS));

	_mov_r32_m32abs(REG_EAX, &REG(RA));
	_add_r32_imm(REG_EAX, SIMM16);
	_mov_m32abs_r32(&REG(RA), REG_EAX);
	_push_r32(REG_EAX);
	_call((genf *)WRITE32);
	_add_r32_imm(REG_ESP, 8);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_stwux(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_m32abs(&REG(RS));

	_mov_r32_m32abs(REG_EAX, &REG(RA));
	_add_r32_m32abs(REG_EAX, &REG(RB));
	_mov_m32abs_r32(&REG(RA), REG_EAX);
	_push_r32(REG_EAX);
	_call((genf *)WRITE32);
	_add_r32_imm(REG_ESP, 8);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_stwx(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_m32abs(&REG(RS));

	_mov_r32_m32abs(REG_EAX, &REG(RB));
	if (RA != 0)
	{
		_add_r32_m32abs(REG_EAX, &REG(RA));
	}
	_push_r32(REG_EAX);
	_call((genf *)WRITE32);
	_add_r32_imm(REG_ESP, 8);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_subfx(drc_core *drc, UINT32 op)
{
	_mov_r32_m32abs(REG_EDX, &REG(RB));
	_sub_r32_m32abs(REG_EDX, &REG(RA));
	_mov_m32abs_r32(&REG(RT), REG_EDX);

	if (OEBIT) {
		mame_printf_debug("recompile_subfx: OEBIT set !\n");
		return RECOMPILE_UNIMPLEMENTED;
	}
	if (RCBIT) {
		append_set_cr0(drc);
	}

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_subfcx(drc_core *drc, UINT32 op)
{
	if (OEBIT)
	{
		_mov_m32abs_r32(&ppc_icount, REG_EBP);
		_push_imm(op);
		_call((genf *)ppc_subfcx);
		_add_r32_imm(REG_ESP, 4);
		_mov_r32_m32abs(REG_EBP, &ppc_icount);
	}
	else
	{
		_xor_r32_r32(REG_EAX, REG_EAX);
		_mov_r32_m32abs(REG_EBX, &XER);
		_and_r32_imm(REG_EBX, ~0x20000000);		// clear carry
		_mov_r32_m32abs(REG_EDX, &REG(RB));
		_sub_r32_m32abs(REG_EDX, &REG(RA));
		_mov_m32abs_r32(&REG(RT), REG_EDX);
		_setcc_r8(COND_NC, REG_AL);				// subtract carry is inverse
		_shl_r32_imm(REG_EAX, 29);				// move carry to correct location in XER
		_or_r32_r32(REG_EBX, REG_EAX);			// insert carry to XER
		_mov_m32abs_r32(&XER, REG_EBX);

		//if (OEBIT) {
		//  mame_printf_debug("recompile_subfcx: OEBIT set !\n");
		//  return RECOMPILE_UNIMPLEMENTED;
		//}
		if (RCBIT) {
			append_set_cr0(drc);
		}
	}
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_subfex(drc_core *drc, UINT32 op)
{
	_xor_r32_r32(REG_EAX, REG_EAX);
	_mov_r32_m32abs(REG_EBX, &XER);
	_mov_r32_m32abs(REG_EDX, &REG(RB));
	_mov_r32_m32abs(REG_ECX, &REG(RA));
	_bt_r32_imm(REG_EBX, 29);				// XER carry to carry flag
	_cmc();									// invert carry
	_adc_r32_imm(REG_ECX, 0);
	_sub_r32_r32(REG_EDX, REG_ECX);
	_mov_m32abs_r32(&REG(RT), REG_EDX);
	_setcc_r8(COND_NC, REG_AL);				// subtract carry is inverse
	_and_r32_imm(REG_EBX, ~0x20000000);		// clear carry
	_shl_r32_imm(REG_EAX, 29);				// move carry to correct location in XER
	_or_r32_r32(REG_EBX, REG_EAX);			// insert carry to XER
	_mov_m32abs_r32(&XER, REG_EBX);

	if (OEBIT) {
		mame_printf_debug("recompile_subfex: OEBIT set !\n");
		return RECOMPILE_UNIMPLEMENTED;
	}
	if (RCBIT) {
		append_set_cr0(drc);
	}

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_subfic(drc_core *drc, UINT32 op)
{
	_xor_r32_r32(REG_EAX, REG_EAX);
	_mov_r32_m32abs(REG_EBX, &XER);
	_and_r32_imm(REG_EBX, ~0x20000000);		// clear carry
	_mov_r32_imm(REG_EDX, SIMM16);
	_sub_r32_m32abs(REG_EDX, &REG(RA));
	_mov_m32abs_r32(&REG(RT), REG_EDX);
	_setcc_r8(COND_NC, REG_AL);				// subtract carry is inverse
	_shl_r32_imm(REG_EAX, 29);				// move carry to correct location in XER
	_or_r32_r32(REG_EBX, REG_EAX);			// insert carry to XER
	_mov_m32abs_r32(&XER, REG_EBX);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_subfmex(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_subfmex);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_subfzex(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_subfzex);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_sync(drc_core *drc, UINT32 op)
{
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_tw(drc_core *drc, UINT32 op)
{
	link_info link1 = {0}, link2 = {0}, link3 = {0}, link4 = {0}, link5 = {0}, link6 = {0};
	int do_link1 = 0;
	int do_link2 = 0;
	int do_link3 = 0;
	int do_link4 = 0;
	int do_link5 = 0;

	_mov_r32_m32abs(REG_EAX, &REG(RA));
	_mov_r32_m32abs(REG_EDX, &REG(RB));
	_cmp_r32_r32(REG_EAX, REG_EDX);

	if (RT & 0x10) {
		_jcc_near_link(COND_L, &link1);		// less than = signed <
		do_link1 = 1;
	}
	if (RT & 0x08) {
		_jcc_near_link(COND_G, &link2);		// greater = signed >
		do_link2 = 1;
	}
	if (RT & 0x04) {
		_jcc_near_link(COND_E, &link3);		// equal
		do_link3 = 1;
	}
	if (RT & 0x02) {
		_jcc_near_link(COND_B, &link4);		// below = unsigned <
		do_link4 = 1;
	}
	if (RT & 0x01) {
		_jcc_near_link(COND_A, &link5);		// above = unsigned >
		do_link5 = 1;
	}
	_jmp_near_link(&link6);

	if (do_link1) {
		_resolve_link(&link1);
	}
	if (do_link2) {
		_resolve_link(&link2);
	}
	if (do_link3) {
		_resolve_link(&link3);
	}
	if (do_link4) {
		_resolve_link(&link4);
	}
	if (do_link5) {
		_resolve_link(&link5);
	}
	// generate exception
	_mov_m32abs_imm(&SRR0, temp_ppc_pc + 4);
	_jmp(ppc.generate_trap_exception);

	// no exception
	_resolve_link(&link6);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_twi(drc_core *drc, UINT32 op)
{
	link_info link1 = {0}, link2 = {0}, link3 = {0}, link4 = {0}, link5 = {0}, link6 = {0};
	int do_link1 = 0;
	int do_link2 = 0;
	int do_link3 = 0;
	int do_link4 = 0;
	int do_link5 = 0;

	_mov_r32_m32abs(REG_EAX, &REG(RA));
	_mov_r32_imm(REG_EDX, SIMM16);
	_cmp_r32_r32(REG_EAX, REG_EDX);

	if (RT & 0x10) {
		_jcc_near_link(COND_L, &link1);		// less than = signed <
		do_link1 = 1;
	}
	if (RT & 0x08) {
		_jcc_near_link(COND_G, &link2);		// greater = signed >
		do_link2 = 1;
	}
	if (RT & 0x04) {
		_jcc_near_link(COND_E, &link3);		// equal
		do_link3 = 1;
	}
	if (RT & 0x02) {
		_jcc_near_link(COND_B, &link4);		// below = unsigned <
		do_link4 = 1;
	}
	if (RT & 0x01) {
		_jcc_near_link(COND_A, &link5);		// above = unsigned >
		do_link5 = 1;
	}
	_jmp_near_link(&link6);

	if (do_link1) {
		_resolve_link(&link1);
	}
	if (do_link2) {
		_resolve_link(&link2);
	}
	if (do_link3) {
		_resolve_link(&link3);
	}
	if (do_link4) {
		_resolve_link(&link4);
	}
	if (do_link5) {
		_resolve_link(&link5);
	}
	// generate exception
	_mov_m32abs_imm(&SRR0, temp_ppc_pc + 4);
	_jmp(ppc.generate_trap_exception);

	// no exception
	_resolve_link(&link6);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_xorx(drc_core *drc, UINT32 op)
{
	_mov_r32_m32abs(REG_EDX, &REG(RS));
	_xor_r32_m32abs(REG_EDX, &REG(RB));
	_mov_m32abs_r32(&REG(RA), REG_EDX);

	if (RCBIT) {
		append_set_cr0(drc);
	}

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_xori(drc_core *drc, UINT32 op)
{
	_mov_r32_m32abs(REG_EDX, &REG(RS));
	_xor_r32_imm(REG_EDX, UIMM16);
	_mov_m32abs_r32(&REG(RA), REG_EDX);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_xoris(drc_core *drc, UINT32 op)
{
	_mov_r32_m32abs(REG_EDX, &REG(RS));
	_xor_r32_imm(REG_EDX, UIMM16 << 16);
	_mov_m32abs_r32(&REG(RA), REG_EDX);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_dccci(drc_core *drc, UINT32 op)
{
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_dcread(drc_core *drc, UINT32 op)
{
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_icbt(drc_core *drc, UINT32 op)
{
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_iccci(drc_core *drc, UINT32 op)
{
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_icread(drc_core *drc, UINT32 op)
{
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_rfci(drc_core *drc, UINT32 op)
{
	mame_printf_debug("PPCDRC: recompile rfci\n");
	return RECOMPILE_UNIMPLEMENTED;
}

#if HAS_PPC403
static UINT32 recompile_mfdcr(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_mfdcr);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_mtdcr(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_mtdcr);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_wrtee(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_wrtee);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_wrteei(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_wrteei);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}
#endif



static UINT32 recompile_invalid(drc_core *drc, UINT32 op)
{
	mame_printf_debug("PPCDRC: Invalid opcode %08X PC : %X\n", op, ppc.pc);
	return RECOMPILE_UNIMPLEMENTED;
}



/* PowerPC 60x Recompilers */

static UINT32 recompile_lfs(drc_core *drc,UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
#if USE_SSE2
	if (RA == 0)
	{
		_push_imm(SIMM16);
	}
	else
	{
		_mov_r32_m32abs(REG_EAX, &REG(RA));
		_add_r32_imm(REG_EAX, SIMM16);
		_push_r32(REG_EAX);
	}
	_call((genf*)READ32);
	_add_r32_imm(REG_ESP, 4);
	_movd_r128_r32(REG_XMM0, REG_EAX);
	_cvtss2sd_r128_r128(REG_XMM1, REG_XMM0);		// convert float to double
	_movq_m64abs_r128(&FPR(RT), REG_XMM1);
#else
	_push_imm(op);
	_call((genf *)ppc_lfs);
	_add_r32_imm(REG_ESP, 4);
#endif
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_lfsu(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
#if USE_SSE2
	_mov_r32_m32abs(REG_EDX, &REG(RA));
	_add_r32_imm(REG_EDX, SIMM16);
	_mov_m32abs_r32(&REG(RA), REG_EDX);
	_push_r32(REG_EDX);
	_call((genf *)READ32);
	_add_r32_imm(REG_ESP, 4);
	_movd_r128_r32(REG_XMM0, REG_EAX);
	_cvtss2sd_r128_r128(REG_XMM1, REG_XMM0);		// convert float to double
	_movq_m64abs_r128(&FPR(RT), REG_XMM1);
#else
	_push_imm(op);
	_call((genf *)ppc_lfsu);
	_add_r32_imm(REG_ESP, 4);
#endif
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_lfd(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	if (RA == 0)
	{
		_push_imm(SIMM16);
	}
	else
	{
		_mov_r32_m32abs(REG_EAX, &REG(RA));
		_add_r32_imm(REG_EAX, SIMM16);
		_push_r32(REG_EAX);
	}
	_call((genf*)READ64);
	_add_r32_imm(REG_ESP, 4);
	_mov_m64abs_r64(&FPR(RT), REG_EDX, REG_EAX);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_lfdu(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_mov_r32_m32abs(REG_EDX, &REG(RA));
	_add_r32_imm(REG_EDX, SIMM16);
	_mov_m32abs_r32(&REG(RA), REG_EDX);
	_push_r32(REG_EDX);
	_call((genf*)READ64);
	_add_r32_imm(REG_ESP, 4);
	_mov_m64abs_r64(&FPR(RT), REG_EDX, REG_EAX);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_stfs(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
#if USE_SSE2
	_movq_r128_m64abs(REG_XMM0, &FPR(RT));
	_cvtsd2ss_r128_r128(REG_XMM1, REG_XMM0);		// convert double to float
	_movd_r32_r128(REG_EAX, REG_XMM1);
	_push_r32(REG_EAX);
	if (RA == 0)
	{
		_push_imm(SIMM16);
	}
	else
	{
		_mov_r32_m32abs(REG_EAX, &REG(RA));
		_add_r32_imm(REG_EAX, SIMM16);
		_push_r32(REG_EAX);
	}
	_call((genf *)WRITE32);
	_add_r32_imm(REG_ESP, 8);
#else
	_push_imm(op);
	_call((genf *)ppc_stfs);
	_add_r32_imm(REG_ESP, 4);
#endif
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_stfsu(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
#if USE_SSE2
	_movq_r128_m64abs(REG_XMM0, &FPR(RT));
	_cvtsd2ss_r128_r128(REG_XMM1, REG_XMM0);		// convert double to float
	_movd_r32_r128(REG_EAX, REG_XMM1);
	_push_r32(REG_EAX);

	_mov_r32_m32abs(REG_EAX, &REG(RA));
	_add_r32_imm(REG_EAX, SIMM16);
	_mov_m32abs_r32(&REG(RA), REG_EAX);
	_push_r32(REG_EAX);
	_call((genf *)WRITE32);
	_add_r32_imm(REG_ESP, 8);
#else
	_push_imm(op);
	_call((genf *)ppc_stfsu);
	_add_r32_imm(REG_ESP, 4);
#endif
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_stfd(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_mov_r64_m64abs(REG_EDX, REG_EAX, &FPR(RT));
	_push_r32(REG_EDX);
	_push_r32(REG_EAX);
	if (RA == 0)
	{
		_push_imm(SIMM16);
	}
	else
	{
		_mov_r32_m32abs(REG_EAX, &REG(RA));
		_add_r32_imm(REG_EAX, SIMM16);
		_push_r32(REG_EAX);
	}
	_call((genf *)WRITE64);
	_add_r32_imm(REG_ESP, 12);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_stfdu(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_mov_r64_m64abs(REG_EDX, REG_EAX, &FPR(RT));
	_push_r32(REG_EDX);
	_push_r32(REG_EAX);

	_mov_r32_m32abs(REG_EAX, &REG(RA));
	_add_r32_imm(REG_EAX, SIMM16);
	_mov_m32abs_r32(&REG(RA), REG_EAX);
	_push_r32(REG_EAX);
	_call((genf *)WRITE64);
	_add_r32_imm(REG_ESP, 12);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_lfdux(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_mov_r32_m32abs(REG_EDX, &REG(RA));
	_add_r32_m32abs(REG_EDX, &REG(RB));
	_mov_m32abs_r32(&REG(RA), REG_EDX);
	_push_r32(REG_EDX);
	_call((genf*)READ64);
	_add_r32_imm(REG_ESP, 4);
	_mov_m64abs_r64(&FPR(RT), REG_EDX, REG_EAX);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_lfdx(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_mov_r32_m32abs(REG_EAX, &REG(RB));
	if (RA != 0)
	{
		_add_r32_m32abs(REG_EAX, &REG(RA));
	}
	_push_r32(REG_EAX);
	_call((genf*)READ64);
	_add_r32_imm(REG_ESP, 4);
	_mov_m64abs_r64(&FPR(RT), REG_EDX, REG_EAX);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_lfsux(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
#if USE_SSE2
	_mov_r32_m32abs(REG_EDX, &REG(RA));
	_add_r32_m32abs(REG_EDX, &REG(RB));
	_mov_m32abs_r32(&REG(RA), REG_EDX);
	_push_r32(REG_EDX);
	_call((genf *)READ32);
	_add_r32_imm(REG_ESP, 4);
	_movd_r128_r32(REG_XMM0, REG_EAX);
	_cvtss2sd_r128_r128(REG_XMM1, REG_XMM0);		// convert float to double
	_movq_m64abs_r128(&FPR(RT), REG_XMM1);
#else
	_push_imm(op);
	_call((genf *)ppc_lfsux);
	_add_r32_imm(REG_ESP, 4);
#endif
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_lfsx(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
#if USE_SSE2
	_mov_r32_m32abs(REG_EAX, &REG(RB));
	if (RA != 0)
	{
		_add_r32_m32abs(REG_EAX, &REG(RA));
	}
	_push_r32(REG_EAX);
	_call((genf *)READ32);
	_add_r32_imm(REG_ESP, 4);
	_movd_r128_r32(REG_XMM0, REG_EAX);
	_cvtss2sd_r128_r128(REG_XMM1, REG_XMM0);		// convert float to double
	_movq_m64abs_r128(&FPR(RT), REG_XMM1);
#else
	_push_imm(op);
	_call((genf *)ppc_lfsx);
	_add_r32_imm(REG_ESP, 4);
#endif
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_mfsr(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_mfsr);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_mfsrin(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_mfsrin);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_mftb(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_mftb);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_mtsr(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_mtsr);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_mtsrin(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_mtsrin);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_dcba(drc_core *drc, UINT32 op)
{
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_stfdux(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_mov_r64_m64abs(REG_EDX, REG_EAX, &FPR(RT));
	_push_r32(REG_EDX);
	_push_r32(REG_EAX);

	_mov_r32_m32abs(REG_EAX, &REG(RA));
	_add_r32_m32abs(REG_EAX, &REG(RB));
	_mov_m32abs_r32(&REG(RA), REG_EAX);
	_push_r32(REG_EAX);
	_call((genf *)WRITE64);
	_add_r32_imm(REG_ESP, 12);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_stfdx(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_mov_r64_m64abs(REG_EDX, REG_EAX, &FPR(RT));
	_push_r32(REG_EDX);
	_push_r32(REG_EAX);

	_mov_r32_m32abs(REG_EAX, &REG(RB));
	if (RA != 0)
	{
		_add_r32_m32abs(REG_EAX, &REG(RA));
	}
	_push_r32(REG_EAX);
	_call((genf *)WRITE64);
	_add_r32_imm(REG_ESP, 12);

	_push_imm(op);
	_call((genf *)ppc_stfdx);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_stfiwx(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
#if USE_SSE2
	_movq_r128_m64abs(REG_XMM0, &FPR(RT));
	_movd_r32_r128(REG_EAX, REG_XMM0);
	_push_r32(REG_EAX);

	_mov_r32_m32abs(REG_EAX, &REG(RB));
	if (RA != 0)
	{
		_add_r32_m32abs(REG_EAX, &REG(RA));
	}
	_push_r32(REG_EAX);
	_call((genf *)WRITE32);
	_add_r32_imm(REG_ESP, 8);
#else
	_push_imm(op);
	_call((genf *)ppc_stfiwx);
	_add_r32_imm(REG_ESP, 4);
#endif
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_stfsux(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
#if USE_SSE2
	_movq_r128_m64abs(REG_XMM0, &FPR(RT));
	_cvtsd2ss_r128_r128(REG_XMM1, REG_XMM0);		// convert double to float
	_movd_r32_r128(REG_EAX, REG_XMM1);
	_push_r32(REG_EAX);

	_mov_r32_m32abs(REG_EAX, &REG(RA));
	_add_r32_m32abs(REG_EAX, &REG(RB));
	_mov_m32abs_r32(&REG(RA), REG_EAX);
	_push_r32(REG_EAX);
	_call((genf *)WRITE32);
	_add_r32_imm(REG_ESP, 8);
#else
	_push_imm(op);
	_call((genf *)ppc_stfsux);
	_add_r32_imm(REG_ESP, 4);
#endif
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_stfsx(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
#if USE_SSE2
	_movq_r128_m64abs(REG_XMM0, &FPR(RT));
	_cvtsd2ss_r128_r128(REG_XMM1, REG_XMM0);		// convert double to float
	_movd_r32_r128(REG_EAX, REG_XMM1);
	_push_r32(REG_EAX);

	_mov_r32_m32abs(REG_EAX, &REG(RB));
	if (RA != 0)
	{
		_add_r32_m32abs(REG_EAX, &REG(RA));
	}
	_push_r32(REG_EAX);
	_call((genf *)WRITE32);
	_add_r32_imm(REG_ESP, 8);
#else
	_push_imm(op);
	_call((genf *)ppc_stfsx);
	_add_r32_imm(REG_ESP, 4);
#endif
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_tlbia(drc_core *drc, UINT32 op)
{
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_tlbie(drc_core *drc, UINT32 op)
{
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_tlbsync(drc_core *drc, UINT32 op)
{
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_eciwx(drc_core *drc, UINT32 op)
{
	mame_printf_debug("PPCDRC: eciwx unimplemented\n");
	return RECOMPILE_UNIMPLEMENTED;
}

static UINT32 recompile_ecowx(drc_core *drc, UINT32 op)
{
	mame_printf_debug("PPCDRC: ecowx unimplemented\n");
	return RECOMPILE_UNIMPLEMENTED;
}

static UINT32 recompile_fabsx(drc_core *drc, UINT32 op)
{
#if !COMPILE_FPU || !USE_SSE2
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_fabsx);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);
#else
	_mov_r64_m64abs(REG_EDX, REG_EAX, &FPR(RB));
	_and_r32_imm(REG_EDX, 0x7fffffff);
	_mov_m64abs_r64(&FPR(RT), REG_EDX, REG_EAX);

	if (RCBIT) {
		append_set_cr1(drc);
	}
#endif
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_faddx(drc_core *drc, UINT32 op)
{
#if !COMPILE_FPU || !USE_SSE2
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_faddx);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);
#else
	_movq_r128_m64abs(REG_XMM0, &FPR(RA));
	_movq_r128_m64abs(REG_XMM1, &FPR(RB));
	_addsd_r128_r128(REG_XMM0, REG_XMM1);
	_movq_m64abs_r128(&FPR(RT), REG_XMM0);

	if (RCBIT) {
		append_set_cr1(drc);
	}
#endif
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_fcmpo(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_fcmpo);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_fcmpu(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_fcmpu);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_fctiwx(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_fctiwx);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_fctiwzx(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_fctiwzx);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_fdivx(drc_core *drc, UINT32 op)
{
#if !COMPILE_FPU || !USE_SSE2
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_fdivx);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);
#else
	_movq_r128_m64abs(REG_XMM0, &FPR(RA));
	_movq_r128_m64abs(REG_XMM1, &FPR(RB));
	_divsd_r128_r128(REG_XMM0, REG_XMM1);
	_movq_m64abs_r128(&FPR(RT), REG_XMM0);

	if (RCBIT) {
		append_set_cr1(drc);
	}
#endif

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_fmrx(drc_core *drc, UINT32 op)
{
#if USE_SSE2
	_movq_r128_m64abs(REG_XMM0, &FPR(RB));
	_movq_m64abs_r128(&FPR(RT), REG_XMM0);

	if (RCBIT) {
		append_set_cr1(drc);
	}
#else
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_fmrx);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);
#endif

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_fnabsx(drc_core *drc, UINT32 op)
{
#if !COMPILE_FPU || !USE_SSE2
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_fnabsx);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);
#else
	_mov_r64_m64abs(REG_EDX, REG_EAX, &FPR(RB));
	_or_r32_imm(REG_EDX, 0x80000000);
	_mov_m64abs_r64(&FPR(RT), REG_EDX, REG_EAX);

	if (RCBIT) {
		append_set_cr1(drc);
	}
#endif

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_fnegx(drc_core *drc, UINT32 op)
{
#if !COMPILE_FPU || !USE_SSE2
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_fnegx);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);
#else
	_mov_r64_m64abs(REG_EDX, REG_EAX, &FPR(RB));
	_xor_r32_imm(REG_EDX, 0x80000000);
	_mov_m64abs_r64(&FPR(RT), REG_EDX, REG_EAX);

	if (RCBIT) {
		append_set_cr1(drc);
	}
#endif

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_frspx(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_frspx);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);
/*
    _movq_r128_m64abs(REG_XMM0, &FPR(RB));
    _movq_m64abs_r128(&FPR(RT), REG_XMM0);

    if (RCBIT) {
        append_set_cr1(drc);
    }
*/
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_frsqrtex(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_frsqrtex);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_fsqrtx(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_fsqrtx);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_fsubx(drc_core *drc, UINT32 op)
{
#if !COMPILE_FPU || !USE_SSE2
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_fsubx);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);
#else
	_movq_r128_m64abs(REG_XMM0, &FPR(RA));
	_movq_r128_m64abs(REG_XMM1, &FPR(RB));
	_subsd_r128_r128(REG_XMM0, REG_XMM1);
	_movq_m64abs_r128(&FPR(RT), REG_XMM0);

	if (RCBIT) {
		append_set_cr1(drc);
	}
#endif

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_mffsx(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_mffsx);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_mtfsb0x(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_mtfsb0x);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_mtfsb1x(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_mtfsb1x);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_mtfsfx(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_mtfsfx);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_mtfsfix(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_mtfsfix);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_mcrfs(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_mcrfs);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_faddsx(drc_core *drc, UINT32 op)
{
#if !COMPILE_FPU || !USE_SSE2
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_faddsx);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);
#else
	_movq_r128_m64abs(REG_XMM0, &FPR(RA));
	_movq_r128_m64abs(REG_XMM1, &FPR(RB));
	_addsd_r128_r128(REG_XMM0, REG_XMM1);
	_movq_m64abs_r128(&FPR(RT), REG_XMM0);

	if (RCBIT) {
		append_set_cr1(drc);
	}
#endif

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_fdivsx(drc_core *drc, UINT32 op)
{
#if !COMPILE_FPU || !USE_SSE2
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_fdivsx);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);
#else
	_movq_r128_m64abs(REG_XMM0, &FPR(RA));
	_movq_r128_m64abs(REG_XMM1, &FPR(RB));
	_divsd_r128_r128(REG_XMM0, REG_XMM1);
	_movq_m64abs_r128(&FPR(RT), REG_XMM0);

	if (RCBIT) {
		append_set_cr1(drc);
	}
#endif

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_fresx(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_fresx);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_fsqrtsx(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_fsqrtsx);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_fsubsx(drc_core *drc, UINT32 op)
{
#if !COMPILE_FPU || !USE_SSE2
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_fsubsx);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);
#else
	_movq_r128_m64abs(REG_XMM0, &FPR(RA));
	_movq_r128_m64abs(REG_XMM1, &FPR(RB));
	_subsd_r128_r128(REG_XMM0, REG_XMM1);
	_movq_m64abs_r128(&FPR(RT), REG_XMM0);

	if (RCBIT) {
		append_set_cr1(drc);
	}
#endif

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_fmaddx(drc_core *drc, UINT32 op)
{
#if !COMPILE_FPU || !USE_SSE2
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_fmaddx);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);
#else
	_movq_r128_m64abs(REG_XMM0, &FPR(RA));
	_movq_r128_m64abs(REG_XMM1, &FPR(RC));
	_movq_r128_m64abs(REG_XMM2, &FPR(RB));
	_mulsd_r128_r128(REG_XMM0, REG_XMM1);
	_addsd_r128_r128(REG_XMM0, REG_XMM2);
	_movq_m64abs_r128(&FPR(RT), REG_XMM0);

	if (RCBIT) {
		append_set_cr1(drc);
	}
#endif

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_fmsubx(drc_core *drc, UINT32 op)
{
#if !COMPILE_FPU || !USE_SSE2
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_fmsubx);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);
#else
	_movq_r128_m64abs(REG_XMM0, &FPR(RA));
	_movq_r128_m64abs(REG_XMM1, &FPR(RC));
	_movq_r128_m64abs(REG_XMM2, &FPR(RB));
	_mulsd_r128_r128(REG_XMM0, REG_XMM1);
	_subsd_r128_r128(REG_XMM0, REG_XMM2);
	_movq_m64abs_r128(&FPR(RT), REG_XMM0);

	if (RCBIT) {
		append_set_cr1(drc);
	}
#endif

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_fmulx(drc_core *drc, UINT32 op)
{
#if !COMPILE_FPU || !USE_SSE2
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_fmulx);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);
#else
	_movq_r128_m64abs(REG_XMM0, &FPR(RA));
	_movq_r128_m64abs(REG_XMM1, &FPR(RC));
	_mulsd_r128_r128(REG_XMM0, REG_XMM1);
	_movq_m64abs_r128(&FPR(RT), REG_XMM0);

	if (RCBIT) {
		append_set_cr1(drc);
	}
#endif

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_fnmaddx(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_fnmaddx);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_fnmsubx(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_fnmsubx);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_fselx(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_fselx);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_fmaddsx(drc_core *drc, UINT32 op)
{
#if !COMPILE_FPU || !USE_SSE2
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_fmaddsx);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);
#else
	_movq_r128_m64abs(REG_XMM0, &FPR(RA));
	_movq_r128_m64abs(REG_XMM1, &FPR(RC));
	_movq_r128_m64abs(REG_XMM2, &FPR(RB));
	_mulsd_r128_r128(REG_XMM0, REG_XMM1);
	_addsd_r128_r128(REG_XMM0, REG_XMM2);
	_movq_m64abs_r128(&FPR(RT), REG_XMM0);

	if (RCBIT) {
		append_set_cr1(drc);
	}
#endif

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_fmsubsx(drc_core *drc, UINT32 op)
{
#if !COMPILE_FPU || !USE_SSE2
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_fmsubsx);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);
#else
	_movq_r128_m64abs(REG_XMM0, &FPR(RA));
	_movq_r128_m64abs(REG_XMM1, &FPR(RC));
	_movq_r128_m64abs(REG_XMM2, &FPR(RB));
	_mulsd_r128_r128(REG_XMM0, REG_XMM1);
	_subsd_r128_r128(REG_XMM0, REG_XMM2);
	_movq_m64abs_r128(&FPR(RT), REG_XMM0);

	if (RCBIT) {
		append_set_cr1(drc);
	}
#endif

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_fmulsx(drc_core *drc, UINT32 op)
{
#if !COMPILE_FPU || !USE_SSE2
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_fmulsx);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);
#else
	_movq_r128_m64abs(REG_XMM0, &FPR(RA));
	_movq_r128_m64abs(REG_XMM1, &FPR(RC));
	_mulsd_r128_r128(REG_XMM0, REG_XMM1);
	_movq_m64abs_r128(&FPR(RT), REG_XMM0);

	if (RCBIT) {
		append_set_cr1(drc);
	}
#endif

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_fnmaddsx(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_fnmaddsx);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_fnmsubsx(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_fnmsubsx);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

// PPC602

#if (HAS_PPC602)
static UINT32 recompile_esa(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_esa);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_dsa(drc_core *drc, UINT32 op)
{
	_mov_m32abs_r32(&ppc_icount, REG_EBP);
	_push_imm(op);
	_call((genf *)ppc_dsa);
	_add_r32_imm(REG_ESP, 4);
	_mov_r32_m32abs(REG_EBP, &ppc_icount);

	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_tlbli(drc_core *drc, UINT32 op)
{
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}

static UINT32 recompile_tlbld(drc_core *drc, UINT32 op)
{
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}
#endif /* HAS_PPC602 */

