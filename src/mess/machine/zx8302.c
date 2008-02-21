#include "driver.h"
#include "zx8302.h"
#include <time.h>

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

#define MDV_SECTOR_COUNT	255
#define MDV_SECTOR_LENGTH	686
#define MDV_IMAGE_LENGTH	(MDV_SECTOR_COUNT * MDV_SECTOR_LENGTH)

#define MDV_PREAMBLE_LENGTH			12
#define MDV_GAP_LENGTH				120

#define MDV_OFFSET_HEADER_PREAMBLE	0
#define MDV_OFFSET_HEADER			MDV_OFFSET_HEADER_PREAMBLE + MDV_PREAMBLE_LENGTH
#define MDV_OFFSET_DATA_PREAMBLE	28
#define MDV_OFFSET_DATA				MDV_OFFSET_DATA_PREAMBLE + MDV_PREAMBLE_LENGTH
#define MDV_OFFSET_GAP				566

static struct ZX8302
{
	const zx8302_interface *intf;

	UINT8 tcr;
	UINT8 tdr;
	UINT8 irq;
	UINT32 ctr;
	UINT8 idr;
	UINT8 status;
	int tx_bits;
	int ser1_rxd, ser1_cts;
	int ser2_txd, ser2_dtr;
	int netout, netin;
	int mdrdw, mdselck, mdseld, erase;
	int mdv_motor, mdv_offset;
} zx8302;

static emu_timer *zx8302_txd_timer, *zx8302_ipc_timer, *zx8302_rtc_timer, *zx8302_gap_timer;

static UINT8 *mdv_image;

/* Interrupts */

READ8_HANDLER( zx8302_irq_status_r )
{
	return zx8302.irq;
}

WRITE8_HANDLER( zx8302_irq_acknowledge_w )
{
	zx8302.irq &= ~data;

	if (!zx8302.irq)
	{
		zx8302.intf->irq_callback(CLEAR_LINE);
	}
}

static void zx8302_interrupt(UINT8 line)
{
	zx8302.irq |= line;
	zx8302.intf->irq_callback(HOLD_LINE);
}

INTERRUPT_GEN( zx8302_int ) // remove this!
{
	zx8302_interrupt(ZX8302_INT_FRAME);
}

void zx8302_vsync_w(void)
{
	zx8302_interrupt(ZX8302_INT_FRAME);
}

/* IPC */

static TIMER_CALLBACK( zx8302_ipc_tick )
{
	*zx8302.intf->baudx4 = !(*zx8302.intf->baudx4);
}

static TIMER_CALLBACK( zx8302_delayed_ipc_command )
{
	zx8302.idr = param;
	*zx8302.intf->comdata = BIT(zx8302.idr, 1);
}

WRITE8_HANDLER( zx8302_ipc_command_w )
{
	timer_set(ATTOTIME_IN_NSEC(480), NULL, data, zx8302_delayed_ipc_command);
	*zx8302.intf->comdata = BIT(data, 0);
}

/* Serial */

static void zx8302_txd(int level)
{
	switch (zx8302.tcr & ZX8302_MODE_MASK)
	{
	case ZX8302_MODE_SER1:
		zx8302.ser1_rxd = level;
		break;

	case ZX8302_MODE_SER2:
		zx8302.ser2_txd = level;
		break;

	case ZX8302_MODE_MDV:
		// TODO
		break;

	case ZX8302_MODE_NET:
		zx8302.netout = level;
		break;
	}
}

static TIMER_CALLBACK( zx8302_txd_tick )
{
	switch (zx8302.tx_bits)
	{
	case ZX8302_TXD_START:
		if (!(zx8302.irq & ZX8302_INT_TRANSMIT))
		{
			zx8302_txd(0);
			zx8302.tx_bits++;
		}
		break;

	default:
		zx8302_txd(BIT(zx8302.tdr, 0));
		zx8302.tdr >>= 1;
		zx8302.tx_bits++;
		break;

	case ZX8302_TXD_STOP:
		zx8302_txd(1);
		zx8302.tx_bits++;
		break;

	case ZX8302_TXD_STOP2:
		zx8302_txd(1);
		zx8302.tx_bits = ZX8302_TXD_START;
		zx8302.status &= ~ZX8302_STATUS_TX_BUFFER_FULL;
		zx8302_interrupt(ZX8302_INT_TRANSMIT);
		break;
	}
}

WRITE8_HANDLER( zx8302_data_w )
{
	zx8302.tdr = data;
	zx8302.status |= ZX8302_STATUS_TX_BUFFER_FULL;
}

WRITE8_HANDLER( zx8302_control_w )
{
	int baud = (19200 >> (data & ZX8302_BAUD_MASK));
	int baudx4 = baud * 4;

	zx8302.tcr = data;

	timer_adjust_periodic(zx8302_txd_timer, attotime_zero, 0, ATTOTIME_IN_HZ(baud));
	timer_adjust_periodic(zx8302_ipc_timer, attotime_zero, 0, ATTOTIME_IN_HZ(baudx4));
}

/* Real Time Clock */

static TIMER_CALLBACK( zx8302_rtc_tick )
{
	zx8302.ctr++;
}

READ8_HANDLER( zx8302_rtc_r )
{
	switch (offset)
	{
	case 0:
		return (zx8302.ctr >> 24) & 0xff;
	case 1:
		return (zx8302.ctr >> 16) & 0xff;
	case 2:
		return (zx8302.ctr >> 8) & 0xff;
	case 3:
		return zx8302.ctr;
	}

	return 0;
}

WRITE8_HANDLER( zx8302_rtc_w )
{
	// TODO
}

/* Microdrive */

static TIMER_CALLBACK( zx8302_gap_tick )
{
	if (zx8302.mdv_motor)
	{
		zx8302_interrupt(ZX8302_INT_GAP);
	}
}

static UINT8 zx8302_get_microdrive_status(void)
{
	UINT8 status = 0;

	if (zx8302.mdv_motor)
	{
		switch (zx8302.mdv_offset % MDV_SECTOR_LENGTH)
		{
		case MDV_OFFSET_HEADER_PREAMBLE:
			zx8302.status &= ~ZX8302_STATUS_MICRODRIVE_GAP;
			zx8302.mdv_offset += MDV_PREAMBLE_LENGTH;
			break;

		case MDV_OFFSET_HEADER:
			zx8302.status |= ZX8302_STATUS_RX_BUFFER_FULL;
			break;

		case MDV_OFFSET_DATA_PREAMBLE:
			zx8302.status &= ~ZX8302_STATUS_RX_BUFFER_FULL;
			zx8302.mdv_offset += MDV_PREAMBLE_LENGTH;
			break;

		case MDV_OFFSET_DATA:
			zx8302.status |= ZX8302_STATUS_RX_BUFFER_FULL;
			break;

		case MDV_OFFSET_GAP:
			zx8302.status &= ~ZX8302_STATUS_RX_BUFFER_FULL;
			zx8302.status |= ZX8302_STATUS_MICRODRIVE_GAP;
			zx8302.mdv_offset += MDV_GAP_LENGTH;
			break;
		}
	}

	return status;
}

READ8_HANDLER( zx8302_mdv_track_r )
{
	UINT8 data = mdv_image[zx8302.mdv_offset];

	zx8302.mdv_offset++;

	if (zx8302.mdv_offset == MDV_IMAGE_LENGTH)
	{
		zx8302.mdv_offset = 0;
	}

	return data;
}

/*
static int zx8302_get_selected_microdrive(void)
{
    int i;

    for (i = 0; i < 8; i++)
    {
        if (BIT(zx8302.mdv_motor, i)) return 8 - i;
    }

    return 0;
}
*/

WRITE8_HANDLER( zx8302_mdv_control_w )
{
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

	zx8302.mdseld = BIT(data, 0);
	zx8302.mdselck = BIT(data, 1);
	zx8302.mdrdw = BIT(data, 2) ? 0 : 1;
	zx8302.erase = BIT(data, 3);

	// Microdrive selection shift register

	if (zx8302.mdselck)
	{
		zx8302.mdv_motor >>= 1;
		zx8302.mdv_motor |= (zx8302.mdseld << 7);

		zx8302.mdv_offset = 0;
		zx8302.status &= ~ZX8302_STATUS_RX_BUFFER_FULL;
	}
}

static DEVICE_LOAD( zx8302_microdrive )
{
	int	filesize = image_length(image);

	if (filesize == MDV_IMAGE_LENGTH)
	{
		if (image_fread(image, mdv_image, filesize) == filesize)
		{
			return INIT_PASS;
		}
	}

	return INIT_FAIL;
}

void zx8302_microdrive_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:							info->i = IO_CASSETTE; break;
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		case DEVINFO_INT_READABLE:						info->i = 1; break;
		case DEVINFO_INT_WRITEABLE:						info->i = 0; break;
		case DEVINFO_INT_CREATABLE:						info->i = 0; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_zx8302_microdrive; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "mdv"); break;
	}
}

/* Status */

READ8_HANDLER( zx8302_status_r )
{
	/*

        bit     description

        0       Network port
        1       Transmit buffer full
        2       Receive buffer full
        3       Microdrive GAP
        4       SER1 DTR
        5       SER2 CTS
        6       Latched COMCTL
        7       Latched COMDATA

    */

	UINT8 data = 0;

	data |= *zx8302.intf->comdata << 7;
	data |= *zx8302.intf->comctl << 6;
	data |= zx8302.ser1_cts << 5;
	data |= zx8302.ser2_dtr << 4;
	data |= zx8302_get_microdrive_status();
	data |= zx8302.status;

	*zx8302.intf->comctl = 1;

	return data;
}

/* Configuration */

void zx8302_config(const zx8302_interface *intf)
{
	state_save_register_global(zx8302.tcr);
	state_save_register_global(zx8302.tdr);
	state_save_register_global(zx8302.irq);
	state_save_register_global(zx8302.ctr);
	state_save_register_global(zx8302.idr);
	state_save_register_global(zx8302.tx_bits);
	state_save_register_global(zx8302.ser1_rxd);
	state_save_register_global(zx8302.ser1_cts);
	state_save_register_global(zx8302.ser2_txd);
	state_save_register_global(zx8302.ser2_dtr);
	state_save_register_global(zx8302.netout);
	state_save_register_global(zx8302.netin);
	state_save_register_global(zx8302.mdrdw);
	state_save_register_global(zx8302.mdselck);
	state_save_register_global(zx8302.mdseld);
	state_save_register_global(zx8302.erase);
	state_save_register_global(zx8302.mdv_motor);
	state_save_register_global(zx8302.mdv_offset);

	memset(&zx8302, 0, sizeof(zx8302));

	zx8302_txd_timer = timer_alloc(zx8302_txd_tick, NULL);
	zx8302_ipc_timer = timer_alloc(zx8302_ipc_tick, NULL);
	zx8302_rtc_timer = timer_alloc(zx8302_rtc_tick, NULL);
	zx8302_gap_timer = timer_alloc(zx8302_gap_tick, NULL);

	zx8302.intf = intf;

	timer_adjust_periodic(zx8302_rtc_timer, attotime_zero, 0, ATTOTIME_IN_HZ(intf->rtc_clock/32768));
	timer_adjust_periodic(zx8302_gap_timer, attotime_zero, 0, ATTOTIME_IN_MSEC(31));

	*zx8302.intf->comctl = 1;
	*zx8302.intf->comdata = 1;

	zx8302.ctr = time(NULL) + 283996800;

	mdv_image = auto_malloc(MDV_IMAGE_LENGTH);
}
