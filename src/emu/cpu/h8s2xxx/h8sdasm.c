/*

	Hitachi H8S/22xx MCU

	(c) 2001-2007 Tim Schuerewegen

	H8S/2241
	H8S/2246
	H8S/2323

*/

#include "h8sdasm.h"

#include "h8sopcd.h"

static const char* REG_NAME_32[] =
{
	"er0", "er1", "er2", "er3", "er4", "er5", "er6", "sp",
	"???", "???", "???", "???", "???", "???", "???", "???"
};

static const char* REG_NAME_16[] =
{
	"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
	"e0", "e1", "e2", "e3", "e4", "e5", "e6", "e7"
};

static const char* REG_NAME_8[] =
{
	"r0h", "r1h", "r2h", "r3h", "r4h", "r5h", "r6h", "r7h",
	"r0l", "r1l", "r2l", "r3l", "r4l", "r5l", "r6l", "r7l"
};

#define RN8(x)   REG_NAME_8[x]
#define RN16(x)  REG_NAME_16[x]
#define RN32(x)  REG_NAME_32[x]

offs_t h8s2xxx_disassemble( char *buffer, offs_t pc, const UINT8 *oprom, const UINT8 *opram)
{
	const char* instr_text;
	int index;
	UINT32 instr_size, instr_flag = 0;
	UINT8 instr[10];

	// resolve instruction
	if (oprom) memcpy( instr, oprom, sizeof( instr));
	index = h8s_opcode_find( pc, instr, oprom ? sizeof( instr) : 0);

	instr_text = h8s_instr_table[index].text;
	instr_size = h8s_instr_table[index].size;

	// disasm
	switch (index)
	{

		case INSTR_MOV_AI16R32_R8 :
		{
			H8S_GET_DATA_AI16R32_R8
			sprintf( buffer, instr_text, imm, RN32( reg_src), RN8( reg_dst));
		}
		break;

		case INSTR_MOV_R8_AI16R32 :
		{
			H8S_GET_DATA_R8_AI16R32
			sprintf( buffer, instr_text, RN8( reg_src), imm, RN32( reg_dst));
		}
		break;

		case INSTR_MOV_AI16R32_R16 :
		{
			H8S_GET_DATA_AI16R32_R16
			sprintf( buffer, instr_text, imm, RN32( reg_src), RN16( reg_dst));
		}
		break;

		case INSTR_MOV_R16_AI16R32 :
		{
			H8S_GET_DATA_R16_AI16R32
			sprintf( buffer, instr_text, RN16( reg_src), imm, RN32( reg_dst));
		}
		break;

		case INSTR_MOV_R32_AI16R32 :
		{
			H8S_GET_DATA_R32_AI16R32
			sprintf( buffer, instr_text, RN32( reg_src), imm, RN32( reg_dst));
		}
		break;

		case INSTR_MOV_AI16R32_R32 :
		{
			H8S_GET_DATA_AI16R32_R32
			sprintf( buffer, instr_text, imm, RN32( reg_src), RN32( reg_dst));
		}
		break;

		case INSTR_MOV_R8_AI32R32 :
		{
			H8S_GET_DATA_R8_AI32R32
			sprintf( buffer, instr_text, RN8( reg_src), imm, RN32( reg_dst));
		}
		break;

		case INSTR_MOV_AI32R32_R8 :
		{
			H8S_GET_DATA_AI32R32_R8
			sprintf( buffer, instr_text, imm, RN32( reg_src), RN8( reg_dst));
		}
		break;

		case INSTR_MOV_AI32R32_R16 :
		{
			H8S_GET_DATA_AI32R32_R16
			sprintf( buffer, instr_text, imm, RN32( reg_src), RN16( reg_dst));
		}
		break;

		case INSTR_MOV_R16_AI32R32 :
		{
			H8S_GET_DATA_R16_AI32R32
			sprintf( buffer, instr_text, RN16( reg_src), imm, RN32( reg_dst));
		}
		break;

		case INSTR_MOV_AI32_R32 :
		{
			H8S_GET_DATA_AI32_R32
			sprintf( buffer, instr_text, imm, RN32( reg_dst));
		}
		break;

		case INSTR_MOV_AI32R32_R32 :
		{
			H8S_GET_DATA_AI32R32_R32
			sprintf( buffer, instr_text, imm, RN32( reg_src), RN32( reg_dst));
		}
		break;

		case INSTR_MOV_R32_AI32R32 :
		{
			H8S_GET_DATA_R32_AI32R32
			sprintf( buffer, instr_text, RN32( reg_src), imm, RN32( reg_dst));
		}
		break;

		case INSTR_MOV_I32_R32 :
		case INSTR_ADD_I32_R32 :
		case INSTR_CMP_I32_R32 :
		case INSTR_SUB_I32_R32 :
		case INSTR_AND_I32_R32 :
		case INSTR_OR_I32_R32  :
		case INSTR_XOR_I32_R32 :
		{
			H8S_GET_DATA_I32_R32
			sprintf( buffer, instr_text, imm, RN32( reg_dst));
		}
		break;

		case INSTR_MOV_AR32_R16  :
		case INSTR_MOV_AR32I_R16 :
		{
			H8S_GET_DATA_AR32_R16
			sprintf( buffer, instr_text, RN32( reg_src), RN16( reg_dst));
		}
		break;

		case INSTR_MOV_R16_AR32  :
		case INSTR_MOV_R16_AR32D :
		{
			H8S_GET_DATA_R16_AR32
			sprintf( buffer, instr_text, RN16( reg_src), RN32( reg_dst));
		}
		break;

		case INSTR_MOV_AR32_R32  :
		case INSTR_MOV_AR32I_R32 :
		{
			H8S_GET_DATA_AR32_R32
			sprintf( buffer, instr_text, RN32( reg_src), RN32( reg_dst));
		}
		break;

		case INSTR_MOV_R32_AI32 :
		{
			H8S_GET_DATA_R32_AI32
			sprintf( buffer, instr_text, RN32( reg_src), imm);
		}
		break;

		case INSTR_MOV_R32_R32 :
		case INSTR_SUB_R32_R32 :
		case INSTR_CMP_R32_R32 :
		case INSTR_ADD_R32_R32 :
		{
			H8S_GET_DATA_R32_R32_1
			sprintf( buffer, instr_text, RN32( reg_src), RN32( reg_dst));
		}
		break;

		case INSTR_OR_R32_R32  :
		case INSTR_XOR_R32_R32 :
		case INSTR_AND_R32_R32 :
		{
			H8S_GET_DATA_R32_R32_3
			sprintf( buffer, instr_text, RN32( reg_src), RN32( reg_dst));
		}
		break;

		case INSTR_SUB_R16_R16 :
		case INSTR_MOV_R16_R16 :
		case INSTR_ADD_R16_R16 :
		case INSTR_CMP_R16_R16 :
		case INSTR_OR_R16_R16  :
		case INSTR_XOR_R16_R16 :
		case INSTR_AND_R16_R16 :
		{
			H8S_GET_DATA_R16_R16
			sprintf( buffer, instr_text, RN16( reg_src), RN16( reg_dst));
		}
		break;

		case INSTR_SUB_R8_R8  :
		case INSTR_MOV_R8_R8  :
		case INSTR_CMP_R8_R8  :
		case INSTR_OR_R8_R8   :
		case INSTR_XOR_R8_R8  :
		case INSTR_AND_R8_R8  :
		case INSTR_ADD_R8_R8  :
		case INSTR_SUBX_R8_R8 :
		case INSTR_ADDX_R8_R8 :
		case INSTR_BCLR_R8_R8 :
		{
			H8S_GET_DATA_R8_R8
			sprintf( buffer, instr_text, RN8( reg_src), RN8( reg_dst));
		}
		break;

		case INSTR_MOV_AR32_R8  :
		case INSTR_MOV_AR32I_R8 :
		{
			H8S_GET_DATA_AR32_R8
			sprintf( buffer, instr_text, RN32( reg_src), RN8( reg_dst));
		}
		break;

		case INSTR_MOV_R8_AR32  :
		case INSTR_MOV_R8_AR32D :
		{
			H8S_GET_DATA_R8_AR32
			sprintf( buffer, instr_text, RN8( reg_src), RN32( reg_dst));
		}
		break;

		case INSTR_MOV_R8_AI16 :
		{
			H8S_GET_DATA_R8_AI16
			sprintf( buffer, instr_text, RN8( reg_src), imm);
		}
		break;

		case INSTR_MOV_R32_AI16 :
		{
			H8S_GET_DATA_R32_AI16
			sprintf( buffer, instr_text, RN32( reg_src), imm);
		}
		break;

		case INSTR_MOV_R16_AI16 :
		{
			H8S_GET_DATA_R16_AI16
			sprintf( buffer, instr_text, RN16( reg_src), imm);
		}
		break;

		case INSTR_MOV_AI16_R8 :
		{
			H8S_GET_DATA_AI16_R8
			sprintf( buffer, instr_text, imm, RN8( reg_dst));
		}
		break;

		case INSTR_MOV_AI16_R16 :
		{
			H8S_GET_DATA_AI16_R16
			sprintf( buffer, instr_text, imm, RN16( reg_dst));
		}
		break;

		case INSTR_ORC_I8_CCR  :
		case INSTR_ANDC_I8_CCR :
		{
			H8S_GET_DATA_I8_CCR
			sprintf( buffer, instr_text, imm);
		}
		break;

		case INSTR_MOV_I16_R16 :
		case INSTR_ADD_I16_R16 :
		case INSTR_CMP_I16_R16 :
		case INSTR_SUB_I16_R16 :
		case INSTR_XOR_I16_R16 :
		case INSTR_AND_I16_R16 :
		{
			H8S_GET_DATA_I16_R16
			sprintf( buffer, instr_text, imm, RN16( reg_dst));
		}
		break;

		case INSTR_BRA_O8 :
		case INSTR_BRN_O8 :
		case INSTR_BHI_O8 :
		case INSTR_BLS_O8 :
		case INSTR_BCC_O8 :
		case INSTR_BCS_O8 :
		case INSTR_BNE_O8 :
		case INSTR_BEQ_O8 :
		case INSTR_BVC_O8 :
		case INSTR_BVS_O8 :
		case INSTR_BPL_O8 :
		case INSTR_BMI_O8 :
		case INSTR_BGE_O8 :
		case INSTR_BLT_O8 :
		case INSTR_BGT_O8 :
		case INSTR_BLE_O8 :
		case INSTR_BSR_O8 :
		{
			H8S_GET_DATA_O8
			sprintf( buffer, instr_text, pc + (char)imm + instr_size);
			if (index == INSTR_BSR_O8) instr_flag |= DASMFLAG_STEP_OVER;
		}
		break;

		case INSTR_BRA_O16 :
		case INSTR_BRN_O16 :
		case INSTR_BHI_O16 :
		case INSTR_BLS_O16 :
		case INSTR_BCC_O16 :
		case INSTR_BCS_O16 :
		case INSTR_BNE_O16 :
		case INSTR_BEQ_O16 :
		case INSTR_BVC_O16 :
		case INSTR_BVS_O16 :
		case INSTR_BPL_O16 :
		case INSTR_BMI_O16 :
		case INSTR_BGE_O16 :
		case INSTR_BLT_O16 :
		case INSTR_BGT_O16 :
		case INSTR_BLE_O16 :
		case INSTR_BSR_O16 :
		{
			H8S_GET_DATA_O16
			sprintf( buffer, instr_text, pc + (short)imm + instr_size);
			if (index == INSTR_BSR_O16) instr_flag |= DASMFLAG_STEP_OVER;
		}
		break;

		case INSTR_JSR_I24 :
		case INSTR_JMP_I24 :
		{
			H8S_GET_DATA_I24
			sprintf( buffer, instr_text, imm);
			if (index == INSTR_JSR_I24) instr_flag |= DASMFLAG_STEP_OVER;
		}
		break;

		case INSTR_JSR_AR32 :
		case INSTR_JMP_AR32 :
		{
			H8S_GET_DATA_AR32
			sprintf( buffer, instr_text, RN32( reg_src));
			if (index == INSTR_JSR_AR32) instr_flag |= DASMFLAG_STEP_OVER;
		}
		break;

		case INSTR_ADDS_XX_R32 :
		case INSTR_SUBS_XX_R32 :
		{
			H8S_GET_DATA_Ix_R32_ADDS_SUBS
			sprintf( buffer, instr_text, imm, RN32( reg_dst));
		}
		break;

		case INSTR_AND_I8_R8  :
		case INSTR_MOV_I8_R8  :
		case INSTR_OR_I8_R8   :
		case INSTR_ADD_I8_R8  :
		case INSTR_ADDX_I8_R8 :
		case INSTR_CMP_I8_R8  :
		case INSTR_XOR_I8_R8  :
		{
			H8S_GET_DATA_I8_R8
			sprintf( buffer, instr_text, imm, RN8( reg_dst));
		}
		break;

		case INSTR_MOV_R8_AI8 :
		{
			H8S_GET_DATA_R8_AI8
			sprintf( buffer, instr_text, RN8( reg_src), imm);
		}
		break;

		case INSTR_MOV_AI8_R8 :
		{
			H8S_GET_DATA_AI8_R8
			sprintf( buffer, instr_text, imm, RN8( reg_dst));
		}
		break;

		case INSTR_MOV_AI16_R32 :
		{
			H8S_GET_DATA_AI16_R32
			sprintf( buffer, instr_text, imm, RN32( reg_dst));
		}
		break;

		#ifdef ALIAS_PUSH

		case INSTR_PUSH_R16 :
		{
			H8S_GET_DATA_R16_PUSH
			sprintf( buffer, instr_text, RN16( reg_src));
		}
		break;

		case INSTR_PUSH_R32 :
		{
			H8S_GET_DATA_R32_PUSH
			sprintf( buffer, instr_text, RN32( reg_src));
		}
		break;

		#endif

		#ifdef ALIAS_POP

		case INSTR_POP_R16 :
		{
			H8S_GET_DATA_R16_POP
			sprintf( buffer, instr_text, RN16( reg_dst));
		}
		break;

		case INSTR_POP_R32 :
		{
			H8S_GET_DATA_R32_POP
			sprintf( buffer, instr_text, RN32( reg_dst));
		}
		break;

		#endif

		case INSTR_ROTL_XX_R32  :
		case INSTR_ROTXL_XX_R32 :
		case INSTR_ROTR_XX_R32  :
		case INSTR_SHAL_XX_R32  :
		case INSTR_SHAR_XX_R32  :
		case INSTR_SHLL_XX_R32  :
		case INSTR_SHLR_XX_R32  :
		{
			H8S_GET_DATA_Ix_R32_SH
			sprintf( buffer, instr_text, imm, RN32( reg_dst));
		}
		break;

		case INSTR_ROTL_XX_R16 :
		case INSTR_SHAL_XX_R16 :
		case INSTR_SHAR_XX_R16 :
		case INSTR_SHLL_XX_R16 :
		case INSTR_SHLR_XX_R16 :
		{
			H8S_GET_DATA_Ix_R16_SH
			sprintf( buffer, instr_text, imm, RN16( reg_dst));
		}
		break;

		case INSTR_ROTL_XX_R8  :
		case INSTR_ROTR_XX_R8  :
		case INSTR_ROTXL_XX_R8 :
		case INSTR_SHAL_XX_R8  :
		case INSTR_SHLL_XX_R8  :
		case INSTR_SHLR_XX_R8  :
		{
			H8S_GET_DATA_Ix_R8_SH
			sprintf( buffer, instr_text, imm, RN8( reg_dst));
		}
		break;

		case INSTR_STM_R32R32_AR32D :
		{
			H8S_GET_DATA_R32R32_AR32D
			sprintf( buffer, instr_text, RN32( reg_src), RN32( reg_src+reg_cnt), RN32( reg_dst));
		}
		break;

		case INSTR_LDM_AR32I_R32R32 :
		{
			H8S_GET_DATA_AR32I_R32R32
			sprintf( buffer, instr_text, RN32( reg_src), RN32( reg_dst-reg_cnt), RN32( reg_dst));
		}
		break;

		case INSTR_MOV_R32_AR32  :
		case INSTR_MOV_R32_AR32D :
		{
			H8S_GET_DATA_R32_AR32
			sprintf( buffer, instr_text, RN32( reg_src), RN32( reg_dst));
		}
		break;

		case INSTR_MOV_AI32_R8 :
		{
			H8S_GET_DATA_AI32_R8
			sprintf( buffer, instr_text, imm, RN8( reg_dst));
		}
		break;

		case INSTR_MOV_AI32_R16 :
		{
			H8S_GET_DATA_AI32_R16
			sprintf( buffer, instr_text, imm, RN16( reg_dst));
		}
		break;

		case INSTR_MOV_R16_AI32 :
		{
			H8S_GET_DATA_R16_AI32
			sprintf( buffer, instr_text, RN16( reg_src), imm);
		}
		break;

		case INSTR_BCLR_Ix_AI32 :
		case INSTR_BSET_Ix_AI32 :
		case INSTR_BNOT_Ix_AI32 :
		case INSTR_BTST_Ix_AI32 :
		{
			H8S_GET_DATA_Ix_AI32
			sprintf( buffer, instr_text, imm_l, imm_r);
		}
		break;

		case INSTR_BCLR_R8_AI32 :
		case INSTR_BSET_R8_AI32 :
		{
			H8S_GET_DATA_R8_AI32_BCLR
			sprintf( buffer, instr_text, RN8( reg_src), imm);
		}
		break;

		case INSTR_MOV_R8_AI32 :
		{
			H8S_GET_DATA_R8_AI32
			sprintf( buffer, instr_text, RN8( reg_src), imm);
		}
		break;

		case INSTR_NOT_R8 :
		case INSTR_NEG_R8 :
		{
			H8S_GET_DATA_R8
			sprintf( buffer, instr_text, RN8( reg_src));
		}
		break;

		case INSTR_NEG_R16  :
		case INSTR_NOT_R16  :
		case INSTR_EXTU_R16 :
		case INSTR_EXTS_R16 :
		{
			H8S_GET_DATA_R16
			sprintf( buffer, instr_text, RN16( reg_src));
		}
		break;

		case INSTR_EXTS_R32 :
		case INSTR_EXTU_R32 :
		case INSTR_NEG_R32  :
		case INSTR_NOT_R32  :
		{
			H8S_GET_DATA_R32
			sprintf( buffer, instr_text, RN32( reg_src));
		}
		break;

		///////////////
		// INC - DEC //
		///////////////

		case INSTR_DEC_XX_R8 :
		case INSTR_INC_XX_R8 :
		{
			H8S_GET_DATA_Ix_R8_INC_DEC
			sprintf( buffer, instr_text, RN8( reg_dst));
		}
		break;

		case INSTR_INC_XX_R16 :
		case INSTR_DEC_XX_R16 :
		{
			H8S_GET_DATA_Ix_R16_INC_DEC;
			sprintf( buffer, instr_text, imm, RN16( reg_dst));
		}
		break;

		case INSTR_INC_XX_R32 :
		case INSTR_DEC_XX_R32 :
		{
			H8S_GET_DATA_Ix_R32_INC_DEC
			sprintf( buffer, instr_text, imm, RN32( reg_dst));
		}
		break;

		case INSTR_BCLR_R8_AI8 :
		{
			H8S_GET_DATA_R8_AI8_BCLR
			sprintf( buffer, instr_text, RN8( reg_src), imm);
		}
		break;

		case INSTR_BAND_Ix_R8  :
		case INSTR_BTST_Ix_R8  :
		case INSTR_BLD_Ix_R8   :
		case INSTR_BILD_Ix_R8  :
		case INSTR_BST_Ix_R8   :
		case INSTR_BCLR_Ix_R8  :
		case INSTR_BIAND_Ix_R8 :
		{
			H8S_GET_DATA_Ix_R8
			sprintf( buffer, instr_text, imm, RN8( reg_dst));
		}
		break;

		case INSTR_DIVXU_R8_R16 :
		case INSTR_MULXU_R8_R16 :
		{
			H8S_GET_DATA_R8_R16_1
			sprintf( buffer, instr_text, RN8( reg_src), RN16( reg_dst));
		}
		break;

		case INSTR_DIVXU_R16_R32 :
		case INSTR_MULXU_R16_R32 :
		{
			H8S_GET_DATA_R16_R32_1
			sprintf( buffer, instr_text, RN16( reg_src), RN32( reg_dst));
		}
		break;

		case INSTR_LDC_R8_CCR :
		{
			H8S_GET_DATA_R8_LDC
			sprintf( buffer, instr_text, RN8( reg_src));
		}
		break;

		case INSTR_LDC_AR32I_CCR :
		{
			H8S_GET_DATA_R32_LDC
			sprintf( buffer, instr_text, RN32( reg_src));
		}
		break;

		case INSTR_STC_CCR_R8 :
		{
			H8S_GET_DATA_R8_STC
			sprintf( buffer, instr_text, RN8( reg_dst));
		}
		break;

		case INSTR_STC_CCR_AR32D :
		{
			H8S_GET_DATA_R32_STC
			sprintf( buffer, instr_text, RN32( reg_dst));
		}
		break;

		case INSTR_DIVXS_R16_R32 :
		case INSTR_MULXS_R16_R32 :
		{
			H8S_GET_DATA_R16_R32_3
			sprintf( buffer, instr_text, RN16( reg_src), RN32( reg_dst));
		}
		break;

		case INSTR_MULXS_R8_R16 :
		{
			H8S_GET_DATA_R8_R16_3
			sprintf( buffer, instr_text, RN8( reg_src), RN16( reg_dst));
		}
		break;

		case INSTR_BIAND_Ix_AR32 :
		case INSTR_BILD_Ix_AR32  :
		case INSTR_BAND_Ix_AR32  :
		case INSTR_BTST_Ix_AR32  :
		case INSTR_BSET_Ix_AR32  :
		case INSTR_BCLR_Ix_AR32  :
		{
			H8S_GET_DATA_Ix_AR32
			sprintf( buffer, instr_text, imm, RN32( reg_dst));
		}
		break;

		case INSTR_BCLR_Ix_AI8  :
		case INSTR_BSET_Ix_AI8  :
		case INSTR_BNOT_Ix_AI8  :
		case INSTR_BTST_Ix_AI8  :
		case INSTR_BAND_Ix_AI8  :
		case INSTR_BIAND_Ix_AI8 :
		{
			H8S_GET_DATA_Ix_AI8
			sprintf( buffer, instr_text, imm_l, imm_r);
		}
		break;

		case INSTR_BCLR_R8_AR32 :
		{
			H8S_GET_DATA_R8_AR32_BCLR
			sprintf( buffer, instr_text, RN8( reg_src), RN8( reg_dst));
		}
		break;

		case INSTR_RTS :
		case INSTR_RTE :
		{
			strcpy( buffer, instr_text);
			instr_flag |= DASMFLAG_STEP_OUT;
		}
		break;

		default :
		{
			strcpy( buffer, instr_text);
		}
		break;
	}

	if (instr_size < 2) instr_size = 2; // 0 = loops forever, 1 = assert

	return instr_size | instr_flag | DASMFLAG_SUPPORTED;
}
