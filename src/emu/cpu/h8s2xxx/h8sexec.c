/*

	Hitachi H8S/2xxx MCU

	(c) 2001-2007 Tim Schuerewegen

	H8S/2241
	H8S/2246
	H8S/2323

*/

// assume external device, 8-bit bus, 2-state access
// i * 4 + j * 4 + k * 4 + l * 2 + m * 4 + n * 1
#define H8S_CYCLES_I(i)       cpu_icount -= (i) * 4
#define H8S_CYCLES_IK(i,k)    cpu_icount -= (i) * 4 + (k) * 4
#define H8S_CYCLES_IKN(i,k,n) cpu_icount -= (i) * 4 + (k) * 4 + (n) * 1
#define H8S_CYCLES_IL(i,l)    cpu_icount -= (i) * 4 + (l) * 2
#define H8S_CYCLES_ILN(i,l,n) cpu_icount -= (i) * 4 + (l) * 2 + (n) * 1
#define H8S_CYCLES_IM(i,m)    cpu_icount -= (i) * 4 + (m) * 4
#define H8S_CYCLES_IMN(i,m,n) cpu_icount -= (i) * 4 + (m) * 4 + (n) * 1
#define H8S_CYCLES_IN(i,n)    cpu_icount -= (i) * 4 + (n) * 1

#define H8S_EXEC_INSTR(name)  void h8s_exec_instr_##name( UINT8 *instr)

#define H8S_EVAL_BRA    TRUE
#define H8S_EVAL_BRN    FALSE
//#define H8S_EVAL_BHI    (  H8S_GET_FLAG_C && !H8S_GET_FLAG_Z )
//#define H8S_EVAL_BLS    ( !H8S_GET_FLAG_C ||  H8S_GET_FLAG_Z )
#define H8S_EVAL_BHI    ( ! ( H8S_GET_FLAG_C || H8S_GET_FLAG_Z ) )
#define H8S_EVAL_BLS    (   ( H8S_GET_FLAG_C || H8S_GET_FLAG_Z ) )
#define H8S_EVAL_BCC    ( !H8S_GET_FLAG_C )
#define H8S_EVAL_BCS    (  H8S_GET_FLAG_C )
#define H8S_EVAL_BNE    ( !H8S_GET_FLAG_Z )
#define H8S_EVAL_BEQ    (  H8S_GET_FLAG_Z )
#define H8S_EVAL_BPL    ( !H8S_GET_FLAG_N )
#define H8S_EVAL_BMI    (  H8S_GET_FLAG_N )
#define H8S_EVAL_BGE    ( ( !H8S_GET_FLAG_N && !H8S_GET_FLAG_V ) || (  H8S_GET_FLAG_N &&  H8S_GET_FLAG_V ) )
#define H8S_EVAL_BLT    ( (  H8S_GET_FLAG_N && !H8S_GET_FLAG_V ) || ( !H8S_GET_FLAG_N &&  H8S_GET_FLAG_V ) )
#define H8S_EVAL_BGT    ((!H8S_GET_FLAG_N && !H8S_GET_FLAG_V && !H8S_GET_FLAG_Z) || (H8S_GET_FLAG_N && H8S_GET_FLAG_V && !H8S_GET_FLAG_Z))
#define H8S_EVAL_BLE    (((H8S_GET_FLAG_N && !H8S_GET_FLAG_V) || (!H8S_GET_FLAG_N && H8S_GET_FLAG_V)) || H8S_GET_FLAG_Z)

#define NEG(data,bits)  (((data) & (1 << ((bits) - 1))) != 0)
#define POS(data,bits)  (((data) & (1 << ((bits) - 1))) == 0)

#define H8S_FLAG_XXX_NZ(data,bits) \
	H8S_SET_FLAG_Z( data == 0 ); \
	H8S_SET_FLAG_N( NEG( data, bits));

// C = Sm � !Dm + !Dm � Rm + Sm � Rm
#define H8S_FLAG_SUB_C(d,s,r,b) \
	H8S_SET_FLAG_C ( (NEG(s,b) && POS(d,b)) || (POS(d,b) && NEG(r,b)) || (NEG(s,b) && NEG(r,b)) );

// C = Sm � Dm + Dm � !Rm + Sm � !Rm
#define H8S_FLAG_ADD_C(d,s,r,b) \
	H8S_SET_FLAG_C ( (NEG(s,b) && NEG(d,b)) || (NEG(d,b) && POS(r,b)) || (NEG(s,b) && POS(r,b)) );

// V = !Sm � Dm � !Rm + Sm � !Dm � Rm
#define H8S_FLAG_SUB_V(d,s,r,b) \
	H8S_SET_FLAG_V ( ((POS(s,b) && NEG(d,b) && POS(r,b)) || (NEG(s,b) &&  POS(d,b) && NEG(r,b))) );

// V = Sm � Dm � !Rm + !Sm � !Dm � Rm
#define H8S_FLAG_ADD_V(d,s,r,b) \
	H8S_SET_FLAG_V ( ((NEG(s,b) && NEG(d,b) && POS(r,b)) || (POS(s,b) &&  POS(d,b) && NEG(r,b))) );

#define H8S_FLAG_SUB_CV(d,s,r,b) \
	H8S_FLAG_SUB_C(d,s,r,b); \
	H8S_FLAG_SUB_V(d,s,r,b);

#define H8S_FLAG_SUB_NZCV(d,s,r,b) \
	H8S_FLAG_XXX_NZ( r, b); \
	H8S_FLAG_SUB_C(d,s,r,b); \
	H8S_FLAG_SUB_V(d,s,r,b);

#define H8S_FLAG_MOV_NZV( val, x) \
	H8S_FLAG_XXX_NZ( val, x); \
	H8S_SET_FLAG_V( 0);

#define H8S_FLAG_ADD_NZCV( val, op1, op2, x) \
	H8S_FLAG_XXX_NZ( val, x); \
	H8S_FLAG_ADD_C( op1, op2, val, x); \
	H8S_FLAG_ADD_V( op1, op2, val, x);

/////////
// CCR //
/////////

static UINT8 cpu_get_ccr( void)
{
	UINT8 i, ccr;
	ccr = 0;
	for (i=0;i<8;i++)
	{
		ccr = ccr << 1;
		if (cpu_get_flag( 7 - i)) ccr += 1;
	}
	return ccr;
}

static void cpu_set_ccr( UINT8 ccr)
{
	UINT8 i;
	for (i=0;i<8;i++) cpu_set_flag( i, (ccr >> i) & 1);
}

/////////////////
// MOV XX , R8 //
/////////////////

// mov.b #imm8, reg8 [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_i8_r8 )
{
	H8S_GET_DATA_I8_R8
	H8S_FLAG_MOV_NZV( imm, 8);
	H8S_SET_REG_8( reg_dst, imm);
	H8S_CYCLES_I( 1);
}

// mov.b reg8, reg8 [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_r8_r8 )
{
	H8S_GET_DATA_R8_R8
	UINT8 val = H8S_GET_REG_8( reg_src);
	H8S_FLAG_MOV_NZV( val, 8);
	H8S_SET_REG_8( reg_dst, val);
	H8S_CYCLES_I( 1);
}

// mov.b @reg32, reg8 [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_ar32_r8 )
{
	H8S_GET_DATA_AR32_R8
	UINT8 val = H8S_GET_MEM_8( H8S_GET_REG_32( reg_src));
	H8S_FLAG_MOV_NZV( val, 8);
	H8S_SET_REG_8( reg_dst, val);
	H8S_CYCLES_IL( 1, 1);
}

// mov.b @(imm:16,reg32), reg8 [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_ai16r32_r8 )
{
	H8S_GET_DATA_AI16R32_R8
	UINT8 val = H8S_GET_MEM_8( (short)imm + H8S_GET_REG_32( reg_src));
	H8S_FLAG_MOV_NZV( val, 8);
	H8S_SET_REG_8( reg_dst, val);
	H8S_CYCLES_IL( 2, 1);
}

// mov.b @(imm:32,reg32), reg8 [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_ai32r32_r8 )
{
	H8S_GET_DATA_AI32R32_R8
	UINT8 val = H8S_GET_MEM_8( (int)imm + H8S_GET_REG_32( reg_src));
	H8S_FLAG_MOV_NZV( val, 8);
	H8S_SET_REG_8( reg_dst, val);
	H8S_CYCLES_IL( 4, 1);
}

// mov.b @reg32+, reg8 [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_ar32i_r8 )
{
	H8S_GET_DATA_AR32_R8
	UINT32 mem = H8S_GET_REG_32( reg_src);
	UINT8 val = H8S_GET_MEM_8( mem);
	H8S_SET_REG_32( reg_src, mem + 1);
	H8S_FLAG_MOV_NZV( val, 8);
	H8S_SET_REG_8( reg_dst, val);
	H8S_CYCLES_ILN( 1, 1, 1);
}

// mov.b @imm:8, reg8 [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_ai8_r8 )
{
	H8S_GET_DATA_AI8_R8
	UINT8 val = H8S_GET_MEM_8( imm);
	H8S_FLAG_MOV_NZV( val, 8);
	H8S_SET_REG_8( reg_dst, val);
	H8S_CYCLES_IL( 1, 1);
}

// mov.b @imm:16, reg8 [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_ai16_r8 )
{
	H8S_GET_DATA_AI16_R8
	UINT8 val = H8S_GET_MEM_8( imm);
	H8S_FLAG_MOV_NZV( val, 8);
	H8S_SET_REG_8( reg_dst, val);
	H8S_CYCLES_IL( 2, 1);
}

// mov.b @imm:32, reg8 [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_ai32_r8 )
{
	H8S_GET_DATA_AI32_R8
	UINT8 val = H8S_GET_MEM_8( imm);
	H8S_FLAG_MOV_NZV( val, 8);
	H8S_SET_REG_8( reg_dst, val);
	H8S_CYCLES_IL( 3, 1);
}

/////////////////
// MOV R8 , XX //
/////////////////

// mov.b reg8, @reg32 [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_r8_ar32 )
{
	H8S_GET_DATA_R8_AR32
	UINT8 val = H8S_GET_REG_8( reg_src);
	H8S_SET_MEM_8( H8S_GET_REG_32( reg_dst), val);
	H8S_FLAG_MOV_NZV( val, 8);
	H8S_CYCLES_IL( 1, 1);
}

// mov.b reg8, @(imm:16,reg32) [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_r8_ai16r32 )
{
	H8S_GET_DATA_R8_AI16R32
	UINT8 val = H8S_GET_REG_8( reg_src);
	H8S_SET_MEM_8( (short)imm + H8S_GET_REG_32( reg_dst), val);
	H8S_FLAG_MOV_NZV( val, 8);
	H8S_CYCLES_IL( 2, 1);
}

// mov.b reg8, @(imm:32,reg32) [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_r8_ai32r32 )
{
	H8S_GET_DATA_R8_AI32R32
	UINT8 val = H8S_GET_REG_8( reg_src);
	H8S_SET_MEM_8( (int)imm + H8S_GET_REG_32( reg_dst), val);
	H8S_FLAG_MOV_NZV( val, 8);
	H8S_CYCLES_IL( 4, 1);
}

// mov.b reg8, @-reg32 [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_r8_ar32d )
{
	H8S_GET_DATA_R8_AR32
	UINT32 mem = H8S_GET_REG_32( reg_dst);
	UINT8 val = H8S_GET_REG_8( reg_src);
	H8S_SET_REG_32( reg_dst, mem - 1);
	H8S_SET_MEM_8( mem - 1, val);
	H8S_FLAG_MOV_NZV( val, 8);
	H8S_CYCLES_ILN( 1, 1, 1);
}

// mov.b reg8, @imm:8 [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_r8_ai8 )
{
	H8S_GET_DATA_R8_AI8
	UINT8 val = H8S_GET_REG_8( reg_src);
	H8S_SET_MEM_8( imm, val);
	H8S_FLAG_MOV_NZV( val, 8);
	H8S_CYCLES_IL( 1, 1);
}

// mov.b reg8, @imm:16 [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_r8_ai16 )
{
	H8S_GET_DATA_R8_AI16
	UINT8 val = H8S_GET_REG_8( reg_src);
	H8S_SET_MEM_8( imm, val);
	H8S_FLAG_MOV_NZV( val, 8);
	H8S_CYCLES_IL( 2, 1);
}

// mov.b reg8, @imm:32 [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_r8_ai32 )
{
	H8S_GET_DATA_R8_AI32
	UINT8 val = H8S_GET_REG_8( reg_src);
	H8S_SET_MEM_8( imm, val);
	H8S_FLAG_MOV_NZV( val, 8);
	H8S_CYCLES_IL( 3, 1);
}

//////////////////
// MOV XX , R16 //
//////////////////

// mov.w #imm16, reg16 [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_i16_r16 )
{
	H8S_GET_DATA_I16_R16
	H8S_FLAG_MOV_NZV( imm, 16);
	H8S_SET_REG_16( reg_dst, imm);
	H8S_CYCLES_I( 2);
}

// mov.w reg16, reg16 [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_r16_r16 )
{
	H8S_GET_DATA_R16_R16
	UINT16 val = H8S_GET_REG_16( reg_src);
	H8S_FLAG_MOV_NZV( val, 16);
	H8S_SET_REG_16( reg_dst, val);
	H8S_CYCLES_I( 1);
}

// mov.w @reg32, reg16 [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_ar32_r16 )
{
	H8S_GET_DATA_AR32_R16
	UINT16 val = H8S_GET_MEM_16( H8S_GET_REG_32( reg_src));
	H8S_FLAG_MOV_NZV( val, 16);
	H8S_SET_REG_16( reg_dst, val);
	H8S_CYCLES_IM( 1, 1);
}

// mov.w @(imm:16,reg32), reg16 [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_ai16r32_r16 )
{
	H8S_GET_DATA_AI16R32_R16
	UINT16 val = H8S_GET_MEM_16( (short)imm + H8S_GET_REG_32( reg_src));
	H8S_FLAG_MOV_NZV( val, 16);
	H8S_SET_REG_16( reg_dst, val);
	H8S_CYCLES_IM( 2, 1);
}

// mov.w @(imm:32,reg32), reg16 [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_ai32r32_r16 )
{
	H8S_GET_DATA_AI32R32_R16
	UINT16 val = H8S_GET_MEM_16( (int)imm + H8S_GET_REG_32( reg_src));
	H8S_FLAG_MOV_NZV( val, 16);
	H8S_SET_REG_16( reg_dst, val);
	H8S_CYCLES_IM( 4, 1);
}

// mov.w @reg32+, reg16 [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_ar32i_r16 )
{
	H8S_GET_DATA_AR32_R16
	UINT32 mem = H8S_GET_REG_32( reg_src);
	UINT16 val = H8S_GET_MEM_16( mem);
	H8S_SET_REG_32( reg_src, mem + 2);
	H8S_FLAG_MOV_NZV( val, 16);
	H8S_SET_REG_16( reg_dst, val);
	H8S_CYCLES_IMN( 1, 1, 1);
}

// mov.w @imm:16, reg16 [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_ai16_r16 )
{
	H8S_GET_DATA_AI16_R16
	UINT16 val = H8S_GET_MEM_16( imm);
	H8S_FLAG_MOV_NZV( val, 16);
	H8S_SET_REG_16( reg_dst, val);
	H8S_CYCLES_IM( 2, 1);
}

// mov.w @imm:32, reg16 [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_ai32_r16 )
{
	H8S_GET_DATA_AI32_R16
	UINT16 val = H8S_GET_MEM_16( imm);
	H8S_FLAG_MOV_NZV( val, 16);
	H8S_SET_REG_16( reg_dst, val);
	H8S_CYCLES_IM( 3, 1);
}

//////////////////
// MOV R16 , XX //
//////////////////

// mov.w reg16, @reg32 [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_r16_ar32 )
{
	H8S_GET_DATA_R16_AR32
	UINT16 val = H8S_GET_REG_16( reg_src);
	H8S_SET_MEM_16( H8S_GET_REG_32( reg_dst), val);
	H8S_FLAG_MOV_NZV( val, 16);
	H8S_CYCLES_IM( 1, 1);
}

// mov.w reg16, @(imm:16,reg32) [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_r16_ai16r32 )
{
	H8S_GET_DATA_R16_AI16R32
	UINT16 val = H8S_GET_REG_16( reg_src);
	H8S_SET_MEM_16( (short)imm + H8S_GET_REG_32( reg_dst), val);
	H8S_FLAG_MOV_NZV( val, 16);
	H8S_CYCLES_IM( 2, 1);
}

// mov.w reg16, @(imm:32,reg32)
H8S_EXEC_INSTR( mov_r16_ai32r32 )
{
	H8S_GET_DATA_R16_AI32R32
	UINT16 val = H8S_GET_REG_16( reg_src);
	H8S_SET_MEM_16( (int)imm + H8S_GET_REG_32( reg_dst), val);
	H8S_FLAG_MOV_NZV( val, 16);
	H8S_CYCLES_IM( 4, 1);
}

// mov.w reg16, @-reg32
H8S_EXEC_INSTR( mov_r16_ar32d )
{
	H8S_GET_DATA_R16_AR32
	UINT32 mem = H8S_GET_REG_32( reg_dst);
	UINT16 val = H8S_GET_REG_16( reg_src);
	H8S_SET_REG_32( reg_dst, mem - 2);
	H8S_SET_MEM_16( mem - 2, val);
	H8S_FLAG_MOV_NZV( val, 16);
	H8S_CYCLES_IMN( 1, 1, 1);
}

// mov.w reg16, @imm:16 [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_r16_ai16 )
{
	H8S_GET_DATA_R16_AI16
	UINT16 val = H8S_GET_REG_16( reg_src);
	H8S_SET_MEM_16( imm, val);
	H8S_FLAG_MOV_NZV( val, 16);
	H8S_CYCLES_IM( 2, 1);
}

// mov.w reg16, @imm:32 [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_r16_ai32 )
{
	H8S_GET_DATA_R16_AI32
	UINT16 val = H8S_GET_REG_16( reg_src);
	H8S_SET_MEM_16( imm, val);
	H8S_FLAG_MOV_NZV( val, 16);
	H8S_CYCLES_IM( 3, 1);
}

//////////////////
// MOV XX , R32 //
//////////////////

// mov.l #imm32, reg32 [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_i32_r32 )
{
	H8S_GET_DATA_I32_R32
	UINT32 val = imm;
	H8S_FLAG_MOV_NZV( val, 32);
	H8S_SET_REG_32( reg_dst, val);
	H8S_CYCLES_I( 3);
}

// mov.l reg32, reg32 [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_r32_r32 )
{
	H8S_GET_DATA_R32_R32_1
	UINT32 val = H8S_GET_REG_32( reg_src);
	H8S_FLAG_MOV_NZV( val, 32);
	H8S_SET_REG_32( reg_dst, val);
	H8S_CYCLES_I( 1);
}

// mov.l @reg32, reg32 [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_ar32_r32 )
{
	H8S_GET_DATA_AR32_R32
	UINT32 val = H8S_GET_MEM_32( H8S_GET_REG_32( reg_src));
	H8S_FLAG_MOV_NZV( val, 32);
	H8S_SET_REG_32( reg_dst, val);
	H8S_CYCLES_IM( 2, 2);
}

// mov.l @(imm:16,reg32), reg32 [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_ai16r32_r32 )
{
	H8S_GET_DATA_AI16R32_R32
	UINT32 val = H8S_GET_MEM_32( (short)imm + H8S_GET_REG_32( reg_src));
	H8S_FLAG_MOV_NZV( val, 32);
	H8S_SET_REG_32( reg_dst, val);
	H8S_CYCLES_IM( 3, 2);
}

// mov.l @(imm:32,reg32), reg32 [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_ai32r32_r32 )
{
	H8S_GET_DATA_AI32R32_R32
	UINT32 val = H8S_GET_MEM_32( (int)imm + H8S_GET_REG_32( reg_src));
	H8S_FLAG_MOV_NZV( val, 32);
	H8S_SET_REG_32( reg_dst, val);
	H8S_CYCLES_IM( 5, 2);
}

// mov.l @reg32+, reg32
H8S_EXEC_INSTR( mov_ar32i_r32 )
{
	H8S_GET_DATA_AR32_R32
	UINT32 mem = H8S_GET_REG_32( reg_src);
	UINT32 val = H8S_GET_MEM_32( mem);
	H8S_SET_REG_32( reg_src, mem + 4);
	H8S_FLAG_MOV_NZV( val, 32);
	H8S_SET_REG_32( reg_dst, val);
	H8S_CYCLES_IMN( 2, 2, 1);
}

// mov.l @imm:16, reg32 [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_ai16_r32 )
{
	H8S_GET_DATA_AI16_R32
	UINT32 val = H8S_GET_MEM_32( imm);
	H8S_FLAG_MOV_NZV( val, 32);
	H8S_SET_REG_32( reg_dst, val);
	H8S_CYCLES_IM( 3, 2);
}

// mov.l @imm:32, reg32 [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_ai32_r32 )
{
	H8S_GET_DATA_AI32_R32
	UINT32 val = H8S_GET_MEM_32( imm);
	H8S_FLAG_MOV_NZV( val, 32);
	H8S_SET_REG_32( reg_dst, val);
	H8S_CYCLES_IM( 4, 2);
}

//////////////////
// MOV R32 , XX //
//////////////////

// mov.l reg32, @reg32 [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_r32_ar32 )
{
	H8S_GET_DATA_R32_AR32
	UINT32 val = H8S_GET_REG_32( reg_src);
	H8S_SET_MEM_32( H8S_GET_REG_32( reg_dst), val);
	H8S_FLAG_MOV_NZV( val, 32);
	H8S_CYCLES_IM( 2, 2);
}

// mov.l reg32, @(imm:16,reg32) [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_r32_ai16r32 )
{
	H8S_GET_DATA_R32_AI16R32
	UINT32 val = H8S_GET_REG_32( reg_src);
	H8S_SET_MEM_32( (short)imm + H8S_GET_REG_32( reg_dst), val);
	H8S_FLAG_MOV_NZV( val, 32);
	H8S_CYCLES_IM( 3, 2);
}

// mov.l reg32, @(imm:32,reg32) [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_r32_ai32r32 )
{
	H8S_GET_DATA_R32_AI32R32
	UINT32 val = H8S_GET_REG_32( reg_src);
	H8S_SET_MEM_32( (int)imm + H8S_GET_REG_32( reg_dst), val);
	H8S_FLAG_MOV_NZV( val, 32);
	H8S_CYCLES_IM( 5, 2);
}

// mov.l reg32, @-reg32
H8S_EXEC_INSTR( mov_r32_ar32d )
{
	H8S_GET_DATA_R32_AR32
	UINT32 mem = H8S_GET_REG_32( reg_dst);
	UINT32 val = H8S_GET_REG_32( reg_src);
	H8S_SET_REG_32( reg_dst, mem - 4);
	H8S_SET_MEM_32( mem - 4, val);
	H8S_FLAG_MOV_NZV( val, 32);
	H8S_CYCLES_IMN( 2, 2, 1);
}

// mov.l reg32, @imm:16 [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_r32_ai16 )
{
	H8S_GET_DATA_R32_AI16
	UINT32 val = H8S_GET_REG_32( reg_src);
	H8S_SET_MEM_32( imm, val);
	H8S_FLAG_MOV_NZV( val, 32);
	H8S_CYCLES_IM( 3, 2);
}

// mov.l reg32, @imm:32 [N=* Z=* V=0]
H8S_EXEC_INSTR( mov_r32_ai32 )
{
	H8S_GET_DATA_R32_AI32
	UINT32 val = H8S_GET_REG_32( reg_src);
	H8S_SET_MEM_32( imm, val);
	H8S_FLAG_MOV_NZV( val, 32);
	H8S_CYCLES_IM( 4, 2);
}

//////////
// PUSH //
//////////

#ifdef ALIAS_PUSH

// push.w reg16 [N=* Z=* V=0]
H8S_EXEC_INSTR( push_r16 )
{
	H8S_GET_DATA_R16_PUSH
	UINT32 mem = H8S_GET_SP();
	UINT16 val = H8S_GET_REG_16( reg_src);
	H8S_SET_SP( mem - 2);
	H8S_SET_MEM_16( mem - 2, val);
	H8S_FLAG_XXX_NZ( val, 16);
	H8S_SET_FLAG_V( 0);
	H8S_CYCLES_IMN( 1, 1, 1);
}

// push.l reg32 [N=* Z=* V=0]
H8S_EXEC_INSTR( push_r32 )
{
	H8S_GET_DATA_R32_PUSH
	UINT32 mem = H8S_GET_SP();
	UINT32 val = H8S_GET_REG_32( reg_src);
	H8S_SET_SP( mem - 4);
	H8S_SET_MEM_32( mem - 4, val);
	H8S_FLAG_XXX_NZ( val, 32);
	H8S_SET_FLAG_V( 0);
	H8S_CYCLES_IMN( 2, 2, 1);
}

#endif

/////////
// POP //
/////////

#ifdef ALIAS_POP

// pop.w reg16 [N=* Z=* V=0]
H8S_EXEC_INSTR( pop_r16 )
{
	H8S_GET_DATA_R16_POP
	UINT32 mem = H8S_GET_SP();
	UINT16 val = H8S_GET_MEM_16( mem);
	H8S_SET_SP( mem + 2);
	H8S_SET_REG_16( reg_dst, val);
	H8S_FLAG_XXX_NZ( val, 16);
	H8S_SET_FLAG_V( 0);
	H8S_CYCLES_IMN( 1, 1, 1);
}

// pop.l reg32 [N=* Z=* V=0]
H8S_EXEC_INSTR( pop_r32 )
{
	H8S_GET_DATA_R32_POP
	UINT32 mem = H8S_GET_SP();
	UINT32 val = H8S_GET_MEM_32( mem);
	H8S_SET_SP( mem + 4);
	H8S_SET_REG_32( reg_dst, val);
	H8S_FLAG_XXX_NZ( val, 32);
	H8S_SET_FLAG_V( 0);
	H8S_CYCLES_IMN( 2, 2, 1);
}

#endif

/////////
// LDM //
/////////

// ldm.l @reg32+, reg32-reg32 [-]
H8S_EXEC_INSTR( ldm_ar32i_r32r32 )
{
	UINT32 data, i;
	H8S_GET_DATA_AR32I_R32R32
	for (i=0;i<=reg_cnt;i++)
	{
		data = H8S_GET_MEM_32( H8S_GET_REG_32( reg_src));
		H8S_SET_REG_32( reg_src, H8S_GET_REG_32( reg_src) + 4);
		H8S_SET_REG_32( reg_dst - i, data);
	}
	H8S_CYCLES_IKN( 2, 2 + reg_cnt * 2, 1);
}

/////////
// STM //
/////////

//stm.l reg32-reg32, @-reg32 [-]
H8S_EXEC_INSTR( stm_r32r32_ar32d )
{
	UINT32 i;
	H8S_GET_DATA_R32R32_AR32D
	for (i=0;i<=reg_cnt;i++)
	{
		H8S_SET_REG_32( reg_dst, H8S_GET_REG_32( reg_dst) - 4);
		H8S_SET_MEM_32( H8S_GET_REG_32( reg_dst), H8S_GET_REG_32( reg_src + i));
	}
	H8S_CYCLES_IKN( 2, 2 + reg_cnt * 2, 1);
}

/////////
// ADD //
/////////

// add.b #imm8, reg8 [H=* N=* Z=* V=* C=*]
H8S_EXEC_INSTR( add_i8_r8 )
{
	H8S_GET_DATA_I8_R8
	UINT8 val1 = H8S_GET_REG_8( reg_dst);
	UINT8 val3 = val1 + imm;
	H8S_FLAG_ADD_NZCV( val3, val1, imm, 8);
	H8S_SET_REG_8( reg_dst, val3);
	H8S_CYCLES_I( 1);
}

// add.b reg8, reg8 [H=* N=* Z=* V=* C=*]
H8S_EXEC_INSTR( add_r8_r8 )
{
	H8S_GET_DATA_R8_R8
	UINT8 val1 = H8S_GET_REG_8( reg_dst);
	UINT8 val2 = H8S_GET_REG_8( reg_src);
	UINT8 val3 = val1 + val2;
	H8S_FLAG_ADD_NZCV( val3, val1, val2, 8);
	H8S_SET_REG_8( reg_dst, val3);
	H8S_CYCLES_I( 1);
}

// add.w #imm16, reg16 [H=* N=* Z=* V=* C=*]
H8S_EXEC_INSTR( add_i16_r16 )
{
	H8S_GET_DATA_I16_R16
	UINT16 val1 = H8S_GET_REG_16( reg_dst);
	UINT16 val3 = val1 + imm;
	H8S_FLAG_ADD_NZCV( val3, val1, imm, 16);
	H8S_SET_REG_16( reg_dst, val3);
	H8S_CYCLES_I( 2);
}

// add.w reg16, reg16 [H=* N=* Z=* V=* C=*]
H8S_EXEC_INSTR( add_r16_r16 )
{
	H8S_GET_DATA_R16_R16
	UINT16 val1 = H8S_GET_REG_16( reg_dst);
	UINT16 val2 = H8S_GET_REG_16( reg_src);
	UINT16 val3 = val1 + val2;
	H8S_FLAG_ADD_NZCV( val3, val1, val2, 16);
	H8S_SET_REG_16( reg_dst, val3);
	H8S_CYCLES_I( 1);
}

// add.l #imm32, reg32 [H=* N=* Z=* V=* C=*]
H8S_EXEC_INSTR( add_i32_r32 )
{
	H8S_GET_DATA_I32_R32
	UINT32 val1 = H8S_GET_REG_32( reg_dst);
	UINT32 val3 = val1 + imm;
	H8S_FLAG_ADD_NZCV( val3, val1, imm, 32);
	H8S_SET_REG_32( reg_dst, val3);
	H8S_CYCLES_I( 3);
}

// add.l reg32, reg32 [H=* N=* Z=* V=* C=*]
H8S_EXEC_INSTR( add_r32_r32 )
{
	H8S_GET_DATA_R32_R32_1
	UINT32 val1 = H8S_GET_REG_32( reg_dst);
	UINT32 val2 = H8S_GET_REG_32( reg_src);
	UINT32 val3 = val1 + val2;
	H8S_FLAG_ADD_NZCV( val3, val1, val2, 32);
	H8S_SET_REG_32( reg_dst, val3);
	H8S_CYCLES_I( 1);
}

//////////
// ADDX //
//////////

// addx #imm8, reg8 [H=* N=* Z=* V=* C=*]
H8S_EXEC_INSTR( addx_i8_r8 )
{
	H8S_GET_DATA_I8_R8
	UINT8 flagc = (H8S_GET_FLAG_C) ? 1 : 0;
	UINT8 val1 = H8S_GET_REG_8( reg_dst);
	UINT8 val2 = imm + flagc;
	UINT8 val3 = val1 + val2;
	H8S_FLAG_ADD_NZCV( val3, val1, val2, 8);
	H8S_SET_REG_8( reg_dst, val3);
	H8S_CYCLES_I( 1);
}

// addx reg8, reg8 [H=* N=* Z=* V=* C=*]
H8S_EXEC_INSTR( addx_r8_r8 )
{
	cpu_halt( "todo: INSTR_ADDX_R8_R8");
	H8S_CYCLES_I( 1);
}

//////////
// ADDS //
//////////

// adds #imm, reg32 [-]
H8S_EXEC_INSTR( adds_xx_r32 )
{
	H8S_GET_DATA_Ix_R32_ADDS_SUBS
	H8S_SET_REG_32( reg_dst, H8S_GET_REG_32( reg_dst) + imm);
	H8S_CYCLES_I( 1);
}

/////////
// INC //
/////////

// inc.b #imm, reg8 [N=* Z=* V=*]
H8S_EXEC_INSTR( inc_xx_r8 )
{
	H8S_GET_DATA_Ix_R8_INC_DEC
	UINT8 val1 = H8S_GET_REG_8( reg_dst);
	UINT8 val3 = val1 + 1; // instruction only supports #1
	H8S_FLAG_XXX_NZ( val3, 8);
	H8S_SET_FLAG_V( POS( val1, 8) && NEG( val3, 8));
	H8S_SET_REG_8( reg_dst, val3);
	H8S_CYCLES_I( 1);
}

// inc.w #imm, reg16 [N=* Z=* V=*]
H8S_EXEC_INSTR( inc_xx_r16 )
{
	H8S_GET_DATA_Ix_R16_INC_DEC
	UINT16 val1 = H8S_GET_REG_16( reg_dst);
	UINT16 val3 = val1 + imm;
	H8S_FLAG_XXX_NZ( val3, 16);
	H8S_SET_FLAG_V( POS( val1, 16) && NEG( val3, 16));
	H8S_SET_REG_16( reg_dst, val3);
	H8S_CYCLES_I( 1);
}

// inc.l #imm, reg32 [N=* Z=* V=*]
H8S_EXEC_INSTR( inc_xx_r32 )
{
	H8S_GET_DATA_Ix_R32_INC_DEC
	UINT32 val1 = H8S_GET_REG_32( reg_dst);
	UINT32 val3 = val1 + imm;
	H8S_FLAG_XXX_NZ( val3, 32);
	H8S_SET_FLAG_V( POS( val1, 32) && NEG( val3, 32));
	H8S_SET_REG_32( reg_dst, val3);
	H8S_CYCLES_I( 1);
}

/////////
// SUB //
/////////

// sub.b reg8, reg8 [H=* N=* Z=* V=* C=*]
H8S_EXEC_INSTR( sub_r8_r8 )
{
	H8S_GET_DATA_R8_R8
	UINT8 val1 = H8S_GET_REG_8( reg_dst);
	UINT8 val2 = H8S_GET_REG_8( reg_src);
	UINT8 val3 = val1 - val2;
	H8S_FLAG_SUB_NZCV( val1, val2, val3, 8);
	H8S_SET_REG_8( reg_dst, val3);
	H8S_CYCLES_I( 1);
}

// sub.w #imm16, reg16 [H=* N=* Z=* V=* C=*]
H8S_EXEC_INSTR( sub_i16_r16 )
{
	H8S_GET_DATA_I16_R16
	UINT16 val1 = H8S_GET_REG_16( reg_dst);
	UINT16 val3 = val1 - imm;
	H8S_FLAG_SUB_NZCV( val1, imm, val3, 16);
	H8S_SET_REG_16( reg_dst, val3);
	H8S_CYCLES_I( 2);
}

// sub.w reg16, reg16 [H=* N=* Z=* V=* C=*]
H8S_EXEC_INSTR( sub_r16_r16 )
{
	H8S_GET_DATA_R16_R16
	UINT16 val1 = H8S_GET_REG_16( reg_dst);
	UINT16 val2 = H8S_GET_REG_16( reg_src);
	UINT16 val3 = val1 - val2;
	H8S_FLAG_SUB_NZCV( val1, val2, val3, 16);
	H8S_SET_REG_16( reg_dst, val3);
	H8S_CYCLES_I( 1);
}

// sub.l #imm32, reg32 [H=* N=* Z=* V=* C=*]
H8S_EXEC_INSTR( sub_i32_r32 )
{
	H8S_GET_DATA_I32_R32
	UINT32 val1 = H8S_GET_REG_32( reg_dst);
	UINT32 val3 = val1 - imm;
	H8S_FLAG_SUB_NZCV( val1, imm, val3, 32);
	H8S_SET_REG_32( reg_dst, val3);
	H8S_CYCLES_I( 3);
}

// sub.l reg32, reg32 [H=* N=* Z=* V=* C=*]
H8S_EXEC_INSTR( sub_r32_r32 )
{
	H8S_GET_DATA_R32_R32_1
	UINT32 val1 = H8S_GET_REG_32( reg_dst);
	UINT32 val2 = H8S_GET_REG_32( reg_src);
	UINT32 val3 = val1 - val2;
	H8S_FLAG_SUB_NZCV( val1, val2, val3, 32);
	H8S_SET_REG_32( reg_dst, val3);
	H8S_CYCLES_I( 1);
}

//////////
// SUBX //
//////////

// subx reg8, reg8 [H=* N=* Z=* V=* C=*]
H8S_EXEC_INSTR( subx_r8_r8 )
{
	H8S_GET_DATA_R8_R8
	UINT8 flagc = (H8S_GET_FLAG_C) ? 1 : 0;
	UINT8 val1 = H8S_GET_REG_8( reg_dst);
	UINT8 val2 = H8S_GET_REG_8( reg_src) + flagc;
	UINT8 val3 = val1 - val2;
	if (val3 != 0) H8S_SET_FLAG_Z( 0);
	H8S_SET_FLAG_N( NEG( val3, 8));
	H8S_FLAG_SUB_CV( val1, val2, val3, 8);
	H8S_SET_REG_8( reg_dst, val3);
	H8S_CYCLES_I( 1);
}

//////////
// SUBS //
//////////

// subs #imm, reg32 [-]
H8S_EXEC_INSTR( subs_xx_r32 )
{
	H8S_GET_DATA_Ix_R32_ADDS_SUBS
	H8S_SET_REG_32( reg_dst, H8S_GET_REG_32( reg_dst) - imm);
	H8S_CYCLES_I( 1);
}

/////////
// DEC //
/////////

// dec.b #imm, reg8 [N=* Z=* V=*]
H8S_EXEC_INSTR( dec_xx_r8 )
{
	H8S_GET_DATA_Ix_R8_INC_DEC
	UINT8 val1 = H8S_GET_REG_8( reg_dst);
	UINT8 val3 = val1 - 1; // instruction only supports #1
	H8S_FLAG_XXX_NZ( val3, 8);
	H8S_SET_FLAG_V( NEG( val1, 8) && POS( val3, 8));
	H8S_SET_REG_8( reg_dst, val3);
	H8S_CYCLES_I( 1);
}

// dec.w #imm, reg16 [N=* Z=* V=*]
H8S_EXEC_INSTR( dec_xx_r16 )
{
	H8S_GET_DATA_Ix_R16_INC_DEC
	UINT16 val1 = H8S_GET_REG_16( reg_dst);
	UINT16 val3 = val1 - imm;
	H8S_FLAG_XXX_NZ( val3, 16);
	H8S_SET_FLAG_V( NEG( val1, 16) && POS( val3, 16));
	H8S_SET_REG_16( reg_dst, val3);
	H8S_CYCLES_I( 1);
}

// dec.l #imm, reg32 [N=* Z=* V=*]
H8S_EXEC_INSTR( dec_xx_r32 )
{
	H8S_GET_DATA_Ix_R32_INC_DEC
	UINT32 val1 = H8S_GET_REG_32( reg_dst);
	UINT32 val3 = val1 - imm;
	H8S_FLAG_XXX_NZ( val3, 32);
	H8S_SET_FLAG_V( NEG( val1, 32) && POS( val3, 32));
	H8S_SET_REG_32( reg_dst, val3);
	H8S_CYCLES_I( 1);
}

///////////
// MULXU //
///////////

// mulxu.b reg8, reg16 [-]
H8S_EXEC_INSTR( mulxu_r8_r16 )
{
	H8S_GET_DATA_R8_R16_1
	UINT16 val = (UINT16)((H8S_GET_REG_16( reg_dst) & 0xFF) * H8S_GET_REG_8( reg_src));
	H8S_SET_REG_16( reg_dst, val);
	H8S_CYCLES_IN( 1, 11);
}

// mulxu.w reg16, reg32 [-]
H8S_EXEC_INSTR( mulxu_r16_r32 )
{
	H8S_GET_DATA_R16_R32_1
	UINT32 val = (UINT32)((H8S_GET_REG_32( reg_dst) & 0xFFFF) * H8S_GET_REG_16( reg_src));
	H8S_SET_REG_32( reg_dst, val);
	H8S_CYCLES_IN( 1, 19);
}

///////////
// MULXS //
///////////

// mulxs.w reg8, reg16 [N=* Z=*]
H8S_EXEC_INSTR( mulxs_r8_r16 )
{
	H8S_GET_DATA_R8_R16_3
	UINT16 val = (UINT16)((char)(H8S_GET_REG_16( reg_dst) & 0xFF) * (char)H8S_GET_REG_8( reg_src));
	H8S_FLAG_XXX_NZ( val, 16);
	H8S_SET_REG_16( reg_dst, val);
	H8S_CYCLES_IN( 2, 11);
}

// mulxs.w reg16, reg32 [N=* Z=*]
H8S_EXEC_INSTR( mulxs_r16_r32 )
{
	H8S_GET_DATA_R16_R32_3
	UINT32 val = (UINT32)((short)(H8S_GET_REG_16( reg_dst) & 0xFFFF) * (short)H8S_GET_REG_16( reg_src));
	H8S_FLAG_XXX_NZ( val, 32);
	H8S_SET_REG_32( reg_dst, val);
	H8S_CYCLES_IN( 2, 19);
}

///////////
// DIVXU //
///////////

// divxu.b reg8, reg16 [N=* Z=*]
H8S_EXEC_INSTR( divxu_r8_r16 )
{
	H8S_GET_DATA_R8_R16_1
	UINT8 val1 = H8S_GET_REG_8( reg_src);
	// valid results are not assured if division by zero is attempted
	if (val1 != 0)
	{
		UINT16 val2 = H8S_GET_REG_16( reg_dst);
		UINT16 val3 = 0;
		H8S_FLAG_XXX_NZ( val1, 8);
		val3 |= (val2 % val1) << 8; // remainder
		val3 |= (val2 / val1) << 0; // quotient
		H8S_SET_REG_16( reg_dst, val3);
	}
	H8S_CYCLES_IN( 1, 11);
}

// divxu.w reg16, reg32 [N=* Z=*]
H8S_EXEC_INSTR( divxu_r16_r32 )
{
	H8S_GET_DATA_R16_R32_1
	UINT16 val1 = H8S_GET_REG_16( reg_src);
	// valid results are not assured if division by zero is attempted
	if (val1 != 0)
	{
		UINT32 val2 = H8S_GET_REG_32( reg_dst);
		UINT32 val3 = 0;
		H8S_FLAG_XXX_NZ( val1, 16);
		val3 |= (val2 % val1) << 16; // remainder
		val3 |= (val2 / val1) <<  0; // quotient
		H8S_SET_REG_32( reg_dst, val3);
	}
	H8S_CYCLES_IN( 1, 19);
}

///////////
// DIVXS //
///////////

// divxs.w reg16, reg32 [N=* Z=*]
H8S_EXEC_INSTR( divxs_r16_r32 )
{
	H8S_GET_DATA_R16_R32_3
	UINT16 val1 = H8S_GET_REG_16( reg_src);
	// valid results are not assured if division by zero is attempted
	if (val1 != 0)
	{
		UINT32 val2 = (int)H8S_GET_REG_32( reg_dst);
		UINT32 val3 = 0;
		H8S_SET_FLAG_Z( val1 == 0);
		val3 |= ((int)val2 % (short)val1) << 16; // remainder
		val3 |= ((int)val2 / (short)val1) <<  0; // quotient
		H8S_SET_FLAG_N( NEG( val3 & 0xFFFF, 16));
		H8S_SET_REG_32( reg_dst, val3);
	}
	H8S_CYCLES_IN( 2, 19);
}

/////////
// CMP //
/////////

// cmp.b #imm8, reg8 [H=* N=* Z=* V=* C=*]
H8S_EXEC_INSTR( cmp_i8_r8 )
{
	H8S_GET_DATA_I8_R8
	UINT8 val1 = H8S_GET_REG_8( reg_dst);
	UINT8 val3 = val1 - imm;
	H8S_FLAG_SUB_NZCV( val1, imm, val3, 8);
	H8S_CYCLES_I( 1);
}

// cmp.b reg8, reg8 [H=* N=* Z=* V=* C=*]
H8S_EXEC_INSTR( cmp_r8_r8 )
{
	H8S_GET_DATA_R8_R8
	UINT8 val1 = H8S_GET_REG_8( reg_dst);
	UINT8 val2 = H8S_GET_REG_8( reg_src);
	UINT8 val3 = val1 - val2;
	H8S_FLAG_SUB_NZCV( val1, val2, val3, 8);
	H8S_CYCLES_I( 1);
}

// cmp.w #imm16, reg16 [H=* N=* Z=* V=* C=*]
H8S_EXEC_INSTR( cmp_i16_r16 )
{
	H8S_GET_DATA_I16_R16
	UINT16 val1 = H8S_GET_REG_16( reg_dst);
	UINT16 val3 = val1 - imm;
	H8S_FLAG_SUB_NZCV( val1, imm, val3, 16);
	H8S_CYCLES_I( 2);
}

// cmp.w reg16, reg16 [H=* N=* Z=* V=* C=*]
H8S_EXEC_INSTR( cmp_r16_r16 )
{
	H8S_GET_DATA_R16_R16
	UINT16 val1 = H8S_GET_REG_16( reg_dst);
	UINT16 val2 = H8S_GET_REG_16( reg_src);
	UINT16 val3 = val1 - val2;
	H8S_FLAG_SUB_NZCV( val1, val2, val3, 16);
	H8S_CYCLES_I( 1);
}

// cmp.l #imm32, reg32 [H=* N=* Z=* V=* C=*]
H8S_EXEC_INSTR( cmp_i32_r32 )
{
	H8S_GET_DATA_I32_R32
	UINT32 val1 = H8S_GET_REG_32( reg_dst);
	UINT32 val3 = val1 - imm;
	H8S_FLAG_SUB_NZCV( val1, imm, val3, 32);
	H8S_CYCLES_I( 3);
}

// cmp.l reg32, reg32 [H=* N=* Z=* V=* C=*]
H8S_EXEC_INSTR( cmp_r32_r32 )
{
	H8S_GET_DATA_R32_R32_1
	UINT32 val1 = H8S_GET_REG_32( reg_dst);
	UINT32 val2 = H8S_GET_REG_32( reg_src);
	UINT32 val3 = val1 - val2;
	H8S_FLAG_SUB_NZCV( val1, val2, val3, 32);
	H8S_CYCLES_I( 1);
}

/////////
// NEG //
/////////

// neg.b reg8 [H=* N=* Z=* V=* C=*]
H8S_EXEC_INSTR( neg_r8 )
{
	H8S_GET_DATA_R8
	UINT8 val2 = H8S_GET_REG_8( reg_src);
	UINT8 val3 = 0 - val2;
	H8S_FLAG_SUB_NZCV( 0, val2, val3, 8);
	H8S_SET_REG_8( reg_src, val3);
	H8S_CYCLES_I( 1);
}

// neg.w reg16 [H=* N=* Z=* V=* C=*]
H8S_EXEC_INSTR( neg_r16 )
{
	H8S_GET_DATA_R16
	UINT16 val2 = H8S_GET_REG_16( reg_src);
	UINT16 val3 = 0 - val2;
	H8S_FLAG_SUB_NZCV( 0, val2, val3, 16);
	H8S_SET_REG_16( reg_src, val3);
	H8S_CYCLES_I( 1);
}

// neg.l reg32 [H=* N=* Z=* V=* C=*]
H8S_EXEC_INSTR( neg_r32 )
{
	H8S_GET_DATA_R32
	UINT32 val2 = H8S_GET_REG_32( reg_src);
	UINT32 val3 = 0 - val2;
	H8S_FLAG_SUB_NZCV( 0, val2, val3, 32);
	H8S_SET_REG_32( reg_src, val3);
	H8S_CYCLES_I( 1);
}

//////////
// EXTU //
//////////

// extu.w reg16 [N=0 Z=* V=0]
H8S_EXEC_INSTR( extu_r16 )
{
	H8S_GET_DATA_R16
	UINT16 val = H8S_GET_REG_16( reg_src) & 0xFF;
	H8S_SET_FLAG_Z( val == 0);
	H8S_SET_FLAG_N( 0);
	H8S_SET_FLAG_V( 0);
	H8S_SET_REG_16( reg_src, val);
	H8S_CYCLES_I( 1);
}

// extu.l reg32 [N=0 Z=* V=0]
H8S_EXEC_INSTR( extu_r32 )
{
	H8S_GET_DATA_R32
	UINT32 val = H8S_GET_REG_32( reg_src) & 0xFFFF;
	H8S_SET_FLAG_Z( val == 0);
	H8S_SET_FLAG_N( 0);
	H8S_SET_FLAG_V( 0);
	H8S_SET_REG_32( reg_src, val);
	H8S_CYCLES_I( 1);
}

//////////
// EXTS //
//////////

// exts.w reg16 [N=* Z=* V=0]
H8S_EXEC_INSTR( exts_r16 )
{
	H8S_GET_DATA_R16
	UINT16 val = H8S_GET_REG_16( reg_src) & 0xFF;
	if (val & 0x80) val = val | 0xFF00;
	H8S_FLAG_XXX_NZ( val, 16);
	H8S_SET_FLAG_V( 0);
	H8S_SET_REG_16( reg_src, val);
	H8S_CYCLES_I( 1);
}

// exts.l reg32 [N=* Z=* V=0]
H8S_EXEC_INSTR( exts_r32 )
{
	H8S_GET_DATA_R32
	UINT32 val = H8S_GET_REG_32( reg_src) & 0xFFFF;
	if (val & 0x8000) val = val | 0xFFFF0000;
	H8S_FLAG_XXX_NZ( val, 32);
	H8S_SET_FLAG_V( 0);
	H8S_SET_REG_32( reg_src, val);
	H8S_CYCLES_I( 1);
}

/////////
// AND //
/////////

// and.b #imm8, reg8 [N=* Z=* V=0]
H8S_EXEC_INSTR( and_i8_r8 )
{
	H8S_GET_DATA_I8_R8
	UINT8 val = H8S_GET_REG_8( reg_dst) & imm;
	H8S_FLAG_XXX_NZ( val, 8);
	H8S_SET_FLAG_V( 0);
	H8S_SET_REG_8( reg_dst, val);
	H8S_CYCLES_I( 1);
}

// and.b reg8, reg8 [N=* Z=* V=0]
H8S_EXEC_INSTR( and_r8_r8 )
{
	H8S_GET_DATA_R8_R8
	UINT8 val = H8S_GET_REG_8( reg_dst) & H8S_GET_REG_8( reg_src);
	H8S_FLAG_XXX_NZ( val, 8);
	H8S_SET_FLAG_V( 0);
	H8S_SET_REG_8( reg_dst, val);
	H8S_CYCLES_I( 1);
}

// and.w #imm16, reg16 [N=* Z=* V=0]
H8S_EXEC_INSTR( and_i16_r16 )
{
	H8S_GET_DATA_I16_R16
	UINT16 val = H8S_GET_REG_16( reg_dst) & imm;
	H8S_FLAG_XXX_NZ( val, 16);
	H8S_SET_FLAG_V( 0);
	H8S_SET_REG_16( reg_dst, val);
	H8S_CYCLES_I( 2);
}

// and.w reg16, reg16 [N=* Z=* V=0]
H8S_EXEC_INSTR( and_r16_r16 )
{
	H8S_GET_DATA_R16_R16
	UINT16 val = H8S_GET_REG_16( reg_dst) & H8S_GET_REG_16( reg_src);
	H8S_FLAG_XXX_NZ( val, 16);
	H8S_SET_FLAG_V( 0);
	H8S_SET_REG_16( reg_dst, val);
	H8S_CYCLES_I( 1);
}

// and.l #imm32, reg32 [N=* Z=* V=0]
H8S_EXEC_INSTR( and_i32_r32 )
{
	H8S_GET_DATA_I32_R32
	UINT32 val = H8S_GET_REG_32( reg_dst) & imm;
	H8S_FLAG_XXX_NZ( val, 32);
	H8S_SET_FLAG_V( 0);
	H8S_SET_REG_32( reg_dst, val);
	H8S_CYCLES_I( 3);
}

// and.l reg32, reg32 [N=* Z=* V=0]
H8S_EXEC_INSTR( and_r32_r32 )
{
	H8S_GET_DATA_R32_R32_3
	UINT32 val = H8S_GET_REG_32( reg_dst) & H8S_GET_REG_32( reg_src);
	H8S_FLAG_XXX_NZ( val, 32);
	H8S_SET_FLAG_V( 0);
	H8S_SET_REG_32( reg_dst, val);
	H8S_CYCLES_I( 2);
}

////////
// OR //
////////

// or.b #imm8, reg8 [N=* Z=* V=0]
H8S_EXEC_INSTR( or_i8_r8 )
{
	H8S_GET_DATA_I8_R8
	UINT8 val = H8S_GET_REG_8( reg_dst) | imm;
	H8S_FLAG_XXX_NZ( val, 8);
	H8S_SET_FLAG_V( 0);
	H8S_SET_REG_8( reg_dst, val);
	H8S_CYCLES_I( 1);
}

// or.b reg8, reg8 [N=* Z=* V=0]
H8S_EXEC_INSTR( or_r8_r8 )
{
	H8S_GET_DATA_R8_R8
	UINT8 val = H8S_GET_REG_8( reg_dst) | H8S_GET_REG_8( reg_src);
	H8S_FLAG_XXX_NZ( val, 8);
	H8S_SET_FLAG_V( 0);
	H8S_SET_REG_8( reg_dst, val);
	H8S_CYCLES_I( 1);
}

// or.w reg16, reg16 [N=* Z=* V=0]
H8S_EXEC_INSTR( or_r16_r16 )
{
	H8S_GET_DATA_R16_R16
	UINT16 val = H8S_GET_REG_16( reg_dst) | H8S_GET_REG_16( reg_src);
	H8S_FLAG_XXX_NZ( val, 16);
	H8S_SET_FLAG_V( 0);
	H8S_SET_REG_16( reg_dst, val);
	H8S_CYCLES_I( 1);
}

// or.l #imm32, reg32 [N=* Z=* V=0]
H8S_EXEC_INSTR( or_i32_r32 )
{
	H8S_GET_DATA_I32_R32
	UINT32 val = H8S_GET_REG_32( reg_dst) | imm;
	H8S_FLAG_XXX_NZ( val, 32);
	H8S_SET_FLAG_V( 0);
	H8S_SET_REG_32( reg_dst, val);
	H8S_CYCLES_I( 3);
}

// or.l reg32, reg32 [N=* Z=* V=0]
H8S_EXEC_INSTR( or_r32_r32 )
{
	H8S_GET_DATA_R32_R32_3
	UINT32 val = H8S_GET_REG_32( reg_dst) | H8S_GET_REG_32( reg_src);
	H8S_FLAG_XXX_NZ( val, 32);
	H8S_SET_FLAG_V( 0);
	H8S_SET_REG_32( reg_dst, val);
	H8S_CYCLES_I( 2);
}

/////////
// XOR //
/////////

// xor.b #imm8, reg8 [N=* Z=* V=0]
H8S_EXEC_INSTR( xor_i8_r8 )
{
	H8S_GET_DATA_I8_R8
	UINT8 val = H8S_GET_REG_8( reg_dst) ^ imm;
	H8S_FLAG_XXX_NZ( val, 8);
	H8S_SET_FLAG_V( 0);
	H8S_SET_REG_8( reg_dst, val);
	H8S_CYCLES_I( 1);
}

// xor.b reg8, reg8 [N=* Z=* V=0]
H8S_EXEC_INSTR( xor_r8_r8 )
{
	H8S_GET_DATA_R8_R8
	UINT8 val = H8S_GET_REG_8( reg_dst) ^ H8S_GET_REG_8( reg_src);
	H8S_FLAG_XXX_NZ( val, 8);
	H8S_SET_FLAG_V( 0);
	H8S_SET_REG_8( reg_dst, val);
	H8S_CYCLES_I( 1);
}

// xor.w #imm16, reg16 [N=* Z=* V=0]
H8S_EXEC_INSTR( xor_i16_r16 )
{
	H8S_GET_DATA_I16_R16
	UINT16 val = H8S_GET_REG_16( reg_dst) ^ imm;
	H8S_FLAG_XXX_NZ( val, 16);
	H8S_SET_FLAG_V( 0);
	H8S_SET_REG_16( reg_dst, val);
	H8S_CYCLES_I( 2);
}

// xor.w reg16, reg16 [N=* Z=* V=0]
H8S_EXEC_INSTR( xor_r16_r16 )
{
	H8S_GET_DATA_R16_R16
	UINT16 val = H8S_GET_REG_16( reg_dst) ^ H8S_GET_REG_16( reg_src);
	H8S_FLAG_XXX_NZ( val, 16);
	H8S_SET_FLAG_V( 0);
	H8S_SET_REG_16( reg_dst, val);
	H8S_CYCLES_I( 1);
}

// xor.l #imm32, reg32 [N=* Z=* V=0]
H8S_EXEC_INSTR( xor_i32_r32 )
{
	H8S_GET_DATA_I32_R32
	UINT32 val = H8S_GET_REG_32( reg_dst) ^ imm;
	H8S_FLAG_XXX_NZ( val, 32);
	H8S_SET_FLAG_V( 0);
	H8S_SET_REG_32( reg_dst, val);
	H8S_CYCLES_I( 3);
}

// xor.l reg32, reg32 [N=* Z=* V=0]
H8S_EXEC_INSTR( xor_r32_r32 )
{
	H8S_GET_DATA_R32_R32_3
	UINT32 val = H8S_GET_REG_32( reg_dst) ^ H8S_GET_REG_32( reg_src);
	H8S_FLAG_XXX_NZ( val, 32);
	H8S_SET_FLAG_V( 0);
	H8S_SET_REG_32( reg_dst, val);
	H8S_CYCLES_I( 2);
}

/////////
// NOT //
/////////

// not.b reg8 [N=* Z=* V=0]
H8S_EXEC_INSTR( not_r8 )
{
	H8S_GET_DATA_R8
	UINT8 val = H8S_GET_REG_8( reg_src) ^ 0xFF;
	H8S_FLAG_XXX_NZ( val, 8);
	H8S_SET_FLAG_V( 0);
	H8S_SET_REG_8( reg_src, val);
	H8S_CYCLES_I( 1);
}

// not.w reg16 [N=* Z=* V=0]
H8S_EXEC_INSTR( not_r16 )
{
	H8S_GET_DATA_R16
	UINT16 val = H8S_GET_REG_16( reg_src) ^ 0xFFFF;
	H8S_FLAG_XXX_NZ( val, 16);
	H8S_SET_FLAG_V( 0);
	H8S_SET_REG_16( reg_src, val);
	H8S_CYCLES_I( 1);
}

// not.l reg32 [N=* Z=* V=0]
H8S_EXEC_INSTR( not_r32 )
{
	H8S_GET_DATA_R32
	UINT32 val = H8S_GET_REG_32( reg_src) ^ 0xFFFFFFFF;
	H8S_FLAG_XXX_NZ( val, 32);
	H8S_SET_FLAG_V( 0);
	H8S_SET_REG_32( reg_src, val);
	H8S_CYCLES_I( 1);
}

//////////
// SHAL //
//////////

// flag v calculated correctly?
#define SHAL_XX( val, imm, x) \
	H8S_SET_FLAG_V( (((val >> (x - 1)) & 1) != ((val >> (x - 1 - imm)) & 1))); \
	H8S_SET_FLAG_C( (val & ((0x80 << (x - 8)) >> (imm - 1)))); \
	val = val << imm; \
	H8S_FLAG_XXX_NZ( val, x);

// shal.b #imm, reg8 [N=* Z=* V=* C=*]
H8S_EXEC_INSTR( shal_xx_r8 )
{
	H8S_GET_DATA_Ix_R8_SH
	UINT8 val = H8S_GET_REG_8( reg_dst);
	SHAL_XX( val, imm, 8);
	H8S_SET_REG_8( reg_dst, val);
	H8S_CYCLES_I( 1);
}

// shal.w #imm, reg16 [N=* Z=* V=* C=*]
H8S_EXEC_INSTR( shal_xx_r16 )
{
	H8S_GET_DATA_Ix_R16_SH
	UINT16 val = H8S_GET_REG_16( reg_dst);
	SHAL_XX( val, imm, 16);
	H8S_SET_REG_16( reg_dst, val);
	H8S_CYCLES_I( 1);
}

// shal.l #imm, reg32 [N=* Z=* V=* C=*]
H8S_EXEC_INSTR( shal_xx_r32 )
{
	H8S_GET_DATA_Ix_R32_SH
	UINT32 val = H8S_GET_REG_32( reg_dst);
	SHAL_XX( val, imm, 32);
	H8S_SET_REG_32( reg_dst, val);
	H8S_CYCLES_I( 1);
}

//////////
// SHAR //
//////////

// this code supports shift by 1 and 2
#define SHAR_XX( val, imm, x) \
	H8S_SET_FLAG_C( (val & imm)); \
	val = (val >> imm) | (val & (0x80 << (x - 8))); \
	if (imm == 2) val = val | ((val & (0x80 << (x - 8))) >> 1); \
	H8S_FLAG_XXX_NZ( val, x); \
	H8S_SET_FLAG_V( 0);

// shar.w #imm, reg16 [N=* Z=* V=0 C=*]
H8S_EXEC_INSTR( shar_xx_r16 )
{
	H8S_GET_DATA_Ix_R16_SH
	UINT16 val = H8S_GET_REG_16( reg_dst);
	SHAR_XX( val, imm, 16);
	H8S_SET_REG_16( reg_dst, val);
	H8S_CYCLES_I( 1);
}

// shar.l #imm, reg32 [N=* Z=* V=0 C=*]
H8S_EXEC_INSTR( shar_xx_r32 )
{
	H8S_GET_DATA_Ix_R32_SH
	UINT32 val = H8S_GET_REG_32( reg_dst);
	SHAR_XX( val, imm, 32);
	H8S_SET_REG_32( reg_dst, val);
	H8S_CYCLES_I( 1);
}

//////////
// SHLL //
//////////

// this code supports shift by 1, 2, 3, etc.
#define SHLL_XX( val, imm, x) \
	H8S_SET_FLAG_C( (val & ((0x80 << (x - 8)) >> (imm - 1)))); \
	val = val << imm; \
	H8S_FLAG_XXX_NZ( val, x); \
	H8S_SET_FLAG_V( 0);

// shll.b #imm, reg8 [N=* Z=* V=0 C=*]
H8S_EXEC_INSTR( shll_xx_r8 )
{
	H8S_GET_DATA_Ix_R8_SH
	UINT8 val = H8S_GET_REG_8( reg_dst);
	SHLL_XX( val, imm, 8);
	H8S_SET_REG_8( reg_dst, val);
	H8S_CYCLES_I( 1);
}

// shll.w #imm, reg16 [N=* Z=* V=0 C=*]
H8S_EXEC_INSTR( shll_xx_r16 )
{
	H8S_GET_DATA_Ix_R16_SH
	UINT16 val = H8S_GET_REG_16( reg_dst);
	SHLL_XX( val, imm, 16);
	H8S_SET_REG_16( reg_dst, val);
	H8S_CYCLES_I( 1);
}

// shll.l #imm, reg32 [N=* Z=* V=0 C=*]
H8S_EXEC_INSTR( shll_xx_r32 )
{
	H8S_GET_DATA_Ix_R32_SH
	UINT32 val = H8S_GET_REG_32( reg_dst);
	SHLL_XX( val, imm, 32);
	H8S_SET_REG_32( reg_dst, val);
	H8S_CYCLES_I( 1);
}

//////////
// SHLR //
//////////

// this code supports shift by 1, 2, 3, etc.
#define SHLR_XX( val, imm, x ) \
	H8S_SET_FLAG_C( (val & (1 << (imm - 1)))); \
	val = val >> imm; \
	H8S_SET_FLAG_Z( val == 0 ); \
	H8S_SET_FLAG_N( 0); \
	H8S_SET_FLAG_V( 0);

// shlr.b #imm, reg8 [N=0 Z=* V=0 C=*]
H8S_EXEC_INSTR( shlr_xx_r8 )
{
	H8S_GET_DATA_Ix_R8_SH
	UINT8 val = H8S_GET_REG_8( reg_dst);
	SHLR_XX( val, imm, 8);
	H8S_SET_REG_8( reg_dst, val);
	H8S_CYCLES_I( 1);
}

// shlr.w #imm, reg16 [N=0 Z=* V=0 C=*]
H8S_EXEC_INSTR( shlr_xx_r16 )
{
	H8S_GET_DATA_Ix_R16_SH
	UINT16 val = H8S_GET_REG_16( reg_dst);
	SHLR_XX( val, imm, 16);
	H8S_SET_REG_16( reg_dst, val);
	H8S_CYCLES_I( 1);
}

// shlr.l #imm, reg32 [N=0 Z=* V=0 C=*]
H8S_EXEC_INSTR( shlr_xx_r32 )
{
	H8S_GET_DATA_Ix_R32_SH
	UINT32 val = H8S_GET_REG_32( reg_dst);
	SHLR_XX( val, imm, 32);
	H8S_SET_REG_32( reg_dst, val);
	H8S_CYCLES_I( 1);
}

///////////
// ROTXL //
///////////

// this code supports shift by 1, 2, 3, etc.
#define ROTXL_XX( reg, imm, x) \
	if (H8S_GET_FLAG_C) flagc = 1; else flagc = 0; \
	H8S_SET_FLAG_C( (val & ((0x80 << (x - 8)) >> (imm - 1)))); \
	if (imm == 1) \
		val = (val << imm) | flagc; \
	else \
		val = (val << imm) | (val >> (x - (imm - 1))) | (flagc << (imm - 1)); \
	H8S_FLAG_XXX_NZ( val, x); \
	H8S_SET_FLAG_V( 0);

// rotxl.b #imm, reg8 [N=* Z=* V=0 C=*]
H8S_EXEC_INSTR( rotxl_xx_r8 )
{
	H8S_GET_DATA_Ix_R8_SH
	UINT8 val = H8S_GET_REG_8( reg_dst);
	UINT8 flagc;
	ROTXL_XX( val, imm, 8);
	H8S_SET_REG_8( reg_dst, val);
	H8S_CYCLES_I( 1);
}

// rotxl.l #imm, reg32 [N=* Z=* V=0 C=*]
H8S_EXEC_INSTR( rotxl_xx_r32 )
{
	H8S_GET_DATA_Ix_R32_SH
	UINT32 val = H8S_GET_REG_32( reg_dst);
	UINT8 flagc;
	ROTXL_XX( val, imm, 32);
	H8S_SET_REG_32( reg_dst, val);
	H8S_CYCLES_I( 1);
}

//////////
// ROTL //
//////////

// this code supports shift by 1, 2, 3, etc.
#define ROTL_XX( reg, imm, x) \
	val = (val << imm) | (val >> (x - imm)); \
	H8S_SET_FLAG_C( val & 1); \
	H8S_FLAG_XXX_NZ( val, x); \
	H8S_SET_FLAG_V( 0);

// rotl.b #imm, reg8 [N=* Z=* V=0 C=*]
H8S_EXEC_INSTR( rotl_xx_r8 )
{
	H8S_GET_DATA_Ix_R8_SH
	UINT8 val = H8S_GET_REG_8( reg_dst);
	ROTL_XX( val, imm, 8);
	H8S_SET_REG_8( reg_dst, val);
	H8S_CYCLES_I( 1);
}

// rotl.w #imm, reg16 [N=* Z=* V=0 C=*]
H8S_EXEC_INSTR( rotl_xx_r16 )
{
	H8S_GET_DATA_Ix_R16_SH
	UINT16 val = H8S_GET_REG_16( reg_dst);
	ROTL_XX( val, imm, 16);
	H8S_SET_REG_16( reg_dst, val);
	H8S_CYCLES_I( 1);
}

// rotl.l #imm, reg32 [N=* Z=* V=0 C=*]
H8S_EXEC_INSTR( rotl_xx_r32 )
{
	H8S_GET_DATA_Ix_R32_SH
	UINT32 val = H8S_GET_REG_32( reg_dst);
	ROTL_XX( val, imm, 32);
	H8S_SET_REG_32( reg_dst, val);
	H8S_CYCLES_I( 1);
}

//////////
// ROTR //
//////////

#define ROTR_XX( reg, imm, x) \
	val = (val >> imm) | (val << (x - imm)); \
	H8S_SET_FLAG_C( (val >> (x - 1))); \
	H8S_FLAG_XXX_NZ( val, x); \
	H8S_SET_FLAG_V( 0);

// rotr.b #imm, reg8 [N=* Z=* V=0 C=*]
H8S_EXEC_INSTR( rotr_xx_r8 )
{
	H8S_GET_DATA_Ix_R8_SH
	UINT8 val = H8S_GET_REG_8( reg_dst);
	ROTR_XX( val, imm, 8);
	H8S_SET_REG_8( reg_dst, val);
	H8S_CYCLES_I( 1);
}

// rotr.l #imm, reg32 [N=* Z=* V=0 C=*]
H8S_EXEC_INSTR( rotr_xx_r32 )
{
	H8S_GET_DATA_Ix_R32_SH
	UINT32 val = H8S_GET_REG_32( reg_dst);
	ROTR_XX( val, imm, 32);
	H8S_SET_REG_32( reg_dst, val);
	H8S_CYCLES_I( 1);
}

//////////
// BSET //
//////////

// bset #imm, @reg32 [-]
H8S_EXEC_INSTR( bset_ix_ar32 )
{
	H8S_GET_DATA_Ix_AR32
	UINT8 val = H8S_GET_MEM_8( H8S_GET_REG_32( reg_dst)) | (1 << imm);
	H8S_SET_MEM_8( H8S_GET_REG_32( reg_dst), val);
	H8S_CYCLES_IL( 2, 2);
}

// bset #imm, @imm:8 [-]
H8S_EXEC_INSTR( bset_ix_ai8 )
{
	H8S_GET_DATA_Ix_AI8
	UINT8 val = H8S_GET_MEM_8( imm_r) | (1 << imm_l);
	H8S_SET_MEM_8( imm_r, val);
	H8S_CYCLES_IL( 2, 2);
}

// bset #imm, @imm:32 [-]
H8S_EXEC_INSTR( bset_ix_ai32 )
{
	H8S_GET_DATA_Ix_AI32
	UINT8 val = H8S_GET_MEM_8( imm_r) | (1 << imm_l);
	H8S_SET_MEM_8( imm_r, val);
	H8S_CYCLES_IL( 4, 2);
}

// bset reg8, @imm:32 [-]
H8S_EXEC_INSTR( bset_r8_ai32 )
{
	H8S_GET_DATA_R8_AI32_BCLR
	UINT8 val = H8S_GET_MEM_8( imm) | (1 << H8S_GET_REG_8( reg_src));
	H8S_SET_MEM_8( imm, val);
	H8S_CYCLES_IL( 4, 2);
}

//////////
// BCLR //
//////////

// bclr #imm, @reg32 [-]
H8S_EXEC_INSTR( bclr_ix_ar32 )
{
	H8S_GET_DATA_Ix_AR32
	UINT8 val = H8S_GET_MEM_8( H8S_GET_REG_32( reg_dst)) & ((1 << imm) ^ 0xFF);
	H8S_SET_MEM_8( H8S_GET_REG_32( reg_dst), val);
	H8S_CYCLES_IL( 2, 2);
}

// bclr #imm, @imm:8 [-]
H8S_EXEC_INSTR( bclr_ix_ai8 )
{
	H8S_GET_DATA_Ix_AI8
	UINT8 val = H8S_GET_MEM_8( imm_r) & ((1 << imm_l) ^ 0xFF);
	H8S_SET_MEM_8( imm_r, val);
	H8S_CYCLES_IL( 2, 2);
}

// bclr #imm, @imm:32 [-]
H8S_EXEC_INSTR( bclr_ix_ai32 )
{
	H8S_GET_DATA_Ix_AI32
	UINT8 val = H8S_GET_MEM_8( imm_r) & ((1 << imm_l) ^ 0xFF);
	H8S_SET_MEM_8( imm_r, val);
	H8S_CYCLES_IL( 4, 2);
}

// bclr reg8, @imm:32 [-]
H8S_EXEC_INSTR( bclr_r8_ai32 )
{
	H8S_GET_DATA_R8_AI32_BCLR
	UINT8 val = H8S_GET_MEM_8( imm) & ((1 << H8S_GET_REG_8( reg_src)) ^ 0xFF);
	H8S_SET_MEM_8( imm, val);
	H8S_CYCLES_IL( 4, 2);
}

//////////
// BNOT //
//////////

// bnot #imm, @imm:8 [-]
H8S_EXEC_INSTR( bnot_ix_ai8 )
{
	H8S_GET_DATA_Ix_AI8
	UINT8 val = H8S_GET_MEM_8( imm_r) ^ (1 << imm_l);
	H8S_SET_MEM_8( imm_r, val);
	H8S_CYCLES_IL( 2, 2);
}

// bnot #imm, @imm:32 [-]
H8S_EXEC_INSTR( bnot_ix_ai32 )
{
	H8S_GET_DATA_Ix_AI32
	UINT8 val = H8S_GET_MEM_8( imm_r) ^ (1 << imm_l);
	H8S_SET_MEM_8( imm_r, val);
	H8S_CYCLES_IL( 4, 2);
}

//////////
// BTST //
//////////

// btst #imm, reg8 [Z=*]
H8S_EXEC_INSTR( btst_ix_r8 )
{
	H8S_GET_DATA_Ix_R8
	UINT8 val = H8S_GET_REG_8( reg_dst);
	H8S_SET_FLAG_Z( !(val & (1 << imm)));
	H8S_CYCLES_I( 1);
}

// btst #imm, @reg32 [Z=*]
H8S_EXEC_INSTR( btst_ix_ar32 )
{
	H8S_GET_DATA_Ix_AR32
	UINT8 val = H8S_GET_MEM_8( H8S_GET_REG_32( reg_dst));
	H8S_SET_FLAG_Z( !(val & (1 << imm)));
	H8S_CYCLES_IL( 2, 1);
}

// btst #imm, @imm:8 [Z=*]
H8S_EXEC_INSTR( btst_ix_ai8 )
{
	H8S_GET_DATA_Ix_AI8
	UINT8 val = H8S_GET_MEM_8(imm_r);
	H8S_SET_FLAG_Z( !(val & (1 << imm_l)));
	H8S_CYCLES_IL( 2, 1);
}

// btst #imm, @imm:32 [Z=*]
H8S_EXEC_INSTR( btst_ix_ai32 )
{
	H8S_GET_DATA_Ix_AI32
	UINT8 val = H8S_GET_MEM_8( imm_r);
	H8S_SET_FLAG_Z( !(val & (1 << imm_l)));
	H8S_CYCLES_IL( 4, 1);
}

/////////
// BLD //
/////////

// bld #imm, reg8 [C=*]
H8S_EXEC_INSTR( bld_ix_r8 )
{
	H8S_GET_DATA_Ix_R8
	UINT8 val = H8S_GET_REG_8( reg_dst);
	H8S_SET_FLAG_C( (val & (1 << imm)));
	H8S_CYCLES_I( 1);
}

/////////
// BST //
/////////

// bst #imm, reg8 [-]
H8S_EXEC_INSTR( bst_ix_r8 )
{
	H8S_GET_DATA_Ix_R8
	UINT8 val = H8S_GET_REG_8( reg_dst);
	if (H8S_GET_FLAG_C) val = val | (1 << imm); else val = val & (~(1 << imm));
	H8S_SET_REG_8( reg_dst, val);
	H8S_CYCLES_I( 1);
}

/////////
// BXX //
/////////

// bra imm8(=off):8 [-]
H8S_EXEC_INSTR( bra_o8 )
{
	H8S_GET_DATA_O8
	if (H8S_EVAL_BRA) H8S_SET_PC( H8S_GET_PC() + (char)imm);
	H8S_CYCLES_I( 2);
}

// brn imm8(=off):8 [-]
H8S_EXEC_INSTR( brn_o8 )
{
	H8S_GET_DATA_O8
	if (H8S_EVAL_BRN) H8S_SET_PC( H8S_GET_PC() + (char)imm);
	H8S_CYCLES_I( 2);
}

// bhi imm8(=off):8 [-]
H8S_EXEC_INSTR( bhi_o8 )
{
	H8S_GET_DATA_O8
	if (H8S_EVAL_BHI) H8S_SET_PC( H8S_GET_PC() + (char)imm);
	H8S_CYCLES_I( 2);
}

// bls imm8(=off):8 [-]
H8S_EXEC_INSTR( bls_o8 )
{
	H8S_GET_DATA_O8
	if (H8S_EVAL_BLS) H8S_SET_PC( H8S_GET_PC() + (char)imm);
	H8S_CYCLES_I( 2);
}

// bcc imm8(=off):8 [-]
H8S_EXEC_INSTR( bcc_o8 )
{
	H8S_GET_DATA_O8
	if (H8S_EVAL_BCC) H8S_SET_PC( H8S_GET_PC() + (char)imm);
	H8S_CYCLES_I( 2);
}

// bcs imm8(=off):8 [-]
H8S_EXEC_INSTR( bcs_o8 )
{
	H8S_GET_DATA_O8
	if (H8S_EVAL_BCS) H8S_SET_PC( H8S_GET_PC() + (char)imm);
	H8S_CYCLES_I( 2);
}

// bne imm8(=off):8 [-]
H8S_EXEC_INSTR( bne_o8 )
{
	H8S_GET_DATA_O8
	if (H8S_EVAL_BNE) H8S_SET_PC( H8S_GET_PC() + (char)imm);
	H8S_CYCLES_I( 2);
}

// beq imm8(=off):8 [-]
H8S_EXEC_INSTR( beq_o8 )
{
	H8S_GET_DATA_O8
	if (H8S_EVAL_BEQ) H8S_SET_PC( H8S_GET_PC() + (char)imm);
	H8S_CYCLES_I( 2);
}

// bpl imm8(=off):8 [-]
H8S_EXEC_INSTR( bpl_o8 )
{
	H8S_GET_DATA_O8
	if (H8S_EVAL_BPL) H8S_SET_PC( H8S_GET_PC() + (char)imm);
	H8S_CYCLES_I( 2);
}

// bmi imm8(=off):8 [-]
H8S_EXEC_INSTR( bmi_o8 )
{
	H8S_GET_DATA_O8
	if (H8S_EVAL_BMI) H8S_SET_PC( H8S_GET_PC() + (char)imm);
	H8S_CYCLES_I( 2);
}

// bge imm8(=off):8 [-]
H8S_EXEC_INSTR( bge_o8 )
{
	H8S_GET_DATA_O8
	if (H8S_EVAL_BGE) H8S_SET_PC( H8S_GET_PC() + (char)imm);
	H8S_CYCLES_I( 2);
}

// blt imm8(=off):8 [-]
H8S_EXEC_INSTR( blt_o8 )
{
	H8S_GET_DATA_O8
	if (H8S_EVAL_BLT) H8S_SET_PC( H8S_GET_PC() + (char)imm);
	H8S_CYCLES_I( 2);
}

// bgt imm8(=off):8 [-]
H8S_EXEC_INSTR( bgt_o8 )
{
	H8S_GET_DATA_O8
	if (H8S_EVAL_BGT) H8S_SET_PC( H8S_GET_PC() + (char)imm);
	H8S_CYCLES_I( 2);
}

// ble imm8(=off):8 [-]
H8S_EXEC_INSTR( ble_o8 )
{
	H8S_GET_DATA_O8
	if (H8S_EVAL_BLE) H8S_SET_PC( H8S_GET_PC() + (char)imm);
	H8S_CYCLES_I( 2);
}

// bra imm16(=off):16 [-]
H8S_EXEC_INSTR( bra_o16 )
{
	H8S_GET_DATA_O16
	if (H8S_EVAL_BRA) H8S_SET_PC( H8S_GET_PC() + (short)imm);
	H8S_CYCLES_IN( 2, 1);
}

// brn imm16(=off):16 [-]
H8S_EXEC_INSTR( brn_o16 )
{
	H8S_GET_DATA_O16
	if (H8S_EVAL_BRN) H8S_SET_PC( H8S_GET_PC() + (short)imm);
	H8S_CYCLES_IN( 2, 1);
}

// bhi imm16(=off):16 [-]
H8S_EXEC_INSTR( bhi_o16 )
{
	H8S_GET_DATA_O16
	if (H8S_EVAL_BHI) H8S_SET_PC( H8S_GET_PC() + (short)imm);
	H8S_CYCLES_IN( 2, 1);
}

// bls imm16(=off):16 [-]
H8S_EXEC_INSTR( bls_o16 )
{
	H8S_GET_DATA_O16
	if (H8S_EVAL_BLS) H8S_SET_PC( H8S_GET_PC() + (short)imm);
	H8S_CYCLES_IN( 2, 1);
}

// bcc imm16(=off):16 [-]
H8S_EXEC_INSTR( bcc_o16 )
{
	H8S_GET_DATA_O16
	if (H8S_EVAL_BCC) H8S_SET_PC( H8S_GET_PC() + (short)imm);
	H8S_CYCLES_IN( 2, 1);
}

// bcs imm16(=off):16 [-]
H8S_EXEC_INSTR( bcs_o16 )
{
	H8S_GET_DATA_O16
	if (H8S_EVAL_BCS) H8S_SET_PC( H8S_GET_PC() + (short)imm);
	H8S_CYCLES_IN( 2, 1);
}

// bne imm16(=off):16 [-]
H8S_EXEC_INSTR( bne_o16 )
{
	H8S_GET_DATA_O16
	if (H8S_EVAL_BNE) H8S_SET_PC( H8S_GET_PC() + (short)imm);
	H8S_CYCLES_IN( 2, 1);
}

// beq imm16(=off):16 [-]
H8S_EXEC_INSTR( beq_o16 )
{
	H8S_GET_DATA_O16
	if (H8S_EVAL_BEQ) H8S_SET_PC( H8S_GET_PC() + (short)imm);
	H8S_CYCLES_IN( 2, 1);
}

// bpl imm8(=off):16 [-]
H8S_EXEC_INSTR( bpl_o16 )
{
	H8S_GET_DATA_O16
	if (H8S_EVAL_BPL) H8S_SET_PC( H8S_GET_PC() + (short)imm);
	H8S_CYCLES_IN( 2, 1);
}

// bmi imm16(=off):16 [-]
H8S_EXEC_INSTR( bmi_o16 )
{
	H8S_GET_DATA_O16
	if (H8S_EVAL_BMI) H8S_SET_PC( H8S_GET_PC() + (short)imm);
	H8S_CYCLES_IN( 2, 1);
}

// bge imm16(=off):16 [-]
H8S_EXEC_INSTR( bge_o16 )
{
	H8S_GET_DATA_O16
	if (H8S_EVAL_BGE) H8S_SET_PC( H8S_GET_PC() + (short)imm);
	H8S_CYCLES_IN( 2, 1);
}

// blt imm16(=off):16 [-]
H8S_EXEC_INSTR( blt_o16 )
{
	H8S_GET_DATA_O16
	if (H8S_EVAL_BLT) H8S_SET_PC( H8S_GET_PC() + (short)imm);
	H8S_CYCLES_IN( 2, 1);
}

// bgt imm16(=off):16 [-]
H8S_EXEC_INSTR( bgt_o16 )
{
	H8S_GET_DATA_O16
	if (H8S_EVAL_BGT) H8S_SET_PC( H8S_GET_PC() + (short)imm);
	H8S_CYCLES_IN( 2, 1);
}

// ble imm16(=off):16 [-]
H8S_EXEC_INSTR( ble_o16 )
{
	H8S_GET_DATA_O16
	if (H8S_EVAL_BLE) H8S_SET_PC( H8S_GET_PC() + (short)imm);
	H8S_CYCLES_IN( 2, 1);
}

/////////
// JMP //
/////////

// jmp @reg32 [-]
H8S_EXEC_INSTR( jmp_ar32 )
{
	H8S_GET_DATA_AR32
	H8S_SET_PC( H8S_GET_REG_32( reg_src) - 2); // 2 = instruction size
	H8S_CYCLES_I( 2);
}

// jmp imm:24 [-]
H8S_EXEC_INSTR( jmp_i24 )
{
	H8S_GET_DATA_I24
	H8S_SET_PC( imm - 4); // 4 = instruction size
	H8S_CYCLES_IN( 2, 1);
}

/////////
// BSR //
/////////

// bsr imm:8 [-]
H8S_EXEC_INSTR( bsr_o8 )
{
	H8S_GET_DATA_O8
	UINT32 pc = H8S_GET_PC(), sp = H8S_GET_SP();
	H8S_SET_MEM_32( sp - 4, pc + 2); // 2 = instruction size
	H8S_SET_PC( pc + (char)imm);
	H8S_SET_SP( sp - 4);
	H8S_CYCLES_IK( 2, 2);
}

// bsr imm:16 [-]
H8S_EXEC_INSTR( bsr_o16 )
{
	H8S_GET_DATA_O16
	UINT32 pc, sp;
	pc = H8S_GET_PC();
	sp = H8S_GET_SP();
	H8S_SET_MEM_32( sp - 4, pc + 4); // 4 = instruction size
	H8S_SET_PC( pc + (short)imm);
	H8S_SET_SP( sp - 4);
	H8S_CYCLES_IKN( 2, 2, 1);
}

/////////
// JSR //
/////////

// jsr @reg32 [-]
H8S_EXEC_INSTR( jsr_ar32 )
{
	H8S_GET_DATA_AR32
	UINT32 sp = H8S_GET_SP();
	H8S_SET_MEM_32( sp - 4, H8S_GET_PC() + 2); // 2 = instruction size
	H8S_SET_PC( H8S_GET_REG_32( reg_src) - 2);
	H8S_SET_SP( sp - 4);
	H8S_CYCLES_IK( 2, 2);
}

// jsr imm:24 [-]
H8S_EXEC_INSTR( jsr_i24 )
{
	H8S_GET_DATA_I24
	UINT32 sp = H8S_GET_SP();
	//UINT32 imm32 = H8S_GET_MEM_32( pc) & 0x00FFFFFF;

	// running cybiko xtreme app on cybiko classic causes this
	if (imm == 0) cpu_halt( "jump to address 0 - this happens when you run a cybikoxtreme app on a cybiko classic system");

	H8S_SET_MEM_32( sp - 4, H8S_GET_PC() + 4); // 4 = instruction size
	H8S_SET_SP( sp - 4);
	H8S_SET_PC( imm - 4);
	H8S_CYCLES_IKN( 2, 2, 1);
}

/////////
// RTS //
/////////

// rts [-]
H8S_EXEC_INSTR( rts )
{
	UINT32 sp, data;
	sp = H8S_GET_SP();
	data = H8S_GET_MEM_32( sp);
	H8S_SET_SP( sp + 4);
	H8S_SET_PC( (data & 0xFFFFFF) - 2); // 2 = instruction size
	H8S_CYCLES_IKN( 2, 2, 1);
}

/////////
// RTE //
/////////

// rte [*]
H8S_EXEC_INSTR( rte )
{
	UINT32 sp, data;
	sp = H8S_GET_SP();
	data = H8S_GET_MEM_32( sp);
	H8S_SET_SP( sp + 4);
	cpu_set_ccr( (data >> 24) & 0xFF);
	H8S_SET_PC( (data & 0xFFFFFF) - 2); // 2 = instruction size
	H8S_CYCLES_IKN( 2, 2, 1);
}

///////////
// SLEEP //
///////////

// sleep [-]
H8S_EXEC_INSTR( sleep )
{
	H8S_CYCLES_IN( 1, 1);
}

/////////
// LDC //
/////////

// ldc.b reg8, ccr [*]
H8S_EXEC_INSTR( ldc_r8_ccr )
{
	H8S_GET_DATA_R8_LDC
	cpu_set_ccr( H8S_GET_REG_8( reg_src));
	H8S_CYCLES_I( 1);
}

// ldc.w @reg32+, ccr [*]
H8S_EXEC_INSTR( ldc_ar32i_ccr )
{
	H8S_GET_DATA_R32_LDC
	UINT32 mem = H8S_GET_REG_32( reg_src);
	cpu_set_ccr( (UINT8)(H8S_GET_MEM_16( mem) & 0xFF));
	H8S_SET_REG_32( reg_src, mem + 2);
	H8S_CYCLES_IMN( 2, 1, 1);
}

/////////
// STC //
////////

// stc.b ccr, reg8 [-]
H8S_EXEC_INSTR( stc_ccr_r8 )
{
	H8S_GET_DATA_R8_STC
	H8S_SET_REG_8( reg_dst, cpu_get_ccr());
	H8S_CYCLES_I( 1);
}

// stc.w ccr, @-reg32 [-]
H8S_EXEC_INSTR( stc_ccr_ar32d )
{
	H8S_GET_DATA_R32_STC
	UINT32 mem = H8S_GET_REG_32( reg_dst);
	H8S_SET_MEM_16( mem - 2, cpu_get_ccr());
	H8S_SET_REG_32( reg_dst, mem - 2);
	H8S_CYCLES_IMN( 2, 1, 1);
}

//////////
// ANDC //
//////////

// andc #imm8, ccr [*]
H8S_EXEC_INSTR( andc_i8_ccr )
{
	H8S_GET_DATA_I8_CCR
	cpu_set_ccr( cpu_get_ccr() & imm);
	H8S_CYCLES_I( 1);
}

/////////
// ORC //
/////////

// orc #imm8, ccr [*]
H8S_EXEC_INSTR( orc_i8_ccr )
{
	H8S_GET_DATA_I8_CCR
	cpu_set_ccr( cpu_get_ccr() | imm);
	H8S_CYCLES_I( 1);
}

/////////
// NOP //
/////////

// nop [-]
H8S_EXEC_INSTR( nop )
{
	H8S_CYCLES_I( 1);
}

////////////
// EEPMOV //
////////////

// eepmov.w [-]
H8S_EXEC_INSTR( eepmov_w )
{
	UINT32 src, dst, cnt, i;
	cnt = H8S_GET_REG_16( 4);
	src = H8S_GET_REG_32( 5);
	dst = H8S_GET_REG_32( 6);
	for (i=0;i<cnt;i++)
	{
		H8S_SET_MEM_8( dst++, H8S_GET_MEM_8( src++));
	}
	H8S_SET_REG_16( 4, 0);
	H8S_SET_REG_32( 5, src);
	H8S_SET_REG_32( 6, dst);
	H8S_CYCLES_IL( 2, 2 + cnt * 2);
}

///////////////
// INTERRUPT //
///////////////

void h8s_exec_interrupt( UINT32 vecnum)
{
	UINT32 sp, data;
	data = (cpu_get_ccr() << 24) | H8S_GET_PC();
	sp = H8S_GET_SP();
	H8S_SET_MEM_32( sp - 4, data);
	H8S_SET_SP( sp - 4);
	cpu_set_flag( FLAG_I, 1);
	H8S_SET_PC( H8S_GET_MEM_32( vecnum << 2));
}

/////////////
// EXECUTE //
/////////////

void h8s_exec_instr( UINT32 index, UINT8 *instr)
{
	switch (index)
	{
		// MOV XX, R8
		case INSTR_MOV_I8_R8      : h8s_exec_instr_mov_i8_r8( instr); break;
		case INSTR_MOV_R8_R8      : h8s_exec_instr_mov_r8_r8( instr); break;
		case INSTR_MOV_AR32_R8    : h8s_exec_instr_mov_ar32_r8( instr); break;
		case INSTR_MOV_AI16R32_R8 : h8s_exec_instr_mov_ai16r32_r8( instr); break;
		case INSTR_MOV_AI32R32_R8 : h8s_exec_instr_mov_ai32r32_r8( instr); break;
		case INSTR_MOV_AR32I_R8   : h8s_exec_instr_mov_ar32i_r8( instr); break;
		case INSTR_MOV_AI8_R8     : h8s_exec_instr_mov_ai8_r8( instr); break;
		case INSTR_MOV_AI16_R8    : h8s_exec_instr_mov_ai16_r8( instr); break;
		case INSTR_MOV_AI32_R8    : h8s_exec_instr_mov_ai32_r8( instr); break;
		// MOV R8, XX
		case INSTR_MOV_R8_AR32    : h8s_exec_instr_mov_r8_ar32( instr); break;
		case INSTR_MOV_R8_AI16R32 : h8s_exec_instr_mov_r8_ai16r32( instr); break;
		case INSTR_MOV_R8_AI32R32 : h8s_exec_instr_mov_r8_ai32r32( instr); break;
		case INSTR_MOV_R8_AR32D   : h8s_exec_instr_mov_r8_ar32d( instr); break;
		case INSTR_MOV_R8_AI8     : h8s_exec_instr_mov_r8_ai8( instr); break;
		case INSTR_MOV_R8_AI16    : h8s_exec_instr_mov_r8_ai16( instr); break;
		case INSTR_MOV_R8_AI32    : h8s_exec_instr_mov_r8_ai32( instr); break;
		// MOV XX, R16
		case INSTR_MOV_I16_R16     : h8s_exec_instr_mov_i16_r16( instr); break;
		case INSTR_MOV_R16_R16     : h8s_exec_instr_mov_r16_r16( instr); break;
		case INSTR_MOV_AR32_R16    : h8s_exec_instr_mov_ar32_r16( instr); break;
		case INSTR_MOV_AI16R32_R16 : h8s_exec_instr_mov_ai16r32_r16( instr); break;
		case INSTR_MOV_AI32R32_R16 : h8s_exec_instr_mov_ai32r32_r16( instr); break;
		case INSTR_MOV_AR32I_R16   : h8s_exec_instr_mov_ar32i_r16( instr); break;
		case INSTR_MOV_AI16_R16    : h8s_exec_instr_mov_ai16_r16( instr); break;
		case INSTR_MOV_AI32_R16    : h8s_exec_instr_mov_ai32_r16( instr); break;
		// MOV R16, XX
		case INSTR_MOV_R16_AR32    : h8s_exec_instr_mov_r16_ar32( instr); break;
		case INSTR_MOV_R16_AI16R32 : h8s_exec_instr_mov_r16_ai16r32( instr); break;
		case INSTR_MOV_R16_AI32R32 : h8s_exec_instr_mov_r16_ai32r32( instr); break;
		case INSTR_MOV_R16_AR32D   : h8s_exec_instr_mov_r16_ar32d( instr); break;
		case INSTR_MOV_R16_AI16    : h8s_exec_instr_mov_r16_ai16( instr); break;
		case INSTR_MOV_R16_AI32    : h8s_exec_instr_mov_r16_ai32( instr); break;
		// MOV XX, R32
		case INSTR_MOV_I32_R32     : h8s_exec_instr_mov_i32_r32( instr); break;
		case INSTR_MOV_R32_R32     : h8s_exec_instr_mov_r32_r32( instr); break;
		case INSTR_MOV_AR32_R32    : h8s_exec_instr_mov_ar32_r32( instr); break;
		case INSTR_MOV_AI16R32_R32 : h8s_exec_instr_mov_ai16r32_r32( instr); break;
		case INSTR_MOV_AI32R32_R32 : h8s_exec_instr_mov_ai32r32_r32( instr); break;
		case INSTR_MOV_AR32I_R32   : h8s_exec_instr_mov_ar32i_r32( instr); break;
		case INSTR_MOV_AI16_R32    : h8s_exec_instr_mov_ai16_r32( instr); break;
		case INSTR_MOV_AI32_R32    : h8s_exec_instr_mov_ai32_r32( instr); break;
		// MOV R32, XX
		case INSTR_MOV_R32_AR32    : h8s_exec_instr_mov_r32_ar32( instr); break;
		case INSTR_MOV_R32_AI16R32 : h8s_exec_instr_mov_r32_ai16r32( instr); break;
		case INSTR_MOV_R32_AI32R32 : h8s_exec_instr_mov_r32_ai32r32( instr); break;
		case INSTR_MOV_R32_AR32D   : h8s_exec_instr_mov_r32_ar32d( instr); break;
		case INSTR_MOV_R32_AI16    : h8s_exec_instr_mov_r32_ai16( instr); break;
		case INSTR_MOV_R32_AI32    : h8s_exec_instr_mov_r32_ai32( instr); break;
		// PUSH
		#ifdef ALIAS_PUSH
		case INSTR_PUSH_R16 : h8s_exec_instr_push_r16( instr); break;
		case INSTR_PUSH_R32 : h8s_exec_instr_push_r32( instr); break;
		#endif
		// POP
		#ifdef ALIAS_POP
		case INSTR_POP_R16 : h8s_exec_instr_pop_r16( instr); break;
		case INSTR_POP_R32 : h8s_exec_instr_pop_r32( instr); break;
		#endif
		// LDM
		case INSTR_LDM_AR32I_R32R32 : h8s_exec_instr_ldm_ar32i_r32r32( instr); break;
		// STM
		case INSTR_STM_R32R32_AR32D : h8s_exec_instr_stm_r32r32_ar32d( instr); break;
		// ADD
		case INSTR_ADD_I8_R8   : h8s_exec_instr_add_i8_r8( instr); break;
		case INSTR_ADD_R8_R8   : h8s_exec_instr_add_r8_r8( instr); break;
		case INSTR_ADD_I16_R16 : h8s_exec_instr_add_i16_r16( instr); break;
		case INSTR_ADD_R16_R16 : h8s_exec_instr_add_r16_r16( instr); break;
		case INSTR_ADD_I32_R32 : h8s_exec_instr_add_i32_r32( instr); break;
		case INSTR_ADD_R32_R32 : h8s_exec_instr_add_r32_r32( instr); break;
		// ADDX
		case INSTR_ADDX_I8_R8 : h8s_exec_instr_addx_i8_r8( instr); break;
		case INSTR_ADDX_R8_R8 : h8s_exec_instr_addx_r8_r8( instr); break;
		// ADDS
		case INSTR_ADDS_XX_R32 : h8s_exec_instr_adds_xx_r32( instr); break;
		// INC
		case INSTR_INC_XX_R8  : h8s_exec_instr_inc_xx_r8( instr); break;
		case INSTR_INC_XX_R16 : h8s_exec_instr_inc_xx_r16( instr); break;
		case INSTR_INC_XX_R32 : h8s_exec_instr_inc_xx_r32( instr); break;
		// SUB
		case INSTR_SUB_R8_R8   : h8s_exec_instr_sub_r8_r8( instr); break;
		case INSTR_SUB_I16_R16 : h8s_exec_instr_sub_i16_r16( instr); break;
		case INSTR_SUB_R16_R16 : h8s_exec_instr_sub_r16_r16( instr); break;
		case INSTR_SUB_I32_R32 : h8s_exec_instr_sub_i32_r32( instr); break;
		case INSTR_SUB_R32_R32 : h8s_exec_instr_sub_r32_r32( instr); break;
		// SUBX
		case INSTR_SUBX_R8_R8 : h8s_exec_instr_subx_r8_r8( instr); break;
		// SUBS
		case INSTR_SUBS_XX_R32 : h8s_exec_instr_subs_xx_r32( instr); break;
		// DEC
		case INSTR_DEC_XX_R8  : h8s_exec_instr_dec_xx_r8( instr); break;
		case INSTR_DEC_XX_R16 : h8s_exec_instr_dec_xx_r16( instr); break;
		case INSTR_DEC_XX_R32 : h8s_exec_instr_dec_xx_r32( instr); break;
		// MULXU
		case INSTR_MULXU_R8_R16  : h8s_exec_instr_mulxu_r8_r16( instr); break;
		case INSTR_MULXU_R16_R32 : h8s_exec_instr_mulxu_r16_r32( instr); break;
		// MULXS
		case INSTR_MULXS_R8_R16  : h8s_exec_instr_mulxs_r8_r16( instr); break;
		case INSTR_MULXS_R16_R32 : h8s_exec_instr_mulxs_r16_r32( instr); break;
		// DIVXU
		case INSTR_DIVXU_R8_R16  : h8s_exec_instr_divxu_r8_r16( instr); break;
		case INSTR_DIVXU_R16_R32 : h8s_exec_instr_divxu_r16_r32( instr); break;
		// DIVXS
		case INSTR_DIVXS_R16_R32 : h8s_exec_instr_divxs_r16_r32( instr); break;
		// CMP
		case INSTR_CMP_I8_R8   : h8s_exec_instr_cmp_i8_r8( instr); break;
		case INSTR_CMP_R8_R8   : h8s_exec_instr_cmp_r8_r8( instr); break;
		case INSTR_CMP_I16_R16 : h8s_exec_instr_cmp_i16_r16( instr); break;
		case INSTR_CMP_R16_R16 : h8s_exec_instr_cmp_r16_r16( instr); break;
		case INSTR_CMP_I32_R32 : h8s_exec_instr_cmp_i32_r32( instr); break;
		case INSTR_CMP_R32_R32 : h8s_exec_instr_cmp_r32_r32( instr); break;
		// NEG
		case INSTR_NEG_R8  : h8s_exec_instr_neg_r8( instr); break;
		case INSTR_NEG_R16 : h8s_exec_instr_neg_r16( instr); break;
		case INSTR_NEG_R32 : h8s_exec_instr_neg_r32( instr); break;
		// EXTU
		case INSTR_EXTU_R16 : h8s_exec_instr_extu_r16( instr); break;
		case INSTR_EXTU_R32 : h8s_exec_instr_extu_r32( instr); break;
		// EXTS
		case INSTR_EXTS_R16 : h8s_exec_instr_exts_r16( instr); break;
		case INSTR_EXTS_R32 : h8s_exec_instr_exts_r32( instr); break;
		// AND
		case INSTR_AND_I8_R8   : h8s_exec_instr_and_i8_r8( instr); break;
		case INSTR_AND_R8_R8   : h8s_exec_instr_and_r8_r8( instr); break;
		case INSTR_AND_I16_R16 : h8s_exec_instr_and_i16_r16( instr); break;
		case INSTR_AND_R16_R16 : h8s_exec_instr_and_r16_r16( instr); break;
		case INSTR_AND_I32_R32 : h8s_exec_instr_and_i32_r32( instr); break;
		case INSTR_AND_R32_R32 : h8s_exec_instr_and_r32_r32( instr); break;
		// OR
		case INSTR_OR_I8_R8   : h8s_exec_instr_or_i8_r8( instr); break;
		case INSTR_OR_R8_R8   : h8s_exec_instr_or_r8_r8( instr); break;
		case INSTR_OR_R16_R16 : h8s_exec_instr_or_r16_r16( instr); break;
		case INSTR_OR_I32_R32 : h8s_exec_instr_or_i32_r32( instr); break;
		case INSTR_OR_R32_R32 : h8s_exec_instr_or_r32_r32( instr); break;
		// XOR
		case INSTR_XOR_I8_R8   : h8s_exec_instr_xor_i8_r8( instr); break;
		case INSTR_XOR_R8_R8   : h8s_exec_instr_xor_r8_r8( instr); break;
		case INSTR_XOR_I16_R16 : h8s_exec_instr_xor_i16_r16( instr); break;
		case INSTR_XOR_R16_R16 : h8s_exec_instr_xor_r16_r16( instr); break;
		case INSTR_XOR_I32_R32 : h8s_exec_instr_xor_i32_r32( instr); break;
		case INSTR_XOR_R32_R32 : h8s_exec_instr_xor_r32_r32( instr); break;
		// NOT
		case INSTR_NOT_R8  : h8s_exec_instr_not_r8( instr); break;
		case INSTR_NOT_R16 : h8s_exec_instr_not_r16( instr); break;
		case INSTR_NOT_R32 : h8s_exec_instr_not_r32( instr); break;
		// SHAL
		case INSTR_SHAL_XX_R8  : h8s_exec_instr_shal_xx_r8( instr); break;
		case INSTR_SHAL_XX_R16 : h8s_exec_instr_shal_xx_r16( instr); break;
		case INSTR_SHAL_XX_R32 : h8s_exec_instr_shal_xx_r32( instr); break;
		// SHAR
		case INSTR_SHAR_XX_R16 : h8s_exec_instr_shar_xx_r16( instr); break;
		case INSTR_SHAR_XX_R32 : h8s_exec_instr_shar_xx_r32( instr); break;
		// SHLL
		case INSTR_SHLL_XX_R8  : h8s_exec_instr_shll_xx_r8( instr); break;
		case INSTR_SHLL_XX_R16 : h8s_exec_instr_shll_xx_r16( instr); break;
		case INSTR_SHLL_XX_R32 : h8s_exec_instr_shll_xx_r32( instr); break;
		// SHLR
		case INSTR_SHLR_XX_R8  : h8s_exec_instr_shlr_xx_r8( instr); break;
		case INSTR_SHLR_XX_R16 : h8s_exec_instr_shlr_xx_r16( instr); break;
		case INSTR_SHLR_XX_R32 : h8s_exec_instr_shlr_xx_r32( instr); break;
		// ROTXL
		case INSTR_ROTXL_XX_R8  : h8s_exec_instr_rotxl_xx_r8( instr); break;
		case INSTR_ROTXL_XX_R32 : h8s_exec_instr_rotxl_xx_r32( instr); break;
		// ROTL
		case INSTR_ROTL_XX_R8  : h8s_exec_instr_rotl_xx_r8( instr); break;
		case INSTR_ROTL_XX_R16 : h8s_exec_instr_rotl_xx_r16( instr); break;
		case INSTR_ROTL_XX_R32 : h8s_exec_instr_rotl_xx_r32( instr); break;
		// ROTR
		case INSTR_ROTR_XX_R8  : h8s_exec_instr_rotr_xx_r8( instr); break;
		case INSTR_ROTR_XX_R32 : h8s_exec_instr_rotr_xx_r32( instr); break;
		// BSET
		case INSTR_BSET_Ix_AR32 : h8s_exec_instr_bset_ix_ar32( instr); break;
		case INSTR_BSET_Ix_AI32 : h8s_exec_instr_bset_ix_ai32( instr); break;
		case INSTR_BSET_R8_AI32 : h8s_exec_instr_bset_r8_ai32( instr); break;
		case INSTR_BSET_Ix_AI8  : h8s_exec_instr_bset_ix_ai8( instr); break;
		// BCLR
		case INSTR_BCLR_Ix_AR32 : h8s_exec_instr_bclr_ix_ar32( instr); break;
		case INSTR_BCLR_Ix_AI32 : h8s_exec_instr_bclr_ix_ai32( instr); break;
		case INSTR_BCLR_R8_AI32 : h8s_exec_instr_bclr_r8_ai32( instr); break;
		case INSTR_BCLR_Ix_AI8  : h8s_exec_instr_bclr_ix_ai8( instr); break;
		// BNOT
		case INSTR_BNOT_Ix_AI8  : h8s_exec_instr_bnot_ix_ai8( instr); break;
		case INSTR_BNOT_Ix_AI32 : h8s_exec_instr_bnot_ix_ai32( instr); break;
		// BTST
		case INSTR_BTST_Ix_R8   : h8s_exec_instr_btst_ix_r8( instr); break;
		case INSTR_BTST_Ix_AR32 : h8s_exec_instr_btst_ix_ar32( instr); break;
		case INSTR_BTST_Ix_AI32 : h8s_exec_instr_btst_ix_ai32( instr); break;
		case INSTR_BTST_Ix_AI8  : h8s_exec_instr_btst_ix_ai8( instr); break;
		// BLD
		case INSTR_BLD_Ix_R8 : h8s_exec_instr_bld_ix_r8( instr); break;
		// BST
		case INSTR_BST_Ix_R8 : h8s_exec_instr_bst_ix_r8( instr); break;
		// BXX
		case INSTR_BRA_O8  : h8s_exec_instr_bra_o8( instr); break;
		case INSTR_BHI_O8  : h8s_exec_instr_bhi_o8( instr); break;
		case INSTR_BLS_O8  : h8s_exec_instr_bls_o8( instr); break;
		case INSTR_BCC_O8  : h8s_exec_instr_bcc_o8( instr); break;
		case INSTR_BCS_O8  : h8s_exec_instr_bcs_o8( instr); break;
		case INSTR_BNE_O8  : h8s_exec_instr_bne_o8( instr); break;
		case INSTR_BEQ_O8  : h8s_exec_instr_beq_o8( instr); break;
		case INSTR_BPL_O8  : h8s_exec_instr_bpl_o8( instr); break;
		case INSTR_BMI_O8  : h8s_exec_instr_bmi_o8( instr); break;
		case INSTR_BGE_O8  : h8s_exec_instr_bge_o8( instr); break;
		case INSTR_BLT_O8  : h8s_exec_instr_blt_o8( instr); break;
		case INSTR_BGT_O8  : h8s_exec_instr_bgt_o8( instr); break;
		case INSTR_BLE_O8  : h8s_exec_instr_ble_o8( instr); break;
		case INSTR_BRA_O16 : h8s_exec_instr_bra_o16( instr); break;
		case INSTR_BHI_O16 : h8s_exec_instr_bhi_o16( instr); break;
		case INSTR_BLS_O16 : h8s_exec_instr_bls_o16( instr); break;
		case INSTR_BCC_O16 : h8s_exec_instr_bcc_o16( instr); break;
		case INSTR_BCS_O16 : h8s_exec_instr_bcs_o16( instr); break;
		case INSTR_BNE_O16 : h8s_exec_instr_bne_o16( instr); break;
		case INSTR_BEQ_O16 : h8s_exec_instr_beq_o16( instr); break;
		case INSTR_BPL_O16 : h8s_exec_instr_bpl_o16( instr); break;
		case INSTR_BMI_O16 : h8s_exec_instr_bmi_o16( instr); break;
		case INSTR_BGE_O16 : h8s_exec_instr_bge_o16( instr); break;
		case INSTR_BLT_O16 : h8s_exec_instr_blt_o16( instr); break;
		case INSTR_BGT_O16 : h8s_exec_instr_bgt_o16( instr); break;
		case INSTR_BLE_O16 : h8s_exec_instr_ble_o16( instr); break;
		// JMP
		case INSTR_JMP_AR32 : h8s_exec_instr_jmp_ar32( instr); break;
		case INSTR_JMP_I24  : h8s_exec_instr_jmp_i24( instr); break;
		// BSR
		case INSTR_BSR_O8  : h8s_exec_instr_bsr_o8( instr); break;
		case INSTR_BSR_O16 : h8s_exec_instr_bsr_o16( instr); break;
		// JSR
		case INSTR_JSR_AR32 : h8s_exec_instr_jsr_ar32( instr); break;
		case INSTR_JSR_I24  : h8s_exec_instr_jsr_i24( instr); break;
		// RTS
		case INSTR_RTS : h8s_exec_instr_rts( instr); break;
		// RTE
		case INSTR_RTE : h8s_exec_instr_rte( instr); break;
		// SLEEP
		case INSTR_SLEEP : h8s_exec_instr_sleep( instr); break;
		// LDC
		case INSTR_LDC_R8_CCR    : h8s_exec_instr_ldc_r8_ccr( instr); break;
		case INSTR_LDC_AR32I_CCR : h8s_exec_instr_ldc_ar32i_ccr( instr); break;
		// STC
		case INSTR_STC_CCR_R8    : h8s_exec_instr_stc_ccr_r8( instr); break;
		case INSTR_STC_CCR_AR32D : h8s_exec_instr_stc_ccr_ar32d( instr); break;
		// ANDC
		case INSTR_ANDC_I8_CCR : h8s_exec_instr_andc_i8_ccr( instr); break;
		// ORC
		case INSTR_ORC_I8_CCR : h8s_exec_instr_orc_i8_ccr( instr); break;
		// NOP
		case INSTR_NOP : h8s_exec_instr_nop( instr); break;
		// EEPMOV
		case INSTR_EEPMOV_W : h8s_exec_instr_eepmov_w( instr); break;
		// ILLEGAL
		default : cpu_halt( "unemulated instruction"); break;
	}
}
