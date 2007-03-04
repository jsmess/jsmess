/* Harris HD6402 or AY-3-1015 */

/* the chip has a lot of inputs and a lot of outputs and these could
be wired up in any order to a I/O output.

  Use hd6402_set_input(mask, data)

  to set an input to a specified state.

  The outputs can also be wired in any order to a I/O port, so 
  a callback is executed with the updated output state. 
  Use

	hd6402_set_callback

  to setup the callback which will be executed with the new state

 */
#include "serial.h"

/* inputs */

/* receiver register disable */
#define HD6402_INPUT_RRD		0x00000001
/* status flags disable */
#define HD6402_INPUT_SFD		0x00000002
/* data received reset */
#define HD6402_INPUT_DRR		0x00000004
/* master reset */
#define HD6402_INPUT_MR			0x00000008
/* transmit buffer register load */
#define HD6402_INPUT_TBRL		0x00000010
/* control register load */
#define HD6402_INPUT_CRL		0x00000020
/* parity inhibit */
#define HD6402_INPUT_PI			0x00000040
/* stop bit select */
#define HD6402_INPUT_SBS		0x00000080
/* character length select 1 */
#define HD6402_INPUT_CLS1		0x00000100
/* character length select 2 */
#define HD6402_INPUT_CLS2		0x00000200
/* even parity enable */
#define HD6402_INPUT_EPE		0x00000400

/* outputs */

/* parity error */
#define HD6402_OUTPUT_PE		0x00000800
/* framing error */
#define HD6402_OUTPUT_FE		0x00001000
/* overrun error */
#define HD6402_OUTPUT_OE		0x00002000
/* data received */
#define HD6402_OUTPUT_DR		0x00004000
/* transmitter buffer register empty */
#define HD6402_OUTPUT_TBRE		0x00008000
/* transmit register empty */
#define HD6402_OUTPUT_TRE		0x00010000

struct hd6402
{
	/* state of input and output pins */
	unsigned long state;

	unsigned char received_char;

	/* callback to execute when state changes */
	void (*callback)(int mask, int data);

	struct serial_connection connection;
	struct serial_receive_register receive_reg;
	struct serial_transmit_register transmit_reg;
	struct data_form data_form;
};

/* reset */
void	hd6402_reset(void);

void	hd6402_init(void);
/* set callback which will be executed when output pins change state */
void	hd6402_set_callback(void (*callback)(int,int));


/* The hd6402 has a transmit and receive clock input for external clocks.
these two functions are used to update the state of hd6402 at each clock pulse */

/* transmit clk input */
void	hd6402_transmit_clock(void);
/* receive clk input */
void	hd6402_receive_clock(void);

/* set state of input(s) */
void	hd6402_set_input(int mask, int data);

/* link the specified connection to the hd6402 */
void	hd6402_connect(struct serial_connection *other_connection);
/* transmit write (tbr pins) */
WRITE8_HANDLER(hd6402_data_w);
/* receive read (rbr pins) */
 READ8_HANDLER(hd6402_data_r);
