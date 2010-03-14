/*
    SCSIBus.c

    Implementation of a raw SCSI/SASI bus for machines that don't use a SCSI
    controler chip such as the RM Nimbus, which implements it as a bunch of
    74LS series chips.

*/

#include "emu.h"
#include "machine/scsi.h"
#include "machine/scsibus.h"

/* LOGLEVEL 0=no logging, 1=just commands and data, 2=everything ! */

#define LOGLEVEL			0

#define LOG(level,...)      if(LOGLEVEL>=level) logerror(__VA_ARGS__)

typedef struct _scsibus_t scsibus_t;
struct _scsibus_t
{
	SCSIInstance            *devices[8];
	const SCSIBus_interface *interface;

    UINT8       linestate;
	UINT8       last_id;
	UINT8       phase;

    UINT8       command[CMD_BUF_SIZE];
    UINT8       cmd_idx;
    UINT8       is_linked;

	UINT8       data[DATA_BUF_SIZE+RW_BUFFER_HEAD_BYTES];
    UINT16      data_idx;
    int         xfer_count;
    int         bytes_left;
    int         data_last;

    emu_timer   *req_timer;
    emu_timer   *ack_timer;

    UINT8       initialised;
};

static void set_scsi_line_now(running_device *device, UINT8 line, UINT8 state);
static void set_scsi_line_ack(running_device *device, UINT8 state);
static void scsi_in_line_changed(running_device *device, UINT8 line, UINT8 state);
static void scsi_out_line_change(running_device *device, UINT8 line, UINT8 state);
static void scsi_out_line_change_now(running_device *device, UINT8 line, UINT8 state);
static void scsi_out_line_req(running_device *device, UINT8 state);
static TIMER_CALLBACK(req_timer_callback);
static TIMER_CALLBACK(ack_timer_callback);
static void scsi_change_phase(running_device *device, UINT8 newphase);

static const char *const linenames[] =
{
    "select", "busy", "request", "acknoledge", "C/D", "I/O", "message", "reset"
};

static const char *const phasenames[] =
{
    "data out", "data in", "command", "status", "none", "none", "message out", "message in", "bus free","select"
};

INLINE scsibus_t *get_token(running_device *device)
{
	assert(device->type == SCSIBUS);
	return (scsibus_t *) device->token;
}

static void scsibus_read_data(scsibus_t   *bus)
{
    bus->data_last = (bus->bytes_left >= DATA_BUF_SIZE) ? DATA_BUF_SIZE : bus->bytes_left;

    LOG(1,"SCSIBUS:scsibus_read_data bus->bytes_left=%04X, bus->data_last=%04X, bus->xfer_count=%04X\n",bus->bytes_left,bus->data_last,bus->xfer_count);

    if (bus->data_last > 0)
    {
        SCSIReadData(bus->devices[bus->last_id], bus->data, bus->data_last);
        bus->bytes_left-=bus->data_last;
        bus->data_idx=0;
    }
}

static void scsibus_write_data(scsibus_t   *bus)
{
    if(bus->bytes_left >= DATA_BUF_SIZE)
    {
        SCSIWriteData(bus->devices[bus->last_id], bus->data, DATA_BUF_SIZE);

        bus->bytes_left-=DATA_BUF_SIZE;
        bus->data_idx=0;
    }
}

/* SCSI Bus read/write */

UINT8 scsi_data_r(running_device *device)
{
    scsibus_t   *bus = get_token(device);
    UINT8       result = 0;

    switch (bus->phase)
    {
        case SCSI_PHASE_DATAIN :
            result=bus->data[bus->data_idx++];

            // check to see if we have reached the end of the block buffer
            // and that there is more data to read from the scsi disk
            if((bus->data_idx==DATA_BUF_SIZE) && (bus->bytes_left>0) && IS_READ_COMMAND())
            {
                scsibus_read_data(bus);
            }
            break;

        case SCSI_PHASE_STATUS :
            result=0;              // no errors for the time being !
            bus->cmd_idx++;
            break;

        case SCSI_PHASE_MESSAGE_IN :
            result=0;              // no errors for the time being !
            bus->cmd_idx++;
            break;
    }

    LOG(2,"scsi_data_r : phase=%s, data_idx=%d, cmd_idx=%d\n",phasenames[bus->phase],bus->data_idx,bus->cmd_idx);
    return result;
}

void scsi_data_w(running_device *device, UINT8 data)
{
    scsibus_t   *bus = get_token(device);

    switch (bus->phase)
    {
        // Note this assumes we only have one initiator and therefore
        // only one line active.
        case SCSI_PHASE_BUS_FREE :
            bus->last_id=driveno(data);
            break;

        case SCSI_PHASE_COMMAND :
            bus->command[bus->cmd_idx++]=data;
            break;

        case SCSI_PHASE_DATAOUT :

            //LOG(1,"SCSIBUS:xfer_count=%02X, bytes_left=%02X data_idx=%02X\n",bus->xfer_count,bus->bytes_left,bus->data_idx);

            // Write (blocks) and write buffer, just buffer it
            if(IS_WRITE_COMMAND() || IS_COMMAND(SCSI_CMD_BUFFER_WRITE) || IS_COMMAND(SCSI_CMD_MODE_SELECT))
                bus->data[bus->data_idx++]=data;

            if(IS_COMMAND(SCSI_CMD_FORMAT_UNIT))
            {
                // Only store the first 4 bytes of the bad block list (the header)
                if(bus->data_idx<4)
                    bus->data[bus->data_idx++]=data;
                else
                    bus->data_idx++;

                // When we have the first 3 bytes, calculate how many more are in the
                // bad block list.
                if(bus->data_idx==3)
                {
                    bus->xfer_count+=((bus->data[2]<<8)+bus->data[3]);
                    bus->data_last=bus->xfer_count;
                }
            }

            // If the data buffer is full, and we are writing blocks flush it to the SCSI disk
            if((bus->data_idx == DATA_BUF_SIZE) && IS_WRITE_COMMAND())
                scsibus_write_data(bus);
            break;
    }
}

/* Get/Set lines */

UINT8 get_scsi_lines(running_device *device)
{
    scsibus_t   *bus = get_token(device);

    return bus->linestate;
}

UINT8 get_scsi_line(running_device *device, UINT8 lineno)
{
    scsibus_t   *bus = get_token(device);
    UINT8       result=0;

    switch (lineno)
    {
        case SCSI_LINE_SEL      : result=(bus->linestate & (1<<SCSI_LINE_SEL)) >> SCSI_LINE_SEL;
        case SCSI_LINE_BSY      : result=(bus->linestate & (1<<SCSI_LINE_BSY)) >> SCSI_LINE_BSY;
        case SCSI_LINE_REQ      : result=(bus->linestate & (1<<SCSI_LINE_REQ)) >> SCSI_LINE_REQ;
        case SCSI_LINE_ACK      : result=(bus->linestate & (1<<SCSI_LINE_ACK)) >> SCSI_LINE_ACK;
        case SCSI_LINE_CD       : result=(bus->linestate & (1<<SCSI_LINE_CD )) >> SCSI_LINE_CD;
        case SCSI_LINE_IO       : result=(bus->linestate & (1<<SCSI_LINE_IO )) >> SCSI_LINE_IO;
        case SCSI_LINE_MSG      : result=(bus->linestate & (1<<SCSI_LINE_MSG)) >> SCSI_LINE_MSG;
        case SCSI_LINE_RESET    : result=(bus->linestate & (1<<SCSI_LINE_RESET)) >> SCSI_LINE_RESET;
    }

    LOG(2,"get_scsi_line(%s)=%d\n",linenames[lineno],result);

    return result;
}

void set_scsi_line(running_device *device, UINT8 line, UINT8 state)
{
    scsibus_t   *bus = get_token(device);
    UINT8       changed;

    changed = ((bus->linestate & (1<<line)) != (state << line));

    LOG(2,"set_scsi_line(%s,%d), changed=%d, linestate=%02X\n",linenames[line],state,changed,bus->linestate);

    if(changed)
    {
        if (line==SCSI_LINE_ACK)
            set_scsi_line_ack(device,state);
        else
            set_scsi_line_now(device,line,state);
    }
}

static void set_scsi_line_now(running_device *device, UINT8 line, UINT8 state)
{
    scsibus_t   *bus = get_token(device);

    if(state)
        bus->linestate |= (1<<line);
    else
        bus->linestate &= ~(1<<line);

    scsi_in_line_changed(device,line,state);
}

void set_scsi_line_ack(running_device *device, UINT8 state)
{
    scsibus_t   *bus = get_token(device);

    timer_adjust_oneshot(bus->ack_timer,ATTOTIME_IN_NSEC(ACK_DELAY_NS),state);
}

void dump_command_bytes(scsibus_t   *bus)
{
    int byteno;

    logerror("sending command 0x%02X to ScsiID %d\n",bus->command[0],bus->last_id);

    for(byteno=0; byteno<bus->cmd_idx; byteno++)
    {
        logerror("%02X ",bus->command[byteno]);
    }
    logerror("\n\n");
}

static void scsibus_exec_command(running_device *device)
{
    scsibus_t   *bus = get_token(device);
    int         command_local = 0;
    int         newphase;

    if(LOGLEVEL)
        dump_command_bytes(bus);

    bus->is_linked=bus->command[bus->cmd_idx-1] & 0x01;

    // Check for locally executed commands, and if found execute them
    switch (bus->command[0])
    {
        // Format unit
        case SCSI_CMD_FORMAT_UNIT :
            LOG(1,"SCSIBUS: format unit bus->command[1]=%02X & 0x10\n",(bus->command[1] & 0x10));
            command_local=1;
            if((bus->command[1] & 0x10)==0x10)
                SCSISetPhase(bus->devices[bus->last_id],SCSI_PHASE_DATAOUT);
            else
                SCSISetPhase(bus->devices[bus->last_id],SCSI_PHASE_STATUS);

            bus->xfer_count=4;
            bus->data_last=bus->xfer_count;
            bus->bytes_left=0;
            break;

        case SCSI_CMD_READ_DEFECT :
            LOG(1,"SCSIBUS: read defect list\n");
            command_local=1;

            bus->data[0] = 0x00;
            bus->data[1] = bus->command[2];
            bus->data[3] = 0x00;         // defect list len msb
            bus->data[4] = 0x00;         // defect list len lsb

            bus->xfer_count=4;
            bus->data_last=bus->xfer_count;
            bus->bytes_left=0;
            SCSISetPhase(bus->devices[bus->last_id], SCSI_PHASE_DATAIN );
            break;

        // write buffer
        case SCSI_CMD_BUFFER_WRITE :
            LOG(1,"SCSIBUS: write_buffer\n");
            command_local=1;
            bus->xfer_count=(bus->command[7]<<8)+bus->command[8];
            bus->data_last=bus->xfer_count;
            bus->bytes_left=0;
            SCSISetPhase(bus->devices[bus->last_id],SCSI_PHASE_DATAOUT);
            break;

        // read buffer
        case SCSI_CMD_BUFFER_READ   :
            LOG(1,"SCSIBUS: read_buffer\n");
            command_local=1;
            bus->xfer_count=(bus->command[7]<<8)+bus->command[8];
            bus->data_last=bus->xfer_count;
            bus->bytes_left=0;
            SCSISetPhase(bus->devices[bus->last_id],SCSI_PHASE_DATAIN);
            break;
    }


    // Check for locally executed command, if not then pass it on
    // to the disk driver
    if(!command_local)
    {
        SCSISetCommand(bus->devices[bus->last_id], bus->command, bus->cmd_idx);
        SCSIExecCommand(bus->devices[bus->last_id], &bus->xfer_count);
        bus->bytes_left=bus->xfer_count;
        bus->data_last=bus->xfer_count;
        bus->data_idx=0;
    }

    SCSIGetPhase(bus->devices[bus->last_id], &newphase);

    scsi_change_phase(device,newphase);

    LOG(1,"SCSIBUS:xfer_count=%02X, bytes_left=%02X data_idx=%02X\n",bus->xfer_count,bus->bytes_left,bus->data_idx);

    // This is correct as we need to read from disk for commands other than just read data
    if ((bus->phase == SCSI_PHASE_DATAIN) && (!command_local))
        scsibus_read_data(bus);
}

static int datain_done(scsibus_t   *bus)
{
    int result=0;

    // Read data commands
    if(IS_READ_COMMAND() && (bus->data_idx == bus->data_last) && (bus->bytes_left == 0))
        result=1;
    else if (bus->data_idx==bus->data_last)
        result=1;

    return result;
}

static int dataout_done(scsibus_t   *bus)
{
    int result=0;

    // Write data commands
    if(IS_WRITE_COMMAND() && (bus->data_idx == 0) && (bus->bytes_left == 0))
        result=1;
    else if (bus->data_idx==bus->data_last)
        result=1;

    return result;
}

static void scsi_in_line_changed(running_device *device, UINT8 line, UINT8 state)
{
    scsibus_t   *bus = get_token(device);

    // Reset aborts and returns to bus free
    if((line==SCSI_LINE_RESET) && (state==0))
    {
        scsi_change_phase(device,SCSI_PHASE_BUS_FREE);
        bus->cmd_idx=0;
        bus->data_idx=0;
        bus->is_linked=0;

        return;
    }

    switch (bus->phase)
    {
        case SCSI_PHASE_BUS_FREE :
            if((line==SCSI_LINE_SEL) && (bus->devices[bus->last_id]!=NULL))
            {
                if(state==0)
                    scsi_out_line_change(device,SCSI_LINE_BSY,0);
                else
                    scsi_change_phase(device,SCSI_PHASE_COMMAND);
            }
            break;

        case SCSI_PHASE_COMMAND :
            if(line==SCSI_LINE_ACK)
            {
                if(state)
                {
                    // If the command is ready go and execute it
                    if(bus->cmd_idx==get_scsi_cmd_len(bus->command[0]))
                    {
                        scsibus_exec_command(device);
                    }
                    else
                        scsi_out_line_change(device,SCSI_LINE_REQ,0);
                }
                else
                    scsi_out_line_change(device,SCSI_LINE_REQ,1);
            }
            break;

        case SCSI_PHASE_DATAIN :
            if(line==SCSI_LINE_ACK)
            {
                if(state)
                {
                    if(datain_done(bus))
                        scsi_change_phase(device,SCSI_PHASE_STATUS);
                    else
                        scsi_out_line_change(device,SCSI_LINE_REQ,0);
                }
                else
                    scsi_out_line_change(device,SCSI_LINE_REQ,1);
            }
            break;

        case SCSI_PHASE_DATAOUT :
            if(line==SCSI_LINE_ACK)
            {
                if(state)
                {
                    if(dataout_done(bus))
                        scsi_change_phase(device,SCSI_PHASE_STATUS);
                    else
                        scsi_out_line_change(device,SCSI_LINE_REQ,0);
                }
                else
                    scsi_out_line_change(device,SCSI_LINE_REQ,1);
            }
            break;

        case SCSI_PHASE_STATUS :
            if(line==SCSI_LINE_ACK)
            {
                if(state)
                {
                    if(bus->cmd_idx > 0)
                    {
                        scsi_change_phase(device,SCSI_PHASE_MESSAGE_IN);
                    }
                    else
                        scsi_out_line_change(device,SCSI_LINE_REQ,0);
                }
                else
                    scsi_out_line_change(device,SCSI_LINE_REQ,1);
            }
            break;

        case SCSI_PHASE_MESSAGE_IN :
            if(line==SCSI_LINE_ACK)
            {
                if(state)
                {
                    if(bus->cmd_idx > 0)
                    {
                        if(bus->is_linked)
                            scsi_change_phase(device,SCSI_PHASE_COMMAND);
                        else
                            scsi_change_phase(device,SCSI_PHASE_BUS_FREE);
                    }
                    else
                        scsi_out_line_change(device,SCSI_LINE_REQ,0);
                }
                else
                    scsi_out_line_change(device,SCSI_LINE_REQ,1);
            }
            break;
    }
}

static void scsi_out_line_change(running_device *device, UINT8 line, UINT8 state)
{
    if(line==SCSI_LINE_REQ)
        scsi_out_line_req(device,state);
    else
        scsi_out_line_change_now(device,line,state);
}

static void scsi_out_line_change_now(running_device *device, UINT8 line, UINT8 state)
{
    scsibus_t   *bus = get_token(device);

    if(state)
        bus->linestate |= (1<<line);
    else
        bus->linestate &= ~(1<<line);

    LOG(2,"scsi_out_line_change(%s,%d)\n",linenames[line],state);

    if(bus->interface->line_change_cb!=NULL)
        bus->interface->line_change_cb(device,line,state);
}

static void scsi_out_line_req(running_device *device, UINT8 state)
{
    scsibus_t   *bus = get_token(device);

    timer_adjust_oneshot(bus->req_timer,ATTOTIME_IN_NSEC(REQ_DELAY_NS),state);
}


static TIMER_CALLBACK(req_timer_callback)
{
    scsi_out_line_change_now((running_device *)ptr, SCSI_LINE_REQ, param);
}

static TIMER_CALLBACK(ack_timer_callback)
{
    set_scsi_line_now((running_device *)ptr, SCSI_LINE_ACK, param);
}


UINT8 get_scsi_phase(running_device *device)
{
    scsibus_t   *bus = get_token(device);

    return bus->phase;
}


static void scsi_change_phase(running_device *device, UINT8 newphase)
{
    scsibus_t   *bus = get_token(device);

    LOG(1,"scsi_change_phase() from=%s, to=%s\n",phasenames[bus->phase],phasenames[newphase]);

    bus->phase=newphase;
    bus->cmd_idx=0;
    bus->data_idx=0;

    switch(bus->phase)
    {
        case SCSI_PHASE_BUS_FREE :
            scsi_out_line_change(device,SCSI_LINE_CD,1);
            scsi_out_line_change(device,SCSI_LINE_IO,1);
            scsi_out_line_change(device,SCSI_LINE_MSG,1);
            scsi_out_line_change(device,SCSI_LINE_REQ,1);
            scsi_out_line_change(device,SCSI_LINE_BSY,1);
            break;

        case SCSI_PHASE_COMMAND :
            scsi_out_line_change(device,SCSI_LINE_CD,0);
            scsi_out_line_change(device,SCSI_LINE_IO,1);
            scsi_out_line_change(device,SCSI_LINE_MSG,1);
            scsi_out_line_change(device,SCSI_LINE_REQ,0);
            break;

        case SCSI_PHASE_DATAOUT :
            scsi_out_line_change(device,SCSI_LINE_CD,1);
            scsi_out_line_change(device,SCSI_LINE_IO,1);
            scsi_out_line_change(device,SCSI_LINE_MSG,1);
            scsi_out_line_change(device,SCSI_LINE_REQ,0);
            break;

        case SCSI_PHASE_DATAIN :
            scsi_out_line_change(device,SCSI_LINE_CD,1);
            scsi_out_line_change(device,SCSI_LINE_IO,0);
            scsi_out_line_change(device,SCSI_LINE_MSG,1);
            scsi_out_line_change(device,SCSI_LINE_REQ,0);
            break;

        case SCSI_PHASE_STATUS :
            scsi_out_line_change(device,SCSI_LINE_CD,0);
            scsi_out_line_change(device,SCSI_LINE_IO,0);
            scsi_out_line_change(device,SCSI_LINE_MSG,1);
            scsi_out_line_change(device,SCSI_LINE_REQ,0);
            break;

        case SCSI_PHASE_MESSAGE_OUT :
            scsi_out_line_change(device,SCSI_LINE_CD,0);
            scsi_out_line_change(device,SCSI_LINE_IO,1);
            scsi_out_line_change(device,SCSI_LINE_MSG,0);
            scsi_out_line_change(device,SCSI_LINE_REQ,0);
            break;

        case SCSI_PHASE_MESSAGE_IN :
            scsi_out_line_change(device,SCSI_LINE_CD,0);
            scsi_out_line_change(device,SCSI_LINE_IO,0);
            scsi_out_line_change(device,SCSI_LINE_MSG,0);
            scsi_out_line_change(device,SCSI_LINE_REQ,0);
            break;
    }
}

UINT8 driveno(UINT8  drivesel)
{
    switch (drivesel)
    {
        case 0x01   : return 0;
        case 0x02   : return 1;
        case 0x04   : return 2;
        case 0x08   : return 3;
        case 0x10   : return 4;
        case 0x20   : return 5;
        case 0x40   : return 6;
        case 0x80   : return 7;
        default : return 0;
    }
}

// get the length of a SCSI command based on it's command byte type
int get_scsi_cmd_len(int cbyte)
{
	int group;

	group = (cbyte>>5) & 7;

	if (group == 0) return 6;
	if (group == 1 || group == 2) return 10;
	if (group == 5) return 12;

	fatalerror("scsibus: Unknown SCSI command group %d\n", group);

	return 6;
}

void init_scsibus(running_device *device)
{
    scsibus_t               *bus = get_token(device);
    const SCSIConfigTable   *scsidevs = bus->interface->scsidevs;
	int                     devno;

    if(!bus->initialised)
    {
        // try to open the devices
        for (devno = 0; devno < scsidevs->devs_present; devno++)
        {
            LOG(1,"init_scsibus devno=%d \n",devno);
            SCSIAllocInstance( device->machine, scsidevs->devices[devno].scsiClass, &bus->devices[scsidevs->devices[devno].scsiID], scsidevs->devices[devno].diskregion );
        }

        bus->initialised=1;
    }
 }

static DEVICE_START( scsibus )
{
    scsibus_t               *bus = get_token(device);

	assert(device->baseconfig().static_config != NULL);
    bus->interface = (const SCSIBus_interface*)device->baseconfig().static_config;

	memset(bus->devices, 0, sizeof(bus->devices));

    // Flag not initialised
    bus->initialised=0;

    // All lines start high - inactive
    bus->linestate=0xFF;

    // Start with bus free
    bus->phase=SCSI_PHASE_BUS_FREE;

    // Setup req/ack timers
    bus->req_timer=timer_alloc(device->machine, req_timer_callback, (void *)device);
    bus->ack_timer=timer_alloc(device->machine, ack_timer_callback, (void *)device);
}

static DEVICE_STOP( scsibus )
{
    scsibus_t               *bus = get_token(device);
    int                     devno;

	for (devno = 0; devno < bus->interface->scsidevs->devs_present; devno++)
	{
        SCSIDeleteInstance( bus->devices[devno] );
    }
}

static DEVICE_RESET( scsibus )
{
}

DEVICE_GET_INFO( scsibus )
{
	switch ( state )
    {
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:			info->i = sizeof(scsibus_t);				break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:	info->i = 0;				    		break;
		case DEVINFO_INT_CLASS:				    info->i = DEVICE_CLASS_PERIPHERAL;			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:				    info->start = DEVICE_START_NAME(scsibus);	break;
		case DEVINFO_FCT_STOP:				    info->stop  = DEVICE_STOP_NAME(scsibus);	break;
		case DEVINFO_FCT_RESET:				    info->reset = DEVICE_RESET_NAME(scsibus);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:				    strcpy(info->s, "SCSI bus");				break;
		case DEVINFO_STR_FAMILY:			    strcpy(info->s, "SCSI");				    break;
		case DEVINFO_STR_VERSION:			    strcpy(info->s, "1.0");				    break;
		case DEVINFO_STR_SOURCE_FILE:			strcpy(info->s, __FILE__);				    break;
		case DEVINFO_STR_CREDITS:			    strcpy(info->s, "Copyright the MAME and MESS Teams"); break;
	}
}
