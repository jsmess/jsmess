/*
    Myarc Hard and Floppy Disk Controller
    Michael Zapf
*/
#include "emu.h"
#include "peribox.h"
#include "ti_fdc.h"
#include "machine/wd17xx.h"
#include "formats/ti99_dsk.h"
#include "imagedev/flopdrv.h"
#include "ti99_hd.h"
#include "machine/mm58274c.h"
#include "hfdc.h"

#define CRU_BASE 0x1100

#define hfdc_region "hfdc_region"

#define HFDC_MAX_FLOPPY 4
#define HFDC_MAX_HARD 4

#define TAPE_ADDR	0x0fc0
#define HDC_R_ADDR	0x0fd0
#define HDC_W_ADDR	0x0fd2
#define CLK_ADDR	0x0fe0
#define RAM_ADDR	0x1000

typedef ti99_pebcard_config ti99_hfdc_config;

typedef struct _ti99_hfdc_state
{
	/* When TRUE, card is accessible. Indicated by a LED. */
	int					selected;

	/* Card select mask. Required to support Genmod. */
	int					select_mask;

	/* Card select mask. Required to support Genmod. */
	int					select_value;

	/* When TRUE, triggers motor monoflop. */
	int					strobe_motor;

	/* Clock divider bit 0. Unused in this emulation. */
	int 				CD0;

	/* Clock divider bit 1. Unused in this emulation. */
	int					CD1;

	/* count 4.23s from rising edge of motor_on */
	emu_timer			*motor_on_timer;

	/* Link to the HDC9234 controller on the board. */
	device_t		*controller;

	/* Link to the clock chip on the board. */
	device_t		*clock;

	/* Determines whether we have access to the CRU bits. */
	int 				cru_select;

	/* IRQ state */
	int 				irq;

	/* DMA in Progress state */
	int 				dip;

	/* Output 1 latch */
	UINT8				output1_latch;

	/* Output 2 latch */
	UINT8				output2_latch;

	/* Connected floppy drives. */
	device_t		*floppy_unit[HFDC_MAX_FLOPPY];

	/* Connected harddisk drives. */
	device_t		*harddisk_unit[HFDC_MAX_HARD];

	/* DMA address latch */
	UINT32				dma_address;

	/* DSR ROM */
	UINT8				*rom;

	/* ROM banks. */
	int 				rom_offset;

	/* HFDC RAM */
	UINT8				*ram;

	/* RAM banks */
	int 				ram_offset[4];

	/* Callback lines to the main system. */
	ti99_peb_connect	lines;

} ti99_hfdc_state;

INLINE ti99_hfdc_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == HFDC);

	return (ti99_hfdc_state *)downcast<legacy_device_base *>(device)->token();
}

/*
    Resets the drive geometry. This is required because the heuristic of
    the default implementation sets the drive geometry to the geometry
    of the medium.
*/
static void set_geometry(device_t *drive, floppy_type_t type)
{
	if (drive!=NULL)
		floppy_drive_set_geometry(drive, type);
	else
		logerror("ti99/HFDC: Drive not found\n");
}

static void set_all_geometries(device_t *device, floppy_type_t type)
{
	set_geometry(device->siblingdevice(FLOPPY_0), type);
	set_geometry(device->siblingdevice(FLOPPY_1), type);
	set_geometry(device->siblingdevice(FLOPPY_2), type);
}

/*************************************************************************/

/*
    Read disk CRU interface
    HFDC manual p. 44

    CRU Select=0
    xx00          Disk controller interrupt
    xx02          Motor status
    xx04          DMA in progress

    Note that the cru line settings on the board do not match the
    description in the manual. 02 and 04 swap their positions (checked with
    board and schematics). Similarly, the dip switches
    for the drive configurations are not correctly enumerated in the manual.

    CRU Select=1
    xx00          Switch 4 status \_ drive 2 config
    xx02          Switch 3 status /
    xx04          Switch 2 status \_ drive 1 config
    xx06          Switch 1 status /
    xx08          Switch 8 status \_ drive 4 config
    xx0A          Switch 7 status /
    xx0C          Switch 6 status \_ drive 3 config
    xx0E          Switch 5 status /

    config: 00 = 9/16/18 sectors, 40 tracks, 16 msec
        10 = 9/16/18 sectors, 40 tracks, 8 msec
        01 = 9/16/18 sectors, 80 tracks, 2 msec
        11 = 36 sectors, 80 tracks, 2 msec
*/
static READ8Z_DEVICE_HANDLER( cru_r )
{
	ti99_hfdc_state *card = get_safe_token(device);
	UINT8 reply;

	if ((offset & 0xff00)==CRU_BASE)
	{
		if ((offset & 0x00ff)==0)  /* CRU bits 0-7 */
		{
			/* CRU bits */
			if (card->cru_select)
			{
				// DIP switches.  Logic levels are inverted (on->0, off->1).  CRU
				// bit order is the reverse of DIP-switch order, too (dip 1 -> bit 7,
				// dip 8 -> bit 0).
				// MZ: 00 should not be used since there is a bug in the
				// DSR of the HFDC which causes problems with SD disks
				// (controller tries DD and then fails to fall back to SD) */
				reply = ~(input_port_read(device->machine(), "HFDCDIP"));
			}
			else
			{
				reply = 0;

				if (card->irq)			reply |= 0x01;
				if (card->dip)			reply |= 0x02;
				if (card->strobe_motor)	reply |= 0x04;
			}
			*value = reply;
		}
		else /* CRU bits 8+ */
			*value = 0;
	}
}

/*
    Write disk CRU interface
    HFDC manual p. 43

    CRU rel. address    Definition                      Active
    00                  Device Service Routine Select   HIGH
    02                  Reset to controller chip        LOW
    04                  Floppy Motor on / CD0           HIGH
    06                  ROM page select 0 / CD1
    08                  ROM page select 1 / CRU input select
    12/4/6/8/A          RAM page select at 0x5400
    1C/E/0/2/4          RAM page select at 0x5800
    26/8/A/C/E          RAM page select at 0x5C00

    RAM bank select: bank 0..31; 12 = LSB (accordingly for other two areas)
    ROM bank select: bank 0..3; 06 = MSB, 07 = LSB
    Bit number = (CRU_rel_address - base_address)/2
    CD0 and CD1 are Clock Divider selections for the Floppy Data Separator (FDC9216)
*/
static WRITE8_DEVICE_HANDLER( cru_w )
{
	ti99_hfdc_state *card = get_safe_token(device);

	if ((offset & 0xff00)==CRU_BASE)
	{
		int bit = (offset >> 1) & 0x1f;

		if (bit >= 9 && bit < 24)
		{
			if (data)
				// we leave index 0 unchanged; modify indexes 1-3
				card->ram_offset[(bit-4)/5] |= 0x0400 << ((bit-9)%5);
			else
				card->ram_offset[(bit-4)/5] &= ~(0x0400 << ((bit-9)%5));

			return;
		}

		switch (bit)
		{
		case 0:
			card->selected = data;
			break;

		case 1:
			if (!data) smc92x4_reset(card->controller);  // active low
			break;

		case 2:
			card->CD0 = data;
			if (data && !card->strobe_motor)
			{
				card->motor_on_timer->adjust(attotime::from_msec(4230));
			}
			card->strobe_motor = data;
			break;

		case 3:
			card->CD1 = data;
			if (data) card->rom_offset |= 0x2000;
			else card->rom_offset &= ~0x2000;
			break;

		case 4:
			card->cru_select = data;
			if (data) card->rom_offset |= 0x1000;
			else card->rom_offset &= ~0x1000;
			break;

		default:
			 logerror("ti99/HFDC: Attempt to set undefined CRU bit %d\n", bit);
		}
	}
}

/*
    read a byte in disk DSR space
    HFDC manual, p. 44
    Memory map as seen by the 99/4A PEB

    0x4000 - 0x4fbf one of four possible ROM pages
    0x4fc0 - 0x4fcf tape select
    0x4fd0 - 0x4fdf disk controller select
    0x4fe0 - 0x4fff time chip select

    0x5000 - 0x53ff static RAM page 0x10
    0x5400 - 0x57ff static RAM page any of 32 pages
    0x5800 - 0x5bff static RAM page any of 32 pages
    0x5c00 - 0x5fff static RAM page any of 32 pages
*/

static READ8Z_DEVICE_HANDLER( data_r )
{
	ti99_hfdc_state *card = get_safe_token(device);

	if (card->selected && ((offset & card->select_mask)==card->select_value))
	{
		/* DSR region */
		if ((offset & 0x1000)==0x0000)
		{
			if ((offset & 0x0fc0)==0x0fc0)
			{
				// Tape: 4fc0...4fcf
				if ((offset & 0x1ff0)==TAPE_ADDR)
				{
					logerror("ti99/HFDC: Tape support not available (access to address %04x)\n", offset);
					return;
				}

				// HDC9234: 4fd0..4fdf / read: 4fd0,4 (mirror 8,c)
				// read: 0100 1111 1101 xx00
				if ((offset & 0x1ff3)==HDC_R_ADDR)
				{
					*value = smc92x4_r(card->controller, (offset>>2)&1);
					return;
				}

				if ((offset & 0x1fe1)==CLK_ADDR)
				{
					*value = mm58274c_r(card->clock, (offset & 0x001e) >> 1);
					return;
				}
			}
			else
			{
				// ROM
				*value = card->rom[card->rom_offset + (offset & 0x0fff)];
				return;
			}
		}

		// RAM: 0101 xxxx xxxx xxxx
		if ((offset & 0x1000)==RAM_ADDR)
		{
			// 0101 00xx xxxx xxxx  static 0x08
			// 0101 01xx xxxx xxxx  bank 1
			// 0101 10xx xxxx xxxx  bank 2
			// 0101 11xx xxxx xxxx  bank 3
			int bank = (offset & 0x0c00) >> 10;
			*value = card->ram[card->ram_offset[bank] + (offset & 0x03ff)];
		}
	}
}

/*
    Write a byte to the controller.
*/
static WRITE8_DEVICE_HANDLER( data_w )
{
	ti99_hfdc_state *card = get_safe_token(device);
	if (card->selected && ((offset & card->select_mask)==card->select_value))
	{
		// Tape: 4fc0...4fcf
		if ((offset & 0x1ff0)==TAPE_ADDR)
		{
			logerror("ti99/HFDC: Tape support not available (access to address %04x)\n", offset);
			return;
		}

		// HDC9234: 4fd0..4fdf / write: 4fd2,6 (mirror a,e)
		// write: 0100 1111 1101 xx10
		if ((offset & 0x1ff3)==HDC_W_ADDR)
		{
			smc92x4_w(card->controller, (offset>>2)&1, data);
			return;
		}

		if ((offset & 0x1fe1)==CLK_ADDR)
		{
			mm58274c_w(card->clock, (offset & 0x001e) >> 1, data);
			return;
		}

		// RAM: 0101 xxxx xxxx xxxx
		if ((offset & 0x1000)==RAM_ADDR)
		{
			// 0101 00xx xxxx xxxx  static 0x08
			// 0101 01xx xxxx xxxx  bank 1
			// 0101 10xx xxxx xxxx  bank 2
			// 0101 11xx xxxx xxxx  bank 3
			int bank = (offset & 0x0c00) >> 10;
			card->ram[card->ram_offset[bank] + (offset & 0x03ff)] = data;
		}
	}
}

/*
    Called whenever the state of the sms9234 interrupt pin changes.
*/
static WRITE_LINE_DEVICE_HANDLER( intrq_w )
{
	// Note that the callback functions deliver the calling device, which is
	// the controller in this case.
	ti99_hfdc_state *card = get_safe_token(device->owner());
	card->irq = state;

	/* Set INTA */
	if (state)
	{
		devcb_call_write_line(&card->lines.inta, ASSERT_LINE);
	}
	else
	{
		devcb_call_write_line(&card->lines.inta, CLEAR_LINE);
	}
}

/*
    Called whenever the state of the sms9234 DMA in progress changes.
*/
static WRITE_LINE_DEVICE_HANDLER( dip_w )
{
	ti99_hfdc_state *card = get_safe_token(device->owner());
	card->dip = state;
}

// device ok
static WRITE8_DEVICE_HANDLER( auxbus_out )
{
	ti99_hfdc_state *card = get_safe_token(device->owner());
	switch (offset)
	{
	case INPUT_STATUS:
		logerror("ti99/HFDC: Invalid operation: S0=S1=0, but tried to write (expected: read drive status)\n");
		break;

	case OUTPUT_DMA_ADDR:
		/* Value is dma address byte. Shift previous contents to the left. */
		card->dma_address = ((card->dma_address << 8) + (data&0xff))&0xffffff;
		break;

	case OUTPUT_OUTPUT1:
		/* value is output1 */
		card->output1_latch = data;
		break;

	case OUTPUT_OUTPUT2:
		/* value is output2 */
		card->output2_latch = data;
		break;
	}
}

static int line_to_index(UINT8 line, int max)
{
	// Calculate the binary logarithm of the given 2-multiple
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
    Select the HFDC floppy disk unit
    This function implements the selection logic on the PCB (outside the
    controller chip)
    (Callback function)
*/

static device_t *current_floppy(device_t *controller)
{
	int disk_unit = -1;
	ti99_hfdc_state *card = get_safe_token(controller->owner());

	disk_unit = line_to_index(card->output1_latch & 0x0f, HFDC_MAX_FLOPPY);

	if (disk_unit<0)
	{
		logerror("No unit selected\n");
		return NULL;
	}

	return card->floppy_unit[disk_unit];
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

static device_t *current_harddisk(device_t *controller)
{
	ti99_hfdc_state *card = get_safe_token(controller->owner());

	int disk_unit = -1;
	disk_unit = line_to_index((card->output1_latch>>4)&0x0f, HFDC_MAX_HARD);

	/* The HFDC does not use disk_unit=0. The first HD has index 1. */

	if (disk_unit>=0)
		return card->harddisk_unit[disk_unit];
	else
		return NULL;
}

static READ_LINE_DEVICE_HANDLER( auxbus_in )
{
	ti99_hfdc_state *card = get_safe_token(device->owner());
	device_t *drive;
	UINT8 reply = 0;

	if (card->output1_latch==0)
	{
		return 0; /* is that the correct default value? */
	}

	/* If a floppy is selected, we have one line set among the four programmable outputs. */
	if ((card->output1_latch & 0x0f)!=0)
	{
		drive = current_floppy(device);

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
		drive = current_harddisk(device);
		state = ti99_mfm_harddisk_status(drive);
		if (state & MFMHD_TRACK00)		reply |= DS_TRK00;
		if (state & MFMHD_SEEKCOMP)		reply |= DS_SKCOM;
		if (state & MFMHD_WRFAULT)		reply |= DS_WRFAULT;
		if (state & MFMHD_INDEX)		reply |= DS_INDEX;
		if (state & MFMHD_READY)		reply |= DS_READY;
	}

	return reply;
}

/*
    Read a byte from buffer in DMA mode
*/
static UINT8 hfdc_dma_read_callback(device_t *device)
{
	ti99_hfdc_state *card = get_safe_token(device->owner());
	UINT8 value = card->ram[card->dma_address & 0x7fff];
	card->dma_address++;
	return value;
}

/*
    Write a byte to buffer in DMA mode
*/
static void hfdc_dma_write_callback(device_t *device, UINT8 data)
{
	ti99_hfdc_state *card = get_safe_token(device->owner());
	card->ram[card->dma_address & 0x7fff] = data;
	card->dma_address++;
}

/*
    callback called at the end of the motor movement
*/
static TIMER_CALLBACK(motor_on_timer_callback)
{
	// currently no activity here
}

/*
    Preliminary MFM harddisk interface.
*/
static void hfdc_harddisk_get_next_id(device_t *controller, int head, chrn_id_hd *id)
{
	device_t *harddisk = current_harddisk(controller);
	if (harddisk != NULL)
		ti99_mfm_harddisk_get_next_id(harddisk, head, id);
}

static void hfdc_harddisk_seek(device_t *controller, int direction)
{
	device_t *harddisk = current_harddisk(controller);
	if (harddisk != NULL)
		ti99_mfm_harddisk_seek(harddisk, direction);
}

static void hfdc_harddisk_read_sector(device_t *controller, int cylinder, int head, int sector, UINT8 **buf, int *sector_length)
{
	device_t *harddisk = current_harddisk(controller);
	if (harddisk != NULL)
		ti99_mfm_harddisk_read_sector(harddisk, cylinder, head, sector, buf, sector_length);
}

static void hfdc_harddisk_write_sector(device_t *controller, int cylinder, int head, int sector, UINT8 *buf, int sector_length)
{
	device_t *harddisk = current_harddisk(controller);
	if (harddisk != NULL)
		ti99_mfm_harddisk_write_sector(harddisk, cylinder, head, sector, buf, sector_length);
}

static void hfdc_harddisk_read_track(device_t *controller, int head, UINT8 **buffer, int *data_count)
{
	device_t *harddisk = current_harddisk(controller);
	if (harddisk != NULL)
		ti99_mfm_harddisk_read_track(harddisk, head, buffer, data_count);
}

static void hfdc_harddisk_write_track(device_t *controller, int head, UINT8 *buffer, int data_count)
{
	device_t *harddisk = current_harddisk(controller);
	if (harddisk != NULL)
		ti99_mfm_harddisk_write_track(harddisk, head, buffer, data_count);
}

const smc92x4_interface ti99_smc92x4_interface =
{
	FALSE,		/* do not use the full track layout */
	DEVCB_LINE(intrq_w),
	DEVCB_LINE(dip_w),
	DEVCB_HANDLER(auxbus_out),
	DEVCB_LINE(auxbus_in),

	current_floppy,
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

static const ti99_peb_card hfdc_card =
{
	data_r,
	data_w,
	cru_r,
	cru_w,

	NULL, NULL,	NULL, NULL
};

static DEVICE_START( ti99_hfdc )
{
	ti99_hfdc_state *card = get_safe_token(device);

	/* Resolve the callbacks to the PEB */
	peb_callback_if *topeb = (peb_callback_if *)device->baseconfig().static_config();
	devcb_resolve_write_line(&card->lines.inta, &topeb->inta, device);
	// The HFDC does not use READY; it has on-board RAM for DMA

	card->motor_on_timer = device->machine().scheduler().timer_alloc(FUNC(motor_on_timer_callback), (void *)device);
	card->ram = NULL;
	card->ram_offset[0] = 0x2000; // static bank
	card->controller = device->subdevice("smc92x4");
	card->clock = device->subdevice("mm58274c");
	astring *region = new astring();
	astring_assemble_3(region, device->tag(), ":", hfdc_region);
	card->rom = device->machine().region(astring_c(region))->base();
}

static DEVICE_STOP( ti99_hfdc )
{
	ti99_hfdc_state *card = get_safe_token(device);
	if (card->ram) free(card->ram);
}

static DEVICE_RESET( ti99_hfdc )
{
	ti99_hfdc_state *card = get_safe_token(device);
	static const char *const flopname[] = {FLOPPY_0, FLOPPY_1, FLOPPY_2, FLOPPY_3};
	static const char *const hardname[] = {MFMHD_0, MFMHD_1, MFMHD_2};

	/* If the card is selected in the menu, register the card */
	if (input_port_read(device->machine(), "DISKCTRL") == DISK_HFDC)
	{
		device_t *peb = device->owner();
		int success = mount_card(peb, device, &hfdc_card, get_pebcard_config(device)->slot);
		if (!success) return;

		card->select_mask = 0x7e000;
		card->select_value = 0x74000;

		if (input_port_read(device->machine(), "MODE")==GENMOD)
		{
			// GenMod card modification
			logerror("HFDC: Configuring for GenMod\n");
			card->select_mask = 0x1fe000;
			card->select_value = 0x174000;
		}

		// Allocate 32 KiB for on-board buffer memory
		if (card->ram==NULL)
		{
			card->ram = (UINT8*)malloc(32768);
//          memset(card->ram, 0, 32768);
		}

		if (input_port_read(device->machine(), "HFDCDIP")&0x55)
			ti99_set_80_track_drives(TRUE);
		else
			ti99_set_80_track_drives(FALSE);

		smc92x4_set_timing(card->controller, input_port_read(device->machine(), "DRVSPD"));

		// Connect floppy drives to controller. Note that *this* is the
		// controller, not the controller chip. The pcb contains a select
		// logic.
		int i;
		for (i = 0; i < 4; i++)
		{
			card->floppy_unit[i] = device->siblingdevice(flopname[i]);

			if (card->floppy_unit[i]!=NULL)
			{
				floppy_drive_set_controller(card->floppy_unit[i], device);
				//          floppy_drive_set_index_pulse_callback(floppy_unit[i], smc92x4_index_pulse_callback);
				floppy_drive_set_rpm(card->floppy_unit[i], 300.);
			}
			else
			{
				logerror("hfdc: Image %s is null\n", flopname[i]);
			}
		}

		/* In the HFDC ROM, WDSx selects drive x; drive 0 is not used */
		for (i = 1; i < 4; i++)
		{
			card->harddisk_unit[i] = device->siblingdevice(hardname[i-1]);
		}
		card->harddisk_unit[0] = NULL;

		floppy_type_t flop = FLOPPY_STANDARD_5_25_DSHD;
		set_all_geometries(device, flop);

		card->strobe_motor = 0;
		card->ram_offset[1] = card->ram_offset[2] = card->ram_offset[3] = 0;
		card->rom_offset = 0;
	}
	// TODO: Check how to make use of   floppy_mon_w(w->drive, CLEAR_LINE);
}

static const mm58274c_interface floppy_mm58274c_interface =
{
	1,	/*  mode 24*/
	0   /*  first day of week */
};

MACHINE_CONFIG_FRAGMENT( ti99_hfdc )
	MCFG_SMC92X4_ADD("smc92x4", ti99_smc92x4_interface )
	MCFG_MM58274C_ADD("mm58274c", floppy_mm58274c_interface)
MACHINE_CONFIG_END

ROM_START( ti99_hfdc )
	ROM_REGION(0x4000, hfdc_region, 0)
	ROM_LOAD_OPTIONAL("hfdc.bin", 0x0000, 0x4000, CRC(66fbe0ed) SHA1(11df2ecef51de6f543e4eaf8b2529d3e65d0bd59)) /* HFDC disk DSR ROM */
ROM_END

static const char DEVTEMPLATE_SOURCE[] = __FILE__;

#define DEVTEMPLATE_ID(p,s)             p##ti99_hfdc##s
#define DEVTEMPLATE_FEATURES            DT_HAS_START | DT_HAS_STOP | DT_HAS_RESET | DT_HAS_ROM_REGION | DT_HAS_INLINE_CONFIG | DT_HAS_MACHINE_CONFIG
#define DEVTEMPLATE_NAME                "Myarc Hard and Floppy Disk Controller Card"
#define DEVTEMPLATE_SHORTNAME           "myarchfdc"
#define DEVTEMPLATE_FAMILY              "Peripheral expansion"
#include "devtempl.h"

DEFINE_LEGACY_DEVICE( HFDC, ti99_hfdc );

