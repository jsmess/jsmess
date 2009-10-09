/******************************************************************************


    Philips CD-I
    -------------------

    Preliminary MAME driver by Roberto Fresca, David Haywood & Angelo Salese
    MESS improvements by Harmony
    Help provided by CD-i Fan


*******************************************************************************

STATUS:

Some games will start up, but fail horribly.

TODO:

-Proper handling of the 68070 (68k with 32 address lines instead of 24)
 & handle the extra features properly (UART,DMA,Timers etc.)

-Full emulation of the 66470 and/or MCD212 Video Chip (still many unhandled features)

-Unknown sound chip (CD-i ADPCM);

*******************************************************************************/


#define CLOCK_A	XTAL_30MHz
#define CLOCK_B	XTAL_19_6608MHz

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "sound/2413intf.h"
#include "devices/chd_cd.h"

static UINT16 *planea;
static UINT16 *planeb;
static UINT16 *cdram;

emu_timer *test_timer;

#define ENABLE_UART_PRINTING (0)

#define VERBOSE_LEVEL	(11)

#define ENABLE_VERBOSE_LOG (0)

#if ENABLE_VERBOSE_LOG
INLINE void verboselog(running_machine *machine, int n_level, const char *s_fmt, ...)
{
	if( VERBOSE_LEVEL >= n_level )
	{
		va_list v;
		char buf[ 32768 ];
		va_start( v, s_fmt );
		vsprintf( buf, s_fmt, v );
		va_end( v );
		logerror( "%08x: %s", cpu_get_pc(cputag_get_cpu(machine, "maincpu")), buf );
	}
}
#else
#define verboselog(x,y,z,...)
#endif

/***********************
* Forward declarations *
***********************/

// SCC68070
static void scc68070_set_timer_callback(int channel);
static TIMER_CALLBACK( scc68070_timer0_callback );
static READ16_HANDLER( scc68070_periphs_r );
static WRITE16_HANDLER( scc68070_periphs_w );

// CDIC
static READ16_HANDLER( cdic_r );
static WRITE16_HANDLER( cdic_w );

// SLAVE
static TIMER_CALLBACK( slave_trigger_readback_int );
static void slave_prepare_readback(running_machine *machine, attotime delay, UINT8 channel, UINT8 count, UINT8 data0, UINT8 data1, UINT8 data2, UINT8 data3, UINT8 cmd);
static void perform_mouse_update(running_machine *machine);
static INPUT_CHANGED( mouse_update );
static READ16_HANDLER( slave_r );
static WRITE16_HANDLER( slave_w );

// RTC
static READ16_HANDLER( m48t08_r );
static WRITE16_HANDLER( m48t08_w );

// Platform-specific
static void cdi220_draw_lcd(running_machine *machine, int y);

// MCD212
static READ16_HANDLER(mcd212_r);
static WRITE16_HANDLER(mcd212_w);
INLINE void mcd212_set_register(running_machine *machine, int channel, UINT8 reg, UINT32 value);
INLINE void mcd212_set_vsr(int channel, UINT32 value);
INLINE UINT32 mcd212_get_vsr(int channel);
INLINE void mcd212_set_dcp(int channel, UINT32 value);
INLINE UINT32 mcd212_get_dcp(int channel);
INLINE void mcd212_set_display_parameters(int channel, UINT8 value);
static void mcd212_process_ica(running_machine *machine, int channel);
static void mcd212_process_dca(running_machine *machine, int channel);
static void mcd212_process_vsr(running_machine *machine, int channel, UINT32 *pixels);
static void mcd212_draw_cursor(running_machine *machine, UINT32 *scanline, int y);
static void mcd212_mix_lines(running_machine *machine, UINT32 *plane_a, UINT32 *plane_b, UINT32 *out);
static void mcd212_draw_scanline(running_machine *machine, int y);
TIMER_CALLBACK( mcd212_perform_scan );
static VIDEO_START(cdi);

// Miscellaneous
TIMER_CALLBACK( test_timer_callback );

/***********************
* On-board peripherals *
***********************/

typedef struct
{
	UINT8 reserved0;
	UINT8 data_register;
	UINT8 reserved1;
	UINT8 address_register;
	UINT8 reserved2;
	UINT8 status_register;
	UINT8 reserved3;
	UINT8 control_register;
	UINT8 reserved;
	UINT8 clock_control_register;
} scc68070_i2c_regs_t;

#define ISR_MST		0x80	// Master
#define ISR_TRX		0x40	// Transmitter
#define ISR_BB		0x20	// Busy
#define ISR_PIN		0x10	// No Pending Interrupt
#define ISR_AL		0x08	// Arbitration Lost
#define ISR_AAS		0x04	// Addressed As Slave
#define ISR_AD0		0x02	// Address Zero
#define ISR_LRB		0x01	// Last Received Bit

typedef struct
{
	UINT8 reserved0;
	UINT8 mode_register;
	UINT8 reserved1;
	UINT8 status_register;
	UINT8 reserved2;
	UINT8 clock_select;
	UINT8 reserved3;
	UINT8 command_register;
	UINT8 reserved4;
	UINT8 transmit_holding_register;
	UINT8 reserved5;
	UINT8 receive_holding_register;
} scc68070_uart_regs_t;

#define UMR_OM			0xc0
#define UMR_OM_NORMAL	0x00
#define UMR_OM_ECHO		0x40
#define UMR_OM_LOOPBACK	0x80
#define UMR_OM_RLOOP	0xc0
#define UMR_TXC			0x10
#define UMR_PC			0x08
#define UMR_P			0x04
#define UMR_SB			0x02
#define UMR_CL			0x01

#define USR_RB			0x80
#define USR_FE			0x40
#define USR_PE			0x20
#define USR_OE			0x10
#define USR_TXEMT		0x08
#define USR_TXRDY		0x04
#define USR_RXRDY		0x01

typedef struct
{
	UINT8 timer_status_register;
	UINT8 timer_control_register;
	UINT16 reload_register;
	UINT16 timer0;
	UINT16 timer1;
	UINT16 timer2;
	emu_timer* timer0_timer;
} scc68070_timer_regs_t;

#define TSR_OV0			0x80
#define TSR_MA1			0x40
#define TSR_CAP1		0x20
#define TSR_OV1			0x10
#define TSR_MA2			0x08
#define TSR_CAP2		0x04
#define TSR_OV2			0x02

#define TCR_E1			0xc0
#define TCR_E1_NONE		0x00
#define TCR_E1_RISING	0x40
#define TCR_E1_FALLING	0x80
#define TCR_E1_BOTH		0xc0
#define TCR_M1			0x30
#define TCR_M1_NONE		0x00
#define TCR_M1_MATCH	0x10
#define TCR_M1_CAPTURE	0x20
#define TCR_M1_COUNT	0x30
#define TCR_E2			0x0c
#define TCR_E2_NONE		0x00
#define TCR_E2_RISING	0x04
#define TCR_E2_FALLING	0x08
#define TCR_E2_BOTH		0x0c
#define TCR_M2			0x03
#define TCR_M2_NONE		0x00
#define TCR_M2_MATCH	0x01
#define TCR_M2_CAPTURE	0x02
#define TCR_M2_COUNT	0x03

typedef struct
{
	UINT8 channel_status;
	UINT8 channel_error;

	UINT8 reserved0[2];

	UINT8 device_control;
	UINT8 operation_control;
	UINT8 sequence_control;
	UINT8 channel_control;

	UINT8 reserved1[3];

	UINT16 transfer_counter;

	UINT32 memory_address_counter;

	UINT8 reserved2[4];

	UINT32 device_address_counter;

	UINT8 reserved3[40];
} scc68070_dma_channel_t;

#define CSR_COC			0x80
#define CSR_NDT			0x20
#define CSR_ERR			0x10
#define CSR_CA			0x08

#define CER_EC			0x1f
#define CER_NONE		0x00
#define CER_TIMING		0x02
#define CER_BUSERR_MEM	0x09
#define CER_BUSERR_DEV	0x0a
#define CER_SOFT_ABORT	0x11

#define DCR1_ERM		0x80
#define DCR1_DT			0x30

#define DCR2_ERM		0x80
#define DCR2_DT			0x30
#define DCR2_DS			0x08

#define OCR_D			0x80
#define OCR_D_M2D		0x00
#define OCR_D_D2M		0x80
#define OCR_OS			0x30
#define OCR_OS_BYTE		0x00
#define OCR_OS_WORD		0x10

#define SCR2_MAC		0x0c
#define SCR2_MAC_NONE	0x00
#define SCR2_MAC_INC	0x04
#define SCR2_DAC		0x03
#define SCR2_DAC_NONE	0x00
#define SCR2_DAC_INC	0x01

#define CCR_SO			0x80
#define CCR_SA			0x10
#define CCR_INE			0x08
#define CCR_IPL			0x07

typedef struct
{
	scc68070_dma_channel_t channel[2];
} scc68070_dma_regs_t;

typedef struct
{
	UINT16 attr;
	UINT16 length;
	UINT8  undefined;
	UINT8  segment;
	UINT16 base;
} scc68070_mmu_desc_t;

typedef struct
{
	UINT8 status;
	UINT8 control;

	UINT8 reserved[0x3e];

	scc68070_mmu_desc_t desc[8];
} scc68070_mmu_regs_t;

typedef struct
{
	UINT16 lir;
	scc68070_i2c_regs_t i2c;
	scc68070_uart_regs_t uart;
	scc68070_timer_regs_t timers;
	UINT8 picr1;
	UINT8 picr2;
	scc68070_dma_regs_t dma;
	scc68070_mmu_regs_t mmu;
} scc68070_regs_t;

static scc68070_regs_t scc68070_regs;

static void scc68070_set_timer_callback(int channel)
{
	UINT32 compare = 0;
	attotime period;
	switch(channel)
	{
		case 0:
			compare = 0x10000 - scc68070_regs.timers.timer0;
			period = attotime_mul(ATTOTIME_IN_HZ(CLOCK_A/10), compare);
            timer_adjust_oneshot(scc68070_regs.timers.timer0_timer, period, 0);
			break;
		default:
			fatalerror( "Unsupported timer channel to scc68070_set_timer_callback!\n" );
	}
}

static TIMER_CALLBACK( scc68070_timer0_callback )
{
	scc68070_regs.timers.timer0 = scc68070_regs.timers.reload_register;
	scc68070_regs.timers.timer_status_register |= TSR_OV0;
	if(scc68070_regs.picr1 & 7)
	{
		UINT8 interrupt = scc68070_regs.picr1 & 7;
		scc68070_regs.timers.timer_status_register |= TSR_OV0;
		cpu_set_input_line_vector(cputag_get_cpu(machine, "maincpu"), M68K_IRQ_1 + (interrupt - 1), 56 + interrupt);
		cputag_set_input_line(machine, "maincpu", M68K_IRQ_1 + (interrupt - 1), ASSERT_LINE);
	}
	scc68070_set_timer_callback(0);
}

static READ16_HANDLER( scc68070_periphs_r )
{
	switch(offset)
	{
		// Interupts: 80001001
		case 0x1000/2: // LIR priority level
			return scc68070_regs.lir;

		// I2C interface: 80002001 to 80002009
		case 0x2000/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: I2C Data Register: %04x & %04x\n", scc68070_regs.i2c.data_register, mem_mask);
			}
			return scc68070_regs.i2c.data_register;
		case 0x2002/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: I2C Address Register: %04x & %04x\n", scc68070_regs.i2c.address_register, mem_mask);
			}
			return scc68070_regs.i2c.address_register;
		case 0x2004/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: I2C Status Register: %04x & %04x\n", scc68070_regs.i2c.status_register, mem_mask);
			}
			return scc68070_regs.i2c.status_register;
		case 0x2006/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: I2C Control Register: %04x & %04x\n", scc68070_regs.i2c.control_register, mem_mask);
			}
			return scc68070_regs.i2c.control_register;
		case 0x2008/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: I2C Clock Control Register: %04x & %04x\n", scc68070_regs.i2c.clock_control_register, mem_mask);
			}
			return scc68070_regs.i2c.clock_control_register;

		// UART interface: 80002011 to 8000201b
		case 0x2010/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: UART Mode Register: %04x & %04x\n", scc68070_regs.uart.mode_register, mem_mask);
			}
			return scc68070_regs.uart.mode_register | 0x20;
		case 0x2012/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: UART Status Register: %04x & %04x\n", scc68070_regs.uart.status_register, mem_mask);
			}
			return scc68070_regs.uart.status_register /*| USR_TXEMT*/ | USR_TXRDY | (1 << 1) | USR_RXRDY;
		case 0x2014/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: UART Clock Select: %04x & %04x\n", scc68070_regs.uart.clock_select, mem_mask);
			}
			return scc68070_regs.uart.clock_select | 0x08;
		case 0x2016/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: UART Command Register: %02x & %04x\n", scc68070_regs.uart.command_register, mem_mask);
			}
			return scc68070_regs.uart.command_register | 0x80;
		case 0x2018/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: UART Transmit Holding Register: %02x & %04x\n", scc68070_regs.uart.transmit_holding_register, mem_mask);
			}
			return scc68070_regs.uart.transmit_holding_register;
		case 0x201a/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: UART Receive Holding Register: %02x & %04x\n", scc68070_regs.uart.receive_holding_register, mem_mask);
				return scc68070_regs.uart.receive_holding_register;
			}
			return 0;

		// Timers: 80002020 to 80002029
		case 0x2020/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: Timer Control Register: %02x & %04x\n", scc68070_regs.timers.timer_control_register, mem_mask);
			}
			if(ACCESSING_BITS_8_15)
			{
				verboselog(space->machine, 12, "scc68070_periphs_r: Timer Status Register: %02x & %04x\n", scc68070_regs.timers.timer_status_register, mem_mask);
			}
			return (scc68070_regs.timers.timer_status_register << 8) | scc68070_regs.timers.timer_control_register;
			break;
		case 0x2022/2:
			verboselog(space->machine, 2, "scc68070_periphs_r: Timer Reload Register: %04x & %04x\n", scc68070_regs.timers.reload_register, mem_mask);
			return scc68070_regs.timers.reload_register;
		case 0x2024/2:
			verboselog(space->machine, 2, "scc68070_periphs_r: Timer 0: %04x & %04x\n", scc68070_regs.timers.timer0, mem_mask);
			return scc68070_regs.timers.timer0;
		case 0x2026/2:
			verboselog(space->machine, 2, "scc68070_periphs_r: Timer 1: %04x & %04x\n", scc68070_regs.timers.timer1, mem_mask);
			return scc68070_regs.timers.timer1;
		case 0x2028/2:
			verboselog(space->machine, 2, "scc68070_periphs_r: Timer 2: %04x & %04x\n", scc68070_regs.timers.timer2, mem_mask);
			return scc68070_regs.timers.timer2;

		// PICR1: 80002045
		case 0x2044/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: Peripheral Interrupt Control Register 1: %02x & %04x\n", scc68070_regs.picr1, mem_mask);
			}
			return scc68070_regs.picr1;

		// PICR2: 80002047
		case 0x2046/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: Peripheral Interrupt Control Register 2: %02x & %04x\n", scc68070_regs.picr2, mem_mask);
			}
			return scc68070_regs.picr2;

		// DMA controller: 80004000 to 8000406d
		case 0x4000/2:
		case 0x4040/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: DMA(%d) Error Register: %04x & %04x\n", (offset - 0x2000) / 32, scc68070_regs.dma.channel[(offset - 0x2000) / 32].channel_error, mem_mask);
			}
			if(ACCESSING_BITS_8_15)
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: DMA(%d) Status Register: %04x & %04x\n", (offset - 0x2000) / 32, scc68070_regs.dma.channel[(offset - 0x2000) / 32].channel_status, mem_mask);
			}
			return (scc68070_regs.dma.channel[(offset - 0x2000) / 32].channel_status << 8) | scc68070_regs.dma.channel[(offset - 0x2000) / 32].channel_error;
			break;
		case 0x4004/2:
		case 0x4044/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: DMA(%d) Operation Control Register: %02x & %04x\n", (offset - 0x2000) / 32, scc68070_regs.dma.channel[(offset - 0x2000) / 32].operation_control, mem_mask);
			}
			if(ACCESSING_BITS_8_15)
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: DMA(%d) Device Control Register: %02x & %04x\n", (offset - 0x2000) / 32, scc68070_regs.dma.channel[(offset - 0x2000) / 32].device_control, mem_mask);
			}
			return (scc68070_regs.dma.channel[(offset - 0x2000) / 32].device_control << 8) | scc68070_regs.dma.channel[(offset - 0x2000) / 32].operation_control;
		case 0x4006/2:
		case 0x4046/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: DMA(%d) Channel Control Register: %02x & %04x\n", (offset - 0x2000) / 32, scc68070_regs.dma.channel[(offset - 0x2000) / 32].channel_control, mem_mask);
			}
			if(ACCESSING_BITS_8_15)
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: DMA(%d) Sequence Control Register: %02x & %04x\n", (offset - 0x2000) / 32, scc68070_regs.dma.channel[(offset - 0x2000) / 32].sequence_control, mem_mask);
			}
			return (scc68070_regs.dma.channel[(offset - 0x2000) / 32].sequence_control << 8) | scc68070_regs.dma.channel[(offset - 0x2000) / 32].channel_control;
		case 0x400a/2:
			verboselog(space->machine, 2, "scc68070_periphs_r: DMA(%d) Memory Transfer Counter: %04x & %04x\n", (offset - 0x2000) / 32, scc68070_regs.dma.channel[(offset - 0x2000) / 32].transfer_counter, mem_mask);
			return scc68070_regs.dma.channel[(offset - 0x2000) / 32].transfer_counter;
		case 0x400c/2:
		case 0x404c/2:
			verboselog(space->machine, 2, "scc68070_periphs_r: DMA(%d) Memory Address Counter (High Word): %04x & %04x\n", (offset - 0x2000) / 32, (scc68070_regs.dma.channel[(offset - 0x2000) / 32].memory_address_counter >> 16), mem_mask);
			return (scc68070_regs.dma.channel[(offset - 0x2000) / 32].memory_address_counter >> 16);
		case 0x400e/2:
		case 0x404e/2:
			verboselog(space->machine, 2, "scc68070_periphs_r: DMA(%d) Memory Address Counter (Low Word): %04x & %04x\n", (offset - 0x2000) / 32, scc68070_regs.dma.channel[(offset - 0x2000) / 32].memory_address_counter, mem_mask);
			return scc68070_regs.dma.channel[(offset - 0x2000) / 32].memory_address_counter;
		case 0x4014/2:
		case 0x4054/2:
			verboselog(space->machine, 2, "scc68070_periphs_r: DMA(%d) Device Address Counter (High Word): %04x & %04x\n", (offset - 0x2000) / 32, (scc68070_regs.dma.channel[(offset - 0x2000) / 32].device_address_counter >> 16), mem_mask);
			return (scc68070_regs.dma.channel[(offset - 0x2000) / 32].device_address_counter >> 16);
		case 0x4016/2:
		case 0x4056/2:
			verboselog(space->machine, 2, "scc68070_periphs_r: DMA(%d) Device Address Counter (Low Word): %04x & %04x\n", (offset - 0x2000) / 32, scc68070_regs.dma.channel[(offset - 0x2000) / 32].device_address_counter, mem_mask);
			return scc68070_regs.dma.channel[(offset - 0x2000) / 32].device_address_counter;

		// MMU: 80008000 to 8000807f
		case 0x8000/2:	// Status / Control register
			if(ACCESSING_BITS_0_7)
			{	// Control
				verboselog(space->machine, 2, "scc68070_periphs_r: MMU Control: %02x & %04x\n", scc68070_regs.mmu.control, mem_mask);
				return scc68070_regs.mmu.control;
			}	// Status
			else
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: MMU Status: %02x & %04x\n", scc68070_regs.mmu.status, mem_mask);
				return scc68070_regs.mmu.status;
			}
			break;
		case 0x8040/2:
		case 0x8048/2:
		case 0x8050/2:
		case 0x8058/2:
		case 0x8060/2:
		case 0x8068/2:
		case 0x8070/2:
		case 0x8078/2:	// Attributes (SD0-7)
			verboselog(space->machine, 2, "scc68070_periphs_r: MMU descriptor %d attributes: %04x & %04x\n", (offset - 0x4020) / 4, scc68070_regs.mmu.desc[(offset - 0x4020) / 4].attr, mem_mask);
			return scc68070_regs.mmu.desc[(offset - 0x4020) / 4].attr;
		case 0x8042/2:
		case 0x804a/2:
		case 0x8052/2:
		case 0x805a/2:
		case 0x8062/2:
		case 0x806a/2:
		case 0x8072/2:
		case 0x807a/2:	// Segment Length (SD0-7)
			verboselog(space->machine, 2, "scc68070_periphs_r: MMU descriptor %d length: %04x & %04x\n", (offset - 0x4020) / 4, scc68070_regs.mmu.desc[(offset - 0x4020) / 4].length, mem_mask);
			return scc68070_regs.mmu.desc[(offset - 0x4020) / 4].length;
		case 0x8044/2:
		case 0x804c/2:
		case 0x8054/2:
		case 0x805c/2:
		case 0x8064/2:
		case 0x806c/2:
		case 0x8074/2:
		case 0x807c/2:	// Segment Number (SD0-7, A0=1 only)
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: MMU descriptor %d segment: %02x & %04x\n", (offset - 0x4020) / 4, scc68070_regs.mmu.desc[(offset - 0x4020) / 4].segment, mem_mask);
				return scc68070_regs.mmu.desc[(offset - 0x4020) / 4].segment;
			}
			break;
		case 0x8046/2:
		case 0x804e/2:
		case 0x8056/2:
		case 0x805e/2:
		case 0x8066/2:
		case 0x806e/2:
		case 0x8076/2:
		case 0x807e/2:	// Base Address (SD0-7)
			verboselog(space->machine, 2, "scc68070_periphs_r: MMU descriptor %d base: %04x & %04x\n", (offset - 0x4020) / 4, scc68070_regs.mmu.desc[(offset - 0x4020) / 4].base, mem_mask);
			return scc68070_regs.mmu.desc[(offset - 0x4020) / 4].base;
		default:
			verboselog(space->machine, 0, "scc68070_periphs_r: Unknown address: %04x & %04x\n", offset * 2, mem_mask);
			break;
	}

	return 0;
}

static WRITE16_HANDLER( scc68070_periphs_w )
{
	switch(offset)
	{
		// Interupts: 80001001
		case 0x1000/2: // LIR priority level
			verboselog(space->machine, 2, "scc68070_periphs_w: LIR: %04x & %04x\n", data, mem_mask);
			COMBINE_DATA(&scc68070_regs.lir);
			break;

		// I2C interface: 80002001 to 80002009
		case 0x2000/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: I2C Data Register: %04x & %04x\n", data, mem_mask);
				scc68070_regs.i2c.data_register = data & 0x00ff;
			}
			break;
		case 0x2002/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: I2C Address Register: %04x & %04x\n", data, mem_mask);
				scc68070_regs.i2c.address_register = data & 0x00ff;
			}
			break;
		case 0x2004/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: I2C Status Register: %04x & %04x\n", data, mem_mask);
				scc68070_regs.i2c.status_register = data & 0x00ff;
			}
			break;
		case 0x2006/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: I2C Control Register: %04x & %04x\n", data, mem_mask);
				scc68070_regs.i2c.control_register = data & 0x00ff;
			}
			break;
		case 0x2008/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: I2C Clock Control Register: %04x & %04x\n", data, mem_mask);
				scc68070_regs.i2c.clock_control_register = data & 0x00ff;
			}
			break;

		// UART interface: 80002011 to 8000201b
		case 0x2010/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: UART Mode Register: %04x & %04x\n", data, mem_mask);
				scc68070_regs.uart.mode_register = data & 0x00ff;
			}
			break;
		case 0x2012/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: UART Status Register: %04x & %04x\n", data, mem_mask);
				scc68070_regs.uart.status_register = data & 0x00ff;
			}
			break;
		case 0x2014/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: UART Clock Select: %04x & %04x\n", data, mem_mask);
				scc68070_regs.uart.clock_select = data & 0x00ff;
			}
			break;
		case 0x2016/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: UART Command Register: %04x & %04x\n", data, mem_mask);
				scc68070_regs.uart.command_register = data & 0x00ff;
			}
			break;
		case 0x2018/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: UART Transmit Holding Register: %04x & %04x: %c\n", data, mem_mask, (data >= 0x20 && data < 0x7f) ? (data & 0x00ff) : ' ');
				if((data >= 0x20 && data < 0x7f) || data == 0x08)
				{
					printf( "%c", data & 0x00ff );
				}
				if(data == 0x0d)
				{
					printf( "\n" );
				}
				scc68070_regs.uart.transmit_holding_register = data & 0x00ff;
			}
			break;
		case 0x201a/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: UART Receive Holding Register: %04x & %04x\n", data, mem_mask);
				scc68070_regs.uart.receive_holding_register = data & 0x00ff;
			}
			break;

		// Timers: 80002020 to 80002029
		case 0x2020/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: Timer Control Register: %04x & %04x\n", data, mem_mask);
				scc68070_regs.timers.timer_control_register = data & 0x00ff;
			}
			if(ACCESSING_BITS_8_15)
			{
				verboselog(space->machine, 12, "scc68070_periphs_w: Timer Status Register: %04x & %04x\n", data, mem_mask);
				scc68070_regs.timers.timer_status_register &= ~(data >> 8);
				if(!scc68070_regs.timers.timer_status_register)
				{
					UINT8 interrupt = scc68070_regs.picr1 & 7;
					cputag_set_input_line(space->machine, "maincpu", M68K_IRQ_1 + (interrupt - 1), CLEAR_LINE);
				}
			}
			break;
		case 0x2022/2:
			verboselog(space->machine, 2, "scc68070_periphs_w: Timer Reload Register: %04x & %04x\n", data, mem_mask);
			COMBINE_DATA(&scc68070_regs.timers.reload_register);
			break;
		case 0x2024/2:
			verboselog(space->machine, 2, "scc68070_periphs_w: Timer 0: %04x & %04x\n", data, mem_mask);
			COMBINE_DATA(&scc68070_regs.timers.timer0);
			scc68070_set_timer_callback(0);
			break;
		case 0x2026/2:
			verboselog(space->machine, 2, "scc68070_periphs_w: Timer 1: %04x & %04x\n", data, mem_mask);
			COMBINE_DATA(&scc68070_regs.timers.timer1);
			break;
		case 0x2028/2:
			verboselog(space->machine, 2, "scc68070_periphs_w: Timer 2: %04x & %04x\n", data, mem_mask);
			COMBINE_DATA(&scc68070_regs.timers.timer2);
			break;

		// PICR1: 80002045
		case 0x2044/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: Peripheral Interrupt Control Register 1: %04x & %04x\n", data, mem_mask);
				scc68070_regs.picr1 = data & 0x00ff;
			}
			break;

		// PICR2: 80002047
		case 0x2046/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: Peripheral Interrupt Control Register 2: %04x & %04x\n", data, mem_mask);
				scc68070_regs.picr2 = data & 0x00ff;
			}
			break;

		// DMA controller: 80004000 to 8000406d
		case 0x4000/2:
		case 0x4040/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: DMA(%d) Error (invalid): %04x & %04x\n", (offset - 0x2000) / 32, data, mem_mask);
			}
			if(ACCESSING_BITS_8_15)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: DMA(%d) Status: %04x & %04x\n", (offset - 0x2000) / 32, data, mem_mask);
				scc68070_regs.dma.channel[(offset - 0x2000) / 32].channel_status &= ~(data & 0xb0);
			}
			break;
		case 0x4004/2:
		case 0x4044/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: DMA(%d) Operation Control Register: %04x & %04x\n", (offset - 0x2000) / 32, data, mem_mask);
				scc68070_regs.dma.channel[(offset - 0x2000) / 32].operation_control = data & 0x00ff;
			}
			if(ACCESSING_BITS_8_15)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: DMA(%d) Device Control Register: %04x & %04x\n", (offset - 0x2000) / 32, data, mem_mask);
				scc68070_regs.dma.channel[(offset - 0x2000) / 32].device_control = data >> 8;
			}
			break;
		case 0x4006/2:
		case 0x4046/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: DMA(%d) Channel Control Register: %04x & %04x\n", (offset - 0x2000) / 32, data, mem_mask);
				scc68070_regs.dma.channel[(offset - 0x2000) / 32].channel_control = data & 0x007f;
				if(data & CCR_SO)
				{
					scc68070_regs.dma.channel[(offset - 0x2000) / 32].channel_status |= CSR_COC;
				}
			}
			if(ACCESSING_BITS_8_15)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: DMA(%d) Sequence Control Register: %04x & %04x\n", (offset - 0x2000) / 32, data, mem_mask);
				scc68070_regs.dma.channel[(offset - 0x2000) / 32].sequence_control = data >> 8;
			}
			break;
		case 0x400a/2:
			verboselog(space->machine, 2, "scc68070_periphs_w: DMA(%d) Memory Transfer Counter: %04x & %04x\n", (offset - 0x2000) / 32, data, mem_mask);
			COMBINE_DATA(&scc68070_regs.dma.channel[(offset - 0x2000) / 32].transfer_counter);
			break;
		case 0x400c/2:
		case 0x404c/2:
			verboselog(space->machine, 2, "scc68070_periphs_w: DMA(%d) Memory Address Counter (High Word): %04x & %04x\n", (offset - 0x2000) / 32, data, mem_mask);
			scc68070_regs.dma.channel[(offset - 0x2000) / 32].memory_address_counter &= ~(mem_mask << 16);
			scc68070_regs.dma.channel[(offset - 0x2000) / 32].memory_address_counter |= data << 16;
			break;
		case 0x400e/2:
		case 0x404e/2:
			verboselog(space->machine, 2, "scc68070_periphs_w: DMA(%d) Memory Address Counter (Low Word): %04x & %04x\n", (offset - 0x2000) / 32, data, mem_mask);
			scc68070_regs.dma.channel[(offset - 0x2000) / 32].memory_address_counter &= ~mem_mask;
			scc68070_regs.dma.channel[(offset - 0x2000) / 32].memory_address_counter |= data;
			break;
		case 0x4014/2:
		case 0x4054/2:
			verboselog(space->machine, 2, "scc68070_periphs_w: DMA(%d) Device Address Counter (High Word): %04x & %04x\n", (offset - 0x2000) / 32, data, mem_mask);
			scc68070_regs.dma.channel[(offset - 0x2000) / 32].device_address_counter &= ~(mem_mask << 16);
			scc68070_regs.dma.channel[(offset - 0x2000) / 32].device_address_counter |= data << 16;
			break;
		case 0x4016/2:
		case 0x4056/2:
			verboselog(space->machine, 2, "scc68070_periphs_w: DMA(%d) Device Address Counter (Low Word): %04x & %04x\n", (offset - 0x2000) / 32, data, mem_mask);
			scc68070_regs.dma.channel[(offset - 0x2000) / 32].device_address_counter &= ~mem_mask;
			scc68070_regs.dma.channel[(offset - 0x2000) / 32].device_address_counter |= data;
			break;

		// MMU: 80008000 to 8000807f
		case 0x8000/2:	// Status / Control register
			if(ACCESSING_BITS_0_7)
			{	// Control
				verboselog(space->machine, 2, "scc68070_periphs_w: MMU Control: %04x & %04x\n", data, mem_mask);
				scc68070_regs.mmu.control = data & 0x00ff;
			}	// Status
			else
			{
				verboselog(space->machine, 0, "scc68070_periphs_w: MMU Status (invalid): %04x & %04x\n", data, mem_mask);
			}
			break;
		case 0x8040/2:
		case 0x8048/2:
		case 0x8050/2:
		case 0x8058/2:
		case 0x8060/2:
		case 0x8068/2:
		case 0x8070/2:
		case 0x8078/2:	// Attributes (SD0-7)
			verboselog(space->machine, 2, "scc68070_periphs_w: MMU descriptor %d attributes: %04x & %04x\n", (offset - 0x4020) / 4, data, mem_mask);
			COMBINE_DATA(&scc68070_regs.mmu.desc[(offset - 0x4020) / 4].attr);
			break;
		case 0x8042/2:
		case 0x804a/2:
		case 0x8052/2:
		case 0x805a/2:
		case 0x8062/2:
		case 0x806a/2:
		case 0x8072/2:
		case 0x807a/2:	// Segment Length (SD0-7)
			verboselog(space->machine, 2, "scc68070_periphs_w: MMU descriptor %d length: %04x & %04x\n", (offset - 0x4020) / 4, data, mem_mask);
			COMBINE_DATA(&scc68070_regs.mmu.desc[(offset - 0x4020) / 4].length);
			break;
		case 0x8044/2:
		case 0x804c/2:
		case 0x8054/2:
		case 0x805c/2:
		case 0x8064/2:
		case 0x806c/2:
		case 0x8074/2:
		case 0x807c/2:	// Segment Number (SD0-7, A0=1 only)
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: MMU descriptor %d segment: %04x & %04x\n", (offset - 0x4020) / 4, data, mem_mask);
				scc68070_regs.mmu.desc[(offset - 0x4020) / 4].segment = data & 0x00ff;
			}
			break;
		case 0x8046/2:
		case 0x804e/2:
		case 0x8056/2:
		case 0x805e/2:
		case 0x8066/2:
		case 0x806e/2:
		case 0x8076/2:
		case 0x807e/2:	// Base Address (SD0-7)
			verboselog(space->machine, 2, "scc68070_periphs_w: MMU descriptor %d base: %04x & %04x\n", (offset - 0x4020) / 4, data, mem_mask);
			COMBINE_DATA(&scc68070_regs.mmu.desc[(offset - 0x4020) / 4].base);
			break;
		default:
			verboselog(space->machine, 0, "scc68070_periphs_w: Unknown address: %04x = %04x & %04x\n", offset * 2, data, mem_mask);
			break;
	}
}

#if ENABLE_UART_PRINTING
static READ16_HANDLER( uart_loopback_enable )
{
	return 0x1234;
}
#endif

typedef struct
{
	UINT16 busy_clear_countdown;
	UINT16 command; 			// CDIC Command Register (0x303c00)
	UINT32 time;				// CDIC Time Register (0x303c02)
	UINT16 file;				// CDIC File Register (0x303c06)
	UINT32 channel; 			// CDIC Channel Register (0x303c08)
	UINT16 audio_channel;		// CDIC Audio Channel Register (0x303c0c)

	UINT16 audio_buffer;		// CDIC Audio Buffer Register (0x303ff4)
	UINT16 x_buffer;			// CDIC X-Buffer Register (0x303ff6)
	UINT16 dma_control;			// CDIC DMA Control Register (0x303ff8)
	UINT16 z_buffer;			// CDIC Z-Buffer Register (0x303ffa)
	UINT16 interrupt_vector;	// CDIC Interrupt Vector Register (0x303ffc)
	UINT16 data_buffer;			// CDIC Data Buffer Register (0x303ffe)

	UINT8 buffer_target;

	emu_timer *interrupt_timer;
	cdrom_file *cd;
} cdic_regs_t;

static cdic_regs_t cdic_regs;

static TIMER_CALLBACK( cdic_trigger_readback_int )
{
	switch(cdic_regs.command)
	{
		case 0x29: // Read Mode 1
		case 0x2a: // Read Mode 2
		{
			UINT8 buffer[2560] = { 0 };
			UINT32 msf = cdic_regs.time >> 8;
			UINT32 lba = 0;
			int index = 0;
			UINT8 nybbles[6] =
			{
				 msf & 0x0000000f,
				(msf & 0x000000f0) >> 4,
				(msf & 0x00000f00) >> 8,
				(msf & 0x0000f000) >> 12,
				(msf & 0x000f0000) >> 16,
				(msf & 0x00f00000) >> 20
			};
			nybbles[2] -= 2;
			if(nybbles[2] > 9)
			{
				nybbles[2] += 10;
				nybbles[3]--;
			}
			lba = nybbles[0] + nybbles[1]*10 + ((nybbles[2] + nybbles[3]*10)*75) + ((nybbles[4] + nybbles[5]*10)*75*60);

			printf( "Reading Mode %d sector from MSF location %06x\n", cdic_regs.command - 0x28, cdic_regs.time | 2 );
			verboselog(machine, 0, "Reading Mode %d sector from MSF location %06x\n", cdic_regs.command - 0x28, cdic_regs.time | 2 );

			cdic_regs.data_buffer ^= 0x0001;

			cdrom_read_data(cdic_regs.cd, lba, buffer, CD_TRACK_RAW_DONTCARE);
			for(index = 6; index < 2352/2; index++)
			{
				if(cdic_regs.time == 0x00332300 || cdic_regs.time == 0x00332400)
				{
					//printf( "%02x %02x ", buffer[index*2], buffer[index*2 + 1] );
				}
				cdram[(cdic_regs.data_buffer & 5) * (0xa00/2) + (index - 6)] = (buffer[index*2] << 8) | buffer[index*2 + 1];
			}
			//printf( "\n" );

			cdic_regs.time += 0x100;
			if((cdic_regs.time & 0x00000f00) == 0xa00)
			{
				cdic_regs.time &= 0xfffff0ff;
				cdic_regs.time += 0x00001000;
			}
			if((cdic_regs.time & 0x0000ff00) == 0x7500)
			{
				cdic_regs.time &= 0xffff00ff;
				cdic_regs.time += 0x00010000;
				if((cdic_regs.time & 0xf0000) == 0xa0000)
				{
					cdic_regs.time &= 0xfff0ffff;
					cdic_regs.time += 0x100000;
					if((cdic_regs.time & 0xf00000) == 0xf00000)
					{
						cdic_regs.time &= 0xff0fffff;
						cdic_regs.time += 0x01000000;
					}
				}
			}

			if(((buffer[22] & 0x24) == 0x24) && (cdic_regs.channel & cdic_regs.audio_channel & (1 << buffer[21])))
			{
				// Ignore the audio sector for now.
			}
			else if((buffer[22] & 0x0e) == 0x00)
			{
				// Empty message sector, ignore it
			}
			else //if((buffer[22] & 0x04) == 0x04)
			{
				cdic_regs.x_buffer |= 0x8000;

				cdic_regs.data_buffer |= 0x4000;
				cdic_regs.data_buffer &= 0x7fff;

				printf( "Setting CDIC interrupt line\n" );
				verboselog(machine, 0, "Setting CDIC interrupt line\n" );
				cpu_set_input_line_vector(cputag_get_cpu(machine, "maincpu"), M68K_IRQ_4, 128);
				cputag_set_input_line(machine, "maincpu", M68K_IRQ_4, ASSERT_LINE);
			}

			timer_adjust_oneshot(cdic_regs.interrupt_timer, ATTOTIME_IN_HZ(75), 0); // 75Hz = 1x CD-ROM speed

			break;
		}
		case 0x2e: // Abort
			timer_adjust_oneshot(cdic_regs.interrupt_timer, ATTOTIME_IN_HZ(75), 0); // 75Hz = 1x CD-ROM speed
			cdic_regs.data_buffer &= 0x3fff;
			break;
	}
}

static READ16_HANDLER( cdic_r )
{
	offset += 0x3c00/2;
	switch(offset)
	{
		case 0x3c00/2: // Command register
			verboselog(space->machine, 0, "cdic_r: Command Register = %04x & %04x\n", cdic_regs.command, mem_mask);
			return cdic_regs.command;
		case 0x3c08/2: // Channel register (MSW)
			verboselog(space->machine, 0, "cdic_r: Channel Register (MSW) = %04x & %04x\n", cdic_regs.channel >> 16, mem_mask);
			return cdic_regs.channel >> 16;
		case 0x3c0a/2: // Channel register (LSW)
			verboselog(space->machine, 0, "cdic_r: Channel Register (LSW) = %04x & %04x\n", cdic_regs.channel & 0x0000ffff, mem_mask);
			return cdic_regs.channel & 0x0000ffff;
		case 0x3c0c/2: // Audio Channel register
			verboselog(space->machine, 0, "cdic_r: Audio Channel Register = %04x & %04x\n", cdic_regs.audio_channel, mem_mask);
			return cdic_regs.audio_channel;
		case 0x3ff4/2:
			verboselog(space->machine, 0, "cdic_r: Audio Buffer Register = %04x & %04x\n", cdic_regs.audio_buffer, mem_mask);
			return cdic_regs.audio_buffer;
		case 0x3ff6/2:
		{
			UINT16 temp = cdic_regs.x_buffer;
			cdic_regs.x_buffer &= 0x7fff;
			verboselog(space->machine, 0, "cdic_r: X-Buffer Register = %04x & %04x\n", temp, mem_mask);
			return temp;
		}
		case 0x3ffa/2:
			verboselog(space->machine, 0, "cdic_r: Z-Buffer Register = %04x & %04x\n", cdic_regs.z_buffer, mem_mask);
			return cdic_regs.z_buffer;
		case 0x3ffe/2:
		{
			UINT16 temp = cdic_regs.data_buffer;
			cputag_set_input_line(space->machine, "maincpu", M68K_IRQ_4, CLEAR_LINE);
			verboselog(space->machine, 0, "cdic_r: Data buffer Register = %04x & %04x\n", temp, mem_mask);
			verboselog(space->machine, 0, "Clearing CDIC interrupt line\n" );
			printf("Clearing CDIC interrupt line\n" );
			return temp;
		}
		default:
			verboselog(space->machine, 0, "cdic_r: UNIMPLEMENTED: Unknown address: %04x & %04x\n", offset*2, mem_mask);
			return 0;
	}
}

static WRITE16_HANDLER( cdic_w )
{
	offset += 0x3c00/2;
	switch(offset)
	{
		case 0x3c00/2: // Command register
			verboselog(space->machine, 0, "cdic_w: Command Register = %04x & %04x\n", data, mem_mask);
			COMBINE_DATA(&cdic_regs.command);
			break;
		case 0x3c02/2: // Time register (MSW)
			cdic_regs.time &= ~(mem_mask << 16);
			cdic_regs.time |= (data & mem_mask) << 16;
			verboselog(space->machine, 0, "cdic_w: Time Register (MSW) = %04x & %04x\n", data, mem_mask);
			break;
		case 0x3c04/2: // Time register (LSW)
			cdic_regs.time &= ~mem_mask;
			cdic_regs.time |= data & mem_mask;
			verboselog(space->machine, 0, "cdic_w: Time Register (LSW) = %04x & %04x\n", data, mem_mask);
			break;
		case 0x3c06/2: // File register
			verboselog(space->machine, 0, "cdic_w: File Register = %04x & %04x\n", data, mem_mask);
			COMBINE_DATA(&cdic_regs.file);
			break;
		case 0x3c08/2: // Channel register (MSW)
			cdic_regs.channel &= ~(mem_mask << 16);
			cdic_regs.channel |= (data & mem_mask) << 16;
			verboselog(space->machine, 0, "cdic_w: Channel Register (MSW) = %04x & %04x\n", data, mem_mask);
			break;
		case 0x3c0a/2: // Channel register (LSW)
			cdic_regs.channel &= ~mem_mask;
			cdic_regs.channel |= data & mem_mask;
			verboselog(space->machine, 0, "cdic_w: Channel Register (LSW) = %04x & %04x\n", data, mem_mask);
			break;
		case 0x3c0c/2: // Audio Channel register
			verboselog(space->machine, 0, "cdic_w: Audio Channel Register = %04x & %04x\n", data, mem_mask);
			COMBINE_DATA(&cdic_regs.audio_channel);
			break;
		case 0x3ff8/2:
		{
			UINT32 start = scc68070_regs.dma.channel[0].memory_address_counter;
			UINT32 count = scc68070_regs.dma.channel[0].transfer_counter;
			UINT32 index = 0;
			UINT32 src_index = (data & 0x7fff) >> 1;
			UINT16 *memory = planea;
			verboselog(space->machine, 0, "memory address counter: %08x\n", scc68070_regs.dma.channel[0].memory_address_counter);
			verboselog(space->machine, 0, "cdic_w: DMA Control Register = %04x & %04x\n", data, mem_mask);
			verboselog(space->machine, 0, "Doing copy, transferring %04x bytes\n", count * 2 );
			printf("Doing copy, transferring %04x bytes\n", count * 2 );
			if((start & 0x00f00000) == 0x00200000)
			{
				start -= 0x00200000;
				memory = planeb;
			}
			for(index = start / 2; index < (start / 2 + count); index++)
			{
				memory[index] = cdram[src_index++];
				if(cdic_regs.time == 0x00332400 || cdic_regs.time == 0x00332500)
				{
					printf( "%04x ", cdram[src_index - 1] );
				}
			}
			printf( "\n" );
			scc68070_regs.dma.channel[0].memory_address_counter += scc68070_regs.dma.channel[0].transfer_counter * 2;
			break;
		}
		case 0x3ffa/2:
			verboselog(space->machine, 0, "cdic_w: Z-Buffer Register = %04x & %04x\n", data, mem_mask);
			COMBINE_DATA(&cdic_regs.z_buffer);
			break;
		case 0x3ffc/2:
			verboselog(space->machine, 0, "cdic_w: Interrupt Vector Register = %04x & %04x\n", data, mem_mask);
			COMBINE_DATA(&cdic_regs.interrupt_vector);
			break;
		case 0x3ffe/2:
		{
			verboselog(space->machine, 0, "cdic_w: Data Buffer Register = %04x & %04x\n", data, mem_mask);
			COMBINE_DATA(&cdic_regs.data_buffer);
			cdic_regs.data_buffer &= 0xbfff;
			if(cdic_regs.data_buffer & 0x8000)
			{
				switch(cdic_regs.command)
				{
					case 0x23: // Reset Mode 1
					case 0x24: // Reset Mode 2
						timer_adjust_oneshot(cdic_regs.interrupt_timer, attotime_never, 0);
						cdic_regs.data_buffer &= 0x3fff;
						break;
					case 0x29: // Read Mode 1
					case 0x2a: // Read Mode 2
					case 0x2e: // Abort
						timer_adjust_oneshot(cdic_regs.interrupt_timer, ATTOTIME_IN_HZ(75), 0); // 75Hz = 1x CD-ROM speed
						break;
				}
			}
			break;
		}
		default:
			verboselog(space->machine, 0, "cdic_w: UNIMPLEMENTED: Unknown address: %04x = %04x & %04x\n", offset*2, data, mem_mask);
			break;
	}
}

typedef struct
{
	UINT8 out_buf[4];
	UINT8 out_index;
	UINT8 out_count;
	UINT8 out_cmd;
} slave_channel_t;

typedef struct
{
	slave_channel_t channel[4];
	emu_timer *interrupt_timer;

	UINT8 in_buf[17];
	UINT8 in_index;
	UINT8 in_count;

	UINT8 polling_active;

	UINT8 xbus_interrupt_enable;

	UINT8 lcd_state[16];
} slave_regs_t;

static slave_regs_t slave_regs;

static TIMER_CALLBACK( slave_trigger_readback_int )
{
	verboselog(machine, 0, "Asserting IRQ2\n" );
	cpu_set_input_line_vector(cputag_get_cpu(machine, "maincpu"), M68K_IRQ_2, 26);
	cputag_set_input_line(machine, "maincpu", M68K_IRQ_2, ASSERT_LINE);
	timer_adjust_oneshot(slave_regs.interrupt_timer, attotime_never, 0);
}

static void slave_prepare_readback(running_machine *machine, attotime delay, UINT8 channel, UINT8 count, UINT8 data0, UINT8 data1, UINT8 data2, UINT8 data3, UINT8 cmd)
{
	slave_regs.channel[channel].out_index = 0;
	slave_regs.channel[channel].out_count = count;
	slave_regs.channel[channel].out_buf[0] = data0;
	slave_regs.channel[channel].out_buf[1] = data1;
	slave_regs.channel[channel].out_buf[2] = data2;
	slave_regs.channel[channel].out_buf[3] = data3;
	slave_regs.channel[channel].out_cmd = cmd;

	timer_adjust_oneshot(slave_regs.interrupt_timer, delay, 0);
}

static void perform_mouse_update(running_machine *machine)
{
	UINT16 x = input_port_read(machine, "MOUSEX");
	UINT16 y = input_port_read(machine, "MOUSEY");
	UINT8 buttons = input_port_read(machine, "MOUSEBTN");

	if(slave_regs.polling_active)
	{
		slave_prepare_readback(machine, ATTOTIME_IN_HZ(10000), 0, 4, ((x & 0x380) >> 7) | (buttons << 4), x & 0x7f, (y & 0x380) >> 7, y & 0x7f, 0xf7);
	}
}

static INPUT_CHANGED( mouse_update )
{
	perform_mouse_update(field->port->machine);
}

static READ16_HANDLER( slave_r )
{
	if(slave_regs.channel[offset].out_count)
	{
		UINT8 ret = slave_regs.channel[offset].out_buf[slave_regs.channel[offset].out_index];
		verboselog(space->machine, 0, "slave_r: Channel %d: %d, %02x\n", offset, slave_regs.channel[offset].out_index, ret );
		if(slave_regs.channel[offset].out_index == 0)
		{
			switch(slave_regs.channel[offset].out_cmd)
			{
				case 0xb0:
				case 0xb1:
				case 0xf0:
				case 0xf7:
					verboselog(space->machine, 0, "slave_r: De-asserting IRQ2\n" );
					cputag_set_input_line(space->machine, "maincpu", M68K_IRQ_2, CLEAR_LINE);
					break;
			}
		}
		slave_regs.channel[offset].out_index++;
		slave_regs.channel[offset].out_count--;
		if(!slave_regs.channel[offset].out_count)
		{
			slave_regs.channel[offset].out_index = 0;
			slave_regs.channel[offset].out_cmd = 0;
			memset(slave_regs.channel[offset].out_buf, 0, 4);
		}
		return ret;
	}
	verboselog(space->machine, 0, "slave_r: Channel %d: %d\n", offset, slave_regs.channel[offset].out_index );
	return 0xff;
}

static WRITE16_HANDLER( slave_w )
{
	switch(offset)
	{
		case 0:
			if(slave_regs.in_index)
			{
				verboselog(space->machine, 0, "slave_w: Channel %d: %d = %02x\n", offset, slave_regs.in_index, data & 0x00ff );
				slave_regs.in_buf[slave_regs.in_index] = data & 0x00ff;
				slave_regs.in_index++;
				if(slave_regs.in_index == slave_regs.in_count)
				{
					switch(slave_regs.in_buf[0])
					{
						case 0xc0: case 0xc1: case 0xc2: case 0xc3: case 0xc4: case 0xc5: case 0xc6: case 0xc7:
						case 0xc8: case 0xc9: case 0xca: case 0xcb: case 0xcc: case 0xcd: case 0xce: case 0xcf:
						case 0xd0: case 0xd1: case 0xd2: case 0xd3: case 0xd4: case 0xd5: case 0xd6: case 0xd7:
						case 0xd8: case 0xd9: case 0xda: case 0xdb: case 0xdc: case 0xdd: case 0xde: case 0xdf:
						case 0xe0: case 0xe1: case 0xe2: case 0xe3: case 0xe4: case 0xe5: case 0xe6: case 0xe7:
						case 0xe8: case 0xe9: case 0xea: case 0xeb: case 0xec: case 0xed: case 0xee: case 0xef:
						case 0xf0: case 0xf1: case 0xf2: case 0xf3: case 0xf4: case 0xf5: case 0xf6: case 0xf7:
						case 0xf8: case 0xf9: case 0xfa: case 0xfb: case 0xfc: case 0xfd: case 0xfe: case 0xff:	// Update Mouse Position
							//perform_mouse_update(space->machine);
							memset(slave_regs.in_buf, 0, 17);
							slave_regs.in_index = 0;
							slave_regs.in_count = 0;
							break;
					}
				}
			}
			else
			{
				slave_regs.in_buf[slave_regs.in_index] = data & 0x00ff;
				slave_regs.in_index++;
				switch(data & 0x00ff)
				{
					case 0xc0: case 0xc1: case 0xc2: case 0xc3: case 0xc4: case 0xc5: case 0xc6: case 0xc7:
					case 0xc8: case 0xc9: case 0xca: case 0xcb: case 0xcc: case 0xcd: case 0xce: case 0xcf:
					case 0xd0: case 0xd1: case 0xd2: case 0xd3: case 0xd4: case 0xd5: case 0xd6: case 0xd7:
					case 0xd8: case 0xd9: case 0xda: case 0xdb: case 0xdc: case 0xdd: case 0xde: case 0xdf:
					case 0xe0: case 0xe1: case 0xe2: case 0xe3: case 0xe4: case 0xe5: case 0xe6: case 0xe7:
					case 0xe8: case 0xe9: case 0xea: case 0xeb: case 0xec: case 0xed: case 0xee: case 0xef:
					case 0xf0: case 0xf1: case 0xf2: case 0xf3: case 0xf4: case 0xf5: case 0xf6: case 0xf7:
					case 0xf8: case 0xf9: case 0xfa: case 0xfb: case 0xfc: case 0xfd: case 0xfe: case 0xff:
						verboselog(space->machine, 0, "slave_w: Channel %d: Update Mouse Position (0x%02x)\n", offset, data & 0x00ff );
						slave_regs.in_count = 3;
						break;
					default:
						verboselog(space->machine, 0, "slave_w: Channel %d: Unknown register: %02x\n", offset, data & 0x00ff );
						slave_regs.in_index = 0;
						break;
				}
			}
			break;
		case 1:
			if(slave_regs.in_index)
			{
				verboselog(space->machine, 0, "slave_w: Channel %d: %d = %02x\n", offset, slave_regs.in_index, data & 0x00ff );
				slave_regs.in_buf[slave_regs.in_index] = data & 0x00ff;
				slave_regs.in_index++;
				if(slave_regs.in_index == slave_regs.in_count)
				{
					switch(slave_regs.in_buf[0])
					{
						case 0xf0: // Set Front Panel LCD
							memcpy(slave_regs.lcd_state, slave_regs.in_buf + 1, 16);
							memset(slave_regs.in_buf, 0, 17);
							slave_regs.in_index = 0;
							slave_regs.in_count = 0;
							break;
						default:
							memset(slave_regs.in_buf, 0, 17);
							slave_regs.in_index = 0;
							slave_regs.in_count = 0;
							break;
					}
				}
			}
			else
			{
				switch(data & 0x00ff)
				{
					default:
						verboselog(space->machine, 0, "slave_w: Channel %d: Unknown register: %02x\n", offset, data & 0x00ff );
						memset(slave_regs.in_buf, 0, 17);
						slave_regs.in_index = 0;
						slave_regs.in_count = 0;
						break;
				}
			}
			break;
		case 2:
			if(slave_regs.in_index)
			{
				verboselog(space->machine, 0, "slave_w: Channel %d: %d = %02x\n", offset, slave_regs.in_index, data & 0x00ff );
				slave_regs.in_buf[slave_regs.in_index] = data & 0x00ff;
				slave_regs.in_index++;
				if(slave_regs.in_index == slave_regs.in_count)
				{
					switch(slave_regs.in_buf[0])
					{
						case 0xf0: // Set Front Panel LCD
							memset(slave_regs.in_buf + 1, 0, 16);
							slave_regs.in_count = 17;
							break;
						default:
							memset(slave_regs.in_buf, 0, 17);
							slave_regs.in_index = 0;
							slave_regs.in_count = 0;
							break;
					}
				}
			}
			else
			{
				slave_regs.in_buf[slave_regs.in_index] = data & 0x00ff;
				slave_regs.in_index++;
				switch(data & 0x00ff)
				{
					case 0xf0: // Set Front Panel LCD
						verboselog(space->machine, 0, "slave_w: Channel %d: Set Front Panel LCD (0xf0)\n", offset );
						slave_regs.in_count = 17;
						break;
					default:
						verboselog(space->machine, 0, "slave_w: Channel %d: Unknown register: %02x\n", offset, data & 0x00ff );
						memset(slave_regs.in_buf, 0, 17);
						slave_regs.in_index = 0;
						slave_regs.in_count = 0;
						break;
				}
			}
			break;
		case 3:
			if(slave_regs.in_index)
			{
				verboselog(space->machine, 0, "slave_w: Channel %d: %d = %02x\n", offset, slave_regs.in_index, data & 0x00ff );
				slave_regs.in_buf[slave_regs.in_index] = data & 0x00ff;
				slave_regs.in_index++;
				if(slave_regs.in_index == slave_regs.in_count)
				{
					switch(slave_regs.in_buf[0])
					{
						case 0xb0: // Request Disc Status
							memset(slave_regs.in_buf, 0, 17);
							slave_regs.in_index = 0;
							slave_regs.in_count = 0;
							slave_prepare_readback(space->machine, ATTOTIME_IN_HZ(4), 3, 4, 0xb0, 0x00, 0x02, 0x15, 0xb0);
							break;
						//case 0xb1: // Request Disc Base
							//memset(slave_regs.in_buf, 0, 17);
							//slave_regs.in_index = 0;
							//slave_regs.in_count = 0;
							//slave_prepare_readback(space->machine, ATTOTIME_IN_HZ(10000), 3, 4, 0xb1, 0x00, 0x02, 0x00, 0xb1);
							break;
						default:
							memset(slave_regs.in_buf, 0, 17);
							slave_regs.in_index = 0;
							slave_regs.in_count = 0;
							break;
					}
				}
			}
			else
			{
				slave_regs.in_buf[slave_regs.in_index] = data & 0x00ff;
				slave_regs.in_index++;
				switch(data & 0x00ff)
				{
					case 0xb0: // Request Disc Status
						verboselog(space->machine, 0, "slave_w: Channel %d: Request Disc Status (0xb0)\n", offset );
						slave_regs.in_count = 4;
						break;
					case 0xb1: // Request Disc Status
						verboselog(space->machine, 0, "slave_w: Channel %d: Request Disc Base (0xb1)\n", offset );
						slave_regs.in_count = 4;
						break;
					case 0xf0: // Get SLAVE revision
						verboselog(space->machine, 0, "slave_w: Channel %d: Get SLAVE Revision (0xf0)\n", offset );
						slave_regs.in_index = 0;
						slave_regs.in_buf[0] = 0;
						slave_regs.channel[0].out_index = 0;
						slave_regs.channel[0].out_count = 2;
						slave_regs.channel[0].out_buf[0] = 0x32;
						slave_regs.channel[0].out_buf[1] = 0x31;
						slave_regs.channel[0].out_cmd = 0xf0;
						slave_prepare_readback(space->machine, ATTOTIME_IN_HZ(10000), 2, 2, 0xf6, 1, 0, 0, 0xf0);
						break;
					case 0xf6: // NTSC/PAL
						verboselog(space->machine, 0, "slave_w: Channel %d: Check NTSC/PAL (0xf6)\n", offset );
						slave_prepare_readback(space->machine, attotime_never, 2, 2, 0xf6, 1, 0, 0, 0xf6);
						slave_regs.in_index = 0;
						break;
					case 0xf7: // Activate input polling
						verboselog(space->machine, 0, "slave_w: Channel %d: Activate Input Polling (0xf7)\n", offset );
						slave_regs.polling_active = 1;
						slave_regs.in_index = 0;
						break;
					case 0xfa: // Enable X-Bus interrupts
						verboselog(space->machine, 0, "slave_w: Channel %d: X-Bus Interrupt Enable (0xfa)\n", offset );
						slave_regs.xbus_interrupt_enable = 1;
						slave_regs.in_index = 0;
						break;
					default:
						verboselog(space->machine, 0, "slave_w: Channel %d: Unknown register: %02x\n", offset, data & 0x00ff );
						memset(slave_regs.in_buf, 0, 17);
						slave_regs.in_index = 0;
						slave_regs.in_count = 0;
						break;
				}
			}
			break;
	}
}

typedef struct
{
	UINT8 nvram[0x2000];
	emu_timer *clock;
} m48t08_regs_t;

m48t08_regs_t m48t08;

static READ16_HANDLER( m48t08_r )
{
	verboselog(space->machine, 11, "m48t08_r: %04x = %02x & %04x\n", offset, m48t08.nvram[offset], mem_mask );
	return m48t08.nvram[offset] << 8;
}

static WRITE16_HANDLER( m48t08_w )
{
	verboselog(space->machine, 11, "m48t08_w: %04x = %02x\n", offset, data >> 8);
	switch(offset)
	{
		case 0x1ff9:
			if(m48t08.nvram[0x1ff8] & 0x80)
			{
				m48t08.nvram[0x1ff9] = (data >> 8) & 0x00ff;
			}
			break;
		case 0x1ffa:
			if(m48t08.nvram[0x1ff8] & 0x80)
			{
				m48t08.nvram[0x1ffa] = (data >> 8) & 0x7f;
			}
			break;
		case 0x1ffb:
			if(m48t08.nvram[0x1ff8] & 0x80)
			{
				m48t08.nvram[0x1ffb] = (data >> 8) & 0x003f;
			}
			break;
		case 0x1ffc:
			if(m48t08.nvram[0x1ff8] & 0x80)
			{
				m48t08.nvram[0x1ffc] = (data >> 8) & 0x0047;
			}
			break;
		case 0x1ffd:
			if(m48t08.nvram[0x1ff8] & 0x80)
			{
				m48t08.nvram[0x1ffd] = (data >> 8) & 0x003f;
			}
			break;
		case 0x1ffe:
			if(m48t08.nvram[0x1ff8] & 0x80)
			{
				m48t08.nvram[0x1ffe] = (data >> 8) & 0x001f;
			}
			break;
		case 0x1fff:
			if(m48t08.nvram[0x1ff8] & 0x80)
			{
				m48t08.nvram[0x1fff] = (data >> 8) & 0x00ff;
			}
			break;
		default:
			m48t08.nvram[offset] = (data & mem_mask) >> 8;
			break;
	}
}

/*************************
*     Video Hardware     *
*************************/

typedef struct
{
	UINT8 csrr;
	UINT16 csrw;
	UINT16 dcr;
	UINT16 vsr;
	UINT16 ddr;
	UINT16 dcp;
	UINT32 dca;
	UINT32 clut[256];
	UINT32 image_coding_method;
	UINT32 transparency_control;
	UINT32 plane_order;
	UINT32 clut_bank;
	UINT32 transparent_color_a;
	UINT32 reserved0;
	UINT32 transparent_color_b;
	UINT32 mask_color_a;
	UINT32 reserved1;
	UINT32 mask_color_b;
	UINT32 dyuv_abs_start_a;
	UINT32 dyuv_abs_start_b;
	UINT32 reserved2;
	UINT32 cursor_position;
	UINT32 cursor_control;
	UINT32 cursor_pattern[16];
	UINT32 region_control[8];
	UINT32 backdrop_color;
	UINT32 mosaic_hold_a;
	UINT32 mosaic_hold_b;
	UINT32 weight_factor_a;
	UINT32 weight_factor_b;
} mcd212_channel_t;

typedef struct
{
	mcd212_channel_t channel[2];
	emu_timer *scan_timer;
} mcd212_t;

mcd212_t mcd212;

#define MCD212_CURCNT_COLOR			0x00000f	// Cursor color
#define MCD212_CURCNT_CUW			0x008000	// Cursor width
#define MCD212_CURCNT_COF			0x070000	// Cursor off time
#define MCD212_CURCNT_COF_SHIFT		16
#define MCD212_CURCNT_CON			0x280000	// Cursor on time
#define MCD212_CURCNT_CON_SHIFT		19
#define MCD212_CURCNT_BLKC			0x400000	// Blink type
#define MCD212_CURCNT_EN			0x800000	// Cursor enable

#define MCD212_TCR_DISABLE_MX		0x800000	// Mix disable
#define MCD212_TCR_TB				0x000f00	// Plane B
#define MCD212_TCR_TB_SHIFT			8
#define MCD212_TCR_TA				0x00000f	// Plane A
#define MCD212_TCR_COND_1			0x0			// Transparent if: Always (Plane Disabled)
#define MCD212_TCR_COND_KEY_1		0x1			// Transparent if: Color Key = True
#define MCD212_TCR_COND_XLU_1		0x2			// Transparent if: Transparency Bit = 1
#define MCD212_TCR_COND_RF0_1		0x3			// Transparent if: Region Flag 0 = True
#define MCD212_TCR_COND_RF1_1		0x4			// Transparent if: Region Flag 1 = True
#define MCD212_TCR_COND_RF0KEY_1	0x5			// Transparent if: Region Flag 0 = True || Color Key = True
#define MCD212_TCR_COND_RF1KEY_1	0x6			// Transparent if: Region Flag 1 = True || Color Key = True
#define MCD212_TCR_COND_UNUSED0		0x7			// Unused
#define MCD212_TCR_COND_0			0x8			// Transparent if: Never (No Transparent Area)
#define MCD212_TCR_COND_KEY_0		0x9			// Transparent if: Color Key = False
#define MCD212_TCR_COND_XLU_0		0xa			// Transparent if: Transparency Bit = 0
#define MCD212_TCR_COND_RF0_0		0xb			// Transparent if: Region Flag 0 = False
#define MCD212_TCR_COND_RF1_0		0xc			// Transparent if: Region Flag 1 = False
#define MCD212_TCR_COND_RF0KEY_0	0xd			// Transparent if: Region Flag 0 = False && Color Key = False
#define MCD212_TCR_COND_RF1KEY_0	0xe			// Transparent if: Region Flag 1 = False && Color Key = False
#define MCD212_TCR_COND_UNUSED1		0xf			// Unused

#define MCD212_POR_AB				0			// Plane A in front of Plane B
#define MCD212_POR_BA				1			// Plane B in front of Plane A

#define MCD212_CSR2R_IT1			0x0004	// Interrupt 1
#define MCD212_CSR2R_IT2			0x0002	// Interrupt 2
#define MCD212_CSR2R_BE				0x0001	// Bus Error

#define MCD212_DCR_DE				0x8000	// Display Enable
#define MCD212_DCR_CF				0x4000	// Crystal Frequency
#define MCD212_DCR_FD				0x2000	// Frame Duration
#define MCD212_DCR_SM				0x1000	// Scan Mode
#define MCD212_DCR_CM				0x0800	// Color Mode Ch.1/2
#define MCD212_DCR_ICA				0x0200	// ICA Enable Ch.1/2
#define MCD212_DCR_DCA				0x0100	// DCA Enable Ch.1/2

#define MCD212_DDR_FT				0x0300	// Display File Type
#define MCD212_DDR_FT_BMP			0x0000	// Bitmap
#define MCD212_DDR_FT_BMP2			0x0100	// Bitmap (alt.)
#define MCD212_DDR_FT_RLE			0x0200	// Run-Length Encoded
#define MCD212_DDR_FT_MOSAIC		0x0300	// Mosaic

static const UINT16 cdi220_lcd_char[20*22] =
{
	0x2000, 0x2000, 0x2000, 0x2000, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0200, 0x0200, 0x0200, 0x0200,
	0x2000, 0x2000, 0x2000, 0x2000, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0200, 0x0200, 0x0200, 0x0200,
	0x2000, 0x2000, 0x2000, 0x2000, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0200, 0x0200, 0x0200, 0x0200,
	0x2000, 0x2000, 0x2000, 0x2000, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0200, 0x0200, 0x0200, 0x0200,
	0x2000, 0x2000, 0x2000, 0x2000, 0x8000, 0x8000, 0x0000, 0x0000, 0x0001, 0x0001, 0x0001, 0x0001, 0x0000, 0x0000, 0x0002, 0x0002, 0x0200, 0x0200, 0x0200, 0x0200,
	0x2000, 0x2000, 0x2000, 0x2000, 0x8000, 0x8000, 0x8000, 0x0000, 0x0001, 0x0001, 0x0001, 0x0001, 0x0000, 0x0002, 0x0002, 0x0002, 0x0200, 0x0200, 0x0200, 0x0200,
	0x2000, 0x2000, 0x2000, 0x2000, 0x8000, 0x8000, 0x8000, 0x8000, 0x0001, 0x0001, 0x0001, 0x0001, 0x0002, 0x0002, 0x0002, 0x0002, 0x0200, 0x0200, 0x0200, 0x0200,
	0x2000, 0x2000, 0x2000, 0x2000, 0x0000, 0x8000, 0x8000, 0x8000, 0x0001, 0x0001, 0x0001, 0x0001, 0x0002, 0x0002, 0x0002, 0x0000, 0x0200, 0x0200, 0x0200, 0x0200,
	0x2000, 0x2000, 0x2000, 0x2000, 0x0000, 0x0000, 0x8000, 0x8000, 0x0001, 0x0001, 0x0001, 0x0001, 0x0002, 0x0002, 0x0000, 0x0000, 0x0200, 0x0200, 0x0200, 0x0200,
	0x2000, 0x2000, 0x2000, 0x2000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x0200, 0x0200, 0x0200, 0x0200,
	0x2000, 0x2000, 0x2000, 0x2000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x0200, 0x0200, 0x0200, 0x0200,
	0x1000, 0x1000, 0x1000, 0x1000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x0400, 0x0400, 0x0400, 0x0400,
	0x1000, 0x1000, 0x1000, 0x1000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x0400, 0x0400, 0x0400, 0x0400,
	0x1000, 0x1000, 0x1000, 0x1000, 0x0000, 0x0000, 0x0010, 0x0010, 0x0001, 0x0001, 0x0001, 0x0001, 0x0008, 0x0008, 0x0000, 0x0000, 0x0400, 0x0400, 0x0400, 0x0400,
	0x1000, 0x1000, 0x1000, 0x1000, 0x0000, 0x0010, 0x0010, 0x0010, 0x0001, 0x0001, 0x0001, 0x0001, 0x0008, 0x0008, 0x0008, 0x0000, 0x0400, 0x0400, 0x0400, 0x0400,
	0x1000, 0x1000, 0x1000, 0x1000, 0x0010, 0x0010, 0x0010, 0x0010, 0x0001, 0x0001, 0x0001, 0x0001, 0x0008, 0x0008, 0x0008, 0x0008, 0x0400, 0x0400, 0x0400, 0x0400,
	0x1000, 0x1000, 0x1000, 0x1000, 0x0010, 0x0010, 0x0010, 0x0000, 0x0001, 0x0001, 0x0001, 0x0001, 0x0000, 0x0008, 0x0008, 0x0008, 0x0400, 0x0400, 0x0400, 0x0400,
	0x1000, 0x1000, 0x1000, 0x1000, 0x0010, 0x0010, 0x0000, 0x0000, 0x0001, 0x0001, 0x0001, 0x0001, 0x0000, 0x0000, 0x0008, 0x0008, 0x0400, 0x0400, 0x0400, 0x0400,
	0x1000, 0x1000, 0x1000, 0x1000, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0400, 0x0400, 0x0400, 0x0400,
	0x1000, 0x1000, 0x1000, 0x1000, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0400, 0x0400, 0x0400, 0x0400,
	0x1000, 0x1000, 0x1000, 0x1000, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0400, 0x0400, 0x0400, 0x0400,
	0x1000, 0x1000, 0x1000, 0x1000, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0400, 0x0400, 0x0400, 0x0400
};

static void cdi220_draw_lcd(running_machine *machine, int y)
{
	bitmap_t *bitmap = tmpbitmap;
	UINT32 *scanline = BITMAP_ADDR32(bitmap, y, 0);
	int x = 0;
	int lcd = 0;
	for(lcd = 0; lcd < 8; lcd++)
	{
		UINT16 data = (slave_regs.lcd_state[lcd*2] << 8) | slave_regs.lcd_state[lcd*2 + 1];
		for(x = 0; x < 20; x++)
		{
			if(data & cdi220_lcd_char[y*20 + x])
			{
				scanline[(7 - lcd)*24 + x] = 0x00ffffff;
			}
			else
			{
				scanline[(7 - lcd)*24 + x] = 0;
			}
		}
	}
	for(x = 8*24; x < 768; x++)
	{
		scanline[x] = 0;
	}
}

static READ16_HANDLER(mcd212_r)
{
	UINT8 channel = 1 - (offset / 8);
	switch(offset)
	{
		case 0x00/2:
		case 0x10/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 12, "mcd212_r: Status Register %d: %02x & %04x\n", channel + 1, mcd212.channel[1 - (offset / 8)].csrr, mem_mask);
				if(channel == 0)
				{
					return mcd212.channel[0].csrr;
				}
				else
				{
					UINT8 old_csr = mcd212.channel[1].csrr;
					UINT8 interrupt1 = (scc68070_regs.lir >> 4) & 7;
					//UINT8 interrupt2 = scc68070_regs.lir & 7;
					mcd212.channel[1].csrr &= ~(MCD212_CSR2R_IT1 | MCD212_CSR2R_IT2);
					if(interrupt1)
					{
						cputag_set_input_line(space->machine, "maincpu", M68K_IRQ_1 + (interrupt1 - 1), CLEAR_LINE);
					}
					//if(interrupt2)
					//{
					//	cputag_set_input_line(space->machine, "maincpu", M68K_IRQ_1 + (interrupt2 - 1), CLEAR_LINE);
					//}
					return old_csr;
				}
			}
			else
			{
				verboselog(space->machine, 2, "mcd212_r: Unknown Register %d: %04x\n", channel + 1, mem_mask);
			}
			break;
		case 0x02/2:
		case 0x12/2:
			verboselog(space->machine, 2, "mcd212_r: Display Command Register %d: %04x & %04x\n", (1 - (offset / 8)) + 1, mcd212.channel[1 - (offset / 8)].dcr, mem_mask);
			return mcd212.channel[1 - (offset / 8)].dcr;
		case 0x04/2:
		case 0x14/2:
			verboselog(space->machine, 2, "mcd212_r: Video Start Register %d: %04x & %04x\n", (1 - (offset / 8)) + 1, mcd212.channel[1 - (offset / 8)].vsr, mem_mask);
			return mcd212.channel[1 - (offset / 8)].vsr;
		case 0x08/2:
		case 0x18/2:
			verboselog(space->machine, 2, "mcd212_r: Display Decoder Register %d: %04x & %04x\n", (1 - (offset / 8)) + 1, mcd212.channel[1 - (offset / 8)].ddr, mem_mask);
			return mcd212.channel[1 - (offset / 8)].ddr;
		case 0x0a/2:
		case 0x1a/2:
			verboselog(space->machine, 2, "mcd212_r: DCA Pointer Register %d: %04x & %04x\n", (1 - (offset / 8)) + 1, mcd212.channel[1 - (offset / 8)].dcp, mem_mask);
			return mcd212.channel[1 - (offset / 8)].dcp;
		default:
			verboselog(space->machine, 2, "mcd212_r: Unknown Register %d & %04x\n", (1 - (offset / 8)) + 1, mem_mask);
			break;
	}

	return 0;
}

static WRITE16_HANDLER(mcd212_w)
{
	switch(offset)
	{
		case 0x00/2:
		case 0x10/2:
			verboselog(space->machine, 2, "mcd212_w: Status Register %d: %04x & %04x\n", (1 - (offset / 8)) + 1, data, mem_mask);
			COMBINE_DATA(&mcd212.channel[1 - (offset / 8)].csrw);
			break;
		case 0x02/2:
		case 0x12/2:
			verboselog(space->machine, 2, "mcd212_w: Display Command Register %d: %04x & %04x\n", (1 - (offset / 8)) + 1, data, mem_mask);
			COMBINE_DATA(&mcd212.channel[1 - (offset / 8)].dcr);
			break;
		case 0x04/2:
		case 0x14/2:
			verboselog(space->machine, 2, "mcd212_w: Video Start Register %d: %04x & %04x\n", (1 - (offset / 8)) + 1, data, mem_mask);
			COMBINE_DATA(&mcd212.channel[1 - (offset / 8)].vsr);
			break;
		case 0x08/2:
		case 0x18/2:
			verboselog(space->machine, 2, "mcd212_w: Display Decoder Register %d: %04x & %04x\n", (1 - (offset / 8)) + 1, data, mem_mask);
			COMBINE_DATA(&mcd212.channel[1 - (offset / 8)].ddr);
			break;
		case 0x0a/2:
		case 0x1a/2:
			verboselog(space->machine, 2, "mcd212_w: DCA Pointer Register %d: %04x & %04x\n", (1 - (offset / 8)) + 1, data, mem_mask);
			COMBINE_DATA(&mcd212.channel[1 - (offset / 8)].dcp);
			break;
		default:
			verboselog(space->machine, 2, "mcd212_w: Unknown Register %d: %04x & %04x\n", (1 - (offset / 8)) + 1, data, mem_mask);
			break;
	}
}

INLINE void mcd212_set_register(running_machine *machine, int channel, UINT8 reg, UINT32 value)
{
	switch(reg)
	{
		case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87: // CLUT 0 - 63
		case 0x88: case 0x89: case 0x8a: case 0x8b: case 0x8c: case 0x8d: case 0x8e: case 0x8f:
		case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97:
		case 0x98: case 0x99: case 0x9a: case 0x9b: case 0x9c: case 0x9d: case 0x9e: case 0x9f:
		case 0xa0: case 0xa1: case 0xa2: case 0xa3: case 0xa4: case 0xa5: case 0xa6: case 0xa7:
		case 0xa8: case 0xa9: case 0xaa: case 0xab: case 0xac: case 0xad: case 0xae: case 0xaf:
		case 0xb0: case 0xb1: case 0xb2: case 0xb3: case 0xb4: case 0xb5: case 0xb6: case 0xb7:
		case 0xb8: case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: case 0xbe: case 0xbf:
			verboselog(machine, 11, "          %04xxxxx: %d: CLUT[%d] = %08x\n", channel * 0x20, channel, mcd212.channel[channel].clut_bank * 0x40 + (reg - 0x80), value );
			mcd212.channel[channel].clut[mcd212.channel[channel].clut_bank * 0x40 + (reg - 0x80)] = value;
			break;
		case 0xc0: // Image Coding Method
			if(channel == 0)
			{
				verboselog(machine, 11, "          %04xxxxx: %d: Image Coding Method = %08x\n", channel * 0x20, channel, value );
				mcd212.channel[channel].image_coding_method = value;
			}
			break;
		case 0xc1: // Transparency Control
			if(channel == 0)
			{
				verboselog(machine, 11, "          %04xxxxx: %d: Transparency Control = %08x\n", channel * 0x20, channel, value );
				mcd212.channel[channel].transparency_control = value;
			}
			break;
		case 0xc2: // Plane Order
			if(channel == 0)
			{
				verboselog(machine, 11, "          %04xxxxx: %d: Plane Order = %08x\n", channel * 0x20, channel, value & 7);
				mcd212.channel[channel].plane_order = value & 0x00000007;
			}
			break;
		case 0xc3: // CLUT Bank Register
			verboselog(machine, 11, "          %04xxxxx: %d: CLUT Bank Register = %08x\n", channel * 0x20, channel, value & 3);
			mcd212.channel[channel].clut_bank = value & 0x00000003;
			break;
		case 0xc4: // Transparent Color A
			if(channel == 0)
			{
				verboselog(machine, 11, "          %04xxxxx: %d: Transparent Color A = %08x\n", channel * 0x20, channel, value );
				mcd212.channel[channel].transparent_color_a = value;
			}
			break;
		case 0xc6: // Transparent Color B
			if(channel == 1)
			{
				verboselog(machine, 11, "          %04xxxxx: %d: Transparent Color B = %08x\n", channel * 0x20, channel, value );
				mcd212.channel[channel].transparent_color_b = value;
			}
			break;
		case 0xc7: // Mask Color A
			if(channel == 0)
			{
				verboselog(machine, 11, "          %04xxxxx: %d: Mask Color A = %08x\n", channel * 0x20, channel, value );
				mcd212.channel[channel].mask_color_a = value;
			}
			break;
		case 0xc9: // Mask Color B
			if(channel == 1)
			{
				verboselog(machine, 11, "          %04xxxxx: %d: Mask Color B = %08x\n", channel * 0x20, channel, value );
				mcd212.channel[channel].mask_color_b = value;
			}
			break;
		case 0xca: // Delta YUV Absolute Start Value A
			if(channel == 0)
			{
				verboselog(machine, 11, "          %04xxxxx: %d: Delta YUV Absolute Start Value A = %08x\n", channel * 0x20, channel, value );
				mcd212.channel[channel].dyuv_abs_start_a = value;
			}
			break;
		case 0xcb: // Delta YUV Absolute Start Value B
			if(channel == 1)
			{
				verboselog(machine, 11, "          %04xxxxx: %d: Delta YUV Absolute Start Value B = %08x\n", channel * 0x20, channel, value );
				mcd212.channel[channel].dyuv_abs_start_b = value;
			}
			break;
		case 0xcd: // Cursor Position
			if(channel == 0)
			{
				verboselog(machine, 11, "          %04xxxxx: %d: Cursor Position = %08x\n", channel * 0x20, channel, value );
				mcd212.channel[channel].cursor_position = value;
			}
			break;
		case 0xce: // Cursor Control
			if(channel == 0)
			{
				verboselog(machine, 11, "          %04xxxxx: %d: Cursor Control = %08x\n", channel * 0x20, channel, value );
				mcd212.channel[channel].cursor_control = value;
			}
			break;
		case 0xcf: // Cursor Pattern
			if(channel == 0)
			{
				verboselog(machine, 11, "          %04xxxxx: %d: Cursor Pattern[%d] = %04x\n", channel * 0x20, channel, (value >> 16) & 0x000f, value & 0x0000ffff);
				mcd212.channel[channel].cursor_pattern[(value >> 16) & 0x000f] = value & 0x0000ffff;
			}
			break;
		case 0xd0: // Region Control 0-7
		case 0xd1:
		case 0xd2:
		case 0xd3:
		case 0xd4:
		case 0xd5:
		case 0xd6:
		case 0xd7:
			verboselog(machine, 11, "          %04xxxxx: %d: Region Control %d = %08x\n", channel * 0x20, channel, reg & 7, value );
			mcd212.channel[channel].region_control[reg & 7] = value;
			break;
		case 0xd8: // Backdrop Color
			if(channel == 0)
			{
				verboselog(machine, 11, "          %04xxxxx: %d: Backdrop Color = %08x\n", channel * 0x20, channel, value );
				mcd212.channel[channel].backdrop_color = value;
			}
			break;
		case 0xd9: // Mosaic Pixel Hold Factor A
			if(channel == 0)
			{
				verboselog(machine, 11, "          %04xxxxx: %d: Mosaic Pixel Hold Factor A = %08x\n", channel * 0x20, channel, value );
				mcd212.channel[channel].mosaic_hold_a = value;
			}
			break;
		case 0xda: // Mosaic Pixel Hold Factor B
			if(channel == 1)
			{
				verboselog(machine, 11, "          %04xxxxx: %d: Mosaic Pixel Hold Factor B = %08x\n", channel * 0x20, channel, value );
				mcd212.channel[channel].mosaic_hold_b = value;
			}
			break;
		case 0xdb: // Weight Factor A
			if(channel == 0)
			{
				verboselog(machine, 11, "          %04xxxxx: %d: Weight Factor A = %08x\n", channel * 0x20, channel, value );
				mcd212.channel[channel].weight_factor_a = value;
			}
			break;
		case 0xdc: // Weight Factor B
			if(channel == 1)
			{
				verboselog(machine, 11, "          %04xxxxx: %d: Weight Factor B = %08x\n", channel * 0x20, channel, value );
				mcd212.channel[channel].weight_factor_b = value;
			}
			break;
	}
}

INLINE void mcd212_set_vsr(int channel, UINT32 value)
{
	mcd212.channel[channel].vsr = value & 0x0000ffff;
	mcd212.channel[channel].dcr &= 0xffc0;
	mcd212.channel[channel].dcr |= (value >> 16) & 0x003f;
}

INLINE UINT32 mcd212_get_vsr(int channel)
{
	return ((mcd212.channel[channel].dcr & 0x3f) << 16) | mcd212.channel[channel].vsr;
}

INLINE void mcd212_set_dcp(int channel, UINT32 value)
{
	mcd212.channel[channel].dcp = value & 0x0000ffff;
	mcd212.channel[channel].ddr &= 0xffc0;
	mcd212.channel[channel].ddr |= (value >> 16) & 0x003f;
}

INLINE UINT32 mcd212_get_dcp(int channel)
{
	return ((mcd212.channel[channel].ddr & 0x3f) << 16) | mcd212.channel[channel].dcp;
}

INLINE void mcd212_set_display_parameters(int channel, UINT8 value)
{
	mcd212.channel[channel].ddr &= 0xf0ff;
	mcd212.channel[channel].ddr |= (value & 0x0f) << 8;
	mcd212.channel[channel].dcr &= 0xf7ff;
	mcd212.channel[channel].dcr |= (value & 0x10) << 7;
}

static void mcd212_process_ica(running_machine *machine, int channel)
{
	UINT16 *ica = channel ? planeb : planea;
	UINT32 addr = 0x000400/2;
	UINT32 cmd = 0;
	while(1)
	{
		UINT8 stop = 0;
		cmd = ica[addr++] << 16;
		cmd |= ica[addr++];
		switch((cmd & 0xff000000) >> 24)
		{
			case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:	// STOP
			case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f:
				verboselog(machine, 11, "%08x: %08x: ICA %d: STOP\n", addr * 2 + channel * 0x200000, cmd, channel );
				stop = 1;
				break;
			case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17: // NOP
			case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
				verboselog(machine, 12, "%08x: %08x: ICA %d: NOP\n", addr * 2 + channel * 0x200000, cmd, channel );
				break;
			case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27: // RELOAD DCP
			case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f:
				verboselog(machine, 11, "%08x: %08x: ICA %d: RELOAD DCP\n", addr * 2 + channel * 0x200000, cmd, channel );
				mcd212_set_dcp(channel, cmd & 0x001fffff);
				break;
			case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37: // RELOAD DCP and STOP
			case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f:
				verboselog(machine, 11, "%08x: %08x: ICA %d: RELOAD DCP and STOP\n", addr * 2 + channel * 0x200000, cmd, channel );
				mcd212_set_dcp(channel, cmd & 0x001fffff);
				stop = 1;
				break;
			case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47: // RELOAD ICA
			case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f:
				verboselog(machine, 11, "%08x: %08x: ICA %d: RELOAD ICA\n", addr * 2 + channel * 0x200000, cmd, channel );
				addr = (cmd & 0x001fffff) / 2;
				break;
			case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57: // RELOAD VSR and STOP
			case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5e: case 0x5f:
				verboselog(machine, 11, "%08x: %08x: ICA %d: RELOAD VSR and STOP\n", addr * 2 + channel * 0x200000, cmd, channel );
				mcd212_set_vsr(channel, cmd & 0x001fffff);
				stop = 1;
				break;
			case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67: // INTERRUPT
			case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x6c: case 0x6d: case 0x6e: case 0x6f:
				verboselog(machine, 11, "%08x: %08x: ICA %d: INTERRUPT\n", addr * 2 + channel * 0x200000, cmd, channel );
				mcd212.channel[1].csrr |= 1 << (2 - channel);
				if(mcd212.channel[1].csrr & (MCD212_CSR2R_IT1 | MCD212_CSR2R_IT2))
				{
					UINT8 interrupt = (scc68070_regs.lir >> 4) & 7;
					if(interrupt)
					{
						cpu_set_input_line_vector(cputag_get_cpu(machine, "maincpu"), M68K_IRQ_1 + (interrupt - 1), 56 + interrupt);
						cputag_set_input_line(machine, "maincpu", M68K_IRQ_1 + (interrupt - 1), ASSERT_LINE);
					}
				}
				/*
				if(mcd212.channel[1].csrr & MCD212_CSR2R_IT2)
				{
					UINT8 interrupt = scc68070_regs.lir & 7;
					if(interrupt)
					{
						cpu_set_input_line_vector(cputag_get_cpu(machine, "maincpu"), M68K_IRQ_1 + (interrupt - 1), 24 + interrupt);
						cputag_set_input_line(machine, "maincpu", M68K_IRQ_1 + (interrupt - 1), ASSERT_LINE);
					}
				}
				*/
				break;
			case 0x78: case 0x79: case 0x7a: case 0x7b: case 0x7c: case 0x7d: case 0x7e: case 0x7f: // RELOAD DISPLAY PARAMETERS
				verboselog(machine, 11, "%08x: %08x: ICA %d: RELOAD DISPLAY PARAMETERS\n", addr * 2 + channel * 0x200000, cmd, channel );
				mcd212_set_display_parameters(channel, cmd & 0x1f);
				break;
			default:
				mcd212_set_register(machine, channel, cmd >> 24, cmd & 0x00ffffff);
				break;
		}
		if(stop)
		{
			break;
		}
	}
}

static void mcd212_process_dca(running_machine *machine, int channel)
{
	UINT16 *dca = channel ? planeb : planea;
	UINT32 addr = (mcd212.channel[channel].dca & 0x0007ffff) / 2; //(mcd212_get_dcp(channel) & 0x0007ffff) / 2; // mcd212.channel[channel].dca / 2;
	UINT32 cmd = 0;
	UINT32 count = 0;
	UINT32 max = 64; //(mcd212.channel[channel].dcr & MCD212_DCR_CM) ? 64 : 32;
	UINT8 addr_changed = 0;
	//printf( "max = %d\n", max );
	while(1)
	{
		UINT8 stop = 0;
		cmd = dca[addr++] << 16;
		cmd |= dca[addr++];
		count += 4;
		switch((cmd & 0xff000000) >> 24)
		{
			case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:	// STOP
			case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f:
				verboselog(machine, 11, "%08x: %08x: DCA %d: STOP\n", addr * 2 + channel * 0x200000, cmd, channel );
				stop = 1;
				break;
			case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17: // NOP
			case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
				verboselog(machine, 11, "%08x: %08x: DCA %d: NOP\n", addr * 2 + channel * 0x200000, cmd, channel );
				break;
			case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27: // RELOAD DCP
			case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f:
				verboselog(machine, 11, "%08x: %08x: DCA %d: RELOAD DCP (NOP)\n", addr * 2 + channel * 0x200000, cmd, channel );
				break;
			case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37: // RELOAD DCP and STOP
			case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f:
				verboselog(machine, 11, "%08x: %08x: DCA %d: RELOAD DCP and STOP\n", addr * 2 + channel * 0x200000, cmd, channel );
				mcd212_set_dcp(channel, cmd & 0x001fffff);
				addr = (cmd & 0x0007ffff) / 2;
				addr_changed = 1;
				stop = 1;
				break;
			case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47: // RELOAD VSR
			case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f:
				verboselog(machine, 11, "%08x: %08x: DCA %d: RELOAD VSR\n", addr * 2 + channel * 0x200000, cmd, channel );
				mcd212_set_vsr(channel, cmd & 0x001fffff);
				break;
			case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57: // RELOAD VSR and STOP
			case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5e: case 0x5f:
				verboselog(machine, 11, "%08x: %08x: DCA %d: RELOAD VSR and STOP\n", addr * 2 + channel * 0x200000, cmd, channel );
				mcd212_set_vsr(channel, cmd & 0x001fffff);
				stop = 1;
				break;
			case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67: // INTERRUPT
			case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x6c: case 0x6d: case 0x6e: case 0x6f:
				verboselog(machine, 11, "%08x: %08x: DCA %d: INTERRUPT\n", addr * 2 + channel * 0x200000, cmd, channel );
				mcd212.channel[1].csrr |= 1 << (2 - channel);
				if(mcd212.channel[1].csrr & (MCD212_CSR2R_IT1 | MCD212_CSR2R_IT2))
				{
					UINT8 interrupt = (scc68070_regs.lir >> 4) & 7;
					if(interrupt)
					{
						cpu_set_input_line_vector(cputag_get_cpu(machine, "maincpu"), M68K_IRQ_1 + (interrupt - 1), 56 + interrupt);
						cputag_set_input_line(machine, "maincpu", M68K_IRQ_1 + (interrupt - 1), ASSERT_LINE);
					}
				}
				/*
				if(mcd212.channel[1].csrr & MCD212_CSR2R_IT2)
				{
					UINT8 interrupt = scc68070_regs.lir & 7;
					if(interrupt)
					{
						cpu_set_input_line_vector(cputag_get_cpu(machine, "maincpu"), M68K_IRQ_1 + (interrupt - 1), 24 + interrupt);
						cputag_set_input_line(machine, "maincpu", M68K_IRQ_1 + (interrupt - 1), ASSERT_LINE);
					}
				}
				*/
				break;
			case 0x78: case 0x79: case 0x7a: case 0x7b: case 0x7c: case 0x7d: case 0x7e: case 0x7f: // RELOAD DISPLAY PARAMETERS
				verboselog(machine, 11, "%08x: %08x: DCA %d: RELOAD DISPLAY PARAMETERS\n", addr * 2 + channel * 0x200000, cmd, channel );
				mcd212_set_display_parameters(channel, cmd & 0x1f);
				break;
			default:
				mcd212_set_register(machine, channel, cmd >> 24, cmd & 0x00ffffff);
				break;
		}
		if(stop != 0 || count == max)
		{
			break;
		}
	}
	mcd212.channel[channel].dca = addr * 2;
}

static UINT8 mcd212_dyuv_quantizer[16] = { 0, 1, 4, 9, 16, 27, 44, 79, 128, 177, 212, 229, 240, 247, 252, 255 };

INLINE UINT8 MCD212_LIM(INT32 in)
{
    if(in < 0)
    {
        return 0;
    }
    else if(in > 255)
    {
        return 255;
    }
    return (UINT8)in;
}

static void mcd212_process_vsr(running_machine *machine, int channel, UINT32 *pixels)
{
	UINT8 *data = channel ? (UINT8*)planeb : (UINT8*)planea;
	UINT32 vsr = mcd212_get_vsr(channel) & 0x0007ffff;
	UINT8 done = 0;
	int x = 0;
	UINT32 icm_mask = channel ? 0x00000f00 : 0x0000000f;
	UINT32 icm_shift = channel ? 8 : 0;
	UINT8 icm = (mcd212.channel[0].image_coding_method & icm_mask) >> icm_shift;

	//printf( "vsr before: %08x: ", vsr );
	//fflush(stdout);

	if(!icm)
	{
		for(; x < 768; x++)
		{
			pixels[x++] = 0x00070707;
			pixels[x] = 0x00070707;
		}
		return;
	}

	while(!done)
	{
		UINT8 byte = data[vsr++ ^ 1];
		//printf( "%02x ", byte );
		//fflush(stdout);
		switch(mcd212.channel[channel].ddr & MCD212_DDR_FT)
		{
			case MCD212_DDR_FT_BMP:
			case MCD212_DDR_FT_BMP2:
				if(mcd212.channel[channel].dcr & MCD212_DCR_CM)
				{
					// 4-bit Bitmap
					verboselog(machine, 0, "Unsupported display mode: 4-bit Bitmap\n" );
				}
				else
				{
					// 8-bit Bitmap
					if(icm == 5)
					{
						UINT8 y;
						INT16 u;
						INT16 v;
						switch(channel)
						{
							case 0:
								y = (mcd212.channel[0].dyuv_abs_start_a >> 16) & 0x000000ff;
								u = (mcd212.channel[0].dyuv_abs_start_a >>  8) & 0x000000ff;
								v = (mcd212.channel[0].dyuv_abs_start_a >>  0) & 0x000000ff;
								break;
							case 1:
								y = (mcd212.channel[1].dyuv_abs_start_b >> 16) & 0x000000ff;
								u = (mcd212.channel[1].dyuv_abs_start_b >>  8) & 0x000000ff;
								v = (mcd212.channel[1].dyuv_abs_start_b >>  0) & 0x000000ff;
								break;
							default:
								y = u = v = 0x80;
								break;
						}
						for(; x < 768;)
						{
							UINT8 r, g, b;
							UINT16 curr = (byte << 8) | data[vsr++ ^ 1];
							UINT16 next = (x < 766) ? ((data[(vsr + 1) ^ 1] << 8) | data[(vsr + 2) ^ 1]) : curr;
							UINT8 y0 = (INT8)mcd212_dyuv_quantizer[(curr >>  8) & 0x000f];
							INT16 u0 = (INT8)mcd212_dyuv_quantizer[(curr >> 12) & 0x000f];
							INT16 v0 = (INT8)mcd212_dyuv_quantizer[(curr >>  4) & 0x000f];
							UINT8 y1 = (INT8)mcd212_dyuv_quantizer[(curr >>  0) & 0x000f];
							INT16 ut = (INT8)mcd212_dyuv_quantizer[(next >> 12) & 0x000f];
							INT16 vt = (INT8)mcd212_dyuv_quantizer[(next >>  4) & 0x000f];
							INT16 u1 = (u0 + ut) / 2;
							INT16 v1 = (v0 + vt) / 2;
							u += u0;
							v += v0;
							y += y0;
            				r = MCD212_LIM( ((y << 8) + 351 * (v - 128)) >> 8 );
            				g = MCD212_LIM( ((y << 8) + (86 * (u - 128) + 179 * (v - 128))) >> 8 );
            				b = MCD212_LIM( ((y << 8) + 444 * (u - 128)) >> 8 );
            				pixels[x++] = (r << 16) | (g << 8) | b;
            				pixels[x++] = (r << 16) | (g << 8) | b;
							u += u1;
							v += v1;
							y += y1;
            				r = MCD212_LIM( ((y << 8) + 351 * (v - 128)) >> 8 );
            				g = MCD212_LIM( ((y << 8) + (86 * (u - 128) + 179 * (v - 128))) >> 8 );
            				b = MCD212_LIM( ((y << 8) + 444 * (u - 128)) >> 8 );
            				pixels[x++] = (r << 16) | (g << 8) | b;
            				pixels[x++] = (r << 16) | (g << 8) | b;
							byte = data[vsr++ ^ 1];
						}
						mcd212_set_vsr(channel, vsr - 1);
					}
					else if(icm == 3)
					{
						for(; x < 768; x++)
						{
							pixels[x++] = mcd212.channel[channel].clut[((channel == 1) ? 0x80 : 0x00) + (byte & 0x7f)];
							pixels[x] = mcd212.channel[channel].clut[((channel == 1) ? 0x80 : 0x00) + (byte & 0x7f)];
							byte = data[vsr++ ^ 1];
						}
						mcd212_set_vsr(channel, vsr - 1);
					}
					else
					{
						for(; x < 768; x++)
						{
							pixels[x++] = 0x00070707;
							pixels[x] = 0x00070707;
						}
					}
				}
				done = 1;
				break;
			case MCD212_DDR_FT_RLE:
				if(mcd212.channel[channel].dcr & MCD212_DCR_CM)
				{
					// 4-bit RLE
					verboselog(machine, 0, "Unsupported display mode: 4-bit RLE\n" );
					done = 1;
				}
				else
				{
					// 8-bit RLE
					fflush(stdout);
					if(byte & 0x80)
					{
						// Run length
						UINT8 length = data[vsr++ ^ 1];
						//printf( "%02x ", length );
						//fflush(stdout);
						if(!length)
						{
							// Go to the end of the line
							for(; x < 768; x++)
							{
								pixels[x++] = mcd212.channel[channel].clut[((channel == 1) ? 0x80 : 0x00) + (byte & 0x7f)];
								pixels[x] = mcd212.channel[channel].clut[((channel == 1) ? 0x80 : 0x00) + (byte & 0x7f)];
							}
							done = 1;
							mcd212_set_vsr(channel, vsr);
						}
						else
						{
							int end = x + (length * 2);
							//printf( "x = %d length = %d end = %d\n", x, length * 2, end);
							//fflush(stdout);
							for(; x < end && x < 768; x++)
							{
								pixels[x++] = mcd212.channel[channel].clut[byte & 0x7f];
								pixels[x] = mcd212.channel[channel].clut[byte & 0x7f];
							}
							if(x >= 768)
							{
								done = 1;
								mcd212_set_vsr(channel, vsr);
							}
						}
					}
					else
					{
						// Single pixel
						pixels[x++] = mcd212.channel[channel].clut[byte & 0x7f];
						pixels[x++] = mcd212.channel[channel].clut[byte & 0x7f];
						if(x >= 768)
						{
							done = 1;
							mcd212_set_vsr(channel, vsr);
						}
					}
				}
				break;
			case MCD212_DDR_FT_MOSAIC:
				verboselog(machine, 0, "Unsupported display mode: Mosaic\n" );
				done = 1;
				break;
		}
	}

	//printf( ": vsr after: %08x\n", vsr);
	//mcd212_set_vsr(channel, vsr);
}

static const UINT32 mcd212_4bpp_color[16] =
{
	0x00000000, 0x0000007f, 0x00007f00, 0x00007f7f, 0x007f0000, 0x007f007f, 0x007f7f00, 0x007f7f7f,
	0x00000000, 0x000000ff, 0x0000ff00, 0x0000ffff, 0x00ff0000, 0x00ff00ff, 0x00ffff00, 0x00ffffff
};

static void mcd212_draw_cursor(running_machine *machine, UINT32 *scanline, int y)
{
	if(mcd212.channel[0].cursor_control & MCD212_CURCNT_EN)
	{
		UINT16 curx = mcd212.channel[0].cursor_position & 0x3ff;
		UINT16 cury = (mcd212.channel[0].cursor_position >> 12) & 0x3ff;
		UINT32 x = 0;
		cury += 22;
		if(y >= cury && y < (cury + 16))
		{
			UINT32 color = mcd212_4bpp_color[mcd212.channel[0].cursor_control & MCD212_CURCNT_COLOR];
			y -= cury;
			for(x = curx; x < curx + 16; x++)
			{
				if(mcd212.channel[0].cursor_pattern[y] & (1 << (15 - (x - curx))))
				{
					scanline[x] = color;
				}
				else
				{
				}
			}
		}
	}
}

static void mcd212_mix_lines(running_machine *machine, UINT32 *plane_a, UINT32 *plane_b, UINT32 *out)
{
	int x = 0;
	UINT32 backdrop = mcd212_4bpp_color[mcd212.channel[0].backdrop_color];
	for(x = 0; x < 768; x++)
	{
		out[x] = backdrop;
		if(!(mcd212.channel[0].transparency_control & MCD212_TCR_DISABLE_MX))
		{
			INT32 weight_a = mcd212.channel[0].weight_factor_a;
			INT32 weight_b = mcd212.channel[1].weight_factor_b;
			INT32 ar = (plane_a[x] >> 16) & 0x000000ff;
			INT32 ag = (plane_a[x] >>  8) & 0x000000ff;
			INT32 ab = (plane_a[x] >>  0) & 0x000000ff;
			INT32 br = (plane_b[x] >> 16) & 0x000000ff;
			INT32 bg = (plane_b[x] >>  8) & 0x000000ff;
			INT32 bb = (plane_b[x] >>  0) & 0x000000ff;
			UINT8 abr = MCD212_LIM((MCD212_LIM(ar - 16) * weight_a) / 64 + (MCD212_LIM(br - 16) * weight_b) / 64 + 16);
			UINT8 abg = MCD212_LIM((MCD212_LIM(ag - 16) * weight_a) / 64 + (MCD212_LIM(bg - 16) * weight_b) / 64 + 16);
			UINT8 abb = MCD212_LIM((MCD212_LIM(ab - 16) * weight_a) / 64 + (MCD212_LIM(bb - 16) * weight_b) / 64 + 16);
			out[x] = (abr << 16) | (abg << 8) | abb;
		}
		else
		{
			UINT8 transparency_mode[2] =
			{
				(mcd212.channel[0].transparency_control >> 0) & 0x0f,
				(mcd212.channel[0].transparency_control >> 8) & 0x0f
			};
			UINT8 plane_enable[2] =
			{
				0,
				0
			};
			int index = 0;
			for(index = 0; index < 2; index++)
			{
				UINT32 *plane = index ? plane_b : plane_a;
				UINT32 transparent_color = index ? mcd212.channel[1].transparent_color_b : mcd212.channel[0].transparent_color_a;
				switch(transparency_mode[index])
				{
					case 0:
						plane_enable[index] = 0;
						break;
					case 1:
						if(plane[x] != transparent_color)
						{
							plane_enable[index] = 1;
						}
						break;
					case 8:
						plane_enable[index] = 1;
						break;
					case 9:
						if(plane[x] == transparent_color)
						{
							plane_enable[index] = 1;
						}
						break;
					default:
						verboselog(machine, 0, "Unhandled transparency mode for plane %c: %d\n", index ? 'B' : 'A', transparency_mode[index]);
						break;
				}
			}
			switch(mcd212.channel[0].plane_order)
			{
				case MCD212_POR_AB:
					if(plane_enable[0])
					{
						out[x] = plane_a[x];
					}
					else if(plane_enable[1])
					{
						out[x] = plane_b[x];
					}
					break;
				case MCD212_POR_BA:
					if(plane_enable[1])
					{
						out[x] = plane_b[x];
					}
					else if(plane_enable[0])
					{
						out[x] = plane_a[x];
					}
					break;
			}
		}
	}
}

static void mcd212_draw_scanline(running_machine *machine, int y)
{
	bitmap_t *bitmap = tmpbitmap;
	UINT32 plane_a[768];
	UINT32 plane_b[768];
	UINT32 out[768];
	UINT32 *scanline = BITMAP_ADDR32(bitmap, y, 0);
	int x;
	mcd212_process_vsr(machine, 0, plane_a);
	mcd212_process_vsr(machine, 1, plane_b);
	mcd212_mix_lines(machine, plane_a, plane_b, out);
	for(x = 0; x < 768; x++)
	{
		scanline[x] = out[x];
	}
	mcd212_draw_cursor(machine, scanline, y);
}

TIMER_CALLBACK( mcd212_perform_scan )
{
	int scanline = video_screen_get_vpos(machine->primary_screen);
	if(/*mcd212.channel[0].dcr & MCD212_DCR_DE*/1)
	{
		if(scanline == 0)
		{
			// Process ICA
			int index = 0;
			verboselog(machine, 0, "FRAME START\n" );
			mcd212.channel[0].csrr &= 0x7f;
			for(index = 0; index < 2; index++)
			{
				if(mcd212.channel[index].dcr & MCD212_DCR_ICA)
				{
					mcd212_process_ica(machine, index);
				}
			}
			cdi220_draw_lcd(machine, scanline);
		}
		else if(scanline < 22)
		{
			cdi220_draw_lcd(machine, scanline);
		}
		else if(scanline >= 22)
		{
			int index = 0;
			mcd212.channel[0].csrr |= 0x80;
			// Process VSR
			mcd212_draw_scanline(machine, scanline);
			// Process DCA
			for(index = 0; index < 2; index++)
			{
				if(mcd212.channel[index].dcr & MCD212_DCR_DCA)
				{
					if(scanline == 22)
					{
						mcd212.channel[index].dca = mcd212_get_dcp(index);
					}
					mcd212_process_dca(machine, index);
				}
			}
			if(scanline == 261)
			{
				mcd212.channel[0].csrr ^= 0x20;
			}
		}
	}
	timer_adjust_oneshot(mcd212.scan_timer, video_screen_get_time_until_pos(machine->primary_screen, ( scanline + 1 ) % 262, 0), 0);
}

static VIDEO_START(cdi)
{
	VIDEO_START_CALL(generic_bitmapped);
	mcd212.channel[0].csrr = 0x00;
	mcd212.channel[1].csrr = 0x00;
	mcd212.channel[0].clut_bank = 0;
	mcd212.channel[1].clut_bank = 0;
	mcd212.scan_timer = timer_alloc(machine, mcd212_perform_scan, 0);
	timer_adjust_oneshot(mcd212.scan_timer, video_screen_get_time_until_pos(machine->primary_screen, 0, 0), 0);
}

TIMER_CALLBACK( test_timer_callback )
{
	// This function manually triggers interrupt requests as a test.
	static UINT8 set = 0;
	if(set == 0)
	{
		set = 1;
		cpu_set_input_line_vector(cputag_get_cpu(machine, "maincpu"), M68K_IRQ_4, 60);
		cputag_set_input_line(machine, "maincpu", M68K_IRQ_4, ASSERT_LINE);
		timer_adjust_oneshot(test_timer, ATTOTIME_IN_HZ(10000), 0);
	}
	else
	{
		set = 0;
		cputag_set_input_line(machine, "maincpu", M68K_IRQ_4, CLEAR_LINE);
		timer_adjust_oneshot(test_timer, attotime_never, 0);
	}
}

/*************************
*      Memory maps       *
*************************/

static WRITE16_HANDLER(cdic_ram_w)
{
	verboselog(space->machine, 0, "cdic_ram_w: %08x = %04x & %04x\n", 0x00300000 + offset*2, data, mem_mask);
	COMBINE_DATA(&cdram[offset]);
}

static READ16_HANDLER(cdic_ram_r)
{
	verboselog(space->machine, 0, "cdic_ram_r: %08x = %04x & %04x\n", 0x00300000 + offset*2, cdram[offset], mem_mask);
	return cdram[offset];
}

static ADDRESS_MAP_START( cdimono1_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000000, 0x0007ffff) AM_RAM AM_BASE(&planea)
	AM_RANGE(0x00200000, 0x0027ffff) AM_RAM AM_BASE(&planeb)
#if ENABLE_UART_PRINTING
	AM_RANGE(0x00301400, 0x00301403) AM_READ(uart_loopback_enable)
	//AM_RANGE(0x00301404, 0x00303bff) AM_RAM
#else
	AM_RANGE(0x00300000, 0x00303bff) AM_READWRITE(cdic_ram_r, cdic_ram_w) AM_BASE(&cdram)
#endif
	AM_RANGE(0x00303c00, 0x00303fff) AM_READWRITE(cdic_r, cdic_w)
	AM_RANGE(0x00310000, 0x00317fff) AM_READWRITE(slave_r, slave_w)
	//AM_RANGE(0x00318000, 0x0031ffff) AM_NOP
	AM_RANGE(0x00320000, 0x00323fff) AM_READWRITE(m48t08_r, m48t08_w)
	AM_RANGE(0x00400000, 0x0047ffff) AM_ROM AM_REGION("maincpu", 0)
	AM_RANGE(0x004fffe0, 0x004fffff) AM_READWRITE(mcd212_r, mcd212_w)
	AM_RANGE(0x00500000, 0x0057ffff) AM_RAM
	AM_RANGE(0x00580000, 0x00ffffff) AM_NOP
	AM_RANGE(0x80000000, 0x8000807f) AM_READWRITE(scc68070_periphs_r, scc68070_periphs_w)
ADDRESS_MAP_END

/*************************
*      Input ports       *
*************************/

static INPUT_PORTS_START( cdi )
	PORT_START("MOUSEX")
	PORT_BIT(0x3ff, 0x000, IPT_MOUSE_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_CHANGED(mouse_update, 0)

	PORT_START("MOUSEY")
	PORT_BIT(0x1ff, 0x000, IPT_MOUSE_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_CHANGED(mouse_update, 0)

	PORT_START("MOUSEBTN")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_CODE(MOUSECODE_BUTTON1) PORT_NAME("Mouse Button 1") PORT_CHANGED(mouse_update, 0)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_CODE(MOUSECODE_BUTTON2) PORT_NAME("Mouse Button 2") PORT_CHANGED(mouse_update, 0)
	PORT_BIT(0xfc, IP_ACTIVE_HIGH, IPT_UNUSED)
INPUT_PORTS_END

static MACHINE_RESET( cdi )
{
	UINT16 *src   = (UINT16*)memory_region( machine, "maincpu" );
	UINT16 *dst   = planea;
	const device_config *cdrom_dev = devtag_get_device(machine, "cdrom");
	memcpy(dst, src, 0x8);

	scc68070_regs.timers.timer0_timer = timer_alloc(machine, scc68070_timer0_callback, 0);
	timer_adjust_oneshot(scc68070_regs.timers.timer0_timer, attotime_never, 0);

	test_timer = timer_alloc(machine, test_timer_callback, 0);
	timer_adjust_oneshot(test_timer, attotime_never, 0);

	slave_regs.interrupt_timer = timer_alloc(machine, slave_trigger_readback_int, 0);
	timer_adjust_oneshot(slave_regs.interrupt_timer, attotime_never, 0);

	cdic_regs.interrupt_timer = timer_alloc(machine, cdic_trigger_readback_int, 0);
	timer_adjust_oneshot(cdic_regs.interrupt_timer, attotime_never, 0);

	if( cdrom_dev )
	{
		cdic_regs.cd = mess_cd_get_cdrom_file(cdrom_dev);
	}

	device_reset(cputag_get_cpu(machine, "maincpu"));

	cdic_regs.z_buffer = 0;
	cdic_regs.channel = 0xffffffff;
}

/*************************
*    Machine Drivers     *
*************************/

static MACHINE_DRIVER_START( cdimono1 )
	MDRV_CPU_ADD("maincpu", SCC68070, CLOCK_A/2)	/* SCC-68070 CCA84 datasheet */
	MDRV_CPU_PROGRAM_MAP(cdimono1_mem)

	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_SIZE(768, 262)
	MDRV_SCREEN_VISIBLE_AREA(0, 768-1, 0, 262-1) //dynamic resolution,TODO

	MDRV_PALETTE_LENGTH(0x100)

	MDRV_VIDEO_START(cdi)
	MDRV_VIDEO_UPDATE(generic_bitmapped)

	MDRV_MACHINE_RESET(cdi)

	MDRV_CDROM_ADD( "cdrom" )
MACHINE_DRIVER_END

/*************************
*        Rom Load        *
*************************/

ROM_START( cdimono1 )
	ROM_REGION(0x80000, "maincpu", 0)
	ROM_SYSTEM_BIOS( 0, "mcdi200", "Magnavox CD-i 200" )
	ROMX_LOAD( "cdi200.rom", 0x000000, 0x80000, CRC(40c4e6b9) SHA1(d961de803c89b3d1902d656ceb9ce7c02dccb40a), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "mcdi200", "Philips CD-i 220 F2" )
	ROMX_LOAD( "cdi220b.rom", 0x000000, 0x80000, CRC(279683ca) SHA1(53360a1f21ddac952e95306ced64186a3fc0b93e), ROM_BIOS(2) )
	// This one is a Mono-IV board, needs to be a separate driver
	//ROM_SYSTEM_BIOS( 1, "pcdi490", "Philips CD-i 490" )
	//ROMX_LOAD( "cdi490.rom", 0x000000, 0x80000, CRC(e115f45b) SHA1(f71be031a5dfa837de225081b2ddc8dcb74a0552), ROM_BIOS(2) )
	// This one is a Mini-MMC board, needs to be a separate driver
	//ROM_SYSTEM_BIOS( 2, "pcdi910m", "Philips CD-i 910" )
	//ROMX_LOAD( "cdi910.rom", 0x000000, 0x80000,  CRC(8ee44ed6) SHA1(3fcdfa96f862b0cb7603fb6c2af84cac59527b05), ROM_BIOS(3) )
ROM_END

/*************************
*      Game driver(s)    *
*************************/

/*    YEAR  NAME      PARENT    COMPAT    MACHINE   INPUT     INIT      CONFIG    COMPANY     FULLNAME   FLAGS */
CONS( 1991, cdimono1, 0,        0,        cdimono1, cdi,      0,        0,        "Philips",  "CD-i (Mono-I)",   GAME_NO_SOUND | GAME_NOT_WORKING )
