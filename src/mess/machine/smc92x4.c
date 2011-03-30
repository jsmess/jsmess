/*
    HDC9224 and HDC9234 Hard and Floppy Disk Controller (HFDC)

    This controller handles MFM and FM encoded floppy disks and hard disks.
    The SMC9224 is used in some DEC systems.  The HDC9234 is used in the
    Myarc HFDC card for the TI99/4a.  The main difference between the two
    chips is the way the ECC bytes are computed; there are differences in
    the way seek times are computed, too.

    References:
    * SMC HDC9234 preliminary data book (1988)

    Michael Zapf, April 2010

    First version by Raphael Nabet, 2003
*/

#include "emu.h"
#include "imageutl.h"
#include "imagedev/flopdrv.h"
#include "imagedev/harddriv.h"
#include "harddisk.h"

#include "smc92x4.h"

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/*
    Definition of bits in the status register
*/
#define ST_INTPEND	0x80		/* interrupt pending */
#define ST_DMAREQ	0x40		/* DMA request */
#define ST_DONE		0x20		/* command done */
#define ST_TERMCOD	0x18		/* termination code (see below) */
#define ST_RDYCHNG	0x04		/* ready change */
#define ST_OVRUN	0x02		/* overrun/underrun */
#define ST_BADSECT	0x01		/* bad sector */

/*
    Definition of the termination codes (INT_STATUS)
*/
#define ST_TC_SUCCESS	0x00	/* Successful completion */
#define ST_TC_RDIDERR	0x08	/* Error in READ-ID sequence */
#define ST_TC_SEEKERR	0x10	/* Error in SEEK sequence */
#define ST_TC_DATAERR	0x18	/* Error in DATA-TRANSFER seq. */


/*
    Definition of bits in the Termination-Conditions register
*/
#define TC_CRCPRE	0x80		/* CRC register preset, must be 1 */
#define TC_UNUSED	0x40		/* bit 6 is not used and must be 0 */
#define TC_INTDONE	0x20		/* interrupt on done */
#define TC_TDELDAT	0x10		/* terminate on deleted data */
#define TC_TDSTAT3	0x08		/* terminate on drive status 3 change */
#define TC_TWPROT	0x04		/* terminate on write-protect (FDD only) */
#define TC_INTRDCH	0x02		/* interrupt on ready change (FDD only) */
#define TC_TWRFLT	0x01		/* interrupt on write-fault (HDD only) */

/*
    Definition of bits in the Chip-Status register
*/
#define CS_RETREQ	0x80		/* retry required */
#define CS_ECCATT	0x40		/* ECC correction attempted */
#define CS_CRCERR	0x20		/* ECC/CRC error */
#define CS_DELDATA	0x10		/* deleted data mark */
#define CS_SYNCERR	0x08		/* synchronization error */
#define CS_COMPERR	0x04		/* compare error */
#define CS_PRESDRV	0x03		/* present drive selected */

/*
    Definition of bits in the Mode register
*/
#define MO_TYPE		0x80		/* Hard disk (1) or floppy (0) */
#define MO_CRCECC	0x60		/* Values for CRC/ECC handling */
#define MO_DENSITY	0x10		/* FM = 1; MFM = 0 */
#define MO_UNUSED	0x08		/* Unused, 0 */
#define MO_STEPRATE	0x07		/* Step rates */

/*
    hfdc state structure

    status
    ab7     ecc error
    ab6     index pulse
    ab5     seek complete
    ab4     track 0
    ab3     user-defined
    ab2     write-protected
    ab1     drive ready
    ab0     write fault

    output1
    ab7     drive select 3
    ab6     drive select 2
    ab5     drive select 1
    ab4     drive select 0
    ab3     programmable outputs
    ab2     programmable outputs
    ab1     programmable outputs
    ab0     programmable outputs

    output2
    ab7     drive select 3* (active low, used for tape operations)
    ab6     reduce write current
    ab5     step direction
    ab4     step pulse
    ab3     desired head 3
    ab2     desired head 2
    ab1     desired head 1
    ab0     desired head 0
*/

#define OUT1_DRVSEL3	0x80
#define OUT1_DRVSEL2	0x40
#define OUT1_DRVSEL1	0x20
#define OUT1_DRVSEL0	0x10
#define OUT2_DRVSEL3_	0x80
#define OUT2_REDWRT	0x40
#define OUT2_STEPDIR	0x20
#define OUT2_STEPPULSE	0x10
#define OUT2_HEADSEL3	0x08
#define OUT2_HEADSEL2	0x04
#define OUT2_HEADSEL1	0x02
#define OUT2_HEADSEL0	0x01

#define DRIVE_TYPE	0x03
#define TYPE_AT 	0x00
#define TYPE_FLOPPY 	0x02  /* for testing on any floppy type */
#define TYPE_FLOPPY8	0x02
#define TYPE_FLOPPY5	0x03

#define MAX_SECTOR_LEN 256

#define FORMAT_LONG FALSE

#define ERROR	0
#define DONE	1
#define AGAIN	2
#define UNDEF	3

typedef struct _smc92x4_state
{
	devcb_resolved_write_line	out_intrq_func;		/* INT line */
	devcb_resolved_write_line	out_dip_func;		/* DMA in progress line */
	devcb_resolved_write8		out_auxbus_func;	/* AB0-7 lines (using S0,S1 as address) */
	devcb_resolved_read_line	in_auxbus_func;		/* AB0-7 lines (only single case) */

	UINT8 output1;		/* UDC-internal register "output1" */
	UINT8 output2;		/* UDC-internal register "output2" */

	int	selected_drive_type;
	// We need to store the types, although this is not the case in the
	// real hardware. The reason is that poll_drives wants to select the
	// drives without knowing their kinds. In reality this does not matter,
	// since we only check for the seek_complete line, but in our case,
	// floppy and harddrive are pretty different, and we need to know which
	// one to check.
	int	types[4];
	int	head_load_delay_enable;
	int	register_pointer;

	UINT8 register_r[12];
	UINT8 register_w[12];

	/* timers to delay execution/completion of commands */
	emu_timer *timer_data, *timer_rs, *timer_ws, *timer_seek, *timer_track;

	/* Flag which determines whether to use realistic timing. */
	int use_real_timing;

	/* Required to store the current iteration within restore. */
	int seek_count;

	/* Recent command. */
	UINT8 command;

	/* Stores the step direction. */
	int step_direction;

	/* Stores the buffered flag. */
	int buffered;

	/* Stores the recent id field. */
	chrn_id_hd recent_id;

	/* Indicates whether an ID has been found. */
	int found_id;

	/* Indicates whether we are already past the seek_id phase in read/write sector. */
	int after_seek;

	/* Determines whether the command will be continued. */
	int to_be_continued;

	/* Pointer to interface */
	const smc92x4_interface *intf;

} smc92x4_state;

/*
    Step rates in microseconds for MFM. This is set in the mode register,
    bits 0-2. Single density doubles all values.
*/
static const int step_hd[]	= { 22, 50, 100, 200, 400, 800, 1600, 3200 };
static const int step_flop8[]	= { 218, 500, 1000, 2000, 4000, 8000, 16000, 32000 };
static const int step_flop5[]	= { 436, 1000, 2000, 4000, 8000, 16000, 32000, 64000 };

/*
    0.2 seconds for a revolution (300 rpm), +50% average waiting for index
    hole. Should be properly done with the index hole detection; we're
    simulating this with a timer for now.
*/
#define TRACKTIME_FLOPPY 300000
#define TRACKTIME_HD 1000

/*
    Register names of the HDC. The left part is the set of write registers,
    while the right part are the read registers.
*/
enum
{
	DMA7_0=0,
	DMA15_8=1,
	DMA23_16=2,
	DESIRED_SECTOR=3,
	DESIRED_HEAD=4, 	CURRENT_HEAD=4,
	DESIRED_CYLINDER=5, 	CURRENT_CYLINDER=5,
	SECTOR_COUNT=6, 	CURRENT_IDENT=6,
	RETRY_COUNT=7,		TEMP_STORAGE2=7,
	MODE=8, 		CHIP_STATUS=8,
	INT_COMM_TERM=9,	DRIVE_STATUS=9,
	DATA_DELAY=10,		DATA=10,
	COMMAND=11, 		INT_STATUS=11
};

#define TRKSIZE_DD		6144
#define TRKSIZE_SD		3172

/***************************************************************************
    PROTOTYPES
***************************************************************************/

static void smc92x4_process_after_callback(device_t *device);
static void data_transfer_read(device_t *device, chrn_id_hd id, int transfer_enable);
static void data_transfer_write(device_t *device, chrn_id_hd id, int deldata, int redcur, int precomp, int write_long);

#if 0
static void dump_contents(UINT8 *buffer, int length)
{
	for (int i=0; i < length; i+=16)
	{
		for (int j=0; j < 16; j+=2)
		{
			printf("%02x%02x ", buffer[i+j], buffer[i+j+1]);
		}
		printf("\n");
	}
}
#endif

INLINE smc92x4_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	return (smc92x4_state *)downcast<legacy_device_base *>(device)->token();
}


static int image_is_single_density(device_t *current_floppy)
{
	floppy_image *image = flopimg_get_image(current_floppy);
	return (floppy_get_track_size(image, 0, 0)<4000);
}

static int controller_set_to_single_density(device_t *device)
{
	smc92x4_state *w = get_safe_token(device);
	return ((w->register_w[MODE]&MO_DENSITY)!=0);
}

static void copyid(chrn_id id1, chrn_id_hd *id2)
{
	id2->C = id1.C & 0xff;
	id2->H = id1.H;
	id2->R = id1.R;
	id2->N = id1.N;
	id2->data_id = id1.data_id;
	id2->flags = id1.flags;
}

/*
    Set IRQ
*/
static void smc92x4_set_interrupt(device_t *device)
{
	smc92x4_state *w = get_safe_token(device);
	if ((w->register_r[INT_STATUS] & ST_INTPEND) == 0)
	{
		w->register_r[INT_STATUS] |= ST_INTPEND;
		devcb_call_write_line(&w->out_intrq_func, ASSERT_LINE);
	}
}

/*
    Set DMA in progress
*/
static void smc92x4_set_dip(device_t *device, int value)
{
	smc92x4_state *w = get_safe_token(device);
	devcb_call_write_line(&w->out_dip_func, value);
}

/*
    Assert Command Done status bit, triggering interrupts as needed
*/
static void set_command_done(device_t *device, int flags)
{
	//assert(! (w->status & ST_DONE))
	smc92x4_state *w = get_safe_token(device);
	//logerror("smc92x4 command %02x done, flags=%02x\n", w->command, flags);

	w->register_r[INT_STATUS] |= ST_DONE;
	w->register_r[INT_STATUS] &= ~ST_TERMCOD; /* clear the previously set flags */
	w->register_r[INT_STATUS] |= flags;

	/* sm92x4 spec, p. 6 */
	if (w->register_w[INT_COMM_TERM] & TC_INTDONE)
		smc92x4_set_interrupt(device);
}

/*
    Preserve previously set termination code
*/
static void set_command_done(device_t *device)
{
	//assert(! (w->status & ST_DONE))
	smc92x4_state *w = get_safe_token(device);
	//logerror("smc92x4 command %02x done\n", w->command);

	w->register_r[INT_STATUS] |= ST_DONE;

	/* sm92x4 spec, p. 6 */
	if (w->register_w[INT_COMM_TERM] & TC_INTDONE)
		smc92x4_set_interrupt(device);
}

/*
    Clear IRQ
*/
static void smc92x4_clear_interrupt(device_t *device)
{
	smc92x4_state *w = get_safe_token(device);
	if ((w->register_r[INT_STATUS] & ST_INTPEND) != 0)
	{
		devcb_call_write_line(&w->out_intrq_func, CLEAR_LINE);
	}
}

/*
    Sets the DMA address on the external counter. This counter is attached
    to the auxiliary bus on the PCB.
*/
static void set_dma_address(device_t *device, int pos2316, int pos1508, int pos0700)
{
	smc92x4_state *w = get_safe_token(device);
	devcb_call_write8(&w->out_auxbus_func, OUTPUT_DMA_ADDR, w->register_r[pos2316]);
	devcb_call_write8(&w->out_auxbus_func, OUTPUT_DMA_ADDR, w->register_r[pos1508]);
	devcb_call_write8(&w->out_auxbus_func, OUTPUT_DMA_ADDR, w->register_r[pos0700]);
}

static void dma_add_offset(device_t *device, int offset)
{
	smc92x4_state *w = get_safe_token(device);
	int dma_address = (w->register_w[DMA23_16]<<16) + (w->register_w[DMA15_8]<<8) + w->register_w[DMA7_0];
	dma_address += offset;

	w->register_w[DMA23_16] = w->register_r[DMA23_16] = (dma_address & 0xff0000)>>16;
	w->register_w[DMA15_8]  = w->register_r[DMA15_8]  = (dma_address & 0x00ff00)>>8;
	w->register_w[DMA7_0]   = w->register_r[DMA7_0]   = (dma_address & 0x0000ff);
}

/*
    Get the state from outside and latch it in the register.
    There should be a bus driver on the PCB which provides the signals from
    both the hard and floppy drives during S0=S1=0 and STB*=0 times via the
    auxiliary bus.
*/
static void sync_status_in(device_t *device)
{
	UINT8 prev;
	smc92x4_state *w = get_safe_token(device);

	prev = w->register_r[DRIVE_STATUS];
	w->register_r[DRIVE_STATUS] = devcb_call_read_line(&w->in_auxbus_func);

	/* Raise interrupt if ready changes. TODO: Check this more closely. */
//  logerror("disk status = %02x\n", reply);
	if (((w->register_r[DRIVE_STATUS] & DS_READY) != (prev & DS_READY))
		& (w->register_r[INT_STATUS] & ST_RDYCHNG))
	{
		smc92x4_set_interrupt(device);
	}
}

/*
    Push the output registers over the auxiliary bus. It is expected that
    the PCB contains latches to store the values.
*/
static void sync_latches_out(device_t *device)
{
	smc92x4_state *w = get_safe_token(device);
	w->output1 = (w->output1 & 0xf0) | (w->register_w[RETRY_COUNT]&0x0f);

	devcb_call_write8(&w->out_auxbus_func, OUTPUT_OUTPUT1, w->output1);
	devcb_call_write8(&w->out_auxbus_func, OUTPUT_OUTPUT2, w->output2);
}

/*************************************************************
    Timed requests and callbacks
*************************************************************/
#if 0
/* setup a timed data request - data request will be triggered in a few usecs time */
static void smc92x4_timed_data_request(device_t *device)
{
	smc92x4_state *w = get_safe_token(device);
	int time = controller_set_to_single_density(device)? 128 : 32;

	if (!w->use_real_timing)
		time = 1;

	/* set new timer */
	w->timer_data->adjust(attotime::from_usec(time));
}
#endif

/* setup a timed read sector - read sector will be triggered in a few usecs time */
static void smc92x4_timed_sector_read_request(device_t *device)
{
	smc92x4_state *w = get_safe_token(device);
	int time=0;

	/* set new timer */
	/* Average time from sector to sector. */
	if (w->selected_drive_type & TYPE_FLOPPY)
		time = (controller_set_to_single_density(device))? 30000 : 15000;
	else
		time = 1000;

	if (!w->use_real_timing)
		time = 1;

	w->timer_rs->adjust(attotime::from_usec(time));
	w->to_be_continued = TRUE;
}

/* setup a timed write sector - write sector will be triggered in a few usecs time */
static void smc92x4_timed_sector_write_request(device_t *device)
{
	smc92x4_state *w = get_safe_token(device);
	int time=0;

	/* Average time from sector to sector. */
	if (w->selected_drive_type & TYPE_FLOPPY)
		time = (controller_set_to_single_density(device))? 30000 : 15000;
	else
		time = 1000;

	if (!w->use_real_timing)
		time = 1;

	w->timer_ws->adjust(attotime::from_usec(time));
	w->to_be_continued = TRUE;
}

/*
    Set up a timed track read/write
*/
static void smc92x4_timed_track_request(device_t *device)
{
	smc92x4_state *w = get_safe_token(device);
	int time = 0;

	if (w->selected_drive_type & TYPE_FLOPPY)
		time = TRACKTIME_FLOPPY;
	else
		time = TRACKTIME_HD;

	if (!w->use_real_timing)
		time = 1;

	w->timer_track->adjust(attotime::from_usec(time));

	w->to_be_continued = TRUE;
}

/*
    Set up a timed track seek
*/
static void smc92x4_timed_seek_request(device_t *device)
{
	smc92x4_state *w = get_safe_token(device);
	int time = 0;

	int index = w->register_w[MODE] & MO_STEPRATE;
	int fm = controller_set_to_single_density(device);

	/* Get seek time. */
	if ((w->selected_drive_type & DRIVE_TYPE) == TYPE_FLOPPY8)
		time = step_flop8[index];

	else if ((w->selected_drive_type & DRIVE_TYPE) == TYPE_FLOPPY5)
		time = step_flop5[index];
	else
		time = step_hd[index];

	if (fm)
		time = time * 2;

	if (!w->use_real_timing)
	{
		logerror("smc92x4 info: Disk access without delays\n");
		time = 1;
	}

	w->timer_seek->adjust(attotime::from_usec(time));
	w->to_be_continued = TRUE;
}

#if 0
static TIMER_CALLBACK(smc92x4_data_callback)
{
/*  device_t *device = (device_t *)ptr;
    smc92x4_state *w = get_safe_token(device); */
}
#endif

static TIMER_CALLBACK(smc92x4_read_sector_callback)
{
	device_t *device = (device_t *)ptr;
	smc92x4_state *w = get_safe_token(device);
	int transfer_enabled = w->command & 0x01;

	/* Now read the sector. */
	data_transfer_read(device, w->recent_id, transfer_enabled);
	sync_status_in(device);
	smc92x4_process_after_callback(device);
}

static TIMER_CALLBACK(smc92x4_write_sector_callback)
{
	device_t *device = (device_t *)ptr;
	smc92x4_state *w = get_safe_token(device);
	int deldata = w->command & 0x10;
	int redcur =  w->command & 0x08;
	int precomp = w->command & 0x07;
	int write_long = ((w->register_w[MODE]& MO_CRCECC)==0x40);

	if (deldata)
		logerror("smc92x4 warn: Write deleted data mark not supported. Writing normal mark.\n");

	/* Now write the sector. */
	data_transfer_write(device, w->recent_id, deldata, redcur, precomp, write_long);
	sync_status_in(device);
	smc92x4_process_after_callback(device);
}

/*
    Callback for track access. Nothing interesting here, just used for
    delaying.
*/
static TIMER_CALLBACK(smc92x4_track_callback)
{
	device_t *device = (device_t *)ptr;
	sync_status_in(device);
	smc92x4_process_after_callback(device);
}

/*
    Callback for seek request.
*/
static TIMER_CALLBACK(smc92x4_seek_callback)
{
	device_t *device = (device_t *)ptr;
	smc92x4_state *w = get_safe_token(device);
	device_t *current_floppy = NULL;
/*  int buffered = ((w->register_w[MODE] & MO_STEPRATE)==0); */
/*  int buffered = (w->command & 0x01); */

	if (w->selected_drive_type & TYPE_FLOPPY)
	{
		current_floppy = (*w->intf->current_floppy)(device);
		if (current_floppy==NULL)
		{
			logerror("smc92x4 error: seek callback: no floppy\n");
			w->register_r[INT_STATUS] |= ST_TC_SEEKERR;
		}
		else
		{
			logerror("smc92x4 step %s direction %d\n", current_floppy->tag(), w->step_direction);
			floppy_drive_seek(current_floppy, w->step_direction);
		}
	}
	else
	{
		// logerror("smc92x4 step harddisk direction %d\n", w->step_direction);
		(*w->intf->mfmhd_seek)(device, w->step_direction);
	}
	sync_status_in(device);

	smc92x4_process_after_callback(device);
}

/*********************************************************************
    Common functions
*********************************************************************/

/*
    Calculate the ident byte from the cylinder. The specification does not
    define idents beyond cylinder 1023, but formatting programs seem to
    continue with 0xfd for cylinders between 1024 and 2047.
*/
UINT8 cylinder_to_ident(int cylinder)
{
	if (cylinder < 256) return 0xfe;
	if (cylinder < 512) return 0xff;
	if (cylinder < 768) return 0xfc;
	if (cylinder < 1024) return 0xfd;
	return 0xfd;
}

/*
    Calculate the offset from the ident. This is only useful for AT drives;
    these drives cannot have more than 1024 cylinders.
*/
static int ident_to_cylinder(UINT8 ident)
{
	switch (ident)
	{
	case 0xfe: return 0;
	case 0xff: return 256;
	case 0xfc: return 512;
	case 0xfd: return 768;
	default: return -1;
	}
}

/*
    Common function to set the read registers from the recently read id.
*/
static void update_id_regs(device_t *device, chrn_id_hd id)
{
	// Flags for current head register. Note that the sizes are not in
	// sequence (128, 256, 512, 1024). This is only interesting for AT
	// mode.
	static const UINT8 sizeflag[] = { 0x60, 0x00, 0x20, 0x40 };
	smc92x4_state *w = get_safe_token(device);

	w->register_r[CURRENT_CYLINDER] = id.C & 0xff;

	w->register_r[CURRENT_HEAD] = id.H & 0x0f;

	if (id.flags & BAD_SECTOR)
		w->register_r[CURRENT_HEAD] |= 0x80;

	if ((w->selected_drive_type & DRIVE_TYPE) == TYPE_AT)
		w->register_r[CURRENT_HEAD] |= sizeflag[id.N];
	else
		w->register_r[CURRENT_HEAD] |= ((id.C & 0x700)>>4);

	w->register_r[CURRENT_IDENT] = cylinder_to_ident(id.C);
}

/*
    Common procedure: read_id_field (as described in the specification)
*/
static void read_id_field(device_t *device, int *steps, int *direction, chrn_id_hd *id)
{
	smc92x4_state *w = get_safe_token(device);
	int des_cylinder, cur_cylinder;
	int found = FALSE;
	sync_latches_out(device);
	sync_status_in(device);

	// Set command termination code. The error code is set first, and
	// on success, it is cleared.
	w->register_r[INT_STATUS] |= ST_TC_RDIDERR;

	/* Try to find an ID field. */
	if (w->selected_drive_type & TYPE_FLOPPY)
	{
		chrn_id idflop;
		device_t *disk_img = (*w->intf->current_floppy)(device);
		if (flopimg_get_image(disk_img) == NULL)
		{
			logerror("smc92x4 warn: No disk in drive\n");
			w->register_r[CHIP_STATUS] |= CS_SYNCERR;
			return;
		}

		/* Check whether image and controller are set to the same density. */
		if ((image_is_single_density(disk_img) && controller_set_to_single_density(device))
			|| (!image_is_single_density(disk_img) && !controller_set_to_single_density(device))
			)
		{
			found = floppy_drive_get_next_id(disk_img, w->register_w[DESIRED_HEAD]&0x0f, &idflop);
			copyid(idflop, id); /* Need to use bigger values for HD, but we don't use separate variables here */
		}
		else
		{
			logerror("smc92x4 warn: Controller and medium density do not match.\n");
		}
		sync_status_in(device);

		if (!found)
		{
			logerror("smc92x4 error: read_id_field (floppy): sync error\n");
			w->register_r[CHIP_STATUS] |= CS_SYNCERR;
			return;
		}
	}
	else
	{
		(*w->intf->mfmhd_get_next_id)(device, w->register_w[DESIRED_HEAD]&0x0f, id);
		sync_status_in(device);
		if (!(w->register_r[DRIVE_STATUS]& DS_READY))
		{
			logerror("smc92x4 error: read_id_field (harddisk): sync error\n");
			w->register_r[CHIP_STATUS] |= CS_SYNCERR;
			return;
		}
	}

	w->register_r[CHIP_STATUS] &= ~CS_SYNCERR;

	/* Update the registers. */
	update_id_regs(device, *id);

	if (id->flags & BAD_CRC)
	{
		w->register_r[CHIP_STATUS] |= CS_CRCERR;
		return;
	}

	/* Calculate the steps. */
	if ((w->selected_drive_type & DRIVE_TYPE) == TYPE_AT)
	{
		/* Note: the spec says CURRENT_REGISTER, but that seems wrong. */
		des_cylinder = ((w->register_r[DATA] & 0x03)<<8) | w->register_w[DESIRED_CYLINDER];
		cur_cylinder = ident_to_cylinder(w->register_r[CURRENT_IDENT]) + w->register_r[CURRENT_CYLINDER];
	}
	else
	{
		des_cylinder = ((w->register_w[DESIRED_HEAD] & 0x70)<<4) | w->register_w[DESIRED_CYLINDER];
		cur_cylinder = ((w->register_r[CURRENT_HEAD] & 0x70)<<4) | w->register_r[CURRENT_CYLINDER];
	}

	if (des_cylinder >= cur_cylinder)
	{
		*steps = des_cylinder - cur_cylinder;
		*direction = +1;
	}
	else
	{
		*steps = cur_cylinder - des_cylinder;
		*direction = -1;
	}

	//logerror("smc92x4 seek required: %d steps\n", *steps);

	w->register_r[INT_STATUS] &= ~ST_TC_RDIDERR;
	return;
}

/*
    Common procedure: verify (as described in the specification)
*/
static int verify(device_t *device, chrn_id_hd *id, int check_sector)
{
	smc92x4_state *w = get_safe_token(device);
	int maxtry = 132;  /* approx. 33792/(256*32) */
	int pass = 0;
	int found = FALSE;
	int foundsect = FALSE;
	int des_cylinder = 0;

	// Set command termination code. The error code is set first, and
	// on success, it is cleared.
	w->register_r[INT_STATUS] |= ST_TC_SEEKERR;

	while (pass < maxtry && !foundsect)
	{
		pass++;
		/* Try to find an ID field. */
		if (w->selected_drive_type & TYPE_FLOPPY)
		{
			chrn_id idflop;
			device_t *disk_img = (*w->intf->current_floppy)(device);
			found = floppy_drive_get_next_id(disk_img, w->register_w[DESIRED_HEAD]&0x0f, &idflop);
			copyid(idflop, id);
			if (/* pass==1 && */!found)
			{
				w->register_r[CHIP_STATUS] |= CS_SYNCERR;
				logerror("smc92x4 error: verify (floppy): sync error\n");
				return ERROR;
			}
		}
		else
		{
			(*w->intf->mfmhd_get_next_id)(device, w->register_w[DESIRED_HEAD]&0x0f, id);
			sync_status_in(device);
			if (!(w->register_r[DRIVE_STATUS]& DS_READY))
			{
				logerror("smc92x4 error: verify (harddisk): sync error\n");
				w->register_r[CHIP_STATUS] |= CS_SYNCERR;
				return ERROR;
			}
		}

		w->register_r[CHIP_STATUS] &= ~CS_SYNCERR;

		/* Compare with the desired sector ID. */
		if ((w->selected_drive_type & DRIVE_TYPE) == TYPE_AT)
		{
			/* Note: the spec says CURRENT_CYLINDER, but that seems wrong. */
			des_cylinder = ((w->register_r[DATA] & 0x03)<<8) | w->register_w[DESIRED_CYLINDER];
		}
		else
		{
			des_cylinder = ((w->register_w[DESIRED_HEAD] & 0x70)<<4) | w->register_w[DESIRED_CYLINDER];
		}
//      logerror("smc92x4 check id: current = (%d,%d,%d), required = (%d,%d,%d)\n", id->C, id->H, id->R, des_cylinder, w->register_w[DESIRED_HEAD] & 0x0f, w->register_w[DESIRED_SECTOR]);
		if ((des_cylinder == id->C)
			&& ((w->register_w[DESIRED_HEAD] & 0x0f) == id->H))
		{
			if (!check_sector ||  (w->register_w[DESIRED_SECTOR] == id->R))
				foundsect = TRUE;
		}
	}
	if (!foundsect)
	{
		w->register_r[CHIP_STATUS] |= CS_COMPERR;
		logerror("smc92x4 error: verify: sector not found, seek error (desired cyl=%d/sec=%d, current cyl=%d/sec=%d)\n", des_cylinder, w->register_w[DESIRED_SECTOR], id->C, id->R);
		return ERROR;
	}

	w->register_r[INT_STATUS] &= ~ST_TC_SEEKERR;
	return DONE;
}

/*
    Common procedure: data_transfer(read) (as described in the specification)
*/
static void data_transfer_read(device_t *device, chrn_id_hd id, int transfer_enable)
{
	int i, retry, sector_len;
	smc92x4_state *w = get_safe_token(device);
	device_t *current_floppy = NULL;

	int sector_data_id;
	UINT8 *buf;

	sync_latches_out(device);
	sync_status_in(device);

	current_floppy = (*w->intf->current_floppy)(device);

	// Set command termination code. The error code is set first, and
	// on success, it is cleared.
	w->register_r[INT_STATUS] |= ST_TC_DATAERR;

	// Save the value. Note that the retry count is only in the first four
	// bits, and it is stored in one's complement. In this implementation
	// we don't work with it.
	retry = w->register_w[RETRY_COUNT];

	/* We are already at the correct sector (found by the verify sequence) */

	/* Find the data sync mark. Easy, we assume it has been found. */

	if (id.flags & ID_FLAG_DELETED_DATA)
		w->register_r[CHIP_STATUS] |= CS_DELDATA;
	else
		w->register_r[CHIP_STATUS] &= ~CS_DELDATA;

	/* Found. Now update the current cylinder/head registers. */
	update_id_regs(device, id);

	/* Initiate the DMA. We assume the DMARQ is positive. */
	w->register_r[INT_STATUS] &= ~ST_OVRUN;

	sector_len = 1 << (id.N+7);
	sector_data_id = id.data_id;

	if (w->selected_drive_type & TYPE_FLOPPY)
	{
		buf = (UINT8 *)malloc(sector_len);
		floppy_drive_read_sector_data(current_floppy, id.H, sector_data_id, buf, sector_len);
	}
	else
	{
		/* buf is allocated within the function, and sector_len is also set */
		(*w->intf->mfmhd_read_sector)(device, id.C, id.H, id.R, &buf, &sector_len);
	}
	sync_status_in(device);

	if (transfer_enable)
	{
		/* Copy via DMA into controller RAM. */
		set_dma_address(device, DMA23_16, DMA15_8, DMA7_0);

		smc92x4_set_dip(device, TRUE);
		for (i=0; i < sector_len; i++)
		{
			(*w->intf->dma_write_callback)(device, buf[i]);
		}
		smc92x4_set_dip(device, FALSE);
	}
	free(buf);

	/* Check CRC. We assume everything is OK, no retry required. */
	w->register_r[CHIP_STATUS] &= ~CS_RETREQ;
	w->register_r[CHIP_STATUS] &= ~CS_CRCERR;
	w->register_r[CHIP_STATUS] &= ~CS_ECCATT;

	/* Update the DMA registers. */
	if (transfer_enable)
		dma_add_offset(device, sector_len);

	/* Decrement sector count. */
	w->register_w[SECTOR_COUNT] = (w->register_w[SECTOR_COUNT]-1)&0xff;

	/* Clear the error bits. */
	w->register_r[INT_STATUS] &= ~ST_TC_DATAERR;

	if (w->register_w[SECTOR_COUNT] == 0)
		return;

	/* Else this is a multi-sector operation. */
	w->register_w[DESIRED_SECTOR] =
	w->register_r[DESIRED_SECTOR] =
		(w->register_w[DESIRED_SECTOR]+1) & 0xff;

	/* Reinitialize retry count. */
	w->register_w[RETRY_COUNT] = retry;
}

/*
    Common procedure: data_transfer(write) (as described in the specification)
*/
static void data_transfer_write(device_t *device, chrn_id_hd id, int deldata, int redcur, int precomp, int write_long)
{
	int retry, i, sector_len;
	smc92x4_state *w = get_safe_token(device);
	device_t *current_floppy = NULL;
	UINT8 *buf;
	int sector_data_id;

	sync_latches_out(device);
	sync_status_in(device);

	current_floppy = (*w->intf->current_floppy)(device);

	// Set command termination code. The error code is set first, and
	// on success, it is cleared.
	w->register_r[INT_STATUS] |= ST_TC_DATAERR;

	// Save the value. Note that the retry count is only in the first four
	// bits, and it is stored in one's complement. In this implementation
	// we don't work with it.
	retry = w->register_w[RETRY_COUNT];

	/* Initiate the DMA. We assume the DMARQ is positive. */
	w->register_r[INT_STATUS] &= ~ST_OVRUN;

	sector_len = 1 << (id.N+7);
	sector_data_id = id.data_id;

	buf = (UINT8 *)malloc(sector_len);

	/* Copy via DMA from controller RAM. */
	set_dma_address(device, DMA23_16, DMA15_8, DMA7_0);

	smc92x4_set_dip(device, TRUE);
	for (i=0; i<sector_len; i++)
	{
		buf[i] = (*w->intf->dma_read_callback)(device);
	}
	smc92x4_set_dip(device, FALSE);

	if (write_long)
	{
		logerror("smc92x4 warn: write sector: Write_long not supported. Performing a normal write.\n");
	}

	if (w->selected_drive_type & TYPE_FLOPPY)
	{
		floppy_drive_write_sector_data(current_floppy, id.H, sector_data_id, (char *) buf, sector_len, FALSE);
	}
	else
	{
		(*w->intf->mfmhd_write_sector)(device, id.C, id.H, id.R, buf, sector_len);
	}
	free(buf);
	sync_status_in(device);

	w->register_r[CHIP_STATUS] &= ~CS_RETREQ;
	w->register_r[CHIP_STATUS] &= ~CS_CRCERR;
	w->register_r[CHIP_STATUS] &= ~CS_ECCATT;

	/* Update the DMA registers. */
	dma_add_offset(device, sector_len);

	/* Decrement sector count. */
	w->register_w[SECTOR_COUNT] = (w->register_w[SECTOR_COUNT]-1)&0xff;

	/* Clear the error bits. */
	w->register_r[INT_STATUS] &= ~ST_TC_DATAERR;

	if (w->register_w[SECTOR_COUNT] == 0)
		return;

	/* Else this is a multi-sector operation. */
	w->register_w[DESIRED_SECTOR] =
	w->register_r[DESIRED_SECTOR] =
		(w->register_w[DESIRED_SECTOR]+1) & 0xff;

	/* Reinitialize retry count. */
	w->register_w[RETRY_COUNT] = retry;
}

static void read_sectors_continue(device_t *device);
static void write_sectors_continue(device_t *device);

/*
    Read sectors physical / logical. Physical means that the first, the
    second, the third sector appearing under the head will be read. These
    sectors are usually not in logical sequence. The ordering depends on
    the interleave pattern.
*/
static void read_write_sectors(device_t *device)
{
	smc92x4_state *w = get_safe_token(device);

	int logical = w->command & 0x04;
	int implied_seek_disabled = w->command & 0x02; /* for read */
	int write = (w->command & 0x80);

	w->after_seek = FALSE;

	w->to_be_continued = FALSE;

	if (write) /* write sectors */
	{
		logical = w->command & 0x20;
		implied_seek_disabled = w->command & 0x40;
	}

	if (!logical)
		implied_seek_disabled = FALSE;

	if (!w->found_id)
	{
		/* First start. */
		read_id_field(device, &w->seek_count, &w->step_direction, &w->recent_id);
	}

	if ((w->register_r[INT_STATUS] & ST_TC_DATAERR) == ST_TC_SUCCESS)
	{
		w->found_id = TRUE;
		/* Perform the seek for the cylinder. */
		if (!implied_seek_disabled && w->seek_count > 0)
		{
			smc92x4_timed_seek_request(device);
		}
		else
		{
			if (write)
				write_sectors_continue(device);
			else
				read_sectors_continue(device);
		}

	}
	else
	{
		set_command_done(device, ST_TC_RDIDERR);
	}
}


static void read_sectors_continue(device_t *device)
{
	smc92x4_state *w = get_safe_token(device);

	int check_sector = TRUE; /* always check the first sector */
	int state = AGAIN;
	int logical = w->command & 0x04;

	/* Needed for the two ways of re-entry: during the seek process, and during sector read */
	if (!w->after_seek)
	{
		// logerror("smc92x4 continue with sector read\n");
		w->seek_count--;
		if (w->seek_count > 0)
		{
			read_write_sectors(device);
			return;
		}
		w->after_seek = TRUE;
	}
	else
	{
		/* we are here after the sector read. */
		w->to_be_continued = FALSE;
		if (w->register_r[INT_STATUS] & ST_TERMCOD)
		{
			logerror("smc92x4 error: data error during sector read: INTSTATUS=%02x\n", w->register_r[INT_STATUS]);
			set_command_done(device, ST_TC_DATAERR);
			return;
		}
	}

	w->to_be_continued = FALSE;

	/* Wait for SEEK_COMPLETE. We assume the signal has appeared. */

	if (w->register_w[SECTOR_COUNT] > 0 /* && !(w->register_r[DRIVE_STATUS] & DS_INDEX) */)
	{
		/* Call the verify sequence. */
		state = verify(device, &w->recent_id, check_sector);
		if (state==ERROR)
		{
			// TODO: set command done?
			logerror("smc92x4 error: verify error during sector read\n");
			return;
		}

		/* For read physical, only verify the first sector. */
		if (!logical)
			check_sector = FALSE;

		if (w->recent_id.flags & BAD_SECTOR)
		{
			logerror("smc92x4 error: Bad sector, seek error\n");
			set_command_done(device, ST_TC_SEEKERR);
		}
		else
		{
			smc92x4_timed_sector_read_request(device);
		}
	}
	else
	{
		set_command_done(device, ST_TC_SUCCESS);
		//logerror("smc92x4 read sector command done\n");
	}
}

static void write_sectors_continue(device_t *device)
{
	smc92x4_state *w = get_safe_token(device);

	int check_sector = TRUE; /* always check the first sector */
	int state = AGAIN;
	int logical = w->command & 0x20;

	/* Needed for the two ways of re-entry: during the seek process, and during sector write */
	if (!w->after_seek)
	{
		w->seek_count--;
		if (w->seek_count > 0)
		{
			read_write_sectors(device);
			return;
		}
		w->after_seek = TRUE;
	}
	else
	{
		/* we are here after the sector write. */
		w->to_be_continued = FALSE;
		if (w->register_r[INT_STATUS] & ST_TERMCOD)
		{
			logerror("smc92x4 error: data error during sector write\n");
			set_command_done(device, ST_TC_DATAERR);
			return;
		}
	}

	w->to_be_continued = FALSE;

	if ((w->register_w[RETRY_COUNT] & 0xf0)!= 0xf0)
		logerror("smc92x4 warn: RETRY_COUNT in write sector should be set to 0. Ignored.\n");

	/* Wait for SEEK_COMPLETE. We assume the signal has appeared. */
	if (w->register_w[SECTOR_COUNT] > 0)
	{
		/* Call the verify sequence. */
		state = verify(device, &w->recent_id, check_sector);
		if (state==ERROR)
		{
			logerror("smc92x4 error: verify error during sector write\n");
			return;
		}

		/* For write physical, only verify the first sector. */
		if (!logical)
			check_sector = FALSE;

/*      printf("smc92x4 write sectors CYL=%02x, HEA=%02x, SEC=%02x, MOD=%02x, CNT=%02x, TRY=%02x\n",
            w->register_w[DESIRED_CYLINDER],
            w->register_w[DESIRED_HEAD],
            w->register_w[DESIRED_SECTOR],
            w->register_w[MODE],
            w->register_w[SECTOR_COUNT],
            w->register_w[RETRY_COUNT]); */
		smc92x4_timed_sector_write_request(device);
	}
	else
	{
		set_command_done(device, ST_TC_SUCCESS);
		// logerror("smc92x4 write sector command done\n");
	}
}

/*********************************************************************
    Command implementations
*********************************************************************/

/*
    Handle the restore command
*/
static void restore_drive(device_t *device)
{
	smc92x4_state *w = get_safe_token(device);

	/* TODO: int_after_seek_complete required for buffered seeks */
	sync_status_in(device);

	if (w->seek_count>=4096 || !(w->register_r[DRIVE_STATUS] & DS_READY))
	{
		logerror("smc92x4 error: seek error in restore\n");
		w->register_r[INT_STATUS] |= ST_TC_SEEKERR;
		return;
	}

	if (w->register_r[DRIVE_STATUS] & DS_TRK00)
	{
		w->register_r[INT_STATUS] |= ST_TC_SUCCESS;
		/* Issue interrupt */
		smc92x4_set_interrupt(device);
		return;
	}

	w->step_direction = -1;
	smc92x4_timed_seek_request(device);
}

static void restore_continue(device_t *device)
{
	smc92x4_state *w = get_safe_token(device);

	w->seek_count++;

	/* Next iteration */
	restore_drive(device);

	w->to_be_continued = FALSE;
}

/*
    Handle the step command. Note that the CURRENT_CYLINDER register is not
    updated (this would break the format procedure).
*/
static void step_in_out(device_t *device)
{
	smc92x4_state *w = get_safe_token(device);
	int direction = (w->command & 0x02)? -1 : +1;
	// bit 0: 0 -> command ends after last seek pulse, 1 -> command
	// ends when the drive asserts the seek complete pin
	int buffered = w->command & 0x01;

	w->step_direction = direction;
	w->buffered = buffered;

	smc92x4_timed_seek_request(device);
	//logerror("smc92x4 waiting for drive step\n");
}

static void step_continue(device_t *device)
{
	smc92x4_state *w = get_safe_token(device);
	//logerror("smc92x4 step continue\n");
	w->to_be_continued = FALSE;
	set_command_done(device,  ST_TC_SUCCESS);
}


static void drive_select(device_t *device, int drive);

/*
    Poll drives
    This command is used to find out which drive has complete a buffered
    seek (RESTORE, SEEK IN/OUT with BUFFERED set to one)
*/
static void poll_drives(device_t *device)
{
	int mask = 0x08;
	smc92x4_state *w = get_safe_token(device);
	int i;
	int flags = w->command & 0x0f;

/* Spec is unclear: Do we continue to poll the drives after we have checked each
one for the first time? We are not interested in locking up the emulator, so
we decide to poll only once. */
	for (i=3; i>=0; i--)
	{
		if (flags & mask)
		{
			/* Poll drive */
			drive_select(device, i|((w->types[i])<<2));
			sync_latches_out(device);
			sync_status_in(device);
			if (w->register_r[DRIVE_STATUS] & DS_SKCOM) return;
		}
		mask = mask>>1;
	}
}

static void drive_select(device_t *device, int driveparm)
{
	smc92x4_state *w = get_safe_token(device);

	w->output1 = (0x10 << (driveparm & 0x03)) | (w->register_w[RETRY_COUNT]&0x0f);
	w->selected_drive_type = (driveparm>>2) & 0x03;
	w->head_load_delay_enable = (driveparm>>4)&0x01;

	// We need to store the type of the drive for the poll_drives command
	// to be able to correctly select the device (floppy or hard disk).
	w->types[driveparm&0x03] = w->selected_drive_type;

	// Copy the DMA registers to registers CURRENT_HEAD, CURRENT_CYLINDER,
	// and CURRENT_IDENT. This is required during formatting (p. 14) as the
	// format command reuses the registers for formatting parameters.
	w->register_r[CURRENT_HEAD] = w->register_r[DMA7_0];
	w->register_r[CURRENT_CYLINDER] = w->register_r[DMA15_8];
	w->register_r[CURRENT_IDENT] = w->register_r[DMA23_16];

	sync_latches_out(device);
	sync_status_in(device);
}

/*
    Command SEEK/READID
*/
static void seek_read_id(device_t *device)
{
	smc92x4_state *w = get_safe_token(device);
	int step_enable = (w->command & 0x04);
	int verify_id = (w->command & 0x01);
	int wait = (w->command & 0x02);

	w->to_be_continued = FALSE;

	if (!w->found_id)
	{
		/* First start. */
		read_id_field(device, &w->seek_count, &w->step_direction, &w->recent_id);
		w->found_id = TRUE;
	}

	if ((w->register_r[INT_STATUS] & ST_TC_DATAERR) == ST_TC_SUCCESS)
	{
		/* Perform the seek for the cylinder. */
		if (step_enable && w->seek_count > 0)
		{
			smc92x4_timed_seek_request(device);
		}
		else
		{
			if (wait)
				logerror("smc92x4 warn: seed_read_id: Waiting for seek_complete not implemented.\n");

			if (verify_id)
			{
				verify(device, &w->recent_id, TRUE);
			}
		}
	}
	else
	{
		set_command_done(device, ST_TC_RDIDERR);
	}
}

static void seek_read_id_continue(device_t *device)
{
	smc92x4_state *w = get_safe_token(device);
	w->seek_count--;
	seek_read_id(device);
	w->to_be_continued = FALSE;
}

/*
    Formats a track starting from the detection of an index mark until the
    detection of another index mark.
    The formatting is done exclusively by the controller; user programs may
    set parameters for gaps and interleaving.

    1. Before starting the command, the user program must have set up a
    sector sequence table in the controller RAM (located on the PCB):
    (ident, cylinder, head, sector1, size)  (5 bytes)
    (ident, cylinder, head, sector2, size)
    (ident, cylinder, head, sector3, size)
    ...
    ident is not required for floppy FM operation. size is not required
    for IBM AT-compatible hard disks.

    2. The DMA registers must point to the beginning of the table

    3. DRIVE_SELECT must be executed (which moves DMA regs to CUR_HEAD ...)

    4. DESIRED_HEAD register must be loaded

    5. The following setup must be done:

    GAP 0 size      DMA7_0          (2s comp)
    GAP 1 size      DMA15_8         (2s comp)
    GAP 2 size      DMA23_16        (2s comp)
    GAP 3 size      DESIRED_SECTOR      (2s comp)
    Sync size       DESIRED_CYLINDER    (1s comp)
    Sector count        SECTOR_COUNT        (1s comp)
    Sector size multiple    RETRY_COUNT         (1s comp)

    GAP4 is variable and fills the rest of the track until the next
    index hole. Usually we have 247 bytes for FM and 598 for MFM.

    6. The step rate and density must be loaded into the MODE register

    7. The drive must be stepped to the desired track.

    8. Now this command may be started.

    All data bytes of a sector are filled with 0xe5. The gaps will be filled
    with 0x4e (MFM) or 0xff (FM).

    To format another track, the sector id table must be updated, and steps
    7 and 8 must be repeated. If the DESIRED_HEAD register must be updated,
    the complete setup process must be done.

    Options: Command = 011m rppp
    m = set data mark (0 = set deleted)
    r = current (1 = reduced)
    ppp = precompensation (for sector size = (2^ppp) * 128; ppp<4 for floppy )

    ===============

    One deviation from the specification: The TI-99 family uses the SDF
    and TDF formats. The TDF formats complies with the formats produced
    by the standard TI disk controller. It does not use index address marks.
    So we could integrate a translation in ti99_dsk when writing to disk,
    but when reading, how can ti99_dsk know whether to blow up the format
    to full length for this controller or keep it as is for the other
    controller? Unless someone comes up with a better idea, we implement
    an "undocumented" option in this controller, allowing to create a
    different track layout.

    So there are two layouts for the floppy track:

    - full format, including the index address mark, according to the
      specification

    - short format without index AM which matches the PC99 format used
      for the TI-99 family.

    The formats are determined by setting the flag in the smc92x4
    interface structure.
*/
static void format_floppy_track(device_t *device, int flags)
{
	smc92x4_state *w = get_safe_token(device);
	device_t *current_floppy = NULL;

	floppy_image *floppy;
	int i,index,j, exp_size;
	int gap0, gap1, gap2, gap3, gap4, sync1, sync2, count, size, fm;
	int gap_byte, pre_gap, crc, mark, inam;
	UINT8 curr_ident, curr_cyl, curr_head, curr_sect, curr_size;

	int normal_data_mark = flags & 0x10;

	UINT8 *buffer;

	/* Determine the track size. We cannot allow different sizes in this design. */
	int data_count = 0;

	sync_status_in(device);

	current_floppy = (*w->intf->current_floppy)(device);
	floppy = flopimg_get_image(current_floppy);

	if (floppy != NULL)
		data_count = floppy_get_track_size(floppy, 0, 0);

	if (data_count==0)
	{
		if (controller_set_to_single_density(device))
			data_count = TRKSIZE_SD;
		else
			data_count = TRKSIZE_DD;
	}

	/* Build buffer */
	buffer = (UINT8*)malloc(data_count);

	fm = controller_set_to_single_density(device);

	sync2 = (~w->register_w[DESIRED_CYLINDER])&0xff;
	gap2 = (-w->register_w[DMA23_16])&0xff;
	count = (~w->register_w[SECTOR_COUNT])&0xff;
	size = (~w->register_w[RETRY_COUNT])&0xff;
	gap_byte = (fm)? 0xff : 0x4e;

	if (w->intf->full_track_layout)
	{
		/* Including the index AM. */
		gap0 = (-w->register_w[DMA7_0])&0xff;
		gap1 = (-w->register_w[DMA15_8])&0xff;
		gap3 = (-w->register_w[DESIRED_SECTOR])&0xff;
		gap4 = (fm)? 247 : 598;
		pre_gap = gap_byte;
		sync1 = sync2;
		inam = sync2 + ((fm)? 1 : 4);
	}
	else
	{
		/* Specific overrides for this format. We do not have the index mark. */
		gap0 = (fm)? 16 : 40;
		gap1 = 0;
		gap3 = (fm)? 45 : 24;
		gap4 = (fm)? 231 : 712;
		pre_gap = (fm)? 0x00 : 0x4e;
		sync1 = (fm)? 6 : 10;
		inam = 0;
	}

	index = 0;

	mark = (fm)? 10 : 16;  /* ID/DAM + A1 + CRC */

	exp_size = gap0 + inam + gap1 + count*(sync1 + mark + gap2 + sync2 + size*128 + gap3) + gap4;

	if (exp_size != data_count)
		logerror("smc92x4 warn: The current track length in the image (%d) does not match the new track length (%d). Keeping the old length. This will break the image (sorry).\n", data_count, exp_size);

	/* use the backup registers set up during drive_select */
	set_dma_address(device, CURRENT_IDENT, CURRENT_CYLINDER, CURRENT_HEAD);

	memset(&buffer[index], pre_gap, gap0);
	index += gap0;

	if (w->intf->full_track_layout)
	{
		/* Create the Index AM */
		memset(&buffer[index], 0x00, sync1);
		index += sync1;
		if (!fm)
		{
			memset(&buffer[index], 0xc2, 3);
			index += 3;
		}
		memset(&buffer[index], gap_byte, gap1);
		index += gap1;
	}

	/* for each sector */
	for (j=0; j < count; j++)
	{
		memset(&buffer[index], 0x00, sync1);
		index += sync1;

		if (!fm)
		{
			memset(&buffer[index], 0xa1, 3);
			index += 3;
		}

		buffer[index++] = 0xfe;

		smc92x4_set_dip(device, TRUE);
		if (!fm) curr_ident = (*w->intf->dma_read_callback)(device);
		curr_cyl = (*w->intf->dma_read_callback)(device);
		curr_head = (*w->intf->dma_read_callback)(device);
		curr_sect = (*w->intf->dma_read_callback)(device);
		curr_size = (*w->intf->dma_read_callback)(device);
		smc92x4_set_dip(device, FALSE);

		buffer[index++] = curr_cyl;
		buffer[index++] = curr_head;
		buffer[index++] = curr_sect;
		buffer[index++] = curr_size;

		// if (j==0) logerror("current_floppy=%s, format track %d, head %d\n",  current_floppy->tag(), curr_cyl, curr_head);

		/* Calculate CRC16 (5 bytes for ID) */
		crc = ccitt_crc16(0xffff, &buffer[index-5], 5);
		buffer[index++] = (crc>>8)&0xff;
		buffer[index++] = crc & 0xff;

		memset(&buffer[index], gap_byte, gap2);
		index += gap2;

		memset(&buffer[index], 0x00, sync2);
		index += sync2;

		if (!fm)
		{
			memset(&buffer[index], 0xa1, 3);
			index += 3;
		}

		if (normal_data_mark) buffer[index++] = 0xfb;
		else buffer[index++] = 0xf8;

		/* Sector data */
		for (i=0; i < 128*size; i++) buffer[index++] = 0xe5;

		/* Calculate CRC16 (128*size+1 bytes for sector) */
		crc = ccitt_crc16(0xffff, &buffer[index-128*size-1], 128*size+1);
		buffer[index++] = (crc>>8)&0xff;
		buffer[index++] = crc & 0xff;

		memset(&buffer[index], gap_byte, gap3);
		index += gap3;
	}

	memset(&buffer[index], gap_byte, gap4);
	index += gap4;

	floppy_drive_write_track_data_info_buffer(current_floppy, w->register_w[DESIRED_HEAD]&0x0f, (char *)buffer, &data_count);
	free(buffer);
	sync_status_in(device);
}

/*
    Create the track layout of a MFM hard disk. Like floppy disks,
    MFM hard disks are interfaced on a track base, that is, we have to
    create a complete track layout.

    For more explanations see the comments to format_floppy_track.
*/
static void format_harddisk_track(device_t *device, int flags)
{
	smc92x4_state *w = get_safe_token(device);
	int i,index,j;
	int gap0, gap1, gap2, gap3, gap4, sync, count, size, fm, gap_byte, crc;
	UINT8 curr_ident, curr_cyl, curr_head, curr_sect, curr_size;

	int normal_data_mark = flags & 0x10;
	int data_count=0;

	UINT8 *buffer;

	sync_status_in(device);

	/* Build buffer */
	gap0 = (-w->register_w[DMA7_0])&0xff;
	gap1 = (-w->register_w[DMA15_8])&0xff;
	gap2 = (-w->register_w[DMA23_16])&0xff;
	gap3 = (-w->register_w[DESIRED_SECTOR])&0xff;
	gap4 = 340;

	sync = (~w->register_w[DESIRED_CYLINDER])&0xff;
	count = (~w->register_w[SECTOR_COUNT])&0xff;
	size = (~w->register_w[RETRY_COUNT])&0xff;

	data_count = gap1 + count*(sync+12+gap2+sync+size*128+gap3)+gap4;

	buffer = (UINT8*)malloc(data_count);

	index = 0;
	fm = controller_set_to_single_density(device);
	gap_byte = 0x4e;

	/* use the backup registers set up during drive_select */
	set_dma_address(device, CURRENT_IDENT, CURRENT_CYLINDER, CURRENT_HEAD);

	for (i=0; i < gap1; i++) buffer[index++] = gap_byte;

	/* Now write the sectors. */
	for (j=0; j < count; j++)
	{
		for (i=0; i < sync; i++) buffer[index++] = 0x00;
		buffer[index++] = 0xa1;

		smc92x4_set_dip(device, TRUE);
		curr_ident = (*w->intf->dma_read_callback)(device);
		curr_cyl = (*w->intf->dma_read_callback)(device);
		curr_head = (*w->intf->dma_read_callback)(device);
		curr_sect = (*w->intf->dma_read_callback)(device);
		curr_size = (*w->intf->dma_read_callback)(device);
		smc92x4_set_dip(device, FALSE);

		buffer[index++] = curr_ident;
		buffer[index++] = curr_cyl;
		buffer[index++] = curr_head;
		buffer[index++] = curr_sect;
		buffer[index++] = curr_size;

		/* Calculate CRC16 (5 bytes for ID) */
		crc = ccitt_crc16(0xffff, &buffer[index-5], 5);
		buffer[index++] = (crc>>8)&0xff;
		buffer[index++] = crc & 0xff;

		/* GAP 2 */
		for (i=0; i < gap2; i++) buffer[index++] = gap_byte;
		for (i=0; i < sync; i++) buffer[index++] = 0x00;

		buffer[index++] = 0xa1;

		if (normal_data_mark) buffer[index++] = 0xfb;
		else buffer[index++] = 0xf8;

		/* Sector data */
		for (i=0; i < 128*size; i++) buffer[index++] = 0xe5;

		/* Calculate CRC16 (128*size+1 bytes for sector) */
		crc = ccitt_crc16(0xffff, &buffer[index-128*size-1], 128*size+1);
		buffer[index++] = (crc>>8)&0xff;
		buffer[index++] = crc & 0xff;

		/* GAP 3 */
		for (i=0; i < 3; i++) buffer[index++] = 0;  /* check that, unclear in spec */
		for (i=0; i < gap3 - 3; i++) buffer[index++] = gap_byte;
	}
	/* GAP 4 */
	for (i=0; i < gap4; i++) buffer[index++] = gap_byte;

	(*w->intf->mfmhd_write_track)(device, w->register_w[DESIRED_HEAD]&0x0f, buffer, data_count);
	free(buffer);
	sync_status_in(device);
}

/*
    Read a floppy track.
    A complete track is read at the position of the head. It reads the
    track from one index pulse to the next index pulse. (Note that the
    spec talks about "index mark" and "signal from the drive" which is
    a bit confusing, since the index AM is behind Gap0, the index hole
    is before Gap0. We should check with a real device. Also, it does not
    speak about the head, so we assume the head is set in the DESIRED_HEAD
    register.)

    TODO: The TDF format does not support index marks. Need to define TDF
    in a more flexible way. Also consider format variations.
    (Hint: Do a check for the standard "IBM" format. Should probably do
    a translation from IBM to PC99 and back. Requires to parse the track
    image before. Need to decide whether we generally translate to the IBM
    format or generally to the PC99 format between controller and format.
    Format is the image is always PC99.)
*/
static void read_floppy_track(device_t *device, int transfer_only_ids)
{
	smc92x4_state *w = get_safe_token(device);
	device_t *current_floppy = (*w->intf->current_floppy)(device);

	floppy_image *floppy;
	/* Determine the track size. We cannot allow different sizes in this design. */
	int data_count = 0;
	int i;
	UINT8 *buffer;

	sync_latches_out(device);

	floppy = flopimg_get_image(current_floppy);

	/* Determine the track size. We cannot allow different sizes in this design. */
	if (floppy != NULL)
		data_count = floppy_get_track_size(floppy, 0, 0);

	if (data_count==0)
	{
		if (controller_set_to_single_density(device))
			data_count = TRKSIZE_SD;
		else
			data_count = TRKSIZE_DD;
	}

	buffer = (UINT8*)malloc(data_count);

	floppy_drive_read_track_data_info_buffer( current_floppy, w->register_w[DESIRED_HEAD]&0x0f, (char *)buffer, &data_count);
	sync_status_in(device);

	// Transfer the buffer to the external memory. We assume the memory
	// pointer has been set appropriately in the registers.
	set_dma_address(device, DMA23_16, DMA15_8, DMA7_0);

	if (transfer_only_ids)
	{
		logerror("smc92x4 warn: read track: Ignoring transfer-only-ids. Reading complete track.\n");
	}

	smc92x4_set_dip(device, TRUE);
	for (i=0; i < data_count; i++)
	{
		(*w->intf->dma_write_callback)(device, buffer[i]);
	}
	smc92x4_set_dip(device, FALSE);

	free(buffer);
}

static void read_harddisk_track(device_t *device, int transfer_only_ids)
{
	smc92x4_state *w = get_safe_token(device);

	/* Determine the track size. We cannot allow different sizes in this design. */
	int i;
	UINT8 *buffer;
	int data_count=0;

	sync_latches_out(device);

	/* buffer and data_count are allocated and set by the function. */
	(*w->intf->mfmhd_read_track)(device, w->register_w[DESIRED_HEAD]&0x0f, &buffer, &data_count);
	sync_status_in(device);

	if (!(w->register_r[DRIVE_STATUS] & DS_READY))
	{
		logerror("smc92x4 error: read harddisk track failed.\n");
		/* no buffer was allocated */
	}

	// Transfer the buffer to the external memory. We assume the memory
	// pointer has been set appropriately in the registers.
	set_dma_address(device, DMA23_16, DMA15_8, DMA7_0);

	if (transfer_only_ids)
	{
		logerror("smc92x4 warn: read track: Ignoring transfer-only-ids. Reading complete track.\n");
	}

	smc92x4_set_dip(device, TRUE);
	for (i=0; i < data_count; i++)
	{
		(*w->intf->dma_write_callback)(device, buffer[i]);
	}
	smc92x4_set_dip(device, FALSE);

	free(buffer);
}


static void read_track(device_t *device)
{
	smc92x4_state *w = get_safe_token(device);
	int transfer_only_ids = w->command & 0x01;

	if (w->selected_drive_type & TYPE_FLOPPY)
	{
		read_floppy_track(device, transfer_only_ids);
	}
	else
	{
		read_harddisk_track(device, transfer_only_ids);
	}

	smc92x4_timed_track_request(device);
}

static void read_track_continue(device_t *device)
{
	smc92x4_state *w = get_safe_token(device);
	w->to_be_continued = FALSE;
	set_command_done(device,  ST_TC_SUCCESS);
}

static void format_track(device_t *device)
{
	smc92x4_state *w = get_safe_token(device);
	int flags = w->command & 0x1f;

	if (w->selected_drive_type & TYPE_FLOPPY)
		format_floppy_track(device, flags);
	else
		format_harddisk_track(device, flags);

	smc92x4_timed_track_request(device);
}

static void format_track_continue(device_t *device)
{
	smc92x4_state *w = get_safe_token(device);
	w->to_be_continued = FALSE;
	set_command_done(device,  ST_TC_SUCCESS);
}

/*
    Continue to process after callback
*/
static void smc92x4_process_after_callback(device_t *device)
{
	smc92x4_state *w = get_safe_token(device);
	UINT8 opcode = w->command;
	if (opcode >= 0x02 && opcode <= 0x03)
	{
		restore_continue(device);
	}
	else if (opcode >= 0x04 && opcode <= 0x07)
	{
		step_continue(device);
	}
	else if (opcode >= 0x50 && opcode <= 0x57)
	{
		seek_read_id_continue(device);
	}
	else if ((opcode >= 0x58 && opcode <= 0x59)||(opcode >= 0x5C && opcode <= 0x5f))
	{
		read_sectors_continue(device);
	}
	else if (opcode >= 0x5a && opcode <= 0x5b)
	{
		read_track_continue(device);
	}
	else if (opcode >= 0x60 && opcode <= 0x7f)
	{
		format_track_continue(device);
	}
	else if (opcode >= 0x80)
	{
		write_sectors_continue(device);
	}
	else
	{
		logerror("smc92x4 warn: Invalid command %x or command changed while waiting for callback\n", opcode);
	}
}

/*
    Process a command
*/
static void smc92x4_process_command(device_t *device, UINT8 opcode)
{
	smc92x4_state *w = get_safe_token(device);

	//logerror("smc92x4 process command %02x\n", opcode);
	if (w->to_be_continued)
	{
		logerror("smc92x4 warn: previous command %02x not complete\n", w->command);
	}

	/* Reset DONE and BAD_SECTOR. */
	w->register_r[INT_STATUS] &= ~(ST_DONE | ST_BADSECT);

	/* Reset interrupt line (not explicitly mentioned in spec, but seems
    reasonable) */
	smc92x4_clear_interrupt(device);
	w->register_r[INT_STATUS] &= ~(ST_INTPEND | ST_RDYCHNG);

	w->command = opcode;
	w->found_id = FALSE;
	w->seek_count = 0;

	if (opcode == 0x00)
	{
		/* RESET */
		/* same effect as the RST* pin being active */
		logerror("smc92x4 info: reset command\n");
		smc92x4_reset(device);
	}
	else if (opcode == 0x01)
	{
		/* DESELECT DRIVE */
		/* done when no drive is in use */
		logerror("smc92x4 info: drdeselect command\n");
		w->output1 &= ~(OUT1_DRVSEL3|OUT1_DRVSEL2|OUT1_DRVSEL1|OUT1_DRVSEL0);
		w->output2 |= OUT2_DRVSEL3_;
		/* sync the latches on the PCB */
		sync_latches_out(device);
		sync_status_in(device);
	}
	else if (opcode >= 0x02 && opcode <= 0x03)
	{
		/* RESTORE DRIVE */
		// bit 0: 0 -> command ends after last seek pulse, 1 -> command
		// ends when the drive asserts the seek complete pin
		logerror("smc92x4 info: restore command %X\n", opcode);
		restore_drive(device);
	}
	else if (opcode >= 0x04 && opcode <= 0x07)
	{
		/* STEP IN/OUT ONE CYLINDER */
		logerror("smc92x4 info: step in/out command %X\n", opcode);
		step_in_out(device);
	}
	else if (opcode >= 0x08 && opcode <= 0x0f)
	{
		/* TAPE BACKUP (08-0f)*/
		logerror("smc92x4 warn: tape backup command %X not implemented\n", opcode);
	}
	else if (opcode >= 0x10 && opcode <= 0x1f)
	{
		/* POLLDRIVE */
		logerror("smc92x4 info: polldrive command %X\n", opcode);
		poll_drives(device);
	}
	else if (opcode >= 0x20 && opcode <= 0x3f)
	{
		/* DRIVE SELECT */
		logerror("smc92x4 info: drselect command %X\n", opcode);
		drive_select(device, opcode&0x1f);
	}
	else if (opcode >= 0x40 && opcode <= 0x4f)
	{
		/* SETREGPTR */
		logerror("smc92x4 info: setregptr command %X\n", opcode);
		w->register_pointer = opcode & 0xf;
		// Spec does not say anything about the effect of setting an
		// invalid value (only "care should be taken")
		if (w->register_pointer > 10)
		{
			logerror("smc92x4 error: set register pointer: Invalid register number: %d. Setting to 10.\n", w->register_pointer);
			w->register_pointer = 10;
		}
	}
	else if (opcode >= 0x50 && opcode <= 0x57)
	{
		/* SEEK/READ ID */
		logerror("smc92x4 seekreadid command %X\n", opcode);
		seek_read_id(device);
	}
	else if ((opcode >= 0x58 && opcode <= 0x59)
		|| (opcode >= 0x5C && opcode <= 0x5f)
		|| (opcode >= 0x80))
	{
		/* READ/WRITE SECTORS PHYSICAL/LOGICAL */
		logerror("smc92x4 info: read/write sector command %X\n", opcode);
		read_write_sectors(device);
	}
	else if (opcode >= 0x5A && opcode <= 0x5b)
	{
		/* READ TRACK */
		logerror("smc92x4 info: read track command %X\n", opcode);
		read_track(device);
	}
	else if (opcode >= 0x60 && opcode <= 0x7f)
	{
		/* FORMAT TRACK */
		logerror("smc92x4 info: format track command %X\n", opcode);
		format_track(device);
	}
	else
	{
		logerror("smc92x4 warn: Invalid command %x, ignored\n", opcode);
	}

	if (!w->to_be_continued)
		set_command_done(device);
}

/***************************************************************************
    Memory accessors
****************************************************************************/

/*
    Read a byte of data from a smc92x4 controller
    The address (offset) encodes the C/D* line (command and /data)
*/
READ8_DEVICE_HANDLER( smc92x4_r )
{
	UINT8 reply = 0;
	smc92x4_state *w = get_safe_token(device);

	if ((offset & 1) == 0)
	{
		/* data register */
		reply = w->register_r[w->register_pointer];
		// logerror("smc92x4 register_r[%d] -> %02x\n", w->register_pointer, reply);
		/* Autoincrement until DATA is reached. */
		if (w->register_pointer < DATA)
			w->register_pointer++;
	}
	else
	{
		/* status register */
		reply = w->register_r[INT_STATUS];
		// Spec (p.3) : The interrupt pin is reset to its inactive state
		// when the UDC interrupt status register is read.
		// logerror("smc92x4 interrupt status read = %02x\n", reply);
		smc92x4_clear_interrupt(device);
		/* Clear the bits due to int status register read. */
		w->register_r[INT_STATUS] &= ~(ST_INTPEND | ST_RDYCHNG);
	}
	return reply;
}

/*
    Write a byte to a smc99x4 controller
    The address (offset) encodes the C/D* line (command and /data)
*/
WRITE8_DEVICE_HANDLER( smc92x4_w )
{
	smc92x4_state *w = get_safe_token(device);
	data &= 0xff;

	if ((offset & 1) == 0)
	{
		/* data register */
		// logerror("smc92x4 register_w[%d] <- %X\n", w->register_pointer, data);
		w->register_w[w->register_pointer] = data;

		// The DMA registers and the sector register for read and
		// write are identical.
		if (w->register_pointer < DESIRED_HEAD)
			w->register_r[w->register_pointer] = data;

		/* Autoincrement until DATA is reached. */
		if (w->register_pointer < DATA)
			w->register_pointer++;
	}
	else
	{
		/* command register */
		smc92x4_process_command(device, data);
	}
}



/***************************************************************************
    DEVICE FUNCTIONS
***************************************************************************/

static DEVICE_START( smc92x4 )
{
	smc92x4_state *w = get_safe_token(device);
	assert(device->baseconfig().static_config() != NULL);

	w->intf = (const smc92x4_interface*)device->baseconfig().static_config();
	devcb_resolve_write_line(&w->out_intrq_func, &w->intf->out_intrq_func, device);
	devcb_resolve_write_line(&w->out_dip_func, &w->intf->out_dip_func, device);
	devcb_resolve_write8(&w->out_auxbus_func, &w->intf->out_auxbus_func, device);
	devcb_resolve_read_line(&w->in_auxbus_func, &w->intf->in_auxbus_func, device);

	/* allocate timers */
	/* w->timer_data = device->machine().scheduler().timer_alloc(FUNC(smc92x4_data_callback), (void *)device); */
	w->timer_rs = device->machine().scheduler().timer_alloc(FUNC(smc92x4_read_sector_callback), (void *)device);
	w->timer_ws = device->machine().scheduler().timer_alloc(FUNC(smc92x4_write_sector_callback), (void *)device);
	w->timer_track = device->machine().scheduler().timer_alloc(FUNC(smc92x4_track_callback), (void *)device);
	w->timer_seek = device->machine().scheduler().timer_alloc(FUNC(smc92x4_seek_callback), (void *)device);

	w->use_real_timing = TRUE;
}

static DEVICE_RESET( smc92x4 )
{
	smc92x4_state *w = get_safe_token(device);
	smc92x4_clear_interrupt(device);
	smc92x4_set_dip(device, FALSE);

	for (int i=0; i<11; i++)
		w->register_r[i] = w->register_w[i] = 0;
}

void smc92x4_reset(device_t *device)
{
	DEVICE_RESET_CALL( smc92x4 );
}

void smc92x4_set_timing(device_t *device, int realistic)
{
	smc92x4_state *w = get_safe_token(device);
	w->use_real_timing = realistic;
	logerror("smc92x4: use realistic timing: %02x\n", realistic);
}

/***************************************************************************
    DEVICE GETINFO
***************************************************************************/

static const char DEVTEMPLATE_SOURCE[] = __FILE__;

#define DEVTEMPLATE_ID(p,s)				p##smc92x4##s
#define DEVTEMPLATE_FEATURES				DT_HAS_START | DT_HAS_RESET
#define DEVTEMPLATE_NAME				"SMC92x4"
#define DEVTEMPLATE_FAMILY				"SMC92x4"
#define DEVTEMPLATE_VERSION				"1.0"
#define DEVTEMPLATE_CREDITS				"Copyright MESS Team"
#include "devtempl.h"

DEFINE_LEGACY_DEVICE(SMC92X4, smc92x4);
