/*
    tms9901.h: header file for tms9901.c

    Raphael Nabet
*/
#ifndef __TMS9901_H__
#define __TMS9901_H__

/***************************************************************************
    MACROS
***************************************************************************/

DECLARE_LEGACY_DEVICE(TMS9901, tms9901);

/* Masks for the interrupts levels available on TMS9901 */
#define TMS9901_INT1 0x0002
#define TMS9901_INT2 0x0004
#define TMS9901_INT3 0x0008		/* overriden by the timer interrupt */
#define TMS9901_INT4 0x0010
#define TMS9901_INT5 0x0020
#define TMS9901_INT6 0x0040
#define TMS9901_INT7 0x0080
#define TMS9901_INT8 0x0100
#define TMS9901_INT9 0x0200
#define TMS9901_INTA 0x0400
#define TMS9901_INTB 0x0800
#define TMS9901_INTC 0x1000
#define TMS9901_INTD 0x2000
#define TMS9901_INTE 0x4000
#define TMS9901_INTF 0x8000

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef void (*tms9901_int_callback_func)(device_t *device, int intreq, int ic);
#define TMS9901_INT_CALLBACK(name)	void name(device_t *device, int intreq, int ic )

typedef struct _tms9901_interface tms9901_interface;
struct _tms9901_interface
{
	int supported_int_mask;	/* a bit for each input pin whose state is always notified to the TMS9901 core */
	read8_device_func  read_handlers[4];	  /* 4*8 bits */
	write8_device_func write_handlers[16]; /* 16 Pn outputs */
	tms9901_int_callback_func interrupt_callback; /* called when interrupt bus state changes */
	double clock_rate;
};


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

void tms9901_set_single_int(device_t *device, int pin_number, int state);

READ8_DEVICE_HANDLER ( tms9901_cru_r );
WRITE8_DEVICE_HANDLER( tms9901_cru_w );

/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MCFG_TMS9901_ADD(_tag, _intrf) \
	MCFG_DEVICE_ADD(_tag, TMS9901, 0) \
	MCFG_DEVICE_CONFIG(_intrf)

#endif /* __TMS9901_H__ */
