/**********************************************************************

    NEC ??PD7201 Multiprotocol Serial Communications Controller

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************
                            _____   _____
                   CLK   1 |*    \_/     | 40  Vcc
                _RESET   2 |             | 39  _CTSA
                 _DCDA   3 |             | 38  _RTSA
                 _RxCB   4 |             | 37  TxDA
                 _DCDB   5 |             | 36  _TxCA
                 _CTSB   6 |             | 35  _RxCA
                 _TxCB   7 |             | 34  RxDA
                  TxDB   8 |             | 33  _SYNCA
                  RxDB   9 |             | 32  _WAITA/DRQRxA
          _RTSB/_SYNCB  10 |   UPD7201   | 31  _DTRA/_HAO
        _WAITB/_DRQTxA  11 |             | 30  _PRO/DRQTxB
                    D7  12 |             | 29  _PRI/DRQRxB
                    D6  13 |             | 28  _INT
                    D5  14 |             | 27  _INTAK
                    D4  15 |             | 26  _DTRB/_HAI
                    D3  16 |             | 25  B/_A
                    D2  17 |             | 24  C/_D
                    D1  18 |             | 23  _CS
                    D0  19 |             | 22  _RD
                   Vss  20 |_____________| 21  _WR

**********************************************************************/

#ifndef __UPD7201_H__
#define __UPD7201_H__

#include "devcb.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

DECLARE_LEGACY_DEVICE(UPD7201, upd7201);

#define MCFG_UPD7201_ADD(_tag, _clock, _intrf) \
	MCFG_DEVICE_ADD(_tag, UPD7201, _clock) \
	MCFG_DEVICE_CONFIG(_intrf)

#define UPD7201_INTERFACE(name) \
	const upd7201_interface (name)=

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _upd7201_interface upd7201_interface;
struct _upd7201_interface
{
	devcb_write_line	out_int_func;

	struct
	{
		int	rx_clock;		/* serial receive clock */
		int	tx_clock;		/* serial transmit clock */

		devcb_write_line	out_drqrx_func;
		devcb_write_line	out_drqtx_func;

		devcb_read_line		in_rxd_func;
		devcb_write_line	out_txd_func;

		devcb_read_line		in_cts_func;
		devcb_read_line		in_dcd_func;
		devcb_write_line	out_rts_func;
		devcb_write_line	out_dtr_func;

		devcb_write_line	out_wait_func;
		devcb_write_line	out_sync_func;
	} channel[2];
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/
/* register access (A1=C/_D A0=B/_A) */
READ8_DEVICE_HANDLER( upd7201_cd_ba_r );
WRITE8_DEVICE_HANDLER( upd7201_cd_ba_w );

/* register access (A1=B/_A A0=C/_D) */
READ8_DEVICE_HANDLER( upd7201_ba_cd_r );
WRITE8_DEVICE_HANDLER( upd7201_ba_cd_w );

/* interrupt acknowledge */
READ8_DEVICE_HANDLER( upd7201_intak_r );

/* external synchronization */
WRITE_LINE_DEVICE_HANDLER( upd7201_synca_w );
WRITE_LINE_DEVICE_HANDLER( upd7201_syncb_w );

/* clear to send */
WRITE_LINE_DEVICE_HANDLER( upd7201_ctsa_w );
WRITE_LINE_DEVICE_HANDLER( upd7201_ctsb_w );

/* data terminal ready, hold acknowledge */
READ_LINE_DEVICE_HANDLER( upd7201_dtra_r );
READ_LINE_DEVICE_HANDLER( upd7201_dtrb_r );
WRITE_LINE_DEVICE_HANDLER( upd7201_hai_w );

/* serial data */
WRITE_LINE_DEVICE_HANDLER( upd7201_rxda_w );
READ_LINE_DEVICE_HANDLER( upd7201_txda_r );
WRITE_LINE_DEVICE_HANDLER( upd7201_rxdb_w );
READ_LINE_DEVICE_HANDLER( upd7201_txdb_r );

/* clock inputs */
WRITE_LINE_DEVICE_HANDLER( upd7201_rxca_w );
WRITE_LINE_DEVICE_HANDLER( upd7201_rxcb_w );
WRITE_LINE_DEVICE_HANDLER( upd7201_txca_w );
WRITE_LINE_DEVICE_HANDLER( upd7201_txcb_w );


#endif	/* __UPD7201_H__ */
