/*

	Hitachi H8S/2xxx MCU

	(c) 2001-2007 Tim Schuerewegen

	H8S/2241
	H8S/2246
	H8S/2323

*/

#include "h8sopcd.h"

#define FETCH_BYTE(x) if (x >= fetch) instr[x] = program_read_byte_16be( pc + x)

H8S_INSTR_TABLE_ENTRY *h8s_instr_table;

//------------------------------------------------------------------------------
UINT16 h8s_opcode_find_01_00( UINT32 pc, UINT8 *instr, UINT8 fetch)
{
	// read
	FETCH_BYTE(2);
	FETCH_BYTE(3);
	// 01 + 00 + xx
	switch (instr[2])
	{
		// 01 + 00 + 69
		case 0x69 :
		{
			// 01 00 69 42 : mov.l @er4, er2
			// 01 00 69 xy
			if ((instr[3] & 0x88) == 0x00) return INSTR_MOV_AR32_R32;
			// 01 00 69 A2 : mov.l er2, @er2
			// 01 00 69 xy
			if ((instr[3] & 0x88) == 0x80) return INSTR_MOV_R32_AR32;
		}
		break;
		// 01 + 00 + 6B
		case 0x6B :
		{
			// read
			FETCH_BYTE(4);
			FETCH_BYTE(5);
			// 01 00 6B ..
			if ((instr[3] & 0x20) == 0x00)
			{
				// 01 00 6B 03 EC 00 : mov.l @0xEC00:16, er3
				// 01 00 6B 0x aa aa
				if ((instr[3] & 0xF8) == 0x00) return INSTR_MOV_AI16_R32;
				// 01 00 6B 82 F4 10 : mov.l er2, @0xF410:16
				// 01 00 6B 8x aa aa
				if ((instr[3] & 0xF8) == 0x80) return INSTR_MOV_R32_AI16;
			}
			else
			{
				// read
				FETCH_BYTE(6);
				FETCH_BYTE(7);
				// 01 00 6B 23 00 20 00 6C : mov.l @0x20006C:32, er3
				// 01 00 6B 2x dd dd dd dd
				if ((instr[3] & 0xF8) == 0x20) return INSTR_MOV_AI32_R32;
				// 01 00 6B A0 00 20 00 0C : mov.l er0, @0x20000C:32
				// 01 00 6B Ax dd dd dd dd
				if ((instr[3] & 0xF8) == 0xA0) return INSTR_MOV_R32_AI32;
			}
		}
		break;
		// 01 + 00 + 6D
		case 0x6D :
		{
			// 01 00 6D F2 : push.l er2
			// 01 00 6D Fx
			#ifdef ALIAS_PUSH
			if ((instr[3] & 0xF8) == 0xF0) return INSTR_PUSH_R32; // mov.l erx, @-sp
			#endif
			// 01 00 6D 74 : pop.l er4
			// 01 00 6D 7x
			#ifdef ALIAS_POP
			if ((instr[3] & 0xF8) == 0x70) return INSTR_POP_R32; // mov.l @sp+, erx
			#endif
			// 01 00 6D C2 : mov.l er2, @-er5
			// 01 00 6D xy
			if ((instr[3] & 0x88) == 0x80) return INSTR_MOV_R32_AR32D;
			// 01 00 6D 52 : mov.l @er5+, er2
			// 01 00 6D xy
			if ((instr[3] & 0x88) == 0x00) return INSTR_MOV_AR32I_R32;
		}
		break;
		// 01 + 00 + 6F
		case 0x6F :
		{
			// read
			FETCH_BYTE(4);
			FETCH_BYTE(5);
			// 01 00 6F 53 12 34 : mov.l @(0x1234:16,er5), er3
			// 01 00 6F xy dd dd
			if ((instr[3] & 0x88) == 0x00) return INSTR_MOV_AI16R32_R32;
			// 01 00 6F D3 12 34 : mov.l er3, @(0x1234:16,er5)
			// 01 00 6F xy dd dd
			if ((instr[3] & 0x88) == 0x80) return INSTR_MOV_R32_AI16R32;
		}
		break;
		// 01 + 00 + 78
		case 0x78 :
		{
			// read
			FETCH_BYTE(4);
			FETCH_BYTE(5);
			// 01 00 78 xx 6B ..
			if (instr[4] == 0x6B)
			{
				// read
				FETCH_BYTE(6);
				FETCH_BYTE(7);
				FETCH_BYTE(8);
				FETCH_BYTE(9);
				// 01 00 78 10 6B 25 00 20 98 92 : mov.l @(0x209892:32,er1), er5
				// 01 00 78 x0 6B 2y dd dd dd dd
				if (((instr[3] & 0x8F) == 0x00) && ((instr[5] & 0xF8) == 0x20)) return INSTR_MOV_AI32R32_R32;
				// 01 00 78 90 6B A5 00 20 98 92 : mov.l er5, @(0x209892:32,er1)
				// 01 00 78 x0 6B Ay dd dd dd dd
				if (((instr[3] & 0x8F) == 0x80) && ((instr[5] & 0xF8) == 0xA0)) return INSTR_MOV_R32_AI32R32;
			}
		}
		break;
		// 01 + 00 + other
		default :
		{
			return INSTR_UNKNOWN;
		}
		break;
	}
	// other
	return INSTR_UNKNOWN;
}

//------------------------------------------------------------------------------
UINT16 h8s_opcode_find_01_C0( UINT32 pc, UINT8 *instr, UINT8 fetch)
{
	// read
	FETCH_BYTE(2);
	FETCH_BYTE(3);
	// 01 + C0 + xx
	switch (instr[2])
	{
		// 01 + C0 + 50
		case 0x50 :
		{
			// 01 C0 50 xy
			return INSTR_MULXS_R8_R16;
		}
		break;
		// 01 + C0 + 52
		case 0x52 :
		{
			// 01 C0 52 56 : mulxs.w r5, er6
			// 01 C0 52 xy
			if ((instr[3] & 0x08) == 0x00) return INSTR_MULXS_R16_R32;
		}
		break;
		// 01 + C0 + other
		default :
		{
			return INSTR_UNKNOWN;
		}
		break;
	}
	// other
	return INSTR_UNKNOWN;
}

//------------------------------------------------------------------------------
UINT16 h8s_opcode_find_01_D0( UINT32 pc, UINT8 *instr, UINT8 fetch)
{
	// read
	FETCH_BYTE(2);
	FETCH_BYTE(3);
	// 01 + D0 + xx
	switch (instr[2])
	{
		// 01 + D0 + 53
		case 0x53 :
		{
			// 01 D0 53 46 : divxs.w r4, er6
			// 01 D0 53 xy
			if ((instr[3] & 0x08) == 0x00) return INSTR_DIVXS_R16_R32;
		}
		break;
		// 01 + D0 + other
		default :
		{
			return INSTR_UNKNOWN;
		}
		break;
	}
	// other
	return INSTR_UNKNOWN;
}

//------------------------------------------------------------------------------
UINT16 h8s_opcode_find_01( UINT32 pc, UINT8 *instr, UINT8 fetch)
{
	// 01 + xx
	switch (instr[1])
	{
		// 01 + 00
		case 0x00 :
		{
			return h8s_opcode_find_01_00( pc, instr, fetch);
		}
		break;
		// 01 + 10..30
		case 0x10 :
		case 0x20 :
		case 0x30 :
		{
			// read
			FETCH_BYTE(2);
			FETCH_BYTE(3);
			// 01 + 10..30 + xx
			switch (instr[2])
			{
				// 01 + 10..30 + 6D
				case 0x6D :
				{
					// 01 10 6D 75 : ldm.l @sp+, er4-er5
					// 01 20 6D 76 : ldm.l @sp+, er4-er6
					// 01 x0 6D zy
					if ((instr[3] & 0x88) == 0x00) return INSTR_LDM_AR32I_R32R32;
					// 01 10 6D F4 : stm.l er4-er5, @-sp
					// 01 30 6D F0 : stm.l er0-er3, @-sp
					// 01 x0 6D zy
					if ((instr[3] & 0x88) == 0x80) return INSTR_STM_R32R32_AR32D;
				}
				break;
				// 01 + 10..30 + other
				default :
				{
					return INSTR_UNKNOWN;
				}
				break;
			}
		}
		break;
		// 01 + 40
		case 0x40 :
		{
			// read
			FETCH_BYTE(2);
			FETCH_BYTE(3);
			// 01 + 40 + xx
			switch (instr[2])
			{
				// 01 + 40 + 6D + xx
				case 0x6D :
				{
					// 01 40 6D F0 : stc.w ccr, @-sp
					// 01 40 6D x0
					if ((instr[3] & 0x8F) == 0x80) return INSTR_STC_CCR_AR32D;
					// 01 40 6D 70 : ldc.w @sp+, ccr
					// 01 40 6D x0
					if ((instr[3] & 0x8F) == 0x00) return INSTR_LDC_AR32I_CCR;
				}
				break;
				// 01 + 40 + 6D + other
				default :
				{
					return INSTR_UNKNOWN;
				}
				break;

			}
		}
		break;
		// 01 + 80
		case 0x80 :
		{
			// 01 80 : sleep
			// 01 80
			return INSTR_SLEEP;
		}
		break;
		// 01 + C0
		case 0xC0 :
		{
			return h8s_opcode_find_01_C0( pc, instr, fetch);
		}
		break;
		// 01 + D0
		case 0xD0 :
		{
			return h8s_opcode_find_01_D0( pc, instr, fetch);
		}
		break;
		// 01 + F0
		case 0xF0 :
		{
			// read
			FETCH_BYTE(2);
			FETCH_BYTE(3);
			// 01 + F0 + xx
			switch (instr[2])
			{
				// 01 + F0 + 64
				case 0x64 :
				{
					// 01 F0 64 05 : or.l er0, er5
					// 01 F0 64 xy
					if ((instr[3] & 0x88) == 0x00) return INSTR_OR_R32_R32;
				}
				break;
				// 01 + F0 + 65
				case 0x65 :
				{
					// 01 F0 65 32 : xor.l er3, er2
					// 01 F0 65 xy
					if ((instr[3] & 0x88) == 0x00) return INSTR_XOR_R32_R32;
				}
				break;
				// 01 + F0 + 66
				case 0x66 :
				{
					// 01 F0 66 32 : and.l er3, er2
					// 01 F0 66 xy
					if ((instr[3] & 0x88) == 0x00) return INSTR_AND_R32_R32;
				}
				break;
				// 01 + F0 + other
				default :
				{
					return INSTR_UNKNOWN;
				}
				break;
			}
		}
		break;
		// 01 + other
		default :
		{
			return INSTR_UNKNOWN;
		}
		break;
	}
	// other
	return INSTR_UNKNOWN;
}

//------------------------------------------------------------------------------
UINT16 h8s_opcode_find( UINT32 pc, UINT8 *instr, UINT8 fetch)
{
	// read
	FETCH_BYTE(0);
	FETCH_BYTE(1);
	// xx
	switch (instr[0])
	{
		// 00
		case 0x00 :
		{
			if (instr[1] == 0x00) return INSTR_NOP;
		}
		break;
		// 01
		case 0x01 :
		{
			return h8s_opcode_find_01( pc, instr, fetch);
		}
		break;
		// 02
		case 0x02 :
		{
			// 02 08 : stc.b ccr, r0l
			// 02 0x
			if ((instr[1] & 0xF0) == 0x00) return INSTR_STC_CCR_R8;
		}
		break;
		// 03
		case 0x03 :
		{
			// 03 0C : ldc.b r4l, ccr
			// 03 0x
			if ((instr[1] & 0xF0) == 0x00) return INSTR_LDC_R8_CCR;
		}
		break;
		// 04
		case 0x04 :
		{
			// 04 80 : orc #0x80, ccr
			// 04 dd
			return INSTR_ORC_I8_CCR;
		}
		break;
		// 06
		case 0x06 :
		{
			// 06 7F : andc #0x7F, ccr
			// 06 dd
			return INSTR_ANDC_I8_CCR;
		}
		break;
		// 08
		case 0x08 :
		{
			// 08 DC : add.b r5l, r4l
			// 08 xy
			return INSTR_ADD_R8_R8;
		}
		break;
		// 09
		case 0x09 :
		{
			// 09 02 : add.w r0, r2
			// 09 8E : add.w e0, e6
			// 09 xy
			return INSTR_ADD_R16_R16;
		}
		break;
		// 0A
		case 0x0A :
		{
			// 0A 04 : inc.b r4h | 0A 0x | only supports inc #1
			if ((instr[1] & 0xF0) == 0x00) return INSTR_INC_XX_R8;
			// 0A 82 : add.l er0, er2
			// 0A xy
			if ((instr[1] & 0x88) == 0x80) return INSTR_ADD_R32_R32;
		}
		break;
		// 0B
		case 0x0B :
		{
			// 0B 03 : adds #1, er3
			// 0B 84 : adds #2, er4
			// 0B 97 : adds #4, sp
			if ((instr[1] & 0xF8) == 0x00) return INSTR_ADDS_XX_R32;
			if ((instr[1] & 0xF8) == 0x80) return INSTR_ADDS_XX_R32;
			if ((instr[1] & 0xF8) == 0x90) return INSTR_ADDS_XX_R32;

			// 0B 5E : inc.w #1, e6  | 0B DE : inc.w #2, e6
			if ((instr[1] & 0x70) == 0x50) return INSTR_INC_XX_R16;
			// 0B 75 : inc.l #1, er5 | 0B F0 : inc.l #2, er0
			if ((instr[1] & 0x78) == 0x70) return INSTR_INC_XX_R32;
		}
		break;
		// 0C | 0C 88 : mov.b r0l, r0l | 0C xy
		case 0x0C :	return INSTR_MOV_R8_R8; break;
		// 0D | 0D 00 : mov.w r0, r0 | 0D 2A : mov.w r2, e2 | 0D B1 : mov.w e3, r1 | 0D xy
		case 0x0D : return INSTR_MOV_R16_R16; break;
		// 0E | 0E C0 : addx r4l,r0h | 0E xy : addx r4l,r0h | 0E xy
		case 0x0E : return INSTR_ADDX_R8_R8; break;
		// 0F
		case 0x0F :
		{
			// 0F C1 : mov.l er4, er1
			// 0F xy
			if ((instr[1] & 0x88) == 0x80) return INSTR_MOV_R32_R32;
		}
		break;
		// 10
		case 0x10 :
		{
			// 10 08 : shll.b #1, r0l | 10 48 : shll.b #2, r0l
			if ((instr[1] & 0xB0) == 0x00) return INSTR_SHLL_XX_R8;
			// 10 83 : shal.b #1, r3h | 10 C3 : shal.b #2, r3h
			if ((instr[1] & 0xB0) == 0x80) return INSTR_SHAL_XX_R8;
			// 10 9E : shal.w #1, e6  | 10 DE : shal.w #2, e6
			if ((instr[1] & 0xB0) == 0x90) return INSTR_SHAL_XX_R16;
			// 10 B3 : shal.l #1, er3 | 10 F3 : shal.l #2, er3
			if ((instr[1] & 0xB8) == 0xB0) return INSTR_SHAL_XX_R32;
			// 10 1C : shll.w #1, e4  | 10 52 : shll.w #2, r2
			if ((instr[1] & 0xB0) == 0x10) return INSTR_SHLL_XX_R16;
			// 10 32 : shll.l #1, er2 | 10 75 : shll.l #2, er5
			if ((instr[1] & 0xB8) == 0x30) return INSTR_SHLL_XX_R32;
		}
		break;
		// 11
		case 0x11 :
		{
			// 11 0C : shlr.b #1, r4l | 11 4C : shlr.b #2, r4l
			if ((instr[1] & 0xB0) == 0x00) return INSTR_SHLR_XX_R8;
			// 11 12 : shlr.w #1, r2  | 11 5E : shlr.w #2, e6
			if ((instr[1] & 0xB0) == 0x10) return INSTR_SHLR_XX_R16;
			// 11 30 : shlr.l #1, er0 | 11 72 : shlr.l #2, er2
			if ((instr[1] & 0xB8) == 0x30) return INSTR_SHLR_XX_R32;
			// 11 9D : shar.w #1, e5  | 11 D6 : shar.w #2, r6
			if ((instr[1] & 0xB0) == 0x90) return INSTR_SHAR_XX_R16;
			// 11 B0 : shar.l #1, er0 | 11 F0 : shar.l #2, er0
			if ((instr[1] & 0xB8) == 0xB0) return INSTR_SHAR_XX_R32;
		}
		break;
		// 12
		case 0x12 :
		{
			// 12 0B : rotxl.b #1, r3l | 12 4B : rotxl.b #2, r3l
			if ((instr[1] & 0xB0) == 0x00) return INSTR_ROTXL_XX_R8;
			// 12 30 : rotxl.l #1, er0 | 12 70 : rotxl.l #2, er0
			if ((instr[1] & 0xB8) == 0x30) return INSTR_ROTXL_XX_R32;
			// 12 8A : rotl.b #1, r2l  | 12 C9 : rotl.b #2, r1l
			if ((instr[1] & 0xB0) == 0x80) return INSTR_ROTL_XX_R8;
			// 12 93 : rotl.w #1, r3   | 12 D3 : rotl.w #2, r3
			if ((instr[1] & 0xB0) == 0x90) return INSTR_ROTL_XX_R16;
			// 12 B2 : rotl.l #1, er2  | 12 F2 : rotl.l #1, er2
			if ((instr[1] & 0xB8) == 0xB0) return INSTR_ROTL_XX_R32;
		}
		break;
		// 13
		case 0x13 :
		{
			// 13 89 : rotr.b #1, r1l | 13 CA : rotr.b #2, r2l
			if ((instr[1] & 0xB0) == 0x80) return INSTR_ROTR_XX_R8;
			// 13 B3 : rotr.l #1, er3 | 13 F3 : rotr.l #2, er3
			if ((instr[1] & 0xB8) == 0xB0) return INSTR_ROTR_XX_R32;
		}
		break;
		// 14
		case 0x14 :
		{
			// 14 9A : or.b r1l, r2l
			// 14 xy
			return INSTR_OR_R8_R8;
		}
		break;
		// 15
		case 0x15 :
		{
			// 15 BA : xor.b r3l, r2l
			// 15 xy
			return INSTR_XOR_R8_R8;
		}
		break;
		// 16
		case 0x16 :
		{
			// 16 8A : and.b r0l, r2l
			// 16 xy
			return INSTR_AND_R8_R8;
		}
		break;
		// 17
		case 0x17 :
		{
			// 17 08 : not.b r0l  | 17 0x
			if ((instr[1] & 0xF0) == 0x00) return INSTR_NOT_R8;
			// 17 11 : not.w r1   | 17 1x
			if ((instr[1] & 0xF0) == 0x10) return INSTR_NOT_R16;
			// 17 32 : not.l er2  | 17 3x
			if ((instr[1] & 0xF8) == 0x30) return INSTR_NOT_R32;
			// 17 53 : extu.w r3  | 17 5x
			if ((instr[1] & 0xF0) == 0x50) return INSTR_EXTU_R16;
			// 17 74 : extu.l er4 | 17 7x
			if ((instr[1] & 0xF8) == 0x70) return INSTR_EXTU_R32;
			// 17 86 : neg.b r6h  | 17 8x
			if ((instr[1] & 0xF0) == 0x80) return INSTR_NEG_R8;
			// 17 9D : neg.w e5   | 17 9x
			if ((instr[1] & 0xF0) == 0x90) return INSTR_NEG_R16;
			// 17 B0 : neg.l er0  | 17 Bx
			if ((instr[1] & 0xF8) == 0xB0) return INSTR_NEG_R32;
			// 17 D3 : exts.w r3  | 17 Dx
			if ((instr[1] & 0xF0) == 0xD0) return INSTR_EXTS_R16;
			// 17 F2 : exts.l er2 | 17 Fx
			if ((instr[1] & 0xF8) == 0xF0) return INSTR_EXTS_R32;
		}
		break;
		// 18
		case 0x18 :
		{
			// 18 AA : sub.b r2l, r2l
			// 18 xy
			return INSTR_SUB_R8_R8;
		}
		break;
		// 19 | 19 11 : sub.w r1, r1 | 19 99 : sub.w e1, e1 | 19 xy
		case 0x19 :
		{
			return INSTR_SUB_R16_R16;
		}
		break;
		// 1A
		case 0x1A :
		{
			// 1A 0E : dec.b r6l | 1A 0x
			if ((instr[1] & 0xF0) == 0x00) return INSTR_DEC_XX_R8;
			// 1A A2 : sub.l er2, er2 | 1A xy
			if ((instr[1] & 0x88) == 0x80) return INSTR_SUB_R32_R32;
		}
		break;
		// 1B
		case 0x1B :
		{
			// 1B 5A : dec.w #1, e2  | 1B D0 : dec.w #2, r0
			if ((instr[1] & 0x70) == 0x50) return INSTR_DEC_XX_R16;
			// 1B 74 : dec.l #1, er4 | 1B F0 : dec.l #2, er0
			if ((instr[1] & 0x78) == 0x70) return INSTR_DEC_XX_R32;
			// 1B 03 : subs #1, er3
			// 1B 83 : subs #2, er3
			// 1B 94 : subs #4, er4
			if ((instr[1] & 0xF8) == 0x00) return INSTR_SUBS_XX_R32;
			if ((instr[1] & 0xF8) == 0x80) return INSTR_SUBS_XX_R32;
			if ((instr[1] & 0xF8) == 0x90) return INSTR_SUBS_XX_R32;
		}
		break;
		// 1C | 1C BA : cmp.b r3l, r2l | 1C xy
		case 0x1C :
		{
			return INSTR_CMP_R8_R8;
		}
		break;
		// 1D | 1D 23 : cmp.w r2, r3 | 1D 4C : cmp.w r4, e4 | 1D xy
		case 0x1D :
		{
			return INSTR_CMP_R16_R16;
		}
		break;
		// 1E | 1E 55 : subx r5h, r5h | 1E xy
		case 0x1E :
		{
			return INSTR_SUBX_R8_R8;
		}
		break;
		// 1F | 1F 83 : cmp.l er0, er3 | 1F xy
		case 0x1F :
		{
			if ((instr[1] & 0x88) == 0x80) return INSTR_CMP_R32_R32;
		}
		break;
		// 20..2F | 2A 61 : mov.b @0xFFFFFF61:8, r2l | 2x dd
		case 0x20 : case 0x21 : case 0x22 : case 0x23 :
		case 0x24 : case 0x25 :	case 0x26 :	case 0x27 :
		case 0x28 :	case 0x29 :	case 0x2A :	case 0x2B :
		case 0x2C :	case 0x2D :	case 0x2E :	case 0x2F :
		{
			return INSTR_MOV_AI8_R8;
		}
		break;
		// 30..3F | 3A 88 : mov.b r2l, @0xFFFFFF88:8 | 3x dd
		case 0x30 : case 0x31 : case 0x32 : case 0x33 :
		case 0x34 : case 0x35 :	case 0x36 :	case 0x37 :
		case 0x38 :	case 0x39 :	case 0x3A :	case 0x3B :
		case 0x3C :	case 0x3D :	case 0x3E :	case 0x3F :
		{
			return INSTR_MOV_R8_AI8;
		}
		break;
		// 40 | 40 FE : bra xxxxxxxx:8 | 40 aa
		case 0x40 : return INSTR_BRA_O8; break;
		// 41 | 41 FC : brn xxxxxxxx:8 | 41 aa
		case 0x41 : return INSTR_BRN_O8; break;
		// 42 | 42 F4 : bhi xxxxxxxx:8 | 42 aa
		case 0x42 : return INSTR_BHI_O8; break;
		// 43 | 43 0C : bls xxxxxxxx:8 | 43 aa
		case 0x43 : return INSTR_BLS_O8; break;
		// 44 | 44 0A : bcc xxxxxxxx:8 | 44 aa
		case 0x44 : return INSTR_BCC_O8; break;
		// 45 | 45 F6 : bcs xxxxxxxx:8 | 45 aa
		case 0x45 : return INSTR_BCS_O8; break;
		// 46 | 46 10 : bne xxxxxxxx:8 | 46 aa
		case 0x46 : return INSTR_BNE_O8; break;
		// 47 | 47 CC : beq xxxxxxxx:8 | 47 aa
		case 0x47 : return INSTR_BEQ_O8; break;
		// 48 | 48 EE : bvc xxxxxxxx:8 | 48 aa
		case 0x48 : return INSTR_BVC_O8; break;
		// 49 | 49 EE : bvs xxxxxxxx:8 | 49 aa
		case 0x49 : return INSTR_BVS_O8; break;
		// 4A | 4A 0A : bpl xxxxxxxx:8 | 4A aa
		case 0x4A : return INSTR_BPL_O8; break;
		// 4B | 4B 08 : bmi xxxxxxxx:8 | 4B aa
		case 0x4B : return INSTR_BMI_O8; break;
		// 4C | 4C 10 : bge xxxxxxxx:8 | 4C aa
		case 0x4C : return INSTR_BGE_O8; break;
		// 4D | 4D EE : blt xxxxxxxx:8 | 4D aa
		case 0x4D : return INSTR_BLT_O8; break;
		// 4E | 4E 20 : bgt xxxxxxxx:8 | 4E aa
		case 0x4E : return INSTR_BGT_O8; break;
		// 4F | 4F 30 : ble xxxxxxxx:8 | 4F aa
		case 0x4F : return INSTR_BLE_O8; break;
		// 50 | 50 D4 : mulxu.b r5l, r4 | 50 xy
		case 0x50 : return INSTR_MULXU_R8_R16; break;
		// 51 | 51 B2 : divxu.b r3l, r2 | 51 xy
		case 0x51 : return INSTR_DIVXU_R8_R16; break;
		// 52
		case 0x52 :
		{
			// 52 12 : mulxu.w r1, er2
			// 52 D1 : mulxu.w e5, er1
			// 52 xy
			if ((instr[1] & 0x08) == 0x00) return INSTR_MULXU_R16_R32;
		}
		break;
		// 53
		case 0x53 :
		{
			// 53 60 : divxu.w r6, er0
			// 53 B1 : divxu.w e3, er1
			// 53 xy
			if ((instr[1] & 0x08) == 0x00) return INSTR_DIVXU_R16_R32;
		}
		break;
		// 54
		case 0x54 :
		{
			// 54 70 : rts
			if (instr[1] == 0x70) return INSTR_RTS;
		}
		break;
		// 55 | 55 B0 : bsr xxxxxxxx:8 | 55 oo
		case 0x55 : return INSTR_BSR_O8; break;
		// 56
		case 0x56 :
		{
			// 56 70 : rte
			if (instr[1] == 0x70) return INSTR_RTE;
		}
		break;
		// 58
		case 0x58 :
		{
			// read
			FETCH_BYTE(2);
			FETCH_BYTE(3);
			// 58 + xx
			switch (instr[1])
			{
				// 58 + 00 | 58 00 FF 70 : bra xxxxxxxx:16 | 58 00 aa aa
				case 0x00 : return INSTR_BRA_O16; break;
				// 58 + 10 | 58 10 FF 70 : brn xxxxxxxx:16 | 58 10 aa aa
				case 0x10 : return INSTR_BRN_O16; break;
				// 58 + 20 | 58 20 FE 68 : bhi xxxxxxxx:16 | 58 20 aa aa
				case 0x20 : return INSTR_BHI_O16; break;
				// 58 + 30 | 58 30 FF 3A : bls xxxxxxxx:16 | 58 30 aa aa
				case 0x30 : return INSTR_BLS_O16; break;
				// 58 + 40 | 58 40 00 DE : bcc xxxxxxxx:16 | 58 40 aa aa
				case 0x40 : return INSTR_BCC_O16; break;
				// 58 + 50 | 58 50 FF 68 : bcs xxxxxxxx:16 | 58 50 aa aa
				case 0x50 : return INSTR_BCS_O16; break;
				// 58 + 60 | 58 60 FF 70 : bne xxxxxxxx:16 | 58 60 aa aa
				case 0x60 : return INSTR_BNE_O16; break;
				// 58 + 70 | 58 70 00 94 : beq xxxxxxxx:16 | 58 70 aa aa
				case 0x70 : return INSTR_BEQ_O16; break;
				// 58 + 80 | 58 80 00 94 : bvc xxxxxxxx:16 | 58 80 aa aa
				case 0x80 : return INSTR_BVC_O16; break;
				// 58 + 90 | 58 90 00 94 : bvs xxxxxxxx:16 | 58 90 aa aa
				case 0x90 : return INSTR_BVS_O16; break;
				// 58 + A0 | 58 A0 00 94 : bpl xxxxxxxx:16 | 58 A0 aa aa
				case 0xA0 : return INSTR_BPL_O16; break;
				// 58 + B0 | 58 B0 00 94 : bmi xxxxxxxx:16 | 58 B0 aa aa
				case 0xB0 : return INSTR_BMI_O16; break;
				// 58 + C0 | 58 C0 FF 04 : bge xxxxxxxx:16 | 58 C0 aa aa
				case 0xC0 : return INSTR_BGE_O16; break;
				// 58 + D0 | 58 D0 FF 58 : blt xxxxxxxx:16 | 58 D0 aa aa
				case 0xD0 : return INSTR_BLT_O16; break;
				// 58 + E0 | 58 E0 00 B0 : bgt xxxxxxxx:16 | 58 E0 aa aa
				case 0xE0 : return INSTR_BGT_O16; break;
				// 58 + F0 | 58 F0 FF 50 : ble xxxxxxxx:16 | 58 F0 aa aa
				case 0xF0 : return INSTR_BLE_O16; break;
				// 58 + other
				default :
				{
					return INSTR_UNKNOWN;
				}
				break;
			}
		}
		break;
		// 59
		case 0x59 :
		{
			// 59 00 : jmp @er0
			// 59 20 : jmp @er2
			// 59 x0
			if ((instr[1] & 0x8F) == 0x00) return INSTR_JMP_AR32;
		}
		break;
		// 5A
		case 0x5A :
		{
			// read
			FETCH_BYTE(2);
			FETCH_BYTE(3);
			// 5A 00 4B 46 : jmp 00004B46:24
			// 5A aa aa aa
			return INSTR_JMP_I24;
		}
		break;
		// 5C
		case 0x5C :
		{
			// read
			FETCH_BYTE(2);
			FETCH_BYTE(3);
			// 5C 00 FF 70 : bsr sub_0_200038:16
			if (instr[1] == 0x00) return INSTR_BSR_O16;
		}
		break;
		// 5D
		case 0x5D :
		{
			// 5D 20 : jsr @er2
			// 5D x0
			if ((instr[1] & 0x8F) == 0x00) return INSTR_JSR_AR32;
		}
		break;
		// 5E
		case 0x5E :
		{
			// read
			FETCH_BYTE(2);
			FETCH_BYTE(3);
			// 5E 00 15 94 : jsr 00001594:24
			// 5E dd dd dd
			return INSTR_JSR_I24;
		}
		break;
		// 62 | 62 93 : bclr r1l, r3h | 62 xy
		case 0x62 : return INSTR_BCLR_R8_R8; break;
		// 64 | 64 24 : or.w r2, r4 | 64 DD : or.w e5, e5 | 64 xy
		case 0x64 : return INSTR_OR_R16_R16; break;
		// 65 | 65 32 : xor.w r3, r2 | 65 xy
		case 0x65 : return INSTR_XOR_R16_R16; break;
		// 66 | 66 32 : and.w r3, r2 | 66 B8 : and.w e3, e0 | 66 xy
		case 0x66 : return INSTR_AND_R16_R16; break;
		// 67 | 67 08 : bst #0, r0l | 67 xy
		case 0x67 :
		{
			if ((instr[1] & 0x80) == 0x00) return INSTR_BST_Ix_R8;
		}
		break;
		// 68
		case 0x68 :
		{
			// 68 4A : mov.b @er4, r2l
			// 68 xy
			if ((instr[1] & 0x80) == 0x00) return INSTR_MOV_AR32_R8;
			// 68 BA : mov.b r2l, @er3
			// 68 yx
			if ((instr[1] & 0x80) == 0x80) return INSTR_MOV_R8_AR32;
		}
		break;
		// 69
		case 0x69 :
		{
			// 69 42 : mov.w @er4, r2
			// 69 xy
			if ((instr[1] & 0x80) == 0x00) return INSTR_MOV_AR32_R16;
			// 69 92 : mov.w r2, @er1
			// 69 F8 : mov.w e0, @sp
			// 69 xy
			if ((instr[1] & 0x80) == 0x80) return INSTR_MOV_R16_AR32;
		}
		break;
		// 6A
		case 0x6A :
		{
			// read
			FETCH_BYTE(2);
			FETCH_BYTE(3);
			// 6A ..
			if ((instr[1] & 0x20) == 0x00)
			{
				// 6A 0A FE D5 : mov.b @0xFED5:16, r2l
				// 6A 0x aa aa
				if ((instr[1] & 0xF0) == 0x00) return INSTR_MOV_AI16_R8;
				// 6A 88 FE D0 : mov.b r0l, @0xFED0:16
				// 6A 8x aa aa
				if ((instr[1] & 0xF0) == 0x80) return INSTR_MOV_R8_AI16;
			}
			else
			{
				// read
				FETCH_BYTE(4);
				FETCH_BYTE(5);
				// 6A 28 00 20 00 00 : mov.b @0x200000:32, r0l
				// 6A 2x dd dd dd dd
				if ((instr[1] & 0xF0) == 0x20) return INSTR_MOV_AI32_R8;
				// 6A AA 00 20 00 00 : mov.b r2l, @0x200000:32
				// 6A Ax dd dd dd dd
				if ((instr[1] & 0xF0) == 0xA0) return INSTR_MOV_R8_AI32;
				// 6A 3x ..
				if ((instr[1] & 0xF0) == 0x30)
				{
					// read
					FETCH_BYTE(6);
					FETCH_BYTE(7);
					// 6A 30 00 23 06 6C 73 00 : btst #0, @0x23066C:32
					// 6A 30 aa aa aa aa 73 x0
					if ((instr[1] == 0x30) && (instr[6] == 0x73) && ((instr[7] & 0x8F) == 0x00)) return INSTR_BTST_Ix_AI32;
					// 6A 38 00 FF FF 61 71 30 : bnot #3, @0xFFFF61:32
					// 6A 38 aa aa aa aa 71 x0
					if ((instr[1] == 0x38) && (instr[6] == 0x71) && ((instr[7] & 0x8F) == 0x00)) return INSTR_BNOT_Ix_AI32;
					// 6A 38 00 4B 49 7C 60 80 : bset r0l, @0x4B497C:32
					// 6A 38 aa aa aa aa 60 x0
					if ((instr[1] == 0x38) && (instr[6] == 0x60) && ((instr[7] & 0x0F) == 0x00)) return INSTR_BSET_R8_AI32;
					// 6A 38 00 FF FF 3C 62 90 : bclr r1l, @0xFFFF3C:32
					// 6A 38 aa aa aa aa 62 x0
					if ((instr[1] == 0x38) && (instr[6] == 0x62) && ((instr[7] & 0x0F) == 0x00)) return INSTR_BCLR_R8_AI32;
					// 6A 38 00 FF FF 8C 72 70 : bclr #7, @0xFFFF8C:32
					// 6A 38 aa aa aa aa 72 x0
					if ((instr[1] == 0x38) && (instr[6] == 0x72) && ((instr[7] & 0x8F) == 0x00)) return INSTR_BCLR_Ix_AI32;
					// 6A 38 00 FF FF 34 70 10 : bset #1, @0xFFFF34:32
					// 6A 38 aa aa aa aa 70 x0
					if ((instr[1] == 0x38) && (instr[6] == 0x70) && ((instr[7] & 0x8F) == 0x00)) return INSTR_BSET_Ix_AI32;
				}
			}
		}
		break;
		// 6B
		case 0x6B :
		{
			// read
			FETCH_BYTE(2);
			FETCH_BYTE(3);
			// 6B ..
			if ((instr[1] & 0x20) == 0x00)
			{
				// 6B 02 FF 3C : mov.w @0xFF3C:16, r2
				// 6B 0x dd dd
				if ((instr[1] & 0xF0) == 0x00) return INSTR_MOV_AI16_R16;
				// 6B 82 FF 3C : mov.w r2, @0xFF3C:16
				// 6B 8x dd dd
				if ((instr[1] & 0xF0) == 0x80) return INSTR_MOV_R16_AI16;
			}
			else
			{
				// read
				FETCH_BYTE(4);
				FETCH_BYTE(5);
				// 6B 22 00 20 02 5E : mov.w @0x20025E:32, r2
				// 6B 2x dd dd dd dd
				if ((instr[1] & 0xF0) == 0x20) return INSTR_MOV_AI32_R16;
				// 6B A4 00 20 02 5E : mov.w r4, @0x20025E:32
				// 6B Ax dd dd dd dd
				if ((instr[1] & 0xF0) == 0xA0) return INSTR_MOV_R16_AI32;
			}
		}
		break;
		// 6C
		case 0x6C :
		{
			// 6C 1A : mov.b @er1+, r2l
			// 6C xy
			if ((instr[1] & 0x80) == 0x00) return INSTR_MOV_AR32I_R8;
			// 6C CA : mov.b r2l, @-er4
			// 6C xy
			if ((instr[1] & 0x80) == 0x80) return INSTR_MOV_R8_AR32D;
		}
		break;
		// 6D
		case 0x6D :
		{
			// 6D F6 : push.w r6
			// 6D Fx
			#ifdef ALIAS_PUSH
			if ((instr[1] & 0xF0) == 0xF0) return INSTR_PUSH_R16; // mov.w rx, @-sp
			#endif
			// 6D 76 : pop.w r6
			// 6D 7x
			#ifdef ALIAS_POP
			if ((instr[1] & 0xF0) == 0x70) return INSTR_POP_R16; // mov.w @sp+, rx
			#endif
			// 6D B1 : mov.w r1, @-er3
			// 6D xy
			if ((instr[1] & 0x80) == 0x80) return INSTR_MOV_R16_AR32D;
			// 6D 53 : mov.w @er5+, r3
			// 6D xy
			if ((instr[1] & 0x80) == 0x00) return INSTR_MOV_AR32I_R16;
		}
		break;
		// 6E
		case 0x6E :
		{
			// read
			FETCH_BYTE(2);
			FETCH_BYTE(3);
			// 6E 79 00 0F : mov.b @(0x000F:16,sp), r1l
			// 6E xy dd dd
			if ((instr[1] & 0x80) == 0x00) return INSTR_MOV_AI16R32_R8;
			// 6E DA 04 0C : mov.b r2l, @(0x40C:16,er5)
			// 6E xy dd dd
			if ((instr[1] & 0x80) == 0x80) return INSTR_MOV_R8_AI16R32;
		}
		break;
		// 6F
		case 0x6F :
		{
			// read
			FETCH_BYTE(2);
			FETCH_BYTE(3);
			// 6F 50 04 14 : mov.w @(0x0414:16,er5), r0
			// 6F 7C 00 0E : mov.w @(0x000E:16,sp), e4
			// 6F xy dd dd
			if ((instr[1] & 0x80) == 0x00) return INSTR_MOV_AI16R32_R16;
			// 6F D0 04 0E : mov.w r0, @(0x040E:16,er5)
			// 6F F8 00 04 : mov.w e0, @(0x0004:16,sp)
			// 6F xy dd dd
			if ((instr[1] & 0x80) == 0x80) return INSTR_MOV_R16_AI16R32;
		}
		break;
		// 72
		case 0x72 :
		{
			// 72 4A : bclr #4, r2l
			// 72 xy
			if ((instr[1] & 0x80) == 0x00) return INSTR_BCLR_Ix_R8;
		}
		break;
		// 73
		case 0x73 :
		{
			// 73 6A : btst #6, r2l
			// 73 xy
			if ((instr[1] & 0x80) == 0x00) return INSTR_BTST_Ix_R8;
		}
		break;
		// 76
		case 0x76 :
		{
			// 76 25 : band #2, r5h
			// 76 xy
			if ((instr[1] & 0x80) == 0x00) return INSTR_BAND_Ix_R8;
			// 76 C2 : biand #4, r2h
			// 76 xy
			if ((instr[1] & 0x80) == 0x80) return INSTR_BIAND_Ix_R8;
		}
		break;
		// 77
		case 0x77 :
		{
			// 77 2A : bld #2, r2l
			// 77 xy
			if ((instr[1] & 0x80) == 0x00) return INSTR_BLD_Ix_R8;
			// 77 C2 : bild #4, r2h
			// 77 xy
			if ((instr[1] & 0x80) == 0x80) return INSTR_BILD_Ix_R8;
		}
		break;
		// 78
		case 0x78 :
		{
			// read
			FETCH_BYTE(2);
			FETCH_BYTE(3);
			FETCH_BYTE(4);
			FETCH_BYTE(5);
			FETCH_BYTE(6);
			FETCH_BYTE(7);
			// 78 00 6A 28 00 80 00 00 : mov.b @(0x800000:32,er0), r0l
			// 78 y0 6A 2x dd dd dd dd
			if (((instr[1] & 0x8F) == 0x00) && (instr[2] == 0x6A) && ((instr[3] & 0xF0) == 0x20)) return INSTR_MOV_AI32R32_R8;
			// 78 20 6B 20 00 10 04 10 : mov.w @(0x00100410:32,er2), r0
			// 78 x0 6B 2y dd dd dd dd
			if (((instr[1] & 0x8F) == 0x00) && (instr[2] == 0x6B) && ((instr[3] & 0xF0) == 0x20)) return INSTR_MOV_AI32R32_R16;
			// 78 20 6B A4 00 22 64 A6 : mov.w r4, @(0x2264A6:32,er2)
			// 78 x0 6B Ay dd dd dd dd
			if (((instr[1] & 0x8F) == 0x00) && (instr[2] == 0x6B) && ((instr[3] & 0xF0) == 0xA0)) return INSTR_MOV_R16_AI32R32;
			// 78 40 6A AD 00 80 00 00 : mov.b r5l, @(0x800000:32,er4)
			// 78 y0 6A Ax dd dd dd dd
			if (((instr[1] & 0x8F) == 0x00) && (instr[2] == 0x6A) && ((instr[3] & 0xF0) == 0xA0)) return INSTR_MOV_R8_AI32R32;
		}
		break;
		// 79
		case 0x79 :
		{
			// read
			FETCH_BYTE(2);
			FETCH_BYTE(3);
			// 79 02 3F FF : mov.w #0x3FFF, r2
			// 79 0B 00 03 : mov.w #3, e3
			// 79 0x dd dd
			if ((instr[1] & 0xF0) == 0x00) return INSTR_MOV_I16_R16;
			// 79 10 FF D0 : add.w #0xFFD0, r0
			// 79 1E 00 08 : add.w #0x0008, e6
			// 79 1x dd dd
			if ((instr[1] & 0xF0) == 0x10) return INSTR_ADD_I16_R16;
			// 79 20 FF FF : cmp.w #0xFFFF, r0
			// 79 2E 00 A0 : cmp.w #0x00A0, e6
			// 79 2x dd dd
			if ((instr[1] & 0xF0) == 0x20) return INSTR_CMP_I16_R16;
			// 79 37 00 1C : sub.w #0x1C, sp
			// 79 3x dd dd
			if ((instr[1] & 0xF0) == 0x30) return INSTR_SUB_I16_R16;
			// 79 5E 00 02 : xor.w #2, e6
			// 79 5x dd dd
			if ((instr[1] & 0xF0) == 0x50) return INSTR_XOR_I16_R16;
			// 79 63 00 01 : and.w #1, r3
			// 79 6x dd dd
			if ((instr[1] & 0xF0) == 0x60) return INSTR_AND_I16_R16;
		}
		break;
		// 7A
		case 0x7A :
		{
			// read
			FETCH_BYTE(2);
			FETCH_BYTE(3);
			FETCH_BYTE(4);
			FETCH_BYTE(5);
			// 7A 07 00 FF FB 00 : mov.l #0xFFFB00, sp
			// 7A 0x dd dd dd dd
			if ((instr[1] & 0xF8) == 0x00) return INSTR_MOV_I32_R32;
			// 7A 17 00 1F FF FC : add.l #0x1FFFFC, sp
			// 7A 1x dd dd dd dd
			if ((instr[1] & 0xF8) == 0x10) return INSTR_ADD_I32_R32;
			// 7A 22 12 34 EA CD : cmp.l #0x1234EACD, er2
			// 7A 2x dd dd dd dd
			if ((instr[1] & 0xF8) == 0x20) return INSTR_CMP_I32_R32;
			// 7A 37 00 00 10 48 : sub.l #0x1048, sp
			// 7A 3x dd dd dd dd
			if ((instr[1] & 0xF8) == 0x30) return INSTR_SUB_I32_R32;
			// 7A 45 82 00 00 00 : or.l #0x82000000, er5
			// 7A 4x dd dd dd dd
			if ((instr[1] & 0xF8) == 0x40) return INSTR_OR_I32_R32;
			// 7A 50 00 00 00 03 : xor.l #3, er0
			// 7A 5x dd dd dd dd
			if ((instr[1] & 0xF8) == 0x50) return INSTR_XOR_I32_R32;
			// 7A 63 00 00 00 0F : and.l #0xF, er3
			// 7A 6x dd dd dd dd
			if ((instr[1] & 0xF8) == 0x60) return INSTR_AND_I32_R32;
		}
		break;
		// 7B
		case 0x7B :
		{
			// read
			FETCH_BYTE(2);
			FETCH_BYTE(3);
			// 7B D4 59 8F : eepmov.w
			// 7B D4 59 8F
			if ((instr[1] == 0xD4) && (instr[2] == 0x59) && (instr[3] == 0x8F)) return INSTR_EEPMOV_W;
		}
		break;
		// 7C
		case 0x7C :
		{
			// read
			FETCH_BYTE(2);
			FETCH_BYTE(3);
			// 7C 20 73 70 : btst #7, @er2
			// 7C x0 73 y0
			if (((instr[1] & 0x8F) == 0x00) && (instr[2] == 0x73) && ((instr[3] & 0x8F) == 0x00)) return INSTR_BTST_Ix_AR32;
			// 7C 20 76 40 : band #4, @er2
			// 7C x0 76 y0
			if (((instr[1] & 0x8F) == 0x00) && (instr[2] == 0x76) && ((instr[3] & 0x8F) == 0x00)) return INSTR_BAND_Ix_AR32;
			// 7C 40 76 F0 : biand #7, @er4
			// 7C x0 76 y0
			if (((instr[1] & 0x8F) == 0x00) && (instr[2] == 0x76) && ((instr[3] & 0x8F) == 0x80)) return INSTR_BIAND_Ix_AR32;
			// 7C 40 77 F0 : bild #7, @er4
			// 7C x0 77 y0
			if (((instr[1] & 0x8F) == 0x00) && (instr[2] == 0x77) && ((instr[3] & 0x8F) == 0x80)) return INSTR_BILD_Ix_AR32;
		}
		break;
		// 7D
		case 0x7D :
		{
			// read
			FETCH_BYTE(2);
			FETCH_BYTE(3);
			// 7D 30 62 40 : bclr r4h, @er3
			// 7D x0 62 y0
			if (((instr[1] & 0x8F) == 0x00) && (instr[2] == 0x62) && ((instr[3] & 0x8F) == 0x00)) return INSTR_BCLR_R8_AR32;
			// 7D 50 72 20 : bclr #2, @er5
			// 7D x0 72 y0
			if (((instr[1] & 0x8F) == 0x00) && (instr[2] == 0x72) && ((instr[3] & 0x8F) == 0x00)) return INSTR_BCLR_Ix_AR32;
			// 7D 60 70 30 : bset #3, @er6
			// 7D x0 70 y0
			if (((instr[1] & 0x8F) == 0x00) && (instr[2] == 0x70) && ((instr[3] & 0x8F) == 0x00)) return INSTR_BSET_Ix_AR32;
		}
		break;
		// 7E
		case 0x7E :
		{
			// read
			FETCH_BYTE(2);
			FETCH_BYTE(3);
			// 7E 5E 73 00 : btst #0, @0xFFFFFF5E:8
			// 7E aa 73 x0
			if ((instr[2] == 0x73) && ((instr[3] & 0x8F) == 0x00)) return INSTR_BTST_Ix_AI8;
			// 7E C0 76 50 : band #5, @P1:8
			// 7E xx 76 y0
			if ((instr[2] == 0x76) && ((instr[3] & 0x8F) == 0x00)) return INSTR_BAND_Ix_AI8;
			// 7E C0 76 80 : biand #0, @P1:8
			// 7E xx 76 y0
			if ((instr[2] == 0x76) && ((instr[3] & 0x8F) == 0x80)) return INSTR_BIAND_Ix_AI8;
		}
		break;
		// 7F
		case 0x7F :
		{
			// read
			FETCH_BYTE(2);
			FETCH_BYTE(3);
			// 7F C0 62 50 : bclr r5h, @0xFFFFC0:8
			// 7F aa 62 x0
			if ((instr[2] == 0x62) && ((instr[3] & 0x0F) == 0x00)) return INSTR_BCLR_R8_AI8;
			// 7F 6E 70 10 : bset #1, @0xFFFF6E:8
			// 7F aa 70 y0
			if ((instr[2] == 0x70) && ((instr[3] & 0x8F) == 0x00)) return INSTR_BSET_Ix_AI8;
			// 7F 6F 71 00 : bnot #0, @0xFFFF6F:8
			// 7F aa 71 y0
			if ((instr[2] == 0x71) && ((instr[3] & 0x8F) == 0x00)) return INSTR_BNOT_Ix_AI8;
			// 7F 8C 72 70 : bclr #7, @0xFFFF8C:8
			// 7F aa 72 x0
			if ((instr[2] == 0x72) && ((instr[3] & 0x8F) == 0x00)) return INSTR_BCLR_Ix_AI8;
		}
		break;
		// 80..8F | 89 50 : add.b #0x50, r1l | 8A 02 : add.b #2, r2l | 8x dd
		case 0x80 : case 0x81 : case 0x82 : case 0x83 :
		case 0x84 : case 0x85 :	case 0x86 :	case 0x87 :
		case 0x88 :	case 0x89 :	case 0x8A :	case 0x8B :
		case 0x8C :	case 0x8D :	case 0x8E :	case 0x8F :
		{
			return INSTR_ADD_I8_R8;
		}
		break;
		// 90..9F | 9B 02 : addx #2, r3l | 9x dd
		case 0x90 : case 0x91 : case 0x92 : case 0x93 :
		case 0x94 : case 0x95 :	case 0x96 :	case 0x97 :
		case 0x98 :	case 0x99 :	case 0x9A :	case 0x9B :
		case 0x9C :	case 0x9D :	case 0x9E :	case 0x9F :
		{
			return INSTR_ADDX_I8_R8;
		}
		break;
		// A0..AF | AA 20 : cmp.b #0x20, r2l | Ax dd
		case 0xA0 : case 0xA1 : case 0xA2 : case 0xA3 :
		case 0xA4 : case 0xA5 :	case 0xA6 :	case 0xA7 :
		case 0xA8 :	case 0xA9 :	case 0xAA :	case 0xAB :
		case 0xAC :	case 0xAD :	case 0xAE :	case 0xAF :
		{
			return INSTR_CMP_I8_R8;
		}
		break;
		// C0..CF | CA 04 : or.b #4, r2l | Cx dd
		case 0xC0 : case 0xC1 : case 0xC2 : case 0xC3 :
		case 0xC4 : case 0xC5 :	case 0xC6 :	case 0xC7 :
		case 0xC8 :	case 0xC9 :	case 0xCA :	case 0xCB :
		case 0xCC :	case 0xCD :	case 0xCE :	case 0xCF :
		{
			return INSTR_OR_I8_R8;
		}
		break;
		// D0..DF | D8 01 : xor.b #1, r0l | Dx dd
		case 0xD0 : case 0xD1 : case 0xD2 : case 0xD3 :
		case 0xD4 : case 0xD5 :	case 0xD6 :	case 0xD7 :
		case 0xD8 :	case 0xD9 :	case 0xDA :	case 0xDB :
		case 0xDC :	case 0xDD :	case 0xDE :	case 0xDF :
		{
			return INSTR_XOR_I8_R8;
		}
		break;
		// E0..EF | EA 7F : and.b #0x7F, r2l | Ex dd
		case 0xE0 : case 0xE1 : case 0xE2 : case 0xE3 :
		case 0xE4 : case 0xE5 :	case 0xE6 :	case 0xE7 :
		case 0xE8 :	case 0xE9 :	case 0xEA :	case 0xEB :
		case 0xEC :	case 0xED :	case 0xEE :	case 0xEF :
		{
			return INSTR_AND_I8_R8;
		}
		break;
		// F0..FF | FA 7F : mov.b #0x7F, r2l | Fx dd
		case 0xF0 : case 0xF1 : case 0xF2 : case 0xF3 :
		case 0xF4 : case 0xF5 :	case 0xF6 :	case 0xF7 :
		case 0xF8 :	case 0xF9 :	case 0xFA :	case 0xFB :
		case 0xFC :	case 0xFD :	case 0xFE :	case 0xFF :
		{
			return INSTR_MOV_I8_R8;
		}
		break;
		// other
		default :
		{
			return INSTR_UNKNOWN;
		}
		break;
	}
	// other
	return INSTR_UNKNOWN;
}

//------------------------------------------------------------------------------
void h8s_opcode_set_info( int instr, int size, const char *text)
{
	h8s_instr_table[instr].size = size;
	h8s_instr_table[instr].text = text;
}

//------------------------------------------------------------------------------
void h8s_opcode_init( void)
{
	UINT32 size;
	size = sizeof( H8S_INSTR_TABLE_ENTRY) * INSTR_MAX;
	h8s_instr_table = malloc( size);
	memset( h8s_instr_table, 0, size);

	h8s_opcode_set_info( INSTR_UNKNOWN          ,  0 , "unknown"                   );

	h8s_opcode_set_info( INSTR_NOP              ,  2 , "nop"                       );

	h8s_opcode_set_info( INSTR_BCC_O8           ,  2 , "bcc 0x%06X:8"              );
	h8s_opcode_set_info( INSTR_BCC_O16          ,  4 , "bcc 0x%06X:16"             );
	h8s_opcode_set_info( INSTR_BCS_O8           ,  2 , "bcs 0x%06X:8"              );
	h8s_opcode_set_info( INSTR_BCS_O16          ,  4 , "bcs 0x%06X:16"             );
	h8s_opcode_set_info( INSTR_BEQ_O8           ,  2 , "beq 0x%06X:8"              );
	h8s_opcode_set_info( INSTR_BEQ_O16          ,  4 , "beq 0x%06X:16"             );
	h8s_opcode_set_info( INSTR_BGE_O8           ,  2 , "bge 0x%06X:8"              );
	h8s_opcode_set_info( INSTR_BGE_O16          ,  4 , "bge 0x%06X:16"             );
	h8s_opcode_set_info( INSTR_BGT_O8           ,  2 , "bgt 0x%06X:8"              );
	h8s_opcode_set_info( INSTR_BGT_O16          ,  4 , "bgt 0x%06X:16"             );
	h8s_opcode_set_info( INSTR_BHI_O8           ,  2 , "bhi 0x%06X:8"              );
	h8s_opcode_set_info( INSTR_BHI_O16          ,  4 , "bhi 0x%06X:16"             );
	h8s_opcode_set_info( INSTR_BLE_O8           ,  2 , "ble 0x%06X:8"              );
	h8s_opcode_set_info( INSTR_BLE_O16          ,  4 , "ble 0x%06X:16"             );
	h8s_opcode_set_info( INSTR_BLS_O8           ,  2 , "bls 0x%06X:8"              );
	h8s_opcode_set_info( INSTR_BLS_O16          ,  4 , "bls 0x%06X:16"             );
	h8s_opcode_set_info( INSTR_BLT_O8           ,  2 , "blt 0x%06X:8"              );
	h8s_opcode_set_info( INSTR_BLT_O16          ,  4 , "blt 0x%06X:16"             );
	h8s_opcode_set_info( INSTR_BMI_O8           ,  2 , "bmi 0x%06X:8"              );
	h8s_opcode_set_info( INSTR_BMI_O16          ,  4 , "bmi 0x%06X:16"             );
	h8s_opcode_set_info( INSTR_BNE_O8           ,  2 , "bne 0x%06X:8"              );
	h8s_opcode_set_info( INSTR_BNE_O16          ,  4 , "bne 0x%06X:16"             );
	h8s_opcode_set_info( INSTR_BPL_O8           ,  2 , "bpl 0x%06X:8"              );
	h8s_opcode_set_info( INSTR_BPL_O16          ,  4 , "bpl 0x%06X:16"             );
	h8s_opcode_set_info( INSTR_BRA_O8           ,  2 , "bra 0x%06X:8"              );
	h8s_opcode_set_info( INSTR_BRA_O16          ,  4 , "bra 0x%06X:16"             );
	h8s_opcode_set_info( INSTR_BRN_O8           ,  2 , "brn 0x%06X:8"              );
	h8s_opcode_set_info( INSTR_BRN_O16          ,  4 , "brn 0x%06X:16"             );
	h8s_opcode_set_info( INSTR_BVC_O8           ,  2 , "bvc 0x%06X:8"              );
	h8s_opcode_set_info( INSTR_BVC_O16          ,  4 , "bvc 0x%06X:16"             );
	h8s_opcode_set_info( INSTR_BVS_O8           ,  2 , "bvs 0x%06X:8"              );
	h8s_opcode_set_info( INSTR_BVS_O16          ,  4 , "bvs 0x%06X:16"             );

	h8s_opcode_set_info( INSTR_MOV_AI8_R8       ,  2 , "mov.b @0x%06X:8, %s"       );
	h8s_opcode_set_info( INSTR_MOV_AI16R32_R8   ,  4 , "mov.b @(0x%06X:16,%s), %s" );
	h8s_opcode_set_info( INSTR_MOV_AI16R32_R16  ,  4 , "mov.w @(0x%06X:16,%s), %s" );
	h8s_opcode_set_info( INSTR_MOV_AI16R32_R32  ,  6 , "mov.l @(0x%06X:16,%s),%s"  );
	h8s_opcode_set_info( INSTR_MOV_AI16_R8      ,  4 , "mov.b @0x%06X:16, %s"      );
	h8s_opcode_set_info( INSTR_MOV_AI16_R16     ,  4 , "mov.w @0x%06X:16, %s"      );
	h8s_opcode_set_info( INSTR_MOV_AI16_R32     ,  6 , "mov.l @0x%06X:16, %s"      );
	h8s_opcode_set_info( INSTR_MOV_AI32R32_R8   ,  8 , "mov.b @(0x%06X:32,%s), %s" );
	h8s_opcode_set_info( INSTR_MOV_AI32R32_R16  ,  8 , "mov.w @(0x%06X:32,%s), %s" );
	h8s_opcode_set_info( INSTR_MOV_AI32R32_R32  , 10 , "mov.l @(0x%06X:32,%s), %s" );
	h8s_opcode_set_info( INSTR_MOV_AI32_R8      ,  6 , "mov.b @0x%06X:32, %s"      );
	h8s_opcode_set_info( INSTR_MOV_AI32_R16     ,  6 , "mov.w @0x%06X:32, %s"      );
	h8s_opcode_set_info( INSTR_MOV_AI32_R32     ,  8 , "mov.l @0x%06X:32, %s"      );
	h8s_opcode_set_info( INSTR_MOV_AR32_R8      ,  2 , "mov.b @%s, %s"             );
	h8s_opcode_set_info( INSTR_MOV_AR32_R16     ,  2 , "mov.w @%s, %s"             );
	h8s_opcode_set_info( INSTR_MOV_AR32_R32     ,  4 , "mov.l @%s, %s"             );
	h8s_opcode_set_info( INSTR_MOV_AR32I_R8     ,  2 , "mov.b @%s+, %s"            );
	h8s_opcode_set_info( INSTR_MOV_AR32I_R16    ,  2 , "mov.w @%s+, %s"            );
	h8s_opcode_set_info( INSTR_MOV_AR32I_R32    ,  4 , "mov.l @%s+, %s"            );
	h8s_opcode_set_info( INSTR_MOV_I8_R8        ,  2 , "mov.b #0x%02X, %s"         );
	h8s_opcode_set_info( INSTR_MOV_I16_R16      ,  4 , "mov.w #0x%04X, %s"         );
	h8s_opcode_set_info( INSTR_MOV_I32_R32      ,  6 , "mov.l #0x%08X, %s"         );
	h8s_opcode_set_info( INSTR_MOV_R8_AI8       ,  2 , "mov.b %s, @0x%06X:8"       );
	h8s_opcode_set_info( INSTR_MOV_R8_AI16      ,  4 , "mov.b %s, @0x%06X:16"      );
	h8s_opcode_set_info( INSTR_MOV_R8_AI16R32   ,  4 , "mov.b %s, @(0x%06X:16,%s)" );
	h8s_opcode_set_info( INSTR_MOV_R8_AI32      ,  6 , "mov.b %s, @0x%06X:32"      );
	h8s_opcode_set_info( INSTR_MOV_R8_AI32R32   ,  8 , "mov.b %s, @(0x%06X:32,%s)" );
	h8s_opcode_set_info( INSTR_MOV_R8_AR32      ,  2 , "mov.b %s, @%s"             );
	h8s_opcode_set_info( INSTR_MOV_R8_AR32D     ,  2 , "mov.b %s, @-%s"            );
	h8s_opcode_set_info( INSTR_MOV_R8_R8        ,  2 , "mov.b %s, %s"              );
	h8s_opcode_set_info( INSTR_MOV_R16_AI16     ,  4 , "mov.w %s, @0x%06X:16"      );
	h8s_opcode_set_info( INSTR_MOV_R16_AI16R32  ,  4 , "mov.w %s, @(0x%06X:16,%s)" );
	h8s_opcode_set_info( INSTR_MOV_R16_AI32     ,  6 , "mov.w %s, @0x%06X:32"      );
	h8s_opcode_set_info( INSTR_MOV_R16_AI32R32  ,  8 , "mov.w %s, @(0x%06X:32,%s)" );
	h8s_opcode_set_info( INSTR_MOV_R16_AR32     ,  2 , "mov.w %s, @%s"             );
	h8s_opcode_set_info( INSTR_MOV_R16_AR32D    ,  2 , "mov.w %s, @-%s"            );
	h8s_opcode_set_info( INSTR_MOV_R16_R16      ,  2 , "mov.w %s, %s"              );
	h8s_opcode_set_info( INSTR_MOV_R32_AI16     ,  6 , "mov.l %s, @0x%06X:16"      );
	h8s_opcode_set_info( INSTR_MOV_R32_AI16R32  ,  6 , "mov.l %s, @(0x%06X:16,%s)" );
	h8s_opcode_set_info( INSTR_MOV_R32_AI32     ,  8 , "mov.l %s, @0x%06X:32"      );
	h8s_opcode_set_info( INSTR_MOV_R32_AI32R32  , 10 , "mov.l %s, @(0x%06X:32,%s)" );
	h8s_opcode_set_info( INSTR_MOV_R32_AR32     ,  4 , "mov.l %s, @%s"             );
	h8s_opcode_set_info( INSTR_MOV_R32_AR32D    ,  4 , "mov.l %s, @-%s"            );
	h8s_opcode_set_info( INSTR_MOV_R32_R32      ,  2 , "mov.l %s, %s"              );

	#ifdef ALIAS_PUSH
	h8s_opcode_set_info( INSTR_PUSH_R16         ,  2 , "push.w %s"                 );
	h8s_opcode_set_info( INSTR_PUSH_R32         ,  4 , "push.l %s"                 );
	#endif

	#ifdef ALIAS_POP
	h8s_opcode_set_info( INSTR_POP_R16          ,  2 , "pop.w %s"                  );
	h8s_opcode_set_info( INSTR_POP_R32          ,  4 , "pop.l %s"                  );
	#endif

	h8s_opcode_set_info( INSTR_ADD_I8_R8        ,  2 , "add.b #0x%02X, %s"         );
	h8s_opcode_set_info( INSTR_ADD_I16_R16      ,  4 , "add.w #0x%04X, %s"         );
	h8s_opcode_set_info( INSTR_ADD_I32_R32      ,  6 , "add.l #0x%08X, %s"         );
	h8s_opcode_set_info( INSTR_ADD_R8_R8        ,  2 , "add.b %s, %s"              );
	h8s_opcode_set_info( INSTR_ADD_R16_R16      ,  2 , "add.w %s, %s"              );
	h8s_opcode_set_info( INSTR_ADD_R32_R32      ,  2 , "add.l %s, %s"              );

	h8s_opcode_set_info( INSTR_ADDS_XX_R32      ,  2 , "adds #%d, %s"              );
	h8s_opcode_set_info( INSTR_ADDS_XX_R32      ,  2 , "adds #%d, %s"              );
	h8s_opcode_set_info( INSTR_ADDS_XX_R32      ,  2 , "adds #%d, %s"              );

	h8s_opcode_set_info( INSTR_ADDX_I8_R8       ,  2 , "addx #0x%02X, %s"          );
	h8s_opcode_set_info( INSTR_ADDX_R8_R8       ,  2 , "addx %s, %s"               );

	h8s_opcode_set_info( INSTR_AND_I8_R8        ,  2 , "and.b #0x%02X, %s"         );
	h8s_opcode_set_info( INSTR_AND_I16_R16      ,  4 , "and.w #0x%04X, %s"         );
	h8s_opcode_set_info( INSTR_AND_I32_R32      ,  6 , "and.l #0x%08X, %s"         );
	h8s_opcode_set_info( INSTR_AND_R8_R8        ,  2 , "and.b %s, %s"              );
	h8s_opcode_set_info( INSTR_AND_R16_R16      ,  2 , "and.w %s, %s"              );
	h8s_opcode_set_info( INSTR_AND_R32_R32      ,  4 , "and.l %s, %s"              );

	h8s_opcode_set_info( INSTR_ANDC_I8_CCR      ,  2 , "andc #0x%02X, ccr"         );

	h8s_opcode_set_info( INSTR_BAND_Ix_AI8      ,  4 , "band #%d, @0x%06X:8"       );
	h8s_opcode_set_info( INSTR_BAND_Ix_AR32     ,  4 , "band #%d, @%s"             );
	h8s_opcode_set_info( INSTR_BAND_Ix_R8       ,  2 , "band #%d, %s"              );

	h8s_opcode_set_info( INSTR_BCLR_Ix_AI8      ,  4 , "bclr #%d, @0x%06X:8"       );
	h8s_opcode_set_info( INSTR_BCLR_Ix_AI32     ,  8 , "bclr #%d, @0x%06X:32"      );
	h8s_opcode_set_info( INSTR_BCLR_Ix_AR32     ,  4 , "bclr #%d, @%s"             );
	h8s_opcode_set_info( INSTR_BCLR_Ix_R8       ,  2 , "bclr #%d, %s"              );
	h8s_opcode_set_info( INSTR_BCLR_R8_AI8      ,  4 , "bclr %s, @0x%06X:8"        );
	h8s_opcode_set_info( INSTR_BCLR_R8_AI32     ,  8 , "bclr %s, @0x%06X:32"       );
	h8s_opcode_set_info( INSTR_BCLR_R8_AR32     ,  4 , "bclr %s, @%s"              );
	h8s_opcode_set_info( INSTR_BCLR_R8_R8       ,  2 , "bclr %s, %s"               );

	h8s_opcode_set_info( INSTR_BIAND_Ix_AI8     ,  4 , "biand #%d, @0x%06X:8"      );
	h8s_opcode_set_info( INSTR_BIAND_Ix_AR32    ,  4 , "biand #%d, @%s"            );
	h8s_opcode_set_info( INSTR_BIAND_Ix_R8      ,  2 , "biand #%d, %s"             );

	h8s_opcode_set_info( INSTR_BILD_Ix_AR32     ,  4 , "bild #%d, @%s"             );
	h8s_opcode_set_info( INSTR_BILD_Ix_R8       ,  2 , "bild #%d, %s"              );

	h8s_opcode_set_info( INSTR_BLD_Ix_R8        ,  2 , "bld #%d, %s"               );

	h8s_opcode_set_info( INSTR_BNOT_Ix_AI8      ,  4 , "bnot #%d, @0x%06X:8"       );
	h8s_opcode_set_info( INSTR_BNOT_Ix_AI32     ,  8 , "bnot #%d, @0x%06X:32"      );

	h8s_opcode_set_info( INSTR_BSET_Ix_AI8      ,  4 , "bset #%d, @0x%06X:8"       );
	h8s_opcode_set_info( INSTR_BSET_Ix_AI32     ,  8 , "bset #%d, @0x%06X:32"      );
	h8s_opcode_set_info( INSTR_BSET_Ix_AR32     ,  4 , "bset #%d, @%s"             );
	h8s_opcode_set_info( INSTR_BSET_R8_AI32     ,  8 , "bset %s, @0x%06X:32"       );

	h8s_opcode_set_info( INSTR_BSR_O8           ,  2 , "bsr 0x%06X:8"              );
	h8s_opcode_set_info( INSTR_BSR_O16          ,  4 , "bsr 0x%06X:16"             );

	h8s_opcode_set_info( INSTR_BST_Ix_R8        ,  2 , "bst #%d, %s"               );

	h8s_opcode_set_info( INSTR_BTST_Ix_AI8      ,  4 , "btst #%d, @0x%06X:8"       );
	h8s_opcode_set_info( INSTR_BTST_Ix_AI32     ,  8 , "btst #%d, @0x%06X:32"      );
	h8s_opcode_set_info( INSTR_BTST_Ix_AR32     ,  4 , "btst #%d, @%s"             );
	h8s_opcode_set_info( INSTR_BTST_Ix_R8       ,  2 , "btst #%d, %s"              );

	h8s_opcode_set_info( INSTR_CMP_I8_R8        ,  2 , "cmp.b #0x%02X, %s"         );
	h8s_opcode_set_info( INSTR_CMP_I16_R16      ,  4 , "cmp.w #0x%04X, %s"         );
	h8s_opcode_set_info( INSTR_CMP_I32_R32      ,  6 , "cmp.l #0x%08X, %s"         );
	h8s_opcode_set_info( INSTR_CMP_R8_R8        ,  2 , "cmp.b %s, %s"              );
	h8s_opcode_set_info( INSTR_CMP_R16_R16      ,  2 , "cmp.w %s, %s"              );
	h8s_opcode_set_info( INSTR_CMP_R32_R32      ,  2 , "cmp.l %s, %s"              );

	h8s_opcode_set_info( INSTR_DEC_XX_R8        ,  2 , "dec.b %s"                  );
	h8s_opcode_set_info( INSTR_DEC_XX_R16       ,  2 , "dec.w #%d, %s"             );
	h8s_opcode_set_info( INSTR_DEC_XX_R32       ,  2 , "dec.l #%d, %s"             );

	h8s_opcode_set_info( INSTR_DIVXS_R16_R32    ,  4 , "divxs.w %s, %s"            );

	h8s_opcode_set_info( INSTR_DIVXU_R8_R16     ,  2 , "divxu.b %s, %s"            );
	h8s_opcode_set_info( INSTR_DIVXU_R16_R32    ,  2 , "divxu.w %s, %s"            );

	h8s_opcode_set_info( INSTR_EEPMOV_W         ,  4 , "eepmov.w"                  );

	h8s_opcode_set_info( INSTR_EXTS_R16         ,  2 , "exts.w %s"                 );
	h8s_opcode_set_info( INSTR_EXTS_R32         ,  2 , "exts.l %s"                 );

	h8s_opcode_set_info( INSTR_EXTU_R16         ,  2 , "extu.w %s"                 );
	h8s_opcode_set_info( INSTR_EXTU_R32         ,  2 , "extu.l %s"                 );

	h8s_opcode_set_info( INSTR_INC_XX_R8        ,  2 , "inc.b #1, %s"              );
	h8s_opcode_set_info( INSTR_INC_XX_R16       ,  2 , "inc.w #%d, %s"             );
	h8s_opcode_set_info( INSTR_INC_XX_R32       ,  2 , "inc.l #%d, %s"             );
	// JMP
	h8s_opcode_set_info( INSTR_JMP_AR32         ,  2 , "jmp @%s"                   );
	h8s_opcode_set_info( INSTR_JMP_I24          ,  4 , "jmp 0x%06X:24"             );
	// JSR
	h8s_opcode_set_info( INSTR_JSR_AR32         ,  2 , "jsr @%s"                   );
	h8s_opcode_set_info( INSTR_JSR_I24          ,  4 , "jsr 0x%06X:24"             );
	// LDC
	h8s_opcode_set_info( INSTR_LDC_AR32I_CCR    ,  4 , "ldc.w @%s+, ccr"           );
	h8s_opcode_set_info( INSTR_LDC_R8_CCR       ,  2 , "ldc.b %s, ccr"             );
	// LDM
	h8s_opcode_set_info( INSTR_LDM_AR32I_R32R32 ,  4 , "ldm.l @%s+, %s-%s"         );
	// MULXS
	h8s_opcode_set_info( INSTR_MULXS_R8_R16     ,  4 , "mulxs.b %s, %s"            );
	h8s_opcode_set_info( INSTR_MULXS_R16_R32    ,  4 , "mulxs.w %s, %s"            );
	// MULXU
	h8s_opcode_set_info( INSTR_MULXU_R8_R16     ,  2 , "mulxu.b %s, %s"            );
	h8s_opcode_set_info( INSTR_MULXU_R16_R32    ,  2 , "mulxu.w %s, %s"            );
	// NEG
	h8s_opcode_set_info( INSTR_NEG_R8           ,  2 , "neg.b %s"                  );
	h8s_opcode_set_info( INSTR_NEG_R16          ,  2 , "neg.w %s"                  );
	h8s_opcode_set_info( INSTR_NEG_R32          ,  2 , "neg.l %s"                  );
	// NOT
	h8s_opcode_set_info( INSTR_NOT_R8           ,  2 , "not.b %s"                  );
	h8s_opcode_set_info( INSTR_NOT_R16          ,  2 , "not.w %s"                  );
	h8s_opcode_set_info( INSTR_NOT_R32          ,  2 , "not.l %s"                  );
	// OR
	h8s_opcode_set_info( INSTR_OR_I8_R8         ,  2 , "or.b #0x%02X, %s"          );
	h8s_opcode_set_info( INSTR_OR_I32_R32       ,  6 , "or.l #0x%08X, %s"          );
	h8s_opcode_set_info( INSTR_OR_R8_R8         ,  2 , "or.b %s, %s"               );
	h8s_opcode_set_info( INSTR_OR_R16_R16       ,  2 , "or.w %s, %s"               );
	h8s_opcode_set_info( INSTR_OR_R32_R32       ,  4 , "or.l %s, %s"               );
	// ORC
	h8s_opcode_set_info( INSTR_ORC_I8_CCR       ,  2 , "orc #0x%02X, ccr"          );
	// ROTL
	h8s_opcode_set_info( INSTR_ROTL_XX_R8       ,  2 , "rotl.b #%d, %s"            );
	h8s_opcode_set_info( INSTR_ROTL_XX_R16      ,  2 , "rotl.w #%d, %s"            );
	h8s_opcode_set_info( INSTR_ROTL_XX_R32      ,  2 , "rotl.l #%d, %s"            );
	// ROTR
	h8s_opcode_set_info( INSTR_ROTR_XX_R8       ,  2 , "rotr.b #%d, %s"            );
	h8s_opcode_set_info( INSTR_ROTR_XX_R32      ,  2 , "rotr.l #%d, %s"            );
	// ROTXL
	h8s_opcode_set_info( INSTR_ROTXL_XX_R8      ,  2 , "rotxl.b #%d, %s"           );
	h8s_opcode_set_info( INSTR_ROTXL_XX_R32     ,  2 , "rotxl.l #%d, %s"           );
	// RTE
	h8s_opcode_set_info( INSTR_RTE              ,  2 , "rte"                       );
	// RTS
	h8s_opcode_set_info( INSTR_RTS              ,  2 , "rts"                       );
	// SHAL
	h8s_opcode_set_info( INSTR_SHAL_XX_R8       ,  2 , "shal.b #%d, %s"            );
	h8s_opcode_set_info( INSTR_SHAL_XX_R16      ,  2 , "shal.w #%d, %s"            );
	h8s_opcode_set_info( INSTR_SHAL_XX_R32      ,  2 , "shal.l #%d, %s"            );
	// SHAR
	h8s_opcode_set_info( INSTR_SHAR_XX_R16      ,  2 , "shar.w #%d, %s"            );
	h8s_opcode_set_info( INSTR_SHAR_XX_R32      ,  2 , "shar.l #%d, %s"            );
	// SHLL
	h8s_opcode_set_info( INSTR_SHLL_XX_R8       ,  2 , "shll.b #%d, %s"            );
	h8s_opcode_set_info( INSTR_SHLL_XX_R16      ,  2 , "shll.w #%d, %s"            );
	h8s_opcode_set_info( INSTR_SHLL_XX_R32      ,  2 , "shll.l #%d, %s"            );
	// SHLR
	h8s_opcode_set_info( INSTR_SHLR_XX_R8       ,  2 , "shlr.b #%d, %s"            );
	h8s_opcode_set_info( INSTR_SHLR_XX_R16      ,  2 , "shlr.w #%d, %s"            );
	h8s_opcode_set_info( INSTR_SHLR_XX_R32      ,  2 , "shlr.l #%d, %s"            );
	// SLEEP
	h8s_opcode_set_info( INSTR_SLEEP            ,  2 , "sleep"                     );
	// STC
	h8s_opcode_set_info( INSTR_STC_CCR_AR32D    ,  4 , "stc.w ccr, @-%s"           );
	h8s_opcode_set_info( INSTR_STC_CCR_R8       ,  2 , "stc.b ccr, %s"             );
	// STM
	h8s_opcode_set_info( INSTR_STM_R32R32_AR32D ,  4 , "stm.l %s-%s, @-%s"         );
	// SUB
	h8s_opcode_set_info( INSTR_SUB_I16_R16      ,  4 , "sub.w #0x%04X, %s"         );
	h8s_opcode_set_info( INSTR_SUB_I32_R32      ,  6 , "sub.l #0x%08X, %s"         );
	h8s_opcode_set_info( INSTR_SUB_R8_R8        ,  2 , "sub.b %s, %s"              );
	h8s_opcode_set_info( INSTR_SUB_R16_R16      ,  2 , "sub.w %s, %s"              );
	h8s_opcode_set_info( INSTR_SUB_R32_R32      ,  2 , "sub.l %s, %s"              );
	// SUBS
	h8s_opcode_set_info( INSTR_SUBS_XX_R32      ,  2 , "subs #%d, %s"              );
	// SUBX
	h8s_opcode_set_info( INSTR_SUBX_R8_R8       ,  2 , "subx %s, %s"               );
	// XOR
	h8s_opcode_set_info( INSTR_XOR_I8_R8        ,  2 , "xor.b #0x%02X, %s"         );
	h8s_opcode_set_info( INSTR_XOR_I16_R16      ,  4 , "xor.w #0x%04X, %s"         );
	h8s_opcode_set_info( INSTR_XOR_I32_R32      ,  6 , "or.l #0x%08X, %s"          );
	h8s_opcode_set_info( INSTR_XOR_R8_R8        ,  2 , "xor.b %s, %s"              );
	h8s_opcode_set_info( INSTR_XOR_R16_R16      ,  2 , "xor.w %s, %s"              );
	h8s_opcode_set_info( INSTR_XOR_R32_R32      ,  4 , "xor.l %s, %s"              );

}

//------------------------------------------------------------------------------
void h8s_opcode_exit( void)
{
	free( h8s_instr_table);
}
