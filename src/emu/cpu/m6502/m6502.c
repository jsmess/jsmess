/*****************************************************************************
 *
 *   m6502.c
 *   Portable 6502/65c02/65sc02/6510/n2a03 emulator V1.2
 *
 *   Copyright Juergen Buchmueller, all rights reserved.
 *   65sc02 core Copyright Peter Trauner.
 *   Deco16 portions Copyright Bryan McPhail.
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
/* 2.February 2000 PeT added 65sc02 subtype */
/* 10.March   2000 PeT added 6502 set overflow input line */
/* 13.September 2000 PeT N2A03 jmp indirect */

#if ((HAS_M65SC02 || HAS_DECO16) && !HAS_M65C02)
#undef HAS_M65C02
#define HAS_M65C02 1
#endif

#include "debugger.h"
#include "deprecat.h"
#include "m6502.h"
#include "ops02.h"
#include "ill02.h"


#define M6502_NMI_VEC	0xfffa
#define M6502_RST_VEC	0xfffc
#define M6502_IRQ_VEC	0xfffe

#define DECO16_RST_VEC	0xfff0
#define DECO16_IRQ_VEC	0xfff2
#define DECO16_NMI_VEC	0xfff4

#define VERBOSE 0

#define LOG(x)	do { if (VERBOSE) logerror x; } while (0)



/****************************************************************************
 * The 6502 registers.
 ****************************************************************************/
typedef struct
{
	UINT8	subtype;		/* currently selected cpu sub type */
	void	(*const *insn)(void); /* pointer to the function pointer table */
	PAIR	ppc;			/* previous program counter */
	PAIR	pc; 			/* program counter */
	PAIR	sp; 			/* stack pointer (always 100 - 1FF) */
	PAIR	zp; 			/* zero page address */
	PAIR	ea; 			/* effective address */
	UINT8	a;				/* Accumulator */
	UINT8	x;				/* X index register */
	UINT8	y;				/* Y index register */
	UINT8	p;				/* Processor status */
	UINT8	pending_irq;	/* nonzero if an IRQ is pending */
	UINT8	after_cli;		/* pending IRQ and last insn cleared I */
	UINT8	nmi_state;
	UINT8	irq_state;
	UINT8   so_state;
	int 	(*irq_callback)(int irqline);	/* IRQ callback */
	read8_machine_func rdmem_id;					/* readmem callback for indexed instructions */
	write8_machine_func wrmem_id;				/* writemem callback for indexed instructions */

#if (HAS_M6510) || (HAS_M6510T) || (HAS_M8502) || (HAS_M7501)
	UINT8    ddr;
	UINT8    port;
	UINT8	(*port_read)(UINT8 direction);
	void	(*port_write)(UINT8 direction, UINT8 data);
#endif

}	m6502_Regs;

static int m6502_IntOccured = 0;
static int m6502_ICount = 0;

static m6502_Regs m6502;

static READ8_HANDLER( default_rdmem_id ) { return program_read_byte_8le(offset); }
static WRITE8_HANDLER( default_wdmem_id ) { program_write_byte_8le(offset, data); }

/***************************************************************
 * include the opcode macros, functions and tables
 ***************************************************************/
#include "t6502.c"

#if (HAS_M6510)
#include "t6510.c"
#endif

#include "opsn2a03.h"

#if (HAS_N2A03)
#include "tn2a03.c"
#endif

#include "opsc02.h"

#if (HAS_M65C02)
#include "t65c02.c"
#endif

#if (HAS_M65SC02)
#include "t65sc02.c"
#endif

#if (HAS_DECO16)
#include "tdeco16.c"
#endif

/*****************************************************************************
 *
 *      6502 CPU interface functions
 *
 *****************************************************************************/

static void m6502_common_init(int index, int clock, const void *config, int (*irqcallback)(int), UINT8 subtype, void (*const *insn)(void), const char *type)
{
	memset(&m6502, 0, sizeof(m6502));
	m6502.irq_callback = irqcallback;
	m6502.subtype = subtype;
	m6502.insn = insn;
	m6502.rdmem_id = default_rdmem_id;
	m6502.wrmem_id = default_wdmem_id;

	state_save_register_item(type, index, m6502.pc.w.l);
	state_save_register_item(type, index, m6502.sp.w.l);
	state_save_register_item(type, index, m6502.p);
	state_save_register_item(type, index, m6502.a);
	state_save_register_item(type, index, m6502.x);
	state_save_register_item(type, index, m6502.y);
	state_save_register_item(type, index, m6502.pending_irq);
	state_save_register_item(type, index, m6502.after_cli);
	state_save_register_item(type, index, m6502.nmi_state);
	state_save_register_item(type, index, m6502.irq_state);
	state_save_register_item(type, index, m6502.so_state);

#if (HAS_M6510) || (HAS_M6510T) || (HAS_M8502) || (HAS_M7501)
	if (subtype == SUBTYPE_6510)
	{
		state_save_register_item(type, index, m6502.port);
		state_save_register_item(type, index, m6502.ddr);
	}
#endif
}

static void m6502_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	m6502_common_init(index, clock, config, irqcallback, SUBTYPE_6502, insn6502, "m6502");
}

static void m6502_reset(void)
{
	/* wipe out the rest of the m6502 structure */
	/* read the reset vector into PC */
	PCL = RDMEM(M6502_RST_VEC);
	PCH = RDMEM(M6502_RST_VEC+1);

	m6502.sp.d = 0x01ff;	/* stack pointer starts at page 1 offset FF */
	m6502.p = F_T|F_I|F_Z|F_B|(P&F_D);	/* set T, I and Z flags */
	m6502.pending_irq = 0;	/* nonzero if an IRQ is pending */
	m6502.after_cli = 0;	/* pending IRQ and last insn cleared I */
	m6502.irq_state = 0;
	m6502.nmi_state = 0;

	change_pc(PCD);
}

static void m6502_exit(void)
{
	/* nothing to do yet */
}

static void m6502_get_context (void *dst)
{
	if( dst )
		*(m6502_Regs*)dst = m6502;
}

static void m6502_set_context (void *src)
{
	if( src )
	{
		m6502 = *(m6502_Regs*)src;
		change_pc(PCD);
	}
}

INLINE void m6502_take_irq(void)
{
	if( !(P & F_I) )
	{
		EAD = M6502_IRQ_VEC;
		m6502_ICount -= 2;
		PUSH(PCH);
		PUSH(PCL);
		PUSH(P & ~F_B);
		P |= F_I;		/* set I flag */
		PCL = RDMEM(EAD);
		PCH = RDMEM(EAD+1);
		LOG(("M6502#%d takes IRQ ($%04x)\n", cpu_getactivecpu(), PCD));
		/* call back the cpuintrf to let it clear the line */
		if (m6502.irq_callback) (*m6502.irq_callback)(0);
		change_pc(PCD);
	}
	m6502.pending_irq = 0;
}

#define NR_OPS		258
#define MAX_OP_LEN	32
enum {
	OP_FETCH=1,	/* Opcode fetch, increase pc */
	FETCH,		/* Fetch a byte from memory, store byte in tmp */
	STORE,		/* Store a byte in memory writes byte from tmp */
	/* Basic instructions */
	/* 1st pass instructions */
	_BPL, _BMI, _BVC, _BVS, _BCC, _BCS, _BNE, _BEQ,
	_ADC, _ANC, _AND, _ARR, _ASL, _ASR, _AST, _ASX, _AXA, _BIT, _BRK, _CLC, _CLD, _CLI, _CLV, _CMP, _CPX, _CPY,
	_DCP, _DEC, _DEX, _DEY, _EOR, _INC, _INX, _INY, _ISB, _JMP, _JSR, _KIL,
	_LAX, _LDA, _LDX, _LDY, _LSR, _OAL, _NOP, _ORA, _PHA, _PHP, _PLA, _PLP, _RLA, _ROL, _ROR, _RTI, _RRA, _RTS,
	_SAH, _SAX, _SBC, _SEC, _SED, _SEI, _SLO, _SRE, _SSH, _STA, _STX, _STY, _SXH, _SYH, _TAX, _TAY, _TSX, _TXA, _TXS, _TYA,
	/* addressing modes */
	_EA_ABS, _EA_ABX_NP, _EA_ABX_P, _EA_ABY_NP, _EA_ABY_P, _EA_IND, _EA_IDX, _EA_IDY_NP, _EA_IDY_P, _EA_ZPG, _EA_ZPX, _EA_ZPY,
	_RD_DUM, _RD_IMM,
	_RD_EA, _WR_EA,
	_WB_ACC, _RD_ACC,

/*
	_EA_ZPG: _RD_OP, _WB_ZP, _WB_EA_ZP
	_EA_ZPX: _RD_OP, _WB_ZP, _Z_IDX, _WB_EA_ZP
	_EA_ZPY: _RD_OP, _WB_ZP, _Z_IDY, _WB_EA_ZP
	_EA_ABS: _RD_OP, _WB_EAL, _RD_OP, _WB_EAH
	_EA_ABX_NP: _RD_OP, _WB_EAL, _RD_OP, _WB_EAH, _IDX, _RD_EA, _EA_C, (_RD_EA/_WR_EA)
	_EA_ABX_P: _RD_OP, _WB_EAL, _RD_OP, _WB_EAH, _IDX, _RD_EA, _EA_C, (_RD_EA_C/_WR_EA_C)
	_EA_ABY_NP: _RD_OP, _WB_EAL, _RD_OP, _WB_EAH, _IDX, _RD_EA, _EA_C, (_RD_EA/_WR_EA)
*/
};

static const int m6502_ops[NR_OPS][MAX_OP_LEN] = {
	/* 0x00 - 0x0F */
	{ _BRK, OP_FETCH },	/* 00 - 7 BRK */
	{ _EA_IDX, _RD_EA, _ORA, OP_FETCH },	/* 01 - 6 ORA IDX */
	{ _KIL, OP_FETCH },	/* 02 - 1 KIL */
	{ _EA_IDX, _RD_EA, _WR_EA, _SLO, _WR_EA, OP_FETCH },	/* 03 - 7 SLO IDX */
	{ _EA_ZPG, _RD_EA, _NOP, OP_FETCH },	/* 04 - 3 NOP ZPG */
	{ _EA_ZPG, _RD_EA, _ORA, OP_FETCH },	/* 05 - 3 ORA ZPG */
	{ _EA_ZPG, _RD_EA, _WR_EA, _ASL, _WR_EA, OP_FETCH },	/* 06 - 5 ASL ZPG */
	{ _EA_ZPG, _RD_EA, _WR_EA, _SLO, _WR_EA, OP_FETCH },	/* 07 - 5 SLO ZPG */
	{ _RD_DUM, _PHP, OP_FETCH },	/* 08 - 3 PHP */
	{ _RD_IMM, _ORA, OP_FETCH },	/* 09 - 2 ORA IMM */
	{ _RD_DUM, _RD_ACC, _ASL, _WB_ACC, OP_FETCH },	/* 0a - 2 ASL A */
	{ _RD_IMM, _ANC, OP_FETCH },	/* 0b - 2 ANC IMM */
	{ _EA_ABS, _RD_EA, _NOP, OP_FETCH },	/* 0c - 4 NOP ABS */
	{ _EA_ABS, _RD_EA, _ORA, OP_FETCH },	/* 0d - 4 ORA ABS */
	{ _EA_ABS, _RD_EA, _WR_EA, _ASL, _WR_EA, OP_FETCH },	/* 0e - 6 ASL ABS */
	{ _EA_ABS, _RD_EA, _WR_EA, _SLO, _WR_EA, OP_FETCH },	/* 0f - 6 SLO ABS */
	/* 0x10 - 0x1F */
	{ _BPL, OP_FETCH },	/* 10 - 2-4 BPL REL */
	{ _EA_IDY_P, _RD_EA, _ORA, OP_FETCH },	/* 11 - 5 ORA IDY page penalty */
	{ _KIL, OP_FETCH },	/* 12 - 1 KIL */
	{ _EA_IDY_NP, _RD_EA, _WR_EA, _SLO, _WR_EA, OP_FETCH },	/* 13 - 7 SLO IDY */
	{ _EA_ZPX, _RD_EA, _NOP, OP_FETCH },	/* 14 - 4 NOP ZPX */
	{ _EA_ZPX, _RD_EA, _ORA, OP_FETCH },	/* 15 - 4 ORA ZPX */
	{ _EA_ZPX, _RD_EA, _WR_EA, _ASL, _WR_EA, OP_FETCH },	/* 16 - 4 ASL ZPG */
	{ _EA_ZPX, _RD_EA, _WR_EA, _SLO, _WR_EA, OP_FETCH },	/* 17 - 6 SLO ZPX */
	{ _RD_DUM, _CLC, OP_FETCH },	/* 18 - 2 CLC */
	{ _EA_ABY_P, _RD_EA, _ORA, OP_FETCH },	/* 19 - 4 ORA ABY page penalty */
	{ _RD_DUM, _NOP, OP_FETCH },	/* 1a - 2 NOP */
	{ _EA_ABY_NP, _RD_EA, _WR_EA, _SLO, _WR_EA, OP_FETCH },	/* 1b - 7 SLO ABY */
	{ _EA_ABX_P, _RD_EA, _NOP, OP_FETCH },	/* 1c - NOP ABX page penalty */
	{ _EA_ABX_P, _RD_EA, _ORA, OP_FETCH },	/* 1d - ORA ABX page penalty */
	{ _EA_ABX_NP, _RD_EA, _WR_EA, _ASL, _WR_EA, OP_FETCH },	/* 1e - 7 ASL ABX */
	{ _EA_ABX_NP, _RD_EA, _WR_EA, _SLO, _WR_EA, OP_FETCH },	/* 1f - 7 SLA ABX */
	/* 0x20 - 0x2F */
	{ _JSR, OP_FETCH },	/* 20 - 6 JSR */
	{ _EA_IDX, _RD_EA, _AND, OP_FETCH },	/* 21 - 6 AND IDX */
	{ _KIL, OP_FETCH },	/* 22 - 1 KIL */
	{ _EA_IDX, _RD_EA, _WR_EA, _RLA, _WR_EA, OP_FETCH },	/* 23 - 7 RLA IDX */
	{ _EA_ZPG, _RD_EA, _BIT, OP_FETCH },	/* 24 - 3 BIT ZPG */
	{ _EA_ZPG, _RD_EA, _AND, OP_FETCH },	/* 25 - 3 AND ZPG */
	{ _EA_ZPG, _RD_EA, _WR_EA, _ROL, _WR_EA, OP_FETCH },	/* 26 - 5 ROL ZPG */
	{ _EA_ZPG, _RD_EA, _WR_EA, _RLA, _WR_EA, OP_FETCH },	/* 27 - 5 RLA ZPG */
	{ _RD_DUM, _PLP, OP_FETCH },	/* 28 - 4 PLP */
	{ _RD_IMM, _AND, OP_FETCH },	/* 29 - 2 AND IMM */
	{ _RD_DUM, _RD_ACC, _ROL, _WB_ACC, OP_FETCH },	/* 2a - 2 ROL A */
	{ _RD_IMM, _ANC, OP_FETCH },	/* 2b - 2 ANC IMM */
	{ _EA_ABS, _RD_EA, _BIT, OP_FETCH },	/* 2c - 4 BIT ABS */
	{ _EA_ABS, _RD_EA, _AND, OP_FETCH },	/* 2d - 4 AND ABS */
	{ _EA_ABS, _RD_EA, _WR_EA, _ROL, _WR_EA, OP_FETCH },	/* 2e - 6 ROL ABS */
	{ _EA_ABS, _RD_EA, _WR_EA, _RLA, _WR_EA, OP_FETCH },	/* 2f - 6 RLA ABS */
	/* 0x30 - 0x3F */
	{ _BMI, OP_FETCH },	/* 30 - 2-4 BMI REL */
	{ _EA_IDY_P, _RD_EA, _AND, OP_FETCH },	/* 31 - 5 AND IDY page penalty */
	{ _KIL, OP_FETCH },	/* 32 - 1 KIL */
	{ _EA_IDY_NP, _RD_EA, _WR_EA, _RLA, _WR_EA, OP_FETCH },	/* 33 - 7 RLA IDY */
	{ _EA_ZPX, _RD_EA, _NOP, OP_FETCH },	/* 34 - 4 NOP ZPX */
	{ _EA_ZPX, _RD_EA, _AND, OP_FETCH },	/* 35 - 4 AND ZPX */
	{ _EA_ZPX, _RD_EA, _WR_EA, _ROL, _WR_EA, OP_FETCH },	/* 36 - 6 ROL ZPX */
	{ _EA_ZPX, _RD_EA, _WR_EA, _RLA, _WR_EA, OP_FETCH },	/* 37 - 6 RLA ZPX */
	{ _RD_DUM, _SEC, OP_FETCH },	/* 38 - 2 SEC */
	{ _EA_ABY_P, _RD_EA, _AND, OP_FETCH },	/* 39 - 4 AND ABY page penalty */
	{ _RD_DUM, _NOP, OP_FETCH },	/* 3a - 2 NOP */
	{ _EA_ABY_NP, _RD_EA, _WR_EA, _RLA, _WR_EA, OP_FETCH },	/* 3b - 7 RLA ABY */
	{ _EA_ABX_P, _RD_EA, _NOP, OP_FETCH },	/* 3c - 4 NOP ABX page penalty */
	{ _EA_ABX_P, _RD_EA, _AND, OP_FETCH },	/* 3d - 4 AND ABX page penalty */
	{ _EA_ABX_NP, _RD_EA, _WR_EA, _ROL, _WR_EA, OP_FETCH },	/* 3e - 7 ROL ABX */
	{ _EA_ABX_NP, _RD_EA, _WR_EA, _RLA, _WR_EA, OP_FETCH },	/* 3f - 7 RLA ABX */
	/* 0x40 - 0x4F */
	{ _RTI, OP_FETCH },	/* 40 - 6 RTI */
	{ _EA_IDX, _RD_EA, _EOR, OP_FETCH },	/* 41 - 6 EOR IDX */
	{ _KIL, OP_FETCH },	/* 42 - 1 KIL */
	{ _EA_IDX, _RD_EA, _WR_EA, _SRE, _WR_EA, OP_FETCH },	/* 43 - 7 SRE IDX */
	{ _EA_ZPG, _RD_EA, _NOP, OP_FETCH },	/* 44 - 3 NOP ZPG */
	{ _EA_ZPG, _RD_EA, _EOR, OP_FETCH },	/* 45 - 3 EOR ZPG */
	{ _EA_ZPG, _RD_EA, _WR_EA, _LSR, _WR_EA, OP_FETCH },	/* 46 - 5 LSR ZPG */
	{ _EA_ZPG, _RD_EA, _WR_EA, _SRE, _WR_EA, OP_FETCH },	/* 47 - 5 SRE ZPG */
	{ _RD_DUM, _PHA, OP_FETCH },	/* 48 - 3 PHA */
	{ _RD_IMM, _EOR, OP_FETCH },	/* 49 - 2 EOR IMM */
	{ _RD_DUM, _RD_ACC, _ROL, _WB_ACC, OP_FETCH },	/* 4a - 2 LSR A */
	{ _RD_IMM, _ASR, _WB_ACC, OP_FETCH },	/* 4b - 2 ASR IMM */
	{ _EA_ABS, _JMP, OP_FETCH },	/* 4c - 3 JMP ABS */
	{ _EA_ABS, _RD_EA, _EOR, OP_FETCH },	/* 4d - 4 EOR ABS */
	{ _EA_ABS, _RD_EA, _WR_EA, _LSR, _WR_EA, OP_FETCH },	/* 4e - 6 LSR ABS */
	{ _EA_ABS, _RD_EA, _WR_EA, _SRE, _WR_EA, OP_FETCH },	/* 4f - 6 SRE ABS */
	/* 0x50 - 0x5F */
	{ _BVC, OP_FETCH },	/* 50 - 2-4 BVC REL */
	{ _EA_IDY_P, _RD_EA, _EOR, OP_FETCH },	/* 51 - EOR IDY page penalty */
	{ _KIL, OP_FETCH },	/* 52 - 1 KIL */
	{ _EA_IDY_NP, _RD_EA, _WR_EA, _SRE, _WR_EA, OP_FETCH },	/* 53 - 7 SRE IDY */
	{ _EA_ZPX, _RD_EA, _NOP, OP_FETCH },	/* 54 - 4 NOP ZPX */
	{ _EA_ZPX, _RD_EA, _EOR, OP_FETCH },	/* 55 - 4 EOR ZPX */
	{ _EA_ZPX, _RD_EA, _WR_EA, _LSR, _WR_EA, OP_FETCH },	/* 56 - 6 LSR ZPX */
	{ _EA_ZPX, _RD_EA, _WR_EA, _SRE, _WR_EA, OP_FETCH },	/* 57 - 6 SRE ZPX */
	{ _RD_DUM, _CLI, OP_FETCH },	/* 58 - 2 CLI */
	{ _EA_ABY_P, _RD_EA, _EOR, OP_FETCH },	/* 59 - 4 EOR ABY page penalty */
	{ _RD_DUM, _NOP, OP_FETCH },	/* 5a - 2 NOP */
	{ _EA_ABY_NP, _RD_EA, _WR_EA, _SRE, _WR_EA, OP_FETCH },	/* 5b - 7 SRE ABY */
	{ _EA_ABX_P, _RD_EA, _NOP, OP_FETCH },	/* 5c - 4 NOP ABX page penalty */
	{ _EA_ABX_P, _RD_EA, _EOR, OP_FETCH },	/* 5d - 4 EOR ABX page penalty */
	{ _EA_ABX_NP, _RD_EA, _WR_EA, _LSR, _WR_EA, OP_FETCH },	/* 5e - 7 LSR ABX */
	{ _EA_ABX_NP, _RD_EA, _WR_EA, _SRE, _WR_EA, OP_FETCH },	/* 5f - 7 SRE ABX */
	/* 0x60 - 0x6F */
	{ _RTS, OP_FETCH },	/* 60 - 6 RTS */
	{ _EA_IDX, _RD_EA, _ADC, OP_FETCH },	/* 61 - 6 ADC IDX */
	{ _KIL, OP_FETCH },	/* 62 - 1 KIL */
	{ _EA_IDX, _RD_EA, _WR_EA, _RRA, _WR_EA, OP_FETCH },	/* 63 - 7 RRA IDX */
	{ _EA_ZPG, _RD_EA, _NOP, OP_FETCH },	/* 64 - 3 NOP ZPG */
	{ _EA_ZPG, _RD_EA, _ADC, OP_FETCH },	/* 65 - 3 ADC ZPG */
	{ _EA_ZPG, _RD_EA, _WR_EA, _ROR, _WR_EA, OP_FETCH },	/* 66 - 5 ROR ZPG */
	{ _EA_ZPG, _RD_EA, _WR_EA, _RRA, _WR_EA, OP_FETCH },	/* 67 - 5 RRA ZPG */
	{ _RD_DUM, _PLA, OP_FETCH },	/* 68 - 4 PLA */
	{ _RD_IMM, _ADC, OP_FETCH },	/* 69 - 2 ADC IMM */
	{ _RD_DUM, _RD_ACC, _ROR, _WB_ACC, OP_FETCH },	/* 6a - 2 ROR A */
	{ _RD_IMM, _ARR, _WB_ACC, OP_FETCH },	/* 6b - 2 ARR IMM */
	{ _EA_IND, _JMP, OP_FETCH },	/* 6c - 5 JMP IND */
	{ _EA_ABS, _RD_EA, _ADC, OP_FETCH },	/* 6d - 4 AdC ABS */
	{ _EA_ABS, _RD_EA, _WR_EA, _ROR, _WR_EA, OP_FETCH },	/* 6e - 6 ROR ABS */
	{ _EA_ABS, _RD_EA, _WR_EA, _RRA, _WR_EA, OP_FETCH },	/* 6f - 6 RRA ABS */
	/* 0x70 - 0x7F */
	{ _BVS, OP_FETCH },	/* 70 - 2-4 BVS REL */
	{ _EA_IDY_P, _RD_EA, _ADC, OP_FETCH },	/* 71 - 5 ADC IDY page penalty */
	{ _KIL, OP_FETCH },	/* 72 - KIL */
	{ _EA_IDY_NP, _RD_EA, _WR_EA, _RRA, _WR_EA, OP_FETCH },	/* 73 - 7 RRA IDY */
	{ _EA_ZPX, _RD_EA, _NOP, OP_FETCH },	/* 74 - 4 NOP ZPX */
	{ _EA_ZPX, _RD_EA, _ADC, OP_FETCH },	/* 75 - 4 ADC ZPX */
	{ _EA_ZPX, _RD_EA, _WR_EA, _ROR, _WR_EA, OP_FETCH },	/* 76 - 6 ROR ZPX */
	{ _EA_ZPX, _RD_EA, _WR_EA, _RRA, _WR_EA, OP_FETCH },	/* 77 - 6 RRA ZPX */
	{ _RD_DUM, _SEI, OP_FETCH },	/* 78 - 2 SEI */
	{ _EA_ABY_P, _RD_EA, _ADC, OP_FETCH },	/* 79 - 4 ADC ABY page penalty */
	{ _RD_DUM, _NOP, OP_FETCH },	/* 7a - 2 NOP */
	{ _EA_ABY_NP, _RD_EA, _WR_EA, _RRA, _WR_EA, OP_FETCH },	/* 7b - 7 RRA ABY */
	{ _EA_ABX_P, _RD_EA, _NOP, OP_FETCH },	/* 7c - 4 NOP ABX page penalty */
	{ _EA_ABX_P, _RD_EA, _ADC, OP_FETCH },	/* 7d - 4 ADC ABX page penalty */
	{ _EA_ABX_NP, _RD_EA, _WR_EA, _ROR, _WR_EA, OP_FETCH },	/* 7e - 7 ROR ABS */
	{ _EA_ABX_NP, _RD_EA, _WR_EA, _RRA, _WR_EA, OP_FETCH },	/* 7f - 7 RRA ABX */
	/* 0x80 - 0x8F */
	{ _RD_IMM, _NOP, OP_FETCH },	/* 80 - 2 NOP IMM */
	{ _EA_IDX, _STA, _WR_EA, OP_FETCH },	/* 81 - 6 STA IDX */
	{ _RD_IMM, _NOP, OP_FETCH },	/* 82 - 2 NOP IMM */
	{ _EA_IDX, _SAX, _WR_EA, OP_FETCH },	/* 83 - 6 SAX IDX */
	{ _EA_ZPG, _STY, _WR_EA, OP_FETCH },	/* 84 - 3 STY ZPG */
	{ _EA_ZPG, _STA, _WR_EA, OP_FETCH },	/* 85 - 3 STA ZPG */
	{ _EA_ZPG, _STX, _WR_EA, OP_FETCH },	/* 86 - 3 STX ZPG */
	{ _EA_ZPG, _SAX, _WR_EA, OP_FETCH },	/* 87 - 3 SAX ZPG */
	{ _RD_DUM, _DEY, OP_FETCH },	/* 88 - 2 DEY */
	{ _RD_IMM, _NOP, OP_FETCH },	/* 89 - 2 NOP IMM */
	{ _RD_DUM, _TXA, OP_FETCH },	/* 8a - 2 TXA */
	{ _RD_IMM, _AXA, OP_FETCH },	/* 8b - 2 AXA IMM */
	{ _EA_ABS, _STY, _WR_EA, OP_FETCH },	/* 8c - 4 STY ABS */
	{ _EA_ABS, _STA, _WR_EA, OP_FETCH },	/* 8d - 4 STA ABS */
	{ _EA_ABS, _STX, _WR_EA, OP_FETCH },	/* 8e - 4 STX ABS */
	{ _EA_ABS, _SAX, _WR_EA, OP_FETCH },	/* 8f - 4 SAX ABS */
	/* 0x90 - 0x9F */
	{ _BCC, OP_FETCH },	/* 90 - 2-4 BCC IMS */
	{ _EA_IDY_NP, _STA, _WR_EA, OP_FETCH },	/* 91 - 6 STA IDY */
	{ _KIL, OP_FETCH },	/* 92 - 1 KIL */
	{ _EA_IDY_NP, _SAH, _WR_EA, OP_FETCH },	/* 93 - SAH IDY */
	{ _EA_ZPX, _STY, _WR_EA, OP_FETCH },	/* 94 - 4 STY ZPX */
	{ _EA_ZPX, _STA, _WR_EA, OP_FETCH },	/* 95 - 4 STA ZPX */
	{ _EA_ZPY, _STX, _WR_EA, OP_FETCH },	/* 96 - 4 STX ZPY */
	{ _EA_ZPY, _SAX, _WR_EA, OP_FETCH },	/* 97 - 4 SAX ZPY */
	{ _RD_DUM, _TYA, OP_FETCH },	/* 98 - 2 TYA */
	{ _EA_ABY_NP, _STA, _WR_EA, OP_FETCH },	/* 99 - 5 STA ABY */
	{ _RD_DUM, _TXS, OP_FETCH },	/* 9a - 2 TXS */
	{ _EA_ABY_NP, _SSH, _WR_EA, OP_FETCH },	/* 9b - 5 SSH ABY */
	{ _EA_ABX_NP, _SYH, _WR_EA, OP_FETCH },	/* 9c - 5 SYH ABX */
	{ _EA_ABX_NP, _STA, _WR_EA, OP_FETCH },	/* 9d - 5 STA ABX */
	{ _EA_ABY_NP, _SXH, _WR_EA, OP_FETCH },	/* 9e - 5 SXH ABY */
	{ _EA_ABY_NP, _SAH, OP_FETCH },	/* 9f - 5 SAH ABY */
	/* 0xA0 - 0xAF */
	{ _RD_IMM, _LDY, OP_FETCH },	/* a0 - 2 LDY IMM */
	{ _EA_IDX, _RD_EA, _LDA, OP_FETCH },	/* a1 - 6 LDA IDX */
	{ _RD_IMM, _LDX, OP_FETCH },	/* a2 - 2 LDX IMM */
	{ _EA_IDX, _RD_EA, _LAX, OP_FETCH },	/* a3 - 6 LAX IDX */
	{ _EA_ZPG, _RD_EA, _LDY, OP_FETCH },	/* a4 - 3 LDY ZPG */
	{ _EA_ZPG, _RD_EA, _LDA, OP_FETCH },	/* a5 - 3 LDA ZPG */
	{ _EA_ZPG, _RD_EA, _LDX, OP_FETCH },	/* a6 - 3 LDX ZPG */
	{ _EA_ZPG, _RD_EA, _LAX, OP_FETCH },	/* a7 - 3 LAX ZPG */
	{ _RD_DUM, _TAY, OP_FETCH },	/* a8 - 2 TAY */
	{ _RD_IMM, _LDA, OP_FETCH },	/* a9 - 2 LDA IMM */
	{ _RD_DUM, _TAX, OP_FETCH },	/* aa - 2 TAX */
	{ _RD_IMM, _OAL, OP_FETCH },	/* ab - 2 OAL IMM */
	{ _EA_ABS, _RD_EA, _LDY, OP_FETCH },	/* ac - 4 LDY ABS */
	{ _EA_ABS, _RD_EA, _LDA, OP_FETCH },	/* ad - 4 LDA ABS */
	{ _EA_ABS, _RD_EA, _LDX, OP_FETCH },	/* ae - 4 LDX ABS */
	{ _EA_ABS, _RD_EA, _LAX, OP_FETCH },	/* af - 4 LAX ABS */
	/* 0xB0 - 0xBF */
	{ _BCS, OP_FETCH },	/* b0 - 2-4 BCS REL */
	{ _EA_IDY_P, _RD_EA, _LDA, OP_FETCH },	/* b1 - 5 LDA IDY page penalty */
	{ _KIL, OP_FETCH },	/* b2 - 1 KIL */
	{ _EA_IDY_P, _RD_EA, _LAX, OP_FETCH },	/* b3 - 5 LAX IDY page penalty */
	{ _EA_ZPX, _RD_EA, _LDY, OP_FETCH },	/* b4 - 4 LDY ZPX */
	{ _EA_ZPX, _RD_EA, _LDA, OP_FETCH },	/* b5 - 4 LDA ZPX */
	{ _EA_ZPY, _RD_EA, _LDX, OP_FETCH },	/* b6 - 4 LDX ZPY */
	{ _EA_ZPY, _RD_EA, _LAX, OP_FETCH },	/* b7 - 4 LAX ZPY */
	{ _RD_DUM, _CLV, OP_FETCH },	/* b8 - 2 CLV */
	{ _EA_ABY_P, _RD_EA, _LDA, OP_FETCH },	/* b9 - 4 LDA ABY page penalty */
	{ _RD_DUM, _TSX, OP_FETCH },	/* ba - 2 TSX */
	{ _EA_ABY_P, _RD_EA, _AST, OP_FETCH },	/* bb - 4 AST ABY page penalty */
	{ _EA_ABX_P, _RD_EA, _LDY, OP_FETCH },	/* bc - 4 LDY ABX page penalty */
	{ _EA_ABX_P, _RD_EA, _LDA, OP_FETCH },	/* bd - 4 LDA ABX page penalty */
	{ _EA_ABY_P, _RD_EA, _LDX, OP_FETCH },	/* be - 4 LDX ABY page penalty */
	{ _EA_ABY_P, _RD_EA, _LAX, OP_FETCH },	/* bf - 4 LAX ABY page penalty */
	/* 0xC0 - 0xCF */
	{ _RD_IMM, _CPY, OP_FETCH },	/* c0 - 2 CPY IMM */
	{ _EA_IDX, _RD_EA, _CMP, OP_FETCH },	/* c1 - 6 CMP IDX */
	{ _RD_IMM, _NOP, OP_FETCH },	/* c2 - 2 NOP IMM */
	{ _EA_IDX, _RD_EA, _WR_EA, _DCP, _WR_EA, OP_FETCH },	/* c3 - 7 DCP IDX */
	{ _EA_ZPG, _RD_EA, _CPY, OP_FETCH },	/* c4 - 3 CPY ZPG */
	{ _EA_ZPG, _RD_EA, _CMP, OP_FETCH },	/* c5 - 3 CMP ZPG */
	{ _EA_ZPG, _RD_EA, _WR_EA, _DEC, _WR_EA, OP_FETCH },	/* c6 - 5 DEC ZPG */
	{ _EA_ZPG, _RD_EA, _WR_EA, _DCP, _WR_EA, OP_FETCH },	/* c7 - 5 DCP ZPG */
	{ _RD_DUM, _INY, OP_FETCH },	/* c8 - 2 INY */
	{ _RD_IMM, _CMP, OP_FETCH },	/* c9 - 2 CMP IMM */
	{ _RD_DUM, _DEX, OP_FETCH },	/* ca - 2 DEX */
	{ _RD_IMM, _ASX, OP_FETCH },	/* cb - 2 ASX IMM */
	{ _EA_ABS, _RD_EA, _CPY, OP_FETCH },	/* cc - 4 CPY ABS */
	{ _EA_ABS, _RD_EA, _CMP, OP_FETCH },	/* cd - 4 CMP ABS */
	{ _EA_ABS, _RD_EA, _WR_EA, _DEC, _WR_EA, OP_FETCH },	/* ce - 6 DEC ABS */
	{ _EA_ABS, _RD_EA, _WR_EA, _DCP, _WR_EA, OP_FETCH },	/* cf - 6 DCP ABS */
	/* 0xD0 - 0xDF */
	{ _BNE, OP_FETCH },	/* d0 - 2-4 BNE REL */
	{ _EA_IDY_P, _RD_EA, _CMP, OP_FETCH },	/* d1 - 5 CMP IDX page penalty */
	{ _KIL, OP_FETCH },	/* d2 - 1 KIL */
	{ _EA_IDY_NP, _RD_EA, _WR_EA, _DCP, _WR_EA, OP_FETCH },	/* d3 - 7 DCP, IDY */
	{ _EA_ZPX, _RD_EA, _NOP, OP_FETCH },	/* d4 - 4 NOP ZPX */
	{ _EA_ZPX, _RD_EA, _CMP, OP_FETCH },	/* d5 - 4 CMP ZPX */
	{ _EA_ZPX, _RD_EA, _WR_EA, _DEC, _WR_EA, OP_FETCH },	/* d6 - 6 DEC ZPX */
	{ _EA_ZPX, _RD_EA, _WR_EA, _DCP, _WR_EA, OP_FETCH },	/* d7 - 6 DCP ZPX */
	{ _RD_DUM, _CLD, OP_FETCH },	/* d8 - 2 CLD */
	{ _EA_ABY_P, _RD_EA, _CMP, OP_FETCH },	/* d9 - 4 CMP ABY page penalty */
	{ _RD_DUM, _NOP, OP_FETCH },	/* da - 2 NOP */
	{ _EA_ABY_NP, _RD_EA, _WR_EA, _DCP, _WR_EA, OP_FETCH },	/* db - 7 DCP ABY */
	{ _EA_ABX_P, _RD_EA, _NOP, OP_FETCH },	/* dc - 4 NOP ABX page penalty */
	{ _EA_ABX_P, _RD_EA, _CMP, OP_FETCH },	/* dd - 4 CMP ABX page penalty */
	{ _EA_ABX_NP, _RD_EA, _WR_EA, _DEC, _WR_EA, OP_FETCH },	/* de - 7 DEC ABX */
	{ _EA_ABX_NP, _RD_EA, _WR_EA, _DCP, _WR_EA, OP_FETCH },	/* df - 7 DCP ABX */
	/* 0xE0 - 0xEF */
	{ _RD_IMM, _CPX, OP_FETCH },	/* e0 - 2 CPX IMM */
	{ _EA_IDX, _RD_EA, _SBC, OP_FETCH },	/* e1 - 6 SBC IDX */
	{ _RD_IMM, _NOP, OP_FETCH },	/* e2 - 2 NOP IMM */
	{ _EA_IDX, _RD_EA, _WR_EA, _ISB, _WR_EA, OP_FETCH },	/* e3 - 7 ISB IDX */
	{ _EA_ZPG, _RD_EA, _CPX, OP_FETCH },	/* e4 - 3 CPX ZPG */
	{ _EA_ZPG, _RD_EA, _SBC, OP_FETCH },	/* e5 - 3 SBC ZPG */
	{ _EA_ZPG, _RD_EA, _WR_EA, _INC, _WR_EA, OP_FETCH },	/* e6 - 5 INC ZPG */
	{ _EA_ZPG, _RD_EA, _WR_EA, _ISB, _WR_EA, OP_FETCH },	/* e7 - 5 ISB ZPG */
	{ _RD_DUM, _INX, OP_FETCH },	/* e8 - 2 INX */
	{ _RD_IMM, _SBC, OP_FETCH },	/* e9 - 2 SBC IMM */
	{ _RD_DUM, _NOP, OP_FETCH },	/* ea - 2 NOP */
	{ _RD_IMM, _SBC, OP_FETCH },	/* eb - 2 SBC IMM */
	{ _EA_ABS, _RD_EA, _CPX, OP_FETCH },	/* ec - 4 CPX ABS */
	{ _EA_ABS, _RD_EA, _SBC, OP_FETCH },	/* ed - 4 SBC ABS */
	{ _EA_ABS, _RD_EA, _WR_EA, _INC, _WR_EA, OP_FETCH },	/* ee - 6 INC ABS */
	{ _EA_ABS, _RD_EA, _WR_EA, _ISB, _WR_EA, OP_FETCH },	/* ef - 6 ISB ABS */
	/* 0xF0 - 0xFF */
	{ _BEQ, OP_FETCH },	/* f0 - 2-4 BEQ REL */
	{ _EA_IDY_P, _RD_EA, _SBC, OP_FETCH },	/* f1 - 5 SBC page penalty */
	{ _KIL, OP_FETCH },	/* f2 - 1 KIL */
	{ _EA_IDY_NP, _RD_EA, _WR_EA, _ISB, _WR_EA, OP_FETCH },	/* f3 - 7 ISB IDY */
	{ _EA_ZPX, _RD_EA, _NOP, OP_FETCH },	/* f4 - 4 NOP ZPX */
	{ _EA_ZPX, _RD_EA, _SBC, OP_FETCH },	/* f5 - 4 SBC ZPX */
	{ _EA_ZPX, _RD_EA, _WR_EA, _INC, _WR_EA, OP_FETCH },	/* f6 - 6 INC ZPX */
	{ _EA_ZPX, _RD_EA, _WR_EA, _ISB, _WR_EA, OP_FETCH },	/* f7 - 6 ISB ZPX */
	{ _RD_DUM, _SED, OP_FETCH },	/* f8 - 2 SED */
	{ _EA_ABY_P, _RD_EA, _SBC, OP_FETCH },	/* f9 - 4 SBC ABY page penalty */
	{ _RD_DUM, _NOP, OP_FETCH },	/* fa - 2 NOP */
	{ _EA_ABY_NP, _RD_EA, _WR_EA, _ISB, _WR_EA, OP_FETCH },	/* fb - 7 ISB ABY */
	{ _EA_ABX_P, _RD_EA, _NOP, OP_FETCH },	/* fc - 4 NOP ABX page penalty */
	{ _EA_ABX_P, _RD_EA, _SBC, OP_FETCH },	/* fd - 4 SBC ABX page penalty */
	{ _EA_ABX_NP, _RD_EA, _WR_EA, _INC, _WR_EA, OP_FETCH },	/* fe - 7 INC ABX */
	{ _EA_ABX_NP, _RD_EA, _WR_EA, _ISB, _WR_EA, OP_FETCH },	/* ff - 7 ISB ABX */
	/* special cases */
	{ OP_FETCH },	/* RESET */
	{ OP_FETCH },	/* TAKE_IRQ */
};

static int m6502_execute2(int cycles) {
	m6502_ICount = cycles;

	change_pc(PCD);

	do {
		debugger_instruction_hook(Machine, PCD);
		m6502_ICount--;
	} while (m6502_ICount > 0);

	return cycles - m6502_ICount;
}

static int m6502_execute(int cycles)
{
	m6502_ICount = cycles;

	change_pc(PCD);

	do
	{
		UINT8 op;
		PPC = PCD;

		debugger_instruction_hook(Machine, PCD);

		/* if an irq is pending, take it now */
		if( m6502.pending_irq )
			m6502_take_irq();

		op = RDOP();
		(*m6502.insn[op])();

		/* check if the I flag was just reset (interrupts enabled) */
		if( m6502.after_cli )
		{
			LOG(("M6502#%d after_cli was >0", cpu_getactivecpu()));
			m6502.after_cli = 0;
			if (m6502.irq_state != CLEAR_LINE)
			{
				LOG((": irq line is asserted: set pending IRQ\n"));
				m6502.pending_irq = 1;
			}
			else
			{
				LOG((": irq line is clear\n"));
			}
		}
		else {
			if ( m6502.pending_irq == 2 ) {
				if ( m6502_IntOccured - m6502_ICount > 1 ) {
					m6502.pending_irq = 1;
				}
			}
			if( m6502.pending_irq == 1 )
				m6502_take_irq();
			if ( m6502.pending_irq == 2 ) {
				m6502.pending_irq = 1;
			}
		}

	} while (m6502_ICount > 0);

	return cycles - m6502_ICount;
}

static void m6502_set_irq_line(int irqline, int state)
{
	if (irqline == INPUT_LINE_NMI)
	{
		if (m6502.nmi_state == state) return;
		m6502.nmi_state = state;
		if( state != CLEAR_LINE )
		{
			LOG(( "M6502#%d set_nmi_line(ASSERT)\n", cpu_getactivecpu()));
			EAD = M6502_NMI_VEC;
			m6502_ICount -= 2;
			PUSH(PCH);
			PUSH(PCL);
			PUSH(P & ~F_B);
			P |= F_I;		/* set I flag */
			PCL = RDMEM(EAD);
			PCH = RDMEM(EAD+1);
			LOG(("M6502#%d takes NMI ($%04x)\n", cpu_getactivecpu(), PCD));
			change_pc(PCD);
		}
	}
	else
	{
		if( irqline == M6502_SET_OVERFLOW )
		{
			if( m6502.so_state && !state )
			{
				LOG(( "M6502#%d set overflow\n", cpu_getactivecpu()));
				P|=F_V;
			}
			m6502.so_state=state;
			return;
		}
		m6502.irq_state = state;
		if( state != CLEAR_LINE )
		{
			LOG(( "M6502#%d set_irq_line(ASSERT)\n", cpu_getactivecpu()));
			m6502.pending_irq = 1;
//          m6502.pending_irq = 2;
			m6502_IntOccured = m6502_ICount;
		}
	}
}



/****************************************************************************
 * 2A03 section
 ****************************************************************************/
#if (HAS_N2A03)

static void n2a03_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	m6502_common_init(index, clock, config, irqcallback, SUBTYPE_2A03, insn2a03, "n2a03");
}

/* The N2A03 is integrally tied to its PSG (they're on the same die).
   Bit 7 of address $4011 (the PSG's DPCM control register), when set,
   causes an IRQ to be generated.  This function allows the IRQ to be called
   from the PSG core when such an occasion arises. */
void n2a03_irq(void)
{
  m6502_take_irq();
}
#endif


/****************************************************************************
 * 6510 section
 ****************************************************************************/
#if (HAS_M6510)

static void m6510_init (int index, int clock, const void *config, int (*irqcallback)(int))
{
	m6502_common_init(index, clock, config, irqcallback, SUBTYPE_6510, insn6510, "m6510");
}

static void m6510_reset (void)
{
	m6502_reset();
	m6502.port = 0xff;
	m6502.ddr = 0x00;
}

static UINT8 m6510_get_port(void)
{
	return (m6502.port & m6502.ddr) | (m6502.ddr ^ 0xff);
}

static READ8_HANDLER( m6510_read_0000 )
{
	UINT8 result = 0x00;

	switch(offset)
	{
		case 0x0000:	/* DDR */
			result = m6502.ddr;
			break;
		case 0x0001:	/* Data Port */
			if (m6502.port_read)
				result = m6502.port_read( m6502.ddr );
			result = (m6502.ddr & m6502.port) | (~m6502.ddr & result);
			break;
	}
	return result;
}

static WRITE8_HANDLER( m6510_write_0000 )
{
	switch(offset)
	{
		case 0x0000:	/* DDR */
			m6502.ddr = data;
			break;
		case 0x0001:	/* Data Port */
			m6502.port = data;
			break;
	}

	if (m6502.port_write)
		m6502.port_write( m6502.ddr, m6502.port & m6502.ddr );
}

static ADDRESS_MAP_START(m6510_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x0001) AM_READWRITE(m6510_read_0000, m6510_write_0000)
ADDRESS_MAP_END

#endif


/****************************************************************************
 * 65C02 section
 ****************************************************************************/
#if (HAS_M65C02)

static void m65c02_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	m6502_common_init(index, clock, config, irqcallback, SUBTYPE_65C02, insn65c02, "m65c02");
}

static void m65c02_reset (void)
{
	m6502_reset();
	P &=~F_D;
}

INLINE void m65c02_take_irq(void)
{
	if( !(P & F_I) )
	{
		EAD = M6502_IRQ_VEC;
		m6502_ICount -= 2;
		PUSH(PCH);
		PUSH(PCL);
		PUSH(P & ~F_B);
		P = (P & ~F_D) | F_I;		/* knock out D and set I flag */
		PCL = RDMEM(EAD);
		PCH = RDMEM(EAD+1);
		LOG(("M65c02#%d takes IRQ ($%04x)\n", cpu_getactivecpu(), PCD));
		/* call back the cpuintrf to let it clear the line */
		if (m6502.irq_callback) (*m6502.irq_callback)(0);
		change_pc(PCD);
	}
	m6502.pending_irq = 0;
}

static int m65c02_execute(int cycles)
{
	m6502_ICount = cycles;

	change_pc(PCD);

	do
	{
		UINT8 op;
		PPC = PCD;

		debugger_instruction_hook(Machine, PCD);

		op = RDOP();
		(*m6502.insn[op])();

		/* if an irq is pending, take it now */
		if( m6502.pending_irq )
			m65c02_take_irq();


		/* check if the I flag was just reset (interrupts enabled) */
		if( m6502.after_cli )
		{
			LOG(("M6502#%d after_cli was >0", cpu_getactivecpu()));
			m6502.after_cli = 0;
			if (m6502.irq_state != CLEAR_LINE)
			{
				LOG((": irq line is asserted: set pending IRQ\n"));
				m6502.pending_irq = 1;
			}
			else
			{
				LOG((": irq line is clear\n"));
			}
		}
		else
		if( m6502.pending_irq )
			m65c02_take_irq();

	} while (m6502_ICount > 0);

	return cycles - m6502_ICount;
}

static void m65c02_set_irq_line(int irqline, int state)
{
	if (irqline == INPUT_LINE_NMI)
	{
		if (m6502.nmi_state == state) return;
		m6502.nmi_state = state;
		if( state != CLEAR_LINE )
		{
			LOG(( "M6502#%d set_nmi_line(ASSERT)\n", cpu_getactivecpu()));
			EAD = M6502_NMI_VEC;
			m6502_ICount -= 2;
			PUSH(PCH);
			PUSH(PCL);
			PUSH(P & ~F_B);
			P = (P & ~F_D) | F_I;		/* knock out D and set I flag */
			PCL = RDMEM(EAD);
			PCH = RDMEM(EAD+1);
			LOG(("M6502#%d takes NMI ($%04x)\n", cpu_getactivecpu(), PCD));
			change_pc(PCD);
		}
	}
	else
		m6502_set_irq_line(irqline,state);
}
#endif

/****************************************************************************
 * 65SC02 section
 ****************************************************************************/
#if (HAS_M65SC02)
static void m65sc02_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	m6502_common_init(index, clock, config, irqcallback, SUBTYPE_65SC02, insn65sc02, "m65sc02");
}
#endif

/****************************************************************************
 * DECO16 section
 ****************************************************************************/
#if (HAS_DECO16)

static void deco16_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	m6502_common_init(index, clock, config, irqcallback, SUBTYPE_DECO16, insndeco16, "deco16");
}


static void deco16_reset (void)
{
	m6502_reset();
	m6502.subtype = SUBTYPE_DECO16;
	m6502.insn = insndeco16;

    PCL = RDMEM(DECO16_RST_VEC+1);
    PCH = RDMEM(DECO16_RST_VEC);

	m6502.sp.d = 0x01ff;	/* stack pointer starts at page 1 offset FF */
	m6502.p = F_T|F_I|F_Z|F_B|(P&F_D);	/* set T, I and Z flags */
	m6502.pending_irq = 0;	/* nonzero if an IRQ is pending */
	m6502.after_cli = 0;	/* pending IRQ and last insn cleared I */

	change_pc(PCD);
}

INLINE void deco16_take_irq(void)
{
	if( !(P & F_I) )
	{
		EAD = DECO16_IRQ_VEC;
		m6502_ICount -= 2;
		PUSH(PCH);
		PUSH(PCL);
		PUSH(P & ~F_B);
		P |= F_I;		/* set I flag */
		PCL = RDMEM(EAD+1);
		PCH = RDMEM(EAD);
		LOG(("M6502#%d takes IRQ ($%04x)\n", cpu_getactivecpu(), PCD));
		/* call back the cpuintrf to let it clear the line */
		if (m6502.irq_callback) (*m6502.irq_callback)(0);
		change_pc(PCD);
	}
	m6502.pending_irq = 0;
}

static void deco16_set_irq_line(int irqline, int state)
{
	if (irqline == INPUT_LINE_NMI)
	{
		if (m6502.nmi_state == state) return;
		m6502.nmi_state = state;
		if( state != CLEAR_LINE )
		{
			LOG(( "M6502#%d set_nmi_line(ASSERT)\n", cpu_getactivecpu()));
			EAD = DECO16_NMI_VEC;
			m6502_ICount -= 7;
			PUSH(PCH);
			PUSH(PCL);
			PUSH(P & ~F_B);
			P |= F_I;		/* set I flag */
			PCL = RDMEM(EAD+1);
			PCH = RDMEM(EAD);
			LOG(("M6502#%d takes NMI ($%04x)\n", cpu_getactivecpu(), PCD));
			change_pc(PCD);
		}
	}
	else
	{
		if( irqline == M6502_SET_OVERFLOW )
		{
			if( m6502.so_state && !state )
			{
				LOG(( "M6502#%d set overflow\n", cpu_getactivecpu()));
				P|=F_V;
			}
			m6502.so_state=state;
			return;
		}
		m6502.irq_state = state;
		if( state != CLEAR_LINE )
		{
			LOG(( "M6502#%d set_irq_line(ASSERT)\n", cpu_getactivecpu()));
			m6502.pending_irq = 1;
		}
	}
}

static int deco16_execute(int cycles)
{
	m6502_ICount = cycles;

	change_pc(PCD);

	do
	{
		UINT8 op;
		PPC = PCD;

		debugger_instruction_hook(Machine, PCD);

		op = RDOP();
		(*m6502.insn[op])();

		/* if an irq is pending, take it now */
		if( m6502.pending_irq )
			deco16_take_irq();


		/* check if the I flag was just reset (interrupts enabled) */
		if( m6502.after_cli )
		{
			LOG(("M6502#%d after_cli was >0", cpu_getactivecpu()));
			m6502.after_cli = 0;
			if (m6502.irq_state != CLEAR_LINE)
			{
				LOG((": irq line is asserted: set pending IRQ\n"));
				m6502.pending_irq = 1;
			}
			else
			{
				LOG((": irq line is clear\n"));
			}
		}
		else
		if( m6502.pending_irq )
			deco16_take_irq();

	} while (m6502_ICount > 0);

	return cycles - m6502_ICount;
}

#endif



/**************************************************************************
 * Generic set_info
 **************************************************************************/

static void m6502_set_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are set as 64-bit signed integers --- */
		case CPUINFO_INT_INPUT_STATE + M6502_IRQ_LINE:		m6502_set_irq_line(M6502_IRQ_LINE, info->i); break;
		case CPUINFO_INT_INPUT_STATE + M6502_SET_OVERFLOW:	m6502_set_irq_line(M6502_SET_OVERFLOW, info->i); break;
		case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:		m6502_set_irq_line(INPUT_LINE_NMI, info->i); break;

		case CPUINFO_INT_PC:							PCW = info->i; change_pc(PCD);			break;
		case CPUINFO_INT_REGISTER + M6502_PC:			m6502.pc.w.l = info->i;					break;
		case CPUINFO_INT_SP:							S = info->i;							break;
		case CPUINFO_INT_REGISTER + M6502_S:			m6502.sp.b.l = info->i;					break;
		case CPUINFO_INT_REGISTER + M6502_P:			m6502.p = info->i;						break;
		case CPUINFO_INT_REGISTER + M6502_A:			m6502.a = info->i;						break;
		case CPUINFO_INT_REGISTER + M6502_X:			m6502.x = info->i;						break;
		case CPUINFO_INT_REGISTER + M6502_Y:			m6502.y = info->i;						break;
		case CPUINFO_INT_REGISTER + M6502_EA:			m6502.ea.w.l = info->i;					break;
		case CPUINFO_INT_REGISTER + M6502_ZP:			m6502.zp.w.l = info->i;					break;

		/* --- the following bits of info are set as pointers to data or functions --- */
		case CPUINFO_PTR_M6502_READINDEXED_CALLBACK:	m6502.rdmem_id = (read8_machine_func) info->f; break;
		case CPUINFO_PTR_M6502_WRITEINDEXED_CALLBACK:	m6502.wrmem_id = (write8_machine_func) info->f; break;
	}
}



/**************************************************************************
 * Generic get_info
 **************************************************************************/

void m6502_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof(m6502);				break;
		case CPUINFO_INT_INPUT_LINES:					info->i = 2;							break;
		case CPUINFO_INT_DEFAULT_IRQ_VECTOR:			info->i = 0;							break;
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;
		case CPUINFO_INT_CLOCK_MULTIPLIER:				info->i = 1;							break;
		case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 1;							break;
		case CPUINFO_INT_MIN_INSTRUCTION_BYTES:			info->i = 1;							break;
		case CPUINFO_INT_MAX_INSTRUCTION_BYTES:			info->i = 4;							break;
		case CPUINFO_INT_MIN_CYCLES:					info->i = 1;							break;
		case CPUINFO_INT_MAX_CYCLES:					info->i = 10;							break;

		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 8;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 16;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM: info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_DATA:	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_IO: 		info->i = 0;					break;

		case CPUINFO_INT_INPUT_STATE + M6502_IRQ_LINE:		info->i = m6502.irq_state;			break;
		case CPUINFO_INT_INPUT_STATE + M6502_SET_OVERFLOW:	info->i = m6502.so_state;			break;
		case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:		info->i = m6502.nmi_state;			break;

		case CPUINFO_INT_PREVIOUSPC:					info->i = m6502.ppc.w.l;				break;

		case CPUINFO_INT_PC:							info->i = PCD;							break;
		case CPUINFO_INT_REGISTER + M6502_PC:			info->i = m6502.pc.w.l;					break;
		case CPUINFO_INT_SP:							info->i = S;							break;
		case CPUINFO_INT_REGISTER + M6502_S:			info->i = m6502.sp.b.l;					break;
		case CPUINFO_INT_REGISTER + M6502_P:			info->i = m6502.p;						break;
		case CPUINFO_INT_REGISTER + M6502_A:			info->i = m6502.a;						break;
		case CPUINFO_INT_REGISTER + M6502_X:			info->i = m6502.x;						break;
		case CPUINFO_INT_REGISTER + M6502_Y:			info->i = m6502.y;						break;
		case CPUINFO_INT_REGISTER + M6502_EA:			info->i = m6502.ea.w.l;					break;
		case CPUINFO_INT_REGISTER + M6502_ZP:			info->i = m6502.zp.w.l;					break;
		case CPUINFO_INT_REGISTER + M6502_SUBTYPE:		info->i = m6502.subtype;				break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_SET_INFO:						info->setinfo = m6502_set_info;			break;
		case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = m6502_get_context;	break;
		case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = m6502_set_context;	break;
		case CPUINFO_PTR_INIT:							info->init = m6502_init;				break;
		case CPUINFO_PTR_RESET:							info->reset = m6502_reset;				break;
		case CPUINFO_PTR_EXIT:							info->exit = m6502_exit;				break;
		case CPUINFO_PTR_EXECUTE:						info->execute = m6502_execute;			break;
		case CPUINFO_PTR_BURN:							info->burn = NULL;						break;
		case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = m6502_dasm;			break;
		case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &m6502_ICount;			break;
		case CPUINFO_PTR_M6502_READINDEXED_CALLBACK:	info->f = (genf *) m6502.rdmem_id;		break;
		case CPUINFO_PTR_M6502_WRITEINDEXED_CALLBACK:	info->f = (genf *) m6502.wrmem_id;		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "M6502");				break;
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s, "Mostek 6502");			break;
		case CPUINFO_STR_CORE_VERSION:					strcpy(info->s, "1.2");					break;
		case CPUINFO_STR_CORE_FILE:						strcpy(info->s, __FILE__);				break;
		case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s, "Copyright Juergen Buchmueller, all rights reserved."); break;

		case CPUINFO_STR_FLAGS:
			sprintf(info->s, "%c%c%c%c%c%c%c%c",
				m6502.p & 0x80 ? 'N':'.',
				m6502.p & 0x40 ? 'V':'.',
				m6502.p & 0x20 ? 'R':'.',
				m6502.p & 0x10 ? 'B':'.',
				m6502.p & 0x08 ? 'D':'.',
				m6502.p & 0x04 ? 'I':'.',
				m6502.p & 0x02 ? 'Z':'.',
				m6502.p & 0x01 ? 'C':'.');
			break;

		case CPUINFO_STR_REGISTER + M6502_PC:			sprintf(info->s, "PC:%04X", m6502.pc.w.l); break;
		case CPUINFO_STR_REGISTER + M6502_S:			sprintf(info->s, "S:%02X", m6502.sp.b.l); break;
		case CPUINFO_STR_REGISTER + M6502_P:			sprintf(info->s, "P:%02X", m6502.p); break;
		case CPUINFO_STR_REGISTER + M6502_A:			sprintf(info->s, "A:%02X", m6502.a); break;
		case CPUINFO_STR_REGISTER + M6502_X:			sprintf(info->s, "X:%02X", m6502.x); break;
		case CPUINFO_STR_REGISTER + M6502_Y:			sprintf(info->s, "Y:%02X", m6502.y); break;
		case CPUINFO_STR_REGISTER + M6502_EA:			sprintf(info->s, "EA:%04X", m6502.ea.w.l); break;
		case CPUINFO_STR_REGISTER + M6502_ZP:			sprintf(info->s, "ZP:%03X", m6502.zp.w.l); break;
	}
}


#if (HAS_N2A03)
/**************************************************************************
 * CPU-specific set_info
 **************************************************************************/

void n2a03_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = n2a03_init;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "N2A03");				break;

		default:										m6502_get_info(state, info);			break;
	}
}
#endif


#if (HAS_M6510) || (HAS_M6510T) || (HAS_M8502) || (HAS_M7501)
/**************************************************************************
 * CPU-specific set_info
 **************************************************************************/

static void m6510_set_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are set as pointers to data or functions --- */
		case CPUINFO_PTR_M6510_PORTREAD:	m6502.port_read = (UINT8 (*)(UINT8)) info->f;	break;
		case CPUINFO_PTR_M6510_PORTWRITE:	m6502.port_write = (void (*)(UINT8,UINT8)) info->f;	break;

		default:							m6502_set_info(state, info);						break;
	}
}

void m6510_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_SET_INFO:						info->setinfo = m6510_set_info;			break;
		case CPUINFO_PTR_INIT:							info->init = m6510_init;				break;
		case CPUINFO_PTR_RESET:							info->reset = m6510_reset;				break;
		case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = m6510_dasm;			break;
		case CPUINFO_PTR_INTERNAL_MEMORY_MAP:			info->internal_map8 = address_map_m6510_mem; break;
		case CPUINFO_PTR_M6510_PORTREAD:				info->f = (genf *) m6502.port_read;		break;
		case CPUINFO_PTR_M6510_PORTWRITE:				info->f = (genf *) m6502.port_write;	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "M6510");				break;

		/* --- the following bits of info are set as 64-bit signed integers --- */
		case CPUINFO_INT_M6510_PORT:					info->i = m6510_get_port();				break;

		default:										m6502_get_info(state, info);			break;
	}
}
#endif


#if (HAS_M6510T)
/**************************************************************************
 * CPU-specific set_info
 **************************************************************************/

void m6510t_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "M6510T");				break;

		default:										m6510_get_info(state, info);			break;
	}
}
#endif


#if (HAS_M7501)
/**************************************************************************
 * CPU-specific set_info
 **************************************************************************/

void m7501_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "M7501");				break;

		default:										m6510_get_info(state, info);			break;
	}
}
#endif


#if (HAS_M8502)
/**************************************************************************
 * CPU-specific set_info
 **************************************************************************/

void m8502_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "M8502");				break;

		default:										m6510_get_info(state, info);			break;
	}
}
#endif


#if (HAS_M65C02)
/**************************************************************************
 * CPU-specific set_info
 **************************************************************************/

static void m65c02_set_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are set as 64-bit signed integers --- */
		case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:	m65c02_set_irq_line(INPUT_LINE_NMI, info->i); break;

		default:										m6502_set_info(state, info);			break;
	}
}

void m65c02_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_SET_INFO:						info->setinfo = m65c02_set_info;		break;
		case CPUINFO_PTR_INIT:							info->init = m65c02_init;				break;
		case CPUINFO_PTR_RESET:							info->reset = m65c02_reset;				break;
		case CPUINFO_PTR_EXECUTE:						info->execute = m65c02_execute;			break;
		case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = m65c02_dasm;		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "M65C02");				break;

		default:										m6502_get_info(state, info);			break;
	}
}
#endif


#if (HAS_M65SC02)
/**************************************************************************
 * CPU-specific set_info
 **************************************************************************/

void m65sc02_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = m65sc02_init;				break;
		case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = m65sc02_dasm;		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "M65SC02");				break;
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s, "Metal Oxid Semiconductor MOS 6502"); break;
		case CPUINFO_STR_CORE_VERSION:					strcpy(info->s, "1.0beta");				break;
		case CPUINFO_STR_CORE_FILE:						strcpy(info->s, __FILE__);				break;
		case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s, "Copyright Juergen Buchmueller\nCopyright Peter Trauner\nall rights reserved."); break;

		default:										m65c02_get_info(state, info);			break;
	}
}
#endif


#if (HAS_DECO16)
/**************************************************************************
 * CPU-specific set_info
 **************************************************************************/

static void deco16_set_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are set as 64-bit signed integers --- */
		case CPUINFO_INT_INPUT_STATE + M6502_IRQ_LINE:		deco16_set_irq_line(M6502_IRQ_LINE, info->i); break;
		case CPUINFO_INT_INPUT_STATE + M6502_SET_OVERFLOW:	deco16_set_irq_line(M6502_SET_OVERFLOW, info->i); break;
		case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:		deco16_set_irq_line(INPUT_LINE_NMI, info->i); break;

		default:										m6502_set_info(state, info);			break;
	}
}

void deco16_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 8;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 8;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_SET_INFO:						info->setinfo = deco16_set_info;		break;
		case CPUINFO_PTR_INIT:							info->init = deco16_init;				break;
		case CPUINFO_PTR_RESET:							info->reset = deco16_reset;				break;
		case CPUINFO_PTR_EXECUTE:						info->execute = deco16_execute;			break;
		case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = deco16_dasm;		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "DECO CPU16");			break;
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s, "DECO");				break;
		case CPUINFO_STR_CORE_VERSION:					strcpy(info->s, "0.1");					break;
		case CPUINFO_STR_CORE_FILE:						strcpy(info->s, __FILE__);				break;
		case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s, "Copyright Juergen Buchmueller\nCopyright Bryan McPhail\nall rights reserved."); break;

		default:										m6502_get_info(state, info);			break;
	}
}
#endif
