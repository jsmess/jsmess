/*
    TI Standard Floppy Disk Controller
    September 2010

    Michael Zapf
*/
#include "emu.h"
#include "peribox.h"
#include "ti_fdc.h"
#include "machine/wd17xx.h"
#include "formats/ti99_dsk.h"
#include "imagedev/flopdrv.h"

#define CRU_BASE 0x1100

#define fdc_IRQ 1
#define fdc_DRQ 2

#define fdc_region "fdc_region"

typedef ti99_pebcard_config ti99_fdc_config;

typedef struct _ti99_fdc_state
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
	int					SIDSEL;

	/* count 4.23s from rising edge of motor_on */
	emu_timer			*motor_on_timer;

	/* Link to the FDC1771 controller on the board. */
	device_t		*controller;

	/* DSR ROM */
	UINT8				*rom;

	/* Callback lines to the main system. */
	ti99_peb_connect	lines;

} ti99_fdc_state;

/* Those defines are required because the WD17xx DEVICE_START implementation
assumes that the floppy devices are either at root level or at the parent
level. Our floppy devices, however, are at the grandparent level as seen from
the controller. */

#define PFLOPPY_0 "peribox:floppy0"
#define PFLOPPY_1 "peribox:floppy1"
#define PFLOPPY_2 "peribox:floppy2"

INLINE ti99_fdc_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == TIFDC);

	return (ti99_fdc_state *)downcast<legacy_device_base *>(device)->token();
}

/*
    Call this when the state of DSKhold or DRQ/IRQ or DVENA change

    Emulation is faulty because the CPU is actually stopped in the midst of
    instruction, at the end of the memory access

    TODO: This has to be replaced by the proper READY handling that is already
    prepared here. (Requires READY handling by the CPU.)
*/
static void fdc_handle_hold(device_t *device)
{
	ti99_fdc_state *card = get_safe_token(device);
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
		logerror("ti99/FDC: Drive not found\n");
}

static void set_all_geometries(device_t *device, floppy_type_t type)
{
	set_geometry(device->machine().device(PFLOPPY_0), type);
	set_geometry(device->machine().device(PFLOPPY_1), type);
	set_geometry(device->machine().device(PFLOPPY_2), type);
}


/*************************************************************************/

/*
    The CRU read handler.
    bit 0: HLD pin
    bit 1-3: drive n active
    bit 4: 0: motor strobe on
    bit 5: always 0
    bit 6: always 1
    bit 7: selected side
*/
static READ8Z_DEVICE_HANDLER( cru_r )
{
	ti99_fdc_state *card = get_safe_token(device);

	if ((offset & 0xff00)==CRU_BASE)
	{
		int addr = offset & 0x07;
		UINT8 reply = 0;
		if (addr == 0)
		{
			// deliver bits 0-7
			// TODO: HLD pin
			// The DVENA state is returned inverted
			if (card->DVENA) reply |= ((card->DSEL)<<1);
			else reply |= 0x10;
			reply |= 0x40;
			if (card->SIDSEL) reply |= 0x80;
		}
		*value = reply;
	}
}

static WRITE8_DEVICE_HANDLER( cru_w )
{
	ti99_fdc_state *card = get_safe_token(device);
	int drive, drivebit;

	if ((offset & 0xff00)==CRU_BASE)
	{
		int bit = (offset >> 1) & 0x07;
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
				fdc_handle_hold(device);
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
			fdc_handle_hold(device);
			break;

		case 3:
			/* Load disk heads (HLT pin) (bit 3). Not implemented. */
			break;

		case 4:
		case 5:
		case 6:
			/* Select drive X (bits 4-6) */
			drive = bit-4;					/* drive # (0-2) */
			drivebit = 1<<drive;

			if (data)
			{
				if (!(card->DSEL & drivebit))			/* select drive */
				{
					if (card->DSEL != 0)
						logerror("ti_fdc: Multiple drives selected, %02x\n", card->DSEL);
					card->DSEL |= drivebit;
					wd17xx_set_drive(card->controller, drive);
					/*wd17xx_set_side(DSKside);*/
				}
			}
			else
				card->DSEL &= ~drivebit;
			break;

		case 7:
			/* Select side of disk (bit 7) */
			card->SIDSEL = data;
			wd17xx_set_side(card->controller, data);
			break;
		}
	}
}

/*
    Read a byte from the ROM or from the controller
*/
static READ8Z_DEVICE_HANDLER( data_r )
{
	ti99_fdc_state *card = get_safe_token(device);

	if (card->selected)
	{
		if ((offset & card->select_mask)==card->select_value)
		{
			// only use the even addresses from 1ff0 to 1ff6.
			// Note that data is inverted.
			// 0101 1111 1111 0xx0
			UINT8 reply = 0;

			if ((offset & 0x1ff9)==0x1ff0)
				reply = wd17xx_r(card->controller, (offset >> 1)&0x03) ^ 0xff;
			else
				reply = card->rom[offset & 0x1fff];

			*value = reply;
		}
	}
}

/*
    Write a byte to the controller.
*/
static WRITE8_DEVICE_HANDLER( data_w )
{
	ti99_fdc_state *card = get_safe_token(device);
	if (card->selected)
	{
		if ((offset & card->select_mask)==card->select_value)
		{
			// only use the even addresses from 1ff8 to 1ffe.
			// Note that data is inverted.
			// 0101 1111 1111 1xx0
			if ((offset & 0x1ff9)==0x1ff8)
				wd17xx_w(card->controller, (offset >> 1)&0x03, data ^ 0xff);
		}
	}
}

/*
    Callback, called from the controller chip whenever DRQ/IRQ state change
*/
static WRITE_LINE_DEVICE_HANDLER( ti_fdc_intrq_w )
{
	device_t *carddev = device->owner();
	ti99_fdc_state *card = get_safe_token(carddev);

	if (state)
	{
		card->DRQ_IRQ_status |= fdc_IRQ;
		// Note that INTB is actually not used in the TI-99 family. But the
		// controller asserts the line nevertheless
		devcb_call_write_line(&card->lines.intb, TRUE);
	}
	else
	{
		card->DRQ_IRQ_status &= ~fdc_IRQ;
		devcb_call_write_line(&card->lines.intb, FALSE);
	}
	fdc_handle_hold(carddev);
}

static WRITE_LINE_DEVICE_HANDLER( ti_fdc_drq_w )
{
	device_t *carddev = device->owner();
	ti99_fdc_state *card = get_safe_token(carddev);

	if (state)
		card->DRQ_IRQ_status |= fdc_DRQ;
	else
		card->DRQ_IRQ_status &= ~fdc_DRQ;

	fdc_handle_hold(carddev);
}

/*
    callback called at the end of DVENA pulse
*/
static TIMER_CALLBACK(motor_on_timer_callback)
{
	device_t *device = (device_t *)ptr;
	ti99_fdc_state *card = get_safe_token(device);
	card->DVENA = 0;
	fdc_handle_hold(device);
}

const wd17xx_interface ti_wd17xx_interface =
{
	DEVCB_NULL,
	DEVCB_LINE(ti_fdc_intrq_w),
	DEVCB_LINE(ti_fdc_drq_w),
	{ PFLOPPY_0, PFLOPPY_1, PFLOPPY_2, NULL }
};

static const ti99_peb_card fdc_card =
{
	data_r,
	data_w,
	cru_r,
	cru_w,

	NULL, NULL,	NULL, NULL
};

static DEVICE_START( ti99_fdc )
{
	ti99_fdc_state *card = get_safe_token(device);

	/* Resolve the callbacks to the PEB */
	peb_callback_if *topeb = (peb_callback_if *)device->baseconfig().static_config();
	devcb_resolve_write_line(&card->lines.ready, &topeb->ready, device);

	card->motor_on_timer = device->machine().scheduler().timer_alloc(FUNC(motor_on_timer_callback), (void *)device);

	astring *region = new astring();
	astring_assemble_3(region, device->tag(), ":", fdc_region);
	card->rom = device->machine().region(astring_c(region))->base();
	card->controller = device->subdevice("fd1771");
}

static DEVICE_STOP( ti99_fdc )
{
	logerror("ti99_fdc: stop\n");
}

static DEVICE_RESET( ti99_fdc )
{
	ti99_fdc_state *card = get_safe_token(device);

	/* If the card is selected in the menu, register the card */
	if (input_port_read(device->machine(), "DISKCTRL") == DISK_TIFDC)
	{
		device_t *peb = device->owner();
		int success = mount_card(peb, device, &fdc_card, get_pebcard_config(device)->slot);
		if (!success) return;

		card->DSEL = 0;
		card->SIDSEL = 0;
		card->DVENA = 0;
		card->strobe_motor = 0;

		card->select_mask = 0x7e000;
		card->select_value = 0x74000;

		if (input_port_read(device->machine(), "MODE")==GENMOD)
		{
			// GenMod card modification
			card->select_mask = 0x1fe000;
			card->select_value = 0x174000;
		}

		ti99_set_80_track_drives(FALSE);

		floppy_type_t type = FLOPPY_STANDARD_5_25_DSDD_40;
		set_all_geometries(device, type);
	}
	// TODO: Check that:    floppy_mon_w(w->drive, CLEAR_LINE);
}

#if 0
static WRITE_LINE_DEVICE_HANDLER( ti99_fdc_ready )
{
	// Caution: The device pointer passed to this function is the calling
	// device. That is, if we want *this* device, we need to take the owner.
	ti99_fdc_state *card = get_safe_token(device->owner());
	devcb_call_write_line( &card->lines.ready, state );
}
#endif

MACHINE_CONFIG_FRAGMENT( ti99_fdc )
	MCFG_WD1771_ADD("fd1771", ti_wd17xx_interface )
MACHINE_CONFIG_END

ROM_START( ti99_fdc )
	ROM_REGION(0x2000, fdc_region, 0)
	ROM_LOAD_OPTIONAL("disk.bin", 0x0000, 0x2000, CRC(8f7df93f) SHA1(ed91d48c1eaa8ca37d5055bcf67127ea51c4cad5)) /* TI disk DSR ROM */
ROM_END

static const char DEVTEMPLATE_SOURCE[] = __FILE__;

#define DEVTEMPLATE_ID(p,s)             p##ti99_fdc##s
#define DEVTEMPLATE_FEATURES            DT_HAS_START | DT_HAS_STOP | DT_HAS_RESET | DT_HAS_ROM_REGION | DT_HAS_INLINE_CONFIG | DT_HAS_MACHINE_CONFIG
#define DEVTEMPLATE_NAME                "TI Floppy Disk Controller Card"
#define DEVTEMPLATE_SHORTNAME           "tifdc"
#define DEVTEMPLATE_FAMILY              "Peripheral expansion"
#include "devtempl.h"

DEFINE_LEGACY_DEVICE( TIFDC, ti99_fdc );

