/*
 * ncr5380.c
 *
 * NCR 5380 SCSI controller, as seen in many 680x0 Macs,
 * official Apple add-on cards for the Apple II series,
 * and probably some PC cards as well.
 *
 * Preliminary emulation by R. Belmont.
 *
 * References:
 * Zilog 5380 manual
 * "Inside Macintosh: Devices" (online at http://www.manolium.org/dev/techsupport/insidemac/Devices/Devices-2.html )
 *
 * NOTES:
 * This implementation is tied closely to the drivers found in the Mac Plus ROM and the routines in Mac
 * System 6.0.3 that it patches out the ROM traps with.  While attempts have been made to
 * have the behavior work according to the manual and not the specific Apple driver code,
 * there are almost certainly areas where that is true.  In particular, IRQs are not implemented
 * as they are unused on the Mac Plus.
 *
 */

#include "emu.h"
#include "ncr5380.h"

#define VERBOSE	(0)

static const char *const rnames[] =
{
	"Current data",
	"Initiator cmd",
	"Mode",
	"Target cmd",
	"Bus status",
	"Bus and status",
	"Input data",
	"Reset parity"
};

static const char *const wnames[] =
{
	"Output data",
	"Initiator cmd",
	"Mode",
	"Target cmd",
	"Select enable",
	"Start DMA",
	"DMA target",
	"DMA initiator rec"
};

static SCSIInstance *devices[8];	// SCSI IDs 0-7
static const struct NCR5380interface *intf;

static UINT8 n5380_Registers[8];
static UINT8 last_id;
static UINT8 n5380_Command[32];
static INT32 cmd_ptr, d_ptr, d_limit, next_req_flag;
static UINT8 n5380_Data[512];

// get the length of a SCSI command based on it's command byte type
static int get_cmd_len(int cbyte)
{
	int group;

	group = (cbyte>>5) & 7;

	if (group == 0) return 6;
	if (group == 1 || group == 2) return 10;
	if (group == 5) return 12;

	fatalerror("NCR5380: Unknown SCSI command group %d", group);

	return 6;
}

WRITE8_HANDLER(ncr5380_w)
{
	int reg = offset & 7;

	if (VERBOSE)
		logerror("NCR5380: %02x to %s (reg %d) [PC=%x]\n", data, wnames[reg], reg, cpu_get_pc(space->machine->firstcpu));

	switch( reg )
	{
		case R5380_OUTDATA:
			// if we're in the command phase, collect the command bytes
			if ((n5380_Registers[R5380_BUSSTATUS] & 0x1c) == 0x08)
			{
				n5380_Command[cmd_ptr++] = data;
			}

			// if we're in the select phase, this is the target id
			if (n5380_Registers[R5380_INICOMMAND] == 0x04)
			{
				data &= 0x7f;	// clear the high bit
				if (data == 0x40)
				{
					last_id = 6;
				}
				else if (data == 0x20)
				{
					last_id = 5;
				}
				else if (data == 0x10)
				{
					last_id = 4;
				}
				else if (data == 0x08)
				{
					last_id = 3;
				}
				else if (data == 0x04)
				{
					last_id = 2;
				}
				else if (data == 0x02)
				{
					last_id = 1;
				}
				else if (data == 0x01)
				{
					last_id = 0;
				}
			}

			// if this is a write, accumulate accordingly
			if (((n5380_Registers[R5380_BUSSTATUS] & 0x1c) == 0x00) && (n5380_Registers[R5380_INICOMMAND] == 1))
			{
				n5380_Data[d_ptr] = data;

				// if we've hit a sector, flush
				if (d_ptr == 511)
				{
					ncr5380_write_data(512, &n5380_Data[0]);

					d_limit -= 512;
					d_ptr = 0;

					// no more data?  set DMA END flag
					if (d_limit <= 0)
					{
						n5380_Registers[R5380_BUSANDSTAT] = 0xc8;
					}
				}
				else
				{
					d_ptr++;
				}

				// make sure we don't upset the status readback
				data = 0;
			}
			break;

		case R5380_INICOMMAND:
			if (data == 0)	// dropping the bus
			{
				// make sure it's not busy
				n5380_Registers[R5380_BUSSTATUS] &= ~0x40;

				// are we in the command phase?
				if ((n5380_Registers[R5380_BUSSTATUS] & 0x1c) == 0x08)
				{
					// is the current command complete?
					if (get_cmd_len(n5380_Command[0]) == cmd_ptr)
					{
						if (VERBOSE)
							logerror("NCR5380: Command (to ID %d): %x %x %x %x %x %x %x %x %x %x (PC %x)\n", last_id, n5380_Command[0], n5380_Command[1], n5380_Command[2], n5380_Command[3], n5380_Command[4], n5380_Command[5], n5380_Command[6], n5380_Command[7], n5380_Command[8], n5380_Command[9], cpu_get_pc(space->cpu));

						SCSISetCommand(devices[last_id], &n5380_Command[0], 16);
						SCSIExecCommand(devices[last_id], &d_limit);

						if (VERBOSE)
							logerror("NCR5380: Command returned %d bytes\n",  d_limit);

						d_ptr = 0;

						// is data available?
						if (d_limit > 0)
						{
							// make sure for transfers under 512 bytes that we always pad with a zero
							if (d_limit < 512)
							{
								n5380_Data[d_limit] = 0;
							}

							// read back the amount available, or 512 bytes, whichever is smaller
							ncr5380_read_data((d_limit < 512) ? d_limit : 512, n5380_Data);

							// raise REQ to indicate data is available
							n5380_Registers[R5380_BUSSTATUS] |= 0x20;
						}
					}
				}

			}

			if (data == 5)	// want the bus?
			{
				// if the device exists, make the bus busy.
				// otherwise don't.

				if (devices[last_id])
				{
					if (VERBOSE)
						logerror("NCR5380: Giving the bus for ID %d\n", last_id);
					n5380_Registers[R5380_BUSSTATUS] |= 0x40;
				}
				else
				{
					if (VERBOSE)
						logerror("NCR5380: Rejecting the bus for ID %d\n", last_id);
					n5380_Registers[R5380_BUSSTATUS] &= ~0x40;
				}
			}

			if (data == 1)	// data bus (prelude to command?)
			{
				// raise REQ
				n5380_Registers[R5380_BUSSTATUS] |= 0x20;
			}

			if (data & 0x10)	// ACK drops REQ
			{
				// drop REQ
				n5380_Registers[R5380_BUSSTATUS] &= ~0x20;
			}
			break;

		case R5380_MODE:
			if (data == 2)	// DMA
			{
				// put us in DMA mode
				n5380_Registers[R5380_BUSANDSTAT] |= 0x40;
			}

			if (data == 1)	// arbitrate?
			{
				n5380_Registers[R5380_INICOMMAND] |= 0x40;	// set arbitration in progress
				n5380_Registers[R5380_INICOMMAND] &= ~0x20;	// clear "lost arbitration"
			}

			if (data == 0)
			{
				// drop DMA mode
				n5380_Registers[R5380_BUSANDSTAT] &= ~0x40;
			}
			break;

		case R5380_TARGETCMD:
			// sync the bus phase with what was just written
			n5380_Registers[R5380_BUSSTATUS] &= ~0x1c;
			n5380_Registers[R5380_BUSSTATUS] |= (data & 7)<<2;

			// and set the "phase match" flag
			n5380_Registers[R5380_BUSANDSTAT] |= 0x08;

			// and set /REQ
			n5380_Registers[R5380_BUSSTATUS] |= 0x20;

			// if we're entering the command phase, start accumulating the data
			if ((n5380_Registers[R5380_BUSSTATUS] & 0x1c) == 0x08)
			{
				cmd_ptr = 0;
			}
			break;

		default:
			break;
	}

	n5380_Registers[reg] = data;

	// note: busandstat overlaps startdma, so we need to do this here!
	if (reg == R5380_STARTDMA)
	{
		n5380_Registers[R5380_BUSANDSTAT] = 0x48;
	}
}

READ8_HANDLER(ncr5380_r)
{
	int reg = offset & 7;
	UINT8 rv = 0;

	switch( reg )
	{
		case R5380_CURDATA:
		case R5380_INPUTDATA:
			rv = n5380_Registers[reg];

			// if we're in the data transfer phase or DMA, readback device data instead
			if (((n5380_Registers[R5380_BUSSTATUS] & 0x1c) == 0x04) || (n5380_Registers[R5380_BUSSTATUS] & 0x40))
			{
				rv = n5380_Data[d_ptr];

				// if the limit's less than 512, only read "limit" bytes
				if (d_limit < 512)
				{
					if (d_ptr < (d_limit-1))
					{
						d_ptr++;
					}
					else
					{
						next_req_flag = 1;
					}
				}
				else
				{
					if (d_ptr < 511)
					{
						d_ptr++;
					}
					else
					{
						d_limit -= 512;
						d_ptr = 0;

						next_req_flag = 1;

						// don't issue a "false" read
						if (d_limit > 0)
						{
							ncr5380_read_data((d_limit < 512) ? d_limit : 512, n5380_Data);
						}
						else
						{
							// if this is DMA, signal DMA end
							if (n5380_Registers[R5380_BUSSTATUS] & 0x40)
							{
								n5380_Registers[R5380_BUSSTATUS] |= 0x80;
							}

							// drop /REQ
							n5380_Registers[R5380_BUSSTATUS] &= ~0x20;

							// clear phase match
							n5380_Registers[R5380_BUSANDSTAT] &= ~0x08;
						}
					}
				}

			}
			break;

		default:
			rv = n5380_Registers[reg];

			// temporarily drop /REQ
			if ((reg == R5380_BUSSTATUS) && (next_req_flag))
			{
				rv &= ~0x20;
				next_req_flag = 0;
			}
			break;
	}

	if (VERBOSE)
		logerror("NCR5380: read %s (reg %d) = %02x [PC=%x]\n", rnames[reg], reg, rv, cpu_get_pc(space->machine->firstcpu));

	return rv;
}

void ncr5380_init( running_machine *machine, const struct NCR5380interface *interface )
{
	// save interface pointer for later
	intf = interface;

	memset(n5380_Registers, 0, sizeof(n5380_Registers));
	memset(n5380_Data, 0, sizeof(n5380_Data));
	memset(devices, 0, sizeof(devices));

	next_req_flag = 0;
	state_save_register_item_array(machine, "ncr5380", NULL, 0, n5380_Registers);
	state_save_register_item_array(machine, "ncr5380", NULL, 0, n5380_Command);
	state_save_register_item_array(machine, "ncr5380", NULL, 0, n5380_Data);
	state_save_register_item(machine, "ncr5380", NULL, 0, last_id);
	state_save_register_item(machine, "ncr5380", NULL, 0, cmd_ptr);
	state_save_register_item(machine, "ncr5380", NULL, 0, d_ptr);
	state_save_register_item(machine, "ncr5380", NULL, 0, d_limit);
	state_save_register_item(machine, "ncr5380", NULL, 0, next_req_flag);
}

void ncr5380_scan_devices( running_machine *machine )
{
	int i;

	// try to open the devices
	for (i = 0; i < intf->scsidevs->devs_present; i++)
	{
		// if a device wasn't already allocated
		if (!devices[intf->scsidevs->devices[i].scsiID])
		{
			SCSIAllocInstance( machine, 
					intf->scsidevs->devices[i].scsiClass, 
					&devices[intf->scsidevs->devices[i].scsiID],
					intf->scsidevs->devices[i].diskregion );
		}
	}
}

void ncr5380_exit( const struct NCR5380interface *interface )
{
	int i;

	// clean up the devices
	for (i = 0; i < interface->scsidevs->devs_present; i++)
	{
		SCSIDeleteInstance( devices[interface->scsidevs->devices[i].scsiID] );
	}
}

void ncr5380_read_data(int bytes, UINT8 *pData)
{
	if (devices[last_id])
	{
		if (VERBOSE)
			logerror("NCR5380: issuing read for %d bytes\n", bytes);
		SCSIReadData(devices[last_id], pData, bytes);
	}
	else
	{
		logerror("ncr5380: read unknown device SCSI ID %d\n", last_id);
	}
}

void ncr5380_write_data(int bytes, UINT8 *pData)
{
	if (devices[last_id])
	{
		SCSIWriteData(devices[last_id], pData, bytes);
	}
	else
	{
		logerror("ncr5380: write to unknown device SCSI ID %d\n", last_id);
	}
}

void *ncr5380_get_device(int id)
{
	void *ret;

	if (devices[id])
	{
		logerror("ncr5380: fetching dev pointer for SCSI ID %d\n", id);
		SCSIGetDevice(devices[id], &ret);

		return ret;
	}

	return NULL;
}

#if USE_5380_DEVICE
//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

ncr5380_device_config::ncr5380_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
    : device_config(mconfig, static_alloc_device_config, "5380 SCSI", tag, owner, clock)
{
}

device_config *ncr5380_device_config::static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
{ 
	return global_alloc(ncr5380_device_config(mconfig, tag, owner, clock));
}

device_t *ncr5380_device_config::alloc_device(running_machine &machine) const
{
	return auto_alloc(&machine, ncr5380_device(machine, *this));
}

//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void ncr5380_device_config::device_config_complete()
{
	// inherit a copy of the static data
	const NCR5380interface *intf = reinterpret_cast<const NCR5380interface *>(static_config());
	if (intf != NULL)
		*static_cast<NCR5380interface *>(this) = *intf;

	// or initialize to defaults if none provided
	else
	{
		scsidevs = NULL;
		irq_callback = NULL;
	}
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

const device_type NCR5380 = ncr5380_device_config::static_alloc_device_config;

//-------------------------------------------------
//  via6522_device - constructor
//-------------------------------------------------

ncr5380_device::ncr5380_device(running_machine &_machine, const ncr5380_device_config &config)
    : device_t(_machine, config),
      m_config(config)
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void ncr5380_device::device_start()
{
	int i;

	memset(m_5380_Registers, 0, sizeof(m_5380_Registers));
	memset(m_5380_Data, 0, sizeof(m_5380_Data));
	memset(m_scsi_devices, 0, sizeof(m_scsi_devices));

	m_next_req_flag = 0;

	// try to open the devices
	for (i = 0; i < m_config.scsidevs->devs_present; i++)
	{
		SCSIAllocInstance( machine, 
				m_config.scsidevs->devices[i].scsiClass, 
				&m_scsi_devices[m_config.scsidevs->devices[i].scsiID],
				m_config.scsidevs->devices[i].diskregion );
	}

	state_save_register_device_item_array(this, 0, m_5380_Registers);
	state_save_register_device_item_array(this, 0, m_Command);
	state_save_register_device_item_array(this, 0, m_5380_Data);
	state_save_register_device_item(this, 0, m_last_id);
	state_save_register_device_item(this, 0, m_cmd_ptr);
	state_save_register_device_item(this, 0, m_d_ptr);
	state_save_register_device_item(this, 0, m_d_limit);
	state_save_register_device_item(this, 0, m_next_req_flag);
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void ncr5380_device::device_reset()
{
	memset(m_5380_Registers, 0, sizeof(m_5380_Registers));
	memset(m_5380_Data, 0, sizeof(m_5380_Data));
	memset(m_scsi_devices, 0, sizeof(m_scsi_devices));

	m_next_req_flag = 0;
	m_cmd_ptr = 0;
	m_d_ptr = 0;
	m_d_limit = 0;
	m_last_id = 0;
}

UINT8 ncr5380_device::read(UINT8 offset)
{
	return 0;
}

void ncr5380_device::write(UINT8 offset, UINT8 data)
{
}

void ncr5380_device::read_data(int bytes, UINT8 *pData)
{
}

void ncr5380_device::write_data(int bytes, UINT8 *pData)
{
}

void *ncr5380_device::get_scsi_device(int id)
{
	return NULL;
}
#endif
