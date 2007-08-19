/*

	Hitachi H8S/2xxx MCU

	(c) 2001-2007 Tim Schuerewegen

	H8S/2241
	H8S/2246
	H8S/2323

*/

#ifndef _H8SOPCD_H_
#define _H8SOPCD_H_

#include "driver.h"

#define ALIAS_PUSH
#define ALIAS_POP

enum
{
	// UNKNOWN
	INSTR_UNKNOWN = 0       ,
	// NOP
	INSTR_NOP               ,
	// MOV
	INSTR_MOV_AI8_R8        ,
	INSTR_MOV_AI16_R8       ,
	INSTR_MOV_AI16_R16      ,
	INSTR_MOV_AI16_R32      ,
	INSTR_MOV_AI16R32_R8    ,
	INSTR_MOV_AI16R32_R16   ,
	INSTR_MOV_AI16R32_R32   ,
	INSTR_MOV_AI32R32_R8    ,
	INSTR_MOV_AI32R32_R16   ,
	INSTR_MOV_AI32R32_R32   ,
	INSTR_MOV_AI32_R8       ,
	INSTR_MOV_AI32_R16      ,
	INSTR_MOV_AI32_R32      ,
	INSTR_MOV_AR32_R8       ,
	INSTR_MOV_AR32_R16      ,
	INSTR_MOV_AR32_R32      ,
	INSTR_MOV_AR32I_R8      ,
	INSTR_MOV_AR32I_R16     ,
	INSTR_MOV_AR32I_R32     ,
	INSTR_MOV_I8_R8         ,
	INSTR_MOV_I16_R16       ,
	INSTR_MOV_I32_R32       ,
	INSTR_MOV_R8_AI8        ,
	INSTR_MOV_R8_AI16       ,
	INSTR_MOV_R8_AI16R32    ,
	INSTR_MOV_R8_AI32       ,
	INSTR_MOV_R8_AI32R32    ,
	INSTR_MOV_R8_AR32       ,
	INSTR_MOV_R8_AR32D      ,
	INSTR_MOV_R8_R8         ,
	INSTR_MOV_R16_AI16      ,
	INSTR_MOV_R16_AI16R32   ,
	INSTR_MOV_R16_AI32      ,
	INSTR_MOV_R16_AI32R32   ,
	INSTR_MOV_R16_AR32      ,
	INSTR_MOV_R16_AR32D     ,
	INSTR_MOV_R16_R16       ,
	INSTR_MOV_R32_AI16      ,
	INSTR_MOV_R32_AI16R32   ,
	INSTR_MOV_R32_AI32      ,
	INSTR_MOV_R32_AI32R32   ,
	INSTR_MOV_R32_AR32      ,
	INSTR_MOV_R32_AR32D     ,
	INSTR_MOV_R32_R32       ,
	// PUSH
	#ifdef ALIAS_PUSH
	INSTR_PUSH_R16          ,
	INSTR_PUSH_R32          ,
	#endif
	// POP
	#ifdef ALIAS_POP
	INSTR_POP_R16           ,
	INSTR_POP_R32           ,
	#endif
	// BRANCH (8-bit)
	INSTR_BCC_O8            , // branch carry clear / high or same (C=0)
	INSTR_BCS_O8            , // branch carry set / low (C=1)
	INSTR_BEQ_O8            , // branch equal (Z=1)
	INSTR_BGE_O8            , // branch greater or equal (N or V = 0)
	INSTR_BGT_O8            , // branch greater than (Z and (N or V) = 0)
	INSTR_BHI_O8            , // branch high (C and Z = 0)
	INSTR_BLE_O8            , // branch less or equal (Z and (N or V) = 1)
	INSTR_BLS_O8            , // branch low or same (C and Z = 1)
	INSTR_BLT_O8            , // branch less than (N or V = 1)
	INSTR_BMI_O8            , // branch minus (N=1)
	INSTR_BNE_O8            , // branch not equal (Z=0)
	INSTR_BPL_O8            , // branch plus (N=0)
	INSTR_BRA_O8            , // branch always
	INSTR_BRN_O8            , // branch never
	INSTR_BVC_O8            , // ?
	INSTR_BVS_O8            , // ?
	// BRANCH (16-bit)
	INSTR_BCC_O16           ,
	INSTR_BCS_O16           ,
	INSTR_BEQ_O16           ,
	INSTR_BGE_O16           ,
	INSTR_BGT_O16           ,
	INSTR_BHI_O16           ,
	INSTR_BLE_O16           ,
	INSTR_BLS_O16           ,
	INSTR_BLT_O16           ,
	INSTR_BMI_O16           ,
	INSTR_BNE_O16           ,
	INSTR_BPL_O16           ,
	INSTR_BRA_O16           ,
	INSTR_BRN_O16           ,
	INSTR_BVC_O16           ,
	INSTR_BVS_O16           ,
	// ADD
	INSTR_ADD_I8_R8         ,
	INSTR_ADD_I16_R16       ,
	INSTR_ADD_I32_R32       ,
	INSTR_ADD_R8_R8         ,
	INSTR_ADD_R16_R16       ,
	INSTR_ADD_R32_R32       ,
	// ADDS
	INSTR_ADDS_XX_R32       ,
	// ADDX
	INSTR_ADDX_I8_R8        ,
	INSTR_ADDX_R8_R8        ,
	// AND
	INSTR_AND_I8_R8         ,
	INSTR_AND_I16_R16       ,
	INSTR_AND_I32_R32       ,
	INSTR_AND_R8_R8         ,
	INSTR_AND_R16_R16       ,
	INSTR_AND_R32_R32       ,
	// ANDC
	INSTR_ANDC_I8_CCR       ,
	// BAND
	INSTR_BAND_Ix_AI8       ,
	INSTR_BAND_Ix_AR32      ,
	INSTR_BAND_Ix_R8        ,
	// BCLR
	INSTR_BCLR_Ix_AI8       ,
	INSTR_BCLR_Ix_AI32      ,
	INSTR_BCLR_Ix_AR32      ,
	INSTR_BCLR_Ix_R8        ,
	INSTR_BCLR_R8_AI8       ,
	INSTR_BCLR_R8_AI32      ,
	INSTR_BCLR_R8_AR32      ,
	INSTR_BCLR_R8_R8        ,
	// BIAND
	INSTR_BIAND_Ix_AI8      ,
	INSTR_BIAND_Ix_AR32     ,
	INSTR_BIAND_Ix_R8       ,
	// BILD
	INSTR_BILD_Ix_AR32      ,
	INSTR_BILD_Ix_R8        ,
	// BLD
	INSTR_BLD_Ix_R8         ,
	// BNOT
	INSTR_BNOT_Ix_AI8       ,
	INSTR_BNOT_Ix_AI32      ,
	// BSET
	INSTR_BSET_Ix_AI8       ,
	INSTR_BSET_Ix_AI32      ,
	INSTR_BSET_Ix_AR32      ,
	INSTR_BSET_R8_AI32      ,
	// BSR
	INSTR_BSR_O8            ,
	INSTR_BSR_O16           ,
	// BST
	INSTR_BST_Ix_R8         ,
	// BTST
	INSTR_BTST_Ix_AI8       ,
	INSTR_BTST_Ix_AI32      ,
	INSTR_BTST_Ix_AR32      ,
	INSTR_BTST_Ix_R8        ,
	// CMP
	INSTR_CMP_I8_R8         ,
	INSTR_CMP_I16_R16       ,
	INSTR_CMP_I32_R32       ,
	INSTR_CMP_R8_R8         ,
	INSTR_CMP_R16_R16       ,
	INSTR_CMP_R32_R32       ,
	// DEC
	INSTR_DEC_XX_R8         ,
	INSTR_DEC_XX_R16        ,
	INSTR_DEC_XX_R32        ,
	// DIVXU
	INSTR_DIVXU_R8_R16      ,
	INSTR_DIVXU_R16_R32     ,
	// DIVXS
	INSTR_DIVXS_R16_R32     ,
	// EEPMOV
	INSTR_EEPMOV_W          ,
	// EXTS
	INSTR_EXTS_R16          ,
	INSTR_EXTS_R32          ,
	// EXTU
	INSTR_EXTU_R16          ,
	INSTR_EXTU_R32          ,
	// INC
	INSTR_INC_XX_R8         ,
	INSTR_INC_XX_R16        ,
	INSTR_INC_XX_R32        ,
	// JMP
	INSTR_JMP_AR32          ,
	INSTR_JMP_I24           ,
	// JSR
	INSTR_JSR_AR32          ,
	INSTR_JSR_I24           ,
	// LDC
	INSTR_LDC_AR32I_CCR     ,
	INSTR_LDC_R8_CCR        ,
	// LDM
	INSTR_LDM_AR32I_R32R32  ,
	// MULXS
	INSTR_MULXS_R8_R16      ,
	INSTR_MULXS_R16_R32     ,
	// MULXU
	INSTR_MULXU_R8_R16      ,
	INSTR_MULXU_R16_R32     ,
	// NEG
	INSTR_NEG_R8            ,
	INSTR_NEG_R16           ,
	INSTR_NEG_R32           ,
	// NOT
	INSTR_NOT_R8            ,
	INSTR_NOT_R16           ,
	INSTR_NOT_R32           ,
	// OR
	INSTR_OR_I8_R8          ,
	INSTR_OR_I32_R32        ,
	INSTR_OR_R8_R8          ,
	INSTR_OR_R16_R16        ,
	INSTR_OR_R32_R32        ,
	// ORC
	INSTR_ORC_I8_CCR        ,
	// ROTL
	INSTR_ROTL_XX_R8        ,
	INSTR_ROTL_XX_R16       ,
	INSTR_ROTL_XX_R32       ,
	// ROTR
	INSTR_ROTR_XX_R8        ,
	INSTR_ROTR_XX_R32       ,
	// ROTXL
	INSTR_ROTXL_XX_R8       ,
	INSTR_ROTXL_XX_R32      ,
	// RTE
	INSTR_RTE               ,
	// RTS
	INSTR_RTS               ,
	// SHAL
	INSTR_SHAL_XX_R8        ,
	INSTR_SHAL_XX_R16       ,
	INSTR_SHAL_XX_R32       ,
	// SHAR
	INSTR_SHAR_XX_R16       ,
	INSTR_SHAR_XX_R32       ,
	// SHLL
	INSTR_SHLL_XX_R8        ,
	INSTR_SHLL_XX_R16       ,
	INSTR_SHLL_XX_R32       ,
	// SHLR
	INSTR_SHLR_XX_R8        ,
	INSTR_SHLR_XX_R16       ,
	INSTR_SHLR_XX_R32       ,
	// SLEEP
	INSTR_SLEEP             ,
	// STC
	INSTR_STC_CCR_AR32D     ,
	INSTR_STC_CCR_R8        ,
	// STM
	INSTR_STM_R32R32_AR32D  ,
	// SUB
	INSTR_SUB_I16_R16       ,
	INSTR_SUB_I32_R32       ,
	INSTR_SUB_R8_R8         ,
	INSTR_SUB_R16_R16       ,
	INSTR_SUB_R32_R32       ,
	// SUBS
	INSTR_SUBS_XX_R32       ,
	// SUBX
	INSTR_SUBX_R8_R8        ,
	// XOR
	INSTR_XOR_I8_R8         ,
	INSTR_XOR_I16_R16       ,
	INSTR_XOR_I32_R32       ,
	INSTR_XOR_R8_R8         ,
	INSTR_XOR_R16_R16       ,
	INSTR_XOR_R32_R32       ,
	// max
	INSTR_MAX
};

// xx -> 0x00FFFFxx
#define AI8(x) ((x) | 0x00FFFF00)

// 0000..7FFF -> 0x0000xxxx
// 8000..FFFF -> 0x00FFxxxx
#define AI16(x) ((short)(x) & 0x00FFFFFF)

#define INSTR_U16(x) (instr[x+0] <<  8) | (instr[x+1] <<  0)
#define INSTR_U32(x) (instr[x+0] << 24) | (instr[x+1] << 16) | (instr[x+2] << 8) | (instr[x+3] << 0)

static const UINT8 TABLE_IMM_ADDS_SUBS[] = { 1, 0, 0, 0, 0, 0, 0, 0, 2, 4, 0, 0, 0, 0, 0, 0 };

// R8 ..

#define H8S_GET_DATA_R8 \
	UINT8 reg_src = (instr[1] >> 0) & 0xF;

#define H8S_GET_DATA_R8_R8 \
	UINT8 reg_src = (instr[1] >> 4) & 0xF; \
	UINT8 reg_dst = (instr[1] >> 0) & 0xF;

#define H8S_GET_DATA_R8_R16_1 \
	UINT8 reg_src = (instr[1] >> 4) & 0xF; \
	UINT8 reg_dst = (instr[1] >> 0) & 0xF;

#define H8S_GET_DATA_R8_R16_3 \
	UINT8 reg_src = (instr[3] >> 4) & 0xF; \
	UINT8 reg_dst = (instr[3] >> 0) & 0xF;

#define H8S_GET_DATA_R8_AI8 \
	UINT8  reg_src = instr[0] & 0xF; \
	UINT32 imm     = AI8(instr[1]);

#define H8S_GET_DATA_R8_AI8_BCLR \
	UINT8  reg_src = (instr[3] >> 4) & 0xF; \
	UINT32 imm     = AI8(instr[1]);

#define H8S_GET_DATA_R8_AI16 \
	UINT8  reg_src = (instr[1] >> 0) & 0xF; \
	UINT32 imm     = AI16(INSTR_U16(2));

#define H8S_GET_DATA_R8_AI32 \
	UINT8  reg_src = (instr[1] >> 0) & 0xF; \
	UINT32 imm     = INSTR_U32(2);

#define H8S_GET_DATA_R8_AI32R32 \
	UINT8  reg_dst = (instr[1] >> 4) & 7; \
	UINT8  reg_src = (instr[3] >> 0) & 0xF; \
	UINT32 imm     = INSTR_U32(4);

#define H8S_GET_DATA_R8_AI32_BCLR \
	UINT32 imm     = INSTR_U32(2); \
	UINT8  reg_src = (instr[7] >> 4) & 0xF;

#define H8S_GET_DATA_R8_AI16R32 \
	UINT8  reg_dst = (instr[1] >> 4) & 7; \
	UINT8  reg_src = (instr[1] >> 0) & 0xF; \
	UINT16 imm     = INSTR_U16(2);

#define H8S_GET_DATA_R8_AR32 \
	UINT8 reg_src = (instr[1] >> 0) & 0xF; \
	UINT8 reg_dst = (instr[1] >> 4) & 7;

#define H8S_GET_DATA_R8_AR32_BCLR \
	UINT8 reg_src = (instr[3] >> 4) & 0xF; \
	UINT8 reg_dst = (instr[1] >> 4) & 7;

#define H8S_GET_DATA_R8_LDC \
	UINT8 reg_src = (instr[1] >> 0) & 0xF;

#define H8S_GET_DATA_R8_STC \
	UINT8 reg_dst = (instr[1] >> 0) & 0xF;

// R16 ..

#define H8S_GET_DATA_R16_R16 \
	UINT8 reg_src = (instr[1] >> 4) & 0xF; \
	UINT8 reg_dst = (instr[1] >> 0) & 0xF;

#define H8S_GET_DATA_R16_R32_1 \
	UINT8 reg_src = (instr[1] >> 4) & 0xF; \
	UINT8 reg_dst = (instr[1] >> 0) & 7;

#define H8S_GET_DATA_R16_R32_3 \
	UINT8 reg_src = (instr[3] >> 4) & 0xF; \
	UINT8 reg_dst = (instr[3] >> 0) & 7;

#define H8S_GET_DATA_R16_AI16 \
	UINT8  reg_src = (instr[1] >> 0) & 0xF; \
	UINT32 imm     = AI16(INSTR_U16(2));

#define H8S_GET_DATA_R16_AI32 \
	UINT8  reg_src = (instr[1] >> 0) & 0xF; \
	UINT32 imm     = INSTR_U32(2);

#define H8S_GET_DATA_R16 \
	UINT8 reg_src = (instr[1] >> 0) & 0xF;

#define H8S_GET_DATA_R16_AI16R32 \
	UINT8  reg_dst = (instr[1] >> 4) & 7; \
	UINT8  reg_src = (instr[1] >> 0) & 0xF; \
	UINT16 imm     = INSTR_U16(2);

#define H8S_GET_DATA_R16_AI32R32 \
	UINT8  reg_dst = (instr[1] >> 4) & 7; \
	UINT8  reg_src = (instr[3] >> 0) & 0xF; \
	UINT32 imm     = INSTR_U32(4);

#define H8S_GET_DATA_R16_AR32 \
	UINT8 reg_dst = (instr[1] >> 4) & 7; \
	UINT8 reg_src = (instr[1] >> 0) & 0xF;

// R32 ..

#define H8S_GET_DATA_R32 \
	UINT8 reg_src = (instr[1] >> 0) & 7;

#define H8S_GET_DATA_R32_R32_1 \
	UINT8 reg_src = (instr[1] >> 4) & 7; \
	UINT8 reg_dst = (instr[1] >> 0) & 7;

#define H8S_GET_DATA_R32_R32_3 \
	UINT8 reg_src = (instr[3] >> 4) & 7; \
	UINT8 reg_dst = (instr[3] >> 0) & 7;

#define H8S_GET_DATA_R32_AR32 \
	UINT8 reg_dst = (instr[3] >> 4) & 7; \
	UINT8 reg_src = (instr[3] >> 0) & 7;

#define H8S_GET_DATA_R32_AI16 \
	UINT8  reg_src = (instr[3] >> 0) & 7; \
	UINT32 imm     = AI16(INSTR_U16(4));

#define H8S_GET_DATA_R32_AI16R32 \
	UINT8  reg_dst = (instr[3] >> 4) & 7; \
	UINT8  reg_src = (instr[3] >> 0) & 7; \
	UINT16 imm     = INSTR_U16(4);

#define H8S_GET_DATA_R32_AI32 \
	UINT8  reg_src = (instr[3] >> 0) & 7; \
	UINT32 imm     = INSTR_U32(4);

#define H8S_GET_DATA_R32_AI32R32 \
	UINT8  reg_dst = (instr[3] >> 4) & 7; \
	UINT8  reg_src = (instr[5] >> 0) & 7; \
	UINT32 imm     = INSTR_U32(6);

#define H8S_GET_DATA_R32_LDC \
	UINT8 reg_src = (instr[3] >> 4) & 7;

#define H8S_GET_DATA_R32_STC \
	UINT8 reg_dst = (instr[3] >> 4) & 7;

#ifdef ALIAS_PUSH

#define H8S_GET_DATA_R16_PUSH \
	UINT8 reg_src = (instr[1] >> 0) & 0xF;
#define H8S_GET_DATA_R32_PUSH \
	UINT8 reg_src = (instr[3] >> 0) & 7;

#endif

#ifdef ALIAS_POP

#define H8S_GET_DATA_R16_POP \
	UINT8 reg_dst = (instr[1] >> 0) & 0xF;
#define H8S_GET_DATA_R32_POP \
	UINT8 reg_dst = (instr[3] >> 0) & 7;

#endif

// R32R32 ..

#define H8S_GET_DATA_R32R32_AR32D \
	UINT8 reg_cnt = (instr[1] >> 4) & 0xF; \
	UINT8 reg_dst = (instr[3] >> 4) & 7; \
	UINT8 reg_src = (instr[3] >> 0) & 7;

// I8 ..

#define H8S_GET_DATA_I8_R8 \
	UINT8 reg_dst = instr[0] & 0xF; \
	UINT8 imm     = instr[1];

#define H8S_GET_DATA_I8_CCR \
	UINT8 imm = instr[1];

// I16 ..

#define H8S_GET_DATA_I16_R16 \
	UINT8  reg_dst = (instr[1] >> 0) & 0xF; \
	UINT16 imm     = INSTR_U16(2);

// I24 ..

#define H8S_GET_DATA_I24 \
	UINT32 imm = (instr[1] << 16) | (instr[2] << 8) | (instr[3] << 0);

// I32 ..

#define H8S_GET_DATA_I32_R32 \
	UINT8  reg_dst = (instr[1] >> 0) & 7; \
	UINT32 imm     = INSTR_U32(2);

// AI8 ..

#define H8S_GET_DATA_AI8_R8 \
	UINT8  reg_dst = instr[0] & 0xF; \
	UINT32 imm     = AI8(instr[1]);

// AI16 ..

#define H8S_GET_DATA_AI16_R8 \
	UINT8  reg_dst = (instr[1] >> 0) & 0xF; \
	UINT32 imm     = AI16(INSTR_U16(2));

#define H8S_GET_DATA_AI16_R16 \
	UINT8  reg_dst = (instr[1] >> 0) & 0xF; \
	UINT32 imm     = AI16(INSTR_U16(2));

#define H8S_GET_DATA_AI16_R32 \
	UINT8  reg_dst = (instr[3] >> 0) & 7; \
	UINT32 imm     = AI16(INSTR_U16(4));

// AI32 ..

#define H8S_GET_DATA_AI32_R8 \
	UINT8  reg_dst = (instr[1] >> 0) & 0xF; \
	UINT32 imm     = INSTR_U32(2);

#define H8S_GET_DATA_AI32_R16 \
	UINT8  reg_dst = (instr[1] >> 0) & 0xF; \
	UINT32 imm     = INSTR_U32(2);

#define H8S_GET_DATA_AI32_R32 \
	UINT8  reg_dst = (instr[3] >> 0) & 7; \
	UINT32 imm     = INSTR_U32(4);

// AI16R32

#define H8S_GET_DATA_AI16R32_R16 \
	UINT8  reg_src = (instr[1] >> 4) & 7; \
	UINT8  reg_dst = (instr[1] >> 0) & 0xF; \
	UINT16 imm     = INSTR_U16(2);

#define H8S_GET_DATA_AI16R32_R32 \
	UINT8  reg_src = (instr[3] >> 4) & 7; \
	UINT8  reg_dst = (instr[3] >> 0) & 7; \
	UINT16 imm     = INSTR_U16(4);

#define H8S_GET_DATA_AI16R32_R8 \
	UINT8  reg_src = (instr[1] >> 4) & 7; \
	UINT8  reg_dst = (instr[1] >> 0) & 0xF; \
	UINT16 imm     = INSTR_U16(2);

// AI32R32 ..

#define H8S_GET_DATA_AI32R32_R8 \
	UINT8  reg_src = (instr[1] >> 4) & 7; \
	UINT8  reg_dst = (instr[3] >> 0) & 0xF; \
	UINT32 imm     = INSTR_U32(4);

#define H8S_GET_DATA_AI32R32_R16 \
	UINT8  reg_src = (instr[1] >> 4) & 7; \
	UINT8  reg_dst = (instr[3] >> 0) & 0xF; \
	UINT32 imm     = INSTR_U32(4);

#define H8S_GET_DATA_AI32R32_R32 \
	UINT8  reg_src = (instr[3] >> 4) & 7; \
	UINT8  reg_dst = (instr[5] >> 0) & 7; \
	UINT32 imm     = INSTR_U32(6);

// AR32I ..

//#define H8S_GET_DATA_AR32I_R8
//	UINT8 reg_src = (instr[1] >> 4) & 7;
//	UINT8 reg_dst = (instr[1] >> 0) & 0xF;

//#define H8S_GET_DATA_AR32I_R16
//	UINT8 reg_src = (instr[1] >> 4) & 7;
//	UINT8 reg_dst = (instr[1] >> 0) & 0xF;

//#define H8S_GET_DATA_AR32I_R32
//	UINT8 reg_src = (instr[3] >> 4) & 7;
//	UINT8 reg_dst = (instr[3] >> 0) & 7;

#define H8S_GET_DATA_AR32I_R32R32 \
	UINT8 reg_cnt = (instr[1] >> 4) & 0xF; \
	UINT8 reg_src = (instr[3] >> 4) & 7; \
	UINT8 reg_dst = (instr[3] >> 0) & 7;

// AR32 ..

#define H8S_GET_DATA_AR32 \
	UINT8 reg_src = (instr[1] >> 4) & 7;

#define H8S_GET_DATA_AR32_R32 \
	UINT8 reg_src = (instr[3] >> 4) & 7; \
	UINT8 reg_dst = (instr[3] >> 0) & 7;

#define H8S_GET_DATA_AR32_R16 \
	UINT8 reg_src = (instr[1] >> 4) & 7; \
	UINT8 reg_dst = (instr[1] >> 0) & 0xF;

#define H8S_GET_DATA_AR32_R8 \
	UINT8 reg_src = (instr[1] >> 4) & 7; \
	UINT8 reg_dst = (instr[1] >> 0) & 0xF;

// O8 ..

#define H8S_GET_DATA_O8 \
	UINT8 imm = instr[1];

// O16 ..

#define H8S_GET_DATA_O16 \
	UINT16 imm = INSTR_U16(2);

// Ix ..

#define H8S_GET_DATA_Ix_R8 \
	UINT8 imm     = (instr[1] >> 4) & 7; \
	UINT8 reg_dst = (instr[1] >> 0) & 0xF;

#define H8S_GET_DATA_Ix_R8_SH \
	UINT8 reg_dst = (instr[1] >> 0) & 0xF; \
	UINT8 imm = ((instr[1] >> 6) & 1) + 1;

#define H8S_GET_DATA_Ix_R16_SH \
	UINT8 reg_dst = (instr[1] >> 0) & 0xF; \
	UINT8 imm     = ((instr[1] >> 6) & 1) + 1;

#define H8S_GET_DATA_Ix_R32_SH \
	UINT8 reg_dst = (instr[1] >> 0) & 7; \
	UINT8 imm     = ((instr[1] >> 6) & 1) + 1;

#define H8S_GET_DATA_Ix_AI8 \
	UINT8  imm_l = (instr[3] >> 4) & 7; \
	UINT32 imm_r = AI8(instr[1]);

#define H8S_GET_DATA_Ix_AI32 \
	UINT32 imm_r = INSTR_U32(2); \
	UINT8  imm_l = (instr[7] >> 4) & 0xF;

#define H8S_GET_DATA_Ix_AR32 \
	UINT8 reg_dst = (instr[1] >> 4) & 7; \
	UINT8 imm     = (instr[3] >> 4) & 7;

// 2nd byte - imm - 00 = 1, 80 = 2, 90 = 4
#define H8S_GET_DATA_Ix_R32_ADDS_SUBS \
	UINT8 reg_dst = (instr[1] >> 0) & 7; \
	UINT8 imm     = TABLE_IMM_ADDS_SUBS[(instr[1] >> 4) & 0xF];

#define H8S_GET_DATA_Ix_R8_INC_DEC \
	UINT8 reg_dst = (instr[1] >> 0) & 0xF;

#define H8S_GET_DATA_Ix_R16_INC_DEC \
	UINT8 reg_dst = (instr[1] >> 0) & 0xF; \
	UINT8 imm     = ((instr[1] >> 7) & 1) + 1;

#define H8S_GET_DATA_Ix_R32_INC_DEC \
	UINT8 reg_dst = (instr[1] >> 0) & 7; \
	UINT8 imm     = ((instr[1] >> 7) & 1) + 1;

typedef struct
{
	UINT8 size;
	const char* text;
} H8S_INSTR_TABLE_ENTRY;

extern H8S_INSTR_TABLE_ENTRY *h8s_instr_table;

void h8s_opcode_init( void);
void h8s_opcode_exit( void);
UINT16 h8s_opcode_find( UINT32 pc, UINT8 *instr, UINT8 fetch);

#endif
