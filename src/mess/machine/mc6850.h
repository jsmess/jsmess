/**********************************************************************

	Motorola 6850 ACIA interface and emulation

	This function is a simple emulation of up to 4 MC6850
	Asynchronous Communications Interface Adapter.

**********************************************************************/

#ifndef ACIA_6850
#define ACIA_6850

#define ACIA_6850_MAX 4

#define ACIA_6850_CTRL 0
#define ACIA_6850_DATA 1

#define ACIA_6850_RDRF	0x01	/* Receive data register full */
#define ACIA_6850_TDRE	0x02	/* Transmit data register empty */
#define ACIA_6850_dcd	0x04	/* Data carrier detect, active low */
#define ACIA_6850_cts	0x08	/* Clear to send, active low */
#define ACIA_6850_FE	0x10	/* Framing error */
#define ACIA_6850_OVRN	0x20	/* Receiver overrun */
#define ACIA_6850_PE	0x40	/* Parity error */
#define ACIA_6850_irq	0x80	/* Interrupt request, active low */

struct acia6850_interface
{
	read8_handler in_status_func;
	read8_handler in_recv_func;
	write8_handler out_status_func;
	write8_handler out_tran_func;
};

void acia6850_unconfig(void);
void acia6850_config_old( int which, const struct acia6850_interface *intf);
void acia6850_reset(void);
int acia6850_read( int which, int offset);
void acia6850_write( int which, int offset, int data);

 READ8_HANDLER( acia6850_0_r );
 READ8_HANDLER( acia6850_1_r );
 READ8_HANDLER( acia6850_2_r );
 READ8_HANDLER( acia6850_3_r );

WRITE8_HANDLER( acia6850_0_w );
WRITE8_HANDLER( acia6850_1_w );
WRITE8_HANDLER( acia6850_2_w );
WRITE8_HANDLER( acia6850_3_w );

#endif /* ACIA_6850 */



/***** coded added by GJ to emulate the core of a 6850 */



struct MC6850_interface
{
	void (*out_transmit_data_func)(int offset, int data);
	void (*out_RTS_func)(int offset, int data);
	void (*out_IRQ_func)(int level);
};

void MC6850_Receive_Clock(int Receive_Data);
void MC6850_Reset(int clocklength);

void MC6850_data_w(int offset, int data);
int MC6850_data_r(int offset);

void MC6850_control_w(int offset, int data);
int MC6850_status_r(int offset);

void MC6850_config(const struct MC6850_interface *intf);

