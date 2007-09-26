/***************************************************************************

    mips3fe.h

    Front-end for MIPS3 recompiler

***************************************************************************/

#ifndef __MIPS3FE_H__
#define __MIPS3FE_H__

#include "mips3com.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

/* register flags */
#define REGFLAG_R(n)					((UINT64)1 << (n))
#define REGFLAG_HI						(REGFLAG_R(32))
#define REGFLAG_LO						(REGFLAG_R(33))
#define REGFLAG_CPR1(n)					((UINT64)1 << (n))
#define REGFLAG_FCC						(REGFLAG_CPR1(32))

/* instruction description flags */
#define IDESC_IS_UNCONDITIONAL_BRANCH	0x00000001		/* instruction is unconditional branch */
#define IDESC_IS_CONDITIONAL_BRANCH		0x00000002		/* instruction is conditional branch */
#define IDESC_IS_LIKELY_BRANCH			0x00000004		/* instruction is likely branch */
#define IDESC_CAN_TRIGGER_SW_INT		0x00000008		/* instruction can trigger a software interrupt */
#define IDESC_CAN_EXPOSE_EXTERNAL_INT	0x00000010		/* instruction can expose an external interrupt */
#define IDESC_CAN_CAUSE_EXCEPTION		0x00000020		/* instruction may generate exception */
#define IDESC_WILL_CAUSE_EXCEPTION		0x00000040		/* instruction will generate exception */
#define IDESC_INVALID_OPCODE			0x00000080		/* instruction is invalid */
#define IDESC_READS_MEMORY				0x00000100		/* instruction reads memory */
#define IDESC_WRITES_MEMORY				0x00000200		/* instruction writes memory */
#define IDESC_VALIDATE_TLB				0x00000400		/* instruction must validate TLB */
#define IDESC_MODIFIES_TRANSLATION		0x00000800		/* instruction modifies the TLB */
#define IDESC_REDISPATCH				0x00001000		/* instruction must redispatch */
#define IDESC_RETURN_TO_START			0x00002000		/* instruction must jump back to the beginning */
#define IDESC_IS_BRANCH_TARGET			0x00004000		/* instruction is the target of a branch */
#define IDESC_IN_DELAY_SLOT				0x00008000		/* instruction is in the delay slot of a branch */
#define IDESC_INTRABLOCK_BRANCH			0x00010000		/* instruction branches within the block */
#define IDESC_END_SEQUENCE				0x00020000		/* this is the last instruction in a sequence */

/* pre-defined groups of flags */
#define IDESC_IS_BRANCH					(IDESC_IS_UNCONDITIONAL_BRANCH | IDESC_IS_CONDITIONAL_BRANCH)



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _mips3_opcode_desc mips3_opcode_desc;
struct _mips3_opcode_desc
{
	mips3_opcode_desc *next;					/* pointer to next description */
	mips3_opcode_desc *delay;					/* pointer to delay slot description */
	mips3_opcode_desc *branch;					/* pointer back to branch description for delay slots */

	UINT32			pc;							/* PC of this opcode */
	UINT32			physpc;						/* physical PC of this opcode */
	UINT32			targetpc;					/* target PC if we are a branch */

	UINT32 *		opptr;						/* physical memory pointer */
	UINT32			flags;						/* opcode flags */
	UINT32			cycles;						/* number of cycles needed to execute */

	UINT64			intused;					/* integer registers used */
	UINT64			intmod;						/* integer registers modified */
	UINT64			fpused;						/* FPU registers used */
	UINT64			fpmod;						/* FPU registers modified */
};



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

void mips3fe_init(void);
void mips3fe_exit(void);

mips3_opcode_desc *mips3fe_describe_sequence(mips3_state *mips, UINT32 startpc, UINT32 minpc, UINT32 maxpc);
void mips3fe_release_descriptions(mips3_opcode_desc *desc);

#endif
