/*****************************************************************************
 *
 *   tbl2a03.c
 *   2a03 opcode functions and function pointer table
 *
 *   The 2a03 is a 6502 CPU that does not support the decimal mode
 *   of the ADC and SBC instructions, so all opcodes except ADC/SBC
 *   are simply mapped to the m6502 ones.
 *
 *   Copyright (c) 1998,1999,2000 Juergen Buchmueller, all rights reserved.
 *
 *   - This source code is released as freeware for non-commercial purposes.
 *   - You are free to use and redistribute this code in modified or
 *     unmodified form, provided you list me in the credits.
 *   - If you modify this source code, you must add a notice to each modified
 *     source file that it has been changed.  If you're a nice person, you
 *     will clearly mark each change too.  :)
 *   - If you wish to use this for commercial purposes, please contact me at
 *     pullmoll@t-online.de
 *   - The author of this copywritten work reserves the right to change the
 *     terms of its usage and license at any time, including retroactively
 *   - This entire notice must remain in the source code.
 *
 *****************************************************************************/

/*
  based on the nmos 6502
  illegal opcodes NOT added yet!
  (no program to test, all illegal opcodes not using adc/sbc should
  be the same as in nmos6502/6510!?)

  b flag handling might be changed,
  although only nmos series b-flag handling is quite sure
*/


#undef	OP
#define OP(nn) INLINE void n2a03_##nn(void)

#define EA_ABX_P		\
	EA_ABS;			\
	if (EAL + X > 0xff)	\
		m6502_ICount--;	\
	EAW += X

#define EA_ABY_P		\
	EA_ABS;			\
	if (EAL + Y > 0xff)	\
		m6502_ICount--;	\
	EAW += Y

#define EA_IDY_NP		\
	ZPL = RDOPARG();	\
	EAL = RDMEM(ZPD);	\
	ZPL++;			\
	EAH = RDMEM(ZPD);	\
	EAW += Y

#define RD_ABX_P	EA_ABX_P; tmp = RDMEM(EAD)
#define RD_ABY_P	EA_ABY_P; tmp = RDMEM(EAD)
#define RD_IDY_NP	EA_IDY_NP; tmp = RDMEM_ID(EAD)
#define WR_IDY_NP	EA_IDY_NP; WRMEM_ID(EAD, tmp)


#define TOP_P			\
	EA_ABS;			\
	if (EAL + X > 0xff)	\
		m6502_ICount--;


/*****************************************************************************
 *****************************************************************************
 *
 *   overrides for 2a03 opcodes
 *
 *****************************************************************************
 ********** insn   temp     cycles             rdmem   opc  wrmem   **********/
#define n2a03_00 m6502_00									/* 7 BRK */
#define n2a03_20 m6502_20									/* 6 JSR */
#define n2a03_40 m6502_40									/* 6 RTI */
#define n2a03_60 m6502_60									/* 6 RTS */
OP(80) {		  m6502_ICount -= 2;		 DOP;		  } /* 2 DOP */
#define n2a03_a0 m6502_a0									/* 2 LDY IMM */
#define n2a03_c0 m6502_c0									/* 2 CPY IMM */
#define n2a03_e0 m6502_e0									/* 2 CPX IMM */

#define n2a03_10 m6502_10									/* 2 BPL */
#define n2a03_30 m6502_30									/* 2 BMI */
#define n2a03_50 m6502_50									/* 2 BVC */
#define n2a03_70 m6502_70									/* 2 BVS */
#define n2a03_90 m6502_90									/* 2 BCC */
#define n2a03_b0 m6502_b0									/* 2 BCS */
#define n2a03_d0 m6502_d0									/* 2 BNE */
#define n2a03_f0 m6502_f0									/* 2 BEQ */

#define n2a03_01 m6502_01									/* 6 ORA IDX */
#define n2a03_21 m6502_21									/* 6 AND IDX */
#define n2a03_41 m6502_41									/* 6 EOR IDX */
OP(61) { int tmp; m6502_ICount -= 6; RD_IDX; ADC_NES;	  } /* 6 ADC IDX */
#define n2a03_81 m6502_81									/* 6 STA IDX */
#define n2a03_a1 m6502_a1									/* 6 LDA IDX */
#define n2a03_c1 m6502_c1									/* 6 CMP IDX */
OP(e1) { int tmp; m6502_ICount -= 6; RD_IDX; SBC_NES;	  } /* 6 SBC IDX */

#define n2a03_11 m6502_11									/* 5 ORA IDY */
#define n2a03_31 m6502_31									/* 5 AND IDY */
#define n2a03_51 m6502_51									/* 5 EOR IDY */
OP(71) { int tmp; m6502_ICount -= 5; RD_IDY; ADC_NES;	  } /* 5 ADC IDY */
OP(91) { int tmp; m6502_ICount -= 6;             STA; WR_IDY_NP; } /* 6 STA IDY no page penalty */
#define n2a03_b1 m6502_b1									/* 5 LDA IDY */
#define n2a03_d1 m6502_d1									/* 5 CMP IDY */
OP(f1) { int tmp; m6502_ICount -= 5; RD_IDY; SBC_NES;	  } /* 5 SBC IDY */

OP(02) {		  m6502_ICount -= 2;		 KIL;		  } /* 2 KIL */
OP(22) {		  m6502_ICount -= 2;		 KIL;		  } /* 2 KIL */
OP(42) {		  m6502_ICount -= 2;		 KIL;		  } /* 2 KIL */
OP(62) {		  m6502_ICount -= 2;		 KIL;		  } /* 2 KIL */
OP(82) {		  m6502_ICount -= 2;		 DOP;		  } /* 2 DOP */
#define n2a03_a2 m6502_a2									/* 2 LDX IMM */
OP(c2) {		  m6502_ICount -= 2;		 DOP;		  } /* 2 DOP */
OP(e2) {		  m6502_ICount -= 2;		 DOP;		  } /* 2 DOP */

OP(12) {		  m6502_ICount -= 2;		 KIL;		  } /* 2 KIL */
OP(32) {		  m6502_ICount -= 2;		 KIL;		  } /* 2 KIL */
OP(52) {		  m6502_ICount -= 2;		 KIL;		  } /* 2 KIL */
OP(72) {		  m6502_ICount -= 2;		 KIL;		  } /* 2 KIL */
OP(92) {		  m6502_ICount -= 2;		 KIL;		  } /* 2 KIL */
OP(b2) {		  m6502_ICount -= 2;		 KIL;		  } /* 2 KIL */
OP(d2) {		  m6502_ICount -= 2;		 KIL;		  } /* 2 KIL */
OP(f2) {		  m6502_ICount -= 2;		 KIL;		  } /* 2 KIL */

OP(03) { int tmp; m6502_ICount -= 8; RD_IDX; SLO; WB_EA;  } /* 8 SLO IDX */
OP(23) { int tmp; m6502_ICount -= 8; RD_IDX; RLA; WB_EA;  } /* 8 RLA IDX */
OP(43) { int tmp; m6502_ICount -= 8; RD_IDX; SRE; WB_EA;  } /* 8 SRE IDX */
OP(63) { int tmp; m6502_ICount -= 8; RD_IDX; RRA; WB_EA;  } /* 8 RRA IDX */
OP(83) { int tmp; m6502_ICount -= 6;		 SAX; WR_IDX; } /* 6 SAX IDX */
OP(a3) { int tmp; m6502_ICount -= 6; RD_IDX; LAX;		  } /* 6 LAX IDX */
OP(c3) { int tmp; m6502_ICount -= 8; RD_IDX; DCP; WB_EA;  } /* 8 DCP IDX */
OP(e3) { int tmp; m6502_ICount -= 8; RD_IDX; ISB; WB_EA;  } /* 8 ISB IDX */

OP(13) { int tmp; m6502_ICount -= 8; RD_IDY_NP; SLO; WB_EA; } /* 8 SLO IDY no page penalty */
OP(33) { int tmp; m6502_ICount -= 8; RD_IDY_NP; RLA; WB_EA; } /* 8 RLA IDY no page penalty */
OP(53) { int tmp; m6502_ICount -= 8; RD_IDY_NP; SRE; WB_EA; } /* 8 SRE IDY no page penalty */
OP(73) { int tmp; m6502_ICount -= 8; RD_IDY_NP; RRA; WB_EA; } /* 8 RRA IDY no page penalty */
OP(93) { int tmp; m6502_ICount -= 6; EA_IDY_NP; SAH; WB_EA; } /* 6 SAH IDY no page penalty */
OP(b3) { int tmp; m6502_ICount -= 5; RD_IDY; LAX;		  } /* 5 LAX IDY */
OP(d3) { int tmp; m6502_ICount -= 8; RD_IDY_NP; DCP; WB_EA; } /* 8 DCP IDY no page penalty */
OP(f3) { int tmp; m6502_ICount -= 8; RD_IDY_NP; ISB; WB_EA; } /* 8 ISB IDY no page penalty */


OP(04) {		  m6502_ICount -= 3;		 DOP;		  } /* 2 DOP */
#define n2a03_24 m6502_24									/* 3 BIT ZPG */
OP(44) {		  m6502_ICount -= 3;		 DOP;		  } /* 2 DOP */
OP(64) {		  m6502_ICount -= 3;		 DOP;		  } /* 2 DOP */
#define n2a03_84 m6502_84									/* 3 STY ZPG */
#define n2a03_a4 m6502_a4									/* 3 LDY ZPG */
#define n2a03_c4 m6502_c4									/* 3 CPY ZPG */
#define n2a03_e4 m6502_e4									/* 3 CPX ZPG */

OP(14) {		  m6502_ICount -= 4;		 DOP;		  } /* 2 DOP */
OP(34) {		  m6502_ICount -= 4;		 DOP;		  } /* 2 DOP */
OP(54) {		  m6502_ICount -= 4;		 DOP;		  } /* 2 DOP */
OP(74) {		  m6502_ICount -= 4;		 DOP;		  } /* 2 DOP */
#define n2a03_94 m6502_94									/* 4 STY ZP_X */
#define n2a03_b4 m6502_b4									/* 4 LDY ZP_X */
OP(d4) {		  m6502_ICount -= 4;		 DOP;		  } /* 2 DOP */
OP(f4) {		  m6502_ICount -= 4;		 DOP;		  } /* 2 DOP */

#define n2a03_05 m6502_05									/* 3 ORA ZPG */
#define n2a03_25 m6502_25									/* 3 AND ZPG */
#define n2a03_45 m6502_45									/* 3 EOR ZPG */
OP(65) { int tmp; m6502_ICount -= 3; RD_ZPG; ADC_NES;	  } /* 3 ADC ZPG */
#define n2a03_85 m6502_85									/* 3 STA ZPG */
#define n2a03_a5 m6502_a5									/* 3 LDA ZPG */
#define n2a03_c5 m6502_c5									/* 3 CMP ZPG */
OP(e5) { int tmp; m6502_ICount -= 3; RD_ZPG; SBC_NES;	  } /* 3 SBC ZPG */

#define n2a03_15 m6502_15									/* 4 ORA ZPX */
#define n2a03_35 m6502_35									/* 4 AND ZPX */
#define n2a03_55 m6502_55									/* 4 EOR ZPX */
OP(75) { int tmp; m6502_ICount -= 4; RD_ZPX; ADC_NES;	  } /* 4 ADC ZPX */
#define n2a03_95 m6502_95									/* 4 STA ZPX */
#define n2a03_b5 m6502_b5									/* 4 LDA ZPX */
#define n2a03_d5 m6502_d5									/* 4 CMP ZPX */
OP(f5) { int tmp; m6502_ICount -= 4; RD_ZPX; SBC_NES;	  } /* 4 SBC ZPX */

#define n2a03_06 m6502_06									/* 5 ASL ZPG */
#define n2a03_26 m6502_26									/* 5 ROL ZPG */
#define n2a03_46 m6502_46									/* 5 LSR ZPG */
#define n2a03_66 m6502_66									/* 5 ROR ZPG */
#define n2a03_86 m6502_86									/* 3 STX ZPG */
#define n2a03_a6 m6502_a6									/* 3 LDX ZPG */
#define n2a03_c6 m6502_c6									/* 5 DEC ZPG */
#define n2a03_e6 m6502_e6									/* 5 INC ZPG */

#define n2a03_16 m6502_16									/* 6 ASL ZPX */
#define n2a03_36 m6502_36									/* 6 ROL ZPX */
#define n2a03_56 m6502_56									/* 6 LSR ZPX */
#define n2a03_76 m6502_76									/* 6 ROR ZPX */
#define n2a03_96 m6502_96									/* 4 STX ZPY */
#define n2a03_b6 m6502_b6									/* 4 LDX ZPY */
#define n2a03_d6 m6502_d6									/* 6 DEC ZPX */
#define n2a03_f6 m6502_f6									/* 6 INC ZPX */

OP(07) { int tmp; m6502_ICount -= 5; RD_ZPG; SLO; WB_EA;  } /* 5 SLO ZPG */
OP(27) { int tmp; m6502_ICount -= 5; RD_ZPG; RLA; WB_EA;  } /* 5 RLA ZPG */
OP(47) { int tmp; m6502_ICount -= 5; RD_ZPG; SRE; WB_EA;  } /* 5 SRE ZPG */
OP(67) { int tmp; m6502_ICount -= 5; RD_ZPG; RRA; WB_EA;  } /* 5 RRA ZPG */
OP(87) { int tmp; m6502_ICount -= 3;		 SAX; WR_ZPG; } /* 3 SAX ZPG */
OP(a7) { int tmp; m6502_ICount -= 3; RD_ZPG; LAX;		  } /* 3 LAX ZPG */
OP(c7) { int tmp; m6502_ICount -= 5; RD_ZPG; DCP; WB_EA;  } /* 5 DCP ZPG */
OP(e7) { int tmp; m6502_ICount -= 5; RD_ZPG; ISB; WB_EA;  } /* 5 ISB ZPG */

OP(17) { int tmp; m6502_ICount -= 6; RD_ZPX; SLO; WB_EA;  } /* 4 SLO ZPX */
OP(37) { int tmp; m6502_ICount -= 6; RD_ZPX; RLA; WB_EA;  } /* 4 RLA ZPX */
OP(57) { int tmp; m6502_ICount -= 6; RD_ZPX; SRE; WB_EA;  } /* 4 SRE ZPX */
OP(77) { int tmp; m6502_ICount -= 6; RD_ZPX; RRA; WB_EA;  } /* 4 RRA ZPX */
OP(97) { int tmp; m6502_ICount -= 4;		 SAX; WR_ZPY; } /* 4 SAX ZPY */
OP(b7) { int tmp; m6502_ICount -= 4; RD_ZPY; LAX;		  } /* 4 LAX ZPY */
OP(d7) { int tmp; m6502_ICount -= 6; RD_ZPX; DCP; WB_EA;  } /* 6 DCP ZPX */
OP(f7) { int tmp; m6502_ICount -= 6; RD_ZPX; ISB; WB_EA;  } /* 6 ISB ZPX */

#define n2a03_08 m6502_08									/* 2 PHP */
#define n2a03_28 m6502_28									/* 2 PLP */
#define n2a03_48 m6502_48									/* 2 PHA */
#define n2a03_68 m6502_68									/* 2 PLA */
#define n2a03_88 m6502_88									/* 2 DEY */
#define n2a03_a8 m6502_a8									/* 2 TAY */
#define n2a03_c8 m6502_c8									/* 2 INY */
#define n2a03_e8 m6502_e8									/* 2 INX */

#define n2a03_18 m6502_18									/* 2 CLC */
#define n2a03_38 m6502_38									/* 2 SEC */
#define n2a03_58 m6502_58									/* 2 CLI */
#define n2a03_78 m6502_78									/* 2 SEI */
#define n2a03_98 m6502_98									/* 2 TYA */
#define n2a03_b8 m6502_b8									/* 2 CLV */
#define n2a03_d8 m6502_d8									/* 2 CLD */
#define n2a03_f8 m6502_f8									/* 2 SED */

#define n2a03_09 m6502_09									/* 2 ORA IMM */
#define n2a03_29 m6502_29									/* 2 AND IMM */
#define n2a03_49 m6502_49									/* 2 EOR IMM */
OP(69) { int tmp; m6502_ICount -= 2; RD_IMM; ADC_NES;	  } /* 2 ADC IMM */
OP(89) {		  m6502_ICount -= 2;		 DOP;		  } /* 2 DOP */
#define n2a03_a9 m6502_a9									/* 2 LDA IMM */
#define n2a03_c9 m6502_c9									/* 2 CMP IMM */
OP(e9) { int tmp; m6502_ICount -= 2; RD_IMM; SBC_NES;	  } /* 2 SBC IMM */

OP(19) { int tmp; m6502_ICount -= 4; RD_ABY_P; ORA;       } /* 4 ORA ABY page penalty */
OP(39) { int tmp; m6502_ICount -= 4; RD_ABY_P; AND;       } /* 4 AND ABY page penalty */
OP(59) { int tmp; m6502_ICount -= 4; RD_ABY_P; EOR;       } /* 4 EOR ABY page penalty */
OP(79) { int tmp; m6502_ICount -= 4; RD_ABY_P; ADC_NES;	  } /* 4 ADC ABY page penalty */
#define n2a03_99 m6502_99									/* 5 STA ABY */
OP(b9) { int tmp; m6502_ICount -= 4; RD_ABY_P; LDA;       } /* 4 LDA ABY page penalty */
OP(d9) { int tmp; m6502_ICount -= 4; RD_ABY_P; CMP;       } /* 4 CMP ABY page penalty */
OP(f9) { int tmp; m6502_ICount -= 4; RD_ABY_P; SBC_NES;	  } /* 4 SBC ABY page penalty */

#define n2a03_0a m6502_0a									/* 2 ASL A */
#define n2a03_2a m6502_2a									/* 2 ROL A */
#define n2a03_4a m6502_4a									/* 2 LSR A */
#define n2a03_6a m6502_6a									/* 2 ROR A */
#define n2a03_8a m6502_8a									/* 2 TXA */
#define n2a03_aa m6502_aa									/* 2 TAX */
#define n2a03_ca m6502_ca									/* 2 DEX */
#define n2a03_ea m6502_ea									/* 2 NOP */

OP(1a) {		  m6502_ICount -= 2;		 NOP;		  } /* 2 NOP */
OP(3a) {		  m6502_ICount -= 2;		 NOP;		  } /* 2 NOP */
OP(5a) {		  m6502_ICount -= 2;		 NOP;		  } /* 2 NOP */
OP(7a) {		  m6502_ICount -= 2;		 NOP;		  } /* 2 NOP */
#define n2a03_9a m6502_9a									/* 2 TXS */
#define n2a03_ba m6502_ba									/* 2 TSX */
OP(da) {		  m6502_ICount -= 2;		 NOP;		  } /* 2 NOP */
OP(fa) {		  m6502_ICount -= 2;		 NOP;		  } /* 2 NOP */

OP(0b) { int tmp; m6502_ICount -= 2; RD_IMM; ANC;		  } /* 2 ANC IMM */
OP(2b) { int tmp; m6502_ICount -= 2; RD_IMM; ANC;		  } /* 2 ANC IMM */
OP(4b) { int tmp; m6502_ICount -= 2; RD_IMM; ASR; WB_ACC; } /* 2 ASR IMM */
OP(6b) { int tmp; m6502_ICount -= 2; RD_IMM; ARR_NES; WB_ACC; } /* 2 ARR IMM */
OP(8b) { int tmp; m6502_ICount -= 2; RD_IMM; AXA;         } /* 2 AXA IMM */
OP(ab) { int tmp; m6502_ICount -= 2; RD_IMM; OAL;         } /* 2 OAL IMM */
OP(cb) { int tmp; m6502_ICount -= 2; RD_IMM; ASX;		  } /* 2 ASX IMM */
OP(eb) { int tmp; m6502_ICount -= 2; RD_IMM; SBC_NES;		  } /* 2 SBC IMM */

OP(1b) { int tmp; m6502_ICount -= 7; RD_ABY; SLO; WB_EA;  } /* 4 SLO ABY */
OP(3b) { int tmp; m6502_ICount -= 7; RD_ABY; RLA; WB_EA;  } /* 4 RLA ABY */
OP(5b) { int tmp; m6502_ICount -= 7; RD_ABY; SRE; WB_EA;  } /* 4 SRE ABY */
OP(7b) { int tmp; m6502_ICount -= 7; RD_ABY; RRA; WB_EA;  } /* 4 RRA ABY */
OP(9b) { int tmp; m6502_ICount -= 5; EA_ABY; SSH; WB_EA;  } /* 5 SSH ABY */
OP(bb) { int tmp; m6502_ICount -= 4; RD_ABY_P; AST;		  } /* 4 AST ABY page penalty */
OP(db) { int tmp; m6502_ICount -= 7; RD_ABY; DCP; WB_EA;  } /* 7 DCP ABY */
OP(fb) { int tmp; m6502_ICount -= 7; RD_ABY; ISB; WB_EA;  } /* 7 ISB ABY */

OP(0c) {		  m6502_ICount -= 4;		 TOP;		  } /* 4 TOP */
#define n2a03_2c m6502_2c									/* 4 BIT ABS */
#define n2a03_4c m6502_4c									/* 3 JMP ABS */
#define n2a03_6c m6502_6c									/* 5 JMP IND */
#define n2a03_8c m6502_8c									/* 4 STY ABS */
#define n2a03_ac m6502_ac									/* 4 LDY ABS */
#define n2a03_cc m6502_cc									/* 4 CPY ABS */
#define n2a03_ec m6502_ec									/* 4 CPX ABS */

OP(1c) {		  m6502_ICount -= 4;		 TOP_P;		  } /* 4 TOP page penalty */
OP(3c) {		  m6502_ICount -= 4;		 TOP_P;		  } /* 4 TOP page penalty */
OP(5c) {		  m6502_ICount -= 4;		 TOP_P;		  } /* 4 TOP page penalty */
OP(7c) {		  m6502_ICount -= 4;		 TOP_P;		  } /* 4 TOP page penalty */
OP(9c) { int tmp; m6502_ICount -= 5; EA_ABX; SYH; WB_EA;  } /* 5 SYH ABX */
OP(bc) { int tmp; m6502_ICount -= 4; RD_ABX_P; LDY;       } /* 4 LDY ABX page penalty */
OP(dc) {		  m6502_ICount -= 4;		 TOP_P;		  } /* 4 TOP page penalty */
OP(fc) {		  m6502_ICount -= 4;		 TOP_P;		  } /* 4 TOP page penalty */

#define n2a03_0d m6502_0d									/* 4 ORA ABS */
#define n2a03_2d m6502_2d									/* 4 AND ABS */
#define n2a03_4d m6502_4d									/* 4 EOR ABS */
OP(6d) { int tmp; m6502_ICount -= 4; RD_ABS; ADC_NES;	  } /* 4 ADC ABS */
#define n2a03_8d m6502_8d									/* 4 STA ABS */
#define n2a03_ad m6502_ad									/* 4 LDA ABS */
#define n2a03_cd m6502_cd									/* 4 CMP ABS */
OP(ed) { int tmp; m6502_ICount -= 4; RD_ABS; SBC_NES;	  } /* 4 SBC ABS */

OP(1d) { int tmp; m6502_ICount -= 4; RD_ABX_P; ORA;       } /* 4 ORA ABX page penalty */
OP(3d) { int tmp; m6502_ICount -= 4; RD_ABX_P; AND;       } /* 4 AND ABX page penalty */
OP(5d) { int tmp; m6502_ICount -= 4; RD_ABX_P; EOR;       } /* 4 EOR ABX page penalty */
OP(7d) { int tmp; m6502_ICount -= 4; RD_ABX_P; ADC_NES;	  } /* 4 ADC ABX page penalty */
#define n2a03_9d m6502_9d									/* 5 STA ABX */
OP(bd) { int tmp; m6502_ICount -= 4; RD_ABX_P; LDA;       } /* 4 LDA ABX page penalty */
OP(dd) { int tmp; m6502_ICount -= 4; RD_ABX_P; CMP;       } /* 4 CMP ABX page penalty */
OP(fd) { int tmp; m6502_ICount -= 4; RD_ABX_P; SBC_NES;	  } /* 4 SBC ABX page penalty */

#define n2a03_0e m6502_0e									/* 6 ASL ABS */
#define n2a03_2e m6502_2e									/* 6 ROL ABS */
#define n2a03_4e m6502_4e									/* 6 LSR ABS */
#define n2a03_6e m6502_6e									/* 6 ROR ABS */
#define n2a03_8e m6502_8e									/* 5 STX ABS */
#define n2a03_ae m6502_ae									/* 4 LDX ABS */
#define n2a03_ce m6502_ce									/* 6 DEC ABS */
#define n2a03_ee m6502_ee									/* 6 INC ABS */

#define n2a03_1e m6502_1e									/* 7 ASL ABX */
#define n2a03_3e m6502_3e									/* 7 ROL ABX */
#define n2a03_5e m6502_5e									/* 7 LSR ABX */
#define n2a03_7e m6502_7e									/* 7 ROR ABX */
OP(9e) { int tmp; m6502_ICount -= 5; EA_ABY; SXH; WB_EA;  } /* 5 SXH ABY */
OP(be) { int tmp; m6502_ICount -= 4; RD_ABY_P; LDX;       } /* 4 LDX ABY page penalty */
#define n2a03_de m6502_de									/* 7 DEC ABX */
#define n2a03_fe m6502_fe									/* 7 INC ABX */

OP(0f) { int tmp; m6502_ICount -= 6; RD_ABS; SLO; WB_EA;  } /* 6 SLO ABS */
OP(2f) { int tmp; m6502_ICount -= 6; RD_ABS; RLA; WB_EA;  } /* 6 RLA ABS */
OP(4f) { int tmp; m6502_ICount -= 6; RD_ABS; SRE; WB_EA;  } /* 6 SRE ABS */
OP(6f) { int tmp; m6502_ICount -= 6; RD_ABS; RRA; WB_EA;  } /* 6 RRA ABS */
OP(8f) { int tmp; m6502_ICount -= 4;		 SAX; WR_ABS; } /* 4 SAX ABS */
OP(af) { int tmp; m6502_ICount -= 4; RD_ABS; LAX;		  } /* 4 LAX ABS */
OP(cf) { int tmp; m6502_ICount -= 6; RD_ABS; DCP; WB_EA;  } /* 6 DCP ABS */
OP(ef) { int tmp; m6502_ICount -= 6; RD_ABS; ISB; WB_EA;  } /* 6 ISB ABS */

OP(1f) { int tmp; m6502_ICount -= 7; RD_ABX; SLO; WB_EA;  } /* 7 SLO ABX */
OP(3f) { int tmp; m6502_ICount -= 7; RD_ABX; RLA; WB_EA;  } /* 7 RLA ABX */
OP(5f) { int tmp; m6502_ICount -= 7; RD_ABX; SRE; WB_EA;  } /* 7 SRE ABX */
OP(7f) { int tmp; m6502_ICount -= 7; RD_ABX; RRA; WB_EA;  } /* 7 RRA ABX */
OP(9f) { int tmp; m6502_ICount -= 5; EA_ABY; SAH; WB_EA;  } /* 5 SAH ABY */
OP(bf) { int tmp; m6502_ICount -= 4; RD_ABY_P; LAX;		  } /* 4 LAX ABY page penalty */
OP(df) { int tmp; m6502_ICount -= 7; RD_ABX; DCP; WB_EA;  } /* 7 DCP ABX */
OP(ff) { int tmp; m6502_ICount -= 7; RD_ABX; ISB; WB_EA;  } /* 7 ISB ABX */

static void (*insn2a03[0x100])(void) = {
	n2a03_00,n2a03_01,n2a03_02,n2a03_03,n2a03_04,n2a03_05,n2a03_06,n2a03_07,
	n2a03_08,n2a03_09,n2a03_0a,n2a03_0b,n2a03_0c,n2a03_0d,n2a03_0e,n2a03_0f,
	n2a03_10,n2a03_11,n2a03_12,n2a03_13,n2a03_14,n2a03_15,n2a03_16,n2a03_17,
	n2a03_18,n2a03_19,n2a03_1a,n2a03_1b,n2a03_1c,n2a03_1d,n2a03_1e,n2a03_1f,
	n2a03_20,n2a03_21,n2a03_22,n2a03_23,n2a03_24,n2a03_25,n2a03_26,n2a03_27,
	n2a03_28,n2a03_29,n2a03_2a,n2a03_2b,n2a03_2c,n2a03_2d,n2a03_2e,n2a03_2f,
	n2a03_30,n2a03_31,n2a03_32,n2a03_33,n2a03_34,n2a03_35,n2a03_36,n2a03_37,
	n2a03_38,n2a03_39,n2a03_3a,n2a03_3b,n2a03_3c,n2a03_3d,n2a03_3e,n2a03_3f,
	n2a03_40,n2a03_41,n2a03_42,n2a03_43,n2a03_44,n2a03_45,n2a03_46,n2a03_47,
	n2a03_48,n2a03_49,n2a03_4a,n2a03_4b,n2a03_4c,n2a03_4d,n2a03_4e,n2a03_4f,
	n2a03_50,n2a03_51,n2a03_52,n2a03_53,n2a03_54,n2a03_55,n2a03_56,n2a03_57,
	n2a03_58,n2a03_59,n2a03_5a,n2a03_5b,n2a03_5c,n2a03_5d,n2a03_5e,n2a03_5f,
	n2a03_60,n2a03_61,n2a03_62,n2a03_63,n2a03_64,n2a03_65,n2a03_66,n2a03_67,
	n2a03_68,n2a03_69,n2a03_6a,n2a03_6b,n2a03_6c,n2a03_6d,n2a03_6e,n2a03_6f,
	n2a03_70,n2a03_71,n2a03_72,n2a03_73,n2a03_74,n2a03_75,n2a03_76,n2a03_77,
	n2a03_78,n2a03_79,n2a03_7a,n2a03_7b,n2a03_7c,n2a03_7d,n2a03_7e,n2a03_7f,
	n2a03_80,n2a03_81,n2a03_82,n2a03_83,n2a03_84,n2a03_85,n2a03_86,n2a03_87,
	n2a03_88,n2a03_89,n2a03_8a,n2a03_8b,n2a03_8c,n2a03_8d,n2a03_8e,n2a03_8f,
	n2a03_90,n2a03_91,n2a03_92,n2a03_93,n2a03_94,n2a03_95,n2a03_96,n2a03_97,
	n2a03_98,n2a03_99,n2a03_9a,n2a03_9b,n2a03_9c,n2a03_9d,n2a03_9e,n2a03_9f,
	n2a03_a0,n2a03_a1,n2a03_a2,n2a03_a3,n2a03_a4,n2a03_a5,n2a03_a6,n2a03_a7,
	n2a03_a8,n2a03_a9,n2a03_aa,n2a03_ab,n2a03_ac,n2a03_ad,n2a03_ae,n2a03_af,
	n2a03_b0,n2a03_b1,n2a03_b2,n2a03_b3,n2a03_b4,n2a03_b5,n2a03_b6,n2a03_b7,
	n2a03_b8,n2a03_b9,n2a03_ba,n2a03_bb,n2a03_bc,n2a03_bd,n2a03_be,n2a03_bf,
	n2a03_c0,n2a03_c1,n2a03_c2,n2a03_c3,n2a03_c4,n2a03_c5,n2a03_c6,n2a03_c7,
	n2a03_c8,n2a03_c9,n2a03_ca,n2a03_cb,n2a03_cc,n2a03_cd,n2a03_ce,n2a03_cf,
	n2a03_d0,n2a03_d1,n2a03_d2,n2a03_d3,n2a03_d4,n2a03_d5,n2a03_d6,n2a03_d7,
	n2a03_d8,n2a03_d9,n2a03_da,n2a03_db,n2a03_dc,n2a03_dd,n2a03_de,n2a03_df,
	n2a03_e0,n2a03_e1,n2a03_e2,n2a03_e3,n2a03_e4,n2a03_e5,n2a03_e6,n2a03_e7,
	n2a03_e8,n2a03_e9,n2a03_ea,n2a03_eb,n2a03_ec,n2a03_ed,n2a03_ee,n2a03_ef,
	n2a03_f0,n2a03_f1,n2a03_f2,n2a03_f3,n2a03_f4,n2a03_f5,n2a03_f6,n2a03_f7,
	n2a03_f8,n2a03_f9,n2a03_fa,n2a03_fb,n2a03_fc,n2a03_fd,n2a03_fe,n2a03_ff
};

