/***************************************************************************

    mips3fe.c

    Front-end for MIPS3 recompiler

***************************************************************************/

#include <stddef.h>
#include "cpuintrf.h"
#include "mips3fe.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define MAX_STACK_DEPTH		100



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _pc_stack_entry pc_stack_entry;
struct _pc_stack_entry
{
	UINT32		targetpc;
	UINT32		srcpc;
};



/***************************************************************************
    LOCAL VARAIBLES
***************************************************************************/

static mips3_opcode_desc *desc_free_list;
static mips3_opcode_desc **desc_array;
static UINT32 desc_array_size;
static UINT32 dummy_nop = 0;



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static mips3_opcode_desc **build_sequence(mips3_opcode_desc **tailptr, int start, int end, UINT32 endflag, int maxseq);

static int describe_one(mips3_state *mips, UINT32 curpc, mips3_opcode_desc *curdesc);
static void describe_instruction(mips3_state *mips, UINT32 pc, UINT32 op, mips3_opcode_desc *desc);
static void describe_instruction_special(mips3_state *mips, UINT32 pc, UINT32 op, mips3_opcode_desc *desc);
static void describe_instruction_regimm(mips3_state *mips, UINT32 pc, UINT32 op, mips3_opcode_desc *desc);
static void describe_instruction_idt(mips3_state *mips, UINT32 pc, UINT32 op, mips3_opcode_desc *desc);
static void describe_instruction_cop0(mips3_state *mips, UINT32 pc, UINT32 op, mips3_opcode_desc *desc);
static void describe_instruction_cop1(mips3_state *mips, UINT32 pc, UINT32 op, mips3_opcode_desc *desc);
static void describe_instruction_cop1x(mips3_state *mips, UINT32 pc, UINT32 op, mips3_opcode_desc *desc);
static void describe_instruction_cop2(mips3_state *mips, UINT32 pc, UINT32 op, mips3_opcode_desc *desc);



/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE mips3_opcode_desc *desc_alloc(void)
{
	mips3_opcode_desc *desc = desc_free_list;

	/* pull a description off of the free list or allocate a new one */
	if (desc != NULL)
		desc_free_list = desc->next;
	else
		desc = malloc_or_die(sizeof(*desc));
	return desc;
}


INLINE void desc_free(mips3_opcode_desc *desc)
{
	/* just put ourselves on the free list */
	desc->next = desc_free_list;
	desc_free_list = desc;
}


INLINE void invalid_opcode(mips3_opcode_desc *desc)
{
	desc->flags |= IDESC_WILL_CAUSE_EXCEPTION | IDESC_END_SEQUENCE | IDESC_INVALID_OPCODE;
}


INLINE int is_fixed_mapped(UINT32 pc)
{
	return (pc >= 0x80000000 && pc < 0xc0000000);
}



/***************************************************************************
    CORE IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    mips3fe_init - initialization
-------------------------------------------------*/

void mips3fe_init(void)
{
	desc_free_list = NULL;
	desc_array = NULL;
	desc_array_size = 0;
}


/*-------------------------------------------------
    mipsfe_exit - clean up after ourselves
-------------------------------------------------*/

void mips3fe_exit(void)
{
	/* free our free list of descriptions */
	while (desc_free_list != NULL)
	{
		mips3_opcode_desc *freeme = desc_free_list;
		desc_free_list = desc_free_list->next;
		free(freeme);
	}

	/* free the description array */
	if (desc_array != NULL)
		free(desc_array);
	desc_array_size = 0;
}


/*-------------------------------------------------
    mips3fe_describe_sequence - describe a
    sequence of instructions that falls within
    the given range
-------------------------------------------------*/

mips3_opcode_desc *mips3fe_describe_sequence(mips3_state *mips, UINT32 startpc, UINT32 minpc, UINT32 maxpc, int maxseq)
{
	pc_stack_entry pcstack[MAX_STACK_DEPTH];
	pc_stack_entry *pcstackptr = &pcstack[0];
	mips3_opcode_desc *head, **tailptr;

	/* make sure our array is big enough */
	if ((maxpc - minpc) / 4 + 10 > desc_array_size)
	{
		desc_array_size = (maxpc - minpc) / 4 + 10;
		desc_array = realloc(desc_array, desc_array_size * sizeof(desc_array[0]));
		memset(desc_array, 0, desc_array_size * sizeof(desc_array[0]));
	}

	/* add the initial PC to the stack */
	pcstackptr->srcpc = 0;
	pcstackptr->targetpc = startpc;
	pcstackptr++;

	/* loop while we still have a stack */
	while (pcstackptr != &pcstack[0])
	{
		pc_stack_entry *curstack = --pcstackptr;
		mips3_opcode_desc *prevdesc = NULL;
		mips3_opcode_desc *curdesc;
		UINT32 curpc;

		/* if we've already hit this PC, just mark it a branch target and continue */
		curdesc = desc_array[(curstack->targetpc - minpc) / 4];
		if (curdesc != NULL)
		{
			curdesc->flags |= IDESC_IS_BRANCH_TARGET;

			/* if the branch crosses a page boundary, mark the target as needing to revalidate */
			if (!is_fixed_mapped(curdesc->pc) && ((curstack->srcpc ^ curdesc->pc) & ~0xfff) != 0)
				curdesc->flags |= IDESC_VALIDATE_TLB | IDESC_CAN_CAUSE_EXCEPTION;

			/* continue processing */
			continue;
		}

		/* loop until we exit the block */
		for (curpc = curstack->targetpc; curpc < maxpc && desc_array[(curpc - minpc) / 4] == NULL; curpc += 4)
		{
			/* describe one instruction */
			desc_array[(curpc - minpc) / 4] = curdesc = desc_alloc();
			if (!describe_one(mips, curpc, curdesc))
				break;

			/* first instruction in a block is always a branch target */
			if (curpc == curstack->targetpc)
				curdesc->flags |= IDESC_IS_BRANCH_TARGET;

			/* first instruction in a block or page must always validate the TLB */
			if (!is_fixed_mapped(curpc) && (curpc == startpc || (curpc & 0xfff) == 0))
				curdesc->flags |= IDESC_VALIDATE_TLB | IDESC_CAN_CAUSE_EXCEPTION;

			/* if we are a branch, add the branch target to our stack */
			if ((curdesc->flags & IDESC_IS_BRANCH) && curdesc->targetpc >= minpc && curdesc->targetpc < maxpc && pcstackptr < &pcstack[MAX_STACK_DEPTH])
			{
				curdesc->flags |= IDESC_INTRABLOCK_BRANCH;
				pcstackptr->srcpc = curdesc->pc;
				pcstackptr->targetpc = curdesc->targetpc;
				pcstackptr++;
			}

			/* we now become the previous desc */
			prevdesc = curdesc;

			/* if we're done, we're done */
			if (curdesc->flags & IDESC_END_SEQUENCE)
				break;

			/* if this is a likely branch, advance an extra 4 */
			if (curdesc->flags & IDESC_IS_LIKELY_BRANCH)
				curpc += 4;
		}
	}

	/* now build the list of descriptions in order */
	/* first from startpc -> maxpc, then from minpc -> startpc */
	head = NULL;
	tailptr = build_sequence(&head, (startpc - minpc) / 4, (maxpc - minpc) / 4, IDESC_REDISPATCH, maxseq);
	tailptr = build_sequence(tailptr, (minpc - minpc) / 4, (startpc - minpc) / 4, IDESC_RETURN_TO_START, maxseq);
	return head;
}


/*-------------------------------------------------
    mips3fe_release_descriptions - release a
    list of descriptions
-------------------------------------------------*/

void mips3fe_release_descriptions(mips3_opcode_desc *desc)
{
	while (desc != NULL)
	{
		mips3_opcode_desc *freeme = desc;
		if (desc->delay != NULL)
			mips3fe_release_descriptions(desc->delay);
		desc = desc->next;
		desc_free(freeme);
	}
}



/***************************************************************************
    INTERNAL HELPERS
***************************************************************************/

/*-------------------------------------------------
    accumulate_live_info_forwards - recursively
    accumulate live register liveness information
    walking in a forward direction
-------------------------------------------------*/

static void accumulate_live_info_forwards(mips3_opcode_desc *desc, UINT64 *gprread, UINT64 *gprwrite, UINT64 *fprread, UINT64 *fprwrite)
{
	/* set the initial information */
	desc->gpr.liveread = (*gprread |= desc->gpr.used);
	desc->gpr.livewrite = (*gprwrite |= desc->gpr.modified);
	desc->fpr.liveread = (*fprread |= desc->fpr.used);
	desc->fpr.livewrite = (*fprwrite |= desc->fpr.modified);

	/* recursively handle delay slots */
	if (desc->delay != NULL)
		accumulate_live_info_forwards(desc->delay, gprread, gprwrite, fprread, fprwrite);
}


/*-------------------------------------------------
    accumulate_live_info_backwards - recursively
    accumulate live register liveness information
    walking in a backwards direction
-------------------------------------------------*/

static void accumulate_live_info_backwards(mips3_opcode_desc *desc, UINT64 *gprread, UINT64 *gprwrite, UINT64 *fprread, UINT64 *fprwrite)
{
	/* recursively handle delay slots */
	if (desc->delay != NULL)
		accumulate_live_info_backwards(desc->delay, gprread, gprwrite, fprread, fprwrite);

	/* accumulate the info from this instruction */
	desc->gpr.liveread &= (*gprread |= desc->gpr.used);
	desc->gpr.livewrite &= (*gprwrite |= desc->gpr.modified);
	desc->fpr.liveread &= (*fprread |= desc->fpr.used);
	desc->fpr.livewrite &= (*fprwrite |= desc->fpr.modified);
}


/*-------------------------------------------------
    build_sequence - build an ordered sequence
    of instructions
-------------------------------------------------*/

static mips3_opcode_desc **build_sequence(mips3_opcode_desc **tailptr, int start, int end, UINT32 endflag, int maxseq)
{
	UINT64 gprread = 0, gprwrite = 0;
	UINT64 fprread = 0, fprwrite = 0;
	int consecutive = 0;
	int seqstart = -1;
	int descnum;

	/* iterate in order from start to end, picking up all non-NULL instructions */
	for (descnum = start; descnum < end; descnum++)
		if (desc_array[descnum] != NULL)
		{
			mips3_opcode_desc *desc = desc_array[descnum];
			mips3_opcode_desc *next = NULL;
			int nextoffs;

			/* determine the next instruction */
			nextoffs = (desc->flags & IDESC_IS_LIKELY_BRANCH) ? 2 : 1;
			if (descnum + nextoffs < end)
				next = desc_array[descnum + nextoffs];

			/* start a new sequence if we aren't already in the middle of one */
			if (seqstart == -1)
				seqstart = descnum;

			/* if this is a likely branch and there is a branch into the delay slot, mark it as */
			/* end-of-sequence so that the common flow will branch over it */
			if ((desc->flags & IDESC_IS_LIKELY_BRANCH) && desc_array[descnum + 1] != NULL)
				desc->flags |= IDESC_END_SEQUENCE;

			/* if the next instruction is a branch target, mark this instruction as end of sequence */
			if (next != NULL && (next->flags & IDESC_IS_BRANCH_TARGET))
				desc->flags |= IDESC_END_SEQUENCE;

			/* if we are the last instruction, indicate end-of-sequence and redispatch */
			if (next == NULL)
				desc->flags |= IDESC_END_SEQUENCE | endflag;

			/* if we exceed the maximum consecutive count, cut off the sequence */
			if (!(desc->flags & IDESC_END_SEQUENCE) && ++consecutive >= maxseq)
				desc->flags |= IDESC_END_SEQUENCE;
			if (desc->flags & IDESC_END_SEQUENCE)
				consecutive = 0;

			/* update register accumulators */
			accumulate_live_info_forwards(desc, &gprread, &gprwrite, &fprread, &fprwrite);

			/* if this is the end of a sequence, work backwards */
			if (desc->flags & IDESC_END_SEQUENCE)
			{
				int backdesc;

				/* loop until all the registers have been accounted for */
				gprread = gprwrite = 0;
				fprread = fprwrite = 0;
				for (backdesc = descnum; backdesc != seqstart - 1; backdesc--)
					if (desc_array[backdesc] != NULL)
						accumulate_live_info_backwards(desc_array[backdesc], &gprread, &gprwrite, &fprread, &fprwrite);

				/* reset the register states */
				seqstart = -1;
				gprread = gprwrite = 0;
				fprread = fprwrite = 0;
			}

			/* add us to the end of the list and clear our array slot */
			*tailptr = desc;
			tailptr = &desc->next;
		}

	/* zap the array */
	memset(&desc_array[start], 0, (end - start) * sizeof(desc_array[0]));

	/* return the final tailptr */
	return tailptr;
}



/***************************************************************************
    INSTRUCTION PARSERS
***************************************************************************/

/*-------------------------------------------------
    describe_one - describe a single instruction
-------------------------------------------------*/

static int describe_one(mips3_state *mips, UINT32 curpc, mips3_opcode_desc *curdesc)
{
	/* fill in default values */
	curdesc->next = NULL;
	curdesc->delay = NULL;
	curdesc->pc = curpc;
	curdesc->targetpc = ~0;
	curdesc->flags = 0;
	curdesc->cycles = 1;
	curdesc->gpr.used = 0;
	curdesc->gpr.modified = 0;
	curdesc->gpr.liveread = 0;
	curdesc->gpr.livewrite = 0;
	curdesc->fpr.used = 0;
	curdesc->fpr.modified = 0;
	curdesc->fpr.liveread = 0;
	curdesc->fpr.livewrite = 0;

	/* determine the physical PC and pointer to opcode for this instruction */
	curdesc->physpc = curpc;
	if (!mips3com_translate_address(mips, ADDRESS_SPACE_PROGRAM, &curdesc->physpc))
	{
		/* if we're not mapped, generate an exception */
		curdesc->opptr = &dummy_nop;
		curdesc->flags = IDESC_CAN_CAUSE_EXCEPTION | IDESC_END_SEQUENCE | IDESC_REDISPATCH;
		return FALSE;
	}
	curdesc->opptr = memory_get_op_ptr(cpu_getactivecpu(), curdesc->physpc, FALSE);
	assert(curdesc->opptr != NULL);

	/* describe this instruction */
	describe_instruction(mips, curpc, *curdesc->opptr, curdesc);

	/* if we are a branch, describe the delay slot as well */
	if (curdesc->flags & IDESC_IS_BRANCH)
	{
		curdesc->delay = desc_alloc();
		describe_one(mips, curpc + 4, curdesc->delay);
		curdesc->delay->flags |= IDESC_IN_DELAY_SLOT;
		curdesc->delay->branch = curdesc;
	}
	return TRUE;
}


/*-------------------------------------------------
    describe_instruction - build a description
    of a single instruction
-------------------------------------------------*/

static void describe_instruction(mips3_state *mips, UINT32 pc, UINT32 op, mips3_opcode_desc *desc)
{
	/* parse the instruction */
	switch (op >> 26)
	{
		case 0x00:	/* SPECIAL */
			describe_instruction_special(mips, pc, op, desc);
			break;

		case 0x01:	/* REGIMM */
			describe_instruction_regimm(mips, pc, op, desc);
			break;

		case 0x10:	/* COP0 */
			describe_instruction_cop0(mips, pc, op, desc);
			break;

		case 0x11:	/* COP1 */
			describe_instruction_cop1(mips, pc, op, desc);
			break;

		case 0x12:	/* COP2 */
			describe_instruction_cop2(mips, pc, op, desc);
			break;

		case 0x13:	/* COP1X - MIPS IV */
			if (mips->flavor < MIPS3_TYPE_MIPS_IV)
			{
				invalid_opcode(desc);
				break;
			}
			describe_instruction_cop1x(mips, pc, op, desc);
			break;

		case 0x1c:	/* IDT-specific opcodes: mad/madu/mul on R4640/4650, msub on RC32364 */
			describe_instruction_idt(mips, pc, op, desc);
			break;

		case 0x02:	/* J */
			desc->flags |= IDESC_IS_UNCONDITIONAL_BRANCH | IDESC_END_SEQUENCE;
			desc->targetpc = (pc & 0xf0000000) | (LIMMVAL << 2);
			break;

		case 0x03:	/* JAL */
			desc->gpr.modified |= REGFLAG_R(31);
			desc->flags |= IDESC_IS_UNCONDITIONAL_BRANCH | IDESC_END_SEQUENCE;
			desc->targetpc = (pc & 0xf0000000) | (LIMMVAL << 2);
			break;

		case 0x14:	/* BEQL */
		case 0x15:	/* BNEL */
			desc->flags |= IDESC_IS_LIKELY_BRANCH;
			/* fall through */

		case 0x04:	/* BEQ */
		case 0x05:	/* BNE */
			if (((op >> 26) == 0x04 || (op >> 26) == 0x14) && RSREG == RTREG)
				desc->flags |= IDESC_IS_UNCONDITIONAL_BRANCH | IDESC_END_SEQUENCE;
			else
			{
				desc->gpr.used |= REGFLAG_R(RSREG) | REGFLAG_R(RTREG);
				desc->flags |= IDESC_IS_CONDITIONAL_BRANCH;
			}
			desc->targetpc = pc + 4 + (SIMMVAL << 2);
			break;

		case 0x16:	/* BLEZL */
		case 0x17:	/* BGTZL */
			desc->flags |= IDESC_IS_LIKELY_BRANCH;
			/* fall through */

		case 0x06:	/* BLEZ */
		case 0x07:	/* BGTZ */
			if (((op >> 26) == 0x06 || (op >> 26) == 0x16) && RSREG == 0)
				desc->flags |= IDESC_IS_UNCONDITIONAL_BRANCH | IDESC_END_SEQUENCE;
			else
			{
				desc->gpr.used |= REGFLAG_R(RSREG);
				desc->flags |= IDESC_IS_CONDITIONAL_BRANCH;
			}
			desc->targetpc = pc + 4 + (SIMMVAL << 2);
			break;

		case 0x08:	/* ADDI */
		case 0x18:	/* DADDI */
			desc->gpr.used |= REGFLAG_R(RSREG);
			desc->gpr.modified |= REGFLAG_R(RTREG);
			desc->flags |= IDESC_CAN_CAUSE_EXCEPTION;
			break;

		case 0x09:	/* ADDIU */
		case 0x0a:	/* SLTI */
		case 0x0b:	/* SLTIU */
		case 0x0c:	/* ANDI */
		case 0x0d:	/* ORI */
		case 0x0e:	/* XORI */
		case 0x19:	/* DADDIU */
			desc->gpr.used |= REGFLAG_R(RSREG);
			desc->gpr.modified |= REGFLAG_R(RTREG);
			break;

		case 0x0f:	/* LUI */
			desc->gpr.modified |= REGFLAG_R(RTREG);
			break;

		case 0x1a:	/* LDL */
		case 0x1b:	/* LDR */
		case 0x22:	/* LWL */
		case 0x26:	/* LWR */
			desc->gpr.used |= REGFLAG_R(RTREG);
		case 0x20:	/* LB */
		case 0x21:	/* LH */
		case 0x23:	/* LW */
		case 0x24:	/* LBU */
		case 0x25:	/* LHU */
		case 0x27:	/* LWU */
		case 0x30:	/* LL */
		case 0x34:	/* LLD */
		case 0x37:	/* LD */
			desc->gpr.used |= REGFLAG_R(RSREG);
			desc->gpr.modified |= REGFLAG_R(RTREG);
			desc->flags |= IDESC_READS_MEMORY | IDESC_CAN_CAUSE_EXCEPTION;
			break;

		case 0x28:	/* SB */
		case 0x29:	/* SH */
		case 0x2a:	/* SWL */
		case 0x2b:	/* SW */
		case 0x2c:	/* SDL */
		case 0x2d:	/* SDR */
		case 0x2e:	/* SWR */
		case 0x38:	/* SC */
		case 0x3c:	/* SCD */
		case 0x3f:	/* SD */
			desc->gpr.used |= REGFLAG_R(RSREG) | REGFLAG_R(RTREG);
			desc->flags |= IDESC_WRITES_MEMORY | IDESC_CAN_CAUSE_EXCEPTION;
			break;

		case 0x31:	/* LWC1 */
		case 0x35:	/* LDC1 */
			desc->gpr.used |= REGFLAG_R(RSREG);
			desc->fpr.modified |= REGFLAG_CPR1(RTREG);
			desc->flags |= IDESC_READS_MEMORY | IDESC_CAN_CAUSE_EXCEPTION;
			break;

		case 0x39:	/* SWC1 */
		case 0x3d:	/* SDC1 */
			desc->gpr.used |= REGFLAG_R(RSREG);
			desc->fpr.used |= REGFLAG_CPR1(RTREG);
			desc->flags |= IDESC_WRITES_MEMORY | IDESC_CAN_CAUSE_EXCEPTION;
			break;

		case 0x32:	/* LWC2 */
		case 0x36:	/* LDC2 */
			desc->gpr.used |= REGFLAG_R(RSREG);
			desc->flags |= IDESC_READS_MEMORY | IDESC_CAN_CAUSE_EXCEPTION;
			break;

		case 0x3a:	/* SWC2 */
		case 0x3e:	/* SDC2 */
			desc->gpr.used |= REGFLAG_R(RSREG);
			desc->flags |= IDESC_WRITES_MEMORY | IDESC_CAN_CAUSE_EXCEPTION;
			break;

		case 0x2f:	/* CACHE */
			/* effective no-op */
			break;

		case 0x33:	/* PREF */
			if (mips->flavor < MIPS3_TYPE_MIPS_IV)
				invalid_opcode(desc);
			else
				; /* effective no-op */
			break;

		default: /* anything else */
			invalid_opcode(desc);
			break;
	}

	/* remove any modifications of R0 */
	desc->gpr.modified &= ~REGFLAG_R(0);
}


/*-------------------------------------------------
    describe_instruction_special - build a
    description of a single instruction in the
    'special' group
-------------------------------------------------*/

static void describe_instruction_special(mips3_state *mips, UINT32 pc, UINT32 op, mips3_opcode_desc *desc)
{
	switch (op & 63)
	{
		case 0x00:	/* SLL */
		case 0x02:	/* SRL */
		case 0x03:	/* SRA */
		case 0x38:	/* DSLL */
		case 0x3a:	/* DSRL */
		case 0x3b:	/* DSRA */
		case 0x3c:	/* DSLL32 */
		case 0x3e:	/* DSRL32 */
		case 0x3f:	/* DSRA32 */
			desc->gpr.used |= REGFLAG_R(RTREG);
			desc->gpr.modified |= REGFLAG_R(RDREG);
			break;

		case 0x0a:	/* MOVZ - MIPS IV */
		case 0x0b:	/* MOVN - MIPS IV */
			if (mips->flavor < MIPS3_TYPE_MIPS_IV)
			{
				invalid_opcode(desc);
				break;
			}
			desc->gpr.used |= REGFLAG_R(RDREG);
		case 0x04:	/* SLLV */
		case 0x06:	/* SRLV */
		case 0x07:	/* SRAV */
		case 0x14:	/* DSLLV */
		case 0x16:	/* DSRLV */
		case 0x17:	/* DSRAV */
		case 0x21:	/* ADDU */
		case 0x23:	/* SUBU */
		case 0x24:	/* AND */
		case 0x25:	/* OR */
		case 0x26:	/* XOR */
		case 0x27:	/* NOR */
		case 0x2a:	/* SLT */
		case 0x2b:	/* SLTU */
		case 0x2d:	/* DADDU */
		case 0x2f:	/* DSUBU */
			desc->gpr.used |= REGFLAG_R(RSREG) | REGFLAG_R(RTREG);
			desc->gpr.modified |= REGFLAG_R(RDREG);
			break;

		case 0x20:	/* ADD */
		case 0x22:	/* SUB */
		case 0x2c:	/* DADD */
		case 0x2e:	/* DSUB */
			desc->gpr.used |= REGFLAG_R(RSREG) | REGFLAG_R(RTREG);
			desc->gpr.modified |= REGFLAG_R(RDREG);
			desc->flags |= IDESC_CAN_CAUSE_EXCEPTION;
			break;

		case 0x30:	/* TGE */
		case 0x31:	/* TGEU */
		case 0x32:	/* TLT */
		case 0x33:	/* TLTU */
		case 0x34:	/* TEQ */
		case 0x36:	/* TNE */
			desc->gpr.used |= REGFLAG_R(RSREG) | REGFLAG_R(RTREG);
			desc->flags |= IDESC_CAN_CAUSE_EXCEPTION;
			break;

		case 0x01:	/* MOVF - MIPS IV */
			if (mips->flavor < MIPS3_TYPE_MIPS_IV)
			{
				invalid_opcode(desc);
				break;
			}
			desc->gpr.used |= REGFLAG_R(RSREG);
			desc->fpr.used |= REGFLAG_FCC;
			desc->gpr.modified |= REGFLAG_R(RDREG);
			break;

		case 0x08:	/* JR */
			desc->gpr.used |= REGFLAG_R(RSREG);
			desc->flags |= IDESC_IS_UNCONDITIONAL_BRANCH | IDESC_END_SEQUENCE;
			break;

		case 0x09:	/* JALR */
			desc->gpr.used |= REGFLAG_R(RSREG);
			desc->gpr.modified |= REGFLAG_R(RDREG);
			desc->flags |= IDESC_IS_UNCONDITIONAL_BRANCH | IDESC_END_SEQUENCE;
			break;

		case 0x10:	/* MFHI */
			desc->gpr.used |= REGFLAG_HI;
			desc->gpr.modified |= REGFLAG_R(RDREG);
			break;

		case 0x11:	/* MTHI */
			desc->gpr.used |= REGFLAG_R(RSREG);
			desc->gpr.modified |= REGFLAG_HI;
			break;

		case 0x12:	/* MFLO */
			desc->gpr.used |= REGFLAG_LO;
			desc->gpr.modified |= REGFLAG_R(RDREG);
			break;

		case 0x13:	/* MTLO */
			desc->gpr.used |= REGFLAG_R(RSREG);
			desc->gpr.modified |= REGFLAG_LO;
			break;

		case 0x18:	/* MULT */
		case 0x19:	/* MULTU */
			desc->gpr.used |= REGFLAG_R(RSREG) | REGFLAG_R(RTREG);
			desc->gpr.modified |= REGFLAG_LO | REGFLAG_HI;
			desc->cycles = 3;
			break;

		case 0x1a:	/* DIV */
		case 0x1b:	/* DIVU */
			desc->gpr.used |= REGFLAG_R(RSREG) | REGFLAG_R(RTREG);
			desc->gpr.modified |= REGFLAG_LO | REGFLAG_HI;
			desc->cycles = 35;
			break;

		case 0x1c:	/* DMULT */
		case 0x1d:	/* DMULTU */
			desc->gpr.used |= REGFLAG_R(RSREG) | REGFLAG_R(RTREG);
			desc->gpr.modified |= REGFLAG_LO | REGFLAG_HI;
			desc->cycles = 7;
			break;

		case 0x1e:	/* DDIV */
		case 0x1f:	/* DDIVU */
			desc->gpr.used |= REGFLAG_R(RSREG) | REGFLAG_R(RTREG);
			desc->gpr.modified |= REGFLAG_LO | REGFLAG_HI;
			desc->cycles = 67;
			break;

		case 0x0c:	/* SYSCALL */
		case 0x0d:	/* BREAK */
			desc->flags |= IDESC_WILL_CAUSE_EXCEPTION | IDESC_END_SEQUENCE;
			break;

		case 0x0f:	/* SYNC */
			/* effective no-op */
			break;

		default:	/* anything else */
			invalid_opcode(desc);
			break;
	}
}


/*-------------------------------------------------
    describe_instruction_regimm - build a
    description of a single instruction in the
    'regimm' group
-------------------------------------------------*/

static void describe_instruction_regimm(mips3_state *mips, UINT32 pc, UINT32 op, mips3_opcode_desc *desc)
{
	switch (RTREG)
	{
		case 0x02:	/* BLTZL */
		case 0x03:	/* BGEZL */
			desc->flags |= IDESC_IS_LIKELY_BRANCH;
			/* fall through */

		case 0x00:	/* BLTZ */
		case 0x01:	/* BGEZ */
			if (RTREG == 0x01 && RSREG == 0)
				desc->flags |= IDESC_IS_UNCONDITIONAL_BRANCH | IDESC_END_SEQUENCE;
			else
			{
				desc->gpr.used |= REGFLAG_R(RSREG);
				desc->flags |= IDESC_IS_CONDITIONAL_BRANCH;
			}
			desc->targetpc = pc + 4 + (SIMMVAL << 2);
			break;

		case 0x08:	/* TGEI */
		case 0x09:	/* TGEIU */
		case 0x0a:	/* TLTI */
		case 0x0b:	/* TLTIU */
		case 0x0c:	/* TEQI */
		case 0x0e:	/* TNEI */
			desc->gpr.used |= REGFLAG_R(RSREG);
			desc->flags |= IDESC_CAN_CAUSE_EXCEPTION;
			break;

		case 0x12:	/* BLTZALL */
		case 0x13:	/* BGEZALL */
			desc->flags |= IDESC_IS_LIKELY_BRANCH;
			/* fall through */

		case 0x10:	/* BLTZAL */
		case 0x11:	/* BGEZAL */
			if (RTREG == 0x11 && RSREG == 0)
				desc->flags |= IDESC_IS_UNCONDITIONAL_BRANCH | IDESC_END_SEQUENCE;
			else
			{
				desc->gpr.used |= REGFLAG_R(RSREG);
				desc->flags |= IDESC_IS_CONDITIONAL_BRANCH;
			}
			desc->gpr.modified |= REGFLAG_R(31);
			desc->targetpc = pc + 4 + (SIMMVAL << 2);
			break;

		default:	/* anything else */
			invalid_opcode(desc);
			break;
	}
}


/*-------------------------------------------------
    describe_instruction_idt - build a
    description of a single instruction in the
    IDT-specific group
-------------------------------------------------*/

static void describe_instruction_idt(mips3_state *mips, UINT32 pc, UINT32 op, mips3_opcode_desc *desc)
{
	switch (op & 0x1f)
	{
		case 0: /* MAD */
		case 1: /* MADU */
			desc->gpr.used |= REGFLAG_R(RSREG) | REGFLAG_R(RTREG) | REGFLAG_LO | REGFLAG_HI;
			desc->gpr.modified |= REGFLAG_LO | REGFLAG_HI;
			break;

		case 2: /* MUL */
			desc->gpr.used |= REGFLAG_R(RSREG) | REGFLAG_R(RTREG);
			desc->gpr.modified |= REGFLAG_R(RDREG);
			desc->cycles = 3;
			break;

		default: /* anything else */
			invalid_opcode(desc);
			break;
	}
}


/*-------------------------------------------------
    describe_instruction_cop0 - build a
    description of a single instruction in the
    COP0 group
-------------------------------------------------*/

static void describe_instruction_cop0(mips3_state *mips, UINT32 pc, UINT32 op, mips3_opcode_desc *desc)
{
	/* any COP0 instruction can potentially cause an exception */
	desc->flags |= IDESC_CAN_CAUSE_EXCEPTION;

	switch (RSREG)
	{
		case 0x00:	/* MFCz */
		case 0x01:	/* DMFCz */
		case 0x02:	/* CFCz */
			desc->gpr.modified |= REGFLAG_R(RTREG);
			break;

		case 0x04:	/* MTCz */
		case 0x05:	/* DMTCz */
		case 0x06:	/* CTCz */
			desc->gpr.used |= REGFLAG_R(RTREG);
			if (RSREG == 0x04 || RSREG == 0x05)
			{
				if (RDREG == COP0_Cause)
					desc->flags |= IDESC_CAN_TRIGGER_SW_INT;
				if (RDREG == COP0_Status)
					desc->flags |= IDESC_CAN_EXPOSE_EXTERNAL_INT;
			}
			break;

		case 0x08:	/* BC */
			switch (RTREG)
			{
				case 0x00:	/* BCzF */
				case 0x01:	/* BCzT */
					desc->flags |= IDESC_IS_CONDITIONAL_BRANCH;
					desc->targetpc = pc + 4 + (SIMMVAL << 2);
					break;

				default: /* anything else */
					invalid_opcode(desc);
					break;
			}
			break;

		case 0x10:	case 0x11:	case 0x12:	case 0x13:	case 0x14:	case 0x15:	case 0x16:	case 0x17:
		case 0x18:	case 0x19:	case 0x1a:	case 0x1b:	case 0x1c:	case 0x1d:	case 0x1e:	case 0x1f:	/* COP */
			switch (op & 0x01ffffff)
			{
				case 0x01:	/* TLBR */
				case 0x08:	/* TLBP */
				case 0x20:	/* WAIT */
					break;

				case 0x02:	/* TLBWI */
				case 0x06:	/* TLBWR */
					desc->flags |= IDESC_MODIFIES_TRANSLATION;
					break;

				case 0x18:	/* ERET */
					desc->flags |= IDESC_IS_UNCONDITIONAL_BRANCH | IDESC_END_SEQUENCE;
					break;

				default: /* anything else */
					invalid_opcode(desc);
					break;
			}
			break;

		default: /* anything else */
			invalid_opcode(desc);
			break;
	}
}


/*-------------------------------------------------
    describe_instruction_cop1 - build a
    description of a single instruction in the
    COP1 group
-------------------------------------------------*/

static void describe_instruction_cop1(mips3_state *mips, UINT32 pc, UINT32 op, mips3_opcode_desc *desc)
{
	/* any COP1 instruction can potentially cause an exception */
//  desc->flags |= IDESC_CAN_CAUSE_EXCEPTION;

	switch (RSREG)
	{
		case 0x00:	/* MFCz */
		case 0x01:	/* DMFCz */
			desc->fpr.used |= REGFLAG_CPR1(RDREG);
			desc->gpr.modified |= REGFLAG_R(RTREG);
			break;

		case 0x02:	/* CFCz */
			desc->gpr.modified |= REGFLAG_R(RTREG);
			break;

		case 0x04:	/* MTCz */
		case 0x05:	/* DMTCz */
			desc->gpr.used |= REGFLAG_R(RTREG);
			desc->fpr.modified |= REGFLAG_CPR1(RDREG);
			break;

		case 0x06:	/* CTCz */
			desc->gpr.used |= REGFLAG_R(RTREG);
			break;

		case 0x08:	/* BC */
			switch (RTREG)
			{
				case 0x02:	/* BCzFL */
				case 0x03:	/* BCzTL */
					desc->flags |= IDESC_IS_LIKELY_BRANCH;
					/* fall through */

				case 0x00:	/* BCzF */
				case 0x01:	/* BCzT */
					desc->fpr.used |= REGFLAG_FCC;
					desc->flags |= IDESC_IS_CONDITIONAL_BRANCH;
					desc->targetpc = pc + 4 + (SIMMVAL << 2);
					break;

				default: /* anything else */
					invalid_opcode(desc);
					break;
			}
			break;

		case 0x10:	case 0x11:	case 0x12:	case 0x13:	case 0x14:	case 0x15:	case 0x16:	case 0x17:
		case 0x18:	case 0x19:	case 0x1a:	case 0x1b:	case 0x1c:	case 0x1d:	case 0x1e:	case 0x1f:	/* COP */
			switch (op & 0x3f)
			{
				case 0x12:	/* MOVZ - MIPS IV */
				case 0x13:	/* MOVN - MIPS IV */
					if (mips->flavor < MIPS3_TYPE_MIPS_IV)
					{
						invalid_opcode(desc);
						break;
					}
				case 0x00:	/* ADD */
				case 0x01:	/* SUB */
				case 0x02:	/* MUL */
				case 0x03:	/* DIV */
					desc->fpr.used |= REGFLAG_CPR1(FSREG) | REGFLAG_CPR1(FTREG);
					desc->fpr.modified |= REGFLAG_CPR1(FDREG);
					break;

				case 0x15:	/* RECIP - MIPS IV */
				case 0x16:	/* RSQRT - MIPS IV */
					if (mips->flavor < MIPS3_TYPE_MIPS_IV)
					{
						invalid_opcode(desc);
						break;
					}
				case 0x04:	/* SQRT */
				case 0x05:	/* ABS */
				case 0x06:	/* MOV */
				case 0x07:	/* NEG */
				case 0x08:	/* ROUND.L */
				case 0x09:	/* TRUNC.L */
				case 0x0a:	/* CEIL.L */
				case 0x0b:	/* FLOOR.L */
				case 0x0c:	/* ROUND.W */
				case 0x0d:	/* TRUNC.W */
				case 0x0e:	/* CEIL.W */
				case 0x0f:	/* FLOOR.W */
				case 0x20:	/* CVT.S */
				case 0x21:	/* CVT.D */
				case 0x24:	/* CVT.W */
				case 0x25:	/* CVT.L */
					desc->fpr.used |= REGFLAG_CPR1(FSREG);
					desc->fpr.modified |= REGFLAG_CPR1(FDREG);
					break;

				case 0x11:	/* MOVT/F - MIPS IV */
					if (mips->flavor < MIPS3_TYPE_MIPS_IV)
					{
						invalid_opcode(desc);
						break;
					}
					desc->fpr.used |= REGFLAG_CPR1(FSREG) | REGFLAG_FCC;
					desc->fpr.modified |= REGFLAG_CPR1(FDREG);
					break;

				case 0x30:	case 0x38:	/* C.F */
				case 0x31:	case 0x39:	/* C.UN */
					desc->fpr.modified |= REGFLAG_FCC;
					break;

				case 0x32:	case 0x3a:	/* C.EQ */
				case 0x33:	case 0x3b:	/* C.UEQ */
				case 0x34:	case 0x3c:	/* C.OLT */
				case 0x35:	case 0x3d:	/* C.ULT */
				case 0x36:	case 0x3e:	/* C.OLE */
				case 0x37:	case 0x3f:	/* C.ULE */
					desc->fpr.used |= REGFLAG_CPR1(FSREG) | REGFLAG_CPR1(FTREG);
					desc->fpr.modified |= REGFLAG_FCC;
					break;

				default: /* anything else */
					invalid_opcode(desc);
					break;
			}
			break;

		default: /* anything else */
			invalid_opcode(desc);
			break;
	}
}


/*-------------------------------------------------
    describe_instruction_cop1x - build a
    description of a single instruction in the
    COP1X group
-------------------------------------------------*/

static void describe_instruction_cop1x(mips3_state *mips, UINT32 pc, UINT32 op, mips3_opcode_desc *desc)
{
	/* any COP1 instruction can potentially cause an exception */
//  desc->flags |= IDESC_CAN_CAUSE_EXCEPTION;

	switch (op & 0x3f)
	{
		case 0x00:	/* LWXC1 */
		case 0x01:	/* LDXC1 */
			desc->gpr.used |= REGFLAG_R(RSREG) | REGFLAG_R(RTREG);
			desc->fpr.modified |= REGFLAG_CPR1(FDREG);
			desc->flags |= IDESC_READS_MEMORY;
			break;

		case 0x02:	/* SWXC1 */
		case 0x03:	/* SDXC1 */
			desc->gpr.used |= REGFLAG_R(RSREG) | REGFLAG_R(RTREG);
			desc->fpr.used |= REGFLAG_CPR1(FDREG);
			desc->flags |= IDESC_WRITES_MEMORY;
			break;

		case 0x0f:	/* PREFX */
			/* effective no-op */
			break;

		case 0x20:	case 0x21:	/* MADD */
		case 0x28:	case 0x29:	/* MSUB */
		case 0x30:	case 0x31:	/* NMADD */
		case 0x32:	case 0x39:	/* NMSUB */
			desc->fpr.used |= REGFLAG_CPR1(FSREG) | REGFLAG_CPR1(FTREG) | REGFLAG_CPR1(FRREG);
			desc->fpr.modified |= REGFLAG_CPR1(FDREG);
			break;

		default: /* anything else */
			invalid_opcode(desc);
			break;
	}
}


/*-------------------------------------------------
    describe_instruction_cop2 - build a
    description of a single instruction in the
    COP2 group
-------------------------------------------------*/

static void describe_instruction_cop2(mips3_state *mips, UINT32 pc, UINT32 op, mips3_opcode_desc *desc)
{
	/* any COP2 instruction can potentially cause an exception */
	desc->flags |= IDESC_CAN_CAUSE_EXCEPTION;

	switch (RSREG)
	{
		case 0x00:	/* MFCz */
		case 0x01:	/* DMFCz */
		case 0x02:	/* CFCz */
			desc->gpr.modified |= REGFLAG_R(RTREG);
			break;

		case 0x04:	/* MTCz */
		case 0x05:	/* DMTCz */
		case 0x06:	/* CTCz */
			desc->gpr.used |= REGFLAG_R(RTREG);
			break;

		case 0x08:	/* BC */
			switch (RTREG)
			{
				case 0x00:	/* BCzF */
				case 0x01:	/* BCzT */
					desc->flags |= IDESC_IS_CONDITIONAL_BRANCH;
					desc->targetpc = pc + 4 + (SIMMVAL << 2);
					break;

				default: /* anything else */
					invalid_opcode(desc);
					break;
			}
			break;

		default: /* anything else */
			invalid_opcode(desc);
			break;
	}
}
