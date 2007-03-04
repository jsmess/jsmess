/* Intel 8271 Floppy Disc Controller */
/* used in BBC Micro B,Acorn Atom */
/* Jun 2000. Kev Thacker */

/* TODO:

	- Scan commands
	- Check the commands work properly using a BBC disc copier program 
    - check if 0 is specified as number of sectors, how many sectors
    is actually transfered
	- deleted data functions (error if data finds deleted data?)
*/

#include "includes/i8271.h"
#include "devices/flopdrv.h"

I8271_STATE_t I8271_STATE;

static void i8271_command_execute(void);
static void i8271_command_continue(void);
static void i8271_command_complete(int result, int int_rq);
static void i8271_data_request(void);
static void i8271_timed_data_request(void);
/* locate sector for read/write operation */
static int i8271_find_sector(void);
/* do a read operation */
static void i8271_do_read(void);
static void i8271_do_write(void);
static void i8271_do_read_id(void);
static void i8271_set_irq_state(int);
static void i8271_set_dma_drq(void);

static void	i8271_data_timer_callback(int code);
static void	i8271_timed_command_complete_callback(int code);

static char temp_buffer[16384];

// uncomment to give verbose debug information
//#define VERBOSE

static I8271 i8271;

#ifdef VERBOSE
#define FDC_LOG(x) logerror("I8271: %s\n",x)
#define FDC_LOG_COMMAND(x) logerror("I8271: COMMAND %s\n",x)
#else
#define FDC_LOG(x)
#define FDC_LOG_COMMAND(x)
#endif

static mess_image *current_image(void)
{
	return image_from_devtype_and_index(IO_FLOPPY, i8271.drive);
}

void i8271_init(i8271_interface *iface)
{
	memset(&i8271, 0, sizeof(I8271));

	if (iface!=NULL)
	{
		memcpy(&i8271.fdc_interface, iface, sizeof(i8271_interface));
	}
	i8271.data_timer = timer_alloc(i8271_data_timer_callback);
	i8271.command_complete_timer = timer_alloc(i8271_timed_command_complete_callback);
	i8271.drive = 0;
	i8271.pExecutionPhaseData = temp_buffer;

    i8271_reset();
}

	
static void i8271_seek_to_track(int track)
{
	mess_image *img = current_image();

	if (track==0)
	{
		/* seek to track 0 */
		unsigned char StepCount = 0x0ff;

        /*logerror("step\n"); */

		while (
			/* track 0 not set */
			(!floppy_drive_get_flag_state(img, FLOPPY_DRIVE_HEAD_AT_TRACK_0)) &&
			/* not seeked more than 255 tracks */
			(StepCount!=0)
			)
		{
/*            logerror("step\n"); */
			StepCount--;
			floppy_drive_seek(img, -1);
		}

		i8271.CurrentTrack[i8271.drive] = 0;
	
		/* failed to find track 0? */
		if (StepCount==0)
		{
			/* Completion Type: operator intervation probably required for recovery */
			/* Completion code: track 0 not found */
			i8271.ResultRegister |= (2<<3) | 2<<1;
		}

		/* step out - towards track 0 */
		i8271.drive_control_output &=~(1<<2);
	}
	else
	{

		signed int SignedTracks;

		/* calculate number of tracks to seek */
		SignedTracks = track - i8271.CurrentTrack[i8271.drive];

		/* step towards 0 */
		i8271.drive_control_output &= ~(1<<2);
		
		if (SignedTracks>0)
		{
			/* step away from 0 */
			i8271.drive_control_output |= (1<<2);
		}


		/* seek to track 0 */
		floppy_drive_seek(img, SignedTracks);

		i8271.CurrentTrack[i8271.drive] = track;
	}
}
	

static void	i8271_data_timer_callback(int code)
{
	/* ok, trigger data request now */
	i8271_data_request();

	/* stop it */
	timer_reset(i8271.data_timer, TIME_NEVER); 
}

/* setup a timed data request - data request will be triggered in a few usecs time */
static void i8271_timed_data_request(void)
{
	int usecs;

	/* 64 for single density */
	usecs = 64;

	/* set timers */
	timer_reset(i8271.command_complete_timer, TIME_NEVER);
	timer_adjust(i8271.data_timer, TIME_IN_USEC(usecs), 0, 0);
}


static void	i8271_timed_command_complete_callback(int code)
{
	i8271_command_complete(1,1);

	/* stop it, but don't allow it to be free'd */
	timer_reset(i8271.command_complete_timer, TIME_NEVER); 
}

/* setup a irq to occur 128us later - in reality this would be much later, because the int would
come after reading the two CRC bytes at least! This function is used when a irq is required at
command completion. Required for read data and write data, where last byte could be missed! */
static void i8271_timed_command_complete(void)
{
	int usecs;

	/* 64 for single density - 2 crc bytes later*/
	usecs = 64*2;

	/* set timers */
	timer_reset(i8271.data_timer, TIME_NEVER);
	timer_adjust(i8271.command_complete_timer, TIME_IN_USEC(usecs), 0, 0);
}

void i8271_reset()
{
	i8271.StatusRegister = 0;	//I8271_STATUS_INT_REQUEST | I8271_STATUS_NON_DMA_REQUEST;
	i8271.Mode = 0x0c0; /* bits 0, 1 are initialized to zero */
	i8271.ParameterCountWritten = 0;
	i8271.ParameterCount = 0;

	/* if timer is active remove */
	timer_reset(i8271.command_complete_timer, TIME_NEVER);
	timer_reset(i8271.data_timer, TIME_NEVER);

	/* clear irq */
	i8271_set_irq_state(0);
	/* clear dma */
	i8271_set_dma_drq();
}

static void i8271_set_irq_state(int state)
{
	i8271.StatusRegister &= ~I8271_STATUS_INT_REQUEST;
	if (state)
	{
		i8271.StatusRegister |= I8271_STATUS_INT_REQUEST;
	}

	if (i8271.fdc_interface.interrupt)
	{
		i8271.fdc_interface.interrupt((i8271.StatusRegister & I8271_STATUS_INT_REQUEST));
	}
}

static void i8271_set_dma_drq(void)
{
	if (i8271.fdc_interface.dma_request)
	{
		i8271.fdc_interface.dma_request((i8271.flags & I8271_FLAGS_DATA_REQUEST), (i8271.flags & I8271_FLAGS_DATA_DIRECTION));
	}
}

static void i8271_load_bad_tracks(int surface)
{
	i8271.BadTracks[(surface<<1) + 0] = i8271.CommandParameters[1];
	i8271.BadTracks[(surface<<1) + 1] = i8271.CommandParameters[2];
	i8271.CurrentTrack[surface] = i8271.CommandParameters[3];
}

static void i8271_write_bad_track(int surface, int track, int data)
{
	i8271.BadTracks[(surface<<1) + (track-1)] = data;
}

static void i8271_write_current_track(int surface, int track)
{
	i8271.CurrentTrack[surface] = track;
}

static int i8271_read_current_track(int surface)
{
	return i8271.CurrentTrack[surface];
}

static int i8271_read_bad_track(int surface, int track)
{
	return i8271.BadTracks[(surface<<1) + (track-1)];
}

static void i8271_get_drive(void)
{
	/* &40 = drive 0 side 0 */
	/* &80 = drive 1 side 0 */


	
	if (i8271.CommandRegister & (1<<6))
	{
		i8271.drive = 0;
	}

	if (i8271.CommandRegister & (1<<7))
	{
		i8271.drive = 1;
	}

}

static void i8271_check_all_parameters_written(void)
{
	if (i8271.ParameterCount == i8271.ParameterCountWritten)
	{
		i8271.StatusRegister &= ~I8271_STATUS_COMMAND_FULL;

		i8271_command_execute();
	}
}


static void i8271_update_state(void)
{
	switch (i8271.state)
	{
		/* fdc reading data and passing it to cpu which must read it */
		case I8271_STATE_EXECUTION_READ:
		{
	//		/* if data request has been cleared, i.e. caused by a read of the register */
	//		if ((i8271.flags & I8271_FLAGS_DATA_REQUEST)==0)
			{
				/* setup data with byte */
				i8271.data = i8271.pExecutionPhaseData[i8271.ExecutionPhaseCount];
			
/*				logerror("read data %02x\n", i8271.data); */

				/* update counters */
				i8271.ExecutionPhaseCount++;
				i8271.ExecutionPhaseTransferCount--;

			//	logerror("Count: %04x\n", i8271.ExecutionPhaseCount);
			//	logerror("Remaining: %04x\n", i8271.ExecutionPhaseTransferCount);

				/* completed? */
				if (i8271.ExecutionPhaseTransferCount==0)
				{
					/* yes */

			//		logerror("sector read complete!\n");
					/* continue command */
					i8271_command_continue();
				}
				else
				{
					/* no */

					/* issue data request */
					i8271_timed_data_request();
				}
			}
		}
		break;

		/* fdc reading data and passing it to cpu which must read it */
		case I8271_STATE_EXECUTION_WRITE:
		{
			/* setup data with byte */
			i8271.pExecutionPhaseData[i8271.ExecutionPhaseCount] = i8271.data;
			/* update counters */
			i8271.ExecutionPhaseCount++;
			i8271.ExecutionPhaseTransferCount--;

			/* completed? */
			if (i8271.ExecutionPhaseTransferCount==0)
			{
				/* yes */

				/* continue command */
				i8271_command_continue();
			}
			else
			{
				/* no */

				/* issue data request */
				i8271_timed_data_request();
			}
		}
		break;

		default:
			break;
	}
}

static void i8271_initialise_execution_phase_read(int transfer_size)
{
	/* read */
	i8271.flags |= I8271_FLAGS_DATA_DIRECTION;
	i8271.ExecutionPhaseCount = 0;
	i8271.ExecutionPhaseTransferCount = transfer_size;	
	i8271.state = I8271_STATE_EXECUTION_READ;
}


static void i8271_initialise_execution_phase_write(int transfer_size)
{
	/* write */
	i8271.flags &= ~I8271_FLAGS_DATA_DIRECTION;
	i8271.ExecutionPhaseCount = 0;
	i8271.ExecutionPhaseTransferCount = transfer_size;
	i8271.state = I8271_STATE_EXECUTION_WRITE;
}

/* for data transfers */
static void i8271_data_request(void)
{
	i8271.flags |= I8271_FLAGS_DATA_REQUEST;

	if ((i8271.Mode & 0x01)!=0)
	{
		/* non-dma */
		i8271.StatusRegister |= I8271_STATUS_NON_DMA_REQUEST;
		/* set int */
		i8271_set_irq_state(1);
	}
	else
	{
		/* dma */
		i8271.StatusRegister &= ~I8271_STATUS_NON_DMA_REQUEST;

		i8271_set_dma_drq();
	}
}

static void i8271_command_complete(int result, int int_rq)
{
	/* not busy, and not a execution phase data request in non-dma mode */
	i8271.StatusRegister &= ~(I8271_STATUS_COMMAND_BUSY | I8271_STATUS_NON_DMA_REQUEST);
	
	if (result)
	{
		i8271.StatusRegister |= I8271_STATUS_RESULT_FULL;
	}

	if (int_rq)
	{
		/* trigger an int */
		i8271_set_irq_state(1);
    }
	
	/* correct?? */
    i8271.drive_control_output &=~1;
}


/* for data transfers */
static void i8271_clear_data_request(void)
{
	i8271.flags &= ~I8271_FLAGS_DATA_REQUEST;

	if ((i8271.Mode & 0x01)!=0)
	{
		/* non-dma */
		i8271.StatusRegister &= ~I8271_STATUS_NON_DMA_REQUEST;
		/* set int */
		i8271_set_irq_state(0);
	}
	else
	{
		/* dma */
		i8271_set_dma_drq();
	}
}


static void i8271_command_continue(void)
{
	switch (i8271.Command)
	{
		case I8271_COMMAND_READ_DATA_MULTI_RECORD:
		case I8271_COMMAND_READ_DATA_SINGLE_RECORD:
		{
			/* completed all sectors? */
			i8271.Counter--;
			/* increment sector id */
			i8271.ID_R++;

			/* end command? */
			if (i8271.Counter==0)
			{
				
				i8271_timed_command_complete();
				return;
			}
			
			i8271_do_read();
		}
		break;

		case I8271_COMMAND_WRITE_DATA_MULTI_RECORD:
		case I8271_COMMAND_WRITE_DATA_SINGLE_RECORD:
		{
			/* put the buffer to the sector */
			floppy_drive_write_sector_data(current_image(), i8271.side, i8271.data_id, i8271.pExecutionPhaseData, 1<<(i8271.ID_N+7),0);

			/* completed all sectors? */
			i8271.Counter--;
			/* increment sector id */
			i8271.ID_R++;

			/* end command? */
			if (i8271.Counter==0)
			{
				
				i8271_timed_command_complete();
				return;
			}
			
			i8271_do_write();
		}
		break;

		case I8271_COMMAND_READ_ID:
		{
			i8271.Counter--;

			if (i8271.Counter==0)
			{
				i8271_timed_command_complete();
				return;
			}

			i8271_do_read_id();
		}
		break;

		default:
			break;
	}
}

static void i8271_do_read(void)
{
	/* find the sector */
	if (i8271_find_sector())
	{
		/* get the sector into the buffer */
		floppy_drive_read_sector_data(current_image(), i8271.side, i8271.data_id, i8271.pExecutionPhaseData, 1<<(i8271.ID_N+7));
			
		/* initialise for reading */
        i8271_initialise_execution_phase_read(1<<(i8271.ID_N+7));
		
		/* update state - gets first byte and triggers a data request */
		i8271_timed_data_request();
		return;
	}
#ifdef VERBOSE
	logerror("error getting sector data\n");
#endif

	i8271_timed_command_complete();
}

static void i8271_do_read_id(void)
{
	chrn_id	id;

	/* get next id from disc */
	floppy_drive_get_next_id(current_image(), i8271.side,&id);

	i8271.pExecutionPhaseData[0] = id.C;
	i8271.pExecutionPhaseData[1] = id.H;
	i8271.pExecutionPhaseData[2] = id.R;
	i8271.pExecutionPhaseData[3] = id.N;

	i8271_initialise_execution_phase_read(4);
}


static void i8271_do_write(void)
{
	/* find the sector */
	if (i8271_find_sector())
	{
		/* initialise for reading */
        i8271_initialise_execution_phase_write(1<<(i8271.ID_N+7));
		
		/* update state - gets first byte and triggers a data request */
		i8271_timed_data_request();
		return;
	}
#ifdef VERBOSE
	logerror("error getting sector data\n");
#endif

	i8271_timed_command_complete();
}



static int i8271_find_sector(void)
{
	mess_image *img = current_image();
//	int track_count_attempt;

//	track_count_attempt
	/* find sector within one revolution of the disc - 2 index pulses */

	/* number of times we have seen index hole */
	int index_count = 0;

	/* get sector id's */
	do
    {
		chrn_id id;

		/* get next id from disc */
		if (floppy_drive_get_next_id(img, i8271.side,&id))
		{
			/* tested on Amstrad CPC - All bytes must match, otherwise
			a NO DATA error is reported */
			if (id.R == i8271.ID_R)
			{
				/* TODO: Is this correct? What about bad tracks? */
				if (id.C == i8271.CurrentTrack[i8271.drive])
				{
					i8271.data_id = id.data_id;
					return 1;
				}
				else
				{
					/* TODO: if track doesn't match, the real 8271 does a step */


					return 0;
				}
			}
		}

		 /* index set? */
		if (floppy_drive_get_flag_state(img, FLOPPY_DRIVE_INDEX))
		{
			index_count++;
		}
   
	}
	while (index_count!=2);

	/* completion type: command/drive error */
	/* completion code: sector not found */
	i8271.ResultRegister |= (3<<3);
	
	return 0;
}

static void i8271_command_execute(void)
{
	mess_image *img = current_image();

	/* clear it = good completion status */
	/* this will be changed if anything bad happens! */
	i8271.ResultRegister = 0;

	switch (i8271.Command)
	{
		case I8271_COMMAND_SPECIFY:
		{
			switch (i8271.CommandParameters[0])
			{
				case 0x0d:
				{
#ifdef VERBOSE
					logerror("Initialization\n");
#endif
					i8271.StepRate = i8271.CommandParameters[1];
					i8271.HeadSettlingTime = i8271.CommandParameters[2];
					i8271.IndexCountBeforeHeadUnload = (i8271.CommandParameters[3]>>4) & 0x0f;
					i8271.HeadLoadTime = (i8271.CommandParameters[3] & 0x0f);
				}
				break;

				case 0x010:
				{
#ifdef VERBOSE
					logerror("Load bad Tracks Surface 0\n");
#endif
					i8271_load_bad_tracks(0);

				}
				break;

				case 0x018:
				{
#ifdef VERBOSE
					logerror("Load bad Tracks Surface 1\n");
#endif
					i8271_load_bad_tracks(1);

				}
				break;
			}

			/* no result */
			i8271_command_complete(0,0);
		}
		break;

		case I8271_COMMAND_READ_SPECIAL_REGISTER:
		{
			/* unknown - what is read when a special register that isn't allowed is specified? */
			int data = 0x0ff;

			switch (i8271.CommandParameters[0])
			{
				case I8271_SPECIAL_REGISTER_MODE_REGISTER:
				{
					data = i8271.Mode;
				}
				break;

				case I8271_SPECIAL_REGISTER_SURFACE_0_CURRENT_TRACK:
				{
					data = i8271_read_current_track(0);

				}
				break;

				case I8271_SPECIAL_REGISTER_SURFACE_1_CURRENT_TRACK:
				{
					data = i8271_read_current_track(1);
				}
				break;

				case I8271_SPECIAL_REGISTER_SURFACE_0_BAD_TRACK_1:
				{
					data = i8271_read_bad_track(0,1);
				}
				break;

				case I8271_SPECIAL_REGISTER_SURFACE_0_BAD_TRACK_2:
				{
					data = i8271_read_bad_track(0,2);
				}
				break;

				case I8271_SPECIAL_REGISTER_SURFACE_1_BAD_TRACK_1:
				{
					data = i8271_read_bad_track(1,1);
				}
				break;

				case I8271_SPECIAL_REGISTER_SURFACE_1_BAD_TRACK_2:
				{
					data = i8271_read_bad_track(1,2);
				}
				break;

				case I8271_SPECIAL_REGISTER_DRIVE_CONTROL_OUTPUT_PORT:
				{
					FDC_LOG_COMMAND("Read Drive Control Output port\n");

					i8271_get_drive();

					/* assumption: select bits reflect the select bits from the previous
					command. i.e. read drive status */
					data = (i8271.drive_control_output & ~0x0c0)
						| (i8271.CommandRegister & 0x0c0);


				}
				break;

				case I8271_SPECIAL_REGISTER_DRIVE_CONTROL_INPUT_PORT:
				{
					/* bit 7: not used */
					/* bit 6: ready 1 */
					/* bit 5: write fault */
					/* bit 4: index */
					/* bit 3: wr prot */
					/* bit 2: rdy 0 */
					/* bit 1: track 0 */
					/* bit 0: cnt/opi */

					FDC_LOG_COMMAND("Read Drive Control Input port\n");


					i8271.drive_control_input = (1<<6) | (1<<2);

					/* bit 3 = 0 if write protected */
					if (!floppy_drive_get_flag_state(img, FLOPPY_DRIVE_DISK_WRITE_PROTECTED))
					{
						i8271.drive_control_input |= (1<<3);
					}

					/* bit 1 = 0 if head at track 0 */
					if (!floppy_drive_get_flag_state(img, FLOPPY_DRIVE_HEAD_AT_TRACK_0))
					{
						i8271.drive_control_input |= (1<<1);
					}

					
					/* need to setup this register based on drive selected */
					data = i8271.drive_control_input;


			

				}
				break;

			}

			i8271.ResultRegister = data;

			i8271_command_complete(1,0);
		}
		break;


		case I8271_COMMAND_WRITE_SPECIAL_REGISTER:
		{
			switch (i8271.CommandParameters[0])
			{
				case I8271_SPECIAL_REGISTER_MODE_REGISTER:
				{
					/* TODO: Check bits 6-7 and 5-2 are valid */
					i8271.Mode = i8271.CommandParameters[1];

#ifdef VERBOSE
					if (i8271.Mode & 0x01)
					{
						logerror("Mode: Non-DMA\n");
					}
					else
					{
						logerror("Mode: DMA\n");
					}

					if (i8271.Mode & 0x02)
					{
						logerror("Single actuator\n");
					}
					else
					{
						logerror("Double actuator\n");
					}
#endif
				}
				break;

				case I8271_SPECIAL_REGISTER_SURFACE_0_CURRENT_TRACK:
				{
#ifdef VERBOSE
					logerror("Surface 0 Current Track\n");
#endif
					i8271_write_current_track(0, i8271.CommandParameters[1]);
				}
				break;

				case I8271_SPECIAL_REGISTER_SURFACE_1_CURRENT_TRACK:
				{
#ifdef VERBOSE
					logerror("Surface 1 Current Track\n");
#endif
					i8271_write_current_track(1, i8271.CommandParameters[1]);
				}
				break;

				case I8271_SPECIAL_REGISTER_SURFACE_0_BAD_TRACK_1:
				{
#ifdef VERBOSE
					logerror("Surface 0 Bad Track 1\n");
#endif
					i8271_write_bad_track(0, 1, i8271.CommandParameters[1]);
				}
				break;

				case I8271_SPECIAL_REGISTER_SURFACE_0_BAD_TRACK_2:
				{
#ifdef VERBOSE
					logerror("Surface 0 Bad Track 2\n");
#endif
				i8271_write_bad_track(0, 2,i8271.CommandParameters[1]);
				}
				break;

				case I8271_SPECIAL_REGISTER_SURFACE_1_BAD_TRACK_1:
				{
#ifdef VERBOSE
					logerror("Surface 1 Bad Track 1\n");
#endif


					i8271_write_bad_track(1, 1, i8271.CommandParameters[1]);
				}
				break;

				case I8271_SPECIAL_REGISTER_SURFACE_1_BAD_TRACK_2:
				{
#ifdef VERBOSE
					logerror("Surface 1 Bad Track 2\n");
#endif

					i8271_write_bad_track(1, 2, i8271.CommandParameters[1]);
				}
				break;

				case I8271_SPECIAL_REGISTER_DRIVE_CONTROL_OUTPUT_PORT:
				{
//					/* get drive selected */
//					i8271.drive = (i8271.CommandParameters[1]>>6) & 0x03;

					FDC_LOG_COMMAND("Write Drive Control Output port\n");


#ifdef VERBOSE
					if (i8271.CommandParameters[1] & 0x01)
					{
						logerror("Write Enable\n");
					}
					if (i8271.CommandParameters[1] & 0x02)
					{
						logerror("Seek/Step\n");
					}
					if (i8271.CommandParameters[1] & 0x04)
					{
						logerror("Direction\n");
					}
					if (i8271.CommandParameters[1] & 0x08)
					{
						logerror("Load Head\n");
					}
					if (i8271.CommandParameters[1] & 0x010)
					{
						logerror("Low head current\n");
					}
					if (i8271.CommandParameters[1] & 0x020)
					{
						logerror("Write Fault Reset\n");
					}

					logerror("Select %02x\n", (i8271.CommandParameters[1] & 0x0c0)>>6);
#endif
					/* get drive */
					i8271_get_drive();

					/* on bbc dfs 09 this is the side select output */
					i8271.side = (i8271.CommandParameters[1]>>5) & 0x01;

					/* load head - on mini-sized drives this turns on the disc motor,
					on standard-sized drives this loads the head and turns the motor on */
					floppy_drive_set_motor_state(img, i8271.CommandParameters[1] & 0x08);
					floppy_drive_set_ready_state(img, 1, 1);

					/* step pin changed? if so perform a step in the direction indicated */
					if (((i8271.drive_control_output^i8271.CommandParameters[1]) & (1<<1))!=0)
					{
						/* step pin changed state? */

						if ((i8271.CommandParameters[1] & (1<<1))!=0)
						{
							signed int signed_tracks;

							if ((i8271.CommandParameters[1] & (1<<2))!=0)
							{
								signed_tracks = 1;
							}
							else
							{
								signed_tracks = -1;
							}

							floppy_drive_seek(img, signed_tracks);
						}
					}

					i8271.drive_control_output = i8271.CommandParameters[1];


				}
				break;

				case I8271_SPECIAL_REGISTER_DRIVE_CONTROL_INPUT_PORT:
				{

					FDC_LOG_COMMAND("Write Drive Control Input port\n");

					//					i8271.drive_control_input = i8271.CommandParameters[1];
				}
				break;

			}

			/* write doesn't supply a result */
			i8271_command_complete(0,0);
		}
		break;

		case I8271_COMMAND_READ_DRIVE_STATUS:
		{
			unsigned char status;

			i8271_get_drive();

			/* no write fault */
			status = 0;
			
			status |= (1<<2) | (1<<6);

			/* these two do not appear to be set at all! ?? */
			if (floppy_drive_get_flag_state(image_from_devtype_and_index(IO_FLOPPY, 0), FLOPPY_DRIVE_READY))
			{
				status |= (1<<2);
			}

			if (floppy_drive_get_flag_state(image_from_devtype_and_index(IO_FLOPPY, 1), FLOPPY_DRIVE_READY))
			{
				status |= (1<<6);
			}
			
			/* bit 3 = 1 if write protected */
			if (floppy_drive_get_flag_state(img, FLOPPY_DRIVE_DISK_WRITE_PROTECTED))
			{
				status |= (1<<3);
			}

			/* bit 1 = 1 if head at track 0 */
			if (floppy_drive_get_flag_state(img, FLOPPY_DRIVE_HEAD_AT_TRACK_0))
			{
				status |= (1<<1);
			}

			i8271.ResultRegister = status;
			i8271_command_complete(1,0);

		}
		break;

		case I8271_COMMAND_SEEK:
		{
			i8271_get_drive();


			i8271_seek_to_track(i8271.CommandParameters[0]);

			/* check for bad seek */
			i8271_timed_command_complete();

		}
		break;

		case I8271_COMMAND_READ_DATA_MULTI_RECORD:
		{
			/* N value as stored in ID field */
			i8271.ID_N = (i8271.CommandParameters[2]>>5) & 0x07;

			/* starting sector id */
			i8271.ID_R = i8271.CommandParameters[1];

			/* number of sectors to transfer */
			i8271.Counter = i8271.CommandParameters[2] & 0x01f;


			FDC_LOG_COMMAND("READ DATA MULTI RECORD");

#ifdef VERBOSE
			logerror("Sector Count: %02x\n", i8271.Counter);
			logerror("Track: %02x\n",i8271.CommandParameters[0]);
			logerror("Sector: %02x\n", i8271.CommandParameters[1]);
			logerror("Sector Length: %02x bytes\n", 1<<(i8271.ID_N+7));
#endif

			i8271_get_drive();

			if (!floppy_drive_get_flag_state(img, FLOPPY_DRIVE_READY))
			{
				/* Completion type: operation intervention probably required for recovery */
				/* Completion code: Drive not ready */
				i8271.ResultRegister = (2<<3);
				i8271_timed_command_complete();
			}
			else
			{
				i8271_seek_to_track(i8271.CommandParameters[0]);


				i8271_do_read();
			}

		}
		break;

		case I8271_COMMAND_READ_DATA_SINGLE_RECORD:
		{
			FDC_LOG_COMMAND("READ DATA SINGLE RECORD");

			i8271.ID_N = 0;
			i8271.Counter = 1;
			i8271.ID_R = i8271.CommandParameters[1];

#ifdef VERBOSE
			logerror("Sector Count: %02x\n", i8271.Counter);
			logerror("Track: %02x\n",i8271.CommandParameters[0]);
			logerror("Sector: %02x\n", i8271.CommandParameters[1]);
			logerror("Sector Length: %02x bytes\n", 1<<(i8271.ID_N+7));
#endif
			i8271_get_drive();

			if (!floppy_drive_get_flag_state(img, FLOPPY_DRIVE_READY))
			{
				/* Completion type: operation intervention probably required for recovery */
				/* Completion code: Drive not ready */
				i8271.ResultRegister = (2<<3);
				i8271_timed_command_complete();
			}
			else
			{
				i8271_seek_to_track(i8271.CommandParameters[0]);

				i8271_do_read();
			}

		}
		break;

		case I8271_COMMAND_WRITE_DATA_MULTI_RECORD:
		{
			/* N value as stored in ID field */
			i8271.ID_N = (i8271.CommandParameters[2]>>5) & 0x07;

			/* starting sector id */
			i8271.ID_R = i8271.CommandParameters[1];

			/* number of sectors to transfer */
			i8271.Counter = i8271.CommandParameters[2] & 0x01f;

			FDC_LOG_COMMAND("READ DATA MULTI RECORD");

#ifdef VERBOSE
			logerror("Sector Count: %02x\n", i8271.Counter);
			logerror("Track: %02x\n",i8271.CommandParameters[0]);
			logerror("Sector: %02x\n", i8271.CommandParameters[1]);
			logerror("Sector Length: %02x bytes\n", 1<<(i8271.ID_N+7));
#endif

			i8271_get_drive();

            i8271.drive_control_output &=~1;
            
			if (!floppy_drive_get_flag_state(img, FLOPPY_DRIVE_READY))
			{
				/* Completion type: operation intervention probably required for recovery */
				/* Completion code: Drive not ready */
				i8271.ResultRegister = (2<<3);
				i8271_timed_command_complete();
			}
			else
			{
				if (floppy_drive_get_flag_state(img, FLOPPY_DRIVE_DISK_WRITE_PROTECTED))
				{
					/* Completion type: operation intervention probably required for recovery */
					/* Completion code: Drive write protected */
					i8271.ResultRegister = (2<<3) | (1<<1);
					i8271_timed_command_complete();
				}
				else
				{
                    i8271.drive_control_output |=1;

					i8271_seek_to_track(i8271.CommandParameters[0]);

					i8271_do_write();
				}
			}
		}
		break;

		case I8271_COMMAND_WRITE_DATA_SINGLE_RECORD:
		{
			FDC_LOG_COMMAND("WRITE DATA SINGLE RECORD");

			i8271.ID_N = 0;
			i8271.Counter = 1;
			i8271.ID_R = i8271.CommandParameters[1];


#ifdef VERBOSE
			logerror("Sector Count: %02x\n", i8271.Counter);
			logerror("Track: %02x\n",i8271.CommandParameters[0]);
			logerror("Sector: %02x\n", i8271.CommandParameters[1]);
			logerror("Sector Length: %02x bytes\n", 1<<(i8271.ID_N+7));
#endif
			i8271_get_drive();

            i8271.drive_control_output &=~1;

			if (!floppy_drive_get_flag_state(img, FLOPPY_DRIVE_READY))
			{
				/* Completion type: operation intervention probably required for recovery */
				/* Completion code: Drive not ready */
				i8271.ResultRegister = (2<<3);
				i8271_timed_command_complete();
			}
			else
			{
				if (floppy_drive_get_flag_state(img, FLOPPY_DRIVE_DISK_WRITE_PROTECTED))
				{
					/* Completion type: operation intervention probably required for recovery */
					/* Completion code: Drive write protected */
					i8271.ResultRegister = (2<<3) | (1<<1);
					i8271_timed_command_complete();
				}
				else
				{

                    i8271.drive_control_output |=1;

					i8271_seek_to_track(i8271.CommandParameters[0]);

					i8271_do_write();
				}
			}

		}
		break;


		case I8271_COMMAND_READ_ID:
		{
			FDC_LOG_COMMAND("READ ID");

#ifdef VERBOSE
			logerror("Track: %02x\n",i8271.CommandParameters[0]);
			logerror("ID Field Count: %02x\n", i8271.CommandParameters[2]);
#endif

			i8271_get_drive();
	
			if (!floppy_drive_get_flag_state(img, FLOPPY_DRIVE_READY))
			{
				/* Completion type: operation intervention probably required for recovery */
				/* Completion code: Drive not ready */
				i8271.ResultRegister = (2<<3);
				i8271_timed_command_complete();
			}
			else
			{

				i8271.Counter = i8271.CommandParameters[2];

				i8271_seek_to_track(i8271.CommandParameters[0]);

				i8271_do_read_id();
			}
		}
		break;

		default:
#ifdef VERBOSE
			logerror("ERROR Unrecognised Command\n");
#endif
			break;
	}
}



WRITE8_HANDLER(i8271_w)
{
	switch (offset & 3)
	{
		case 0:
		{
#ifdef VERBOSE
			logerror("I8271 W Command Register: %02x\n", data);
#endif

			i8271.CommandRegister = data;
			i8271.Command = i8271.CommandRegister & 0x03f;

			i8271.StatusRegister |= I8271_STATUS_COMMAND_BUSY | I8271_STATUS_COMMAND_FULL;
			i8271.StatusRegister &= ~I8271_STATUS_PARAMETER_FULL | I8271_STATUS_RESULT_FULL;
			i8271.ParameterCountWritten = 0;

			switch (i8271.Command)
			{
				case I8271_COMMAND_SPECIFY:
				{
					FDC_LOG_COMMAND("SPECIFY");

					i8271.ParameterCount = 4;
				}
				break;

				case I8271_COMMAND_SEEK:
				{
					FDC_LOG_COMMAND("SEEK");

					i8271.ParameterCount = 1;
				}
				break;

				case I8271_COMMAND_READ_DRIVE_STATUS:
				{
					FDC_LOG_COMMAND("READ DRIVE STATUS");

					i8271.ParameterCount = 0;
				}
				break;

				case I8271_COMMAND_READ_SPECIAL_REGISTER:
				{
					FDC_LOG_COMMAND("READ SPECIAL REGISTER");

					i8271.ParameterCount = 1;
				}
				break;

				case I8271_COMMAND_WRITE_SPECIAL_REGISTER:
				{
					FDC_LOG_COMMAND("WRITE SPECIAL REGISTER");

					i8271.ParameterCount = 2;
				}
				break;

				case I8271_COMMAND_FORMAT:
				{
					i8271.ParameterCount = 5;
				}
				break;

				case I8271_COMMAND_READ_ID:
				{
					i8271.ParameterCount = 3;

				}
				break;


				case I8271_COMMAND_READ_DATA_SINGLE_RECORD:
				case I8271_COMMAND_READ_DATA_AND_DELETED_DATA_SINGLE_RECORD:
				case I8271_COMMAND_WRITE_DATA_SINGLE_RECORD:
				case I8271_COMMAND_WRITE_DELETED_DATA_SINGLE_RECORD:
				case I8271_COMMAND_VERIFY_DATA_AND_DELETED_DATA_SINGLE_RECORD:
				{
					i8271.ParameterCount = 2;
				}
				break;

				case I8271_COMMAND_READ_DATA_MULTI_RECORD:
				case I8271_COMMAND_READ_DATA_AND_DELETED_DATA_MULTI_RECORD:
				case I8271_COMMAND_WRITE_DATA_MULTI_RECORD:
				case I8271_COMMAND_WRITE_DELETED_DATA_MULTI_RECORD:
				case I8271_COMMAND_VERIFY_DATA_AND_DELETED_DATA_MULTI_RECORD:
				{
					i8271.ParameterCount = 3;
				}
				break;

				case I8271_COMMAND_SCAN_DATA:
				case I8271_COMMAND_SCAN_DATA_AND_DELETED_DATA:
				{
					i8271.ParameterCount = 5;
				}
				break;






			}

			i8271_check_all_parameters_written();
		}
		break;

		case 1:
		{
#ifdef VERBOSE
			logerror("I8271 W Parameter Register: %02x\n",data);
#endif
			i8271.ParameterRegister = data;

			if (i8271.ParameterCount!=0)
			{
				i8271.CommandParameters[i8271.ParameterCountWritten] = data;
				i8271.ParameterCountWritten++;
			}

			i8271_check_all_parameters_written();
		}
		break;

		case 2:
		{
#ifdef VERBOSE
			logerror("I8271 W Reset Register: %02x\n", data);
#endif
			if (((data ^ i8271.ResetRegister) & 0x01)!=0)
			{
				if ((data & 0x01)==0)
				{	
					i8271_reset();
				}
			}
			
			i8271.ResetRegister = data;

		
		}
		break;

		default:
			break;
	}
}

 READ8_HANDLER(i8271_r)
{
	switch (offset & 3)
	{
		case 0:
		{
			/* bit 1,0 are zero other bits contain status data */
			i8271.StatusRegister &= ~0x03;
#ifdef VERBOSE
			logerror("I8271 R Status Register: %02x\n",i8271.StatusRegister);
#endif
			return i8271.StatusRegister;
		}

		case 1:
		{

			if ((i8271.StatusRegister & I8271_STATUS_COMMAND_BUSY)==0)
			{
				/* clear IRQ */
				i8271_set_irq_state(0);

				i8271.StatusRegister &= ~I8271_STATUS_RESULT_FULL;
#ifdef VERBOSE
				logerror("I8271 R Result Register %02x\n",i8271.ResultRegister);
#endif
				return i8271.ResultRegister;
			}

			/* not useful information when command busy */
			return 0x0ff;
		}


		default:
			break;
	}

	return 0x0ff;
}


/* to be completed! */
 READ8_HANDLER(i8271_dack_r)
{
	return i8271_data_r(offset);
}

/* to be completed! */
WRITE8_HANDLER(i8271_dack_w)
{
	i8271_data_w(offset, data);
}

 READ8_HANDLER(i8271_data_r)
{
	i8271_clear_data_request();

	i8271_update_state();

  //  logerror("I8271 R data: %02x\n",i8271.data);


	return i8271.data;
}

WRITE8_HANDLER(i8271_data_w)
{
	i8271.data = data;

//    logerror("I8271 W data: %02x\n",i8271.data);

	i8271_clear_data_request();

	i8271_update_state();
}

