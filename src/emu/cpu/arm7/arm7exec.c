/*****************************************************************************
 *
 *   arm7exec.c
 *   Portable ARM7TDMI Core Emulator
 *
 *   Copyright (c) 2004 Steve Ellenoff, all rights reserved.
 *
 *   - This source code is released as freeware for non-commercial purposes.
 *   - You are free to use and redistribute this code in modified or
 *     unmodified form, provided you list me in the credits.
 *   - If you modify this source code, you must add a notice to each modified
 *     source file that it has been changed.  If you're a nice person, you
 *     will clearly mark each change too.  :)
 *   - If you wish to use this for commercial purposes, please contact me at
 *     sellenoff@hotmail.com
 *   - The author of this copywritten work reserves the right to change the
 *     terms of its usage and license at any time, including retroactively
 *   - This entire notice must remain in the source code.
 *
 *  This work is based on:
 *  #1) 'Atmel Corporation ARM7TDMI (Thumb) Datasheet - January 1999'
 *  #2) Arm 2/3/6 emulator By Bryan McPhail (bmcphail@tendril.co.uk) and Phil Stroffolino (MAME CORE 0.76)
 *
 *****************************************************************************/

/******************************************************************************
 *  Notes:
 *         This file contains the code to run during the CPU EXECUTE METHOD.
 *         It has been split into it's own file (from the arm7core.c) so it can be
 *         directly compiled into any cpu core that wishes to use it.
 *
 *         It should be included as follows in your cpu core:
 *
 *         int arm7_execute( int cycles )
 *         {
 *         #include "arm7exec.c"
 *         }
 *
*****************************************************************************/

/* This implementation uses an improved switch() for hopefully faster opcode fetches compared to my last version
.. though there's still room for improvement. */
{
    UINT32 pc;
    UINT32 insn;

    ARM7_ICOUNT = cycles;
    do
    {
    	CALL_MAME_DEBUG;

	/* handle Thumb instructions if active */
	if( T_IS_SET(GET_CPSR) )
	{
		UINT32 readword;
		UINT32 addr;
		UINT32 rm, rn, rs, rd, op2, imm, rrs;
		INT32 offs;
		pc = R15;
		insn = cpu_readop16(pc & (~1));
		ARM7_ICOUNT += (3 - thumbCycles[insn >> 8]);
		switch( ( insn & THUMB_INSN_TYPE ) >> THUMB_INSN_TYPE_SHIFT )
		{
			case 0x0: /* Logical shifting */
				SET_CPSR(GET_CPSR &~ (N_MASK | Z_MASK));
				if( insn & THUMB_SHIFT_R ) /* Shift right */
				{
					rs = ( insn & THUMB_ADDSUB_RS ) >> THUMB_ADDSUB_RS_SHIFT;
					rd = ( insn & THUMB_ADDSUB_RD ) >> THUMB_ADDSUB_RD_SHIFT;
					rrs = GET_REGISTER(rs);
					offs = ( insn & THUMB_SHIFT_AMT ) >> THUMB_SHIFT_AMT_SHIFT;
					VERBOSELOG((2,"%08x:  Thumb instruction: LSR R%d, R%d (%08x), %d\n", pc, rd, rs, GET_REGISTER(rs), offs));
					R15 += 2;
					if( offs == 0 )
					{
						offs = 32;
					}
					SET_REGISTER( rd, GET_REGISTER(rs) >> offs );
					SET_CPSR(GET_CPSR | Z_MASK);
					if( rrs & ( 1 << (offs-1) ) )
					{
						SET_CPSR(GET_CPSR | C_MASK);
					}
					else
					{
						SET_CPSR(GET_CPSR &~ C_MASK);
					}
					SET_CPSR(GET_CPSR &~ ( Z_MASK | N_MASK ) );
					SET_CPSR( GET_CPSR | HandleALUNZFlags( GET_REGISTER(rd) ) );
				}
				else /* Shift left */
				{
					rs = ( insn & THUMB_ADDSUB_RS ) >> THUMB_ADDSUB_RS_SHIFT;
					rd = ( insn & THUMB_ADDSUB_RD ) >> THUMB_ADDSUB_RD_SHIFT;
					rrs = GET_REGISTER(rs);
					offs = ( insn & THUMB_SHIFT_AMT ) >> THUMB_SHIFT_AMT_SHIFT;
					VERBOSELOG((2,"%08x:  Thumb instruction: LSL R%d, R%d (%08x), %d\n", pc, rd, rs, GET_REGISTER(rs), offs));
					R15 += 2;
					if( offs == 0 )
					{
						SET_REGISTER( rd, 0 );
						SET_CPSR(GET_CPSR | Z_MASK);
						if( rrs & 1 )
						{
							SET_CPSR(GET_CPSR | C_MASK);
						}
						else
						{
							SET_CPSR(GET_CPSR &~ C_MASK);
						}
					}
					else
					{
						SET_REGISTER( rd, GET_REGISTER(rs) << offs );
						if( !GET_REGISTER(rd) )
						{
							SET_CPSR(GET_CPSR | Z_MASK);
						}
						if( GET_REGISTER(rd) & ( 1 << 31 ) )
						{
							SET_CPSR(GET_CPSR | N_MASK);
						}
					}
					SET_CPSR( GET_CPSR &~ ( Z_MASK | N_MASK ) );
					SET_CPSR( GET_CPSR | HandleALUNZFlags( GET_REGISTER(rd) ) );
				}
				break;
			case 0x1: /* Arithmetic */
				if( insn & THUMB_INSN_ADDSUB )
				{
					switch( ( insn & THUMB_ADDSUB_TYPE ) >> THUMB_ADDSUB_TYPE_SHIFT )
					{
						case 0x0: /* ADD Rd, Rs, Rn */
							rn = GET_REGISTER( ( insn & THUMB_ADDSUB_RNIMM ) >> THUMB_ADDSUB_RNIMM_SHIFT );
							rs = GET_REGISTER( ( insn & THUMB_ADDSUB_RS ) >> THUMB_ADDSUB_RS_SHIFT );
							rd = ( insn & THUMB_ADDSUB_RD ) >> THUMB_ADDSUB_RD_SHIFT;
							SET_REGISTER( rd, rs + rn );
							VERBOSELOG((2,"%08x:  Thumb instruction: ADD R%d, R%d (%08x), R%d (%08x)\n", pc, rd, ( insn & THUMB_ADDSUB_RS ) >> THUMB_ADDSUB_RS_SHIFT, rs, ( insn & THUMB_ADDSUB_RNIMM ) >> THUMB_ADDSUB_RNIMM_SHIFT, rn));
							HandleThumbALUAddFlags( GET_REGISTER(rd), rs, rn );
							break;
						case 0x1: /* SUB Rd, Rs, Rn */
							rn = GET_REGISTER( ( insn & THUMB_ADDSUB_RNIMM ) >> THUMB_ADDSUB_RNIMM_SHIFT );
							rs = GET_REGISTER( ( insn & THUMB_ADDSUB_RS ) >> THUMB_ADDSUB_RS_SHIFT );
							rd = ( insn & THUMB_ADDSUB_RD ) >> THUMB_ADDSUB_RD_SHIFT;
							SET_REGISTER( rd, rs - rn );
							VERBOSELOG((2,"%08x:  Thumb instruction: SUB R%d, R%d (%08x), R%d (%08x)\n", pc, rd, ( insn & THUMB_ADDSUB_RS ) >> THUMB_ADDSUB_RS_SHIFT, rs, ( insn & THUMB_ADDSUB_RNIMM ) >> THUMB_ADDSUB_RNIMM_SHIFT, rn));
							HandleThumbALUSubFlags( GET_REGISTER(rd), rs, rn );
							break;
						case 0x2: /* ADD Rd, Rs, #imm */
							imm = ( insn & THUMB_ADDSUB_RNIMM ) >> THUMB_ADDSUB_RNIMM_SHIFT;
							rs = GET_REGISTER( ( insn & THUMB_ADDSUB_RS ) >> THUMB_ADDSUB_RS_SHIFT );
							rd = ( insn & THUMB_ADDSUB_RD ) >> THUMB_ADDSUB_RD_SHIFT;
							VERBOSELOG((2,"%08x:  Thumb instruction: ADD R%d, R%d (%08x), #%d\n", pc, rd, ( insn & THUMB_ADDSUB_RS ) >> THUMB_ADDSUB_RS_SHIFT, rs, imm));
							SET_REGISTER( rd, rs + imm );
							HandleThumbALUAddFlags( GET_REGISTER(rd), rs, imm );
							break;
						case 0x3: /* SUB Rd, Rs, #imm */
							imm = ( insn & THUMB_ADDSUB_RNIMM ) >> THUMB_ADDSUB_RNIMM_SHIFT;
							rs = GET_REGISTER( ( insn & THUMB_ADDSUB_RS ) >> THUMB_ADDSUB_RS_SHIFT );
							rd = ( insn & THUMB_ADDSUB_RD ) >> THUMB_ADDSUB_RD_SHIFT;
							VERBOSELOG((2,"%08x:  Thumb instruction: SUB R%d, R%d (%08x), #%d\n", pc, rd, ( insn & THUMB_ADDSUB_RS ) >> THUMB_ADDSUB_RS_SHIFT, rs, imm));
							SET_REGISTER( rd, rs - imm );
							HandleThumbALUSubFlags( GET_REGISTER(rd), rs,imm );
							break;
						default:
							fatalerror("%08x:  Undefined Thumb instruction: %04x\n", pc, insn);
							R15 += 2;
							break;
					}
				}
				else
				{
					/* ASR.. */
					//if( insn & THUMB_SHIFT_R ) /* Shift right */
					{
						rs = ( insn & THUMB_ADDSUB_RS ) >> THUMB_ADDSUB_RS_SHIFT;
						rd = ( insn & THUMB_ADDSUB_RD ) >> THUMB_ADDSUB_RD_SHIFT;
						rrs = GET_REGISTER(rs);
						offs = ( insn & THUMB_SHIFT_AMT ) >> THUMB_SHIFT_AMT_SHIFT;
						VERBOSELOG((2,"%08x:  Thumb instruction: LSR R%d, R%d (%08x), %d\n", pc, rd, rs, GET_REGISTER(rs), offs));
						R15 += 2;
						if( offs == 0 )
						{
							offs = 32;
						}
						SET_REGISTER( rd, (INT32)(GET_REGISTER(rs)) >> offs );
						if( rrs & ( 1 << (offs-1) ) )
						{
							SET_CPSR(GET_CPSR | C_MASK);
						}
						else
						{
							SET_CPSR(GET_CPSR &~ C_MASK);
						}
						SET_CPSR(GET_CPSR &~ (N_MASK | Z_MASK));
						SET_CPSR( GET_CPSR | HandleALUNZFlags( GET_REGISTER(rd) ) );
					}

				}
				break;
			case 0x2: /* CMP / MOV */
				if( insn & THUMB_INSN_CMP )
				{
					rn = GET_REGISTER( ( insn & THUMB_INSN_IMM_RD ) >> THUMB_INSN_IMM_RD_SHIFT );
					op2 = ( insn & THUMB_INSN_IMM );
					rd = rn - op2;
					HandleThumbALUSubFlags( rd, rn, op2 );
					//mame_printf_debug("%08x: xxx Thumb instruction: CMP R%d (%08x), %02x (N=%d, Z=%d, C=%d, V=%d)\n", pc, ( insn & THUMB_INSN_IMM_RD ) >> THUMB_INSN_IMM_RD_SHIFT, GET_REGISTER( ( insn & THUMB_INSN_IMM_RD ) >> THUMB_INSN_IMM_RD_SHIFT ), op2, N_IS_SET(GET_CPSR) ? 1 : 0, Z_IS_SET(GET_CPSR) ? 1 : 0, C_IS_SET(GET_CPSR) ? 1 : 0, V_IS_SET(GET_CPSR) ? 1 : 0);
					//VERBOSELOG((2,"%08x:  Thumb instruction: CMP R%d (%08x), %02x (N=%d, Z=%d, C=%d, V=%d)\n", pc, ( insn & THUMB_INSN_IMM_RD ) >> THUMB_INSN_IMM_RD_SHIFT, GET_REGISTER( ( insn & THUMB_INSN_IMM_RD ) >> THUMB_INSN_IMM_RD_SHIFT ), op2, N_IS_SET(GET_CPSR) ? 1 : 0, Z_IS_SET(GET_CPSR) ? 1 : 0, C_IS_SET(GET_CPSR) ? 1 : 0, V_IS_SET(GET_CPSR) ? 1 : 0));
				}
				else
				{
					rd = ( insn & THUMB_INSN_IMM_RD ) >> THUMB_INSN_IMM_RD_SHIFT;
					op2 = ( insn & THUMB_INSN_IMM );
					SET_REGISTER( rd, op2 );
					SET_CPSR( GET_CPSR &~ ( Z_MASK | N_MASK ) );
					SET_CPSR( GET_CPSR | HandleALUNZFlags( GET_REGISTER(rd) ) );
					VERBOSELOG((2,"%08x:  Thumb instruction: MOV R%d, %02x\n", pc, rd, op2));
					R15 += 2;
				}
				break;
			case 0x3: /* ADD/SUB immediate */
				if( insn & THUMB_INSN_SUB ) /* SUB Rd, #Offset8 */
				{
					rn = GET_REGISTER( ( insn & THUMB_INSN_IMM_RD ) >> THUMB_INSN_IMM_RD_SHIFT );
					op2 = ( insn & THUMB_INSN_IMM );
					//VERBOSELOG((2,"%08x:  Thumb instruction: SUB R%d, %02x\n", pc, ( insn & THUMB_INSN_IMM_RD ) >> THUMB_INSN_IMM_RD_SHIFT, op2));
					//mame_printf_debug("%08x:  Thumb instruction: SUB R%d, %02x\n", pc, ( insn & THUMB_INSN_IMM_RD ) >> THUMB_INSN_IMM_RD_SHIFT, op2);
					rd = rn - op2;
					SET_REGISTER( ( insn & THUMB_INSN_IMM_RD ) >> THUMB_INSN_IMM_RD_SHIFT, rd );
					HandleThumbALUSubFlags( rd, rn, op2 );
				}
				else /* ADD Rd, #Offset8 */
				{
					rn = GET_REGISTER( ( insn & THUMB_INSN_IMM_RD ) >> THUMB_INSN_IMM_RD_SHIFT );
					op2 = insn & THUMB_INSN_IMM;
					rd = rn + op2;
					//mame_printf_debug("%08x:  Thumb instruction: ADD R%d, %02x\n", pc, ( insn & THUMB_INSN_IMM_RD ) >> THUMB_INSN_IMM_RD_SHIFT, op2);
					// VERBOSELOG((2,"%08x:  Thumb instruction: ADD R%d, %02x\n", pc, ( insn & THUMB_INSN_IMM_RD ) >> THUMB_INSN_IMM_RD_SHIFT, op2));
					SET_REGISTER( ( insn & THUMB_INSN_IMM_RD ) >> THUMB_INSN_IMM_RD_SHIFT, rd );
					HandleThumbALUAddFlags( rd, rn, op2 );
				}
				break;
			case 0x4: /* Rd & Rm instructions */
				switch( ( insn & THUMB_GROUP4_TYPE ) >> THUMB_GROUP4_TYPE_SHIFT )
				{
					case 0x0:
						switch( ( insn & THUMB_ALUOP_TYPE ) >> THUMB_ALUOP_TYPE_SHIFT )
						{
							case 0x0: /* AND Rd, Rs */  /* todo add dasm for this */
								rs = ( insn & THUMB_ADDSUB_RS ) >> THUMB_ADDSUB_RS_SHIFT;
								rd = ( insn & THUMB_ADDSUB_RD ) >> THUMB_ADDSUB_RD_SHIFT;
								SET_REGISTER( rd, GET_REGISTER(rd) & GET_REGISTER(rs) );
								SET_CPSR( GET_CPSR &~ ( Z_MASK | N_MASK ) );
								SET_CPSR( GET_CPSR | HandleALUNZFlags( GET_REGISTER(rd) ) );
								VERBOSELOG((2,"%08x:  Thumb instruction: AND R%d, R%d (%08x) (=%08x)\n", pc, rd, rs, GET_REGISTER(rs), GET_REGISTER(rd)));
								R15 += 2;
								break;
							case 0x1: /* EOR Rd, Rs */
								rs = ( insn & THUMB_ADDSUB_RS ) >> THUMB_ADDSUB_RS_SHIFT;
								rd = ( insn & THUMB_ADDSUB_RD ) >> THUMB_ADDSUB_RD_SHIFT;
								SET_REGISTER( rd, GET_REGISTER(rd) ^ GET_REGISTER(rs) );
								SET_CPSR( GET_CPSR &~ ( Z_MASK | N_MASK ) );
								SET_CPSR( GET_CPSR | HandleALUNZFlags( GET_REGISTER(rd) ) );
								VERBOSELOG((2,"%08x:  Thumb instruction: EOR R%d, R%d (%08x) (=%08x)\n", pc, rd, rs, GET_REGISTER(rs), GET_REGISTER(rd)));
								R15 += 2;
								break;
							case 0x3: /* LSR Rd, Rs */
								rs = ( insn & THUMB_ADDSUB_RS ) >> THUMB_ADDSUB_RS_SHIFT;
								rd = ( insn & THUMB_ADDSUB_RD ) >> THUMB_ADDSUB_RD_SHIFT;
								// printf( "LSR R%d (%08x), R%d (%08x) (= %08x)\n", rd, GET_REGISTER(rd), rs, GET_REGISTER(rs), GET_REGISTER(rd) << GET_REGISTER(rs) );
								VERBOSELOG((2,"%08x:  Thumb instruction: LSR R%d (%08x), R%d (%08x) (=%08x)\n", pc, rd, GET_REGISTER(rd), rs, GET_REGISTER(rs), GET_REGISTER(rd) << GET_REGISTER(rs)));
								SET_REGISTER( rd, GET_REGISTER(rd) >> GET_REGISTER(rs) );
								SET_CPSR( GET_CPSR | HandleALUNZFlags( GET_REGISTER(rd) ) );
								R15 += 2;
								break;
							case 0x5: /* ADC Rd, Rs - toto: add dasm -- is this right here? */
								rs = ( insn & THUMB_ADDSUB_RS ) >> THUMB_ADDSUB_RS_SHIFT;
								rd = ( insn & THUMB_ADDSUB_RD ) >> THUMB_ADDSUB_RD_SHIFT;
								VERBOSELOG((2,"%08x:  Thumb instruction: ADC R%d (%08x), R%d (%08x) (=%08x)\n", pc, rd, GET_REGISTER(rd), rs, GET_REGISTER(rs), GET_REGISTER(rd) | GET_REGISTER(rs)));
								op2=(GET_CPSR & C_MASK)?1:0;
								rn=GET_REGISTER(rd) + GET_REGISTER(rs) + op2;
								HandleThumbALUAddFlags( rn, GET_REGISTER(rd), GET_REGISTER(rs)+op2 ); //?
								SET_REGISTER( rd, rn);
								break;
							case 0x7: /* ROR Rd, Rs */
								rs = ( insn & THUMB_ADDSUB_RS ) >> THUMB_ADDSUB_RS_SHIFT;
								rd = ( insn & THUMB_ADDSUB_RD ) >> THUMB_ADDSUB_RD_SHIFT;
								imm = GET_REGISTER(rs) & 0x0000001f;
								SET_REGISTER( rd, ( GET_REGISTER(rd) >> imm ) | ( GET_REGISTER(rd) << ( 32 - imm ) ) );
								SET_CPSR( GET_CPSR &~ C_MASK );
								if( GET_REGISTER(rd) & 0x80000000 )
								{
									SET_CPSR( GET_CPSR | C_BIT );
								}
								SET_CPSR( GET_CPSR &~ ( Z_MASK | N_MASK ) );
								SET_CPSR( GET_CPSR | HandleALUNZFlags( GET_REGISTER(rd) ) );
								VERBOSELOG((2,"%08x:  Thumb instruction: TST R%d, R%d (%08x) (%02x)\n", pc, rd, rs, GET_REGISTER(rs), imm));
								R15 += 2;
								break;
							case 0x8: /* TST Rd, Rs */
								rs = ( insn & THUMB_ADDSUB_RS ) >> THUMB_ADDSUB_RS_SHIFT;
								rd = ( insn & THUMB_ADDSUB_RD ) >> THUMB_ADDSUB_RD_SHIFT;
								SET_CPSR( GET_CPSR &~ ( Z_MASK | N_MASK ) );
								SET_CPSR( GET_CPSR | HandleALUNZFlags( GET_REGISTER(rd) & GET_REGISTER(rs) ) );
								VERBOSELOG((2,"%08x:  Thumb instruction: TST R%d (%08x), R%d (%08x) (N=%d, Z=%d)\n", pc, rd, GET_REGISTER(rd), rs, GET_REGISTER(rs), N_IS_SET(GET_CPSR) ? 1 : 0, Z_IS_SET(GET_CPSR) ? 1 : 0));
								R15 += 2;
								break;
							case 0x9: /* NEG Rd, Rs - todo: check me */
								rs = ( insn & THUMB_ADDSUB_RS ) >> THUMB_ADDSUB_RS_SHIFT;
								rd = ( insn & THUMB_ADDSUB_RD ) >> THUMB_ADDSUB_RD_SHIFT;
								rn = 0 - GET_REGISTER(rs);
								SET_REGISTER( rd, rn );
								HandleThumbALUSubFlags( GET_REGISTER(rd), 0, rn );
								VERBOSELOG((2,"%08x:  Thumb instruction: NEG R%d (%08x), R%d (%08x) (N=%d, Z=%d, C=%d, V=%d)\n", pc, rd, GET_REGISTER(rd), rs, GET_REGISTER(rs), N_IS_SET(GET_CPSR) ? 1 : 0, Z_IS_SET(GET_CPSR) ? 1 : 0, C_IS_SET(GET_CPSR) ? 1 : 0, V_IS_SET(GET_CPSR) ? 1 : 0));
								break;
							case 0xa: /* CMP Rd, Rs */
								rs = ( insn & THUMB_ADDSUB_RS ) >> THUMB_ADDSUB_RS_SHIFT;
								rd = ( insn & THUMB_ADDSUB_RD ) >> THUMB_ADDSUB_RD_SHIFT;
								rn = GET_REGISTER(rd) - GET_REGISTER(rs);
								HandleThumbALUSubFlags( rn, GET_REGISTER(rd), GET_REGISTER(rs) );
								VERBOSELOG((2,"%08x:  Thumb instruction: CMP R%d (%08x), R%d (%08x) (N=%d, Z=%d, C=%d, V=%d)\n", pc, rd, GET_REGISTER(rd), rs, GET_REGISTER(rs), N_IS_SET(GET_CPSR) ? 1 : 0, Z_IS_SET(GET_CPSR) ? 1 : 0, C_IS_SET(GET_CPSR) ? 1 : 0, V_IS_SET(GET_CPSR) ? 1 : 0));
								break;
							case 0xb: /* CMN Rd, Rs - check flags, add dasm */
								rs = ( insn & THUMB_ADDSUB_RS ) >> THUMB_ADDSUB_RS_SHIFT;
								rd = ( insn & THUMB_ADDSUB_RD ) >> THUMB_ADDSUB_RD_SHIFT;
								rn = GET_REGISTER(rd) + GET_REGISTER(rs);
								HandleThumbALUAddFlags( rn, GET_REGISTER(rd), GET_REGISTER(rs) );
								VERBOSELOG((2,"%08x:  Thumb instruction: CMN R%d (%08x), R%d (%08x) (N=%d, Z=%d, C=%d, V=%d)\n", pc, rd, GET_REGISTER(rd), rs, GET_REGISTER(rs), N_IS_SET(GET_CPSR) ? 1 : 0, Z_IS_SET(GET_CPSR) ? 1 : 0, C_IS_SET(GET_CPSR) ? 1 : 0, V_IS_SET(GET_CPSR) ? 1 : 0));
								break;
							case 0xc: /* ORR Rd, Rs */
								rs = ( insn & THUMB_ADDSUB_RS ) >> THUMB_ADDSUB_RS_SHIFT;
								rd = ( insn & THUMB_ADDSUB_RD ) >> THUMB_ADDSUB_RD_SHIFT;
								VERBOSELOG((2,"%08x:  Thumb instruction: ORR R%d (%08x), R%d (%08x) (=%08x)\n", pc, rd, GET_REGISTER(rd), rs, GET_REGISTER(rs), GET_REGISTER(rd) | GET_REGISTER(rs)));
								SET_REGISTER( rd, GET_REGISTER(rd) | GET_REGISTER(rs) );
								SET_CPSR( GET_CPSR &~ ( Z_MASK | N_MASK ) );
								SET_CPSR( GET_CPSR | HandleALUNZFlags( GET_REGISTER(rd) ) );
								R15 += 2;
								break;
							case 0xd: /* MUL Rd, Rs */
								rs = ( insn & THUMB_ADDSUB_RS ) >> THUMB_ADDSUB_RS_SHIFT;
								rd = ( insn & THUMB_ADDSUB_RD ) >> THUMB_ADDSUB_RD_SHIFT;
								rn = GET_REGISTER(rd) * GET_REGISTER(rs);
								VERBOSELOG((2,"%08x:  Thumb instruction: MUL R%d (%08x), R%d (%08x) (=%08x)\n", pc, rd, GET_REGISTER(rd), rs, GET_REGISTER(rs), rn));
								SET_CPSR( GET_CPSR &~ ( Z_MASK | N_MASK ) );
								SET_REGISTER( rd, rn );
								SET_CPSR( GET_CPSR | HandleALUNZFlags( rn ) );
								R15 += 2;
								break;
							case 0xe: /* BIC Rd, Rs */
								rs = ( insn & THUMB_ADDSUB_RS ) >> THUMB_ADDSUB_RS_SHIFT;
								rd = ( insn & THUMB_ADDSUB_RD ) >> THUMB_ADDSUB_RD_SHIFT;
								SET_REGISTER( rd, GET_REGISTER(rd) & (~GET_REGISTER(rs)) );
								SET_CPSR( GET_CPSR &~ ( Z_MASK | N_MASK ) );
								SET_CPSR( GET_CPSR | HandleALUNZFlags( GET_REGISTER(rd) ) );
								VERBOSELOG((2,"%08x:  Thumb instruction: BIC R%d, R%d (%08x) (=%08x)\n", pc, rd, rs, GET_REGISTER(rs), GET_REGISTER(rd)));
								R15 += 2;
								break;
							case 0xf: /* MVN Rd, Rs */
								rs = ( insn & THUMB_ADDSUB_RS ) >> THUMB_ADDSUB_RS_SHIFT;
								rd = ( insn & THUMB_ADDSUB_RD ) >> THUMB_ADDSUB_RD_SHIFT;
								op2 = GET_REGISTER(rs);
								SET_REGISTER( rd, ~op2 );
								SET_CPSR( GET_CPSR &~ ( Z_MASK | N_MASK ) );
								SET_CPSR( GET_CPSR | HandleALUNZFlags( GET_REGISTER(rd) ) );
								VERBOSELOG((2,"%08x:  Thumb instruction: MVN R%d, R%d (%08x, ~=%08x)\n", pc, rd, rs, op2, GET_REGISTER(rd)));
								R15 += 2;
								break;
							default:
								fatalerror("%08x:  oops Undefined Thumb instruction: %04x\n", pc, insn);
								R15 += 2;
								break;
						}
						break;
					case 0x1:
						switch( ( insn & THUMB_HIREG_OP ) >> THUMB_HIREG_OP_SHIFT )
						{
							case 0x0: /* ADD Rd, Rs */
								rs = ( insn & THUMB_HIREG_RS ) >> THUMB_HIREG_RS_SHIFT;
								rd = insn & THUMB_HIREG_RD;
								switch( ( insn & THUMB_HIREG_H ) >> THUMB_HIREG_H_SHIFT )
								{
									case 0x2: /* ADD HRd, Rs */
										VERBOSELOG((2,"%08x:  Thumb instruction: ADD R%d (%08x), R%d (%08x)\n", pc, rd + 8, GET_REGISTER(rd+8), rs, GET_REGISTER(rs)));
										SET_REGISTER( rd+8, GET_REGISTER(rd+8) + GET_REGISTER(rs) );
										if (rd == 7) // urgh.. kov2 and martmast need this.. maybe anything else relying on R15 is wrong too
											R15 += 2;
										break;
								}
								//printf("fix me!\n");
								R15 += 2;

								break;
							case 0x1: /* CMP */
								switch( ( insn & THUMB_HIREG_H ) >> THUMB_HIREG_H_SHIFT )
								{
									case 0x1: /* CMP Rd, HRs */
										rs = GET_REGISTER( ( ( insn & THUMB_HIREG_RS ) >> THUMB_HIREG_RS_SHIFT ) + 8 );
										rd = GET_REGISTER( insn & THUMB_HIREG_RD );
										rn = rd - rs;
										HandleThumbALUSubFlags( rn, rs, rd );
										VERBOSELOG((2,"%08x:  Thumb instruction: CMP R%d (%08x), R%d (%08x)\n", pc, insn & THUMB_HIREG_RD, rd, ( ( insn & THUMB_HIREG_RS ) >> THUMB_HIREG_RS_SHIFT ) + 8, rs ));
										break;
									default:
										fatalerror("%08x:  Undefined Thumb instruction: %04x\n", pc, insn);
										R15 += 2;
										break;
								}
								break;
							case 0x2: /* MOV */
								switch( ( insn & THUMB_HIREG_H ) >> THUMB_HIREG_H_SHIFT )
								{
									case 0x1:
										rs = ( insn & THUMB_HIREG_RS ) >> THUMB_HIREG_RS_SHIFT;
										rd = insn & THUMB_HIREG_RD;
										if( rs == 7 )
										{
											SET_REGISTER( rd, GET_REGISTER(rs + 8) + 4 );
										}
										else
										{
											SET_REGISTER( rd, GET_REGISTER(rs + 8) );
										}
										VERBOSELOG((2,"%08x:  Thumb instruction: MOV R%d, R%d (%08x)\n", pc, rd, rs + 8, GET_REGISTER(rs + 8)));
										R15 += 2;
										break;
									case 0x2:
										rs = ( insn & THUMB_HIREG_RS ) >> THUMB_HIREG_RS_SHIFT;
										rd = insn & THUMB_HIREG_RD;
										SET_REGISTER( rd + 8, GET_REGISTER(rs) );
										VERBOSELOG((2,"%08x:  Thumb instruction: MOV R%d, R%d (%08x)\n", pc, rd + 8, rs, GET_REGISTER(rs)));
										if( rd != 7 )
										{
											R15 += 2;
										}
										break;
									default:
										fatalerror("%08x:  Undefined Thumb instruction: %04x\n", pc, insn);
										R15 += 2;
										break;
								}
								break;
							case 0x3:
								switch( ( insn & THUMB_HIREG_H ) >> THUMB_HIREG_H_SHIFT )
								{
									case 0x0:
										rd = ( insn & THUMB_HIREG_RS ) >> THUMB_HIREG_RS_SHIFT;
										addr = GET_REGISTER(rd);
										if( addr & 1 )
										{
											VERBOSELOG((2,"%08x:  Thumb instruction: BX R%d (%08x)\n", pc, rd, addr));
											addr &= ~1;
										}
										else
										{
											SET_CPSR(GET_CPSR &~ T_MASK);
											VERBOSELOG((2,"%08x:  Thumb instruction: BX R%d (%08x) (Switching to ARM mode)\n", pc, rd, addr ));
											if( addr & 2 )
											{
												addr += 2;
											}
										}
										R15 = addr;
										break;
									case 0x1:
										addr = GET_REGISTER( ( ( insn & THUMB_HIREG_RS ) >> THUMB_HIREG_RS_SHIFT ) + 8 );
										if( ( ( ( insn & THUMB_HIREG_RS ) >> THUMB_HIREG_RS_SHIFT ) + 8 ) == 15 )
										{
											addr += 2;
										}
										if( addr & 1 )
										{
											VERBOSELOG((2,"%08x:  Thumb instruction: BX R%d (%08x)\n", pc, ( ( insn & THUMB_HIREG_RS ) >> THUMB_HIREG_RS_SHIFT ) + 8, addr));
											addr &= ~1;
										}
										else
										{
											SET_CPSR(GET_CPSR &~ T_MASK);
											VERBOSELOG((2,"%08x:  Thumb instruction: BX R%d (%08x) (Switching to ARM mode)\n", pc, ( ( insn & THUMB_HIREG_RS ) >> THUMB_HIREG_RS_SHIFT ) + 8, addr));
											if( addr & 2 )
											{
												addr += 2;
											}
										}
										R15 = addr;
										break;
									default:
										fatalerror("%08x:  Undefined Thumb instruction: %04x\n", pc, insn);
										R15 += 2;
										break;
								}
								break;
							default:
								fatalerror("%08x:  Undefined Thumb instruction: %04x\n", pc, insn);
								R15 += 2;
								break;
						}
						break;
					case 0x2:
					case 0x3:
						readword = READ32( ( R15 & ~2 ) + 4 + ( ( insn & THUMB_INSN_IMM ) << 2 ) );
						SET_REGISTER( ( insn & THUMB_INSN_IMM_RD ) >> THUMB_INSN_IMM_RD_SHIFT, readword );
						VERBOSELOG((2,"%08x:  Thumb instruction: LDR R%d, %08x [PC, #%03x]\n", pc, ( insn & THUMB_INSN_IMM_RD ) >> THUMB_INSN_IMM_RD_SHIFT, readword, ( insn & THUMB_INSN_IMM ) << 2));
						R15 += 2;
						break;
					default:
						fatalerror("%08x:  Undefined Thumb instruction: %04x\n", pc, insn);
						R15 += 2;
						break;
				}
				break;
			case 0x5: /* LDR* STR* */
				switch( ( insn & THUMB_GROUP5_TYPE ) >> THUMB_GROUP5_TYPE_SHIFT )
				{
					case 0x0: /* STR Rd, [Rn, Rm] */
						rm = ( insn & THUMB_GROUP5_RM ) >> THUMB_GROUP5_RM_SHIFT;
						rn = ( insn & THUMB_GROUP5_RN ) >> THUMB_GROUP5_RN_SHIFT;
						rd = ( insn & THUMB_GROUP5_RD ) >> THUMB_GROUP5_RD_SHIFT;
						addr = GET_REGISTER(rn) + GET_REGISTER(rm);
						WRITE32( addr, GET_REGISTER(rd) );
						VERBOSELOG((2,"%08x:  Thumb instruction: STR %08x [R%d (%08x) + R%d (%08x)], R%d (%08x)\n", pc, addr, rn, GET_REGISTER(rn), rm, GET_REGISTER(rm), rd, GET_REGISTER(rd)));
						R15 += 2;
						break;
					case 0x1: /* STRH Rd, [Rn, Rm] */
						rm = ( insn & THUMB_GROUP5_RM ) >> THUMB_GROUP5_RM_SHIFT;
						rn = ( insn & THUMB_GROUP5_RN ) >> THUMB_GROUP5_RN_SHIFT;
						rd = ( insn & THUMB_GROUP5_RD ) >> THUMB_GROUP5_RD_SHIFT;
						addr = GET_REGISTER(rn) + GET_REGISTER(rm);
						WRITE16( addr, GET_REGISTER(rd) );
						VERBOSELOG((2,"%08x:  Thumb instruction: STRH %08x [R%d (%08x) + R%d (%08x)], R%d (%08x)\n", pc, addr, rn, GET_REGISTER(rn), rm, GET_REGISTER(rm), rd, GET_REGISTER(rd)));
						R15 += 2;
						break;
					case 0x2: /* STRB Rd, [Rn, Rm] */
						rm = ( insn & THUMB_GROUP5_RM ) >> THUMB_GROUP5_RM_SHIFT;
 						rn = ( insn & THUMB_GROUP5_RN ) >> THUMB_GROUP5_RN_SHIFT;
						rd = ( insn & THUMB_GROUP5_RD ) >> THUMB_GROUP5_RD_SHIFT;
 						addr = GET_REGISTER(rn) + GET_REGISTER(rm);
 						WRITE8( addr, GET_REGISTER(rd) );
						VERBOSELOG((2,"%08x:  Thumb instruction: STRB %08x [R%d (%08x) + R%d (%08x)], R%d (%08x)\n", pc, addr, rn, GET_REGISTER(rn), rm, GET_REGISTER(rm), rd, GET_REGISTER(rd)));
						R15 += 2;
						break;
					case 0x3: /* LDSB Rd, [Rn, Rm] todo, add dasm */
						rm = ( insn & THUMB_GROUP5_RM ) >> THUMB_GROUP5_RM_SHIFT;
						rn = ( insn & THUMB_GROUP5_RN ) >> THUMB_GROUP5_RN_SHIFT;
						rd = ( insn & THUMB_GROUP5_RD ) >> THUMB_GROUP5_RD_SHIFT;
						addr = GET_REGISTER(rn) + GET_REGISTER(rm);
						op2 = READ8( addr );
						if( op2 & 0x00000080 )
						{
							op2 |= 0xffffff00;
						}
						SET_REGISTER( rd, op2 );
						VERBOSELOG((2,"%08x:  Thumb instruction: LDSB R%d, [R%d (%08x) + R%d (%08x)] (%08x)\n", pc, rd, rn, GET_REGISTER(rn), rm, GET_REGISTER(rm), GET_REGISTER(rd)));
						R15 += 2;
						break;
					case 0x4: /* LDR Rd, [Rn, Rm] */
						rm = ( insn & THUMB_GROUP5_RM ) >> THUMB_GROUP5_RM_SHIFT;
						rn = ( insn & THUMB_GROUP5_RN ) >> THUMB_GROUP5_RN_SHIFT;
						rd = ( insn & THUMB_GROUP5_RD ) >> THUMB_GROUP5_RD_SHIFT;
						addr = GET_REGISTER(rn) + GET_REGISTER(rm);
						op2 = READ32( addr );
						SET_REGISTER( rd, op2 );
						VERBOSELOG((2,"%08x:  Thumb instruction: LDR R%d, [R%d (%08x) + R%d (%08x)] (%08x)\n", pc, rd, rn, GET_REGISTER(rn), rm, GET_REGISTER(rm), GET_REGISTER(rd)));
						R15 += 2;
						break;
					case 0x5: /* LDRH Rd, [Rn, Rm] */
						rm = ( insn & THUMB_GROUP5_RM ) >> THUMB_GROUP5_RM_SHIFT;
						rn = ( insn & THUMB_GROUP5_RN ) >> THUMB_GROUP5_RN_SHIFT;
						rd = ( insn & THUMB_GROUP5_RD ) >> THUMB_GROUP5_RD_SHIFT;
						addr = GET_REGISTER(rn) + GET_REGISTER(rm);
						op2 = READ16( addr );
						SET_REGISTER( rd, op2 );
						VERBOSELOG((2,"%08x:  Thumb instruction: LDRH R%d, [R%d (%08x) + R%d (%08x)] (%08x)\n", pc, rd, rn, GET_REGISTER(rn), rm, GET_REGISTER(rm), GET_REGISTER(rd)));
						R15 += 2;
						break;

					case 0x6: /* LDRB Rd, [Rn, Rm] */
						rm = ( insn & THUMB_GROUP5_RM ) >> THUMB_GROUP5_RM_SHIFT;
						rn = ( insn & THUMB_GROUP5_RN ) >> THUMB_GROUP5_RN_SHIFT;
						rd = ( insn & THUMB_GROUP5_RD ) >> THUMB_GROUP5_RD_SHIFT;
						addr = GET_REGISTER(rn) + GET_REGISTER(rm);
						op2 = READ8( addr );
						SET_REGISTER( rd, op2 );
						VERBOSELOG((2,"%08x:  Thumb instruction: LDRB R%d, [R%d (%08x) + R%d (%08x)] (%08x)\n", pc, rd, rn, GET_REGISTER(rn), rm, GET_REGISTER(rm), GET_REGISTER(rd)));
						R15 += 2;
						break;
					case 0x7: /* LDSH Rd, [Rn, Rm] */
						rm = ( insn & THUMB_GROUP5_RM ) >> THUMB_GROUP5_RM_SHIFT;
						rn = ( insn & THUMB_GROUP5_RN ) >> THUMB_GROUP5_RN_SHIFT;
						rd = ( insn & THUMB_GROUP5_RD ) >> THUMB_GROUP5_RD_SHIFT;
						addr = GET_REGISTER(rn) + GET_REGISTER(rm);
						op2 = READ16( addr );
						if( op2 & 0x00008000 )
						{
							op2 |= 0xffff0000;
						}
						SET_REGISTER( rd, op2 );
						VERBOSELOG((2,"%08x:  Thumb instruction: LDSH R%d, [R%d (%08x) + R%d (%08x)] (%08x)\n", pc, rd, rn, GET_REGISTER(rn), rm, GET_REGISTER(rm), GET_REGISTER(rd)));
						R15 += 2;
						break;
					default:
						fatalerror("%08x:  Undefined Thumb instruction: %04x\n", pc, insn);
						R15 += 2;
						break;
				}
				break;
			case 0x6: /* Word Store w/ Immediate Offset */
				if( insn & THUMB_LSOP_L ) /* Load */
				{
					rn = ( insn & THUMB_ADDSUB_RS ) >> THUMB_ADDSUB_RS_SHIFT;
					rd = insn & THUMB_ADDSUB_RD;
					offs = ( ( insn & THUMB_LSOP_OFFS ) >> THUMB_LSOP_OFFS_SHIFT ) << 2;
					SET_REGISTER( rd, READ32(GET_REGISTER(rn) + offs) ); // fix
					VERBOSELOG((2,"%08x:  Thumb instruction: LDR R%d [R%d + #%02x (%08x)] (=%08x)\n", pc, rd, rn, offs, GET_REGISTER(rn) + offs, GET_REGISTER(rd)));
					R15 += 2;
				}
				else /* Store */
				{
					rn = ( insn & THUMB_ADDSUB_RS ) >> THUMB_ADDSUB_RS_SHIFT;
					rd = insn & THUMB_ADDSUB_RD;
					offs = ( ( insn & THUMB_LSOP_OFFS ) >> THUMB_LSOP_OFFS_SHIFT ) << 2;
					WRITE32( GET_REGISTER(rn) + offs, GET_REGISTER(rd) );
					VERBOSELOG((2,"%08x:  Thumb instruction: STR R%d [R%d + #%02x (%08x)] \n", pc, rd, rn, offs, GET_REGISTER(rn) + offs));
					R15 += 2;
				}
				break;
			case 0x7: /* Byte Store w/ Immeidate Offset */
				if( insn & THUMB_LSOP_L ) /* Load */
				{
					rn = ( insn & THUMB_ADDSUB_RS ) >> THUMB_ADDSUB_RS_SHIFT;
					rd = insn & THUMB_ADDSUB_RD;
					offs = ( insn & THUMB_LSOP_OFFS ) >> THUMB_LSOP_OFFS_SHIFT;
					SET_REGISTER( rd, READ8( GET_REGISTER(rn) + offs ) );
					VERBOSELOG((2,"%08x:  Thumb instruction: LDRB R%d [R%d + #%02x (%08x)] (=%08x)\n", pc, rd, rn, offs, GET_REGISTER(rn) + offs, GET_REGISTER(rd)));
					R15 += 2;
				}
				else /* Store */
				{
					rn = ( insn & THUMB_ADDSUB_RS ) >> THUMB_ADDSUB_RS_SHIFT;
					rd = insn & THUMB_ADDSUB_RD;
					offs = ( insn & THUMB_LSOP_OFFS ) >> THUMB_LSOP_OFFS_SHIFT;
					WRITE8( GET_REGISTER(rn) + offs, GET_REGISTER(rd) );
					VERBOSELOG((2,"%08x:  Thumb instruction: STRB R%d [R%d + #%02x (%08x)] \n", pc, rd, rn, offs, GET_REGISTER(rn) + offs));
					R15 += 2;
				}
				break;
			case 0x8: /* Load/Store Halfword */
				if( insn & THUMB_HALFOP_L ) /* Load */
				{
					imm = ( insn & THUMB_HALFOP_OFFS ) >> THUMB_HALFOP_OFFS_SHIFT;
					rs = ( insn & THUMB_ADDSUB_RS ) >> THUMB_ADDSUB_RS_SHIFT;
					rd = ( insn & THUMB_ADDSUB_RD ) >> THUMB_ADDSUB_RD_SHIFT;
					SET_REGISTER( rd, READ16( GET_REGISTER(rs) + ( imm << 1 ) ) );
					VERBOSELOG((2,"%08x:  Thumb instruction: LDRH R%d (%08x), [R%d, #%03x] (%08x)\n", pc, rd, GET_REGISTER(rd), rs, ( imm << 1 ), GET_REGISTER(rs) + ( imm << 1 )));
					R15 += 2;
				}
				else /* Store */
				{
					imm = ( insn & THUMB_HALFOP_OFFS ) >> THUMB_HALFOP_OFFS_SHIFT;
					rs = ( insn & THUMB_ADDSUB_RS ) >> THUMB_ADDSUB_RS_SHIFT;
					rd = ( insn & THUMB_ADDSUB_RD ) >> THUMB_ADDSUB_RD_SHIFT;
					WRITE16( GET_REGISTER(rs) + ( imm << 1 ), GET_REGISTER(rd) );
					VERBOSELOG((2,"%08x:  Thumb instruction: STRH R%d (%08x), [R%d, #%03x] (%08x)\n", pc, rd, GET_REGISTER(rd), rs, ( imm << 1 ), GET_REGISTER(rs) + ( imm << 1 )));
					R15 += 2;
				}
				break;
			case 0x9: /* Stack-Relative Load/Store */
				if( insn & THUMB_STACKOP_L )
				{
					rd = ( insn & THUMB_STACKOP_RD ) >> THUMB_STACKOP_RD_SHIFT;
					offs = (INT8)( insn & THUMB_INSN_IMM );
					readword = READ32( GET_REGISTER(13) + ( (INT32)offs << 2 ) );
					SET_REGISTER( rd, readword );
					VERBOSELOG((2,"%08x:  Thumb instruction: LDR R%d, %08x [SP, #%03x]\n", pc, rd, GET_REGISTER(rd), offs << 2 ));
					R15 += 2;
				}
				else
				{
					rd = ( insn & THUMB_STACKOP_RD ) >> THUMB_STACKOP_RD_SHIFT;
					offs = (INT8)( insn & THUMB_INSN_IMM );
					WRITE32( GET_REGISTER(13) + ( (INT32)offs << 2 ), GET_REGISTER(rd) );
					VERBOSELOG((2,"%08x:  Thumb instruction: STR R%d, %08x [SP, #%03x]\n", pc, rd, GET_REGISTER(rd), offs << 2 ));
					R15 += 2;
				}
				break;
			case 0xa: /* Get relative address */
				if( insn & THUMB_RELADDR_SP ) /* ADD Rd, SP, #nn */
				{
					rd = ( insn & THUMB_RELADDR_RD ) >> THUMB_RELADDR_RD_SHIFT;
					offs = (UINT8)( insn & THUMB_INSN_IMM ) << 2;
					SET_REGISTER( rd, GET_REGISTER(13) + offs );
					VERBOSELOG((2,"%08x:  Thumb instruction: ADD R%d, SP, #%03x (%08x)\n", pc, rd, offs, GET_REGISTER(13) + offs ));
					R15 += 2;
				}
				else /* ADD Rd, PC, #nn */
				{
					rd = ( insn & THUMB_RELADDR_RD ) >> THUMB_RELADDR_RD_SHIFT;
					offs = (UINT8)( insn & THUMB_INSN_IMM ) << 2;
					SET_REGISTER( rd, ( ( R15 + 4 ) & ~2 ) + offs );
					VERBOSELOG((2,"%08x:  Thumb instruction: ADD R%d, PC, #%03x (%08x)\n", pc, rd, offs, ( ( R15 + 4 ) & ~2 ) + offs ));
					R15 += 2;
				}
				break;
			case 0xb: /* Stack-Related Opcodes */
				switch( ( insn & THUMB_STACKOP_TYPE ) >> THUMB_STACKOP_TYPE_SHIFT )
				{
					case 0x0: /* ADD SP, #imm */
						addr = ( insn & THUMB_INSN_IMM );
						addr &= ~THUMB_INSN_IMM_S;
						VERBOSELOG((2,"%08x:  Thumb instruction: ADD SP, #", pc));
						if( insn & THUMB_INSN_IMM_S )
						{
							VERBOSELOG((2,"-"));
						}
						VERBOSELOG((2,"%03x\n", addr << 2));
						SET_REGISTER( 13, GET_REGISTER(13) + ( ( insn & THUMB_INSN_IMM_S ) ? -( addr << 2 ) : ( addr << 2 ) ) );
						VERBOSELOG((2,"           SP after = %08x\n", GET_REGISTER(13)));
						R15 += 2;
						break;
					case 0x5: /* PUSH {Rlist}{LR} */
						VERBOSELOG((2,"%08x:  Thumb instruction: PUSH {LR, ", pc));
						SET_REGISTER( 13, GET_REGISTER(13) - 4 );
						WRITE32( GET_REGISTER(13), GET_REGISTER(14) );
						for( offs = 7; offs >= 0; offs-- )
						{
							if( insn & ( 1 << offs ) )
							{
								SET_REGISTER( 13, GET_REGISTER(13) - 4 );
								WRITE32( GET_REGISTER(13), GET_REGISTER(offs) );
								VERBOSELOG((2,"R%d, ", offs));
							}
						}
						VERBOSELOG((2,"}\n"));
						R15 += 2;
						break;
					case 0x4: /* PUSH {Rlist} */
						VERBOSELOG((2,"%08x:  Thumb instruction: PUSH {", pc));
						for( offs = 7; offs >= 0; offs-- )
						{
							if( insn & ( 1 << offs ) )
							{
								SET_REGISTER( 13, GET_REGISTER(13) - 4 );
								WRITE32( GET_REGISTER(13), GET_REGISTER(offs) );
								VERBOSELOG((2,"R%d, ", offs));
							}
						}
						VERBOSELOG((2,"}\n"));
						R15 += 2;
						break;
					case 0xc: /* POP {Rlist} */
						VERBOSELOG((2,"%08x:  Thumb instruction: POP {", pc));
						for( offs = 0; offs < 8; offs++ )
						{
							if( insn & ( 1 << offs ) )
							{
								VERBOSELOG((2,"R%d, ", offs));
								SET_REGISTER( offs, READ32( GET_REGISTER(13) ) );
								SET_REGISTER( 13, GET_REGISTER(13) + 4 );
							}
						}
						VERBOSELOG((2,"}\n"));
						R15 += 2;
						break;
					case 0xd: /* POP {Rlist}{PC} */
						VERBOSELOG((2,"%08x:  Thumb instruction: POP {", pc));
						for( offs = 0; offs < 8; offs++ )
						{
							if( insn & ( 1 << offs ) )
							{
								VERBOSELOG((2,"R%d, ", offs));
								SET_REGISTER( offs, READ32( GET_REGISTER(13) ) );
								SET_REGISTER( 13, GET_REGISTER(13) + 4 );
							}
						}
						R15 = READ32( GET_REGISTER(13) ) & ~1;
						SET_REGISTER( 13, GET_REGISTER(13) + 4 );
						VERBOSELOG((2,"PC}\n"));
						break;
					default:
						fatalerror("%08x:  Undefined Thumb instruction: %04x\n", pc, insn);
						R15 += 2;
						break;
				}
				break;
			case 0xc: /* Multiple Load/Store */
				if( insn & THUMB_MULTLS ) /* Load */
				{
					rd = ( insn & THUMB_MULTLS_BASE ) >> THUMB_MULTLS_BASE_SHIFT;
					VERBOSELOG((2,"%08x:  Thumb instruction: LDMIA R%d!,{", pc, rd));
					for( offs = 0; offs < 8; offs++ )
					{
						if( insn & ( 1 << offs ) )
						{
							VERBOSELOG((2,"R%d, ", offs));
							SET_REGISTER( offs, READ32( GET_REGISTER(rd) ) );
							SET_REGISTER( rd, GET_REGISTER(rd) + 4 );
						}
					}
					VERBOSELOG((2,"}\n"));
					R15 += 2;
				}
				else /* Store */
				{
					rd = ( insn & THUMB_MULTLS_BASE ) >> THUMB_MULTLS_BASE_SHIFT;
					VERBOSELOG((2,"%08x:  Thumb instruction: STMIA R%d!,{", pc, rd));
					for( offs = 0; offs < 8; offs++ )
					{
						if( insn & ( 1 << offs ) )
						{
							VERBOSELOG((2,"R%d, ", offs));
							WRITE32( GET_REGISTER(rd), GET_REGISTER(offs) );
							SET_REGISTER( rd, GET_REGISTER(rd) + 4 );
						}
					}
					VERBOSELOG((2,"}\n"));
					R15 += 2;
				}
				break;
			case 0xd: /* Conditional Branch */
				offs = (INT8)( insn & THUMB_INSN_IMM );
				switch( ( insn & THUMB_COND_TYPE ) >> THUMB_COND_TYPE_SHIFT )
				{
					case COND_EQ:
						VERBOSELOG((2,"%08x:  Thumb instruction: BEQ %08x (%02x)\n", pc, pc + 4 + (offs << 1), offs << 1));
	        	    	if( Z_IS_SET(GET_CPSR) )
	        	    	{
							R15 += 4 + (offs << 1);
						}
						else
						{
							R15 += 2;
						}
						break;
					case COND_NE:
						VERBOSELOG((2,"%08x:  Thumb instruction: BNE %08x (%02x)\n", pc, pc + 4 + (offs << 1), offs << 1));
	        	    	if( Z_IS_CLEAR(GET_CPSR) )
	        	    	{
							R15 += 4 + (offs << 1);
						}
						else
						{
							R15 += 2;
						}
						break;
					case COND_CS:
						VERBOSELOG((2,"%08x:  Thumb instruction: BCS %08x (%02x)\n", pc, pc + 4 + (offs << 1), offs << 1));
	        	    	if( C_IS_SET(GET_CPSR) )
	        	    	{
							R15 += 4 + (offs << 1);
						}
						else
						{
							R15 += 2;
						}
						break;
					case COND_CC:
						VERBOSELOG((2,"%08x:  Thumb instruction: BCC %08x (%02x)\n", pc, pc + 4 + (offs << 1), offs << 1));
	        	    	if( C_IS_CLEAR(GET_CPSR) )
	        	    	{
							R15 += 4 + (offs << 1);
						}
						else
						{
							R15 += 2;
						}
						break;
					case COND_MI:
						VERBOSELOG((2,"%08x:  Thumb instruction: BMI %08x (%02x)\n", pc, pc + 4 + (offs << 1), offs << 1));
	        	    	if( N_IS_SET(GET_CPSR) )
	        	    	{
							R15 += 4 + (offs << 1);
						}
						else
						{
							R15 += 2;
						}
						break;
					case COND_PL:
						VERBOSELOG((2,"%08x:  Thumb instruction: BPL %08x (%02x)\n", pc, pc + 4 + (offs << 1), offs << 1));
	        	    	if( N_IS_CLEAR(GET_CPSR) )
	        	    	{
							R15 += 4 + (offs << 1);
						}
						else
						{
							R15 += 2;
						}
						break;
					case COND_VS:
						VERBOSELOG((2,"%08x:  Thumb instruction: BVS %08x (%02x)\n", pc, pc + 4 + (offs << 1), offs << 1));
	        	    	if( V_IS_SET(GET_CPSR) )
	        	    	{
							R15 += 4 + (offs << 1);
						}
						else
						{
							R15 += 2;
						}
						break;
					case COND_VC:
						VERBOSELOG((2,"%08x:  Thumb instruction: BVC %08x (%02x)\n", pc, pc + 4 + (offs << 1), offs << 1));
	        	    	if( V_IS_CLEAR(GET_CPSR) )
	        	    	{
							R15 += 4 + (offs << 1);
						}
						else
						{
							R15 += 2;
						}
						break;
					case COND_HI:
						VERBOSELOG((2,"%08x:  Thumb instruction: BHI %08x (%02x)\n", pc, pc + 4 + (offs << 1), offs << 1));
	        	    	if( C_IS_SET(GET_CPSR) && Z_IS_CLEAR(GET_CPSR) )
	        	    	{
							R15 += 4 + (offs << 1);
						}
						else
						{
							R15 += 2;
						}
						break;
					case COND_LS:
						VERBOSELOG((2,"%08x:  Thumb instruction: BLS %08x (%02x)\n", pc, pc + 4 + (offs << 1), offs << 1));
	        	    	if( C_IS_CLEAR(GET_CPSR) || Z_IS_SET(GET_CPSR) )
	        	    	{
							R15 += 4 + (offs << 1);
						}
						else
						{
							R15 += 2;
						}
						break;
					case COND_GE:
						VERBOSELOG((2,"%08x:  Thumb instruction: BGE %08x (%02x)\n", pc, pc + 4 + (offs << 1), offs << 1));
	        	    	if( !(GET_CPSR & N_MASK) == !(GET_CPSR & V_MASK) )
	        	    	{
							R15 += 4 + (offs << 1);
						}
						else
						{
							R15 += 2;
						}
						break;
					case COND_LT:
						VERBOSELOG((2,"%08x:  Thumb instruction: BLT %08x (%02x)\n", pc, pc + 4 + (offs << 1), offs << 1));
	        	    	if( !(GET_CPSR & N_MASK) != !(GET_CPSR & V_MASK) )
	        	    	{
							R15 += 4 + (offs << 1);
						}
						else
						{
							R15 += 2;
						}
						break;
					case COND_GT:
						VERBOSELOG((2,"%08x:  Thumb instruction: BGT %08x (%02x)\n", pc, pc + 4 + (offs << 1), offs << 1));
	        	    	if( Z_IS_CLEAR(GET_CPSR) && ( !(GET_CPSR & N_MASK) == !(GET_CPSR & V_MASK) ) )
	        	    	{
							R15 += 4 + (offs << 1);
						}
						else
						{
							R15 += 2;
						}
						break;
					case COND_LE:
						VERBOSELOG((2,"%08x:  Thumb instruction: BLE %08x (%02x)\n", pc, pc + 4 + (offs << 1), offs << 1));
	        	    	if( Z_IS_SET(GET_CPSR) || ( !(GET_CPSR & N_MASK) != !(GET_CPSR & V_MASK) ) )
	        	    	{
							R15 += 4 + (offs << 1);
						}
						else
						{
							R15 += 2;
						}
						break;
					case COND_AL:
						VERBOSELOG((2,"%08x:  Thumb instruction: BAL %08x (%02x)\n", pc, pc + 4 + (offs << 1), offs << 1));
						R15 += offs;
						break;
					case COND_NV:
						VERBOSELOG((2,"%08x:  Thumb instruction: BNV %08x (%02x)\n", pc, pc + 4 + (offs << 1), offs << 1));
						R15 += 2;
						break;
				}
				break;
			case 0xe: /* B #offs */
				offs = ( insn & THUMB_BRANCH_OFFS ) << 1;
				if( offs & 0x00000800 )
				{
					offs |= 0xfffff800;
				}
				VERBOSELOG((2,"%08x:  Thumb instruction: B #%08x (%08x)\n", pc, offs, pc + 4 + offs));
				R15 += 4 + offs;
				break;
			case 0xf: /* BL */
				if( insn & THUMB_BLOP_LO )
				{
					addr = GET_REGISTER(14);
					addr += ( insn & THUMB_BLOP_OFFS ) << 1;
					SET_REGISTER( 14, ( R15 + 2 ) | 1 );
					VERBOSELOG((2,"%08x:  Thumb instruction: BL (LO) %08x\n", pc, addr));
					R15 = addr;
	//                      R15 += 2;
				}
				else
				{
					addr = ( insn & THUMB_BLOP_OFFS ) << 12;
					if( addr & ( 1 << 22 ) )
					{
						addr |= 0xff800000;
					}
					addr += R15 + 4;
					SET_REGISTER( 14, addr );
					VERBOSELOG((2,"%08x:  Thumb instruction: BL (HI) %08x\n", pc, addr));
					R15 += 2;
				}
				break;
			default:
				fatalerror("%08x:  Undefined Thumb instruction: %04x\n", pc, insn);
				R15 += 2;
				break;
		}
	}
	else
	{

        /* load 32 bit instruction */
        pc = R15;
        insn = cpu_readop32(pc);

        /* process condition codes for this instruction */
        switch (insn >> INSN_COND_SHIFT)
        {
        case COND_EQ:
            if (Z_IS_CLEAR(GET_CPSR)) goto L_Next;
            break;
        case COND_NE:
            if (Z_IS_SET(GET_CPSR)) goto L_Next;
            break;
        case COND_CS:
            if (C_IS_CLEAR(GET_CPSR)) goto L_Next;
            break;
        case COND_CC:
            if (C_IS_SET(GET_CPSR)) goto L_Next;
            break;
        case COND_MI:
            if (N_IS_CLEAR(GET_CPSR)) goto L_Next;
            break;
        case COND_PL:
            if (N_IS_SET(GET_CPSR)) goto L_Next;
            break;
        case COND_VS:
            if (V_IS_CLEAR(GET_CPSR)) goto L_Next;
            break;
        case COND_VC:
            if (V_IS_SET(GET_CPSR)) goto L_Next;
            break;
        case COND_HI:
            if (C_IS_CLEAR(GET_CPSR) || Z_IS_SET(GET_CPSR)) goto L_Next;
            break;
        case COND_LS:
            if (C_IS_SET(GET_CPSR) && Z_IS_CLEAR(GET_CPSR)) goto L_Next;
            break;
        case COND_GE:
            if (!(GET_CPSR & N_MASK) != !(GET_CPSR & V_MASK)) goto L_Next; /* Use x ^ (x >> ...) method */
            break;
        case COND_LT:
            if (!(GET_CPSR & N_MASK) == !(GET_CPSR & V_MASK)) goto L_Next;
            break;
        case COND_GT:
            if (Z_IS_SET(GET_CPSR) || (!(GET_CPSR & N_MASK) != !(GET_CPSR & V_MASK))) goto L_Next;
            break;
        case COND_LE:
            if (Z_IS_CLEAR(GET_CPSR) && (!(GET_CPSR & N_MASK) == !(GET_CPSR & V_MASK))) goto L_Next;
            break;
        case COND_NV:
            goto L_Next;
        }
        /*******************************************************************/
        /* If we got here - condition satisfied, so decode the instruction */
        /*******************************************************************/
        switch( (insn & 0xF000000)>>24 )
        {
            case 0:
            case 1:
            case 2:
            case 3:
                /* Branch and Exchange (BX) */
                if( (insn&0x0ffffff0)==0x012fff10 )     //bits 27-4 == 000100101111111111110001
                {
                    R15 = GET_REGISTER(insn & 0x0f);
                    //If new PC address has A0 set, switch to Thumb mode
                    if(R15 & 1) {
                        SET_CPSR(GET_CPSR|T_MASK);
                        LOG(("%08x: Setting Thumb Mode due to R15 change to %08x\n",pc,R15));
        	            R15--;
                    }
                }
                else
                /* Multiply OR Swap OR Half Word Data Transfer */
                if( (insn & 0x0e000000)==0 && (insn & 0x80) && (insn & 0x10) )  //bits 27-25 == 000, bit 7=1, bit 4=1
                {
                    /* Half Word Data Transfer */
                    if(insn & 0x60)         //bits = 6-5 != 00
                    {
                        HandleHalfWordDT(insn);
                    }
                    else
                    /* Swap */
                    if(insn & 0x01000000)   //bit 24 = 1
                    {
                        HandleSwap(insn);
                    }
                    /* Multiply Or Multiply Long */
                    else
                    {
                        /* multiply long */
                        if( insn&0x800000 ) //Bit 23 = 1 for Multiply Long
                        {
                            /* Signed? */
                            if( insn&0x00400000 )
                                HandleSMulLong(insn);
                            else
                                HandleUMulLong(insn);
                        }
                        /* multiply */
                        else
                        {
                            HandleMul(insn);
                        }
                        R15 += 4;
                    }
                }
                else
                /* Data Processing OR PSR Transfer */
                if( (insn & 0x0c000000) ==0 )   //bits 27-26 == 00 - This check can only exist properly after Multiplication check above
                {
                    /* PSR Transfer (MRS & MSR) */
                    if( ((insn&0x0100000)==0) && ((insn&0x01800000)==0x01000000) ) //( S bit must be clear, and bit 24,23 = 10 )
                    {
                        HandlePSRTransfer(insn);
                        ARM7_ICOUNT += 2;       //PSR only takes 1 - S Cycle, so we add + 2, since at end, we -3..
                        R15 += 4;
                    }
                    /* Data Processing */
                    else
                    {
                        HandleALU(insn);
                    }
                }
                break;
            /* Data Transfer - Single Data Access */
            case 4:
            case 5:
            case 6:
            case 7:
                HandleMemSingle(insn);
                R15 += 4;
                break;
            /* Block Data Transfer/Access */
            case 8:
            case 9:
                HandleMemBlock(insn);
                R15 += 4;
                break;
            /* Branch or Branch & Link */
            case 0xa:
            case 0xb:
                HandleBranch(insn);
                break;
            /* Co-Processor Data Transfer */
            case 0xc:
            case 0xd:
                HandleCoProcDT(insn);
                R15 += 4;
                break;
            /* Co-Processor Data Operation or Register Transfer */
            case 0xe:
                if(insn & 0x10)
                    HandleCoProcRT(insn);
                else
                    HandleCoProcDO(insn);
                R15 += 4;
                break;
            /* Software Interrupt */
            case 0x0f:
                ARM7.pendingSwi = 1;
                ARM7_CHECKIRQ;
                //couldn't find any cycle counts for SWI
                break;
            /* Undefined */
            default:
                ARM7.pendingSwi = 1;
                ARM7_CHECKIRQ;
                ARM7_ICOUNT -= 1;               //undefined takes 4 cycles (page 77)
                LOG(("%08x:  Undefined instruction\n",pc-4));
                L_Next:
                    R15 += 4;
                    ARM7_ICOUNT +=2;    //Any unexecuted instruction only takes 1 cycle (page 193)
        }
        }

		change_pc(R15);

        ARM7_CHECKIRQ;

        /* All instructions remove 3 cycles.. Others taking less / more will have adjusted this # prior to here */
        ARM7_ICOUNT -= 3;
    } while( ARM7_ICOUNT > 0 );

    return cycles - ARM7_ICOUNT;
}

