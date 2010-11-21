/*
    SCSIBus.h

    Implementation of a raw SCSI/SASI bus for machines that don't use a SCSI
    controler chip such as the RM Nimbus, which implements it as a bunch of
    74LS series chips.

*/

#ifndef _SCSIBUS_H_
#define _SCSIBUS_H_

#include "emu.h"
#include "machine/scsidev.h"


/***************************************************************************
    MACROS
***************************************************************************/

DECLARE_LEGACY_DEVICE(SCSIBUS, scsibus);

#define MDRV_SCSIBUS_ADD(_tag, _intrf) \
	MDRV_DEVICE_ADD(_tag, SCSIBUS, 0) \
	MDRV_DEVICE_CONFIG(_intrf)


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define SCSI_LINE_SEL   0
#define SCSI_LINE_BSY   1
#define SCSI_LINE_REQ   2
#define SCSI_LINE_ACK   3
#define SCSI_LINE_CD    4
#define SCSI_LINE_IO    5
#define SCSI_LINE_MSG   6
#define SCSI_LINE_RESET 7

// Perhaps thse should be in scsi.h ?
#define SCSI_PHASE_BUS_FREE 8
#define SCSI_PHASE_SELECT   9

#define REQ_DELAY_NS    90
#define ACK_DELAY_NS    90

#define CMD_BUF_SIZE    32
#define DATA_BUF_SIZE   512

#define SCSI_CMD_FORMAT_UNIT    0x04
#define SCSI_COMMAND_INQUIRY    0x12
#define SCSI_CMD_MODE_SELECT    0x15
#define SCSI_CMD_READ_CAPACITY  0x25
#define SCSI_CMD_READ_DEFECT    0x37
#define SCSI_CMD_BUFFER_WRITE   0x3B
#define SCSI_CMD_BUFFER_READ    0x3C

#define RW_BUFFER_HEAD_BYTES    0x04

#define IS_COMMAND(cmd)         (bus->command[0]==cmd)
#define IS_READ_COMMAND()       ((bus->command[0]==0x08) || (bus->command[0]==0x28) || (bus->command[0]==0xa8))
#define IS_WRITE_COMMAND()      ((bus->command[0]==0x0a) || (bus->command[0]==0x2a))


/***************************************************************************
    INTERFACE
***************************************************************************/

typedef struct _SCSIBus_interface SCSIBus_interface;
struct _SCSIBus_interface
{
    const SCSIConfigTable *scsidevs;		/* SCSI devices */
    void (*line_change_cb)(running_device *device, UINT8 line, UINT8 state);
};

/* SCSI Bus read/write */

UINT8 scsi_data_r(running_device *device);
void scsi_data_w(running_device *device, UINT8 data);

/* Get/Set lines */

UINT8 get_scsi_lines(running_device *device);
UINT8 get_scsi_line(running_device *device, UINT8 lineno);
void set_scsi_line(running_device *device, UINT8 line, UINT8 state);

/* Get current bus phase */

UINT8 get_scsi_phase(running_device *device);

/* utility functions */

/* get a drive's number from it's select line */
UINT8 scsibus_driveno(UINT8  drivesel);

/* get the number of bytes for a scsi command */
int get_scsi_cmd_len(int cbyte);

/* Initialisation at machine reset time */
void init_scsibus(running_device *device);

#endif
