/**********************************************************************

    Motorola 6850 ACIA interface and emulation

    This function is a simple emulation of a pair of MC6850
    Asynchronous Communications Interface Adapters.
    This has largely been stolen from the Sente SAC-1 driver
    by Aaron Giles

**********************************************************************/

#include "driver.h"

#define LOGM4(x)	logerror x
#define LOGMV(x)	logerror x

/* 68k 6850 states */
static UINT8 m6850_status;
static UINT8 m6850_control;
static UINT8 m6850_input;
static UINT8 m6850_output;
static UINT8 m6850_output_temp;
static UINT8 m6850_data_ready;

/* 6809 CPU 6850 states */
static UINT8 m6850_mpu4_status;
static UINT8 m6850_mpu4_control;
static UINT8 m6850_mpu4_input;
static UINT8 m6850_mpu4_output;
static UINT8 m6850_mpu4_output_temp;
static UINT8 m6850_mpu4_data_ready;

UINT8 m6850_irq_state; // referenced from drivers/mpu4.c
void update_mpu68_interrupts(void); // referenced from drivers/mpu4.c

void cpu0_irq(int state);
/*************************************
 *
 *  6850 UART communications
 *
 *************************************/

static void m6850_update_io(void)
{
	UINT8 new_state;

	/* mpu4 -> main CPU communications */
	if (m6850_mpu4_data_ready)
	{
		/* set the overrun bit if the data in the destination hasn't been read yet */
		if (m6850_status & 0x01)
			m6850_status |= 0x20;

		/* copy the mpu4's output to our input */
		m6850_input = m6850_mpu4_output;
		LOGMV(("VID ACIA INPUT = %x \n",m6850_input));

		/* set the receive register full bit */
		m6850_status |= 0x01;

		/* set the mpu4's transmitter register empty bit */
		m6850_mpu4_status |= 0x02;
		m6850_mpu4_data_ready = 0;
	}

	/* main -> mpu4 CPU communications */
	if (m6850_data_ready)
	{
		/* set the overrun bit if the data in the destination hasn't been read yet */
		if (m6850_mpu4_status & 0x01)
			m6850_mpu4_status |= 0x20;

		/* copy the main CPU's output to our input */
		m6850_mpu4_input = m6850_output;
		LOGM4(("MPU4 ACIA INPUT = %x \n",m6850_mpu4_input));
		/* set the receive register full bit */
		m6850_mpu4_status |= 0x01;

		/* set the main CPU's trasmitter register empty bit */
		m6850_status |= 0x02;
		m6850_data_ready = 0;
	}

	/* check for reset states */
	if ((m6850_control & 3) == 3)
	{
		m6850_status = 0x02;
	}
	if ((m6850_mpu4_control & 3) == 3)
		m6850_mpu4_status = 0x02;

	/* check for transmit/receive IRQs on the main CPU */
	new_state = 0;
	if ((m6850_control & 0x80) && (m6850_status & 0x21)) new_state = 1;
	if ((m6850_control & 0x60) == 0x20 && (m6850_status & 0x02)) new_state = 1;

	/* apply the change */
	if (new_state && !(m6850_status & 0x80))
	{
		m6850_irq_state = 1;
		update_mpu68_interrupts();
		m6850_status |= 0x80;
	}
	else if (!new_state && (m6850_status & 0x80))
	{
		m6850_irq_state = 0;
		update_mpu68_interrupts();
		m6850_status &= ~0x80;
	}

	/* check for transmit/receive IRQs on the mpu4 CPU */
	new_state = 0;
	if ((m6850_mpu4_control & 0x80) && (m6850_mpu4_status & 0x21)) new_state = 1;
	if ((m6850_mpu4_control & 0x60) == 0x20 && (m6850_mpu4_status & 0x02)) new_state = 1;

	/* apply the change */
	if (new_state && !(m6850_mpu4_status & 0x80))
	{
		cpu0_irq(1);
		m6850_mpu4_status |= 0x80;
	}
	else if (!new_state && (m6850_mpu4_status & 0x80))
	{
		cpu0_irq(0);
		m6850_mpu4_status &= ~0x80;
	}
}

/*************************************
 *
 *  6850 UART (main CPU)
 *
 *************************************/

READ16_HANDLER( vidcard_uart_rx_r )
{
	int result;
//  offset = (offset << 8 | 0x00ff);

	/* status register is at offset 0 */
	if (offset == 0)
	{
		result = m6850_status;
		LOGMV(("MPU4 ACIA STAT = %x \n",m6850_status));
	}

	/* input register is at offset 1 */
	else
	{
		result = m6850_input;

		/* clear the overrun and receive buffer full bits */
		m6850_status &= ~0x21;
		m6850_update_io();
	}
	return ((result << 8) | 0x00ff);
}


static void m6850_data_ready_callback(int param)
{
	/* set the output data byte and indicate that we're ready to go */
	m6850_output = param;
	m6850_data_ready = 1;
	m6850_update_io();
}

static void m6850_mpu4_data_ready_callback(int param)
{
	/* set the output data byte and indicate that we're ready to go */
	m6850_mpu4_output = param;
	m6850_mpu4_data_ready = 1;
	m6850_update_io();
}


static void m6850_w_callback(int param)
{
	/* indicate that the transmit buffer is no longer empty and update the I/O state */
	m6850_status &= ~0x02;
	m6850_output_temp = param;
	m6850_update_io();

	/* set a timer for 500usec later to actually transmit the data */
//  timer_set(TIME_IN_USEC(500), param, m6850_data_ready_callback);
}

static void m6850_mpu4_w_callback(int param)
{
	/* indicate that the transmit buffer is no longer empty and update the I/O state */
	m6850_mpu4_status &= ~0x02;
	m6850_mpu4_output_temp = param;
	m6850_update_io();

	/* set a timer for 500usec later to actually transmit the data */
//  timer_set(TIME_IN_USEC(500), param, m6850_data_ready_callback);
}

WRITE16_HANDLER( vidcard_uart_tx_w )
{
//  offset = (offset << 8 | 0x00ff);
	/* control register is at offset 0 */
	if (offset == 0)
	{
		m6850_control = (data & 0xff);
		LOGMV(("VID ACIA CTRL = %x \n",m6850_control));

		/* re-update since interrupt enables could have been modified */
		m6850_update_io();
	}

	/* output register is at offset 1; set a timer to synchronize the CPUs */
	else
		timer_set(TIME_NOW, (data & 0xff), m6850_w_callback);
		LOGMV(("VID ACIA SENDING = %x \n",data&0xff));
}



/*************************************
 *
 *  6850 UART (mpu4 CPU)
 *
 *************************************/

READ8_HANDLER( mpu4_uart_rx_r )
{
	int result;

	/* status register is at offset 0 */
	if (offset == 0)
	{
		result = m6850_mpu4_status;
		LOGM4(("MPU4 ACIA STAT = %x \n",m6850_mpu4_status));
	}

	/* input register is at offset 1 */
	else
	{
		result = m6850_mpu4_input;

		/* clear the overrun and receive buffer full bits */
		m6850_mpu4_status &= ~0x21;
		m6850_update_io();
	}

	return result;
}


WRITE8_HANDLER( mpu4_uart_tx_w )
{
	/* control register is at offset 0 */
	if (offset == 0)
	{
		m6850_mpu4_control = data;
		LOGM4(("MPU4 ACIA CTRL = %x \n",m6850_mpu4_control));
	}
	/* output register is at offset 1 */
	else
	{
//      m6850_mpu4_output_temp = data;
//      m6850_mpu4_status &= ~0x02;
		timer_set(TIME_NOW, (data), m6850_mpu4_w_callback);
		LOGM4(("MPU4 ACIA SENDING = %x \n",data));
	}

	/* re-update since interrupt enables could have been modified */
	m6850_update_io();
}

