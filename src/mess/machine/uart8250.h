/**********************************************************************

	8250 UART interface and emulation

**********************************************************************/

#ifndef UART8250_H
#define UART8250_H

typedef enum { TYPE8250, TYPE16550 } UART_TYPE;

typedef struct
{
	UART_TYPE type;
	long clockin;
	void (*interrupt)(int id, int int_state);

#define UART8250_HANDSHAKE_OUT_DTR 0x01
#define UART8250_HANDSHAKE_OUT_RTS 0x02
	void (*transmit)(int id, int data);
	void (*handshake_out)(int id, int data);

	// refresh object connected to this port
	void (*refresh_connected)(int id);
} uart8250_interface;



void uart8250_init(int n, const uart8250_interface *);
void uart8250_reset(int n);
void uart8250_receive(int n, int data);

#define UART8250_HANDSHAKE_IN_DSR 0x020
#define UART8250_HANDSHAKE_IN_CTS 0x010
#define UART8250_INPUTS_RING_INDICATOR 0x0040
#define UART8250_INPUTS_DATA_CARRIER_DETECT 0x0080
void uart8250_handshake_in(int n, int data);

UINT8 uart8250_r(int n, offs_t offset);
void uart8250_w(int n, offs_t offset, UINT8 data);

/* helpers */
READ8_HANDLER ( uart8250_0_r );
READ8_HANDLER ( uart8250_1_r );
READ8_HANDLER ( uart8250_2_r );
READ8_HANDLER ( uart8250_3_r );
WRITE8_HANDLER ( uart8250_0_w );
WRITE8_HANDLER ( uart8250_1_w );
WRITE8_HANDLER ( uart8250_2_w );
WRITE8_HANDLER ( uart8250_3_w );

READ32_HANDLER ( uart8250_32le_0_r );
READ32_HANDLER ( uart8250_32le_1_r );
READ32_HANDLER ( uart8250_32le_2_r );
READ32_HANDLER ( uart8250_32le_3_r );
WRITE32_HANDLER ( uart8250_32le_0_w );
WRITE32_HANDLER ( uart8250_32le_1_w );
WRITE32_HANDLER ( uart8250_32le_2_w );
WRITE32_HANDLER ( uart8250_32le_3_w );

READ64_HANDLER ( uart8250_64be_0_r );
READ64_HANDLER ( uart8250_64be_1_r );
READ64_HANDLER ( uart8250_64be_2_r );
READ64_HANDLER ( uart8250_64be_3_r );
WRITE64_HANDLER ( uart8250_64be_0_w );
WRITE64_HANDLER ( uart8250_64be_1_w );
WRITE64_HANDLER ( uart8250_64be_2_w );
WRITE64_HANDLER ( uart8250_64be_3_w );


#endif /* UART8250_H */
