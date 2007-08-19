/*

	Hitachi H8S/2xxx MCU

	(c) 2001-2007 Tim Schuerewegen

	H8S/2241
	H8S/2246
	H8S/2323

*/

#include "h8s2xxx.h"

#include "driver.h"
#include "debugger.h"

#include "h8sopcd.h"
#include "h8sdasm.h"

#ifdef MAME_DEBUG
#include "debug/debugcpu.h"
#endif

#define LOG_LEVEL  1
#define _logerror(level,...)  if (LOG_LEVEL > level) logerror(__VA_ARGS__)

//#define ENABLE_SCI_TIMER

///////////////
// MEMCONV.H //
///////////////

static TIMER_CALLBACK( h8s_tmr_callback );
static TIMER_CALLBACK( h8s_tpu_callback );
static TIMER_CALLBACK( h8s_sci_callback );

void h8s_tmr_init( void);
void h8s_tpu_init( void);
void h8s_sci_init( void);

///////////////
// MEMCONV.H //
///////////////

INLINE UINT16 read16be_with_read8_handler( read8_handler handler, offs_t offset, UINT16 mem_mask)
{
	UINT16 data = 0;
	if ((mem_mask & 0xff00) != 0xff00) data |= ((UINT16) handler(offset * 2 + 0)) << 8;
	if ((mem_mask & 0x00ff) != 0x00ff) data |= ((UINT16) handler(offset * 2 + 1)) << 0;
	return data;
}

INLINE void write16be_with_write8_handler( write8_handler handler, offs_t offset, UINT16 data, UINT16 mem_mask)
{
	if ((mem_mask & 0xff00) != 0xff00) handler(offset * 2 + 0, data >> 8);
	if ((mem_mask & 0x00ff) != 0x00ff) handler(offset * 2 + 1, data >> 0);
}

/////////////
// TIMER.C //
/////////////

static void mame_timer_adjust_pulse( mame_timer *timer, UINT32 hz, int num)
{
	mame_timer_adjust( timer, MAME_TIME_IN_HZ( hz), num, MAME_TIME_IN_HZ( hz));
}

/////////
// ... //
/////////

#define FLAG_C  0
#define FLAG_V  1
#define FLAG_Z  2
#define FLAG_N  3
#define FLAG_U  4
#define FLAG_H  5
#define FLAG_X  6 // UI
#define FLAG_I  7

#define H8S_REG_SP  7

#define H8S_GET_FLAG_C  cpu_get_flag( FLAG_C)
#define H8S_GET_FLAG_V  cpu_get_flag( FLAG_V)
#define H8S_GET_FLAG_Z  cpu_get_flag( FLAG_Z)
#define H8S_GET_FLAG_N  cpu_get_flag( FLAG_N)
#define H8S_GET_FLAG_U  cpu_get_flag( FLAG_U)
#define H8S_GET_FLAG_H  cpu_get_flag( FLAG_H)
#define H8S_GET_FLAG_X  cpu_get_flag( FLAG_X)
#define H8S_GET_FLAG_I  cpu_get_flag( FLAG_I)

#define H8S_SET_FLAG_C(x)  cpu_set_flag( FLAG_C, x)
#define H8S_SET_FLAG_V(x)  cpu_set_flag( FLAG_V, x)
#define H8S_SET_FLAG_Z(x)  cpu_set_flag( FLAG_Z, x)
#define H8S_SET_FLAG_N(x)  cpu_set_flag( FLAG_N, x)
#define H8S_SET_FLAG_U(x)  cpu_set_flag( FLAG_U, x)
#define H8S_SET_FLAG_H(x)  cpu_set_flag( FLAG_H, x)
#define H8S_SET_FLAG_X(x)  cpu_set_flag( FLAG_X, x)
#define H8S_SET_FLAG_I(x)  cpu_set_flag( FLAG_I, x)

/////////
// ... //
/////////

typedef struct
{
	UINT32 tgr, irq, out;
} H8S2XXX_TPU_ITEM;

typedef struct
{
	mame_timer *timer;
	H8S2XXX_TPU_ITEM item[8];
	int prescaler, tgrmax, tgrcur;
} H8S2XXX_TPU;

typedef struct
{
	mame_timer *timer;
	UINT32 bitrate;
} H8S2XXX_SCI;

typedef struct
{
	mame_timer *timer;
} H8S2XXX_TMR;

typedef struct
{
	int cpunum;
	UINT32 pc;
	PAIR reg[8];
	UINT8 flag[8];
	UINT8 onchip_reg[0x1C0];
	UINT32 irq_req[3];
	bool irq_pending;
	H8S2XXX_TMR tmr[2];
	H8S2XXX_TPU tpu[3];
#ifdef ENABLE_SCI_TIMER
	H8S2XXX_SCI sci[3];
#endif
} H8S2XXX;

static H8S2XXX h8s;

// instruction counter
static int cpu_icount;

// registers
enum { H8S_PC = 1, H8S_ER0, H8S_ER1, H8S_ER2, H8S_ER3, H8S_ER4, H8S_ER5, H8S_ER6, H8S_ER7 };

// external interrupt lines
enum { H8S_IRQ0 = 0, H8S_IRQ1, H8S_IRQ2, H8S_IRQ3, H8S_IRQ4, H8S_IRQ5, H8S_IRQ6, H8S_IRQ7 };

///////////////////
// MEMORY ACCESS //
///////////////////

#define H8S_GET_MEM_8(addr)   h8s_mem_read_8(addr)
#define H8S_GET_MEM_16(addr)  h8s_mem_read_16(addr)
#define H8S_GET_MEM_32(addr)  h8s_mem_read_32(addr)

#define H8S_SET_MEM_8(addr,data)   h8s_mem_write_8(addr,data)
#define H8S_SET_MEM_16(addr,data)  h8s_mem_write_16(addr,data)
#define H8S_SET_MEM_32(addr,data)  h8s_mem_write_32(addr,data)

INLINE UINT8 h8s_mem_read_8( UINT32 addr)
{
	return program_read_byte_16be( addr);
}

INLINE UINT16 h8s_mem_read_16( UINT32 addr)
{
	return program_read_word_16be( addr);
}

INLINE UINT32 h8s_mem_read_32( UINT32 addr)
{
	return (program_read_word_16be( addr + 0) << 16) | (program_read_word_16be( addr + 2) << 0);
}

INLINE void h8s_mem_write_8( UINT32 addr, UINT8 data)
{
	program_write_byte_16be( addr, data);
}

INLINE void h8s_mem_write_16( UINT32 addr, UINT16 data)
{
	program_write_word_16be( addr, data);
}

INLINE void h8s_mem_write_32( UINT32 addr, UINT32 data)
{
	program_write_word_16be( addr + 0, (data >> 16) & 0xFFFF);
	program_write_word_16be( addr + 2, (data >>  0) & 0xFFFF);
}

/////////////////////
// REGISTER ACCESS //
/////////////////////

#define H8S_GET_REG_8(reg)   h8s_get_reg_8(reg)
#define H8S_GET_REG_16(reg)  h8s_get_reg_16(reg)
#define H8S_GET_REG_32(reg)  h8s_get_reg_32(reg)

#define H8S_SET_REG_8(reg,data)   h8s_set_reg_8(reg,data)
#define H8S_SET_REG_16(reg,data)  h8s_set_reg_16(reg,data)
#define H8S_SET_REG_32(reg,data)  h8s_set_reg_32(reg,data)

#define H8S_GET_PC()      h8s_get_pc()
#define H8S_SET_PC(data)  h8s_set_pc(data)

#define H8S_GET_SP()      h8s_get_reg_32(H8S_REG_SP)
#define H8S_SET_SP(data)  h8s_set_reg_32(H8S_REG_SP,data)

static UINT32 h8s_get_reg_32( UINT8 reg)
{
	return h8s.reg[reg].d;
}

static void h8s_set_reg_32( UINT8 reg, UINT32 data)
{
	h8s.reg[reg].d = data;
}

static UINT16 h8s_get_reg_16( UINT8 reg)
{
	if (reg < 8) return h8s.reg[reg].w.l; else return h8s.reg[reg-8].w.h;
}

static void h8s_set_reg_16( UINT8 reg, UINT16 data)
{
	if (reg < 8) h8s.reg[reg].w.l = data; else h8s.reg[reg-8].w.h = data;
}

static UINT8 h8s_get_reg_8( UINT8 reg)
{
	if (reg < 8) return h8s.reg[reg].b.h; else return h8s.reg[reg-8].b.l;
}

static void h8s_set_reg_8( UINT8 reg, UINT8 data)
{
	if (reg < 8) h8s.reg[reg].b.h = data; else h8s.reg[reg-8].b.l = data;
}

static UINT32 h8s_get_pc( void)
{
	return h8s.pc;
}

static void h8s_set_pc( UINT32 pc)
{
	h8s.pc = pc;
	change_pc( pc);
}

/////////////////
// FLAG ACCESS //
/////////////////

static int cpu_get_flag( int flag)
{
	return h8s.flag[flag];
}

static void cpu_set_flag( int flag, int value)
{
	h8s.flag[flag] = value;
}

/////////
// ... //
/////////

static void cpu_halt( const char *text)
{
	fatalerror( "halt! %s (PC=%08X)", text, h8s.pc);
}

/*
static void cpu_cycles( int i, int j, int k, int l, int m, int n)
{
	// assume external device, 8-bit bus, 2-state access
	cpu_icount -= i * 4 + j * 4 + k * 4 + l * 2 + m * 4 + n * 1;
}
*/

/////////////////////
// REGISTER LAYOUT //
/////////////////////

/*
static UINT8 h8s2xxx_reg_layout[] =
{
	H8S_PC, H8S_ER0, H8S_ER1, H8S_ER2, H8S_ER3, H8S_ER4, H8S_ER5, H8S_ER6, H8S_ER7, 0
};
*/

/////////////////////////
// DEBUG REGISTER LIST //
/////////////////////////

/*
static int h8s2xxx_debug_register_list[] =
{
	H8S_PC, H8S_ER0, H8S_ER1, H8S_ER2, H8S_ER3, H8S_ER4, H8S_ER5, H8S_ER6, H8S_ER7, -1
};
*/

///////////////////
// WINDOW LAYOUT //
///////////////////

/*
static UINT8 h8s2xxx_win_layout[] =
{
	27,  0, 53,  4,	// register window (top rows)
	 0,  0, 26, 22,	// disassembler window (left colums)
	27,  5, 53,  8,	// memory #1 window (right, upper middle)
	27, 14, 53,  8,	// memory #2 window (right, lower middle)
	 0, 23, 80,  1, // command line window (bottom rows)
};
*/

////////////
// DTVECR //
////////////

#define DTVECR_ADDR(x) (0x400 + (x << 1))

/////////
// ... //
/////////

void h8s2xxx_interrupt_request( UINT32 vecnum)
{
	UINT8 idx, bit;
//	logerror( "h8s2xxx_interrupt_request (%d)\n", vecnum);
	idx = vecnum >> 5;
	bit = vecnum & 0x1F;
	h8s.irq_req[idx] |= (1 << bit);
	h8s.irq_pending = TRUE;
//	logerror( "irq_req = %08X %08X %08X\n", h8s.irq_req[0], h8s.irq_req[1], h8s.irq_req[2]);
}

void h8s_dtce_execute( UINT32 addr_dtce, UINT8 mask_dtce, UINT32 addr_dtvecr)
{
	UINT32 data[3], dtc_vect, dtc_sar, dtc_dar, cnt, i;
	UINT8 dtc_mra, dtc_mrb, sz;
	UINT16 dtc_cra, dtc_crb;
	// get dtc info
	dtc_vect  = 0xFF0000 | h8s_mem_read_16( addr_dtvecr);
	data[0]   = h8s_mem_read_32( dtc_vect + 0);
	data[1]   = h8s_mem_read_32( dtc_vect + 4);
	data[2]   = h8s_mem_read_32( dtc_vect + 8);
	dtc_mra   = (data[0] >> 24) & 0xFF;
	dtc_sar   = (data[0] >>  0) & 0xFFFFFF;
	dtc_mrb   = (data[1] >> 24) & 0xFF;
	dtc_dar   = (data[1] >>  0) & 0xFFFFFF;
	dtc_cra   = (data[2] >> 16) & 0xFFFF;
	dtc_crb   = (data[2] >>  0) & 0xFFFF;
	_logerror( 3, "dtc : vect %08X mra %02X sar %08X mrb %02X dar %08X cra %04X crb %04X\n", dtc_vect, dtc_mra, dtc_sar, dtc_mrb, dtc_dar, dtc_cra, dtc_crb);
	// execute
	if ((dtc_mra & 0x0E) != 0x00) cpu_halt( "todo: dtc unsupported");
	sz = 1 << (dtc_mra & 0x01);
	cnt = dtc_cra;
	for (i=0;i<cnt;i++)
	{
		if (dtc_sar == H8S_IO_ADDR( H8S_IO_RDR1)) h8s_mem_write_8( H8S_IO_ADDR( H8S_IO_SSR1), h8s_mem_read_8( H8S_IO_ADDR( H8S_IO_SSR1)) & (~H8S_SSR_TDRE));
		if (dtc_mra & 0x01) h8s_mem_write_16( dtc_dar, h8s_mem_read_16( dtc_sar)); else h8s_mem_write_8( dtc_dar, h8s_mem_read_8( dtc_sar));
		if (dtc_dar == H8S_IO_ADDR( H8S_IO_TDR0)) h8s_mem_write_8( H8S_IO_ADDR( H8S_IO_SSR0), h8s_mem_read_8( H8S_IO_ADDR( H8S_IO_SSR0)) & (~H8S_SSR_TDRE));
		if (dtc_mra & 0x80) { if (dtc_mra & 0x40) dtc_sar -= sz; else dtc_sar += sz; }
		if (dtc_mra & 0x20) { if (dtc_mra & 0x10) dtc_dar -= sz; else dtc_dar += sz; }
	}
	h8s_mem_write_8( addr_dtce, h8s_mem_read_8( addr_dtce) & (~mask_dtce));
}

void h8s_dtce_check( int vecnum)
{
	UINT32 dtce = 0;
	int bit = 0;
	// get dtce info
	switch (vecnum)
	{
		// DTCEA
		case H8S_INT_IRQ0  : dtce = H8S_IO_DTCEA; bit = 7; break;
		case H8S_INT_IRQ1  : dtce = H8S_IO_DTCEA; bit = 6; break;
		case H8S_INT_IRQ2  : dtce = H8S_IO_DTCEA; bit = 5; break;
		case H8S_INT_IRQ3  : dtce = H8S_IO_DTCEA; bit = 4; break;
		case H8S_INT_IRQ4  : dtce = H8S_IO_DTCEA; bit = 3; break;
		case H8S_INT_IRQ5  : dtce = H8S_IO_DTCEA; bit = 2; break;
		case H8S_INT_IRQ6  : dtce = H8S_IO_DTCEA; bit = 1; break;
		case H8S_INT_IRQ7  : dtce = H8S_IO_DTCEA; bit = 0; break;
		// DTCEB
		case H8S_INT_ADI   : dtce = H8S_IO_DTCEB; bit = 6; break;
		case H8S_INT_TGI0A : dtce = H8S_IO_DTCEB; bit = 5; break;
		case H8S_INT_TGI0B : dtce = H8S_IO_DTCEB; bit = 4; break;
		case H8S_INT_TGI0C : dtce = H8S_IO_DTCEB; bit = 3; break;
		case H8S_INT_TGI0D : dtce = H8S_IO_DTCEB; bit = 2; break;
		case H8S_INT_TGI1A : dtce = H8S_IO_DTCEB; bit = 1; break;
		case H8S_INT_TGI1B : dtce = H8S_IO_DTCEB; bit = 0; break;
		// DTCEC
		case H8S_INT_TGI2A : dtce = H8S_IO_DTCEC; bit = 7; break;
		case H8S_INT_TGI2B : dtce = H8S_IO_DTCEC; bit = 6; break;
		// DTCED
		case H8S_INT_CMIA0 : dtce = H8S_IO_DTCED; bit = 3; break;
		case H8S_INT_CMIB0 : dtce = H8S_IO_DTCED; bit = 2; break;
		case H8S_INT_CMIA1 : dtce = H8S_IO_DTCED; bit = 1; break;
		case H8S_INT_CMIB1 : dtce = H8S_IO_DTCED; bit = 0; break;
		// DTCEE
		case H8S_INT_RXI0  : dtce = H8S_IO_DTCEE; bit = 3; break;
		case H8S_INT_TXI0  : dtce = H8S_IO_DTCEE; bit = 2; break;
		case H8S_INT_RXI1  : dtce = H8S_IO_DTCEE; bit = 1; break;
		case H8S_INT_TXI1  : dtce = H8S_IO_DTCEE; bit = 0; break;
		// DTCEF
		case H8S_INT_RXI2  : dtce = H8S_IO_DTCEF; bit = 7; break;
		case H8S_INT_TXI2  : dtce = H8S_IO_DTCEF; bit = 6; break;
	}
	// execute
	if ((dtce != 0) && (h8s.onchip_reg[dtce] & (1 << bit))) h8s_dtce_execute( H8S_IO_ADDR( dtce), (1 << bit), DTVECR_ADDR( vecnum));
}

#include "h8sexec.c"

void h8s2xxx_interrupt_check( void)
{
	int i, j, vecnum = -1;
	if (h8s.flag[FLAG_I]) return;
	for (i=0;i<3;i++)
	{
		if (h8s.irq_req[i] != 0)
		{
			for (j=0;j<32;j++)
			{
				if (h8s.irq_req[i] & (1 << j))
				{
					vecnum = i * 32 + j;
					h8s.irq_req[i] &= ~(1 << j);
					break;
				}
			}
		}
		if (vecnum != -1) break;
	}
	h8s.irq_pending = ((h8s.irq_req[0] != 0) || (h8s.irq_req[1] != 0) || (h8s.irq_req[2] != 0));

//	logerror( "irq_req = %08X %08X %08X\n", h8s.irq_req[0], h8s.irq_req[1], h8s.irq_req[2]);

	h8s_dtce_check( vecnum);

	h8s_exec_interrupt( vecnum);
}

void h8s2xxx_get_context( void *context)
{
	_logerror( 0, "h8s2xxx_get_context\n");
	memcpy( context, &h8s, sizeof( h8s));
}

void h8s2xxx_set_context( void *context)
{
	_logerror( 0, "h8s2xxx_set_context\n");
	memcpy( &h8s, context, sizeof( h8s));
}

void h8s2xxx_state_save( int cpu)
{
	const char *name, *names[] = { "unknown", "h8s2241", "h8s2246", "h8s2323" };
	switch (Machine->drv->cpu[cpu].cpu_type)
	{
		case CPU_H8S2241 : name = names[1]; break;
		case CPU_H8S2246 : name = names[2]; break;
		case CPU_H8S2323 : name = names[3]; break;
		default          : name = names[0]; break;
	}
	state_save_register_item( name, cpu, h8s.pc);
	state_save_register_item_array( name, cpu, h8s.reg);
	state_save_register_item_array( name, cpu, h8s.flag);
}

void h8s2xxx_init( int index, int clock, const void *config, int (*irqcallback)(int))
{
	_logerror( 0, "h8s2xxx_init (%d/%d)\n", index, clock);
	//
	memset( &h8s, 0, sizeof( h8s));
	h8s.cpunum = index;
	// ...
	h8s_opcode_init();
	h8s_tmr_init();
	h8s_tpu_init();
	h8s_sci_init();
	// state save
	h8s2xxx_state_save( index);
}

void h8s2xxx_exit( void)
{
	_logerror( 0, "h8s2xxx_exit\n");
	h8s_opcode_exit();
}

void h8s2xxx_reset( void)
{
	_logerror( 0, "h8s2xxx_reset\n");
	// loads cpu program counter from vector table
	H8S_SET_PC( h8s_mem_read_32( 0));
	// clear trace bit in exr to 0
	// -> todo
	// set interrupt mask bits in ccr and exr to 1 (todo)
	// -> todo
	// other ccr bits and general registers are not initialized (todo)
	// -> todo
	// in particular, the stack pointer (ER7) is not initialized (todo)
	// -> todo
	// initial values for onchip registers (todo: add more)
	h8s.onchip_reg[H8S_IO_SSR0] = 0x84;
	h8s.onchip_reg[H8S_IO_SSR1] = 0x84;
	h8s.onchip_reg[H8S_IO_SSR2] = 0x84;
	h8s.onchip_reg[H8S_IO_TDR0] = 0xFF;
	h8s.onchip_reg[H8S_IO_TDR1] = 0xFF;
	h8s.onchip_reg[H8S_IO_TDR2] = 0xFF;
	h8s.onchip_reg[H8S_IO_BRR0] = 0xFF;
	h8s.onchip_reg[H8S_IO_BRR1] = 0xFF;
	h8s.onchip_reg[H8S_IO_BRR2] = 0xFF;
}

int h8s2xxx_execute( int cycles)
{
	UINT32 index;
	UINT8 oprom[10];
	UINT16 opcode;
	// debug
	_logerror( 1, "h8s2xxx_execute (%d)\n", cycles);
	// todo
	cpu_icount = cycles;
	while (cpu_icount > 0)
	{
		CALL_MAME_DEBUG;

//		logerror( "%08X - ER0=%08X ER1=%08X ER2=%08X ER3=%08X ER4=%08X ER5=%08X ER6=%08X ER7=%08X\n", h8s.pc, h8s.regs[0], h8s.regs[1], h8s.regs[2], h8s.regs[3], h8s.regs[4], h8s.regs[5], h8s.regs[6], h8s.regs[7]);

		if (h8s.irq_pending) h8s2xxx_interrupt_check();

		opcode = cpu_readop16( h8s.pc);
		oprom[0] = (opcode >> 8) & 0xFF;
		oprom[1] = (opcode >> 0) & 0xFF;

		index = h8s_opcode_find( h8s.pc, oprom, sizeof( opcode));

//		logerror( "%08X - %02X %02X %02X %02X ...\n", h8s.pc, oprom[0], oprom[1], oprom[2], oprom[3]);

		h8s_exec_instr( index, oprom);

		h8s.pc += h8s_instr_table[index].size;
	}

	return cycles - cpu_icount;
}

//////////////
// NOT USED //
//////////////

void h8s2xxx_burn( int cycles)
{
	_logerror( 0, "h8s2xxx_burn\n");
}

int h8s2xxx_translate( int space, offs_t *address)
{
	_logerror( 0, "h8s2xxx_translate\n");
	return 1;
}

int h8s2xxx_read( int space, UINT32 offset, int size, UINT64 *value)
{
	_logerror( 0, "h8s2xxx_read\n");
	return 0;
}

int h8s2xxx_write( int space, UINT32 offset, int size, UINT64 value)
{
	_logerror( 0, "h8s2xxx_write\n");
	return 0;
}

int h8s2xxx_readop( UINT32 offset, int size, UINT64 *value)
{
	_logerror( 0, "h8s2xxx_readop\n");
	*value = 0;
	while (size-- > 0) *value = (*value << 8) | h8s_mem_read_8( offset++);
	return 1;
}

#ifdef MAME_DEBUG
UINT64 h8s2xxx_debug_test( UINT32 ref, UINT32 params, UINT64 *param)
{
	return 0;
}
#endif

#ifdef MAME_DEBUG
void h8s2xxx_setup_commands( void)
{
	_logerror( 0, "h8s2xxx_setup_commands\n");
	symtable_add_function( global_symtable, "test", 0, 1, 1, h8s2xxx_debug_test);
}
#endif

///////////
// TIMER //
///////////

typedef struct
{
	UINT32 reg_tcsr, reg_tcr, reg_tcnt;
	UINT8 int_ovi;
} H8S_TMR_ENTRY;

const H8S_TMR_ENTRY H8S_TMR_TABLE[] =
{
	// TMR 0
	{
		H8S_IO_TCSR0, H8S_IO_TCR0, H8S_IO_TCNT0,
		H8S_INT_OVI0
	},
	// TMR 1
	{
		H8S_IO_TCSR1, H8S_IO_TCR1, H8S_IO_TCNT1,
		H8S_INT_OVI1
	}
};

const UINT32 TCR_CKS[] = { 0, 8, 64, 8192 };

const H8S_TMR_ENTRY *h8s_tmr_entry( int num)
{
	return &H8S_TMR_TABLE[num];
}

void h8s_tmr_init( void)
{
	int i;
	for (i=0;i<2;i++)
	{
		h8s.tmr[i].timer = mame_timer_alloc( h8s_tmr_callback);
		mame_timer_adjust( h8s.tmr[i].timer, time_never, i, time_zero);
	}
}

static TIMER_CALLBACK( h8s_tmr_callback)
{
	const H8S_TMR_ENTRY *info = h8s_tmr_entry( param);
	_logerror( 2, "h8s_tmr_callback (%d)\n", param);
	// disable timer
	mame_timer_adjust( h8s.tmr[param].timer, time_never, param, time_zero);
	// set timer overflow flag
	h8s.onchip_reg[info->reg_tcsr] |= 0x20;
	// overflow interrupt
	if (h8s.onchip_reg[info->reg_tcr] & 0x20) h8s2xxx_interrupt_request( info->int_ovi);
}

void h8s_tmr_resync( int num)
{
	const H8S_TMR_ENTRY *info = h8s_tmr_entry( num);
	UINT8 cks, tcr, tcnt;
	// get timer data
	tcr = h8s.onchip_reg[info->reg_tcr];
	tcnt = h8s.onchip_reg[info->reg_tcnt];
	//
	cks = tcr & 3;
	if (cks != 0)
	{
		UINT32 cycles, hz;
		cycles = TCR_CKS[cks] * (0xFF - tcnt);
		if (cycles == 0) cycles = 1; // cybiko game "lost in labyrinth" => divide by zero
		hz = MAME_TIME_TO_CYCLES( h8s.cpunum, MAME_TIME_IN_SEC( 1)) / cycles;
		_logerror( 2, "cycles %d hz %d\n", cycles, hz);
		mame_timer_adjust( h8s.tmr[num].timer, MAME_TIME_IN_HZ( hz), num, time_zero);
	}
	else
	{
		_logerror( 2, "time never\n");
		mame_timer_adjust( h8s.tmr[num].timer, time_never, num, time_zero);
	}
}

/////////
// TPU //
/////////

void h8s_tpu_init( void)
{
	int i;
	for (i=0;i<3;i++)
	{
		h8s.tpu[i].timer = mame_timer_alloc( h8s_tpu_callback);
		mame_timer_adjust( h8s.tpu[i].timer, time_never, i, time_zero);
	}
}

const UINT32 TPSC[] = { 1, 4, 16, 64, 0, 0, 256, 0 };

void set_p1dr_tiocb1( int data)
{
	UINT8 p1dr;
	p1dr = h8s.onchip_reg[H8S_IO_P1DR];
	if (data) p1dr |= H8S_P1_TIOCB1; else p1dr &= ~H8S_P1_TIOCB1;
	io_write_byte_8( H8S_IO_ADDR( H8S_IO_P1DR), p1dr);
	h8s.onchip_reg[H8S_IO_P1DR] = p1dr;
}

void h8s_tpu_start( int num)
{
	UINT8 ttcr, tior, tier, tsr;
	UINT16 tgra, tgrb, tcnt;
	UINT32 hz, i, cycles;
	H8S2XXX_TPU *tpu = &h8s.tpu[num];
	switch (num)
	{
		case 1 :
		{
			ttcr = h8s.onchip_reg[H8S_IO_TTCR1];
			tior = h8s.onchip_reg[H8S_IO_TIOR1];
			tier = h8s.onchip_reg[H8S_IO_TIER1];
			tsr  = h8s.onchip_reg[H8S_IO_TSR1];
			tgra = (h8s.onchip_reg[H8S_IO_TGR1A+0] << 8) | (h8s.onchip_reg[H8S_IO_TGR1A+1] << 0);
			tgrb = (h8s.onchip_reg[H8S_IO_TGR1B+0] << 8) | (h8s.onchip_reg[H8S_IO_TGR1B+1] << 0);
			tcnt = (h8s.onchip_reg[H8S_IO_TTCNT1+0] << 8) | (h8s.onchip_reg[H8S_IO_TTCNT1+1] << 0);

			//printf( "TTCR %02X TIOR %02X TIER %02X TSR %02X TGRA %04X TGRB %04X TCNT %04X\n", ttcr, tior, tier, tsr, tgra, tgrb, tcnt);

			if ((ttcr != 0x21) || (tior != 0x56) || ((tier != 0x00) && (tier != 0x01)) || (tsr != 0x00))
			{
				fatalerror( "TPU: not supported\n");
			}

			// start
			i = 0;
			tpu->item[i].tgr = 0;
			tpu->item[i].out = (i + 1) & 1;
			tpu->item[i].irq = 0;
			// TGRB
			if (tgrb < tgra) i = 1; else i = 2;
			tpu->item[i].tgr = tgrb;
			tpu->item[i].out = (i + 1) & 1;
			if (tier & 2) tpu->item[i].irq = H8S_INT_TGI1B; else tpu->item[i].irq = 0;
			// TGRA
			if (tgrb < tgra) i = 2; else i = 1;
			tpu->item[i].tgr = tgra;
			tpu->item[i].out = (i + 1) & 1;
			if (tier & 1) tpu->item[i].irq = H8S_INT_TGI1A; else tpu->item[i].irq = 0;
			// ...
			tpu->tgrcur = 0;
			tpu->tgrmax = 2;
			tpu->prescaler = TPSC[ttcr & 7];
			cycles = tpu->item[1].tgr - tpu->item[0].tgr;
			// ajust timer
			hz = MAME_TIME_TO_CYCLES( h8s.cpunum, MAME_TIME_IN_SEC( 1)) / (tpu->prescaler * cycles);
			mame_timer_adjust( tpu->timer, MAME_TIME_IN_HZ( hz), num, time_zero);
			// output
			set_p1dr_tiocb1( tpu->item[0].out);
			//
			tpu->tgrcur++;
		}
	}
}

void h8s_tpu_stop( int num)
{
	mame_timer_adjust( h8s.tpu[num].timer, time_never, num, time_zero);
}

void h8s_tpu_resync( UINT8 data)
{
	if (data & 1) h8s_tpu_start( 0); else h8s_tpu_stop( 0);
	if (data & 2) h8s_tpu_start( 1); else h8s_tpu_stop( 1);
	if (data & 4) h8s_tpu_start( 2); else h8s_tpu_stop( 2);
}

static TIMER_CALLBACK( h8s_tpu_callback)
{
	H8S2XXX_TPU *tpu = &h8s.tpu[param];
	_logerror( 2, "h8s_tpu_callback (%d)\n", param);
	switch (param)
	{
		case 1 :
		{
			UINT32 hz, cycles;
			int tiocb1, vecnum;
			// get info
			vecnum = tpu->item[tpu->tgrcur].irq;
			tiocb1 = tpu->item[tpu->tgrcur].out;
			if (tpu->tgrcur >= tpu->tgrmax) tpu->tgrcur = 0;
			cycles = tpu->item[tpu->tgrcur+1].tgr - tpu->item[tpu->tgrcur+0].tgr;
			tpu->tgrcur++;
			// adjust timer
			hz = MAME_TIME_TO_CYCLES( h8s.cpunum, MAME_TIME_IN_SEC( 1)) / (tpu->prescaler * cycles);
			mame_timer_adjust( tpu->timer, MAME_TIME_IN_HZ( hz), 1, time_zero);
			// output
			set_p1dr_tiocb1( tiocb1);
			// interrupt
			if (vecnum != 0) h8s2xxx_interrupt_request( vecnum);
		}
		break;
	}
}

/////////////////////////////////
// SERIAL CONTROLLER INTERFACE //
/////////////////////////////////

typedef struct
{
	UINT32 reg_smr, reg_brr, reg_scr, reg_tdr, reg_ssr, reg_rdr;
	UINT32 reg_pdr, reg_port;
	UINT8 port_mask_sck, port_mask_txd, port_mask_rxd;
	UINT8 int_tx, int_rx;
} H8S_SCI_ENTRY;

const H8S_SCI_ENTRY H8S_SCI_TABLE[] =
{
	// SCI 0
	{
		H8S_IO_SMR0, H8S_IO_BRR0, H8S_IO_SCR0, H8S_IO_TDR0, H8S_IO_SSR0, H8S_IO_RDR0,
		H8S_IO_P3DR, H8S_IO_PORT3,
		H8S_P3_SCK0, H8S_P3_TXD0, H8S_P3_RXD0,
		H8S_INT_TXI0, H8S_INT_RXI0
	},
	// SCI 1
	{
		H8S_IO_SMR1, H8S_IO_BRR1, H8S_IO_SCR1, H8S_IO_TDR1, H8S_IO_SSR1, H8S_IO_RDR1,
		H8S_IO_P3DR, H8S_IO_PORT3,
		H8S_P3_SCK1, H8S_P3_TXD1, H8S_P3_RXD1,
		H8S_INT_TXI1, H8S_INT_RXI1
	},
	// SCI 2
	{
		H8S_IO_SMR2, H8S_IO_BRR2, H8S_IO_SCR2, H8S_IO_TDR2, H8S_IO_SSR2, H8S_IO_RDR2,
		H8S_IO_P5DR, H8S_IO_PORT5,
		H8S_P5_SCK2, H8S_P5_TXD2, H8S_P5_RXD2,
		H8S_INT_TXI2, H8S_INT_RXI2
	}
};

const H8S_SCI_ENTRY *h8s_sci_entry( int num)
{
	return &H8S_SCI_TABLE[num];
}

void h8s_sci_init( void)
{
	int i;
	for (i=0;i<3;i++)
	{
		#ifdef ENABLE_SCI_TIMER
		h8s.sci[i].timer = mame_timer_alloc( h8s_sci_callback);
		mame_timer_adjust( h8s.sci[i].timer, time_never, i, time_zero);
		#endif
	}
}

static TIMER_CALLBACK( h8s_sci_callback)
{
	_logerror( 2, "h8s_sci_callback (%d)\n", param);
}

void h8s_sci_start( int num)
{
	#ifdef ENABLE_SCI_TIMER
	mame_timer_adjust_pulse( h8s.sci[num].timer, h8s.sci[num].bitrate, num);
	#endif
}

void h8s_sci_stop( int num)
{
	#ifdef ENABLE_SCI_TIMER
	mame_timer_adjust( h8s.sci[num].timer, time_never, num, time_zero);
	#endif
}

void h8s_sci_execute( int num)
{
	UINT8 scr, tdr, ssr, rdr, tsr, rsr, pdr, port;
	int i;
	const H8S_SCI_ENTRY *info = h8s_sci_entry( num);
//	logerror( "h8s_sci_execute(%d)\n", num);
	// load regs
	scr = h8s.onchip_reg[info->reg_scr];
	tdr = h8s.onchip_reg[info->reg_tdr];
	ssr = h8s.onchip_reg[info->reg_ssr];
	rdr = h8s.onchip_reg[info->reg_rdr];
	tsr = 0;
	rsr = 0;
	pdr = h8s.onchip_reg[info->reg_pdr] & (~info->port_mask_sck);
	// move byte from TDR to TSR
	if (scr & H8S_SCR_TE)
	{
		tsr = tdr;
		ssr |= H8S_SSR_TDRE;
	}
	// generate transmit data empty interrupt
	if ((scr & H8S_SCR_TIE) && (ssr & H8S_SSR_TDRE)) h8s2xxx_interrupt_request( info->int_tx);
	// transmit/receive bits
	for (i=0;i<8;i++)
	{
		// write bit
		if (scr & H8S_SCR_TE)
		{
			if (tsr & (1 << i)) pdr = pdr | info->port_mask_txd; else pdr = pdr & (~info->port_mask_txd);
			io_write_byte_8( H8S_IO_ADDR( info->reg_pdr), pdr);
		}
		// clock high to low
		io_write_byte_8( H8S_IO_ADDR( info->reg_pdr), pdr | info->port_mask_sck);
		io_write_byte_8( H8S_IO_ADDR( info->reg_pdr), pdr);
		// read bit
		if (scr & H8S_SCR_RE)
		{
			port = io_read_byte_8( H8S_IO_ADDR( info->reg_port));
			if (port & info->port_mask_rxd) rsr = rsr | (1 << i);
		}
	}
	// move byte from RSR to RDR
	if (scr & H8S_SCR_RE)
	{
		rdr = rsr;
		//ssr |= H8S_SSR_RDRF;
	}
	// generate receive data full interrupt
	if ((scr & H8S_SCR_RIE) && (ssr & H8S_SSR_RDRF)) h8s2xxx_interrupt_request( info->int_rx);
	// save regs
	h8s.onchip_reg[info->reg_scr] = scr;
	h8s.onchip_reg[info->reg_tdr] = tdr;
	h8s.onchip_reg[info->reg_ssr] = ssr;
	h8s.onchip_reg[info->reg_rdr] = rdr;
}

const double SCR_CKE[] = { 0.5, 2, 8, 32 }; // = 2 ^ ((2 * cke) - 1)
const int SMR_MODE[] = { 64, 8 };

void h8s_sci_rate( int num)
{
	UINT8 brr, scr, smr, cke, mode;
	UINT32 bitrate;
	const H8S_SCI_ENTRY *info = h8s_sci_entry( num);
	// read regs
	brr = h8s.onchip_reg[info->reg_brr];
	scr = h8s.onchip_reg[info->reg_scr];
	smr = h8s.onchip_reg[info->reg_smr];
	_logerror( 2, "BRR %02X SCR %02X SMR %02X\n", brr, scr, smr);
	// calculate bitrate
	cke = (scr >> 0) & 3;
	mode = (smr >> 7) & 1;
	bitrate = (UINT32)((MAME_TIME_TO_CYCLES( h8s.cpunum, MAME_TIME_IN_SEC( 1)) / (brr + 1)) / (SMR_MODE[mode] * SCR_CKE[cke]));
	_logerror( 2, "SCI%d bitrate %d\n", num, bitrate);
	// store bitrate
	#ifdef ENABLE_SCI_TIMER
	h8s.sci[num].bitrate = bitrate;
	#endif
}

////////////////////////////
// INTERNAL I/O REGISTERS //
////////////////////////////

WRITE8_HANDLER( h8s2xxx_onchip_reg_w_byte )
{
	h8s.onchip_reg[offset] = data;
	switch (offset)
	{
		// SCI 0
		case H8S_IO_SSR0  : if ((data & H8S_SSR_TDRE) == 0) h8s_sci_execute( 0); break;
		case H8S_IO_SCR0  : if (data & H8S_SCR_TIE) h8s2xxx_interrupt_request( h8s_sci_entry(0)->int_tx); break;
		case H8S_IO_BRR0  : h8s_sci_rate( 0); h8s_sci_start( 0); break;
		// SCI 1
		case H8S_IO_SSR1  : if ((data & H8S_SSR_TDRE) == 0) h8s_sci_execute( 1); break;
		case H8S_IO_SCR1  :	if (data & H8S_SCR_RIE) h8s2xxx_interrupt_request( h8s_sci_entry(1)->int_rx); break;
		case H8S_IO_BRR1  : h8s_sci_rate( 1); h8s_sci_start( 1); break;
		// SCI 2
		case H8S_IO_SSR2  : if ((data & H8S_SSR_TDRE) == 0) h8s_sci_execute( 2); break;
		case H8S_IO_SCR2  : if (data & H8S_SCR_TIE) h8s2xxx_interrupt_request( h8s_sci_entry(2)->int_tx); break;
		case H8S_IO_BRR2  : h8s_sci_rate( 2); h8s_sci_start( 2); break;
		// TMR 0 (8-bit)
		case H8S_IO_TCR0  : h8s_tmr_resync( 0); break;
		case H8S_IO_TCNT0 : h8s_tmr_resync( 0); break;
		// TMR 1
		case H8S_IO_TCR1  : h8s_tmr_resync( 1); break;
		case H8S_IO_TCNT1 : h8s_tmr_resync( 1); break;
		// ports
		case H8S_IO_P3DR  : io_write_byte_8( H8S_IO_ADDR( offset), data); break;
		case H8S_IO_PFDR  : io_write_byte_8( H8S_IO_ADDR( offset), data); break;
		case H8S_IO_PFDDR : io_write_byte_8( H8S_IO_ADDR( offset), data); break;
		// TPU
		case H8S_IO_TSTR  : h8s_tpu_resync( data); break;
	}
}

READ8_HANDLER( h8s2xxx_onchip_reg_r_byte )
{
	UINT8 data;
	switch (offset)
	{
		// SCI 0
		case H8S_IO_SSR0  : data = H8S_SSR_TDRE | H8S_SSR_TEND; break;
		case H8S_IO_RDR0  : data = io_read_byte_8( H8S_IO_ADDR( offset)); break;
		// SCI 1
		case H8S_IO_SSR1  : data = H8S_SSR_TDRE | H8S_SSR_TEND; break;
		// SCI 2
		case H8S_IO_SSR2 :
		{
			data = h8s.onchip_reg[offset];
			if (!(h8s.onchip_reg[H8S_IO_SCR2] & H8S_SCR_TE)) data |= H8S_SSR_TDRE;
		}
		break;
		// ports
		case H8S_IO_PORT1 : data = io_read_byte_8( H8S_IO_ADDR( offset)); break;
		case H8S_IO_PORTF : data = io_read_byte_8( H8S_IO_ADDR( offset)); break;
		case H8S_IO_P3DR  : data = 0; break; // todo: without this cybiko hangs
		// default
		default : data = h8s.onchip_reg[offset]; break;
	}
	return data;
}

WRITE8_HANDLER( h8s2241_onchip_reg_w_byte )
{
	_logerror( 3, "[%08X] h8s2241_onchip_reg_w_byte (%08X/%02X)\n", H8S_GET_PC(), H8S_IO_ADDR( offset), data);
	h8s2xxx_onchip_reg_w_byte( offset, data);
}

READ8_HANDLER( h8s2241_onchip_reg_r_byte )
{
	UINT8 data;
	_logerror( 3, "[%08X] h8s2241_onchip_reg_r_byte (%08X)\n", H8S_GET_PC(), H8S_IO_ADDR( offset));
	data = h8s2xxx_onchip_reg_r_byte( offset);
	_logerror( 3, "%02X\n", data);
	return data;
}

WRITE8_HANDLER( h8s2246_onchip_reg_w_byte )
{
	_logerror( 3, "[%08X] h8s2246_onchip_reg_w_byte (%08X/%02X)\n", H8S_GET_PC(), H8S_IO_ADDR( offset), data);
	h8s2xxx_onchip_reg_w_byte( offset, data);
}

READ8_HANDLER( h8s2246_onchip_reg_r_byte )
{
	UINT8 data;
	_logerror( 3, "[%08X] h8s2246_onchip_reg_r_byte (%08X)\n", H8S_GET_PC(), H8S_IO_ADDR( offset));
	data = h8s2xxx_onchip_reg_r_byte( offset);
	_logerror( 3, "%02X\n", data);
	return data;
}

WRITE8_HANDLER( h8s2323_onchip_reg_w_byte )
{
	_logerror( 3, "[%08X] h8s2323_onchip_reg_w_byte (%08X/%02X)\n", H8S_GET_PC(), H8S_IO_ADDR( offset), data);
	h8s2xxx_onchip_reg_w_byte( offset, data);
	switch (offset)
	{
		case H8S_IO_DMABCRL :
		{
			if ((data & 0x40) && (data & 0x80))
			{
				UINT32 i, dma_src, dma_dst;
				UINT16 dma_cnt, dma_con;
				int sz;
				dma_src = h8s_mem_read_32( H8S_IO_ADDR( H8S_IO_MAR1AH));
				dma_dst = h8s_mem_read_32( H8S_IO_ADDR( H8S_IO_MAR1BH));
				dma_cnt = h8s_mem_read_16( H8S_IO_ADDR( H8S_IO_ETCR1A));
				dma_con = h8s_mem_read_16( H8S_IO_ADDR( H8S_IO_DMACR1A));
				sz = (dma_con & 0x8000) ? 2 : 1;
				for (i=0;i<dma_cnt;i++)
				{
					if (dma_con & 0x8000) h8s_mem_write_16( dma_dst, h8s_mem_read_16( dma_src)); else h8s_mem_write_8( dma_dst, h8s_mem_read_8( dma_src));
					if (dma_con & 0x2000) { if (dma_con & 0x4000) dma_src -= sz; else dma_src += sz; }
					if (dma_con & 0x0020) { if (dma_con & 0x0040) dma_dst -= sz; else dma_dst += sz; }
				}
				h8s.onchip_reg[H8S_IO_DMABCRL] &= ~0x40;
			}
		}
		break;
	}
}

READ8_HANDLER( h8s2323_onchip_reg_r_byte )
{
	UINT8 data;
	_logerror( 3, "[%08X] h8s2323_onchip_reg_r_byte (%08X)\n", H8S_GET_PC(), H8S_IO_ADDR( offset));
	switch (offset)
	{
		// timer hack for cybiko xtreme (increase timer value after low byte has been read)
		case H8S_IO_TTCNT5 + 1 :
		{
			UINT16 tcnt = (h8s.onchip_reg[H8S_IO_TTCNT5+0] << 8) + (h8s.onchip_reg[H8S_IO_TTCNT5+1] << 0);
			tcnt = tcnt + 1000;
			data = h8s.onchip_reg[offset];
			h8s.onchip_reg[H8S_IO_TTCNT5+0] = (tcnt >> 8) & 0xFF;
			h8s.onchip_reg[H8S_IO_TTCNT5+1] = (tcnt >> 0) & 0xFF;
			break;
		}
		// skip "recharge the batteries"
		case H8S_IO_PORTA : data = h8s.onchip_reg[offset] | (1 << 6); break;
		// default
		default : data = h8s2xxx_onchip_reg_r_byte( offset); break;
	}
	_logerror( 3, "%02X\n", data);
	return data;
}

static READ16_HANDLER( h8s2241_onchip_reg_r )
{
	return read16be_with_read8_handler( h8s2241_onchip_reg_r_byte, offset, mem_mask);
}

static WRITE16_HANDLER( h8s2241_onchip_reg_w )
{
	write16be_with_write8_handler( h8s2241_onchip_reg_w_byte, offset, data, mem_mask);
}

static READ16_HANDLER( h8s2246_onchip_reg_r )
{
	return read16be_with_read8_handler( h8s2246_onchip_reg_r_byte, offset, mem_mask);
}

static WRITE16_HANDLER( h8s2246_onchip_reg_w )
{
	write16be_with_write8_handler( h8s2246_onchip_reg_w_byte, offset, data, mem_mask);
}

static READ16_HANDLER( h8s2323_onchip_reg_r )
{
	return read16be_with_read8_handler( h8s2323_onchip_reg_r_byte, offset, mem_mask);
}

static WRITE16_HANDLER( h8s2323_onchip_reg_w )
{
	write16be_with_write8_handler( h8s2323_onchip_reg_w_byte, offset, data, mem_mask);
}

/////////////////
// ADDRESS MAP //
/////////////////

static ADDRESS_MAP_START( h8s2241, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE( 0xFFDC00, 0xFFFBFF ) AM_RAM // on-chip ram
	AM_RANGE( 0xFFFE40, 0xFFFFFF ) AM_READWRITE( h8s2241_onchip_reg_r, h8s2241_onchip_reg_w ) // internal i/o registers
ADDRESS_MAP_END

static ADDRESS_MAP_START( h8s2246, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE( 0xFFDC00, 0xFFFBFF ) AM_RAM // on-chip ram
	AM_RANGE( 0xFFFE40, 0xFFFFFF ) AM_READWRITE( h8s2246_onchip_reg_r, h8s2246_onchip_reg_w ) // internal i/o registers
ADDRESS_MAP_END

static ADDRESS_MAP_START( h8s2323, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE( 0xFFDC00, 0xFFFBFF ) AM_RAM // on-chip ram
	AM_RANGE( 0xFFFE40, 0xFFFFFF ) AM_READWRITE( h8s2323_onchip_reg_r, h8s2323_onchip_reg_w ) // internal i/o registers
ADDRESS_MAP_END

//////////
// INFO //
//////////

void h8s2xxx_set_info( UINT32 state, cpuinfo *info)
{
//	logerror( "h8s2xxx_set_info (state %08X)\n", state);
	switch (state)
	{
		// --- the following bits of info are returned as 64-bit signed integers ---

		// R/W: the current stack pointer value
		case CPUINFO_INT_SP : H8S_SET_SP( info->i); break;
		// R/W: the current PC value
		case CPUINFO_INT_PC : H8S_SET_PC( info->i); break;
		// R/W: the previous PC value
		case CPUINFO_INT_PREVIOUSPC : break;
		// R/W: states for each input line
		case CPUINFO_INT_INPUT_STATE + H8S_IRQ0 : if (info->i) h8s2xxx_interrupt_request( 16); break;
		case CPUINFO_INT_INPUT_STATE + H8S_IRQ1 : if (info->i) h8s2xxx_interrupt_request( 17); break;
		case CPUINFO_INT_INPUT_STATE + H8S_IRQ2 : if (info->i) h8s2xxx_interrupt_request( 18); break;
		case CPUINFO_INT_INPUT_STATE + H8S_IRQ3 : if (info->i) h8s2xxx_interrupt_request( 19); break;
		case CPUINFO_INT_INPUT_STATE + H8S_IRQ4 : if (info->i) h8s2xxx_interrupt_request( 20); break;
		case CPUINFO_INT_INPUT_STATE + H8S_IRQ5 : if (info->i) h8s2xxx_interrupt_request( 21); break;
		case CPUINFO_INT_INPUT_STATE + H8S_IRQ6 : if (info->i) h8s2xxx_interrupt_request( 22); break;
		case CPUINFO_INT_INPUT_STATE + H8S_IRQ7 : if (info->i) h8s2xxx_interrupt_request( 23); break;
		// R/W: states for each output line
		case CPUINFO_INT_OUTPUT_STATE : break;
		// R/W: values of up to MAX_REGs registers
		case CPUINFO_INT_REGISTER + H8S_PC  : H8S_SET_PC( info->i); break;
		case CPUINFO_INT_REGISTER + H8S_ER0 : H8S_SET_REG_32( 0, info->i); break;
		case CPUINFO_INT_REGISTER + H8S_ER1 : H8S_SET_REG_32( 1, info->i); break;
		case CPUINFO_INT_REGISTER + H8S_ER2 : H8S_SET_REG_32( 2, info->i); break;
		case CPUINFO_INT_REGISTER + H8S_ER3 : H8S_SET_REG_32( 3, info->i); break;
		case CPUINFO_INT_REGISTER + H8S_ER4 : H8S_SET_REG_32( 4, info->i); break;
		case CPUINFO_INT_REGISTER + H8S_ER5 : H8S_SET_REG_32( 5, info->i); break;
		case CPUINFO_INT_REGISTER + H8S_ER6 : H8S_SET_REG_32( 6, info->i); break;
		case CPUINFO_INT_REGISTER + H8S_ER7 : H8S_SET_REG_32( 7, info->i); break;
	}
}

static void cpuinfo_str_flags( char *str)
{
	strcpy( str, "........");
	if (H8S_GET_FLAG_C) str[0] = 'C';
	if (H8S_GET_FLAG_V) str[1] = 'V';
	if (H8S_GET_FLAG_Z) str[2] = 'Z';
	if (H8S_GET_FLAG_N) str[3] = 'N';
	if (H8S_GET_FLAG_U) str[4] = 'U';
	if (H8S_GET_FLAG_H) str[5] = 'H';
	if (H8S_GET_FLAG_X) str[6] = 'X'; // 'UI'
	if (H8S_GET_FLAG_I) str[7] = 'I';
}

void h8s2xxx_get_info( UINT32 state, cpuinfo *info)
{
//	logerror( "h8s2xxx_get_info (state %08X)\n", state);
	switch (state)
	{
		// --- the following bits of info are returned as 64-bit signed integers ---

		// R/O : size of CPU context in bytes
		case CPUINFO_INT_CONTEXT_SIZE : info->i = sizeof( H8S2XXX); break;
		// R/O : number of input lines
		case CPUINFO_INT_INPUT_LINES : break;
		// R/O : number of output lines
		case CPUINFO_INT_OUTPUT_LINES : break;
		// R/O: default IRQ vector
		case CPUINFO_INT_DEFAULT_IRQ_VECTOR : break;
		// R/O: either CPU_IS_BE or CPU_IS_LE
		case CPUINFO_INT_ENDIANNESS : info->i = CPU_IS_BE; break;
		// R/O: internal clock divider
		case CPUINFO_INT_CLOCK_DIVIDER : break;
		// R/O: minimum bytes per instruction
		case CPUINFO_INT_MIN_INSTRUCTION_BYTES : info->i = 2; break;
		// R/O: maximum bytes per instruction
		case CPUINFO_INT_MAX_INSTRUCTION_BYTES : info->i = 10; break;
		// R/O: minimum cycles for a single instruction
		case CPUINFO_INT_MIN_CYCLES : break;
		// R/O: maximum cycles for a single instruction
		case CPUINFO_INT_MAX_CYCLES : break;
		// R/O: data bus size for each address space (8,16,32,64)
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM : info->i = 16; break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO      : info->i =  8; break;
		// R/O: address bus size for each address space (12-32)
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM : info->i = 24; break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO      : info->i = 16; break;
		// R/O: shift applied to addresses each address space (+3 means >>3, -1 means <<1)
		case CPUINFO_INT_ADDRBUS_SHIFT : break;
		// R/O: address bus size for logical accesses in each space (0=same as physical)
		case CPUINFO_INT_LOGADDR_WIDTH : break;
		// R/O: size of a page log 2 (i.e., 12=4096), or 0 if paging not supported
		case CPUINFO_INT_PAGE_SHIFT : break;
		// R/W: the current stack pointer value
		case CPUINFO_INT_SP : info->i = H8S_GET_SP(); break;
		// R/W: the current PC value
		case CPUINFO_INT_PC : info->i = H8S_GET_PC(); break;
		// R/W: the previous PC value
		case CPUINFO_INT_PREVIOUSPC : break;
		// R/W: states for each input line
		case CPUINFO_INT_INPUT_STATE : break;
		// R/W: states for each output line
		case CPUINFO_INT_OUTPUT_STATE : break;
		// R/W: values of up to MAX_REGs registers
		case CPUINFO_INT_REGISTER + H8S_PC  : info->i = H8S_GET_PC(); break;
		case CPUINFO_INT_REGISTER + H8S_ER0 : info->i = H8S_GET_REG_32( 0); break;
		case CPUINFO_INT_REGISTER + H8S_ER1 : info->i = H8S_GET_REG_32( 1); break;
		case CPUINFO_INT_REGISTER + H8S_ER2 : info->i = H8S_GET_REG_32( 2); break;
		case CPUINFO_INT_REGISTER + H8S_ER3 : info->i = H8S_GET_REG_32( 3); break;
		case CPUINFO_INT_REGISTER + H8S_ER4 : info->i = H8S_GET_REG_32( 4); break;
		case CPUINFO_INT_REGISTER + H8S_ER5 : info->i = H8S_GET_REG_32( 5); break;
		case CPUINFO_INT_REGISTER + H8S_ER6 : info->i = H8S_GET_REG_32( 6); break;
		case CPUINFO_INT_REGISTER + H8S_ER7 : info->i = H8S_GET_REG_32( 7); break;

		// --- the following bits of info are returned as pointers to data or functions ---

		case CPUINFO_PTR_SET_INFO             : info->setinfo         = h8s2xxx_set_info; break;
		case CPUINFO_PTR_GET_CONTEXT          : info->getcontext      = h8s2xxx_get_context; break;
		case CPUINFO_PTR_SET_CONTEXT          : info->setcontext      = h8s2xxx_set_context; break;
		case CPUINFO_PTR_INIT                 : info->init            = h8s2xxx_init; break;
		case CPUINFO_PTR_RESET                : info->reset           = h8s2xxx_reset; break;
		case CPUINFO_PTR_EXIT                 : info->exit            = h8s2xxx_exit; break;
		case CPUINFO_PTR_EXECUTE              : info->execute         = h8s2xxx_execute; break;
		case CPUINFO_PTR_BURN                 : info->burn            = NULL; break; // h8s2xxx_burn
#ifdef MAME_DEBUG
		case CPUINFO_PTR_DISASSEMBLE          : info->disassemble     = h8s2xxx_disassemble; break;
#endif
		case CPUINFO_PTR_TRANSLATE            : info->translate       = NULL; break; // h8s2xxx_translate
		case CPUINFO_PTR_READ                 : info->read            = NULL; break; // h8s2xxx_read
		case CPUINFO_PTR_WRITE                : info->write           = NULL; break; // h8s2xxx_write
		case CPUINFO_PTR_READOP               : info->readop          = NULL; break; // h8s2xxx_readop
#ifdef MAME_DEBUG
		case CPUINFO_PTR_DEBUG_SETUP_COMMANDS : info->setup_commands  = NULL; break; // h8s2xxx_setup_commands
#endif
		case CPUINFO_PTR_INSTRUCTION_COUNTER  : info->icount          = &cpu_icount; break;
		case CPUINFO_PTR_DEBUG_REGISTER_LIST  : info->p               = NULL; break; // &h8s2xxx_debug_register_list

		// --- the following bits of info are returned as NULL-terminated strings ---

		case CPUINFO_STR_NAME               : strcpy( info->s = cpuintrf_temp_str(), "H8S/2xxx"); break;
		case CPUINFO_STR_CORE_FAMILY        : strcpy( info->s = cpuintrf_temp_str(), "Hitachi H8S"); break;
		case CPUINFO_STR_CORE_VERSION       : strcpy( info->s = cpuintrf_temp_str(), "1.0"); break;
		case CPUINFO_STR_CORE_FILE          : strcpy( info->s = cpuintrf_temp_str(), __FILE__); break;
		case CPUINFO_STR_CORE_CREDITS       : strcpy( info->s = cpuintrf_temp_str(), "Copyright (C) 2001-2006 Tim Schuerewegen"); break;
		case CPUINFO_STR_FLAGS              : cpuinfo_str_flags( info->s = cpuintrf_temp_str()); break;
		case CPUINFO_STR_REGISTER + H8S_PC  : sprintf( info->s = cpuintrf_temp_str(), "PC :%08X", H8S_GET_PC()); break;
		case CPUINFO_STR_REGISTER + H8S_ER0 : sprintf( info->s = cpuintrf_temp_str(), "ER0:%08X", H8S_GET_REG_32( 0)); break;
		case CPUINFO_STR_REGISTER + H8S_ER1 : sprintf( info->s = cpuintrf_temp_str(), "ER1:%08X", H8S_GET_REG_32( 1)); break;
		case CPUINFO_STR_REGISTER + H8S_ER2 : sprintf( info->s = cpuintrf_temp_str(), "ER2:%08X", H8S_GET_REG_32( 2)); break;
		case CPUINFO_STR_REGISTER + H8S_ER3 : sprintf( info->s = cpuintrf_temp_str(), "ER3:%08X", H8S_GET_REG_32( 3)); break;
		case CPUINFO_STR_REGISTER + H8S_ER4 : sprintf( info->s = cpuintrf_temp_str(), "ER4:%08X", H8S_GET_REG_32( 4)); break;
		case CPUINFO_STR_REGISTER + H8S_ER5 : sprintf( info->s = cpuintrf_temp_str(), "ER5:%08X", H8S_GET_REG_32( 5)); break;
		case CPUINFO_STR_REGISTER + H8S_ER6 : sprintf( info->s = cpuintrf_temp_str(), "ER6:%08X", H8S_GET_REG_32( 6)); break;
		case CPUINFO_STR_REGISTER + H8S_ER7 : sprintf( info->s = cpuintrf_temp_str(), "ER7:%08X", H8S_GET_REG_32( 7)); break;
	}
}

#if (HAS_H8S2241)
void h8s2241_get_info( UINT32 state, cpuinfo *info)
{
//	logerror( "h8s2241_get_info (state %08X)\n", state);
	switch (state)
	{
		case CPUINFO_STR_NAME : strcpy( info->s = cpuintrf_temp_str(), "H8S/2241"); break;
		case CPUINFO_PTR_INTERNAL_MEMORY_MAP + ADDRESS_SPACE_PROGRAM : info->internal_map = &construct_map_h8s2241; break;
		default : h8s2xxx_get_info( state, info); break;
	}
}
#endif

#if (HAS_H8S2246)
void h8s2246_get_info( UINT32 state, cpuinfo *info)
{
//	logerror( "h8s2246_get_info (state %08X)\n", state);
	switch (state)
	{
		case CPUINFO_STR_NAME : strcpy( info->s = cpuintrf_temp_str(), "H8S/2246"); break;
		case CPUINFO_PTR_INTERNAL_MEMORY_MAP + ADDRESS_SPACE_PROGRAM : info->internal_map = &construct_map_h8s2246; break;
		default : h8s2xxx_get_info( state, info); break;
	}
}
#endif

#if (HAS_H8S2323)
void h8s2323_get_info( UINT32 state, cpuinfo *info)
{
//	logerror( "h8s2323_get_info (state %08X)\n", state);
	switch (state)
	{
		case CPUINFO_STR_NAME : strcpy( info->s = cpuintrf_temp_str(), "H8S/2323"); break;
		case CPUINFO_PTR_INTERNAL_MEMORY_MAP + ADDRESS_SPACE_PROGRAM : info->internal_map = &construct_map_h8s2323; break;
		default : h8s2xxx_get_info( state, info); break;
	}
}
#endif
