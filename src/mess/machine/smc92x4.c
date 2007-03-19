/*
	SMC9224 and SMC9234 Hard and Floppy Disk Controller (HFDC)

	This controller handles MFM and FM encoded floppy disks and hard disks.
	The SMC9224 is used in some DEC systems.  The SMC9234 is used in the Myarc
	HFDC card for the TI99/4a.  The main difference between the two chips is
	the way the ECC bytes are computed; there are differences in the way seek
	times are computed, too.

	References:
	* SMC HDC9234 preliminary data book (1988)

	Raphael Nabet, 2003
*/

#include "driver.h"
#include "devices/flopdrv.h"

#include "smc92x4.h"

#define MAX_HFDC 1
#define MAX_SECTOR_LEN 2048	/* ??? */

/*
	hfdc state structure
*/
typedef struct hfdc_t
{
	UINT8 status;		/* command status register */
	UINT8 disk_sel;		/* disk selection state */
	UINT8 regs[10+6];	/* 4th through 10th registers are actually 2 distinct
						registers each (one is r/o and the other w/o).  The
						11th register ("data") is only used for communication
						with the drive in non-DMA modes??? */
	int reg_ptr;
	int (*select_callback)(int which, select_mode_t select_mode, int select_line, int gpos);
	UINT8 (*dma_read_callback)(int which, offs_t offset);
	void (*dma_write_callback)(int which, offs_t offset, UINT8 data);
	void (*int_callback)(int which, int state);
} hfdc_t;

enum
{
	disk_sel_none = (UINT8) -1
};

enum
{
	hfdc_reg_dma_low = 0,
	hfdc_reg_dma_mid,
	hfdc_reg_dma_high,
	hfdc_reg_des_sector,
	hfdc_reg_des_head,
	hfdc_reg_des_cyl,
	hfdc_reg_sector_count,
	hfdc_reg_retry_count,
	hfdc_reg_mode,
	hfdc_reg_term,
		hfdc_reg_cur_head,
		hfdc_reg_cur_cyl,
		hfdc_reg_read_6,
		hfdc_reg_read_7,
		hfdc_reg_chip_stat,
		hfdc_reg_drive_stat
};

/*
	Definition of bits in the status register
*/
#define ST_INTPEND	(1<<7)		/* interrupt pending */
#define ST_DMAREQ	(1<<6)		/* DMA request */
#define ST_DONE		(1<<5)		/* command done */
#define ST_TERMCOD	(3<<3)		/* termination code (see below) */
#define ST_RDYCHNG	(1<<2)		/* ready change */
#define ST_OVRUN	(1<<1)		/* overrun/underrun */
#define ST_BADSECT	(1<<0)		/* bad sector */

/*
	Definition of the termination codes
*/
#define ST_TC_SUCCESS	(0<<3)	/* Successful completion */
#define ST_TC_RDIDERR	(1<<3)	/* Error in READ-ID sequence */
#define ST_TC_VRFYERR	(2<<3)	/* Error in VERIFY sequence */
#define ST_TC_DATAERR	(3<<3)	/* Error in DATA-TRANSFER seq. */


/*
	Definition of bits in the Termination-Conditions register
*/
#define TC_CRCPRE	(1<<7)		/* CRC register preset, must be 1 */
#define TC_UNUSED	(1<<6)		/* bit 6 is not used and must be 0 */
#define TC_INTDONE	(1<<5)		/* interrupt on done */
#define TC_TDELDAT	(1<<4)		/* terminate on deleted data */
#define TC_TDSTAT3	(1<<3)		/* terminate on drive status 3 change */
#define TC_TWPROT	(1<<2)		/* terminate on write-protect (FDD only) */
#define TC_INTRDCH	(1<<1)		/* interrupt on ready change (FDD only) */
#define TC_TWRFLT	(1<<0)		/* interrupt on write-fault (HDD only) */

/*
	Definition of bits in the Chip-Status register
*/
#define CS_RETREQ	(1<<7)		/* retry required */
#define CS_ECCATT	(1<<6)		/* ECC correction attempted */
#define CS_ECCERR	(1<<5)		/* ECC/CRC error */
#define CS_DELDATA	(1<<4)		/* deleted data mark */
#define CS_SYNCERR	(1<<3)		/* synchronization error */
#define CS_COMPERR	(1<<2)		/* compare error */
#define CS_PRESDRV	(3<<0)		/* present drive selected */

/*
	Definition of bits in the Disk-Status register
*/
#define DS_SELACK	(1<<7)		/* select acknowledge (harddisk only!) */
#define DS_INDEX	(1<<6)		/* index point */
#define DS_SKCOM	(1<<5)		/* seek complete */
#define DS_TRK00	(1<<4)		/* track 0 */
#define DS_DSTAT3	(1<<3)		/* drive status 3 (MBZ) */
#define DS_WRPROT	(1<<2)		/* write protect (floppy only!) */
#define DS_READY	(1<<1)		/* drive ready bit */
#define DS_WRFAULT	(1<<0)		/* write fault */

static hfdc_t hfdc[MAX_HFDC];

/*
	Perform the read id field sequence for a floppy disk (but do not perform
	any implied seek)

	which: udc controller index
	disk_unit: floppy drive index
	head: selected head
*/
static int floppy_read_id(int which, int disk_unit, int head)
{
	mess_image *disk_img = image_from_devtype_and_index(IO_FLOPPY, disk_unit);
	UINT8 revolution_count;
	chrn_id id;

	revolution_count = 0;

	while (revolution_count < 4)
	{
		if (floppy_drive_get_next_id(disk_img, head, &id))
		{
			hfdc[which].regs[hfdc_reg_cur_cyl] = id.C;
			hfdc[which].regs[hfdc_reg_cur_head] = id.H;
			hfdc[which].regs[hfdc_reg_read_6] = 0xfe;	/* ident byte */
			if (id.flags & ID_FLAG_DELETED_DATA)
				hfdc[which].regs[hfdc_reg_chip_stat] |= CS_DELDATA;
			return TRUE;
		}
	}

	hfdc[which].status |= ST_TC_RDIDERR;
	hfdc[which].regs[hfdc_reg_chip_stat] |= CS_SYNCERR;
	return FALSE;
}

/*
	Perform the verify sequence for a floppy disk

	which: udc controller index
	disk_unit: floppy drive index
*/
static int floppy_find_sector(int which, int disk_unit, int cylinder, int head, int check_secnum, int sector, int *sector_data_id, int *sector_len)
{
	mess_image *disk_img = image_from_devtype_and_index(IO_FLOPPY, disk_unit);
	UINT8 revolution_count;
	chrn_id id;

	revolution_count = 0;

	while (revolution_count < 4)
	{
		if (floppy_drive_get_next_id(disk_img, head, &id))
		{
			/* compare id values */
			if ((id.C == cylinder) && (id.H == head) && ((id.R == sector) || ! check_secnum))
			{
				* sector_data_id = id.data_id;
				* sector_len = 1 << (id.N+7);
				assert((* sector_len) < MAX_SECTOR_LEN);

				hfdc[which].regs[hfdc_reg_cur_cyl] = id.C;
				hfdc[which].regs[hfdc_reg_cur_head] = id.H;
				hfdc[which].regs[hfdc_reg_read_6] = 0xfe;	/* ident byte (right???) */
				if (id.flags & ID_FLAG_DELETED_DATA)
					hfdc[which].regs[hfdc_reg_chip_stat] |= CS_DELDATA;
				else
					hfdc[which].regs[hfdc_reg_chip_stat] &= ~CS_DELDATA;
				return TRUE;
			}
		}

		 /* index set? */
		if (floppy_drive_get_flag_state(disk_img, FLOPPY_DRIVE_INDEX))
		{
			/* update revolution count */
			revolution_count++;
		}
	}

	return FALSE;
}

static int smc92x4_floppy_read_sector(int which, int disk_unit, int cylinder, int head, int check_secnum, int sector, int *dma_address)
{
	int sector_data_id, sector_len;
	UINT8 buf[MAX_SECTOR_LEN];
	int i;
	mess_image *disk_img = image_from_devtype_and_index(IO_FLOPPY, disk_unit);

	if (! floppy_find_sector(which, disk_unit, cylinder, head, check_secnum, sector, & sector_data_id, & sector_len))
	{
		hfdc[which].status |= ST_TC_RDIDERR;
		return FALSE;
	}

	floppy_drive_read_sector_data(disk_img, head, sector_data_id, (char *) buf, sector_len);
	for (i=0; i<sector_len; i++)
	{
		(*hfdc[which].dma_write_callback)(which, *dma_address, buf[i]);
		*dma_address = ((*dma_address) + 1) & 0xffffff;
	}
	return TRUE;
}

static int smc92x4_floppy_write_sector(int which, int disk_unit, int cylinder, int head, int check_secnum, int sector, int *dma_address)
{
	int sector_data_id, sector_len;
	UINT8 buf[MAX_SECTOR_LEN];
	int i;
	mess_image *disk_img = image_from_devtype_and_index(IO_FLOPPY, disk_unit);

	if (! floppy_find_sector(which, disk_unit, cylinder, head, check_secnum, sector, & sector_data_id, & sector_len))
	{
		hfdc[which].status |= ST_TC_RDIDERR;
		return FALSE;
	}

	for (i=0; i<sector_len; i++)
	{
		buf[i] = (*hfdc[which].dma_read_callback)(which, *dma_address);
		*dma_address = ((*dma_address) + 1) & 0xffffff;
	}
	floppy_drive_write_sector_data(disk_img, head, sector_data_id, (char *) buf, sector_len, FALSE);
	return TRUE;
}

static void floppy_step(int which, int disk_unit, int direction)
{
	mess_image *disk_img = image_from_devtype_and_index(IO_FLOPPY, disk_unit);
	floppy_drive_seek(disk_img, direction);
}

static UINT8 floppy_get_disk_status(int which, int disk_unit)
{
	mess_image *disk_img = image_from_devtype_and_index(IO_FLOPPY, disk_unit);
	int status = floppy_status(disk_img, -1);
	int reply;

	reply = 0;
	if (status & FLOPPY_DRIVE_INDEX)
		reply |= DS_INDEX;
	reply |= DS_SKCOM;
	if (status & FLOPPY_DRIVE_HEAD_AT_TRACK_0)
		reply |= DS_TRK00;
	if (status & FLOPPY_DRIVE_DISK_WRITE_PROTECTED)
		reply |= DS_WRPROT;
	if /*(status & FLOPPY_DRIVE_READY)*/(image_exists(disk_img))
		reply |= DS_READY;

	return reply;
}

#include "devices/harddriv.h"
#include "harddisk.h"

static struct
{
	hard_disk_file *hd_handle;
	unsigned int wp : 1;		/* TRUE if disk is write-protected */
	int current_cylinder;

	/* disk geometry */
	unsigned int cylinders, heads, sectors_per_track, bytes_per_sector;
} hd[10];

int smc92x4_hd_load(mess_image *image, int disk_unit)
{
	const hard_disk_info *info;

	if (device_load_mess_hd(image) == INIT_PASS)
	{
		hd[disk_unit].hd_handle = mess_hd_get_hard_disk_file(image);
		hd[disk_unit].wp = !image_is_writable(image);
		hd[disk_unit].current_cylinder = 0;
		info = hard_disk_get_info(hd[disk_unit].hd_handle);
		hd[disk_unit].cylinders = info->cylinders;
		hd[disk_unit].heads = info->heads;
		hd[disk_unit].sectors_per_track = info->sectors;
		hd[disk_unit].bytes_per_sector = info->sectorbytes;

		if ((hd[disk_unit].cylinders > 2048) || (hd[disk_unit].heads > 16)
				|| (hd[disk_unit].sectors_per_track > 256)
				|| (hd[disk_unit].bytes_per_sector > 2048))
		{
			smc92x4_hd_unload(image, disk_unit);
			return INIT_FAIL;
		}
		if (hd[disk_unit].bytes_per_sector != 256)
		{
			smc92x4_hd_unload(image, disk_unit);
			return INIT_FAIL;
		}
		return INIT_PASS;
	}	

	return INIT_FAIL;
}

void smc92x4_hd_unload(mess_image *image, int disk_unit)
{
	device_unload_mess_hd(image);
	hd[disk_unit].hd_handle = NULL;
}

static int harddisk_read_id(int which, int disk_unit, int head)
{
	hfdc[which].regs[hfdc_reg_cur_cyl] = hd[disk_unit].current_cylinder & 0xff;
	hfdc[which].regs[hfdc_reg_cur_head] = ((hd[disk_unit].current_cylinder >> 4) & 0x70) | head;
	hfdc[which].regs[hfdc_reg_read_6] = 0xfe;	/* ident byte */
	/*if ()
		hfdc[which].regs[hfdc_reg_chip_stat] |= CS_DELDATA;*/

	return TRUE;
}

static int harddisk_chs_to_lba(int which, int disk_unit, unsigned int cylinder, unsigned int head, unsigned int sector, UINT32 *lba)
{
	if (cylinder >= hd[disk_unit].cylinders)
	{
		hd[disk_unit].current_cylinder = hd[disk_unit].cylinders-1;
		hfdc[which].status |= ST_TC_RDIDERR;
		return FALSE;
	}
	else
	{
		hd[disk_unit].current_cylinder = cylinder;
	}
	if ((head >= hd[disk_unit].heads) || (sector >= hd[disk_unit].sectors_per_track))
	{
		hfdc[which].status |= ST_TC_RDIDERR;
		return FALSE;
	}

	* lba = (cylinder*hd[disk_unit].heads + head)*hd[disk_unit].sectors_per_track + sector;

	return TRUE;
}

static int harddisk_read_sector(int which, int disk_unit, int cylinder, int head, int sector, int *dma_address)
{
	UINT32 lba;
	UINT8 buf[MAX_SECTOR_LEN];
	int i;


	if (!hd[disk_unit].hd_handle)
	{
		hfdc[which].status |= ST_TC_RDIDERR;
		return FALSE;
	}

	if (! harddisk_chs_to_lba(which, disk_unit, cylinder, head, sector, &lba))
		return FALSE;

	if (! hard_disk_read(hd[disk_unit].hd_handle, lba, buf))
	{
		hfdc[which].status |= ST_TC_DATAERR;
		return FALSE;
	}
	for (i=0; i<hd[disk_unit].bytes_per_sector; i++)
	{
		(*hfdc[which].dma_write_callback)(which, *dma_address, buf[i]);
		*dma_address = ((*dma_address) + 1) & 0xffffff;
	}

	return TRUE;
}

static int harddisk_write_sector(int which, int disk_unit, int cylinder, int head, int sector, int *dma_address)
{
	UINT32 lba;
	UINT8 buf[MAX_SECTOR_LEN];
	int i;


	if (!hd[disk_unit].hd_handle)
	{
		hfdc[which].status |= ST_TC_RDIDERR;
		return FALSE;
	}

	if (! harddisk_chs_to_lba(which, disk_unit, cylinder, head, sector, &lba))
		return FALSE;

	for (i=0; i<hd[disk_unit].bytes_per_sector; i++)
	{
		buf[i] = (*hfdc[which].dma_read_callback)(which, *dma_address);
		*dma_address = ((*dma_address) + 1) & 0xffffff;
	}
	if (! hard_disk_write(hd[disk_unit].hd_handle, lba, buf))
	{
		hfdc[which].status |= ST_TC_DATAERR;
		return FALSE;
	}

	return TRUE;
}

static void harddisk_step(int which, int disk_unit, int direction)
{
	hd[disk_unit].current_cylinder += direction;

	if (hd[disk_unit].current_cylinder >= hd[disk_unit].cylinders)
		hd[disk_unit].current_cylinder = hd[disk_unit].cylinders-1;
	if (hd[disk_unit].current_cylinder < 0)
		hd[disk_unit].current_cylinder = 0;
}

static UINT8 harddisk_get_disk_status(int which, int disk_unit)
{
	int reply;

	reply = 0;

	if (hd[disk_unit].hd_handle)
	{
		reply |= DS_SELACK;
		/*if ()
			reply |= DS_INDEX;*/
		reply |= DS_SKCOM;
		if (hd[disk_unit].current_cylinder == 0)
			reply |= DS_TRK00;
		if (hd[disk_unit].wp)
			reply |= DS_WRPROT;
		if (hd[disk_unit].hd_handle)
			reply |= DS_READY;
		/*if ()
			reply |= DS_WRFAULT;*/
	}

	return reply;
}



/*
	Initialize the controller
*/
void smc92x4_init(int which, const smc92x4_intf *intf)
{
	memset(& hfdc[which], 0, sizeof(hfdc[which]));
	hfdc[which].select_callback = intf->select_callback;
	hfdc[which].dma_read_callback = intf->dma_read_callback;
	hfdc[which].dma_write_callback = intf->dma_write_callback;
	hfdc[which].int_callback = intf->int_callback;
	if (hfdc[which].int_callback)
		(*hfdc[which].int_callback)(which, 0);
}

/*
	Reset the controller
*/
void smc92x4_reset(int which)
{
	int i;

	hfdc[which].status = 0;			// ???
	for (i=0; i<10; i++)
		hfdc[which].regs[i] = 0;	// ???
	hfdc[which].reg_ptr = 0;		// ???
	if (hfdc[which].int_callback)
		(*hfdc[which].int_callback)(which, 0);
}

/*
	Change the state of IRQ
*/
static void smc92x4_set_interrupt(int which, int state)
{
	if ((state != 0) != ((hfdc[which].status & ST_INTPEND) != 0))
	{
		if (state)
			hfdc[which].status |= ST_INTPEND;
		else
			hfdc[which].status &= ~ST_INTPEND;

		if (hfdc[which].int_callback)
			(*hfdc[which].int_callback)(which, state);
	}
}

/*
	Select currently selected drive 
*/
static int get_selected_drive(int which, select_mode_t *select_mode, int *disk_unit)
{
	if (hfdc[which].disk_sel == disk_sel_none)
	{
		* select_mode = sm_undefined;
		* disk_unit = -1;
	}
	else
	{
		* select_mode = (select_mode_t) ((hfdc[which].disk_sel >> 2) & 0x3);

		switch (* select_mode)
		{
		case sm_at_harddisk:
		case sm_harddisk:
		case sm_floppy_slow:
		case sm_floppy_fast:
			if (hfdc[which].select_callback)
				* disk_unit = (* hfdc[which].select_callback)(which, * select_mode, hfdc[which].disk_sel & 0x3, hfdc[which].regs[hfdc_reg_retry_count] & 0xf);
			else
				* disk_unit = hfdc[which].disk_sel & 0x3;

		case sm_undefined:
			break;
		}
	}

	return (* disk_unit != -1);
}

/*
	Assert Command Done status bit, triggering interrupts as needed
*/
INLINE void set_command_done(int which)
{
	//assert(! (hfdc[which].status & ST_DONE))
	hfdc[which].status |= ST_DONE;
	if (hfdc[which].regs[hfdc_reg_term] & TC_INTDONE)
		smc92x4_set_interrupt(which, 1);
}

INLINE void step(int which, select_mode_t select_mode, int disk_unit, int direction)
{
	switch (select_mode)
	{
	case sm_undefined:
	default:
		break;

	case sm_at_harddisk:
		break;

	case sm_harddisk:
		/* hard disks may ignore programmed seeks */
		harddisk_step(which, disk_unit, direction);
		break;

	case sm_floppy_slow:
	case sm_floppy_fast:
		floppy_step(which, disk_unit, direction);
		break;
	}
}

/*
	Perform the read_id phase on a drive
*/
INLINE int read_id(int which, select_mode_t select_mode, int disk_unit, int head,
					int do_implied_seek, int desired_cylinder)
{
	int reply;

	switch (select_mode)
	{
	case sm_undefined:
	default:
		reply = FALSE;
		break;

	case sm_at_harddisk:
		reply = FALSE;
		break;

	case sm_harddisk:
		reply = harddisk_read_id(which, disk_unit, head);
		break;

	case sm_floppy_slow:
	case sm_floppy_fast:
		reply = floppy_read_id(which, disk_unit, head);
		break;
	}

	if (do_implied_seek && reply)
	{
		/* perform implied seek */
		int current_cylinder;
		int step_direction, step_count;
		int i;

		if (select_mode == sm_at_harddisk)
		{
			switch (hfdc[which].regs[hfdc_reg_read_6])
			{
			case 0xfe:
				current_cylinder = hfdc[which].regs[hfdc_reg_cur_cyl];
				break;
			case 0xff:
				current_cylinder = 256 + hfdc[which].regs[hfdc_reg_cur_cyl];
				break;
			case 0xfc:
				current_cylinder = 512 + hfdc[which].regs[hfdc_reg_cur_cyl];
				break;
			case 0xfd:
				current_cylinder = 1024 + hfdc[which].regs[hfdc_reg_cur_cyl];
				break;
			default:
				assert(1);
				current_cylinder = 0;
				break;
			}
		}
		else if (select_mode == sm_harddisk)
			current_cylinder = ((hfdc[which].regs[hfdc_reg_cur_head] & 0x70) << 4) | hfdc[which].regs[hfdc_reg_cur_cyl];
		else
			current_cylinder = hfdc[which].regs[hfdc_reg_cur_cyl];

		if (current_cylinder < desired_cylinder)
		{
			step_direction = +1;
			step_count = desired_cylinder - current_cylinder;
		}
		else
		{
			step_direction = -1;
			step_count = current_cylinder - desired_cylinder;
		}

		for (i=0; i<step_count; i++)
			step(which, select_mode, disk_unit, step_direction);
	}

	return reply;
}

/*
	Get the status of a drive
*/
INLINE UINT8 get_disk_status(int which, select_mode_t select_mode, int disk_unit)
{
	UINT8 reply;

	switch (select_mode)
	{
	case sm_undefined:
	default:
		reply = 0;
		break;

	case sm_at_harddisk:
		reply = 0;
		break;

	case sm_harddisk:
		reply = harddisk_get_disk_status(which, disk_unit);
		break;

	case sm_floppy_slow:
	case sm_floppy_fast:
		reply = floppy_get_disk_status(which, disk_unit);
		break;

	}

	return reply;
}

/*
	Handle the step command
*/
static void do_step(int which, int mode)
{
	select_mode_t select_mode;
	int disk_unit;

	hfdc[which].status = 0;

	/* reset chip status */
	hfdc[which].regs[hfdc_reg_chip_stat] = hfdc[which].disk_sel & 0x3;

	if (!get_selected_drive(which, & select_mode, & disk_unit))
	{
		/* no unit has been selected */
		hfdc[which].status |= ST_TC_RDIDERR;
		hfdc[which].regs[hfdc_reg_chip_stat] |= CS_SYNCERR;
		goto cleanup;
	}

	step(which, select_mode, disk_unit, (mode & 2) ? -1 : +1);

cleanup:
	/* update register state */
	set_command_done(which);

	/* set drive status */
	hfdc[which].regs[hfdc_reg_drive_stat] = get_disk_status(which, select_mode, disk_unit);
}

/*
	Handle the restore command
*/
static void do_restore(int which, int mode)
{
	select_mode_t select_mode;
	int disk_unit;
	int seek_count;

	hfdc[which].status = 0;

	/* reset chip status */
	hfdc[which].regs[hfdc_reg_chip_stat] = hfdc[which].disk_sel & 0x3;

	if (!get_selected_drive(which, & select_mode, & disk_unit))
	{
		/* no unit has been selected */
		/* the drive status will be 0, therefore the SEEK_COMPLETE bit will be
		cleared, which is what we want */
		goto cleanup;
	}

	switch (select_mode)
	{
	case sm_undefined:
	default:
		break;

	case sm_at_harddisk:
		break;

	case sm_harddisk:
	case sm_floppy_slow:
	case sm_floppy_fast:
		seek_count = 0;
		/* limit iterations to 2048 to prevent an endless loop if the disc is locked */
		while ((seek_count < 2048) && ! (get_disk_status(which, select_mode, disk_unit) & DS_TRK00))
		{
			step(which, select_mode, disk_unit, -1);
			seek_count++;
		}
		break;
	}

cleanup:
	/* update register state */
	set_command_done(which);

	/* set drive status */
	hfdc[which].regs[hfdc_reg_drive_stat] = get_disk_status(which, select_mode, disk_unit);
}

/*
	Handle the read command

	which: disk controller selected
	physical_flag: if 1, read consecutive sectors, ignoring sector IDs
	mode: extra flags (ignored)
*/
static void do_read(int which, int physical_flag, int mode)
{
	select_mode_t select_mode;
	int disk_unit;
	int dma_base_address, dma_address;
	int cylinder, head, sector;

	hfdc[which].status = 0;

	/* reset chip status */
	hfdc[which].regs[hfdc_reg_chip_stat] = hfdc[which].disk_sel & 0x3;

	dma_base_address = hfdc[which].regs[hfdc_reg_dma_low]
						| (hfdc[which].regs[hfdc_reg_dma_mid] << 8)
						| (hfdc[which].regs[hfdc_reg_dma_high] << 16);

	sector = hfdc[which].regs[hfdc_reg_des_sector];
	head = hfdc[which].regs[hfdc_reg_des_head] & 0xf;
	cylinder = (((int) hfdc[which].regs[hfdc_reg_des_head] << 4) & 0x700)
				| hfdc[which].regs[hfdc_reg_des_cyl];

	if (!get_selected_drive(which, & select_mode, & disk_unit))
	{
		/* no unit has been selected */
		hfdc[which].status |= ST_TC_RDIDERR;
		hfdc[which].regs[hfdc_reg_chip_stat] |= CS_SYNCERR;
		goto cleanup;
	}

	if (! (get_disk_status(which, select_mode, disk_unit) & DS_READY))
	{
		/* unit is not ready */
		hfdc[which].status |= ST_TC_RDIDERR;
		hfdc[which].regs[hfdc_reg_chip_stat] |= CS_SYNCERR;
		goto cleanup;
	}

#if 0
	/* if we enable write-protect test in read operations, the ti99 HFDC DSR is
	unable to read write-protected floppies! */
	if ((hfdc[which].regs[hfdc_reg_term] & TC_TWPROT)
			&& (get_disk_status(which, select_mode, disk_unit) & DS_WRPROT))
	{
		/* unit is write protected */
		hfdc[which].status |= ST_TC_RDIDERR;	/* right??? */
		goto cleanup;
	}
#endif

	read_id(which, select_mode, disk_unit, head, ! (mode & 0x02), cylinder);

	while (hfdc[which].regs[hfdc_reg_sector_count])
	{
		/* do read sector */
		dma_address = dma_base_address;
		switch (select_mode)
		{
		case sm_undefined:
		default:
			break;

		case sm_at_harddisk:
			break;

		case sm_harddisk:
			if (! harddisk_read_sector(which, disk_unit, cylinder, head, sector, &dma_address))
				goto cleanup;
			break;

		case sm_floppy_slow:
		case sm_floppy_fast:
			if (! smc92x4_floppy_read_sector(which, disk_unit, cylinder, head, 1, sector, &dma_address))
				goto cleanup;
			break;
		}

		hfdc[which].regs[hfdc_reg_sector_count]--;
		if (hfdc[which].regs[hfdc_reg_sector_count])
		{
			/* read more sectors */

			/* increment desired sector */
			sector++;

			/* update DMA address */
			dma_base_address = dma_address;
		}
	}

cleanup:
	/* update register state */
	set_command_done(which);

	/* update DMA address */
	hfdc[which].regs[hfdc_reg_dma_low] = dma_base_address & 0xff;
	hfdc[which].regs[hfdc_reg_dma_mid] = (dma_base_address >> 8) & 0xff;
	hfdc[which].regs[hfdc_reg_dma_high] = (dma_base_address >> 16) & 0xff;

	/* update desired sector register */
	hfdc[which].regs[hfdc_reg_des_sector] = sector;

	/* set drive status */
	hfdc[which].regs[hfdc_reg_drive_stat] = get_disk_status(which, select_mode, disk_unit);
}

/*
	Handle the write command

	which: disk controller selected
	physical_flag: if 1, ignore CRC/ECC bytes??? (ignored)
	mode: extra flags (ignored)
*/
static void do_write(int which, int physical_flag, int mode)
{
	select_mode_t select_mode;
	int disk_unit;
	int dma_base_address, dma_address;
	int cylinder, head, sector;

	hfdc[which].status = 0;

	/* reset chip status */
	hfdc[which].regs[hfdc_reg_chip_stat] = hfdc[which].disk_sel & 0x3;

	dma_base_address = hfdc[which].regs[hfdc_reg_dma_low]
						| (hfdc[which].regs[hfdc_reg_dma_mid] << 8)
						| (hfdc[which].regs[hfdc_reg_dma_high] << 16);

	sector = hfdc[which].regs[hfdc_reg_des_sector];
	head = hfdc[which].regs[hfdc_reg_des_head] & 0xf;
	cylinder = (((int) hfdc[which].regs[hfdc_reg_des_head] << 4) & 0x700)
				| hfdc[which].regs[hfdc_reg_des_cyl];

	if (!get_selected_drive(which, & select_mode, & disk_unit))
	{
		/* no unit has been selected */
		hfdc[which].status |= ST_TC_RDIDERR;
		hfdc[which].regs[hfdc_reg_chip_stat] |= CS_SYNCERR;
		goto cleanup;
	}

	if (! (get_disk_status(which, select_mode, disk_unit) & DS_READY))
	{
		/* unit is not ready */
		hfdc[which].status |= ST_TC_RDIDERR;
		hfdc[which].regs[hfdc_reg_chip_stat] |= CS_SYNCERR;
		goto cleanup;
	}

	if ((hfdc[which].regs[hfdc_reg_term] & TC_TWPROT)
			&& (get_disk_status(which, select_mode, disk_unit) & DS_WRPROT))
	{
		/* unit is write protected */
		hfdc[which].status |= ST_TC_RDIDERR;	/* right??? */
		goto cleanup;
	}


	read_id(which, select_mode, disk_unit, head, ! (mode & 0x40), cylinder);

	while (hfdc[which].regs[hfdc_reg_sector_count])
	{
		/* do write sector */
		dma_address = dma_base_address;
		switch (select_mode)
		{
		case sm_undefined:
		default:
			break;

		case sm_at_harddisk:
			break;

		case sm_harddisk:
			if (!harddisk_write_sector(which, disk_unit, cylinder, head, sector, &dma_address))
				goto cleanup;
			break;

		case sm_floppy_slow:
		case sm_floppy_fast:
			if (!smc92x4_floppy_write_sector(which, disk_unit, cylinder, head, 1, sector, &dma_address))
				goto cleanup;
			break;
		}

		hfdc[which].regs[hfdc_reg_sector_count]--;
		if (hfdc[which].regs[hfdc_reg_sector_count])
		{
			/* read more sectors */

			/* increment desired sector */
			sector++;

			/* update DMA address */
			dma_base_address = dma_address;
		}
	}

cleanup:
	/* update register state */
	set_command_done(which);

	/* update DMA address */
	hfdc[which].regs[hfdc_reg_dma_low] = dma_base_address & 0xff;
	hfdc[which].regs[hfdc_reg_dma_mid] = (dma_base_address >> 8) & 0xff;
	hfdc[which].regs[hfdc_reg_dma_high] = (dma_base_address >> 16) & 0xff;

	/* update desired sector register */
	hfdc[which].regs[hfdc_reg_des_sector] = sector;

	/* set drive status */
	hfdc[which].regs[hfdc_reg_drive_stat] = get_disk_status(which, select_mode, disk_unit);
}

/*
	Process a command
*/
static void smc92x4_process_command(int which, int opcode)
{
	if (opcode < 0x80)
	{
		switch (opcode >> 4)
		{
		case 0x0:	/* misc commands */
			if (opcode < 0x04)
			{
				switch (opcode)
				{
				case 0x00:
					/* RESET */
					/* same effect as the RST* pin being active */
					logerror("smc92x4 reset command\n");
					smc92x4_reset(which);
					break;
				case 0x01:
					/* DESELECT DRIVE */
					/* done when no drive is in use */
					logerror("smc92x4 drdeselect command\n");
					hfdc[which].disk_sel = disk_sel_none;
					set_command_done(which);
					break;
				case 0x02:
				case 0x03:
					/* RESTORE DRIVE */
					/* bit 0: 0 -> command ends after last seek pulse, 1 -> command
						ends when the drive asserts the seek complete pin */
					logerror("smc92x4 restore command %X\n", opcode & 0x01);
					do_restore(which, opcode & 0x01);
					break;
				}
			}
			else if (opcode < 0x08)
			{
				/* STEP IN/OUT ONE CYLINDER */
				/* bit 1: direction (0 -> inward, 1 -> outward i.e. toward cyl #0) */
				/* bit 0: 0 -> command ends after last seek pulse, 1 -> command
					ends when the drive asserts the seek complete pin */
				logerror("smc92x4 step command %X\n", opcode & 0x3);
				do_step(which, opcode & 0x03);
			}
			else
			{
				/* TAPE BACKUP */
				logerror("smc92x4 tape backup command %X\n", opcode);
			}
			break;
		case 0x1:
			/* POLLDRIVE */
			/* bit 3-0: one bit for each drive to test */
			logerror("smc92x4 polldrive command %X\n", opcode & 0xf);
			break;
		case 0x2:
		case 0x3:
			/* DRSELECT */
			/* bit 4: if 1, apply head load delay */
			/* bit 3 & 2: disk type (0-3): 0 for hard disk in IBM-AT format, 1
				for hard disk in standard SMC format, 2 for high-speed (8" or
				HD(?)) floppy, 3 for low-speed (5"1/4 DD?) floppy disk */
			/* bit 1 & 0: unit number (0-3)*/
			logerror("smc92x4 drselect command %X\n", opcode & 0xf);
			hfdc[which].disk_sel = opcode & 0xf;
			set_command_done(which);
			break;
		case 0x4:
			/* SETREGPTR */
			/* bit 3-0: reg-number (should be in [0,10]) */
			logerror("smc92x4 setregptr command %X\n", opcode & 0xf);
			hfdc[which].reg_ptr = opcode & 0xf;
			set_command_done(which);
			break;
		case 0x5:
			if (opcode < 0x58)
			{
				/* SEEK/READ ID */
				/* bit 2: step enable */
				/* bit 1: wait for seek complete signal */
				/* bit 0: verify ID */
				logerror("smc92x4 seekreadid command %X\n", opcode & 0x7);
			}
			else
			{
				/* READ */
				/* bits 2-1: 0 -> read physical, 1 -> read track, 2 -> read
					logical with implied seek, 3 -> read logical with no
					implied seek */
				/* bit 0: 0 -> do not transfer data (transfer ID fields for
					read track command), 1 -> transfer data */
				switch ((opcode >> 1) & 0x3)
				{
				case 0:
					logerror("smc92x4 read physical command %X\n", opcode & 0x1);
					do_read(which, 1, opcode & 0x3);
					break;
				case 1:
					logerror("smc92x4 read track command %X\n", opcode & 0x1);
					break;
				case 2:
				case 3:
					logerror("smc92x4 read logical command %X\n", opcode & 0x3);
					do_read(which, 0, opcode & 0x3);
					break;
				}
			}
			break;
		case 0x6:
		case 0x7:
			/* FORMAT TRACK */
			/* bit 4: 0 -> write deleted data mark instead of normal address
				mark */
			/* bit 3: 1 -> write with reduced current */
			/* bit 2-0: write precompensation value */
			logerror("smc92x4 formattrack command %X\n", opcode & 0xf);
			break;
		}
	}
	else
	{
		/* WRITE */
		/* bits 6: 0 -> implied seek, 1-> no implied seek */
		/* bit 5: 0 -> write physical, 1 -> write logical */
		/* bit 4: 1 -> write deleted data mark instead of normal address mark */
		/* bit 3: 1 -> write with reduced current */
		/* bit 2-0: write precompensation value */
		logerror("smc92x4 write command %X\n", opcode & 0x7f);
		switch ((opcode >> 5) & 0x3)
		{
		case 0:
			logerror("smc92x4 write physical command %X\n", opcode & 0x7f);
			do_write(which, 1, opcode & 0x7f);
			break;
		case 1:
			logerror("smc92x4 write logical command %X\n", opcode & 0x7f);
			do_write(which, 0, opcode & 0x7f);
			break;
		case 2:
			logerror("smc92x4 write physical command??? %X\n", opcode & 0x7f);
			do_write(which, 1, opcode & 0x7f);
			break;
		case 3:
			logerror("smc92x4 write logical command??? %X\n", opcode & 0x7f);
			do_write(which, 0, opcode & 0x7f);
			break;
		}
	}
}

/*
	Memory accessors
*/

/*
	Read a byte of data from a smc99x4 controller
*/
int smc92x4_r(int which, int offset)
{
	int reply = 0;

	switch (offset & 1)
	{
	case 0:
		/* data register */
		logerror("smc92x4 data read\n");
		if (hfdc[which].reg_ptr < 10)
		{
			/* config register */
			reply = hfdc[which].regs[(hfdc[which].reg_ptr < 4) ? (hfdc[which].reg_ptr) : (hfdc[which].reg_ptr + 6)];
			hfdc[which].reg_ptr++;
		}
		else if (hfdc[which].reg_ptr == 10)
		{
			/* data register */
		}
		else
		{
			/* ??? */
		}
		break;

	case 1:
		/* status register */
		logerror("smc92x4 status read\n");
		reply = hfdc[which].status;
		smc92x4_set_interrupt(which, 0);
		break;
	}

	return reply;
}

/*
	Write a byte to a smc99x4 controller
*/
void smc92x4_w(int which, int offset, int data)
{
	data &= 0xff;

	switch (offset & 1)
	{
	case 0:
		/* data register */
		logerror("smc92x4 data write %X\n", data);
		if (hfdc[which].reg_ptr < 10)
		{
			/* config register */
			hfdc[which].regs[hfdc[which].reg_ptr] = data;
			hfdc[which].reg_ptr++;
		}
		else if (hfdc[which].reg_ptr == 10)
		{
			/* data register */
		}
		else
		{
			/* ??? */
		}
		break;

	case 1:
		/* command register */
		smc92x4_process_command(which, data);
		break;
	}
}

 READ8_HANDLER(smc92x4_0_r)
{
	return smc92x4_r(0, offset);
}

WRITE8_HANDLER(smc92x4_0_w)
{
	smc92x4_w(0, offset, data);
}

