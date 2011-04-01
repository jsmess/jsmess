/***************************************************************************
    commodore pet series computer

    peter.trauner@jk.uni-linz.ac.at
***************************************************************************/

#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "cpu/m6809/m6809.h"

#include "machine/6821pia.h"
#include "machine/6522via.h"
#include "machine/ieee488.h"

#include "includes/pet.h"

#include "imagedev/cartslot.h"
#include "imagedev/cassette.h"
#include "machine/ram.h"

#define VERBOSE_LEVEL 0
#define DBG_LOG( MACHINE, N, M, A ) \
	do { \
		if(VERBOSE_LEVEL >= N) \
		{ \
			if( M ) \
				logerror("%11.6f: %-24s", MACHINE.time().as_double(), (char*) M ); \
			logerror A; \
		} \
	} while (0)



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
static READ8_DEVICE_HANDLER( pia0_pa_r )
{
	pet_state *state = device->machine().driver_data<pet_state>();
	/*

        bit     description

        PA0     KEY A
        PA1     KEY B
        PA2     KEY C
        PA3     KEY D
        PA4     #1 CASS SWITCH
        PA5     #2 CASS SWITCH
        PA6     _EOI IN
        PA7     DIAG JUMPER

    */

	device_t *ieeebus = device->machine().device("ieee_bus");
	UINT8 data = 0;

	/* key */
	data |= state->m_keyline_select;

	/* #1 cassette switch */
	data |= ((cassette_get_state(device->machine().device("cassette1")) & CASSETTE_MASK_UISTATE) == CASSETTE_STOPPED) << 4;

	/* #2 cassette switch */
	data |= ((cassette_get_state(device->machine().device("cassette2")) & CASSETTE_MASK_UISTATE) == CASSETTE_STOPPED) << 5;

	/* end or identify in */
	data |= ieee488_eoi_r(ieeebus) << 6;

	/* diagnostic jumper */
	data |= 0x80;

	return data;
}

static WRITE8_DEVICE_HANDLER( pia0_pa_w )
{
	pet_state *state = device->machine().driver_data<pet_state>();
	/*

        bit     description

        PA0     KEY A
        PA1     KEY B
        PA2     KEY C
        PA3     KEY D
        PA4     #1 CASS SWITCH
        PA5     #2 CASS SWITCH
        PA6     _EOI IN
        PA7     DIAG JUMPER

    */

	/* key */
	state->m_keyline_select = data & 0x0f;
}

/* Keyboard reading/handling for regular keyboard */
static READ8_DEVICE_HANDLER( kin_r )
{
	pet_state *state = device->machine().driver_data<pet_state>();
	/*

        bit     description

        PB0     KIN0
        PB1     KIN1
        PB2     KIN2
        PB3     KIN3
        PB4     KIN4
        PB5     KIN5
        PB6     KIN6
        PB7     KIN7

    */

	UINT8 data = 0xff;
	static const char *const keynames[] = {
		"ROW0", "ROW1", "ROW2", "ROW3", "ROW4",
		"ROW5", "ROW6", "ROW7", "ROW8", "ROW9"
	};

	if (state->m_keyline_select < 10)
	{
		data = input_port_read(device->machine(), keynames[state->m_keyline_select]);
		/* Check for left-shift lock */
		if ((state->m_keyline_select == 8) && (input_port_read(device->machine(), "SPECIAL") & 0x80))
			data &= 0xfe;
	}
	return data;
}

/* Keyboard handling for business keyboard */
static READ8_DEVICE_HANDLER( petb_kin_r )
{
	UINT8 data = 0xff;
	pet_state *state = device->machine().driver_data<pet_state>();
	static const char *const keynames[] = {
		"ROW0", "ROW1", "ROW2", "ROW3", "ROW4",
		"ROW5", "ROW6", "ROW7", "ROW8", "ROW9"
	};

	if (state->m_keyline_select < 10)
	{
		data = input_port_read(device->machine(), keynames[state->m_keyline_select]);
		/* Check for left-shift lock */
		/* 2008-05 FP: For some reason, superpet read it in the opposite way!! */
		/* While waiting for confirmation from docs, we add a workaround here. */
		if (state->m_superpet)
		{
			if ((state->m_keyline_select == 6) && !(input_port_read(device->machine(), "SPECIAL") & 0x80))
				data &= 0xfe;
		}
		else
		{
			if ((state->m_keyline_select == 6) && (input_port_read(device->machine(), "SPECIAL") & 0x80))
				data &= 0xfe;
		}
	}
	return data;
}

static READ8_DEVICE_HANDLER( cass1_r )
{
	// cassette 1 read
	return (cassette_input(device->machine().device("cassette1")) > +0.0) ? 1 : 0;
}

static WRITE8_DEVICE_HANDLER( eoi_w )
{
	device_t *ieeebus = device->machine().device("ieee_bus");

	ieee488_eoi_w(ieeebus, device, data);
}

static WRITE8_DEVICE_HANDLER( cass1_motor_w )
{
	pet_state *state = device->machine().driver_data<pet_state>();
	if (!data)
	{
		cassette_change_state(device->machine().device("cassette1"),CASSETTE_MOTOR_ENABLED,CASSETTE_MASK_MOTOR);
		state->m_datasette1_timer->adjust(attotime::zero, 0, attotime::from_hz(48000));	// I put 48000 because I was given some .wav with this freq
	}
	else
	{
		cassette_change_state(device->machine().device("cassette1"),CASSETTE_MOTOR_DISABLED ,CASSETTE_MASK_MOTOR);
		state->m_datasette1_timer->reset();
	}
}

static WRITE_LINE_DEVICE_HANDLER( pia0_irq_w )
{
	pet_state *driver_state = device->machine().driver_data<pet_state>();

	driver_state->m_pia0_irq = state;
	int level = (driver_state->m_pia0_irq | driver_state->m_pia1_irq | driver_state->m_via_irq) ? ASSERT_LINE : CLEAR_LINE;

	device_set_input_line(device->machine().firstcpu, INPUT_LINE_IRQ0, level);
}

const pia6821_interface pet_pia0 =
{
	DEVCB_HANDLER(pia0_pa_r),		/* in_a_func */
	DEVCB_HANDLER(kin_r),			/* in_b_func */
	DEVCB_HANDLER(cass1_r),			/* in_ca1_func */
	DEVCB_NULL,						/* in_cb1_func */
	DEVCB_NULL,						/* in_ca2_func */
	DEVCB_NULL,						/* in_cb2_func */
	DEVCB_HANDLER(pia0_pa_w),		/* out_a_func */
	DEVCB_NULL,						/* out_b_func */
	DEVCB_HANDLER(eoi_w),			/* out_ca2_func */
	DEVCB_HANDLER(cass1_motor_w),	/* out_cb2_func */
	DEVCB_LINE(pia0_irq_w),			/* irq_a_func */
	DEVCB_LINE(pia0_irq_w)			/* irq_b_func */
};

const pia6821_interface petb_pia0 =
{
	DEVCB_HANDLER(pia0_pa_r),		/* in_a_func */
	DEVCB_HANDLER(petb_kin_r),		/* in_b_func */
	DEVCB_HANDLER(cass1_r),			/* in_ca1_func */
	DEVCB_NULL,						/* in_cb1_func */
	DEVCB_NULL,						/* in_ca2_func */
	DEVCB_NULL,						/* in_cb2_func */
	DEVCB_HANDLER(pia0_pa_w),		/* out_a_func */
	DEVCB_NULL,						/* out_b_func */
	DEVCB_HANDLER(eoi_w),			/* out_ca2_func */
	DEVCB_HANDLER(cass1_motor_w),	/* out_cb2_func */
	DEVCB_LINE(pia0_irq_w),			/* irq_a_func */
	DEVCB_LINE(pia0_irq_w)			/* irq_b_func */
};

/* pia at 0xe820 (ieee488)
   port a data in
   port b data out
  ca1 atn in
  ca2 ndac out
  cb1 srq in
  cb2 dav out
 */

static WRITE8_DEVICE_HANDLER( do_w )
{
	/*

        bit     description

        PB0     DO-1
        PB1     DO-2
        PB2     DO-3
        PB3     DO-4
        PB4     DO-5
        PB5     DO-6
        PB6     DO-7
        PB7     DO-8

    */

	device_t *ieeebus = device->machine().device("ieee_bus");

	ieee488_dio_w(ieeebus, device, data);
}

static WRITE8_DEVICE_HANDLER( ndac_w )
{
	device_t *ieeebus = device->machine().device("ieee_bus");

	ieee488_ndac_w(ieeebus, device, data);
}

static WRITE8_DEVICE_HANDLER( dav_w )
{
	device_t *ieeebus = device->machine().device("ieee_bus");

	ieee488_dav_w(ieeebus, device, data);
}

static WRITE_LINE_DEVICE_HANDLER( pia1_irq_w )
{
	pet_state *driver_state = device->machine().driver_data<pet_state>();

	driver_state->m_pia1_irq = state;
	int level = (driver_state->m_pia0_irq | driver_state->m_pia1_irq | driver_state->m_via_irq) ? ASSERT_LINE : CLEAR_LINE;

	device_set_input_line(device->machine().firstcpu, INPUT_LINE_IRQ0, level);
}

const pia6821_interface pet_pia1 =
{
	DEVCB_DEVICE_HANDLER("ieee_bus", ieee488_dio_r),/* in_a_func */
	DEVCB_NULL,								/* in_b_func */
	DEVCB_DEVICE_LINE("ieee_bus", ieee488_atn_r),	/* in_ca1_func */
	DEVCB_DEVICE_LINE("ieee_bus", ieee488_srq_r),	/* in_cb1_func */
	DEVCB_NULL,								/* in_ca2_func */
	DEVCB_NULL,								/* in_cb2_func */
	DEVCB_NULL,								/* out_a_func */
	DEVCB_HANDLER(do_w),					/* out_b_func */
	DEVCB_HANDLER(ndac_w),					/* out_ca2_func */
	DEVCB_HANDLER(dav_w),					/* out_cb2_func */
	DEVCB_LINE(pia1_irq_w),					/* irq_a_func */
	DEVCB_LINE(pia1_irq_w)					/* irq_b_func */
};

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
static READ8_DEVICE_HANDLER( via_pb_r )
{
	/*

        bit     description

        PB0     _NDAC IN
        PB1     _NRFD OUT
        PB2     _ATN OUT
        PB3     CASS WRITE
        PB4     #2 CASS MOTOR
        PB5     SYNC IN
        PB6     _NRFD IN
        PB7     _DAV IN

    */

	device_t *ieeebus = device->machine().device("ieee_bus");
	UINT8 data = 0;

	/* not data accepted in */
	data |= ieee488_ndac_r(ieeebus);

	/* sync in */

	/* not ready for data in */
	data |= ieee488_nrfd_r(ieeebus) << 6;

	/* data valid in */
	data |= ieee488_dav_r(ieeebus) << 7;

	return data;
}

static READ_LINE_DEVICE_HANDLER( cass2_r )
{
	// cassette 2 read
	return (cassette_input(device->machine().device("cassette2")) > +0.0) ? 1 : 0;
}

static WRITE8_DEVICE_HANDLER( via_pb_w )
{
	pet_state *state = device->machine().driver_data<pet_state>();
	/*

        bit     description

        PB0     _NDAC IN
        PB1     _NRFD OUT
        PB2     _ATN OUT
        PB3     CASS WRITE
        PB4     #2 CASS MOTOR
        PB5     SYNC IN
        PB6     _NRFD IN
        PB7     _DAV IN

    */

	device_t *ieeebus = device->machine().device("ieee_bus");

	/* not ready for data out */
	ieee488_nrfd_w(ieeebus, device, BIT(data, 1));

	/* attention out */
	ieee488_atn_w(ieeebus, device, BIT(data, 2));

	/* cassette write */
	cassette_output(device->machine().device("cassette1"), BIT(data, 3) ? -(0x5a9e >> 1) : +(0x5a9e >> 1));
	cassette_output(device->machine().device("cassette2"), BIT(data, 3) ? -(0x5a9e >> 1) : +(0x5a9e >> 1));

	/* #2 cassette motor */
	if (BIT(data, 4))
	{
		cassette_change_state(device->machine().device("cassette2"), CASSETTE_MOTOR_ENABLED, CASSETTE_MASK_MOTOR);
		state->m_datasette2_timer->adjust(attotime::zero, 0, attotime::from_hz(48000));	// I put 48000 because I was given some .wav with this freq
	}
	else
	{
		cassette_change_state(device->machine().device("cassette2"), CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);
		state->m_datasette2_timer->reset();
	}
}

static WRITE_LINE_DEVICE_HANDLER( gb_w )
{
	pet_state *drvstate = device->machine().driver_data<pet_state>();
	DBG_LOG(device->machine(), 1, "address line", ("%d\n", state));
	if (state) drvstate->m_font |= 1;
	else drvstate->m_font &= ~1;
}

static WRITE_LINE_DEVICE_HANDLER( via_irq_w )
{
	pet_state *driver_state = device->machine().driver_data<pet_state>();

	driver_state->m_via_irq = state;
	int level = (driver_state->m_pia0_irq | driver_state->m_pia1_irq | driver_state->m_via_irq) ? ASSERT_LINE : CLEAR_LINE;

	device_set_input_line(device->machine().firstcpu, INPUT_LINE_IRQ0, level);
}

const via6522_interface pet_via =
{
	DEVCB_NULL,					/* in_a_func */
	DEVCB_HANDLER(via_pb_r),	/* in_b_func */
	DEVCB_NULL,					/* in_ca1_func */
	DEVCB_LINE(cass2_r),		/* in_cb1_func */
	DEVCB_NULL,					/* in_ca2_func */
	DEVCB_NULL,					/* in_cb2_func */
	DEVCB_NULL,					/* out_a_func */
	DEVCB_HANDLER(via_pb_w),	/* out_b_func */
	DEVCB_NULL,					/* out_ca1_func */
	DEVCB_LINE(gb_w),			/* out_ca2_func */
	DEVCB_NULL,					/* out_ca2_func */
	DEVCB_NULL,					/* out_cb2_func */
	DEVCB_LINE(via_irq_w)		/* out_irq_func */
};


static WRITE8_HANDLER( cbm8096_io_w )
{
	via6522_device *via_0 = space->machine().device<via6522_device>("via6522_0");;
	device_t *pia_0 = space->machine().device("pia_0");
	device_t *pia_1 = space->machine().device("pia_1");
	device_t *mc6845 = space->machine().device("crtc");

	if (offset < 0x10) ;
	else if (offset < 0x14) pia6821_w(pia_0, offset & 3, data);
	else if (offset < 0x20) ;
	else if (offset < 0x24) pia6821_w(pia_1, offset & 3, data);
	else if (offset < 0x40) ;
	else if (offset < 0x50) via_0->write(*space, offset & 0xf, data);
	else if (offset < 0x80) ;
	else if (offset == 0x80) mc6845_address_w(mc6845, 0, data);
	else if (offset == 0x81) mc6845_register_w(mc6845, 0, data);
}

static READ8_HANDLER( cbm8096_io_r )
{
	via6522_device *via_0 = space->machine().device<via6522_device>("via6522_0");;
	device_t *pia_0 = space->machine().device("pia_0");
	device_t *pia_1 = space->machine().device("pia_1");
	device_t *mc6845 = space->machine().device("crtc");

	int data = 0xff;
	if (offset < 0x10) ;
	else if (offset < 0x14) data = pia6821_r(pia_0, offset & 3);
	else if (offset < 0x20) ;
	else if (offset < 0x24) data = pia6821_r(pia_1, offset & 3);
	else if (offset < 0x40) ;
	else if (offset < 0x50) data = via_0->read(*space, offset & 0xf);
	else if (offset < 0x80) ;
	else if (offset == 0x81) data = mc6845_register_r(mc6845, 0);
	return data;
}

static WRITE8_HANDLER( pet80_bank1_w )
{
	pet_state *state = space->machine().driver_data<pet_state>();
	state->m_pet80_bank1_base[offset] = data;
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
	pet_state *state = space->machine().driver_data<pet_state>();
	if (data & 0x80)
	{
		if (data & 0x40)
		{
			space->install_legacy_read_handler(0xe800, 0xefff, FUNC(cbm8096_io_r));
			space->install_legacy_write_handler(0xe800, 0xefff, FUNC(cbm8096_io_w));
		}
		else
		{
			space->install_read_bank(0xe800, 0xefff, "bank7");
			if (!(data & 2))
				space->install_write_bank(0xe800, 0xefff, "bank7");
			else
				space->nop_write(0xe800, 0xefff);
		}


		if ((data & 2) == 0) {
			space->install_write_bank(0xc000, 0xe7ff, "bank6");
			space->install_write_bank(0xf000, 0xffef, "bank8");
			space->install_write_bank(0xfff1, 0xffff, "bank9");
		} else {
			space->nop_write(0xc000, 0xe7ff);
			space->nop_write(0xf000, 0xffef);
			space->nop_write(0xfff1, 0xffff);
		}

		if (data & 0x20)
		{
			state->m_pet80_bank1_base = state->m_memory + 0x8000;
			memory_set_bankptr(space->machine(), "bank1", state->m_pet80_bank1_base);
			space->install_legacy_write_handler(0x8000, 0x8fff, FUNC(pet80_bank1_w));
		}
		else
		{
			if (!(data & 1))
				space->install_write_bank(0x8000, 0x8fff, "bank1");
			else
				space->nop_write(0x8000, 0x8fff);
		}

		if ((data & 1) == 0 ){
			space->install_write_bank(0x9000, 0x9fff, "bank2");
			space->install_write_bank(0xa000, 0xafff, "bank3");
			space->install_write_bank(0xb000, 0xbfff, "bank4");
		} else {
			space->nop_write(0x9000, 0x9fff);
			space->nop_write(0xa000, 0xafff);
			space->nop_write(0xb000, 0xbfff);
		}

		if (data & 4)
		{
			if (!(data & 0x20))
			{
				state->m_pet80_bank1_base = state->m_memory + 0x14000;
				memory_set_bankptr(space->machine(), "bank1", state->m_pet80_bank1_base);
			}
			memory_set_bankptr(space->machine(), "bank2", state->m_memory + 0x15000);
			memory_set_bankptr(space->machine(), "bank3", state->m_memory + 0x16000);
			memory_set_bankptr(space->machine(), "bank4", state->m_memory + 0x17000);
		}
		else
		{
			if (!(data & 0x20))
			{
				state->m_pet80_bank1_base = state->m_memory + 0x10000;
				memory_set_bankptr(space->machine(), "bank1", state->m_pet80_bank1_base);
			}
			memory_set_bankptr(space->machine(), "bank2", state->m_memory + 0x11000);
			memory_set_bankptr(space->machine(), "bank3", state->m_memory + 0x12000);
			memory_set_bankptr(space->machine(), "bank4", state->m_memory + 0x13000);
		}

		if (data & 8)
		{
			if (!(data & 0x40))
			{
				memory_set_bankptr(space->machine(), "bank7", state->m_memory + 0x1e800);
			}
			memory_set_bankptr(space->machine(), "bank6", state->m_memory + 0x1c000);
			memory_set_bankptr(space->machine(), "bank8", state->m_memory + 0x1f000);
			memory_set_bankptr(space->machine(), "bank9", state->m_memory + 0x1fff1);
		}
		else
		{
			if (!(data & 0x40))
			{
				memory_set_bankptr(space->machine(), "bank7", state->m_memory+ 0x1a800);
			}
			memory_set_bankptr(space->machine(), "bank6", state->m_memory + 0x18000);
			memory_set_bankptr(space->machine(), "bank8", state->m_memory + 0x1b000);
			memory_set_bankptr(space->machine(), "bank9", state->m_memory + 0x1bff1);
		}
	}
	else
	{
		state->m_pet80_bank1_base = state->m_memory + 0x8000;
		memory_set_bankptr(space->machine(), "bank1", state->m_pet80_bank1_base );
		space->install_legacy_write_handler(0x8000, 0x8fff, FUNC(pet80_bank1_w));

		memory_set_bankptr(space->machine(), "bank2", state->m_memory + 0x9000);
		space->unmap_write(0x9000, 0x9fff);

		memory_set_bankptr(space->machine(), "bank3", state->m_memory + 0xa000);
		space->unmap_write(0xa000, 0xafff);

		memory_set_bankptr(space->machine(), "bank4", state->m_memory + 0xb000);
		space->unmap_write(0xb000, 0xbfff);

		memory_set_bankptr(space->machine(), "bank6", state->m_memory + 0xc000);
		space->unmap_write(0xc000, 0xe7ff);

		space->install_legacy_read_handler(0xe800, 0xefff, FUNC(cbm8096_io_r));
		space->install_legacy_write_handler(0xe800, 0xefff, FUNC(cbm8096_io_w));

		memory_set_bankptr(space->machine(), "bank8", state->m_memory + 0xf000);
		space->unmap_write(0xf000, 0xffef);

		memory_set_bankptr(space->machine(), "bank9", state->m_memory + 0xfff1);
		space->unmap_write(0xfff1, 0xffff);
	}
}

READ8_HANDLER( superpet_r )
{
	return 0xff;
}

WRITE8_HANDLER( superpet_w )
{
	pet_state *state = space->machine().driver_data<pet_state>();
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
			state->m_spet.bank = data & 0xf;
			memory_configure_bank(space->machine(), "bank1", 0, 16, state->m_supermemory, 0x1000);
			memory_set_bank(space->machine(), "bank1", state->m_spet.bank);
			/* 7 low writeprotects systemlatch */
			break;

		case 6:
		case 7:
			state->m_spet.rom = data & 1;
			break;
	}
}

static TIMER_CALLBACK( pet_interrupt )
{
	pet_state *state = machine.driver_data<pet_state>();
	device_t *pia_0 = machine.device("pia_0");

	pia6821_cb1_w(pia_0, state->m_pia_level);
	state->m_pia_level = !state->m_pia_level;
}


static TIMER_CALLBACK( pet_tape1_timer )
{
	device_t *pia_0 = machine.device("pia_0");
//  cassette 1
	UINT8 data = (cassette_input(machine.device("cassette1")) > +0.0) ? 1 : 0;
	pia6821_ca1_w(pia_0, data);
}

static TIMER_CALLBACK( pet_tape2_timer )
{
	via6522_device *via_0 = machine.device<via6522_device>("via6522_0");
//  cassette 2
	UINT8 data = (cassette_input(machine.device("cassette2")) > +0.0) ? 1 : 0;
	via_0->write_cb1(data);
}


static void pet_common_driver_init( running_machine &machine )
{
	int i;
	pet_state *state = machine.driver_data<pet_state>();

	state->m_font = 0;

	state->m_pet_basic1 = 0;
	state->m_superpet = 0;
	state->m_cbm8096 = 0;

	machine.device("maincpu")->memory().space(AS_PROGRAM)->install_readwrite_bank(0x0000, ram_get_size(machine.device(RAM_TAG)) - 1, "bank10");
	memory_set_bankptr(machine, "bank10", state->m_memory);

	if (ram_get_size(machine.device(RAM_TAG)) < 0x8000)
	{
		machine.device("maincpu")->memory().space(AS_PROGRAM)->nop_readwrite(ram_get_size(machine.device(RAM_TAG)), 0x7FFF);
	}

	/* 2114 poweron ? 64 x 0xff, 64x 0, and so on */
	for (i = 0; i < ram_get_size(machine.device(RAM_TAG)); i += 0x40)
	{
		memset (state->m_memory + i, i & 0x40 ? 0 : 0xff, 0x40);
	}

	/* pet clock */
	machine.scheduler().timer_pulse(attotime::from_msec(10), FUNC(pet_interrupt));

	/* datasette */
	state->m_datasette1_timer = machine.scheduler().timer_alloc(FUNC(pet_tape1_timer));
	state->m_datasette2_timer = machine.scheduler().timer_alloc(FUNC(pet_tape2_timer));
}


DRIVER_INIT( pet2001 )
{
	pet_state *state = machine.driver_data<pet_state>();
	state->m_memory = ram_get_ptr(machine.device(RAM_TAG));
	pet_common_driver_init(machine);
	state->m_pet_basic1 = 1;
	pet_vh_init(machine);
}

DRIVER_INIT( pet )
{
	pet_state *state = machine.driver_data<pet_state>();
	state->m_memory = ram_get_ptr(machine.device(RAM_TAG));
	pet_common_driver_init(machine);
	pet_vh_init(machine);
}

DRIVER_INIT( pet80 )
{
	pet_state *state = machine.driver_data<pet_state>();
	state->m_memory = machine.region("maincpu")->base();

	pet_common_driver_init(machine);
	state->m_cbm8096 = 1;
	state->m_videoram = &state->m_memory[0x8000];
	pet80_vh_init(machine);

}

DRIVER_INIT( superpet )
{
	pet_state *state = machine.driver_data<pet_state>();
	state->m_memory = ram_get_ptr(machine.device(RAM_TAG));
	pet_common_driver_init(machine);
	state->m_superpet = 1;

	state->m_supermemory = auto_alloc_array(machine, UINT8, 0x10000);

	memory_configure_bank(machine, "bank1", 0, 16, state->m_supermemory, 0x1000);
	memory_set_bank(machine, "bank1", 0);

	superpet_vh_init(machine);
}

MACHINE_RESET( pet )
{
	pet_state *state = machine.driver_data<pet_state>();
	device_t *ieeebus = machine.device("ieee_bus");
	device_t *scapegoat = machine.device("pia_0");

	if (state->m_superpet)
	{
		state->m_spet.rom = 0;
		if (input_port_read(machine, "CFG") & 0x04)
		{
			cputag_set_input_line(machine, "maincpu", INPUT_LINE_HALT, 1);
			cputag_set_input_line(machine, "maincpu", INPUT_LINE_HALT, 0);
			state->m_font = 2;
		}
		else
		{
			cputag_set_input_line(machine, "maincpu", INPUT_LINE_HALT, 0);
			cputag_set_input_line(machine, "maincpu", INPUT_LINE_HALT, 1);
			state->m_font = 0;
		}
	}

	if (state->m_cbm8096)
	{
		if (input_port_read(machine, "CFG") & 0x08)
		{
			machine.device("maincpu")->memory().space(AS_PROGRAM)->install_legacy_write_handler(0xfff0, 0xfff0, FUNC(cbm8096_w));
		}
		else
		{
			machine.device("maincpu")->memory().space(AS_PROGRAM)->nop_write(0xfff0, 0xfff0);
		}
		cbm8096_w(machine.device("maincpu")->memory().space(AS_PROGRAM), 0, 0);
	}

//removed   cbm_drive_0_config (input_port_read(machine, "CFG") & 2 ? IEEE : 0, 8);
//removed   cbm_drive_1_config (input_port_read(machine, "CFG") & 1 ? IEEE : 0, 9);
	machine.device("maincpu")->reset();

	ieee488_ren_w(ieeebus, scapegoat, 0);
	ieee488_ifc_w(ieeebus, scapegoat, 0);
	ieee488_ifc_w(ieeebus, scapegoat, 1);
}


INTERRUPT_GEN( pet_frame_interrupt )
{
	pet_state *state = device->machine().driver_data<pet_state>();
	if (state->m_superpet)
	{
		if (input_port_read(device->machine(), "CFG") & 0x04)
		{
			device_set_input_line(device, INPUT_LINE_HALT, 1);
			device_set_input_line(device, INPUT_LINE_HALT, 0);
			state->m_font |= 2;
		}
		else
		{
			device_set_input_line(device, INPUT_LINE_HALT, 0);
			device_set_input_line(device, INPUT_LINE_HALT, 1);
			state->m_font &= ~2;
		}
	}

	set_led_status (device->machine(),1, input_port_read(device->machine(), "SPECIAL") & 0x80 ? 1 : 0);		/* Shift Lock */
}


/***********************************************

    PET Cartridges

***********************************************/

static DEVICE_IMAGE_LOAD(pet_cart)
{
	pet_state *state = image.device().machine().driver_data<pet_state>();
	UINT32 size = image.length();
	const char *filetype = image.filetype();
	int address = 0;

	/* Assign loading address according to extension */
	if (!mame_stricmp(filetype, "90"))
		address = 0x9000;
	else if (!mame_stricmp(filetype, "a0"))
		address = 0xa000;
	else if (!mame_stricmp(filetype, "b0"))
		address = 0xb000;

	logerror("Loading cart %s at %.4x size:%.4x\n", image.filename(), address, size);

	image.fread(state->m_memory + address, size);

	return IMAGE_INIT_PASS;
}

MACHINE_CONFIG_FRAGMENT(pet_cartslot)
	MCFG_CARTSLOT_ADD("cart1")
	MCFG_CARTSLOT_EXTENSION_LIST("90,a0,b0")
	MCFG_CARTSLOT_NOT_MANDATORY
	MCFG_CARTSLOT_LOAD(pet_cart)

	MCFG_CARTSLOT_ADD("cart2")
	MCFG_CARTSLOT_EXTENSION_LIST("90,a0,b0")
	MCFG_CARTSLOT_NOT_MANDATORY
	MCFG_CARTSLOT_LOAD(pet_cart)
MACHINE_CONFIG_END

// 2010-08, FP: this is used by CBM40 & CBM80, and I actually have only found .prg files for these... does cart dumps exist?
MACHINE_CONFIG_FRAGMENT(pet4_cartslot)
	MCFG_CARTSLOT_MODIFY("cart1")
	MCFG_CARTSLOT_EXTENSION_LIST("a0")
	MCFG_CARTSLOT_NOT_MANDATORY
	MCFG_CARTSLOT_LOAD(pet_cart)

	MCFG_CARTSLOT_MODIFY("cart2")
	MCFG_CARTSLOT_EXTENSION_LIST("a0")
	MCFG_CARTSLOT_NOT_MANDATORY
	MCFG_CARTSLOT_LOAD(pet_cart)
MACHINE_CONFIG_END
