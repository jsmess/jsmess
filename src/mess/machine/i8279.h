/***************************************************************************

    Intel 8237 Programmable DMA Controller emulation

    Copyright Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

****************************************************************************
                            _____   _____
                 _I/OR   1 |*    \_/     | 40  A7
                 _I/OW   2 |             | 39  A6
                 _MEMR   3 |             | 38  A5
                 _MEMW   4 |             | 37  A4
                         5 |             | 36  _EOP
                 READY   6 |             | 35  A3
                  HLDA   7 |             | 34  A2
                 ADSTB   8 |             | 33  A1
                   AEN   9 |             | 32  A0
                   HRQ  10 |     8237    | 31  Vcc
                   _CS  11 |             | 30  DB0
                   CLK  12 |             | 29  DB1
                 RESET  13 |             | 28  DB2
                 DACK2  14 |             | 27  DB3
                 DACK3  15 |             | 26  DB4
                 DREQ3  16 |             | 25  DACK0
                 DREQ2  17 |             | 24  DACK1
                 DREQ1  18 |             | 23  DB5
                 DREQ0  19 |             | 22  DB6
                   GND  20 |_____________| 21  DB7

***************************************************************************/

#pragma once

#ifndef __I8279__
#define __I8279__

#include "emu.h"



/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MCFG_I8279_ADD(_tag, _clock, _config) \
	MCFG_DEVICE_ADD(_tag, I8279, _clock) \
	MCFG_DEVICE_CONFIG(_config)

#define I8279_INTERFACE(_name) \
	const i8279_interface (_name) =


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/


// ======================> i8279_interface

struct i8279_interface
{
	devcb_write_line	m_out_hrq_cb;
	devcb_write_line	m_out_eop_cb;

	/* accessors to main memory */
	devcb_read8			m_in_memr_cb;
	devcb_write8		m_out_memw_cb;

	/* channel accessors */
	devcb_read8			m_in_ior_cb[4];
	devcb_write8		m_out_iow_cb[4];
	devcb_write_line	m_out_dack_cb[4];
};



// ======================> i8279_device

class i8279_device :  public device_t,
                      public i8279_interface
{
public:
    // construction/destruction
    i8279_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	/* register access */
	UINT8 i8279_r(UINT32 offset);
	void i8279_w(UINT32 offset, UINT8 data);

	/* hold acknowledge */
	void i8279_hlda_w(UINT8 state) { m_hlda = state; }

	/* data request */
	void i8279_drq_write(int channel, int state);

	void i8279_timerproc();

protected:
    // device-level overrides
    virtual void device_config_complete();
    virtual void device_start();
    virtual void device_reset();
    virtual void device_post_load() { }
    virtual void device_clock_changed() { }

	static TIMER_CALLBACK( i8279_timerproc_callback );
	static TIMER_CALLBACK( receive_event_callback );

private:

	void i8279_do_read();
	void i8279_do_write();
	void i8279_advance();
	void i8327_set_dack(int channel);

	/* States that the i8279 device can be in */
	enum dma8279_state
	{
		DMA8279_SI,			/* Idle state */
		DMA8279_S0,			/* HRQ has been triggered, waiting to receive HLDA */
	//  DMA8279_SW,         /* Wait state */
		DMA8279_SC,			/* Cascade mode, waiting for peripheral */

		/* Normal transfer states */
		DMA8279_S1,			/* Output A8-A15; only used when A8-A15 really needs to be output */
		DMA8279_S2,			/* Output A0-A7 */
		DMA8279_S3,			/* Initiate read; skipped in compressed timing. On the S2->S3 transition DACK is set. */
		DMA8279_S4,			/* Perform read/write */

		/* Memory to memory transfer states */
		DMA8279_S11,		/* Output A8-A15 */
	//  DMA8279_S12,        /* Output A0-A7 */
	//  DMA8279_S13,        /* Initiate read */
	//  DMA8279_S14,        /* Perform read/write */
	//  DMA8279_S21,        /* Output A8-A15 */
	//  DMA8279_S22,        /* Output A0-A7 */
	//  DMA8279_S23,        /* Initiate read */
	//  DMA8279_S24,        /* Perform read/write */
	};

	devcb_resolved_write_line	m_out_hrq_func;
	devcb_resolved_write_line	m_out_eop_func;
	devcb_resolved_read8		m_in_memr_func;
	devcb_resolved_write8		m_out_memw_func;

	emu_timer *m_timer;

	struct
	{
		devcb_resolved_read8		m_in_ior_func;
		devcb_resolved_write8		m_out_iow_func;
		devcb_resolved_write_line	m_out_dack_func;
		UINT16 m_base_address;
		UINT16 m_base_count;
		UINT16 m_address;
		UINT16 m_count;
		UINT8 m_mode;
		int m_high_address_changed;
	} m_chan[4];

	UINT32 m_msb : 1;
	UINT32 m_eop : 1;
	UINT8 m_temp;
	UINT8 m_temporary_data;
	UINT8 m_command;
	UINT8 m_drq;
	UINT8 m_mask;
	UINT8 m_hrq;
	UINT8 m_hlda;

	/* bits  0- 3 :  Terminal count for channels 0-3
     * bits  4- 7 :  Transfer in progress for channels 0-3 */
	UINT8 m_status;

	dma8279_state m_state;		/* State the device is currently in */
	int m_service_channel;		/* Channel we will be servicing */
	int m_last_service_channel;	/* Previous channel we serviced; used to determine channel priority. */
};


// device type definition
extern const device_type I8279;



/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* register access */
READ8_DEVICE_HANDLER( i8279_r );
WRITE8_DEVICE_HANDLER( i8279_w );

/* hold acknowledge */
WRITE_LINE_DEVICE_HANDLER( i8279_hlda_w );

/* ready */
WRITE_LINE_DEVICE_HANDLER( i8279_ready_w );

/* data request */
WRITE_LINE_DEVICE_HANDLER( i8279_dreq0_w );
WRITE_LINE_DEVICE_HANDLER( i8279_dreq1_w );
WRITE_LINE_DEVICE_HANDLER( i8279_dreq2_w );
WRITE_LINE_DEVICE_HANDLER( i8279_dreq3_w );

/* end of process */
WRITE_LINE_DEVICE_HANDLER( i8279_eop_w );

#endif
