/*********************************************************************

    8530scc.c

    Zilog 8530 SCC (Serial Control Chip) code

*********************************************************************/


#include "emu.h"
#include "8530scc.h"


/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG_SCC	0



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef enum
{
	IRQ_NONE,
	IRQ_A_RX,
	IRQ_A_RX_SPECIAL,
	IRQ_B_RX,
	IRQ_B_RX_SPECIAL,
	IRQ_A_TX,
	IRQ_B_TX,
	IRQ_A_EXT,
	IRQ_B_EXT
} SCCIRQType;

typedef struct
{
	int txIRQEnable;
	int rxIRQEnable;
	int extIRQEnable;
	int baudIRQEnable;
	int txIRQPending;
	int rxIRQPending;
	int extIRQPending;
	int baudIRQPending;
	int txEnable;
	int rxEnable;
	int txUnderrun;
	int txUnderrunEnable;
	int syncHunt;
	int DCDEnable;
	int CTSEnable;
	int rxData;
	int txData;

	emu_timer *baudtimer;

	UINT8 reg_val[16];
} sccChan;

typedef struct _scc8530_t scc8530_t;
struct _scc8530_t
{
	int mode;
	int reg;
	int status;
	int IRQV;
	int MasterIRQEnable;
	int lastIRQStat;
	int clock;
	SCCIRQType IRQType;

	sccChan channel[2];
};



/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE scc8530_t *get_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == SCC8530);

	return (scc8530_t *) downcast<legacy_device_base *>(device)->token();
}


INLINE const scc8530_interface *get_interface(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == SCC8530);

	return (const scc8530_interface *) downcast<const legacy_device_config_base &>(device->baseconfig()).inline_config();
}


/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    scc_updateirqs
-------------------------------------------------*/

static void scc_updateirqs(device_t *device)
{
	scc8530_t *scc = get_token(device);
	int irqstat;

	irqstat = 0;
	if (scc->MasterIRQEnable)
	{
		if ((scc->channel[0].txIRQEnable) && (scc->channel[0].txIRQPending))
		{
			scc->IRQType = IRQ_B_TX;
			irqstat = 1;
		}
		else if ((scc->channel[1].txIRQEnable) && (scc->channel[1].txIRQPending))
		{
			scc->IRQType = IRQ_A_TX;
			irqstat = 1;
		}
		else if ((scc->channel[0].extIRQEnable) && (scc->channel[0].extIRQPending))
		{
			scc->IRQType = IRQ_B_EXT;
			irqstat = 1;
		}
		else if ((scc->channel[1].extIRQEnable) && (scc->channel[1].extIRQPending))
		{
			scc->IRQType = IRQ_A_EXT;
			irqstat = 1;
		}
	}
	else
	{
		scc->IRQType = IRQ_NONE;
	}

//  printf("SCC: irqstat %d, last %d\n", irqstat, scc->lastIRQStat);
//  printf("ch0: en %d pd %d  ch1: en %d pd %d\n", scc->channel[0].txIRQEnable, scc->channel[0].txIRQPending, scc->channel[1].txIRQEnable, scc->channel[1].txIRQPending);

	// don't spam the driver with unnecessary transitions
	if (irqstat != scc->lastIRQStat)
	{
		const scc8530_interface *intf = get_interface(device);

		scc->lastIRQStat = irqstat;

		// tell the driver the new IRQ line status if possible
		if ((intf != NULL) && (intf->irq != NULL))
		{
			#if LOG_SCC
			printf("SCC8530 IRQ status => %d\n", irqstat);
			#endif
			(*intf->irq)(device, irqstat);
		}
	}
}

/*-------------------------------------------------
    scc_initchannel
-------------------------------------------------*/
static void scc8530_initchannel(scc8530_t *scc, int ch)
{
	scc->channel[ch].syncHunt = 1;
}

/*-------------------------------------------------
    scc_resetchannel
-------------------------------------------------*/
static void scc8530_resetchannel(scc8530_t *scc, int ch)
{
	emu_timer *timersave = scc->channel[ch].baudtimer;

	memset(&scc->channel[ch], 0, sizeof(sccChan));

	scc->channel[ch].txUnderrun = 1;
	scc->channel[ch].baudtimer = timersave;

	scc->channel[ch].baudtimer->adjust(attotime::never, ch);
}

/*-------------------------------------------------
    scc8530_baud_expire - baud rate timer expiry
-------------------------------------------------*/

static TIMER_CALLBACK( scc8530_baud_expire )
{
	device_t *device = (device_t *)ptr;
	scc8530_t *scc = get_token(device);
	sccChan *pChan = &scc->channel[param];
	int brconst = pChan->reg_val[13]<<8 | pChan->reg_val[14];
	int rate = scc->clock / brconst;

	// is baud counter IRQ enabled on this channel?
	// always flag pending in case it's enabled after this
	pChan->baudIRQPending = 1;
	if (pChan->baudIRQEnable)
	{
		if (pChan->extIRQEnable)
		{
			pChan->extIRQPending = 1;
			pChan->baudIRQPending = 0;
			scc_updateirqs(device);
		}
	}

	// reset timer according to current register values
	pChan->baudtimer->adjust(attotime::from_hz(rate), param, attotime::from_hz(rate));
}

/*-------------------------------------------------
    DEVICE_START( scc8530 )
-------------------------------------------------*/

static DEVICE_START( scc8530 )
{
	scc8530_t *scc = get_token(device);
	memset(scc, 0, sizeof(*scc));
	scc->clock = device->clock();

	scc->channel[0].baudtimer = device->machine().scheduler().timer_alloc(FUNC(scc8530_baud_expire), (void *)device);
	scc->channel[1].baudtimer = device->machine().scheduler().timer_alloc(FUNC(scc8530_baud_expire), (void *)device);
}


/*-------------------------------------------------
    DEVICE_RESET( scc8530 )
-------------------------------------------------*/
static DEVICE_RESET( scc8530 )
{
	scc8530_t *scc = get_token(device);

	scc->IRQType = IRQ_NONE;
	scc->MasterIRQEnable = 0;
	scc->IRQV = 0;

	scc8530_initchannel(scc, 0);
	scc8530_initchannel(scc, 1);
	scc8530_resetchannel(scc, 0);
	scc8530_resetchannel(scc, 1);
}

/*-------------------------------------------------
    scc_set_status
-------------------------------------------------*/

void scc8530_set_status(device_t *device, int status)
{
	scc8530_t *scc = get_token(device);
	scc->status = status;
}

/*-------------------------------------------------
    scc_acknowledge
-------------------------------------------------*/

static void scc_acknowledge(device_t *device)
{
	const scc8530_interface *intf = get_interface(device);
	if ((intf != NULL) && (intf->irq != NULL))
		(*intf->irq)(device, 0);
}

/*-------------------------------------------------
    scc_getareg
-------------------------------------------------*/

static int scc_getareg(device_t *device)
{
	scc8530_t *scc = get_token(device);

	/* Not yet implemented */
	#if LOG_SCC
	printf("SCC: port A reg %d read 0x%02x\n", scc->reg, scc->channel[0].reg_val[scc->reg]);
	#endif
	return scc->channel[0].reg_val[scc->reg];
}



/*-------------------------------------------------
    scc_getareg
-------------------------------------------------*/

static int scc_getbreg(device_t *device)
{
	scc8530_t *scc = get_token(device);

	#if LOG_SCC
	printf("SCC: port B reg %i read 0x%02x\n", scc->reg, scc->channel[1].reg_val[scc->reg]);
	#endif

	if (scc->reg == 2)
	{
		/* HACK! but lets the Mac Plus mouse move again.  Needs further investigation. */
		scc_acknowledge(device);

		return scc->status;
	}

	return scc->channel[1].reg_val[scc->reg];
}



/*-------------------------------------------------
    scc_putreg
-------------------------------------------------*/

static void scc_putreg(device_t *device, int ch, int data)
{
	scc8530_t *scc = get_token(device);
	sccChan *pChan = &scc->channel[ch];

	scc->channel[ch].reg_val[scc->reg] = data;
	#if LOG_SCC
	printf("SCC: port %c reg %d write 0x%02x\n", 'A'+ch, scc->reg, data);
	#endif

	switch (scc->reg)
	{
		case 0:	// command register
			switch ((data >> 3) & 7)
			{
				case 1:	// select high registers (handled elsewhere)
					break;

				case 2: // reset external and status IRQs
					pChan->syncHunt = 0;
					break;

				case 5: // ack Tx IRQ
					pChan->txIRQPending = 0;
					scc_updateirqs(device);
					break;

				case 0:	// nothing
				case 3: // send SDLC abort
				case 4: // enable IRQ on next Rx byte
				case 6: // reset errors
				case 7: // reset highest IUS
					// we don't handle these yet
					break;

			}
			break;

		case 1:	// Tx/Rx IRQ and data transfer mode defintion
			pChan->extIRQEnable = (data & 1);
			pChan->txIRQEnable = (data & 2) ? 1 : 0;
			pChan->rxIRQEnable = (data >> 3) & 3;
			scc_updateirqs(device);
			break;

		case 2: // IRQ vector
			scc->IRQV = data;
			break;

		case 3: // Rx parameters and controls
			pChan->rxEnable = (data & 1);
			pChan->syncHunt = (data & 0x10) ? 1 : 0;
			break;

		case 5: // Tx parameters and controls
//          printf("ch %d TxEnable = %d [%02x]\n", ch, data & 8, data);
			pChan->txEnable = data & 8;

			if (pChan->txEnable)
			{
				pChan->reg_val[0] |= 0x04;	// Tx empty
			}
			break;

		case 4: // Tx/Rx misc parameters and modes
		case 6: // sync chars/SDLC address field
		case 7: // sync char/SDLC flag
			break;

		case 9: // master IRQ control
			scc->MasterIRQEnable = (data & 8) ? 1 : 0;
			scc_updateirqs(device);

			// channel reset command
			switch ((data>>6) & 3)
			{
				case 0:	// do nothing
					break;

				case 1:	// reset channel B
					scc8530_resetchannel(scc, 0);
					break;

				case 2: // reset channel A
					scc8530_resetchannel(scc, 1);
					break;

				case 3: // force h/w reset (entire chip)
					scc->IRQType = IRQ_NONE;
					scc->MasterIRQEnable = 0;
					scc->IRQV = 0;

					scc8530_initchannel(scc, 0);
					scc8530_initchannel(scc, 1);
					scc8530_resetchannel(scc, 0);
					scc8530_resetchannel(scc, 1);

					// make sure we stop yanking the IRQ line if we were
					scc_updateirqs(device);
					break;

			}
			break;

		case 10:	// misc transmitter/receiver control bits
		case 11:	// clock mode control
		case 12:	// lower byte of baud rate gen
		case 13:	// upper byte of baud rate gen
			break;

		case 14:	// misc control bits
			if (data & 0x01)	// baud rate generator enable?
			{
				int brconst = pChan->reg_val[13]<<8 | pChan->reg_val[14];
				int rate = scc->clock / brconst;

				pChan->baudtimer->adjust(attotime::from_hz(rate), ch, attotime::from_hz(rate));
			}
			break;

		case 15:	// external/status interrupt control
			pChan->baudIRQEnable = (data & 2) ? 1 : 0;
			pChan->DCDEnable = (data & 8) ? 1 : 0;
			pChan->CTSEnable = (data & 0x20) ? 1 : 0;
			pChan->txUnderrunEnable = (data & 0x40) ? 1 : 0;
			break;
	}
}

/*-------------------------------------------------
    scc8530_get_reg_a
-------------------------------------------------*/

UINT8 scc8530_get_reg_a(device_t *device, int reg)
{
	scc8530_t *scc = get_token(device);
	return scc->channel[0].reg_val[reg];
}



/*-------------------------------------------------
    scc8530_get_reg_b
-------------------------------------------------*/

UINT8 scc8530_get_reg_b(device_t *device, int reg)
{
	scc8530_t *scc = get_token(device);
	return scc->channel[1].reg_val[reg];
}



/*-------------------------------------------------
    scc8530_set_reg_a
-------------------------------------------------*/

void scc8530_set_reg_a(device_t *device, int reg, UINT8 data)
{
	scc8530_t *scc = get_token(device);
	scc->channel[0].reg_val[reg] = data;
}



/*-------------------------------------------------
    scc8530_set_reg_a
-------------------------------------------------*/

void scc8530_set_reg_b(device_t *device, int reg, UINT8 data)
{
	scc8530_t *scc = get_token(device);
	scc->channel[1].reg_val[reg] = data;
}



/*-------------------------------------------------
    scc8530_r
-------------------------------------------------*/

READ8_DEVICE_HANDLER(scc8530_r)
{
	scc8530_t *scc = get_token(device);
	UINT8 result = 0;

	offset %= 4;

	switch(offset)
	{
		case 0:
			/* Channel B (Printer Port) Control */
			if (scc->mode == 1)
				scc->mode = 0;
			else
				scc->reg = 0;

			result = scc_getbreg(device);
			break;

		case 1:
			/* Channel A (Modem Port) Control */
			if (scc->mode == 1)
				scc->mode = 0;
			else
				scc->reg = 0;

			result = scc_getareg(device);
			break;

		case 2:
			/* Channel B (Printer Port) Data */
			return scc->channel[1].rxData;
			break;

		case 3:
			/* Channel A (Modem Port) Data */
			return scc->channel[0].rxData;
			break;
	}
	return result;
}



/*-------------------------------------------------
    scc8530_w
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER(scc8530_w)
{
	scc8530_t *scc = get_token(device);
	sccChan *pChan;

	offset &= 3;

//  printf("SCC: mode %d data %x offset %d\n", scc->mode, data, offset);

	switch(offset)
	{
		case 0:
			/* Channel B (Printer Port) Control */
			if (scc->mode == 0)
			{
				if((data & 0xf0) == 0)  // not a reset command
				{
					scc->mode = 1;
					scc->reg = data & 0x0f;
//                  scc_putbreg(device, data & 0xf0);
				}
				else if (data == 0x10)
				{
					pChan = &scc->channel[1];
					// clear ext. interrupts
					pChan->extIRQPending = 0;
					pChan->baudIRQPending = 0;
					scc_updateirqs(device);
				}
			}
			else
			{
				scc->mode = 0;
				scc_putreg(device, 1, data);
			}
			break;

		case 1:
			/* Channel A (Modem Port) Control */
			if (scc->mode == 0)
			{
				if((data & 0xf0) == 0)  // not a reset command
				{
					scc->mode = 1;
					scc->reg = data & 0x0f;
//                  scc_putareg(device, data & 0xf0);
				}
				else if (data == 0x10)
				{
					pChan = &scc->channel[0];
					// clear ext. interrupts
					pChan->extIRQPending = 0;
					pChan->baudIRQPending = 0;
					scc_updateirqs(device);
				}
			}
			else
			{
				scc->mode = 0;
				scc_putreg(device, 0, data);
			}
			break;

		case 2:
			/* Channel B (Printer Port) Data */
			pChan = &scc->channel[1];

			if (pChan->txEnable)
			{
				pChan->txData = data;
				// local loopback?
				if (pChan->reg_val[14] & 0x10)
				{
					pChan->rxData = data;
					pChan->reg_val[0] |= 0x01;	// Rx character available
				}
				pChan->reg_val[1] |= 0x01;	// All sent
				pChan->reg_val[0] |= 0x04;	// Tx empty
				pChan->txUnderrun = 1;
				pChan->txIRQPending = 1;
				scc_updateirqs(device);
			}
			break;

		case 3:
			/* Channel A (Modem Port) Data */
			pChan = &scc->channel[0];

			if (pChan->txEnable)
			{
				pChan->txData = data;
				// local loopback?
				if (pChan->reg_val[14] & 0x10)
				{
					pChan->rxData = data;
					pChan->reg_val[0] |= 0x01;	// Rx character available
				}
				pChan->reg_val[1] |= 0x01;	// All sent
				pChan->reg_val[0] |= 0x04;	// Tx empty
				pChan->txUnderrun = 1;
				pChan->txIRQPending = 1;
				scc_updateirqs(device);
			}
			break;
	}
}



/*-------------------------------------------------
    DEVICE_GET_INFO( scc8530 )
-------------------------------------------------*/

DEVICE_GET_INFO( scc8530 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(scc8530_t);				break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = sizeof(scc8530_interface);		break;

		/* --- the following bits of info are returned as pointers to functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(scc8530);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(scc8530);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Zilog 8530 SCC");			break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Zilog 8530 SCC");			break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.5");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						/* Nothing */								break;
	}
}

DEFINE_LEGACY_DEVICE(SCC8530, scc8530);
