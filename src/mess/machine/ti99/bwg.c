/*
    SNUG BwG Disk Controller
    Based on WD1773
    Double Density, Double-sided

    * Supports Double Density.
    * As this card includes its own RAM, it does not need to allocate a portion
      of VDP RAM to store I/O buffers.
    * Includes a MM58274C RTC.
    * Support an additional floppy drive, for a total of 4 floppies.

    Reference:
    * BwG Disketten-Controller: Beschreibung der DSR
        <http://home.t-online.de/home/harald.glaab/snug/bwg.pdf>

    Michael Zapf, September 2010

    FIXME: IO error 50, CALLs ok
*/

#include "emu.h"
#include "peribox.h"
#include "bwg.h"
#include "machine/wd17xx.h"
#include "formats/ti99_dsk.h"
#include "imagedev/flopdrv.h"
#include "machine/mm58274c.h"

#define CRU_BASE 0x1100

#define bwg_IRQ 1
#define bwg_DRQ 2

#define bwg_region "bwg_region"

typedef ti99_pebcard_config ti99_bwg_config;

typedef struct _ti99_bwg_state
{
	/* Holds the status of the DRQ and IRQ lines. */
	int					DRQ_IRQ_status;

	/* When TRUE, card is accessible. Indicated by a LED. */
	int					selected;

	/* Used for GenMod. */
	int					select_mask;
	int					select_value;

	/* When TRUE, keeps DVENA high. */
	int					strobe_motor;

	/* Signal DVENA. When TRUE, makes some drive turning. */
	int					DVENA;

	/* When TRUE the CPU is halted while DRQ/IRQ are true. */
	int					hold;

	/* Indicates which drive has been selected. Values are 0, 1, 2, and 4. */
	// 000 = no drive
	// 001 = drive 1
	// 010 = drive 2
	// 100 = drive 3
	int					DSEL;

	/* Signal SIDSEL. 0 or 1, indicates the selected head. */
	int					SIDE;

	/* ROM and RAM offset. */
	int					rom_offset, ram_offset;

	/* Indicates whether the clock is mapped into the address space. */
	int					rtc_enabled;

	/* count 4.23s from rising edge of motor_on */
	emu_timer			*motor_on_timer;

	/* Link to the FDC1771 controller on the board. */
	device_t		*controller;

	/* Link to the real-time clock on the board. */
	device_t		*clock;

	/* DSR ROM */
	UINT8				*rom;

	/* On-board RAM */
	UINT8				*ram;

	/* DIP swiches */
	UINT8				dip1, dip2, dip34;

	/* Callback lines to the main system. */
	ti99_peb_connect	lines;

} ti99_bwg_state;

INLINE ti99_bwg_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == BWG);

	return (ti99_bwg_state *)downcast<legacy_device_base *>(device)->token();
}

/*
    Call this when the state of DSKhold or DRQ/IRQ or DVENA change

    Emulation is faulty because the CPU is actually stopped in the midst of
    instruction, at the end of the memory access

    TODO: This has to be replaced by the proper READY handling that is already
    prepared here. (Requires READY handling by the CPU.)
*/
static void bwg_handle_hold(device_t *device)
{
	ti99_bwg_state *card = get_safe_token(device);
	line_state state;

	if (card->hold && (!card->DRQ_IRQ_status) && card->DVENA)
		state = ASSERT_LINE;
	else
		state = CLEAR_LINE;

	// TODO: use READY
	cputag_set_input_line(device->machine(), "maincpu", INPUT_LINE_HALT, state);
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
		logerror("ti99/BwG: Drive not found\n");
}

/* Those defines are required because the WD17xx DEVICE_START implementation
assumes that the floppy devices are either at root level or at the parent
level. Our floppy devices, however, are at the grandparent level as seen from
the controller. */

#define PFLOPPY_0 "peribox:floppy0"
#define PFLOPPY_1 "peribox:floppy1"
#define PFLOPPY_2 "peribox:floppy2"
#define PFLOPPY_3 "peribox:floppy3"

static void set_all_geometries(device_t *device, floppy_type_t type)
{
	set_geometry(device->machine().device(PFLOPPY_0), type);
	set_geometry(device->machine().device(PFLOPPY_1), type);
	set_geometry(device->machine().device(PFLOPPY_2), type);
	set_geometry(device->machine().device(PFLOPPY_3), type);
}


/*************************************************************************/

/*
    Callback, called from the controller chip whenever DRQ/IRQ state change
*/
static WRITE_LINE_DEVICE_HANDLER( ti_bwg_intrq_w )
{
	device_t *carddev = device->owner();
	ti99_bwg_state *card = get_safe_token(carddev);

	if (state)
	{
		card->DRQ_IRQ_status |= bwg_IRQ;
		// Note that INTB is actually not used in the TI-99 family. But the
		// controller asserts the line nevertheless
		devcb_call_write_line(&card->lines.intb, TRUE);
	}
	else
	{
		card->DRQ_IRQ_status &= ~bwg_IRQ;
		devcb_call_write_line(&card->lines.intb, FALSE);
	}
	bwg_handle_hold(carddev);
}

static WRITE_LINE_DEVICE_HANDLER( ti_bwg_drq_w )
{
	device_t *carddev = device->owner();
	ti99_bwg_state *card = get_safe_token(carddev);

	if (state)
		card->DRQ_IRQ_status |= bwg_DRQ;
	else
		card->DRQ_IRQ_status &= ~bwg_DRQ;

	bwg_handle_hold(carddev);
}

/*
    callback called at the end of DVENA pulse
*/
static TIMER_CALLBACK(motor_on_timer_callback)
{
	device_t *device = (device_t *)ptr;
	ti99_bwg_state *card = get_safe_token(device);
	card->DVENA = 0;
	bwg_handle_hold(device);
}

#if 0
static WRITE_LINE_DEVICE_HANDLER( ti99_bwg_ready )
{
	// Caution: The device pointer passed to this function is the calling
	// device. That is, if we want *this* device, we need to take the owner.
	ti99_bwg_state *card = get_safe_token(device->owner());
	devcb_call_write_line( &card->lines.ready, state );
}
#endif

/*
    CRU read handler. *=inverted.
    bit 0: DSK4 connected*
    bit 1: DSK1 connected*
    bit 2: DSK2 connected*
    bit 3: DSK3 connected*
    bit 4: Dip 1
    bit 5: Dip 2
    bit 6: Dip 3
    bit 7: Dip 4
*/
static READ8Z_DEVICE_HANDLER( cru_r )
{
	ti99_bwg_state *card = get_safe_token(device);
	UINT8 reply = 0;

	if ((offset & 0xff00)==CRU_BASE)
	{
		if ((offset & 0x00ff)==0)
		{
			// Assume that we have 4 drives connected
			// If we want to do that properly, we need to check the actually
			// available drives (not the images!). But why should we connect less?
			reply = 0x00;

			// DIP switches. Note that a closed switch means 0
			// xx01 1111   11 = nur dsk1; 10 = 1+2, 01=1/2/3, 00=1-4
			if (card->dip1) reply |= 0x10;
			if (card->dip2) reply |= 0x20;
			reply |= (card->dip34 << 6);
			*value = ~reply;
		}
		else
			*value = 0;
	}
}

static WRITE8_DEVICE_HANDLER( cru_w )
{
	ti99_bwg_state *card = get_safe_token(device);
	int drive, drivebit;

	if ((offset & 0xff00)==CRU_BASE)
	{
		int bit = (offset >> 1) & 0x0f;
		switch (bit)
		{
		case 0:
			/* (De)select the card. Indicated by a LED on the board. */
			card->selected = data;
			break;

		case 1:
			/* Activate motor */
			if (data && !card->strobe_motor)
			{	/* on rising edge, set motor_running for 4.23s */
				card->DVENA = TRUE;
				bwg_handle_hold(device);
				card->motor_on_timer->adjust(attotime::from_msec(4230));
			}
			card->strobe_motor = data;
			break;

		case 2:
			/* Set disk ready/hold (bit 2) */
			// 0: ignore IRQ and DRQ
			// 1: TMS9900 is stopped until IRQ or DRQ are set
			// OR the motor stops rotating - rotates for 4.23s after write
			// to CRU bit 1
			// This is not emulated and could cause the TI99 to lock up
			card->hold = data;
			bwg_handle_hold(device);
			break;

		case 4:
		case 5:
		case 6:
		case 8:
			/* Select drive 0-2 (DSK1-DSK3) (bits 4-6) */
			/* Select drive 3 (DSK4) (bit 8) */
			drive = (bit == 8) ? 3 : (bit - 4);		/* drive # (0-3) */
			drivebit = 1<<drive;

			if (data)
			{
				if (!(card->DSEL & drivebit))			/* select drive */
				{
					if (card->DSEL != 0)
						logerror("ti99/BwG: Multiple drives selected, %02x\n", card->DSEL);
					card->DSEL |= drivebit;
					wd17xx_set_drive(card->controller, drive);
				}
			}
			else
				card->DSEL &= ~drivebit;
			break;

		case 7:
			/* Select side of disk (bit 7) */
			card->SIDE = data;
			wd17xx_set_side(card->controller, card->SIDE);
			break;

		case 10:
			/* double density enable (active low) */
			wd17xx_dden_w(card->controller, data ? ASSERT_LINE : CLEAR_LINE);
			break;

		case 11:
			/* EPROM A13 */
			if (data)
				card->rom_offset |= 0x2000;
			else
				card->rom_offset &= ~0x2000;
			break;

		case 13:
			/* RAM A10 */
			if (data)
				card->ram_offset = 0x0400;
			else
				card->ram_offset = 0x0000;
			break;

		case 14:
			/* Override FDC with RTC (active high) */
			card->rtc_enabled = data;
			break;

		case 15:
			/* EPROM A14 */
			if (data)
				card->rom_offset |= 0x4000;
			else
				card->rom_offset &= ~0x4000;
			break;

		case 3:
		case 9:
		case 12:
			/* Unused (bit 3, 9 & 12) */
			break;
		}
	}
}

/*
    Read a byte
    4000 - 5bff: ROM (4 banks)

    rtc disabled:
    5c00 - 5fef: RAM
    5ff0 - 5fff: Controller (f0 = status, f2 = track, f4 = sector, f6 = data)

    rtc enabled:
    5c00 - 5fdf: RAM
    5fe0 - 5fff: Clock (even addr)
*/

static READ8Z_DEVICE_HANDLER( data_r )
{
	ti99_bwg_state *card = get_safe_token(device);

	if (card->selected)
	{
		if ((offset & card->select_mask)==card->select_value)
		{
			// 010x xxxx xxxx xxxx
			if ((offset & 0x1c00)==0x1c00)
			{
				// ...1 11xx xxxx xxxx
				if (card->rtc_enabled)
				{
					if ((offset & 0x03e1)==0x03e0)
					{
						// .... ..11 111x xxx0
						*value = mm58274c_r(card->clock, (offset & 0x001e) >> 1);
					}
					else
					{
						*value = card->ram[card->ram_offset + (offset&0x03ff)];
					}
				}
				else
				{
					if ((offset & 0x03f9)==0x03f0)
					{
						// .... ..11 1111 0xx0
						// Note that the value is inverted again on the board,
						// so we can drop the inversion
						*value = wd17xx_r(card->controller, (offset >> 1)&0x03);
					}
					else
					{
						*value = card->ram[card->ram_offset + (offset&0x03ff)];
					}
				}
			}
			else
			{
				*value = card->rom[card->rom_offset + (offset & 0x1fff)];
			}
		}
	}
}

/*
    Write a byte
    4000 - 5bff: ROM, ignore write (4 banks)

    rtc disabled:
    5c00 - 5fef: RAM
    5ff0 - 5fff: Controller (f8 = command, fa = track, fc = sector, fe = data)

    rtc enabled:
    5c00 - 5fdf: RAM
    5fe0 - 5fff: Clock (even addr)
*/
static WRITE8_DEVICE_HANDLER( data_w )
{
	ti99_bwg_state *card = get_safe_token(device);

	if (card->selected)
	{
		if ((offset & card->select_mask)==card->select_value)
		{
			// 010x xxxx xxxx xxxx
			if ((offset & 0x1c00)==0x1c00)
			{
				// ...1 11xx xxxx xxxx
				if (card->rtc_enabled)
				{
					if ((offset & 0x03e1)==0x03e0)
					{
						// .... ..11 111x xxx0
						mm58274c_w(card->clock, (offset & 0x001e) >> 1, data);
					}
					else
					{
						card->ram[card->ram_offset + (offset&0x03ff)] = data;
					}
				}
				else
				{
					if ((offset & 0x03f9)==0x03f8)
					{
						// .... ..11 1111 1xx0
						// Note that the value is inverted again on the board,
						// so we can drop the inversion
						wd17xx_w(card->controller, (offset >> 1)&0x03, data);
					}
					else
					{
						card->ram[card->ram_offset + (offset&0x03ff)] = data;
					}
				}
			}
		}
	}
}

static DEVICE_START( ti99_bwg )
{
	ti99_bwg_state *card = get_safe_token(device);

	/* Resolve the callbacks to the PEB */
	peb_callback_if *topeb = (peb_callback_if *)device->baseconfig().static_config();
	devcb_resolve_write_line(&card->lines.ready, &topeb->ready, device);

	card->motor_on_timer = device->machine().scheduler().timer_alloc(FUNC(motor_on_timer_callback), (void *)device);
	card->controller = device->subdevice("wd1773");
	card->clock = device->subdevice("mm58274c");
	astring *region = new astring();
	astring_assemble_3(region, device->tag(), ":", bwg_region);
	card->rom = device->machine().region(astring_c(region))->base();
	card->ram = NULL;
}

static DEVICE_STOP( ti99_bwg )
{
	ti99_bwg_state *card = get_safe_token(device);
	logerror("ti99_bwg: stop\n");
	if (card->ram) free(card->ram);
}

static const ti99_peb_card bwg_card =
{
	data_r,
	data_w,
	cru_r,
	cru_w,
	NULL, NULL,	NULL, NULL
};

static DEVICE_RESET( ti99_bwg )
{
	ti99_bwg_state *card = get_safe_token(device);

	/* If the card is selected in the menu, register the card */
	if (input_port_read(device->machine(), "DISKCTRL") == DISK_BWG)
	{
		device_t *peb = device->owner();
		int success = mount_card(peb, device, &bwg_card, get_pebcard_config(device)->slot);
		if (!success) return;

		// Allocate 2 KiB for on-board buffer memory (6116 chip)
		if (card->ram==NULL)
		{
			card->ram = (UINT8*)malloc(2048);
			memset(card->ram, 0, 2048);
		}

		card->dip1 = input_port_read(device->machine(), "BWGDIP1");
		card->dip2 = input_port_read(device->machine(), "BWGDIP2");
		card->dip34 = input_port_read(device->machine(), "BWGDIP34");

		card->DSEL = 0;
		card->SIDE = 0;
		card->DVENA = 0;
		card->strobe_motor = 0;

		ti99_set_80_track_drives(FALSE);

		floppy_type_t flop = FLOPPY_STANDARD_5_25_DSDD_40;
		set_all_geometries(device, flop);

		card->strobe_motor = 0;
		card->ram_offset = 0;
		card->rom_offset = 0;
		card->rtc_enabled = 0;

		card->select_mask = 0x7e000;
		card->select_value = 0x74000;

		if (input_port_read(device->machine(), "MODE")==GENMOD)
		{
			// GenMod card modification
			card->select_mask = 0x1fe000;
			card->select_value = 0x174000;
		}

		wd17xx_dden_w(card->controller, CLEAR_LINE);	// Correct?
	}
}

const wd17xx_interface ti_wd17xx_interface =
{
	DEVCB_NULL,
	DEVCB_LINE(ti_bwg_intrq_w),
	DEVCB_LINE(ti_bwg_drq_w),
	{ PFLOPPY_0, PFLOPPY_1, PFLOPPY_2, PFLOPPY_3 }
};

static const mm58274c_interface floppy_mm58274c_interface =
{
	1,	/*  mode 24*/
	0   /*  first day of week */
};

MACHINE_CONFIG_FRAGMENT( ti99_bwg )
	MCFG_WD1773_ADD("wd1773", ti_wd17xx_interface )
	MCFG_MM58274C_ADD("mm58274c", floppy_mm58274c_interface)
MACHINE_CONFIG_END

ROM_START( ti99_bwg )
	ROM_REGION(0x8000, bwg_region, 0)
	ROM_LOAD_OPTIONAL("bwg.bin", 0x0000, 0x8000, CRC(06f1ec89) SHA1(6ad77033ed268f986d9a5439e65f7d391c4b7651)) /* BwG disk DSR ROM */
ROM_END

static const char DEVTEMPLATE_SOURCE[] = __FILE__;

#define DEVTEMPLATE_ID(p,s)             p##ti99_bwg##s
#define DEVTEMPLATE_FEATURES            DT_HAS_START | DT_HAS_STOP | DT_HAS_RESET | DT_HAS_ROM_REGION | DT_HAS_INLINE_CONFIG | DT_HAS_MACHINE_CONFIG
#define DEVTEMPLATE_NAME                "SNUG BwG Floppy Disk Controller Card"
#define DEVTEMPLATE_SHORTNAME           "snugfdc"
#define DEVTEMPLATE_FAMILY              "Peripheral expansion"
#include "devtempl.h"

DEFINE_LEGACY_DEVICE( BWG, ti99_bwg );

