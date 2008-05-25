/*

	X68000 custom SASI Hard Disk controller

 0xe96001 (R/W) - SASI data I/O
 0xe96003 (W)   - SEL signal high (0)
 0xe96003 (R)   - SASI status
                  - bit 4 = MSG - if 1, content of data line is a message
                  - bit 3 = Command / Data - if 1, content of data line is a command or status, otherwise it is data.
                  - bit 2 = I/O - if 0, Host -> Controller (Output), otherwise Controller -> Host (Input).
                  - bit 1 = BSY - if 1, HD is busy.
                  - bit 0 = REQ - if 1, host is demanding data transfer to the host.
 0xe96005 (W/O) - data is arbitrary (?)
 0xe96007 (W/O) - SEL signal low (1)

*/

#include "x68k_hdc.h"

static TIMER_CALLBACK( req_delay )
{
	sasi_ctrl_t* sasi = ptr;
	sasi->req = 1;
	sasi->status_port |= 0x01;
}

DEVICE_START( x68k_hdc )
{
	sasi_ctrl_t* sasi = device->token;

	assert(device->machine != NULL);

	sasi->status = 0x00;
	sasi->status_port = 0x00;
	sasi->phase = SASI_PHASE_BUSFREE;
}

WRITE16_DEVICE_HANDLER( x68k_hdc_w )
{
	sasi_ctrl_t* sasi = device->token;

	switch(offset)
	{
	case 0x00:  // data I/O
		if(sasi->phase == SASI_PHASE_WRITE)
		{
			if(sasi->transfer_byte_count == 0)
			{
				switch(sasi->command[0])
				{
				case SASI_CMD_SPECIFY:
					sasi->transfer_byte_total = 10;
					break;
				default:
					sasi->transfer_byte_total = 0x800;
				}
			}
			// TODO: Do something with written data

			if(sasi->command[0] == SASI_CMD_SPECIFY)
			{
				logerror("SPECIFY: wrote 0x%02x\n",data);
			}

			sasi->req = 0;
			sasi->status_port &= ~0x01;
			timer_set(ATTOTIME_IN_NSEC(450),sasi,0,req_delay);
			sasi->transfer_byte_count++;
			if(sasi->transfer_byte_count >= sasi->transfer_byte_total)
			{
				// End of transfer
				sasi->phase = SASI_PHASE_STATUS;
				sasi->io = 1;
				sasi->status_port |= 0x04;
				sasi->cd = 1;
				sasi->status_port |= 0x08;
				logerror("SASI: Write transfer complete\n");
			}
		}
		if(sasi->phase == SASI_PHASE_COMMAND)
		{
			if(sasi->command_byte_count == 0)
			{
				// first command byte
				sasi->current_command = data;
				switch(data >> 5)  // high 3 bits determine command class, and therefore, length
				{
				case 0:
					sasi->command_byte_total = 6;
					break;
				case 1:
					sasi->command_byte_total = 10;
					break;
				case 2:
					sasi->command_byte_total = 8;
					break;
				default:
					sasi->command_byte_total = 6;
				}
			}
			sasi->command[sasi->command_byte_count] = data;
			// reset REQ temporarily
			sasi->req = 0;
			sasi->status_port &= ~0x01;
			timer_set(ATTOTIME_IN_NSEC(450),sasi,0,req_delay);

			sasi->command_byte_count++;
			if(sasi->command_byte_count >= sasi->command_byte_total)
			{
				// End of command
				// TODO: Actually execute SASI command

				switch(sasi->command[0])
				{
				case SASI_CMD_REZERO_UNIT:
					sasi->phase = SASI_PHASE_STATUS;
					sasi->io = 1;  // Output
					sasi->status_port |= 0x04;  // C/D remains the same
					logerror("SASI: REZERO UNIT\n");
					break;
				case SASI_CMD_REQUEST_SENSE:
					sasi->phase = SASI_PHASE_READ;
					sasi->io = 1;
					sasi->status_port |= 0x04;
					sasi->cd = 0;
					sasi->status_port &= ~0x08;
					sasi->transfer_byte_count = 0;
					sasi->transfer_byte_total = 0;
					logerror("SASI: REQUEST SENSE\n");
					break;
				case SASI_CMD_SPECIFY:
					sasi->phase = SASI_PHASE_WRITE;
					sasi->io = 0;
					sasi->status_port &= ~0x04;
					sasi->cd = 0;  // Data
					sasi->status_port &= ~0x08;
					sasi->transfer_byte_count = 0;
					sasi->transfer_byte_total = 0;
					logerror("SASI: SPECIFY\n");
					break;
				default:
					sasi->phase = SASI_PHASE_STATUS;
					sasi->io = 1;  // Output
					sasi->status_port |= 0x04;  // C/D remains the same
					sasi->status = 0x02;
					logerror("SASI: Invalid or unimplemented SASI command (0x%02x) recieved.\n",sasi->command[0]);
				}
			}
		}
		break;
	case 0x01:
		if(data == 0)
		{
			if(sasi->phase == SASI_PHASE_SELECTION)
			{
				// Go to Command phase
				sasi->phase = SASI_PHASE_COMMAND;
				sasi->cd = 1;   // data port expects a command or status
				sasi->status_port |= 0x08;
				sasi->command_byte_count = 0;
				sasi->command_byte_total = 0;
				timer_set(ATTOTIME_IN_NSEC(45),sasi,0,req_delay);
			}
		}
		break;
	case 0x02:
		break;
	case 0x03:
		if(data != 0)
		{
			if(sasi->phase == SASI_PHASE_BUSFREE)
			{
				// Go to Selection phase
				sasi->phase = SASI_PHASE_SELECTION;
				sasi->bsy = 1;  // HDC is now busy
				sasi->status_port |= 0x02;
			}
		}
		break;
	}

	logerror("SASI: write to HDC, offset %04x, data %04x\n",offset,data);
}

READ16_DEVICE_HANDLER( x68k_hdc_r )
{
	sasi_ctrl_t* sasi = device->token;
	int retval;

	switch(offset)
	{
	case 0x00:
		if(sasi->phase == SASI_PHASE_MESSAGE)
		{
			sasi->phase = SASI_PHASE_BUSFREE;
			sasi->msg = 0;
			sasi->cd = 0;
			sasi->io = 0;
			sasi->bsy = 0;
			sasi->req = 0;
			sasi->status = 0;
			sasi->status_port = 0;  // reset all status bits to 0
			return 0x00;
		}
		if(sasi->phase == SASI_PHASE_STATUS)
		{
			sasi->phase = SASI_PHASE_MESSAGE;
			sasi->msg = 1;
			sasi->status_port |= 0x10;
			// reset REQ temporarily
			sasi->req = 0;
			sasi->status_port &= ~0x01;
			timer_set(ATTOTIME_IN_NSEC(450),sasi,0,req_delay);

			return sasi->status;
		}
		if(sasi->phase == SASI_PHASE_READ)
		{
			if(sasi->transfer_byte_count == 0)
			{
				switch(sasi->command[0])
				{
				case SASI_CMD_REQUEST_SENSE:
					// set up sense bytes
					sasi->sense[0] = 0x01;  // "No index signal"
					sasi->sense[1] = 0;
					sasi->sense[2] = 0;
					sasi->sense[3] = 0;
					if(sasi->command[3] == 0)
						sasi->transfer_byte_total = 4;
					else
						sasi->transfer_byte_total = sasi->command[3];
					break;
				default:
					sasi->transfer_byte_total = 0;
				}
			}
			
			switch(sasi->command[0])
			{
			case SASI_CMD_REQUEST_SENSE:
				retval = sasi->sense[sasi->transfer_byte_count];
				logerror("REQUEST SENSE: read value 0x%02x\n",retval);
				break;
			default:
				retval = 0;
			}

			sasi->req = 0;
			sasi->status_port &= ~0x01;
			timer_set(ATTOTIME_IN_NSEC(450),sasi,0,req_delay);
			sasi->transfer_byte_count++;
			if(sasi->transfer_byte_count >= sasi->transfer_byte_total)
			{
				// End of transfer
				sasi->phase = SASI_PHASE_STATUS;
				sasi->io = 1;
				sasi->status_port |= 0x04;
				sasi->cd = 1;
				sasi->status_port |= 0x08;
				logerror("SASI: Read transfer complete\n");
			}

			return retval;
		}
		return 0x00;
	case 0x01:
		logerror("SASI: [%08x] read from status port, read 0x%02x\n",activecpu_get_pc(),sasi->status_port);
		return sasi->status_port;
	case 0x02:
		return 0xff;  // write-only
	case 0x03:
		return 0xff;  // write-only
	default:
		return 0xff;
	}
}

DEVICE_SET_INFO( x68k_hdc )
{
	switch (state)
	{
		/* no parameters to set */
	}
}

DEVICE_GET_INFO(x68k_hdc)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_OTHER;				break;
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(sasi_ctrl_t);				break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_SET_INFO:						info->set_info = DEVICE_SET_INFO_NAME(x68k_hdc); break;
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(x68k_hdc);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							/*info->reset = DEVICE_RESET_NAME(x68k_hdc);*/	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							info->s = "X68000 SASI Hard Disk Controller";	break;
		case DEVINFO_STR_FAMILY:						info->s = "SASI Hard Disk Controller";			break;
		case DEVINFO_STR_VERSION:						info->s = "1.0";							break;
		case DEVINFO_STR_SOURCE_FILE:					info->s = __FILE__;							break;
		case DEVINFO_STR_CREDITS:						info->s = "Copyright the MESS Team"; 		break;
	}
}

