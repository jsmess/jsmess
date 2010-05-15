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

    CHANGES

    * Removed format definition and put it in separate file (ti99_dsk)
      Michael Zapf, Feb 2010
*/

#include "emu.h"
#include "wd17xx.h"
#include "smc92x4.h"
#include "ti99_4x.h"  /* required for memory region offsets */
#include "99_peb.h"
#include "99_dsk.h"
#include "mm58274c.h"
#include "devices/flopdrv.h"
#include "devices/harddriv.h"
#include "devices/ti99_hd.h"
#include "harddisk.h"
#include "formats/ti99_dsk.h"

#define MAX_FLOPPIES 4

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
static emu_timer *motor_on_timer;

/*
    Resets the drive geometry. This is required because the heuristic of
    the default implementation sets the drive geometry to the geometry
    of the medium.
*/
static void set_geometry(running_device *drive, floppy_type_t type)
{
	if (drive!=NULL)
		floppy_drive_set_geometry(drive, type);
	else
		logerror("Drive not found\n");
}

static void set_all_geometries(running_machine *machine, floppy_type_t type)
{
	set_geometry(devtag_get_device(machine, FLOPPY_0), type);
	set_geometry(devtag_get_device(machine, FLOPPY_1), type);
	set_geometry(devtag_get_device(machine, FLOPPY_2), type);
	set_geometry(devtag_get_device(machine, FLOPPY_3), type);
}

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
	DEVCB_NULL,
	DEVCB_LINE(ti99_fdc_intrq_w),
	DEVCB_LINE(ti99_fdc_drq_w),
	{FLOPPY_0, FLOPPY_1, FLOPPY_2, FLOPPY_3}
};

/*
    Initializes all available controllers. This routine is only used in the
    init state of the emulator. During the normal operation, only the
    reset routines are used.
*/
// TODO: Check that: 	floppy_mon_w(w->drive, CLEAR_LINE);

void ti99_floppy_controllers_init_all(running_machine *machine)
{
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
	running_device *fdc = devtag_get_device(machine, "wd179x");
	ti99_disk_DSR = memory_region(machine, region_dsr) + offset_fdc_dsr;
	DSEL = 0;
	DSKnum = -1;
	DSKside = 0;

	DVENA = 0;
	motor_on = 0;

	ti99_set_80_track_drives(FALSE);
	
	set_all_geometries(machine, FLOPPY_DRIVE_DS_40);
	
	ti99_peb_set_card_handlers(0x1100, & fdc_handlers);
	if (fdc) {
		wd17xx_reset(fdc);		/* resets the fdc */
		wd17xx_dden_w(fdc, ASSERT_LINE);
	}
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
	running_device *fdc = devtag_get_device(machine, "wd179x");

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
	running_device *fdc = devtag_get_device(space->machine, "wd179x");
	
	/* only use the even addresses from 1ff0 to 1ff6. 
	   Note that data is inverted. */
	if (offset >= 0x1ff0 && (offset & 0x09)==0)
		return wd17xx_r(fdc, (offset >> 1)&0x03) ^ 0xff;
	else
		return ti99_disk_DSR[offset];
}

/*
    write a byte in disk DSR space
*/
static WRITE8_HANDLER(fdc_mem_w)
{
	running_device *fdc = devtag_get_device(space->machine, "wd179x");

	/* only use the even addresses from 1ff8 to 1ffe. 
	   Note that data is inverted. */
	if (offset >= 0x1ff0 && (offset & 0x09)==0x08)
		wd17xx_w(fdc, (offset >> 1)&0x03, data ^ 0xff);
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
	running_device *fdc = devtag_get_device(machine, "wd179x");

	ti99_disk_DSR = memory_region(machine, region_dsr) + offset_ccfdc_dsr;
	DSEL = 0;
	DSKnum = -1;
	DSKside = 0;

	DVENA = 0;
	motor_on = 0;

	ti99_set_80_track_drives(FALSE);
	
	set_all_geometries(machine, FLOPPY_DRIVE_DS_40);

	ti99_peb_set_card_handlers(0x1100, & ccfdc_handlers);

	wd17xx_reset(fdc);		/* initialize the floppy disk controller */
	wd17xx_dden(fdc, CLEAR_LINE);
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
	running_device *fdc = devtag_get_device(machine, "wd179x");

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
		wd17xx_dden(fdc, data ? ASSERT_LINE : CLEAR_LINE);
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
	running_device *fdc = devtag_get_device(machine, "wd179x");

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

	ti99_set_80_track_drives(FALSE);
	
	set_all_geometries(machine, FLOPPY_DRIVE_DS_40);

	ti99_peb_set_card_handlers(0x1100, & bwg_handlers);

	wd17xx_reset(fdc);		/* initialize the floppy disk controller */
	wd17xx_dden_w(fdc, CLEAR_LINE);
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
	running_device *fdc = devtag_get_device(machine, "wd179x");

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
		wd17xx_dden_w(fdc, data ? ASSERT_LINE : CLEAR_LINE);
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
	running_device *fdc = devtag_get_device(space->machine, "wd179x");

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
	running_device *fdc = devtag_get_device(space->machine, "wd179x");

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

    For smc92x4 (HFDC-internal DSR)
    FM:  gap0=3, gap1=26, gap2=11, gap3=27, gap4=42, sync=6, count=9, size=2, fm=16, gap_byte=ff
    MFM: gap0=2, gap1=30, gap2=22, gap3=20, gap4=42, sync=12, count=18, size=2, fm=0, gap_byte=4e
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

#define HFDC_MAX_FLOPPY 4
#define HFDC_MAX_HARD 4

static int hfdc_rtc_enable;
static int hfdc_ram_offset[3];
static int hfdc_rom_offset;
static int cru_sel;
static UINT8 *hfdc_ram;

static int hfdc_irq_state;
static int hfdc_dip_state;

static running_device *floppy_unit[4];
static running_device *harddisk_unit[4];

static UINT8 output1_latch;
static UINT8 output2_latch;

static int dma_address;

static int line_to_index(UINT8 line, int max)
{
	int index = 0;
	while (index < max)
	{
		if ((line & 0x01)!=0)
			break;
		else
		{
			line = line >> 1;
			index++;
		}
	}
	if (index==max) return -1;
	return index;
}

/*
	Select the HFDC hard disk unit
	This function implements the selection logic on the PCB (outside the
	controller chip)
	The HDC9234 allows up to four drives to be selected by the select lines.
	This is not enough for controlling four floppy drives and three hard 
	drives, so the user programmable outputs are used to select the floppy
	drives. This selection happens completely outside of the controller,
	so we must implement it here.
*/

static running_device *hfdc_current_harddisk(void)
{
	int disk_unit = -1;
	disk_unit = line_to_index((output1_latch>>4)&0x0f, HFDC_MAX_HARD);

	/* The HFDC does not use disk_unit=0. The first HD has index 1. */
/*	if (disk_unit>=0) printf("Select hard disk %d\n", disk_unit);
	else printf("No hard disk selected\n");
*/
	return harddisk_unit[disk_unit];
}

/*
	Select the HFDC floppy disk unit
	This function implements the selection logic on the PCB (outside the
	controller chip)
*/

static running_device *hfdc_current_floppy(void)
{	
	int disk_unit = -1;
	disk_unit = line_to_index(output1_latch & 0x0f, HFDC_MAX_FLOPPY);

	if (disk_unit<0)
	{
	//	printf("No unit selected\n");
		return NULL;
	}

	// printf("Select floppy disk %d (output1=%02x)\n", index, output1_latch);	
	return floppy_unit[disk_unit];
}	

/*
    Called whenever the state of the sms9234 interrupt pin changes.
*/
static WRITE_LINE_DEVICE_HANDLER( ti99_hfdc_intrq_w )
{
	hfdc_irq_state = state;

	/* Set INTA */
	if (state)
	{
		ti99_peb_set_ila_bit(device->machine, intb_fdc_bit, 1);
	}
	else
	{
		ti99_peb_set_ila_bit(device->machine, intb_fdc_bit, 0);
	}

	/* TODO: Check */
	fdc_handle_hold(device->machine);
}

/*
    Called whenever the state of the sms9234 DMA in progress changes.
*/
static WRITE_LINE_DEVICE_HANDLER( ti99_hfdc_dip_w )
{
	hfdc_dip_state = state;
}

static WRITE8_DEVICE_HANDLER( ti99_hfdc_auxbus_out )
{
	switch (offset)
	{
	case INPUT_STATUS:
		logerror("99_dsk: Invalid operation: S0=S1=0, but tried to write (expected: read drive status)\n");
		return;
	case OUTPUT_DMA_ADDR:
		/* Value is dma address byte. Shift previous contents to the left. */
		dma_address = ((dma_address << 8) + (data&0xff))&0xffffff;
		break;
	case OUTPUT_OUTPUT1:
		/* value is output1 */
		output1_latch = data;
		break;
	case OUTPUT_OUTPUT2:
		/* value is output2 */
		output2_latch = data;
		break;
	}
}

static READ_LINE_DEVICE_HANDLER( ti99_hfdc_auxbus_in )
{
	running_device *drive;
	UINT8 reply = 0;

	if (output1_latch==0)
	{
		// printf("Neither floppy nor hard disk selected.\n");
		return 0; /* is that the correct default value? */
	}

	/* If a floppy is selected, we have one line set among the four programmable outputs. */
	if ((output1_latch & 0x0f)!=0)
	{
		drive = hfdc_current_floppy();

		/* Get floppy status. */
		if (floppy_drive_get_flag_state(drive, FLOPPY_DRIVE_INDEX) == FLOPPY_DRIVE_INDEX)
			reply |= DS_INDEX;
		if (floppy_tk00_r(drive) == CLEAR_LINE)
			reply |= DS_TRK00;
		if (floppy_wpt_r(drive) == CLEAR_LINE)
			reply |= DS_WRPROT;

		/* if (image_exists(disk_img)) */  

		reply |= DS_READY;  /* Floppies don't have a READY line; line is pulled up */
		reply |= DS_SKCOM;  /* Same here. */
	}
	else /* one of the first four lines must be selected */
	{
		UINT8 state;
		drive = hfdc_current_harddisk();
		state = ti99_mfm_harddisk_status(drive);
		if (state & MFMHD_TRACK00) 
			reply |= DS_TRK00;
		if (state & MFMHD_SEEKCOMP)
			reply |= DS_SKCOM;
		if (state & MFMHD_WRFAULT)
			reply |= DS_WRFAULT;
		if (state & MFMHD_INDEX)
			reply |= DS_INDEX;
		if (state & MFMHD_READY)
			reply |= DS_READY;
	}
	return reply;
}

/*
    Read a byte from buffer in DMA mode
*/
static UINT8 hfdc_dma_read_callback(void)
{
	UINT8 value = hfdc_ram[dma_address & 0x7fff];
	dma_address++;	
	return value;
}

/*
    Write a byte to buffer in DMA mode
*/
static void hfdc_dma_write_callback(UINT8 data)
{
	hfdc_ram[dma_address & 0x7fff] = data;
	dma_address++;
}

/* 
	Preliminary MFM harddisk interface.
*/
static void hfdc_harddisk_get_next_id(int head, chrn_id_hd *id)
{
	running_device *harddisk = hfdc_current_harddisk();
	ti99_mfm_harddisk_get_next_id(harddisk, head, id);
}

static void hfdc_harddisk_seek(int direction)
{
	running_device *harddisk = hfdc_current_harddisk();
	ti99_mfm_harddisk_seek(harddisk, direction);
}

static void hfdc_harddisk_read_sector(int cylinder, int head, int sector, UINT8 **buf, int *sector_length)
{
	running_device *harddisk = hfdc_current_harddisk();
	if (harddisk != NULL)
		ti99_mfm_harddisk_read_sector(harddisk, cylinder, head, sector, buf, sector_length);
}

static void hfdc_harddisk_write_sector(int cylinder, int head, int sector, UINT8 *buf, int sector_length)
{
	running_device *harddisk = hfdc_current_harddisk();
	if (harddisk != NULL)
		ti99_mfm_harddisk_write_sector(harddisk, cylinder, head, sector, buf, sector_length);	
}

static void hfdc_harddisk_read_track(int head, UINT8 **buffer, int *data_count)
{
	running_device *harddisk = hfdc_current_harddisk();
	if (harddisk != NULL)
		ti99_mfm_harddisk_read_track(harddisk, head, buffer, data_count);
}

static void hfdc_harddisk_write_track(int head, UINT8 *buffer, int data_count)
{
	running_device *harddisk = hfdc_current_harddisk();
	if (harddisk != NULL)
		ti99_mfm_harddisk_write_track(harddisk, head, buffer, data_count);
}

/*
    Reset fdc card, set up handlers
*/
void ti99_hfdc_reset(running_machine *machine)
{
	running_device *device = devtag_get_device(machine, "smc92x4");
	const char *flopname[] = {FLOPPY_0, FLOPPY_1, FLOPPY_2, FLOPPY_3};
	const char *hardname[] = {MFMHD_0, MFMHD_1, MFMHD_2};
	
	int i;
	
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

	if ((input_port_read(machine, "DISKCTRL") & DISK_HFDC) 
		&& (input_port_read(machine, "HFDCDIP")&0x55)) 
		ti99_set_80_track_drives(TRUE);
	else
		ti99_set_80_track_drives(FALSE);
	
	smc92x4_set_timing(device, input_port_read(machine, "DRVSPD"));
	
	set_all_geometries(machine, FLOPPY_DRIVE_DS_80);

	/* Connect floppy drives to controller. Note that *this* is the 
	   controller, not the controller chip. The pcb contains a select
	   logic.
	   TODO: Turn this implementation into a device so that this is 
	   correctly modelled.
	*/
	for (i = 0; i < 4; i++)
	{
		if (device->owner != NULL)
		{
			floppy_unit[i] = device->owner->subdevice(flopname[i]);
			if (floppy_unit[i]==NULL)
			{
				floppy_unit[i] = devtag_get_device(device->machine, flopname[i]); 
			}
		}
		else
		{
			floppy_unit[i] = devtag_get_device(device->machine, flopname[i]); 
		}

		if (floppy_unit[i]!=NULL) 
		{
			floppy_drive_set_controller(floppy_unit[i], device);
//			floppy_drive_set_index_pulse_callback(floppy_unit[i], smc92x4_index_pulse_callback);
			floppy_drive_set_rpm(floppy_unit[i], 300.);
		}
		else 
		{
			logerror("99_dsk: Image %s is null\n", flopname[i]);
		}
	}

	/* In the HFDC ROM, WDSx selects drive x; drive 0 is not used */
	for (i = 1; i < 4; i++)
	{
		harddisk_unit[i] = devtag_get_device(device->machine, hardname[i-1]);
	}
	harddisk_unit[0] = NULL;
	
	if (device) {
		smc92x4_reset(device);
	}
}

const smc92x4_interface ti99_smc92x4_interface =
{
	FALSE,		/* do not use the full track layout */
	DEVCB_LINE(ti99_hfdc_intrq_w),
	DEVCB_LINE(ti99_hfdc_dip_w),
	DEVCB_HANDLER(ti99_hfdc_auxbus_out),
	DEVCB_LINE(ti99_hfdc_auxbus_in),
	
	hfdc_current_floppy,
	hfdc_dma_read_callback,
	hfdc_dma_write_callback,

	/* Preliminary MFM harddisk interface. */
	hfdc_harddisk_get_next_id,
	hfdc_harddisk_seek,
	hfdc_harddisk_read_sector,
	hfdc_harddisk_write_sector,
	hfdc_harddisk_read_track,
	hfdc_harddisk_write_track
};

int mycheck = 0;

/*
	Read disk CRU interface
	HFDC manual p. 44
	
	CRU Select=0
	CRU rel. address	Definition
	00			Disk controller interrupt
	02			Motor status
	04			DMA in progress
	
	Note that the cru line settings on the board do not match the
	description. 02 and 04 switch positions. Similarly, the dip switches
	for the drive configurations are not correctly enumerated in the manual.
	
	CRU Select=1
	CRU rel. address	Definition
	00			Switch 4 status \_ drive 2 config
	02			Switch 3 status /
	04			Switch 2 status \_ drive 1 config
	06			Switch 1 status /
	08			Switch 8 status \_ drive 4 config
	0A			Switch 7 status /
	0C			Switch 6 status \_ drive 3 config
	0E			Switch 5 status /
	
	config: 00 = 9/16/18 sectors, 40 tracks, 16 msec
		10 = 9/16/18 sectors, 40 tracks, 8 msec
		01 = 9/16/18 sectors, 80 tracks, 2 msec
		11 = 36 sectors, 80 tracks, 2 msec
*/
static int hfdc_cru_r(running_machine *machine, int offset)
{
	int reply;
	if (offset==0)  /* CRU bits 0-7 */
	{
		/* CRU bits */
		if (cru_sel)
		{
			/* DIP switches.  Logic levels are inverted (on->0, off->1).  CRU
			bit order is the reverse of DIP-switch order, too (dip 1 -> bit 7,
			dip 8 -> bit 0). 
			
			MZ: 00 should not be used since there is a bug in the
			DSR of the HFDC which causes problems with SD disks
			(controller tries DD and then fails to fall back to SD) */
				
			reply = ~(input_port_read(machine, "HFDCDIP"));
//			printf("hfdc cru read 7-0 = %02x\n", reply);
		}
		else
		{
			reply = 0;
/*			if (hfdc_irq_state)
				reply |= 1;
			if (motor_on)
				reply |= 2;
			if (hfdc_dma_in_progress)
				reply |= 4;*/
				
			if (hfdc_irq_state)
				reply |= 0x01;
			if (hfdc_dip_state)
				reply |= 0x02; 
			if (motor_on)
				reply |= 0x04;
//			printf("hfdc cru read 7-0 = %02x\n", reply);

		}
	}
	else /* CRU bits 8+ */
		reply = 0;
//	printf("hfdc cru read %d = %d\n", offset, reply);

	return reply;
}


/*
	Write disk CRU interface
	HFDC manual p. 43
	
	CRU rel. address	Definition				Active
	00			Device Service Routine Select		HIGH
	02			Reset to controller chip		LOW
	04			Floppy Motor on / CD0			HIGH
	06			ROM page select 0 / CD1
	08			ROM page select 1 / CRU input select
	12/4/6/8/A		RAM page select at 0x5400
	1C/E/0/2/4		RAM page select at 0x5800
	26/8/A/C/E		RAM page select at 0x5C00    
	
	CD0 and CD1 are Clock Divider selections for the Floppy Data Separator (FDC9216) 
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
		if (!data)
		{
			running_device *device = devtag_get_device(machine, "smc92x4");
			smc92x4_reset(device);
		}
		break;

	case 2:
		/* Strobe motor and Clock Divider Bit 0 (CD0) */
		if (data && !motor_on)
		{
			DVENA = 1;
			fdc_handle_hold(machine);
			timer_adjust_oneshot(motor_on_timer, ATTOTIME_IN_MSEC(4230), 0);
		}
		motor_on = data;
		//printf("99dsk set motor on / clock divider 0 to %d\n", data);
		break;

	case 3:
		/* rom page select 0 and Clock Divider Bit 1 (CD1) */
		if (data)
			hfdc_rom_offset |= 0x2000;
		else
			hfdc_rom_offset &= ~0x2000;
		//printf("99dsk set rom page 0 / clock divider 1 to %d\n", data);
		break;

	case 4:
		/* rom page select 1 + cru sel */
		if (data)
			hfdc_rom_offset |= 0x1000;
		else
			hfdc_rom_offset &= ~0x1000;
		cru_sel = data;
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
//	printf("hfdc cru write bit %d = %d\n", offset, data);
}


/*
	read a byte in disk DSR space
	HFDC manual, p. 44
	Memory map as seen by the 99/4A PEB
	
	0x4000 - 0x4fbf	one of four possible ROM pages
	0x4fc0 - 0x4fcf	tape select
	0x4fd0 - 0x4fdf	disk controller select
	0x4fe0 - 0x4fff	time chip select
	
	0x5000 - 0x53ff	static RAM page 0x10
	0x5400 - 0x57ff	static RAM page any of 32 pages
	0x5800 - 0x5bff	static RAM page any of 32 pages
	0x5c00 - 0x5fff	static RAM page any of 32 pages    
*/
static  READ8_HANDLER(hfdc_mem_r)
{
	running_device *device = devtag_get_device(space->machine, "smc92x4");

	if (offset >= 0x0000 && offset <= 0x0fbf)
	{
		/* dsr ROM */
		return ti99_disk_DSR[hfdc_rom_offset+offset];
	}
	
	if (offset >= 0xfc0 && offset <= 0x0fcf)
	{
		/* tape controller */
		logerror("99_dsk: Tape support not available (access to offset %04x)\n", offset);
		return 0;
	}

	if (offset >= 0xfd0 && offset <= 0x0fdf)
        {
                /* Disk controller. Note that the odd addresses are ignored and
                   the addresses used for the write ports are also masked away
                   since the write operations in the ti99 system are always 
                   accompanied by read operations before.
                */
                if ((offset & 3) == 0x00) 
                {
               		return smc92x4_r(device, (offset>>2)&1);
                }
                else return 0;
        }
	if (offset >= 0x0fe0 && offset <= 0x0fff)
	{
		/* Real-time clock. Again, ignore odd addresses. */
		if ((offset & 1) == 0)
		{
			running_device *clock = devtag_get_device(space->machine, "mm58274c_floppy");
			return mm58274c_r(clock, (offset - 0x0fe0) >> 1);
		}
		else return 0;
	}

	if (offset >= 0x1000 && offset <= 0x13ff)
	{
		/* ram page >08 (hfdc manual says >10) */
		return hfdc_ram[0x2000+(offset-0x1000)];
	}
	
	/* The remaining space is ram with mapper:
	   1400, 1800, 1c00 are the three banks; get the pointer
	   plus the offset within the bank
	*/
	return hfdc_ram[hfdc_ram_offset[(offset-0x1400) >> 10] + (offset & 0x3ff)];
}

/*
    write a byte in disk DSR space
*/
static WRITE8_HANDLER(hfdc_mem_w)
{
	running_device *diskcnt = devtag_get_device(space->machine, "smc92x4");

	if (offset >= 0x0000 && offset <= 0x0fbf)
	{
		/* dsr ROM */
		logerror("99_dsk: Writing to HFDC ROM ignored (access to offset %04x)\n", offset);
		return;
	}
	
	if (offset >= 0xfc0 && offset <= 0x0fcf)
	{
		/* tape controller */
		logerror("99_dsk: Tape support not available (access to offset %04x)\n", offset);
		return;
	}

	if (offset >= 0xfd0 && offset <= 0x0fdf)
        {
                /* Disk controller. Note that the odd addresses are ignored. */
                if ((offset & 3) == 0x02)   /* 0x0fd2 and 0x0fd6 */
                {
               		smc92x4_w(diskcnt, (offset>>2)&1, data);
                }
                return;
        }

	if (offset >= 0x0fe0 && offset <= 0x0fff)
	{
		/* Real-time clock. Again, ignore odd addresses. */
		if ((offset & 1) == 0)
		{
			running_device *clock = devtag_get_device(space->machine, "mm58274c_floppy");
			mm58274c_w(clock, (offset - 0x0fe0) >> 1, data);
		}
		return;
	}

	if (offset >= 0x1000 && offset <= 0x13ff)
	{
		/* ram page >08 (hfdc manual says >10) */
		hfdc_ram[0x2000+(offset-0x1000)] = data;
		return;
	}
	
	/* The remaining space is ram with mapper:
	   1400, 1800, 1c00 are the three banks; get the pointer
	   plus the offset within the bank
	*/	
//	if ((offset&0x0f)==0) printf("\n");
//	printf("%02x ", data);
		
	hfdc_ram[hfdc_ram_offset[(offset-0x1400) >> 10] + (offset & 0x3ff)] = data;
}
