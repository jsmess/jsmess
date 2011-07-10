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

typedef void (*tms9902_rcv_callback_func)(device_t *device);
#define TMS9902_RCV_CALLBACK(name)	void name(device_t *device)

typedef void (*tms9902_xmit_callback_func)(device_t *device, int data);
#define TMS9902_XMIT_CALLBACK(name)	void name(device_t *device, int data )

typedef void (*tms9902_ctrl_callback_func)(device_t *device, int type, int data);
#define TMS9902_CTRL_CALLBACK(name)	void name(device_t *device, int type, int data )

/*
    Constants for configuration markup
*/
#define	TMS9902_CONF 0x80
#define	TMS9902_CONF_XRATE 0x81
#define TMS9902_CONF_RRATE 0x82
#define TMS9902_CONF_STOPB 0x83
#define TMS9902_CONF_DATAB 0x84
#define TMS9902_CONF_PAR   0x85

#define TMS9902_BRK 0x01

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
void tms9902_set_cts(device_t *device, int state);
void tms9902_set_dsr(device_t *device, int state);
void tms9902_receive_data(device_t *device, int data);
void tms9902_clock(device_t *device, int state);

READ8_DEVICE_HANDLER ( tms9902_cru_r );
WRITE8_DEVICE_HANDLER( tms9902_cru_w );

/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MCFG_TMS9902_ADD(_tag, _intrf) \
	MCFG_DEVICE_ADD(_tag, TMS9902, 0) \
	MCFG_DEVICE_CONFIG(_intrf)

#endif /* __TMS9902_H__ */
