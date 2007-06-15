/*****************************************************************************
 *
 *   sh4.h
 *   Portable Hitachi SH-4 (SH7750 family) emulator interface
 *
 *   By R. Belmont, based on sh2.c by Juergen Buchmueller, Mariusz Wojcieszek,
 *      Olivier Galibert, Sylvain Glaize, and James Forshaw.
 *
 *****************************************************************************/

#ifndef _SH4_H
#define _SH4_H

#include "cpuintrf.h"

#define SH4_INT_NONE	-1
#define SH4_INT_VBLIN	0
#define SH4_INT_VBLOUT	1
#define SH4_INT_HBLIN	2
#define SH4_INT_TIMER0	3
#define SH4_INT_TIMER1	4
#define SH4_INT_DSP 	5
#define SH4_INT_SOUND	6
#define SH4_INT_SMPC	7
#define SH4_INT_PAD 	8
#define SH4_INT_DMA2	9
#define SH4_INT_DMA1	10
#define SH4_INT_DMA0	11
#define SH4_INT_DMAILL	12
#define SH4_INT_SPRITE	13
#define SH4_INT_14		14
#define SH4_INT_15		15
#define SH4_INT_ABUS	16

enum {
	SH4_PC=1, SH4_SR, SH4_PR, SH4_GBR, SH4_VBR, SH4_DBR, SH4_MACH, SH4_MACL,
	SH4_R0, SH4_R1, SH4_R2, SH4_R3, SH4_R4, SH4_R5, SH4_R6, SH4_R7,
	SH4_R8, SH4_R9, SH4_R10, SH4_R11, SH4_R12, SH4_R13, SH4_R14, SH4_R15, SH4_EA,
	SH4_R0_BK0, SH4_R1_BK0, SH4_R2_BK0, SH4_R3_BK0, SH4_R4_BK0, SH4_R5_BK0, SH4_R6_BK0, SH4_R7_BK0,
	SH4_R0_BK1, SH4_R1_BK1, SH4_R2_BK1, SH4_R3_BK1, SH4_R4_BK1, SH4_R5_BK1, SH4_R6_BK1, SH4_R7_BK1,
	SH4_SPC, SH4_SSR, SH4_SGR, SH4_FPSCR, SH4_FPUL, SH4_FR0, SH4_FR1, SH4_FR2, SH4_FR3, SH4_FR4, SH4_FR5,
	SH4_FR6, SH4_FR7, SH4_FR8, SH4_FR9, SH4_FR10, SH4_FR11, SH4_FR12, SH4_FR13, SH4_FR14, SH4_FR15,
	SH4_XF0, SH4_XF1, SH4_XF2, SH4_XF3, SH4_XF4, SH4_XF5, SH4_XF6, SH4_XF7,
	SH4_XF8, SH4_XF9, SH4_XF10, SH4_XF11, SH4_XF12, SH4_XF13, SH4_XF14, SH4_XF15,
};

enum
{
	CPUINFO_INT_SH4_FRT_INPUT = CPUINFO_INT_CPU_SPECIFIC
};

enum
{
	CPUINFO_PTR_SH4_FTCSR_READ_CALLBACK = CPUINFO_PTR_CPU_SPECIFIC,
};

struct sh4_config
{
  int is_slave;
};

extern void sh4_get_info(UINT32 state, cpuinfo *info);

WRITE32_HANDLER( sh4_internal_w );
READ32_HANDLER( sh4_internal_r );

#ifdef MAME_DEBUG
extern unsigned DasmSH4( char *dst, unsigned pc, UINT16 opcode );
#endif

#endif /* _SH4_H */

