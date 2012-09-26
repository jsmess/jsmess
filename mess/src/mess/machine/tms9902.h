#ifndef __TMS9902_H__
#define __TMS9902_H__

/***************************************************************************
    MACROS
***************************************************************************/

DECLARE_LEGACY_DEVICE(TMS9902, tms9902);

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef void (*tms9902_int_callback_func)(device_t *device, int INT);
#define TMS9902_INT_CALLBACK(name)	void name(device_t *device, int INT )

typedef void (*tms9902_rst_callback_func)(device_t *device, int RTS);
#define TMS9902_RST_CALLBACK(name)	void name(device_t *device, int RTS )

typedef void (*tms9902_brk_callback_func)(device_t *device, int RTS);
#define TMS9902_BRK_CALLBACK(name)	void name(device_t *device, int BRK )

typedef void (*tms9902_xmit_callback_func)(device_t *device, int data);
#define TMS9902_XMIT_CALLBACK(name)	void name(device_t *device, int data )


typedef struct _tms9902_interface tms9902_interface;
struct _tms9902_interface
{
	double clock_rate;							/* clock rate (2MHz-3.3MHz, with 4MHz overclocking) */
	tms9902_int_callback_func  int_callback;	/* called when interrupt pin state changes */
	tms9902_rst_callback_func  rts_callback;	/* called when Request To Send pin state changes */
	tms9902_brk_callback_func  brk_callback;	/* called when BReaK state changes */
	tms9902_xmit_callback_func xmit_callback;	/* called when a character is transmitted */
};


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

void tms9902_set_cts(device_t *device, int state);
void tms9902_set_dsr(device_t *device, int state);

void tms9902_push_data(device_t *device, int data);

READ8_DEVICE_HANDLER ( tms9902_cru_r );
WRITE8_DEVICE_HANDLER( tms9902_cru_w );

/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MCFG_TMS9902_ADD(_tag, _intrf) \
	MCFG_DEVICE_ADD(_tag, TMS9902, 0) \
	MCFG_DEVICE_CONFIG(_intrf)

#endif /* __TMS9902_H__ */
