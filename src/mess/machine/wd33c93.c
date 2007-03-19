/*
 * wd33c93.c
 *
 * WD/AMD 33c93 SCSI controller, as seen in
 * early PCs, some MSX add-ons, NEC PC-88, and SGI
 * Indigo, Indigo2, and Indy systems.
 *
 * References:
 * WD 33c93 manual
 * NetBSD 33c93 driver
 *
 */

#include "driver.h"
#include "state.h"
#include "wd33c93.h"

typedef struct
{
	void *data;		// device's "this" pointer
	pSCSIDispatch handler;	// device's handler routine	
} SCSIDev;

static SCSIDev devices[8];	// SCSI IDs 0-7
static struct WD33C93interface *intf;

// 33c93 (aka SBIC) registers
enum
{
	SBIC_myid = 0,
	SBIC_cdbsize = 0,
	SBIC_control = 1,
	SBIC_timeo = 2,
	SBIC_cdb1 = 3,
	SBIC_tsecs = 3,
	SBIC_cdb2 = 4,
	SBIC_theads = 4,
	SBIC_cdb3 = 5,
	SBIC_tcyl_hi = 5,
	SBIC_cdb4 = 6,
	SBIC_tcyl_lo = 6,
	SBIC_cdb5 = 7,
	SBIC_addr_hi = 7,
	SBIC_cdb6 = 8,
	SBIC_addr_2 = 8,
	SBIC_cdb7 = 9,
	SBIC_addr_3 = 9,
	SBIC_cdb8 = 10,		
	SBIC_addr_lo = 10,
	SBIC_cdb9 = 11,		
	SBIC_secno = 11,
	SBIC_cdb10 = 12,	
	SBIC_headno = 12,
	SBIC_cdb11 = 13,	
	SBIC_cylno_hi = 13,
	SBIC_cdb12 = 14,	
	SBIC_cylno_lo = 14,
	SBIC_tlun = 15,		
	SBIC_cmd_phase = 16,	
	SBIC_syn = 17,		
	SBIC_count_hi = 18,	
	SBIC_count_med = 19,	
	SBIC_count_lo = 20,	
	SBIC_selid = 21,	// dest ID (33c93 is initiator)
	SBIC_rselid = 22,	// source ID (33c93 is target)
	SBIC_csr = 23,		// CSI status
	SBIC_cmd = 24,		
	SBIC_data = 25,		
	SBIC_queue_tag = 26,	
	SBIC_aux_status = 27,

	SBIC_num_registers
};

#define SBIC_MyID_Advanced	(0x08)	// in myid register, enable "Advanced Features"

#define SBIC_CSR_Reset		(0x00)	// reset
#define SBIC_CSR_Reset_Adv	(0x01)	// reset w/adv. features
#define SBIC_CSR_Selected	(0x11)	// selection complete, in initiator mode
#define SBIC_CSR_XferComplete	(0x16)	// initiator mode completed
#define SBIC_CSR_MIS_2		(0x88)

#define SBIC_AuxStat_DBR	(0x01)	// data buffer ready
#define SBIC_AuxStat_CIP	(0x10)  // command in progress, chip is busy
#define SBIC_AuxStat_BSY	(0x20)	// Level 2 command in progress
#define SBIC_AuxStat_IRQ	(0x80)	// IRQ pending

static UINT8 n33C93_RegisterSelect;
static UINT8 n33C93_Data;
static UINT8 n33C93_Registers[SBIC_num_registers];
static UINT8 last_id;

static void wd33c93_irq( int cmdtype )
{
	// set IRQ flag
	n33C93_Registers[ SBIC_aux_status ] = SBIC_AuxStat_DBR | SBIC_AuxStat_IRQ;
	// clear busy flags
	n33C93_Registers[ SBIC_aux_status ] &= ~(SBIC_AuxStat_CIP | SBIC_AuxStat_BSY);

	// do any additional stuff required by the command
	// (the driver may have done platform-specific DMA during the callback, we handle that here)
	switch (cmdtype)
	{
		case 1:	// select w/ATN and transfer
			n33C93_Registers[ SBIC_csr ] = SBIC_CSR_XferComplete;
			n33C93_Registers[ SBIC_cmd_phase ] = 0x60; // command successfully completed
			break;

		case 2: // select w/ATN, no transfer
			n33C93_Registers[ SBIC_csr ] = SBIC_CSR_Selected;
			n33C93_Registers[ SBIC_cmd_phase ] = 0x10; // target selected
			break;

		default:
			break;
	}

	// call the driver to handle this
	if (intf->irq_callback)
	{
		intf->irq_callback(1);
	}
}

static void wd33c93_immediate_irq(void)
{
	n33C93_Registers[ SBIC_aux_status ] = SBIC_AuxStat_DBR | SBIC_AuxStat_IRQ;
	n33C93_Registers[ SBIC_aux_status ] &= ~(SBIC_AuxStat_CIP | SBIC_AuxStat_BSY);
	if (intf && intf->irq_callback)
	{
		intf->irq_callback(1);
	}
}

WRITE8_HANDLER(wd33c93_w)
{
	switch( offset )
	{
	case 0:
		n33C93_RegisterSelect = data;
		break;
	case 1:
		n33C93_Data = data;
//		logerror("%x to WD register %x (PC=%x)\n", data, n33C93_RegisterSelect, activecpu_get_pc());
		if( n33C93_RegisterSelect <= 0x16 )
		{
			n33C93_Registers[ n33C93_RegisterSelect ] = data;
		}
		if( n33C93_RegisterSelect == SBIC_cmd )
		{
			n33C93_Registers[ SBIC_aux_status ] |= SBIC_AuxStat_CIP;

			switch( n33C93_Data )
			{
			case 0x00: /* Reset controller */
				// this clears all the registers
				memset(n33C93_Registers, 0, sizeof(n33C93_Registers));

				if( n33C93_Registers[ SBIC_myid ] & SBIC_MyID_Advanced )
				{
					n33C93_Registers[ SBIC_csr ] = SBIC_CSR_Reset_Adv;
				}
				else
				{
					n33C93_Registers[ SBIC_csr ] = SBIC_CSR_Reset;
				}

				wd33c93_immediate_irq();

				logerror("WD: Reset controller\n");
				break;
			case 0x01: /* Abort */
				logerror("WD: Abort\n");
				wd33c93_immediate_irq();
				break;
			case 0x04: /* Disconnect */
				logerror("WD: Disconnect\n");
				n33C93_Registers[ SBIC_aux_status ] &= ~(SBIC_AuxStat_CIP | SBIC_AuxStat_BSY);
				break;
			case 0x06: /* Select with ATN */
				logerror("WD: Select with ATN: ID %d (SCSI cmd %02x)\n", n33C93_Registers[ SBIC_selid ]&7, n33C93_Registers[3]);

				last_id = n33C93_Registers[ SBIC_selid ] & 7;

				// we don't support this at this time
				n33C93_Registers[ SBIC_csr ] = 0x42;	// SEL_TimeOut
				n33C93_Registers[ SBIC_cmd_phase ] = 0x00; // no device selected

				wd33c93_clear_dma();	// no DMA

				wd33c93_immediate_irq();
				break;
			case 0x08: /* Select with ATN and transfer */
				logerror("WD: Select with ATN and transfer: ID %d (SCSI cmd %02x)\n", n33C93_Registers[ SBIC_selid ]&7, n33C93_Registers[3]);

				last_id = n33C93_Registers[ SBIC_selid ] & 7;

				if (devices[last_id].handler)
				{
					int xfercount;

					xfercount = devices[last_id].handler(SCSIOP_EXEC_COMMAND, devices[last_id].data, 0, &n33C93_Registers[3]);

					n33C93_Registers[ SBIC_count_lo ] = xfercount & 0xff;
					n33C93_Registers[ SBIC_count_med ] = (xfercount>>8) & 0xff;
					n33C93_Registers[ SBIC_count_hi ] = (xfercount>>16) & 0xff;

					n33C93_Registers[ SBIC_rselid ] |= n33C93_Registers[ SBIC_selid ] & 7;

					timer_set( TIME_IN_HZ( 16384 ), 1, wd33c93_irq );
				}
				else
				{
					n33C93_Registers[ SBIC_csr ] = 0x42;	// SEL_TimeOut
					n33C93_Registers[ SBIC_cmd_phase ] = 0x00; // no device selected

					wd33c93_clear_dma();	// no DMA

					wd33c93_immediate_irq();
				}
				break;
			default:
				logerror( "Unknown SCSI controller command: %02x (PC=%x)\n", n33C93_Data, activecpu_get_pc() );
				break;
			}
		}
		else if ( n33C93_RegisterSelect == SBIC_data)
		{
//			logerror("Write %02x to SBIC data (PC=%x)\n", n33C93_Data, activecpu_get_pc());
		}

		n33C93_RegisterSelect++;
		break;
	default:
//		logerror("ERROR: unk 33c93 W offset %d\n", offset);
		break;
	}
}

READ8_HANDLER(wd33c93_r)
{
	UINT8 rv = 0;

	switch( offset )
	{
	case 0:
//		if (activecpu_get_pc() != 0xbfc167d8)
//			logerror("Read WD aux status = %x (PC=%x)\n", n33C93_Registers[ SBIC_aux_status ], activecpu_get_pc());
		rv = n33C93_Registers[ SBIC_aux_status ];
		break;
	case 1:
//		logerror("Read WD register %x = %x (PC=%x)\n", n33C93_RegisterSelect, n33C93_Registers[ n33C93_RegisterSelect ], activecpu_get_pc());

		// ack IRQ on status read
		if (n33C93_RegisterSelect == SBIC_csr)
		{
			n33C93_Registers[ SBIC_aux_status ] &= ~SBIC_AuxStat_IRQ;
			if (intf && intf->irq_callback)
			{
				intf->irq_callback(1);
			}
		}

		rv = n33C93_Registers[ n33C93_RegisterSelect ];
		n33C93_RegisterSelect++;
		break;
	default:
//		logerror("ERROR: unk 33c93 R offset %d\n", offset);
		break;
	}

	return rv;
}

extern void wd33c93_init( struct WD33C93interface *interface )
{
	int i;

	// save interface pointer for later
	intf = interface;

	memset(n33C93_Registers, 0, sizeof(n33C93_Registers));
	memset(devices, 0, sizeof(devices));
	n33C93_RegisterSelect = 0;

	// try to open the devices
	for (i = 0; i < interface->scsidevs->devs_present; i++)
	{
		devices[interface->scsidevs->devices[i].scsiID].handler = interface->scsidevs->devices[i].handler;
		interface->scsidevs->devices[i].handler(SCSIOP_ALLOC_INSTANCE, &devices[interface->scsidevs->devices[i].scsiID].data, interface->scsidevs->devices[i].diskID, (UINT8 *)NULL);
	}	

	state_save_register_item_array("wd33c93", 0, n33C93_Registers);
	state_save_register_item("wd33c93", 0, n33C93_RegisterSelect);
}

void wd33c93_read_data(int bytes, UINT8 *pData)
{
	if (devices[last_id].handler)
	{
		devices[last_id].handler(SCSIOP_READ_DATA, devices[last_id].data, bytes, pData);
	}
	else
	{
		logerror("wd33c93: request for unknown device SCSI ID %d\n", last_id);
	}
}

void wd33c93_write_data(int bytes, UINT8 *pData)
{
	if (devices[last_id].handler)
	{
		devices[last_id].handler(SCSIOP_WRITE_DATA, devices[last_id].data, bytes, pData);
	}
	else
	{
		logerror("wd33c93: request for unknown device SCSI ID %d\n", last_id);
	}
}

void *wd33c93_get_device(int id)
{
	void *ret;

	if (devices[id].handler)
	{
		devices[id].handler(SCSIOP_GET_DEVICE, devices[id].data, 0, (UINT8 *)&ret);

		return ret;
	}

	return NULL;
}

void wd33c93_clear_dma(void)
{
	// indicate DMA completed by clearing the transfer count
	n33C93_Registers[ SBIC_count_lo ] = 0;
	n33C93_Registers[ SBIC_count_med ] = 0;
	n33C93_Registers[ SBIC_count_hi ] = 0;
}

int wd33c93_get_dma_count(void)
{
	return n33C93_Registers[ SBIC_count_lo ] | n33C93_Registers[ SBIC_count_med ]<<8 | n33C93_Registers[ SBIC_count_hi ]<<16;
}
