/***************************************************************************
    commodore pet series computer

    peter.trauner@jk.uni-linz.ac.at
***************************************************************************/

#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "cpu/m6809/m6809.h"

#include "includes/cbm.h"
#include "machine/6821pia.h"
#include "machine/6522via.h"
#include "includes/cbmserb.h"
#include "includes/cbmieeeb.h"
#include "includes/cbmdrive.h"

#include "includes/pet.h"

#include "devices/cartslot.h"
#include "devices/cassette.h"

#define VERBOSE_LEVEL 0
#define DBG_LOG( MACHINE, N, M, A ) \
	do { \
		if(VERBOSE_LEVEL >= N) \
		{ \
			if( M ) \
				logerror("%11.6f: %-24s", attotime_to_double(timer_get_time(MACHINE)), (char*) M ); \
			logerror A; \
		} \
	} while (0)

int pet_font;
UINT8 *pet_memory;
UINT8 *superpet_memory;
UINT8 *pet_videoram;
static UINT8 *pet80_bank1_base;
static int pet_keyline_select;

static emu_timer *datasette1_timer;
static emu_timer *datasette2_timer;


static READ8_HANDLER( pet_mc6845_register_r )
{
	const device_config *devconf = devtag_get_device(space->machine, "crtc");
	return mc6845_register_r(devconf, offset);
}

static WRITE8_HANDLER( pet_mc6845_register_w )
{
	const device_config *devconf = devtag_get_device(space->machine, "crtc");
	mc6845_register_w(devconf, offset, data);
}

static WRITE8_HANDLER( pet_mc6845_address_w )
{
	const device_config *devconf = devtag_get_device(space->machine, "crtc");
	mc6845_address_w(devconf, offset, data);
}

/* pia at 0xe810
   port a
    7 sense input (low for diagnostics)
    6 ieee eoi in
    5 cassette 2 switch in
    4 cassette 1 switch in
    3..0 keyboard line select

  ca1 cassette 1 read
  ca2 ieee eoi out

  cb1 video sync in
  cb2 cassette 1 motor out
*/
static READ8_DEVICE_HANDLER( pet_pia0_port_a_read )
{
	const device_config *ieeebus = devtag_get_device(device->machine, "ieee_bus");
	int data = 0xf0 | pet_keyline_select;

	if ((cassette_get_state(devtag_get_device(device->machine, "cassette1")) & CASSETTE_MASK_UISTATE) != CASSETTE_STOPPED)
		data &= ~0x10;

	if ((cassette_get_state(devtag_get_device(device->machine, "cassette2")) & CASSETTE_MASK_UISTATE) != CASSETTE_STOPPED)
		data &= ~0x20;

	if (!cbm_ieee_eoi_r(ieeebus, 0))
		data &= ~0x40;

	return data;
}

static WRITE8_DEVICE_HANDLER( pet_pia0_port_a_write )
{
	pet_keyline_select = data & 0x0f;
}

/* Keyboard reading/handling for regular keyboard */
static  READ8_DEVICE_HANDLER( pet_pia0_port_b_read )
{
	UINT8 data = 0xff;
	static const char *const keynames[] = { "ROW0", "ROW1", "ROW2", "ROW3", "ROW4",
										"ROW5", "ROW6", "ROW7", "ROW8", "ROW9" };

	if (pet_keyline_select < 10)
	{
		data = input_port_read(device->machine, keynames[pet_keyline_select]);
		/* Check for left-shift lock */
		if ((pet_keyline_select == 8) && (input_port_read(device->machine, "SPECIAL") & 0x80))
			data &= 0xfe;
	}
	return data;
}

/* Keyboard handling for business keyboard */
static READ8_DEVICE_HANDLER( petb_pia0_port_b_read )
{
	UINT8 data = 0xff;
	pet_state *state = device->machine->driver_data;
	static const char *const keynames[] = { "ROW0", "ROW1", "ROW2", "ROW3", "ROW4",
										"ROW5", "ROW6", "ROW7", "ROW8", "ROW9" };

	if (pet_keyline_select < 10)
	{
		data = input_port_read(device->machine, keynames[pet_keyline_select]);
		/* Check for left-shift lock */
		/* 2008-05 FP: For some reason, superpet read it in the opposite way!! */
		/* While waiting for confirmation from docs, we add a workaround here. */
		if (state->superpet)
		{
			if ((pet_keyline_select == 6) && !(input_port_read(device->machine, "SPECIAL") & 0x80))
				data &= 0xfe;
		}
		else
		{
			if ((pet_keyline_select == 6) && (input_port_read(device->machine, "SPECIAL") & 0x80))
				data &= 0xfe;
		}
	}
	return data;
}

/* NOT WORKING - Just placeholder */
static READ8_DEVICE_HANDLER( pet_pia0_ca1_in )
{
	// cassette 1 read
	return (cassette_input(devtag_get_device(device->machine, "cassette1")) > +0.0) ? 1 : 0;
}


static WRITE8_DEVICE_HANDLER( pet_pia0_ca2_out )
{
	const device_config *ieeebus = devtag_get_device(device->machine, "ieee_bus");
	cbm_ieee_eoi_write(ieeebus, 0, data);
}

static WRITE8_DEVICE_HANDLER( pet_pia0_cb2_out )
{
	if (!data)
	{
		cassette_change_state(devtag_get_device(device->machine, "cassette1"),CASSETTE_MOTOR_ENABLED,CASSETTE_MASK_MOTOR);
		timer_adjust_periodic(datasette1_timer, attotime_zero, 0, ATTOTIME_IN_HZ(48000));	// I put 48000 because I was given some .wav with this freq
	}
	else
	{
		cassette_change_state(devtag_get_device(device->machine, "cassette1"),CASSETTE_MOTOR_DISABLED ,CASSETTE_MASK_MOTOR);
		timer_reset(datasette1_timer, attotime_never);
	}
}


static WRITE_LINE_DEVICE_HANDLER( pet_irq )
{
	int level = state ? 1 : 0;
	static int old_level = 0;
	pet_state *driver_state = device->machine->driver_data;
	if (level != old_level)
	{
		DBG_LOG(device->machine, 3, "mos6502", ("irq %s\n", level ? "start" : "end"));
		if (driver_state->superpet)
			cputag_set_input_line(device->machine, "m6809", M6809_IRQ_LINE, level);
		cputag_set_input_line(device->machine, "maincpu", M6502_IRQ_LINE, level);
		old_level = level;
	}
}

/* pia at 0xe820 (ieee488)
   port a data in
   port b data out
  ca1 atn in
  ca2 ndac out
  cb1 srq in
  cb2 dav out
 */
static READ8_DEVICE_HANDLER( pet_pia1_port_a_read )
{
	const device_config *ieeebus = devtag_get_device(device->machine, "ieee_bus");
	return cbm_ieee_data_r(ieeebus, 0);
}

static WRITE8_DEVICE_HANDLER( pet_pia1_port_b_write )
{
	const device_config *ieeebus = devtag_get_device(device->machine, "ieee_bus");
	cbm_ieee_data_write(ieeebus, 0, data);
}

static READ8_DEVICE_HANDLER( pet_pia1_ca1_read )
{
	const device_config *ieeebus = devtag_get_device(device->machine, "ieee_bus");
	return cbm_ieee_atn_r(ieeebus,0 );
}

static WRITE8_DEVICE_HANDLER( pet_pia1_ca2_write )
{
	const device_config *ieeebus = devtag_get_device(device->machine, "ieee_bus");
	cbm_ieee_ndac_write(ieeebus, 0, data);
}

static WRITE8_DEVICE_HANDLER( pet_pia1_cb2_write )
{
	const device_config *ieeebus = devtag_get_device(device->machine, "ieee_bus");
	cbm_ieee_dav_write(ieeebus, 0, data);
}

static READ8_DEVICE_HANDLER( pet_pia1_cb1_read )
{
	const device_config *ieeebus = devtag_get_device(device->machine, "ieee_bus");
	return cbm_ieee_srq_r(ieeebus, 0);
}

const pia6821_interface pet_pia0 =
{
	DEVCB_HANDLER(pet_pia0_port_a_read),		/* in_a_func */
	DEVCB_HANDLER(pet_pia0_port_b_read),		/* in_b_func */
	DEVCB_HANDLER(pet_pia0_ca1_in),			/* in_ca1_func */
	DEVCB_NULL,						/* in_cb1_func */
	DEVCB_NULL,						/* in_ca2_func */
	DEVCB_NULL,						/* in_cb2_func */
	DEVCB_HANDLER(pet_pia0_port_a_write),		/* out_a_func */
	DEVCB_NULL,						/* out_b_func */
	DEVCB_HANDLER(pet_pia0_ca2_out),			/* out_ca2_func */
	DEVCB_HANDLER(pet_pia0_cb2_out),			/* out_cb2_func */
	DEVCB_NULL,						/* irq_a_func */
	DEVCB_LINE(pet_irq)						/* irq_b_func */
};

const pia6821_interface petb_pia0 =
{
	DEVCB_HANDLER(pet_pia0_port_a_read),		/* in_a_func */
	DEVCB_HANDLER(petb_pia0_port_b_read),		/* in_b_func */
	DEVCB_HANDLER(pet_pia0_ca1_in),			/* in_ca1_func */
	DEVCB_NULL,						/* in_cb1_func */
	DEVCB_NULL,						/* in_ca2_func */
	DEVCB_NULL,						/* in_cb2_func */
	DEVCB_HANDLER(pet_pia0_port_a_write),		/* out_a_func */
	DEVCB_NULL,						/* out_b_func */
	DEVCB_HANDLER(pet_pia0_ca2_out),			/* out_ca2_func */
	DEVCB_HANDLER(pet_pia0_cb2_out),			/* out_cb2_func */
	DEVCB_NULL,						/* irq_a_func */
	DEVCB_LINE(pet_irq)						/* irq_b_func */
};

const pia6821_interface pet_pia1 =
{
	DEVCB_HANDLER(pet_pia1_port_a_read),		/* in_a_func */
	DEVCB_NULL,						/* in_b_func */
	DEVCB_HANDLER(pet_pia1_ca1_read),			/* in_ca1_func */
	DEVCB_HANDLER(pet_pia1_cb1_read),			/* in_cb1_func */
	DEVCB_NULL,						/* in_ca2_func */
	DEVCB_NULL,						/* in_cb2_func */
	DEVCB_NULL,						/* out_a_func */
	DEVCB_HANDLER(pet_pia1_port_b_write),		/* out_b_func */
	DEVCB_HANDLER(pet_pia1_ca2_write),			/* out_ca2_func */
	DEVCB_HANDLER(pet_pia1_cb2_write),			/* out_cb2_func */
	DEVCB_NULL,
	DEVCB_NULL
};

static WRITE8_DEVICE_HANDLER( pet_address_line_11 )
{
	DBG_LOG(device->machine, 1, "address line", ("%d\n", data));
	if (data) pet_font |= 1;
	else pet_font &= ~1;
}

/* userport, cassettes, rest ieee488
   ca1 userport
   ca2 character rom address line 11
   pa user port

   pb0 ieee ndac in
   pb1 ieee nrfd out
   pb2 ieee atn out
   pb3 userport/cassettes
   pb4 cassettes
   pb5 userport/???
   pb6 ieee nrfd in
   pb7 ieee dav in

   cb1 cassettes
   cb2 user port
 */
static READ8_DEVICE_HANDLER( pet_via_port_b_r )
{
	const device_config *ieeebus = devtag_get_device(device->machine, "ieee_bus");
	UINT8 data = 0;

	if (cbm_ieee_ndac_r(ieeebus, 0))
		data |= 0x01;

	//  data & 0x08 -> cassette write (it seems to BOTH cassettes from schematics)

	if (!(data & 0x10))
	{
		cassette_change_state(devtag_get_device(device->machine, "cassette2"),CASSETTE_MOTOR_ENABLED,CASSETTE_MASK_MOTOR);
		timer_adjust_periodic(datasette2_timer, attotime_zero, 0, ATTOTIME_IN_HZ(48000));	// I put 48000 because I was given some .wav with this freq
	}
	else
	{
		cassette_change_state(devtag_get_device(device->machine, "cassette2"),CASSETTE_MOTOR_DISABLED ,CASSETTE_MASK_MOTOR);
		timer_reset(datasette2_timer, attotime_never);
	}

	if (cbm_ieee_nrfd_r(ieeebus, 0))
		data |= 0x40;

	if (cbm_ieee_dav_r(ieeebus, 0))
		data |= 0x80;

	return data;
}

/* NOT WORKING - Just placeholder */
static READ8_DEVICE_HANDLER( pet_via_cb1_r )
{
	// cassette 2 read
	return (cassette_input(devtag_get_device(device->machine, "cassette2")) > +0.0) ? 1 : 0;
}


static WRITE8_DEVICE_HANDLER( pet_via_port_b_w )
{
	const device_config *ieeebus = devtag_get_device(device->machine, "ieee_bus");
	cbm_ieee_nrfd_write(ieeebus, 0, data & 2);
	cbm_ieee_atn_write(ieeebus, 0, data & 4);
}


const via6522_interface pet_via =
{
	DEVCB_NULL,					/* in_a_func */
	DEVCB_HANDLER(pet_via_port_b_r),		/* in_b_func */
	DEVCB_NULL,					/* in_ca1_func */
	DEVCB_HANDLER(pet_via_cb1_r),			/* in_cb1_func */
	DEVCB_NULL,					/* in_ca2_func */
	DEVCB_NULL,					/* in_cb2_func */
	DEVCB_NULL,					/* out_a_func */
	DEVCB_HANDLER(pet_via_port_b_w),		/* out_b_func */
	DEVCB_HANDLER(pet_address_line_11)		/* out_ca2_func */
};

static struct {
	int bank; /* rambank to be switched in 0x9000 */
	int rom; /* rom socket 6502? at 0x9000 */
} spet= { 0 };

static WRITE8_HANDLER( cbm8096_io_w )
{
	const device_config *via_0 = devtag_get_device(space->machine, "via6522_0");
	const device_config *pia_0 = devtag_get_device(space->machine, "pia_0");
	const device_config *pia_1 = devtag_get_device(space->machine, "pia_1");

	if (offset < 0x10) ;
	else if (offset < 0x14) pia6821_w(pia_0, offset & 3, data);
	else if (offset < 0x20) ;
	else if (offset < 0x24) pia6821_w(pia_1, offset & 3, data);
	else if (offset < 0x40) ;
	else if (offset < 0x50) via_w(via_0, offset & 0xf, data);
	else if (offset < 0x80) ;
	else if (offset == 0x80) pet_mc6845_address_w(space, offset, data);
	else if (offset == 0x81) pet_mc6845_register_w(space, offset, data);
}

static READ8_HANDLER( cbm8096_io_r )
{
	const device_config *via_0 = devtag_get_device(space->machine, "via6522_0");
	const device_config *pia_0 = devtag_get_device(space->machine, "pia_0");
	const device_config *pia_1 = devtag_get_device(space->machine, "pia_1");

	int data = 0xff;
	if (offset < 0x10) ;
	else if (offset < 0x14) data = pia6821_r(pia_0, offset & 3);
	else if (offset < 0x20) ;
	else if (offset < 0x24) data = pia6821_r(pia_1, offset & 3);
	else if (offset < 0x40) ;
	else if (offset < 0x50) data = via_r(via_0, offset & 0xf);
	else if (offset < 0x80) ;
	else if (offset == 0x81) data = pet_mc6845_register_r(space, offset);
	return data;
}

static WRITE8_HANDLER( pet80_bank1_w )
{
	pet80_bank1_base[offset] = data;
}

/*
65520        8096 memory control register
        bit 0    1=write protect $8000-BFFF
        bit 1    1=write protect $C000-FFFF
        bit 2    $8000-BFFF bank select
        bit 3    $C000-FFFF bank select
        bit 5    1=screen peek through
        bit 6    1=I/O peek through
        bit 7    1=enable expansion memory

*/
WRITE8_HANDLER( cbm8096_w )
{
	read8_space_func rh;
	write8_space_func wh;

	if (data & 0x80)
	{
		if (data & 0x40)
		{
			rh = cbm8096_io_r;
			wh = cbm8096_io_w;
		}
		else
		{
			rh = SMH_BANK(7);
			if (!(data & 2))
				wh = SMH_BANK(7);
			else
				wh = SMH_NOP;
		}
		memory_install_read8_handler(space, 0xe800, 0xefff, 0, 0, rh);
		memory_install_write8_handler(space, 0xe800, 0xefff, 0, 0, wh);

		memory_install_write8_handler(space, 0xc000, 0xe7ff, 0, 0, (data & 2) == 0 ? SMH_BANK(6) : SMH_NOP);
		memory_install_write8_handler(space, 0xf000, 0xffef, 0, 0, (data & 2) == 0 ? SMH_BANK(8) : SMH_NOP);
		memory_install_write8_handler(space, 0xfff1, 0xffff, 0, 0, (data & 2) == 0 ? SMH_BANK(9) : SMH_NOP);

		if (data & 0x20)
		{
			pet80_bank1_base = pet_memory + 0x8000;
			memory_set_bankptr(space->machine, 1, pet80_bank1_base);
			wh = pet80_bank1_w;
		}
		else
		{
			if (!(data & 1))
				wh = SMH_BANK(1);
			else
				wh = SMH_NOP;
		}
		memory_install_write8_handler(space, 0x8000, 0x8fff, 0, 0, wh);

		memory_install_write8_handler(space, 0x9000, 0x9fff, 0, 0, (data & 1) == 0 ? SMH_BANK(2) : SMH_NOP);
		memory_install_write8_handler(space, 0xa000, 0xafff, 0, 0, (data & 1) == 0 ? SMH_BANK(3) : SMH_NOP);
		memory_install_write8_handler(space, 0xb000, 0xbfff, 0, 0, (data & 1) == 0 ? SMH_BANK(4) : SMH_NOP);


		if (data & 4)
		{
			if (!(data & 0x20))
			{
				pet80_bank1_base = pet_memory + 0x14000;
				memory_set_bankptr(space->machine, 1, pet80_bank1_base);
			}
			memory_set_bankptr(space->machine, 2, pet_memory + 0x15000);
			memory_set_bankptr(space->machine, 3, pet_memory + 0x16000);
			memory_set_bankptr(space->machine, 4, pet_memory + 0x17000);
		}
		else
		{
			if (!(data & 0x20))
			{
				pet80_bank1_base = pet_memory + 0x10000;
				memory_set_bankptr(space->machine, 1, pet80_bank1_base);
			}
			memory_set_bankptr(space->machine, 2, pet_memory + 0x11000);
			memory_set_bankptr(space->machine, 3, pet_memory + 0x12000);
			memory_set_bankptr(space->machine, 4, pet_memory + 0x13000);
		}

		if (data & 8)
		{
			if (!(data & 0x40))
			{
				memory_set_bankptr(space->machine, 7, pet_memory + 0x1e800);
			}
			memory_set_bankptr(space->machine, 6, pet_memory + 0x1c000);
			memory_set_bankptr(space->machine, 8, pet_memory + 0x1f000);
			memory_set_bankptr(space->machine, 9, pet_memory + 0x1fff1);
		}
		else
		{
			if (!(data & 0x40))
			{
				memory_set_bankptr(space->machine, 7, pet_memory+ 0x1a800);
			}
			memory_set_bankptr(space->machine, 6, pet_memory + 0x18000);
			memory_set_bankptr(space->machine, 8, pet_memory + 0x1b000);
			memory_set_bankptr(space->machine, 9, pet_memory + 0x1bff1);
		}
	}
	else
	{
		pet80_bank1_base = pet_memory + 0x8000;
		memory_set_bankptr(space->machine, 1, pet80_bank1_base );
		memory_install_write8_handler(space, 0x8000, 0x8fff, 0, 0, pet80_bank1_w);

		memory_set_bankptr(space->machine, 2, pet_memory + 0x9000);
		memory_install_write8_handler(space, 0x9000, 0x9fff, 0, 0, SMH_UNMAP);

		memory_set_bankptr(space->machine, 3, pet_memory + 0xa000);
		memory_install_write8_handler(space, 0xa000, 0xafff, 0, 0, SMH_UNMAP);

		memory_set_bankptr(space->machine, 4, pet_memory + 0xb000);
		memory_install_write8_handler(space, 0xb000, 0xbfff, 0, 0, SMH_UNMAP);

		memory_set_bankptr(space->machine, 6, pet_memory + 0xc000);
		memory_install_write8_handler(space, 0xc000, 0xe7ff, 0, 0, SMH_UNMAP);

		memory_install_read8_handler(space, 0xe800, 0xefff, 0, 0, cbm8096_io_r);
		memory_install_write8_handler(space, 0xe800, 0xefff, 0, 0, cbm8096_io_w);

		memory_set_bankptr(space->machine, 8, pet_memory + 0xf000);
		memory_install_write8_handler(space, 0xf000, 0xffef, 0, 0, SMH_UNMAP);

		memory_set_bankptr(space->machine, 9, pet_memory + 0xfff1);
		memory_install_write8_handler(space, 0xfff1, 0xffff, 0, 0, SMH_UNMAP);
	}
}

READ8_HANDLER( superpet_r )
{
	return 0xff;
}

WRITE8_HANDLER( superpet_w )
{
	switch (offset)
	{
		case 0:
		case 1:
		case 2:
		case 3:
			/* 3: 1 pull down diagnostic pin on the userport
               1: 1 if jumpered programable ram r/w
               0: 0 if jumpered programable m6809, 1 m6502 selected */
			break;

		case 4:
		case 5:
			spet.bank = data & 0xf;
			memory_configure_bank(space->machine, 1, 0, 16, superpet_memory, 0x1000);
			memory_set_bank(space->machine, 1, spet.bank);
			/* 7 low writeprotects systemlatch */
			break;

		case 6:
		case 7:
			spet.rom = data & 1;
			break;
	}
}

static TIMER_CALLBACK( pet_interrupt )
{
	const device_config *pia_0 = devtag_get_device(machine, "pia_0");
	static int level = 0;

	pia6821_cb1_w(pia_0, 0, level);
	level = !level;
}


/* NOT WORKING - Just placeholder */
static TIMER_CALLBACK( pet_tape1_timer )
{
	const device_config *pia_0 = devtag_get_device(machine, "pia_0");
//  cassette 1
	UINT8 data = (cassette_input(devtag_get_device(machine, "cassette1")) > +0.0) ? 1 : 0;
	pia6821_ca1_w(pia_0, 0, data);
}

/* NOT WORKING - Just placeholder */
static TIMER_CALLBACK( pet_tape2_timer )
{
	const device_config *via_0 = devtag_get_device(machine, "via6522_0");
//  cassette 2
	UINT8 data = (cassette_input(devtag_get_device(machine, "cassette2")) > +0.0) ? 1 : 0;
	via_cb1_w(via_0, 0, data);
}


static void pet_common_driver_init( running_machine *machine )
{
	int i;
	pet_state *state = machine->driver_data;

	pet_font = 0;

	state->pet_basic1 = 0;
	state->superpet = 0;
	state->cbm8096 = 0;

	memory_install_read8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x0000, mess_ram_size - 1, 0, 0, SMH_BANK(10));
	memory_install_write8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x0000, mess_ram_size - 1, 0, 0, SMH_BANK(10));
	memory_set_bankptr(machine, 10, pet_memory);

	if (mess_ram_size < 0x8000)
	{
		memory_install_read8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), mess_ram_size, 0x7FFF, 0, 0, SMH_NOP);
		memory_install_write8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), mess_ram_size, 0x7FFF, 0, 0, SMH_NOP);
	}

	/* 2114 poweron ? 64 x 0xff, 64x 0, and so on */
	for (i = 0; i < mess_ram_size; i += 0x40)
	{
		memset (pet_memory + i, i & 0x40 ? 0 : 0xff, 0x40);
	}

	/* pet clock */
	timer_pulse(machine, ATTOTIME_IN_MSEC(10), NULL, 0, pet_interrupt);

	/* datasette */
	datasette1_timer = timer_alloc(machine, pet_tape1_timer, NULL);
	datasette2_timer = timer_alloc(machine, pet_tape2_timer, NULL);
}


DRIVER_INIT( pet2001 )
{
	pet_state *state = machine->driver_data;
	pet_memory = mess_ram;
	pet_common_driver_init(machine);
	state->pet_basic1 = 1;
	pet_vh_init(machine);
}

DRIVER_INIT( pet )
{
	pet_memory = mess_ram;
	pet_common_driver_init(machine);
	pet_vh_init(machine);
}

DRIVER_INIT( pet80 )
{
	pet_state *state = machine->driver_data;
	pet_memory = memory_region(machine, "maincpu");

	pet_common_driver_init(machine);
	state->cbm8096 = 1;
	videoram = &pet_memory[0x8000];
	videoram_size = 0x800;
	pet80_vh_init(machine);

}

DRIVER_INIT( superpet )
{
	pet_state *state = machine->driver_data;
	pet_memory = mess_ram;
	pet_common_driver_init(machine);
	state->superpet = 1;

	superpet_memory = auto_alloc_array(machine, UINT8, 0x10000);

	memory_configure_bank(machine, 1, 0, 16, superpet_memory, 0x1000);
	memory_set_bank(machine, 1, 0);

	superpet_vh_init(machine);
}

MACHINE_RESET( pet )
{
	pet_state *state = machine->driver_data;

	if (state->superpet)
	{
		spet.rom = 0;
		if (input_port_read(machine, "CFG") & 0x04)
		{
			cputag_set_input_line(machine, "maincpu", INPUT_LINE_HALT, 1);
			cputag_set_input_line(machine, "maincpu", INPUT_LINE_HALT, 0);
			pet_font = 2;
		}
		else
		{
			cputag_set_input_line(machine, "maincpu", INPUT_LINE_HALT, 0);
			cputag_set_input_line(machine, "maincpu", INPUT_LINE_HALT, 1);
			pet_font = 0;
		}
	}

	if (state->cbm8096)
	{
		if (input_port_read(machine, "CFG") & 0x08)
		{
			memory_install_write8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xfff0, 0xfff0, 0, 0, cbm8096_w);
		}
		else
		{
			memory_install_write8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xfff0, 0xfff0, 0, 0, SMH_NOP);
		}
		cbm8096_w(cputag_get_address_space(machine,"maincpu",ADDRESS_SPACE_PROGRAM), 0, 0);
	}

	cbm_drive_0_config (input_port_read(machine, "CFG") & 2 ? IEEE : 0, 8);
	cbm_drive_1_config (input_port_read(machine, "CFG") & 1 ? IEEE : 0, 9);
	device_reset(cputag_get_cpu(machine, "maincpu"));
}


INTERRUPT_GEN( pet_frame_interrupt )
{
	pet_state *state = device->machine->driver_data;
	if (state->superpet)
	{
		if (input_port_read(device->machine, "CFG") & 0x04)
		{
			cpu_set_input_line(device, INPUT_LINE_HALT, 1);
			cpu_set_input_line(device, INPUT_LINE_HALT, 0);
			pet_font |= 2;
		}
		else
		{
			cpu_set_input_line(device, INPUT_LINE_HALT, 0);
			cpu_set_input_line(device, INPUT_LINE_HALT, 1);
			pet_font &= ~2;
		}
	}

	set_led_status (1, input_port_read(device->machine, "SPECIAL") & 0x80 ? 1 : 0);		/* Shift Lock */
}


/***********************************************

    PET Cartridges

***********************************************/


static CBM_ROM pet_cbm_cart[0x20] = { {0} };


static DEVICE_IMAGE_LOAD(pet_cart)
{
	int size = image_length(image), test;
	const char *filetype;
	int address = 0;

	filetype = image_filetype(image);

 	if (!mame_stricmp(filetype, "crt"))
	{
	/* We temporarily remove .crt loading. Previous versions directly used
    the same routines used to load C64 .crt file, but I seriously doubt the
    formats are compatible. While waiting for confirmation about .crt dumps
    for PET machines, we simply do not load .crt files */
	}
	else
	{
		/* Assign loading address according to extension */
		if (!mame_stricmp(filetype, "a0"))
			address = 0xa000;

		else if (!mame_stricmp(filetype, "b0"))
			address = 0xb000;

		logerror("Loading cart %s at %.4x size:%.4x\n", image_filename(image), address, size);

		/* Does cart contain any data? */
		pet_cbm_cart[0].chip = (UINT8*) image_malloc(image, size);
		if (!pet_cbm_cart[0].chip)
			return INIT_FAIL;

		/* Store data, address & size */
		pet_cbm_cart[0].addr = address;
		pet_cbm_cart[0].size = size;
		test = image_fread(image, pet_cbm_cart[0].chip, pet_cbm_cart[0].size);

		if (test != pet_cbm_cart[0].size)
			return INIT_FAIL;
	}

	/* Finally load the cart */
//  This could be needed with .crt support
//  for (i = 0; (i < sizeof(pet_cbm_cart) / sizeof(pet_cbm_cart[0])) && (pet_cbm_cart[i].size != 0); i++)
//      memcpy(pet_memory + pet_cbm_cart[i].addr, pet_cbm_cart[i].chip, pet_cbm_cart[i].size);
	memcpy(pet_memory + pet_cbm_cart[0].addr, pet_cbm_cart[0].chip, pet_cbm_cart[0].size);

	return INIT_PASS;
}

MACHINE_DRIVER_START(pet_cartslot)
	MDRV_CARTSLOT_ADD("cart1")
	MDRV_CARTSLOT_EXTENSION_LIST("crt,a0,b0")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_LOAD(pet_cart)

	MDRV_CARTSLOT_ADD("cart2")
	MDRV_CARTSLOT_EXTENSION_LIST("crt,a0,b0")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_LOAD(pet_cart)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(pet4_cartslot)
	MDRV_CARTSLOT_MODIFY("cart1")
	MDRV_CARTSLOT_EXTENSION_LIST("crt,a0")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_LOAD(pet_cart)

	MDRV_CARTSLOT_MODIFY("cart2")
	MDRV_CARTSLOT_EXTENSION_LIST("crt,a0")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_LOAD(pet_cart)
MACHINE_DRIVER_END
