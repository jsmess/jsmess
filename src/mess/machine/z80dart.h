/***************************************************************************

    Z80 DART Dual Asynchronous Receiver/Transmitter implementation

    Copyright (c) 2008, The MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

****************************************************************************
                            _____   _____
                    D1   1 |*    \_/     | 40  D0
                    D3   2 |             | 39  D2
                    D5   3 |             | 38  D4
                    D7   4 |             | 37  D6
                  _INT   5 |             | 36  _IORQ
                   IEI   6 |             | 35  _CE
                   IEO   7 |             | 34  B/_A
                   _M1   8 |             | 33  C/_D
                   Vdd   9 |             | 32  _RD
               _W/RDYA  10 |  Z80-DART   | 31  GND
                  _RIA  11 |             | 30  _W/RDYB
                  RxDA  12 |             | 29  _RIB
                 _RxCA  13 |             | 28  RxDB
                 _TxCA  14 |             | 27  _RxTxCB
                  TxDA  15 |             | 26  TxDB
                 _DTRA  16 |             | 25  _DTRB
                 _RTSA  17 |             | 24  _RTSB
                 _CTSA  18 |             | 23  _CTSB
                 _DCDA  19 |             | 22  _DCDB
                   CLK  20 |_____________| 21  _RESET

***************************************************************************/

#ifndef __Z80DART_H_
#define __Z80DART_H_

/***************************************************************************
    CALLBACK FUNCTION PROTOTYPES
***************************************************************************/

typedef void (*z80dart_on_int_changed_func) (const device_config *device, int state);
#define Z80DART_ON_INT_CHANGED(name) void name(const device_config *device, int state)

typedef int (*z80dart_rxd_read_func) (const device_config *device, int channel);
#define Z80DART_RXD_READ(name) int name(const device_config *device, int channel)

typedef void (*z80dart_on_txd_changed_func) (const device_config *device, int channel, int level);
#define Z80DART_ON_TXD_CHANGED(name) void name(const device_config *device, int channel, int level)

typedef void (*z80dart_on_wrdy_changed_func) (const device_config *device, int channel, int level);
#define Z80DART_ON_WRDY_CHANGED(name) void name(const device_config *device, int channel, int level)

typedef void (*z80dart_on_rts_changed_func) (const device_config *device, int channel, int level);
#define Z80DART_ON_RTS_CHANGED(name) void name(const device_config *device, int channel, int level)

typedef void (*z80dart_on_dtr_changed_func) (const device_config *device, int channel, int level);
#define Z80DART_ON_DTR_CHANGED(name) void name(const device_config *device, int channel, int level)

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

enum
{
	Z80DART_CH_A = 0,
	Z80DART_CH_B
};

typedef struct _z80dart_interface z80dart_interface;
struct _z80dart_interface
{
	const char *cpu;		/* cpu tag */
	int clock;				/* base clock */

	int rx_clock_a;			/* channel A receive clock */
	int tx_clock_a;			/* channel A transmit clock */
	int rx_tx_clock_b;		/* channel B receive/transmit clock */

	/* this gets called for every change of the INT pin (pin 5) */
	z80dart_on_int_changed_func		on_int_changed;

	/* this gets called for every read of the RxDA/RxDB pins (pin 12/28) */
	z80dart_rxd_read_func			rxd_r;

	/* this gets called for every change of the TxDA/TxDB pins (pin 15/26) */
	z80dart_on_txd_changed_func		on_txd_changed;

	/* this gets called for every change of the DTRA/DTRB pins (pin 16/25) */
	z80dart_on_dtr_changed_func		on_dtr_changed;

	/* this gets called for every change of the RTSA/RTSB pins (pin 17/24) */
	z80dart_on_rts_changed_func		on_rts_changed;

	/* this gets called for every change of the WRDYA/WRDYB pins (pin 10/30) */
	z80dart_on_wrdy_changed_func	on_wrdy_changed;
};
#define Z80DART_INTERFACE(name) const z80dart_interface (name)=

/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MDRV_Z80DART_ADD(_tag, _intrf) \
	MDRV_DEVICE_ADD(_tag, Z80DART, 0) \
	MDRV_DEVICE_CONFIG(_intrf)

#define MDRV_Z80DART_REMOVE(_tag) \
	MDRV_DEVICE_REMOVE(_tag, Z80DART)

/***************************************************************************
    CONTROL REGISTER READ/WRITE
***************************************************************************/

WRITE8_DEVICE_HANDLER( z80dart_c_w );
READ8_DEVICE_HANDLER( z80dart_c_r );

/***************************************************************************
    DATA REGISTER READ/WRITE
***************************************************************************/

WRITE8_DEVICE_HANDLER( z80dart_d_w );
READ8_DEVICE_HANDLER( z80dart_d_r );

/***************************************************************************
    CLOCK MANAGEMENT
***************************************************************************/

void z80dart_rxca_w(const device_config *device);
void z80dart_txca_w(const device_config *device);
void z80dart_rxtxcb_w(const device_config *device);

/***************************************************************************
    CONTROL LINE READ/WRITE
***************************************************************************/

void z80dart_receive_data(const device_config *device, int channel, UINT8 data);

void z80dart_ri_w(const device_config *device, int channel, int level);
void z80dart_dcd_w(const device_config *device, int channel, int level);
void z80dart_cts_w(const device_config *device, int channel, int level);

/***************************************************************************
    READ/WRITE HANDLERS
***************************************************************************/

/* A0 = B/A
   A1 = C/D */
READ8_DEVICE_HANDLER( z80dart_r );
WRITE8_DEVICE_HANDLER( z80dart_w );

/* A0 = C/D
   A1 = B/A */
READ8_DEVICE_HANDLER( z80dart_alt_r );
WRITE8_DEVICE_HANDLER( z80dart_alt_w );

/***************************************************************************
    DEVICE INTERFACE
***************************************************************************/

#define Z80DART DEVICE_GET_INFO_NAME(z80dart)
DEVICE_GET_INFO( z80dart );

#define Z8470 DEVICE_GET_INFO_NAME(z8470)
DEVICE_GET_INFO( z8470 );

#define LH0081 DEVICE_GET_INFO_NAME(lh0088)
DEVICE_GET_INFO( lh0088 );

#define MK3881 DEVICE_GET_INFO_NAME(mk3888)
DEVICE_GET_INFO( mk3888 );

#endif
