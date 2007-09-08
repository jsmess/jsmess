/***************************************************************************

    cpuintrf.h

    Core CPU interface functions and definitions.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#pragma once

#ifndef __CPUINTRF_H__
#define __CPUINTRF_H__

#include "cpuint.h"
#include "cpuexec.h"
#include "mame.h"
#include "state.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define MAX_CPU 8

/* Enum listing all the CPUs */
enum
{
	CPU_DUMMY,
	CPU_Z80,
	CPU_Z180,
	CPU_8080,
	CPU_8085A,
	CPU_M6502,
	CPU_M65C02,
	CPU_M65SC02,
	CPU_M65CE02,
	CPU_M6509,
	CPU_M6510,
	CPU_M6510T,
	CPU_M7501,
	CPU_M8502,
	CPU_N2A03,
	CPU_DECO16,
	CPU_M4510,
	CPU_H6280,
	CPU_I8086,
	CPU_I8088,
	CPU_I80186,
	CPU_I80188,
	CPU_I80286,
	CPU_V20,
	CPU_V25,
	CPU_V30,
	CPU_V33,
	CPU_V35,
	CPU_V60,
	CPU_V70,
	CPU_I8035,
	CPU_I8039,
	CPU_I8048,
	CPU_N7751,
	CPU_I8X41,
	CPU_I8051,
	CPU_I8052,
	CPU_I8751,
	CPU_I8752,
	CPU_M6800,
	CPU_M6801,
	CPU_M6802,
	CPU_M6803,
	CPU_M6808,
	CPU_HD63701,
	CPU_NSC8105,
	CPU_M6805,
	CPU_M68705,
	CPU_HD63705,
	CPU_HD6309,
	CPU_M6809,
	CPU_M6809E,
	CPU_KONAMI,
	CPU_M68000,
	CPU_M68008,
	CPU_M68010,
	CPU_M68EC020,
	CPU_M68020,
	CPU_M68040,
	CPU_T11,
	CPU_S2650,
	CPU_TMS34010,
	CPU_TMS34020,
	CPU_TI990_10,
	CPU_TMS9900,
	CPU_TMS9940,
	CPU_TMS9980,
	CPU_TMS9985,
	CPU_TMS9989,
	CPU_TMS9995,
	CPU_TMS99100,
	CPU_TMS99105A,
	CPU_TMS99110A,
	CPU_Z8000,
	CPU_TMS32010,
	CPU_TMS32025,
	CPU_TMS32026,
	CPU_TMS32031,
	CPU_TMS32051,
	CPU_CCPU,
	CPU_ADSP2100,
 	CPU_ADSP2101,
	CPU_ADSP2104,
	CPU_ADSP2105,
	CPU_ADSP2115,
	CPU_ADSP2181,
	CPU_PSXCPU,
	CPU_ASAP,
	CPU_UPD7810,
	CPU_UPD7807,
	CPU_JAGUARGPU,
	CPU_JAGUARDSP,
	CPU_R3000BE,
	CPU_R3000LE,
	CPU_R4600BE,
	CPU_R4600LE,
	CPU_R4650BE,
	CPU_R4650LE,
	CPU_R4700BE,
	CPU_R4700LE,
	CPU_R5000BE,
	CPU_R5000LE,
	CPU_QED5271BE,
	CPU_QED5271LE,
	CPU_RM7000BE,
	CPU_RM7000LE,
	CPU_ARM,
	CPU_ARM7,
	CPU_SH2,
	CPU_SH4,
	CPU_DSP32C,
	CPU_PIC16C54,
	CPU_PIC16C55,
	CPU_PIC16C56,
	CPU_PIC16C57,
	CPU_PIC16C58,
	CPU_G65816,
	CPU_SPC700,
	CPU_E116T,
	CPU_E116XT,
	CPU_E116XS,
	CPU_E116XSR,
	CPU_E132N,
	CPU_E132T,
	CPU_E132XN,
	CPU_E132XT,
	CPU_E132XS,
	CPU_E132XSR,
	CPU_GMS30C2116,
	CPU_GMS30C2132,
	CPU_GMS30C2216,
	CPU_GMS30C2232,
	CPU_I386,
	CPU_I486,
	CPU_PENTIUM,
	CPU_MEDIAGX,
	CPU_I960,
	CPU_H83002,
	CPU_V810,
	CPU_M37702,
	CPU_M37710,
	CPU_PPC403,
	CPU_PPC601,
	CPU_PPC602,
	CPU_PPC603,
	CPU_MPC8240,
	CPU_SE3208,
	CPU_MC68HC11,
	CPU_ADSP21062,
	CPU_DSP56156,
	CPU_RSP,
	CPU_ALPHA8201,
	CPU_ALPHA8301,
	CPU_CDP1802,
	CPU_COP420,
	CPU_COP410,
	CPU_COP411,
	CPU_TMP90840,
	CPU_TMP90841,
	CPU_TMP91640,
	CPU_TMP91641,
	CPU_APEXC,
	CPU_CP1610,
	CPU_F8,
	CPU_LH5801,
	CPU_PDP1,
	CPU_SATURN,
	CPU_SC61860,
	CPU_TX0_64KW,
	CPU_TX0_8KW,
	CPU_Z80GB,
	CPU_TMS7000,
	CPU_TMS7000_EXL,
	CPU_SM8500,
	CPU_V30MZ,
	CPU_MB8841,
	CPU_MB8842,
	CPU_MB8843,
	CPU_MB8844,
	CPU_MB86233,
	CPU_SSP1610,
	CPU_MINX,
    CPU_COUNT
};


/* Interrupt line constants */
enum
{
	/* line states */
	CLEAR_LINE = 0,				/* clear (a fired, held or pulsed) line */
	ASSERT_LINE,				/* assert an interrupt immediately */
	HOLD_LINE,					/* hold interrupt line until acknowledged */
	PULSE_LINE,					/* pulse interrupt line for one instruction */

	/* internal flags (not for use by drivers!) */
	INTERNAL_CLEAR_LINE = 100 + CLEAR_LINE,
	INTERNAL_ASSERT_LINE = 100 + ASSERT_LINE,

	/* input lines */
	MAX_INPUT_LINES = 32+3,
	INPUT_LINE_IRQ0 = 0,
	INPUT_LINE_IRQ1 = 1,
	INPUT_LINE_IRQ2 = 2,
	INPUT_LINE_IRQ3 = 3,
	INPUT_LINE_IRQ4 = 4,
	INPUT_LINE_IRQ5 = 5,
	INPUT_LINE_IRQ6 = 6,
	INPUT_LINE_IRQ7 = 7,
	INPUT_LINE_IRQ8 = 8,
	INPUT_LINE_IRQ9 = 9,
	INPUT_LINE_NMI = MAX_INPUT_LINES - 3,

	/* special input lines that are implemented in the core */
	INPUT_LINE_RESET = MAX_INPUT_LINES - 2,
	INPUT_LINE_HALT = MAX_INPUT_LINES - 1,

	/* output lines */
	MAX_OUTPUT_LINES = 32
};


/* Maximum number of registers of any CPU */
enum
{
	MAX_REGS = 256
};


/* CPU information constants */
enum
{
	/* --- the following bits of info are returned as 64-bit signed integers --- */
	CPUINFO_INT_FIRST = 0x00000,

	CPUINFO_INT_CONTEXT_SIZE = CPUINFO_INT_FIRST,		/* R/O: size of CPU context in bytes */
	CPUINFO_INT_INPUT_LINES,							/* R/O: number of input lines */
	CPUINFO_INT_OUTPUT_LINES,							/* R/O: number of output lines */
	CPUINFO_INT_DEFAULT_IRQ_VECTOR,						/* R/O: default IRQ vector */
	CPUINFO_INT_ENDIANNESS,								/* R/O: either CPU_IS_BE or CPU_IS_LE */
	CPUINFO_INT_CLOCK_DIVIDER,							/* R/O: internal clock divider */
	CPUINFO_INT_MIN_INSTRUCTION_BYTES,					/* R/O: minimum bytes per instruction */
	CPUINFO_INT_MAX_INSTRUCTION_BYTES,					/* R/O: maximum bytes per instruction */
	CPUINFO_INT_MIN_CYCLES,								/* R/O: minimum cycles for a single instruction */
	CPUINFO_INT_MAX_CYCLES,								/* R/O: maximum cycles for a single instruction */

	CPUINFO_INT_DATABUS_WIDTH,							/* R/O: data bus size for each address space (8,16,32,64) */
	CPUINFO_INT_DATABUS_WIDTH_LAST = CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACES - 1,
	CPUINFO_INT_ADDRBUS_WIDTH,							/* R/O: address bus size for each address space (12-32) */
	CPUINFO_INT_ADDRBUS_WIDTH_LAST = CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACES - 1,
	CPUINFO_INT_ADDRBUS_SHIFT,							/* R/O: shift applied to addresses each address space (+3 means >>3, -1 means <<1) */
	CPUINFO_INT_ADDRBUS_SHIFT_LAST = CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACES - 1,
	CPUINFO_INT_LOGADDR_WIDTH,							/* R/O: address bus size for logical accesses in each space (0=same as physical) */
	CPUINFO_INT_LOGADDR_WIDTH_LAST = CPUINFO_INT_LOGADDR_WIDTH + ADDRESS_SPACES - 1,
	CPUINFO_INT_PAGE_SHIFT,								/* R/O: size of a page log 2 (i.e., 12=4096), or 0 if paging not supported */
	CPUINFO_INT_PAGE_SHIFT_LAST = CPUINFO_INT_PAGE_SHIFT + ADDRESS_SPACES - 1,

	CPUINFO_INT_SP,										/* R/W: the current stack pointer value */
	CPUINFO_INT_PC,										/* R/W: the current PC value */
	CPUINFO_INT_PREVIOUSPC,								/* R/W: the previous PC value */
	CPUINFO_INT_INPUT_STATE,							/* R/W: states for each input line */
	CPUINFO_INT_INPUT_STATE_LAST = CPUINFO_INT_INPUT_STATE + MAX_INPUT_LINES - 1,
	CPUINFO_INT_OUTPUT_STATE,							/* R/W: states for each output line */
	CPUINFO_INT_OUTPUT_STATE_LAST = CPUINFO_INT_OUTPUT_STATE + MAX_OUTPUT_LINES - 1,
	CPUINFO_INT_REGISTER,								/* R/W: values of up to MAX_REGs registers */
	CPUINFO_INT_REGISTER_LAST = CPUINFO_INT_REGISTER + MAX_REGS - 1,

	CPUINFO_INT_CPU_SPECIFIC = 0x08000,					/* R/W: CPU-specific values start here */

	/* --- the following bits of info are returned as pointers to data or functions --- */
	CPUINFO_PTR_FIRST = 0x10000,

	CPUINFO_PTR_SET_INFO = CPUINFO_PTR_FIRST,			/* R/O: void (*set_info)(UINT32 state, INT64 data, void *ptr) */
	CPUINFO_PTR_GET_CONTEXT,							/* R/O: void (*get_context)(void *buffer) */
	CPUINFO_PTR_SET_CONTEXT,							/* R/O: void (*set_context)(void *buffer) */
	CPUINFO_PTR_INIT,									/* R/O: void (*init)(int index, int clock, const void *config, int (*irqcallback)(int)) */
	CPUINFO_PTR_RESET,									/* R/O: void (*reset)(void) */
	CPUINFO_PTR_EXIT,									/* R/O: void (*exit)(void) */
	CPUINFO_PTR_EXECUTE,								/* R/O: int (*execute)(int cycles) */
	CPUINFO_PTR_BURN,									/* R/O: void (*burn)(int cycles) */
	CPUINFO_PTR_DISASSEMBLE,							/* R/O: offs_t (*disassemble)(char *buffer, offs_t pc, const UINT8 *oprom, const UINT8 *opram) */
	CPUINFO_PTR_TRANSLATE,								/* R/O: int (*translate)(int space, offs_t *address) */
	CPUINFO_PTR_READ,									/* R/O: int (*read)(int space, UINT32 offset, int size, UINT64 *value) */
	CPUINFO_PTR_WRITE,									/* R/O: int (*write)(int space, UINT32 offset, int size, UINT64 value) */
	CPUINFO_PTR_READOP,									/* R/O: int (*readop)(UINT32 offset, int size, UINT64 *value) */
	CPUINFO_PTR_DEBUG_SETUP_COMMANDS,					/* R/O: void (*setup_commands)(void) */
	CPUINFO_PTR_INSTRUCTION_COUNTER,					/* R/O: int *icount */
	CPUINFO_PTR_INTERNAL_MEMORY_MAP,					/* R/O: construct_map_t map */
	CPUINFO_PTR_INTERNAL_MEMORY_MAP_LAST = CPUINFO_PTR_INTERNAL_MEMORY_MAP + ADDRESS_SPACES - 1,
	CPUINFO_PTR_DEBUG_REGISTER_LIST,					/* R/O: int *list: list of registers for the debugger */

	CPUINFO_PTR_CPU_SPECIFIC = 0x18000,					/* R/W: CPU-specific values start here */

	/* --- the following bits of info are returned as NULL-terminated strings --- */
	CPUINFO_STR_FIRST = 0x20000,

	CPUINFO_STR_NAME = CPUINFO_STR_FIRST,				/* R/O: name of the CPU */
	CPUINFO_STR_CORE_FAMILY,							/* R/O: family of the CPU */
	CPUINFO_STR_CORE_VERSION,							/* R/O: version of the CPU core */
	CPUINFO_STR_CORE_FILE,								/* R/O: file containing the CPU core */
	CPUINFO_STR_CORE_CREDITS,							/* R/O: credits for the CPU core */
	CPUINFO_STR_FLAGS,									/* R/O: string representation of the main flags value */
	CPUINFO_STR_REGISTER,								/* R/O: string representation of up to MAX_REGs registers */
	CPUINFO_STR_REGISTER_LAST = CPUINFO_STR_REGISTER + MAX_REGS - 1,

	CPUINFO_STR_CPU_SPECIFIC = 0x28000					/* R/W: CPU-specific values start here */
};


/* get_reg/set_reg constants */
enum
{
	/* This value is passed to activecpu_get_reg to retrieve the previous
     * program counter value, ie. before a CPU emulation started
     * to fetch opcodes and arguments for the current instrution. */
	REG_PREVIOUSPC = CPUINFO_INT_PREVIOUSPC - CPUINFO_INT_REGISTER,

	/* This value is passed to activecpu_get_reg to retrieve the current
     * program counter value. */
	REG_PC = CPUINFO_INT_PC - CPUINFO_INT_REGISTER,

	/* This value is passed to activecpu_get_reg to retrieve the current
     * stack pointer value. */
	REG_SP = CPUINFO_INT_SP - CPUINFO_INT_REGISTER
};


/* Endianness constants */
enum
{
	CPU_IS_LE = 0,				/* emulated CPU is little endian */
	CPU_IS_BE					/* emulated CPU is big endian */
};


/* Disassembler constants */
#define DASMFLAG_SUPPORTED		0x80000000	/* are disassembly flags supported? */
#define DASMFLAG_STEP_OUT		0x40000000	/* this instruction should be the end of a step out sequence */
#define DASMFLAG_STEP_OVER		0x20000000	/* this instruction should be stepped over by setting a breakpoint afterwards */
#define DASMFLAG_OVERINSTMASK	0x18000000	/* number of extra instructions to skip when stepping over */
#define DASMFLAG_OVERINSTSHIFT	27			/* bits to shift after masking to get the value */
#define DASMFLAG_LENGTHMASK 	0x0000ffff	/* the low 16-bits contain the actual length */
#define DASMFLAG_STEP_OVER_EXTRA(x)			((x) << DASMFLAG_OVERINSTSHIFT)



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef union _cpuinfo cpuinfo;
union _cpuinfo
{
	INT64	i;											/* generic integers */
	void *	p;											/* generic pointers */
	genf *  f;											/* generic function pointers */
	char *	s;											/* generic strings */

	void	(*setinfo)(UINT32 state, cpuinfo *info);	/* CPUINFO_PTR_SET_INFO */
	void	(*getcontext)(void *context);				/* CPUINFO_PTR_GET_CONTEXT */
	void	(*setcontext)(void *context);				/* CPUINFO_PTR_SET_CONTEXT */
	void	(*init)(int index, int clock, const void *config, int (*irqcallback)(int));/* CPUINFO_PTR_INIT */
	void	(*reset)(void);								/* CPUINFO_PTR_RESET */
	void	(*exit)(void);								/* CPUINFO_PTR_EXIT */
	int		(*execute)(int cycles);						/* CPUINFO_PTR_EXECUTE */
	void	(*burn)(int cycles);						/* CPUINFO_PTR_BURN */
	offs_t	(*disassemble)(char *buffer, offs_t pc, const UINT8 *oprom, const UINT8 *opram);/* CPUINFO_PTR_DISASSEMBLE */
	int 	(*translate)(int space, offs_t *address);	/* CPUINFO_PTR_TRANSLATE */
	int		(*read)(int space, UINT32 offset, int size, UINT64 *value);/* CPUINFO_PTR_READ */
	int		(*write)(int space, UINT32 offset, int size, UINT64 value);/* CPUINFO_PTR_WRITE */
	int		(*readop)(UINT32 offset, int size, UINT64 *value);/* CPUINFO_PTR_READOP */
	void	(*setup_commands)(void);					/* CPUINFO_PTR_DEBUG_SETUP_COMMANDS */
	int *	icount;										/* CPUINFO_PTR_INSTRUCTION_COUNTER */
	construct_map_t internal_map;						/* CPUINFO_PTR_INTERNAL_MEMORY_MAP */
};


typedef struct _cpu_interface cpu_interface;
struct _cpu_interface
{
	/* table of core functions */
	void		(*get_info)(UINT32 state, cpuinfo *info);
	void		(*set_info)(UINT32 state, cpuinfo *info);
	void		(*get_context)(void *buffer);
	void		(*set_context)(void *buffer);
	void		(*init)(int index, int clock, const void *config, int (*irqcallback)(int));
	void		(*reset)(void);
	void		(*exit)(void);
	int			(*execute)(int cycles);
	void		(*burn)(int cycles);
	offs_t		(*disassemble)(char *buffer, offs_t pc, const UINT8 *oprom, const UINT8 *opram);
	int			(*translate)(int space, offs_t *address);

	/* other info */
	size_t		context_size;
	INT8		address_shift;
	int *		icount;
};



/***************************************************************************
    CORE CPU INTERFACE FUNCTIONS
***************************************************************************/

/* reset the internal CPU tracking */
void cpuintrf_init(running_machine *machine);

/* set up the interface for one CPU of a given type */
int	cpuintrf_init_cpu(int cpunum, int cputype, int clock, const void *config, int (*irqcallback)(int));

/* clean up the interface for one CPU */
void cpuintrf_exit_cpu(int cpunum);

/* remember the previous context and set a new one */
void cpuintrf_push_context(int cpunum);

/* restore the previous context */
void cpuintrf_pop_context(void);

/* circular string buffer */
char *cpuintrf_temp_str(void);

/* set the dasm override handler */
void cpuintrf_set_dasm_override(int cpunum, offs_t (*dasm_override)(char *buffer, offs_t pc, const UINT8 *oprom, const UINT8 *opram));



/***************************************************************************
    ACTIVE CPU ACCCESSORS
***************************************************************************/

/* get info accessors */
INT64 activecpu_get_info_int(UINT32 state);
void *activecpu_get_info_ptr(UINT32 state);
genf *activecpu_get_info_fct(UINT32 state);
const char *activecpu_get_info_string(UINT32 state);

/* set info accessors */
void activecpu_set_info_int(UINT32 state, INT64 data);
void activecpu_set_info_ptr(UINT32 state, void *data);
void activecpu_set_info_fct(UINT32 state, genf *data);

/* apply a +/- to the current icount */
void activecpu_adjust_icount(int delta);

/* return the current icount */
int activecpu_get_icount(void);

/* ensure banking is reset properly */
void activecpu_reset_banking(void);

/* set the input line on a CPU -- drivers use cpu_set_input_line() */
void activecpu_set_input_line(int irqline, int state);

/* return the PC, corrected to a byte offset, on the active CPU */
offs_t activecpu_get_physical_pc_byte(void);

/* update the banking on the active CPU */
void activecpu_set_opbase(offs_t val);

/* disassemble a line at a given PC on the active CPU */
offs_t activecpu_dasm(char *buffer, offs_t pc, const UINT8 *oprom, const UINT8 *opram);

#define activecpu_context_size()				activecpu_get_info_int(CPUINFO_INT_CONTEXT_SIZE)
#define activecpu_input_lines()					activecpu_get_info_int(CPUINFO_INT_INPUT_LINES)
#define activecpu_output_lines()				activecpu_get_info_int(CPUINFO_INT_OUTPUT_LINES)
#define activecpu_default_irq_vector()			activecpu_get_info_int(CPUINFO_INT_DEFAULT_IRQ_VECTOR)
#define activecpu_endianness()					activecpu_get_info_int(CPUINFO_INT_ENDIANNESS)
#define activecpu_clock_divider()				activecpu_get_info_int(CPUINFO_INT_CLOCK_DIVIDER)
#define activecpu_min_instruction_bytes()		activecpu_get_info_int(CPUINFO_INT_MIN_INSTRUCTION_BYTES)
#define activecpu_max_instruction_bytes()		activecpu_get_info_int(CPUINFO_INT_MAX_INSTRUCTION_BYTES)
#define activecpu_min_cycles()					activecpu_get_info_int(CPUINFO_INT_MIN_CYCLES)
#define activecpu_max_cycles()					activecpu_get_info_int(CPUINFO_INT_MAX_CYCLES)
#define activecpu_databus_width(space)			activecpu_get_info_int(CPUINFO_INT_DATABUS_WIDTH + (space))
#define activecpu_addrbus_width(space)			activecpu_get_info_int(CPUINFO_INT_ADDRBUS_WIDTH + (space))
#define activecpu_addrbus_shift(space)			activecpu_get_info_int(CPUINFO_INT_ADDRBUS_SHIFT + (space))
#define activecpu_logaddr_width(space)			activecpu_get_info_int(CPUINFO_INT_LOGADDR_WIDTH + (space))
#define activecpu_page_shift(space)				activecpu_get_info_int(CPUINFO_INT_PAGE_SHIFT + (space))
#define activecpu_get_reg(reg)					activecpu_get_info_int(CPUINFO_INT_REGISTER + (reg))
#define activecpu_debug_register_list()			activecpu_get_info_ptr(CPUINFO_PTR_DEBUG_REGISTER_LIST)
#define activecpu_name()						activecpu_get_info_string(CPUINFO_STR_NAME)
#define activecpu_core_family()					activecpu_get_info_string(CPUINFO_STR_CORE_FAMILY)
#define activecpu_core_version()				activecpu_get_info_string(CPUINFO_STR_CORE_VERSION)
#define activecpu_core_file()					activecpu_get_info_string(CPUINFO_STR_CORE_FILE)
#define activecpu_core_credits()				activecpu_get_info_string(CPUINFO_STR_CORE_CREDITS)
#define activecpu_flags()						activecpu_get_info_string(CPUINFO_STR_FLAGS)
#define activecpu_irq_string(irq)				activecpu_get_info_string(CPUINFO_STR_IRQ_STATE + (irq))
#define activecpu_reg_string(reg)				activecpu_get_info_string(CPUINFO_STR_REGISTER + (reg))

#define activecpu_set_reg(reg, val)				activecpu_set_info_int(CPUINFO_INT_REGISTER + (reg), (val))



/***************************************************************************
    SPECIFIC CPU ACCCESSORS
***************************************************************************/

/* get info accessors */
INT64 cpunum_get_info_int(int cpunum, UINT32 state);
void *cpunum_get_info_ptr(int cpunum, UINT32 state);
genf *cpunum_get_info_fct(int cpunum, UINT32 state);
const char *cpunum_get_info_string(int cpunum, UINT32 state);

/* set info accessors */
void cpunum_set_info_int(int cpunum, UINT32 state, INT64 data);
void cpunum_set_info_ptr(int cpunum, UINT32 state, void *data);
void cpunum_set_info_fct(int cpunum, UINT32 state, genf *data);

/* execute the requested cycles on a given CPU */
int cpunum_execute(int cpunum, int cycles);

/* signal a reset for a given CPU */
void cpunum_reset(int cpunum);

/* read a byte from another CPU's memory space */
UINT8 cpunum_read_byte(int cpunum, offs_t address);

/* write a byte from another CPU's memory space */
void cpunum_write_byte(int cpunum, offs_t address, UINT8 data);

/* return a pointer to the saved context of a given CPU, or NULL if the
   context is active (and contained within the CPU core */
void *cpunum_get_context_ptr(int cpunum);

/* return the PC, corrected to a byte offset, on a given CPU */
offs_t cpunum_get_physical_pc_byte(int cpunum);

/* update the banking on a given CPU */
void cpunum_set_opbase(int cpunum, offs_t val);

/* disassemble a line at a given PC on a given CPU */
offs_t cpunum_dasm(int cpunum, char *buffer, offs_t pc, const UINT8 *oprom, const UINT8 *opram);

#define cpunum_context_size(cpunum)				cpunum_get_info_int(cpunum, CPUINFO_INT_CONTEXT_SIZE)
#define cpunum_input_lines(cpunum)				cpunum_get_info_int(cpunum, CPUINFO_INT_INPUT_LINES)
#define cpunum_output_lines(cpunum)				cpunum_get_info_int(cpunum, CPUINFO_INT_OUTPUT_LINES)
#define cpunum_default_irq_vector(cpunum)		cpunum_get_info_int(cpunum, CPUINFO_INT_DEFAULT_IRQ_VECTOR)
#define cpunum_endianness(cpunum)				cpunum_get_info_int(cpunum, CPUINFO_INT_ENDIANNESS)
#define cpunum_clock_divider(cpunum)			cpunum_get_info_int(cpunum, CPUINFO_INT_CLOCK_DIVIDER)
#define cpunum_min_instruction_bytes(cpunum)	cpunum_get_info_int(cpunum, CPUINFO_INT_MIN_INSTRUCTION_BYTES)
#define cpunum_max_instruction_bytes(cpunum)	cpunum_get_info_int(cpunum, CPUINFO_INT_MAX_INSTRUCTION_BYTES)
#define cpunum_min_cycles(cpunum)				cpunum_get_info_int(cpunum, CPUINFO_INT_MIN_CYCLES)
#define cpunum_max_cycles(cpunum)				cpunum_get_info_int(cpunum, CPUINFO_INT_MAX_CYCLES)
#define cpunum_databus_width(cpunum, space)		cpunum_get_info_int(cpunum, CPUINFO_INT_DATABUS_WIDTH + (space))
#define cpunum_addrbus_width(cpunum, space)		cpunum_get_info_int(cpunum, CPUINFO_INT_ADDRBUS_WIDTH + (space))
#define cpunum_addrbus_shift(cpunum, space)		cpunum_get_info_int(cpunum, CPUINFO_INT_ADDRBUS_SHIFT + (space))
#define cpunum_logaddr_width(cpunum, space)		cpunum_get_info_int(cpunum, CPUINFO_INT_LOGADDR_WIDTH + (space))
#define cpunum_page_shift(cpunum, space)		cpunum_get_info_int(cpunum, CPUINFO_INT_PAGE_SHIFT + (space))
#define cpunum_get_reg(cpunum, reg)				cpunum_get_info_int(cpunum, CPUINFO_INT_REGISTER + (reg))
#define cpunum_debug_register_list(cpunum)		cpunum_get_info_ptr(cpunum, CPUINFO_PTR_DEBUG_REGISTER_LIST)
#define cpunum_name(cpunum)						cpunum_get_info_string(cpunum, CPUINFO_STR_NAME)
#define cpunum_core_family(cpunum)				cpunum_get_info_string(cpunum, CPUINFO_STR_CORE_FAMILY)
#define cpunum_core_version(cpunum)				cpunum_get_info_string(cpunum, CPUINFO_STR_CORE_VERSION)
#define cpunum_core_file(cpunum)				cpunum_get_info_string(cpunum, CPUINFO_STR_CORE_FILE)
#define cpunum_core_credits(cpunum)				cpunum_get_info_string(cpunum, CPUINFO_STR_CORE_CREDITS)
#define cpunum_flags(cpunum)					cpunum_get_info_string(cpunum, CPUINFO_STR_FLAGS)
#define cpunum_irq_string(cpunum, irq)			cpunum_get_info_string(cpunum, CPUINFO_STR_IRQ_STATE + (irq))
#define cpunum_reg_string(cpunum, reg)			cpunum_get_info_string(cpunum, CPUINFO_STR_REGISTER + (reg))

#define cpunum_set_reg(cpunum, reg, val)		cpunum_set_info_int(cpunum, CPUINFO_INT_REGISTER + (reg), (val))



/***************************************************************************
    CPU TYPE ACCCESSORS
***************************************************************************/

/* get info accessors */
INT64 cputype_get_info_int(int cputype, UINT32 state);
void *cputype_get_info_ptr(int cputype, UINT32 state);
genf *cputype_get_info_fct(int cputype, UINT32 state);
const char *cputype_get_info_string(int cputype, UINT32 state);

#define cputype_context_size(cputype)			cputype_get_info_int(cputype, CPUINFO_INT_CONTEXT_SIZE)
#define cputype_input_lines(cputype)			cputype_get_info_int(cputype, CPUINFO_INT_INPUT_LINES)
#define cputype_output_lines(cputype)			cputype_get_info_int(cputype, CPUINFO_INT_OUTPUT_LINES)
#define cputype_default_irq_vector(cputype)		cputype_get_info_int(cputype, CPUINFO_INT_DEFAULT_IRQ_VECTOR)
#define cputype_endianness(cputype)				cputype_get_info_int(cputype, CPUINFO_INT_ENDIANNESS)
#define cputype_clock_divider(cputype)			cputype_get_info_int(cputype, CPUINFO_INT_CLOCK_DIVIDER)
#define cputype_min_instruction_bytes(cputype)	cputype_get_info_int(cputype, CPUINFO_INT_MIN_INSTRUCTION_BYTES)
#define cputype_max_instruction_bytes(cputype)	cputype_get_info_int(cputype, CPUINFO_INT_MAX_INSTRUCTION_BYTES)
#define cputype_min_cycles(cputype)				cputype_get_info_int(cputype, CPUINFO_INT_MIN_CYCLES)
#define cputype_max_cycles(cputype)				cputype_get_info_int(cputype, CPUINFO_INT_MAX_CYCLES)
#define cputype_databus_width(cputype, space)	cputype_get_info_int(cputype, CPUINFO_INT_DATABUS_WIDTH + (space))
#define cputype_addrbus_width(cputype, space)	cputype_get_info_int(cputype, CPUINFO_INT_ADDRBUS_WIDTH + (space))
#define cputype_addrbus_shift(cputype, space)	cputype_get_info_int(cputype, CPUINFO_INT_ADDRBUS_SHIFT + (space))
#define cputype_page_shift(cputype, space)		cputype_get_info_int(cputype, CPUINFO_INT_PAGE_SHIFT + (space))
#define cputype_debug_register_list(cputype)	cputype_get_info_ptr(cputype, CPUINFO_PTR_DEBUG_REGISTER_LIST)
#define cputype_name(cputype)					cputype_get_info_string(cputype, CPUINFO_STR_NAME)
#define cputype_core_family(cputype)			cputype_get_info_string(cputype, CPUINFO_STR_CORE_FAMILY)
#define cputype_core_version(cputype)			cputype_get_info_string(cputype, CPUINFO_STR_CORE_VERSION)
#define cputype_core_file(cputype)				cputype_get_info_string(cputype, CPUINFO_STR_CORE_FILE)
#define cputype_core_credits(cputype)			cputype_get_info_string(cputype, CPUINFO_STR_CORE_CREDITS)



/***************************************************************************
    MACROS
***************************************************************************/

#define		activecpu_get_previouspc()			((offs_t)activecpu_get_reg(REG_PREVIOUSPC))
#define		activecpu_get_pc()					((offs_t)activecpu_get_reg(REG_PC))
#define		activecpu_get_sp()					activecpu_get_reg(REG_SP)



/***************************************************************************
    CPU INTERFACE ACCESSORS
***************************************************************************/

/* return a pointer to the interface struct for a given CPU type */
INLINE const cpu_interface *cputype_get_interface(int cputype)
{
	extern cpu_interface cpuintrf[];
	return &cpuintrf[cputype];
}


/* return a the index of the active CPU */
INLINE int cpu_getactivecpu(void)
{
	extern int activecpu;
	return activecpu;
}


/* return a the index of the executing CPU */
INLINE int cpu_getexecutingcpu(void)
{
	extern int executingcpu;
	return executingcpu;
}


/* return a the total number of registered CPUs */
INLINE int cpu_gettotalcpu(void)
{
	extern int totalcpu;
	return totalcpu;
}


/* return the current PC or ~0 if no CPU is active */
INLINE offs_t safe_activecpu_get_pc(void)
{
	return (cpu_getactivecpu() >= 0) ? activecpu_get_pc() : ~0;
}


#endif	/* __CPUINTRF_H__ */
