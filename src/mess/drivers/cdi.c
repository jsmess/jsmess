/******************************************************************************


    Philips CD-I
    -------------------

    Preliminary MAME driver by Roberto Fresca, David Haywood & Angelo Salese
    MESS improvements by Harmony


*******************************************************************************


    *** Hardware Notes ***

    These are actually the specs of the Philips CD-i console.

    Identified:

    - CPU:  1x Philips SCC 68070 CCA84 (16 bits Microprocessor, PLCC) @ 15 MHz
    - VSC:  1x Philips SCC 66470 CAB (Video and System Controller, QFP)

    - Crystals:   1x 30.0000 MHz.
                  1x 19.6608 MHz.


*******************************************************************************

STATUS:

BIOSes will run until attempting an OS call using the methods described here:
http://www.icdia.co.uk/microware/tech/tech_2.pdf

TODO:

-Proper handling of the 68070 (68k with 32 address lines instead of 24)
 & handle the extra features properly (UART,DMA,Timers etc.)

-Proper emulation of the 66470 and/or MCD212 Video Chip (still many unhandled features)

-Inputs;

-Unknown sound chip (it's an ADPCM with eight channels);

-Many unknown memory maps;

*******************************************************************************/


#define CLOCK_A	XTAL_30MHz
#define CLOCK_B	XTAL_19_6608MHz

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "sound/2413intf.h"
#include "devices/chd_cd.h"

static UINT16 *ram;

#define VERBOSE_LEVEL	(5)

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

/*************************
*     Video Hardware     *
*************************/

typedef struct
{
	UINT8 csrr;
	UINT16 csrw;
	UINT8 dcr;
	UINT8 vsr;
	UINT8 ddr;
	UINT8 dcp;
} mcd212_channel_t;

typedef struct
{
	mcd212_channel_t channel[2];
} mcd212_t;

mcd212_t mcd212;

static READ16_HANDLER(mcd212_r)
{
	switch(offset)
	{
		case 0x00/2:
		case 0x10/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "mcd212_r: Status Register %d: %02x & %04x\n", (1 - (offset / 8)) + 1, mcd212.channel[1 - (offset / 8)].csrr, mem_mask);
				mcd212.channel[1 - (offset / 8)].csrr ^= 0x80; // Simulate drawing / not drawing
				return mcd212.channel[1 - (offset / 8)].csrr;
			}
			break;
		case 0x02/2:
		case 0x12/2:
			verboselog(space->machine, 2, "mcd212_r: Display Command Register %d: %04x & %04x\n", (1 - (offset / 8)) + 1, mcd212.channel[1 - (offset / 8)].dcr, mem_mask);
			return mcd212.channel[1 - (offset / 8)].dcr;
			break;
		case 0x04/2:
		case 0x14/2:
			verboselog(space->machine, 2, "mcd212_r: Video Start Register %d: %04x & %04x\n", (1 - (offset / 8)) + 1, mcd212.channel[1 - (offset / 8)].vsr, mem_mask);
			return mcd212.channel[1 - (offset / 8)].vsr;
			break;
		case 0x08/2:
		case 0x18/2:
			verboselog(space->machine, 2, "mcd212_r: Display Decoder Register %d: %04x & %04x\n", (1 - (offset / 8)) + 1, mcd212.channel[1 - (offset / 8)].ddr, mem_mask);
			return mcd212.channel[1 - (offset / 8)].ddr;
			break;
		case 0x0a/2:
		case 0x1a/2:
			verboselog(space->machine, 2, "mcd212_r: DCA Pointer Register %d: %04x & %04x\n", (1 - (offset / 8)) + 1, mcd212.channel[1 - (offset / 8)].dcp, mem_mask);
			return mcd212.channel[1 - (offset / 8)].dcp;
			break;
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
			verboselog(space->machine, 2, "mcd212_w: Status Register %d: %02x & %04x\n", (1 - (offset / 8)) + 1, data, mem_mask);
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

static VIDEO_START(cdi)
{
	mcd212.channel[0].csrr = 0x00;
	mcd212.channel[1].csrr = 0x00;
}

static VIDEO_UPDATE(cdi)
{
	bitmap_fill(bitmap, cliprect, get_black_pen(screen->machine)); //TODO
	return 0;
}


/*************************
* Memory map information *
*************************/

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

	UINT8 transfer_counter;

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

scc68070_regs_t scc68070_regs;

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
			return scc68070_regs.uart.mode_register;
		case 0x2012/2:
			if(ACCESSING_BITS_0_7)
			{
//				verboselog(space->machine, 2, "scc68070_periphs_r: UART Status Register: %04x & %04x\n", scc68070_regs.uart.status_register, mem_mask);
			}
			return scc68070_regs.uart.status_register | USR_TXRDY | USR_RXRDY;
		case 0x2014/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: UART Clock Select: %04x & %04x\n", scc68070_regs.uart.clock_select, mem_mask);
			}
			return scc68070_regs.uart.clock_select;
		case 0x2016/2:
			if(ACCESSING_BITS_0_7)
			{
//				verboselog(space->machine, 2, "scc68070_periphs_r: UART Command Register: %02x & %04x\n", scc68070_regs.uart.command_register, mem_mask);
			}
			return scc68070_regs.uart.command_register;
		case 0x2018/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: UART Transmit Holding Register: %02x & %04x\n", scc68070_regs.uart.transmit_holding_register, mem_mask);
			}
			return scc68070_regs.uart.transmit_holding_register;
		case 0x201a/2:
			if(ACCESSING_BITS_0_7)
			{
//				verboselog(space->machine, 2, "scc68070_periphs_r: UART Receive Holding Register: %02x & %04x\n", scc68070_regs.uart.receive_holding_register, mem_mask);
			}
			return scc68070_regs.uart.receive_holding_register;

		// Timers: 80002020 to 80002029
		case 0x2020/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: Timer Status Register: %02x & %04x\n", scc68070_regs.timers.timer_status_register, mem_mask);
			}
			if(ACCESSING_BITS_8_15)
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: Timer Control Register: %02x & %04x\n", scc68070_regs.timers.timer_control_register, mem_mask);
			}
			return (scc68070_regs.timers.timer_control_register << 8) | scc68070_regs.timers.timer_status_register;
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
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_r: DMA(%d) Memory Transfer Counter Low: %02x & %04x\n", (offset - 0x2000) / 32, scc68070_regs.dma.channel[(offset - 0x2000) / 32].transfer_counter, mem_mask);
			}
			if(ACCESSING_BITS_8_15)
			{
				verboselog(space->machine, 0, "scc68070_periphs_r: DMA(%d) Memory Transfer Counter High (invalid): %04x\n", (offset - 0x2000) / 32, mem_mask);
			}
			return scc68070_regs.dma.channel[(offset - 0x2000) / 32].transfer_counter;
		case 0x400c/2:
		case 0x404c/2:
			verboselog(space->machine, 2, "scc68070_periphs_r: DMA(%d) Memory Address Counter (High Word): %04x & %04x\n", (offset - 0x2000) / 32, (scc68070_regs.dma.channel[(offset - 0x2000) / 32].memory_address_counter >> 16), mem_mask);
			return (scc68070_regs.dma.channel[(offset - 0x2000) / 32].memory_address_counter >> 16);
		case 0x400e/2:
		case 0x404e/2:
			verboselog(space->machine, 2, "scc68070_periphs_r: DMA(%d) Memory Address Counter (Low Word): %04x & %04x\n", (offset - 0x2000) / 32, scc68070_regs.dma.channel[(offset - 0x2000) / 32].device_address_counter, mem_mask);
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
	}

	return 0;
}

static WRITE16_HANDLER( scc68070_periphs_w )
{
	switch(offset)
	{
		// Interupts: 80001001
		case 0x1000/2: // LIR priority level
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
//				verboselog(space->machine, 2, "scc68070_periphs_w: UART Command Register: %04x & %04x\n", data, mem_mask);
				scc68070_regs.uart.command_register = data & 0x00ff;
			}
			break;
		case 0x2018/2:
			if(ACCESSING_BITS_0_7)
			{
//				verboselog(space->machine, 2, "scc68070_periphs_w: UART Transmit Holding Register: %04x & %04x\n", data, mem_mask);
				if(data >= 0x20 && data < 0x7f)
				{
					printf( "%c", data & 0x00ff );
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
				verboselog(space->machine, 2, "scc68070_periphs_w: Timer Status Register: %04x & %04x\n", data, mem_mask);
				scc68070_regs.timers.timer_status_register = data & 0x00ff;
			}
			if(ACCESSING_BITS_8_15)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: Timer Control Register: %04x & %04x\n", data, mem_mask);
				scc68070_regs.timers.timer_control_register = data >> 8;
			}
			break;
		case 0x2022/2:
			verboselog(space->machine, 2, "scc68070_periphs_w: Timer Reload Register: %04x & %04x\n", data, mem_mask);
			COMBINE_DATA(&scc68070_regs.timers.reload_register);
			break;
		case 0x2024/2:
			verboselog(space->machine, 2, "scc68070_periphs_w: Timer 0: %04x & %04x\n", data, mem_mask);
			COMBINE_DATA(&scc68070_regs.timers.timer0);
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
				scc68070_regs.dma.channel[(offset - 0x2000) / 32].channel_status = data >> 8;
			}
			break;
		case 0x4004/2:
		case 0x4044/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: DMA(%d) Operation Control Register: %04x & %04x\n", (offset - 0x2000) / 32, data, mem_mask);
				scc68070_regs.dma.channel[(offset - 0x2000) / 32].operation_control = data >> 8;
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
				scc68070_regs.dma.channel[(offset - 0x2000) / 32].channel_control = data >> 8;
			}
			if(ACCESSING_BITS_8_15)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: DMA(%d) Sequence Control Register: %04x & %04x\n", (offset - 0x2000) / 32, data, mem_mask);
				scc68070_regs.dma.channel[(offset - 0x2000) / 32].sequence_control = data >> 8;
			}
			break;
		case 0x400a/2:
			if(ACCESSING_BITS_0_7)
			{
				verboselog(space->machine, 2, "scc68070_periphs_w: DMA(%d) Memory Transfer Counter Low: %04x & %04x\n", (offset - 0x2000) / 32, data, mem_mask);
				scc68070_regs.dma.channel[(offset - 0x2000) / 32].transfer_counter = data >> 8;
			}
			if(ACCESSING_BITS_8_15)
			{
				verboselog(space->machine, 0, "scc68070_periphs_w: DMA(%d) Memory Transfer Counter High (invalid): %04x & %04x\n", (offset - 0x2000) / 32, data, mem_mask);
			}
			break;
		case 0x400c/2:
		case 0x404c/2:
			verboselog(space->machine, 2, "scc68070_periphs_w: DMA(%d) Memory Address Counter (High Word): %04x & %04x\n", (offset - 0x2000) / 32, data, mem_mask);
			scc68070_regs.dma.channel[(offset - 0x2000) / 32].memory_address_counter &= mem_mask << 16;
			scc68070_regs.dma.channel[(offset - 0x2000) / 32].memory_address_counter |= data << 16;
			break;
		case 0x400e/2:
		case 0x404e/2:
			verboselog(space->machine, 2, "scc68070_periphs_w: DMA(%d) Memory Address Counter (Low Word): %04x & %04x\n", (offset - 0x2000) / 32, data, mem_mask);
			scc68070_regs.dma.channel[(offset - 0x2000) / 32].memory_address_counter &= (0xffff0000) | mem_mask;
			scc68070_regs.dma.channel[(offset - 0x2000) / 32].memory_address_counter |= data;
			break;
		case 0x4014/2:
		case 0x4054/2:
			verboselog(space->machine, 2, "scc68070_periphs_w: DMA(%d) Device Address Counter (High Word): %04x & %04x\n", (offset - 0x2000) / 32, data, mem_mask);
			scc68070_regs.dma.channel[(offset - 0x2000) / 32].device_address_counter &= mem_mask << 16;
			scc68070_regs.dma.channel[(offset - 0x2000) / 32].device_address_counter |= data << 16;
			break;
		case 0x4016/2:
		case 0x4056/2:
			verboselog(space->machine, 2, "scc68070_periphs_w: DMA(%d) Device Address Counter (Low Word): %04x & %04x\n", (offset - 0x2000) / 32, data, mem_mask);
			scc68070_regs.dma.channel[(offset - 0x2000) / 32].device_address_counter &= (0xffff0000) | mem_mask;
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
	}
}

static READ16_HANDLER( magic_r )
{
	return 0x1234;
}

static ADDRESS_MAP_START( cdi_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000000, 0x000fffff) AM_RAM AM_SHARE(1) AM_BASE(&ram)
	AM_RANGE(0x00301400, 0x00301403) AM_READ( magic_r )
	AM_RANGE(0x00400000, 0x0047ffff) AM_ROM AM_REGION("maincpu", 0)
	AM_RANGE(0x004fffe0, 0x004fffff) AM_READWRITE(mcd212_r, mcd212_w)
	AM_RANGE(0x00500000, 0x0057ffff) AM_RAM
	AM_RANGE(0x80000000, 0x8000807f) AM_READWRITE(scc68070_periphs_r, scc68070_periphs_w)
ADDRESS_MAP_END


/*************************
*      Input ports       *
*************************/

static INPUT_PORTS_START( cdi )
INPUT_PORTS_END


static MACHINE_RESET( cdi )
{
	UINT16 *src    = (UINT16*)memory_region( machine, "maincpu" );
	UINT16 *dst    = ram;
	memcpy (dst, src, 0x80000);
	device_reset(cputag_get_cpu(machine, "maincpu"));
}

/*************************
*    Machine Drivers     *
*************************/

//static INTERRUPT_GEN( cdi_irq )
//{
//  if(input_code_pressed(KEYCODE_Z))
//      cpu_set_input_line(device->machine->firstcpu, 1, HOLD_LINE);
//  ram[0x2004/2]^=0xffff;
//}

static MACHINE_DRIVER_START( cdi )
	MDRV_CPU_ADD("maincpu", SCC68070, CLOCK_A/2)	/* SCC-68070 CCA84 datasheet */
	MDRV_CPU_PROGRAM_MAP(cdi_mem)
 	//MDRV_CPU_VBLANK_INT("screen", cdi_irq) /* no interrupts? (it erases the vectors..) */

	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_SIZE(768, 560)
	MDRV_SCREEN_VISIBLE_AREA(0, 760-1, 0, 560-1) //dynamic resolution,TODO

	MDRV_PALETTE_LENGTH(0x100)

	MDRV_VIDEO_START(cdi)
	MDRV_VIDEO_UPDATE(cdi)

	MDRV_MACHINE_RESET(cdi)

	//MDRV_SPEAKER_STANDARD_MONO("mono")
	//MDRV_SOUND_ADD("ym", YM2413, CLOCK_A/12)
	//MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_CDROM_ADD( "cdrom" )
MACHINE_DRIVER_END

/*************************
*        Rom Load        *
*************************/

ROM_START( cdi )
	ROM_REGION(0x80000, "maincpu", 0)
	ROM_SYSTEM_BIOS( 0, "mcdi200", "Magnavox CD-i 200" )
	ROMX_LOAD( "cdi200.rom", 0x000000, 0x80000, CRC(40c4e6b9) SHA1(d961de803c89b3d1902d656ceb9ce7c02dccb40a), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "cdi220", "Philips CD-i 220" )
	ROMX_LOAD( "cdi220.rom", 0x000000, 0x80000, CRC(40c4e6b9) SHA1(d961de803c89b3d1902d656ceb9ce7c02dccb40a), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "pcdi490", "Philips CD-i 490" )
	ROMX_LOAD( "cdi490.rom", 0x000000, 0x80000, CRC(e115f45b) SHA1(f71be031a5dfa837de225081b2ddc8dcb74a0552), ROM_BIOS(3) )
	ROM_SYSTEM_BIOS( 3, "pcdi910m", "Philips CD-i 910" )
	ROMX_LOAD( "cdi910.rom", 0x000000, 0x80000,  CRC(8ee44ed6) SHA1(3fcdfa96f862b0cb7603fb6c2af84cac59527b05), ROM_BIOS(4) )
	/* Bad dump? */
	/* ROM_SYSTEM_BIOS( 4, "cdi205", "Philips CD-i 205" ) */
	/* ROMX_LOAD( "cdi205.rom", 0x000000, 0x7fc00, CRC(79ab41b2) SHA1(656890bbd3115b235bc8c6b1cf29a99e9db37c1b), ROM_BIOS(5) ) */
ROM_END

/*************************
*      Game driver(s)    *
*************************/

/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT   INIT    CONFIG  COMPANY     FULLNAME   FLAGS */
CONS( 1991, cdi,        0,      0,      cdi,        0,      0,      0,      "Philips",  "CD-i",   GAME_NO_SOUND | GAME_NOT_WORKING )
