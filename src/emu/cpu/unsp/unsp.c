/**************************\
*
*   SunPlus u'nSP skeleton
*
*    by Harmony
*
\**************************/

#include "cpuintrf.h"
#include "debugger.h"
#include "unsp.h"

INLINE unsp_state *get_safe_token(const device_config *device)
{
    assert(device != NULL);
    assert(device->token != NULL);
    assert(device->type == CPU);
    assert(cpu_get_type(device) == CPU_UNSP);
    return (unsp_state *)device->token;
}

static void unsp_set_irq_line(unsp_state *unsp, int irqline, int state);

/*****************************************************************************/

#define OP0		(op >> 12)
#define OPA		((op >> 9) & 7)
#define OP1		((op >> 6) & 7)
#define OPN		((op >> 3) & 7)
#define OPB		(op & 7)
#define OPIMM	(op & 0x3f)
#define OP2X	((OP0 < 14 && OP1 == 4 && (OPN >= 1 && OPN <= 3)) || (OP0 == 15 && (OP1 == 1 || OP1 == 2)))
#define UNSP_REG(reg)		unsp->r[UNSP_##reg - 1]
#define UNSP_REG_I(reg)		unsp->r[reg]
#define UNSP_LREG_I(reg)	(((UNSP_REG(SR) << 6) & 0x3f0000) | UNSP_REG_I(reg))
#define UNSP_LPC			(((UNSP_REG(SR) & 0x3f) << 16) | UNSP_REG(PC))

/*****************************************************************************/

static void unimplemented_opcode(unsp_state *unsp, UINT16 op)
{
    fatalerror("UNSP: unknown opcode %04x at %04x\n", op, UNSP_LPC << 1);
}

/*****************************************************************************/

INLINE UINT16 READ16(unsp_state *unsp, UINT32 address)
{
	return memory_read_word_16be(unsp->program, address << 1);
}

INLINE void WRITE16(unsp_state *unsp, UINT32 address, UINT16 data)
{
	memory_write_word_16be(unsp->program, address << 1, data);
}

/*****************************************************************************/

static CPU_INIT( unsp )
{
    unsp_state *unsp = get_safe_token(device);
    memset(unsp->r, 0, sizeof(UINT16) * UNSP_GPR_COUNT);

    unsp->device = device;
    unsp->program = memory_find_address_space(device, ADDRESS_SPACE_PROGRAM);
    unsp->irq = 0;
    unsp->fiq = 0;
}

static CPU_EXIT( unsp )
{
}

static CPU_RESET( unsp )
{
    unsp_state *unsp = get_safe_token(device);
    memset(unsp->r, 0, sizeof(UINT16) * UNSP_GPR_COUNT);

    UNSP_REG(PC) = READ16(unsp, 0xfff7);
}

/*****************************************************************************/

static void unsp_update_nz(unsp_state *unsp, UINT32 value)
{
	UNSP_REG(SR) &= ~0x0300;
	if(value & 0x8000)
	{
		UNSP_REG(SR) |= 0x0200;
	}
	if((UINT16)value == 0)
	{
		UNSP_REG(SR) |= 0x0100;
	}
}

// Cribbed from emu/cpu/arm7/*
#define SIGN_BITS_DIFFER(a, b) (((a) ^ (b)) >> 15)
#define IsNeg(i) ((i) >> 15)
#define IsPos(i) ((~(i)) >> 15)

static void unsp_update_nzsc(unsp_state *unsp, UINT32 value, UINT16 r0, UINT16 r1)
{
	UNSP_REG(SR) &= ~0x00c0;
	unsp_update_nz(unsp, value);
	if(value != (UINT16)value)
	{
		UNSP_REG(SR) |= 0x0040;
	}

	if((INT16)r0 < (INT16)r1)
	{
		UNSP_REG(SR) |= 0x0080;
	}
}

static void unsp_push(unsp_state *unsp, UINT16 value, UINT16 *reg)
{
	WRITE16(unsp, (*reg)--, value);
}

static UINT16 unsp_pop(unsp_state *unsp, UINT16 *reg)
{
	return READ16(unsp, ++(*reg));
}

static CPU_EXECUTE( unsp )
{
    unsp_state *unsp = get_safe_token(device);
    UINT32 op;
    UINT32 lres;
    UINT16 r0, r1;
	lres = 0;

    unsp->icount = cycles;

    while (unsp->icount > 0)
    {
		int jumped = 0;
        debugger_instruction_hook(device, UNSP_LPC<<1);

        op = READ16(unsp, UNSP_LPC);

		if(OP0 < 0xf && OPA == 0x7 && OP1 < 2)
		{
			//print("%s %04x", jmp[OP0], OP1 ? (pc - OPIMM*2) : (pc + OPIMM*2));
			switch(OP0)
			{
				case 0: // JB
					if(!(UNSP_REG(SR) & 0x0040))
					{
						UNSP_REG(PC)++;
						UNSP_REG(PC) += (OP1 == 0) ? OPIMM : (0 - OPIMM);
						jumped = 1;
					}
					break;
				case 1: // JAE
					if(UNSP_REG(SR) & 0x0040)
					{
						UNSP_REG(PC)++;
						UNSP_REG(PC) += (OP1 == 0) ? OPIMM : (0 - OPIMM);
						jumped = 1;
					}
					break;
				case 2: // JGE
					if(!(UNSP_REG(SR) & 0x0080))
					{
						UNSP_REG(PC)++;
						UNSP_REG(PC) += (OP1 == 0) ? OPIMM : (0 - OPIMM);
						jumped = 1;
					}
					break;
				case 3: // JL
					if(UNSP_REG(SR) & 0x0080)
					{
						UNSP_REG(PC)++;
						UNSP_REG(PC) += (OP1 == 0) ? OPIMM : (0 - OPIMM);
						jumped = 1;
					}
					break;
				case 4: // JNE
					if(!(UNSP_REG(SR) & 0x0100))
					{
						UNSP_REG(PC)++;
						UNSP_REG(PC) += (OP1 == 0) ? OPIMM : (0 - OPIMM);
						jumped = 1;
					}
					break;
				case 5: // JE
					if(UNSP_REG(SR) & 0x0100)
					{
						UNSP_REG(PC)++;
						UNSP_REG(PC) += (OP1 == 0) ? OPIMM : (0 - OPIMM);
						jumped = 1;
					}
					break;
				case 6: // JPL
					if(!(UNSP_REG(SR) & 0x0200))
					{
						UNSP_REG(PC)++;
						UNSP_REG(PC) += (OP1 == 0) ? OPIMM : (0 - OPIMM);
						jumped = 1;
					}
					break;
				case 7: // JMI
					if(UNSP_REG(SR) & 0x0200)
					{
						UNSP_REG(PC)++;
						UNSP_REG(PC) += (OP1 == 0) ? OPIMM : (0 - OPIMM);
						jumped = 1;
					}
					break;
				case 8: // JBE
					if((UNSP_REG(SR) & 0x0140) != 0x0040)
					{
						UNSP_REG(PC)++;
						UNSP_REG(PC) += (OP1 == 0) ? OPIMM : (0 - OPIMM);
						jumped = 1;
					}
					break;
				case 9: // JA
					if((UNSP_REG(SR) & 0x0140) == 0x0040)
					{
						UNSP_REG(PC)++;
						UNSP_REG(PC) += (OP1 == 0) ? OPIMM : (0 - OPIMM);
						jumped = 1;
					}
					break;
				case 10: // JLE
					if(UNSP_REG(SR) & 0x0180)
					{
						UNSP_REG(PC)++;
						UNSP_REG(PC) += (OP1 == 0) ? OPIMM : (0 - OPIMM);
						jumped = 1;
					}
					break;
				case 11: // JG
					if(!(UNSP_REG(SR) & 0x0180))
					{
						UNSP_REG(PC)++;
						UNSP_REG(PC) += (OP1 == 0) ? OPIMM : (0 - OPIMM);
						jumped = 1;
					}
					break;
				case 14: // JMP
					UNSP_REG(PC)++;
					UNSP_REG(PC) += (OP1 == 0) ? OPIMM : (0 - OPIMM);
					jumped = 1;
					break;
			}
		}
		else
		{
			switch((OP1 << 4) | OP0)
			{
				// ALU, Indexed
				case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x06: case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d:
					switch(OP0)
					{
						case 0: // add r, [bp+imm6]
							r0 = UNSP_REG_I(OPA);
							r1 = READ16(unsp, UNSP_REG(BP) + OPIMM);
							lres = r0 + r1;
							UNSP_REG_I(OPA) = (UINT16)lres;
							if(OPA == (UNSP_PC-1)) jumped = 1;
							unsp_update_nzsc(unsp, lres, r0, r1);
							break;
						case 1: // adc r, [bp+imm6]
							r0 = UNSP_REG_I(OPA);
							r1 = READ16(unsp, UNSP_REG(BP) + OPIMM);
							lres = r0 + r1;
							if(UNSP_REG(SR) & 0x0040)
							{
								lres++;
							}
							UNSP_REG_I(OPA) = (UINT16)lres;
							if(OPA == (UNSP_PC-1)) jumped = 1;
							unsp_update_nzsc(unsp, lres, r0, r1);
							break;
						case 2: // sub r, [bp+imm6]
							r0 = UNSP_REG_I(OPA);
							r1 = READ16(unsp, UNSP_REG(BP) + OPIMM);
							lres = r0 + (~r1 & 0x0000ffff) + 1;
							UNSP_REG_I(OPA) = (UINT16)lres;
							if(OPA == (UNSP_PC-1)) jumped = 1;
							unsp_update_nzsc(unsp, lres, r0, r1);
							break;
						case 3: // sbc r, [bp+imm6]
							r0 = UNSP_REG_I(OPA);
							r1 = READ16(unsp, UNSP_REG(BP) + OPIMM);
							lres = r0 + (~r1 & 0x0000ffff);
							if(UNSP_REG(SR) & 0x0040)
							{
								lres++;
							}
							UNSP_REG_I(OPA) = (UINT16)lres;
							if(OPA == (UNSP_PC-1)) jumped = 1;
							unsp_update_nzsc(unsp, lres, r0, r1);
							break;
						case 4: // cmp r, [bp+imm6]
							r0 = UNSP_REG_I(OPA);
							r1 = READ16(unsp, UNSP_REG(BP) + OPIMM);
							lres = r0 + (~r1 & 0x0000ffff) + 1;
							unsp_update_nzsc(unsp, lres, r0, r1);
							break;
						case 6: // neg r, [bp+imm6]
							r0 = UNSP_REG_I(OPA);
							r1 = READ16(unsp, UNSP_REG(BP) + OPIMM);
							lres = -r1;
							UNSP_REG_I(OPA) = (UINT16)lres;
							if(OPA == (UNSP_PC-1)) jumped = 1;
							unsp_update_nz(unsp, lres);
							break;
						case 8: // xor r, [bp+imm6]
							r0 = UNSP_REG_I(OPA);
							r1 = READ16(unsp, UNSP_REG(BP) + OPIMM);
							lres = r0 ^ r1;
							UNSP_REG_I(OPA) = (UINT16)lres;
							if(OPA == (UNSP_PC-1)) jumped = 1;
							unsp_update_nz(unsp, lres);
							break;
						case 9: // load r, [bp+imm6]
							UNSP_REG_I(OPA) = READ16(unsp, UNSP_REG(BP) + OPIMM);
							if(OPA == (UNSP_PC-1)) jumped = 1;
							unsp_update_nz(unsp, UNSP_REG_I(OPA));
							break;
						case 10: // or r, [bp+imm6]
							r0 = UNSP_REG_I(OPA);
							r1 = READ16(unsp, UNSP_REG(BP) + OPIMM);
							lres = r0 | r1;
							UNSP_REG_I(OPA) = (UINT16)lres;
							if(OPA == (UNSP_PC-1)) jumped = 1;
							unsp_update_nz(unsp, UNSP_REG_I(OPA));
							break;
						case 11: // and r, [bp+imm6]
							r0 = UNSP_REG_I(OPA);
							r1 = READ16(unsp, UNSP_REG(BP) + OPIMM);
							lres = r0 & r1;
							UNSP_REG_I(OPA) = (UINT16)lres;
							if(OPA == (UNSP_PC-1)) jumped = 1;
							unsp_update_nz(unsp, UNSP_REG_I(OPA));
							break;
						case 13: // store r, [bp+imm6]
							WRITE16(unsp, UNSP_REG(BP) + OPIMM, UNSP_REG_I(OPA));
							break;
						default:
							unimplemented_opcode(unsp, op);
							break;
					}
					break;

				// ALU, Immediate
				case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x16: case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c:
					switch(OP0)
					{
						case 0: // add r, imm6
							r0 = UNSP_REG_I(OPA);
							r1 = OPIMM;
							lres = r0 + r1;
							UNSP_REG_I(OPA) = (UINT16)lres;
							unsp_update_nzsc(unsp, lres, r0, r1);
							if(OPA == (UNSP_PC-1)) jumped = 1;
							break;
						case 1: // adc r, imm6
							r0 = UNSP_REG_I(OPA);
							r1 = OPIMM;
							lres = r0 + r1;
							if(UNSP_REG(SR) & 0x0040)
							{
								lres++;
							}
							UNSP_REG_I(OPA) = (UINT16)lres;
							unsp_update_nzsc(unsp, lres, r0, r1);
							if(OPA == (UNSP_PC-1)) jumped = 1;
							break;
						case 2: // sub r, imm6
							r0 = UNSP_REG_I(OPA);
							r1 = OPIMM;
							lres = r0 + (~r1 & 0x0000ffff) + 1;
							UNSP_REG_I(OPA) = (UINT16)lres;
							unsp_update_nzsc(unsp, lres, r0, r1);
							if(OPA == (UNSP_PC-1)) jumped = 1;
							break;
						case 4: // cmp r, imm6
							r0 = UNSP_REG_I(OPA);
							r1 = OPIMM;
							lres = r0 + (~r1 & 0x0000ffff) + 1;
							unsp_update_nzsc(unsp, lres, r0, r1);
							break;
						case 6: // neg r, imm6
							UNSP_REG_I(OPA) = -OPIMM;
							if(OPA == (UNSP_PC-1)) jumped = 1;
							unsp_update_nz(unsp, UNSP_REG_I(OPA));
							break;
						case 8: // xor r, imm6
							r0 = UNSP_REG_I(OPA);
							r1 = OPIMM;
							lres = r0 ^ r1;
							UNSP_REG_I(OPA) = (UINT16)lres;
							unsp_update_nz(unsp, lres);
							if(OPA == (UNSP_PC-1)) jumped = 1;
							break;
						case 9: // load r, imm6
							UNSP_REG_I(OPA) = OPIMM;
							if(OPA == (UNSP_PC-1)) jumped = 1;
							unsp_update_nz(unsp, UNSP_REG_I(OPA));
							break;
						case 10: // or r, imm6
							r0 = UNSP_REG_I(OPA);
							r1 = OPIMM;
							lres = r0 | r1;
							UNSP_REG_I(OPA) = (UINT16)lres;
							unsp_update_nz(unsp, lres);
							if(OPA == (UNSP_PC-1)) jumped = 1;
							break;
						case 11: // and r, imm6
							r0 = UNSP_REG_I(OPA);
							r1 = OPIMM;
							lres = r0 & r1;
							UNSP_REG_I(OPA) = (UINT16)lres;
							unsp_update_nz(unsp, lres);
							if(OPA == (UNSP_PC-1)) jumped = 1;
							break;
						case 12: // test r, imm6
							r0 = UNSP_REG_I(OPA);
							r1 = OPIMM;
							lres = r0 & r1;
							unsp_update_nz(unsp, (UINT16)lres);
							break;
						default:
							unimplemented_opcode(unsp, op);
							break;
					}
					//print("%s %s, %02x", alu[OP0], reg[OPA], OPIMM);
					break;

				// Pop / Interrupt return
				case 0x29:
					if(op == 0x9a90) // retf
					{
						UNSP_REG(SR) = unsp_pop(unsp, &UNSP_REG(SP));
						UNSP_REG(PC) = unsp_pop(unsp, &UNSP_REG(SP));
						jumped = 1;
						break;
					}
					else if(op == 0x9a98) // reti
					{
						int i;
						UNSP_REG(SR) = unsp_pop(unsp, &UNSP_REG(SP));
						UNSP_REG(PC) = unsp_pop(unsp, &UNSP_REG(SP));
						jumped = 1;
						if(unsp->fiq & 2)
						{
							unsp->fiq &= 1;
						}
						else if(unsp->irq & 2)
						{
							unsp->irq &= 1;
						}
						unsp->sirq &= ~(1 << unsp->curirq);
						logerror("iret (%04x, %04x)\n", unsp->sirq, unsp->curirq);
						for(i = 0; i < 9; i++)
						{
							if((unsp->sirq & (1 << i)) != 0 && i != unsp->curirq)
							{
								unsp->sirq &= ~(1 << i);
								unsp->curirq = 0;
								logerror("iretting from irq %d back to irq %d (%04x, %04x)\n", unsp->curirq, i, unsp->sirq, unsp->curirq);
								unsp_set_irq_line(unsp, UNSP_IRQ0_LINE + i, 1);
								i = -1;
								break;
							}
						}
						if(i != -1)
						{
							unsp->curirq = 0;
							logerror("iret done (%04x, %04x)\n", unsp->sirq, unsp->curirq);
						}
						break;
					}
					else
					{
						r0 = OPN;
						r1 = OPA;
						while(r0--)
						{
							UNSP_REG_I(++r1) = unsp_pop(unsp, &UNSP_REG_I(OPB));
							if(r1 == (UNSP_PC-1)) jumped = 1;
						}
					}
					break;

				// Push
				case 0x2d:
					r0 = OPN;
					r1 = OPA;
					while(r0--)
					{
						unsp_push(unsp, UNSP_REG_I(r1--), &UNSP_REG_I(OPB));
					}
					break;

				// ALU, Indirect
				case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x36: case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d:
					switch(OPN & 3)
					{
						case 0:
							switch(OP0)
							{
								case 0: // add r, [r]
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									lres = r0 + r1;
									UNSP_REG_I(OPA) = (UINT16)lres;
									unsp_update_nzsc(unsp, lres, r0, r1);
									if(OPA == (UNSP_PC-1)) jumped = 1;
									break;
								case 1: // adc r, [r]
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									lres = r0 + r1;
									if(UNSP_REG(SR) & 0x0040) lres++;
									UNSP_REG_I(OPA) = (UINT16)lres;
									unsp_update_nzsc(unsp, lres, r0, r1);
									if(OPA == (UNSP_PC-1)) jumped = 1;
									break;
								case 2: // sub r, [r]
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									lres = r0 + (~r1 & 0x0000ffff) + 1;
									UNSP_REG_I(OPA) = (UINT16)lres;
									unsp_update_nzsc(unsp, lres, r0, r1);
									if(OPA == (UNSP_PC-1)) jumped = 1;
									break;
								case 3: // sbc r, [r]
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									lres = r0 + (~r1 & 0x0000ffff);
									if(UNSP_REG(SR) & 0x0040) lres++;
									UNSP_REG_I(OPA) = (UINT16)lres;
									unsp_update_nzsc(unsp, lres, r0, r1);
									if(OPA == (UNSP_PC-1)) jumped = 1;
									break;
								case 4: // cmp r, [r]
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									lres = r0 + (~r1 & 0x0000ffff) + 1;
									unsp_update_nzsc(unsp, lres, r0, r1);
									break;
								case 6: // neg r, [r]
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									UNSP_REG_I(OPA) = -r1;
									unsp_update_nzsc(unsp, lres, r0, r1);
									if(OPA == (UNSP_PC-1)) jumped = 1;
									break;
								case 9: // load r, [r]
									UNSP_REG_I(OPA) = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									if(OPA == (UNSP_PC-1)) jumped = 1;
									unsp_update_nz(unsp, UNSP_REG_I(OPA));
									break;
								case 10: // or r, [r]
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									lres = r0 | r1;
									UNSP_REG_I(OPA) = (UINT16)lres;
									if(OPA == (UNSP_PC-1)) jumped = 1;
									unsp_update_nz(unsp, lres);
									break;
								case 11: // and r, [r]
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									lres = r0 & r1;
									UNSP_REG_I(OPA) = (UINT16)lres;
									if(OPA == (UNSP_PC-1)) jumped = 1;
									unsp_update_nz(unsp, lres);
									break;
								case 13: // store r, [r]
									WRITE16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB), UNSP_REG_I(OPA));
									break;
								default:
									unimplemented_opcode(unsp, op);
									break;
							}
							break;
						case 1:
							//print("%s %s, [%s%s--]", alu[OP0], reg[OPA], (OPN & 4) ? "ds:" : "", reg[OPB]);
							switch(OP0)
							{
								case 0: // add r, [<ds:>r--]
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									lres = r0 + r1;
									UNSP_REG_I(OPB)--;
									UNSP_REG_I(OPA) = (UINT16)lres;
									if(OPA == (UNSP_PC-1) || OPB == (UNSP_PC-1)) jumped = 1;
									unsp_update_nzsc(unsp, UNSP_REG_I(OPA), r0, r1);
									break;
								case 1: // adc r, [<ds:>r--]
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									lres = r0 + r1;
									if(UNSP_REG(SR) & 0x0040) lres++;
									UNSP_REG_I(OPB)--;
									UNSP_REG_I(OPA) = (UINT16)lres;
									if(OPA == (UNSP_PC-1) || OPB == (UNSP_PC-1)) jumped = 1;
									unsp_update_nzsc(unsp, UNSP_REG_I(OPA), r0, r1);
									break;
								case 2: // sub r, [<ds:>r--]
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									lres = r0 + (~r1 & 0x0000ffff) + 1;
									UNSP_REG_I(OPB)--;
									UNSP_REG_I(OPA) = (UINT16)lres;
									if(OPA == (UNSP_PC-1) || OPB == (UNSP_PC-1)) jumped = 1;
									unsp_update_nzsc(unsp, UNSP_REG_I(OPA), r0, r1);
									break;
								case 3: // sbc r, [<ds:>r--]
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									lres = r0 + (~r1 & 0x0000ffff);
									if(UNSP_REG(SR) & 0x0040) lres++;
									UNSP_REG_I(OPB)--;
									UNSP_REG_I(OPA) = (UINT16)lres;
									if(OPA == (UNSP_PC-1) || OPB == (UNSP_PC-1)) jumped = 1;
									unsp_update_nzsc(unsp, UNSP_REG_I(OPA), r0, r1);
									break;
								case 4: // cmp r, [<ds:>r--]
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									lres = r0 + (~r1 & 0x0000ffff) + 1;
									if(UNSP_REG(SR) & 0x0040) lres++;
									UNSP_REG_I(OPB)--;
									unsp_update_nzsc(unsp, lres, r0, r1);
									break;
								case 6: // neg r, [<ds:>r--]
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									lres = -r1;
									UNSP_REG_I(OPB)--;
									UNSP_REG_I(OPA) = (UINT16)lres;
									if(OPA == (UNSP_PC-1) || OPB == (UNSP_PC-1)) jumped = 1;
									unsp_update_nz(unsp, UNSP_REG_I(OPA));
									break;
								case 8: // xor r, [<ds:>r--]
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									lres = r0 ^ r1;
									UNSP_REG_I(OPB)--;
									UNSP_REG_I(OPA) = (UINT16)lres;
									if(OPA == (UNSP_PC-1) || OPB == (UNSP_PC-1)) jumped = 1;
									unsp_update_nz(unsp, UNSP_REG_I(OPA));
									break;
								case 9: // load r, [<ds:>r--]
									UNSP_REG_I(OPA) = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									UNSP_REG_I(OPB)--;
									if(OPA == (UNSP_PC-1) || OPB == (UNSP_PC-1)) jumped = 1;
									unsp_update_nz(unsp, UNSP_REG_I(OPA));
									break;
								case 10: // or r, [<ds:>r--]
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									lres = r0 | r1;
									UNSP_REG_I(OPB)--;
									UNSP_REG_I(OPA) = (UINT16)lres;
									if(OPA == (UNSP_PC-1) || OPB == (UNSP_PC-1)) jumped = 1;
									unsp_update_nz(unsp, UNSP_REG_I(OPA));
									break;
								case 11: // and r, [<ds:>r--]
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									lres = r0 & r1;
									UNSP_REG_I(OPB)--;
									UNSP_REG_I(OPA) = (UINT16)lres;
									if(OPA == (UNSP_PC-1) || OPB == (UNSP_PC-1)) jumped = 1;
									unsp_update_nz(unsp, UNSP_REG_I(OPA));
									break;
								case 12: // test r, [<ds:>r--]
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									lres = r0 & r1;
									UNSP_REG_I(OPB)--;
									if(OPB == (UNSP_PC-1)) jumped = 1;
									unsp_update_nz(unsp, UNSP_REG_I(OPA));
									break;
								case 13: // store r, [<ds:>r--]
									WRITE16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB), UNSP_REG_I(OPA));
									UNSP_REG_I(OPB)--;
									if(OPB == (UNSP_PC-1)) jumped = 1;
									break;
								default:
									unimplemented_opcode(unsp, op);
									break;
							}
							break;
						case 2:
							switch(OP0)
							{
								case 0: // add r, [<ds:>r++]
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									lres = r0 + r1;
									UNSP_REG_I(OPB)++;
									UNSP_REG_I(OPA) = (UINT16)lres;
									if(OPA == (UNSP_PC-1) || OPB == (UNSP_PC-1)) jumped = 1;
									unsp_update_nzsc(unsp, UNSP_REG_I(OPA), r0, r1);
									break;
								case 1: // adc r, [<ds:>r++]
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									lres = r0 + r1;
									if(UNSP_REG(SR) & 0x0040) lres++;
									UNSP_REG_I(OPB)++;
									UNSP_REG_I(OPA) = (UINT16)lres;
									if(OPA == (UNSP_PC-1) || OPB == (UNSP_PC-1)) jumped = 1;
									unsp_update_nzsc(unsp, UNSP_REG_I(OPA), r0, r1);
									break;
								case 2: // sub r, [<ds:>r++]
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									lres = r0 + (~r1 & 0x0000ffff) + 1;
									UNSP_REG_I(OPB)++;
									UNSP_REG_I(OPA) = (UINT16)lres;
									if(OPA == (UNSP_PC-1) || OPB == (UNSP_PC-1)) jumped = 1;
									unsp_update_nzsc(unsp, UNSP_REG_I(OPA), r0, r1);
									break;
								case 3: // sbc r, [<ds:>r++]
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									lres = r0 + (~r1 & 0x0000ffff);
									if(UNSP_REG(SR) & 0x0040) lres++;
									UNSP_REG_I(OPB)++;
									UNSP_REG_I(OPA) = (UINT16)lres;
									if(OPA == (UNSP_PC-1) || OPB == (UNSP_PC-1)) jumped = 1;
									unsp_update_nzsc(unsp, UNSP_REG_I(OPA), r0, r1);
									break;
								case 4: // cmp r, [<ds:>r++]
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									lres = r0 + (~r1 & 0x0000ffff) + 1;
									UNSP_REG_I(OPB)++;
									if(OPB == (UNSP_PC-1)) jumped = 1;
									unsp_update_nzsc(unsp, lres, r0, r1);
									break;
								case 6: // neg r, [<ds:>r++]
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									lres = -r1;
									unsp_update_nz(unsp, lres);
									UNSP_REG_I(OPB)++;
									UNSP_REG_I(OPA) = (UINT16)lres;
									if(OPA == (UNSP_PC-1) || OPB == (UNSP_PC-1)) jumped = 1;
									break;
								case 8: // xor r, [<ds:>r++]
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									lres = r0 ^ r1;
									unsp_update_nz(unsp, lres);
									UNSP_REG_I(OPB)++;
									UNSP_REG_I(OPA) = (UINT16)lres;
									if(OPA == (UNSP_PC-1) || OPB == (UNSP_PC-1)) jumped = 1;
									break;
								case 9: // load r, [<ds:>r++]
									UNSP_REG_I(OPA) = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									UNSP_REG_I(OPB)++;
									if(OPA == (UNSP_PC-1) || OPB == (UNSP_PC-1)) jumped = 1;
									unsp_update_nz(unsp, UNSP_REG_I(OPA));
									break;
								case 10: // or r, [<ds:>r++]
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									lres = r0 | r1;
									unsp_update_nz(unsp, lres);
									UNSP_REG_I(OPB)++;
									UNSP_REG_I(OPA) = (UINT16)lres;
									if(OPA == (UNSP_PC-1) || OPB == (UNSP_PC-1)) jumped = 1;
									break;
								case 11: // and r, [<ds:>r++]
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									lres = r0 & r1;
									unsp_update_nz(unsp, lres);
									UNSP_REG_I(OPB)++;
									UNSP_REG_I(OPA) = (UINT16)lres;
									if(OPA == (UNSP_PC-1) || OPB == (UNSP_PC-1)) jumped = 1;
									break;
								case 12: // test r, [<ds:>r++]
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									lres = r0 & r1;
									unsp_update_nz(unsp, lres);
									break;
								case 13: // store r, [<ds:>r++]
									WRITE16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB), UNSP_REG_I(OPA));
									UNSP_REG_I(OPB)++;
									if(OPB == (UNSP_PC-1)) jumped = 1;
									break;
								default:
									unimplemented_opcode(unsp, op);
									break;
							}
							break;
						case 3:
							switch(OP0)
							{
								case 0: // add r, [<ds:>++r]
									UNSP_REG_I(OPB)++;
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									lres = r0 + r1;
									UNSP_REG_I(OPA) = (UINT16)lres;
									if(OPA == (UNSP_PC-1) || OPB == (UNSP_PC-1)) jumped = 1;
									unsp_update_nzsc(unsp, UNSP_REG_I(OPA), r0, r1);
									break;
								case 1: // adc r, [<ds:>++r]
									UNSP_REG_I(OPB)++;
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									lres = r0 + r1;
									if(UNSP_REG(SR) & 0x0040) lres++;
									UNSP_REG_I(OPA) = (UINT16)lres;
									if(OPA == (UNSP_PC-1) || OPB == (UNSP_PC-1)) jumped = 1;
									unsp_update_nzsc(unsp, UNSP_REG_I(OPA), r0, r1);
									break;
								case 2: // sub r, [<ds:>++r]
									UNSP_REG_I(OPB)++;
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									lres = r0 + (~r1 & 0x0000ffff) + 1;
									UNSP_REG_I(OPA) = (UINT16)lres;
									if(OPA == (UNSP_PC-1) || OPB == (UNSP_PC-1)) jumped = 1;
									unsp_update_nzsc(unsp, UNSP_REG_I(OPA), r0, r1);
									break;
								case 3: // sbc r, [<ds:>++r]
									UNSP_REG_I(OPB)++;
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									lres = r0 + (~r1 & 0x0000ffff);
									if(UNSP_REG(SR) & 0x0040) lres++;
									UNSP_REG_I(OPA) = (UINT16)lres;
									if(OPA == (UNSP_PC-1) || OPB == (UNSP_PC-1)) jumped = 1;
									unsp_update_nzsc(unsp, UNSP_REG_I(OPA), r0, r1);
									break;
								case 6: // neg r, [<ds:>++r]
									UNSP_REG_I(OPB)++;
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									lres = -r1;
									unsp_update_nz(unsp, lres);
									UNSP_REG_I(OPA) = (UINT16)lres;
									if(OPA == (UNSP_PC-1) || OPB == (UNSP_PC-1)) jumped = 1;
									break;
								case 8: // xor r, [<ds:>++r]
									UNSP_REG_I(OPB)++;
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									lres = r0 ^ r1;
									unsp_update_nz(unsp, lres);
									UNSP_REG_I(OPA) = (UINT16)lres;
									if(OPA == (UNSP_PC-1) || OPB == (UNSP_PC-1)) jumped = 1;
									break;
								case 9: // load r, [<ds:>++r]
									UNSP_REG_I(OPB)++;
									UNSP_REG_I(OPA) = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									if(OPA == (UNSP_PC-1) || OPB == (UNSP_PC-1)) jumped = 1;
									unsp_update_nz(unsp, UNSP_REG_I(OPA));
									break;
								case 10: // or r, [<ds:>++r]
									UNSP_REG_I(OPB)++;
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									lres = r0 | r1;
									unsp_update_nz(unsp, lres);
									UNSP_REG_I(OPA) = (UINT16)lres;
									if(OPA == (UNSP_PC-1) || OPB == (UNSP_PC-1)) jumped = 1;
									break;
								case 11: // and r, [<ds:>++r]
									UNSP_REG_I(OPB)++;
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									lres = r0 & r1;
									unsp_update_nz(unsp, lres);
									UNSP_REG_I(OPA) = (UINT16)lres;
									if(OPA == (UNSP_PC-1) || OPB == (UNSP_PC-1)) jumped = 1;
									break;
								case 12: // test r, [<ds:>++r]
									UNSP_REG_I(OPB)++;
									r0 = UNSP_REG_I(OPA);
									r1 = READ16(unsp, (OPN & 4) ? UNSP_LREG_I(OPB) : UNSP_REG_I(OPB));
									lres = r0 & r1;
									unsp_update_nz(unsp, lres);
									if(OPB == (UNSP_PC-1)) jumped = 1;
									break;
								default:
									unimplemented_opcode(unsp, op);
									break;
							}
							break;
					}
					break;

				// ALU, 16-bit ops
				case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x46: case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c:
					switch(OPN)
					{
						// ALU, Register
						case 0:
							switch(OP0)
							{
								case 0: // add r, r
									r0 = UNSP_REG_I(OPA);
									r1 = UNSP_REG_I(OPB);
									lres = r0 + r1;
									UNSP_REG_I(OPA) = (UINT16)lres;
									if(OPA == (UNSP_PC-1)) jumped = 1;
									unsp_update_nzsc(unsp, lres, r0, r1);
									break;
								case 1: // adc r, r
									r0 = UNSP_REG_I(OPA);
									r1 = UNSP_REG_I(OPB);
									lres = r0 + r1;
									if(UNSP_REG(SR) & 0x0040)
									{
										lres++;
									}
									UNSP_REG_I(OPA) = (UINT16)lres;
									if(OPA == (UNSP_PC-1)) jumped = 1;
									unsp_update_nzsc(unsp, lres, r0, r1);
									break;
								case 2: // sub r, r
									r0 = UNSP_REG_I(OPA);
									r1 = UNSP_REG_I(OPB);
									lres = r0 + (~r1 & 0x0000ffff) + 1;
									UNSP_REG_I(OPA) = (UINT16)lres;
									if(OPA == (UNSP_PC-1)) jumped = 1;
									unsp_update_nzsc(unsp, lres, r0, r1);
									break;
								case 4: // cmp r, r
									r0 = UNSP_REG_I(OPA);
									r1 = UNSP_REG_I(OPB);
									lres = r0 + (~r1 & 0x0000ffff) + 1;
									unsp_update_nzsc(unsp, lres, r0, r1);
									break;
								case 6: // neg r, r
									UNSP_REG_I(OPA) = -UNSP_REG_I(OPB);
									if(OPA == (UNSP_PC-1)) jumped = 1;
									unsp_update_nz(unsp, UNSP_REG_I(OPA));
									break;
								case 8: // xor r, r
									r0 = UNSP_REG_I(OPA);
									r1 = UNSP_REG_I(OPB);
									lres = r0 ^ r1;
									UNSP_REG_I(OPA) = (UINT16)lres;
									if(OPA == (UNSP_PC-1)) jumped = 1;
									unsp_update_nz(unsp, lres);
									break;
								case 9: // load r, r
									UNSP_REG_I(OPA) = UNSP_REG_I(OPB);
									if(OPA == (UNSP_PC-1)) jumped = 1;
									unsp_update_nz(unsp, UNSP_REG_I(OPA));
									break;
								case 10: // or r, r
									r0 = UNSP_REG_I(OPA);
									r1 = UNSP_REG_I(OPB);
									lres = r0 | r1;
									UNSP_REG_I(OPA) = (UINT16)lres;
									if(OPA == (UNSP_PC-1)) jumped = 1;
									unsp_update_nz(unsp, lres);
									break;
								case 11: // and r, r
									r0 = UNSP_REG_I(OPA);
									r1 = UNSP_REG_I(OPB);
									lres = r0 & r1;
									UNSP_REG_I(OPA) = (UINT16)lres;
									if(OPA == (UNSP_PC-1)) jumped = 1;
									unsp_update_nz(unsp, lres);
									break;
								case 12: // test r, r
									r0 = UNSP_REG_I(OPA);
									r1 = UNSP_REG_I(OPB);
									lres = r0 & r1;
									unsp_update_nz(unsp, lres);
									break;
								default:
									unimplemented_opcode(unsp, op);
									break;
							}
							break;

						// ALU, 16-bit Immediate
						case 1:
							if(!((OP0 == 4 || OP0 == 6 || OP0 == 9 || OP0 == 12) && OPA != OPB))
							{
								switch(OP0)
								{
									case 0: // add  r, r, imm16
										UNSP_REG(PC)++;
										r0 = UNSP_REG_I(OPB);
										r1 = READ16(unsp, UNSP_LPC);
										lres = r0 + r1;
										UNSP_REG_I(OPA) = (UINT16)lres;
										if(OPA == (UNSP_PC-1)) jumped = 1;
										unsp_update_nzsc(unsp, lres, r0, r1);
										break;
									case 1: // adc r, r, imm16
										UNSP_REG(PC)++;
										r0 = UNSP_REG_I(OPB);
										r1 = READ16(unsp, UNSP_LPC);
										lres = r0 + r1;
										if(UNSP_REG(SR) & 0x0040) lres++;
										UNSP_REG_I(OPA) = (UINT16)lres;
										if(OPA == (UNSP_PC-1)) jumped = 1;
										unsp_update_nzsc(unsp, lres, r0, r1);
										break;
									case 2: // sub r, r, imm16
										UNSP_REG(PC)++;
										r0 = UNSP_REG_I(OPB);
										r1 = READ16(unsp, UNSP_LPC);
										lres = r0 + (~r1 & 0x0000ffff) + 1;
										UNSP_REG_I(OPA) = (UINT16)lres;
										if(OPA == (UNSP_PC-1)) jumped = 1;
										unsp_update_nzsc(unsp, lres, r0, r1);
										break;
									case 3: // sbc r, r, imm16
										UNSP_REG(PC)++;
										r0 = UNSP_REG_I(OPB);
										r1 = READ16(unsp, UNSP_LPC);
										lres = r0 + (~r1 & 0x0000ffff);
										if(UNSP_REG(SR) & 0x0040) lres++;
										UNSP_REG_I(OPA) = (UINT16)lres;
										if(OPA == (UNSP_PC-1)) jumped = 1;
										unsp_update_nzsc(unsp, lres, r0, r1);
										break;
									case 4: // cmp r, imm16
										UNSP_REG(PC)++;
										r0 = UNSP_REG_I(OPB);
										r1 = READ16(unsp, UNSP_LPC);
										lres = r0 + (~r1 & 0x0000ffff) + 1;
										unsp_update_nzsc(unsp, lres, r0, r1);
										break;
									case 6: // neg r, imm16
										UNSP_REG(PC)++;
										r0 = UNSP_REG_I(OPB);
										r1 = READ16(unsp, UNSP_LPC);
										lres = -r1;
										UNSP_REG_I(OPA) = (UINT16)lres;
										if(OPA == (UNSP_PC-1)) jumped = 1;
										unsp_update_nz(unsp, lres);
										break;
									case 8: // xor r, imm16
										UNSP_REG(PC)++;
										r0 = UNSP_REG_I(OPB);
										r1 = READ16(unsp, UNSP_LPC);
										lres = r0 ^ r1;
										UNSP_REG_I(OPA) = (UINT16)lres;
										if(OPA == (UNSP_PC-1)) jumped = 1;
										unsp_update_nz(unsp, lres);
										break;
									case 9:	// load r, imm16
										UNSP_REG(PC)++;
										lres = READ16(unsp, UNSP_LPC);
										UNSP_REG_I(OPA) = (UINT16)lres;
										if(OPA == (UNSP_PC-1)) jumped = 1;
										unsp_update_nz(unsp, UNSP_REG_I(OPA));
										break;
									case 10: // or r, imm16
										UNSP_REG(PC)++;
										r0 = UNSP_REG_I(OPB);
										r1 = READ16(unsp, UNSP_LPC);
										lres = r0 | r1;
										UNSP_REG_I(OPA) = (UINT16)lres;
										unsp_update_nz(unsp, lres);
										if(OPA == (UNSP_PC-1)) jumped = 1;
										break;
									case 11: // and r, imm16
										UNSP_REG(PC)++;
										r0 = UNSP_REG_I(OPB);
										r1 = READ16(unsp, UNSP_LPC);
										lres = r0 & r1;
										UNSP_REG_I(OPA) = (UINT16)lres;
										unsp_update_nz(unsp, lres);
										if(OPA == (UNSP_PC-1)) jumped = 1;
										break;
									case 12: // test r, imm16
										UNSP_REG(PC)++;
										r0 = UNSP_REG_I(OPB);
										r1 = READ16(unsp, UNSP_LPC);
										lres = r0 & r1;
										unsp_update_nz(unsp, lres);
										break;
									default:
										unimplemented_opcode(unsp, op);
										break;
								}
							}
							else
							{
								unimplemented_opcode(unsp, op);
							}
							break;

						// ALU, Direct 16
						case 2:
							switch(OP0)
							{
								case 0: // add r, [imm16]
									UNSP_REG(PC)++;
									r0 = UNSP_REG_I(OPB);
									r1 = READ16(unsp, READ16(unsp, UNSP_LPC));
									lres = r0 + r1;
									UNSP_REG_I(OPA) = (UINT16)lres;
									unsp_update_nzsc(unsp, lres, r0, r1);
									if(OPA == (UNSP_PC-1)) jumped = 1;
									break;
								case 1: // adc r, [imm16]
									UNSP_REG(PC)++;
									r0 = UNSP_REG_I(OPB);
									r1 = READ16(unsp, READ16(unsp, UNSP_LPC));
									lres = r0 + r1;
									if(UNSP_REG(SR) & 0x0040)
									{
										lres++;
									}
									UNSP_REG_I(OPA) = (UINT16)lres;
									unsp_update_nzsc(unsp, lres, r0, r1);
									if(OPA == (UNSP_PC-1)) jumped = 1;
									break;
								case 2: // sub r, [imm16]
									UNSP_REG(PC)++;
									r0 = UNSP_REG_I(OPB);
									r1 = READ16(unsp, READ16(unsp, UNSP_LPC));
									lres = r0 + (~r1 & 0x0000ffff) + 1;
									UNSP_REG_I(OPA) = (UINT16)lres;
									unsp_update_nzsc(unsp, lres, r0, r1);
									if(OPA == (UNSP_PC-1)) jumped = 1;
									break;
								case 3: // sbc r, [imm16]
									UNSP_REG(PC)++;
									r0 = UNSP_REG_I(OPB);
									r1 = READ16(unsp, READ16(unsp, UNSP_LPC));
									lres = r0 + (~r1 & 0x0000ffff);
									if(UNSP_REG(SR) & 0x0040) lres++;
									UNSP_REG_I(OPA) = (UINT16)lres;
									unsp_update_nzsc(unsp, lres, r0, r1);
									if(OPA == (UNSP_PC-1)) jumped = 1;
									break;
								case 4: // cmp r, [imm16]
									UNSP_REG(PC)++;
									r0 = UNSP_REG_I(OPB);
									r1 = READ16(unsp, READ16(unsp, UNSP_LPC));
									lres = r0 + (~r1 & 0x0000ffff) + 1;
									unsp_update_nzsc(unsp, lres, r0, r1);
									break;
								case 6: // neg r, [imm16]
									UNSP_REG(PC)++;
									r0 = UNSP_REG_I(OPB);
									r1 = READ16(unsp, READ16(unsp, UNSP_LPC));
									lres = -r1;
									UNSP_REG_I(OPA) = (UINT16)lres;
									unsp_update_nz(unsp, UNSP_REG_I(OPA));
									if(OPA == (UNSP_PC-1)) jumped = 1;
									break;
								case 8: // xor r, [imm16]
									UNSP_REG(PC)++;
									r0 = UNSP_REG_I(OPB);
									r1 = READ16(unsp, READ16(unsp, UNSP_LPC));
									lres = r0 ^ r1;
									UNSP_REG_I(OPA) = (UINT16)lres;
									unsp_update_nz(unsp, UNSP_REG_I(OPA));
									if(OPA == (UNSP_PC-1)) jumped = 1;
									break;
								case 9: // load r, [imm16]
									UNSP_REG(PC)++;
									UNSP_REG_I(OPA) = READ16(unsp, READ16(unsp, UNSP_LPC));
									if(OPA == (UNSP_PC-1)) jumped = 1;
									unsp_update_nz(unsp, UNSP_REG_I(OPA));
									break;
								case 10: // or r, [imm16]
									UNSP_REG(PC)++;
									r0 = UNSP_REG_I(OPB);
									r1 = READ16(unsp, READ16(unsp, UNSP_LPC));
									lres = r0 | r1;
									UNSP_REG_I(OPA) = (UINT16)lres;
									unsp_update_nz(unsp, UNSP_REG_I(OPA));
									if(OPA == (UNSP_PC-1)) jumped = 1;
									break;
								case 11: // and r, [imm16]
									UNSP_REG(PC)++;
									r0 = UNSP_REG_I(OPB);
									r1 = READ16(unsp, READ16(unsp, UNSP_LPC));
									lres = r0 & r1;
									UNSP_REG_I(OPA) = (UINT16)lres;
									unsp_update_nz(unsp, UNSP_REG_I(OPA));
									if(OPA == (UNSP_PC-1)) jumped = 1;
									break;
								case 12: // test r, [imm16]
									UNSP_REG(PC)++;
									r0 = UNSP_REG_I(OPB);
									r1 = READ16(unsp, READ16(unsp, UNSP_LPC));
									lres = r0 & r1;
									unsp_update_nz(unsp, UNSP_REG_I(OPA));
									break;
								default:
									unimplemented_opcode(unsp, op);
									break;
							}
							break;

						// ALU, Direct 16
						case 3:
							switch(OP0)
							{
								case 0: // add [imm16], r
									UNSP_REG(PC)++;
									r0 = UNSP_REG_I(OPA);
									r1 = UNSP_REG_I(OPB);
									lres = r0 + r1;
									WRITE16(unsp, READ16(unsp, UNSP_LPC), (UINT16)lres);
									unsp_update_nzsc(unsp, lres, r0, r1);
									break;
								case 1: // adc [imm16], r
									UNSP_REG(PC)++;
									r0 = UNSP_REG_I(OPA);
									r1 = UNSP_REG_I(OPB);
									lres = r0 + r1;
									if(UNSP_REG(SR) & 0x0040) lres++;
									WRITE16(unsp, READ16(unsp, UNSP_LPC), (UINT16)lres);
									unsp_update_nzsc(unsp, lres, r0, r1);
									break;
								case 2: // sub [imm16], r
									UNSP_REG(PC)++;
									r0 = UNSP_REG_I(OPA);
									r1 = UNSP_REG_I(OPB);
									lres = r0 + (~r1 & 0x0000ffff) + 1;
									WRITE16(unsp, READ16(unsp, UNSP_LPC), (UINT16)lres);
									unsp_update_nzsc(unsp, lres, r0, r1);
									break;
								case 3: // sbc [imm16], r
									UNSP_REG(PC)++;
									r0 = UNSP_REG_I(OPA);
									r1 = UNSP_REG_I(OPB);
									lres = r0 + (~r1 & 0x0000ffff);
									if(UNSP_REG(SR) & 0x0040) lres++;
									WRITE16(unsp, READ16(unsp, UNSP_LPC), (UINT16)lres);
									unsp_update_nzsc(unsp, lres, r0, r1);
									break;
								case 4: // cmp [imm16], r
									UNSP_REG(PC)++;
									r0 = UNSP_REG_I(OPA);
									r1 = UNSP_REG_I(OPB);
									lres = r0 + (~r1 & 0x0000ffff) + 1;
									unsp_update_nzsc(unsp, lres, r0, r1);
									break;
								case 6: // neg [imm16], r
									UNSP_REG(PC)++;
									r0 = UNSP_REG_I(OPA);
									r1 = UNSP_REG_I(OPB);
									lres = -r1;
									WRITE16(unsp, READ16(unsp, UNSP_LPC), (UINT16)lres);
									unsp_update_nz(unsp, lres);
									break;
								case 8: // xor [imm16], r
									UNSP_REG(PC)++;
									r0 = UNSP_REG_I(OPA);
									r1 = UNSP_REG_I(OPB);
									lres = r0 ^ r1;
									WRITE16(unsp, READ16(unsp, UNSP_LPC), (UINT16)lres);
									unsp_update_nz(unsp, lres);
									break;
								case 10: // or [imm16], r
									UNSP_REG(PC)++;
									r0 = UNSP_REG_I(OPA);
									r1 = UNSP_REG_I(OPB);
									lres = r0 | r1;
									WRITE16(unsp, READ16(unsp, UNSP_LPC), (UINT16)lres);
									unsp_update_nz(unsp, lres);
									break;
								case 11: // and [imm16], r
									UNSP_REG(PC)++;
									r0 = UNSP_REG_I(OPA);
									r1 = UNSP_REG_I(OPB);
									lres = r0 & r1;
									WRITE16(unsp, READ16(unsp, UNSP_LPC), (UINT16)lres);
									unsp_update_nz(unsp, lres);
									break;
								default:
									unimplemented_opcode(unsp, op);
									break;
							}
							break;

						// ALU, Shifted
						default:
						{
							UINT32 shift = (UNSP_REG_I(OPB) << 4) | unsp->sb;
							if(shift & 0x80000)
							{
								shift |= 0xf00000;
							}
							shift >>= (OPN - 3);
							unsp->sb = shift & 0x0f;
							r1 = (UINT16)(shift >> 4);

							switch(OP0)
							{
								case 9: // load r, r asr n
									UNSP_REG_I(OPA) = r1;
									unsp_update_nz(unsp, UNSP_REG_I(OPA));
									if(OPA == (UNSP_PC-1)) jumped = 1;
									break;
								default:
									//print("%s %s, %s asr %d", alu[OP0], reg[OPA], reg[OPB], (OPN & 3) + 1);
									unimplemented_opcode(unsp, op);
									break;
							}
							break;
						}
					}
					break;

				case 0x4d:
					if(OPN == 3)
					{
						if(OPA == OPB)
						{
							UNSP_REG(PC)++;
							WRITE16(unsp, READ16(unsp, UNSP_LPC), UNSP_REG_I(OPB));
						}
					}
					break;

				// ALU, Shifted
				case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x56: case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c:
					if(OPN & 4)
					{
						switch(OP0)
						{
							case 9: // load r, r >> imm2
								lres = ((UNSP_REG_I(OPB) << 4) | unsp->sb) >> (OPN - 3);
								unsp->sb = lres & 0x0f;
								UNSP_REG_I(OPA) = (UINT16)(lres >> 4);
								unsp_update_nz(unsp, UNSP_REG_I(OPA));
								if(OPA == (UNSP_PC-1)) jumped = 1;
								break;
							default:
								unimplemented_opcode(unsp, op);
								break;
						}
					}
					else
					{
						switch(OP0)
						{
							case 0: // add r, r << imm2
								lres = ((unsp->sb << 16) | UNSP_REG_I(OPB)) << (OPN + 1);
								unsp->sb = (lres >> 16) & 0x0f;
								r0 = UNSP_REG_I(OPA);
								r1 = (UINT16)lres;
								lres = r0 + r1;
								UNSP_REG_I(OPA) = (UINT16)lres;
								unsp_update_nzsc(unsp, lres, r0, r1);
								if(OPA == (UNSP_PC-1)) jumped = 1;
								break;
							case 10: // or r, r << imm2
								lres = ((unsp->sb << 16) | UNSP_REG_I(OPB)) << (OPN + 1);
								unsp->sb = (lres >> 16) & 0x0f;
								r0 = UNSP_REG_I(OPA);
								r1 = (UINT16)lres;
								lres = r0 | r1;
								UNSP_REG_I(OPA) = (UINT16)lres;
								unsp_update_nz(unsp, UNSP_REG_I(OPA));
								if(OPA == (UNSP_PC-1)) jumped = 1;
								break;
							case 9: // load r, r << imm2
								lres = ((unsp->sb << 16) | UNSP_REG_I(OPB)) << (OPN + 1);
								unsp->sb = (lres >> 16) & 0x0f;
								UNSP_REG_I(OPA) = (UINT16)lres;
								unsp_update_nz(unsp, UNSP_REG_I(OPA));
								if(OPA == (UNSP_PC-1)) jumped = 1;
								break;
							default:
								unimplemented_opcode(unsp, op);
								break;
						}
					}
					break;

				// ALU, Rotated
				case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x66: case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x6c:
					if(OPN & 4) // ROR
					{
						lres = ((((unsp->sb << 16) | UNSP_REG_I(OPB)) << 4) | unsp->sb) >> (OPN - 3);
						unsp->sb = lres & 0x0f;
						r1 = (UINT16)(lres >> 4);
					}
					else
					{
						lres = ((((unsp->sb << 16) | UNSP_REG_I(OPB)) << 4) | unsp->sb) << (OPN + 1);
						unsp->sb = (lres >> 20) & 0x0f;
						r1 = (UINT16)(lres >> 4);
					}
					switch(OP0)
					{
						case 9: // load r, r ror imm2
							UNSP_REG_I(OPA) = r1;
							unsp_update_nz(unsp, r1);
							if(OPA == (UNSP_PC-1)) jumped = 1;
							break;
					}
					break;

				// ALU, Direct 8
				case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x76: case 0x78: case 0x79: case 0x7a: case 0x7b: case 0x7c:
					//print("%s %s, [%02x]", alu[OP0], reg[OPA], OPIMM);
					unimplemented_opcode(unsp, op);
					break;

				// Call
				case 0x1f:
					if(OPA == 0)
					{
						UNSP_REG(PC)++;
						r1 = READ16(unsp, UNSP_LPC);
						UNSP_REG(PC)++;
						unsp_push(unsp, UNSP_REG(PC), &UNSP_REG(SP));
						unsp_push(unsp, UNSP_REG(SR), &UNSP_REG(SP));
						UNSP_REG(PC) = r1;
						UNSP_REG(SR) &= 0xffc0;
						UNSP_REG(SR) |= OPIMM;
						jumped = 1;
					}
					break;

				// Multiply, Unsigned * Signed
				case 0x0f:
					if(OPN == 1 && OPA != 7)
					{
						lres = UNSP_REG_I(OPA) * UNSP_REG_I(OPB);
						if(UNSP_REG_I(OPB) & 0x8000)
						{
							lres -= UNSP_REG_I(OPA) << 16;
						}
						UNSP_REG(R4) = lres >> 16;
						UNSP_REG(R3) = (UINT16)lres;
						break;
					}
					break;

				// Multiply, Signed * Signed
				case 0x4f:
					if(OPN == 1 && OPA != 7)
					{
						lres = UNSP_REG_I(OPA) * UNSP_REG_I(OPB);
						if(UNSP_REG_I(OPB) & 0x8000)
						{
							lres -= UNSP_REG_I(OPA) << 16;
						}
						if(UNSP_REG_I(OPA) & 0x8000)
						{
							lres -= UNSP_REG_I(OPB) << 16;
						}
						UNSP_REG(R4) = lres >> 16;
						UNSP_REG(R3) = (UINT16)lres;
						break;
					}
					else
					{
						unimplemented_opcode(unsp, op);
					}
					break;

				// Interrupt flags
				case 0x5f:
					if(OPA == 0)
					{
						switch(OPIMM)
						{
							case 0:
								//print("int off");
								unimplemented_opcode(unsp, op);
								break;
							case 1:
								//print("int irq");
								unimplemented_opcode(unsp, op);
								break;
							case 2:
								//print("int fiq");
								unimplemented_opcode(unsp, op);
								break;
							case 3:
								//print("int irq,fiq");
								unimplemented_opcode(unsp, op);
								break;
							case 8: // irq off
								unsp->irq &= ~1;
								break;
							case 9: // irq on
								unsp->irq |= 1;
								break;
							case 12: // fiq off
								unsp->fiq &= ~1;
								break;
							case 13: // fiq on
								unsp->fiq |= 1;
								break;
							case 37: // nop
								break;
						}
					}
					break;
			}
		}

		if(!jumped)
		{
			UNSP_REG(PC)++;
		}

		unsp->icount -= 5;
		unsp->icount = MAX(unsp->icount, 0);
    }

    return cycles - unsp->icount;
}


/*****************************************************************************/

static void unsp_set_irq_line(unsp_state *unsp, int irqline, int state)
{
	UINT16 irq_vector = 0;

	unsp->sirq &= ~(1 << irqline);

	if(!state)
	{
		logerror("clearing irq %d (%04x, %04x)\n", irqline, unsp->sirq, unsp->curirq);
		return;
	}

	switch (irqline)
	{
		case UNSP_IRQ0_LINE:
		case UNSP_IRQ1_LINE:
		case UNSP_IRQ2_LINE:
		case UNSP_IRQ3_LINE:
		case UNSP_IRQ4_LINE:
		case UNSP_IRQ5_LINE:
		case UNSP_IRQ6_LINE:
		case UNSP_IRQ7_LINE:
			if(unsp->fiq & 2)
			{
				// FIQ is being serviced, ignore this IRQ trigger.
				unsp->sirq |= state << irqline;
				return;
			}
			if(unsp->irq != 1)
			{
				// IRQ is disabled, ignore this IRQ trigger.
				unsp->sirq |= state << irqline;
				return;
			}
			unsp->irq |= 2;
			unsp->curirq |= (1 << irqline);
			logerror("taking irq %d (%04x, %04x)\n", irqline, unsp->sirq, unsp->curirq);
			irq_vector = 0xfff8 + (irqline - UNSP_IRQ0_LINE);
			break;
		case UNSP_FIQ_LINE:
			if(unsp->fiq != 1)
			{
				// FIQ is disabled, ignore this FIQ trigger.
				unsp->sirq |= state << irqline;
				return;
			}
			unsp->fiq |= 2;
			unsp->curirq |= (1 << irqline);
			logerror("taking fiq %d (%04x, %04x)\n", irqline, unsp->sirq, unsp->curirq);
			irq_vector = 0xfff6;
			break;
		case UNSP_BRK_LINE:
			break;
	}

	unsp->saved_sb = unsp->sb;
	unsp_push(unsp, UNSP_REG(PC), &UNSP_REG(SP));
	unsp_push(unsp, UNSP_REG(SR), &UNSP_REG(SP));
	UNSP_REG(PC) = READ16(unsp, irq_vector);
	UNSP_REG(SR) = 0;
}

static CPU_SET_INFO( unsp )
{
    unsp_state *unsp = get_safe_token(device);

    switch (state)
    {
		case CPUINFO_INT_INPUT_STATE + UNSP_IRQ0_LINE:
		case CPUINFO_INT_INPUT_STATE + UNSP_IRQ1_LINE:
		case CPUINFO_INT_INPUT_STATE + UNSP_IRQ2_LINE:
		case CPUINFO_INT_INPUT_STATE + UNSP_IRQ3_LINE:
		case CPUINFO_INT_INPUT_STATE + UNSP_IRQ4_LINE:
		case CPUINFO_INT_INPUT_STATE + UNSP_IRQ5_LINE:
		case CPUINFO_INT_INPUT_STATE + UNSP_IRQ6_LINE:
		case CPUINFO_INT_INPUT_STATE + UNSP_IRQ7_LINE:
		case CPUINFO_INT_INPUT_STATE + UNSP_FIQ_LINE:
		case CPUINFO_INT_INPUT_STATE + UNSP_BRK_LINE:
			unsp_set_irq_line(unsp, state - CPUINFO_INT_INPUT_STATE, (int)info->i);
			break;

        case CPUINFO_INT_REGISTER + UNSP_SP:            UNSP_REG(SP) = info->i;    	break;
        case CPUINFO_INT_REGISTER + UNSP_R1:            UNSP_REG(R1) = info->i;     break;
        case CPUINFO_INT_REGISTER + UNSP_R2:            UNSP_REG(R2) = info->i;     break;
        case CPUINFO_INT_REGISTER + UNSP_R3:            UNSP_REG(R3) = info->i;     break;
        case CPUINFO_INT_REGISTER + UNSP_R4:            UNSP_REG(R4) = info->i;    	break;
        case CPUINFO_INT_REGISTER + UNSP_BP:            UNSP_REG(BP) = info->i;     break;
        case CPUINFO_INT_REGISTER + UNSP_SR:            UNSP_REG(SR) = info->i;     break;
        case CPUINFO_INT_PC: /* Intentional fallthrough */
        case CPUINFO_INT_REGISTER + UNSP_PC:
        	UNSP_REG(PC) = (info->i & 0x0001fffe) >> 1;
        	UNSP_REG(SR) = (UNSP_REG(SR) & 0xffc0) | ((info->i & 0x007e0000) >> 17);
        	break;
        case CPUINFO_INT_REGISTER + UNSP_IRQ:           unsp->irq = info->i;    	break;
        case CPUINFO_INT_REGISTER + UNSP_FIQ:           unsp->fiq = info->i;        break;
        case CPUINFO_INT_REGISTER + UNSP_SB:            unsp->sb = info->i;    	    break;
    }
}

CPU_GET_INFO( unsp )
{
    unsp_state *unsp = (device != NULL && device->token != NULL) ? get_safe_token(device) : NULL;

    switch(state)
    {
        case CPUINFO_INT_CONTEXT_SIZE:          info->i = sizeof(unsp_state);   break;
        case CPUINFO_INT_INPUT_LINES:           info->i = 0;                    break;
        case CPUINFO_INT_DEFAULT_IRQ_VECTOR:    info->i = 0;                    break;
        case DEVINFO_INT_ENDIANNESS:            info->i = ENDIANNESS_BIG;    	break;
        case CPUINFO_INT_CLOCK_MULTIPLIER:      info->i = 1;                    break;
        case CPUINFO_INT_CLOCK_DIVIDER:         info->i = 1;                    break;
        case CPUINFO_INT_MIN_INSTRUCTION_BYTES: info->i = 2;                    break;
        case CPUINFO_INT_MAX_INSTRUCTION_BYTES: info->i = 4;                    break;
        case CPUINFO_INT_MIN_CYCLES:            info->i = 5;                    break;
        case CPUINFO_INT_MAX_CYCLES:            info->i = 5;                    break;

        case CPUINFO_INT_DATABUS_WIDTH_PROGRAM: info->i = 16;                   break;
        case CPUINFO_INT_ADDRBUS_WIDTH_PROGRAM: info->i = 23;                   break;
        case CPUINFO_INT_ADDRBUS_SHIFT_PROGRAM: info->i = 0;                    break;
        case CPUINFO_INT_DATABUS_WIDTH_DATA:    info->i = 0;                    break;
        case CPUINFO_INT_ADDRBUS_WIDTH_DATA:    info->i = 0;                    break;
        case CPUINFO_INT_ADDRBUS_SHIFT_DATA:    info->i = 0;                    break;
        case CPUINFO_INT_DATABUS_WIDTH_IO:      info->i = 0;                    break;
        case CPUINFO_INT_ADDRBUS_WIDTH_IO:      info->i = 0;                    break;
        case CPUINFO_INT_ADDRBUS_SHIFT_IO:      info->i = 0;                    break;

        case CPUINFO_INT_REGISTER + UNSP_SP:	info->i = UNSP_REG(SP);			break;
        case CPUINFO_INT_REGISTER + UNSP_R1:	info->i = UNSP_REG(R1);			break;
        case CPUINFO_INT_REGISTER + UNSP_R2:	info->i = UNSP_REG(R2);			break;
        case CPUINFO_INT_REGISTER + UNSP_R3:	info->i = UNSP_REG(R3);			break;
        case CPUINFO_INT_REGISTER + UNSP_R4:	info->i = UNSP_REG(R4);			break;
        case CPUINFO_INT_REGISTER + UNSP_BP:	info->i = UNSP_REG(BP);			break;
        case CPUINFO_INT_REGISTER + UNSP_SR:	info->i = UNSP_REG(SR);			break;
        case CPUINFO_INT_PC:	/* Intentional fallthrough */
        case CPUINFO_INT_REGISTER + UNSP_PC:	info->i = UNSP_LPC << 1;		break;
        case CPUINFO_INT_REGISTER + UNSP_IRQ:	info->i = unsp->irq;			break;
        case CPUINFO_INT_REGISTER + UNSP_FIQ:	info->i = unsp->fiq;			break;
        case CPUINFO_INT_REGISTER + UNSP_SB:	info->i = unsp->sb;				break;

        /* --- the following bits of info are returned as pointers to data or functions --- */
        case CPUINFO_FCT_SET_INFO:              info->setinfo = CPU_SET_INFO_NAME(unsp);        break;
        case CPUINFO_FCT_INIT:                  info->init = CPU_INIT_NAME(unsp);               break;
        case CPUINFO_FCT_RESET:                 info->reset = CPU_RESET_NAME(unsp);             break;
        case CPUINFO_FCT_EXIT:                  info->exit = CPU_EXIT_NAME(unsp);               break;
        case CPUINFO_FCT_EXECUTE:               info->execute = CPU_EXECUTE_NAME(unsp);         break;
        case CPUINFO_FCT_BURN:                  info->burn = NULL;                              break;
        case CPUINFO_FCT_DISASSEMBLE:           info->disassemble = CPU_DISASSEMBLE_NAME(unsp); break;
        case CPUINFO_PTR_INSTRUCTION_COUNTER:   info->icount = &unsp->icount;               	break;

        /* --- the following bits of info are returned as NULL-terminated strings --- */
        case DEVINFO_STR_NAME:                  strcpy(info->s, "u'nSP");             	break;
        case DEVINFO_STR_FAMILY:                strcpy(info->s, "u'nSP");                	break;
        case DEVINFO_STR_VERSION:               strcpy(info->s, "1.0");                 		break;
        case DEVINFO_STR_SOURCE_FILE:           strcpy(info->s, __FILE__);              break;
        case DEVINFO_STR_CREDITS:              	strcpy(info->s, "Copyright Nicola Salmoria and the MAME Team"); break;

        case CPUINFO_STR_FLAGS:                         strcpy(info->s, " ");                   break;

		case CPUINFO_STR_REGISTER + UNSP_SP:			sprintf(info->s, "SP: %04x", UNSP_REG(SP)); break;
		case CPUINFO_STR_REGISTER + UNSP_R1:			sprintf(info->s, "R1: %04x", UNSP_REG(R1)); break;
		case CPUINFO_STR_REGISTER + UNSP_R2:			sprintf(info->s, "R2: %04x", UNSP_REG(R2)); break;
		case CPUINFO_STR_REGISTER + UNSP_R3:			sprintf(info->s, "R3: %04x", UNSP_REG(R3)); break;
		case CPUINFO_STR_REGISTER + UNSP_R4:			sprintf(info->s, "R4: %04x", UNSP_REG(R4)); break;
		case CPUINFO_STR_REGISTER + UNSP_BP:			sprintf(info->s, "BP: %04x", UNSP_REG(BP)); break;
		case CPUINFO_STR_REGISTER + UNSP_SR:			sprintf(info->s, "SR: %04x", UNSP_REG(SR)); break;
		case CPUINFO_STR_REGISTER + UNSP_PC:			sprintf(info->s, "PC: %06x (%06x)", UNSP_LPC, UNSP_LPC << 1); break;
		case CPUINFO_STR_REGISTER + UNSP_IRQ:			sprintf(info->s, "IRQ: %d", unsp->irq);     break;
		case CPUINFO_STR_REGISTER + UNSP_FIQ:			sprintf(info->s, "FIQ: %d", unsp->fiq);     break;
		case CPUINFO_STR_REGISTER + UNSP_SB:			sprintf(info->s, "SB: %d", unsp->sb);       break;
    }
}
