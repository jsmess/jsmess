/**********************************************************************

    Sinclair ZX8302 emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

    TODO:

    - microdrive
    - RTC register write
    - serial data latching?
    - transmit interrupt timing
    - interface interrupt

*/

#include "driver.h"
#include "zx8302.h"
#include "devices/microdrv.h"
#include <time.h>

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 0

enum
{
	ZX8302_IPC_START,
	ZX8302_IPC_DATA,
	ZX8302_IPC_STOP
};

#define ZX8302_BAUD_19200	0x00
#define ZX8302_BAUD_9600	0x01
#define ZX8302_BAUD_4800	0x02
#define ZX8302_BAUD_2400	0x03
#define ZX8302_BAUD_1200	0x04
#define ZX8302_BAUD_600		0x05
#define ZX8302_BAUD_300		0x06
#define ZX8302_BAUD_75		0x07
#define ZX8302_BAUD_MASK	0x07

#define ZX8302_MODE_SER1	0x00
#define ZX8302_MODE_SER2	0x08
#define ZX8302_MODE_MDV		0x10
#define ZX8302_MODE_NET		0x18
#define ZX8302_MODE_MASK	0x18

#define ZX8302_INT_GAP			0x01
#define ZX8302_INT_INTERFACE	0x02
#define ZX8302_INT_TRANSMIT		0x04
#define ZX8302_INT_FRAME		0x08
#define ZX8302_INT_EXTERNAL		0x10

#define ZX8302_STATUS_NETWORK_PORT		0x01
#define ZX8302_STATUS_TX_BUFFER_FULL	0x02
#define ZX8302_STATUS_RX_BUFFER_FULL	0x04
#define ZX8302_STATUS_MICRODRIVE_GAP	0x08

#define ZX8302_TXD_START	0
#define ZX8302_TXD_STOP		9
#define ZX8302_TXD_STOP2	10

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _zx8302_t zx8302_t;
struct _zx8302_t
{
	devcb_resolved_write_line	out_ipl1l_func;
	devcb_resolved_write_line	out_baudx4_func;
	devcb_resolved_write_line	out_comdata_func;

	/* registers */
	UINT8 idr;						/* IPC data register */
	UINT8 tcr;						/* transfer control register */
	UINT8 tdr;						/* transfer data register */
	UINT8 irq;						/* interrupt register */
	UINT32 ctr;						/* counter register */
	UINT8 status;					/* status register */

	/* IPC communication state */
	int comdata;					/* communication data */
	int comctl;						/* communication control */
	int ipc_state;					/* communication state */
	int ipc_rx;						/* receiving data from IPC */
	int ipc_busy;					/* IPC busy */
	int baudx4;						/* IPC baud x4 */

	/* serial transmit state */
	int tx_bits;					/* bits transmitted */
	int ser1_rxd;					/* serial 1 receive */
	int ser1_cts;					/* serial 1 clear to send */
	int ser2_txd;					/* serial 2 transmit */
	int ser2_dtr;					/* serial 2 data terminal ready */
	int netout;						/* ZXNet output */
	int netin;						/* ZXNet input */

	/* microdrive state */
	int mdrdw;						/* read/write */
	int mdselck;					/* select clock */
	int mdseld;						/* select data */
	int erase;						/* erase head */

	int mdv_motor;					/* selected motor */
	int mdv_offset[8];				/* image offset */

	UINT8 *mdv_image[8];			/* image */

	/* timers */
	emu_timer *txd_timer;			/* transmit timer */
	emu_timer *ipc_timer;			/* baud x4 timer */
	emu_timer *rtc_timer;			/* real time clock timer */
	emu_timer *gap_timer;			/* microdrive gap timer */
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE zx8302_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);

	return (zx8302_t *)device->token;
}

INLINE const zx8302_interface *get_interface(const device_config *device)
{
	assert(device != NULL);
	assert((device->type == ZX8302));
	return (const zx8302_interface *) device->static_config;
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    zx8302_interrupt - interrupt line manager
-------------------------------------------------*/

static void zx8302_interrupt(const device_config *device, UINT8 line)
{
	zx8302_t *zx8302 = get_safe_token(device);

	zx8302->irq |= line;
	devcb_call_write_line(&zx8302->out_ipl1l_func, ASSERT_LINE);
}

/*-------------------------------------------------
    zx8302_irq_status_r - interrupt status
    register
-------------------------------------------------*/

READ8_DEVICE_HANDLER( zx8302_irq_status_r )
{
	zx8302_t *zx8302 = get_safe_token(device);

	return zx8302->irq;
}

/*-------------------------------------------------
    zx8302_irq_acknowledge_w - interrupt
    acknowledge
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( zx8302_irq_acknowledge_w )
{
	zx8302_t *zx8302 = get_safe_token(device);

	zx8302->irq &= ~data;

	if (LOG) logerror("ZX8302 Interrupt Acknowledge: %02x\n", data);

	if (!zx8302->irq)
	{
		devcb_call_write_line(&zx8302->out_ipl1l_func, CLEAR_LINE);
	}
}

/*-------------------------------------------------
    TIMER_CALLBACK( zx8302_ipc_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( zx8302_ipc_tick )
{
	zx8302_t *zx8302 = get_safe_token(ptr);

	zx8302->baudx4 = !zx8302->baudx4;
	devcb_call_write_line(&zx8302->out_baudx4_func, zx8302->baudx4);
}

/*-------------------------------------------------
    zx8302_ipc_comm_tick - IPC serial transfer
-------------------------------------------------*/

static void zx8302_ipc_comm_tick(const device_config *device)
{
	/*

        IPC <-> ZX8302 serial link protocol
        ***********************************

        Send bit to IPC
        ---------------

        1. ZX start bit (COMDATA = 0)
        2. IPC clock (COMCTL = 0, COMTL = 1)
        3. ZX data bit (COMDATA = 0/1)
        4. IPC clock (COMCTL = 0, COMTL = 1)
        5. ZX stop bit (COMDATA = 1)

        Receive bit from IPC
        --------------------

        1. ZX start bit (COMDATA = 0)
        2. IPC clock (COMCTL = 0, COMTL = 1)
        3. IPC data bit (COMDATA = 0/1)
        4. IPC clock (COMCTL = 0, COMTL = 1)
        5. IPC stop bit (COMDATA = 1)

    */

	zx8302_t *zx8302 = get_safe_token(device);

	switch (zx8302->ipc_state)
	{
	case ZX8302_IPC_START:
		if (LOG) logerror("ZX8302 COMDATA START W: %x\n", 0);

		devcb_call_write_line(&zx8302->out_comdata_func, 0);
		zx8302->ipc_busy = 1;
		zx8302->ipc_state = ZX8302_IPC_DATA;
		break;

	case ZX8302_IPC_DATA:
		if (LOG) logerror("ZX8302 COMDATA W: %x\n", BIT(zx8302->idr, 1));

		zx8302->comdata = BIT(zx8302->idr, 1);
		devcb_call_write_line(&zx8302->out_comdata_func, zx8302->comdata);
		zx8302->ipc_state = ZX8302_IPC_STOP;
		break;

	case ZX8302_IPC_STOP:
		if (!zx8302->ipc_rx)
		{
			if (LOG) logerror("ZX8302 COMDATA STOP W: %x\n", 1);

			devcb_call_write_line(&zx8302->out_comdata_func, 1);
			zx8302->ipc_busy = 0;
			zx8302->ipc_state = ZX8302_IPC_START;
		}
		break;
	}
}

/*-------------------------------------------------
    zx8302_comctl_w - IPC COMCTL accessor
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( zx8302_comctl_w )
{
	if (LOG) logerror("IPC COMCTL W: %x\n", state);

	if (state == 1)
	{
		zx8302_ipc_comm_tick(device);
	}
}

/*-------------------------------------------------
    zx8302_comctl_w - IPC COMDATA accessor
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( zx8302_comdata_w )
{
	zx8302_t *zx8302 = get_safe_token(device);

	if (LOG) logerror("IPC COMDATA W: %x\n", state);

	if (zx8302->ipc_state == ZX8302_IPC_DATA || zx8302->ipc_state == ZX8302_IPC_STOP)
	{
		if (zx8302->ipc_rx)
		{
			zx8302->ipc_rx = 0;
			zx8302->ipc_busy = 0;
			zx8302->ipc_state = ZX8302_IPC_START;
		}
		else
		{
			zx8302->ipc_rx = 1;
			zx8302->comdata = state;
		}
	}
}

/*-------------------------------------------------
    TIMER_CALLBACK( zx8302_delayed_ipc_command )
-------------------------------------------------*/

static TIMER_CALLBACK( zx8302_delayed_ipc_command )
{
	zx8302_t *zx8302 = get_safe_token(ptr);

	zx8302->idr = param;
	zx8302->ipc_state = ZX8302_IPC_START;
	zx8302->ipc_rx = 0;

	zx8302_ipc_comm_tick(ptr);
}

/*-------------------------------------------------
    zx8302_ipc_command_w - IPC command register
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( zx8302_ipc_command_w )
{
	if (LOG) logerror("ZX8302 IPC Command: %02x\n", data);

	if (data != 0x01)
	{
		timer_set(device->machine,ATTOTIME_IN_NSEC(480), (void *) device, data, zx8302_delayed_ipc_command);
	}
}

/*-------------------------------------------------
    zx8302_txd - IPC serial transmit
-------------------------------------------------*/

static WRITE_LINE_DEVICE_HANDLER( zx8302_txd )
{
	zx8302_t *zx8302 = get_safe_token(device);

	switch (zx8302->tcr & ZX8302_MODE_MASK)
	{
	case ZX8302_MODE_SER1:
		zx8302->ser1_rxd = state;
		break;

	case ZX8302_MODE_SER2:
		zx8302->ser2_txd = state;
		break;

	case ZX8302_MODE_MDV:
		// TODO
		break;

	case ZX8302_MODE_NET:
		zx8302->netout = state;
		break;
	}
}

/*-------------------------------------------------
    TIMER_CALLBACK( zx8302_txd_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( zx8302_txd_tick )
{
	zx8302_t *zx8302 = get_safe_token(ptr);

	switch (zx8302->tx_bits)
	{
	case ZX8302_TXD_START:
		if (!(zx8302->irq & ZX8302_INT_TRANSMIT))
		{
			zx8302_txd(ptr, 0);
			zx8302->tx_bits++;
		}
		break;

	default:
		zx8302_txd(ptr, BIT(zx8302->tdr, 0));
		zx8302->tdr >>= 1;
		zx8302->tx_bits++;
		break;

	case ZX8302_TXD_STOP:
		zx8302_txd(ptr, 1);
		zx8302->tx_bits++;
		break;

	case ZX8302_TXD_STOP2:
		zx8302_txd(ptr, 1);
		zx8302->tx_bits = ZX8302_TXD_START;
		zx8302->status &= ~ZX8302_STATUS_TX_BUFFER_FULL;
		zx8302_interrupt(ptr, ZX8302_INT_TRANSMIT);
		break;
	}
}

/*-------------------------------------------------
    zx8302_data_w - transmit data register
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( zx8302_data_w )
{
	zx8302_t *zx8302 = get_safe_token(device);

	if (LOG) logerror("ZX8302 Data Register: %02x\n", data);

	zx8302->tdr = data;
	zx8302->status |= ZX8302_STATUS_TX_BUFFER_FULL;
}

/*-------------------------------------------------
    zx8302_control_w - serial control register
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( zx8302_control_w )
{
	zx8302_t *zx8302 = get_safe_token(device);

	int baud = (19200 >> (data & ZX8302_BAUD_MASK));
	int baudx4 = baud * 4;

	if (LOG) logerror("ZX8302 Transmit Control Register: %02x\n", data);

	zx8302->tcr = data;

	timer_adjust_periodic(zx8302->txd_timer, attotime_zero, 0, ATTOTIME_IN_HZ(baud));
	timer_adjust_periodic(zx8302->ipc_timer, attotime_zero, 0, ATTOTIME_IN_HZ(baudx4));
}

/*-------------------------------------------------
    TIMER_CALLBACK( zx8302_rtc_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( zx8302_rtc_tick )
{
	zx8302_t *zx8302 = get_safe_token(ptr);

	zx8302->ctr++;
}

/*-------------------------------------------------
    zx8302_rtc_r - clock register read
-------------------------------------------------*/

READ8_DEVICE_HANDLER( zx8302_rtc_r )
{
	zx8302_t *zx8302 = get_safe_token(device);

	switch (offset)
	{
	case 0:
		return (zx8302->ctr >> 24) & 0xff;
	case 1:
		return (zx8302->ctr >> 16) & 0xff;
	case 2:
		return (zx8302->ctr >> 8) & 0xff;
	case 3:
		return zx8302->ctr;
	}

	return 0;
}

/*-------------------------------------------------
    zx8302_rtc_w - clock register write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( zx8302_rtc_w )
{
//  zx8302_t *zx8302 = get_safe_token(device);

	if (LOG) logerror("ZX8302 Real Time Clock Register: %02x\n", data);
}

/*-------------------------------------------------
    TIMER_CALLBACK( zx8302_gap_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( zx8302_gap_tick )
{
	zx8302_t *zx8302 = get_safe_token(ptr);

	if (zx8302->mdv_motor)
	{
		zx8302_interrupt(ptr, ZX8302_INT_GAP);
	}
}

/*-------------------------------------------------
    zx8302_get_selected_microdrive - gets the
    index of the selected microdrive
-------------------------------------------------*/

static int zx8302_get_selected_microdrive(const device_config *device)
{
	zx8302_t *zx8302 = get_safe_token(device);

    int i;

    for (i = 0; i < 8; i++)
    {
        if (BIT(zx8302->mdv_motor, i)) return 7 - i;
    }

    return 0;
}

/*-------------------------------------------------
    zx8302_get_microdrive_status - gets the
    status of the selected microdrive
-------------------------------------------------*/

static UINT8 zx8302_get_microdrive_status(const device_config *device)
{
	zx8302_t *zx8302 = get_safe_token(device);

	int drive = zx8302_get_selected_microdrive(device);
	UINT8 status = 0;

	if (zx8302->mdv_motor)
	{
		switch (zx8302->mdv_offset[drive] % MDV_SECTOR_LENGTH)
		{
		case MDV_OFFSET_HEADER_PREAMBLE:
			zx8302->status &= ~ZX8302_STATUS_MICRODRIVE_GAP;
			zx8302->mdv_offset[drive] += MDV_PREAMBLE_LENGTH;
			break;

		case MDV_OFFSET_HEADER:
			zx8302->status |= ZX8302_STATUS_RX_BUFFER_FULL;
			break;

		case MDV_OFFSET_DATA_PREAMBLE:
			zx8302->status &= ~ZX8302_STATUS_RX_BUFFER_FULL;
			zx8302->mdv_offset[drive] += MDV_PREAMBLE_LENGTH;
			break;

		case MDV_OFFSET_DATA:
			zx8302->status |= ZX8302_STATUS_RX_BUFFER_FULL;
			break;

		case MDV_OFFSET_GAP:
			zx8302->status &= ~ZX8302_STATUS_RX_BUFFER_FULL;
			zx8302->status |= ZX8302_STATUS_MICRODRIVE_GAP;
			zx8302->mdv_offset[drive] += MDV_GAP_LENGTH;
			break;
		}
	}

	return status;
}

/*-------------------------------------------------
    zx8302_mdv_track_r - microdrive data register
-------------------------------------------------*/

READ8_DEVICE_HANDLER( zx8302_mdv_track_r )
{
	zx8302_t *zx8302 = get_safe_token(device);

	int drive = zx8302_get_selected_microdrive(device);
	UINT8 data = zx8302->mdv_image[drive][zx8302->mdv_offset[drive]];

	zx8302->mdv_offset[drive]++;

	if (zx8302->mdv_offset[drive] == MDV_IMAGE_LENGTH)
	{
		zx8302->mdv_offset[drive] = 0;
	}

	return data;
}

/*-------------------------------------------------
    zx8302_mdv_control_w - microdrive control
    register
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( zx8302_mdv_control_w )
{
	zx8302_t *zx8302 = get_safe_token(device);

	/*

        bit     description

        0       MDSELDH
        1       MDSELCKH
        2       MDRDWL
        3       ERASE
        4
        5
        6
        7

    */

	if (LOG) logerror("ZX8302 Microdrive Control Register: %02x\n", data);

	zx8302->mdseld = BIT(data, 0);
	zx8302->mdselck = BIT(data, 1);
	zx8302->mdrdw = BIT(data, 2) ? 0 : 1;
	zx8302->erase = BIT(data, 3);

	// Microdrive selection shift register

	if (zx8302->mdselck)
	{
		int drive = 0;

		zx8302->mdv_motor >>= 1;
		zx8302->mdv_motor |= (zx8302->mdseld << 7);

		drive = zx8302_get_selected_microdrive(device);

		zx8302->mdv_offset[drive] = 0;
		zx8302->status &= ~ZX8302_STATUS_RX_BUFFER_FULL;
	}
}

/*-------------------------------------------------
    zx8302_status_r - status register
-------------------------------------------------*/

READ8_DEVICE_HANDLER( zx8302_status_r )
{
	zx8302_t *zx8302 = get_safe_token(device);

	/*

        bit     description

        0       Network port
        1       Transmit buffer full
        2       Receive buffer full
        3       Microdrive GAP
        4       SER1 DTR
        5       SER2 CTS
        6       IPC busy
        7       COMDATA

    */

	UINT8 data = 0;

	data |= zx8302->comdata << 7;
	data |= zx8302->ipc_busy << 6;
	data |= zx8302->ser1_cts << 5;
	data |= zx8302->ser2_dtr << 4;
	data |= zx8302_get_microdrive_status(device);
	data |= zx8302->status;

	if (LOG) logerror("ZX8302 Status Register: %02x\n", data);

	return data;
}

/*-------------------------------------------------
    zx8302_vsync_w - vertical sync accessor
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( zx8302_vsync_w )
{
	if (state)
	{
		zx8302_interrupt(device, ZX8302_INT_FRAME);
		if (LOG) logerror("ZX8302 Frame Interrupt\n");
	}
}

/*-------------------------------------------------
    DEVICE_START( zx8302 )
-------------------------------------------------*/

static DEVICE_START( zx8302 )
{
	zx8302_t *zx8302 = get_safe_token(device);
	const zx8302_interface *intf = get_interface(device);
	int i;

	/* resolve callbacks */
	devcb_resolve_write_line(&zx8302->out_ipl1l_func, &intf->out_ipl1l_func, device);
	devcb_resolve_write_line(&zx8302->out_baudx4_func, &intf->out_baudx4_func, device);
	devcb_resolve_write_line(&zx8302->out_comdata_func, &intf->out_comdata_func, device);

	/* set initial values */
	zx8302->comctl = 1;
	zx8302->comdata = 1;
	zx8302->idr = 1;

	zx8302->ctr = time(NULL) + 283996800;

	/* allocate microdrive images */
	for (i = 0; i < 8; i++)
	{
		zx8302->mdv_image[i] = auto_alloc_array(device->machine, UINT8, MDV_IMAGE_LENGTH);
	}

	/* create the timers */
	zx8302->txd_timer = timer_alloc(device->machine, zx8302_txd_tick, (void *)device);
	zx8302->ipc_timer = timer_alloc(device->machine, zx8302_ipc_tick, (void *)device);
	zx8302->rtc_timer = timer_alloc(device->machine, zx8302_rtc_tick, (void *)device);
	zx8302->gap_timer = timer_alloc(device->machine, zx8302_gap_tick, (void *)device);

	timer_adjust_periodic(zx8302->rtc_timer, attotime_zero, 0, ATTOTIME_IN_HZ(intf->rtc_clock/32768));
	timer_adjust_periodic(zx8302->gap_timer, attotime_zero, 0, ATTOTIME_IN_MSEC(31));

	/* register for state saving */

	state_save_register_item(device->machine, "zx8302", device->tag, 0, zx8302->idr);
	state_save_register_item(device->machine, "zx8302", device->tag, 0, zx8302->tcr);
	state_save_register_item(device->machine, "zx8302", device->tag, 0, zx8302->tdr);
	state_save_register_item(device->machine, "zx8302", device->tag, 0, zx8302->irq);
	state_save_register_item(device->machine, "zx8302", device->tag, 0, zx8302->ctr);
	state_save_register_item(device->machine, "zx8302", device->tag, 0, zx8302->status);

	state_save_register_item(device->machine, "zx8302", device->tag, 0, zx8302->comdata);
	state_save_register_item(device->machine, "zx8302", device->tag, 0, zx8302->comctl);
	state_save_register_item(device->machine, "zx8302", device->tag, 0, zx8302->ipc_state);
	state_save_register_item(device->machine, "zx8302", device->tag, 0, zx8302->ipc_rx);
	state_save_register_item(device->machine, "zx8302", device->tag, 0, zx8302->ipc_busy);
	state_save_register_item(device->machine, "zx8302", device->tag, 0, zx8302->baudx4);

	state_save_register_item(device->machine, "zx8302", device->tag, 0, zx8302->tx_bits);
	state_save_register_item(device->machine, "zx8302", device->tag, 0, zx8302->ser1_rxd);
	state_save_register_item(device->machine, "zx8302", device->tag, 0, zx8302->ser1_cts);
	state_save_register_item(device->machine, "zx8302", device->tag, 0, zx8302->ser2_txd);
	state_save_register_item(device->machine, "zx8302", device->tag, 0, zx8302->ser2_dtr);
	state_save_register_item(device->machine, "zx8302", device->tag, 0, zx8302->netout);
	state_save_register_item(device->machine, "zx8302", device->tag, 0, zx8302->netin);

	state_save_register_item(device->machine, "zx8302", device->tag, 0, zx8302->mdrdw);
	state_save_register_item(device->machine, "zx8302", device->tag, 0, zx8302->mdselck);
	state_save_register_item(device->machine, "zx8302", device->tag, 0, zx8302->mdseld);
	state_save_register_item(device->machine, "zx8302", device->tag, 0, zx8302->erase);

	state_save_register_item(device->machine, "zx8302", device->tag, 0, zx8302->mdv_motor);
	state_save_register_item_array(device->machine, "zx8302", device->tag, 0, zx8302->mdv_offset);
}

/*-------------------------------------------------
    DEVICE_RESET( zx8301 )
-------------------------------------------------*/

static DEVICE_RESET( zx8302 )
{
//  zx8302_t *zx8302 = get_safe_token(device);

}

/*-------------------------------------------------
    DEVICE_GET_INFO( zx8301 )
-------------------------------------------------*/

DEVICE_GET_INFO( zx8302 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(zx8302_t);						break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;									break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;				break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(zx8302);		break;
		case DEVINFO_FCT_STOP:							/* Nothing */									break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(zx8302);		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Sinclair ZX8302");				break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Sinclair QL");					break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");							break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);						break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");			break;
	}
}
