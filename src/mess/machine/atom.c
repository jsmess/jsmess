/***********************************************************************

    atom.c

    Functions to emulate general aspects of the machine (RAM, ROM,
    interrupts, I/O ports)

    Many thanks to Kees van Oss for:
    1.  Tape input/output circuit diagram. It describes in great detail how the 2.4 kHz
        tone, 2.4 kHz tone enable, tape output and tape input are connected.
    2.  The DOS rom for the Atom so I could complete the floppy disc emulation.
    3.  Details of the eprom expansion board for the Atom.
    4.  His demo programs which I used to test the driver.

***********************************************************************/


#include "emu.h"
#include "machine/i8255a.h"
#include "video/m6847.h"
#include "machine/ctronics.h"
#include "machine/i8271.h"
#include "machine/6522via.h"
#include "devices/flopdrv.h"
#include "devices/cassette.h"
#include "includes/atom.h"
#include "sound/speaker.h"


UINT8 atom_8255_porta;
static UINT8 atom_8255_portb;
UINT8 atom_8255_portc;


/* I am not sure if this is correct, the atom appears to have a 2.4 kHz timer used for reading tapes?? */
static int	timer_state = 0;

static running_device *cassette_device_image(running_machine *machine)
{
	return devtag_get_device(machine, "cassette");
}



/***************************************************************************
    PRINTER INTERFACE
***************************************************************************/

/* BUSY from the centronics interface is connected to PA7 of the via */
static READ8_DEVICE_HANDLER( atom_printer_busy )
{
	return centronics_busy_r(device) << 7;
}

/* DATA lines 0 to 6 are connected to PA0 to PA6 of the via */
static WRITE8_DEVICE_HANDLER( atom_printer_data )
{
	centronics_data_w(device, 0, data & 0x7f);
}

const via6522_interface atom_6522_interface =
{
	DEVCB_DEVICE_HANDLER("centronics", atom_printer_busy),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DEVICE_HANDLER("centronics", atom_printer_data),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	/* the CA2 output is connected to the STROBE signal on the centronics */
	DEVCB_DEVICE_LINE("centronics", centronics_strobe_w),
	DEVCB_NULL,
	DEVCB_CPU_INPUT_LINE("maincpu", 0)
};



I8255A_INTERFACE( atom_8255_int )
{
	DEVCB_HANDLER(atom_8255_porta_r),
	DEVCB_HANDLER(atom_8255_portb_r),
	DEVCB_HANDLER(atom_8255_portc_r),
	DEVCB_HANDLER(atom_8255_porta_w),
	DEVCB_HANDLER(atom_8255_portb_w),
	DEVCB_DEVICE_HANDLER("speaker", atom_8255_portc_w),
};

static int previous_i8271_int_state = 0;

static void atom_8271_interrupt_callback(running_device *device, int state)
{
	/* I'm assuming that the nmi is edge triggered */
	/* a interrupt from the fdc will cause a change in line state, and
    the nmi will be triggered, but when the state changes because the int
    is cleared this will not cause another nmi */
	/* I'll emulate it like this to be sure */

	if (state!=previous_i8271_int_state)
	{
		if (state)
		{
			/* I'll pulse it because if I used hold-line I'm not sure
            it would clear - to be checked */
			cputag_set_input_line(device->machine, "maincpu", INPUT_LINE_NMI, PULSE_LINE);
		}
	}

	previous_i8271_int_state = state;
}

const i8271_interface atom_8271_interface=
{
	atom_8271_interrupt_callback,
	NULL,
	{FLOPPY_0, FLOPPY_1}
};

/*
                                                                  +--------+
PC0,  Tape Output ------------------------------------------------|
                                                                  |
                                                  +---------|     | NAND   |------CAS OUT
PC1, Enable 2400 Hz ------------------------------|         |     |        |
                                  +------+        | NAND    |--B--|        |
PC4, 2400 Hz ---------------+-----|  INV | ---A---|         |     +--------+
                            |     +------+        +---------+
                            |
                            2400 Hz


PC4      PC1 PC0     :   A     B  CAS OUT
  0      0      0    :    1     1        1
  0      0      1    :    1     1        0
  0      1      0    :    1     0        1
  0      1      1    :    1     0        1
  1      0      0    :    0     1        1
  1      0      1    :    0     1        0
  1      1      0    :    0     1        1
  1      1      1    :    0     1        0


Here is a detail from the Atom circuit diagram about Tape Input:

PC5, Cassette
Input ----------------------------------------------------------------CAS IN
*/
static TIMER_CALLBACK(atom_timer_callback)
{
	/* change timer state */
	timer_state^=1;

	/* the 2.4 kHz signal is notted (A), and nand'ed with the 2.4kz enable, resulting
    in B. The final cassette output is the result of tape output nand'ed with B */


	{
		unsigned char A;
		unsigned char B;
		unsigned char result;

		/* 2.4 kHz signal - notted */
		A = (~timer_state);
		/* 2.4 kHz signal notted, and anded with 2.4 kHz enable */
		B = (~(A & (atom_8255_portc>>1))) & 0x01;

		result = (~(B & atom_8255_portc)) & 0x01;

		/* tape output */
		cassette_output(cassette_device_image(machine), (result & 0x01) ? -1.0 : +1.0);
	}
}

MACHINE_START( atom )
{
	atom_8255_porta = 0xff;
	atom_8255_portb = 0xff;
	atom_8255_portc = 0xff;

	timer_state = 0;
	timer_pulse(machine, ATTOTIME_IN_HZ(2400*2), NULL, 0, atom_timer_callback);

	/* this is temporary */
	/* Kees van Oss mentions that address 8-b are used for the random number
    generator. I don't know if this is hardware, or random data because the
    ram chips are not cleared at start-up. So at this time, these numbers
    are poked into the memory to simulate it. When I have more details I will fix it */
	memory_region(machine, "maincpu")[0x08] = mame_rand(machine) & 0x0ff;
	memory_region(machine, "maincpu")[0x09] = mame_rand(machine) & 0x0ff;
	memory_region(machine, "maincpu")[0x0a] = mame_rand(machine) & 0x0ff;
	memory_region(machine, "maincpu")[0x0b] = mame_rand(machine) & 0x0ff;
}

READ8_DEVICE_HANDLER (atom_8255_porta_r )
{
	/* logerror("8255: Read port a\n"); */
	return (atom_8255_porta);
}

READ8_DEVICE_HANDLER ( atom_8255_portb_r )
{
	int row;
	static const char *const keynames[] = {
		"KEY0", "KEY1", "KEY2", "KEY3", "KEY4", "KEY5",
		"KEY6", "KEY7", "KEY8", "KEY9", "KEY10", "KEY11"
	};

	row = atom_8255_porta & 0x0f;
	/* logerror("8255: Read port b: %02X %02X\n", input_port_read(machine, port),
                                    input_port_read(machine, "KEY10") & 0xc0); */
	return ((input_port_read(device->machine, keynames[row]) & 0x3f) | (input_port_read(device->machine, "KEY10") & 0xc0));
}

READ8_DEVICE_HANDLER ( atom_8255_portc_r )
{
	running_device *mc6847 = devtag_get_device(device->machine, "mc6847");

	atom_8255_portc &= 0x0f;

	/* cassette input */
	if (cassette_input(cassette_device_image(device->machine)) > 0.0)
	{
		atom_8255_portc |= (1<<5);
	}

	/* 2.4 kHz input */
	if (timer_state)
	{
		atom_8255_portc |= (1<<4);
	}

	atom_8255_portc |= (input_port_read(device->machine, "KEY11") & 0x40);
	atom_8255_portc |= mc6847_fs_r(mc6847) << 7;
	/* logerror("8255: Read port c (%02X)\n",atom_8255.atom_8255_portc); */
	return (atom_8255_portc);
}

/* Atom 6847 modes:

0000xxxx    Text
0001xxxx    64x64   4
0011xxxx    128x64  2
0101xxxx    128x64  4
0111xxxx    128x96  2
1001xxxx    128x96  4
1011xxxx    128x192 2
1101xxxx    128x192 4
1111xxxx    256x192 2

*/

WRITE8_DEVICE_HANDLER( atom_8255_porta_w )
{
	running_device *mc6847 = devtag_get_device(device->machine, "mc6847");

	mc6847_ag_w(mc6847, BIT(data, 4));
	mc6847_gm0_w(mc6847, BIT(data, 5));
	mc6847_gm1_w(mc6847, BIT(data, 6));
	mc6847_gm2_w(mc6847, BIT(data, 7));

	atom_8255_porta = data;
}

WRITE8_DEVICE_HANDLER ( atom_8255_portb_w )
{
	atom_8255_portb = data;
}

WRITE8_DEVICE_HANDLER(atom_8255_portc_w)
{
	running_device *mc6847 = devtag_get_device(device->machine, "mc6847");

	speaker_level_w(device, BIT(data, 2));
	mc6847_css_w(mc6847, BIT(data, 3));

	atom_8255_portc = data;
}


/*********************************************/
/* emulates a 16-slot eprom box for the Atom */
static unsigned char selected_eprom = 0;

static void atom_eprom_box_refresh(running_machine *machine)
{
    unsigned char *eprom_data;

	/* get address of eprom data */
	eprom_data = memory_region(machine, "maincpu") + 0x010000 + (selected_eprom<<12);
	/* set bank address */
	memory_set_bankptr(machine, "bank1", eprom_data);
}

void atom_eprom_box_init(running_machine *machine)
{
	/* set initial eprom */
	selected_eprom = 0;
	/* set memory handler */
	/* init */
	atom_eprom_box_refresh(machine);
}

/* write to eprom box, changes eprom selected */
WRITE8_HANDLER(atom_eprom_box_w)
{
	selected_eprom = data & 0x0f;

	atom_eprom_box_refresh(space->machine);
}

/* read from eprom box register, can this be done in the real hardware */
READ8_HANDLER(atom_eprom_box_r)
{
	return selected_eprom;
}

MACHINE_START( atomeb )
{
	MACHINE_START_CALL(atom);
	atom_eprom_box_init(machine);
}


/***************************************************************************
    MC6847 VDP
***************************************************************************/

READ8_DEVICE_HANDLER( atom_mc6847_videoram_r )
{
	mc6847_as_w(device, BIT(device->machine->generic.videoram.u8[offset], 6));
	mc6847_intext_w(device, BIT(device->machine->generic.videoram.u8[offset], 6));
	mc6847_inv_w(device, BIT(device->machine->generic.videoram.u8[offset], 7));

	return device->machine->generic.videoram.u8[offset];
}

VIDEO_UPDATE( atom )
{
	running_device *mc6847 = devtag_get_device(screen->machine, "mc6847");
	return mc6847_update(mc6847, bitmap, cliprect);
}
