/**********************************************************************

	8250 UART interface and emulation

	KT - 14-Jun-2000 - Improved Interrupt setting/clearing
	KT - moved into seperate file so it can be used in Super I/O emulation and
		any other system which uses a PC type COM port
	KT - 24-Jun-2000 - removed pc specific input port tests. More compatible
		with PCW16 and PCW16 doesn't requre the PC input port definitions
		which are not required by the PCW16 hardware

**********************************************************************/

#include "driver.h"
#include "machine/uart8250.h"
#include "includes/pc_mouse.h"
#include "memconv.h"


#define LOG(LEVEL,N,M,A)  \
if( M )logerror("%11.6f: %-24s",timer_get_time(),(char*)M ); logerror A;


//#define VERBOSE_COM 0
#ifdef VERBOSE_COM
#define COM_LOG(n,m,a) LOG(VERBOSE_COM,n,m,a)
#else
#define COM_LOG(n,m,a)
#endif

typedef struct {
	uart8250_interface interface;

	UINT8 thr;  /* 0 -W transmitter holding register */
	UINT8 rbr; /* 0 R- receiver buffer register */
	UINT8 ier;  /* 1 RW interrupt enable register */
	UINT8 dll;  /* 0 RW divisor latch lsb (if DLAB = 1) */
	UINT8 dlm;  /* 1 RW divisor latch msb (if DLAB = 1) */
	UINT8 iir;  /* 2 R- interrupt identification register */
	UINT8 lcr;  /* 3 RW line control register (bit 7: DLAB) */
	UINT8 mcr;  /* 4 RW modem control register */
	UINT8 lsr;  /* 5 R- line status register */
	UINT8 msr;  /* 6 R- modem status register */
	UINT8 scr;  /* 7 RW scratch register */

/* holds int pending state for com */
	UINT8 int_pending;

	// sending circuit
	struct {
		int active;
		UINT8 data;
		double time;
	} send;
} UART8250;

UART8250 uart[4]={ { { TYPE8250 } } };

/* int's pending */
#define COM_INT_PENDING_RECEIVED_DATA_AVAILABLE	0x0001
#define COM_INT_PENDING_TRANSMITTER_HOLDING_REGISTER_EMPTY 0x0002
#define COM_INT_PENDING_RECEIVER_LINE_STATUS 0x0004
#define COM_INT_PENDING_MODEM_STATUS_REGISTER 0x0008

#define DLAB(n) (uart[n].lcr&0x80) //divisor latch access bit
#define LOOP(n) (uart[n].mcr&0x10)




/* setup iir with the priority id */
static void uart8250_setup_iir(int n)
{
	uart[n].iir &= ~(0x04|0x02);

	/* highest to lowest */
	if (uart[n].int_pending & COM_INT_PENDING_RECEIVER_LINE_STATUS)
	{
		uart[n].iir |=0x04|0x02;
		return;
	}

	if (uart[n].int_pending & COM_INT_PENDING_RECEIVED_DATA_AVAILABLE)
	{
		uart[n].iir |=0x04;
		return;
	}

	if (uart[n].int_pending & COM_INT_PENDING_TRANSMITTER_HOLDING_REGISTER_EMPTY)
	{
		uart[n].iir |=0x02;
		return;
	}

	/* modem status has both bits clear */
}


/* ints will continue to be set for as long as there are ints pending */
static void uart8250_update_interrupt(int n)
{
	int state;

	/* disable int output? */
	if ((uart[n].mcr & 0x08) == 0)
		return;

	/* if any bits are set and are enabled */
	if (((uart[n].int_pending&uart[n].ier) & 0x0f) != 0)
	{
		/* trigger next highest priority int */

		/* set int */
		state = 1;

		uart8250_setup_iir(n);

		/* int pending */
		uart[n].iir |= 0x01;
	}
	else
	{
		/* clear int */
		state = 0;

		/* no ints pending */
		uart[n].iir &= ~0x01;
		/* priority level */
		uart[n].iir &= ~(0x04|0x02);
	}


	/* set or clear the int */
	if (uart[n].interface.interrupt)
		uart[n].interface.interrupt(n, state);
}



/* set pending bit and trigger int */
static void uart8250_trigger_int(int n, int flag)
{
	uart[n].int_pending |= flag;

	uart8250_update_interrupt(n);
}



/* clear pending bit, if any ints are pending, then int will be triggered, otherwise it
will be cleared */
static void uart8250_clear_int(int n, int flag)
{
	uart[n].int_pending &= ~flag;

	uart8250_update_interrupt(n);
}



void uart8250_init(int nr, const uart8250_interface *new_interface)
{
	memset(&uart[nr], 0, sizeof(uart[nr]));
	if (new_interface)
		memcpy(&uart[nr].interface, new_interface, sizeof(uart8250_interface));
}



/* 1 based */
void uart8250_reset(int n)
{
	uart[n].ier = 0;
	uart[n].iir = 1;
	uart[n].lcr = 0;
	uart[n].mcr = 0;
	uart[n].lsr = (1<<5) | (1<<6);

	uart[n].send.active=0;

	/* refresh with reset state of register */
	if (uart[n].interface.refresh_connected)
		uart[n].interface.refresh_connected(n);
}



void uart8250_w(int n, offs_t idx, UINT8 data)
{
#ifdef VERBOSE_COM
    static char P[8] = "NONENHNL";  /* names for parity select */
#endif
    int tmp;

	switch (idx)
	{
		case 0:
			if (uart[n].lcr & 0x80)
			{
				uart[n].dll = data;
				tmp = uart[n].dlm * 256 + uart[n].dll;
				COM_LOG(1,"COM_dll_w",("COM%d $%02x: [$%04x = %d baud]\n",
					n+1, data, tmp, (tmp)?uart[n].interface.clockin/16/tmp:0));
			}
			else
			{
				uart[n].thr = data;
				COM_LOG(2,"COM_thr_w",("COM%d $%02x\n", n+1, data));

				if (LOOP(n))
				{
					uart[n].lsr |= 1;
					uart[n].rbr = data;
				}
				/* writing to thr will clear the int */
				uart8250_clear_int(n, COM_INT_PENDING_TRANSMITTER_HOLDING_REGISTER_EMPTY);
			}
			break;
		case 1:
			if (uart[n].lcr & 0x80)
			{
				uart[n].dlm = data;
				tmp = uart[n].dlm * 256 + uart[n].dll;
                COM_LOG(1,"COM_dlm_w",("COM%d $%02x: [$%04x = %d baud]\n",
					n+1, data, tmp, (tmp)?uart[n].interface.clockin/16/tmp:0));
			}
			else
			{
				uart[n].ier = data;
				COM_LOG(2,"COM_ier_w",("COM%d $%02x: enable int on RX %d, THRE %d, RLS %d, MS %d\n",
					n+1, data, data&1, (data>>1)&1, (data>>2)&1, (data>>3)&1));
			}
            break;
		case 2:
			COM_LOG(1,"COM_fcr_w",("COM%d $%02x (16550 only)\n", n+1, data));
            break;
		case 3:
			uart[n].lcr = data;
			COM_LOG(1,"COM_lcr_w",("COM%d $%02x word length %d, stop bits %d, parity %c, break %d, DLAB %d\n",
				n+1, data, 5+(data&3), 1+((data>>2)&1), P[(data>>3)&7], (data>>6)&1, (data>>7)&1));
            break;
		case 4:
			uart[n].mcr = data;
			COM_LOG(1,"COM_mcr_w",("COM%d $%02x DTR %d, RTS %d, OUT1 %d, OUT2 %d, loopback %d\n",
				n+1, data, data&1, (data>>1)&1, (data>>2)&1, (data>>3)&1, (data>>4)&1));
			if (uart[n].interface.handshake_out)
				uart[n].interface.handshake_out(n,data);
            break;
		case 5:
			break;
		case 6:
			break;
		case 7:
			uart[n].scr = data;
			COM_LOG(2,"COM_scr_w",("COM%d $%02x\n", n+1, data));
            break;
	}

	if (uart[n].interface.refresh_connected)
		uart[n].interface.refresh_connected(n);
}



UINT8 uart8250_r(int n, offs_t idx)
{
	int data = 0x0ff;

	switch (idx)
	{
		case 0:
			if (uart[n].lcr & 0x80)
			{
				data = uart[n].dll;
				COM_LOG(1,"COM_dll_r",("COM%d $%02x\n", n+1, data));
			}
			else
			{
				data = uart[n].rbr;
				if( uart[n].lsr & 0x01 )
				{
					uart[n].lsr &= ~0x01;		/* clear data ready status */
					COM_LOG(2,"COM_rbr_r",("COM%d $%02x\n", n+1, data));
				}

				uart8250_clear_int(n, COM_INT_PENDING_RECEIVED_DATA_AVAILABLE);
			}
			break;
		case 1:
			if (uart[n].lcr & 0x80)
			{
				data = uart[n].dlm;
				COM_LOG(1,"COM_dlm_r",("COM%d $%02x\n", n+1, data));
			}
			else
			{
				data = uart[n].ier;
				COM_LOG(2,"COM_ier_r",("COM%d $%02x\n", n+1, data));
            }
            break;
		case 2:
			data = uart[n].iir;
			COM_LOG(2,"COM_iir_r",("COM%d $%02x\n", n+1, data));
			/* this may not be correct. It says that reading this register will
			clear the int if this is the source of the int?? */
			uart8250_clear_int(n, COM_INT_PENDING_TRANSMITTER_HOLDING_REGISTER_EMPTY);
            break;
		case 3:
			data = uart[n].lcr;
			COM_LOG(2,"COM_lcr_r",("COM%d $%02x\n", n+1, data));
            break;
		case 4:
			data = uart[n].mcr;
			COM_LOG(2,"COM_mcr_r",("COM%d $%02x\n", n+1, data));
            break;
		case 5:

#if 0
			if (uart[n].send.active && (timer_get_time()-uart[n].send.time>uart_byte_time(n)))
			{
				// currently polling is enough for pc1512
				uart[n].lsr |= 0x40; /* set TSRE */
				uart[n].send.active = 0;
				if (LOOP(n))
				{
					uart[n].lsr |= 1;
					uart[n].rbr = uart[n].send.data;
				}
			}
#endif
			uart[n].lsr |= 0x20; /* set THRE */
			data = uart[n].lsr;
			if( uart[n].lsr & 0x1f )
			{
				uart[n].lsr &= 0xe1; /* clear FE, PE and OE and BREAK bits */
				COM_LOG(2,"COM_lsr_r",("COM%d $%02x, DR %d, OE %d, PE %d, FE %d, BREAK %d, THRE %d, TSRE %d\n",
					n+1, data, data&1, (data>>1)&1, (data>>2)&1, (data>>3)&1, (data>>4)&1, (data>>5)&1, (data>>6)&1));
			}

			/* reading line status register clears int */
			uart8250_clear_int(n, COM_INT_PENDING_RECEIVER_LINE_STATUS);
            break;
		case 6:
			if (uart[n].mcr & 0x10)	/* loopback test? */
			{
				data = uart[n].mcr << 4;
				/* build delta values */
				uart[n].msr = (uart[n].msr ^ data) >> 4;
				uart[n].msr |= data;
			}
			data = uart[n].msr;
			uart[n].msr &= 0xf0; /* reset delta values */
			COM_LOG(2,"COM_msr_r",("COM%d $%02x\n", n+1, data));

			/* reading msr clears int */
			uart8250_clear_int(n, COM_INT_PENDING_MODEM_STATUS_REGISTER);

			break;
		case 7:
			data = uart[n].scr;
			COM_LOG(2,"COM_scr_r",("COM%d $%02x\n", n+1, data));
            break;
	}

	if (uart[n].interface.refresh_connected)
		uart[n].interface.refresh_connected(n);

    return data;
}



void uart8250_receive(int n, int data)
{
    /* check if data rate 1200 baud is set */
	if( uart[n].dlm != 0x00 || uart[n].dll != 0x60 )
        uart[n].lsr |= 0x08; /* set framing error */

    /* if data not yet serviced */
	if( uart[n].lsr & 0x01 )
		uart[n].lsr |= 0x02; /* set overrun error */

    /* put data into receiver buffer register */
    uart[n].rbr = data;

    /* set data ready status */
    uart[n].lsr |= 0x01;

	/* set pending state for this interrupt. */
	uart8250_trigger_int(n, COM_INT_PENDING_RECEIVED_DATA_AVAILABLE);


//	/* OUT2 + received line data avail interrupt enabled? */
//	if( (COM_mcr[n] & 0x08) && (COM_ier[n] & 0x01) )
//	{
//		if (com_interface.interrupt)
//			com_interface.interrupt(4-(n&1), 1);
//
//	}
}

/**************************************************************************
 *	change the modem status register
 **************************************************************************/
void uart8250_handshake_in(int n, int new_msr)
{
	/* no change in modem status bits? */
	if( ((uart[n].msr ^ new_msr) & 0xf0) == 0 )
		return;

	/* set delta status bits 0..3 and new modem status bits 4..7 */
    uart[n].msr = (((uart[n].msr ^ new_msr) >> 4) & 0x0f) | (new_msr & 0xf0);

	uart8250_trigger_int(n, COM_INT_PENDING_MODEM_STATUS_REGISTER);

//	/* set up interrupt information register */
  //  COM_iir[n] &= ~(0x06 | 0x01);

//    /* OUT2 + modem status interrupt enabled? */
//	if( (COM_mcr[n] & 0x08) && (COM_ier[n] & 0x08) )
//	{
//		if (com_interface.interrupt)
//			com_interface.interrupt(4-(n&1), 1);
//	}
}


READ8_HANDLER ( uart8250_0_r ) { return uart8250_r(0, offset); }
READ8_HANDLER ( uart8250_1_r ) { return uart8250_r(1, offset); }
READ8_HANDLER ( uart8250_2_r ) { return uart8250_r(2, offset); }
READ8_HANDLER ( uart8250_3_r ) { return uart8250_r(3, offset); }
WRITE8_HANDLER ( uart8250_0_w ) { uart8250_w(0, offset, data); }
WRITE8_HANDLER ( uart8250_1_w ) { uart8250_w(1, offset, data); }
WRITE8_HANDLER ( uart8250_2_w ) { uart8250_w(2, offset, data); }
WRITE8_HANDLER ( uart8250_3_w ) { uart8250_w(3, offset, data); }

READ32_HANDLER ( uart8250_32le_0_r ) { return read32le_with_read8_handler(uart8250_0_r, offset, mem_mask); }
READ32_HANDLER ( uart8250_32le_1_r ) { return read32le_with_read8_handler(uart8250_1_r, offset, mem_mask); }
READ32_HANDLER ( uart8250_32le_2_r ) { return read32le_with_read8_handler(uart8250_2_r, offset, mem_mask); }
READ32_HANDLER ( uart8250_32le_3_r ) { return read32le_with_read8_handler(uart8250_3_r, offset, mem_mask); }
WRITE32_HANDLER ( uart8250_32le_0_w ) { write32le_with_write8_handler(uart8250_0_w, offset, data, mem_mask); }
WRITE32_HANDLER ( uart8250_32le_1_w ) { write32le_with_write8_handler(uart8250_1_w, offset, data, mem_mask); }
WRITE32_HANDLER ( uart8250_32le_2_w ) { write32le_with_write8_handler(uart8250_2_w, offset, data, mem_mask); }
WRITE32_HANDLER ( uart8250_32le_3_w ) { write32le_with_write8_handler(uart8250_3_w, offset, data, mem_mask); }

READ64_HANDLER ( uart8250_64be_0_r ) { return read64be_with_read8_handler(uart8250_0_r, offset, mem_mask); }
READ64_HANDLER ( uart8250_64be_1_r ) { return read64be_with_read8_handler(uart8250_1_r, offset, mem_mask); }
READ64_HANDLER ( uart8250_64be_2_r ) { return read64be_with_read8_handler(uart8250_2_r, offset, mem_mask); }
READ64_HANDLER ( uart8250_64be_3_r ) { return read64be_with_read8_handler(uart8250_3_r, offset, mem_mask); }
WRITE64_HANDLER ( uart8250_64be_0_w ) { write64be_with_write8_handler(uart8250_0_w, offset, data, mem_mask); }
WRITE64_HANDLER ( uart8250_64be_1_w ) { write64be_with_write8_handler(uart8250_1_w, offset, data, mem_mask); }
WRITE64_HANDLER ( uart8250_64be_2_w ) { write64be_with_write8_handler(uart8250_2_w, offset, data, mem_mask); }
WRITE64_HANDLER ( uart8250_64be_3_w ) { write64be_with_write8_handler(uart8250_3_w, offset, data, mem_mask); }
