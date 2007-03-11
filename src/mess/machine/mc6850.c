/**********************************************************************

	Motorola 6850 ACIA interface and emulation

	This function is a simple emulation of up to 4 MC6850
	Asynchronous Communications Interface Adapter.

	Todo:
		Handle interrupts.
		Handle state changes.
**********************************************************************/

#include "driver.h"
#include "mc6850.h"

struct acia6850
{
	const struct acia6850_interface *intf;
};

static struct acia6850 acia[ACIA_6850_MAX];

void acia6850_unconfig (void)
{
	memset (&acia, 0, sizeof (acia));
}

void acia6850_config_old(int which, const struct acia6850_interface *intf)
{
	if (which >= ACIA_6850_MAX) return;
	acia[which].intf = intf;
}

void acia6850_reset (void)
{
}

int acia6850_read (int which, int offset)
{
	struct acia6850 *currptr = acia + which;
	int	val = 0;

	switch (offset)
	{
		case ACIA_6850_CTRL:
			if ((*(*currptr).intf).in_status_func)
							val = (*(*currptr).intf).in_status_func(0);
			break;
		case ACIA_6850_DATA:
			if ((*(*currptr).intf).in_recv_func)
							val = (*(*currptr).intf).in_recv_func(0);
			break;
	}
	return (val);
}

void acia6850_write (int which, int offset, int data)
{
	struct acia6850 *currptr = acia + which;

	switch (offset)
	{
		case ACIA_6850_CTRL:
			if ((*(*currptr).intf).out_status_func)
							(*(*currptr).intf).out_status_func(0, data);
			break;
		case ACIA_6850_DATA:
			if ((*(*currptr).intf).out_tran_func)
							(*(*currptr).intf).out_tran_func(0, data);
			break;
	}
}

 READ8_HANDLER( acia6850_0_r ) { return acia6850_read (0, offset); }
 READ8_HANDLER( acia6850_1_r ) { return acia6850_read (1, offset); }
 READ8_HANDLER( acia6850_2_r ) { return acia6850_read (2, offset); }
 READ8_HANDLER( acia6850_3_r ) { return acia6850_read (3, offset); }

WRITE8_HANDLER( acia6850_0_w ) { acia6850_write (0, offset, data); }
WRITE8_HANDLER( acia6850_1_w ) { acia6850_write (1, offset, data); }
WRITE8_HANDLER( acia6850_2_w ) { acia6850_write (2, offset, data); }
WRITE8_HANDLER( acia6850_3_w ) { acia6850_write (3, offset, data); }






/************************************************************************
	MC6850

	MESS Driver By:

 	Gordon Jefferyes
 	mess_bbc@gjeffery.dircon.co.uk

 ************************************************************************/


static int TDR=0;  // Transmit Data Register
static int RDR=0;  // Recieve Data Register
static int STR=0;  // Status Register
static int CTR=0;  // Control Register


static int RSR=0;  // Receive Shift Register
static int RSC=0;  // Receive Shift Counter

static int dcd_count=0;

static int IRQ=0;

// Bits 0,1 of control Register

static int divide_by_lookup[4]={1,16,64,0};
static int divide_by;


// Bits 2,3,4 of control Register

static int word_length_lookup[8]={7,7,7,7,8,8,8,8};
static int word_length;

// Parity Settings
// 2 = even
// 1 = odd
// 0 = no parity bit
static int parity_lookup[8]={2,1,2,1,0,0,2,1};
static int parity;

static int stop_bits_lookup[8]={2,2,1,1,2,1,1,1};
static int stop_bits;

// Bits 5,6 of control Register

static int RTS_lookup[4]={0,0,1,0};
static int RTS;

static int break_level_lookup[4]={0,0,0,1};
static int break_level;

// transmitting Interrupt 0 = Disabled  1 = Enabled
static int transmit_interrupt_enabled_lookup[4]={0,1,0,0};
static int transmit_interrupt_enabled;

// Bit 7 of control Register
static int receive_interrupt_enabled;

static int bytelength=10;


/* ******* call back function stuff ******* */


//Local copy of the MC6850 external procedure calls
static struct MC6850_interface
MC6850_calls= {
	0,// Transmit data ouput
	0,// Request to Send output
	0,// Interupt Request output
};

void MC6850_config(const struct MC6850_interface *intf)
{
	MC6850_calls.out_transmit_data_func=*intf->out_transmit_data_func;
	MC6850_calls.out_RTS_func          =*intf->out_RTS_func;
	MC6850_calls.out_IRQ_func          =*intf->out_IRQ_func;
}

/* ******* Data bus interface ******* */

void MC6850_data_w(int offset, int data)
{

	//logerror("MC6850 write to output buffer %02x\n",data);
	STR=STR&0xfd;
	TDR=data;
}

int MC6850_data_r(int offset)
{
	//logerror("MC6850 Read from RDR value %02x and clear flags\n",RDR);
	STR=STR&0x7e;
	// clear the IRQ
	if ((MC6850_calls.out_IRQ_func) && (IRQ==1))
	{
		(MC6850_calls.out_IRQ_func)(0);
		IRQ=0;
	}
	return RDR;
}


void MC6850_control_w(int offset, int data)
{
	int CTRT; // Temp bits from CTR
	CTR=data;

	//logerror("MC6850 set control flags to %02x\n",CTR);
	divide_by=divide_by_lookup[CTR&0x03];

	CTRT=(CTR>>2)&0x07;
	word_length=word_length_lookup[CTRT];
	parity     =parity_lookup[CTRT];
	stop_bits  =stop_bits_lookup[CTRT];

	CTRT=(CTR>>5)&0x03;
	RTS=RTS_lookup[CTRT];
	/* *** need to call the RTS output function here ***/
	break_level=break_level_lookup[CTRT]; // Don't know what this does
	transmit_interrupt_enabled=transmit_interrupt_enabled_lookup[CTRT];

	receive_interrupt_enabled=(CTR>>7)&0x01;


	bytelength=1;					// start bit
	bytelength+=word_length;		// byte
	if (parity)	bytelength+=1;		// parity bit
	bytelength+=stop_bits;			// stop bit
}


// Bit 0  Receive Data Register Full (RDRF)
// Bit 1  Transmit Data Register Empty (TDRE)
// Bit 2  Data Carrier Detect
// Bit 3  Clear-to-Send
// Bit 4  Framing Error
// Bit 5  Receiver Overrun
// Bit 6  Parity Error
// Bit 7  Interrupt Request


int MC6850_status_r(int offset)
{

	//logerror("MC6850 read status flags %02x\n",STR);
	return STR;
}




/* ******* other input lines ******* */

void MC6850_DCD(int offset, int data)
{
	STR=(STR&0xfb)|((data&1)<<2);
	// check to see if an interrupt should be made.
}

void MC6850_CTS(int offset, int data)
{
	STR=(STR&0xf7)|((data&1)<<3);
	// this also sets the Transmit Controller
}

void MC6850_Receive_Clock(int Receive_Data)
{

	// Only start clocking in the data when a start bit is sent
	if ((RSC>0) || (Receive_Data==0))
	{

		RSR=(RSR>>1)|((Receive_Data&1)<<8);
		RSC+=1;
		// 10 is the default bit size for the cassette input
		if (RSC==bytelength)
		{
			//logerror("MC6850 Full byte recieved $%02x\n",(RSR&0xff));
			if (~(STR&0x01))
			{
				RDR=(RSR&0xff);
				STR=STR|0x81; // set the receive Data Register full and set the Interrupt Request
				//logerror("MC6850 OK to pass on byte to RDR\n");
				// set the IRQ
				// only do interrupt if bit 7 of CTR is high
				if ((MC6850_calls.out_IRQ_func) && (receive_interrupt_enabled) && (IRQ==0))
				{
					(MC6850_calls.out_IRQ_func)(1);
					IRQ=1;
				};
		 	}

			 RSC=0;
			 RSR=0;
			 dcd_count=0;
		};
	} else {
		logerror("Data bit 1 not used\n");
		dcd_count+=1;
		if (dcd_count==8)
		{
			logerror("MC6850 set DCD");
			STR=STR|0x84;
			if ((MC6850_calls.out_IRQ_func) && (receive_interrupt_enabled) && (IRQ==0))
			{
				(MC6850_calls.out_IRQ_func)(1);
					IRQ=1;
			};
		} else {
			if (dcd_count==20) {
				STR=STR&0xfb;
				logerror("MC6850 clear DCD");
			}
		}
	}

}

void MC6850_Reset(int clocklength)
{
	// If a long time has passed and no data has be sent then reset the counters
	// there should be a timer in this code that calls here if no bits are recived after about 1.5 bits of time.
	// but for now I am calling externally.
	RSC=0;
	RSR=0;
	dcd_count=0;
}

int MC6850_Transmit_Clock(void)
{
	// clock out the data and return a Trasmit Data bit

	/* lots of stuff to do in here
	   if we get to outputing data too */
	return 0;
}


/* ******* other output lines ******* */

int MC6850_transmit_data(int offset)
{
	// return the last transmitted bit
	return 0;
}

int MC6850_RTS(int offset)
{
	// is this correct ????
	return RTS;
}

/*
interrupt Request is the only other output
and this does not need to be read
*/


