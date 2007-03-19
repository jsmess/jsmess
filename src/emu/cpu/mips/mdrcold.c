/***************************************************************************
    CONFIGURATION
***************************************************************************/

#define STRIP_NOPS			1		/* 1 is faster */
#define USE_SSE				0		/* can't tell any speed difference here */



/***************************************************************************
    CONSTANTS
***************************************************************************/

/* recompiler flags */
#define RECOMPILE_UNIMPLEMENTED			0x0000
#define RECOMPILE_SUCCESSFUL			0x0001
#define RECOMPILE_SUCCESSFUL_CP(c,p)	(RECOMPILE_SUCCESSFUL | (((c) & 0xfff) << 8) | (((p) & 0xfff) << 20))
#define RECOMPILE_MAY_CAUSE_EXCEPTION	0x0002
#define RECOMPILE_END_OF_STRING			0x0004
#define RECOMPILE_CHECK_INTERRUPTS		0x0008
#define RECOMPILE_CHECK_SW_INTERRUPTS	0x0010
#define RECOMPILE_ADD_DISPATCH			0x0020



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static UINT32 compile_one(drc_core *drc, UINT32 pc, UINT32 physpc);

static void append_generate_exception(drc_core *drc, UINT8 exception);
static void append_readwrite_and_translate(drc_core *drc, int is_write, int size, int is_signed, int do_translate);
static void append_tlb_verify(drc_core *drc, UINT32 pc, void *target);
static void append_update_cycle_counting(drc_core *drc);
static void append_check_interrupts(drc_core *drc, int inline_generate);
static void append_check_sw_interrupts(drc_core *drc, int inline_generate);

static UINT32 recompile_instruction(drc_core *drc, UINT32 pc, UINT32 physpc);
static UINT32 recompile_special(drc_core *drc, UINT32 pc, UINT32 op);
static UINT32 recompile_regimm(drc_core *drc, UINT32 pc, UINT32 op);

static UINT32 recompile_cop0(drc_core *drc, UINT32 pc, UINT32 op);
static UINT32 recompile_cop1(drc_core *drc, UINT32 pc, UINT32 op);
static UINT32 recompile_cop1x(drc_core *drc, UINT32 pc, UINT32 op);



/***************************************************************************
    PRIVATE GLOBAL VARIABLES
***************************************************************************/

static UINT64 dmult_temp1;
static UINT64 dmult_temp2;
static UINT32 jr_temp;

static UINT8 in_delay_slot = 0;



/***************************************************************************
    RECOMPILER CALLBACKS
***************************************************************************/

/*------------------------------------------------------------------
    mips3drc_init
------------------------------------------------------------------*/

static void mips3drc_init(void)
{
	drc_config drconfig;

	/* fill in the config */
	memset(&drconfig, 0, sizeof(drconfig));
	drconfig.cache_size       = CACHE_SIZE;
	drconfig.max_instructions = MAX_INSTRUCTIONS;
	drconfig.address_bits     = 32;
	drconfig.lsbs_to_ignore   = 2;
	drconfig.uses_fp          = 1;
	drconfig.uses_sse         = USE_SSE;
	drconfig.pc_in_memory     = 0;
	drconfig.icount_in_memory = 0;
	drconfig.pcptr            = (UINT32 *)&mips3.pc;
	drconfig.icountptr        = (UINT32 *)&mips3_icount;
	drconfig.esiptr           = NULL;
	drconfig.cb_reset         = mips3drc_reset;
	drconfig.cb_recompile     = mips3drc_recompile;
	drconfig.cb_entrygen      = mips3drc_entrygen;

	/* initialize the compiler */
	mips3.drc = drc_init(cpu_getactivecpu(), &drconfig);
	mips3.drcoptions = MIPS3DRC_FASTEST_OPTIONS;
}



/*------------------------------------------------------------------
    mips3drc_reset
------------------------------------------------------------------*/

static void mips3drc_reset(drc_core *drc)
{
	code_log_reset();

	mips3.generate_interrupt_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_INTERRUPT);
	code_log(drc, "generate_interrupt_exception:", mips3.generate_interrupt_exception);

	mips3.generate_cop_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_BADCOP);
	code_log(drc, "generate_cop_exception:", mips3.generate_cop_exception);

	mips3.generate_overflow_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_OVERFLOW);
	code_log(drc, "generate_overflow_exception:", mips3.generate_overflow_exception);

	mips3.generate_invalidop_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_INVALIDOP);
	code_log(drc, "generate_invalidop_exception:", mips3.generate_invalidop_exception);

	mips3.generate_syscall_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_SYSCALL);
	code_log(drc, "generate_syscall_exception:", mips3.generate_syscall_exception);

	mips3.generate_break_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_BREAK);
	code_log(drc, "generate_break_exception:", mips3.generate_break_exception);

	mips3.generate_trap_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_TRAP);
	code_log(drc, "generate_trap_exception:", mips3.generate_trap_exception);

	mips3.generate_tlbload_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_TLBLOAD);
	code_log(drc, "generate_tlbload_exception:", mips3.generate_tlbload_exception);

	mips3.generate_tlbstore_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_TLBSTORE);
	code_log(drc, "generate_tlbstore_exception:", mips3.generate_tlbstore_exception);

	mips3.handle_pc_tlb_mismatch = drc->cache_top;
	_mov_r32_r32(REG_EAX, REG_EDI);													// mov  eax,edi
	_shr_r32_imm(REG_EAX, 12);														// shr  eax,12
	_mov_r32_m32isd(REG_EBX, REG_EAX, 4, &mips3.tlb_table);							// mov  ebx,tlb_table[eax*4]
	_test_r32_imm(REG_EBX, 2);														// test ebx,2
	_mov_r32_r32(REG_EAX, REG_EDI);													// mov  eax,edi
	_jcc(COND_NZ, mips3.generate_tlbload_exception);								// jnz  generate_tlbload_exception
	_mov_m32abs_r32(drc->pcptr, REG_EDI);											// mov  [pcptr],edi
	_jmp(drc->recompile);															// call recompile
	code_log(drc, "handle_pc_tlb_mismatch:", mips3.handle_pc_tlb_mismatch);

	mips3.read_and_translate_byte_signed = drc->cache_top;
	append_readwrite_and_translate(drc, FALSE, 1, TRUE, TRUE);
	code_log(drc, "read_and_translate_byte_signed:", mips3.read_and_translate_byte_signed);

	mips3.read_and_translate_byte_unsigned = drc->cache_top;
	append_readwrite_and_translate(drc, FALSE, 1, FALSE, TRUE);
	code_log(drc, "read_and_translate_byte_unsigned:", mips3.read_and_translate_byte_unsigned);

	mips3.read_and_translate_word_signed = drc->cache_top;
	append_readwrite_and_translate(drc, FALSE, 2, TRUE, TRUE);
	code_log(drc, "read_and_translate_word_signed:", mips3.read_and_translate_word_signed);

	mips3.read_and_translate_word_unsigned = drc->cache_top;
	append_readwrite_and_translate(drc, FALSE, 2, FALSE, TRUE);
	code_log(drc, "read_and_translate_word_unsigned:", mips3.read_and_translate_word_unsigned);

	mips3.read_and_translate_long = drc->cache_top;
	append_readwrite_and_translate(drc, FALSE, 4, FALSE, TRUE);
	code_log(drc, "read_and_translate_long:", mips3.read_and_translate_long);

	mips3.read_and_translate_double = drc->cache_top;
	append_readwrite_and_translate(drc, FALSE, 8, FALSE, TRUE);
	code_log(drc, "read_and_translate_double:", mips3.read_and_translate_double);

	mips3.write_and_translate_byte = drc->cache_top;
	append_readwrite_and_translate(drc, TRUE, 1, FALSE, TRUE);
	code_log(drc, "write_and_translate_byte:", mips3.write_and_translate_byte);

	mips3.write_and_translate_word = drc->cache_top;
	append_readwrite_and_translate(drc, TRUE, 2, FALSE, TRUE);
	code_log(drc, "write_and_translate_word:", mips3.write_and_translate_word);

	mips3.write_and_translate_long = drc->cache_top;
	append_readwrite_and_translate(drc, TRUE, 4, FALSE, TRUE);
	code_log(drc, "write_and_translate_long:", mips3.write_and_translate_long);

	mips3.write_and_translate_double = drc->cache_top;
	append_readwrite_and_translate(drc, TRUE, 8, FALSE, TRUE);
	code_log(drc, "write_and_translate_double:", mips3.write_and_translate_double);

	mips3.write_back_long = drc->cache_top;
	append_readwrite_and_translate(drc, TRUE, 4, FALSE, FALSE);
	code_log(drc, "write_back_long:", mips3.write_back_long);

	mips3.write_back_double = drc->cache_top;
	append_readwrite_and_translate(drc, TRUE, 8, FALSE, FALSE);
	code_log(drc, "write_back_double:", mips3.write_back_double);
}



/*------------------------------------------------------------------
    mips3drc_recompile
------------------------------------------------------------------*/

static void mips3drc_recompile(drc_core *drc)
{
	int remaining = MAX_INSTRUCTIONS;
	void *start = drc->cache_top;
	UINT32 pc = mips3.pc;
	UINT32 lastpc = pc;
	UINT32 physpc;

	(void)start;

	/* begin the sequence */
	drc_begin_sequence(drc, pc);
	code_log_reset();

	/* make sure our FR flag matches the last time */
	_test_m32abs_imm(&mips3.cpr[0][COP0_Status], SR_FR);							// test [COP0_Status],SR_FR
	_jcc(IS_FR0 ? COND_NZ : COND_Z, drc->recompile);								// jnz/z recompile

	/* ensure that we are still mapped */
	append_tlb_verify(drc, pc, mips3.handle_pc_tlb_mismatch);
	physpc = pc;
	if (!translate_address(ADDRESS_SPACE_PROGRAM, &physpc))
	{
		_mov_r32_r32(REG_EAX, REG_EDI);												// mov  eax,edi
		_jmp(mips3.generate_tlbload_exception);										// jmp  tlbload_exception
		goto exit;
	}
	change_pc(physpc);

	/* loose verification case: one verification here only */
	if (!(mips3.drcoptions & MIPS3DRC_STRICT_VERIFY))
		drc_append_verify_code(drc, cpu_opptr(physpc), 4);

	/* loop until we hit an unconditional branch */
	while (--remaining != 0)
	{
		UINT32 result;

		/* if we crossed a page boundary, check the mapping */
		if ((pc ^ lastpc) & 0xfffff000)
		{
			physpc = pc;
			if (!translate_address(ADDRESS_SPACE_PROGRAM, &physpc))
				break;
			append_tlb_verify(drc, pc, drc->dispatch);
			change_pc(physpc);
		}

		/* compile one instruction */
		physpc = (physpc & 0xfffff000) | (pc & 0xfff);
		result = compile_one(drc, pc, physpc);
		lastpc = pc;
		pc += (INT32)result >> 20;
		if (result & RECOMPILE_END_OF_STRING)
			break;
	}

	/* add dispatcher just in case */
	if (remaining == 0)
		drc_append_dispatcher(drc);

exit:
	/* end the sequence */
	drc_end_sequence(drc);

#if LOG_CODE
{
	char label[40];
	physpc = mips3.pc;
	translate_address(ADDRESS_SPACE_PROGRAM, &physpc);
	sprintf(label, "Code @ %08X (%08X physical)", mips3.pc, physpc);
	code_log(drc, label, start);
}
#endif
}


/*------------------------------------------------------------------
    mips3drc_entrygen
------------------------------------------------------------------*/

static void mips3drc_entrygen(drc_core *drc)
{
	_mov_r32_imm(REG_ESI, &mips3.r[17]);
	append_check_interrupts(drc, 1);
}



/***************************************************************************
    RECOMPILER CORE
***************************************************************************/

/*------------------------------------------------------------------
    compile_one
------------------------------------------------------------------*/

static UINT32 compile_one(drc_core *drc, UINT32 pc, UINT32 physpc)
{
	int pcdelta, cycles, hotnum;
	UINT32 *opptr;
	UINT32 result;

	/* register this instruction */
	drc_register_code_at_cache_top(drc, pc);

	/* get a pointer to the current instruction */
	change_pc(physpc);
	opptr = cpu_opptr(physpc);

	/* emit debugging and self-modifying code checks */
	drc_append_call_debugger(drc);
	if (mips3.drcoptions & MIPS3DRC_STRICT_VERIFY)
		drc_append_verify_code(drc, opptr, 4);

	/* is this a hotspot? */
	for (hotnum = 0; hotnum < MIPS3_MAX_HOTSPOTS; hotnum++)
		if (pc == mips3.hotspot[hotnum].pc && *(UINT32 *)opptr == mips3.hotspot[hotnum].opcode)
			break;

	/* compile the instruction */
	result = recompile_instruction(drc, pc, physpc);

	/* handle the results */
	if (!(result & RECOMPILE_SUCCESSFUL))
		fatalerror("Unimplemented op %08X (%02X,%02X)", *opptr, *opptr >> 26, *opptr & 0x3f);

	pcdelta = (INT32)result >> 20;
	cycles = (result >> 8) & 0xfff;

	/* absorb any NOPs following */
	if (STRIP_NOPS && !Machine->debug_mode)
	{
		if (!(result & (RECOMPILE_END_OF_STRING | RECOMPILE_CHECK_INTERRUPTS | RECOMPILE_CHECK_SW_INTERRUPTS)))
			while ((((pc + pcdelta) ^ pc) & 0xfffff000) == 0 && pcdelta < 120 && opptr[pcdelta/4] == 0)
			{
				pcdelta += 4;
				cycles += 1;
			}
	}

	/* epilogue */
	if (hotnum != MIPS3_MAX_HOTSPOTS)
		cycles += mips3.hotspot[hotnum].cycles;
	drc_append_standard_epilogue(drc, cycles, pcdelta, 1);

	/* check interrupts */
	if (result & RECOMPILE_CHECK_INTERRUPTS)
		append_check_interrupts(drc, 0);
	if (result & RECOMPILE_CHECK_SW_INTERRUPTS)
		append_check_sw_interrupts(drc, 0);
	if (result & RECOMPILE_ADD_DISPATCH)
		drc_append_dispatcher(drc);

	return (result & 0xff) | ((cycles & 0xfff) << 8) | ((pcdelta & 0xfff) << 20);
}



/***************************************************************************
    COMMON ROUTINES
***************************************************************************/

/*------------------------------------------------------------------
    append_generate_exception
------------------------------------------------------------------*/

static void append_generate_exception(drc_core *drc, UINT8 exception)
{
	UINT32 offset = (exception >= EXCEPTION_TLBMOD && exception <= EXCEPTION_TLBSTORE) ? 0x00 : 0x180;
	link_info link1, link2;

	/* assumes address is in EAX */
	if (exception == EXCEPTION_TLBLOAD || exception == EXCEPTION_TLBSTORE)
	{
		_mov_m32abs_r32(&mips3.cpr[0][COP0_BadVAddr], REG_EAX);						// mov  [mips3.cpr[0][BadVAddr]],eax
		_and_m32abs_imm(&mips3.cpr[0][COP0_EntryHi], 0x000000ff);					// and  [mips3.cpr[0][EntryHi]],0x000000ff
		_and_r32_imm(REG_EAX, 0xffffe000);											// and  eax,0xffffe000
		_or_m32abs_r32(&mips3.cpr[0][COP0_EntryHi], REG_EAX);						// or   [mips3.cpr[0][EntryHi]],eax
		_and_m32abs_imm(&mips3.cpr[0][COP0_Context], 0xff800000);					// and  [mips3.cpr[0][Context]],0xff800000
		_shr_r32_imm(REG_EAX, 9);													// shr  eax,9
		_or_m32abs_r32(&mips3.cpr[0][COP0_Context], REG_EAX);						// or   [mips3.cpr[0][Context]],eax
	}

	_mov_m32abs_r32(&mips3.cpr[0][COP0_EPC], REG_EDI);								// mov  [mips3.cpr[0][COP0_EPC]],edi
	_mov_r32_m32abs(REG_EAX, &mips3.cpr[0][COP0_Cause]);							// mov  eax,[mips3.cpr[0][COP0_Cause]]
	_and_r32_imm(REG_EAX, ~0x800000ff);												// and  eax,~0x800000ff
	if (exception)
		_or_r32_imm(REG_EAX, exception << 2);										// or   eax,exception << 2
	_cmp_m32abs_imm(&mips3.nextpc, ~0);												// cmp  [mips3.nextpc],~0
	_jcc_short_link(COND_E, &link1);												// je   skip
	_mov_m32abs_imm(&mips3.nextpc, ~0);												// mov  [mips3.nextpc],~0
	_sub_m32abs_imm(&mips3.cpr[0][COP0_EPC], 4);									// sub  [mips3.cpr[0][COP0_EPC]],4
	_or_r32_imm(REG_EAX, 0x80000000);												// or   eax,0x80000000
	_resolve_link(&link1);															// skip:
	_mov_m32abs_r32(&mips3.cpr[0][COP0_Cause], REG_EAX);							// mov  [mips3.cpr[0][COP0_Cause]],eax
	_mov_r32_m32abs(REG_EAX, &mips3.cpr[0][COP0_Status]);							// mov  eax,[[mips3.cpr[0][COP0_Status]]
	_or_r32_imm(REG_EAX, SR_EXL);													// or   eax,SR_EXL
	_test_r32_imm(REG_EAX, SR_BEV);													// test eax,SR_BEV
	_mov_m32abs_r32(&mips3.cpr[0][COP0_Status], REG_EAX);							// mov  [[mips3.cpr[0][COP0_Status]],eax
	_mov_r32_imm(REG_EDI, 0xbfc00200 + offset);										// mov  edi,0xbfc00200+offset
	_jcc_short_link(COND_NZ, &link2);												// jnz  skip2
	_mov_r32_imm(REG_EDI, 0x80000000 + offset);										// mov  edi,0x80000000+offset
	_resolve_link(&link2);															// skip2:
	drc_append_dispatcher(drc);														// dispatch
}


/*------------------------------------------------------------------
    append_readwrite_and_translate
------------------------------------------------------------------*/

static void append_readwrite_and_translate(drc_core *drc, int is_write, int size, int is_signed, int do_translate)
{
	link_info link1 = { 0 }, link2 = { 0 }, link3 = { 0 };
	int ramnum;

	if (do_translate)
	{
		_mov_r32_m32bd(REG_EAX, REG_ESP, 4);										// mov  eax,[esp+4]
		_mov_r32_r32(REG_EBX, REG_EAX);												// mov  ebx,eax
		_shr_r32_imm(REG_EBX, 12);													// shr  ebx,12
		_mov_r32_m32isd(REG_EBX, REG_EBX, 4, mips3.tlb_table);						// mov  ebx,tlb_table[ebx*4]
		_and_r32_imm(REG_EAX, 0xfff);												// and  eax,0xfff
		_shr_r32_imm(REG_EBX, (is_write ? 1 : 2));									// shr  ebx,2/1 (read/write)
		_lea_r32_m32bisd(REG_EBX, REG_EAX, REG_EBX, (is_write ? 2 : 4), 0);			// lea  ebx,[eax+ebx*4/2] (read/write)
		_jcc_short_link(COND_C, &link1);											// jc   error
	}
	for (ramnum = 0; ramnum < MIPS3_MAX_FASTRAM; ramnum++)
		if (!Machine->debug_mode && mips3.fastram[ramnum].base && (!is_write || !mips3.fastram[ramnum].readonly))
		{
			UINT32 fastbase = (UINT32)((UINT8 *)mips3.fastram[ramnum].base - mips3.fastram[ramnum].start);
			if (mips3.fastram[ramnum].end != 0xffffffff)
			{
				_cmp_r32_imm(REG_EBX, mips3.fastram[ramnum].end);					// cmp  ebx,fastram_end
				_jcc_short_link(COND_A, &link2);									// ja   notram
			}
			if (mips3.fastram[ramnum].start != 0x00000000)
			{
				_cmp_r32_imm(REG_EBX, mips3.fastram[ramnum].start);					// cmp  ebx,fastram_start
				_jcc_short_link(COND_B, &link3);									// jb   notram
			}

			if (!is_write)
			{
				if (size == 1)
				{
					if (mips3.bigendian)
						_xor_r32_imm(REG_EBX, 3);									// xor   ebx,3
					if (is_signed)
						_movsx_r32_m8bd(REG_EAX, REG_EBX, fastbase);				// movsx eax,byte ptr [ebx+fastbase]
					else
						_movzx_r32_m8bd(REG_EAX, REG_EBX, fastbase);				// movzx eax,byte ptr [ebx+fastbase]
				}
				else if (size == 2)
				{
					if (mips3.bigendian)
						_xor_r32_imm(REG_EBX, 2);									// xor   ebx,2
					if (is_signed)
						_movsx_r32_m16bd(REG_EAX, REG_EBX, fastbase);				// movsx eax,word ptr [ebx+fastbase]
					else
						_movzx_r32_m16bd(REG_EAX, REG_EBX, fastbase);				// movzx eax,word ptr [ebx+fastbase]
				}
				else if (size == 4)
					_mov_r32_m32bd(REG_EAX, REG_EBX, fastbase);						// mov   eax,[ebx+fastbase]
				else if (size == 8)
				{
					if (mips3.bigendian)
						_mov_r64_m64bd(REG_EAX, REG_EDX, REG_EBX, fastbase);		// mov   eax:edx,[ebx+fastbase]
					else
						_mov_r64_m64bd(REG_EDX, REG_EAX, REG_EBX, fastbase);		// mov   edx:eax,[ebx+fastbase]
				}
				_ret();																// ret
			}
			else
			{
				if (size == 1)
				{
					_mov_r8_m8bd(REG_AL, REG_ESP, 8);								// mov   al,[esp+8]
					if (mips3.bigendian)
						_xor_r32_imm(REG_EBX, 3);									// xor   ebx,3
					_mov_m8bd_r8(REG_EBX, fastbase, REG_AL);						// mov   [ebx+fastbase],al
				}
				else if (size == 2)
				{
					_mov_r16_m16bd(REG_AX, REG_ESP, 8);								// mov   ax,[esp+8]
					if (mips3.bigendian)
						_xor_r32_imm(REG_EBX, 2);									// xor   ebx,2
					_mov_m16bd_r16(REG_EBX, fastbase, REG_AX);						// mov   [ebx+fastbase],ax
				}
				else if (size == 4)
				{
					_mov_r32_m32bd(REG_EAX, REG_ESP, 8);							// mov   eax,[esp+8]
					_mov_m32bd_r32(REG_EBX, fastbase, REG_EAX);						// mov   [ebx+fastbase],eax
				}
				else if (size == 8)
				{
					if (mips3.bigendian)
					{
						_mov_r32_m32bd(REG_EAX, REG_ESP, 8);						// mov   eax,[esp+8]
						_mov_m32bd_r32(REG_EBX, fastbase+4, REG_EAX);				// mov   [ebx+fastbase+4],eax
						_mov_r32_m32bd(REG_EAX, REG_ESP, 12);						// mov   eax,[esp+12]
						_mov_m32bd_r32(REG_EBX, fastbase, REG_EAX);					// mov   [ebx+fastbase],eax
					}
					else
					{
						_mov_r32_m32bd(REG_EAX, REG_ESP, 8);						// mov   eax,[esp+8]
						_mov_m32bd_r32(REG_EBX, fastbase, REG_EAX);					// mov   [ebx+fastbase],eax
						_mov_r32_m32bd(REG_EAX, REG_ESP, 12);						// mov   eax,[esp+12]
						_mov_m32bd_r32(REG_EBX, fastbase+4, REG_EAX);				// mov   [ebx+fastbase+4],eax
					}
				}
				_ret();																// ret
			}

			if (mips3.fastram[ramnum].end != 0xffffffff)							// notram:
				_resolve_link(&link2);
			if (mips3.fastram[ramnum].start != 0x00000000)
				_resolve_link(&link3);
		}

	if (is_write)
	{
		if (size != 8)
		{
			_mov_m32bd_r32(REG_ESP, 4, REG_EBX);									// mov  [esp+4],ebx
			if (size == 1)
				_jmp(mips3.memory.writebyte);										// jmp  writebyte
			else if (size == 2)
				_jmp(mips3.memory.writeword);										// jmp  writeword
			else
				_jmp(mips3.memory.writelong);										// jmp  writelong
		}
		else
		{
			_push_m32bd(REG_ESP, mips3.bigendian ? 12 : 8);							// push [esp+8/12]
			_push_r32(REG_EBX);														// push ebx
			_call(mips3.memory.writelong);											// call writelong
			_add_r32_imm(REG_EBX, 4);												// add  ebx,4
			_push_m32bd(REG_ESP, mips3.bigendian ? (8+8) : (12+8));					// push [esp+8+8/12]
			_push_r32(REG_EBX);														// push ebx
			_call(mips3.memory.writelong);											// call writelong
			_sub_r32_imm(REG_EBX, 4);												// sub  ebx,4
			_add_r32_imm(REG_ESP, 16);												// add  esp,16
			_ret();																	// ret
		}
	}
	else
	{
		if (size == 1)
		{
			_push_r32(REG_EBX);														// push ebx
			_call(mips3.memory.readbyte);											// call  readbyte
			if (is_signed)
				_movsx_r32_r8(REG_EAX, REG_AL);										// movsx eax,al
			else
				_movzx_r32_r8(REG_EAX, REG_AL);										// movzx eax,al
			_add_r32_imm(REG_ESP, 4);												// add  esp,4
			_ret();																	// ret
		}
		else if (size == 2)
		{
			_push_r32(REG_EBX);														// push ebx
			_call(mips3.memory.readword);											// call  readword
			if (is_signed)
				_movsx_r32_r16(REG_EAX, REG_AX);									// movsx eax,ax
			else
				_movzx_r32_r16(REG_EAX, REG_AX);									// movzx eax,ax
			_add_r32_imm(REG_ESP, 4);												// add  esp,4
			_ret();																	// ret
		}
		else if (size == 4)
		{
			_mov_m32bd_r32(REG_ESP, 4, REG_EBX);									// mov  [esp+4],ebx
			_jmp(mips3.memory.readlong);											// jmp  readlong
		}
		else
		{
			_push_r32(REG_EBX);														// push ebx
			_call(mips3.memory.readlong);											// call readlong
			_add_r32_imm(REG_EBX, 4);												// add  ebx,4
			_mov_m32bd_r32(REG_ESP, 0, REG_EAX);									// mov  [esp],eax
			_push_r32(REG_EBX);														// push ebx
			_call(mips3.memory.readlong);											// call readlong
			_add_r32_imm(REG_ESP, 4);												// add  esp,4
			if (mips3.bigendian)
				_pop_r32(REG_EDX);													// pop  edx
			else
			{
				_mov_r32_r32(REG_EDX, REG_EAX);										// mov  edx,eax
				_pop_r32(REG_EAX);													// pop  eax
			}
			_sub_r32_imm(REG_EBX, 4);												// sub  ebx,4
			_ret();																	// ret
		}
	}
	if (do_translate)
	{
		_resolve_link(&link1);														// error:
		_mov_r32_m32bd(REG_EAX, REG_ESP, 4);										// mov  eax,[esp+4]
		_add_r32_imm(REG_ESP, is_write ? (8+4) : (4+4));							// add  esp,stack_bytes
		_jmp(is_write ? mips3.generate_tlbstore_exception : mips3.generate_tlbload_exception);// jmp    generate_exception
	}
}


/*------------------------------------------------------------------
    append_tlb_verify
------------------------------------------------------------------*/

static void append_tlb_verify(drc_core *drc, UINT32 pc, void *target)
{
	/* addresses 0x80000000-0xbfffffff are direct-mapped; no checking needed */
	if (pc < 0x80000000 || pc >= 0xc0000000)
	{
		_cmp_m32abs_imm(&mips3.tlb_table[pc >> 12], mips3.tlb_table[pc >> 12]);		// cmp  tlbtable[pc >> 12],physpc & 0xfffff000
		_jcc(COND_NE, target);														// jne  handle_pc_tlb_mismatch
	}
}


/*------------------------------------------------------------------
    append_update_cycle_counting
------------------------------------------------------------------*/

static void append_update_cycle_counting(drc_core *drc)
{
	_mov_m32abs_r32(&mips3_icount, REG_EBP);										// mov  [mips3_icount],ebp
	_call((genf *)update_cycle_counting);											// call update_cycle_counting
	_mov_r32_m32abs(REG_EBP, &mips3_icount);										// mov  ebp,[mips3_icount]
}


/*------------------------------------------------------------------
    append_check_interrupts
------------------------------------------------------------------*/

static void append_check_interrupts(drc_core *drc, int inline_generate)
{
	link_info link1, link2, link3;
	_mov_r32_m32abs(REG_EAX, &mips3.cpr[0][COP0_Cause]);							// mov  eax,[mips3.cpr[0][COP0_Cause]]
	_and_r32_m32abs(REG_EAX, &mips3.cpr[0][COP0_Status]);							// and  eax,[mips3.cpr[0][COP0_Status]]
	_and_r32_imm(REG_EAX, 0xfc00);													// and  eax,0xfc00
	if (!inline_generate)
		_jcc_short_link(COND_Z, &link1);											// jz   skip
	else
		_jcc_near_link(COND_Z, &link1);												// jz   skip
	_test_m32abs_imm(&mips3.cpr[0][COP0_Status], SR_IE);							// test [mips3.cpr[0][COP0_Status],SR_IE
	if (!inline_generate)
		_jcc_short_link(COND_Z, &link2);											// jz   skip
	else
		_jcc_near_link(COND_Z, &link2);												// jz   skip
	_test_m32abs_imm(&mips3.cpr[0][COP0_Status], SR_EXL | SR_ERL);					// test [mips3.cpr[0][COP0_Status],SR_EXL | SR_ERL
	if (!inline_generate)
		_jcc(COND_Z, mips3.generate_interrupt_exception);							// jz   generate_interrupt_exception
	else
	{
		_jcc_near_link(COND_NZ, &link3);											// jnz  skip
		append_generate_exception(drc, EXCEPTION_INTERRUPT);						// <generate exception>
		_resolve_link(&link3);														// skip:
	}
	_resolve_link(&link1);															// skip:
	_resolve_link(&link2);
}


/*------------------------------------------------------------------
    append_check_sw_interrupts
------------------------------------------------------------------*/

static void append_check_sw_interrupts(drc_core *drc, int inline_generate)
{
	_test_m32abs_imm(&mips3.cpr[0][COP0_Cause], 0x300);								// test [mips3.cpr[0][COP0_Cause]],0x300
	_jcc(COND_NZ, mips3.generate_interrupt_exception);								// jnz  generate_interrupt_exception
}


/*------------------------------------------------------------------
    append_branch_or_dispatch
------------------------------------------------------------------*/

static void append_branch_or_dispatch(drc_core *drc, UINT32 newpc, int cycles)
{
	void *code = drc_get_code_at_pc(drc, newpc);
	_mov_r32_imm(REG_EDI, newpc);
	drc_append_standard_epilogue(drc, cycles, 0, 1);

	if (code)
		_jmp(code);
	else
		drc_append_tentative_fixed_dispatcher(drc, newpc);
}



/***************************************************************************
    USEFUL PRIMITIVES
***************************************************************************/

#define REGDISP(reg) offsetof(mips3_regs, r[reg]) - offsetof(mips3_regs, r[17])

#define _zero_m64abs(addr) 							\
do { 												\
	if (USE_SSE) 									\
	{												\
		_pxor_r128_r128(REG_XMM0, REG_XMM0);		\
		_movsd_m64abs_r128(addr, REG_XMM0);			\
	}												\
	else											\
		_mov_m64abs_imm32(addr, 0);					\
} while (0)											\

#define _zero_m64bd(base, disp)						\
do { 												\
	if (USE_SSE) 									\
	{												\
		_pxor_r128_r128(REG_XMM0, REG_XMM0);		\
		_movsd_m64bd_r128(base, disp, REG_XMM0);	\
	}												\
	else											\
		_mov_m64bd_imm32(base, disp, 0);			\
} while (0)											\

#define _mov_m64abs_m64abs(dst, src)				\
do { 												\
	if (USE_SSE) 									\
	{												\
		_movsd_r128_m64abs(REG_XMM0, src);			\
		_movsd_m64abs_r128(dst, REG_XMM0);			\
	}												\
	else											\
	{												\
		_mov_r64_m64abs(REG_EDX, REG_EAX, src);		\
		_mov_m64abs_r64(dst, REG_EDX, REG_EAX);		\
	}												\
} while (0)											\

#define _mov_m64bd_m64bd(dstb, dstd, srcb, srcd)	\
do { 												\
	if (USE_SSE) 									\
	{												\
		_movsd_r128_m64bd(REG_XMM0, srcb, srcd);	\
		_movsd_m64bd_r128(dstb, dstd, REG_XMM0);	\
	}												\
	else											\
	{												\
		_mov_r64_m64bd(REG_EDX, REG_EAX, srcb, srcd);\
		_mov_m64bd_r64(dstb, dstd, REG_EDX, REG_EAX);\
	}												\
} while (0)											\

#define _mov_m64bd_m64abs(dstb, dstd, src)			\
do { 												\
	if (USE_SSE) 									\
	{												\
		_movsd_r128_m64abs(REG_XMM0, src);			\
		_movsd_m64bd_r128(dstb, dstd, REG_XMM0);	\
	}												\
	else											\
	{												\
		_mov_r64_m64abs(REG_EDX, REG_EAX, src);		\
		_mov_m64bd_r64(dstb, dstd, REG_EDX, REG_EAX);\
	}												\
} while (0)											\

#define _mov_m64abs_m64bd(dst, srcb, srcd)			\
do { 												\
	if (USE_SSE) 									\
	{												\
		_movsd_r128_m64bd(REG_XMM0, srcb, srcd);	\
		_movsd_m64abs_r128(dst, REG_XMM0);			\
	}												\
	else											\
	{												\
		_mov_r64_m64bd(REG_EDX, REG_EAX, srcb, srcd);\
		_mov_m64abs_r64(dst, REG_EDX, REG_EAX);		\
	}												\
} while (0)											\

#define _save_pc_before_call()						\
do { 												\
	if (mips3.drcoptions & MIPS3DRC_FLUSH_PC)		\
		_mov_m32abs_r32(drc->pcptr, REG_EDI);		\
} while (0)

#define FPR32(x)									\
	(IS_FR0 ? &((float *)&mips3.cpr[1][0])[x] : 	\
	 (float *)&mips3.cpr[1][x])						\

#define FPR64(x)									\
	(IS_FR0 ? (double *)&mips3.cpr[1][(x)/2] :		\
	 (double *)&mips3.cpr[1][x])					\


/***************************************************************************
    CORE RECOMPILATION
***************************************************************************/

static void ddiv(INT64 *rs, INT64 *rt)
{
	if (*rt)
	{
		mips3.lo = *rs / *rt;
		mips3.hi = *rs % *rt;
	}
}

static void ddivu(UINT64 *rs, UINT64 *rt)
{
	if (*rt)
	{
		mips3.lo = *rs / *rt;
		mips3.hi = *rs % *rt;
	}
}


/*------------------------------------------------------------------
    recompile_delay_slot
------------------------------------------------------------------*/

static int recompile_delay_slot(drc_core *drc, UINT32 pc)
{
	UINT8 *saved_top;
	UINT32 result;
	UINT32 physpc;

#ifdef MAME_DEBUG
	if (Machine->debug_mode)
	{
		/* emit debugging */
		_mov_r32_imm(REG_EDI, pc);
		drc_append_call_debugger(drc);
	}
#endif

	saved_top = drc->cache_top;

	/* recompile the instruction as-is */
	in_delay_slot = 1;
	physpc = pc;
	translate_address(ADDRESS_SPACE_PROGRAM, &physpc);
	result = recompile_instruction(drc, pc, physpc);								// <next instruction>
	in_delay_slot = 0;

	/* if the instruction can cause an exception, recompile setting nextpc */
	if (result & RECOMPILE_MAY_CAUSE_EXCEPTION)
	{
		drc->cache_top = saved_top;
		_mov_m32abs_imm(&mips3.nextpc, 0);											// bogus nextpc for exceptions
		result = recompile_instruction(drc, pc, physpc);							// <next instruction>
		_mov_m32abs_imm(&mips3.nextpc, ~0);											// reset nextpc
	}

	return (result >> 8) & 0xfff;
}


/*------------------------------------------------------------------
    recompile_instruction
------------------------------------------------------------------*/

static UINT32 recompile_instruction(drc_core *drc, UINT32 pc, UINT32 physpc)
{
	static UINT32 ldl_mask[] =
	{
		0x00000000,0x00000000,
		0x00000000,0x000000ff,
		0x00000000,0x0000ffff,
		0x00000000,0x00ffffff,
		0x00000000,0xffffffff,
		0x000000ff,0xffffffff,
		0x0000ffff,0xffffffff,
		0x00ffffff,0xffffffff
	};
	static UINT32 ldr_mask[] =
	{
		0x00000000,0x00000000,
		0xff000000,0x00000000,
		0xffff0000,0x00000000,
		0xffffff00,0x00000000,
		0xffffffff,0x00000000,
		0xffffffff,0xff000000,
		0xffffffff,0xffff0000,
		0xffffffff,0xffffff00
	};
	static UINT32 sdl_mask[] =
	{
		0x00000000,0x00000000,
		0xff000000,0x00000000,
		0xffff0000,0x00000000,
		0xffffff00,0x00000000,
		0xffffffff,0x00000000,
		0xffffffff,0xff000000,
		0xffffffff,0xffff0000,
		0xffffffff,0xffffff00
	};
	static UINT32 sdr_mask[] =
	{
		0x00000000,0x00000000,
		0x00000000,0x000000ff,
		0x00000000,0x0000ffff,
		0x00000000,0x00ffffff,
		0x00000000,0xffffffff,
		0x000000ff,0xffffffff,
		0x0000ffff,0xffffffff,
		0x00ffffff,0xffffffff
	};
	link_info link1, link2 = { 0 }, link3;
	UINT32 op = cpu_readop32(physpc);
	int cycles;

	code_log_add_entry(pc, op, drc->cache_top);

	switch (op >> 26)
	{
		case 0x00:	/* SPECIAL */
			return recompile_special(drc, pc, op);

		case 0x01:	/* REGIMM */
			return recompile_regimm(drc, pc, op);

		case 0x02:	/* J */
			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			append_branch_or_dispatch(drc, (pc & 0xf0000000) | (LIMMVAL << 2), 1+cycles);// <branch or dispatch>
			return RECOMPILE_SUCCESSFUL_CP(0,0) | RECOMPILE_END_OF_STRING;

		case 0x03:	/* JAL */
			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			_mov_m64bd_imm32(REG_ESI, REGDISP(31), pc + 8);							// mov  [31],pc + 8
			append_branch_or_dispatch(drc, (pc & 0xf0000000) | (LIMMVAL << 2), 1+cycles);// <branch or dispatch>
			return RECOMPILE_SUCCESSFUL_CP(0,0) | RECOMPILE_END_OF_STRING;

		case 0x04:	/* BEQ */
			if (RSREG == RTREG)
			{
				cycles = recompile_delay_slot(drc, pc + 4);							// <next instruction>
				append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);	// <branch or dispatch>
				return RECOMPILE_SUCCESSFUL_CP(0,0) | RECOMPILE_END_OF_STRING;
			}
			else if (RSREG == 0)
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));					// mov  eax,[rtreg].lo
				_or_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG)+4);					// or   eax,[rtreg].hi
				_jcc_near_link(COND_NZ, &link1);									// jnz  skip
			}
			else if (RTREG == 0)
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));					// mov  eax,[rsreg].lo
				_or_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG)+4);					// or   eax,[rsreg].hi
				_jcc_near_link(COND_NZ, &link1);									// jnz  skip
			}
			else
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));					// mov  eax,[rsreg].lo
				_cmp_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));					// cmp  eax,[rtreg].lo
				_jcc_near_link(COND_NE, &link1);									// jne  skip
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG)+4);					// mov  eax,[rsreg].hi
				_cmp_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG)+4);					// cmp  eax,[rtreg].hi
				_jcc_near_link(COND_NE, &link2);									// jne  skip
			}

			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		// <branch or dispatch>
			_resolve_link(&link1);													// skip:
			if (RSREG != 0 && RTREG != 0)
				_resolve_link(&link2);												// skip:
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x05:	/* BNE */
			if (RSREG == 0)
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));					// mov  eax,[rtreg].lo
				_or_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG)+4);					// or   eax,[rtreg].hi
				_jcc_near_link(COND_Z, &link1);										// jz   skip
			}
			else if (RTREG == 0)
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));					// mov  eax,[rsreg].lo
				_or_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG)+4);					// or   eax,[rsreg].hi
				_jcc_near_link(COND_Z, &link1);										// jz   skip
			}
			else
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));					// mov  eax,[rsreg].lo
				_cmp_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));					// cmp  eax,[rtreg].lo
				_jcc_short_link(COND_NE, &link2);									// jne  takeit
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG)+4);					// mov  eax,[rsreg].hi
				_cmp_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG)+4);					// cmp  eax,[rtreg].hi
				_jcc_near_link(COND_E, &link1);										// je   skip
				_resolve_link(&link2);												// takeit:
			}

			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		// <branch or dispatch>
			_resolve_link(&link1);													// skip:
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x06:	/* BLEZ */
			if (RSREG == 0)
			{
				cycles = recompile_delay_slot(drc, pc + 4);							// <next instruction>
				append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);	// <branch or dispatch>
				return RECOMPILE_SUCCESSFUL_CP(0,0) | RECOMPILE_END_OF_STRING;
			}
			else
			{
				_cmp_m32bd_imm(REG_ESI, REGDISP(RSREG)+4, 0);						// cmp  [rsreg].hi,0
				_jcc_near_link(COND_G, &link1);										// jg   skip
				_jcc_short_link(COND_L, &link2);									// jl   takeit
				_cmp_m32bd_imm(REG_ESI, REGDISP(RSREG), 0);							// cmp  [rsreg].lo,0
				_jcc_near_link(COND_NE, &link3);									// jne  skip
				_resolve_link(&link2);												// takeit:
			}

			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		// <branch or dispatch>
			_resolve_link(&link1);													// skip:
			_resolve_link(&link3);													// skip:
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x07:	/* BGTZ */
			if (RSREG == 0)
				return RECOMPILE_SUCCESSFUL_CP(1,4);
			else
			{
				_cmp_m32bd_imm(REG_ESI, REGDISP(RSREG)+4, 0);						// cmp  [rsreg].hi,0
				_jcc_near_link(COND_L, &link1);										// jl   skip
				_jcc_short_link(COND_G, &link2);									// jg   takeit
				_cmp_m32bd_imm(REG_ESI, REGDISP(RSREG), 0);							// cmp  [rsreg].lo,0
				_jcc_near_link(COND_E, &link3);										// je   skip
				_resolve_link(&link2);												// takeit:
			}

			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		// <branch or dispatch>
			_resolve_link(&link1);													// skip:
			_resolve_link(&link3);													// skip:
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x08:	/* ADDI */
			if (RSREG != 0)
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));					// mov  eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add  eax,SIMMVAL
				_jcc(COND_O, mips3.generate_overflow_exception);					// jo   generate_overflow_exception
				if (RTREG != 0)
				{
					_cdq();															// cdq
					_mov_m64bd_r64(REG_ESI, REGDISP(RTREG), REG_EDX, REG_EAX);		// mov  [rtreg],edx:eax
				}
			}
			else if (RTREG != 0)
				_mov_m64bd_imm32(REG_ESI, REGDISP(RTREG), SIMMVAL);					// mov  [rtreg],const
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x09:	/* ADDIU */
			if (RTREG != 0)
			{
				if (RSREG != 0)
				{
					_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));				// mov  eax,[rsreg]
					_add_r32_imm(REG_EAX, SIMMVAL);									// add  eax,SIMMVAL
					_cdq();															// cdq
					_mov_m64bd_r64(REG_ESI, REGDISP(RTREG), REG_EDX, REG_EAX);		// mov  [rtreg],edx:eax
				}
				else
					_mov_m64bd_imm32(REG_ESI, REGDISP(RTREG), SIMMVAL);				// mov  [rtreg],const
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x0a:	/* SLTI */
			if (RTREG != 0)
			{
				if (RSREG != 0)
				{
					_mov_r64_m64bd(REG_EDX, REG_EAX, REG_ESI, REGDISP(RSREG));		// mov  edx:eax,[rsreg]
					_sub_r32_imm(REG_EAX, SIMMVAL);									// sub  eax,[rtreg].lo
					_sbb_r32_imm(REG_EDX, ((INT32)SIMMVAL >> 31));					// sbb  edx,[rtreg].lo
					_shr_r32_imm(REG_EDX, 31);										// shr  edx,31
					_mov_m32bd_r32(REG_ESI, REGDISP(RTREG), REG_EDX);				// mov  [rdreg].lo,edx
					_mov_m32bd_imm(REG_ESI, REGDISP(RTREG)+4, 0);					// mov  [rdreg].hi,0
				}
				else
				{
					_mov_m32bd_imm(REG_ESI, REGDISP(RTREG), (0 < SIMMVAL));			// mov  [rtreg].lo,const
					_mov_m32bd_imm(REG_ESI, REGDISP(RTREG)+4, 0);					// mov  [rtreg].hi,sign-extend(const)
				}
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x0b:	/* SLTIU */
			if (RTREG != 0)
			{
				if (RSREG != 0)
				{
					_xor_r32_r32(REG_ECX, REG_ECX);									// xor  ecx,ecx
					_cmp_m32bd_imm(REG_ESI, REGDISP(RSREG)+4, ((INT32)SIMMVAL >> 31));// cmp  [rsreg].hi,upper
					_jcc_short_link(COND_B, &link1);								// jb   takeit
					_jcc_short_link(COND_A, &link2);								// ja   skip
					_cmp_m32bd_imm(REG_ESI, REGDISP(RSREG), SIMMVAL);				// cmp  [rsreg].lo,lower
					_jcc_short_link(COND_AE, &link3);								// jae  skip
					_resolve_link(&link1);											// takeit:
					_add_r32_imm(REG_ECX, 1);										// add  ecx,1
					_resolve_link(&link2);											// skip:
					_resolve_link(&link3);											// skip:
					_mov_m32bd_r32(REG_ESI, REGDISP(RTREG), REG_ECX);				// mov  [rtreg].lo,ecx
					_mov_m32bd_imm(REG_ESI, REGDISP(RTREG)+4, 0);					// mov  [rtreg].hi,sign-extend(const)
				}
				else
				{
					_mov_m32bd_imm(REG_ESI, REGDISP(RTREG), (0 < SIMMVAL));			// mov  [rtreg].lo,const
					_mov_m32bd_imm(REG_ESI, REGDISP(RTREG)+4, 0);					// mov  [rtreg].hi,sign-extend(const)
				}
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x0c:	/* ANDI */
			if (RTREG != 0)
			{
				if (RSREG == RTREG)
				{
					_and_m32bd_imm(REG_ESI, REGDISP(RTREG), UIMMVAL);				// and  [rtreg],UIMMVAL
					_mov_m32bd_imm(REG_ESI, REGDISP(RTREG)+4, 0);					// mov  [rtreg].hi,0
				}
				else if (RSREG != 0)
				{
					_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));				// mov  eax,[rsreg].lo
					_and_r32_imm(REG_EAX, UIMMVAL);									// and  eax,UIMMVAL
					_mov_m32bd_r32(REG_ESI, REGDISP(RTREG), REG_EAX);				// mov  [rtreg].lo,eax
					_mov_m32bd_imm(REG_ESI, REGDISP(RTREG)+4, 0);					// mov  [rtreg].hi,0
				}
				else
					_mov_m64bd_imm32(REG_ESI, REGDISP(RTREG), 0);					// mov  [rtreg],0
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x0d:	/* ORI */
			if (RTREG != 0)
			{
				if (RSREG == RTREG)
					_or_m32bd_imm(REG_ESI, REGDISP(RTREG), UIMMVAL);				// or   [rtreg],UIMMVAL
				else if (RSREG != 0)
				{
					_mov_r64_m64bd(REG_EDX, REG_EAX, REG_ESI, REGDISP(RSREG));		// mov  edx:eax,[rsreg]
					_or_r32_imm(REG_EAX, UIMMVAL);									// or   eax,UIMMVAL
					_mov_m64bd_r64(REG_ESI, REGDISP(RTREG), REG_EDX, REG_EAX);		// mov  [rtreg],edx:eax
				}
				else
					_mov_m64bd_imm32(REG_ESI, REGDISP(RTREG), UIMMVAL);				// mov  [rtreg],const
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x0e:	/* XORI */
			if (RTREG != 0)
			{
				if (RSREG == RTREG)
					_xor_m32bd_imm(REG_ESI, REGDISP(RTREG), UIMMVAL);				// xor  [rtreg],UIMMVAL
				else if (RSREG != 0)
				{
					_mov_r64_m64bd(REG_EDX, REG_EAX, REG_ESI, REGDISP(RSREG));		// mov  edx:eax,[rsreg]
					_xor_r32_imm(REG_EAX, UIMMVAL);									// xor  eax,UIMMVAL
					_mov_m64bd_r64(REG_ESI, REGDISP(RTREG), REG_EDX, REG_EAX);		// mov  [rtreg],edx:eax
				}
				else
					_mov_m64bd_imm32(REG_ESI, REGDISP(RTREG), UIMMVAL);				// mov  [rtreg],const
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x0f:	/* LUI */
			if (RTREG != 0)
				_mov_m64bd_imm32(REG_ESI, REGDISP(RTREG), UIMMVAL << 16);			// mov  [rtreg],const << 16
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x10:	/* COP0 */
			return recompile_cop0(drc, pc, op) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x11:	/* COP1 */
			return recompile_cop1(drc, pc, op) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x12:	/* COP2 */
			_jmp((void *)mips3.generate_invalidop_exception);						// jmp  generate_invalidop_exception
			return RECOMPILE_SUCCESSFUL | RECOMPILE_MAY_CAUSE_EXCEPTION | RECOMPILE_END_OF_STRING;

		case 0x13:	/* COP1X - R5000 */
			if (!mips3.is_mips4)
			{
				_jmp((void *)mips3.generate_invalidop_exception);					// jmp  generate_invalidop_exception
				return RECOMPILE_SUCCESSFUL | RECOMPILE_MAY_CAUSE_EXCEPTION | RECOMPILE_END_OF_STRING;
			}
			return recompile_cop1x(drc, pc, op) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x14:	/* BEQL */
			if (RSREG == RTREG)
			{
				cycles = recompile_delay_slot(drc, pc + 4);							// <next instruction>
				append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);	// <branch or dispatch>
				return RECOMPILE_SUCCESSFUL_CP(0,0) | RECOMPILE_END_OF_STRING;
			}
			else if (RSREG == 0)
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));					// mov  eax,[rtreg].lo
				_or_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG)+4);					// or   eax,[rtreg].hi
				_jcc_near_link(COND_NZ, &link1);									// jnz  skip
			}
			else if (RTREG == 0)
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));					// mov  eax,[rsreg].lo
				_or_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG)+4);					// or   eax,[rsreg].hi
				_jcc_near_link(COND_NZ, &link1);									// jnz  skip
			}
			else
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));					// mov  eax,[rsreg].lo
				_cmp_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));					// cmp  eax,[rtreg].lo
				_jcc_near_link(COND_NE, &link1);									// jne  skip
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG)+4);					// mov  eax,[rsreg].hi
				_cmp_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG)+4);					// cmp  eax,[rtreg].hi
				_jcc_near_link(COND_NE, &link2);									// jne  skip
			}

			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		// <branch or dispatch>
			_resolve_link(&link1);													// skip:
			if (RSREG != 0 && RTREG != 0)
				_resolve_link(&link2);												// skip:
			return RECOMPILE_SUCCESSFUL_CP(1,8);

		case 0x15:	/* BNEL */
			if (RSREG == 0)
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));					// mov  eax,[rtreg].lo
				_or_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG)+4);					// or   eax,[rtreg].hi
				_jcc_near_link(COND_Z, &link1);										// jz   skip
			}
			else if (RTREG == 0)
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));					// mov  eax,[rsreg].lo
				_or_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG)+4);					// or   eax,[rsreg].hi
				_jcc_near_link(COND_Z, &link1);										// jz   skip
			}
			else
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));					// mov  eax,[rsreg].lo
				_cmp_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));					// cmp  eax,[rtreg].lo
				_jcc_short_link(COND_NE, &link2);									// jne  takeit
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG)+4);					// mov  eax,[rsreg].hi
				_cmp_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG)+4);					// cmp  eax,[rtreg].hi
				_jcc_near_link(COND_E, &link1);										// je   skip
				_resolve_link(&link2);												// takeit:
			}

			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		// <branch or dispatch>
			_resolve_link(&link1);													// skip:
			return RECOMPILE_SUCCESSFUL_CP(1,8);

		case 0x16:	/* BLEZL */
			if (RSREG == 0)
			{
				cycles = recompile_delay_slot(drc, pc + 4);							// <next instruction>
				append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);	// <branch or dispatch>
				return RECOMPILE_SUCCESSFUL_CP(0,0) | RECOMPILE_END_OF_STRING;
			}
			else
			{
				_cmp_m32bd_imm(REG_ESI, REGDISP(RSREG)+4, 0);						// cmp  [rsreg].hi,0
				_jcc_near_link(COND_G, &link1);										// jg   skip
				_jcc_short_link(COND_L, &link2);									// jl   takeit
				_cmp_m32bd_imm(REG_ESI, REGDISP(RSREG), 0);							// cmp  [rsreg].lo,0
				_jcc_near_link(COND_NE, &link3);									// jne  skip
				_resolve_link(&link2);												// takeit:
			}

			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		// <branch or dispatch>
			_resolve_link(&link1);													// skip:
			_resolve_link(&link3);													// skip:
			return RECOMPILE_SUCCESSFUL_CP(1,8);

		case 0x17:	/* BGTZL */
			if (RSREG == 0)
				return RECOMPILE_SUCCESSFUL_CP(1,8);
			else
			{
				_cmp_m32bd_imm(REG_ESI, REGDISP(RSREG)+4, 0);						// cmp  [rsreg].hi,0
				_jcc_near_link(COND_L, &link1);										// jl   skip
				_jcc_short_link(COND_G, &link2);									// jg   takeit
				_cmp_m32bd_imm(REG_ESI, REGDISP(RSREG), 0);							// cmp  [rsreg].lo,0
				_jcc_near_link(COND_E, &link3);										// je   skip
				_resolve_link(&link2);												// takeit:
			}

			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		// <branch or dispatch>
			_resolve_link(&link1);													// skip:
			_resolve_link(&link3);													// skip:
			return RECOMPILE_SUCCESSFUL_CP(1,8);

		case 0x18:	/* DADDI */
			if (RSREG != 0)
			{
				_mov_r64_m64bd(REG_EDX, REG_EAX, REG_ESI, REGDISP(RSREG));			// mov  edx:eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add  eax,SIMMVAL
				_adc_r32_imm(REG_EDX, (SIMMVAL < 0) ? -1 : 0);						// adc  edx,signext(SIMMVAL)
				_jcc(COND_O, mips3.generate_overflow_exception);					// jo   generate_overflow_exception
				if (RTREG != 0)
					_mov_m64bd_r64(REG_ESI, REGDISP(RTREG), REG_EDX, REG_EAX);		// mov  [rtreg],edx:eax
			}
			else if (RTREG != 0)
				_mov_m64bd_imm32(REG_ESI, REGDISP(RTREG), SIMMVAL);					// mov  [rtreg],const
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x19:	/* DADDIU */
			if (RTREG != 0)
			{
				if (RSREG != 0)
				{
					_mov_r64_m64bd(REG_EDX, REG_EAX, REG_ESI, REGDISP(RSREG));		// mov  edx:eax,[rsreg]
					_add_r32_imm(REG_EAX, SIMMVAL);									// add  eax,SIMMVAL
					_adc_r32_imm(REG_EDX, (SIMMVAL < 0) ? -1 : 0);					// adc  edx,signext(SIMMVAL)
					_mov_m64bd_r64(REG_ESI, REGDISP(RTREG), REG_EDX, REG_EAX);		// mov  [rtreg],edx:eax
				}
				else
					_mov_m64bd_imm32(REG_ESI, REGDISP(RTREG), SIMMVAL);				// mov  [rtreg],const
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x1a:	/* LDL */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov  [mips3_icount],ebp
			_save_pc_before_call();													// save pc
			_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));						// mov  eax,[rsreg]
			if (SIMMVAL)
				_add_r32_imm(REG_EAX, SIMMVAL);										// add  eax,SIMMVAL
			_push_r32(REG_EAX);														// push eax
			_and_r32_imm(REG_EAX, ~7);												// and  eax,~7
			_push_r32(REG_EAX);														// push eax
			_call((genf *)mips3.read_and_translate_double);							// call read_and_translate_double
			_add_r32_imm(REG_ESP, 4);												// add  esp,4
			_pop_r32(REG_ECX);														// pop  ecx

			if (RTREG != 0)
			{
				_and_r32_imm(REG_ECX, 7);											// and  ecx,7
				_shl_r32_imm(REG_ECX, 3);											// shl  ecx,3
				if (!mips3.bigendian)
					_xor_r32_imm(REG_ECX, 0x38);									// xor  ecx,0x38
				_test_r32_imm(REG_ECX, 0x20);										// test ecx,0x20
				_jcc_short_link(COND_Z, &link1);									// jz   skip
				_mov_r32_r32(REG_EDX, REG_EAX);										// mov  edx,eax
				_xor_r32_r32(REG_EAX, REG_EAX);										// xor  eax,eax
				_resolve_link(&link1);												// skip:
				_shld_r32_r32_cl(REG_EDX, REG_EAX);									// shld edx,eax,cl
				_shl_r32_cl(REG_EAX);												// shl  eax,cl
				_mov_r32_m32bd(REG_EBX, REG_ESI, REGDISP(RTREG));					// mov  ebx,[rtreg].lo
				_and_r32_m32bd(REG_EBX, REG_ECX, ldl_mask + 1);						// and  ebx,[ldl_mask + ecx + 4]
				_or_r32_r32(REG_EAX, REG_EBX);										// or   eax,ebx
				_mov_r32_m32bd(REG_EBX, REG_ESI, REGDISP(RTREG)+4);					// mov  ebx,[rtreg].hi
				_and_r32_m32bd(REG_EBX, REG_ECX, ldl_mask);							// and  ebx,[ldl_mask + ecx]
				_or_r32_r32(REG_EDX, REG_EBX);										// or   edx,ebx
				_mov_m64bd_r64(REG_ESI, REGDISP(RTREG), REG_EDX, REG_EAX);			// mov  [rtreg],edx:eax
			}
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov  ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x1b:	/* LDR */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov  [mips3_icount],ebp
			_save_pc_before_call();													// save pc
			_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));						// mov  eax,[rsreg]
			if (SIMMVAL)
				_add_r32_imm(REG_EAX, SIMMVAL);										// add  eax,SIMMVAL
			_push_r32(REG_EAX);														// push eax
			_and_r32_imm(REG_EAX, ~7);												// and  eax,~7
			_push_r32(REG_EAX);														// push eax
			_call((genf *)mips3.read_and_translate_double);							// call read_and_translate_double
			_add_r32_imm(REG_ESP, 4);												// add  esp,4
			_pop_r32(REG_ECX);														// pop  ecx

			if (RTREG != 0)
			{
				_and_r32_imm(REG_ECX, 7);											// and  ecx,7
				_shl_r32_imm(REG_ECX, 3);											// shl  ecx,3
				if (mips3.bigendian)
					_xor_r32_imm(REG_ECX, 0x38);									// xor  ecx,0x38
				_test_r32_imm(REG_ECX, 0x20);										// test ecx,0x20
				_jcc_short_link(COND_Z, &link1);									// jz   skip
				_mov_r32_r32(REG_EAX, REG_EDX);										// mov  eax,edx
				_xor_r32_r32(REG_EDX, REG_EDX);										// xor  edx,edx
				_resolve_link(&link1);												// skip:
				_shrd_r32_r32_cl(REG_EAX, REG_EDX);									// shrd eax,edx,cl
				_shr_r32_cl(REG_EDX);												// shr  edx,cl
				_mov_r32_m32bd(REG_EBX, REG_ESI, REGDISP(RTREG));					// mov  ebx,[rtreg].lo
				_and_r32_m32bd(REG_EBX, REG_ECX, ldr_mask + 1);						// and  ebx,[ldr_mask + ecx + 4]
				_or_r32_r32(REG_EAX, REG_EBX);										// or   eax,ebx
				_mov_r32_m32bd(REG_EBX, REG_ESI, REGDISP(RTREG)+4);					// mov  ebx,[rtreg].hi
				_and_r32_m32bd(REG_EBX, REG_ECX, ldr_mask);							// and  ebx,[ldr_mask + ecx]
				_or_r32_r32(REG_EDX, REG_EBX);										// or   edx,ebx
				_mov_m64bd_r64(REG_ESI, REGDISP(RTREG), REG_EDX, REG_EAX);			// mov  [rtreg],edx:eax
			}
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov  ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x1c:	/* IDT-specific opcodes: mad/madu/mul on R4640/4650, msub on RC32364 */
			switch (op & 0x1f)
			{
				case 0: /* MAD */
					if (RSREG != 0 && RTREG != 0)
					{
						_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));		// mov  eax,[rsreg]
						_mov_r32_m32bd(REG_EDX, REG_ESI, REGDISP(RTREG));		// mov  edx,[rtreg]
						_imul_r32(REG_EDX);										// imul edx
						_add_r32_m32abs(REG_EAX, &mips3.lo);					// add  eax,[lo]
						_adc_r32_m32abs(REG_EDX, &mips3.hi);					// adc  edx,[hi]
						_mov_r32_r32(REG_EBX, REG_EDX);							// mov  ebx,edx
						_cdq();													// cdq
						_mov_m64abs_r64(&mips3.lo, REG_EDX, REG_EAX);			// mov  [lo],edx:eax
						_mov_r32_r32(REG_EAX, REG_EBX);							// mov  eax,ebx
						_cdq();													// cdq
						_mov_m64abs_r64(&mips3.hi, REG_EDX, REG_EAX);			// mov  [hi],edx:eax
					}
					return RECOMPILE_SUCCESSFUL_CP(3,4);

				case 1: /* MADU */
					if (RSREG != 0 && RTREG != 0)
					{
						_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));		// mov  eax,[rsreg]
						_mov_r32_m32bd(REG_EDX, REG_ESI, REGDISP(RTREG));		// mov  edx,[rtreg]
						_mul_r32(REG_EDX);										// mul  edx
						_add_r32_m32abs(REG_EAX, &mips3.lo);					// add  eax,[lo]
						_adc_r32_m32abs(REG_EDX, &mips3.hi);					// adc  edx,[hi]
						_mov_r32_r32(REG_EBX, REG_EDX);							// mov  ebx,edx
						_cdq();													// cdq
						_mov_m64abs_r64(&mips3.lo, REG_EDX, REG_EAX);			// mov  [lo],edx:eax
						_mov_r32_r32(REG_EAX, REG_EBX);							// mov  eax,ebx
						_cdq();													// cdq
						_mov_m64abs_r64(&mips3.hi, REG_EDX, REG_EAX);			// mov  [hi],edx:eax
					}
					return RECOMPILE_SUCCESSFUL_CP(3,4);

				case 2: /* MUL */
					if (RDREG != 0)
					{
						if (RSREG != 0 && RTREG != 0)
						{
							_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));		// mov  eax,[rsreg]
							_imul_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));		// imul eax,[rtreg]
							_cdq();													// cdq
							_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);// mov  [rdreg],edx:eax
						}
						else
							_zero_m64bd(REG_ESI, REGDISP(RDREG));
					}
					return RECOMPILE_SUCCESSFUL_CP(3,4);
			}
			break;

		case 0x20:	/* LB */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov  [mips3_icount],ebp
			_save_pc_before_call();													// save pc
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));					// mov  eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add  eax,SIMMVAL
				_push_r32(REG_EAX);													// push eax
			}
			else if (RSREG != 0)
				_push_m32bd(REG_ESI, REGDISP(RSREG));								// push [rsreg]
			else
				_push_imm(SIMMVAL);													// push SIMMVAL
			_call(mips3.read_and_translate_byte_signed);							// call read_and_translate_byte_signed
			_add_r32_imm(REG_ESP, 4);												// add  esp,4
			if (RTREG != 0)
			{
				_cdq();																// cdq
				_mov_m64bd_r64(REG_ESI, REGDISP(RTREG), REG_EDX, REG_EAX);			// mov  [rtreg],edx:eax
			}
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov  ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x21:	/* LH */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov  [mips3_icount],ebp
			_save_pc_before_call();													// save pc
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));					// mov  eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add  eax,SIMMVAL
				_push_r32(REG_EAX);													// push eax
			}
			else if (RSREG != 0)
				_push_m32bd(REG_ESI, REGDISP(RSREG));								// push [rsreg]
			else
				_push_imm(SIMMVAL);													// push SIMMVAL
			_call(mips3.read_and_translate_word_signed);							// call read_and_translate_word_signed
			_add_r32_imm(REG_ESP, 4);												// add  esp,4
			if (RTREG != 0)
			{
				_cdq();																// cdq
				_mov_m64bd_r64(REG_ESI, REGDISP(RTREG), REG_EDX, REG_EAX);			// mov  [rtreg],edx:eax
			}
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov  ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x22:	/* LWL */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov  [mips3_icount],ebp
			_save_pc_before_call();													// save pc
			_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));						// mov  eax,[rsreg]
			if (SIMMVAL)
				_add_r32_imm(REG_EAX, SIMMVAL);										// add  eax,SIMMVAL
			_push_r32(REG_EAX);														// push eax
			_and_r32_imm(REG_EAX, ~3);												// and  eax,~3
			_push_r32(REG_EAX);														// push eax
			_call((genf *)mips3.read_and_translate_long);							// call read_and_translate_long
			_add_r32_imm(REG_ESP, 4);												// add  esp,4
			_pop_r32(REG_ECX);														// pop  ecx

			if (RTREG != 0)
			{
				_and_r32_imm(REG_ECX, 3);											// and  ecx,3
				_shl_r32_imm(REG_ECX, 3);											// shl  ecx,3
				if (!mips3.bigendian)
					_xor_r32_imm(REG_ECX, 0x18);									// xor  ecx,0x18
				_shl_r32_cl(REG_EAX);												// shl  eax,cl
				_mov_r32_m32bd(REG_EBX, REG_ESI, REGDISP(RTREG));					// mov  ebx,[rtreg].lo
				_and_r32_m32bd(REG_EBX, REG_ECX, ldl_mask + 1);						// and  ebx,[ldl_mask + ecx + 4]
				_or_r32_r32(REG_EAX, REG_EBX);										// or   eax,ebx
				_cdq();																// cdq
				_mov_m64bd_r64(REG_ESI, REGDISP(RTREG), REG_EDX, REG_EAX);			// mov  [rtreg],edx:eax
			}
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov  ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x23:	/* LW */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov  [mips3_icount],ebp
			_save_pc_before_call();													// save pc
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));					// mov  eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add  eax,SIMMVAL
				_push_r32(REG_EAX);													// push eax
			}
			else if (RSREG != 0)
				_push_m32bd(REG_ESI, REGDISP(RSREG));								// push [rsreg]
			else
				_push_imm(SIMMVAL);													// push SIMMVAL
			_call(mips3.read_and_translate_long);									// call read_and_translate_long
			_add_r32_imm(REG_ESP, 4);												// add  esp,4
			if (RTREG != 0)
			{
				_cdq();																// cdq
				_mov_m64bd_r64(REG_ESI, REGDISP(RTREG), REG_EDX, REG_EAX);			// mov  [rtreg],edx:eax
			}
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov  ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x24:	/* LBU */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov  [mips3_icount],ebp
			_save_pc_before_call();													// save pc
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));					// mov  eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add  eax,SIMMVAL
				_push_r32(REG_EAX);													// push eax
			}
			else if (RSREG != 0)
				_push_m32bd(REG_ESI, REGDISP(RSREG));								// push [rsreg]
			else
				_push_imm(SIMMVAL);													// push SIMMVAL
			_call(mips3.read_and_translate_byte_unsigned);							// call read_and_translate_byte_unsigned
			_add_r32_imm(REG_ESP, 4);												// add  esp,4
			if (RTREG != 0)
			{
				_mov_m32bd_imm(REG_ESI, REGDISP(RTREG)+4, 0);						// mov  [rtreg].hi,0
				_mov_m32bd_r32(REG_ESI, REGDISP(RTREG), REG_EAX);					// mov  [rtreg].lo,eax
			}
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov  ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x25:	/* LHU */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov  [mips3_icount],ebp
			_save_pc_before_call();													// save pc
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));					// mov  eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add  eax,SIMMVAL
				_push_r32(REG_EAX);													// push eax
			}
			else if (RSREG != 0)
				_push_m32bd(REG_ESI, REGDISP(RSREG));								// push [rsreg]
			else
				_push_imm(SIMMVAL);													// push SIMMVAL
			_call(mips3.read_and_translate_word_unsigned);							// call read_and_translate_word_unsigned
			_add_r32_imm(REG_ESP, 4);												// add  esp,4
			if (RTREG != 0)
			{
				_mov_m32bd_imm(REG_ESI, REGDISP(RTREG)+4, 0);						// mov  [rtreg].hi,0
				_mov_m32bd_r32(REG_ESI, REGDISP(RTREG), REG_EAX);					// mov  [rtreg].lo,eax
			}
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov  ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x26:	/* LWR */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov  [mips3_icount],ebp
			_save_pc_before_call();													// save pc
			_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));						// mov  eax,[rsreg]
			if (SIMMVAL)
				_add_r32_imm(REG_EAX, SIMMVAL);										// add  eax,SIMMVAL
			_push_r32(REG_EAX);														// push eax
			_and_r32_imm(REG_EAX, ~3);												// and  eax,~3
			_push_r32(REG_EAX);														// push eax
			_call((genf *)mips3.read_and_translate_long);							// call read_and_translate_long
			_add_r32_imm(REG_ESP, 4);												// add  esp,4
			_pop_r32(REG_ECX);														// pop  ecx

			if (RTREG != 0)
			{
				_and_r32_imm(REG_ECX, 3);											// and  ecx,3
				_shl_r32_imm(REG_ECX, 3);											// shl  ecx,3
				if (mips3.bigendian)
					_xor_r32_imm(REG_ECX, 0x18);									// xor  ecx,0x18
				_shr_r32_cl(REG_EAX);												// shr  eax,cl
				_mov_r32_m32bd(REG_EBX, REG_ESI, REGDISP(RTREG));					// mov  ebx,[rtreg].lo
				_and_r32_m32bd(REG_EBX, REG_ECX, ldr_mask);							// and  ebx,[ldr_mask + ecx]
				_or_r32_r32(REG_EAX, REG_EBX);										// or   eax,ebx
				_cdq();																// cdq
				_mov_m64bd_r64(REG_ESI, REGDISP(RTREG), REG_EDX, REG_EAX);			// mov  [rtreg],edx:eax
			}
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov  ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x27:	/* LWU */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov  [mips3_icount],ebp
			_save_pc_before_call();													// save pc
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));					// mov  eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add  eax,SIMMVAL
				_push_r32(REG_EAX);													// push eax
			}
			else if (RSREG != 0)
				_push_m32bd(REG_ESI, REGDISP(RSREG));								// push [rsreg]
			else
				_push_imm(SIMMVAL);													// push SIMMVAL
			_call(mips3.read_and_translate_long);									// call read_and_translate_long
			_add_r32_imm(REG_ESP, 4);												// add  esp,4
			if (RTREG != 0)
			{
				_mov_m32bd_imm(REG_ESI, REGDISP(RTREG)+4, 0);						// mov  [rtreg].hi,0
				_mov_m32bd_r32(REG_ESI, REGDISP(RTREG), REG_EAX);					// mov  [rtreg].lo,eax
			}
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov  ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x28:	/* SB */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov  [mips3_icount],ebp
			_save_pc_before_call();													// save pc
			if (RTREG != 0)
				_push_m32bd(REG_ESI, REGDISP(RTREG));								// push dword [rtreg]
			else
				_push_imm(0);														// push 0
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));					// mov  eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add  eax,SIMMVAL
				_push_r32(REG_EAX);													// push eax
			}
			else if (RSREG != 0)
				_push_m32bd(REG_ESI, REGDISP(RSREG));								// push [rsreg]
			else
				_push_imm(SIMMVAL);													// push SIMMVAL
			_call(mips3.write_and_translate_byte);									// call writebyte
			_add_r32_imm(REG_ESP, 8);												// add  esp,8
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov  ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x29:	/* SH */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov  [mips3_icount],ebp
			_save_pc_before_call();													// save pc
			if (RTREG != 0)
				_push_m32bd(REG_ESI, REGDISP(RTREG));								// push dword [rtreg]
			else
				_push_imm(0);														// push 0
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));					// mov  eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add  eax,SIMMVAL
				_push_r32(REG_EAX);													// push eax
			}
			else if (RSREG != 0)
				_push_m32bd(REG_ESI, REGDISP(RSREG));								// push [rsreg]
			else
				_push_imm(SIMMVAL);													// push SIMMVAL
			_call(mips3.write_and_translate_word);									// call writeword
			_add_r32_imm(REG_ESP, 8);												// add  esp,8
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov  ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x2a:	/* SWL */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov  [mips3_icount],ebp
			_save_pc_before_call();													// save pc
			_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));						// mov  eax,[rsreg]
			if (SIMMVAL)
				_add_r32_imm(REG_EAX, SIMMVAL);										// add  eax,SIMMVAL
			_push_r32(REG_EAX);														// push eax
			_and_r32_imm(REG_EAX, ~3);												// and  eax,~3
			_push_r32(REG_EAX);														// push eax
			_call((genf *)mips3.read_and_translate_long);							// call read_and_translate_long
			_mov_r32_m32bd(REG_ECX, REG_ESP, 4);									// mov  ecx,[esp+4]

			_and_r32_imm(REG_ECX, 3);												// and  ecx,3
			_shl_r32_imm(REG_ECX, 3);												// shl  ecx,3
			if (!mips3.bigendian)
				_xor_r32_imm(REG_ECX, 0x18);										// xor  ecx,0x18

			_and_r32_m32bd(REG_EAX, REG_ECX, sdl_mask);								// and  eax,[sdl_mask + ecx]

			if (RTREG != 0)
			{
				_mov_r32_m32bd(REG_EDX, REG_ESI, REGDISP(RTREG));					// mov  edx,[rtreg]
				_shr_r32_cl(REG_EDX);												// shr  edx,cl
				_or_r32_r32(REG_EAX, REG_EDX);										// or   eax,edx
			}

			_push_r32(REG_EAX);														// push eax
			_push_r32(REG_EBX);														// push ebx
			_call((genf *)mips3.write_back_long);									// call writelong
			_add_r32_imm(REG_ESP, 16);												// add  esp,16

			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov  ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x2b:	/* SW */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov  [mips3_icount],ebp
			_save_pc_before_call();													// save pc
			if (RTREG != 0)
				_push_m32bd(REG_ESI, REGDISP(RTREG));								// push dword [rtreg]
			else
				_push_imm(0);														// push 0
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));					// mov  eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add  eax,SIMMVAL
				_push_r32(REG_EAX);													// push eax
			}
			else if (RSREG != 0)
				_push_m32bd(REG_ESI, REGDISP(RSREG));								// push [rsreg]
			else
				_push_imm(SIMMVAL);													// push SIMMVAL
			_call(mips3.write_and_translate_long);									// call write_and_translate_long
			_add_r32_imm(REG_ESP, 8);												// add  esp,8
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov  ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x2c:	/* SDL */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov  [mips3_icount],ebp
			_save_pc_before_call();													// save pc
			_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));						// mov  eax,[rsreg]
			if (SIMMVAL)
				_add_r32_imm(REG_EAX, SIMMVAL);										// add  eax,SIMMVAL
			_push_r32(REG_EAX);														// push eax
			_and_r32_imm(REG_EAX, ~7);												// and  eax,~7
			_push_r32(REG_EAX);														// push eax
			_call((genf *)mips3.read_and_translate_double);							// call read_and_translate_double
			_add_r32_imm(REG_ESP, 4);												// add  esp,4
			_pop_r32(REG_ECX);														// pop  ecx

			_and_r32_imm(REG_ECX, 7);												// and  ecx,7
			_shl_r32_imm(REG_ECX, 3);												// shl  ecx,3
			if (!mips3.bigendian)
				_xor_r32_imm(REG_ECX, 0x38);										// xor  ecx,0x38

			_and_r32_m32bd(REG_EAX, REG_ECX, sdl_mask + 1);							// and  eax,[sdl_mask + ecx + 4]
			_and_r32_m32bd(REG_EDX, REG_ECX, sdl_mask);								// and  eax,[sdl_mask + ecx]

			if (RTREG != 0)
			{
				_push_r32(REG_EBX);													// push ebx
				_push_r32(REG_ESI);													// push esi
				_test_r32_imm(REG_ECX, 0x20);										// test ecx,0x20
				_mov_r64_m64abs(REG_ESI, REG_EBX, &mips3.r[RTREG]);					// mov  esi:ebx,[rtreg]
				_jcc_short_link(COND_Z, &link1);									// jz   skip
				_mov_r32_r32(REG_EBX, REG_ESI);										// mov  ebx,esi
				_xor_r32_r32(REG_ESI, REG_ESI);										// xor  esi,esi
				_resolve_link(&link1);												// skip:
				_shrd_r32_r32_cl(REG_EBX, REG_ESI);									// shrd ebx,esi,cl
				_shr_r32_cl(REG_ESI);												// shr  esi,cl
				_or_r32_r32(REG_EAX, REG_EBX);										// or   eax,ebx
				_or_r32_r32(REG_EDX, REG_ESI);										// or   edx,esi
				_pop_r32(REG_ESI);													// pop  esi
				_pop_r32(REG_EBX);													// pop  ebx
			}

			_push_r32(REG_EDX);														// push edx
			_push_r32(REG_EAX);														// push eax
			_push_r32(REG_EBX);														// push ebx
			_call((genf *)mips3.write_back_double);									// call write_back_double
			_add_r32_imm(REG_ESP, 12);												// add  esp,12

			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov  ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x2d:	/* SDR */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov  [mips3_icount],ebp
			_save_pc_before_call();													// save pc
			_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));						// mov  eax,[rsreg]
			if (SIMMVAL)
				_add_r32_imm(REG_EAX, SIMMVAL);										// add  eax,SIMMVAL
			_push_r32(REG_EAX);														// push eax
			_and_r32_imm(REG_EAX, ~7);												// and  eax,~7
			_push_r32(REG_EAX);														// push eax
			_call((genf *)mips3.read_and_translate_double);							// call read_and_translate_double
			_add_r32_imm(REG_ESP, 4);												// add  esp,4
			_pop_r32(REG_ECX);														// pop  ecx

			_and_r32_imm(REG_ECX, 7);												// and  ecx,7
			_shl_r32_imm(REG_ECX, 3);												// shl  ecx,3
			if (mips3.bigendian)
				_xor_r32_imm(REG_ECX, 0x38);										// xor  ecx,0x38

			_and_r32_m32bd(REG_EAX, REG_ECX, sdr_mask + 1);							// and  eax,[sdr_mask + ecx + 4]
			_and_r32_m32bd(REG_EDX, REG_ECX, sdr_mask);								// and  eax,[sdr_mask + ecx]

			if (RTREG != 0)
			{
				_push_r32(REG_EBX);													// push ebx
				_push_r32(REG_ESI);													// push esi
				_test_r32_imm(REG_ECX, 0x20);										// test ecx,0x20
				_mov_r64_m64abs(REG_ESI, REG_EBX, &mips3.r[RTREG]);					// mov  esi:ebx,[rtreg]
				_jcc_short_link(COND_Z, &link1);									// jz   skip
				_mov_r32_r32(REG_ESI, REG_EBX);										// mov  esi,ebx
				_xor_r32_r32(REG_EBX, REG_EBX);										// xor  ebx,ebx
				_resolve_link(&link1);												// skip:
				_shld_r32_r32_cl(REG_ESI, REG_EBX);									// shld esi,ebx,cl
				_shl_r32_cl(REG_EBX);												// shl  ebx,cl
				_or_r32_r32(REG_EAX, REG_EBX);										// or   eax,ebx
				_or_r32_r32(REG_EDX, REG_ESI);										// or   edx,esi
				_pop_r32(REG_ESI);													// pop  esi
				_pop_r32(REG_EBX);													// pop  ebx
			}

			_push_r32(REG_EDX);														// push edx
			_push_r32(REG_EAX);														// push eax
			_push_r32(REG_EBX);														// push ebx
			_call((genf *)mips3.write_back_double);									// call write_back_double
			_add_r32_imm(REG_ESP, 12);												// add  esp,12

			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov  ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x2e:	/* SWR */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov  [mips3_icount],ebp
			_save_pc_before_call();													// save pc
			_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));						// mov  eax,[rsreg]
			if (SIMMVAL)
				_add_r32_imm(REG_EAX, SIMMVAL);										// add  eax,SIMMVAL
			_push_r32(REG_EAX);														// push eax
			_and_r32_imm(REG_EAX, ~3);												// and  eax,~3
			_push_r32(REG_EAX);														// push eax
			_call((genf *)mips3.read_and_translate_long);							// call read_and_translate_long
			_mov_r32_m32bd(REG_ECX, REG_ESP, 4);									// mov  ecx,[esp+4]

			_and_r32_imm(REG_ECX, 3);												// and  ecx,3
			_shl_r32_imm(REG_ECX, 3);												// shl  ecx,3
			if (mips3.bigendian)
				_xor_r32_imm(REG_ECX, 0x18);										// xor  ecx,0x18

			_and_r32_m32bd(REG_EAX, REG_ECX, sdr_mask + 1);							// and  eax,[sdr_mask + ecx + 4]

			if (RTREG != 0)
			{
				_mov_r32_m32bd(REG_EDX, REG_ESI, REGDISP(RTREG));					// mov  edx,[rtreg]
				_shl_r32_cl(REG_EDX);												// shl  edx,cl
				_or_r32_r32(REG_EAX, REG_EDX);										// or   eax,edx
			}

			_push_r32(REG_EAX);														// push eax
			_push_r32(REG_EBX);														// push ebx
			_call((genf *)mips3.write_back_long);									// call write_back_long
			_add_r32_imm(REG_ESP, 16);												// add  esp,16

			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov  ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x2f:	/* CACHE */
			return RECOMPILE_SUCCESSFUL_CP(1,4);

//      case 0x30:  /* LL */        logerror("mips3 Unhandled op: LL\n");                                   break;

		case 0x31:	/* LWC1 */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov  [mips3_icount],ebp
			_save_pc_before_call();													// save pc
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));					// mov  eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add  eax,SIMMVAL
				_push_r32(REG_EAX);													// push eax
			}
			else if (RSREG != 0)
				_push_m32bd(REG_ESI, REGDISP(RSREG));								// push [rsreg]
			else
				_push_imm(SIMMVAL);													// push SIMMVAL
			_call(mips3.read_and_translate_long);									// call read_and_translate_long
			_add_r32_imm(REG_ESP, 4);												// add  esp,4
			_mov_m32abs_r32(FPR32(RTREG), REG_EAX);									// mov  [rtreg],eax
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov  ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x32:	/* LWC2 */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov  [mips3_icount],ebp
			_save_pc_before_call();													// save pc
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));					// mov  eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add  eax,SIMMVAL
				_push_r32(REG_EAX);													// push eax
			}
			else if (RSREG != 0)
				_push_m32bd(REG_ESI, REGDISP(RSREG));								// push [rsreg]
			else
				_push_imm(SIMMVAL);													// push SIMMVAL
			_call(mips3.read_and_translate_long);									// call read_and_translate_long
			_add_r32_imm(REG_ESP, 4);												// add  esp,4
			_mov_m32abs_r32(&mips3.cpr[2][RTREG], REG_EAX);							// mov  [rtreg],eax
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov  ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x33:	/* PREF */
			if (mips3.is_mips4)
				return RECOMPILE_SUCCESSFUL_CP(1,4);
			else
			{
				_jmp((void *)mips3.generate_invalidop_exception);					// jmp  generate_invalidop_exception
				return RECOMPILE_SUCCESSFUL | RECOMPILE_MAY_CAUSE_EXCEPTION | RECOMPILE_END_OF_STRING;
			}

//      case 0x34:  /* LLD */       logerror("mips3 Unhandled op: LLD\n");                                  break;

		case 0x35:	/* LDC1 */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov  [mips3_icount],ebp
			_save_pc_before_call();													// save pc
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));					// mov  eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add  eax,SIMMVAL
				_push_r32(REG_EAX);													// push eax
			}
			else if (RSREG != 0)
				_push_m32bd(REG_ESI, REGDISP(RSREG));								// push [rsreg]
			else
				_push_imm(SIMMVAL);													// push SIMMVAL
			_call((genf *)mips3.read_and_translate_double);							// call read_and_translate_double
			_mov_m32abs_r32(LO(FPR64(RTREG)), REG_EAX);								// mov  [rtreg].lo,eax
			_mov_m32abs_r32(HI(FPR64(RTREG)), REG_EDX);								// mov  [rtreg].hi,edx
			_add_r32_imm(REG_ESP, 4);												// add  esp,4
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov  ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x36:	/* LDC2 */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov  [mips3_icount],ebp
			_save_pc_before_call();													// save pc
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));					// mov  eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add  eax,SIMMVAL
				_push_r32(REG_EAX);													// push eax
			}
			else if (RSREG != 0)
				_push_m32bd(REG_ESI, REGDISP(RSREG));								// push [rsreg]
			else
				_push_imm(SIMMVAL);													// push SIMMVAL
			_call((genf *)mips3.read_and_translate_double);							// call read_and_translate_long
			_mov_m32abs_r32(LO(&mips3.cpr[2][RTREG]), REG_EAX);						// mov  [rtreg].lo,eax
			_mov_m32abs_r32(HI(&mips3.cpr[2][RTREG]), REG_EDX);						// mov  [rtreg].hi,edx
			_add_r32_imm(REG_ESP, 4);												// add  esp,4
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov  ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x37:	/* LD */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov  [mips3_icount],ebp
			_save_pc_before_call();													// save pc
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));					// mov  eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add  eax,SIMMVAL
				_push_r32(REG_EAX);													// push eax
			}
			else if (RSREG != 0)
				_push_m32bd(REG_ESI, REGDISP(RSREG));								// push [rsreg]
			else
				_push_imm(SIMMVAL);													// push SIMMVAL
			_call((genf *)mips3.read_and_translate_double);							// call read_and_translate_double
			if (RTREG != 0)
			{
				_mov_m32bd_r32(REG_ESI, REGDISP(RTREG), REG_EAX);					// mov  [rtreg].lo,eax
				_mov_m32bd_r32(REG_ESI, REGDISP(RTREG)+4, REG_EDX);					// mov  [rtreg].hi,edx
			}

			_add_r32_imm(REG_ESP, 4);												// add  esp,4
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov  ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

//      case 0x38:  /* SC */        logerror("mips3 Unhandled op: SC\n");                                   break;

		case 0x39:	/* SWC1 */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov  [mips3_icount],ebp
			_save_pc_before_call();													// save pc
			_push_m32abs(FPR32(RTREG));												// push dword [rtreg]
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));					// mov  eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add  eax,SIMMVAL
				_push_r32(REG_EAX);													// push eax
			}
			else if (RSREG != 0)
				_push_m32bd(REG_ESI, REGDISP(RSREG));								// push [rsreg]
			else
				_push_imm(SIMMVAL);													// push SIMMVAL
			_call(mips3.write_and_translate_long);									// call write_and_translate_long
			_add_r32_imm(REG_ESP, 8);												// add  esp,8
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov  ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x3a:	/* SWC2 */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov  [mips3_icount],ebp
			_save_pc_before_call();													// save pc
			_push_m32abs(&mips3.cpr[2][RTREG]);										// push dword [rtreg]
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));					// mov  eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add  eax,SIMMVAL
				_push_r32(REG_EAX);													// push eax
			}
			else if (RSREG != 0)
				_push_m32bd(REG_ESI, REGDISP(RSREG));								// push [rsreg]
			else
				_push_imm(SIMMVAL);													// push SIMMVAL
			_call(mips3.write_and_translate_long);									// call write_and_translate_long
			_add_r32_imm(REG_ESP, 8);												// add  esp,8
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov  ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

//      case 0x3b:  /* SWC3 */      invalid_instruction(op);                                                break;
//      case 0x3c:  /* SCD */       logerror("mips3 Unhandled op: SCD\n");                                  break;

		case 0x3d:	/* SDC1 */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov  [mips3_icount],ebp
			_save_pc_before_call();													// save pc
			_push_m32abs(HI(FPR64(RTREG)));											// push dword [rtreg].hi
			_push_m32abs(LO(FPR64(RTREG)));											// push dword [rtreg].lo
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));					// mov  eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add  eax,SIMMVAL
				_push_r32(REG_EAX);													// push eax
			}
			else if (RSREG != 0)
				_push_m32bd(REG_ESI, REGDISP(RSREG));								// push [rsreg]
			else
				_push_imm(SIMMVAL);													// push SIMMVAL
			_call((genf *)mips3.write_and_translate_double);						// call write_and_translate_double
			_add_r32_imm(REG_ESP, 12);												// add  esp,12
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov  ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x3e:	/* SDC2 */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov  [mips3_icount],ebp
			_save_pc_before_call();													// save pc
			_push_m32abs(HI(&mips3.cpr[2][RTREG]));									// push dword [rtreg].hi
			_push_m32abs(LO(&mips3.cpr[2][RTREG]));									// push dword [rtreg].lo
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));					// mov  eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add  eax,SIMMVAL
				_push_r32(REG_EAX);													// push eax
			}
			else if (RSREG != 0)
				_push_m32bd(REG_ESI, REGDISP(RSREG));								// push [rsreg]
			else
				_push_imm(SIMMVAL);													// push SIMMVAL
			_call((genf *)mips3.write_and_translate_double);						// call write_and_translate_double
			_add_r32_imm(REG_ESP, 12);												// add  esp,12
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov  ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x3f:	/* SD */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov  [mips3_icount],ebp
			_save_pc_before_call();													// save pc
			if (RTREG != 0)
			{
				_push_m32bd(REG_ESI, REGDISP(RTREG)+4);								// push   dword [rtreg].hi
				_push_m32bd(REG_ESI, REGDISP(RTREG));								// push   dword [rtreg].lo
			}
			else
			{
				_push_imm(0);														// push 0
				_push_imm(0);														// push 0
			}
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));					// mov  eax,[rsreg]
				_add_r32_imm(REG_EAX, SIMMVAL);										// add  eax,SIMMVAL
				_push_r32(REG_EAX);													// push eax
			}
			else if (RSREG != 0)
				_push_m32bd(REG_ESI, REGDISP(RSREG));								// push [rsreg]
			else
				_push_imm(SIMMVAL);													// push SIMMVAL
			_call((genf *)mips3.write_and_translate_double);						// call write_and_translate_double
			_add_r32_imm(REG_ESP, 12);												// add  esp,12
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov  ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

//      default:    /* ??? */       invalid_instruction(op);                                                break;
	}
	return RECOMPILE_UNIMPLEMENTED;
}


/*------------------------------------------------------------------
    recompile_special
------------------------------------------------------------------*/

static UINT32 recompile_special1(drc_core *drc, UINT32 pc, UINT32 op)
{
	link_info link1, link2, link3;
	int cycles;

	switch (op & 63)
	{
		case 0x00:	/* SLL */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));				// mov  eax,[rtreg]
					if (SHIFT != 0)
						_shl_r32_imm(REG_EAX, SHIFT);								// shl  eax,SHIFT
					_cdq();															// cdq
					_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);		// mov  [rdreg],edx:eax
				}
				else
					_zero_m64bd(REG_ESI, REGDISP(RDREG));
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x01:	/* MOVF - R5000*/
			if (!mips3.is_mips4)
			{
				_jmp((void *)mips3.generate_invalidop_exception);					// jmp  generate_invalidop_exception
				return RECOMPILE_SUCCESSFUL | RECOMPILE_MAY_CAUSE_EXCEPTION | RECOMPILE_END_OF_STRING;
			}
			_cmp_m8abs_imm(&mips3.cf[1][(op >> 18) & 7], 0);						// cmp  [cf[x]],0
			_jcc_short_link(((op >> 16) & 1) ? COND_Z : COND_NZ, &link1);			// jz/nz skip
			_mov_r64_m64bd(REG_EDX, REG_EAX, REG_ESI, REGDISP(RSREG));				// mov  edx:eax,[rsreg]
			_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);				// mov  [rdreg],edx:eax
			_resolve_link(&link1);													// skip:
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x02:	/* SRL */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));				// mov  eax,[rtreg]
					if (SHIFT != 0)
						_shr_r32_imm(REG_EAX, SHIFT);								// shr  eax,SHIFT
					_cdq();															// cdq
					_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);		// mov  [rdreg],edx:eax
				}
				else
					_zero_m64bd(REG_ESI, REGDISP(RDREG));
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x03:	/* SRA */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));				// mov  eax,[rtreg]
					if (SHIFT != 0)
						_sar_r32_imm(REG_EAX, SHIFT);								// sar  eax,SHIFT
					_cdq();															// cdq
					_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);		// mov  [rdreg],edx:eax
				}
				else
					_zero_m64bd(REG_ESI, REGDISP(RDREG));
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x04:	/* SLLV */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));				// mov  eax,[rtreg]
					if (RSREG != 0)
					{
						_mov_r32_m32bd(REG_ECX, REG_ESI, REGDISP(RSREG));			// mov  ecx,[rsreg]
						_shl_r32_cl(REG_EAX);										// shl  eax,cl
					}
					_cdq();															// cdq
					_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);		// mov  [rdreg],edx:eax
				}
				else
					_zero_m64bd(REG_ESI, REGDISP(RDREG));
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x06:	/* SRLV */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));				// mov  eax,[rtreg]
					if (RSREG != 0)
					{
						_mov_r32_m32bd(REG_ECX, REG_ESI, REGDISP(RSREG));			// mov  ecx,[rsreg]
						_shr_r32_cl(REG_EAX);										// shr  eax,cl
					}
					_cdq();															// cdq
					_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);		// mov  [rdreg],edx:eax
				}
				else
					_zero_m64bd(REG_ESI, REGDISP(RDREG));
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x07:	/* SRAV */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));				// mov  eax,[rtreg]
					if (RSREG != 0)
					{
						_mov_r32_m32bd(REG_ECX, REG_ESI, REGDISP(RSREG));			// mov  ecx,[rsreg]
						_sar_r32_cl(REG_EAX);										// sar  eax,cl
					}
					_cdq();															// cdq
					_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);		// mov  [rdreg],edx:eax
				}
				else
					_zero_m64bd(REG_ESI, REGDISP(RDREG));
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x08:	/* JR */
			_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));						// mov  eax,[rsreg]
			_mov_m32abs_r32(&jr_temp, REG_EAX);										// mov  jr_temp,eax
			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			_mov_r32_m32abs(REG_EDI, &jr_temp);										// mov  edi,jr_temp
			return RECOMPILE_SUCCESSFUL_CP(1+cycles,0) | RECOMPILE_END_OF_STRING | RECOMPILE_ADD_DISPATCH;

		case 0x09:	/* JALR */
			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			if (RDREG != 0)
				_mov_m64bd_imm32(REG_ESI, REGDISP(RDREG), pc + 8);					// mov  [rdreg],pc + 8
			_mov_r32_m32bd(REG_EDI, REG_ESI, REGDISP(RSREG));						// mov  edi,[rsreg]
			return RECOMPILE_SUCCESSFUL_CP(1+cycles,0) | RECOMPILE_END_OF_STRING | RECOMPILE_ADD_DISPATCH;

		case 0x0a:	/* MOVZ - R5000 */
			if (!mips3.is_mips4)
			{
				_jmp((void *)mips3.generate_invalidop_exception);					// jmp  generate_invalidop_exception
				return RECOMPILE_SUCCESSFUL | RECOMPILE_MAY_CAUSE_EXCEPTION | RECOMPILE_END_OF_STRING;
			}
			if (RDREG != 0)
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));					// mov  eax,[rtreg].lo
				_or_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG)+4);					// or   eax,[rtreg].hi
				_jcc_short_link(COND_NZ, &link1);									// jnz  skip
				_mov_m64bd_m64bd(REG_ESI, REGDISP(RDREG), REG_ESI, REGDISP(RSREG));	// mov  [rdreg],[rsreg]
				_resolve_link(&link1);												// skip:
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x0b:	/* MOVN - R5000 */
			if (!mips3.is_mips4)
			{
				_jmp((void *)mips3.generate_invalidop_exception);					// jmp  generate_invalidop_exception
				return RECOMPILE_SUCCESSFUL | RECOMPILE_MAY_CAUSE_EXCEPTION | RECOMPILE_END_OF_STRING;
			}
			if (RDREG != 0)
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));					// mov  eax,[rtreg].lo
				_or_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG)+4);					// or   eax,[rtreg].hi
				_jcc_short_link(COND_Z, &link1);									// jz   skip
				_mov_m64bd_m64bd(REG_ESI, REGDISP(RDREG), REG_ESI, REGDISP(RSREG));	// mov  [rdreg],[rsreg]
				_resolve_link(&link1);												// skip:
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x0c:	/* SYSCALL */
			_jmp((void *)mips3.generate_syscall_exception);							// jmp  generate_syscall_exception
			return RECOMPILE_SUCCESSFUL | RECOMPILE_MAY_CAUSE_EXCEPTION | RECOMPILE_END_OF_STRING;

		case 0x0d:	/* BREAK */
			_jmp((void *)mips3.generate_break_exception);							// jmp  generate_break_exception
			return RECOMPILE_SUCCESSFUL | RECOMPILE_MAY_CAUSE_EXCEPTION | RECOMPILE_END_OF_STRING;

		case 0x0f:	/* SYNC */
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x10:	/* MFHI */
			if (RDREG != 0)
				_mov_m64bd_m64abs(REG_ESI, REGDISP(RDREG), &mips3.hi);				// mov  [rdreg],[hi]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x11:	/* MTHI */
			_mov_m64abs_m64bd(&mips3.hi, REG_ESI, REGDISP(RSREG));					// mov  [hi],[rsreg]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x12:	/* MFLO */
			if (RDREG != 0)
				_mov_m64bd_m64abs(REG_ESI, REGDISP(RDREG), &mips3.lo);				// mov  [rdreg],[lo]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x13:	/* MTLO */
			_mov_m64abs_m64bd(&mips3.lo, REG_ESI, REGDISP(RSREG));					// mov  [lo],[rsreg]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x14:	/* DSLLV */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					_mov_r64_m64bd(REG_EDX, REG_EAX, REG_ESI, REGDISP(RTREG));		// mov  edx:eax,[rtreg]
					if (RSREG != 0)
					{
						_mov_r32_m32bd(REG_ECX, REG_ESI, REGDISP(RSREG));			// mov  ecx,[rsreg]
						_test_r32_imm(REG_ECX, 0x20);								// test ecx,0x20
						_jcc_short_link(COND_Z, &link1);							// jz   skip
						_mov_r32_r32(REG_EDX, REG_EAX);								// mov  edx,eax
						_xor_r32_r32(REG_EAX, REG_EAX);								// xor  eax,eax
						_resolve_link(&link1);										// skip:
						_shld_r32_r32_cl(REG_EDX, REG_EAX);							// shld edx,eax,cl
						_shl_r32_cl(REG_EAX);										// shl  eax,cl
					}
					_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);		// mov  [rdreg],edx:eax
				}
				else
					_zero_m64bd(REG_ESI, REGDISP(RDREG));
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x16:	/* DSRLV */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					_mov_r64_m64bd(REG_EDX, REG_EAX, REG_ESI, REGDISP(RTREG));		// mov  edx:eax,[rtreg]
					if (RSREG != 0)
					{
						_mov_r32_m32bd(REG_ECX, REG_ESI, REGDISP(RSREG));			// mov  ecx,[rsreg]
						_test_r32_imm(REG_ECX, 0x20);								// test ecx,0x20
						_jcc_short_link(COND_Z, &link1);							// jz   skip
						_mov_r32_r32(REG_EAX, REG_EDX);								// mov  eax,edx
						_xor_r32_r32(REG_EDX, REG_EDX);								// xor  edx,edx
						_resolve_link(&link1);										// skip:
						_shrd_r32_r32_cl(REG_EAX, REG_EDX);							// shrd eax,edx,cl
						_shr_r32_cl(REG_EDX);										// shr  edx,cl
					}
					_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);		// mov  [rdreg],edx:eax
				}
				else
					_zero_m64bd(REG_ESI, REGDISP(RDREG));
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x17:	/* DSRAV */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					_mov_r64_m64bd(REG_EDX, REG_EAX, REG_ESI, REGDISP(RTREG));		// mov  edx:eax,[rtreg]
					if (RSREG != 0)
					{
						_mov_r32_m32bd(REG_ECX, REG_ESI, REGDISP(RSREG));			// mov  ecx,[rsreg]
						_test_r32_imm(REG_ECX, 0x20);								// test ecx,0x20
						_jcc_short_link(COND_Z, &link1);							// jz   skip
						_mov_r32_r32(REG_EAX, REG_EDX);								// mov  eax,edx
						_cdq();														// cdq
						_resolve_link(&link1);										// skip:
						_shrd_r32_r32_cl(REG_EAX, REG_EDX);							// shrd eax,edx,cl
						_sar_r32_cl(REG_EDX);										// sar  edx,cl
					}
					_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);		// mov  [rdreg],edx:eax
				}
				else
					_zero_m64bd(REG_ESI, REGDISP(RDREG));
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x18:	/* MULT */
			_mov_r32_m32bd(REG_ECX, REG_ESI, REGDISP(RTREG));						// mov  ecx,[rtreg].lo
			_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));						// mov  eax,[rsreg].lo
			_imul_r32(REG_ECX);														// imul ecx
			_push_r32(REG_EDX);														// push edx
			_cdq();																	// cdq
			_mov_m64abs_r64(&mips3.lo, REG_EDX, REG_EAX);							// mov  [lo],edx:eax
			_pop_r32(REG_EAX);														// pop  eax
			_cdq();																	// cdq
			_mov_m64abs_r64(&mips3.hi, REG_EDX, REG_EAX);							// mov  [hi],edx:eax
			return RECOMPILE_SUCCESSFUL_CP(4,4);

		case 0x19:	/* MULTU */
			_mov_r32_m32bd(REG_ECX, REG_ESI, REGDISP(RTREG));						// mov  ecx,[rtreg].lo
			_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));						// mov  eax,[rsreg].lo
			_mul_r32(REG_ECX);														// mul  ecx
			_push_r32(REG_EDX);														// push edx
			_cdq();																	// cdq
			_mov_m64abs_r64(&mips3.lo, REG_EDX, REG_EAX);							// mov  [lo],edx:eax
			_pop_r32(REG_EAX);														// pop  eax
			_cdq();																	// cdq
			_mov_m64abs_r64(&mips3.hi, REG_EDX, REG_EAX);							// mov  [hi],edx:eax
			return RECOMPILE_SUCCESSFUL_CP(4,4);

		case 0x1a:	/* DIV */
			if (RTREG != 0)
			{
				_mov_r32_m32bd(REG_ECX, REG_ESI, REGDISP(RTREG));					// mov  ecx,[rtreg].lo
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));					// mov  eax,[rsreg].lo
				_cdq();																// cdq
				_cmp_r32_imm(REG_ECX, 0);											// cmp  ecx,0
				_jcc_short_link(COND_E, &link1);									// je   skip
				_idiv_r32(REG_ECX);													// idiv ecx
				_push_r32(REG_EDX);													// push edx
				_cdq();																// cdq
				_mov_m64abs_r64(&mips3.lo, REG_EDX, REG_EAX);						// mov  [lo],edx:eax
				_pop_r32(REG_EAX);													// pop  eax
				_cdq();																// cdq
				_mov_m64abs_r64(&mips3.hi, REG_EDX, REG_EAX);						// mov  [hi],edx:eax
				_resolve_link(&link1);												// skip:
			}
			return RECOMPILE_SUCCESSFUL_CP(36,4);

		case 0x1b:	/* DIVU */
			if (RTREG != 0)
			{
				_mov_r32_m32bd(REG_ECX, REG_ESI, REGDISP(RTREG));					// mov  ecx,[rtreg].lo
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));					// mov  eax,[rsreg].lo
				_xor_r32_r32(REG_EDX, REG_EDX);										// xor  edx,edx
				_cmp_r32_imm(REG_ECX, 0);											// cmp  ecx,0
				_jcc_short_link(COND_E, &link1);									// je   skip
				_div_r32(REG_ECX);													// div  ecx
				_push_r32(REG_EDX);													// push edx
				_cdq();																// cdq
				_mov_m64abs_r64(&mips3.lo, REG_EDX, REG_EAX);						// mov  [lo],edx:eax
				_pop_r32(REG_EAX);													// pop  eax
				_cdq();																// cdq
				_mov_m64abs_r64(&mips3.hi, REG_EDX, REG_EAX);						// mov  [hi],edx:eax
				_resolve_link(&link1);												// skip:
			}
			return RECOMPILE_SUCCESSFUL_CP(36,4);

		case 0x1c:	/* DMULT */
			_mov_r64_m64bd(REG_EDX, REG_EAX, REG_ESI, REGDISP(RSREG));				// mov  edx:eax,[rsreg]
			_cmp_r32_imm(REG_EDX, 0);												// cmp  edx,0
			_jcc_short_link(COND_GE, &link1);										// jge  skip1
			_mov_r32_r32(REG_ECX, REG_EDX);											// mov  ecx,edx
			_xor_r32_r32(REG_EDX, REG_EDX);											// xor  edx,edx
			_neg_r32(REG_EAX);														// neg  eax
			_sbb_r32_r32(REG_EDX, REG_ECX);											// sbb  edx,ecx
			_resolve_link(&link1);													// skip1:
			_mov_m64abs_r64(&dmult_temp1, REG_EDX, REG_EAX);						// mov  [dmult_temp1],edx:eax

			_mov_r64_m64bd(REG_EDX, REG_EAX, REG_ESI, REGDISP(RTREG));				// mov  edx:eax,[rtreg]
			_cmp_r32_imm(REG_EDX, 0);												// cmp  edx,0
			_jcc_short_link(COND_GE, &link2);										// jge  skip2
			_mov_r32_r32(REG_ECX, REG_EDX);											// mov  ecx,edx
			_xor_r32_r32(REG_EDX, REG_EDX);											// xor  edx,edx
			_neg_r32(REG_EAX);														// neg  eax
			_sbb_r32_r32(REG_EDX, REG_ECX);											// sbb  edx,ecx
			_resolve_link(&link2);													// skip2:
			_mov_m64abs_r64(&dmult_temp2, REG_EDX, REG_EAX);						// mov  [dmult_temp2],edx:eax

			_mov_r32_m32abs(REG_EAX, LO(&dmult_temp1));								// mov  eax,[dmult_temp1].lo
			_mul_m32abs(LO(&dmult_temp2));											// mul  [dmult_temp2].lo
			_mov_r32_r32(REG_ECX, REG_EDX);											// mov  ecx,edx
			_xor_r32_r32(REG_EBX, REG_EBX);											// xor  ebx,ebx
			_mov_m32abs_r32(LO(&mips3.lo), REG_EAX);								// mov  [lo].lo,eax

			_mov_r32_m32abs(REG_EAX, HI(&dmult_temp1));								// mov  eax,[dmult_temp1].hi
			_mul_m32abs(LO(&dmult_temp2));											// mul  [dmult_temp2].lo
			_add_r32_r32(REG_ECX, REG_EAX);											// add  ecx,eax
			_adc_r32_r32(REG_EBX, REG_EDX);											// adc  ebx,edx

			_mov_r32_m32abs(REG_EAX, LO(&dmult_temp1));								// mov  eax,[dmult_temp1].lo
			_mul_m32abs(HI(&dmult_temp2));											// mul  [dmult_temp2].hi
			_add_r32_r32(REG_ECX, REG_EAX);											// add  ecx,eax
			_adc_r32_r32(REG_EBX, REG_EDX);											// adc  ebx,edx
			_mov_m32abs_r32(HI(&mips3.lo), REG_ECX);								// mov  [lo].hi,ecx

			_mov_r32_m32abs(REG_EAX, HI(&dmult_temp1));								// mov  eax,[dmult_temp1].hi
			_mul_m32abs(HI(&dmult_temp2));											// mul  [dmult_temp2].hi
			_add_r32_r32(REG_EBX, REG_EAX);											// add  ebx,eax
			_adc_r32_imm(REG_EDX, 0);												// adc  edx,0
			_mov_m32abs_r32(LO(&mips3.hi), REG_EBX);								// mov  [hi].lo,ebx
			_mov_m32abs_r32(HI(&mips3.hi), REG_EDX);								// mov  [hi].hi,edx

			_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG)+4);						// mov  eax,[rsreg].hi
			_xor_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG)+4);						// xor  eax,[rtreg].hi
			_jcc_short_link(COND_NS, &link3);										// jns  noflip
			_xor_r32_r32(REG_EAX, REG_EAX);											// xor  eax,eax
			_xor_r32_r32(REG_EBX, REG_EBX);											// xor  ebx,ebx
			_xor_r32_r32(REG_ECX, REG_ECX);											// xor  ecx,ecx
			_xor_r32_r32(REG_EDX, REG_EDX);											// xor  edx,edx
			_sub_r32_m32abs(REG_EAX, LO(&mips3.lo));								// sub  eax,[lo].lo
			_sbb_r32_m32abs(REG_EBX, HI(&mips3.lo));								// sbb  ebx,[lo].hi
			_sbb_r32_m32abs(REG_ECX, LO(&mips3.hi));								// sbb  ecx,[hi].lo
			_sbb_r32_m32abs(REG_EDX, HI(&mips3.hi));								// sbb  edx,[hi].hi
			_mov_m64abs_r64(&mips3.lo, REG_EBX, REG_EAX);							// mov  [lo],ebx:eax
			_mov_m64abs_r64(&mips3.hi, REG_EDX, REG_ECX);							// mov  [lo],edx:ecx
			_resolve_link(&link3);													// noflip:
			return RECOMPILE_SUCCESSFUL_CP(8,4);

		case 0x1d:	/* DMULTU */
			_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));						// mov  eax,[rsreg].lo
			_mul_m32bd(REG_ESI, REGDISP(RTREG));									// mul  [rtreg].lo
			_mov_r32_r32(REG_ECX, REG_EDX);											// mov  ecx,edx
			_xor_r32_r32(REG_EBX, REG_EBX);											// xor  ebx,ebx
			_mov_m32abs_r32(LO(&mips3.lo), REG_EAX);								// mov  [lo].lo,eax

			_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG)+4);						// mov  eax,[rsreg].hi
			_mul_m32bd(REG_ESI, REGDISP(RTREG));									// mul  [rtreg].lo
			_add_r32_r32(REG_ECX, REG_EAX);											// add  ecx,eax
			_adc_r32_r32(REG_EBX, REG_EDX);											// adc  ebx,edx

			_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));						// mov  eax,[rsreg].lo
			_mul_m32bd(REG_ESI, REGDISP(RTREG)+4);									// mul  [rtreg].hi
			_add_r32_r32(REG_ECX, REG_EAX);											// add  ecx,eax
			_adc_r32_r32(REG_EBX, REG_EDX);											// adc  ebx,edx
			_mov_m32abs_r32(HI(&mips3.lo), REG_ECX);								// mov  [lo].hi,ecx

			_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG)+4);						// mov  eax,[rsreg].hi
			_mul_m32bd(REG_ESI, REGDISP(RTREG)+4);									// mul  [rtreg].hi
			_add_r32_r32(REG_EBX, REG_EAX);											// add  ebx,eax
			_adc_r32_imm(REG_EDX, 0);												// adc  edx,0
			_mov_m32abs_r32(LO(&mips3.hi), REG_EBX);								// mov  [hi].lo,ebx
			_mov_m32abs_r32(HI(&mips3.hi), REG_EDX);								// mov  [hi].hi,edx
			return RECOMPILE_SUCCESSFUL_CP(8,4);

		case 0x1e:	/* DDIV */
			_push_imm(&mips3.r[RTREG]);												// push [rtreg]
			_push_imm(&mips3.r[RSREG]);												// push [rsreg]
			_call((genf *)ddiv);													// call ddiv
			_add_r32_imm(REG_ESP, 8);												// add  esp,8
			return RECOMPILE_SUCCESSFUL_CP(68,4);

		case 0x1f:	/* DDIVU */
			_push_imm(&mips3.r[RTREG]);												// push [rtreg]
			_push_imm(&mips3.r[RSREG]);												// push [rsreg]
			_call((genf *)ddivu);													// call ddivu
			_add_r32_imm(REG_ESP, 8);												// add  esp,8
			return RECOMPILE_SUCCESSFUL_CP(68,4);
	}

	_jmp((void *)mips3.generate_invalidop_exception);								// jmp  generate_invalidop_exception
	return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;
}

static UINT32 recompile_special2(drc_core *drc, UINT32 pc, UINT32 op)
{
	link_info link1, link2, link3;

	switch (op & 63)
	{
		case 0x20:	/* ADD */
			if (RSREG != 0 && RTREG != 0)
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));					// mov  eax,[rsreg]
				_add_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));					// add  eax,[rtreg]
				_jcc(COND_O, mips3.generate_overflow_exception);					// jo   generate_overflow_exception
				if (RDREG != 0)
				{
					_cdq();															// cdq
					_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);		// mov  [rdreg],edx:eax
				}
			}
			else if (RDREG != 0)
			{
				if (RSREG != 0)
				{
					_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));				// mov  eax,[rsreg]
					_cdq();															// cdq
					_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);		// mov  [rdreg],edx:eax
				}
				else if (RTREG != 0)
				{
					_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));				// mov  eax,[rtreg]
					_cdq();															// cdq
					_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);		// mov  [rdreg],edx:eax
				}
				else
					_zero_m64bd(REG_ESI, REGDISP(RDREG));
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x21:	/* ADDU */
			if (RDREG != 0)
			{
				if (RSREG != 0 && RTREG != 0)
				{
					_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));				// mov  eax,[rsreg]
					_add_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));				// add  eax,[rtreg]
					_cdq();															// cdq
					_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);		// mov  [rdreg],edx:eax
				}
				else if (RSREG != 0)
				{
					_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));				// mov  eax,[rsreg]
					_cdq();															// cdq
					_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);		// mov  [rdreg],edx:eax
				}
				else if (RTREG != 0)
				{
					_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));				// mov  eax,[rtreg]
					_cdq();															// cdq
					_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);		// mov  [rdreg],edx:eax
				}
				else
					_zero_m64bd(REG_ESI, REGDISP(RDREG));
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x22:	/* SUB */
			if (RSREG != 0 && RTREG != 0)
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));					// mov  eax,[rsreg]
				_sub_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));					// sub  eax,[rtreg]
				_jcc(COND_O, mips3.generate_overflow_exception);					// jo   generate_overflow_exception
				if (RDREG != 0)
				{
					_cdq();															// cdq
					_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);		// mov  [rdreg],edx:eax
				}
			}
			else if (RDREG != 0)
			{
				if (RSREG != 0)
				{
					_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));				// mov  eax,[rsreg]
					_cdq();															// cdq
					_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);		// mov  [rdreg],edx:eax
				}
				else if (RTREG != 0)
				{
					_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));				// mov  eax,[rtreg]
					_neg_r32(REG_EAX);												// neg  eax
					_cdq();															// cdq
					_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);		// mov  [rdreg],edx:eax
				}
				else
					_zero_m64bd(REG_ESI, REGDISP(RDREG));
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x23:	/* SUBU */
			if (RDREG != 0)
			{
				if (RSREG != 0 && RTREG != 0)
				{
					_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));				// mov  eax,[rsreg]
					_sub_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));				// sub  eax,[rtreg]
					_cdq();															// cdq
					_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);		// mov  [rdreg],edx:eax
				}
				else if (RSREG != 0)
				{
					_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));				// mov  eax,[rsreg]
					_cdq();															// cdq
					_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);		// mov  [rdreg],edx:eax
				}
				else if (RTREG != 0)
				{
					_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));				// mov  eax,[rtreg]
					_neg_r32(REG_EAX);												// neg  eax
					_cdq();															// cdq
					_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);		// mov  [rdreg],edx:eax
				}
				else
					_zero_m64bd(REG_ESI, REGDISP(RDREG));
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x24:	/* AND */
			if (RDREG != 0)
			{
				if (RSREG != 0 && RTREG != 0)
				{
					if (USE_SSE)
					{
						_movsd_r128_m64bd(REG_XMM0, REG_ESI, REGDISP(RSREG));		// movsd xmm0,[rsreg]
						_movsd_r128_m64bd(REG_XMM1, REG_ESI, REGDISP(RTREG));		// movsd xmm1,[rtreg]
						_pand_r128_r128(REG_XMM0, REG_XMM1);						// pand xmm0,xmm1
						_movsd_m64bd_r128(REG_ESI, REGDISP(RDREG), REG_XMM0);		// mov  [rdreg],xmm0
					}
					else
					{
						_mov_r64_m64bd(REG_EDX, REG_EAX, REG_ESI, REGDISP(RSREG));	// mov  edx:eax,[rsreg]
						_and_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));			// and  eax,[rtreg].lo
						_and_r32_m32bd(REG_EDX, REG_ESI, REGDISP(RTREG)+4);			// and  edx,[rtreg].hi
						_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);	// mov  [rdreg],edx:eax
					}
				}
				else
					_zero_m64bd(REG_ESI, REGDISP(RDREG));
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x25:	/* OR */
			if (RDREG != 0)
			{
				if (RSREG != 0 && RTREG != 0)
				{
					if (USE_SSE)
					{
						_movsd_r128_m64bd(REG_XMM0, REG_ESI, REGDISP(RSREG));		// movsd xmm0,[rsreg]
						_movsd_r128_m64bd(REG_XMM1, REG_ESI, REGDISP(RTREG));		// movsd xmm1,[rtreg]
						_por_r128_r128(REG_XMM0, REG_XMM1);							// por  xmm0,xmm1
						_movsd_m64bd_r128(REG_ESI, REGDISP(RDREG), REG_XMM0);		// mov  [rdreg],xmm0
					}
					else
					{
						_mov_r64_m64bd(REG_EDX, REG_EAX, REG_ESI, REGDISP(RSREG));	// mov  edx:eax,[rsreg]
						_or_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));			// or   eax,[rtreg].lo
						_or_r32_m32bd(REG_EDX, REG_ESI, REGDISP(RTREG)+4);			// or   edx,[rtreg].hi
						_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);	// mov  [rdreg],edx:eax
					}
				}
				else if (RSREG != 0)
					_mov_m64bd_m64bd(REG_ESI, REGDISP(RDREG), REG_ESI, REGDISP(RSREG));	// mov  [rdreg],[rsreg]
				else if (RTREG != 0)
					_mov_m64bd_m64bd(REG_ESI, REGDISP(RDREG), REG_ESI, REGDISP(RTREG));	// mov  [rdreg],[rtreg]
				else
					_zero_m64bd(REG_ESI, REGDISP(RDREG));
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x26:	/* XOR */
			if (RDREG != 0)
			{
				if (RSREG != 0 && RTREG != 0)
				{
					if (USE_SSE)
					{
						_movsd_r128_m64bd(REG_XMM0, REG_ESI, REGDISP(RSREG));		// movsd xmm0,[rsreg]
						_movsd_r128_m64bd(REG_XMM1, REG_ESI, REGDISP(RTREG));		// movsd xmm1,[rtreg]
						_pxor_r128_r128(REG_XMM0, REG_XMM1);						// pxor xmm0,xmm1
						_movsd_m64bd_r128(REG_ESI, REGDISP(RDREG), REG_XMM0);		// mov  [rdreg],xmm0
					}
					else
					{
						_mov_r64_m64bd(REG_EDX, REG_EAX, REG_ESI, REGDISP(RSREG));	// mov  edx:eax,[rsreg]
						_xor_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));			// xor  eax,[rtreg].lo
						_xor_r32_m32bd(REG_EDX, REG_ESI, REGDISP(RTREG)+4);			// xor  edx,[rtreg].hi
						_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);	// mov  [rdreg],edx:eax
					}
				}
				else if (RSREG != 0)
					_mov_m64bd_m64bd(REG_ESI, REGDISP(RDREG), REG_ESI, REGDISP(RSREG));	// mov  [rdreg],[rsreg]
				else if (RTREG != 0)
					_mov_m64bd_m64bd(REG_ESI, REGDISP(RDREG), REG_ESI, REGDISP(RTREG));	// mov  [rdreg],[rtreg]
				else
					_zero_m64bd(REG_ESI, REGDISP(RDREG));
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x27:	/* NOR */
			if (RDREG != 0)
			{
				if (RSREG != 0 && RTREG != 0)
				{
					_mov_r64_m64bd(REG_EDX, REG_EAX, REG_ESI, REGDISP(RSREG));		// mov  edx:eax,[rsreg]
					_or_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));				// or   eax,[rtreg].lo
					_or_r32_m32bd(REG_EDX, REG_ESI, REGDISP(RTREG)+4);				// or   edx,[rtreg].hi
					_not_r32(REG_EDX);												// not  edx
					_not_r32(REG_EAX);												// not  eax
					_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);		// mov  [rdreg],edx:eax
				}
				else if (RSREG != 0)
				{
					_mov_r64_m64bd(REG_EDX, REG_EAX, REG_ESI, REGDISP(RSREG));		// mov  edx:eax,[rsreg]
					_not_r32(REG_EDX);												// not  edx
					_not_r32(REG_EAX);												// not  eax
					_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);		// mov  [rdreg],edx:eax
				}
				else if (RTREG != 0)
				{
					_mov_r64_m64bd(REG_EDX, REG_EAX, REG_ESI, REGDISP(RTREG));		// mov  edx:eax,[rtreg]
					_not_r32(REG_EDX);												// not  edx
					_not_r32(REG_EAX);												// not  eax
					_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);		// mov  [rdreg],edx:eax
				}
				else
				{
					_mov_m32bd_imm(REG_ESI, REGDISP(RDREG), ~0);					// mov  [rtreg].lo,~0
					_mov_m32bd_imm(REG_ESI, REGDISP(RDREG)+4, ~0);					// mov  [rtreg].hi,~0
				}
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x2a:	/* SLT */
			if (RDREG != 0)
			{
				if (RSREG != 0)
					_mov_r64_m64bd(REG_EDX, REG_EAX, REG_ESI, REGDISP(RSREG));		// mov  edx:eax,[rsreg]
				else
				{
					_xor_r32_r32(REG_EDX, REG_EDX);									// xor  edx,edx
					_xor_r32_r32(REG_EAX, REG_EAX);									// xor  eax,eax
				}
				if (RTREG != 0)
				{
					_sub_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));				// sub  eax,[rtreg].lo
					_sbb_r32_m32bd(REG_EDX, REG_ESI, REGDISP(RTREG)+4);				// sbb  edx,[rtreg].lo
				}
				_shr_r32_imm(REG_EDX, 31);											// shr  edx,31
				_mov_m32bd_r32(REG_ESI, REGDISP(RDREG), REG_EDX);					// mov  [rdreg].lo,edx
				_mov_m32bd_imm(REG_ESI, REGDISP(RDREG)+4, 0);						// mov  [rdreg].hi,0
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x2b:	/* SLTU */
			if (RDREG != 0)
			{
				_xor_r32_r32(REG_ECX, REG_ECX);										// xor  ecx,ecx
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG)+4);					// mov  eax,[rsreg].hi
				_cmp_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG)+4);					// cmp  eax,[rtreg].hi
				_jcc_short_link(COND_B, &link1);									// jb   setit
				_jcc_short_link(COND_A, &link2);									// ja   skipit
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));					// mov  eax,[rsreg].lo
				_cmp_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));					// cmp  eax,[rtreg].lo
				_jcc_short_link(COND_AE, &link3);									// jae  skipit
				_resolve_link(&link1);												// setit:
				_add_r32_imm(REG_ECX, 1);											// add  ecx,1
				_resolve_link(&link2);												// skipit:
				_resolve_link(&link3);												// skipit:
				_mov_m32bd_r32(REG_ESI, REGDISP(RDREG), REG_ECX);					// mov  [rdreg].lo,ecx
				_mov_m32bd_imm(REG_ESI, REGDISP(RDREG)+4, 0);						// mov  [rdreg].hi,0
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x2c:	/* DADD */
			if (RSREG != 0 && RTREG != 0)
			{
				_mov_r64_m64bd(REG_EDX, REG_EAX, REG_ESI, REGDISP(RSREG));			// mov  edx:eax,[rsreg]
				_add_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));					// add  eax,[rtreg].lo
				_adc_r32_m32bd(REG_EDX, REG_ESI, REGDISP(RTREG)+4);					// adc  edx,[rtreg].hi
				_jcc(COND_O, mips3.generate_overflow_exception);					// jo   generate_overflow_exception
				if (RDREG != 0)
					_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);		// mov  [rdreg],edx:eax
			}
			else if (RDREG != 0)
			{
				if (RSREG != 0)
					_mov_m64bd_m64bd(REG_ESI, REGDISP(RDREG), REG_ESI, REGDISP(RSREG));	// mov  [rdreg],[rsreg]
				else if (RTREG != 0)
					_mov_m64bd_m64bd(REG_ESI, REGDISP(RDREG), REG_ESI, REGDISP(RTREG));	// mov  [rdreg],[rtreg]
				else
					_zero_m64bd(REG_ESI, REGDISP(RDREG));
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x2d:	/* DADDU */
			if (RDREG != 0)
			{
				if (RSREG != 0 && RTREG != 0)
				{
					if (USE_SSE)
					{
						_movsd_r128_m64bd(REG_XMM0, REG_ESI, REGDISP(RSREG));		// movsd xmm0,[rsreg]
						_movsd_r128_m64bd(REG_XMM1, REG_ESI, REGDISP(RTREG));		// movsd xmm1,[rtreg]
						_paddq_r128_r128(REG_XMM0, REG_XMM1);						// paddq xmm0,xmm1
						_movsd_m64bd_r128(REG_ESI, REGDISP(RDREG), REG_XMM0);		// mov  [rdreg],xmm0
					}
					else
					{
						_mov_r64_m64bd(REG_EDX, REG_EAX, REG_ESI, REGDISP(RSREG));	// mov  edx:eax,[rsreg]
						_add_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));			// add  eax,[rtreg].lo
						_adc_r32_m32bd(REG_EDX, REG_ESI, REGDISP(RTREG)+4);			// adc  edx,[rtreg].hi
						_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);	// mov  [rdreg],edx:eax
					}
				}
				else if (RSREG != 0)
					_mov_m64bd_m64bd(REG_ESI, REGDISP(RDREG), REG_ESI, REGDISP(RSREG));	// mov  [rdreg],[rsreg]
				else if (RTREG != 0)
					_mov_m64bd_m64bd(REG_ESI, REGDISP(RDREG), REG_ESI, REGDISP(RTREG));	// mov  [rdreg],[rtreg]
				else
					_zero_m64bd(REG_ESI, REGDISP(RDREG));
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x2e:	/* DSUB */
			if (RSREG != 0 && RTREG != 0)
			{
				_mov_r64_m64bd(REG_EDX, REG_EAX, REG_ESI, REGDISP(RSREG));			// mov  edx:eax,[rsreg]
				_sub_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));					// sub  eax,[rtreg].lo
				_sbb_r32_m32bd(REG_EDX, REG_ESI, REGDISP(RTREG)+4);					// sbb  edx,[rtreg].hi
				_jcc(COND_O, mips3.generate_overflow_exception);					// jo   generate_overflow_exception
				if (RDREG != 0)
					_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);		// mov  [rdreg],edx:eax
			}
			else if (RDREG != 0)
			{
				if (RSREG != 0)
					_mov_m64bd_m64bd(REG_ESI, REGDISP(RDREG), REG_ESI, REGDISP(RSREG));	// mov  [rdreg],[rsreg]
				else if (RTREG != 0)
				{
					if (USE_SSE)
					{
						_pxor_r128_r128(REG_XMM0, REG_XMM0);						// pxor xmm0,xmm0
						_movsd_r128_m64bd(REG_XMM1, REG_ESI, REGDISP(RTREG));		// movsd xmm1,[rtreg]
						_psubq_r128_r128(REG_XMM0, REG_XMM1);						// psubq xmm0,xmm1
						_movsd_m64bd_r128(REG_ESI, REGDISP(RDREG), REG_XMM0);		// mov  [rdreg],xmm0
					}
					else
					{
						_xor_r32_r32(REG_EAX, REG_EAX);								// xor  eax,eax
						_xor_r32_r32(REG_EDX, REG_EDX);								// xor  edx,edx
						_sub_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));			// sub  eax,[rtreg].lo
						_sbb_r32_m32bd(REG_EDX, REG_ESI, REGDISP(RTREG)+4);			// sbb  edx,[rtreg].hi
						_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);	// mov  [rdreg],edx:eax
					}
				}
				else
					_zero_m64bd(REG_ESI, REGDISP(RDREG));
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x2f:	/* DSUBU */
			if (RDREG != 0)
			{
				if (RSREG != 0 && RTREG != 0)
				{
					if (USE_SSE)
					{
						_movsd_r128_m64bd(REG_XMM0, REG_ESI, REGDISP(RSREG));		// movsd xmm0,[rsreg]
						_movsd_r128_m64bd(REG_XMM1, REG_ESI, REGDISP(RTREG));		// movsd xmm1,[rtreg]
						_psubq_r128_r128(REG_XMM0, REG_XMM1);						// psubq xmm0,xmm1
						_movsd_m64bd_r128(REG_ESI, REGDISP(RDREG), REG_XMM0);		// mov  [rdreg],xmm0
					}
					else
					{
						_mov_r64_m64bd(REG_EDX, REG_EAX, REG_ESI, REGDISP(RSREG));	// mov  edx:eax,[rsreg]
						_sub_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));			// sub  eax,[rtreg].lo
						_sbb_r32_m32bd(REG_EDX, REG_ESI, REGDISP(RTREG)+4);			// sbb  edx,[rtreg].hi
						_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);	// mov  [rdreg],edx:eax
					}
				}
				else if (RSREG != 0)
					_mov_m64bd_m64bd(REG_ESI, REGDISP(RDREG), REG_ESI, REGDISP(RSREG));	// mov  [rdreg],[rsreg]
				else if (RTREG != 0)
				{
					if (USE_SSE)
					{
						_pxor_r128_r128(REG_XMM0, REG_XMM0);						// pxor xmm0,xmm0
						_movsd_r128_m64bd(REG_XMM1, REG_ESI, REGDISP(RTREG));		// movsd xmm1,[rtreg]
						_psubq_r128_r128(REG_XMM0, REG_XMM1);						// psubq xmm0,xmm1
						_movsd_m64bd_r128(REG_ESI, REGDISP(RDREG), REG_XMM0);		// mov  [rdreg],xmm0
					}
					else
					{
						_xor_r32_r32(REG_EAX, REG_EAX);								// xor  eax,eax
						_xor_r32_r32(REG_EDX, REG_EDX);								// xor  edx,edx
						_sub_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));			// sub  eax,[rtreg].lo
						_sbb_r32_m32bd(REG_EDX, REG_ESI, REGDISP(RTREG)+4);			// sbb  edx,[rtreg].hi
						_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);	// mov  [rdreg],edx:eax
					}
				}
				else
					_zero_m64bd(REG_ESI, REGDISP(RDREG));
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x30:	/* TGE */
			if (RSREG != 0)
				_mov_r64_m64bd(REG_EDX, REG_EAX, REG_ESI, REGDISP(RSREG));			// mov  edx:eax,[rsreg]
			else
			{
				_xor_r32_r32(REG_EDX, REG_EDX);										// xor  edx,edx
				_xor_r32_r32(REG_EAX, REG_EAX);										// xor  eax,eax
			}
			if (RTREG != 0)
			{
				_sub_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));					// sub  eax,[rtreg].lo
				_sbb_r32_m32bd(REG_EDX, REG_ESI, REGDISP(RTREG)+4);					// sbb  edx,[rtreg].hi
			}
			else
				_cmp_r32_imm(REG_EDX, 0);											// cmp  edx,0
			_jcc(COND_GE, mips3.generate_trap_exception);							// jge  generate_trap_exception
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x31:	/* TGEU */
			_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG)+4);						// mov  eax,[rsreg].hi
			_cmp_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG)+4);						// cmp  eax,[rtreg].hi
			_jcc(COND_A, mips3.generate_trap_exception);							// ja   generate_trap_exception
			_jcc_short_link(COND_B, &link1);										// jb   skipit
			_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));						// mov  eax,[rsreg].lo
			_cmp_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));						// cmp  eax,[rtreg].lo
			_jcc(COND_AE, mips3.generate_trap_exception);							// jae  generate_trap_exception
			_resolve_link(&link1);													// skipit:
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x32:	/* TLT */
			if (RSREG != 0)
				_mov_r64_m64bd(REG_EDX, REG_EAX, REG_ESI, REGDISP(RSREG));			// mov  edx:eax,[rsreg]
			else
			{
				_xor_r32_r32(REG_EDX, REG_EDX);										// xor  edx,edx
				_xor_r32_r32(REG_EAX, REG_EAX);										// xor  eax,eax
			}
			if (RTREG != 0)
			{
				_sub_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));					// sub  eax,[rtreg].lo
				_sbb_r32_m32bd(REG_EDX, REG_ESI, REGDISP(RTREG)+4);					// sbb  edx,[rtreg].hi
			}
			else
				_cmp_r32_imm(REG_EDX, 0);											// cmp  edx,0
			_jcc(COND_L, mips3.generate_trap_exception);							// jl   generate_trap_exception
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x33:	/* TLTU */
			_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG)+4);						// mov  eax,[rsreg].hi
			_cmp_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG)+4);						// cmp  eax,[rtreg].hi
			_jcc(COND_B, mips3.generate_trap_exception);							// jb   generate_trap_exception
			_jcc_short_link(COND_A, &link1);										// ja   skipit
			_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));						// mov  eax,[rsreg].lo
			_cmp_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));						// cmp  eax,[rtreg].lo
			_jcc(COND_B, mips3.generate_trap_exception);							// jb   generate_trap_exception
			_resolve_link(&link1);													// skipit:
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x34:	/* TEQ */
			_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG)+4);						// mov  eax,[rsreg].hi
			_cmp_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG)+4);						// cmp  eax,[rtreg].hi
			_jcc_short_link(COND_NE, &link1);										// jne  skipit
			_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));						// mov  eax,[rsreg].lo
			_cmp_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));						// cmp  eax,[rtreg].lo
			_jcc(COND_E, mips3.generate_trap_exception);							// je   generate_trap_exception
			_resolve_link(&link1);													// skipit:
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x36:	/* TNE */
			_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG)+4);						// mov  eax,[rsreg].hi
			_cmp_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG)+4);						// cmp  eax,[rtreg].hi
			_jcc_short_link(COND_E, &link1);										// je   skipit
			_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));						// mov  eax,[rsreg].lo
			_cmp_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));						// cmp  eax,[rtreg].lo
			_jcc(COND_NE, mips3.generate_trap_exception);							// jne  generate_trap_exception
			_resolve_link(&link1);													// skipit:
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x38:	/* DSLL */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					if (USE_SSE)
					{
						_movsd_r128_m64bd(REG_XMM0, REG_ESI, REGDISP(RTREG));		// movsd xmm0,[rtreg]
						if (SHIFT)
							_psllq_r128_imm(REG_XMM0, SHIFT);						// psllq xmm0,SHIFT
						_movsd_m64bd_r128(REG_ESI, REGDISP(RDREG), REG_XMM0);		// movsd [rdreg],xmm0
					}
					else
					{
						_mov_r64_m64bd(REG_EDX, REG_EAX, REG_ESI, REGDISP(RTREG));	// mov  edx:eax,[rtreg]
						if (SHIFT != 0)
						{
							_shld_r32_r32_imm(REG_EDX, REG_EAX, SHIFT);				// shld edx,eax,SHIFT
							_shl_r32_imm(REG_EAX, SHIFT);							// shl  eax,SHIFT
						}
						_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);	// mov  [rdreg],edx:eax
					}
				}
				else
					_zero_m64bd(REG_ESI, REGDISP(RDREG));
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x3a:	/* DSRL */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					if (USE_SSE)
					{
						_movsd_r128_m64bd(REG_XMM0, REG_ESI, REGDISP(RTREG));		// movsd xmm0,[rtreg]
						if (SHIFT)
							_psrlq_r128_imm(REG_XMM0, SHIFT);						// psrlq xmm0,SHIFT
						_movsd_m64bd_r128(REG_ESI, REGDISP(RDREG), REG_XMM0);		// movsd [rdreg],xmm0
					}
					else
					{
						_mov_r64_m64bd(REG_EDX, REG_EAX, REG_ESI, REGDISP(RTREG));	// mov  edx:eax,[rtreg]
						if (SHIFT != 0)
						{
							_shrd_r32_r32_imm(REG_EAX, REG_EDX, SHIFT);				// shrd eax,edx,SHIFT
							_shr_r32_imm(REG_EDX, SHIFT);							// shr  edx,SHIFT
						}
						_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);	// mov  [rdreg],edx:eax
					}
				}
				else
					_zero_m64bd(REG_ESI, REGDISP(RDREG));
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x3b:	/* DSRA */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					_mov_r64_m64bd(REG_EDX, REG_EAX, REG_ESI, REGDISP(RTREG));		// mov  edx:eax,[rtreg]
					if (SHIFT != 0)
					{
						_shrd_r32_r32_imm(REG_EAX, REG_EDX, SHIFT);					// shrd eax,edx,SHIFT
						_sar_r32_imm(REG_EDX, SHIFT);								// sar  edx,SHIFT
					}
					_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);		// mov  [rdreg],edx:eax
				}
				else
					_zero_m64bd(REG_ESI, REGDISP(RDREG));
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x3c:	/* DSLL32 */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					if (USE_SSE)
					{
						_movsd_r128_m64bd(REG_XMM0, REG_ESI, REGDISP(RTREG));		// movsd xmm0,[rtreg]
						_psllq_r128_imm(REG_XMM0, SHIFT+32);						// psllq xmm0,SHIFT+32
						_movsd_m64bd_r128(REG_ESI, REGDISP(RDREG), REG_XMM0);		// movsd [rdreg],xmm0
					}
					else
					{
						_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));			// mov  eax,[rtreg].lo
						if (SHIFT != 0)
							_shl_r32_imm(REG_EAX, SHIFT);							// shl  eax,SHIFT
						_mov_m32bd_imm(REG_ESI, REGDISP(RDREG), 0);					// mov  [rdreg].lo,0
						_mov_m32bd_r32(REG_ESI, REGDISP(RDREG)+4, REG_EAX);			// mov  [rdreg].hi,eax
					}
				}
				else
					_zero_m64bd(REG_ESI, REGDISP(RDREG));
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x3e:	/* DSRL32 */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					if (USE_SSE)
					{
						_movsd_r128_m64bd(REG_XMM0, REG_ESI, REGDISP(RTREG));		// movsd xmm0,[rtreg]
						_psrlq_r128_imm(REG_XMM0, SHIFT+32);						// psrlq xmm0,SHIFT+32
						_movsd_m64bd_r128(REG_ESI, REGDISP(RDREG), REG_XMM0);		// movsd [rdreg],xmm0
					}
					else
					{
						_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG)+4);			// mov  eax,[rtreg].hi
						if (SHIFT != 0)
							_shr_r32_imm(REG_EAX, SHIFT);							// shr  eax,SHIFT
						_mov_m32bd_imm(REG_ESI, REGDISP(RDREG)+4, 0);				// mov  [rdreg].hi,0
						_mov_m32bd_r32(REG_ESI, REGDISP(RDREG), REG_EAX);			// mov  [rdreg].lo,eax
					}
				}
				else
					_zero_m64bd(REG_ESI, REGDISP(RDREG));
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x3f:	/* DSRA32 */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG)+4);				// mov  eax,[rtreg].hi
					_cdq();															// cdq
					if (SHIFT != 0)
						_sar_r32_imm(REG_EAX, SHIFT);								// sar  eax,SHIFT
					_mov_m64bd_r64(REG_ESI, REGDISP(RDREG), REG_EDX, REG_EAX);		// mov  [rdreg],edx:eax
				}
				else
					_zero_m64bd(REG_ESI, REGDISP(RDREG));
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);
	}

	_jmp((void *)mips3.generate_invalidop_exception);								// jmp  generate_invalidop_exception
	return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;
}


static UINT32 recompile_special(drc_core *drc, UINT32 pc, UINT32 op)
{
	if (!(op & 32))
		return recompile_special1(drc, pc, op);
	else
		return recompile_special2(drc, pc, op);
}


/*------------------------------------------------------------------
    recompile_regimm
------------------------------------------------------------------*/

static UINT32 recompile_regimm(drc_core *drc, UINT32 pc, UINT32 op)
{
	link_info link1;
	int cycles;

	switch (RTREG)
	{
		case 0x00:	/* BLTZ */
			if (RSREG == 0)
				return RECOMPILE_SUCCESSFUL_CP(1,4);
			else
			{
				_cmp_m32bd_imm(REG_ESI, REGDISP(RSREG)+4, 0);						// cmp  [rsreg].hi,0
				_jcc_near_link(COND_GE, &link1);									// jge  skip
			}

			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		// <branch or dispatch>
			_resolve_link(&link1);													// skip:
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x01:	/* BGEZ */
			if (RSREG == 0)
			{
				cycles = recompile_delay_slot(drc, pc + 4);							// <next instruction>
				append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);	// <branch or dispatch>
				return RECOMPILE_SUCCESSFUL_CP(0,0) | RECOMPILE_END_OF_STRING;
			}
			else
			{
				_cmp_m32bd_imm(REG_ESI, REGDISP(RSREG)+4, 0);						// cmp  [rsreg].hi,0
				_jcc_near_link(COND_L, &link1);										// jl   skip
			}

			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		// <branch or dispatch>
			_resolve_link(&link1);													// skip:
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x02:	/* BLTZL */
			if (RSREG == 0)
				return RECOMPILE_SUCCESSFUL_CP(1,4);
			else
			{
				_cmp_m32bd_imm(REG_ESI, REGDISP(RSREG)+4, 0);						// cmp  [rsreg].hi,0
				_jcc_near_link(COND_GE, &link1);									// jge  skip
			}

			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		// <branch or dispatch>
			_resolve_link(&link1);													// skip:
			return RECOMPILE_SUCCESSFUL_CP(1,8);

		case 0x03:	/* BGEZL */
			if (RSREG == 0)
			{
				cycles = recompile_delay_slot(drc, pc + 4);							// <next instruction>
				append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);	// <branch or dispatch>
				return RECOMPILE_SUCCESSFUL_CP(0,0) | RECOMPILE_END_OF_STRING;
			}
			else
			{
				_cmp_m32bd_imm(REG_ESI, REGDISP(RSREG)+4, 0);						// cmp  [rsreg].hi,0
				_jcc_near_link(COND_L, &link1);										// jl   skip
			}

			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		// <branch or dispatch>
			_resolve_link(&link1);													// skip:
			return RECOMPILE_SUCCESSFUL_CP(1,8);

		case 0x08:	/* TGEI */
			if (RSREG != 0)
			{
				_mov_r64_m64bd(REG_EDX, REG_EAX, REG_ESI, REGDISP(RSREG));			// mov  edx:eax,[rsreg]
				_sub_r32_imm(REG_EAX, SIMMVAL);										// sub  eax,[rtreg].lo
				_sbb_r32_imm(REG_EDX, ((INT32)SIMMVAL >> 31));						// sbb  edx,[rtreg].lo
				_jcc(COND_GE, mips3.generate_trap_exception);						// jge  generate_trap_exception
			}
			else
			{
				if (0 >= SIMMVAL)
					_jmp(mips3.generate_trap_exception);							// jmp  generate_trap_exception
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x09:	/* TGEIU */
			if (RSREG != 0)
			{
				_cmp_m32bd_imm(REG_ESI, REGDISP(RSREG)+4, ((INT32)SIMMVAL >> 31));	// cmp  [rsreg].hi,upper
				_jcc(COND_A, mips3.generate_trap_exception);						// ja   generate_trap_exception
				_jcc_short_link(COND_B, &link1);									// jb   skip
				_cmp_m32bd_imm(REG_ESI, REGDISP(RSREG), SIMMVAL);					// cmp  [rsreg].lo,lower
				_jcc(COND_AE, mips3.generate_trap_exception);						// jae  generate_trap_exception
				_resolve_link(&link1);												// skip:
			}
			else
			{
				if (0 >= SIMMVAL)
					_jmp(mips3.generate_trap_exception);							// jmp  generate_trap_exception
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x0a:	/* TLTI */
			if (RSREG != 0)
			{
				_mov_r64_m64bd(REG_EDX, REG_EAX, REG_ESI, REGDISP(RSREG));			// mov  edx:eax,[rsreg]
				_sub_r32_imm(REG_EAX, SIMMVAL);										// sub  eax,[rtreg].lo
				_sbb_r32_imm(REG_EDX, ((INT32)SIMMVAL >> 31));						// sbb  edx,[rtreg].lo
				_jcc(COND_L, mips3.generate_trap_exception);						// jl   generate_trap_exception
			}
			else
			{
				if (0 < SIMMVAL)
					_jmp(mips3.generate_trap_exception);							// jmp  generate_trap_exception
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x0b:	/* TLTIU */
			if (RSREG != 0)
			{
				_cmp_m32bd_imm(REG_ESI, REGDISP(RSREG)+4, ((INT32)SIMMVAL >> 31));	// cmp  [rsreg].hi,upper
				_jcc(COND_B, mips3.generate_trap_exception);						// jb   generate_trap_exception
				_jcc_short_link(COND_A, &link1);									// ja   skip
				_cmp_m32bd_imm(REG_ESI, REGDISP(RSREG), SIMMVAL);					// cmp  [rsreg].lo,lower
				_jcc(COND_B, mips3.generate_trap_exception);						// jb   generate_trap_exception
				_resolve_link(&link1);												// skip:
			}
			else
			{
				_mov_m32bd_imm(REG_ESI, REGDISP(RTREG), (0 < SIMMVAL));				// mov  [rtreg].lo,const
				_mov_m32bd_imm(REG_ESI, REGDISP(RTREG)+4, 0);						// mov  [rtreg].hi,sign-extend(const)
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x0c:	/* TEQI */
			if (RSREG != 0)
			{
				_cmp_m32bd_imm(REG_ESI, REGDISP(RSREG)+4, ((INT32)SIMMVAL >> 31));	// cmp  [rsreg].hi,upper
				_jcc_short_link(COND_NE, &link1);									// jne  skip
				_cmp_m32bd_imm(REG_ESI, REGDISP(RSREG), SIMMVAL);					// cmp  [rsreg].lo,lower
				_jcc(COND_E, mips3.generate_trap_exception);						// je   generate_trap_exception
				_resolve_link(&link1);												// skip:
			}
			else
			{
				if (0 == SIMMVAL)
					_jmp(mips3.generate_trap_exception);							// jmp  generate_trap_exception
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x0e:	/* TNEI */
			if (RSREG != 0)
			{
				_cmp_m32bd_imm(REG_ESI, REGDISP(RSREG)+4, ((INT32)SIMMVAL >> 31));	// cmp  [rsreg].hi,upper
				_jcc_short_link(COND_E, &link1);									// je   skip
				_cmp_m32bd_imm(REG_ESI, REGDISP(RSREG), SIMMVAL);					// cmp  [rsreg].lo,lower
				_jcc(COND_NE, mips3.generate_trap_exception);						// jne  generate_trap_exception
				_resolve_link(&link1);												// skip:
			}
			else
			{
				if (0 != SIMMVAL)
					_jmp(mips3.generate_trap_exception);							// jmp  generate_trap_exception
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x10:	/* BLTZAL */
			if (RSREG == 0)
				return RECOMPILE_SUCCESSFUL_CP(1,4);
			else
			{
				_cmp_m32bd_imm(REG_ESI, REGDISP(RSREG)+4, 0);						// cmp  [rsreg].hi,0
				_jcc_near_link(COND_GE, &link1);									// jge  skip
			}

			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			_mov_m64abs_imm32(&mips3.r[31], pc + 8);								// mov  [31],pc + 8
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		// <branch or dispatch>
			_resolve_link(&link1);													// skip:
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x11:	/* BGEZAL */
			if (RSREG == 0)
			{
				cycles = recompile_delay_slot(drc, pc + 4);							// <next instruction>
				_mov_m64abs_imm32(&mips3.r[31], pc + 8);							// mov  [31],pc + 8
				append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);	// <branch or dispatch>
				return RECOMPILE_SUCCESSFUL_CP(0,0) | RECOMPILE_END_OF_STRING;
			}
			else
			{
				_cmp_m32bd_imm(REG_ESI, REGDISP(RSREG)+4, 0);						// cmp  [rsreg].hi,0
				_jcc_near_link(COND_L, &link1);										// jl   skip
			}

			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			_mov_m64abs_imm32(&mips3.r[31], pc + 8);								// mov  [31],pc + 8
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		// <branch or dispatch>
			_resolve_link(&link1);													// skip:
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x12:	/* BLTZALL */
			if (RSREG == 0)
				return RECOMPILE_SUCCESSFUL_CP(1,4);
			else
			{
				_cmp_m32bd_imm(REG_ESI, REGDISP(RSREG)+4, 0);						// cmp  [rsreg].hi,0
				_jcc_near_link(COND_GE, &link1);									// jge  skip
			}

			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			_mov_m64abs_imm32(&mips3.r[31], pc + 8);								// mov  [31],pc + 8
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		// <branch or dispatch>
			_resolve_link(&link1);													// skip:
			return RECOMPILE_SUCCESSFUL_CP(1,8);

		case 0x13:	/* BGEZALL */
			if (RSREG == 0)
			{
				cycles = recompile_delay_slot(drc, pc + 4);							// <next instruction>
				_mov_m64abs_imm32(&mips3.r[31], pc + 8);							// mov  [31],pc + 8
				append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);	// <branch or dispatch>
				return RECOMPILE_SUCCESSFUL_CP(0,0) | RECOMPILE_END_OF_STRING;
			}
			else
			{
				_cmp_m32bd_imm(REG_ESI, REGDISP(RSREG)+4, 0);						// cmp  [rsreg].hi,0
				_jcc_near_link(COND_L, &link1);										// jl   skip
			}

			cycles = recompile_delay_slot(drc, pc + 4);								// <next instruction>
			_mov_m64abs_imm32(&mips3.r[31], pc + 8);								// mov  [31],pc + 8
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		// <branch or dispatch>
			_resolve_link(&link1);													// skip:
			return RECOMPILE_SUCCESSFUL_CP(1,8);
	}

	_jmp((void *)mips3.generate_invalidop_exception);								// jmp  generate_invalidop_exception
	return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;
}



/***************************************************************************
    COP0 RECOMPILATION
***************************************************************************/

/*------------------------------------------------------------------
    recompile_set_cop0_reg
------------------------------------------------------------------*/

static UINT32 recompile_set_cop0_reg(drc_core *drc, UINT8 reg)
{
	link_info link1;

	switch (reg)
	{
		case COP0_Cause:
			_mov_r32_m32abs(REG_EBX, &mips3.cpr[0][COP0_Cause]);					// mov  ebx,[mips3.cpr[0][COP0_Cause]]
			_and_r32_imm(REG_EAX, ~0xfc00);											// and  eax,~0xfc00
			_and_r32_imm(REG_EBX, 0xfc00);											// and  ebx,0xfc00
			_or_r32_r32(REG_EAX, REG_EBX);											// or   eax,ebx
			_mov_m32abs_r32(&mips3.cpr[0][COP0_Cause], REG_EAX);					// mov  [mips3.cpr[0][COP0_Cause]],eax
			_and_r32_m32abs(REG_EAX, &mips3.cpr[0][COP0_Status]);					// and  eax,[mips3.cpr[0][COP0_Status]]
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_CHECK_SW_INTERRUPTS;

		case COP0_Status:
			_mov_r32_m32abs(REG_EBX, &mips3.cpr[0][COP0_Status]);					// mov  ebx,[mips3.cpr[0][COP0_Status]]
			_mov_m32abs_r32(&mips3.cpr[0][COP0_Status], REG_EAX);					// mov  [mips3.cpr[0][COP0_Status]],eax
			_xor_r32_r32(REG_EBX, REG_EAX);											// xor  ebx,eax
/*
#ifdef MAME_DEBUG
            _test_r32_imm(REG_EAX, 0xe0);                                           // test eax,0xe0
            _jcc_short_link(COND_Z, &link1);                                        // jz   skip
            _push_r32(REG_EAX);                                                     // push eax
            _push_imm("System set 64-bit addressing mode, SR=%08X");                // push <string>
            _call(mame_printf_debug);                                                     // call mame_printf_debug
            _add_r32_imm(REG_ESP, 8);                                               // add esp,8
            _call(mame_debug_break);                                                // call mame_debug_break
            _resolve_link(&link1);                                                  // skip:
#endif
*/
			_test_r32_imm(REG_EBX, 0x8000);											// test ebx,0x8000
			_jcc_short_link(COND_Z, &link1);										// jz   skip
			append_update_cycle_counting(drc);										// update cycle counting
			_resolve_link(&link1);													// skip:
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_CHECK_INTERRUPTS | RECOMPILE_END_OF_STRING | RECOMPILE_ADD_DISPATCH;

		case COP0_Count:
			_mov_m32abs_r32(&mips3.cpr[0][COP0_Count], REG_EAX);					// mov  [mips3.cpr[0][COP0_Count]],eax
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov  [mips3_icount],ebp
			_push_r32(REG_EAX);														// push eax
			_call((genf *)activecpu_gettotalcycles64);								// call activecpu_gettotalcycles64
			_pop_r32(REG_EBX);														// pop  ebx
			_sub_r32_r32(REG_EAX, REG_EBX);											// sub  eax,ebx
			_sbb_r32_imm(REG_EDX, 0);												// sbb  edx,0
			_sub_r32_r32(REG_EAX, REG_EBX);											// sub  eax,ebx
			_sbb_r32_imm(REG_EDX, 0);												// sbb  edx,0
			_mov_m64abs_r64(&mips3.count_zero_time, REG_EDX, REG_EAX);				// mov  [mips3.count_zero_time],edx:eax
			append_update_cycle_counting(drc);										// update cycle counting
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case COP0_Compare:
			_mov_m32abs_r32(&mips3.cpr[0][COP0_Compare], REG_EAX);					// mov  [mips3.cpr[0][COP0_Compare]],eax
			_and_m32abs_imm(&mips3.cpr[0][COP0_Cause], ~0x8000);					// and  [mips3.cpr[0][COP0_Cause]],~0x8000
			append_update_cycle_counting(drc);										// update cycle counting
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case COP0_PRId:
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case COP0_Config:
			_mov_r32_m32abs(REG_EBX, &mips3.cpr[0][COP0_Config]);					// mov  ebx,[mips3.cpr[0][COP0_Cause]]
			_and_r32_imm(REG_EAX, 0x0007);											// and  eax,0x0007
			_and_r32_imm(REG_EBX, ~0x0007);											// and  ebx,~0x0007
			_or_r32_r32(REG_EAX, REG_EBX);											// or   eax,ebx
			_mov_m32abs_r32(&mips3.cpr[0][COP0_Config], REG_EAX);					// mov  [mips3.cpr[0][COP0_Cause]],eax
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		default:
			_mov_m32abs_r32(&mips3.cpr[0][reg], REG_EAX);							// mov  [mips3.cpr[0][reg]],eax
			return RECOMPILE_SUCCESSFUL_CP(1,4);
	}
	return RECOMPILE_UNIMPLEMENTED;
}


/*------------------------------------------------------------------
    recompile_get_cop0_reg
------------------------------------------------------------------*/

static UINT32 recompile_get_cop0_reg(drc_core *drc, UINT8 reg)
{
	link_info link1;

	switch (reg)
	{
		case COP0_Count:
			_sub_r32_imm(REG_EBP, 250);												// sub  ebp,24
			_jcc_short_link(COND_NS, &link1);										// jns  notneg
			_xor_r32_r32(REG_EBP, REG_EBP);											// xor  ebp,ebp
			_resolve_link(&link1);													// notneg:
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov  [mips3_icount],ebp
			_call((genf *)activecpu_gettotalcycles64);								// call activecpu_gettotalcycles64
			_sub_r32_m32abs(REG_EAX, LO(&mips3.count_zero_time));					// sub  eax,[mips3.count_zero_time+0]
			_sbb_r32_m32abs(REG_EDX, HI(&mips3.count_zero_time));					// sbb  edx,[mips3.count_zero_time+4]
			_shrd_r32_r32_imm(REG_EAX, REG_EDX, 1);									// shrd eax,edx,1
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case COP0_Cause:
			_sub_r32_imm(REG_EBP, 250);												// sub  ebp,24
			_jcc_short_link(COND_NS, &link1);										// jns  notneg
			_xor_r32_r32(REG_EBP, REG_EBP);											// xor  ebp,ebp
			_resolve_link(&link1);													// notneg:
			_mov_r32_m32abs(REG_EAX, &mips3.cpr[0][reg]);							// mov  eax,[mips3.cpr[0][reg]]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		default:
			_mov_r32_m32abs(REG_EAX, &mips3.cpr[0][reg]);							// mov  eax,[mips3.cpr[0][reg]]
			return RECOMPILE_SUCCESSFUL_CP(1,4);
	}
	return RECOMPILE_UNIMPLEMENTED;
}


/*------------------------------------------------------------------
    recompile_cop0
------------------------------------------------------------------*/

static UINT32 recompile_cop0(drc_core *drc, UINT32 pc, UINT32 op)
{
	link_info checklink;
	UINT32 result;

	if (mips3.drcoptions & MIPS3DRC_STRICT_COP0)
	{
		_test_m32abs_imm(&mips3.cpr[0][COP0_Status], SR_KSU_MASK);					// test [mips3.cpr[0][COP0_Status]],SR_KSU_MASK
		_jcc_short_link(COND_Z, &checklink);										// jz   okay
		_test_m32abs_imm(&mips3.cpr[0][COP0_Status], SR_COP0);						// test [mips3.cpr[0][COP0_Status]],SR_COP0
		_jcc(COND_Z, mips3.generate_cop_exception);									// jz   generate_cop_exception
		_resolve_link(&checklink);													// okay:
	}

	switch (RSREG)
	{
		case 0x00:	/* MFCz */
			result = RECOMPILE_SUCCESSFUL_CP(1,4);
			if (RTREG != 0)
			{
				result = recompile_get_cop0_reg(drc, RDREG);						// read cop0 reg
				_cdq();																// cdq
				_mov_m64bd_r64(REG_ESI, REGDISP(RTREG), REG_EDX, REG_EAX);			// mov  [rtreg],edx:eax
			}
			return result;

		case 0x01:	/* DMFCz */
			result = RECOMPILE_SUCCESSFUL_CP(1,4);
			if (RTREG != 0)
			{
				result = recompile_get_cop0_reg(drc, RDREG);						// read cop0 reg
				_cdq();																// cdq
				_mov_m64bd_r64(REG_ESI, REGDISP(RTREG), REG_EDX, REG_EAX);			// mov  [rtreg],edx:eax
			}
			return result;

		case 0x02:	/* CFCz */
			if (RTREG != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.ccr[0][RDREG]);						// mov  eax,[mips3.ccr[0][rdreg]]
				_cdq();																// cdq
				_mov_m64bd_r64(REG_ESI, REGDISP(RTREG), REG_EDX, REG_EAX);			// mov  [rtreg],edx:eax
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x04:	/* MTCz */
			if (RTREG != 0)
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));					// mov  eax,[mips3.r[RTREG]]
			else
				_xor_r32_r32(REG_EAX, REG_EAX);										// xor  eax,eax
			result = recompile_set_cop0_reg(drc, RDREG);							// write cop0 reg
			return result;

		case 0x05:	/* DMTCz */
			if (RTREG != 0)
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));					// mov  eax,[mips3.r[RTREG]]
			else
				_xor_r32_r32(REG_EAX, REG_EAX);										// xor  eax,eax
			result = recompile_set_cop0_reg(drc, RDREG);							// write cop0 reg
			return result;

		case 0x06:	/* CTCz */
			if (RTREG != 0)
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));					// mov  eax,[mips3.r[RTREG]]
			else
				_xor_r32_r32(REG_EAX, REG_EAX);										// xor  eax,eax
			_mov_m32abs_r32(&mips3.ccr[0][RDREG], REG_EAX);							// mov  [mips3.ccr[0][RDREG]],eax
			return RECOMPILE_SUCCESSFUL_CP(1,4);

//      case 0x08:  /* BC */
//          switch (RTREG)
//          {
//              case 0x00:  /* BCzF */  if (!mips3.cf[0][0]) ADDPC(SIMMVAL);                break;
//              case 0x01:  /* BCzF */  if (mips3.cf[0][0]) ADDPC(SIMMVAL);                 break;
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
					drc_append_save_call_restore(drc, (genf *)tlbr, 0);
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x02:	/* TLBWI */
					drc_append_save_call_restore(drc, (genf *)tlbwi, 0);
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x06:	/* TLBWR */
					drc_append_save_call_restore(drc, (genf *)tlbwr, 0);
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x08:	/* TLBP */
					drc_append_save_call_restore(drc, (genf *)tlbp, 0);
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x10:	/* RFE */
					_jmp(mips3.generate_invalidop_exception);						// jmp  generate_invalidop_exception
					return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;

				case 0x18:	/* ERET */
					_mov_r32_m32abs(REG_EDI, &mips3.cpr[0][COP0_EPC]);				// mov  edi,[mips3.cpr[0][COP0_EPC]]
					_and_m32abs_imm(&mips3.cpr[0][COP0_Status], ~SR_EXL);			// and  [mips3.cpr[0][COP0_Status]],~SR_EXL
					return RECOMPILE_SUCCESSFUL_CP(1,0) | RECOMPILE_CHECK_INTERRUPTS | RECOMPILE_END_OF_STRING | RECOMPILE_ADD_DISPATCH;

				default:
					_jmp(mips3.generate_invalidop_exception);						// jmp  generate_invalidop_exception
					return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;
			}
			break;

//      default:
//          _jmp(mips3.generate_invalidop_exception);                               // jmp  generate_invalidop_exception
//          return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;
	}
	return RECOMPILE_UNIMPLEMENTED;
}



/***************************************************************************
    COP1 RECOMPILATION
***************************************************************************/

/*------------------------------------------------------------------
    recompile_cop1
------------------------------------------------------------------*/

static UINT32 recompile_cop1(drc_core *drc, UINT32 pc, UINT32 op)
{
	link_info link1;
	int cycles, i;

	if (mips3.drcoptions & MIPS3DRC_STRICT_COP1)
	{
		_test_m32abs_imm(&mips3.cpr[0][COP0_Status], SR_COP1);						// test [mips3.cpr[0][COP0_Status]],SR_COP1
		_jcc(COND_Z, mips3.generate_cop_exception);									// jz   generate_cop_exception
	}

	switch (RSREG)
	{
		case 0x00:	/* MFCz */
			if (RTREG != 0)
			{
				_mov_r32_m32abs(REG_EAX, FPR32(RDREG));								// mov  eax,[mips3.cpr[1][RDREG]]
				_cdq();																// cdq
				_mov_m64bd_r64(REG_ESI, REGDISP(RTREG), REG_EDX, REG_EAX);			// mov  [mips3.r[RTREG]],edx:eax
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x01:	/* DMFCz */
			if (RTREG != 0)
				_mov_m64bd_m64abs(REG_ESI, REGDISP(RTREG), FPR64(RDREG));
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x02:	/* CFCz */
			if (RTREG != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.ccr[1][RDREG]);						// mov  eax,[mips3.ccr[1][RDREG]]
				if (RDREG == 31)
				{
					_and_r32_imm(REG_EAX, ~0xfe800000);								// and  eax,~0xfe800000
					_xor_r32_r32(REG_EBX, REG_EBX);									// xor  ebx,ebx
					_cmp_m8abs_imm(&mips3.cf[1][0], 0);								// cmp  [cf[0]],0
					_setcc_r8(COND_NZ, REG_BL);										// setnz bl
					_shl_r32_imm(REG_EBX, 23);										// shl  ebx,23
					_or_r32_r32(REG_EAX, REG_EBX);									// or   eax,ebx
					if (mips3.is_mips4)
						for (i = 1; i <= 7; i++)
						{
							_cmp_m8abs_imm(&mips3.cf[1][i], 0);						// cmp  [cf[i]],0
							_setcc_r8(COND_NZ, REG_BL);								// setnz bl
							_shl_r32_imm(REG_EBX, 24+i);							// shl  ebx,24+i
							_or_r32_r32(REG_EAX, REG_EBX);							// or   eax,ebx
						}
				}
				_cdq();																// cdq
				_mov_m64bd_r64(REG_ESI, REGDISP(RTREG), REG_EDX, REG_EAX);			// mov  [mips3.r[RTREG]],edx:eax
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x04:	/* MTCz */
			if (RTREG != 0)
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));					// mov  eax,[mips3.r[RTREG]]
				_mov_m32abs_r32(FPR32(RDREG), REG_EAX);								// mov  [mips3.cpr[1][RDREG]],eax
			}
			else
				_mov_m32abs_imm(FPR32(RDREG), 0);									// mov  [mips3.cpr[1][RDREG]],0
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x05:	/* DMTCz */
			if (RTREG != 0)
				_mov_m64abs_m64bd(FPR64(RDREG), REG_ESI, REGDISP(RTREG));
			else
				_zero_m64abs(FPR64(RDREG));											// mov  [mips3.cpr[1][RDREG]],0
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x06:	/* CTCz */
			if (RTREG != 0)
			{
				_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));					// mov  eax,[mips3.r[RTREG]]
				_mov_m32abs_r32(&mips3.ccr[1][RDREG], REG_EAX);						// mov  [mips3.ccr[1][RDREG]],eax
			}
			else
				_mov_m32abs_imm(&mips3.ccr[1][RDREG], 0);							// mov  [mips3.ccr[1][RDREG]],0
			if (RDREG == 31)
			{
				_mov_r32_m32abs(REG_EAX, LO(&mips3.ccr[1][RDREG]));					// mov  eax,[mips3.ccr[1][RDREG]]
				_test_r32_imm(REG_EAX, 1 << 23);									// test eax,1<<23
				_setcc_m8abs(COND_NZ, &mips3.cf[1][0]);								// setnz [cf[0]]
				if (mips3.is_mips4)
					for (i = 1; i <= 7; i++)
					{
						_test_r32_imm(REG_EAX, 1 << (24+i));						// test eax,1<<(24+i)
						_setcc_m8abs(COND_NZ, &mips3.cf[1][i]);						// setnz [cf[i]]
					}
				_and_r32_imm(REG_EAX, 3);											// and  eax,3
				_test_r32_imm(REG_EAX, 1);											// test eax,1
				_jcc_near_link(COND_Z, &link1);										// jz   skip
				_xor_r32_imm(REG_EAX, 2);											// xor  eax,2
				_resolve_link(&link1);												// skip:
				drc_append_set_fp_rounding(drc, REG_EAX);							// set_rounding(EAX)
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x08:	/* BC */
			switch ((op >> 16) & 3)
			{
				case 0x00:	/* BCzF */
					_cmp_m8abs_imm(&mips3.cf[1][mips3.is_mips4 ? ((op >> 18) & 7) : 0], 0);	// cmp  [cf[x]],0
					_jcc_near_link(COND_NZ, &link1);								// jnz  link1
					cycles = recompile_delay_slot(drc, pc + 4);						// <next instruction>
					append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);// <branch or dispatch>
					_resolve_link(&link1);											// skip:
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x01:	/* BCzT */
					_cmp_m8abs_imm(&mips3.cf[1][mips3.is_mips4 ? ((op >> 18) & 7) : 0], 0);	// cmp  [cf[x]],0
					_jcc_near_link(COND_Z, &link1);									// jz   link1
					cycles = recompile_delay_slot(drc, pc + 4);						// <next instruction>
					append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);// <branch or dispatch>
					_resolve_link(&link1);											// skip:
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x02:	/* BCzFL */
					_cmp_m8abs_imm(&mips3.cf[1][mips3.is_mips4 ? ((op >> 18) & 7) : 0], 0);	// cmp  [cf[x]],0
					_jcc_near_link(COND_NZ, &link1);								// jnz  link1
					cycles = recompile_delay_slot(drc, pc + 4);						// <next instruction>
					append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);// <branch or dispatch>
					_resolve_link(&link1);											// skip:
					return RECOMPILE_SUCCESSFUL_CP(1,8);

				case 0x03:	/* BCzTL */
					_cmp_m8abs_imm(&mips3.cf[1][mips3.is_mips4 ? ((op >> 18) & 7) : 0], 0);	// cmp  [cf[x]],0
					_jcc_near_link(COND_Z, &link1);									// jz   link1
					cycles = recompile_delay_slot(drc, pc + 4);						// <next instruction>
					append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);// <branch or dispatch>
					_resolve_link(&link1);											// skip:
					return RECOMPILE_SUCCESSFUL_CP(1,8);
			}
			break;

		default:
			switch (op & 0x3f)
			{
				case 0x00:
					if (IS_SINGLE(op))	/* ADD.S */
					{
						if (USE_SSE)
						{
							_movss_r128_m32abs(REG_XMM0, FPR32(FSREG));				// movss xmm0,[fsreg]
							_addss_r128_m32abs(REG_XMM0, FPR32(FTREG));				// addss xmm0,[ftreg]
							_movss_m32abs_r128(FPR32(FDREG), REG_XMM0);				// movss [fdreg],xmm0
						}
						else
						{
							_fld_m32abs(FPR32(FSREG));								// fld  [fsreg]
							_fld_m32abs(FPR32(FTREG));								// fld  [ftreg]
							_faddp();												// faddp
							_fstp_m32abs(FPR32(FDREG));								// fstp [fdreg]
						}
					}
					else				/* ADD.D */
					{
						if (USE_SSE)
						{
							_movsd_r128_m64abs(REG_XMM0, FPR64(FSREG));				// movsd xmm0,[fsreg]
							_addsd_r128_m64abs(REG_XMM0, FPR64(FTREG));				// addsd xmm0,[ftreg]
							_movsd_m64abs_r128(FPR64(FDREG), REG_XMM0);				// movsd [fdreg],xmm0
						}
						else
						{
							_fld_m64abs(FPR64(FSREG));								// fld  [fsreg]
							_fld_m64abs(FPR64(FTREG));								// fld  [ftreg]
							_faddp();												// faddp
							_fstp_m64abs(FPR64(FDREG));								// fstp [fdreg]
						}
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x01:
					if (IS_SINGLE(op))	/* SUB.S */
					{
						if (USE_SSE)
						{
							_movss_r128_m32abs(REG_XMM0, FPR32(FSREG));				// movss xmm0,[fsreg]
							_subss_r128_m32abs(REG_XMM0, FPR32(FTREG));				// subss xmm0,[ftreg]
							_movss_m32abs_r128(FPR32(FDREG), REG_XMM0);				// movss [fdreg],xmm0
						}
						else
						{
							_fld_m32abs(FPR32(FSREG));								// fld  [fsreg]
							_fld_m32abs(FPR32(FTREG));								// fld  [ftreg]
							_fsubp();												// fsubp
							_fstp_m32abs(FPR32(FDREG));								// fstp [fdreg]
						}
					}
					else				/* SUB.D */
					{
						if (USE_SSE)
						{
							_movsd_r128_m64abs(REG_XMM0, FPR64(FSREG));				// movsd xmm0,[fsreg]
							_subsd_r128_m64abs(REG_XMM0, FPR64(FTREG));				// subsd xmm0,[ftreg]
							_movsd_m64abs_r128(FPR64(FDREG), REG_XMM0);				// movsd [fdreg],xmm0
						}
						else
						{
							_fld_m64abs(FPR64(FSREG));								// fld  [fsreg]
							_fld_m64abs(FPR64(FTREG));								// fld  [ftreg]
							_fsubp();												// fsubp
							_fstp_m64abs(FPR64(FDREG));								// fstp [fdreg]
						}
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x02:
					if (IS_SINGLE(op))	/* MUL.S */
					{
						if (USE_SSE)
						{
							_movss_r128_m32abs(REG_XMM0, FPR32(FSREG));				// movss xmm0,[fsreg]
							_mulss_r128_m32abs(REG_XMM0, FPR32(FTREG));				// mulss xmm0,[ftreg]
							_movss_m32abs_r128(FPR32(FDREG), REG_XMM0);				// movss [fdreg],xmm0
						}
						else
						{
							_fld_m32abs(FPR32(FSREG));								// fld  [fsreg]
							_fld_m32abs(FPR32(FTREG));								// fld  [ftreg]
							_fmulp();												// fmulp
							_fstp_m32abs(FPR32(FDREG));								// fstp [fdreg]
						}
					}
					else				/* MUL.D */
					{
						if (USE_SSE)
						{
							_movsd_r128_m64abs(REG_XMM0, FPR64(FSREG));				// movsd xmm0,[fsreg]
							_mulsd_r128_m64abs(REG_XMM0, FPR64(FTREG));				// mulsd xmm0,[ftreg]
							_movsd_m64abs_r128(FPR64(FDREG), REG_XMM0);				// movsd [fdreg],xmm0
						}
						else
						{
							_fld_m64abs(FPR64(FSREG));								// fld  [fsreg]
							_fld_m64abs(FPR64(FTREG));								// fld  [ftreg]
							_fmulp();												// fmulp
							_fstp_m64abs(FPR64(FDREG));								// fstp [fdreg]
						}
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x03:
					if (IS_SINGLE(op))	/* DIV.S */
					{
						if (USE_SSE)
						{
							_movss_r128_m32abs(REG_XMM0, FPR32(FSREG));				// movss xmm0,[fsreg]
							_divss_r128_m32abs(REG_XMM0, FPR32(FTREG));				// divss xmm0,[ftreg]
							_movss_m32abs_r128(FPR32(FDREG), REG_XMM0);				// movss [fdreg],xmm0
						}
						else
						{
							_fld_m32abs(FPR32(FSREG));								// fld  [fsreg]
							_fld_m32abs(FPR32(FTREG));								// fld  [ftreg]
							_fdivp();												// fdivp
							_fstp_m32abs(FPR32(FDREG));								// fstp [fdreg]
						}
					}
					else				/* DIV.D */
					{
						if (USE_SSE)
						{
							_movsd_r128_m64abs(REG_XMM0, FPR64(FSREG));				// movsd xmm0,[fsreg]
							_divsd_r128_m64abs(REG_XMM0, FPR64(FTREG));				// divsd xmm0,[ftreg]
							_movsd_m64abs_r128(FPR64(FDREG), REG_XMM0);				// movsd [fdreg],xmm0
						}
						else
						{
							_fld_m64abs(FPR64(FSREG));								// fld  [fsreg]
							_fld_m64abs(FPR64(FTREG));								// fld  [ftreg]
							_fdivp();												// fdivp
							_fstp_m64abs(FPR64(FDREG));								// fstp [fdreg]
						}
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x04:
					if (IS_SINGLE(op))	/* SQRT.S */
					{
						if (USE_SSE)
						{
							_sqrtss_r128_m32abs(REG_XMM0, FPR32(FSREG));			// sqrtss xmm0,[fsreg]
							_movss_m32abs_r128(FPR32(FDREG), REG_XMM0);				// movss [fdreg],xmm0
						}
						else
						{
							_fld_m32abs(FPR32(FSREG));								// fld  [fsreg]
							_fsqrt();												// fsqrt
							_fstp_m32abs(FPR32(FDREG));								// fstp [fdreg]
						}
					}
					else				/* SQRT.D */
					{
						if (USE_SSE)
						{
							_sqrtsd_r128_m64abs(REG_XMM0, FPR64(FSREG));			// sqrtsd xmm0,[fsreg]
							_movsd_m64abs_r128(FPR64(FDREG), REG_XMM0);				// movsd [fdreg],xmm0
						}
						else
						{
							_fld_m64abs(FPR64(FSREG));								// fld  [fsreg]
							_fsqrt();												// fsqrt
							_fstp_m64abs(FPR64(FDREG));								// fstp [fdreg]
						}
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x05:
					if (IS_SINGLE(op))	/* ABS.S */
					{
						_fld_m32abs(FPR32(FSREG));									// fld  [fsreg]
						_fabs();													// fabs
						_fstp_m32abs(FPR32(FDREG));									// fstp [fdreg]
					}
					else				/* ABS.D */
					{
						_fld_m64abs(FPR64(FSREG));									// fld  [fsreg]
						_fabs();													// fabs
						_fstp_m64abs(FPR64(FDREG));									// fstp [fdreg]
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x06:
					if (IS_SINGLE(op))	/* MOV.S */
					{
						_mov_r32_m32abs(REG_EAX, FPR32(FSREG));						// mov  eax,[fsreg]
						_mov_m32abs_r32(FPR32(FDREG), REG_EAX);						// mov  [fdreg],eax
					}
					else				/* MOV.D */
						_mov_m64abs_m64abs(FPR64(FDREG), FPR64(FSREG));
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x07:
					if (IS_SINGLE(op))	/* NEG.S */
					{
						_fld_m32abs(FPR32(FSREG));									// fld  [fsreg]
						_fchs();													// fchs
						_fstp_m32abs(FPR32(FDREG));									// fstp [fdreg]
					}
					else				/* NEG.D */
					{
						_fld_m64abs(FPR64(FSREG));									// fld  [fsreg]
						_fchs();													// fchs
						_fstp_m64abs(FPR64(FDREG));									// fstp [fdreg]
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x08:
					drc_append_set_temp_fp_rounding(drc, FPRND_NEAR);
					if (IS_SINGLE(op))	/* ROUND.L.S */
						_fld_m32abs(FPR32(FSREG));									// fld  [fsreg]
					else				/* ROUND.L.D */
						_fld_m64abs(FPR64(FSREG));									// fld  [fsreg]
					_fistp_m64abs(FPR64(FDREG));									// fistp [fdreg]
					drc_append_restore_fp_rounding(drc);
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x09:
					drc_append_set_temp_fp_rounding(drc, FPRND_CHOP);
					if (IS_SINGLE(op))	/* TRUNC.L.S */
						_fld_m32abs(FPR32(FSREG));									// fld  [fsreg]
					else				/* TRUNC.L.D */
						_fld_m64abs(FPR64(FSREG));									// fld  [fsreg]
					_fistp_m64abs(FPR64(FDREG));									// fistp [fdreg]
					drc_append_restore_fp_rounding(drc);
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x0a:
					drc_append_set_temp_fp_rounding(drc, FPRND_UP);
					if (IS_SINGLE(op))	/* CEIL.L.S */
						_fld_m32abs(FPR32(FSREG));									// fld  [fsreg]
					else				/* CEIL.L.D */
						_fld_m64abs(FPR64(FSREG));									// fld  [fsreg]
					_fistp_m64abs(FPR64(FDREG));									// fistp [fdreg]
					drc_append_restore_fp_rounding(drc);
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x0b:
					drc_append_set_temp_fp_rounding(drc, FPRND_DOWN);
					if (IS_SINGLE(op))	/* FLOOR.L.S */
						_fld_m32abs(FPR32(FSREG));									// fld  [fsreg]
					else				/* FLOOR.L.D */
						_fld_m64abs(FPR64(FSREG));									// fld  [fsreg]
					_fistp_m64abs(FPR64(FDREG));									// fistp [fdreg]
					drc_append_restore_fp_rounding(drc);
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x0c:
					drc_append_set_temp_fp_rounding(drc, FPRND_NEAR);
					if (IS_SINGLE(op))	/* ROUND.W.S */
						_fld_m32abs(FPR32(FSREG));									// fld  [fsreg]
					else				/* ROUND.W.D */
						_fld_m64abs(FPR64(FSREG));									// fld  [fsreg]
					_fistp_m32abs(FPR32(FDREG));									// fistp [fdreg]
					drc_append_restore_fp_rounding(drc);
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x0d:
					drc_append_set_temp_fp_rounding(drc, FPRND_CHOP);
					if (IS_SINGLE(op))	/* TRUNC.W.S */
						_fld_m32abs(FPR32(FSREG));									// fld  [fsreg]
					else				/* TRUNC.W.D */
						_fld_m64abs(FPR64(FSREG));									// fld  [fsreg]
					_fistp_m32abs(FPR32(FDREG));									// fistp [fdreg]
					drc_append_restore_fp_rounding(drc);
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x0e:
					drc_append_set_temp_fp_rounding(drc, FPRND_UP);
					if (IS_SINGLE(op))	/* CEIL.W.S */
						_fld_m32abs(FPR32(FSREG));									// fld  [fsreg]
					else				/* CEIL.W.D */
						_fld_m64abs(FPR64(FSREG));									// fld  [fsreg]
					_fistp_m32abs(FPR32(FDREG));									// fistp [fdreg]
					drc_append_restore_fp_rounding(drc);
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x0f:
					drc_append_set_temp_fp_rounding(drc, FPRND_DOWN);
					if (IS_SINGLE(op))	/* FLOOR.W.S */
						_fld_m32abs(FPR32(FSREG));									// fld  [fsreg]
					else				/* FLOOR.W.D */
						_fld_m64abs(FPR64(FSREG));									// fld  [fsreg]
					_fistp_m32abs(FPR32(FDREG));									// fistp [fdreg]
					drc_append_restore_fp_rounding(drc);
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x11:	/* R5000 */
					if (!mips3.is_mips4)
					{
						_jmp((void *)mips3.generate_invalidop_exception);			// jmp  generate_invalidop_exception
						return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;
					}
					_cmp_m8abs_imm(&mips3.cf[1][(op >> 18) & 7], 0);				// cmp  [cf[x]],0
					_jcc_short_link(((op >> 16) & 1) ? COND_Z : COND_NZ, &link1);	// jz/nz skip
					if (IS_SINGLE(op))	/* MOVT/F.S */
					{
						_mov_r32_m32abs(REG_EAX, FPR32(FSREG));						// mov  eax,[fsreg]
						_mov_m32abs_r32(FPR32(FDREG), REG_EAX);						// mov  [fdreg],eax
					}
					else				/* MOVT/F.D */
						_mov_m64abs_m64abs(FPR64(FDREG), FPR64(FSREG));
					_resolve_link(&link1);											// skip:
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x12:	/* R5000 */
					if (!mips3.is_mips4)
					{
						_jmp((void *)mips3.generate_invalidop_exception);			// jmp  generate_invalidop_exception
						return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;
					}
					_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));				// mov  eax,[rtreg].lo
					_or_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG)+4);				// or   eax,[rtreg].hi
					_jcc_short_link(COND_NZ, &link1);								// jnz  skip
					if (IS_SINGLE(op))	/* MOVZ.S */
					{
						_mov_r32_m32abs(REG_EAX, FPR32(FSREG));						// mov  eax,[fsreg]
						_mov_m32abs_r32(FPR32(FDREG), REG_EAX);						// mov  [fdreg],eax
					}
					else				/* MOVZ.D */
						_mov_m64abs_m64abs(FPR64(FDREG), FPR64(FSREG));
					_resolve_link(&link1);											// skip:
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x13:	/* R5000 */
					if (!mips3.is_mips4)
					{
						_jmp((void *)mips3.generate_invalidop_exception);			// jmp  generate_invalidop_exception
						return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;
					}
					_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));				// mov  eax,[rtreg].lo
					_or_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG)+4);				// or   eax,[rtreg].hi
					_jcc_short_link(COND_Z, &link1);								// jz   skip
					if (IS_SINGLE(op))	/* MOVN.S */
					{
						_mov_r32_m32abs(REG_EAX, FPR32(FSREG));						// mov  eax,[fsreg]
						_mov_m32abs_r32(FPR32(FDREG), REG_EAX);						// mov  [fdreg],eax
					}
					else				/* MOVN.D */
						_mov_m64abs_m64abs(FPR64(FDREG), FPR64(FSREG));
					_resolve_link(&link1);											// skip:
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x15:	/* R5000 */
					if (!mips3.is_mips4)
					{
						_jmp((void *)mips3.generate_invalidop_exception);			// jmp  generate_invalidop_exception
						return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;
					}
					_fld1();														// fld1
					if (IS_SINGLE(op))	/* RECIP.S */
					{
						_fld_m32abs(FPR32(FSREG));									// fld  [fsreg]
						_fdivp();													// fdivp
						_fstp_m32abs(FPR32(FDREG));									// fstp [fdreg]
					}
					else				/* RECIP.D */
					{
						_fld_m64abs(FPR64(FSREG));									// fld  [fsreg]
						_fdivp();													// fdivp
						_fstp_m64abs(FPR64(FDREG));									// fstp [fdreg]
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x16:	/* R5000 */
					if (!mips3.is_mips4)
					{
						_jmp((void *)mips3.generate_invalidop_exception);			// jmp  generate_invalidop_exception
						return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;
					}
					_fld1();														// fld1
					if (IS_SINGLE(op))	/* RSQRT.S */
					{
						_fld_m32abs(FPR32(FSREG));									// fld  [fsreg]
						_fsqrt();													// fsqrt
						_fdivp();													// fdivp
						_fstp_m32abs(FPR32(FDREG));									// fstp [fdreg]
					}
					else				/* RSQRT.D */
					{
						_fld_m64abs(FPR64(FSREG));									// fld  [fsreg]
						_fsqrt();													// fsqrt
						_fdivp();													// fdivp
						_fstp_m64abs(FPR64(FDREG));									// fstp [fdreg]
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x20:
					if (IS_INTEGRAL(op))
					{
						if (IS_SINGLE(op))	/* CVT.S.W */
							_fild_m32abs(FPR32(FSREG));								// fild [fsreg]
						else				/* CVT.S.L */
							_fild_m64abs(FPR64(FSREG));								// fild [fsreg]
					}
					else					/* CVT.S.D */
						_fld_m64abs(FPR64(FSREG));									// fld  [fsreg]
					_fstp_m32abs(FPR32(FDREG));										// fstp [fdreg]
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x21:
					if (IS_INTEGRAL(op))
					{
						if (IS_SINGLE(op))	/* CVT.D.W */
							_fild_m32abs(FPR32(FSREG));								// fild [fsreg]
						else				/* CVT.D.L */
							_fild_m64abs(FPR64(FSREG));								// fild [fsreg]
					}
					else					/* CVT.D.S */
						_fld_m32abs(FPR32(FSREG));									// fld  [fsreg]
					_fstp_m64abs(FPR64(FDREG));										// fstp [fdreg]
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x24:
					if (IS_SINGLE(op))	/* CVT.W.S */
						_fld_m32abs(FPR32(FSREG));									// fld  [fsreg]
					else				/* CVT.W.D */
						_fld_m64abs(FPR64(FSREG));									// fld  [fsreg]
					_fistp_m32abs(FPR32(FDREG));									// fistp [fdreg]
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x25:
					if (IS_SINGLE(op))	/* CVT.L.S */
						_fld_m32abs(FPR32(FSREG));									// fld  [fsreg]
					else				/* CVT.L.D */
						_fld_m64abs(FPR64(FSREG));									// fld  [fsreg]
					_fistp_m64abs(FPR64(FDREG));									// fistp [fdreg]
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x30:
				case 0x38:
					_mov_m8abs_imm(&mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0], 0);	/* C.F.S/D */
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x31:
				case 0x39:
					_mov_m8abs_imm(&mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0], 0);	/* C.UN.S/D */
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x32:
				case 0x3a:
					if (USE_SSE)
					{
						if (IS_SINGLE(op))	/* C.EQ.S */
						{
							_movss_r128_m32abs(REG_XMM0, FPR32(FSREG));				// movss xmm0,[fsreg]
							_comiss_r128_m32abs(REG_XMM0, FPR32(FTREG));			// comiss xmm0,[ftreg]
						}
						else
						{
							_movsd_r128_m64abs(REG_XMM0, FPR64(FSREG));				// movsd xmm0,[fsreg]
							_comisd_r128_m64abs(REG_XMM0, FPR64(FTREG));			// comisd xmm0,[ftreg]
						}
						_setcc_m8abs(COND_E, &mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0]); // sete [cf[x]]
					}
					else
					{
						if (IS_SINGLE(op))	/* C.EQ.S */
						{
							_fld_m32abs(FPR32(FTREG));								// fld  [ftreg]
							_fld_m32abs(FPR32(FSREG));								// fld  [fsreg]
						}
						else
						{
							_fld_m64abs(FPR64(FTREG));								// fld  [ftreg]
							_fld_m64abs(FPR64(FSREG));								// fld  [fsreg]
						}
						_fcompp();													// fcompp
						_fnstsw_ax();												// fnstsw ax
						_sahf();													// sahf
						_setcc_m8abs(COND_E, &mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0]); // sete [cf[x]]
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x33:
				case 0x3b:
					if (USE_SSE)
					{
						if (IS_SINGLE(op))	/* C.UEQ.S */
						{
							_movss_r128_m32abs(REG_XMM0, FPR32(FSREG));				// movss xmm0,[fsreg]
							_ucomiss_r128_m32abs(REG_XMM0, FPR32(FTREG));			// ucomiss xmm0,[ftreg]
						}
						else
						{
							_movsd_r128_m64abs(REG_XMM0, FPR64(FSREG));				// movsd xmm0,[fsreg]
							_ucomisd_r128_m64abs(REG_XMM0, FPR64(FTREG));			// ucomisd xmm0,[ftreg]
						}
						_setcc_m8abs(COND_E, &mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0]); // sete [cf[x]]
					}
					else
					{
						if (IS_SINGLE(op))	/* C.UEQ.S */
						{
							_fld_m32abs(FPR32(FTREG));								// fld  [ftreg]
							_fld_m32abs(FPR32(FSREG));								// fld  [fsreg]
						}
						else
						{
							_fld_m64abs(FPR64(FTREG));								// fld  [ftreg]
							_fld_m64abs(FPR64(FSREG));								// fld  [fsreg]
						}
						_fucompp();													// fucompp
						_fnstsw_ax();												// fnstsw ax
						_sahf();													// sahf
						_setcc_m8abs(COND_E, &mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0]); // sete [cf[x]]
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x34:
				case 0x3c:
					if (USE_SSE)
					{
						if (IS_SINGLE(op))	/* C.OLT.S */
						{
							_movss_r128_m32abs(REG_XMM0, FPR32(FSREG));				// movss xmm0,[fsreg]
							_comiss_r128_m32abs(REG_XMM0, FPR32(FTREG));			// comiss xmm0,[ftreg]
						}
						else
						{
							_movsd_r128_m64abs(REG_XMM0, FPR64(FSREG));				// movsd xmm0,[fsreg]
							_comisd_r128_m64abs(REG_XMM0, FPR64(FTREG));			// comisd xmm0,[ftreg]
						}
						_setcc_m8abs(COND_B, &mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0]); // setb [cf[x]]
					}
					else
					{
						if (IS_SINGLE(op))	/* C.OLT.S */
						{
							_fld_m32abs(FPR32(FTREG));								// fld  [ftreg]
							_fld_m32abs(FPR32(FSREG));								// fld  [fsreg]
						}
						else
						{
							_fld_m64abs(FPR64(FTREG));								// fld  [ftreg]
							_fld_m64abs(FPR64(FSREG));								// fld  [fsreg]
						}
						_fcompp();													// fcompp
						_fnstsw_ax();												// fnstsw ax
						_sahf();													// sahf
						_setcc_m8abs(COND_B, &mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0]); // setb [cf[x]]
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x35:
				case 0x3d:
					if (USE_SSE)
					{
						if (IS_SINGLE(op))	/* C.ULT.S */
						{
							_movss_r128_m32abs(REG_XMM0, FPR32(FSREG));				// movss xmm0,[fsreg]
							_ucomiss_r128_m32abs(REG_XMM0, FPR32(FTREG));			// ucomiss xmm0,[ftreg]
						}
						else
						{
							_movsd_r128_m64abs(REG_XMM0, FPR64(FSREG));				// movsd xmm0,[fsreg]
							_ucomisd_r128_m64abs(REG_XMM0, FPR64(FTREG));			// ucomisd xmm0,[ftreg]
						}
						_setcc_m8abs(COND_B, &mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0]); // setb [cf[x]]
					}
					else
					{
						if (IS_SINGLE(op))	/* C.ULT.S */
						{
							_fld_m32abs(FPR32(FTREG));								// fld  [ftreg]
							_fld_m32abs(FPR32(FSREG));								// fld  [fsreg]
						}
						else
						{
							_fld_m64abs(FPR64(FTREG));								// fld  [ftreg]
							_fld_m64abs(FPR64(FSREG));								// fld  [fsreg]
						}
						_fucompp();													// fucompp
						_fnstsw_ax();												// fnstsw ax
						_sahf();													// sahf
						_setcc_m8abs(COND_B, &mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0]); // setb [cf[x]]
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x36:
				case 0x3e:
					if (USE_SSE)
					{
						if (IS_SINGLE(op))	/* C.OLE.S */
						{
							_movss_r128_m32abs(REG_XMM0, FPR32(FSREG));				// movss xmm0,[fsreg]
							_comiss_r128_m32abs(REG_XMM0, FPR32(FTREG));			// comiss xmm0,[ftreg]
						}
						else
						{
							_movsd_r128_m64abs(REG_XMM0, FPR64(FSREG));				// movsd xmm0,[fsreg]
							_comisd_r128_m64abs(REG_XMM0, FPR64(FTREG));			// comisd xmm0,[ftreg]
						}
						_setcc_m8abs(COND_BE, &mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0]); // setle [cf[x]]
					}
					else
					{
						if (IS_SINGLE(op))	/* C.OLE.S */
						{
							_fld_m32abs(FPR32(FTREG));								// fld  [ftreg]
							_fld_m32abs(FPR32(FSREG));								// fld  [fsreg]
						}
						else
						{
							_fld_m64abs(FPR64(FTREG));								// fld  [ftreg]
							_fld_m64abs(FPR64(FSREG));								// fld  [fsreg]
						}
						_fcompp();													// fcompp
						_fnstsw_ax();												// fnstsw ax
						_sahf();													// sahf
						_setcc_m8abs(COND_BE, &mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0]); // setbe [cf[x]]
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x37:
				case 0x3f:
					if (USE_SSE)
					{
						if (IS_SINGLE(op))	/* C.ULE.S */
						{
							_movss_r128_m32abs(REG_XMM0, FPR32(FSREG));				// movss xmm0,[fsreg]
							_ucomiss_r128_m32abs(REG_XMM0, FPR32(FTREG));			// ucomiss xmm0,[ftreg]
						}
						else
						{
							_movsd_r128_m64abs(REG_XMM0, FPR64(FSREG));				// movsd xmm0,[fsreg]
							_ucomisd_r128_m64abs(REG_XMM0, FPR64(FTREG));			// ucomisd xmm0,[ftreg]
						}
						_setcc_m8abs(COND_BE, &mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0]); // setl [cf[x]]
					}
					else
					{
						if (IS_SINGLE(op))	/* C.ULE.S */
						{
							_fld_m32abs(FPR32(FTREG));								// fld  [ftreg]
							_fld_m32abs(FPR32(FSREG));								// fld  [fsreg]
						}
						else
						{
							_fld_m64abs(FPR64(FTREG));								// fld  [ftreg]
							_fld_m64abs(FPR64(FSREG));								// fld  [fsreg]
						}
						_fucompp();													// fucompp
						_fnstsw_ax();												// fnstsw ax
						_sahf();													// sahf
						_setcc_m8abs(COND_BE, &mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0]); // setbe [cf[x]]
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);
			}
			break;
	}
	_jmp((void *)mips3.generate_invalidop_exception);								// jmp  generate_invalidop_exception
	return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;
}



/***************************************************************************
    COP1X RECOMPILATION
***************************************************************************/

/*------------------------------------------------------------------
    recompile_cop1x
------------------------------------------------------------------*/

static UINT32 recompile_cop1x(drc_core *drc, UINT32 pc, UINT32 op)
{
	if (mips3.drcoptions & MIPS3DRC_STRICT_COP1)
	{
		_test_m32abs_imm(&mips3.cpr[0][COP0_Status], SR_COP1);						// test [mips3.cpr[0][COP0_Status]],SR_COP1
		_jcc(COND_Z, mips3.generate_cop_exception);									// jz   generate_cop_exception
	}

	switch (op & 0x3f)
	{
		case 0x00:		/* LWXC1 */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov  [mips3_icount],ebp
			_save_pc_before_call();													// save pc
			_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));						// mov  eax,[rsreg]
			_add_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));						// add  eax,[rtreg]
			_push_r32(REG_EAX);														// push eax
			_call(mips3.read_and_translate_long);									// call read_and_translate_long
			_add_r32_imm(REG_ESP, 4);												// add  esp,4
			_mov_m32abs_r32(FPR32(FDREG), REG_EAX);									// mov  [fdreg],eax
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov  ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x01:		/* LDXC1 */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov  [mips3_icount],ebp
			_save_pc_before_call();													// save pc
			_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));						// mov  eax,[rsreg]
			_add_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));						// add  eax,[rtreg]
			_push_r32(REG_EAX);														// push eax
			_call(mips3.read_and_translate_double);									// call read_and_translate_double
			_mov_m32abs_r32(LO(FPR64(FDREG)), REG_EAX);								// mov  [fdreg].lo,eax
			_mov_m32abs_r32(HI(FPR64(FDREG)), REG_EDX);								// mov  [fdreg].hi,edx
			_add_r32_imm(REG_ESP, 4);												// add  esp,4
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov  ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x08:		/* SWXC1 */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov  [mips3_icount],ebp
			_save_pc_before_call();													// save pc
			_push_m32abs(FPR32(FSREG));												// push [fsreg]
			_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));						// mov  eax,[rsreg]
			_add_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));						// add  eax,[rtreg]
			_push_r32(REG_EAX);														// push eax
			_call(mips3.write_and_translate_long);									// call write_and_translate_long
			_add_r32_imm(REG_ESP, 8);												// add  esp,8
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov  ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x09:		/* SDXC1 */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								// mov  [mips3_icount],ebp
			_save_pc_before_call();													// save pc
			_push_m32abs(HI(FPR64(FSREG)));											// push [fsreg].hi
			_push_m32abs(LO(FPR64(FSREG)));											// push [fsreg].lo
			_mov_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RSREG));						// mov  eax,[rsreg]
			_add_r32_m32bd(REG_EAX, REG_ESI, REGDISP(RTREG));						// add  eax,[rtreg]
			_call(mips3.write_and_translate_double);								// call write_and_translate_double
			_add_r32_imm(REG_ESP, 12);												// add  esp,12
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								// mov  ebp,[mips3_icount]
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x0f:		/* PREFX */
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x20:		/* MADD.S */
			if (USE_SSE)
			{
				_movss_r128_m32abs(REG_XMM0, FPR32(FSREG));							// movss xmm0,[fsreg]
				_mulss_r128_m32abs(REG_XMM0, FPR32(FTREG));							// mulss xmm0,[ftreg]
				_addss_r128_m32abs(REG_XMM0, FPR32(FRREG));							// addss xmm0,[frreg]
				_movss_m32abs_r128(FPR32(FDREG), REG_XMM0);							// movss [fdreg],xmm0
			}
			else
			{
				_fld_m32abs(FPR32(FRREG));											// fld  [frreg]
				_fld_m32abs(FPR32(FSREG));											// fld  [fsreg]
				_fld_m32abs(FPR32(FTREG));											// fld  [ftreg]
				_fmulp();															// fmulp
				_faddp();															// faddp
				_fstp_m32abs(FPR32(FDREG));											// fstp [fdreg]
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x21:		/* MADD.D */
			if (USE_SSE)
			{
				_movsd_r128_m64abs(REG_XMM0, FPR64(FSREG));							// movsd xmm0,[fsreg]
				_mulsd_r128_m64abs(REG_XMM0, FPR64(FTREG));							// mulsd xmm0,[ftreg]
				_addsd_r128_m64abs(REG_XMM0, FPR64(FRREG));							// addsd xmm0,[frreg]
				_movsd_m64abs_r128(FPR64(FDREG), REG_XMM0);							// movsd [fdreg],xmm0
			}
			else
			{
				_fld_m64abs(FPR64(FRREG));											// fld  [frreg]
				_fld_m64abs(FPR64(FSREG));											// fld  [fsreg]
				_fld_m64abs(FPR64(FTREG));											// fld  [ftreg]
				_fmulp();															// fmulp
				_faddp();															// faddp
				_fstp_m64abs(FPR64(FDREG));											// fstp [fdreg]
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x28:		/* MSUB.S */
			if (USE_SSE)
			{
				_movss_r128_m32abs(REG_XMM0, FPR32(FSREG));							// movss xmm0,[fsreg]
				_mulss_r128_m32abs(REG_XMM0, FPR32(FTREG));							// mulss xmm0,[ftreg]
				_subss_r128_m32abs(REG_XMM0, FPR32(FRREG));							// subss xmm0,[frreg]
				_movss_m32abs_r128(FPR32(FDREG), REG_XMM0);							// movss [fdreg],xmm0
			}
			else
			{
				_fld_m32abs(FPR32(FRREG));											// fld  [frreg]
				_fld_m32abs(FPR32(FSREG));											// fld  [fsreg]
				_fld_m32abs(FPR32(FTREG));											// fld  [ftreg]
				_fmulp();															// fmulp
				_fsubrp();															// fsubrp
				_fstp_m32abs(FPR32(FDREG));											// fstp [fdreg]
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x29:		/* MSUB.D */
			if (USE_SSE)
			{
				_movsd_r128_m64abs(REG_XMM0, FPR64(FSREG));							// movsd xmm0,[fsreg]
				_mulsd_r128_m64abs(REG_XMM0, FPR64(FTREG));							// mulsd xmm0,[ftreg]
				_subsd_r128_m64abs(REG_XMM0, FPR64(FRREG));							// subsd xmm0,[frreg]
				_movsd_m64abs_r128(FPR64(FDREG), REG_XMM0);							// movsd [fdreg],xmm0
			}
			else
			{
				_fld_m64abs(FPR64(FRREG));											// fld  [frreg]
				_fld_m64abs(FPR64(FSREG));											// fld  [fsreg]
				_fld_m64abs(FPR64(FTREG));											// fld  [ftreg]
				_fmulp();															// fmulp
				_fsubrp();															// fsubrp
				_fstp_m64abs(FPR64(FDREG));											// fstp [fdreg]
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x30:		/* NMADD.S */
			if (USE_SSE)
			{
				_pxor_r128_r128(REG_XMM1, REG_XMM1);								// pxor xmm1,xmm1
				_movss_r128_m32abs(REG_XMM0, FPR32(FSREG));							// movss xmm0,[fsreg]
				_mulss_r128_m32abs(REG_XMM0, FPR32(FTREG));							// mulss xmm0,[ftreg]
				_addss_r128_m32abs(REG_XMM0, FPR32(FRREG));							// addss xmm0,[frreg]
				_subss_r128_r128(REG_XMM1, REG_XMM0);								// subss xmm1,xmm0
				_movss_m32abs_r128(FPR32(FDREG), REG_XMM1);							// movss [fdreg],xmm1
			}
			else
			{
				_fld_m32abs(FPR32(FRREG));											// fld  [frreg]
				_fld_m32abs(FPR32(FSREG));											// fld  [fsreg]
				_fld_m32abs(FPR32(FTREG));											// fld  [ftreg]
				_fmulp();															// fmulp
				_faddp();															// faddp
				_fchs();															// fchs
				_fstp_m32abs(FPR32(FDREG));											// fstp [fdreg]
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x31:		/* NMADD.D */
			if (USE_SSE)
			{
				_pxor_r128_r128(REG_XMM1, REG_XMM1);								// pxor xmm1,xmm1
				_movsd_r128_m64abs(REG_XMM0, FPR64(FSREG));							// movsd xmm0,[fsreg]
				_mulsd_r128_m64abs(REG_XMM0, FPR64(FTREG));							// mulsd xmm0,[ftreg]
				_addsd_r128_m64abs(REG_XMM0, FPR64(FRREG));							// addsd xmm0,[frreg]
				_subss_r128_r128(REG_XMM1, REG_XMM0);								// subss xmm1,xmm0
				_movsd_m64abs_r128(FPR64(FDREG), REG_XMM1);							// movsd [fdreg],xmm1
			}
			else
			{
				_fld_m64abs(FPR64(FRREG));											// fld  [frreg]
				_fld_m64abs(FPR64(FSREG));											// fld  [fsreg]
				_fld_m64abs(FPR64(FTREG));											// fld  [ftreg]
				_fmulp();															// fmulp
				_faddp();															// faddp
				_fchs();															// fchs
				_fstp_m64abs(FPR64(FDREG));											// fstp [fdreg]
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x38:		/* NMSUB.S */
			if (USE_SSE)
			{
				_movss_r128_m32abs(REG_XMM0, FPR32(FSREG));							// movss xmm0,[fsreg]
				_mulss_r128_m32abs(REG_XMM0, FPR32(FTREG));							// mulss xmm0,[ftreg]
				_movss_r128_m32abs(REG_XMM1, FPR32(FRREG));							// movss xmm1,[frreg]
				_subss_r128_r128(REG_XMM1, REG_XMM0);								// subss xmm1,xmm0
				_movss_m32abs_r128(FPR32(FDREG), REG_XMM1);							// movss [fdreg],xmm1
			}
			else
			{
				_fld_m32abs(FPR32(FRREG));											// fld  [frreg]
				_fld_m32abs(FPR32(FSREG));											// fld  [fsreg]
				_fld_m32abs(FPR32(FTREG));											// fld  [ftreg]
				_fmulp();															// fmulp
				_fsubp();															// fsubp
				_fstp_m32abs(FPR32(FDREG));											// fstp [fdreg]
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x39:		/* NMSUB.D */
			if (USE_SSE)
			{
				_movsd_r128_m64abs(REG_XMM0, FPR64(FSREG));							// movsd xmm0,[fsreg]
				_mulsd_r128_m64abs(REG_XMM0, FPR64(FTREG));							// mulsd xmm0,[ftreg]
				_movsd_r128_m64abs(REG_XMM1, FPR64(FRREG));							// movsd xmm1,[frreg]
				_subss_r128_r128(REG_XMM1, REG_XMM0);								// subss xmm1,xmm0
				_movsd_m64abs_r128(FPR64(FDREG), REG_XMM1);							// movsd [fdreg],xmm1
			}
			else
			{
				_fld_m64abs(FPR64(FRREG));											// fld  [frreg]
				_fld_m64abs(FPR64(FSREG));											// fld  [fsreg]
				_fld_m64abs(FPR64(FTREG));											// fld  [ftreg]
				_fmulp();															// fmulp
				_fsubp();															// fsubrp
				_fstp_m64abs(FPR64(FDREG));											// fstp [fdreg]
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

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
	_jmp((void *)mips3.generate_invalidop_exception);								// jmp  generate_invalidop_exception
	return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;
}
