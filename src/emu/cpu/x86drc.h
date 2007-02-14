/***************************************************************************

    x86drc.h

    x86 Dynamic recompiler support routines.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#pragma once

#ifndef __X86DRC_H__
#define __X86DRC_H__

#include "mamecore.h"


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/* PC and pointer pair */
struct _pc_ptr_pair
{
	UINT32		pc;
	UINT8 *		target;
};
typedef struct _pc_ptr_pair pc_ptr_pair;

/* core interface structure for the drc common code */
struct _drc_core
{
	UINT8 *		cache_base;				/* base pointer to the compiler cache */
	UINT8 *		cache_top;				/* current top of cache */
	UINT8 *		cache_danger;			/* high water mark for the end */
	UINT8 *		cache_end;				/* end of cache memory */
	size_t		cache_size;				/* cache allocated size */

	void ***	lookup_l1;				/* level 1 lookup */
	void **		lookup_l2_recompile;	/* level 2 lookup populated with recompile pointers */
	UINT8		l1bits;					/* number of bits in level 1 lookup */
	UINT8		l2bits;					/* number of bits in level 2 lookup */
	UINT8		l1shift;				/* shift to go from PC to level 1 lookup */
	UINT32		l2mask;					/* mask to go from PC to level 2 lookup */
	UINT8		l2scale;				/* scale to get from masked PC value to final level 2 lookup */

	void 		(*entry_point)(void);	/* pointer to asm entry point */
	void *		out_of_cycles;			/* pointer to out of cycles jump point */
	void *		recompile;				/* pointer to recompile jump point */
	void *		dispatch;				/* pointer to dispatch jump point */
	void *		flush;					/* pointer to flush jump point */

	UINT32 *	pcptr;					/* pointer to where the PC is stored */
	UINT32 *	icountptr;				/* pointer to where the icount is stored */
	UINT32 *	esiptr;					/* pointer to where the volatile data in ESI is stored */
	UINT8		pc_in_memory;			/* true if the PC is stored in memory */
	UINT8		icount_in_memory;		/* true if the icount is stored in memory */

	UINT8		uses_fp;				/* true if we need the FP unit */
	UINT8		uses_sse;				/* true if we need the SSE unit */
	UINT16		fpcw_curr;				/* current FPU control word */
	UINT32		mxcsr_curr;				/* current SSE control word */
	UINT16		fpcw_save;				/* saved FPU control word */
	UINT32		mxcsr_save;				/* saved SSE control word */

	pc_ptr_pair *sequence_list;			/* PC/pointer sets for the current instruction sequence */
	UINT32		sequence_count;			/* number of instructions in the current sequence */
	UINT32		sequence_count_max;		/* max number of instructions in the current sequence */
	pc_ptr_pair *tentative_list;		/* PC/pointer sets for tentative branches */
	UINT32		tentative_count;		/* number of tentative branches */
	UINT32		tentative_count_max;	/* max number of tentative branches */

	void 		(*cb_reset)(struct _drc_core *drc);		/* callback when the cache is reset */
	void 		(*cb_recompile)(struct _drc_core *drc);	/* callback when code needs to be recompiled */
	void 		(*cb_entrygen)(struct _drc_core *drc);	/* callback before generating the dispatcher on entry */
};
typedef struct _drc_core drc_core;

/* configuration structure for the drc common code */
struct _drc_config
{
	UINT32		cache_size;				/* size of cache to allocate */
	UINT32		max_instructions;		/* maximum instructions per sequence */
	UINT8		address_bits;			/* number of live address bits in the PC */
	UINT8		lsbs_to_ignore;			/* number of LSBs to ignore on the PC */
	UINT8		uses_fp;				/* true if we need the FP unit */
	UINT8		uses_sse;				/* true if we need the SSE unit */
	UINT8		pc_in_memory;			/* true if the PC is stored in memory */
	UINT8		icount_in_memory;		/* true if the icount is stored in memory */

	UINT32 *	pcptr;					/* pointer to where the PC is stored */
	UINT32 *	icountptr;				/* pointer to where the icount is stored */
	UINT32 *	esiptr;					/* pointer to where the volatile data in ESI is stored */

	void 		(*cb_reset)(drc_core *drc);		/* callback when the cache is reset */
	void 		(*cb_recompile)(drc_core *drc);	/* callback when code needs to be recompiled */
	void 		(*cb_entrygen)(drc_core *drc);	/* callback before generating the dispatcher on entry */
};
typedef struct _drc_config drc_config;

/* structure to hold link data to be filled in later */
struct _link_info
{
	UINT8 		size;
	UINT8 *		target;
};
typedef struct _link_info link_info;



/***************************************************************************
    HELPER MACROS
***************************************************************************/

/* useful macros for accessing hi/lo portions of 64-bit values */
#define LO(x)		(&(((UINT32 *)(UINT32)(x))[0]))
#define HI(x)		(&(((UINT32 *)(UINT32)(x))[1]))

extern const UINT8 scale_lookup[];


/***************************************************************************
    CONSTANTS
***************************************************************************/

/* architectural defines */
#define REG_EAX		0
#define REG_ECX		1
#define REG_EDX		2
#define REG_EBX		3
#define REG_ESP		4
#define REG_EBP		5
#define REG_ESI		6
#define REG_EDI		7

#define REG_AX		0
#define REG_CX		1
#define REG_DX		2
#define REG_BX		3
#define REG_SP		4
#define REG_BP		5
#define REG_SI		6
#define REG_DI		7

#define REG_AL		0
#define REG_CL		1
#define REG_DL		2
#define REG_BL		3
#define REG_AH		4
#define REG_CH		5
#define REG_DH		6
#define REG_BH		7

#define REG_MM0		0
#define REG_MM1		1
#define REG_MM2		2
#define REG_MM3		3
#define REG_MM4		4
#define REG_MM5		5
#define REG_MM6		6
#define REG_MM7		7

#define REG_XMM0	0
#define REG_XMM1	1
#define REG_XMM2	2
#define REG_XMM3	3
#define REG_XMM4	4
#define REG_XMM5	5
#define REG_XMM6	6
#define REG_XMM7	7

#define NO_BASE		5

#define COND_A		7
#define COND_AE		3
#define COND_B		2
#define COND_BE		6
#define COND_C		2
#define COND_E		4
#define COND_Z		4
#define COND_G		15
#define COND_GE		13
#define COND_L		12
#define COND_LE		14
#define COND_NA		6
#define COND_NAE	2
#define COND_NB		3
#define COND_NBE	7
#define COND_NC		3
#define COND_NE		5
#define COND_NG		14
#define COND_NGE	12
#define COND_NL		13
#define COND_NLE	15
#define COND_NO		1
#define COND_NP		11
#define COND_NS		9
#define COND_NZ		5
#define COND_O		0
#define COND_P		10
#define COND_PE		10
#define COND_PO		11
#define COND_S		8
#define COND_Z		4

/* rounding modes */
#define FPRND_NEAR	0
#define FPRND_DOWN	1
#define FPRND_UP	2
#define FPRND_CHOP	3

/* register counts */
#define REGCOUNT_MMX	8
#define REGCOUNT_SSE	8

/* features */
#define CPUID_FEATURES_MMX		(1 << 23)
#define CPUID_FEATURES_SSE		(1 << 26)
#define CPUID_FEATURES_SSE2		(1 << 25)
#define CPUID_FEATURES_CMOV		(1 << 15)
#define CPUID_FEATURES_TSC		(1 << 4)



/***************************************************************************
    LOW-LEVEL OPCODE EMITTERS
***************************************************************************/

/* lowest-level opcode emitters */
#define OP1(x)		do { *drc->cache_top++ = (UINT8)(x); } while (0)
#define OP2(x)		do { *(UINT16 *)drc->cache_top = (UINT16)(x); drc->cache_top += 2; } while (0)
#define OP4(x)		do { *(UINT32 *)drc->cache_top = (UINT32)(x); drc->cache_top += 4; } while (0)



/***************************************************************************
    MODRM EMITTERS
***************************************************************************/

// op  reg,reg
#define MODRM_REG(reg, rm) 		\
do { OP1(0xc0 | (((reg) & 7) << 3) | ((rm) & 7)); } while (0)

// op  reg,[addr]
#define MODRM_MABS(reg, addr)	\
do { OP1(0x05 | (((reg) & 7) << 3)); OP4(addr); } while (0)

// op  reg,[base+disp]
#define MODRM_MBD(reg, base, disp) \
do {														\
	if ((UINT32)(disp) == 0 && (base) != REG_ESP && (base) != REG_EBP) \
	{														\
		OP1(0x00 | (((reg) & 7) << 3) | ((base) & 7));		\
	}														\
	else if ((INT8)(INT32)(disp) == (INT32)(disp))			\
	{														\
		if ((base) == REG_ESP)								\
		{													\
			OP1(0x44 | (((reg) & 7) << 3));					\
			OP1(0x24);										\
			OP1((INT32)disp);								\
		}													\
		else												\
		{													\
			OP1(0x40 | (((reg) & 7) << 3) | ((base) & 7));	\
			OP1((INT32)disp);								\
		}													\
	}														\
	else													\
	{														\
		if ((base) == REG_ESP)								\
		{													\
			OP1(0x84 | (((reg) & 7) << 3));					\
			OP1(0x24);										\
			OP4(disp);										\
		}													\
		else												\
		{													\
			OP1(0x80 | (((reg) & 7) << 3) | ((base) & 7));	\
			OP4(disp);										\
		}													\
	}														\
} while (0)

// op  reg,[base+indx*scale+disp]
#define MODRM_MBISD(reg, base, indx, scale, disp)			\
do {														\
	if ((scale) == 1 && (base) == NO_BASE)					\
		MODRM_MBD(reg,indx,disp);							\
	else if ((UINT32)(disp) == 0 || (base) == NO_BASE)		\
	{														\
		OP1(0x04 | (((reg) & 7) << 3));						\
		OP1((scale_lookup[scale] << 6) | (((indx) & 7) << 3) | ((base) & 7));\
		if ((UINT32)(disp) != 0) OP4(disp);					\
	}														\
	else if ((INT8)(INT32)(disp) == (INT32)(disp))			\
	{														\
		OP1(0x44 | (((reg) & 7) << 3));						\
		OP1((scale_lookup[scale] << 6) | (((indx) & 7) << 3) | ((base) & 7));\
		OP1((INT32)disp);									\
	}														\
	else													\
	{														\
		OP1(0x84 | (((reg) & 7) << 3));						\
		OP1((scale_lookup[scale] << 6) | (((indx) & 7) << 3) | ((base) & 7));\
		OP4(disp);											\
	}														\
} while (0)



/***************************************************************************
    SIMPLE OPCODE EMITTERS
***************************************************************************/

#define _pushad() \
do { OP1(0x60); } while (0)

#define _push_r32(reg) \
do { OP1(0x50+(reg)); } while (0)

#define _push_imm(imm) \
do { OP1(0x68); OP4(imm); } while (0)

#define _push_m32abs(addr) \
do { OP1(0xff); MODRM_MABS(6, addr); } while (0)

#define _push_m32bd(base, disp) \
do { OP1(0xff); MODRM_MBD(6, base, disp); } while (0)

#define _popad() \
do { OP1(0x61); } while (0)

#define _pop_r32(reg) \
do { OP1(0x58+(reg)); } while (0)

#define _pop_m32abs(addr) \
do { OP1(0x8f); MODRM_MABS(0, addr); } while (0)

#define _ret() \
do { OP1(0xc3); } while (0)

#define _cdq() \
do { OP1(0x99); } while (0)

#define _lahf() \
do { OP1(0x9F); } while(0);

#define _sahf() \
do { OP1(0x9E); } while(0);

#define _pushfd() \
do { OP1(0x9c); } while(0);

#define _popfd() \
do { OP1(0x9d); } while(0);

#define _bswap_r32(reg) \
do { OP1(0x0F); OP1(0xC8+(reg)); } while (0)

#define _cmc() \
do { OP1(0xf5); } while(0);

#define _int(interrupt) \
do { if (interrupt == 3) { OP1(0xcc); } else { OP1(0xcd); OP1(interrupt); } } while(0)



/***************************************************************************
    MOVE EMITTERS
***************************************************************************/

#define _mov_r8_imm(dreg, imm) \
do { OP1(0xb0 + (dreg)); OP1(imm); } while (0)

#define _mov_r8_r8(dreg, sreg) \
do { OP1(0x8a); MODRM_REG(dreg, sreg); } while (0)

#define _mov_r8_m8abs(dreg, addr) \
do { OP1(0x8a); MODRM_MABS(dreg, addr); } while (0)

#define _mov_r8_m8bd(dreg, base, disp) \
do { OP1(0x8a); MODRM_MBD(dreg, base, disp); } while (0)

#define _mov_r8_m8isd(dreg, indx, scale, disp) \
do { OP1(0x8a); MODRM_MBISD(dreg, NO_BASE, indx, scale, disp); } while (0)

#define _mov_r8_m8bisd(dreg, base, indx, scale, disp) \
do { OP1(0x8a); MODRM_MBISD(dreg, base, indx, scale, disp); } while (0)



#define _mov_r16_imm(dreg, imm) \
do { OP1(0x66); OP1(0xb8 + (dreg)); OP2(imm); } while (0)

#define _mov_r16_r16(dreg, sreg) \
do { OP1(0x66); OP1(0x8b); MODRM_REG(dreg, sreg); } while (0)

#define _mov_r16_m16abs(dreg, addr) \
do { OP1(0x66); OP1(0x8b); MODRM_MABS(dreg, addr); } while (0)

#define _mov_r16_m16bd(dreg, base, disp) \
do { OP1(0x66); OP1(0x8b); MODRM_MBD(dreg, base, disp); } while (0)

#define _mov_r16_m16isd(dreg, indx, scale, disp) \
do { OP1(0x66); OP1(0x8b); MODRM_MBISD(dreg, NO_BASE, indx, scale, disp); } while (0)

#define _mov_r16_m16bisd(dreg, base, indx, scale, disp) \
do { OP1(0x66); OP1(0x8b); MODRM_MBISD(dreg, base, indx, scale, disp); } while (0)



#define _mov_r32_imm(dreg, imm) \
do { OP1(0xb8 + (dreg)); OP4(imm); } while (0)

#define _mov_r32_r32(dreg, sreg) \
do { OP1(0x8b); MODRM_REG(dreg, sreg); } while (0)

#define _mov_r32_m32abs(dreg, addr) \
do { OP1(0x8b); MODRM_MABS(dreg, addr); } while (0)

#define _mov_r32_m32bd(dreg, base, disp) \
do { OP1(0x8b); MODRM_MBD(dreg, base, disp); } while (0)

#define _mov_r32_m32isd(dreg, indx, scale, disp) \
do { OP1(0x8b); MODRM_MBISD(dreg, NO_BASE, indx, scale, disp); } while (0)

#define _mov_r32_m32bisd(dreg, base, indx, scale, disp) \
do { OP1(0x8b); MODRM_MBISD(dreg, base, indx, scale, disp); } while (0)



#define _cmov_r32_r32(cond, dreg, sreg) \
do { OP1(0x0f); OP1(0x40 + (cond)); MODRM_REG(dreg, sreg); } while (0)

#define _cmov_r32_m32abs(cond, dreg, addr) \
do { OP1(0x0f); OP1(0x40 + (cond)); MODRM_MABS(dreg, addr); } while (0)

#define _cmov_r32_m32bd(cond, dreg, base, disp) \
do { OP1(0x0f); OP1(0x40 + (cond)); MODRM_MBD(dreg, base, disp); } while (0)

#define _cmov_r32_m32isd(cond, dreg, indx, scale, disp) \
do { OP1(0x0f); OP1(0x40 + (cond)); MODRM_MBISD(dreg, NO_BASE, indx, scale, disp); } while (0)

#define _cmov_r32_m32bisd(cond, dreg, base, indx, scale, disp) \
do { OP1(0x0f); OP1(0x40 + (cond)); MODRM_MBISD(dreg, base, indx, scale, disp); } while (0)



#define _mov_m8abs_imm(addr, imm) \
do { OP1(0xc6); MODRM_MABS(0, addr); OP1(imm); } while (0)

#define _mov_m8abs_r8(addr, sreg) \
do { OP1(0x88); MODRM_MABS(sreg, addr); } while (0)

#define _mov_m8bd_r8(base, disp, sreg) \
do { OP1(0x88); MODRM_MBD(sreg, base, disp); } while (0)

#define _mov_m8isd_r8(indx, scale, disp, sreg) \
do { OP1(0x88); MODRM_MBISD(sreg, NO_BASE, indx, scale, disp); } while (0)

#define _mov_m8bisd_r8(base, indx, scale, disp, sreg) \
do { OP1(0x88); MODRM_MBISD(sreg, base, indx, scale, disp); } while (0)



#define _mov_m16abs_imm(addr, imm) \
do { OP1(0x66); OP1(0xc7); MODRM_MABS(0, addr); OP2(imm); } while (0)

#define _mov_m16abs_r16(addr, sreg) \
do { OP1(0x66); OP1(0x89); MODRM_MABS(sreg, addr); } while (0)

#define _mov_m16bd_r16(base, disp, sreg) \
do { OP1(0x66); OP1(0x89); MODRM_MBD(sreg, base, disp); } while (0)

#define _mov_m16bisd_r16(base, indx, scale, disp, sreg) \
do { OP1(0x66); OP1(0x89); MODRM_MBISD(sreg, base, indx, scale, disp); } while (0)




#define _mov_m32abs_imm(addr, imm) \
do { OP1(0xc7); MODRM_MABS(0, addr); OP4(imm); } while (0)

#define _mov_m32bd_imm(base, disp, imm) \
do { OP1(0xc7); MODRM_MBD(0, base, disp); OP4(imm); } while (0)

#define _mov_m32bisd_imm(base, indx, scale, addr, imm) \
do { OP1(0xc7); MODRM_MBISD(0, base, indx, scale, addr); OP4(imm); } while (0)



#define _mov_m32abs_r32(addr, sreg) \
do { OP1(0x89); MODRM_MABS(sreg, addr); } while (0)

#define _mov_m32bd_r32(base, disp, sreg) \
do { OP1(0x89); MODRM_MBD(sreg, base, disp); } while (0)

#define _mov_m32isd_r32(indx, scale, addr, dreg) \
do { OP1(0x89); MODRM_MBISD(dreg, NO_BASE, indx, scale, addr); } while (0)

#define _mov_m32bisd_r32(base, indx, scale, addr, dreg) \
do { OP1(0x89); MODRM_MBISD(dreg, base, indx, scale, addr); } while (0)



#define _mov_r64_m64abs(reghi, reglo, addr) \
do { _mov_r32_m32abs(reglo, LO(addr)); _mov_r32_m32abs(reghi, HI(addr)); } while (0)

#define _mov_m64abs_r64(addr, reghi, reglo) \
do { _mov_m32abs_r32(LO(addr), reglo); _mov_m32abs_r32(HI(addr), reghi); } while (0)

#define _mov_m64abs_imm32(addr, imm) \
do { _mov_m32abs_imm(LO(addr), imm); _mov_m32abs_imm(HI(addr), ((INT32)(imm) >> 31)); } while (0)



#define _mov_r64_m64bd(reghi, reglo, base, disp) \
do { _mov_r32_m32bd(reglo, base, disp); _mov_r32_m32bd(reghi, base, (UINT8 *)(disp) + 4); } while (0)

#define _mov_m64bd_r64(base, disp, reghi, reglo) \
do { _mov_m32bd_r32(base, disp, reglo); _mov_m32bd_r32(base, (UINT8 *)(disp) + 4, reghi); } while (0)

#define _mov_m64bd_imm32(base, disp, imm) \
do { _mov_m32bd_imm(base, disp, imm); _mov_m32bd_imm(base, (UINT8 *)(disp) + 4, ((INT32)(imm) >> 31)); } while (0)



#define _movsx_r32_r8(dreg, sreg) \
do { OP1(0x0f); OP1(0xbe); MODRM_REG(dreg, sreg); } while (0)

#define _movsx_r32_r16(dreg, sreg) \
do { OP1(0x0f); OP1(0xbf); MODRM_REG(dreg, sreg); } while (0)

#define _movsx_r32_m8abs(dreg, addr) \
do { OP1(0x0f); OP1(0xbe); MODRM_MABS(dreg, addr); } while (0)

#define _movsx_r32_m16abs(dreg, addr) \
do { OP1(0x0f); OP1(0xbf); MODRM_MABS(dreg, addr); } while (0)

#define _movsx_r32_m8bd(dreg, base, disp) \
do { OP1(0x0f); OP1(0xbe); MODRM_MBD(dreg, base, disp); } while (0)

#define _movsx_r32_m16bd(dreg, base, disp) \
do { OP1(0x0f); OP1(0xbf); MODRM_MBD(dreg, base, disp); } while (0)

#define _movzx_r32_r8(dreg, sreg) \
do { OP1(0x0f); OP1(0xb6); MODRM_REG(dreg, sreg); } while (0)

#define _movzx_r32_r16(dreg, sreg) \
do { OP1(0x0f); OP1(0xb7); MODRM_REG(dreg, sreg); } while (0)

#define _movzx_r32_m8abs(dreg, addr) \
do { OP1(0x0f); OP1(0xb6); MODRM_MABS(dreg, addr); } while (0)

#define _movzx_r32_m16abs(dreg, addr) \
do { OP1(0x0f); OP1(0xb7); MODRM_MABS(dreg, addr); } while (0)

#define _movzx_r32_m8bd(dreg, base, disp) \
do { OP1(0x0f); OP1(0xb6); MODRM_MBD(dreg, base, disp); } while (0)

#define _movzx_r32_m16bd(dreg, base, disp) \
do { OP1(0x0f); OP1(0xb7); MODRM_MBD(dreg, base, disp); } while (0)

#define _movzx_r32_m8isd(dreg, indx, scale, disp) \
do { OP1(0x0f); OP1(0xb6); MODRM_MBISD(dreg, NO_BASE, indx, scale, disp); } while (0)

#define _movzx_r32_m16isd(dreg, indx, scale, disp) \
do { OP1(0x0f); OP1(0xb7); MODRM_MBISD(dreg, NO_BASE, indx, scale, disp); } while (0)

#define _movzx_r32_m8bisd(dreg, base, indx, scale, disp) \
do { OP1(0x0f); OP1(0xb6); MODRM_MBISD(dreg, base, indx, scale, disp); } while (0)

#define _movzx_r32_m16bisd(dreg, base, indx, scale, disp) \
do { OP1(0x0f); OP1(0xb7); MODRM_MBISD(dreg, base, indx, scale, disp); } while (0)



#define _lea_r32_m32bd(dest, base, disp) \
do { OP1(0x8d); MODRM_MBD(dest, base, disp); } while (0)

#define _lea_r32_m32isd(dest, indx, scale, disp) \
do { OP1(0x8d); MODRM_MBISD(dest, NO_BASE, indx, scale, disp); } while (0)

#define _lea_r32_m32bisd(dest, base, indx, scale, disp) \
do { OP1(0x8d); MODRM_MBISD(dest, base, indx, scale, disp); } while (0)



/***************************************************************************
    32-BIT SHIFT EMITTERS
***************************************************************************/

#define _sar_r32_cl(dreg) \
do { OP1(0xd3); MODRM_REG(7, dreg); } while (0)

#define _shl_r32_cl(dreg) \
do { OP1(0xd3); MODRM_REG(4, dreg); } while (0)

#define _shr_r32_cl(dreg) \
do { OP1(0xd3); MODRM_REG(5, dreg); } while (0)

#define _sar_r32_imm(dreg, imm) \
do { \
	if ((imm) == 1) { OP1(0xd1); MODRM_REG(7, dreg); } \
	else { OP1(0xc1); MODRM_REG(7, dreg); OP1(imm); } \
} while (0)

#define _shl_r32_imm(dreg, imm) \
do { \
	if ((imm) == 1) { OP1(0xd1); MODRM_REG(4, dreg); } \
	else { OP1(0xc1); MODRM_REG(4, dreg); OP1(imm); } \
} while (0)

#define _shr_r32_imm(dreg, imm) \
do { \
	if ((imm) == 1) { OP1(0xd1); MODRM_REG(5, dreg); } \
	else { OP1(0xc1); MODRM_REG(5, dreg); OP1(imm); } \
} while (0)

#define _rol_r32_imm(dreg, imm) \
do { \
	if ((imm) == 1) { OP1(0xd1); MODRM_REG(0, dreg); } \
	else { OP1(0xc1); MODRM_REG(0, dreg); OP1(imm); } \
} while (0)

#define _ror_r32_imm(dreg, imm) \
do { \
	if ((imm) == 1) { OP1(0xd1); MODRM_REG(1, dreg); } \
	else { OP1(0xc1); MODRM_REG(1, dreg); OP1(imm); } \
} while (0)



#define _rol_m32abs_imm(addr, imm) \
do { \
	if ((imm) == 1) { OP1(0xd1); MODRM_MABS(0, addr); } \
	else { OP1(0xc1); MODRM_MABS(0, addr); OP1(imm); } \
} while (0)



#define _shld_r32_r32_cl(dreg, sreg) \
do { OP1(0x0f); OP1(0xa5); MODRM_REG(sreg, dreg); } while (0)

#define _shld_r32_r32_imm(dreg, sreg, imm) \
do { OP1(0x0f); OP1(0xa4); MODRM_REG(sreg, dreg); OP1(imm); } while (0)

#define _shrd_r32_r32_cl(dreg, sreg) \
do { OP1(0x0f); OP1(0xad); MODRM_REG(sreg, dreg); } while (0)

#define _shrd_r32_r32_imm(dreg, sreg, imm) \
do { OP1(0x0f); OP1(0xac); MODRM_REG(sreg, dreg); OP1(imm); } while (0)



#define _bt_m32bd_r32(base, disp, reg) \
do { OP1(0x0f); OP1(0xa3); MODRM_MBD(reg, base, disp); } while (0)

#define _bt_m32abs_imm(addr, imm) \
do { OP1(0x0f); OP1(0xba); MODRM_MABS(4, addr); OP1(imm); } while (0)

#define _bt_r32_imm(reg, imm) \
do { OP1(0x0f); OP1(0xba); MODRM_REG(4, reg); OP1(imm); } while (0)

#define _bsr_r32_r32(dreg, sreg) \
do { OP1(0x0f); OP1(0xbd); MODRM_REG(dreg, sreg); } while (0)




/***************************************************************************
    16-BIT SHIFT EMITTERS
***************************************************************************/

#define _sar_r16_cl(dreg) \
do { OP1(0x66); OP1(0xd3); MODRM_REG(7, dreg); } while (0)

#define _shl_r16_cl(dreg) \
do { OP1(0x66); OP1(0xd3); MODRM_REG(4, dreg); } while (0)

#define _shr_r16_cl(dreg) \
do { OP1(0x66); OP1(0xd3); MODRM_REG(5, dreg); } while (0)

#define _sar_r16_imm(dreg, imm) \
do { \
	if ((imm) == 1) { OP1(0x66); OP1(0xd1); MODRM_REG(7, dreg); } \
	else { OP1(0x66); OP1(0xc1); MODRM_REG(7, dreg); OP1(imm); } \
} while (0)

#define _shl_r16_imm(dreg, imm) \
do { \
	if ((imm) == 1) { OP1(0x66); OP1(0xd1); MODRM_REG(4, dreg); } \
	else { OP1(0x66); OP1(0xc1); MODRM_REG(4, dreg); OP1(imm); } \
} while (0)

#define _shr_r16_imm(dreg, imm) \
do { \
	if ((imm) == 1) { OP1(0x66); OP1(0xd1); MODRM_REG(5, dreg); } \
	else { OP1(0x66); OP1(0xc1); MODRM_REG(5, dreg); OP1(imm); } \
} while (0)

#define _rol_r16_imm(dreg, imm) \
do { \
	if ((imm) == 1) { OP1(0x66); OP1(0xd1); MODRM_REG(0, dreg); } \
	else { OP1(0x66); OP1(0xc1); MODRM_REG(0, dreg); OP1(imm); } \
} while (0)

#define _ror_r16_imm(dreg, imm) \
do { \
	if ((imm) == 1) { OP1(0x66); OP1(0xd1); MODRM_REG(1, dreg); } \
	else { OP1(0x66); OP1(0xc1); MODRM_REG(1, dreg); OP1(imm); } \
} while (0)



/***************************************************************************
    UNARY ARITHMETIC EMITTERS
***************************************************************************/

#define _neg_r32(reg) \
do { OP1(0xf7); MODRM_REG(3, reg); } while (0)

#define _not_r32(reg) \
do { OP1(0xf7); MODRM_REG(2, reg); } while (0)



/***************************************************************************
    32-BIT ARITHMETIC EMITTERS
***************************************************************************/

#define _add_r32_r32(r1, r2) \
do { OP1(0x01); MODRM_REG(r2, r1); } while (0)

#define _adc_r32_r32(r1, r2) \
do { OP1(0x11); MODRM_REG(r2, r1); } while (0)

#define _or_r32_r32(r1, r2) \
do { OP1(0x09); MODRM_REG(r2, r1); } while (0)

#define _sub_r32_r32(r1, r2) \
do { OP1(0x29); MODRM_REG(r2, r1); } while (0)

#define _sbb_r32_r32(r1, r2) \
do { OP1(0x19); MODRM_REG(r2, r1); } while (0)

#define _xor_r32_r32(r1, r2) \
do { OP1(0x31); MODRM_REG(r2, r1); } while (0)

#define _cmp_r32_r32(r1, r2) \
do { OP1(0x39); MODRM_REG(r2, r1); } while (0)

#define _test_r32_r32(r1, r2) \
do { OP1(0x85); MODRM_REG(r2, r1); } while (0)



#define _add_r32_m32abs(dreg, addr) \
do { OP1(0x03); MODRM_MABS(dreg, addr); } while (0)

#define _adc_r32_m32abs(dreg, addr) \
do { OP1(0x13); MODRM_MABS(dreg, addr); } while (0)

#define _and_r32_m32abs(dreg, addr) \
do { OP1(0x23); MODRM_MABS(dreg, addr); } while (0)

#define _cmp_r32_m32abs(dreg, addr) \
do { OP1(0x3b); MODRM_MABS(dreg, addr); } while (0)

#define _or_r32_m32abs(dreg, addr) \
do { OP1(0x0b); MODRM_MABS(dreg, addr); } while (0)

#define _sub_r32_m32abs(dreg, addr) \
do { OP1(0x2b); MODRM_MABS(dreg, addr); } while (0)

#define _sbb_r32_m32abs(dreg, addr) \
do { OP1(0x1b); MODRM_MABS(dreg, addr); } while (0)

#define _xor_r32_m32abs(dreg, addr) \
do { OP1(0x33); MODRM_MABS(dreg, addr); } while (0)



#define _add_r32_m32bd(dreg, base, disp) \
do { OP1(0x03); MODRM_MBD(dreg, base, disp); } while (0)

#define _adc_r32_m32bd(dreg, base, disp) \
do { OP1(0x13); MODRM_MBD(dreg, base, disp); } while (0)

#define _and_r32_m32bd(dreg, base, disp) \
do { OP1(0x23); MODRM_MBD(dreg, base, disp); } while (0)

#define _cmp_r32_m32bd(dreg, base, disp) \
do { OP1(0x3b); MODRM_MBD(dreg, base, disp); } while (0)

#define _or_r32_m32bd(dreg, base, disp) \
do { OP1(0x0b); MODRM_MBD(dreg, base, disp); } while (0)

#define _sub_r32_m32bd(dreg, base, disp) \
do { OP1(0x2b); MODRM_MBD(dreg, base, disp); } while (0)

#define _sbb_r32_m32bd(dreg, base, disp) \
do { OP1(0x1b); MODRM_MBD(dreg, base, disp); } while (0)

#define _xor_r32_m32bd(dreg, base, disp) \
do { OP1(0x33); MODRM_MBD(dreg, base, disp); } while (0)

#define _imul_r32_m32bd(dreg, base, disp) \
do { OP1(0x0f); OP1(0xaf); MODRM_MBD(dreg, base, disp); } while (0)



#define _add_m32abs_r32(addr, sreg) \
do { OP1(0x01); MODRM_MABS(sreg, addr); } while (0)

#define _or_m32abs_r32(addr, sreg) \
do { OP1(0x09); MODRM_MABS(sreg, addr); } while (0)


#define _arith_r32_imm_common(reg, dreg, imm)		\
do {												\
	if ((INT8)(imm) == (INT32)(imm))				\
	{												\
		OP1(0x83); MODRM_REG(reg, dreg); OP1(imm);	\
	}												\
	else											\
	{												\
		OP1(0x81); MODRM_REG(reg, dreg); OP4(imm);	\
	}												\
} while (0)

#define _add_r32_imm(dreg, imm) \
do { if ((imm) == 1) OP1(0x40 + dreg); else _arith_r32_imm_common(0, dreg, imm); } while (0)

#define _adc_r32_imm(dreg, imm) \
do { _arith_r32_imm_common(2, dreg, imm); } while (0)

#define _or_r32_imm(dreg, imm) \
do { _arith_r32_imm_common(1, dreg, imm); } while (0)

#define _sbb_r32_imm(dreg, imm) \
do { _arith_r32_imm_common(3, dreg, imm); } while (0)

#define _and_r32_imm(dreg, imm) \
do { _arith_r32_imm_common(4, dreg, imm); } while (0)

#define _sub_r32_imm(dreg, imm) \
do { _arith_r32_imm_common(5, dreg, imm); } while (0)

#define _sub_or_dec_r32_imm(dreg, imm) \
do { if ((imm) == 1) OP1(0x48 + dreg); else _arith_r32_imm_common(5, dreg, imm); } while (0)

#define _sub_or_dec_m32abs_imm(addr, imm) \
do { if ((imm) == 1) {OP1(0xff); MODRM_MABS(1, addr); } else _arith_m32abs_imm_common(5, addr, imm); } while (0)

#define _xor_r32_imm(dreg, imm) \
do { _arith_r32_imm_common(6, dreg, imm); } while (0)

#define _cmp_r32_imm(dreg, imm) \
do { _arith_r32_imm_common(7, dreg, imm); } while (0)

#define _test_r32_imm(dreg, imm) \
do { OP1(0xf7); MODRM_REG(0, dreg); OP4(imm); } while (0)

#define _and_r32_r32(dreg, sreg) \
do { OP1(0x23); MODRM_REG(dreg, sreg); } while (0)

#define _imul_r32_r32(dreg, sreg) \
do { OP1(0x0f); OP1(0xaf); MODRM_REG(dreg, sreg); } while (0)



#define _arith_m32abs_imm_common(reg, addr, imm)	\
do {												\
	if ((INT8)(imm) == (INT32)(imm))				\
	{												\
		OP1(0x83); MODRM_MABS(reg, addr); OP1(imm);	\
	}												\
	else											\
	{												\
		OP1(0x81); MODRM_MABS(reg, addr); OP4(imm);	\
	}												\
} while (0)

#define _add_m32abs_imm(addr, imm) \
do { _arith_m32abs_imm_common(0, addr, imm); } while (0)

#define _adc_m32abs_imm(addr, imm) \
do { _arith_m32abs_imm_common(2, addr, imm); } while (0)

#define _or_m32abs_imm(addr, imm) \
do { _arith_m32abs_imm_common(1, addr, imm); } while (0)

#define _sbb_m32abs_imm(addr, imm) \
do { _arith_m32abs_imm_common(3, addr, imm); } while (0)

#define _and_m32abs_imm(addr, imm) \
do { _arith_m32abs_imm_common(4, addr, imm); } while (0)

#define _sub_m32abs_imm(addr, imm) \
do { _arith_m32abs_imm_common(5, addr, imm); } while (0)

#define _xor_m32abs_imm(addr, imm) \
do { _arith_m32abs_imm_common(6, addr, imm); } while (0)

#define _cmp_m32abs_imm(addr, imm) \
do { _arith_m32abs_imm_common(7, addr, imm); } while (0)

#define _test_m32abs_imm(addr, imm) \
do { OP1(0xf7); MODRM_MABS(0, addr); OP4(imm); } while (0)



#define _arith_m32bd_imm_common(reg, base, disp, imm)	\
do {												\
	if ((INT8)(imm) == (INT32)(imm))				\
	{												\
		OP1(0x83); MODRM_MBD(reg, base, disp); OP1(imm);\
	}												\
	else											\
	{												\
		OP1(0x81); MODRM_MBD(reg, base, disp); OP4(imm);\
	}												\
} while (0)

#define _add_m32bd_imm(base, disp, imm) \
do { _arith_m32bd_imm_common(0, base, disp, imm); } while (0)

#define _adc_m32bd_imm(base, disp, imm) \
do { _arith_m32bd_imm_common(2, base, disp, imm); } while (0)

#define _or_m32bd_imm(base, disp, imm) \
do { _arith_m32bd_imm_common(1, base, disp, imm); } while (0)

#define _sbb_m32bd_imm(base, disp, imm) \
do { _arith_m32bd_imm_common(3, base, disp, imm); } while (0)

#define _and_m32bd_imm(base, disp, imm) \
do { _arith_m32bd_imm_common(4, base, disp, imm); } while (0)

#define _sub_m32bd_imm(base, disp, imm) \
do { _arith_m32bd_imm_common(5, base, disp, imm); } while (0)

#define _xor_m32bd_imm(base, disp, imm) \
do { _arith_m32bd_imm_common(6, base, disp, imm); } while (0)

#define _cmp_m32bd_imm(base, disp, imm) \
do { _arith_m32bd_imm_common(7, base, disp, imm); } while (0)

#define _test_m32bd_imm(base, disp, imm) \
do { OP1(0xf7); MODRM_MBD(0, base, disp); OP4(imm); } while (0)



#define _and_r32_m32bd(dreg, base, disp) \
do { OP1(0x23); MODRM_MBD(dreg, base, disp); } while (0)

#define _add_r32_m32bd(dreg, base, disp) \
do { OP1(0x03); MODRM_MBD(dreg, base, disp); } while (0)



#define _imul_r32(reg) \
do { OP1(0xf7); MODRM_REG(5, reg); } while (0)

#define _mul_r32(reg) \
do { OP1(0xf7); MODRM_REG(4, reg); } while (0)

#define _mul_m32abs(addr) \
do { OP1(0xf7); MODRM_MABS(4, addr); } while (0)

#define _mul_m32bd(base, disp) \
do { OP1(0xf7); MODRM_MBD(4, base, disp); } while (0)

#define _idiv_r32(reg) \
do { OP1(0xf7); MODRM_REG(7, reg); } while (0)

#define _div_r32(reg) \
do { OP1(0xf7); MODRM_REG(6, reg); } while (0)

#define _and_m32bd_r32(base, disp, sreg) \
do { OP1(0x21); MODRM_MBD(sreg, base, disp); } while (0)

#define _add_m32bd_r32(base, disp, sreg) \
do { OP1(0x89); MODRM_MBD(sreg, base, disp); } while (0)

#define _sub_m32bd_r32(base, disp, sreg) \
do { OP1(0x29); MODRM_MBD(sreg, base, disp); } while (0)

#define _rol_r32_cl(reg) \
do { OP1(0xd3);	OP1(0xc0 | ((reg) & 7)); } while(0)

#define _ror_r32_cl(reg) \
do { OP1(0xd3);	OP1(0xc8 | ((reg) & 7)); } while(0)

#define _rcl_r32_cl(reg) \
do { OP1(0xd3);	OP1(0xd0 | ((reg) & 7)); } while(0)

#define _rcr_r32_cl(reg) \
do { OP1(0xd3);	OP1(0xd8 | ((reg) & 7)); } while(0)




/***************************************************************************
    16-BIT ARITHMETIC EMITTERS
***************************************************************************/

#define _add_r16_r16(r1, r2) \
do { OP1(0x66); OP1(0x03); MODRM_REG(r2, r1); } while (0)

#define _adc_r16_r16(r1, r2) \
do { OP1(0x66); OP1(0x11); MODRM_REG(r2, r1); } while (0)

#define _or_r16_r16(r1, r2) \
do { OP1(0x66); OP1(0x09); MODRM_REG(r2, r1); } while (0)

#define _sub_r16_r16(r1, r2) \
do { OP1(0x66); OP1(0x29); MODRM_REG(r2, r1); } while (0)

#define _sbb_r16_r16(r1, r2) \
do { OP1(0x66); OP1(0x19); MODRM_REG(r2, r1); } while (0)

#define _xor_r16_r16(r1, r2) \
do { OP1(0x66); OP1(0x31); MODRM_REG(r2, r1); } while (0)

#define _cmp_r16_r16(r1, r2) \
do { OP1(0x66); OP1(0x39); MODRM_REG(r2, r1); } while (0)

#define _test_r16_r16(r1, r2) \
do { OP1(0x66); OP1(0x85); MODRM_REG(r2, r1); } while (0)



#define _arith_r16_imm_common(reg, dreg, imm)		\
do {												\
	if ((INT8)(imm) == (INT16)(imm))				\
	{												\
		OP1(0x66); OP1(0x83); MODRM_REG(reg, dreg); OP1(imm);	\
	}												\
	else											\
	{												\
		OP1(0x66); OP1(0x81); MODRM_REG(reg, dreg); OP2(imm);	\
	}												\
} while (0)

#define _add_r16_imm(dreg, imm) \
do { if ((imm) == 1) { OP1(0x66); OP1(0x40 + dreg); } else _arith_r16_imm_common(0, dreg, imm); } while (0)

#define _adc_r16_imm(dreg, imm) \
do { _arith_r16_imm_common(2, dreg, imm); } while (0)

#define _or_r16_imm(dreg, imm) \
do { _arith_r16_imm_common(1, dreg, imm); } while (0)

#define _sbb_r16_imm(dreg, imm) \
do { _arith_r16_imm_common(3, dreg, imm); } while (0)

#define _and_r16_imm(dreg, imm) \
do { _arith_r16_imm_common(4, dreg, imm); } while (0)

#define _sub_r16_imm(dreg, imm) \
do { _arith_r16_imm_common(5, dreg, imm); } while (0)

#define _sub_or_dec_r16_imm(dreg, imm) \
do { if ((imm) == 1) OP1(0x48 + dreg); else _arith_r16_imm_common(5, dreg, imm); } while (0)

#define _xor_r16_imm(dreg, imm) \
do { _arith_r16_imm_common(6, dreg, imm); } while (0)

#define _cmp_r16_imm(dreg, imm) \
do { _arith_r16_imm_common(7, dreg, imm); } while (0)



/***************************************************************************
    16-BIT AND 8-BIT ARITHMETIC EMITTERS
***************************************************************************/

#define _or_r8_r8(r1, r2) \
do { OP1(0x08); MODRM_REG(r2, r1); } while (0)

#define _arith_m16abs_imm_common(reg, addr, imm)	\
do {												\
	OP1(0x66);										\
	if ((INT8)(imm) == (INT16)(imm))				\
	{												\
		OP1(0x83); MODRM_MABS(reg, addr); OP1(imm);	\
	}												\
	else											\
	{												\
		OP1(0x81); MODRM_MABS(reg, addr); OP2(imm);	\
	}												\
} while (0)

#define _add_m16abs_imm(addr, imm) \
do { _arith_m16abs_imm_common(0, addr, imm); } while (0)

#define _or_m16abs_imm(addr, imm) \
do { _arith_m16abs_imm_common(1, addr, imm); } while (0)

#define _sbb_m16abs_imm(addr, imm) \
do { _arith_m16abs_imm_common(3, addr, imm); } while (0)

#define _and_m16abs_imm(addr, imm) \
do { _arith_m16abs_imm_common(4, addr, imm); } while (0)

#define _sub_m16abs_imm(addr, imm) \
do { _arith_m16abs_imm_common(5, addr, imm); } while (0)

#define _xor_m16abs_imm(addr, imm) \
do { _arith_m16abs_imm_common(6, addr, imm); } while (0)

#define _cmp_m16abs_imm(addr, imm) \
do { _arith_m16abs_imm_common(7, addr, imm); } while (0)

#define _test_m16abs_imm(addr, imm) \
do { OP1(0xf7); MODRM_MABS(0, addr); OP2(imm); } while (0)

#define _and_m16bd_r16(base, disp, sreg) \
do { OP1(0x66); OP1(0x21); MODRM_MBD(sreg, base, disp); } while (0)

#define _and_r16_m16abs(dreg, addr) \
do { OP1(0x66); OP1(0x25); MODRM_MABS(dreg, addr); } while (0)

#define _shl_r16_cl(dreg) \
do { OP1(0x66); OP1(0xd3); MODRM_REG(4, dreg); } while (0)

#define _shr_r16_cl(dreg) \
do { OP1(0x66); OP1(0xd3); MODRM_REG(5, dreg); } while (0)

#define _rol_r16_cl(reg) \
do { OP1(0x66); OP1(0xd3); OP1(0xc0 | ((reg) & 7)); } while(0)

#define _ror_r16_cl(reg) \
do { OP1(0x66); OP1(0xd3); OP1(0xc8 | ((reg) & 7)); } while(0)

#define _rcl_r16_cl(reg) \
do { OP1(0x66); OP1(0xd3); OP1(0xd0 | ((reg) & 7)); } while(0)

#define _rcr_r16_cl(reg) \
do { OP1(0x66); OP1(0xd3);	OP1(0xd8 | ((reg) & 7)); } while(0)

#define _cmp_r16_m16bisd(reg, base, indx, scale, disp) \
do { OP1(0x66); OP1(0x3b);	MODRM_MBISD(reg, base, indx, scale, disp); } while(0)



#define _arith_m8abs_imm_common(reg, addr, imm)		\
do { OP1(0x80); MODRM_MABS(reg, addr); OP1(imm); } while (0)

#define _add_m8abs_imm(addr, imm) \
do { _arith_m8abs_imm_common(0, addr, imm); } while (0)

#define _or_m8abs_imm(addr, imm) \
do { _arith_m8abs_imm_common(1, addr, imm); } while (0)

#define _adc_m8abs_imm(addr, imm) \
do { _arith_m8abs_imm_common(2, addr, imm); } while (0)

#define _sbb_m8abs_imm(addr, imm) \
do { _arith_m8abs_imm_common(3, addr, imm); } while (0)

#define _and_m8abs_imm(addr, imm) \
do { _arith_m8abs_imm_common(4, addr, imm); } while (0)

#define _sub_m8abs_imm(addr, imm) \
do { _arith_m8abs_imm_common(5, addr, imm); } while (0)

#define _xor_m8abs_imm(addr, imm) \
do { _arith_m8abs_imm_common(6, addr, imm); } while (0)

#define _cmp_m8abs_imm(addr, imm) \
do { _arith_m8abs_imm_common(7, addr, imm); } while (0)

#define _test_m8abs_imm(addr, imm) \
do { OP1(0xf6); MODRM_MABS(0, addr); OP1(imm); } while (0)

#define _test_r8_imm(reg, imm) \
do { OP1(0xf6); MODRM_REG(0, reg); OP1(imm); } while (0)

#define _and_m8bd_r8(base, disp, sreg) \
do { OP1(0x20); MODRM_MBD(sreg, base, disp); } while (0)

#define _shl_r8_imm(dreg, imm) \
do { \
	if ((imm) == 1) { OP1(0xd0); MODRM_REG(4, dreg); } \
	else { OP1(0xc0); MODRM_REG(4, dreg); OP1(imm); } \
} while (0)

#define _shr_r8_imm(dreg, imm) \
do { \
	if ((imm) == 1) { OP1(0xd0); MODRM_REG(5, dreg); } \
	else { OP1(0xc0); MODRM_REG(5, dreg); OP1(imm); } \
} while (0)

#define _shl_r8_cl(dreg) \
do { OP1(0xd2); MODRM_REG(4, dreg); } while (0)

#define _shr_r8_cl(dreg) \
do { OP1(0xd2); MODRM_REG(5, dreg); } while (0)

#define _rol_r8_cl(reg) \
do { OP1(0xd2);	OP1(0xc0 | ((reg) & 7)); } while(0)

#define _ror_r8_cl(reg) \
do { OP1(0xd2);	OP1(0xc8 | ((reg) & 7)); } while(0)

#define _rcl_r8_cl(reg) \
do { OP1(0xd2);	OP1(0xd0 | ((reg) & 7)); } while(0)

#define _rcr_r8_cl(reg) \
do { OP1(0xd2);	OP1(0xd8 | ((reg) & 7)); } while(0)



/***************************************************************************
    FLOATING POINT EMITTERS
***************************************************************************/

#define _fnclex() \
do { OP1(0xdb); OP1(0xe2); } while (0)

#define _fnstsw_ax() \
do { OP1(0xdf); OP1(0xe0); } while (0)

#define _fldcw_m16abs(addr) \
do { OP1(0xd9); MODRM_MABS(5, addr); } while (0)

#define _fldcw_m16isd(indx, scale, addr) \
do { OP1(0xd9); MODRM_MBISD(5, NO_BASE, indx, scale, addr); } while (0)

#define _fnstcw_m16abs(addr) \
do { OP1(0xd9); MODRM_MABS(7, addr); } while (0)



#define _fabs() \
do { OP1(0xd9); OP1(0xe1); } while (0)

#define _faddp() \
do { OP1(0xde); OP1(0xc1); } while (0)

#define _fchs() \
do { OP1(0xd9); OP1(0xe0); } while (0)

#define _fcompp() \
do { OP1(0xde); OP1(0xd9); } while (0)

#define _fdivp() \
do { OP1(0xde); OP1(0xf9); } while (0)

#define _fmulp() \
do { OP1(0xde); OP1(0xc9); } while (0)

#define _fsqrt() \
do { OP1(0xd9); OP1(0xfa); } while (0)

#define _fsubp() \
do { OP1(0xde); OP1(0xe9); } while (0)

#define _fsubrp() \
do { OP1(0xde); OP1(0xe1); } while (0)

#define _fucompp() \
do { OP1(0xda); OP1(0xe9); } while (0)



#define _fld1() \
do { OP1(0xd9); OP1(0xe8); } while (0)

#define _fld_m32abs(addr) \
do { OP1(0xd9); MODRM_MABS(0, addr); } while (0)

#define _fld_m64abs(addr) \
do { OP1(0xdd); MODRM_MABS(0, addr); } while (0)

#define _fild_m32abs(addr) \
do { OP1(0xdb); MODRM_MABS(0, addr); } while (0)

#define _fild_m64abs(addr) \
do { OP1(0xdf); MODRM_MABS(5, addr); } while (0)



#define _fistp_m32abs(addr) \
do { OP1(0xdb); MODRM_MABS(3, addr); } while (0)

#define _fistp_m64abs(addr) \
do { OP1(0xdf); MODRM_MABS(7, addr); } while (0)

#define _fstp_m32abs(addr) \
do { OP1(0xd9); MODRM_MABS(3, addr); } while (0)

#define _fstp_m64abs(addr) \
do { OP1(0xdd); MODRM_MABS(3, addr); } while (0)



/***************************************************************************
    BRANCH EMITTERS
***************************************************************************/

#define _setcc_r8(cond, dreg) \
do { OP1(0x0f); OP1(0x90 + cond); MODRM_REG(0, dreg); } while (0)

#define _setcc_m8abs(cond, addr) \
do { OP1(0x0f); OP1(0x90 + cond); MODRM_MABS(0, addr); } while (0)


#define _jcc_short_link(cond, link) \
do { OP1(0x70 + (cond)); OP1(0x00); (link)->target = drc->cache_top; (link)->size = 1; } while (0)

#define _jcc_near_link(cond, link) \
do { OP1(0x0f); OP1(0x80 + (cond)); OP4(0x00); (link)->target = drc->cache_top; (link)->size = 4; } while (0)

#define _jcc(cond, target) 									\
do {														\
	INT32 delta = (UINT8 *)(target) - (drc->cache_top + 2);	\
	if ((INT8)delta == (INT32)delta)						\
	{														\
		OP1(0x70 + (cond));	OP1(delta);						\
	}														\
	else													\
	{														\
		delta = (UINT8 *)(target) - (drc->cache_top + 6);	\
		OP1(0x0f); OP1(0x80 + (cond)); OP4(delta);			\
	}														\
} while (0)



#define _jmp_short_link(link) \
do { OP1(0xeb); OP1(0x00); (link)->target = drc->cache_top; (link)->size = 1; } while (0)

#define _jmp_near_link(link) \
do { OP1(0xe9); OP4(0x00); (link)->target = drc->cache_top; (link)->size = 4; } while (0)

#define _jmp(target) \
do { OP1(0xe9); OP4((UINT32)(target) - ((UINT32)drc->cache_top + 4)); } while (0)

#define _jmp_r32(reg) \
do { OP1(0xff); MODRM_REG(4, reg); } while (0)



#define _call(target) \
do { OP1(0xe8); OP4((UINT32)(target) - ((UINT32)drc->cache_top + 4)); } while (0)



#define _jmp_m32abs(addr) \
do { OP1(0xff); MODRM_MABS(4, addr); } while (0)

#define _jmp_m32bd(base, disp) \
do { OP1(0xff); MODRM_MBD(4, base, disp); } while (0)

#define _jmp_m32bisd(base, indx, scale, disp) \
do { OP1(0xff); MODRM_MBISD(4, base, indx, scale, disp); } while (0)



#define _resolve_link(link)							\
do {												\
	INT32 delta = drc->cache_top - (link)->target;	\
	if ((link)->size == 1)							\
	{												\
		if ((INT8)delta != delta)					\
			fatalerror("Error: link out of range!\n");	\
		(link)->target[-1] = delta;					\
	}												\
	else if ((link)->size == 4)						\
		*(UINT32 *)&(link)->target[-4] = delta;		\
	else											\
		fatalerror("Unsized link!\n");					\
} while (0)



/***************************************************************************
    SSE EMITTERS
***************************************************************************/

#define _ldmxcsr_m32abs(addr) \
do { OP1(0x0f); OP1(0xae); MODRM_MABS(2, addr); } while (0)

#define _ldmxcsr_m32isd(indx, scale, disp) \
do { OP1(0x0f); OP1(0xae); MODRM_MBISD(2, NO_BASE, indx, scale, disp); } while (0)

#define _stmxcsr_m32abs(addr) \
do { OP1(0x0f); OP1(0xae); MODRM_MABS(3, addr); } while (0)


#define _movd_mmx_r32(r1, r2) \
do { OP1(0x0f); OP1(0x6e); MODRM_REG(r1, r2); } while (0)

#define _movd_r32_mmx(r1, r2) \
do { OP1(0x0f); OP1(0x7e); MODRM_REG(r1, r2); } while (0)

#define _movd_mmx_m32bd(reg, base, disp) \
do { OP1(0x0f); OP1(0x6e); MODRM_MBD(reg, base, disp); } while (0)

#define _movd_mmx_m32bisd(reg, base, indx, scale, disp) \
do { OP1(0x0f); OP1(0x6e); MODRM_MBISD(reg, base, indx, scale, disp); } while (0)

#define _movd_r128_r32(r1, r2) \
do { OP1(0x66); OP1(0x0f); OP1(0x6e); MODRM_REG(r1, r2); } while (0)

#define _movd_r32_r128(r1, r2) \
do { OP1(0x66); OP1(0x0f); OP1(0x7e); MODRM_REG(r2, r1); } while (0)

#define _movd_r128_m32abs(reg, addr) \
do { OP1(0x66); OP1(0x0f); OP1(0x6e); MODRM_MABS(reg, addr); } while (0)

#define _movd_r128_m32isd(reg, indx, scale, disp) \
do { OP1(0x66); OP1(0x0f); OP1(0x6e); MODRM_MBISD(reg, NO_BASE, indx, scale, disp); } while (0)


#define _movq_r128_m64abs(reg, addr) \
do { OP1(0xf3); OP1(0x0f); OP1(0x7e); MODRM_MABS(reg, addr); } while (0)

#define _movq_r128_m64isd(reg, indx, scale, disp) \
do { OP1(0xf3); OP1(0x0f); OP1(0x7e); MODRM_MBISD(reg, NO_BASE, indx, scale, disp); } while (0)

#define _movq_r128_m64bisd(reg, base, indx, scale, disp) \
do { OP1(0xf3); OP1(0x0f); OP1(0x7e); MODRM_MBISD(reg, base, indx, scale, disp); } while (0)

#define _movq_m64abs_r128(addr, reg) \
do { OP1(0x66); OP1(0x0f); OP1(0xd6); MODRM_MABS(reg, addr); } while (0)


#define _movdqa_r128_r128(r1, r2) \
do { OP1(0x66); OP1(0x0f); OP1(0x6f); MODRM_REG(r1, r2); } while (0)

#define _movdqa_r128_m128abs(reg, addr) \
do { OP1(0x66); OP1(0x0f); OP1(0x6f); MODRM_MABS(reg, addr); } while (0)

#define _movdqa_m128abs_r128(addr, reg) \
do { OP1(0x66); OP1(0x0f); OP1(0x7f); MODRM_MABS(reg, addr); } while (0)


#define _movaps_r128_m128abs(reg, addr) \
do { OP1(0x0f); OP1(0x28); MODRM_MABS(reg, addr); } while (0)

#define _movaps_m128abs_r128(addr, reg) \
do { OP1(0x0f); OP1(0x29); MODRM_MABS(reg, addr); } while (0)


#define _movss_r128_m32abs(reg, addr) \
do { OP1(0xf3); OP1(0x0f); OP1(0x10); MODRM_MABS(reg, addr); } while (0)

#define _movss_r128_m32bd(reg, base, disp) \
do { OP1(0xf3); OP1(0x0f); OP1(0x10); MODRM_MBD(reg, base, disp); } while (0)

#define _movss_r128_r128(r1, r2) \
do { OP1(0xf3); OP1(0x0f); OP1(0x10); MODRM_REG(r1, r2); } while (0)

#define _movss_m32abs_r128(addr, reg) \
do { OP1(0xf3); OP1(0x0f); OP1(0x11); MODRM_MABS(reg, addr); } while (0)


#define _movsd_r128_m64abs(reg, addr) \
do { OP1(0xf2); OP1(0x0f); OP1(0x10); MODRM_MABS(reg, addr); } while (0)

#define _movsd_r128_m64bd(reg, base, disp) \
do { OP1(0xf2); OP1(0x0f); OP1(0x10); MODRM_MBD(reg, base, disp); } while (0)

#define _movsd_r128_r128(r1, r2) \
do { OP1(0xf2); OP1(0x0f); OP1(0x10); MODRM_REG(r1, r2); } while (0)

#define _movsd_m64abs_r128(addr, reg) \
do { OP1(0xf2); OP1(0x0f); OP1(0x11); MODRM_MABS(reg, addr); } while (0)

#define _movsd_m64bd_r128(base, disp, reg) \
do { OP1(0xf2); OP1(0x0f); OP1(0x11); MODRM_MBD(reg, base, disp); } while (0)



#define _addps_r128_m128abs(reg, addr) \
do { OP1(0x0f); OP1(0x58); MODRM_MABS(reg, addr); } while (0)

#define _addss_r128_m32abs(reg, addr) \
do { OP1(0xf3); OP1(0x0f); OP1(0x58); MODRM_MABS(reg, addr); } while (0)

#define _addss_r128_m32bd(reg, base, disp) \
do { OP1(0xf3); OP1(0x0f); OP1(0x58); MODRM_MBD(reg, base, disp); } while (0)

#define _addsd_r128_m64abs(reg, addr) \
do { OP1(0xf2); OP1(0x0f); OP1(0x58); MODRM_MABS(reg, addr); } while (0)

#define _andps_r128_m128abs(reg, addr) \
do { OP1(0x0f); OP1(0x54); MODRM_MABS(reg, addr); } while (0)

#define _andpd_r128_m128abs(reg, addr) \
do { OP1(0x66); OP1(0x0f); OP1(0x54); MODRM_MABS(reg, addr); } while (0)

#define _comiss_r128_m32abs(reg, addr) \
do { OP1(0x0f); OP1(0x2f); MODRM_MABS(reg, addr); } while (0)

#define _comisd_r128_m64abs(reg, addr) \
do { OP1(0x66); OP1(0x0f); OP1(0x2f); MODRM_MABS(reg, addr); } while (0)

#define _cvtsi2ss_r128_m32abs(reg, addr) \
do { OP1(0xf3); OP1(0x0f); OP1(0x2a); MODRM_MABS(reg, addr); } while (0)

#define _cvttss2si_r32_m32abs(reg, addr) \
do { OP1(0xf3); OP1(0x0f); OP1(0x2c); MODRM_MABS(reg, addr); } while (0)

#define _cvttsd2si_r32_m64abs(reg, addr) \
do { OP1(0xf2); OP1(0x0f); OP1(0x2c); MODRM_MABS(reg, addr); } while (0)

#define _divss_r128_m32abs(reg, addr) \
do { OP1(0xf3); OP1(0x0f); OP1(0x5e); MODRM_MABS(reg, addr); } while (0)

#define _divsd_r128_m64abs(reg, addr) \
do { OP1(0xf2); OP1(0x0f); OP1(0x5e); MODRM_MABS(reg, addr); } while (0)

#define _mulss_r128_m32abs(reg, addr) \
do { OP1(0xf3); OP1(0x0f); OP1(0x59); MODRM_MABS(reg, addr); } while (0)

#define _mulss_r128_m32bd(reg, base, disp) \
do { OP1(0xf3); OP1(0x0f); OP1(0x59); MODRM_MBD(reg, base, disp); } while (0)

#define _mulsd_r128_m64abs(reg, addr) \
do { OP1(0xf2); OP1(0x0f); OP1(0x59); MODRM_MABS(reg, addr); } while (0)

#define _mulps_r128_m128abs(reg, addr) \
do { OP1(0x0f); OP1(0x59); MODRM_MABS(reg, addr); } while (0)

#define _orps_r128_m128abs(reg, addr) \
do { OP1(0x0f); OP1(0x56); MODRM_MABS(reg, addr); } while (0)

#define _orpd_r128_m128abs(reg, addr) \
do { OP1(0x66); OP1(0x0f); OP1(0x56); MODRM_MABS(reg, addr); } while (0)

#define _rcpss_r128_m32abs(reg, addr) \
do { OP1(0xf3); OP1(0x0f); OP1(0x53); MODRM_MABS(reg, addr); } while (0)

#define _rsqrtss_r128_m32abs(reg, addr) \
do { OP1(0xf3); OP1(0x0f); OP1(0x52); MODRM_MABS(reg, addr); } while (0)

#define _sqrtss_r128_m32abs(reg, addr) \
do { OP1(0xf3); OP1(0x0f); OP1(0x51); MODRM_MABS(reg, addr); } while (0)

#define _sqrtsd_r128_m64abs(reg, addr) \
do { OP1(0xf2); OP1(0x0f); OP1(0x51); MODRM_MABS(reg, addr); } while (0)

#define _subss_r128_m32abs(reg, addr) \
do { OP1(0xf3); OP1(0x0f); OP1(0x5c); MODRM_MABS(reg, addr); } while (0)

#define _subss_r128_m32bd(reg, base, disp) \
do { OP1(0xf3); OP1(0x0f); OP1(0x5c); MODRM_MBD(reg, base, disp); } while (0)

#define _subsd_r128_m64abs(reg, addr) \
do { OP1(0xf2); OP1(0x0f); OP1(0x5c); MODRM_MABS(reg, addr); } while (0)

#define _ucomiss_r128_m32abs(reg, addr) \
do { OP1(0x0f); OP1(0x2e); MODRM_MABS(reg, addr); } while (0)

#define _ucomisd_r128_m64abs(reg, addr) \
do { OP1(0x66); OP1(0x0f); OP1(0x2e); MODRM_MABS(reg, addr); } while (0)

#define _xorps_r128_m128abs(reg, addr) \
do { OP1(0x0f); OP1(0x57); MODRM_MABS(reg, addr); } while (0)

#define _xorpd_r128_m128abs(reg, addr) \
do { OP1(0x66); OP1(0x0f); OP1(0x57); MODRM_MABS(reg, addr); } while (0)



#define _addps_r128_r128(r1, r2) \
do { OP1(0x0f); OP1(0x58); MODRM_REG(r1, r2); } while (0)

#define _addss_r128_r128(r1, r2) \
do { OP1(0xf3); OP1(0x0f); OP1(0x58); MODRM_REG(r1, r2); } while (0)

#define _addsd_r128_r128(r1, r2) \
do { OP1(0xf2); OP1(0x0f); OP1(0x58); MODRM_REG(r1, r2); } while (0)

#define _andps_r128_r128(r1, r2) \
do { OP1(0x0f); OP1(0x54); MODRM_REG(r1, r2); } while (0)

#define _andpd_r128_r128(r1, r2) \
do { OP1(0x66); OP1(0x0f); OP1(0x54); MODRM_REG(r1, r2); } while (0)

#define _cmpps_r128_r128(r1, r2, typ) \
do { OP1(0x0f); OP1(0xc2); MODRM_REG(r1, r2); OP1(typ); } while (0)

#define _comiss_r128_r128(r1, r2) \
do { OP1(0x0f); OP1(0x2f); MODRM_REG(r1, r2); } while (0)

#define _comiss_r128_m32bd(reg, base, disp) \
do { OP1(0x0f); OP1(0x2f); MODRM_MBD(reg, base, disp); } while (0)

#define _comisd_r128_r128(r1, r2) \
do { OP1(0x66); OP1(0x0f); OP1(0x2f); MODRM_REG(r1, r2); } while (0)

#define _cvtdq2ps_r128_r128(r1, r2) \
do { OP1(0x0f); OP1(0x5b); MODRM_REG(r1, r2); } while (0)

#define _cvtdq2pd_r128_r128(r1, r2) \
do { OP1(0xf3); OP1(0x0f); OP1(0xe6); MODRM_REG(r1, r2); } while (0)

#define _cvtps2dq_r128_r128(r1, r2) \
do { OP1(0x66); OP1(0x0f); OP1(0x5b); MODRM_REG(r1, r2); } while (0)

#define _cvtpd2dq_r128_r128(r1, r2) \
do { OP1(0xf2); OP1(0x0f); OP1(0xe6); MODRM_REG(r1, r2); } while (0)

#define _cvtpd2ps_r128_r128(r1, r2) \
do { OP1(0x66); OP1(0x0f); OP1(0x5a); MODRM_REG(r1, r2); } while (0)

#define _cvtps2pd_r128_r128(r1, r2) \
do { OP1(0x0f); OP1(0x5a); MODRM_REG(r1, r2); } while (0)

#define _cvtsd2ss_r128_r128(r1, r2) \
do { OP1(0xf2); OP1(0x0f); OP1(0x5a); MODRM_REG(r1, r2); } while (0)

#define _cvtsi2ss_r128_r32(r1, r2) \
do { OP1(0xf3); OP1(0x0f); OP1(0x2a); MODRM_REG(r1, r2); } while (0)

#define _cvtss2sd_r128_r128(r1, r2) \
do { OP1(0xf3); OP1(0x0f); OP1(0x5a); MODRM_REG(r1, r2); } while (0)

#define _cvttps2dq_r128_r128(r1, r2) \
do { OP1(0xf3); OP1(0x0f); OP1(0x5b); MODRM_REG(r1, r2); } while (0)

#define _cvttpd2dq_r128_r128(r1, r2) \
do { OP1(0x66); OP1(0x0f); OP1(0xe6); MODRM_REG(r1, r2); } while (0)

#define _cvttss2si_r32_r128(r1, r2) \
do { OP1(0xf3); OP1(0x0f); OP1(0x2c); MODRM_REG(r1, r2); } while (0)

#define _cvttsd2si_r32_r128(r1, r2) \
do { OP1(0xf2); OP1(0x0f); OP1(0x2c); MODRM_REG(r1, r2); } while (0)

#define _divps_r128_r128(r1, r2) \
do { OP1(0x0f); OP1(0x5e); MODRM_REG(r1, r2); } while (0)

#define _divss_r128_r128(r1, r2) \
do { OP1(0xf3); OP1(0x0f); OP1(0x5e); MODRM_REG(r1, r2); } while (0)

#define _divsd_r128_r128(r1, r2) \
do { OP1(0xf2); OP1(0x0f); OP1(0x5e); MODRM_REG(r1, r2); } while (0)

#define _maxss_r128_r128(r1, r2) \
do { OP1(0xf3); OP1(0x0f); OP1(0x5f); MODRM_REG(r1, r2); } while (0)

#define _mulps_r128_r128(r1, r2) \
do { OP1(0x0f); OP1(0x59); MODRM_REG(r1, r2); } while (0)

#define _mulss_r128_r128(r1, r2) \
do { OP1(0xf3); OP1(0x0f); OP1(0x59); MODRM_REG(r1, r2); } while (0)

#define _mulsd_r128_r128(r1, r2) \
do { OP1(0xf2); OP1(0x0f); OP1(0x59); MODRM_REG(r1, r2); } while (0)

#define _orps_r128_r128(r1, r2) \
do { OP1(0x0f); OP1(0x56); MODRM_REG(r1, r2); } while (0)

#define _orpd_r128_r128(r1, r2) \
do { OP1(0x66); OP1(0x0f); OP1(0x56); MODRM_REG(r1, r2); } while (0)

#define _rcpps_r128_r128(r1, r2) \
do { OP1(0x0f); OP1(0x53); MODRM_REG(r1, r2); } while (0)

#define _rcpss_r128_r128(r1, r2) \
do { OP1(0xf3); OP1(0x0f); OP1(0x53); MODRM_REG(r1, r2); } while (0)

#define _rsqrtss_r128_r128(r1, r2) \
do { OP1(0xf3); OP1(0x0f); OP1(0x52); MODRM_REG(r1, r2); } while (0)

#define _shufps_r128_r128(r1, r2, imm) \
do { OP1(0x0f); OP1(0xc6); MODRM_REG(r1, r2); OP1(imm); } while (0)

#define _sqrtss_r128_r128(r1, r2) \
do { OP1(0xf3); OP1(0x0f); OP1(0x51); MODRM_REG(r1, r2); } while (0)

#define _sqrtsd_r128_r128(r1, r2) \
do { OP1(0xf2); OP1(0x0f); OP1(0x51); MODRM_REG(r1, r2); } while (0)

#define _subss_r128_r128(r1, r2) \
do { OP1(0xf3); OP1(0x0f); OP1(0x5c); MODRM_REG(r1, r2); } while (0)

#define _subsd_r128_r128(r1, r2) \
do { OP1(0xf2); OP1(0x0f); OP1(0x5c); MODRM_REG(r1, r2); } while (0)

#define _ucomiss_r128_r128(r1, r2) \
do { OP1(0x0f); OP1(0x2e); MODRM_REG(r1, r2); } while (0)

#define _ucomisd_r128_r128(r1, r2) \
do { OP1(0x66); OP1(0x0f); OP1(0x2e); MODRM_REG(r1, r2); } while (0)

#define _unpcklps_r128_r128(r1, r2) \
do { OP1(0x0f); OP1(0x14); MODRM_REG(r1, r2); } while (0)

#define _xorps_r128_r128(r1, r2) \
do { OP1(0x0f); OP1(0x57); MODRM_REG(r1, r2); } while (0)

#define _xorpd_r128_r128(r1, r2) \
do { OP1(0x66); OP1(0x0f); OP1(0x57); MODRM_REG(r1, r2); } while (0)



#define _packuswb_r128_r128(r1, r2) \
do { OP1(0x66); OP1(0x0f); OP1(0x67); MODRM_REG(r1, r2); } while (0)

#define _paddw_r128_r128(r1, r2) \
do { OP1(0x66); OP1(0x0f); OP1(0xfd); MODRM_REG(r1, r2); } while (0)

#define _paddw_r128_m128abs(reg, addr) \
do { OP1(0x66); OP1(0x0f); OP1(0xfd); MODRM_MABS(reg, addr); } while (0)

#define _paddw_r128_m128bisd(reg, base, indx, scale, disp) \
do { OP1(0x66); OP1(0x0f); OP1(0xfd); MODRM_MBISD(reg, base, indx, scale, disp); } while (0)

#define _paddd_r128_m128abs(reg, addr) \
do { OP1(0x66); OP1(0x0f); OP1(0xfe); MODRM_MABS(reg, addr); } while (0)

#define _paddd_r128_m128bd(reg, base, disp) \
do { OP1(0x66); OP1(0x0f); OP1(0xfe); MODRM_MBD(reg, base, disp); } while (0)

#define _paddq_r128_r128(r1, r2) \
do { OP1(0x66); OP1(0x0f); OP1(0xd4); MODRM_REG(r1, r2); } while (0)

#define _pand_r128_r128(r1, r2) \
do { OP1(0x66); OP1(0x0f); OP1(0xdb); MODRM_REG(r1, r2); } while (0)

#define _pand_r128_m128abs(reg, addr) \
do { OP1(0x66); OP1(0x0f); OP1(0xdb); MODRM_MABS(reg, addr); } while (0)

#define _pandn_r128_r128(r1, r2) \
do { OP1(0x66); OP1(0x0f); OP1(0xdf); MODRM_REG(r1, r2); } while (0)

#define _packssdw_r128_r128(r1, r2) \
do { OP1(0x66); OP1(0x0f); OP1(0x6b); MODRM_REG(r1, r2); } while (0)

#define _pextrw_r32_r128(r1, r2, imm) \
do { OP1(0x66); OP1(0x0f); OP1(0xc5); MODRM_REG(r1, r2); OP1(imm); } while (0)

#define _pinsrw_r128_r32(r1, r2, imm) \
do { OP1(0x66); OP1(0x0f); OP1(0xc4); MODRM_REG(r1, r2); OP1(imm); } while (0)

#define _pmaxsw_r128_m128abs(reg, addr) \
do { OP1(0x66); OP1(0x0f); OP1(0xee); MODRM_MABS(reg, addr); } while (0)

#define _pminsw_r128_m128abs(reg, addr) \
do { OP1(0x66); OP1(0x0f); OP1(0xea); MODRM_MABS(reg, addr); } while (0)

#define _pmullw_r128_r128(r1, r2) \
do { OP1(0x66); OP1(0x0f); OP1(0xd5); MODRM_REG(r1, r2); } while (0)

#define _por_r128_r128(r1, r2) \
do { OP1(0x66); OP1(0x0f); OP1(0xeb); MODRM_REG(r1, r2); } while (0)

#define _pshufd_r128_r128(r1, r2, imm) \
do { OP1(0x66); OP1(0x0f); OP1(0x70); MODRM_REG(r1, r2); OP1(imm); } while (0)

#define _pshufhw_r128_r128(r1, r2, imm) \
do { OP1(0xf3); OP1(0x0f); OP1(0x70); MODRM_REG(r1, r2); OP1(imm); } while (0)

#define _pshuflw_r128_r128(r1, r2, imm) \
do { OP1(0xf2); OP1(0x0f); OP1(0x70); MODRM_REG(r1, r2); OP1(imm); } while (0)

#define _psllw_r128_imm(reg, imm) \
do { OP1(0x66); OP1(0x0f); OP1(0x71); MODRM_REG(6, reg); OP1(imm); } while (0)

#define _psllq_r128_imm(reg, imm) \
do { OP1(0x66); OP1(0x0f); OP1(0x73); MODRM_REG(6, reg); OP1(imm); } while (0)

#define _psllq_r128_r128(r1, r2) \
do { OP1(0x66); OP1(0x0f); OP1(0xf3); MODRM_REG(r1, r2); } while (0)

#define _psrlw_r128_imm(reg, imm) \
do { OP1(0x66); OP1(0x0f); OP1(0x71); MODRM_REG(2, reg); OP1(imm); } while (0)

#define _psrld_r128_imm(reg, imm) \
do { OP1(0x66); OP1(0x0f); OP1(0x72); MODRM_REG(2, reg); OP1(imm); } while (0)

#define _psrlq_r128_imm(reg, imm) \
do { OP1(0x66); OP1(0x0f); OP1(0x73); MODRM_REG(2, reg); OP1(imm); } while (0)

#define _psrlq_r128_r128(r1, r2) \
do { OP1(0x66); OP1(0x0f); OP1(0xd3); MODRM_REG(r1, r2); } while (0)

#define _psrldq_r128_imm(reg, imm) \
do { OP1(0x66); OP1(0x0f); OP1(0x73); MODRM_REG(3, reg); OP1(imm); } while (0)

#define _psubw_r128_r128(r1, r2) \
do { OP1(0x66); OP1(0x0f); OP1(0xf9); MODRM_REG(r1, r2); } while (0)

#define _psubw_r128_m128abs(reg, addr) \
do { OP1(0x66); OP1(0x0f); OP1(0xf9); MODRM_MABS(reg, addr); } while (0)

#define _psubd_r128_r128(r1, r2) \
do { OP1(0x66); OP1(0x0f); OP1(0xfa); MODRM_REG(r1, r2); } while (0)

#define _psubd_r128_m128abs(reg, addr) \
do { OP1(0x66); OP1(0x0f); OP1(0xfa); MODRM_MABS(reg, addr); } while (0)

#define _psubd_r128_m128bd(reg, base, disp) \
do { OP1(0x66); OP1(0x0f); OP1(0xfa); MODRM_MBD(reg, base, disp); } while (0)

#define _psubq_r128_r128(r1, r2) \
do { OP1(0x66); OP1(0x0f); OP1(0xfb); MODRM_REG(r1, r2); } while (0)

#define _punpcklbw_r128_r128(r1, r2) \
do { OP1(0x66); OP1(0x0f); OP1(0x60); MODRM_REG(r1, r2); } while (0)

#define _punpcklbw_r128_m128abs(reg, addr) \
do { OP1(0x66); OP1(0x0f); OP1(0x60); MODRM_MABS(reg, addr); } while (0)

#define _punpckldq_r128_r128(r1, r2) \
do { OP1(0x66); OP1(0x0f); OP1(0x62); MODRM_REG(r1, r2); } while (0)

#define _punpcklqdq_r128_r128(r1, r2) \
do { OP1(0x66); OP1(0x0f); OP1(0x6c); MODRM_REG(r1, r2); } while (0)

#define _pxor_r128_r128(r1, r2) \
do { OP1(0x66); OP1(0x0f); OP1(0xef); MODRM_REG(r1, r2); } while (0)

#define _pxor_r128_m128abs(reg, addr) \
do { OP1(0x66); OP1(0x0f); OP1(0xef); MODRM_MABS(reg, addr); } while (0)

#define _punpcklbw_mmx_mmx(r1, r2) \
do { OP1(0x0f); OP1(0x60); MODRM_REG(r1, r2); } while (0)

#define _punpcklwd_mmx_mmx(r1, r2) \
do { OP1(0x0f); OP1(0x61); MODRM_REG(r1, r2); } while (0)

#define _punpckldq_mmx_mmx(r1, r2) \
do { OP1(0x0f); OP1(0x62); MODRM_REG(r1, r2); } while (0)

#define _punpckhwd_mmx_mmx(r1, r2) \
do { OP1(0x0f); OP1(0x69); MODRM_REG(r1, r2); } while (0)

#define _punpckhdq_mmx_mmx(r1, r2) \
do { OP1(0x0f); OP1(0x6A); MODRM_REG(r1, r2); } while (0)


#define _prefetch_m8abs(type, addr) \
do { OP1(0x0f); OP1(0x18); MODRM_MABS(type, addr); } while (0)

#define _prefetch_m8bd(type, base, disp) \
do { OP1(0x0f); OP1(0x18); MODRM_MBD(type, base, disp); } while (0)


#define _emms() \
do { OP1(0x0f); OP1(0x77); } while(0)

#define _mfence() \
do { OP1(0x0f); OP1(0xae); OP1(0xf0); } while(0)

#define _sfence() \
do { OP1(0x0f); OP1(0xae); OP1(0xf8); } while(0)


#define _movnti_m32abs_r32(addr, sreg) \
do { OP1(0x0f); OP1(0xc3); MODRM_MABS(sreg, addr); } while (0)

#define _movnti_m32bd_r32(base, disp, sreg) \
do { OP1(0x0f); OP1(0xc3); MODRM_MBD(sreg, base, disp); } while (0)

#define _movnti_m32isd_r32(indx, scale, addr, dreg) \
do { OP1(0x0f); OP1(0xc3); MODRM_MBISD(dreg, NO_BASE, indx, scale, addr); } while (0)

#define _movnti_m32bisd_r32(base, indx, scale, addr, dreg) \
do { OP1(0x0f); OP1(0xc3); MODRM_MBISD(dreg, base, indx, scale, addr); } while (0)

#define _movntq_m64abs_mmx(addr, sreg) \
do { OP1(0x0f); OP1(0xe7); MODRM_MABS(sreg, addr); } while (0)

#define _movntq_m64bd_mmx(base, disp, sreg) \
do { OP1(0x0f); OP1(0xe7); MODRM_MBD(sreg, base, disp); } while (0)

#define _movntq_m64isd_mmx(indx, scale, addr, dreg) \
do { OP1(0x0f); OP1(0xe7); MODRM_MBISD(dreg, NO_BASE, indx, scale, addr); } while (0)

#define _movntq_m64bisd_mmx(base, indx, scale, addr, dreg) \
do { OP1(0x0f); OP1(0xe7); MODRM_MBISD(dreg, base, indx, scale, addr); } while (0)

#define _movq_mmx_mmx(dreg, sreg) \
do { OP1(0x0f); OP1(0x7f); MODRM_REG(sreg, dreg); } while (0)

#define _movq_mmx_m64bd(dreg, base, disp) \
do { OP1(0x0f); OP1(0x6f); MODRM_MBD(dreg, base, disp); } while (0)

#define _movq_m64abs_mmx(addr, sreg) \
do { OP1(0x0f); OP1(0x7f); MODRM_MABS(sreg, addr); } while (0)

#define _movq_m64bd_mmx(base, disp, sreg) \
do { OP1(0x0f); OP1(0x7f); MODRM_MBD(sreg, base, disp); } while (0)

#define _movq_m64isd_mmx(indx, scale, addr, dreg) \
do { OP1(0x0f); OP1(0x7f); MODRM_MBISD(dreg, NO_BASE, indx, scale, addr); } while (0)

#define _movq_m64bisd_mmx(base, indx, scale, addr, dreg) \
do { OP1(0x0f); OP1(0x7f); MODRM_MBISD(dreg, base, indx, scale, addr); } while (0)

#define _pand_mmx_mmx(r1, r2) \
do { OP1(0x0f); OP1(0xdb); MODRM_REG(r1, r2); } while (0)

#define _pand_mmx_m64abs(reg, addr) \
do { OP1(0x0f); OP1(0xdb); MODRM_MABS(reg, addr); } while (0)

#define _por_mmx_mmx(r1, r2) \
do { OP1(0x0f); OP1(0xeb); MODRM_REG(r1, r2); } while (0)

#define _pxor_mmx_mmx(r1, r2) \
do { OP1(0x0f); OP1(0xef); MODRM_REG(r1, r2); } while (0)

#define _por_mmx_m64abs(reg, addr) \
do { OP1(0x0f); OP1(0xeb); MODRM_MABS(reg, addr); } while (0)

#define _pssrd_mmx_imm(reg, imm) \
do { OP1(0x0f); OP1(0x72); MODRM_REG(2, reg); OP1(imm); } while (0)

#define _pssld_mmx_imm(reg, imm) \
do { OP1(0x0f); OP1(0x72); MODRM_REG(6, reg); OP1(imm); } while (0)

#define _pshufw_mmx_mmx_imm(dreg, sreg, imm) \
do { OP1(0x0f); OP1(0x70); MODRM_REG(dreg, sreg); OP1(imm); } while (0)

#define _packssdw_mmx_mmx(r1, r2) \
do { OP1(0x0f); OP1(0x6b); MODRM_REG(r1, r2); } while (0)



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* init/shutdown */
drc_core *drc_init(UINT8 cpunum, drc_config *config);
void drc_cache_reset(drc_core *drc);
void drc_execute(drc_core *drc);
void drc_exit(drc_core *drc);

/* code management */
void drc_begin_sequence(drc_core *drc, UINT32 pc);
void drc_end_sequence(drc_core *drc);
void drc_register_code_at_cache_top(drc_core *drc, UINT32 pc);
void *drc_get_code_at_pc(drc_core *drc, UINT32 pc);

/* standard appendages */
void drc_append_dispatcher(drc_core *drc);
void drc_append_fixed_dispatcher(drc_core *drc, UINT32 newpc);
void drc_append_tentative_fixed_dispatcher(drc_core *drc, UINT32 newpc);
void drc_append_call_debugger(drc_core *drc);
void drc_append_standard_epilogue(drc_core *drc, INT32 cycles, INT32 pcdelta, int allow_exit);
void drc_append_save_volatiles(drc_core *drc);
void drc_append_restore_volatiles(drc_core *drc);
void drc_append_save_call_restore(drc_core *drc, genf *target, UINT32 stackadj);
void drc_append_verify_code(drc_core *drc, void *code, UINT8 length);

void drc_append_set_fp_rounding(drc_core *drc, UINT8 regindex);
void drc_append_set_temp_fp_rounding(drc_core *drc, UINT8 rounding);
void drc_append_restore_fp_rounding(drc_core *drc);

void drc_append_set_sse_rounding(drc_core *drc, UINT8 regindex);
void drc_append_set_temp_sse_rounding(drc_core *drc, UINT8 rounding);
void drc_append_restore_sse_rounding(drc_core *drc);

/* disassembling drc code */
void drc_dasm(FILE *f, const void *begin, const void *end);

/* x86 CPU features */
UINT32 drc_x86_get_features(void);


#endif	/* __X86DRC_H__ */
