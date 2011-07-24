#ifndef __TMS9902_H__
#define __TMS9902_H__

/***************************************************************************
    MACROS
***************************************************************************/

DECLARE_LEGACY_DEVICE(TMS9902, tms9902);

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef void (*tms9902_int_callback_func)(device_t *device, int state);
#define TMS9902_INT_CALLBACK(name)	void name(device_t *device, int state )

typedef void (*tms9902_rcv_callback_func)(device_t *device, double baudpoll);
#define TMS9902_RCV_CALLBACK(name)	void name(device_t *device, double baudpoll)

typedef void (*tms9902_xmit_callback_func)(device_t *device, int data);
#define TMS9902_XMIT_CALLBACK(name)	void name(device_t *device, int data )

typedef void (*tms9902_ctrl_callback_func)(device_t *device, int type, int param, int value);
#define TMS9902_CTRL_CALLBACK(name)	void name(device_t *device, int type, int param, int value )

// Serial control protocol values
#define TYPE_TMS9902 0x01

// Configuration (output only)
#define CONFIG   0x80
#define RATERECV 0x70
#define RATEXMIT 0x60
#define DATABITS 0x50
#define STOPBITS 0x40
#define PARITY   0x30

// Exceptional states (BRK: both directions; FRMERR/PARERR: input only)
#define EXCEPT 0x40
#define BRK    0x02
#define FRMERR 0x04
#define PARERR 0x06

// Line states (RTS, DTR: output; CTS, DSR, RI, DCD: input)
#define LINES 0x00
#define RTS 0x20
#define CTS 0x10
#define DSR 0x08
#define DCD 0x04
#define DTR 0x02
#define RI  0x01

typedef struct _tms9902_interface tms9902_interface;
struct _tms9902_interface
{
	double clock_rate;							/* clock rate (2MHz-3.3MHz, with 4MHz overclocking) */
	tms9902_int_callback_func  int_callback;	/* called when interrupt pin state changes */
	tms9902_rcv_callback_func  rcv_callback;	/* called when a character is received */
	tms9902_xmit_callback_func xmit_callback;	/* called when a character is transmitted */
	tms9902_ctrl_callback_func ctrl_callback;	/* called for configuration and line changes */
};


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/*
    Functions called by the modem.
*/
void tms9902_rcv_cts(device_t *device, line_state state);
void tms9902_rcv_dsr(device_t *device, line_state state);
void tms9902_rcv_data(device_t *device, UINT8 data);

/*
    Functions to set internal exception states, according to the states
    as received by the serial bridge
*/
void tms9902_rcv_framing_error(device_t *device);
void tms9902_rcv_parity_error(device_t *device);
void tms9902_rcv_break(device_t *device, bool value);

/*
    Control functions
*/
void tms9902_clock(device_t *device, bool state);

READ8_DEVICE_HANDLER ( tms9902_cru_r );
WRITE8_DEVICE_HANDLER( tms9902_cru_w );

/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MCFG_TMS9902_ADD(_tag, _intrf) \
	MCFG_DEVICE_ADD(_tag, TMS9902, 0) \
	MCFG_DEVICE_CONFIG(_intrf)

#endif /* __TMS9902_H__ */
