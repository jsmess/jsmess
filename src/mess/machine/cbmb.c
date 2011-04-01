/***************************************************************************
    commodore b series computer

    peter.trauner@jk.uni-linz.ac.at
***************************************************************************/

#include "emu.h"
#include "cpu/m6502/m6509.h"
#include "sound/sid6581.h"
#include "machine/6525tpi.h"
#include "machine/6526cia.h"
#include "machine/ieee488.h"

#include "includes/cbmb.h"
#include "crsshair.h"

#include "imagedev/cartslot.h"

/* 2008-05 FP: Were these added as a reminder to add configs of
drivers 8 & 9 as in pet.c ? */
#define IEEE8ON 0
#define IEEE9ON 0

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

static TIMER_CALLBACK( cbmb_frame_interrupt );
/* keyboard lines */


/* tpi at 0xfde00
 in interrupt mode
 irq to m6509 irq
 pa7 ieee nrfd
 pa6 ieee ndac
 pa5 ieee eoi
 pa4 ieee dav
 pa3 ieee atn
 pa2 ieee ren
 pa1 ieee io chips te
 pa0 ieee io chip dc
 pb1 ieee seq
 pb0 ieee ifc
 cb ?
 ca chargen rom address line 12
 i4 acia irq
 i3 ?
 i2 cia6526 irq
 i1 self pb0
 i0 tod (50 or 60 hertz frequency)
 */
READ8_DEVICE_HANDLER( cbmb_tpi0_port_a_r )
{
	device_t *ieeebus = device->machine().device("ieee_bus");
	UINT8 data = 0;

	if (ieee488_nrfd_r(ieeebus))
		data |= 0x80;

	if (ieee488_ndac_r(ieeebus))
		data |= 0x40;

	if (ieee488_eoi_r(ieeebus))
		data |= 0x20;

	if (ieee488_dav_r(ieeebus))
		data |= 0x10;

	if (ieee488_atn_r(ieeebus))
		data |= 0x08;

	if (ieee488_ren_r(ieeebus))
        data |= 0x04;

	return data;
}

WRITE8_DEVICE_HANDLER( cbmb_tpi0_port_a_w )
{
	device_t *ieeebus = device->machine().device("ieee_bus");

	ieee488_nrfd_w(ieeebus, device, BIT(data, 7));
	ieee488_ndac_w(ieeebus, device, BIT(data, 6));
	ieee488_eoi_w(ieeebus, device, BIT(data, 5));
	ieee488_dav_w(ieeebus, device, BIT(data, 4));
	ieee488_atn_w(ieeebus, device, BIT(data, 3));
	ieee488_ren_w(ieeebus, device, BIT(data, 2));
}

READ8_DEVICE_HANDLER( cbmb_tpi0_port_b_r )
{
	device_t *ieeebus = device->machine().device("ieee_bus");
	UINT8 data = 0;

	if (ieee488_srq_r(ieeebus))
		data |= 0x02;

	if (ieee488_ifc_r(ieeebus))
		data |= 0x01;

	return data;
}

WRITE8_DEVICE_HANDLER( cbmb_tpi0_port_b_w )
{
	device_t *ieeebus = device->machine().device("ieee_bus");

	ieee488_srq_w(ieeebus, device, BIT(data, 1));
	ieee488_ifc_w(ieeebus, device, BIT(data, 0));
}

/* tpi at 0xfdf00
  cbm 500
   port c7 video address lines?
   port c6 ?
  cbm 600
   port c7 low
   port c6 ntsc 1, 0 pal
  cbm 700
   port c7 high
   port c6 ?
  port c5 .. c0 keyboard line select
  port a7..a0 b7..b0 keyboard input */
WRITE8_DEVICE_HANDLER( cbmb_keyboard_line_select_a )
{
	cbmb_state *state = device->machine().driver_data<cbmb_state>();
	state->m_keyline_a = data;
}

WRITE8_DEVICE_HANDLER( cbmb_keyboard_line_select_b )
{
	cbmb_state *state = device->machine().driver_data<cbmb_state>();
	state->m_keyline_b = data;
}

WRITE8_DEVICE_HANDLER( cbmb_keyboard_line_select_c )
{
	cbmb_state *state = device->machine().driver_data<cbmb_state>();
	state->m_keyline_c = data;
}

READ8_DEVICE_HANDLER( cbmb_keyboard_line_a )
{
	cbmb_state *state = device->machine().driver_data<cbmb_state>();
	int data = 0;
	if (!(state->m_keyline_c & 0x01))
		data |= input_port_read(device->machine(), "ROW0");

	if (!(state->m_keyline_c & 0x02))
		data |= input_port_read(device->machine(), "ROW2");

	if (!(state->m_keyline_c & 0x04))
		data |= input_port_read(device->machine(), "ROW4");

	if (!(state->m_keyline_c & 0x08))
		data |= input_port_read(device->machine(), "ROW6");

	if (!(state->m_keyline_c & 0x10))
		data |= input_port_read(device->machine(), "ROW8");

	if (!(state->m_keyline_c & 0x20))
		data |= input_port_read(device->machine(), "ROW10");

	return data ^0xff;
}

READ8_DEVICE_HANDLER( cbmb_keyboard_line_b )
{
	cbmb_state *state = device->machine().driver_data<cbmb_state>();
	int data = 0;
	if (!(state->m_keyline_c & 0x01))
		data |= input_port_read(device->machine(), "ROW1");

	if (!(state->m_keyline_c & 0x02))
		data |= input_port_read(device->machine(), "ROW3");

	if (!(state->m_keyline_c & 0x04))
		data |= input_port_read(device->machine(), "ROW5");

	if (!(state->m_keyline_c & 0x08))
		data |= input_port_read(device->machine(), "ROW7");

	if (!(state->m_keyline_c & 0x10))
		data |= input_port_read(device->machine(), "ROW9") | ((input_port_read(device->machine(), "SPECIAL") & 0x04) ? 1 : 0 );

	if (!(state->m_keyline_c & 0x20))
		data |= input_port_read(device->machine(), "ROW11");

	return data ^0xff;
}

READ8_DEVICE_HANDLER( cbmb_keyboard_line_c )
{
	int data = 0;
	cbmb_state *state = device->machine().driver_data<cbmb_state>();
	if ((input_port_read(device->machine(), "ROW0") & ~state->m_keyline_a) ||
				(input_port_read(device->machine(), "ROW1") & ~state->m_keyline_b))
		 data |= 0x01;

	if ((input_port_read(device->machine(), "ROW2") & ~state->m_keyline_a) ||
				(input_port_read(device->machine(), "ROW3") & ~state->m_keyline_b))
		 data |= 0x02;

	if ((input_port_read(device->machine(), "ROW4") & ~state->m_keyline_a) ||
				(input_port_read(device->machine(), "ROW5") & ~state->m_keyline_b))
		 data |= 0x04;

	if ((input_port_read(device->machine(), "ROW6") & ~state->m_keyline_a) ||
				(input_port_read(device->machine(), "ROW7") & ~state->m_keyline_b))
		 data |= 0x08;

	if ((input_port_read(device->machine(), "ROW8") & ~state->m_keyline_a) ||
				((input_port_read(device->machine(), "ROW9") | ((input_port_read(device->machine(), "SPECIAL") & 0x04) ? 1 : 0)) & ~state->m_keyline_b))
		 data |= 0x10;

	if ((input_port_read(device->machine(), "ROW10") & ~state->m_keyline_a) ||
				(input_port_read(device->machine(), "ROW11") & ~state->m_keyline_b))
		 data |= 0x20;

	if (!state->m_p500)
	{
		if (!state->m_cbm_ntsc)
			data |= 0x40;

		if (!state->m_cbm700)
			data |= 0x80;
	}
	return data ^0xff;
}

void cbmb_irq( device_t *device, int level )
{
	cbmb_state *state = device->machine().driver_data<cbmb_state>();
	if (level != state->m_old_level)
	{
		DBG_LOG(device->machine(), 3, "mos6509", ("irq %s\n", level ? "start" : "end"));
		cputag_set_input_line(device->machine(), "maincpu", M6502_IRQ_LINE, level);
		state->m_old_level = level;
	}
}

/*
  port a ieee in/out
  pa7 trigger gameport 2
  pa6 trigger gameport 1
  pa1 ?
  pa0 ?
  pb7 .. 4 gameport 2
  pb3 .. 0 gameport 1
 */
static READ8_DEVICE_HANDLER( cbmb_cia_port_a_r )
{
	device_t *ieeebus = device->machine().device("ieee_bus");
	return ieee488_dio_r(ieeebus, 0);
}

static WRITE8_DEVICE_HANDLER( cbmb_cia_port_a_w )
{
	device_t *ieeebus = device->machine().device("ieee_bus");
	ieee488_dio_w(ieeebus, device, data);
}

const mos6526_interface cbmb_cia =
{
	60,
	DEVCB_DEVICE_LINE("tpi6525_0", tpi6525_irq2_level),
	DEVCB_NULL,	/* pc_func */
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(cbmb_cia_port_a_r),
	DEVCB_HANDLER(cbmb_cia_port_a_w),
	DEVCB_NULL,
	DEVCB_NULL
};

WRITE8_HANDLER( cbmb_colorram_w )
{
	cbmb_state *state = space->machine().driver_data<cbmb_state>();
	state->m_colorram[offset] = data | 0xf0;
}

int cbmb_dma_read( running_machine &machine, int offset )
{
	cbmb_state *state = machine.driver_data<cbmb_state>();
	if (offset >= 0x1000)
		return state->m_videoram[offset & 0x3ff];
	else
		return state->m_chargen[offset & 0xfff];
}

int cbmb_dma_read_color( running_machine &machine, int offset )
{
	cbmb_state *state = machine.driver_data<cbmb_state>();
	return state->m_colorram[offset & 0x3ff];
}

WRITE8_DEVICE_HANDLER( cbmb_change_font )
{
	cbmb_vh_set_font(device->machine(), data);
}

static void cbmb_common_driver_init( running_machine &machine )
{
	cbmb_state *state = machine.driver_data<cbmb_state>();
	state->m_chargen = machine.region("maincpu")->base() + 0x100000;
	/*    memset(c64_memory, 0, 0xfd00); */

	machine.scheduler().timer_pulse(attotime::from_msec(10), FUNC(cbmb_frame_interrupt));

	state->m_p500 = 0;
	state->m_cbm700 = 0;
}

DRIVER_INIT( cbm600 )
{
	cbmb_state *state = machine.driver_data<cbmb_state>();
	cbmb_common_driver_init(machine);
	state->m_cbm_ntsc = 1;
	cbm600_vh_init(machine);
}

DRIVER_INIT( cbm600pal )
{
	cbmb_state *state = machine.driver_data<cbmb_state>();
	cbmb_common_driver_init(machine);
	state->m_cbm_ntsc = 0;
	cbm600_vh_init(machine);
}

DRIVER_INIT( cbm600hu )
{
	cbmb_state *state = machine.driver_data<cbmb_state>();
	cbmb_common_driver_init(machine);
	state->m_cbm_ntsc = 0;
}

DRIVER_INIT( cbm700 )
{
	cbmb_state *state = machine.driver_data<cbmb_state>();
	cbmb_common_driver_init(machine);
	state->m_cbm700 = 1;
	state->m_cbm_ntsc = 0;
	cbm700_vh_init(machine);
}

DRIVER_INIT( p500 )
{
	cbmb_state *state = machine.driver_data<cbmb_state>();
	cbmb_common_driver_init(machine);
	state->m_p500 = 1;
	state->m_cbm_ntsc = 1;
}

MACHINE_RESET( cbmb )
{
//removed   cbm_drive_0_config (IEEE8ON ? IEEE : 0, 8);
//removed   cbm_drive_1_config (IEEE9ON ? IEEE : 0, 9);
}


static TIMER_CALLBACK( p500_lightpen_tick )
{
	if ((input_port_read_safe(machine, "CTRLSEL", 0x00) & 0x07) == 0x04)
	{
		/* enable lightpen crosshair */
		crosshair_set_screen(machine, 0, CROSSHAIR_SCREEN_ALL);
	}
	else
	{
		/* disable lightpen crosshair */
		crosshair_set_screen(machine, 0, CROSSHAIR_SCREEN_NONE);
	}
}

static TIMER_CALLBACK(cbmb_frame_interrupt)
{
	cbmb_state *state = machine.driver_data<cbmb_state>();
	device_t *tpi_0 = machine.device("tpi6525_0");

#if 0
	int controller1 = input_port_read(machine, "CTRLSEL") & 0x07;
	int controller2 = input_port_read(machine, "CTRLSEL") & 0x70;
#endif

	tpi6525_irq0_level(tpi_0, state->m_irq_level);
	state->m_irq_level = !state->m_irq_level;
	if (state->m_irq_level) return ;

#if 0
	value = 0xff;
	switch (controller1)
	{
		case 0x00:
			value &= ~input_port_read(machine, "JOY1_1B");			/* Joy1 Directions + Button 1 */
			break;

		case 0x01:
			if (input_port_read(machine, "OTHER") & 0x40)			/* Paddle2 Button */
				value &= ~0x08;
			if (input_port_read(machine, "OTHER") & 0x80)			/* Paddle1 Button */
				value &= ~0x04;
			break;

		case 0x02:
			if (input_port_read(machine, "OTHER") & 0x02)			/* Mouse Button Left */
				value &= ~0x10;
			if (input_port_read(machine, "OTHER") & 0x01)			/* Mouse Button Right */
				value &= ~0x01;
			break;

		case 0x03:
			value &= ~(input_port_read(machine, "JOY1_2B") & 0x1f);	/* Joy1 Directions + Button 1 */
			break;

		case 0x04:
/* was there any input on the lightpen? where is it mapped? */
//          if (input_port_read(machine, "OTHER") & 0x04)           /* Lightpen Signal */
//              value &= ?? ;
			break;

		case 0x07:
			break;

		default:
			logerror("Invalid Controller 1 Setting %d\n", controller1);
			break;
	}

	c64_keyline[8] = value;


	value = 0xff;
	switch (controller2)
	{
		case 0x00:
			value &= ~input_port_read(machine, "JOY2_1B");			/* Joy2 Directions + Button 1 */
			break;

		case 0x10:
			if (input_port_read(machine, "OTHER") & 0x10)			/* Paddle4 Button */
				value &= ~0x08;
			if (input_port_read(machine, "OTHER") & 0x20)			/* Paddle3 Button */
				value &= ~0x04;
			break;

		case 0x20:
			if (input_port_read(machine, "OTHER") & 0x02)			/* Mouse Button Left */
				value &= ~0x10;
			if (input_port_read(machine, "OTHER") & 0x01)			/* Mouse Button Right */
				value &= ~0x01;
			break;

		case 0x30:
			value &= ~(input_port_read(machine, "JOY2_2B") & 0x1f);	/* Joy2 Directions + Button 1 */
			break;

		case 0x40:
/* was there any input on the lightpen? where is it mapped? */
//          if (input_port_read(machine, "OTHER") & 0x04)           /* Lightpen Signal */
//              value &= ?? ;
			break;

		case 0x70:
			break;

		default:
			logerror("Invalid Controller 2 Setting %d\n", controller2);
			break;
	}

	c64_keyline[9] = value;
#endif

// 128u4 FIXME
//  vic2_frame_interrupt (device);

	/* for p500, check if lightpen has been chosen as input: if so, enable crosshair (but c64-like inputs for p500 are not working atm) */
	machine.scheduler().timer_set(attotime::zero, FUNC(p500_lightpen_tick));

	set_led_status(machine, 1, input_port_read(machine, "SPECIAL") & 0x04 ? 1 : 0);		/* Shift Lock */
}


/***********************************************

    CBM Business Computers Cartridges

***********************************************/

static DEVICE_IMAGE_LOAD(cbmb_cart)
{
	UINT32 size = image.length();
	const char *filetype = image.filetype();
	UINT8 *cbmb_memory = image.device().machine().region("maincpu")->base();
	int address = 0;

	/* Assign loading address according to extension */
	if (!mame_stricmp(filetype, "10"))
		address = 0x1000;

	else if (!mame_stricmp(filetype, "20"))
		address = 0x2000;

	else if (!mame_stricmp(filetype, "40"))
		address = 0x4000;

	else if (!mame_stricmp(filetype, "60"))
		address = 0x6000;

	logerror("Loading cart %s at %.4x size:%.4x\n", image.filename(), address, size);

	image.fread(cbmb_memory + 0xf0000 + address, size);

	return IMAGE_INIT_PASS;
}


MACHINE_CONFIG_FRAGMENT(cbmb_cartslot)
	MCFG_CARTSLOT_ADD("cart1")
	MCFG_CARTSLOT_EXTENSION_LIST("10,20,40,60")
	MCFG_CARTSLOT_NOT_MANDATORY
	MCFG_CARTSLOT_LOAD(cbmb_cart)

	MCFG_CARTSLOT_ADD("cart2")
	MCFG_CARTSLOT_EXTENSION_LIST("10,20,40,60")
	MCFG_CARTSLOT_NOT_MANDATORY
	MCFG_CARTSLOT_LOAD(cbmb_cart)
MACHINE_CONFIG_END
