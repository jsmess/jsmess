/*
 * ncr5380.h
 *
 */

#ifndef _NCR5380_H_
#define _NCR5380_H_

#include "machine/scsidev.h"

struct NCR5380interface
{
	SCSIConfigTable *scsidevs;		/* SCSI devices */
	void (*irq_callback)(int state);	/* irq callback */
};

// 5380 registers
enum
{
	R5380_CURDATA = 0,	// current SCSI data (read only)
	R5380_OUTDATA = 0,	// output data (write only)
	R5380_INICOMMAND,	// initiator command
	R5380_MODE,		// mode
	R5380_TARGETCMD,	// target command
	R5380_SELENABLE,	// select enable (write only)
	R5380_BUSSTATUS = R5380_SELENABLE,	// bus status (read only)
	R5380_STARTDMA,		// start DMA send (write only)
	R5380_BUSANDSTAT = R5380_STARTDMA,	// bus and status (read only)
	R5380_DMATARGET,	// DMA target (write only)
	R5380_INPUTDATA = R5380_DMATARGET,	// input data (read only)
	R5380_DMAINIRECV,	// DMA initiator receive (write only)
	R5380_RESETPARITY = R5380_DMAINIRECV	// reset parity/interrupt (read only)
};

// special Mac Plus registers - they implemented it weird
#define R5380_OUTDATA_DTACK	(R5380_OUTDATA | 0x10)
#define R5380_CURDATA_DTACK	(R5380_CURDATA | 0x10)

extern void ncr5380_init( struct NCR5380interface *interface );
extern void ncr5380_read_data(int bytes, UINT8 *pData);
extern void ncr5380_write_data(int bytes, UINT8 *pData);
extern void *ncr5380_get_device(int id);
extern void ncr5380_set_device(int id, void *);
extern READ8_HANDLER(ncr5380_r);
extern WRITE8_HANDLER(ncr5380_w);

#endif
