/****************************************************************************
*             real mode i286 emulator v1.4 by Fabrice Frances               *
*               (initial work based on David Hedley's pcemu)                *
****************************************************************************/
/* 26.March 2000 PeT changed set_irq_line */

#include "host.h"
#include "debugger.h"

#include "i86.h"
#include "i86intf.h"

extern int i386_dasm_one(char *buffer, UINT32 eip, const UINT8 *oprom, int addr_size, int op_size);


/* All pre-i286 CPUs have a 1MB address space */
#define AMASK	0xfffff


/* I86 registers */
typedef union
{									   /* eight general registers */
	UINT16 w[8];					   /* viewed as 16 bits registers */
	UINT8 b[16];					   /* or as 8 bit registers */
}
i86basicregs;

typedef struct
{
	i86basicregs regs;
	UINT32 pc;
	UINT32 prevpc;
	UINT32 base[4];
	UINT16 sregs[4];
	UINT16 flags;
	int (*irq_callback) (int irqline);
	INT32 AuxVal, OverVal, SignVal, ZeroVal, CarryVal, DirVal;		/* 0 or non-0 valued flags */
	UINT8 ParityVal;
	UINT8 TF, IF;				   /* 0 or 1 valued flags */
	UINT8 MF;						   /* V30 mode flag */
	UINT8 int_vector;
	INT8 nmi_state;
	INT8 irq_state;
	INT8 test_state;	/* PJB 03/05 */
	INT32 extra_cycles;       /* extra cycles for interrupts */
}
i86_Regs;


#include "i86time.c"

/***************************************************************************/
/* cpu state                                                               */
/***************************************************************************/

int i86_ICount;

static i86_Regs I;
static unsigned prefix_base;		   /* base address of the latest prefix segment */
static char seg_prefix;				   /* prefix segment indicator */

static UINT8 parity_table[256];

static struct i86_timing cycles;

/* The interrupt number of a pending external interrupt pending NMI is 2.   */
/* For INTR interrupts, the level is caught on the bus during an INTA cycle */

#define PREFIX(name) i86##name
#define PREFIX86(name) i86##name

#define I86
#include "instr86.h"
#include "ea.h"
#include "modrm.h"
#include "table86.h"

#include "instr86.c"
#undef I86

/***************************************************************************/
static void i86_state_register(int index)
{
	static const char type[] = "I86";
	state_save_register_item_array(type, index, I.regs.w);
	state_save_register_item(type, index, I.pc);
	state_save_register_item(type, index, I.prevpc);
	state_save_register_item_array(type, index, I.base);
	state_save_register_item_array(type, index, I.sregs);
	state_save_register_item(type, index, I.flags);
	state_save_register_item(type, index, I.AuxVal);
	state_save_register_item(type, index, I.OverVal);
	state_save_register_item(type, index, I.SignVal);
	state_save_register_item(type, index, I.ZeroVal);
	state_save_register_item(type, index, I.CarryVal);
	state_save_register_item(type, index, I.DirVal);
	state_save_register_item(type, index, I.ParityVal);
	state_save_register_item(type, index, I.TF);
	state_save_register_item(type, index, I.IF);
	state_save_register_item(type, index, I.MF);
	state_save_register_item(type, index, I.int_vector);
	state_save_register_item(type, index, I.nmi_state);
	state_save_register_item(type, index, I.irq_state);
	state_save_register_item(type, index, I.extra_cycles);
	state_save_register_item(type, index, I.test_state);	/* PJB 03/05 */
}

static void i86_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	unsigned int i, j, c;
	static const BREGS reg_name[8] = {AL, CL, DL, BL, AH, CH, DH, BH};
	for (i = 0; i < 256; i++)
	{
		for (j = i, c = 0; j > 0; j >>= 1)
			if (j & 1)
				c++;

		parity_table[i] = !(c & 1);
	}

	for (i = 0; i < 256; i++)
	{
		Mod_RM.reg.b[i] = reg_name[(i & 0x38) >> 3];
		Mod_RM.reg.w[i] = (WREGS) ((i & 0x38) >> 3);
	}

	for (i = 0xc0; i < 0x100; i++)
	{
		Mod_RM.RM.w[i] = (WREGS) (i & 7);
		Mod_RM.RM.b[i] = (BREGS) reg_name[i & 7];
	}

	I.irq_callback = irqcallback;

	i86_state_register(index);
}

static void i86_reset(void)
{
	int (*save_irqcallback)(int);

	save_irqcallback = I.irq_callback;
	memset(&I, 0, sizeof (I));
	I.irq_callback = save_irqcallback;

	I.sregs[CS] = 0xf000;
	I.base[CS] = SegBase(CS);
	I.pc = 0xffff0 & AMASK;
	ExpandFlags(I.flags);

	change_pc(I.pc);
}

static void i86_exit(void)
{
	/* nothing to do ? */
}

/* ASG 971222 -- added these interface functions */

static void i86_get_context(void *dst)
{
	if (dst)
		*(i86_Regs *) dst = I;
}

static void i86_set_context(void *src)
{
	if (src)
	{
		I = *(i86_Regs *)src;
		I.base[CS] = SegBase(CS);
		I.base[DS] = SegBase(DS);
		I.base[ES] = SegBase(ES);
		I.base[SS] = SegBase(SS);
		change_pc(I.pc);
	}
}

static void set_irq_line(int irqline, int state)
{
	if (irqline == INPUT_LINE_NMI)
	{
		if (I.nmi_state == state)
			return;
		I.nmi_state = state;

		/* on a rising edge, signal the NMI */
		if (state != CLEAR_LINE)
			PREFIX(_interrupt)(I86_NMI_INT_VECTOR);
	}
	else
	{
		I.irq_state = state;

		/* if the IF is set, signal an interrupt */
		if (state != CLEAR_LINE && I.IF)
			PREFIX(_interrupt)(-1);
	}
}

/* PJB 03/05 */
static void set_test_line(int state)
{
        I.test_state = !state;
}

static int i86_execute(int num_cycles)
{
	/* copy over the cycle counts if they're not correct */
	if (cycles.id != 8086)
		cycles = i86_cycles;

	/* adjust for any interrupts that came in */
	i86_ICount = num_cycles;
	i86_ICount -= I.extra_cycles;
	I.extra_cycles = 0;

	/* run until we're out */
	while (i86_ICount > 0)
	{
//#define VERBOSE_DEBUG
#ifdef VERBOSE_DEBUG
		logerror("[%04x:%04x]=%02x\tF:%04x\tAX=%04x\tBX=%04x\tCX=%04x\tDX=%04x %d%d%d%d%d%d%d%d%d\n",
				I.sregs[CS], I.pc - I.base[CS], ReadByte(I.pc), I.flags, I.regs.w[AX], I.regs.w[BX], I.regs.w[CX], I.regs.w[DX], I.AuxVal ? 1 : 0, I.OverVal ? 1 : 0,
				I.SignVal ? 1 : 0, I.ZeroVal ? 1 : 0, I.CarryVal ? 1 : 0, I.ParityVal ? 1 : 0, I.TF, I.IF, I.DirVal < 0 ? 1 : 0);
#endif
		CALL_MAME_DEBUG;

		seg_prefix = FALSE;
		I.prevpc = I.pc;
		TABLE86;
	}

	/* adjust for any interrupts that came in */
	i86_ICount -= I.extra_cycles;
	I.extra_cycles = 0;

	return num_cycles - i86_ICount;
}


#ifdef MAME_DEBUG
static offs_t i86_dasm(char *buffer, offs_t pc, const UINT8 *oprom, const UINT8 *opram)
{
	return i386_dasm_one(buffer, pc, oprom, 0, 0);
}
#endif /* MAME_DEBUG */


#if (HAS_I186 || HAS_I188)

#include "i186intf.h"

#undef PREFIX
#define PREFIX(name) i186##name
#define PREFIX186(name) i186##name

#define I186
#include "instr186.h"
#include "table186.h"

#include "instr86.c"
#include "instr186.c"
#undef I186

static int i186_execute(int num_cycles)
{
	/* copy over the cycle counts if they're not correct */
	if (cycles.id != 80186)
		cycles = i186_cycles;

	/* adjust for any interrupts that came in */
	i86_ICount = num_cycles;
	i86_ICount -= I.extra_cycles;
	I.extra_cycles = 0;

	/* run until we're out */
	while (i86_ICount > 0)
	{
#ifdef VERBOSE_DEBUG
		mame_printf_debug("[%04x:%04x]=%02x\tAX=%04x\tBX=%04x\tCX=%04x\tDX=%04x\n", I.sregs[CS], I.pc, ReadByte(I.pc), I.regs.w[AX],
			   I.regs.w[BX], I.regs.w[CX], I.regs.w[DX]);
#endif
		CALL_MAME_DEBUG;

		seg_prefix = FALSE;
		I.prevpc = I.pc;
		TABLE186;
	}

	/* adjust for any interrupts that came in */
	i86_ICount -= I.extra_cycles;
	I.extra_cycles = 0;

	return num_cycles - i86_ICount;
}

#endif



#if defined(INCLUDE_V20) && (HAS_V20 || HAS_V30 || HAS_V33)

#include "v30.h"
#include "v30intf.h"

/* compile the opcode emulating instruction new with v20 timing value */
/* MF flag -> opcode must be compiled new */
#undef PREFIX
#define PREFIX(name) v30##name
#undef PREFIX86
#define PREFIX86(name) v30##name
#undef PREFIX186
#define PREFIX186(name) v30##name
#define PREFIXV30(name) v30##name

#define nec_ICount i86_ICount

static UINT16 bytes[] =
{
	   1,	 2,    4,	 8,
	  16,	32,   64,  128,
	 256,  512, 1024, 2048,
	4096, 8192,16384,32768	/*,65536 */
};

#undef IncWordReg
#undef DecWordReg
#define V20
#include "instr86.h"
#include "instr186.h"
#include "instrv30.h"
#include "tablev30.h"

static void v30_interrupt(unsigned int_num, BOOLEAN md_flag)
{
	unsigned dest_seg, dest_off;

#if 0
	logerror("PC=%05x : NEC Interrupt %02d", activecpu_get_pc(), int_num);
#endif

	v30_pushf();
	I.TF = I.IF = 0;
	if (md_flag)
		SetMD(0);					   /* clear Mode-flag = start 8080 emulation mode */

	if (int_num == -1)
	{
		int_num = (*I.irq_callback) (0);
/*      logerror(" (indirect ->%02d) ",int_num); */
	}

	dest_off = ReadWord(int_num * 4);
	dest_seg = ReadWord(int_num * 4 + 2);

	PUSH(I.sregs[CS]);
	PUSH(I.pc - I.base[CS]);
	I.sregs[CS] = (WORD) dest_seg;
	I.base[CS] = SegBase(CS);
	I.pc = (I.base[CS] + dest_off) & AMASK;
	change_pc(I.pc);
/*  logerror("=%06x\n",activecpu_get_pc()); */
}

static void v30_trap(void)
{
	(*v30_instruction[FETCHOP])();
	v30_interrupt(1, 0);
}


#include "instr86.c"
#include "instr186.c"
#include "instrv30.c"
#undef V20

static void v30_reset(void)
{
	i86_reset(param);
	SetMD(1);
}

static int v30_execute(int num_cycles)
{
	/* copy over the cycle counts if they're not correct */
	if (cycles.id != 30)
		cycles = v30_cycles;

	/* adjust for any interrupts that came in */
	i86_ICount = num_cycles;
	i86_ICount -= I.extra_cycles;
	I.extra_cycles = 0;

	/* run until we're out */
	while (i86_ICount > 0)
	{
#ifdef VERBOSE_DEBUG
		mame_printf_debug("[%04x:%04x]=%02x\tAX=%04x\tBX=%04x\tCX=%04x\tDX=%04x\n", I.sregs[CS], I.pc, ReadByte(I.pc), I.regs.w[AX],
			   I.regs.w[BX], I.regs.w[CX], I.regs.w[DX]);
#endif
		CALL_MAME_DEBUG;

		seg_prefix = FALSE;
		I.prevpc = I.pc;
		TABLEV30;
	}

	/* adjust for any interrupts that came in */
	i86_ICount -= I.extra_cycles;
	I.extra_cycles = 0;

	return num_cycles - i86_ICount;
}

#endif



/**************************************************************************
 * Generic set_info
 **************************************************************************/

static void i86_set_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are set as 64-bit signed integers --- */
		case CPUINFO_INT_INPUT_STATE + 0:				set_irq_line(0, info->i);				break;
		case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:	set_irq_line(INPUT_LINE_NMI, info->i);	break;
		case CPUINFO_INT_INPUT_STATE + INPUT_LINE_TEST:	set_test_line(info->i);					break; /* PJB 03/05 */

		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + I86_PC:
			if (info->i - I.base[CS] >= 0x10000)
			{
				I.base[CS] = info->i & 0xffff0;
				I.sregs[CS] = I.base[CS] >> 4;
			}
			I.pc = info->i;
			break;
		case CPUINFO_INT_REGISTER + I86_IP:				I.pc = I.base[CS] + info->i;			break;
		case CPUINFO_INT_SP:
			if (info->i - I.base[SS] < 0x10000)
			{
				I.regs.w[SP] = info->i - I.base[SS];
			}
			else
			{
				I.base[SS] = info->i & 0xffff0;
				I.sregs[SS] = I.base[SS] >> 4;
				I.regs.w[SP] = info->i & 0x0000f;
			}
			break;
		case CPUINFO_INT_REGISTER + I86_SP:				I.regs.w[SP] = info->i; 				break;
		case CPUINFO_INT_REGISTER + I86_FLAGS: 			I.flags = info->i;	ExpandFlags(info->i); break;
		case CPUINFO_INT_REGISTER + I86_AX:				I.regs.w[AX] = info->i; 				break;
		case CPUINFO_INT_REGISTER + I86_CX:				I.regs.w[CX] = info->i; 				break;
		case CPUINFO_INT_REGISTER + I86_DX:				I.regs.w[DX] = info->i; 				break;
		case CPUINFO_INT_REGISTER + I86_BX:				I.regs.w[BX] = info->i; 				break;
		case CPUINFO_INT_REGISTER + I86_BP:				I.regs.w[BP] = info->i; 				break;
		case CPUINFO_INT_REGISTER + I86_SI:				I.regs.w[SI] = info->i; 				break;
		case CPUINFO_INT_REGISTER + I86_DI:				I.regs.w[DI] = info->i; 				break;
		case CPUINFO_INT_REGISTER + I86_ES:				I.sregs[ES] = info->i;	I.base[ES] = SegBase(ES); break;
		case CPUINFO_INT_REGISTER + I86_CS:				I.sregs[CS] = info->i;	I.base[CS] = SegBase(CS); break;
		case CPUINFO_INT_REGISTER + I86_SS:				I.sregs[SS] = info->i;	I.base[SS] = SegBase(SS); break;
		case CPUINFO_INT_REGISTER + I86_DS:				I.sregs[DS] = info->i;	I.base[DS] = SegBase(DS); break;
		case CPUINFO_INT_REGISTER + I86_VECTOR:			I.int_vector = info->i; 				break;
	}
}



/**************************************************************************
 * Generic get_info
 **************************************************************************/

void i86_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof(I);					break;
		case CPUINFO_INT_INPUT_LINES:					info->i = 1;							break;
		case CPUINFO_INT_DEFAULT_IRQ_VECTOR:			info->i = 0xff;							break;
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;
		case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 1;							break;
		case CPUINFO_INT_MIN_INSTRUCTION_BYTES:			info->i = 1;							break;
		case CPUINFO_INT_MAX_INSTRUCTION_BYTES:			info->i = 8;							break;
		case CPUINFO_INT_MIN_CYCLES:					info->i = 1;							break;
		case CPUINFO_INT_MAX_CYCLES:					info->i = 50;							break;

		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 8;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 20;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM: info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_DATA:	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 8;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 16;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_IO: 		info->i = 0;					break;

		case CPUINFO_INT_INPUT_STATE + 0:				info->i = I.irq_state;					break;
		case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:	info->i = I.nmi_state;					break;

		case CPUINFO_INT_INPUT_STATE + INPUT_LINE_TEST:	info->i = I.test_state;					break; /* PJB 03/05 */

		case CPUINFO_INT_PREVIOUSPC:					info->i = I.prevpc;						break;

		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + I86_PC:				info->i = I.pc;							break;
		case CPUINFO_INT_REGISTER + I86_IP:				info->i = I.pc - I.base[CS];			break;
		case CPUINFO_INT_SP:							info->i = I.base[SS] + I.regs.w[SP];	break;
		case CPUINFO_INT_REGISTER + I86_SP:				info->i = I.regs.w[SP];					break;
		case CPUINFO_INT_REGISTER + I86_FLAGS: 			I.flags = CompressFlags(); info->i = I.flags; break;
		case CPUINFO_INT_REGISTER + I86_AX:				info->i = I.regs.w[AX];					break;
		case CPUINFO_INT_REGISTER + I86_CX:				info->i = I.regs.w[CX];					break;
		case CPUINFO_INT_REGISTER + I86_DX:				info->i = I.regs.w[DX];					break;
		case CPUINFO_INT_REGISTER + I86_BX:				info->i = I.regs.w[BX];					break;
		case CPUINFO_INT_REGISTER + I86_BP:				info->i = I.regs.w[BP];					break;
		case CPUINFO_INT_REGISTER + I86_SI:				info->i = I.regs.w[SI];					break;
		case CPUINFO_INT_REGISTER + I86_DI:				info->i = I.regs.w[DI];					break;
		case CPUINFO_INT_REGISTER + I86_ES:				info->i = I.sregs[ES];					break;
		case CPUINFO_INT_REGISTER + I86_CS:				info->i = I.sregs[CS];					break;
		case CPUINFO_INT_REGISTER + I86_SS:				info->i = I.sregs[SS];					break;
		case CPUINFO_INT_REGISTER + I86_DS:				info->i = I.sregs[DS];					break;
		case CPUINFO_INT_REGISTER + I86_VECTOR:			info->i = I.int_vector;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_SET_INFO:						info->setinfo = i86_set_info;			break;
		case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = i86_get_context;		break;
		case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = i86_set_context;		break;
		case CPUINFO_PTR_INIT:							info->init = i86_init;					break;
		case CPUINFO_PTR_RESET:							info->reset = i86_reset;				break;
		case CPUINFO_PTR_EXIT:							info->exit = i86_exit;					break;
		case CPUINFO_PTR_EXECUTE:						info->execute = i86_execute;			break;
		case CPUINFO_PTR_BURN:							info->burn = NULL;						break;
#ifdef MAME_DEBUG
		case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = i86_dasm;			break;
#endif /* MAME_DEBUG */
		case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &i86_ICount;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "I8086");				break;
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s, "Intel 80x86");			break;
		case CPUINFO_STR_CORE_VERSION:					strcpy(info->s, "1.4");					break;
		case CPUINFO_STR_CORE_FILE:						strcpy(info->s, __FILE__);				break;
		case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s, "Real mode i286 emulator v1.4 by Fabrice Frances\n(initial work I.based on David Hedley's pcemu)"); break;

		case CPUINFO_STR_FLAGS:
			I.flags = CompressFlags();
			sprintf(info->s, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
					I.flags & 0x8000 ? '?' : '.',
					I.flags & 0x4000 ? '?' : '.',
					I.flags & 0x2000 ? '?' : '.',
					I.flags & 0x1000 ? '?' : '.',
					I.flags & 0x0800 ? 'O' : '.',
					I.flags & 0x0400 ? 'D' : '.',
					I.flags & 0x0200 ? 'I' : '.',
					I.flags & 0x0100 ? 'T' : '.',
					I.flags & 0x0080 ? 'S' : '.',
					I.flags & 0x0040 ? 'Z' : '.',
					I.flags & 0x0020 ? '?' : '.',
					I.flags & 0x0010 ? 'A' : '.',
					I.flags & 0x0008 ? '?' : '.',
					I.flags & 0x0004 ? 'P' : '.',
					I.flags & 0x0002 ? 'N' : '.',
					I.flags & 0x0001 ? 'C' : '.');
			break;

		case CPUINFO_STR_REGISTER + I86_PC: 			sprintf(info->s, "PC:%04X", I.pc); break;
		case CPUINFO_STR_REGISTER + I86_IP: 			sprintf(info->s, "IP: %04X", I.pc - I.base[CS]); break;
		case CPUINFO_STR_REGISTER + I86_SP: 			sprintf(info->s, "SP: %04X", I.regs.w[SP]); break;
		case CPUINFO_STR_REGISTER + I86_FLAGS:			sprintf(info->s, "F:%04X", I.flags); break;
		case CPUINFO_STR_REGISTER + I86_AX: 			sprintf(info->s, "AX:%04X", I.regs.w[AX]); break;
		case CPUINFO_STR_REGISTER + I86_CX: 			sprintf(info->s, "CX:%04X", I.regs.w[CX]); break;
		case CPUINFO_STR_REGISTER + I86_DX: 			sprintf(info->s, "DX:%04X", I.regs.w[DX]); break;
		case CPUINFO_STR_REGISTER + I86_BX: 			sprintf(info->s, "BX:%04X", I.regs.w[BX]); break;
		case CPUINFO_STR_REGISTER + I86_BP: 			sprintf(info->s, "BP:%04X", I.regs.w[BP]); break;
		case CPUINFO_STR_REGISTER + I86_SI: 			sprintf(info->s, "SI: %04X", I.regs.w[SI]); break;
		case CPUINFO_STR_REGISTER + I86_DI: 			sprintf(info->s, "DI: %04X", I.regs.w[DI]); break;
		case CPUINFO_STR_REGISTER + I86_ES: 			sprintf(info->s, "ES:%04X", I.sregs[ES]); break;
		case CPUINFO_STR_REGISTER + I86_CS: 			sprintf(info->s, "CS:%04X", I.sregs[CS]); break;
		case CPUINFO_STR_REGISTER + I86_SS: 			sprintf(info->s, "SS:%04X", I.sregs[SS]); break;
		case CPUINFO_STR_REGISTER + I86_DS: 			sprintf(info->s, "DS:%04X", I.sregs[DS]); break;
		case CPUINFO_STR_REGISTER + I86_VECTOR:		 	sprintf(info->s, "V:%02X", I.int_vector); break;
	}
}


#if (HAS_I88)
/**************************************************************************
 * CPU-specific get_info/set_info
 **************************************************************************/

void i88_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "I8088");				break;

		default:										i86_get_info(state, info);				break;
	}
}
#endif


#if (HAS_I186)
/**************************************************************************
 * CPU-specific get_info/set_info
 **************************************************************************/

void i186_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_EXECUTE:						info->execute = i186_execute;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "I80186");				break;

		default:										i86_get_info(state, info);				break;
	}
}
#endif


#if (HAS_I188)
/**************************************************************************
 * CPU-specific get_info/set_info
 **************************************************************************/

void i188_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_EXECUTE:						info->execute = i186_execute;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "I80188");				break;

		default:										i86_get_info(state, info);				break;
	}
}
#endif


#if defined(INCLUDE_V20) && (HAS_V20)
/**************************************************************************
 * CPU-specific get_info/set_info
 **************************************************************************/

void v20_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = v30_init;					break;
		case CPUINFO_PTR_RESET:							info->reset = v30_reset;				break;
		case CPUINFO_PTR_EXECUTE:						info->execute = v30_execute;			break;
		case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = v30_dasm;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "V20");					break;
		case CPUINFO_STR_FLAGS:
			I.flags = CompressFlags();
			sprintf(info->s, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
					I.flags & 0x8000 ? 'M' : '.',
					I.flags & 0x4000 ? '?' : '.',
					I.flags & 0x2000 ? '?' : '.',
					I.flags & 0x1000 ? '?' : '.',
					I.flags & 0x0800 ? 'O' : '.',
					I.flags & 0x0400 ? 'D' : '.',
					I.flags & 0x0200 ? 'I' : '.',
					I.flags & 0x0100 ? 'T' : '.',
					I.flags & 0x0080 ? 'S' : '.',
					I.flags & 0x0040 ? 'Z' : '.',
					I.flags & 0x0020 ? '?' : '.',
					I.flags & 0x0010 ? 'A' : '.',
					I.flags & 0x0008 ? '?' : '.',
					I.flags & 0x0004 ? 'P' : '.',
					I.flags & 0x0002 ? 'N' : '.',
					I.flags & 0x0001 ? 'C' : '.');
			break;

		default:
			i86_get_info(state, info);
			break;
	}
}
#endif


#if defined(INCLUDE_V20) && (HAS_V30)
/**************************************************************************
 * CPU-specific get_info/set_info
 **************************************************************************/

void v30_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = v30_init;					break;
		case CPUINFO_PTR_RESET:							info->reset = v30_reset;				break;
		case CPUINFO_PTR_EXECUTE:						info->execute = v30_execute;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "V30");					break;
		case CPUINFO_STR_FLAGS:
			I.flags = CompressFlags();
			sprintf(info->s, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
					I.flags & 0x8000 ? 'M' : '.',
					I.flags & 0x4000 ? '?' : '.',
					I.flags & 0x2000 ? '?' : '.',
					I.flags & 0x1000 ? '?' : '.',
					I.flags & 0x0800 ? 'O' : '.',
					I.flags & 0x0400 ? 'D' : '.',
					I.flags & 0x0200 ? 'I' : '.',
					I.flags & 0x0100 ? 'T' : '.',
					I.flags & 0x0080 ? 'S' : '.',
					I.flags & 0x0040 ? 'Z' : '.',
					I.flags & 0x0020 ? '?' : '.',
					I.flags & 0x0010 ? 'A' : '.',
					I.flags & 0x0008 ? '?' : '.',
					I.flags & 0x0004 ? 'P' : '.',
					I.flags & 0x0002 ? 'N' : '.',
					I.flags & 0x0001 ? 'C' : '.');
			break;

		default:
			i86_get_info(state, info);
			break;
	}
}
#endif


#if defined(INCLUDE_V20) && (HAS_V33)
/**************************************************************************
 * CPU-specific get_info/set_info
 **************************************************************************/

void v33_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = v33_init;					break;
		case CPUINFO_PTR_RESET:							info->reset = v33_reset;				break;
		case CPUINFO_PTR_EXECUTE:						info->execute = v33_execute;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "V33");					break;
		case CPUINFO_STR_FLAGS:
			I.flags = CompressFlags();
			sprintf(info->s, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
					I.flags & 0x8000 ? 'M' : '.',
					I.flags & 0x4000 ? '?' : '.',
					I.flags & 0x2000 ? '?' : '.',
					I.flags & 0x1000 ? '?' : '.',
					I.flags & 0x0800 ? 'O' : '.',
					I.flags & 0x0400 ? 'D' : '.',
					I.flags & 0x0200 ? 'I' : '.',
					I.flags & 0x0100 ? 'T' : '.',
					I.flags & 0x0080 ? 'S' : '.',
					I.flags & 0x0040 ? 'Z' : '.',
					I.flags & 0x0020 ? '?' : '.',
					I.flags & 0x0010 ? 'A' : '.',
					I.flags & 0x0008 ? '?' : '.',
					I.flags & 0x0004 ? 'P' : '.',
					I.flags & 0x0002 ? 'N' : '.',
					I.flags & 0x0001 ? 'C' : '.');
			break;

		default:
			i86_get_info(state, info);
			break;
	}
}
#endif
