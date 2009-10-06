/*
    TI-99/4 and /4a disk controller emulation.

    Known disk controllers:
    * TI original SD disk controller.  First sold as a side port module
      (PHP1800), later as a card to be inserted in the PE-box (PHP1240).
      Uses a WD1771 FDC, and supports a format of 40 tracks of 9 sectors.
    * TI DD disk controller prototype.  Uses some NEC FDC and supports 16
      sectors per track.  (Not emulated)
    * CorComp FDC.  Specs unknown, but it supports DD.  (Not emulated)
    * SNUG BwG.  A nice DD disk controller.  Supports 18 sectors per track.
    * Myarc HFDC.  Extremely elaborate, supports DD (and even HD, provided that
      a) the data separator is a 9216B and not a 9216, and b) that the OS
      overrides the hfdc DSR, whose HD support is buggy) and MFM harddisks.

    An alternative was installing the hexbus controller (which was just a
    prototype, and is not emulated) and connecting a hexbus floppy disk
    controller.  The integrated hexbus port was the only supported way to
    attach a floppy drive to a TI-99/2 or a TI-99/5.  The integrated hexbus
    port was the recommended way to attach a floppy drive to the TI-99/8,
    but I think this computer supported the TI-99/4(a) disk controllers, too.

    Raphael Nabet, 1999-2004.
*/

#include "driver.h"
#include "machine/wd17xx.h"
#include "smc92x4.h"
#include "ti99_4x.h"
#include "99_peb.h"
#include "99_dsk.h"
#include "mm58274c.h"
#include "formats/basicdsk.h"
#include "devices/flopdrv.h"


#define MAX_FLOPPIES 4
#define TI99DSK_TAG	"ti99dsktag"


static int use_80_track_drives;

/* defines for DRQ_IRQ_status */
enum
{
	fdc_IRQ = 1,
	fdc_DRQ = 2
};

/* current state of the DRQ and IRQ lines */
static int DRQ_IRQ_status;

/* when TRUE the CPU is halted while DRQ/IRQ are true */
static int DSKhold;
/* disk selection bits */
static int DSEL;
/* currently selected disk unit */
static int DSKnum;
/* current side */
static int DSKside;
/* 1 if motor is on */
static int DVENA;
/* on rising edge, sets DVENA for 4.23 seconds on rising edge */
static int motor_on;
/* count 4.23s from rising edge of motor_on */
static void *motor_on_timer;

/*
    call this when the state of DSKhold or DRQ/IRQ or DVENA change

    Emulation is faulty because the CPU is actually stopped in the midst of
    instruction, at the end of the memory access
*/
static void fdc_handle_hold(running_machine *machine)
{
	if (DSKhold && (!DRQ_IRQ_status) && DVENA)
		cputag_set_input_line(machine, "maincpu", INPUT_LINE_HALT, ASSERT_LINE);
	else
		cputag_set_input_line(machine, "maincpu", INPUT_LINE_HALT, CLEAR_LINE);
}

/*
    callback called at the end of DVENA pulse
*/
static TIMER_CALLBACK(motor_on_timer_callback)
{
	DVENA = 0;
	fdc_handle_hold(machine);
}

typedef struct ti99_geometry
{
	UINT8 sides;
	UINT8 tracksperside;
	UINT8 secspertrack;
	UINT8 density;
} ti99_geometry;

static void hfdc_int_callback(int which, int state);
static int hfdc_select_callback(int which, select_mode_t select_mode, int select_line, int gpos);
static UINT8 hfdc_dma_read_callback(int which, offs_t offset);
static void hfdc_dma_write_callback(int which, offs_t offset, UINT8 data);

static const smc92x4_intf hfdc_intf =
{
	hfdc_select_callback,
	hfdc_dma_read_callback,
	hfdc_dma_write_callback,
	hfdc_int_callback
};

/*
    Convert physical sector address to sector offset in disk image
*/
static UINT64 ti99_translate_offset(floppy_image *floppy, const struct basicdsk_geometry *geom, int track, int head, int sector)
{
	UINT64 reply;

#if 0
	/* old MESS format */
	reply = ((track*geom->heads) + head)*geom->sectors + sector;
#else
	/* V9T9 format */
	if (head & 1)
		/* on side 1, tracks are stored in the reverse order */
		reply = ((head*geom->tracks) + (geom->tracks-1 - track))*geom->sectors + sector;
	else
		reply = ((head*geom->tracks) + track)*geom->sectors + sector;
#endif

	return reply;
}

/*
    support for 48TPI disks in 96TPI drives
*/
static int ti99_tracktranslate(const device_config *image, floppy_image *floppy, int physical_track)
{
	struct ti99_geometry *geometry;
	geometry = floppy_tag(floppy, TI99DSK_TAG);

	if (use_80_track_drives && (geometry->tracksperside <= 40))
		return physical_track/2;
	else
		return physical_track;
}

/*
    Sniff the geometry of a v9t9 format disk image
*/
static void ti99_guess_geometry(floppy_image *floppy, ti99_geometry *geometry, int *vote, int *success)
{
	typedef struct ti99_vib
	{
		char	name[10];			/* volume name (10 characters, pad with spaces) */
		UINT8	totsecsMSB;			/* disk length in sectors (big-endian) (usually 360, 720 or 1440) */
		UINT8	totsecsLSB;
		UINT8	secspertrack;		/* sectors per track (usually 9 (FM) or 18 (MFM)) */
		UINT8	id[3];				/* 'DSK' */
		UINT8	protection;			/* 'P' if disk is protected, ' ' otherwise. */
		UINT8	tracksperside;		/* tracks per side (usually 40) */
		UINT8	sides;				/* sides (1 or 2) */
		UINT8	density;			/* 1 (FM) or 2 (MFM) */
		UINT8	res[36];			/* reserved */
		UINT8	abm[200];			/* allocation bitmap: a 1 for each sector in use (sector 0 is LSBit of byte 0, sector 7 is MSBit of byte 0, sector 8 is LSBit of byte 1, etc.) */
	} ti99_vib;

	ti99_vib vib;
	int totsecs;


	/* Read sector 0 to identify format */
	/*if (floppy_image_read(floppy, & vib, 0, sizeof(vib)))*/
	floppy_image_read(floppy, & vib, 0, sizeof(vib));
	{
		/* If we have read the sector successfully, let us parse it */
		totsecs = (vib.totsecsMSB << 8) | vib.totsecsLSB;
		geometry->secspertrack = vib.secspertrack;
		if (geometry->secspertrack == 0)
			/* Some images might be like this, because the original SSSD TI
            controller always assumes 9. */
			geometry->secspertrack = 9;
		geometry->tracksperside = vib.tracksperside;
		if (geometry->tracksperside == 0)
			/* Some images are like this, because the original SSSD TI
            controller always assumes 40. */
			geometry->tracksperside = 40;
		geometry->sides = vib.sides;
		if (geometry->sides == 0)
			/* Some images are like this, because the original SSSD TI
            controller always assumes that tracks beyond 40 are on side 2. */
			geometry->sides = totsecs / (geometry->secspertrack * geometry->tracksperside);
		geometry->density = vib.density;
		if (geometry->density == 0)
			geometry->density = 1;
		/* check that the format makes sense */
		if (((geometry->secspertrack * geometry->tracksperside * geometry->sides) == totsecs)
			&& (geometry->density <= 3) && (totsecs >= 2) && (! memcmp(vib.id, "DSK", 3))
			&& (floppy_image_size(floppy) == totsecs*256))
		{
			/* validate geometry */
			if (vote)
				*vote = 100;
			if (success)
				*success = TRUE;
			return;
		}
	}

	/* If we have been unable to parse the format, let us guess according to
    file lenght */
	switch (floppy_image_size(floppy))
	{
	case 1*40*9*256:	/* 90kbytes: SSSD */
	case 0:
	/*default:*/
		geometry->sides = 1;
		geometry->tracksperside = 40;
		geometry->secspertrack = 9;
		geometry->density = 1;
		if (vote)
			*vote = 50;
		if (success)
			*success = TRUE;
		return;

	case 2*40*9*256:	/* 180kbytes: either DSSD or 18-sector-per-track
                        SSDD.  We assume DSSD since DSSD is more common
                        and is supported by the original TI SD disk
                        controller. */
		geometry->sides = 2;
		geometry->tracksperside = 40;
		geometry->secspertrack = 9;
		geometry->density = 1;
		if (vote)
			*vote = 50;
		if (success)
			*success = TRUE;
		return;

	case 1*40*16*256:	/* 160kbytes: 16-sector-per-track SSDD (standard
                        format for TI DD disk controller prototype, and
                        the TI hexbus disk controller???) */
		geometry->sides = 1;
		geometry->tracksperside = 40;
		geometry->secspertrack = 16;
		geometry->density = 2;
		if (vote)
			*vote = 50;
		if (success)
			*success = TRUE;
		return;

	case 2*40*16*256:	/* 320kbytes: 16-sector-per-track DSDD (standard
                        format for TI DD disk controller prototype, and
                        TI hexbus disk controller???) */
		geometry->sides = 2;
		geometry->tracksperside = 40;
		geometry->secspertrack = 16;
		geometry->density = 2;
		if (vote)
			*vote = 50;
		if (success)
			*success = TRUE;
		return;

	case 2*40*18*256:	/* 360kbytes: 18-sector-per-track DSDD (standard
                        format for most third-party DD disk controllers,
                        but reportedly not supported by the original TI
                        DD disk controller) */
		geometry->sides = 2;
		geometry->tracksperside = 40;
		geometry->secspertrack = 18;
		geometry->density = 2;
		if (vote)
			*vote = 50;
		if (success)
			*success = TRUE;
		return;

	case 2*80*18*256:	/* 720kbytes: 18-sector-per-track 80-track DSDD
                        (Myarc only) */
		geometry->sides = 2;
		geometry->tracksperside = 80;
		geometry->secspertrack = 18;
		geometry->density = 2;
		if (vote)
			*vote = 50;
		if (success)
			*success = TRUE;
		return;

	case 2*80*36*256:	/* 1.44Mbytes: DSHD (Myarc only) */
		geometry->sides = 2;
		geometry->tracksperside = 80;
		geometry->secspertrack = 36;
		geometry->density = 3;
		if (vote)
			*vote = 50;
		if (success)
			*success = TRUE;
		return;

	default:
		logerror("%s:%d: unrecognized disk image geometry\n", __FILE__, __LINE__);
		break;
	}
	if (vote)
		*vote = 0;
	if (success)
		*success = FALSE;
}

static FLOPPY_IDENTIFY(ti99_floppy_identify)
{
	ti99_geometry dummy_geom;

	ti99_guess_geometry(floppy, &dummy_geom, vote, NULL);

	return FLOPPY_ERROR_SUCCESS;
}

static FLOPPY_CONSTRUCT(ti99_floppy_construct)
{
	struct ti99_geometry *geometry1;
	struct basicdsk_geometry geometry2;
	int success;
	floperr_t err;

	geometry1 = floppy_create_tag(floppy, TI99DSK_TAG, sizeof(*geometry1));

	ti99_guess_geometry(floppy, geometry1, NULL, &success);
	if (! success)
		return FLOPPY_ERROR_INVALIDIMAGE;

	memset(&geometry2, 0, sizeof(geometry2));
	geometry2.heads = geometry1->sides;
	geometry2.tracks = geometry1->tracksperside;
	geometry2.sectors = geometry1->secspertrack;
	geometry2.sector_length = 256;
	geometry2.translate_sector = NULL;
	geometry2.translate_offset = ti99_translate_offset;

	err = basicdsk_construct(floppy, &geometry2);
	if (err)
		return err;

	return FLOPPY_ERROR_SUCCESS;
}

FLOPPY_OPTIONS_START( ti99 )
	FLOPPY_OPTION( ti99, "dsk",	"TI99 disk image",	ti99_floppy_identify,	ti99_floppy_construct, NULL)
FLOPPY_OPTIONS_END

static void ti99_install_tracktranslate_procs(running_machine *machine)
{
	int id;
	const device_config *image;

	for (id=0; id<MAX_FLOPPIES; id++)
	{
		image = floppy_get_device(machine, id);
		floppy_install_tracktranslate_proc(image, ti99_tracktranslate);
	}
}

/*
    callback called whenever DRQ/IRQ state change
*/
static WRITE_LINE_DEVICE_HANDLER( ti99_fdc_intrq_w )
{
	if (state)
	{
		DRQ_IRQ_status |= fdc_IRQ;
		ti99_peb_set_ilb_bit(device->machine, intb_fdc_bit, 1);
	}
	else
	{
		DRQ_IRQ_status &= ~fdc_IRQ;
		ti99_peb_set_ilb_bit(device->machine, intb_fdc_bit, 0);
	}

	fdc_handle_hold(device->machine);
}

static WRITE_LINE_DEVICE_HANDLER( ti99_fdc_drq_w )
{
	if (state)
		DRQ_IRQ_status |= fdc_DRQ;
	else
		DRQ_IRQ_status &= ~fdc_DRQ;

	fdc_handle_hold(device->machine);
}

const wd17xx_interface ti99_wd17xx_interface =
{
	DEVCB_LINE(ti99_fdc_intrq_w),
	DEVCB_LINE(ti99_fdc_drq_w),
	NULL,
	{FLOPPY_0, FLOPPY_1, FLOPPY_2, FLOPPY_3}
};


/*
    Initializes all available controllers. This routine is only used in the
    init state of the emulator. During the normal operation, only the
    reset routines are used.
*/
void ti99_floppy_controllers_init_all(running_machine *machine)
{
	/* initialize the controller chip for HFDC */
	smc92x4_init(0, & hfdc_intf);

	ti99_install_tracktranslate_procs(machine);

	motor_on_timer = timer_alloc(machine, motor_on_timer_callback, NULL);
}

/*===========================================================================*/
/*
    TI99/4(a) Floppy disk controller card emulation
*/

/* prototypes */
static TIMER_CALLBACK(motor_on_timer_callback);
static int fdc_cru_r(running_machine *machine, int offset);
static void fdc_cru_w(running_machine *machine, int offset, int data);
static READ8_HANDLER(fdc_mem_r);
static WRITE8_HANDLER(fdc_mem_w);

/* pointer to the fdc ROM data */
static UINT8 *ti99_disk_DSR;

static const ti99_peb_card_handlers_t fdc_handlers =
{
	fdc_cru_r,
	fdc_cru_w,
	fdc_mem_r,
	fdc_mem_w
};

/*
    Reset fdc card, set up handlers
*/
void ti99_fdc_reset(running_machine *machine)
{
	const device_config *fdc = devtag_get_device(machine, "wd179x");
	ti99_disk_DSR = memory_region(machine, region_dsr) + offset_fdc_dsr;
	DSEL = 0;
	DSKnum = -1;
	DSKside = 0;

	DVENA = 0;
	motor_on = 0;

	use_80_track_drives = FALSE;

	ti99_peb_set_card_handlers(0x1100, & fdc_handlers);
	wd17xx_reset(fdc);		/* resets the fdc */
	wd17xx_set_density(fdc,DEN_FM_LO);
}

/*
    Read disk CRU interface

    bit 0: HLD pin
    bit 1-3: drive n active
    bit 4: 0: motor strobe on
    bit 5: always 0
    bit 6: always 1
    bit 7: selected side
*/
static int fdc_cru_r(running_machine *machine, int offset)
{
	int reply;

	switch (offset)
	{
	case 0:
		reply = 0x40;
		if (DSKside)
			reply |= 0x80;
		if (DVENA)
			reply |= (DSEL << 1);
		if (! DVENA)
			reply |= 0x10;
		break;

	default:
		reply = 0;
		break;
	}

	return reply;
}

/*
    Write disk CRU interface
*/
static void fdc_cru_w(running_machine *machine, int offset, int data)
{
	const device_config *fdc = devtag_get_device(machine, "wd179x");

	switch (offset)
	{
	case 0:
		/* WRITE to DISK DSR ROM bit (bit 0) */
		/* handled in xxx_peb_cru_w() */
		break;

	case 1:
		/* Strobe motor (motor is on for 4.23 sec) */
		if (data && !motor_on)
		{	/* on rising edge, set DVENA for 4.23s */
			DVENA = 1;
			fdc_handle_hold(machine);
			timer_adjust_oneshot(motor_on_timer, ATTOTIME_IN_MSEC(4230), 0);
		}
		motor_on = data;
		break;

	case 2:
		/* Set disk ready/hold (bit 2)
            0: ignore IRQ and DRQ
            1: TMS9900 is stopped until IRQ or DRQ are set (OR the motor stops rotating - rotates
              for 4.23s after write to revelant CRU bit, this is not emulated and could cause
              the TI99 to lock...) */
		DSKhold = data;
		fdc_handle_hold(machine);
		break;

	case 3:
		/* Load disk heads (HLT pin) (bit 3) */
		/* ... */
		break;

	case 4:
	case 5:
	case 6:
		/* Select drive X (bits 4-6) */
		{
			int drive = offset-4;					/* drive # (0-2) */

			if (data)
			{
				DSEL |= 1 << drive;

				if (drive != DSKnum)			/* turn on drive... already on ? */
				{
					DSKnum = drive;

					wd17xx_set_drive(fdc,DSKnum);
					/*wd17xx_set_side(DSKside);*/
				}
			}
			else
			{
				DSEL &= ~ (1 << drive);

				if (drive == DSKnum)			/* geez... who cares? */
				{
					DSKnum = -1;				/* no drive selected */
				}
			}
		}
		break;

	case 7:
		/* Select side of disk (bit 7) */
		DSKside = data;
		wd17xx_set_side(fdc,DSKside);
		break;
	}
}


/*
    read a byte in disk DSR space
*/
static  READ8_HANDLER(fdc_mem_r)
{
	const device_config *fdc = devtag_get_device(space->machine, "wd179x");

	switch (offset)
	{
	case 0x1FF0:					/* Status register */
		return (wd17xx_status_r(fdc, offset) ^ 0xFF);
	case 0x1FF2:					/* Track register */
		return wd17xx_track_r(fdc, offset) ^ 0xFF;
	case 0x1FF4:					/* Sector register */
		return wd17xx_sector_r(fdc, offset) ^ 0xFF;
	case 0x1FF6:					/* Data register */
		return wd17xx_data_r(fdc, offset) ^ 0xFF;
	default:						/* DSR ROM */
		return ti99_disk_DSR[offset];
	}
}

/*
    write a byte in disk DSR space
*/
static WRITE8_HANDLER(fdc_mem_w)
{
	const device_config *fdc = devtag_get_device(space->machine, "wd179x");

	data ^= 0xFF;	/* inverted data bus */

	switch (offset)
	{
	case 0x1FF8:					/* Command register */
		wd17xx_command_w(fdc, offset, data);
		break;
	case 0x1FFA:					/* Track register */
		wd17xx_track_w(fdc, offset, data);
		break;
	case 0x1FFC:					/* Sector register */
		wd17xx_sector_w(fdc, offset, data);
		break;
	case 0x1FFE:					/* Data register */
		wd17xx_data_w(fdc, offset, data);
		break;
	}
}


#if HAS_99CCFDC
/*===========================================================================*/
/*
    Alternate fdc: CorcComp FDC

    Advantages:
    * this card supports Double Density.
    * this card support an additional floppy drive, for a total of 4 floppies.

    References:
    * ???
*/

/* prototypes */
static int ccfdc_cru_r(running_machine *machine, int offset);
static void ccfdc_cru_w(running_machine *machine, int offset, int data);
static READ8_HANDLER(ccfdc_mem_r);
static WRITE8_HANDLER(ccfdc_mem_w);

static const ti99_peb_card_handlers_t ccfdc_handlers =
{
	ccfdc_cru_r,
	ccfdc_cru_w,
	ccfdc_mem_r,
	ccfdc_mem_w
};

#if HAS_99CCFDC
void ti99_ccfdc_reset(running_machine *machine)
{
	const device_config *fdc = devtag_get_device(machine, "wd179x");

	ti99_disk_DSR = memory_region(machine, region_dsr) + offset_ccfdc_dsr;
	DSEL = 0;
	DSKnum = -1;
	DSKside = 0;

	DVENA = 0;
	motor_on = 0;

	use_80_track_drives = FALSE;

	ti99_peb_set_card_handlers(0x1100, & ccfdc_handlers);

	wd17xx_reset(fdc);		/* initialize the floppy disk controller */
	wd17xx_set_density(fdc,DEN_MFM_LO);
}
#endif


/*
    Read disk CRU interface

    bit 0: drive 4 active (not emulated)
    bit 1-3: drive n active
    bit 4-7: dip switches 1-4
*/
static int ccfdc_cru_r(int offset)
{
	int reply;

	switch (offset)
	{
	case 0:
		/* CRU bits: beware, logic levels of DIP-switches are inverted  */
		reply = 0x50;
		if (DVENA)
			reply |= ((DSEL << 1) | (DSEL >> 3)) & 0x0f;
		break;

	default:
		reply = 0;
		break;
	}

	return reply;
}


/*
    Write disk CRU interface
*/
static void ccfdc_cru_w(running_machine *machine, int offset, int data)
{
	const device_config *fdc = devtag_get_device(machine, "wd179x");

	switch (offset)
	{
	case 0:
		/* WRITE to DISK DSR ROM bit (bit 0) */
		/* handled in xxx_peb_cru_w() */
		break;

	case 1:
		/* Strobe motor (motor is on for 4.23 sec) */
		if (data && !motor_on)
		{	/* on rising edge, set DVENA for 4.23s */
			DVENA = 1;
			fdc_handle_hold(machine);
			timer_adjust_oneshot(motor_on_timer, ATTOTIME_IN_MSEC(4230), 0);
		}
		motor_on = data;
		break;

	case 2:
		/* Set disk ready/hold (bit 2)
            0: ignore IRQ and DRQ
            1: TMS9900 is stopped until IRQ or DRQ are set (OR the motor stops rotating - rotates
              for 4.23s after write to revelant CRU bit, this is not emulated and could cause
              the TI99 to lock...) */
		DSKhold = data;
		fdc_handle_hold(machine);
		break;

	case 4:
	case 5:
	case 6:
	case 8:
		/* Select drive 0-2 (DSK1-DSK3) (bits 4-6) */
		/* Select drive 3 (DSK4) (bit 8) */
		{
			int drive = (offset == 8) ? 3 : (offset-4);		/* drive # (0-3) */

			if (data)
			{
				DSEL |= 1 << drive;

				if (drive != DSKnum)			/* turn on drive... already on ? */
				{
					DSKnum = drive;

					wd17xx_set_drive(fdc,DSKnum);
					/*wd17xx_set_side(fdc,DSKside);*/
				}
			}
			else
			{
				DSEL &= ~ (1 << drive);

				if (drive == DSKnum)			/* geez... who cares? */
				{
					DSKnum = -1;				/* no drive selected */
				}
			}
		}
		break;

	case 7:
		/* Select side of disk (bit 7) */
		DSKside = data;
		wd17xx_set_side(fdc,DSKside);
		break;

	case 10:
		/* double density enable (active low) */
		wd17xx_set_density(fdc,data ? DEN_FM_LO : DEN_MFM_LO);
		break;

	case 11:
		/* EPROM A13 */
		break;

	case 13:
		/* RAM A10 */
		break;

	case 14:
		/* override FDC with RTC (active high) */
		break;

	case 15:
		/* EPROM A14 */
		break;

	case 3:
	case 9:
	case 12:
		/* Unused (bit 3, 9 & 12) */
		break;
	}
}


/*
    read a byte in disk DSR space
*/
static READ8_HANDLER(ccfdc_mem_r)
{
	int reply = 0;

	reply = ti99_disk_DSR[offset];

	return reply;
}

/*
    write a byte in disk DSR space
*/
static WRITE8_HANDLER(ccfdc_mem_w)
{
}
#endif


/*===========================================================================*/
/*
    Alternate fdc: BwG card from SNUG

    Advantages:
    * this card supports Double Density.
    * as this card includes its own RAM, it does not need to allocate a portion
      of VDP RAM to store I/O buffers.
    * this card includes a MM58274C RTC.
    * this card support an additional floppy drive, for a total of 4 floppies.

    Reference:
    * BwG Disketten-Controller: Beschreibung der DSR
        <http://home.t-online.de/home/harald.glaab/snug/bwg.pdf>
*/

/* prototypes */
static int bwg_cru_r(running_machine *machine, int offset);
static void bwg_cru_w(running_machine *machine, int offset, int data);
static READ8_HANDLER(bwg_mem_r);
static WRITE8_HANDLER(bwg_mem_w);

static const ti99_peb_card_handlers_t bwg_handlers =
{
	bwg_cru_r,
	bwg_cru_w,
	bwg_mem_r,
	bwg_mem_w
};

static int bwg_rtc_enable;
static int bwg_ram_offset;
static int bwg_rom_offset;
static UINT8 *bwg_ram;


/*
    Reset fdc card, set up handlers
*/
void ti99_bwg_reset(running_machine *machine)
{
	const device_config *fdc = devtag_get_device(machine, "wd179x");

	ti99_disk_DSR = memory_region(machine, region_dsr) + offset_bwg_dsr;
        bwg_ram = memory_region(machine, region_dsr) + offset_bwg_ram;
	bwg_ram_offset = 0;
	bwg_rom_offset = 0;
	bwg_rtc_enable = 0;

	DSEL = 0;
	DSKnum = -1;
	DSKside = 0;

	DVENA = 0;
	motor_on = 0;

     	use_80_track_drives = FALSE;

	ti99_peb_set_card_handlers(0x1100, & bwg_handlers);

	wd17xx_reset(fdc);		/* initialize the floppy disk controller */
	wd17xx_set_density(fdc,DEN_MFM_LO);
}

/*
    Read disk CRU interface

    bit 0: drive 4 active (not emulated)
    bit 1-3: drive n active
    bit 4-7: dip switches 1-4
*/
static int bwg_cru_r(running_machine *machine, int offset)
{
	int reply;

	switch (offset)
	{
	case 0:
		/* CRU bits: beware, logic levels of DIP-switches are inverted  */
		reply = 0x50;
		if (DVENA)
			reply |= ((DSEL << 1) | (DSEL >> 3)) & 0x0f;
		break;

	default:
		reply = 0;
		break;
	}

	return reply;
}


/*
    Write disk CRU interface
*/
static void bwg_cru_w(running_machine *machine, int offset, int data)
{
	const device_config *fdc = devtag_get_device(machine, "wd179x");

	switch (offset)
	{
	case 0:
		/* WRITE to DISK DSR ROM bit (bit 0) */
		/* handled in xxx_peb_cru_w() */
		break;

	case 1:
		/* Strobe motor (motor is on for 4.23 sec) */
		if (data && !motor_on)
		{	/* on rising edge, set DVENA for 4.23s */
			DVENA = 1;
			fdc_handle_hold(machine);
			timer_adjust_oneshot(motor_on_timer, ATTOTIME_IN_MSEC(4230), 0);
		}
		motor_on = data;
		break;

	case 2:
		/* Set disk ready/hold (bit 2)
            0: ignore IRQ and DRQ
            1: TMS9900 is stopped until IRQ or DRQ are set (OR the motor stops rotating - rotates
              for 4.23s after write to revelant CRU bit, this is not emulated and could cause
              the TI99 to lock...) */
		DSKhold = data;
		fdc_handle_hold(machine);
		break;

	case 4:
	case 5:
	case 6:
	case 8:
		/* Select drive 0-2 (DSK1-DSK3) (bits 4-6) */
		/* Select drive 3 (DSK4) (bit 8) */
		{
			int drive = (offset == 8) ? 3 : (offset-4);		/* drive # (0-3) */

			if (data)
			{
				DSEL |= 1 << drive;

				if (drive != DSKnum)			/* turn on drive... already on ? */
				{
					DSKnum = drive;

					wd17xx_set_drive(fdc,DSKnum);
					/*wd17xx_set_side(fdc,DSKside);*/
				}
			}
			else
			{
				DSEL &= ~ (1 << drive);

				if (drive == DSKnum)			/* geez... who cares? */
				{
					DSKnum = -1;				/* no drive selected */
				}
			}
		}
		break;

	case 7:
		/* Select side of disk (bit 7) */
		DSKside = data;
		wd17xx_set_side(fdc,DSKside);
		break;

	case 10:
		/* double density enable (active low) */
		wd17xx_set_density(fdc,data ? DEN_FM_LO : DEN_MFM_LO);
		break;

	case 11:
		/* EPROM A13 */
		if (data)
			bwg_rom_offset |= 0x2000;
		else
			bwg_rom_offset &= ~0x2000;
		break;

	case 13:
		/* RAM A10 */
		if (data)
			bwg_ram_offset = 0x0400;
		else
			bwg_ram_offset = 0x0000;
		break;

	case 14:
		/* override FDC with RTC (active high) */
		bwg_rtc_enable = data;
		break;

	case 15:
		/* EPROM A14 */
		if (data)
			bwg_rom_offset |= 0x4000;
		else
			bwg_rom_offset &= ~0x4000;
		break;

	case 3:
	case 9:
	case 12:
		/* Unused (bit 3, 9 & 12) */
		break;
	}
}


/*
    read a byte in disk DSR space
*/
static  READ8_HANDLER(bwg_mem_r)
{
	const device_config *fdc = devtag_get_device(space->machine, "wd179x");

	int reply = 0;

	if (offset < 0x1c00)
		reply = ti99_disk_DSR[bwg_rom_offset+offset];
	else if (offset < 0x1fe0)
		reply = bwg_ram[bwg_ram_offset+(offset-0x1c00)];
	else if (bwg_rtc_enable)
	{
		if (! (offset & 1))
			reply = mm58274c_r(devtag_get_device(space->machine, "mm58274c_floppy"), (offset - 0x1FE0) >> 1);
	}
	else
	{
		if (offset < 0x1ff0)
			reply = bwg_ram[bwg_ram_offset+(offset-0x1c00)];
		else
			switch (offset)
			{
			case 0x1FF0:					/* Status register */
				reply = wd17xx_status_r(fdc, offset);
				break;
			case 0x1FF2:					/* Track register */
				reply = wd17xx_track_r(fdc, offset);
				break;
			case 0x1FF4:					/* Sector register */
				reply = wd17xx_sector_r(fdc, offset);
				break;
			case 0x1FF6:					/* Data register */
				reply = wd17xx_data_r(fdc, offset);
				break;
			default:
				reply = 0;
				break;
			}
	}
	return reply;
}

/*
    write a byte in disk DSR space
*/
static WRITE8_HANDLER(bwg_mem_w)
{
	const device_config *fdc = devtag_get_device(space->machine, "wd179x");

	if (offset < 0x1c00)
		;
	else if (offset < 0x1fe0)
		bwg_ram[bwg_ram_offset+(offset-0x1c00)] = data;
	else if (bwg_rtc_enable)
	{
		if (! (offset & 1))
			mm58274c_w(devtag_get_device(space->machine, "mm58274c_floppy"), (offset - 0x1FE0) >> 1, data);
	}
	else
	{
		if (offset < 0x1ff0)
		{
			bwg_ram[bwg_ram_offset+(offset-0x1c00)] = data;
		}
		else
		{
			switch (offset)
			{
			case 0x1FF8:					/* Command register */
				wd17xx_command_w(fdc, offset, data);
				break;
			case 0x1FFA:					/* Track register */
				wd17xx_track_w(fdc, offset, data);
				break;
			case 0x1FFC:					/* Sector register */
				wd17xx_sector_w(fdc, offset, data);
				break;
			case 0x1FFE:					/* Data register */
				wd17xx_data_w(fdc, offset, data);
				break;
			}
		}
	}
}


/*===========================================================================*/
/*
    Alternate fdc: HFDC card built by Myarc

    Advantages: same as BwG, plus:
    * high density support (only on upgraded cards, I think)
    * hard disk support (only prehistoric mfm hard disks are supported, though)
    * DMA support (I think the DMA controller can only access the on-board RAM)

    This card includes a MM58274C RTC and a 9234 HFDC with its various support
    chips.

    Reference:
    * hfdc manual
        <ftp://ftp.whtech.com//datasheets & manuals/Hardware manuals/hfdc manual.max>
*/

/* prototypes */
static int hfdc_cru_r(running_machine *machine, int offset);
static void hfdc_cru_w(running_machine *machine, int offset, int data);
static  READ8_HANDLER(hfdc_mem_r);
static WRITE8_HANDLER(hfdc_mem_w);

static const ti99_peb_card_handlers_t hfdc_handlers =
{
	hfdc_cru_r,
	hfdc_cru_w,
	hfdc_mem_r,
	hfdc_mem_w
};

static int hfdc_rtc_enable;
static int hfdc_ram_offset[3];
static int hfdc_rom_offset;
static int cru_sel;
static UINT8 *hfdc_ram;
static int hfdc_irq_state;

/*
    Select the correct HFDC disk units.
    floppy disks are selected by the 4 gpos instead of the select lines.
*/
static int hfdc_select_callback(int which, select_mode_t select_mode, int select_line, int gpos)
{
	int disk_unit;

	(void) which;

	switch (select_mode)
	{
	/*case sm_at_harddisk:*/
	case sm_harddisk:
		/* hard disk */
		disk_unit = select_line - 1;
		break;

	case sm_floppy_slow:
	case sm_floppy_fast:
		/* floppy disk */
		/* We use the 4 general purpose output as select lines in order to
        support 4 drives. */
		switch (gpos & 0xf)
		{
		case 1:
			disk_unit = 0;
			break;
		case 2:
			disk_unit = 1;
			break;
		case 4:
			disk_unit = 2;
			break;
		case 8:
			disk_unit = 3;
			break;
		default:
			disk_unit = -1;
			break;
		}
		break;

	default:
		disk_unit = -1;
		break;
	}

	return disk_unit;
}

/*
    Read a byte from buffer in DMA mode
*/
static UINT8 hfdc_dma_read_callback(int which, offs_t offset)
{
	(void) which;
	return hfdc_ram[offset & 0x7fff];
}

/*
    Write a byte to buffer in DMA mode
*/
static void hfdc_dma_write_callback(int which, offs_t offset, UINT8 data)
{
	(void) which;
	hfdc_ram[offset & 0x7fff] = data;
}

/*
    Called whenever the state of the sms9234 interrupt pin changes.
*/
static void hfdc_int_callback(int which, int state)
{
	assert(which == 0);

	hfdc_irq_state = state;
}

/*
    Reset fdc card, set up handlers
*/
void ti99_hfdc_reset(running_machine *machine)
{
	ti99_disk_DSR = memory_region(machine, region_dsr) + offset_hfdc_dsr;
	hfdc_ram = memory_region(machine, region_dsr) + offset_hfdc_ram;
	hfdc_ram_offset[0] = hfdc_ram_offset[1] = hfdc_ram_offset[2] = 0;
	hfdc_rom_offset = 0;
	hfdc_rtc_enable = 0;

	DSEL = 0;
	DSKnum = -1;
	DSKside = 0;

	DVENA = 0;
	motor_on = 0;

	ti99_peb_set_card_handlers(0x1100, & hfdc_handlers);

	use_80_track_drives = TRUE;

	smc92x4_reset(0);
}


/*
    Read disk CRU interface
*/
static int hfdc_cru_r(running_machine *machine, int offset)
{
	int reply;
	switch (offset)
	{
	case 0:
		/* CRU bits */
		if (cru_sel)
			/* DIP switches.  Logic levels are inverted (on->0, off->1).  CRU
            bit order is the reverse of DIP-switch order, too (dip 1 -> bit 7,
            dip 8 -> bit 0).  Return value examples:
                ff -> 4 slow 40-track DD drives
                55 -> 4 fast 40-track DD drives
                aa -> 4 80-track DD drives
                00 -> 4 80-track HD drives */
			reply = use_80_track_drives ? 0x00 : 0x55;
		else
		{
			reply = 0;
			if (hfdc_irq_state)
				reply |= 1;
			if (motor_on)
				reply |= 2;
			/*if (hfdc_dma_in_progress)
                reply |= 4;*/
		}
		break;

	default:
		reply = 0;
		break;
	}

	return reply;
}


/*
    Write disk CRU interface
*/
static void hfdc_cru_w(running_machine *machine, int offset, int data)
{
	switch (offset)
	{
	case 0:
		/* WRITE to DISK DSR ROM bit (bit 0) */
		/* handled in xxx_peb_cru_w() */
		break;

	case 1:
		/* reset fdc (active low) */
		if (data)
		{
			smc92x4_reset(0);
			logerror("CRU sel %d\n", cru_sel);
		}
		break;

	case 2:
		/* Strobe motor + density ("CD0 OF 9216 CHIP") */
		if (data && !motor_on)
		{
			DVENA = 1;
			fdc_handle_hold(machine);
			timer_adjust_oneshot(motor_on_timer, ATTOTIME_IN_MSEC(4230), 0);
		}
		motor_on = data;
		break;

	case 3:
		/* rom page select 0 + density ("CD1 OF 9216 CHIP") */
		if (data)
			hfdc_rom_offset |= 0x2000;
		else
			hfdc_rom_offset &= ~0x2000;
		break;

	case 4:
		/* rom page select 1 + cru sel */
		if (data)
			hfdc_rom_offset |= 0x1000;
		else
			hfdc_rom_offset &= ~0x1000;
		cru_sel = data;
		logerror("CRU sel %d\n", cru_sel);
		break;

	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
	case 16:
	case 17:
	case 18:
	case 19:
	case 20:
	case 21:
	case 22:
	case 23:
		/* ram page select */
		if (data)
			hfdc_ram_offset[(offset-9)/5] |= 0x400 << ((offset-9)%5);
		else
			hfdc_ram_offset[(offset-9)/5] &= ~(0x400 << ((offset-9)%5));
		break;
	}
}


/*
    read a byte in disk DSR space
*/
static  READ8_HANDLER(hfdc_mem_r)
{
	int reply = 0;

	if (offset < 0x0fc0)
	{
		/* dsr ROM */
		reply = ti99_disk_DSR[hfdc_rom_offset+offset];
	}
	else if (offset < 0x0fd0)
	{
		/* tape controller */
	}
	else if (offset < 0x0fe0)
	{
		/* disk controller */
		switch (offset)
		{
		case 0x0fd0:
			reply = smc92x4_r(space->machine,0, 0);
			//logerror("hfdc9234 data read\n");
			break;

		case 0x0fd4:
			reply = smc92x4_r(space->machine,0, 1);
			//logerror("hfdc9234 status read\n");
			break;
		}
		if (offset >= 0x0fd8)
			logerror("hfdc9234 read %d\n", offset);
	}
	else if (offset < 0x1000)
	{
		/* rtc */
		if (! (offset & 1))
			reply = mm58274c_r(devtag_get_device(space->machine, "mm58274c_floppy"), (offset - 0x1FE0) >> 1);
	}
	else if (offset < 0x1400)
	{
		/* ram page >08 (hfdc manual says >10) */
		reply = hfdc_ram[0x2000+(offset-0x1000)];
	}
	else
	{
		/* ram with mapper */
		reply = hfdc_ram[hfdc_ram_offset[(offset-0x1400) >> 10]+((offset-0x1000) & 0x3ff)];
	}
	return reply;
}

/*
    write a byte in disk DSR space
*/
static WRITE8_HANDLER(hfdc_mem_w)
{
	if (offset < 0x0fc0)
	{
		/* dsr ROM */
	}
	else if (offset < 0x0fd0)
	{
		/* tape controller */
	}
	else if (offset < 0x0fe0)
	{
		/* disk controller */
		switch (offset)
		{
		case 0x0fd2:
			smc92x4_w(space->machine,0, 0, data);
			//logerror("hfdc9234 data write %d\n", data);
			break;

		case 0x0fd6:
			smc92x4_w(space->machine,0, 1, data);
			//logerror("hfdc9234 command write %d\n", data);
			break;
		}
		if (offset >= 0x0fd8)
			logerror("hfdc9234 write %d %d\n", offset, data);
	}
	else if (offset < 0x1000)
	{
		/* rtc */
		if (! (offset & 1))
			mm58274c_w(devtag_get_device(space->machine, "mm58274c_floppy"), (offset - 0x1FE0) >> 1, data);
	}
	else if (offset < 0x1400)
	{
		/* ram page >08 (hfdc manual says >10) */
		hfdc_ram[0x2000+(offset-0x1000)] = data;
	}
	else
	{
		/* ram with mapper */
		hfdc_ram[hfdc_ram_offset[(offset-0x1400) >> 10]+((offset-0x1000) & 0x3ff)] = data;
	}
}

