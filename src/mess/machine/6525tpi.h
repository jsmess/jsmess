/*****************************************************************************
 *
 * machine/tpi6525.h
 *
 * mos tri port interface 6525
 * mos triple interface adapter 6523
 *
 * peter.trauner@jk.uni-linz.ac.at
 *
 * used in commodore b series
 * used in commodore c1551 floppy disk drive
 *
 * tia6523 is a tpi6525 without control register!?
 *
 * tia6523
 *   only some lines of port b and c are in the pinout!
 *
 * connector to floppy c1551 (delivered with c1551 as c16 expansion)
 *   port a for data read/write
 *   port b
 *   0 status 0
 *   1 status 1
 *   port c
 *   6 dav output edge data on port a available
 *   7 ack input edge ready for next datum
 *
 ****************************************************************************/

#ifndef __TPI6525_H__
#define __TPI6525_H__


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _tpi6525_interface tpi6525_interface;
struct _tpi6525_interface
{
	read8_device_func in_a_func;
	read8_device_func in_b_func;
	read8_device_func in_c_func;
	write8_device_func out_a_func;
	write8_device_func out_b_func;
	write8_device_func out_c_func;
	write8_device_func out_ca_func;
	write8_device_func out_cb_func;
	void (*irq_func)(device_t *device, int level);
};


/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

DECLARE_LEGACY_DEVICE(TPI6525, tpi6525);

#define MCFG_TPI6525_ADD(_tag, _intrf) \
	MCFG_DEVICE_ADD(_tag, TPI6525, 0) \
	MCFG_DEVICE_CONFIG(_intrf)


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

READ8_DEVICE_HANDLER( tpi6525_r );
WRITE8_DEVICE_HANDLER( tpi6525_w );

READ8_DEVICE_HANDLER( tpi6525_porta_r );
WRITE8_DEVICE_HANDLER( tpi6525_porta_w );

READ8_DEVICE_HANDLER( tpi6525_portb_r );
WRITE8_DEVICE_HANDLER( tpi6525_portb_w );

READ8_DEVICE_HANDLER( tpi6525_portc_r );
WRITE8_DEVICE_HANDLER( tpi6525_portc_w );

void tpi6525_irq0_level(device_t *device, int level);
void tpi6525_irq1_level(device_t *device, int level);
void tpi6525_irq2_level(device_t *device, int level);
void tpi6525_irq3_level(device_t *device, int level);
void tpi6525_irq4_level(device_t *device, int level);

UINT8 tpi6525_get_ddr_a(device_t *device);
UINT8 tpi6525_get_ddr_b(device_t *device);
UINT8 tpi6525_get_ddr_c(device_t *device);


#endif /* __TPI6525_H__ */
