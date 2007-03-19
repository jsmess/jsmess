/***************************************************************************

	machine/nec765.c

	Functions to emulate a NEC765/Intel 8272 compatible floppy disk controller

	Code by Kevin Thacker.

	TODO:

    - overrun condition
	- Scan Commands
	- crc error in id field and crc error in data field errors
	- disc not present, and no sectors on track for data, deleted data, write, write deleted,
		read a track etc
        - end of cylinder condition - almost working, needs fixing  with
                PCW and PC drivers
	- resolve "ready" state stuff (ready state when reset for PC, ready state change while processing command AND
	while idle)

***************************************************************************/

#include "driver.h"
#include "machine/nec765.h"

typedef enum
{
	NEC765_COMMAND_PHASE_FIRST_BYTE,
	NEC765_COMMAND_PHASE_BYTES,
	NEC765_RESULT_PHASE,
	NEC765_EXECUTION_PHASE_READ,
	NEC765_EXECUTION_PHASE_WRITE
} NEC765_PHASE;

/* uncomment the following line for verbose information */
#define LOG_VERBOSE		1
#define LOG_COMMAND		0
#define LOG_EXTRA		0
#define LOG_INTERRUPT	0

/* uncomment this to not allow end of cylinder "error" */
#define NO_END_OF_CYLINDER



/* state of nec765 Interrupt (INT) output */
#define NEC765_INT	0x02
/* data rate for floppy discs (MFM data) */
#define NEC765_DATA_RATE	32
/* state of nec765 terminal count input*/
#define NEC765_TC	0x04

#define NEC765_DMA_MODE 0x08

#define NEC765_SEEK_OPERATION_IS_RECALIBRATE 0x01

#define NEC765_SEEK_ACTIVE 0x010
/* state of nec765 DMA DRQ output */
#define NEC765_DMA_DRQ 0x020
/* state of nec765 FDD READY input */
#define NEC765_FDD_READY 0x040

#define NEC765_RESET 0x080

typedef struct nec765
{
	unsigned long	sector_counter;
	/* version of fdc to emulate */
	NEC765_VERSION version;
	/* main status register */
	unsigned char    FDC_main;
	/* data register */
	unsigned char	nec765_data_reg;

	unsigned char c,h,r,n;

	int sector_id;

	int data_type;

	char format_data[4];

	NEC765_PHASE    nec765_phase;
	unsigned int    nec765_command_bytes[16];
	unsigned int    nec765_result_bytes[16];
	unsigned int    nec765_transfer_bytes_remaining;
	unsigned int    nec765_transfer_bytes_count;
	unsigned int    nec765_status[4];
	/* present cylinder number per drive */
	unsigned int    pcn[4];
	
	/* drive being accessed. drive outputs from fdc */
	unsigned int    drive;
	/* side being accessed: side output from fdc */
	unsigned int	side;

	
	/* step rate time in us */
	unsigned long	srt_in_ms;

	unsigned int	ncn;

//	unsigned int    nec765_id_index;
	char *execution_phase_data;
	unsigned int	nec765_flags;

//	unsigned char specify[2];
//	unsigned char perpendicular_mode[1];

	int command;

	mame_timer *seek_timer;
	mame_timer *timer;
	int timer_type;

	mame_timer *command_timer;
} NEC765;

//static void nec765_setup_data_request(unsigned char Data);
static void nec765_setup_command(void);
static void nec765_continue_command(int dummy);
static int nec765_sector_count_complete(void);
static void nec765_increment_sector(void);
static void nec765_update_state(void);
static void nec765_set_dma_drq(int state);
static void nec765_set_int(int state);

static NEC765 fdc;
static char nec765_data_buffer[32*1024];


static nec765_interface nec765_iface;


static const INT8 nec765_cmd_size[32] =
{
	1,1,9,3,2,9,9,2,1,9,2,1,9,6,1,3,
	1,9,1,1,1,1,9,1,1,9,1,1,1,9,1,1
};



static mess_image *current_image(void)
{
	mess_image *image = NULL;

	if (!nec765_iface.get_image)
	{
		if (fdc.drive < device_count(IO_FLOPPY))
			image = image_from_devtype_and_index(IO_FLOPPY, fdc.drive);
	}
	else
	{
		image = nec765_iface.get_image(fdc.drive);
	}
	return image;
}

static void nec765_setup_drive_and_side(void)
{
	/* drive index nec765 sees */
	fdc.drive = fdc.nec765_command_bytes[1] & 0x03;
	/* side index nec765 sees */
	fdc.side = (fdc.nec765_command_bytes[1]>>2) & 0x01;
}


/* setup status register 0 based on data in status register 1 and 2 */
static void nec765_setup_st0(void)
{
	/* clear completition status bits, drive bits and side bits */
	fdc.nec765_status[0] &= ~((1<<7) | (1<<6) | (1<<2) | (1<<1) | (1<<0));
	/* fill in drive */
	fdc.nec765_status[0] |= fdc.drive | (fdc.side<<2);

	/* fill in completion status bits based on bits in st0, st1, st2 */
	/* no error bits set */
	if ((fdc.nec765_status[1] | fdc.nec765_status[2])==0)
	{
		return;
	}

	fdc.nec765_status[0] |= 0x040;
}


static int nec765_n_to_bytes(int n)
{
	/* 0-> 128 bytes, 1->256 bytes, 2->512 bytes etc */
	/* data_size = ((1<<(N+7)) */
	return 1<<(n+7);
}

static void nec765_set_data_request(void)
{
	fdc.FDC_main |= 0x080;
}

static void nec765_clear_data_request(void)
{
	fdc.FDC_main &= ~0x080;
}

static void nec765_seek_complete(void)
{
	/* tested on Amstrad CPC */

	/* if a seek is done without drive connected: */
	/*  abnormal termination of command,
		seek complete, 
		not ready
	*/

	/* if a seek is done with drive connected, but disc missing: */
	/* abnormal termination of command,
		seek complete,
		not ready */

	/* if a seek is done with drive connected and disc in drive */
	/* seek complete */


	/* On the PC however, it appears that recalibrates and seeks can be performed without
	a disc in the drive. */

	/* Therefore, the above output is dependant on the state of the drive */

	/* In the Amstrad CPC, the drive select is provided by the NEC765. A single port is also
	assigned for setting the drive motor state. The motor state controls the motor of the selected
	drive */

	/* On the PC the drive can be selected with the DIGITAL OUTPUT REGISTER, and the motor of each
	of the 4 possible drives is also settable using the same register */

	/* Assumption for PC: (NOT TESTED - NEEDS VERIFICATION) */

	/* If a seek is done without drive connected: */
	/* abnormal termination of command,
		seek complete,
		fault
		*/

	/* if a seek is done with drive connected, but disc missing: */
	/* seek complete */
	
	/* if a seek is done with drive connected and disc in drive: */
	/* seek complete */

	/* On Amstrad CPC:
		If drive not connected, or drive connected but disc not in drive, not ready! 
		If drive connected and drive motor on, ready!
	   On PC:
	    Drive is always ready!

		In 37c78 docs, the ready bits of the nec765 are marked as unused.
		This indicates it is always ready!!!!!
	*/

	mess_image *img = current_image();

	if (!img)
		return;

	fdc.pcn[fdc.drive] = fdc.ncn;

	/* drive ready? */
	if (floppy_drive_get_flag_state(img, FLOPPY_DRIVE_READY))
	{
		/* yes */

		/* recalibrate? */
		if (fdc.nec765_flags & NEC765_SEEK_OPERATION_IS_RECALIBRATE)
		{
			/* yes */

			/* at track 0? */
			if (floppy_drive_get_flag_state(img, FLOPPY_DRIVE_HEAD_AT_TRACK_0))
			{
				/* yes. Seek complete */
				fdc.nec765_status[0] = 0x020;
			}
			else
			{
				/* no, track 0 failed after 77 steps */
				fdc.nec765_status[0] = 0x040 | 0x020 | 0x010;
			}
		}
		else
		{
			/* no, seek */

			/* seek complete */
			fdc.nec765_status[0] = 0x020;
		}
	}
	else
	{
		/* abnormal termination, not ready */
		fdc.nec765_status[0] = 0x040 | 0x020 | 0x08;		
	}

	/* set drive and side */
	fdc.nec765_status[0] |= fdc.drive | (fdc.side<<2);

	nec765_set_int(0);
	nec765_set_int(1);

	fdc.nec765_flags &= ~NEC765_SEEK_ACTIVE;
}

static void nec765_seek_timer_callback(int param)
{
	/* seek complete */
	nec765_seek_complete();

	timer_reset(fdc.seek_timer, TIME_NEVER);
}

static void nec765_timer_callback(int param)
{
	/* type 0 = data transfer mode in execution phase */
	if (fdc.timer_type == 0)
	{
		/* set data request */
		nec765_set_data_request();

		fdc.timer_type = 4;
		
		if (!(fdc.nec765_flags & NEC765_DMA_MODE))
		{
			/* for pcw */
			timer_reset(fdc.timer, TIME_IN_USEC(27));
		}
		else
		{
			nec765_timer_callback(fdc.timer_type);
		}
	}
	else if (fdc.timer_type==2)
	{
		/* result phase begin */

		/* generate a int for specific commands */
		switch (fdc.command) {
		case 2:		/* read a track */
		case 5:		/* write data */
		case 6:		/* read data */
		case 9:		/* write deleted data */
		case 10:	/* read id */			
		case 12:	/* read deleted data */			
		case 13:	/* format at track */
		case 17:	/* scan equal */
		case 19:	/* scan low or equal */
		case 29:	/* scan high or equal */
			nec765_set_int(1);
			break;

		default:
			break;
		}

		nec765_set_data_request();

		timer_reset(fdc.timer, TIME_NEVER);
	}
	else if (fdc.timer_type == 4)
	{
		/* if in dma mode, a int is not generated per byte. If not in  DMA mode
		a int is generated per byte */
		if (fdc.nec765_flags & NEC765_DMA_MODE)
		{
			nec765_set_dma_drq(1);
		}
		else
		{
			if (fdc.FDC_main & (1<<7))
			{
				/* set int to indicate data is ready */
				nec765_set_int(1);
			}
		}

		timer_reset(fdc.timer, TIME_NEVER);
	}
}

/* after (32-27) the DRQ is set, then 27 us later, the int is set.
I don't know if this is correct, but it is required for the PCW driver.
In this driver, the first NMI calls the handler function, furthur NMI's are
effectively disabled by reading the data before the NMI int can be set.
*/

static void nec765_setup_timed_generic(int timer_type, double duration)
{
	fdc.timer_type = timer_type;

	if (!(fdc.nec765_flags & NEC765_DMA_MODE))
	{
		timer_adjust(fdc.timer, duration, 0, 0);		
	}
	else
	{
		nec765_timer_callback(fdc.timer_type);
		timer_reset(fdc.timer, TIME_NEVER);
	}
}

/* setup data request */
static void nec765_setup_timed_data_request(int bytes)
{
	/* setup timer to trigger in NEC765_DATA_RATE us */
	nec765_setup_timed_generic(0, TIME_IN_USEC(32-27)	/*NEC765_DATA_RATE)*bytes*/);		
}

/* setup result data request */
static void nec765_setup_timed_result_data_request(void)
{
	nec765_setup_timed_generic(2, TIME_IN_USEC(NEC765_DATA_RATE)*2);
}


/* sets up a timer to issue a seek complete in signed_tracks time */
static void nec765_setup_timed_int(int signed_tracks)
{
	/* setup timer to signal after seek time is complete */
	timer_adjust(fdc.seek_timer, 0, 0, TIME_IN_MSEC(fdc.srt_in_ms*abs(signed_tracks)));
}

static void nec765_seek_setup(int is_recalibrate)
{
	mess_image *img;
	int signed_tracks;

	img = current_image();

	fdc.nec765_flags |= NEC765_SEEK_ACTIVE;

	if (is_recalibrate)
	{
		/* head cannot be specified with recalibrate */
		fdc.nec765_command_bytes[1] &=~0x04;
	}

	nec765_setup_drive_and_side();

	fdc.FDC_main |= (1<<fdc.drive);

	/* recalibrate command? */
	if (is_recalibrate)
	{
		fdc.nec765_flags |= NEC765_SEEK_OPERATION_IS_RECALIBRATE;

		fdc.ncn = 0;

		/* if drive is already at track 0, or drive is not ready */
		if (
			floppy_drive_get_flag_state(img, FLOPPY_DRIVE_HEAD_AT_TRACK_0) || 
			(!floppy_drive_get_flag_state(img, FLOPPY_DRIVE_READY))
			)
		{
			/* seek completed */
			nec765_seek_complete();
		}
		else
		{
			/* is drive present? */
			if (image_slotexists(img))
			{
				/* yes - calculate real number of tracks to seek */

				int current_track;
	
				/* get current track */
				current_track = floppy_drive_get_current_track(img);

				/* get number of tracks to seek */
				signed_tracks = -current_track;
			}
			else
			{
				/* no, seek 77 tracks and then stop */
				/* true for NEC765A, but not for other variants */
				signed_tracks = -77;
			}

			if (signed_tracks!=0)
			{
				/* perform seek - if drive isn't present it will not do anything */
				floppy_drive_seek(img, signed_tracks);
			
				nec765_setup_timed_int(signed_tracks);
			}
			else
			{
				nec765_seek_complete();
			}
		}
	}
	else
	{

		fdc.nec765_flags &= ~NEC765_SEEK_OPERATION_IS_RECALIBRATE;

		fdc.ncn = fdc.nec765_command_bytes[2];

		/* get signed tracks */
		signed_tracks = fdc.ncn - fdc.pcn[fdc.drive];

		/* if no tracks to seek, or drive is not ready, seek is complete */
		if ((signed_tracks==0) || (!floppy_drive_get_flag_state(img, FLOPPY_DRIVE_READY)))
		{
			nec765_seek_complete();
		}
		else
		{
			/* perform seek - if drive isn't present it will not do anything */
			floppy_drive_seek(img, signed_tracks);

			/* seek complete - issue an interrupt */
			nec765_setup_timed_int(signed_tracks);
		}
	}

    nec765_idle();

}



static void nec765_setup_execution_phase_read(char *ptr, int size)
{
	fdc.FDC_main |= 0x040;                     /* FDC->CPU */

	fdc.nec765_transfer_bytes_count = 0;
	fdc.nec765_transfer_bytes_remaining = size;
	fdc.execution_phase_data = ptr;
	fdc.nec765_phase = NEC765_EXECUTION_PHASE_READ;

	nec765_setup_timed_data_request(1);
}

static void nec765_setup_execution_phase_write(char *ptr, int size)
{
	fdc.FDC_main &= ~0x040;                     /* FDC->CPU */

	fdc.nec765_transfer_bytes_count = 0;
	fdc.nec765_transfer_bytes_remaining = size;
	fdc.execution_phase_data = ptr;
	fdc.nec765_phase = NEC765_EXECUTION_PHASE_WRITE;

	/* setup a data request with first byte */
	nec765_setup_timed_data_request(1);
}


static void nec765_setup_result_phase(int byte_count)
{
	fdc.FDC_main |= 0x040;                     /* FDC->CPU */
	fdc.FDC_main &= ~0x020;                    /* not execution phase */

	fdc.nec765_transfer_bytes_count = 0;
	fdc.nec765_transfer_bytes_remaining = byte_count;
	fdc.nec765_phase = NEC765_RESULT_PHASE;

	nec765_setup_timed_result_data_request();
}

void nec765_idle(void)
{
	fdc.FDC_main &= ~0x040;                     /* CPU->FDC */
	fdc.FDC_main &= ~0x020;                    /* not execution phase */
	fdc.FDC_main &= ~0x010;                     /* not busy */
	fdc.nec765_phase = NEC765_COMMAND_PHASE_FIRST_BYTE;

	nec765_set_data_request();
}



/* change flags */
static void nec765_change_flags(unsigned int flags, unsigned int mask)
{
	unsigned int new_flags;
	unsigned int changed_flags;

	assert((flags & ~mask) == 0);

	/* compute the new flags and which ones have changed */
	new_flags = fdc.nec765_flags & ~mask;
	new_flags |= flags;
	changed_flags = fdc.nec765_flags ^ new_flags;
	fdc.nec765_flags = new_flags;

	/* if interrupt changed, call the handler */
	if ((changed_flags & NEC765_INT) && nec765_iface.interrupt)
		nec765_iface.interrupt((fdc.nec765_flags & NEC765_INT) ? 1 : 0);

	/* if DRQ changed, call the handler */
	if ((changed_flags & NEC765_DMA_DRQ) && nec765_iface.dma_drq)
		nec765_iface.dma_drq((fdc.nec765_flags & NEC765_DMA_DRQ) ? 1 : 0, fdc.FDC_main & (1<<6));
}



/* set int output */
static void nec765_set_int(int state)
{
	if (LOG_INTERRUPT)
		logerror("nec765_set_int(): state=%d\n", state);
	nec765_change_flags(state ? NEC765_INT : 0, NEC765_INT);
}



/* set dma request output */
static void nec765_set_dma_drq(int state)
{
	nec765_change_flags(state ? NEC765_DMA_DRQ : 0, NEC765_DMA_DRQ);
}



/* Drive ready */

/* 

A drive will report ready if:
- drive is selected
- disc is in the drive
- disk is rotating at a constant speed (normally 300rpm)

On more modern PCs, a ready signal is not provided by the drive.
This signal is not used in the PC design and was eliminated to save costs 
If you look at the datasheets for the modern NEC765 variants, you will see the Ready
signal is not mentioned.

On the original NEC765A, ready signal is required, and some commands will fail if the drive
is not ready.




*/




/* done when ready state of drive changes */
/* this ignores if command is active, in which case command should terminate immediatly
with error */
static void nec765_set_ready_change_callback(mess_image *img, int state)
{
	int drive = image_index_in_device(img);

	logerror("nec765: ready state change\n");

	/* drive that changed state */
	fdc.nec765_status[0] = 0x0c0 | drive;

	/* not ready */
	if (state==0)
		fdc.nec765_status[0] |= 8;

	/* trigger an int */
	nec765_set_int(1);
}




void nec765_init(const nec765_interface *iface, NEC765_VERSION version)
{
	int i;

	fdc.version = version;
	fdc.timer = timer_alloc(nec765_timer_callback);
	fdc.seek_timer = timer_alloc(nec765_seek_timer_callback);
	fdc.command_timer = timer_alloc(nec765_continue_command);
	memset(&nec765_iface, 0, sizeof(nec765_interface));

	if (iface)
	{
		memcpy(&nec765_iface, iface, sizeof(nec765_interface));
	}

	fdc.nec765_flags &= NEC765_FDD_READY;

	nec765_reset(0);

	for (i = 0; i < device_count(IO_FLOPPY); i++)
		floppy_drive_set_ready_state_change_callback(image_from_devtype_and_index(IO_FLOPPY, i), nec765_set_ready_change_callback);
}


/* terminal count input */
void nec765_set_tc_state(int state)
{
	int old_state;

	old_state = fdc.nec765_flags;

	/* clear drq */
	nec765_set_dma_drq(0);

	fdc.nec765_flags &= ~NEC765_TC;
	if (state)
	{
		fdc.nec765_flags |= NEC765_TC;
	}

	/* changed state? */
	if (((fdc.nec765_flags^old_state) & NEC765_TC)!=0)
	{
		/* now set? */
		if ((fdc.nec765_flags & NEC765_TC)!=0)
		{
			/* yes */
			if (fdc.timer)
			{
				if (fdc.timer_type==0)
				{
					timer_reset(fdc.timer, TIME_NEVER);


				}
			}

#ifdef NO_END_OF_CYLINDER
			timer_adjust(fdc.command_timer, TIME_NOW, 0, 0.0);
#else
			nec765_update_state();
#endif
		}
	}
}

READ8_HANDLER(nec765_status_r)
{
	if (LOG_EXTRA)
		logerror("nec765 status r: %02x\n",fdc.FDC_main);
	return fdc.FDC_main;
}


/* control mark handling code */

/* if SK==1, and we are executing a read data command, and a deleted data mark is found,
skip it.
if SK==1, and we are executing a read deleted data command, and a data mark is found,
skip it. */

static int nec765_read_skip_sector(void)
{
	/* skip set? */
	if ((fdc.nec765_command_bytes[0] & (1<<5))!=0)
	{
		/* read data? */
		if (fdc.command == 0x06)
		{
			/* did we just find a sector with deleted data mark? */
			if (fdc.data_type == NEC765_DAM_DELETED_DATA)
			{
				/* skip it */
				return TRUE;
			}
		}
		/* deleted data? */
		else 
		if (fdc.command == 0x0c)
		{
			/* did we just find a sector with data mark ? */
			if (fdc.data_type == NEC765_DAM_DATA)
			{
				/* skip it */
				return TRUE;
			}
		}
	}

	/* do not skip */
	return FALSE;
}

/* this is much closer to how the nec765 actually gets sectors */
/* used by read data, read deleted data, write data, write deleted data */
/* What the nec765 does:

  - get next sector id from disc
  - if sector id matches id specified in command, it will
	search for next data block and read data from it.

  - if the index is seen twice while it is searching for a sector, then the sector cannot be found
*/

static void nec765_get_next_id(chrn_id *id)
{
	mess_image *img = current_image();

	/* get next id from disc */
	floppy_drive_get_next_id(img, fdc.side,id);

	fdc.sector_id = id->data_id;

	/* set correct data type */
	fdc.data_type = NEC765_DAM_DATA;
	if (id->flags & ID_FLAG_DELETED_DATA)
	{
		fdc.data_type = NEC765_DAM_DELETED_DATA;
	}
}

static int nec765_get_matching_sector(void)
{
	mess_image *img = current_image();
	chrn_id id;

	/* number of times we have seen index hole */
	int index_count = 0;

	/* get sector id's */
	do
    {
		nec765_get_next_id(&id);

		/* tested on Amstrad CPC - All bytes must match, otherwise
		a NO DATA error is reported */
		if (id.R == fdc.nec765_command_bytes[4])
		{
			if (id.C == fdc.nec765_command_bytes[2])
			{
				if (id.H == fdc.nec765_command_bytes[3])
				{
					if (id.N == fdc.nec765_command_bytes[5])
					{
						/* end of cylinder is set if:
						1. sector data is read completely (i.e. no other errors occur like
						no data.
						2. sector being read is same specified by EOT
						3. terminal count is not received */
						if (fdc.nec765_command_bytes[4]==fdc.nec765_command_bytes[6])
						{
							/* set end of cylinder */
							fdc.nec765_status[1] |= NEC765_ST1_END_OF_CYLINDER;
						}

						return TRUE;
					}
				}
			}
			else
			{
				/* the specified sector ID was found, however, the C value specified
				in the read/write command did not match the C value read from the disc */

				/* no data - checked on Amstrad CPC */
				fdc.nec765_status[1] |= NEC765_ST1_NO_DATA;
				/* bad C value */
				fdc.nec765_status[2] |= NEC765_ST2_WRONG_CYLINDER;

				if (id.C == 0x0ff)
				{
					/* the C value is 0x0ff which indicates a bad track in the IBM soft-sectored
					format */
					fdc.nec765_status[2] |= NEC765_ST2_BAD_CYLINDER;
				}

				return FALSE;
			}
		}

		 /* index set? */
		if (floppy_drive_get_flag_state(img, FLOPPY_DRIVE_INDEX))
		{
			index_count++;
		}
   
	}
	while (index_count!=2);

	/* no data - specified sector ID was not found */
    fdc.nec765_status[1] |= NEC765_ST1_NO_DATA;
  
	return 0;
}

static void nec765_read_complete(void)
{

/* causes problems!!! - need to fix */
#ifdef NO_END_OF_CYLINDER
	/* set end of cylinder */
	fdc.nec765_status[1] &= ~NEC765_ST1_END_OF_CYLINDER;
#else
	/* completed read command */

	/* end of cylinder is set when:
		- a whole sector has been read
		- terminal count input is not set
		- AND the the sector specified by EOT was read
		*/

	/* if end of cylinder is set, and we did receive a terminal count, then clear it */
	if ((fdc.nec765_flags & NEC765_TC)!=0)
	{
		/* set end of cylinder */
		fdc.nec765_status[1] &= ~NEC765_ST1_END_OF_CYLINDER;
	}
#endif

	nec765_setup_st0();

	fdc.nec765_result_bytes[0] = fdc.nec765_status[0];
	fdc.nec765_result_bytes[1] = fdc.nec765_status[1];
	fdc.nec765_result_bytes[2] = fdc.nec765_status[2];
	fdc.nec765_result_bytes[3] = fdc.nec765_command_bytes[2]; /* C */
	fdc.nec765_result_bytes[4] = fdc.nec765_command_bytes[3]; /* H */
	fdc.nec765_result_bytes[5] = fdc.nec765_command_bytes[4]; /* R */
	fdc.nec765_result_bytes[6] = fdc.nec765_command_bytes[5]; /* N */

	nec765_setup_result_phase(7);
}

static void nec765_read_data(void)
{
	mess_image *img = current_image();

	if (!(floppy_drive_get_flag_state(img, FLOPPY_DRIVE_READY)))
	{
		fdc.nec765_status[0] = 0x0c0 | (1<<4) | fdc.drive | (fdc.side<<2);
		fdc.nec765_status[1] = 0x00;
		fdc.nec765_status[2] = 0x00;

		fdc.nec765_result_bytes[0] = fdc.nec765_status[0];
		fdc.nec765_result_bytes[1] = fdc.nec765_status[1];
		fdc.nec765_result_bytes[2] = fdc.nec765_status[2];
		fdc.nec765_result_bytes[3] = fdc.nec765_command_bytes[2]; /* C */
		fdc.nec765_result_bytes[4] = fdc.nec765_command_bytes[3]; /* H */
		fdc.nec765_result_bytes[5] = fdc.nec765_command_bytes[4]; /* R */
		fdc.nec765_result_bytes[6] = fdc.nec765_command_bytes[5]; /* N */
		nec765_setup_result_phase(7);
		return;
	}

	if (LOG_VERBOSE)
		logerror("sector c: %02x h: %02x r: %02x n: %02x\n",fdc.nec765_command_bytes[2], fdc.nec765_command_bytes[3],fdc.nec765_command_bytes[4], fdc.nec765_command_bytes[5]);

	/* find a sector to read data from */
	{
		int found_sector_to_read;

		found_sector_to_read = 0;
		/* check for finished reading sectors */
		do
		{
			/* get matching sector */
			if (nec765_get_matching_sector())
			{

				/* skip it? */
				if (nec765_read_skip_sector())
				{
					/* yes */

					/* check that we haven't finished reading all sectors */
					if (nec765_sector_count_complete())
					{
						/* read complete */
						nec765_read_complete();
						return;
					}

					/* read not finished */

					/* increment sector count */
					nec765_increment_sector();
				}
				else
				{
					/* found a sector to read */
					found_sector_to_read = 1;
				}
			}
			else
			{
				/* error in finding sector */
				nec765_read_complete();
				return;
			}
		}
		while (found_sector_to_read==0);
	}	
		
	{
		int data_size;

		data_size = nec765_n_to_bytes(fdc.nec765_command_bytes[5]);

		floppy_drive_read_sector_data(img, fdc.side, fdc.sector_id,nec765_data_buffer,data_size);

        nec765_setup_execution_phase_read(nec765_data_buffer, data_size);
	}
}


static void nec765_format_track(void)
{
	mess_image *img = current_image();

	/* write protected? */
	if (floppy_drive_get_flag_state(img, FLOPPY_DRIVE_DISK_WRITE_PROTECTED))
	{
		fdc.nec765_status[1] |= NEC765_ST1_NOT_WRITEABLE;

		nec765_setup_st0();
		/* TODO: Check result is correct */
			fdc.nec765_result_bytes[0] = fdc.nec765_status[0];
            fdc.nec765_result_bytes[1] = fdc.nec765_status[1];
            fdc.nec765_result_bytes[2] = fdc.nec765_status[2];
			fdc.nec765_result_bytes[3] = fdc.format_data[0];
			fdc.nec765_result_bytes[4] = fdc.format_data[1];
			fdc.nec765_result_bytes[5] = fdc.format_data[2];
			fdc.nec765_result_bytes[6] = fdc.format_data[3];
			nec765_setup_result_phase(7);

		return;
	}

    nec765_setup_execution_phase_write(&fdc.format_data[0], 4);
}

static void     nec765_read_a_track(void)
{
	int data_size;

	/* SKIP not allowed with this command! */

	/* get next id */
	chrn_id id;

	nec765_get_next_id(&id);

	/* TO BE CONFIRMED! */
	/* check id from disc */
	if (id.C==fdc.nec765_command_bytes[2])
	{
		if (id.H==fdc.nec765_command_bytes[3])
		{
			if (id.R==fdc.nec765_command_bytes[4])
			{
				if (id.N==fdc.nec765_command_bytes[5])
				{
					/* if ID found, then no data is not set */
					/* otherwise no data will remain set */
					fdc.nec765_status[1] &=~NEC765_ST1_NO_DATA;
				}
			}
		}
	}


	data_size = nec765_n_to_bytes(id.N);
	
	floppy_drive_read_sector_data(current_image(), fdc.side, fdc.sector_id,nec765_data_buffer,data_size);

	nec765_setup_execution_phase_read(nec765_data_buffer, data_size);
}

static int nec765_just_read_last_sector_on_track(void)
{
	if (floppy_drive_get_flag_state(current_image(), FLOPPY_DRIVE_INDEX))
		return 1;
	return 0;
}

static void nec765_write_complete(void)
{

/* causes problems!!! - need to fix */
#ifdef NO_END_OF_CYLINDER
        /* set end of cylinder */
        fdc.nec765_status[1] &= ~NEC765_ST1_END_OF_CYLINDER;
#else
	/* completed read command */

	/* end of cylinder is set when:
	 - a whole sector has been read
	 - terminal count input is not set
	 - AND the the sector specified by EOT was read
	 */
	
	/* if end of cylinder is set, and we did receive a terminal count, then clear it */
	if ((fdc.nec765_flags & NEC765_TC)!=0)
	{
		/* set end of cylinder */
		fdc.nec765_status[1] &= ~NEC765_ST1_END_OF_CYLINDER;
	}
#endif

	nec765_setup_st0();

    fdc.nec765_result_bytes[0] = fdc.nec765_status[0];
    fdc.nec765_result_bytes[1] = fdc.nec765_status[1];
    fdc.nec765_result_bytes[2] = fdc.nec765_status[2];
    fdc.nec765_result_bytes[3] = fdc.nec765_command_bytes[2]; /* C */
    fdc.nec765_result_bytes[4] = fdc.nec765_command_bytes[3]; /* H */
    fdc.nec765_result_bytes[5] = fdc.nec765_command_bytes[4]; /* R */
    fdc.nec765_result_bytes[6] = fdc.nec765_command_bytes[5]; /* N */

    nec765_setup_result_phase(7);
}


static void nec765_write_data(void)
{
	if (!(floppy_drive_get_flag_state(current_image(), FLOPPY_DRIVE_READY)))
	{
		fdc.nec765_status[0] = 0x0c0 | (1<<4) | fdc.drive | (fdc.side<<2);
        fdc.nec765_status[1] = 0x00;
        fdc.nec765_status[2] = 0x00;

        fdc.nec765_result_bytes[0] = fdc.nec765_status[0];
        fdc.nec765_result_bytes[1] = fdc.nec765_status[1];
        fdc.nec765_result_bytes[2] = fdc.nec765_status[2];
        fdc.nec765_result_bytes[3] = fdc.nec765_command_bytes[2]; /* C */
        fdc.nec765_result_bytes[4] = fdc.nec765_command_bytes[3]; /* H */
        fdc.nec765_result_bytes[5] = fdc.nec765_command_bytes[4]; /* R */
        fdc.nec765_result_bytes[6] = fdc.nec765_command_bytes[5]; /* N */
		nec765_setup_result_phase(7);
		return;
	}

	/* write protected? */
	if (floppy_drive_get_flag_state(current_image(), FLOPPY_DRIVE_DISK_WRITE_PROTECTED))
	{
		fdc.nec765_status[1] |= NEC765_ST1_NOT_WRITEABLE;

		nec765_write_complete();
		return;
	}

	if (nec765_get_matching_sector())
	{
		int data_size;

		data_size = nec765_n_to_bytes(fdc.nec765_command_bytes[5]);

        nec765_setup_execution_phase_write(nec765_data_buffer, data_size);
	}
    else
    {
        nec765_setup_result_phase(7);
    }
}


/* return true if we have read all sectors, false if not */
static int nec765_sector_count_complete(void)
{
/* this is not correct?? */
#if 1
	/* if terminal count has been set - yes */
	if (fdc.nec765_flags & NEC765_TC)
	{
		/* completed */
		return 1;
	}


	
	/* multi-track? */
	if (fdc.nec765_command_bytes[0] & 0x080)
	{
		/* it appears that in multi-track mode,
		the EOT parameter of the command is ignored!? -
		or is it ignored the first time and not the next, so that
		if it is started on side 0, it will end at EOT on side 1,
		but if started on side 1 it will end at end of track????
		
		PC driver requires this to end at last sector on side 1, and
		ignore EOT parameter.
		
		To be checked!!!!
		*/

		/* if just read last sector and on side 1 - finish */
		if ((nec765_just_read_last_sector_on_track()) &&
			(fdc.side==1))
		{
			return 1;
		}

		/* if not on second side then we haven't finished yet */
		if (fdc.side!=1)
		{
			/* haven't finished yet */
			return 0;
		}
	}
	else
	{
		/* sector id == EOT? */
		if ((fdc.nec765_command_bytes[4]==fdc.nec765_command_bytes[6]))
		{

			/* completed */
			return 1;
		}
	}
#else

	/* if terminal count has been set - yes */
	if (fdc.nec765_flags & NEC765_TC)
	{
		/* completed */
		return 1;
	}
	
	/* Multi-Track operation:

	Verified on Amstrad CPC.

		disc format used:
			9 sectors per track
			2 sides
			Sector IDs: &01, &02, &03, &04, &05, &06, &07, &08, &09

		Command specified: 
			SIDE = 0,
			C = 0,H = 0,R = 1, N = 2, EOT = 1
		Sectors read:
			Sector 1 side 0
			Sector 1 side 1

		Command specified: 
			SIDE = 0,
			C = 0,H = 0,R = 1, N = 2, EOT = 3
		Sectors read:
			Sector 1 side 0
			Sector 2 side 0
			Sector 3 side 0
			Sector 1 side 1
			Sector 2 side 1
			Sector 3 side 1

			
		Command specified:
			SIDE = 0,
			C = 0, H = 0, R = 7, N = 2, EOT = 3
		Sectors read:
			Sector 7 side 0
			Sector 8 side 0
			Sector 9 side 0
			Sector 10 not found. Error "No Data"

		Command specified:
			SIDE = 1,
			C = 0, H = 1, R = 1, N = 2, EOT = 1
		Sectors read:
			Sector 1 side 1

		Command specified:
			SIDE = 1,
			C = 0, H = 1, R = 1, N = 2, EOT = 2
		Sectors read:
			Sector 1 side 1
			Sector 1 side 2

  */

	/* sector id == EOT? */
	if ((fdc.nec765_command_bytes[4]==fdc.nec765_command_bytes[6]))
	{
		/* multi-track? */
		if (fdc.nec765_command_bytes[0] & 0x080)
		{
			/* if we have reached EOT (fdc.nec765_command_bytes[6]) 
			on side 1, then read is complete */
			if (fdc.side==1)
				return 1;

			return 0;

		}

		/* completed */
		return 1;
	}
#endif
	/* not complete */
	return 0;
}

static void	nec765_increment_sector(void)
{
	/* multi-track? */
	if (fdc.nec765_command_bytes[0] & 0x080)
	{
		/* reached EOT? */
		/* if (fdc.nec765_command_bytes[4]==fdc.nec765_command_bytes[6])*/
		if (nec765_just_read_last_sector_on_track())
		{
			/* yes */

			/* reached EOT */
			/* change side to 1 */
			fdc.side = 1;
			/* reset sector id to 1 */
			fdc.nec765_command_bytes[4] = 1;
			/* set head to 1 for get next sector test */
			fdc.nec765_command_bytes[3] = 1;
		}
		else
		{
			/* increment */
			fdc.nec765_command_bytes[4]++;
		}

	}
	else
	{
		fdc.nec765_command_bytes[4]++;
	}
}

/* control mark handling code */

/* if SK==0, and we are executing a read data command, and a deleted data sector is found,
the data is not skipped. The data is read, but the control mark is set and the read is stopped */
/* if SK==0, and we are executing a read deleted data command, and a data sector is found,
the data is not skipped. The data is read, but the control mark is set and the read is stopped */
static int nec765_read_data_stop(void)
{
	/* skip not set? */
	if ((fdc.nec765_command_bytes[0] & (1<<5))==0)
	{
		/* read data? */
		if (fdc.command == 0x06)
		{
			/* did we just read a sector with deleted data? */
			if (fdc.data_type == NEC765_DAM_DELETED_DATA)
			{
				/* set control mark */
				fdc.nec765_status[2] |= NEC765_ST2_CONTROL_MARK;

				/* quit */
				return TRUE;
			}
		}
		/* deleted data? */
		else 
		if (fdc.command == 0x0c)
		{
			/* did we just read a sector with data? */
			if (fdc.data_type == NEC765_DAM_DATA)
			{
				/* set control mark */
				fdc.nec765_status[2] |= NEC765_ST2_CONTROL_MARK;

				/* quit */
				return TRUE;
			}
		}
	}

	/* continue */
	return FALSE;
}

static void nec765_continue_command(int dummy)
{
	if ((fdc.nec765_phase == NEC765_EXECUTION_PHASE_READ) ||
		(fdc.nec765_phase == NEC765_EXECUTION_PHASE_WRITE))
	{
		switch (fdc.command)
        {
			/* read a track */
			case 0x02:
			{
				fdc.sector_counter++;

				/* sector counter == EOT */
				if (fdc.sector_counter==fdc.nec765_command_bytes[6])
				{
					/* TODO: Add correct info here */

					fdc.nec765_status[1] |= NEC765_ST1_END_OF_CYLINDER;

					nec765_setup_st0();

					fdc.nec765_result_bytes[0] = fdc.nec765_status[0];
					fdc.nec765_result_bytes[1] = fdc.nec765_status[1];
					fdc.nec765_result_bytes[2] = fdc.nec765_status[2];
					fdc.nec765_result_bytes[3] = fdc.nec765_command_bytes[2]; /* C */
					fdc.nec765_result_bytes[4] = fdc.nec765_command_bytes[3]; /* H */
					fdc.nec765_result_bytes[5] = fdc.nec765_command_bytes[4]; /* R */
					fdc.nec765_result_bytes[6] = fdc.nec765_command_bytes[5]; /* N */

					nec765_setup_result_phase(7);
				}
				else
				{
					nec765_read_a_track();
				}
			}
			break;

			/* format track */
			case 0x0d:
			{
				floppy_drive_format_sector(current_image(), fdc.side, fdc.sector_counter,
					fdc.format_data[0], fdc.format_data[1],
					fdc.format_data[2], fdc.format_data[3],
					fdc.nec765_command_bytes[5]);

				fdc.sector_counter++;

				/* sector_counter = SC */
				if (fdc.sector_counter == fdc.nec765_command_bytes[3])
				{
					/* TODO: Check result is correct */
					fdc.nec765_result_bytes[0] = fdc.nec765_status[0];
					fdc.nec765_result_bytes[1] = fdc.nec765_status[1];
					fdc.nec765_result_bytes[2] = fdc.nec765_status[2];
					fdc.nec765_result_bytes[3] = fdc.format_data[0];
					fdc.nec765_result_bytes[4] = fdc.format_data[1];
					fdc.nec765_result_bytes[5] = fdc.format_data[2];
					fdc.nec765_result_bytes[6] = fdc.format_data[3];
					nec765_setup_result_phase(7);
				}
				else
				{
					nec765_format_track();
				}
			}
			break;

			/* write data, write deleted data */
			case 0x09:
			case 0x05:
			{
				/* sector id == EOT */
				UINT8 ddam;

				ddam = 0;
				if (fdc.command == 0x09)
				{
					ddam = 1;
				}

				/* write data to disc */
				floppy_drive_write_sector_data(current_image(), fdc.side, fdc.sector_id,nec765_data_buffer,nec765_n_to_bytes(fdc.nec765_command_bytes[5]),ddam);

				if (nec765_sector_count_complete())
				{
					nec765_increment_sector();
					nec765_write_complete();
				}
				else
				{
					nec765_increment_sector();
					nec765_write_data();
				}
			}
			break;

			/* read data, read deleted data */
			case 0x0c:
			case 0x06:
			{

				/* read all sectors? */

				/* sector id == EOT */
				if (nec765_sector_count_complete() || nec765_read_data_stop())
			    {
					nec765_read_complete();
				}
				else
				{	
					nec765_increment_sector();
					nec765_read_data();
				}
				}
				break;

			default:
				break;
		}
	}
}


static int nec765_get_command_byte_count(void)
{
	fdc.command = fdc.nec765_command_bytes[0] & 0x01f;

	if (fdc.version==NEC765A)
	{
		 return nec765_cmd_size[fdc.command];
    }
	else
	{
		if (fdc.version==SMC37C78)
		{
			switch (fdc.command)
			{
				/* version */
				case 0x010:
					return 1;
			
				/* verify */
				case 0x016:
					return 9;

				/* configure */
				case 0x013:
					return 4;

				/* dumpreg */
				case 0x0e:
					return 1;
			
				/* perpendicular mode */
				case 0x012:
					return 1;

				/* lock */
				case 0x014:
					return 1;
			
				/* seek/relative seek are together! */

				default:
					return nec765_cmd_size[fdc.command];
			}
		}
	}

	return nec765_cmd_size[fdc.command];
}





void nec765_update_state(void)
{
	switch (fdc.nec765_phase) {
	case NEC765_RESULT_PHASE:
		/* set data reg */
		fdc.nec765_data_reg = fdc.nec765_result_bytes[fdc.nec765_transfer_bytes_count];

		if (fdc.nec765_transfer_bytes_count == 0)
		{
			/* clear int for specific commands */
			switch (fdc.command) {
			case 2:	/* read a track */
			case 5:	/* write data */
			case 6:	/* read data */
			case 9:	/* write deleted data */
			case 10:	/* read id */
			case 12:	/* read deleted data */
			case 13:	/* format at track */
			case 17:	/* scan equal */
			case 19:	/* scan low or equal */
			case 29:	/* scan high or equal */
				nec765_set_int(0);
				break;

			default:
				break;
			}
		}

		if (LOG_VERBOSE)
			logerror("NEC765: RESULT: %02x\n", fdc.nec765_data_reg);

		fdc.nec765_transfer_bytes_count++;
		fdc.nec765_transfer_bytes_remaining--;

		if (fdc.nec765_transfer_bytes_remaining==0)
		{
			nec765_idle();
		}
		else
		{
			nec765_set_data_request();
		}
		break;

	case NEC765_EXECUTION_PHASE_READ:
		/* setup data register */
		fdc.nec765_data_reg = fdc.execution_phase_data[fdc.nec765_transfer_bytes_count];
		fdc.nec765_transfer_bytes_count++;
		fdc.nec765_transfer_bytes_remaining--;

		if (LOG_EXTRA)
			logerror("EXECUTION PHASE READ: %02x\n", fdc.nec765_data_reg);

		if ((fdc.nec765_transfer_bytes_remaining==0) || (fdc.nec765_flags & NEC765_TC))
		{
			timer_adjust(fdc.command_timer, TIME_NOW, 0, 0.0);
		}
		else
		{
			/* trigger int */
			nec765_setup_timed_data_request(1);
		}
		break;

	case NEC765_COMMAND_PHASE_FIRST_BYTE:
		fdc.FDC_main |= 0x10;                      /* set BUSY */

		if (LOG_VERBOSE)
			logerror("nec765(): pc=0x%08x command=0x%02x\n", activecpu_get_pc(), fdc.nec765_data_reg);

		/* seek in progress? */
		if (fdc.nec765_flags & NEC765_SEEK_ACTIVE)
		{
			/* any command results in a invalid - I think that seek, recalibrate and
			sense interrupt status may work*/
			fdc.nec765_data_reg = 0;
		}

		fdc.nec765_command_bytes[0] = fdc.nec765_data_reg;

		fdc.nec765_transfer_bytes_remaining = nec765_get_command_byte_count();

		fdc.nec765_transfer_bytes_count = 1;
		fdc.nec765_transfer_bytes_remaining--;

		if (fdc.nec765_transfer_bytes_remaining==0)
		{
			nec765_setup_command();
		}
		else
		{
			/* request more data */
			nec765_set_data_request();
			fdc.nec765_phase = NEC765_COMMAND_PHASE_BYTES;
		}
        break;

    case NEC765_COMMAND_PHASE_BYTES:
		if (LOG_VERBOSE)
			logerror("nec765(): pc=0x%08x command=0x%02x\n", activecpu_get_pc(), fdc.nec765_data_reg);

		fdc.nec765_command_bytes[fdc.nec765_transfer_bytes_count] = fdc.nec765_data_reg;
		fdc.nec765_transfer_bytes_count++;
		fdc.nec765_transfer_bytes_remaining--;

		if (fdc.nec765_transfer_bytes_remaining==0)
		{
			nec765_setup_command();
		}
		else
		{
			/* request more data */
			nec765_set_data_request();
		}
		break;

    case NEC765_EXECUTION_PHASE_WRITE:
		fdc.execution_phase_data[fdc.nec765_transfer_bytes_count]=fdc.nec765_data_reg;
		fdc.nec765_transfer_bytes_count++;
		fdc.nec765_transfer_bytes_remaining--;

		if ((fdc.nec765_transfer_bytes_remaining == 0) || (fdc.nec765_flags & NEC765_TC))
		{
			timer_adjust(fdc.command_timer, TIME_NOW, 0, 0.0);
		}
		else
		{
			nec765_setup_timed_data_request(1);
		}
		break;
	}
}


 READ8_HANDLER(nec765_data_r)
{
	if ((fdc.FDC_main & 0x0c0) == 0x0c0)
	{
		if (
			(fdc.nec765_phase == NEC765_EXECUTION_PHASE_READ) ||
			(fdc.nec765_phase == NEC765_EXECUTION_PHASE_WRITE))
		{

			/* reading the data byte clears the interrupt */
			nec765_set_int(CLEAR_LINE);
		}

		/* reset data request */
		nec765_clear_data_request();

		/* update state */
		nec765_update_state();
	}

	if (LOG_EXTRA)
		logerror("DATA R: %02x\n", fdc.nec765_data_reg);

	return fdc.nec765_data_reg;
}

WRITE8_HANDLER(nec765_data_w)
{
	if (LOG_EXTRA)
		logerror("DATA W: %02x\n", data);

	/* write data to data reg */
	fdc.nec765_data_reg = data;

	if ((fdc.FDC_main & 0x0c0)==0x080)
	{
		if (
			(fdc.nec765_phase == NEC765_EXECUTION_PHASE_READ) ||
			(fdc.nec765_phase == NEC765_EXECUTION_PHASE_WRITE))
		{

			/* reading the data byte clears the interrupt */
			nec765_set_int(CLEAR_LINE);
		}

		/* reset data request */
		nec765_clear_data_request();

		/* update state */
		nec765_update_state();
	}
}

static void nec765_setup_invalid(void)
{
	fdc.command = 0;
	fdc.nec765_result_bytes[0] = 0x080;
	nec765_setup_result_phase(1);
}

static void nec765_setup_command(void)
{
	static const char *commands[] =
	{
		NULL,						/* [00] */
		NULL,						/* [01] */
		"Read Track",				/* [02] */
		"Specify",					/* [03] */
		"Sense Drive Status",		/* [04] */
		"Write Data",				/* [05] */
		"Read Data",				/* [06] */
		"Recalibrate",				/* [07] */
		"Sense Interrupt Status",	/* [08] */
		"Write Deleted Data",		/* [09] */
		"Read ID",					/* [0A] */
		NULL,						/* [0B] */
		"Read Deleted Data",		/* [0C] */
		"Format Track",				/* [0D] */
		"Dump Registers",			/* [0E] */
		"Seek",						/* [0F] */
		"Version",					/* [10] */
		NULL,						/* [11] */
		"Perpendicular Mode",		/* [12] */
		"Configure",				/* [13] */
		"Lock"						/* [14] */
	};

	mess_image *img = current_image();
	const char *cmd = NULL;
	chrn_id id;

	/* if not in dma mode set execution phase bit */
	if (!(fdc.nec765_flags & NEC765_DMA_MODE))
	{
        fdc.FDC_main |= 0x020;              /* execution phase */
	}

	if (LOG_COMMAND)
	{
		if ((fdc.nec765_command_bytes[0] & 0x1f) < sizeof(commands) / sizeof(commands[0]))
			cmd = commands[fdc.nec765_command_bytes[0] & 0x1f];
		logerror("nec765_setup_command(): Setting up command 0x%02X (%s)\n",
			fdc.nec765_command_bytes[0] & 0x1f, cmd ? cmd : "???");
	}

	switch (fdc.nec765_command_bytes[0] & 0x1f)
	{
		case 0x03:      /* specify */
			/* setup step rate */
			fdc.srt_in_ms = 16-((fdc.nec765_command_bytes[1]>>4) & 0x0f);

			fdc.nec765_flags &= ~NEC765_DMA_MODE;

			if ((fdc.nec765_command_bytes[2] & 0x01)==0)
			{
				fdc.nec765_flags |= NEC765_DMA_MODE;
			}

			nec765_idle();
			break;

		case 0x04:  /* sense drive status */
			nec765_setup_drive_and_side();

			fdc.nec765_status[3] = fdc.drive | (fdc.side<<2);

			if (floppy_drive_get_flag_state(img, FLOPPY_DRIVE_DISK_WRITE_PROTECTED))
			{
				fdc.nec765_status[3] |= 0x40;
			}

			if (floppy_drive_get_flag_state(img, FLOPPY_DRIVE_READY))
			{
				fdc.nec765_status[3] |= 0x20;
			}

			if (floppy_drive_get_flag_state(img, FLOPPY_DRIVE_HEAD_AT_TRACK_0))
			{
				fdc.nec765_status[3] |= 0x10;
			}

			fdc.nec765_status[3] |= 0x08;
	                               
			/* two side and fault not set but should be? */
			fdc.nec765_result_bytes[0] = fdc.nec765_status[3];
			nec765_setup_result_phase(1);
			break;

		case 0x07:          /* recalibrate */
			nec765_seek_setup(1);
			break;

		case 0x0f:          /* seek */
			nec765_seek_setup(0);
			break;

		case 0x0a:      /* read id */
			nec765_setup_drive_and_side();

			fdc.nec765_status[0] = fdc.drive | (fdc.side<<2);
			fdc.nec765_status[1] = 0;
			fdc.nec765_status[2] = 0;

			/* drive ready? */
			if (floppy_drive_get_flag_state(img, FLOPPY_DRIVE_READY))
			{
				/* is disk inserted? */
				if (image_exists(img))
				{
					int index_count = 0;

					/* floppy drive is ready and disc is inserted */

					/* this is the id that appears when a disc is not formatted */
					/* to be checked on Amstrad */
					id.C = 0;
					id.H = 0;
					id.R = 0x01;
					id.N = 0x02;

					/* repeat for two index counts before quitting */
					do
					{
						/* get next id from disc */
						if (floppy_drive_get_next_id(img, fdc.side, &id))
						{
							/* got an id - quit */
							break;
						}

						if (floppy_drive_get_flag_state(img, FLOPPY_DRIVE_INDEX))
						{
							/* update index count */
							index_count++;
						}
					}
					while (index_count!=2);
						
					/* at this point, we have seen a id or two index pulses have occured! */
					fdc.nec765_result_bytes[0] = fdc.nec765_status[0];
					fdc.nec765_result_bytes[1] = fdc.nec765_status[1];
					fdc.nec765_result_bytes[2] = fdc.nec765_status[2];
					fdc.nec765_result_bytes[3] = id.C; /* C */
					fdc.nec765_result_bytes[4] = id.H; /* H */
					fdc.nec765_result_bytes[5] = id.R; /* R */
					fdc.nec765_result_bytes[6] = id.N; /* N */

					nec765_setup_result_phase(7);
				}
				else
				{
					/* floppy drive is ready, but no disc is inserted */
					/* this occurs on the PC */
					/* in this case, the command never quits! */
					/* there are no index pulses to stop the command! */
				}
			}
			else
			{
				/* what are id values when drive not ready? */

				/* not ready, abnormal termination */
				fdc.nec765_status[0] |= (1<<3) | (1<<6);
				fdc.nec765_result_bytes[0] = fdc.nec765_status[0];
				fdc.nec765_result_bytes[1] = fdc.nec765_status[1];
				fdc.nec765_result_bytes[2] = fdc.nec765_status[2];
				fdc.nec765_result_bytes[3] = 0; /* C */
				fdc.nec765_result_bytes[4] = 0; /* H */
				fdc.nec765_result_bytes[5] = 0; /* R */
				fdc.nec765_result_bytes[6] = 0; /* N */
			}
			break;


		case 0x08: /* sense interrupt status */
			/* interrupt pending? */
			if (fdc.nec765_flags & NEC765_INT)
			{
				/* yes. Clear int */
				nec765_set_int(CLEAR_LINE);

				/* clear drive seek bits */
				fdc.FDC_main &= ~(1 | 2 | 4 | 8);

				/* return status */
				fdc.nec765_result_bytes[0] = fdc.nec765_status[0];
				/* return pcn */
				fdc.nec765_result_bytes[1] = fdc.pcn[fdc.drive];

				/* return result */
				nec765_setup_result_phase(2);
			}
			else
			{
				/* no int */
				fdc.nec765_result_bytes[0] = 0x80;
				/* return pcn */
				fdc.nec765_result_bytes[1] = fdc.pcn[fdc.drive];

				/* return result */
				nec765_setup_result_phase(2);
			}
			break;

		case 0x06:  /* read data */
			nec765_setup_drive_and_side();

			fdc.nec765_status[0] = fdc.drive | (fdc.side<<2);
			fdc.nec765_status[1] = 0;
			fdc.nec765_status[2] = 0;

			nec765_read_data();
			break;

		case 0x0c:
			/* read deleted data */
			nec765_setup_drive_and_side();

			fdc.nec765_status[0] = fdc.drive | (fdc.side<<2);
			fdc.nec765_status[1] = 0;
			fdc.nec765_status[2] = 0;

			/* .. for now */
			nec765_read_data();
			break;

		case 0x09:
			/* write deleted data */
			nec765_setup_drive_and_side();

			fdc.nec765_status[0] = fdc.drive | (fdc.side<<2);
			fdc.nec765_status[1] = 0;
			fdc.nec765_status[2] = 0;

			/* ... for now */
			nec765_write_data();
			break;

		case 0x02:
			/* read a track */
			nec765_setup_drive_and_side();

			fdc.nec765_status[0] = fdc.drive | (fdc.side<<2);
			fdc.nec765_status[1] = 0;
			fdc.nec765_status[2] = 0;

			fdc.nec765_status[0] |= NEC765_ST1_NO_DATA;

			/* wait for index */
			do
			{
				/* get next id from disc */
				floppy_drive_get_next_id(img, fdc.side,&id);
			}
			while ((floppy_drive_get_flag_state(img, FLOPPY_DRIVE_INDEX))==0);

			fdc.sector_counter = 0;

			nec765_read_a_track();
			break;

		case 0x05:  /* write data */
			nec765_setup_drive_and_side();

			fdc.nec765_status[0] = fdc.drive | (fdc.side<<2);
			fdc.nec765_status[1] = 0;
			fdc.nec765_status[2] = 0;

			nec765_write_data();
			break;

		case 0x0d:	/* format a track */
			nec765_setup_drive_and_side();

			fdc.nec765_status[0] = fdc.drive | (fdc.side<<2);
			fdc.nec765_status[1] = 0;
			fdc.nec765_status[2] = 0;

			fdc.sector_counter = 0;

			nec765_format_track();
			break;

		default:	/* invalid */
			switch (fdc.version)
			{
				case NEC765A:
					nec765_setup_invalid();
					break;

				case NEC765B:
					/* from nec765b data sheet */
					if ((fdc.nec765_command_bytes[0] & 0x01f)==0x010)
					{
						/* version */
						fdc.nec765_status[0] = 0x090;
						fdc.nec765_result_bytes[0] = fdc.nec765_status[0];
						nec765_setup_result_phase(1);
					}
					break;

				case SMC37C78:
					/* TO BE COMPLETED!!! !*/
					switch (fdc.nec765_command_bytes[0] & 0x1f)
					{
						case 0x10:		/* version */
							fdc.nec765_status[0] = 0x90;
							fdc.nec765_result_bytes[0] = fdc.nec765_status[0];
							nec765_setup_result_phase(1);
							break;

						case 0x13:		/* configure */
							nec765_idle();
							break;
							
						case 0x0e:		/* dump reg */
							fdc.nec765_result_bytes[0] = fdc.pcn[0];
							fdc.nec765_result_bytes[1] = fdc.pcn[1];
							fdc.nec765_result_bytes[2] = fdc.pcn[2];
							fdc.nec765_result_bytes[3] = fdc.pcn[3];
								
							nec765_setup_result_phase(10);
							break;

						case 0x12:		/* perpendicular mode */
							nec765_idle();
							break;

						case 0x14:		/* lock */
							nec765_setup_result_phase(1);
							break;
					}
					break;
			}
	}
}


/* dma acknowledge write */
WRITE8_HANDLER(nec765_dack_w)
{
	/* clear request */
	nec765_set_dma_drq(CLEAR_LINE);

	/* write data */
	nec765_data_w(offset, data);
}

 READ8_HANDLER(nec765_dack_r)
{
	/* clear data request */
	nec765_set_dma_drq(CLEAR_LINE);
	
	/* read data */
	return nec765_data_r(offset);	
}


void nec765_reset(int offset)
{
	/* nec765 in idle state - ready to accept commands */
	nec765_idle();

	/* set int low */
	nec765_set_int(0);
	/* set dma drq output */
	nec765_set_dma_drq(0);

	/* tandy 100hx assumes that after NEC is reset, it is in DMA mode */
	fdc.nec765_flags |= NEC765_DMA_MODE;

	/* if ready input is set during reset generate an int */
	if (fdc.nec765_flags & NEC765_FDD_READY)
	{
		int i;
		int a_drive_is_ready;

		fdc.nec765_status[0] = 0x080 | 0x040;
	
		/* for the purpose of pc-xt. If any of the drives have a disk inserted,
		do not set not-ready - need to check with pc_fdc.c whether all drives
		are checked or only the drive selected with the drive select bits?? */

		a_drive_is_ready = 0;
		for (i = 0; i < device_count(IO_FLOPPY); i++)
		{
			if (image_exists(image_from_devtype_and_index(IO_FLOPPY, i)))
			{
				a_drive_is_ready = 1;
				break;
			}

		}

		if (!a_drive_is_ready)
		{
			fdc.nec765_status[0] |= 0x08;
		}

		nec765_set_int(1);	
	}
}

void nec765_set_reset_state(int state)
{
	int flags;

	/* get previous reset state */
	flags = fdc.nec765_flags;

	/* set new reset state */
	/* clear reset */
	fdc.nec765_flags &= ~NEC765_RESET;

	/* reset */
	if (state)
	{
		fdc.nec765_flags |= NEC765_RESET;

		nec765_set_int(0);
	}

	/* reset changed state? */
	if (((flags^fdc.nec765_flags) & NEC765_RESET)!=0)
	{
		/* yes */

		/* no longer reset */
		if ((fdc.nec765_flags & NEC765_RESET)==0)
		{
			/* reset nec */
			nec765_reset(0);
		}
	}
}


void	nec765_set_ready_state(int state)
{
	/* clear ready state */
	fdc.nec765_flags &= ~NEC765_FDD_READY;

	if (state)
	{
		fdc.nec765_flags |= NEC765_FDD_READY;
	}
}
